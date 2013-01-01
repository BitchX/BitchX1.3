#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#elif defined(HAVE_LINUX_SOUNDCARD_H)
#include <linux/soundcard.h>
#elif defined(HAVE_SYS_SOUNDCARD_H)
#include <sys/soundcard.h>
#else
#define NO_SOUNDCARD_H
#endif
#ifndef NO_SOUNDCARD_H
static int dspfd = -1;
#define WAV_RIFF_MAGIC	"RIFF"
#define WAV_WAVE_MAGIC	"WAVE"
#define WAV_FMT_MAGIC	"fmt "
#define WAV_DATA_MAGIC	"data"

typedef struct {
	unsigned char riffID[4];
	unsigned long riffsize;
	unsigned char waveID[4];
	unsigned char fmtID[4];
	unsigned long fmtsize;
	unsigned short w_formattag;
	unsigned short n_channels;
	unsigned long n_samples;
	unsigned long avg_bytes;
	unsigned short n_blockalign;
	unsigned short w_bitssample;
	unsigned char dataID[4];
	unsigned long n_datalen;
} Wave_Header;

char *validate_wav_header(char *header)
{
Wave_Header *w = (Wave_Header *)header;

	if (strncmp(w->riffID, WAV_RIFF_MAGIC, 4))
		return NULL;

	if (strncmp(w->waveID, WAV_WAVE_MAGIC, 4))
		return NULL;

	if (strncmp(w->fmtID, WAV_FMT_MAGIC, 4))
		return NULL;

	if (w->fmtsize != 16)
		return NULL;

#if 0
	if (w->w_formattag != 1)
		return NULL;

	if (w->n_channels != 2 && w->n_channels != 1)
		return NULL;
#endif

	if (strncmp(w->dataID, WAV_DATA_MAGIC, 4))
		return(NULL);
	return (header + sizeof(Wave_Header));
}


int open_dsp(Wave_Header *wav_info)
{
int arg;

	if ((dspfd = open("/dev/dsp",O_WRONLY)) < 0)
		return -1;
	arg = wav_info->w_bitssample;
	if ((ioctl(dspfd, SOUND_PCM_WRITE_BITS, &arg)) == -1)
		return -1;

	arg = wav_info->n_channels;  /* mono or stereo */
	if ((ioctl(dspfd, SOUND_PCM_WRITE_CHANNELS, &arg)) == -1)
		return -1;

	arg = wav_info->n_samples;      /* sampling rate */
	if ((ioctl(dspfd, SOUND_PCM_WRITE_RATE, &arg)) == -1)
		return -1;
	return dspfd;
}

int play_buffer(int dspfd, short *start, short *end)
{
int status;

	status = write(dspfd,(char *)start,(char *)end - (char *)start); 

	if (status != (char *)end - (char *)start)
		return -1;
	return 0;
}

void wave_play_file(int wavfd, int dspfd, short *current, short *audioend, int interval)
{
	while (current < audioend)
	{
		short *endcurrent = current + interval;

		if (endcurrent > audioend)
			endcurrent = audioend;
		if (play_buffer(dspfd,current,endcurrent) == -1)
			endcurrent = audioend;
		current = endcurrent;
	}
}

BUILT_IN_DLL(wav_play)
{
	char *filename = NULL;

	if (dspfd != -1)
	{
		put_it("Already playing a .wav file");
		return;
	}
        if ((filename = next_arg(args, &args)))
	{
		char *inwavbuf;
		short *current;
		short *audioend;
		short *audio;
		Wave_Header *wav_info;
		int wavfd;
		struct stat input_fstat;
		size_t interval;

		if ((wavfd = open(filename,O_RDONLY)) == -1)
		{
			put_it("errno %s", strerror(errno));
			return;
		}
		if (fstat(wavfd,&input_fstat) != 0)
			return;
		if (input_fstat.st_size < sizeof(Wave_Header))
			return;
		if (!(inwavbuf = mmap(NULL, input_fstat.st_size, PROT_READ, MAP_SHARED, wavfd, 0)))
			return;

		if (!(audio = (short *)validate_wav_header(inwavbuf)))
		{
			put_it("Invalid wav file");
			return;
		}
		current = audio;
		wav_info = (Wave_Header *)inwavbuf;
		audioend =  (short *)((char *)audio + wav_info->n_datalen);

		if ((dspfd = open_dsp(wav_info)) == -1)
		{
			close(wavfd);
			munmap(inwavbuf, input_fstat.st_size);
			return;
		}
		interval = (size_t )((double )wav_info->n_samples * 0.1 * 2);
		if (!fork())
		{
			wave_play_file(wavfd, dspfd, current, audioend, interval);
			munmap(inwavbuf,input_fstat.st_size);
			close(wavfd);               
			close(dspfd);
			dspfd = -1;
			_exit(1);
		}
		munmap(inwavbuf,input_fstat.st_size);
		close(wavfd);               
		close(dspfd);
		dspfd = -1;
	}
}



int Wavplay_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
	initialize_module("wavplay");
	add_module_proc(COMMAND_PROC, "Wavplay", "wavplay", NULL, 0, 0, wav_play, NULL);
	bitchsay("Wavplay Module loaded. /wavplay <filename>");
	return 0;
}
#endif
