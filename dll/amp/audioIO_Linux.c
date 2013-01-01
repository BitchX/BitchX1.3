/* this file is a part of amp software, (C) tomislav uzelac 1996,1997

	Origional code by: tomislav uzelac
	Modified by:
	* Dan Nelson - BSD mods.
	* Andrew Richards - moved code from audio.c and added mixer support etc

 */

/* Support for Linux and BSD sound devices */

#include "amp.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "audioIO.h"

#ifdef HAVE_MACHINE_SOUNDCARD_H
#include <machine/soundcard.h>
#elif defined(HAVE_LINUX_SOUNDCARD_H)
#include <linux/soundcard.h>
#elif defined(HAVE_SYS_SOUNDCARD_H)
#include <sys/soundcard.h>
#else
#define NO_SOUNCARD_H
#endif

#ifndef NO_SOUNCARD_H
/* optimal fragment size */

int AUSIZ = 0;

/* declare these static to effectively isolate the audio device */

static int audio_fd;
static int mixer_fd;
static int volumeIoctl;

/* audioOpen() */
/* should open the audio device, perform any special initialization		 */
/* Set the frequency, no of channels and volume. Volume is only set if */
/* it is not -1 */

void
audioOpen(int frequency, int stereo, int volume)
{
	int supportedMixers, play_format=AFMT_S16_LE;

	if ((audio_fd = open ("/dev/dsp", O_WRONLY, 0)) == -1)
		die("Unable to open the audio device\n");

	if (ioctl(audio_fd, SNDCTL_DSP_SETFMT,&play_format) < 0)
		die("Unable to set required audio format\n");
	if ((mixer_fd=open("/dev/mixer",O_RDWR)) == -1)
		warn("Unable to open mixer device\n");

	if (ioctl(mixer_fd, SOUND_MIXER_READ_DEVMASK, &supportedMixers) == -1) {
		warn("Unable to get mixer info assuming master volume\n");
		volumeIoctl=SOUND_MIXER_WRITE_VOLUME;
	} else {
		if ((supportedMixers & SOUND_MASK_PCM) != 0)
			volumeIoctl=SOUND_MIXER_WRITE_PCM;
		else
			volumeIoctl=0;
	}

	/* Set 1 or 2 channels */
	stereo=(stereo ? 1 : 0);

	if (ioctl(audio_fd, SNDCTL_DSP_STEREO, &stereo) < 0)
		die("Unable to set stereo/mono\n");

	/* Set the output frequency */
	if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &frequency) < 0)
		die("Unable to set frequency: %d\n",frequency);

	if (volume != -1)
		audioSetVolume(volume);

	if (ioctl(audio_fd, SNDCTL_DSP_GETBLKSIZE, &AUSIZ) == -1)
		die("Unable to get fragment size\n");
}


/* audioSetVolume - only code this if your system can change the volume while */
/*									playing. sets the output volume 0-100 */

void
audioSetVolume(int volume)
{

	volume=(volume<<8)+volume;
	if ((mixer_fd != -1) && (volumeIoctl!=0))
		if (ioctl(mixer_fd, volumeIoctl, &volume) < 0)
			warn("Unable to set sound volume\n");
}


/* audioFlush() */
/* should flush the audio device */

inline void
audioFlush()
{

	if (ioctl(audio_fd, SNDCTL_DSP_RESET, 0) == -1)
		die("Unable to reset audio device\n");
}


/* audioClose() */
/* should close the audio device and perform any special shutdown */

void
audioClose()
{
	close(audio_fd);
	if (mixer_fd != -1)
		close(mixer_fd);
}


/* audioWrite */
/* writes count bytes from buffer to the audio device */
/* returns the number of bytes actually written */

inline int
audioWrite(char *buffer, int count)
{
	return(write(audio_fd,buffer,count));
}


/* Let buffer.c have the audio descriptor so it can select on it. This means	*/
/* that the program is dependent on a file descriptor to work. Should really */
/* move the select's etc (with inlines of course) in here so that this is the */
/* ONLY file which has hardware dependent audio stuff in it										*/

int
getAudioFd()
{
	return(audio_fd);
}
#endif
