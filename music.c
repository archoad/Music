#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ao/ao.h>


/*
convert mp3 to wav file:
### ffmpeg -i bach.mp3 -map_metadata -1 -acodec pcm_s16le bach.wav
### sox file.mp3 file.wav
control result by:
### exiftool bach.wav
*/


/*
Sources
http://stefaanlippens.net/audio_conversion_cheat_sheet
http://beausievers.com/synth/synthbasics/
https://en.wikipedia.org/wiki/Piano_key_frequencies
http://physics.info/music/
http://puredata.info/docs/StartHere/
http://www.lifesci.sussex.ac.uk/home/Chris_Darwin/Perception/Lecture_Notes/Hearing1/hearing1.html
http://music.columbia.edu/cmc/musicandcomputers/
https://en.wikipedia.org/wiki/Shepard_tone
http://userpages.umbc.edu/~squire/cs455_l18.html
*/

#define RATE 22050
#define LENGTH 1
#define CHANNELS 2
#define BITS_PER_SAMPLE 16
#define MAX_AMPLITUDE 32768 // 2^BITS_PER_SAMPLE / 2
#define NUM_SAMPLES (RATE*LENGTH*CHANNELS)
#define BUF_SIZE 4096
#define couleur(param) printf("\033[%sm",param)


typedef struct _wavHeader {
	//Bloc de déclaration d'un fichier au format WAVE
	char chunkID[4];
	int chunkSize;
	char waveID[4];
	// Bloc décrivant le format audio
	char fmtID[4];
	int fmtSize;
	short audioFormat;
	short nbrChannels;
	int rate;
	int bytePerSec;
	short bytePerBloc;
	short bitsPerSample;
	//Bloc des données
	char dataID[4];
	int dataSize;
} wavHeader;


static short waveForm[NUM_SAMPLES];


void usage(void) {
	couleur("31");
	printf("Michel Dubois -- music -- (c) 2015\n\n");
	couleur("0");
	printf("Syntaxe: music <action> <filename>\n");
	printf("\t<filename> -> name of the WAV file\n");
}


FILE *initiateWavFile(char *filename) {
	wavHeader header;
	FILE *file = NULL;

	strncpy(header.chunkID, "RIFF", 4);
	header.chunkSize = 0;
	strncpy(header.waveID, "WAVE", 4);

	strncpy(header.fmtID, "fmt ", 4);
	header.fmtSize = BITS_PER_SAMPLE;
	header.audioFormat = 1; // 1 for PCM
	header.nbrChannels = CHANNELS;
	header.rate = RATE;
	header.bytePerSec = RATE * CHANNELS * BITS_PER_SAMPLE/8;
	header.bytePerBloc = CHANNELS * BITS_PER_SAMPLE/8;
	header.bitsPerSample = BITS_PER_SAMPLE;

	strncpy(header.dataID, "data", 4);
	header.dataSize = 0;

	file = fopen(filename,"w+");
	fwrite(&header, sizeof(header), 1, file);
	fflush(file);
	return file;
}


double sineWave(double amplitude, double frequency, int sample, double phase) {
	double freq = 0,
		t = 0;

	t = (double)sample / (double)RATE;
	freq = 2 * M_PI * frequency * t;
	return(amplitude * sin(freq + phase));
}


double cosineWave(double amplitude, double frequency, int sample, double phase) {
	double freq = 0,
		t = 0;

	t = (double)sample / (double)RATE;
	freq = 2 * M_PI * frequency * t;
	return(amplitude * cos(freq + phase));
}


double sawtoothWave(double amplitude, double frequency, int sample, double phase, int nharmonic) {
	// On ajoute toutes les harmoniques
	int cpt = 0;
	double result = 0,
		newAmplitude = 0,
		newFrequency = 0;

	for (cpt=1; cpt<=nharmonic; cpt++) {
		newAmplitude = amplitude / (double)cpt;
		newFrequency = frequency * cpt;
		result = result - sineWave(newAmplitude, newFrequency, sample, phase);
	}
	return(result);
}


double squareWave(double amplitude, double frequency, int sample, double phase, int nharmonic) {
	// On ajoute uniquement les harmoniques impairs
	int cpt = 0;
	double result = 0,
		newAmplitude = 0,
		newFrequency = 0;

	for (cpt=1; cpt<=nharmonic; cpt+=2) {
		newAmplitude = amplitude / (double)cpt;
		newFrequency = frequency * cpt;
		result = result + sineWave(newAmplitude, newFrequency, sample, phase);
	}
	return(result);
}


double shepardWave(double amplitude, double frequency, int sample, double phase) {
	int cpt = 0;
	double newAmplitude = 0,
		omega = 0,
		t = 0,
		max = 6,
		result = 0;

	for (cpt=0; cpt<=max; cpt++) {
		t = (double)sample / (double)RATE;
		newAmplitude = amplitude * 0.5 * (1.1 - cos( cpt * 2 * M_PI / max ));
		omega = 2 * M_PI * frequency * pow(2,cpt);
		result = result + (newAmplitude * sin((omega * t) + phase));
	}
	return(result);
}


