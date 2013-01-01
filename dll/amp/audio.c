/* this file is a part of amp software, (C) tomislav uzelac 1996,1997
*/

/* audio.c	main amp source file 
 *
 * Created by: tomislav uzelac	Apr 1996 
 * Karl Anders Oygard added the IRIX code, 10 Mar 1997.
 * Ilkka Karvinen fixed /dev/dsp initialization, 11 Mar 1997.
 * Lutz Vieweg added the HP/UX code, 14 Mar 1997.
 * Dan Nelson added FreeBSD modifications, 23 Mar 1997.
 * Andrew Richards complete reorganisation, new features, 25 Mar 1997
 * Edouard Lafargue added sajber jukebox support, 12 May 1997
 */ 



#include "irc.h"
#include "struct.h"
#include "ircaux.h"
#include "input.h"
#include "module.h"
#include "hook.h"
#define INIT_MODULE
#include "modval.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <sys/time.h>

#ifndef __BEOS__
#include <sys/uio.h>
#endif

#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "amp.h"
#define AUDIO
#include "audio.h"
#include "getbits.h"
#include "huffman.h"
#include "layer2.h"
#include "layer3.h"
#include "position.h"
#include "transform.h"
#include "misc2.h"

int bufferpid = 0;
unsigned long filesize = 0;
unsigned long framesize = 0;


off_t file_size (char *filename)
{
	struct stat statbuf;

	if (!stat(filename, &statbuf))
		return (off_t)(statbuf.st_size);
	else
		return -1;
}

#if 0
void statusDisplay(struct AUDIO_HEADER *header, int frameNo)
{
	return;
}

volatile int intflag = 0;

void (*catchsignal(int signum, void(*handler)()))()
{
	struct sigaction new_sa;
	struct sigaction old_sa;

	new_sa.sa_handler = handler;
	sigemptyset(&new_sa.sa_mask);
	new_sa.sa_flags = 0;
	if (sigaction(signum, &new_sa, &old_sa) == -1)
		return ((void (*)()) -1);
	return (old_sa.sa_handler);
}
#endif
        
int decodeMPEG(struct AUDIO_HEADER *header)
{
int cnt, g, snd_eof;

	/*
	 * decoder loop **********************************
	 */
	snd_eof=0;
	cnt=0;

	while (!snd_eof) 
	{
		while (!snd_eof && ready_audio()) 
		{
			if ((g=gethdr(header))!=0)
			{
				report_header_error(g);
				snd_eof=1;
				break;
                	}

			if (header->protection_bit==0) 
				getcrc();

#if 0
			statusDisplay(header,cnt);	
#endif
			if (header->layer==1) 
			{
				if (layer3_frame(header,cnt)) 
				{
					yell(" error. blip.");
					return -1;
				}
			} 
			else if (header->layer==2)
			{
				if (layer2_frame(header,cnt)) 
				{
					yell(" error. blip.");
					return -1;
				}
			}
			cnt++;
		}
	}
	return 0;
}


BUILT_IN_DLL(mp3_volume)
{
char *vol;
	if ((vol = next_arg(args, &args)))
	{
		int volume = 0;
		volume = my_atol(vol);
		if (volume > 0 && volume <= 100)
		{
			audioSetVolume(volume);
			bitchsay("Volume is now set to %d", volume);
		}
		else
			bitchsay("Volume is between 0 and 100");
	}
	else
		bitchsay("/mp3vol [1-100]");
}

BUILT_IN_DLL(mp3_play)
{
	if (args && *args)
	{
		if (!fork())
		{
			play(args);
			update_input(UPDATE_ALL);
			_exit(1);
		}
		update_input(UPDATE_ALL);
	}
	else
		bitchsay("/mp3 filename");
}

BUILT_IN_FUNCTION(func_convert_time)
{
int hours, minutes, seconds;
	if (!input)
		return m_strdup(empty_string);
	seconds = my_atol(input);
	hours = seconds / ( 60 * 60 );
	minutes = seconds / 60;
	seconds = seconds % 60;
	return m_sprintf("[%02d:%02d:%02d]", hours, minutes, seconds);
}

