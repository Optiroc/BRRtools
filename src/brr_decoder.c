#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>
#include <stdbool.h>
#include "common.h"
#include "brr.h"

static void print_instructions()
{
	printf(
		"\n*** BRR Tools 3.0 ***\n\n"
		"BRR decoder (c) 2013 Bregalad special thanks to Kode54\n"
		"Usage : brr_decoder [options] infile.brr outfile.wav\n"
		"Options :\n"
		"-n number of times to loop through the sample, default 1\n"
		"-l loop start point (in BRR block units), default 0\n"
		"-s output samplerate, default 32000\n"
		"-m minimum sample length in seconds (requires looping enabled)\n"
		"-g simulate SNES' gaussian lowpass filtering\n"
		"\nExample : brr_decoder -n19 -l128 -s16000 some_sample.brr some_sample.wav\n"
	);
	exit(1);
}

int main(const int argc, char *const argv[])
{
	unsigned int looppos = 0, loopcount = 1, samplerate = 32000;
	int size;
	double min_length = 0.0;
	bool gaussian_lowpass = false;

	int c;
	while ((c = getopt(argc, argv, "l:n:s:m:g")) != -1)
	{
		int arg = atoi(optarg);
		switch(c)
		{
			case 'l':
				looppos = arg;
				break;

			case 'n':
				loopcount = arg;
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

	char *inbrr_path = argv[optind];
	char *outwav_path = argv[optind+1];
	// Try to open input BRR file
	FILE *inbrr = fopen(inbrr_path, "rb");
	if(!inbrr)
	{
		fprintf(stderr, "No such file : %s\n", inbrr_path);
		exit(1);
	}

	// Get the size of the input BRR file
	fseek(inbrr, 0L, SEEK_END);
	size = ftell(inbrr);
	fseek(inbrr, 0L, SEEK_SET);
	// Size should be an integer multiple of 9
	if(size%9 != 0)
	{
		fprintf(stderr, "Error : Size of BRR file %s isn't a multiple of 9 bytes.\n", inbrr_path);
		exit(1);
	}

	int blockamount = size/9;
	printf("Number of BRR blocks to decode : %d.\n", blockamount);

	if(looppos<0 || looppos>=blockamount)  								//Make sure the loop position is in range
	{
		fprintf(stderr, "Error : Loop position is out of range\n");
		exit(1);
	}

	//Implement the "minimum length" function
	int min_len_samples = (int)ceil(min_length*samplerate/16.0);
	loopcount = MAX((signed)loopcount, (min_len_samples-(signed)looppos)/(blockamount-(signed)looppos));

	pcm_t olds0[loopcount];
	pcm_t olds1[loopcount];			//Tables to remember value of p1, p2 when looping

	//Create sample buffer
	unsigned int out_blocks = loopcount*(blockamount-looppos)+looppos;
	pcm_t *samples = safe_malloc(out_blocks * 32);

	fseek(inbrr, 0, SEEK_SET);				//Start to read at the beginning of the file
	pcm_t *buf_ptr = samples;

	for(int i=0; i<looppos; ++i) 		//Read the start of the sample before loop point
	{
		fread(BRR, 1, 9, inbrr);
		decodeBRR(buf_ptr);					//Append 16 BRR samples to existing array
		buf_ptr += 16;
	}
	for(int j=0; j<loopcount; ++j)
	{
		fseek(inbrr, looppos*9, SEEK_SET);
		for(int i=looppos; i<blockamount; ++i)
		{
			fread(BRR, 1, 9, inbrr);
			decodeBRR(buf_ptr);			//Append 16 BRR samples to existing array
			if(i == looppos)
			{							//Save the p1 and p2 values on each loop point encounter
				olds0[j] = buf_ptr[0];
				olds1[j] = buf_ptr[1];
			}
			buf_ptr += 16;
		}
	}

	if(loopcount > 1) print_note_info(blockamount - looppos, samplerate);
	print_loop_info(loopcount, olds0, olds1);
	fclose(inbrr);

	// Try to open output WAV file
	FILE *outwav = fopen(outwav_path, "wb");
	if(!outwav)
	{
		fprintf(stderr, "Can't open file %s for writing.\n", outwav_path);
		exit(1);
	}

	// Lowpass filter data to simulate real SNES hardware
	if(gaussian_lowpass) apply_gauss_filter(samples, out_blocks * 16);

	generate_wave_file(outwav, samplerate, samples, out_blocks);

	fclose(outwav);
	free(samples);
	printf("Done !\n");
	return 0;		// Exit without error
}