double beats(double amplitude, double frequency, int sample, double phase, double nbeats) {
	double result = 0;

	result = sineWave(amplitude, frequency, sample, phase) + sineWave(amplitude, frequency+nbeats, sample, phase);
	return(result);
}


double waveshaping(double amplitude, double frequency, int sample, double phase) {
	double x = 0,
		result = 0;

	x = sin((2 * M_PI * frequency * sample / RATE) + phase);
	result = 2*pow(x,3)-x;
	
	return(amplitude * result);
}


double aleaWave(double amplitude, double frequency, double phase) {
	int alea = 0;
	double result = 0;

	alea = rand();
	result = sineWave(amplitude, frequency, alea, phase);
	return(result);
}


void generateWaveForm(void) {
	int i = 0,
		nharmonic = 1,
		sample = 0;
	double amplitude = 0.40 * MAX_AMPLITUDE,
		frequency = 220,
		nbeats = 0,
		phase = 0,
		t = 0;

	while (i<NUM_SAMPLES) {
		waveForm[i] = (short)sineWave(amplitude, frequency, sample, 0);
		waveForm[i+1] = (short)sineWave(amplitude, frequency, sample, M_PI);

		t = sin((M_PI/8) * ((double)sample / (double)RATE));
		//frequency += sin(t)/10000;
		phase = sin( (i * 20 * M_PI) / (double)(RATE * LENGTH) );
		nbeats += 0.0001;
		if ((sample % (RATE*LENGTH/40) == 0) & (sample != 0)) { nharmonic+=1; }
		//if ((sample % (RATE*LENGTH/24) == 0) & (sample != 0)) { frequency = frequency * pow(2.0, (1.0/12.0)); }
		sample++;
		i += CHANNELS;
	}
}


void writeWavFile(FILE *file) {
	fwrite(waveForm, sizeof(short), NUM_SAMPLES, file);
	fflush(file);
}


void closeWavFile(FILE *file) {
	int fileSize = 0,
		chunkSize = 0,
		dataSize = 0;

	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);

	fseek(file, 4, SEEK_SET);
	chunkSize = fileSize - 8;
	fwrite(&chunkSize, sizeof(chunkSize), 1, file);

	fseek(file, sizeof(wavHeader) - sizeof(int), SEEK_SET);
	dataSize = fileSize - sizeof(wavHeader);
	fwrite(&dataSize, sizeof(dataSize), 1, file);

	fclose(file);
}


void createWavFile(char *filename) {
	FILE *f = NULL;

	printf("INFO: Generating wave form\n");
	generateWaveForm();
	printf("INFO: Creating WAV file %s\n", filename);
	f = initiateWavFile(filename);
	printf("INFO: Writing file\n");
	writeWavFile(f);
	printf("INFO: Closing file\n");
	closeWavFile(f);
}


void readWavFile(char *filename) {
	wavHeader *header = malloc(sizeof(wavHeader));
	FILE *file = fopen(filename, "rb");

	fread(header, sizeof(wavHeader), 1, file);
	if (strncmp(header->chunkID, "RIFF", 4) == 0) {
		printf("INFO: File:\t%s\n", filename);
		printf("\tNum channels:\t%d\n", header->nbrChannels);
		printf("\tSample rate:\t%dHz\n", header->rate);
		printf("\tBytes/seconds:\t%d\n", header->bytePerSec);
		printf("\tBytes/sample:\t%d\n", header->bitsPerSample);
		printf("\tBytes/bloc:\t%d\n", header->bytePerBloc);
		printf("\tNbr of samples:\t%d\n", header->dataSize);
	} else {
		printf("INFO: Not a WAV file\n");
	}
	fclose(file);
	free(header);
}


void playWavFile(char *filename) {
	wavHeader *header = malloc(sizeof(wavHeader));
	ao_device *device;
	ao_sample_format format;
	int driver;
	FILE *file = NULL;
	char* buffer = (char*)malloc(BUF_SIZE * sizeof(char));

	printf("INFO: playing %s\n", filename);

	file = fopen(filename, "rb");
	fread(header, sizeof(wavHeader), 1, file);

	ao_initialize();
	driver = ao_default_driver_id();
	memset(&format, 0, sizeof(format));
	format.bits = header->bitsPerSample;
	format.channels = header->nbrChannels;
	format.rate = header->rate;
	format.byte_format = AO_FMT_NATIVE;
	format.matrix = 0;
	device = ao_open_live(driver, &format, NULL);

	while (!feof(file)) {
		fread(buffer, BUF_SIZE, 1, file);
		ao_play(device, buffer, BUF_SIZE);
	}

	ao_close(device);
	ao_shutdown();
	fclose(file);
	free(buffer);
	free(header);
}


int main(int argc, char *argv[]) {
	switch (argc) {
		case 2:
			srand(time(NULL));
			createWavFile(argv[1]);
			readWavFile(argv[1]);
			playWavFile(argv[1]);
			exit(EXIT_SUCCESS);
			break;
		default:
			usage();
			exit(EXIT_FAILURE);
			break;	
	}
	return(0);
}