int Amp_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
	initialize_module("amp");

	initialise_decoder();	/* initialise decoder */
	A_QUIET = TRUE;
	AUDIO_BUFFER_SIZE=300*1024;
	A_SHOW_CNT=FALSE;
	A_SET_VOLUME=-1;
	A_SHOW_TIME=0;
	A_AUDIO_PLAY=TRUE;
	A_DOWNMIX=FALSE;
	add_module_proc(COMMAND_PROC, "Amp", "mp3", NULL, 0, 0, mp3_play, NULL);
	add_module_proc(COMMAND_PROC, "Amp", "mp3vol", NULL, 0, 0, mp3_volume, NULL);
	add_module_proc(ALIAS_PROC, "Amp", "TIMEDECODE", NULL, 0, 0, func_convert_time, NULL);
	bitchsay("Amp Module loaded. /mp3 <filename> /mp3vol <L> <R> $timedecode(seconds)");
	return 0;
}

/* call this once at the beginning
 */
void initialise_decoder(void)
{
	premultiply();
	imdct_init();
	calculate_t43();
}

/* call this before each file is played
 */
void initialise_globals(void)
{
	append=data=nch=0; 
        f_bdirty=TRUE;
        bclean_bytes=0;

	memset(s,0,sizeof s);
	memset(res,0,sizeof res);
}

void report_header_error(int err)
{
char *s = NULL;
	switch (err) {
		case GETHDR_ERR: 
			s = "error reading mpeg bitstream. exiting.";
			break;
		case GETHDR_NS : 
			s = "this is a file in MPEG 2.5 format, which is not defined" \
			    "by ISO/MPEG. It is \"a special Fraunhofer format\"." \
			    "amp does not support this format. sorry.";
			break;
		case GETHDR_FL1: 
			s = "ISO/MPEG layer 1 is not supported by amp (yet).";
			break;
		case GETHDR_FF : 
			s = "free format bitstreams are not supported. sorry.";
			break;	
		case GETHDR_SYN: 
			s = "oops, we're out of sync.";
			break;
		case GETHDR_EOF: 
		default: 		; /* some stupid compilers need the semicolon */
	}	
	if (s)
		do_hook(MODULE_LIST, "AMP ERROR blip %s", s);
}

/* TODO: there must be a check here to see if the audio device has been opened
 * successfuly. This is a bitch because it requires all 6 or 7 OS-specific functions
 * to be changed. Is anyone willing to do this at all???
 */
int setup_audio(struct AUDIO_HEADER *header)
{
	if (A_AUDIO_PLAY)  
	{
		if (AUDIO_BUFFER_SIZE==0)
			audioOpen(t_sampling_frequency[header->ID][header->sampling_frequency],
					(header->mode!=3 && !A_DOWNMIX),A_SET_VOLUME);
		else
			bufferpid = audioBufferOpen(t_sampling_frequency[header->ID][header->sampling_frequency],
					(header->mode!=3 && !A_DOWNMIX),A_SET_VOLUME);
	}
	return 0;
}

void close_audio(void)
{
	if (A_AUDIO_PLAY)
	{
		if (AUDIO_BUFFER_SIZE!=0)
			audioBufferClose();
		else
			audioClose();
	}
}

int ready_audio(void)
{
	return 1;
}

/* remove the trailing spaces from a string */
static void strunpad(char *str)
{
	int i = strlen(str);

	while ((i > 0) && (str[i-1] == ' '))
		i--;
	str[i] = 0;
}
                    
