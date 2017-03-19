# BRRtools

by Bregalad. Special thanks to Kode54.
Release 3.11 6th January 2014.

BRRtools are currently the most evolved tools to convert between standard RIFF .wav format and SNES's built-in BRR sound format.
They have many features never seen before in any other converter, and are open source.

Versions up to 2.1 used to be coded in Java, requiring a Java virtual machine to run.
Because this was an useless layer of abstraction which is only useful when developing, the program was rewritten to not need Java any longer.

I heavily borrowed encoding algorithms from Kode54, which himself heavily borrowed code from some other ADPCM encoder.
This is freeware, feel free to redistribute/improve but DON'T CLAIM IT IS YOUR OWN WORK THANK YOU.

## How do I use that
Just go in this folder with a terminal (mingw is required for Windows) and type "make" should normally work. No need for any libraries, or any crap, just compile and enjoy.

BRR Tools currently comes into 3 parts :

### BRR Decoder
BRR Decoder decodes a .brr sound sample to a .wav file

	Usage:
	brr_decoder [options] infile.brr outfile.wav
	
	Options:
	-n number of times to loop through the sample, default 1
	-l loop start point (in BRR block units), default 0
	-s output samplerate, default 32000
	-m minimum sample length in seconds (requires looping enabled)
	-g simulate SNES' gaussian lowpass filtering

	Example:
	brr_decoder -n19 -l128 -s16000 some_sample.brr some_sample.wav

The minimum length functionality forces a looped sample to loop so that its length is at least the specified number in seconds. If both -m and -n are enabled, the longest of the two possible lengths is used.

If you specify a loop count value >1 with the -n or -m commands, BRRDecoder will then tell you if the looping is stable, and if not it tries to detect when it gets stable.
It will also try to detect which musical note the sample plays (this, of course, depends on the sampling rate).

### SPC Decoder
SPC Decoder is similar to BRRDecoder, but decode one or multiple BRR sound samples directly from a .spc file to .wav file(s)

	Usage:
	spc_decoder [options] infile.spc outfile
	
	Options:
	-n number of times to loop through the sample(s) when applicable, default 1
	-f first sample # to decode (default : 0)
	-l last sample # to decode (default : same as first)
	-s output samplerate, default 32000
	-m minimum output length in seconds (applies only to looped samples)
	-g simulate SNES' gaussian lowpass filtering

	Example:
	spc_decoder -f 3 -l 12 -s 22050 -m 0.8 music.spc music_sample

The number of the sample and the ".wav" extention are appended at the end of the file.
In this example, the files created will be :
outfile_0.wav
outfile_1.wav
etc...

If you only specify the first sample number, SPCDecoder will only decode a single sample, but if you set both first and last samples, it will decode all samples between them.
As with BRRDecoder, info about sample's looping stability and musical note is written on the screen.

### BRR Encoder
BRR Encoder encodes a .wav file to a .brr native SNES sound sample.

	Usage:
	brr_encoder [options] infile.wav outfile.brr
	
	Options:
	-a[ampl] adjust wave amplitude by a factor ampl (default : 1.0)
	-l(pos) enable looping flag in the encoded BRR sample (default: disabled)
		If a number follows the -l flag, this is the input's loop point in samples.
	-f[0123] manually enable filters for BRR blocks (default: all enabled)
	-r[type][ratio] resample input stream, followed by resample ratio (0.0 to 4.0)
		Lower means more samples at output, better quality but increased size.
		Higher means less smaples, worse quality but decreased size.
	-s[type][rate] automatically resample to get the specified samplerate (takes priority over -r)
	-t[N] truncate the input wave to the the first N samples (ignoring any sound data that follows)
	-w disable wrapping (encoded sample will be compatible with old SPC players)
	-g enable treble boost to compensate the gaussian filtering of SNES hardware

	Resampling interpolation types:
	n : nearest neighboor (fastest)
	l : linear
	s : sine
	c : cubic
	b : bandlimited (best quality)

	Examples:
	brr_encoder -l432 -a0.8 -f01 -sc32000 in_sample.wav out_sample.brr
	brr_encoder -l -f23 -rb0.84 -t19 in_sample.wav out_sample.brr

Only .wav files of 8-bit PCM unsigned or 16-bit PCM signed are accepted. For any other sound format, use another program to convert your files to an accepted format, or even better modify the source so your format is supported.

Multi-channel input (stereo, surround or whatever) will automatically be converted to mono by averaging the channels before the encoding starts. This normally happens without problems but in some rare cases this could cause some destructive interferences and parts of the sound could "disappear".

You can select which filters are enabled or disabled. For example if you only want to use filter 1 and 2, you can do that by typing -f12.
If the first 16 samples aren't already all zeros, the encoder will add a zero block, which is necessary to initialise the decoder's history when decoding the sample.

