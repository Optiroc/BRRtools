#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include "common.h"
#include "brr.h"

static void print_instructions()
{
	printf(
		"\n*** BRR Tools 3.0 ***\n\n"
		"SPC decoder (c) 2013 Bregalad special thanks to Kode54\n"
		"Usage : spc_decoder [options] infile.spc outfile\n"
		"Options :\n"
		"-n number of times to loop through the sample(s) when applicable, default 1\n"
		"-f first sample # to decode (default : 0)\n"
		"-l last sample # to decode (default : same as first)\n"
		"-s output samplerate, default 32000\n"
		"-m minimum output length in seconds (applies only to looped samples)\n"
		"-g simulate SNES' gaussian lowpass filtering\n"
		"\nExample : spc_decoder -f3 -l12 -s22050 -m0.8 music.spc music_sample\n"
	);
	exit(1);
}

int main(const int argc, char *const argv[])
{
	int looppos=0, loopcount=1, firstsample=0, lastsample=0, samplerate = 32000, looptimes;
	double min_length = 0.0;
	bool gaussian_lowpass = false;

	u16 sample_adr = 0, loop_adr = 0;
	pcm_t *samples;

	int c;
	while ((c = getopt(argc, argv, "n:f:l:s:m:g")) != -1)
	{
		int arg = atoi(optarg);
		switch(c)
		{
			case 'n':
				loopcount = arg;
				break;

			case 'f':
				firstsample = arg;
				break;

			case 'l':
				lastsample = arg;
				break;

			case 's':
				samplerate = arg;
				break;

			case 'm':
				min_length = atof(optarg);
				break;

			case 'g':
				gaussian_lowpass = true;
				break;

			default:
				printf("Invalid command line syntax !\n");
				print_instructions();
		}
	}

	if(argc - optind != 2) print_instructions();

	char *inspc_path = argv[optind];
	char *outwav_path = argv[optind+1];
	// Try to open input BRR file
	FILE *inspc = fopen(inspc_path, "rb");
	if(!inspc)
	{
		fprintf(stderr, "No such file : %s\n", inspc_path);
		exit(1);
	}

	// Array to store the name of the opened output WAV file
	size_t outwav_name_len = strlen(outwav_path) + 9;
	char *outwav_name = safe_malloc(outwav_name_len);

	fseek(inspc, 0x1015d, SEEK_SET);								//Sample location value in DSP
	unsigned int sample_loc = (fgetc(inspc)<<8)+0x100;				//Location of sample table in file

	unsigned int sample = firstsample;
	do
	{
		// Create output file name
		snprintf(outwav_name, outwav_name_len, "%s_%d.wav", outwav_path, sample);
		// Create output file
		FILE *outwav = fopen(outwav_name, "wb");
		if(!outwav)
		{
			fprintf(stderr, "Error : Can't open %s for writing.", outwav_path);
			continue;
		}

		fseek(inspc, sample_loc + 4*sample, SEEK_SET);				//Read sample number
		fread(&sample_adr, 2, 1, inspc);
		fread(&loop_adr, 2, 1, inspc);

		if((loop_adr-sample_adr) % 9)
		{
			fprintf(stderr, "garbage\n");
			continue;
		}
		looppos=(loop_adr-sample_adr)/9;							//Number of BRR blocks before loop

		int flags, blockamount = 0;
		do
		{	//Compute the number of block before end of sample
			fseek(inspc, sample_adr + 9*blockamount + 0x100, SEEK_SET);
			blockamount++;
			flags = fgetc(inspc);
		}
		while(!(flags&0x01));

		//Check if the sample has looping enabled
		if(flags & 0x02)  							//Samples with loop enabled
		{
			printf("\n* Sample #%d Length : %d Loop : %d.", sample, blockamount, looppos);

			//Implement the "minimum length" function
			int min_len_samples = (int)ceil(min_length*samplerate/16.0);
			looptimes = MAX(loopcount, (signed)(min_len_samples-looppos)/(blockamount-looppos));

			//Tables to remember value of p1, p2 when looping
			pcm_t olds0[looptimes];
			pcm_t olds1[looptimes];

			//Create samples buffer
			unsigned int out_blocks = looptimes*(blockamount-looppos)+looppos;
			samples = safe_malloc(out_blocks * 32);
			pcm_t *buf_ptr = samples;

			//Read the start of the sample before loop point
			fseek(inspc, sample_adr + 0x100, SEEK_SET);
			for(int i=0; i<looppos; ++i)
			{
				fread(BRR, 1, 9, inspc);			//Read a BRR block
				decodeBRR(buf_ptr);					//Append 16 BRR samples to existing array
				buf_ptr += 16;
			}

			for(int j=0; j<looptimes; ++j)
			{
				fseek(inspc, sample_adr + 9*looppos + 0x100, SEEK_SET);
				for(int i=looppos; i<blockamount; ++i)
				{
					fread(BRR, 1, 9, inspc);
					decodeBRR(buf_ptr);
					if(i == looppos)
					{
						olds0[j] = buf_ptr[0];	//Save the sample0 and 1 values on each loop point encounter
						olds1[j] = buf_ptr[1];
					}
					buf_ptr += 16;				//Append 16 BRR samples to existing array
				}
			}
			print_note_info(blockamount-looppos, samplerate);
			print_loop_info(looptimes, olds0, olds1);

			if(gaussian_lowpass) apply_gauss_filter(samples, out_blocks * 16);

			generate_wave_file(outwav, samplerate, samples, out_blocks);
		}
		else 											//Samples with loop disabled
		{
			printf("\n* Sample #%d Lenght : %d Loop : disabled\n", sample, blockamount);

			samples = safe_malloc(blockamount * 32);
			pcm_t *buf_ptr = samples;

			fseek(inspc, sample_adr + 0x100, SEEK_SET);
			for(int k=0; k<blockamount; k++)
			{
				fread(BRR, 1, 9, inspc);
				decodeBRR(buf_ptr);
				buf_ptr += 16;				//Append 16 BRR samples to existing array
			}

			if(gaussian_lowpass) apply_gauss_filter(samples, blockamount * 16);

			generate_wave_file(outwav, samplerate, samples, blockamount);
		}

		fclose(outwav);
		free(samples);
	}
	while(++sample <= lastsample);

	free(outwav_name);
	fclose(inspc);
	return 0;
}