static void print_id3_tag(FILE *fp, char *buf)
{
	struct id3tag {
		char tag[3];
		char title[30];
		char artist[30];
		char album[30];
		char year[4];
		char comment[30];
		unsigned char genre;
	};
	struct idxtag {
		char tag[3];
		char title[90];
		char artist[50];
		char album[50];
		char comment[50];
	};
	struct id3tag *tag = (struct id3tag *) buf;
	struct idxtag *xtag = (struct idxtag *) buf;
	char title[121]="\0";
	char artist[81]="\0";
	char album[81]="\0";
	char year[5]="\0";
	char comment[81]="\0";

	strncpy(title,tag->title,30);
	strncpy(artist,tag->artist,30);
	strncpy(album,tag->album,30);
	strncpy(year,tag->year,4);
	strncpy(comment,tag->comment,30);
	strunpad(title);
	strunpad(artist);
	strunpad(album);
	strunpad(comment);
	
	if ((fseek(fp, 384, SEEK_END) != -1) && (fread(buf, 256, 1, fp) == 1))
	{
		if (!strncmp(buf, "TXG", 3))
		{
			strncat(title, xtag->title, 90);
			strncat(artist, xtag->artist, 50);
			strncat(album, xtag->album, 50);
			strncat(comment, xtag->comment, 50);
			strunpad(title);
			strunpad(artist);
			strunpad(album);
			strunpad(comment);
		}
	}
	if (!do_hook(MODULE_LIST, "AMP ID3 \"%s\" \"%s\" \"%s\" %s %d %s", title, artist, album, year, tag->genre, comment))
	{
		bitchsay("Title  : %.120s  Artist: %s",title, artist);
		bitchsay("Album  : %.80s  Year: %4s, Genre: %d",album, year, (int)tag->genre);
		bitchsay("Comment: %.80s",comment);
	}
}


/* 
 * TODO: add some kind of error reporting here
 */
void play(char *inFileStr)
{
char *f;
long totalframes = 0;
long tseconds = 0;
struct AUDIO_HEADER header;
int bitrate, fs, g, cnt = 0;

	while ((f = new_next_arg(inFileStr, &inFileStr)))
	{
		if (!f || !*f)
			return;	
		if ((in_file=fopen(f,"r"))==NULL) 
		{
			if (!do_hook(MODULE_LIST, "AMP ERROR open %s", f))
				put_it("Could not open file: %s\n", f);
			continue;
		}



		filesize = file_size(f);
		initialise_globals();

		if ((g=gethdr(&header))!=0) 
		{
			report_header_error(g);
			continue;
		}

		if (header.protection_bit==0) 
			getcrc();

		if (setup_audio(&header)!=0) 
		{
			yell("Cannot set up audio. Exiting");
			continue;
		}
	
		filesize -= sizeof(header);

		switch (header.layer)
		{
			case 1:
			{
				if (layer3_frame(&header,cnt)) 
				{
					yell(" error. blip.");
					continue;
				}
				break;
			} 
			case 2:
			{
				if (layer2_frame(&header,cnt)) 
				{
					yell(" error. blip.");
					continue;
				}
				break;
			}
			default:
				continue;
		}

		bitrate=t_bitrate[header.ID][3-header.layer][header.bitrate_index];
	       	fs=t_sampling_frequency[header.ID][header.sampling_frequency];

	        if (header.ID) 
        		framesize=144000*bitrate/fs;
	       	else 
       			framesize=72000*bitrate/fs;



		totalframes = (filesize / (framesize + 1)) - 1;
		tseconds = (totalframes * 1152/
		    t_sampling_frequency[header.ID][header.sampling_frequency]);
                
		if (A_AUDIO_PLAY)
		{
			char *p = strrchr(f, '/');
			if (!p) p = f; else p++;
			if (!do_hook(MODULE_LIST, "AMP PLAY %lu %lu %s", tseconds, filesize, p))
				bitchsay("Playing: %s\n", p);
		}

		/*
		 * 
		 */
		if (!(fseek(in_file, 0, SEEK_END)))
		{
			char id3_tag[256];
			if (!fseek(in_file, -128, SEEK_END) && (fread(id3_tag,128, 1, in_file) == 1))
			{
				if (!strncmp(id3_tag, "TAG", 3))
					print_id3_tag(in_file, id3_tag);
			}
			fseek(in_file,0,SEEK_SET);
		}
		decodeMPEG(&header);
		do_hook(MODULE_LIST, "AMP CLOSE %s", f);
		close_audio();
		fclose(in_file);
	}
}