The encoder allows you to resample your .wav before converting. This is useful so you don't need any external program to make the .wav file the desired length/sample rate.

There is currently 2 ways to do it:

* The -r command is here to resample the input by a specified ratio. For example if you use -rb1.5, the data is first resampled to be 1.5 times shorter. You can specify any real number between 0 and 4. The samplerate from the input WAV file is ignored when you use this.
* The -s command uses the input file samplerate and automatically resamples the data so that it gets the specified output sample-rate.

If -l is used alone, it just set the loop bit in the encoded BRR sample.

However if -l is followed by a number, BRREncoder will slightly resample the data so that the distance between the specified loop point and the end of the sample is a multiple of 16, therefore an integer amount of BRR blocks.

It is safe to use -s or -r in combination with -l, just keep in mind the resampling is adjusted (the value you provide becomes just a order approximation).

You can choose between several interoplation algorithms. I would really recommend to use bandlimited as it is the only algorithm to to guarantee no aliasing in case of resampling by ratio greater than 1.0 (when shortening / worsening the quality of sample) which is often needed to make the samples fit in SPC700's small 64k memory space !

## Known bugs
None known. Please don't complain if bad things (TM) happens if you do stupid things like trying to convert a 1h song in BRR and resample it to fit in 64k. The program have been tested for normal usage only, I did not feel like spending hours to guard against overflow errors in every line of code.

## Trouble shooting
If you have problem encoding your samples into BRR or that they sound somehow wrong / distorted, try to use -a0.9 (that is, reduce the amplitude to 90% of the original). Very often, overflow problems happens when resampling a normalized sample (where the entiere range is used), and reducing the amplitude slightly make it work greatly.

## Compiling BRRTools
The makefile provided is a base to compile BRR tools on both windows systems (using mingw32) and on linux systems. Just change the executable name and flags to suit your needs.

The source files makes some assumptions about type's bitsizes, you can change the typedefs in "common.h" if you are somehow compiling this with a system with different size than the typical 32/64 bit x86 PC.
It's normal there is a few warnings coming there.

## Plans for future updates
* Any suggestions are welcome
* Could someone make a nice GUI for this program ? It would be amazing ! And personally I have zero knowledge in GUI programming.
* Support for the Playstation 1 audio format as well.

## Contact
Contact me at jmasur at bluewin dot ch if you want to give ideas about how I can improve BRRTools, or even better, if you improved the program by yourself.

## History
* v1.0 november 2009
	- First public release

* v2.0 march 2012
	- BRR Decoder:
		- Decodes way faster than before.
		- Changed the command line interface
		- You can now specify any sample rate for the output

	- BRR Encoder:
		- Now supports multi-channel (stereo, surround, etc...) input (converted to mono before encoding)
		- Is now more tolerant to various .wav files
		- I removed the limit on file sizes so now it's possible to convert very long samples (such as entire songs). Now nothing prevents you to stream audio on the SNES !
		- Removed non-working gaussian interpolation (not interesting anyways)
		- Can now resample to a specified sample rate (instead of just specifying the resample factor)
		- The wrapping in the BRR encoding can be disabled.
		- Can automatically resample input so that your loop is an integer multiple of BRR blocks.

	- SPC Decoder:
		- Decodes way faster
		- Changed the command line interface
		- You can now specify any sample rate for the output
		- You have to pick the beginning of the filename for the output file(s)
		- Samples without looping are treated as so

* v2.1 march 2012
	- SPC Decoder:
		- Fixes something in the documentation
		- Added the "minimum length" functionality

* v3.0 october 2013
	- The program was ported from Java programming language to C, so that the efficiency and ease of use is increased. Only source code and win32 binaries are provided, users of different platforms will have to recompile the program by themselves. Command line interface changed (obviously).

	- BRR Decoder:
		- Now has the same "minimum length" functionality as SPCDecoder
		- Able to display which note is playing

	- BRR Encoder:
		- Bandlimited resampling is finally there! No more aliasing when resampling samples with BRR Encoder.
		- Decision of filters before looping is now "smarter" (thanks Kode64)
		- Resampling is reorganized so that it's always a rational ratio

* v3.1 november 2013
	- Now Gaussian filtering is supported both when encoding and decoding with the -g option. When decoding, a lowpass filter will be applied simulating real hardware. When decoding, a trebble boost filter is applied to compensate for the filtering done at decoding time.

* v3.11 january 2014
	- Fixed a little bug in BRR Encoder, that was causing garbage at the begining of samples when resampled with -sb.

* v3.12 decembre 2015
	- Fixed yet another typo in Gaussian filtering, which was causing a reduced volume.
