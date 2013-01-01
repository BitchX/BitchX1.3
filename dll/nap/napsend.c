
#define IN_MODULE
#include "irc.h"
#include "struct.h"
#include "dcc.h"
#include "ircaux.h"
#include "ctcp.h"
#include "cdcc.h"
#include "input.h"
#include "status.h"
#include "lastlog.h"
#include "screen.h"
#include "vars.h" 
#include "misc.h"
#include "output.h"
#include "module.h"
#include "hook.h"
#include "hash2.h"

#include "bsdglob.h"
#include "modval.h"
#include "napster.h"
#include "md5.h"

#include <sys/time.h>
#include <sys/stat.h>
#if !defined(WINNT) || !defined(__EMX__)
#include <sys/mman.h>
#endif

#define DEFAULT_FILEMASK "*.mp3"

#ifndef MAP_FAILED
#define MAP_FAILED (void *) -1
#endif

Stats statistics = { 0, };


#define MP3_ONLY 0
#define ANY_FILE -1
#define VIDEO_ONLY 1
#define IMAGE_ONLY 2


static int current_sending = 0;

GetFile *napster_sendqueue = NULL;

#define MODE_STEREO		0
#define MODE_JOINT_STEREO	1
#define MODE_DUAL_CHANNEL	2
#define MODE_MONO		3

#define DEFAULT_MD5_SIZE 292 * 1024
Files *fserv_files = NULL;

char *mime_string[] = { "audio/", "image/", "video/", "application/", "text/", "" }; 
char *mime_type[] = {	"x-wav", "x-aiff", "x-midi", "x-mod", "x-mp3", /* 0-4 */
			"gif", "jpeg", /* 5-6 */
			"mpeg",  /* 7 */
			"UNIX Compressed", "x-Compressed", "x-bzip2", "x-Zip File", /* 8-11 */
			"x-executable", /* 9 */
			"plain", ""}; /* 10 */
char *audio[] = {".wav", ".aiff", ".mid", ".mod", ".mp3", ""};
char *image[] = {".jpg", ".gif", ""};
char *video[] = {".mpg", ".dat", ""};
char *application[] = {".tar.gz" ".tar.Z", ".Z", ".gz", ".arc", ".bz2", ".zip", ""};

char *find_mime_type(char *fn)
{
static char mime_str[100];
	if (fn)
	{
		int i;
		int type = 4;
		int mim = 10;
		if (!end_strcmp(fn, ".exe", 4))
		{
			type = 3;
			mim = 9;
			sprintf(mime_str, "%s%s", mime_string[type], mime_type[mim]);
			return mime_str;
		}
		for (i = 0; *audio[i]; i++)
		{
			if (!end_strcmp(fn, audio[i], strlen(audio[i])))
			{
				type = 0;
				mim = i;
				sprintf(mime_str, "%s%s", mime_string[type], mime_type[mim]);
				return mime_str;
			}
		}
		for (i = 0; *image[i]; i++)
		{
			if (!end_strcmp(fn, image[i], strlen(image[i])))
			{
				type = 1;
				mim = i + 5;
				sprintf(mime_str, "%s%s", mime_string[type], mime_type[mim]);
				return mime_str;
			}
		}
		for (i = 0; *video[i]; i++)
		{
			if (!end_strcmp(fn, video[i], strlen(video[i])))
			{
				type = 2;
				mim = 7;
				sprintf(mime_str, "%s%s", mime_string[type], mime_type[mim]);
				return mime_str;
			}
		}
		for (i = 0; *application[i]; i++)
		{
			if (!end_strcmp(fn, application[i], strlen(application[i]))) 
			{
				type = 3;
				switch(i)
				{
					case 0:
					case 1:
					case 2:
					case 3:
						mim = 8;
						break;
					case 4:
						mim = 9;
						break;
					case 5:
						mim = 10;
						break;
					case 6:
						mim = 11;
						break;
				}
				sprintf(mime_str, "%s%s", mime_string[type], mime_type[mim]);
				return mime_str;
			}
		}
		sprintf(mime_str, "%s%s", mime_string[type], mime_type[mim]);
		return mime_str;
	}
	return NULL;	
}

void clear_files(Files **f)
{
Files *last, *f1 = *f;
	while (f1)
	{
		last = f1->next;
		new_free(&f1->filename);
		new_free(&f1->checksum);
		new_free(&f1);
		f1 = last;
	}
	*f = NULL;
}

static char *convertnap_dos(char *str)
{
register char *p;
	for (p = str; *p; p++)
		if (*p == '/')
			*p = '\\';
	return str;
}

static char *convertnap_unix(char *arg)
{
register char *x = arg;
	while (*x)
	{
		if (*x == '\\')
			*x = '/';
		x++;
	}
	return arg;
}


char *mode_str(int mode)
{
	switch(mode)
	{	
		case 0:
			return "St";
		case 1:
			return "JS";
		case 2:
			return "DC";
		case 3:
			return "M";
	}
	return empty_string;
}

char *print_time(time_t input)
{
	static char	buff[40];
	time_t		seconds,
			minutes;
	seconds = input;
	minutes = seconds / 60;
	seconds = seconds % 60;
	sprintf(buff, "%02u:%02u", (unsigned int)minutes, (unsigned int)seconds);
	return buff;
}                                        

char *make_mp3_string(FILE *fp, Files *f, char *fs, char *dirbuff)
{
	static char	buffer[2*NAP_BUFFER_SIZE+1];
	char		*s,
			*loc,
			*p,
			*fn;

	if (!fs || !*fs)
		return empty_string;
	memset(buffer, 0, sizeof(buffer));

	loc = LOCAL_COPY(f->filename);
	fn = strrchr(loc, '/');
	*fn++ = 0;
	if ((p = strrchr(loc, '/')))
		*p++ = 0;
	/* fn should point to the filename and p to the dir */
	/* 
	 * init the dir keeper 
	 * or cmp the old dir with the new
	 */
	if (dirbuff && (!*dirbuff || strcmp(dirbuff, p)))
	{
		strcpy(dirbuff, p);
		if (fp)
			fprintf(fp, "\nDirectory [ %s ]\n", dirbuff);
		else
			return NULL;
	}
	/* size bitrate [time] filename */
	s = buffer;
	while (*fs)
	{
		if (*fs == '%')
		{
			int prec = 0, fl = 0;
			fs++;
			if (isdigit(*fs))
			{
				prec = strtol(fs, &fs, 0);
				if (*fs == '.')
					fl = strtoul(fs+1, &fs, 0);
			}
			switch(*fs)
			{
				case '%':
					*s++ = *fs;
					break;
				case 'b':
					sprintf(s, "%*u", prec, f->bitrate);
					break;
				case 's':
					if (!prec) prec = 3;
					sprintf(s, "%*.*f%s", prec, fl, _GMKv(f->filesize), _GMKs(f->filesize));
					break;
				case 't':
					strcpy(s, print_time(f->time));
					break;
				case 'T':
					strcpy(s, ltoa(f->time));
					break;
				case 'f':
					strcpy(s, fn);
					break;
				case 'F':
					strcpy(s, f->filename);
					break;
				case 'M':
					strcpy(s, f->checksum);
					break;
				case 'S':
					strcpy(s, mode_str(f->stereo));
					break;
				case 'H':
					sprintf(s, "%*.*f", prec, fl, ((double)f->freq) / ((double)1000.0));
					break;
				case 'h':
					sprintf(s, "%*u", prec, f->freq);
					break;
				default:
					*s++ = *fs;
					break;
			}
		}
		else if (*fs == '\\')
		{
			fs++;
			switch(*fs)
			{
				case 'n':
					strcpy(s, "\n");
					break;
				case 't':
					strcpy(s, "\t");
					break;
				default:
					*s++ = *fs++;
			}
		}
		else
			*s++ = *fs;
		while (*s) s++;
		fs++;
	}
	if (fp && *buffer)
		fprintf(fp, buffer);
	return buffer;
}


int read_glob_dir(char *path, int globflags, glob_t *globpat, int recurse)
{
	char	buffer[NAP_BUFFER_SIZE+1];
	
	sprintf(buffer, "%s/*", path);
	bsd_glob(buffer, globflags, NULL, globpat);
	if (recurse)
	{
		int i = 0;
		int old_glpathc = globpat->gl_pathc;
		for (i = 0; i < old_glpathc; i++)
		{
			char *fn;
			fn = globpat->gl_pathv[i];
			if (fn[strlen(fn)-1] != '/')
				continue;
			sprintf(buffer, "%s*", fn);
			bsd_glob(buffer, globflags|GLOB_APPEND, NULL, globpat);
		}
		while (i < globpat->gl_pathc)
		{
			for (i = old_glpathc, old_glpathc = globpat->gl_pathc; i < old_glpathc; i++)
			{
				char *fn;
				fn = globpat->gl_pathv[i];
				if (fn[strlen(fn)-1] != '/')
					continue;
				sprintf(buffer, "%s*", fn);
				bsd_glob(buffer, globflags|GLOB_APPEND, NULL, globpat);
			}
		}
	}
	return 0;
}

unsigned int print_mp3(char *pattern, char *format, int freq, int number, int bitrate, int md5)
{
unsigned int count = 0;
Files *new;
char dir[NAP_BUFFER_SIZE];
char *fs = NULL;
	*dir = 0;
	for (new = fserv_files; new; new = new->next)
	{
		if (!pattern || (pattern && wild_match(pattern, new->filename)))
		{
			char *p;
			p = base_name(new->filename);
			if ((bitrate != -1) && (new->bitrate != bitrate))
				continue;
			if ((freq != -1) && (new->freq != freq))
				continue;
			if (do_hook(MODULE_LIST, "NAP MATCH %s %s %u %lu", p, new->checksum, new->bitrate, new->time))
			{
				if (!format || !*format)
				{
					if (md5)
						put_it("\"%s\" %s %dk [%s]", p, new->checksum, new->bitrate, print_time(new->time));
					else
						put_it("\"%s\" %s %dk [%s]", p, mode_str(new->stereo), new->bitrate, print_time(new->time));
				}
				else
				{
					if ((fs = make_mp3_string(NULL, new, format, dir)))
						put_it("%s", fs);
					else
						put_it("%s", make_mp3_string(NULL, new, format, dir));
				}
			}
		}
		if ((number > 0) && (count == number))
			break;
		count++;
	}
	return count;
}

BUILT_IN_DLL(print_napster)
{
	int	count = 0;
	int	bitrate = -1;
	int	number = -1;
	int	freq = -1;
	int	md5 = 0;
	char 	*fs_output = NULL;
	char	*tmp_pat = NULL;
	
	if ((get_dllstring_var("napster_format")))
		fs_output = m_strdup(get_dllstring_var("napster_format"));
	if (args && *args)
	{
		char *tmp;
		while ((tmp = next_arg(args, &args)) && *tmp)
		{
			int len;
			len = strlen(tmp);
			if (!my_strnicmp(tmp, "-BITRATE", len))
			{
				if ((tmp = next_arg(args, &args)))
					bitrate = (unsigned int) my_atol(tmp);
			}
			else if (!my_strnicmp(tmp, "-COUNT", len))
			{
				if ((tmp = next_arg(args, &args)))
					number = (unsigned int) my_atol(tmp);
			} 
			else if (!my_strnicmp(tmp, "-FREQ", 3))
			{
				if ((tmp = next_arg(args, &args)))
					freq = (unsigned int)my_atol(tmp);
			} 
			else if (!my_strnicmp(tmp, "-MD5", 3))
			{
				md5 = 1;
			} 
			else if (!my_strnicmp(tmp, "-FORMAT", 3))
			{
				if ((tmp = new_next_arg(args, &args)))
					malloc_strcpy(&fs_output, tmp);
			} 
			else
			{
				count += print_mp3(tmp, fs_output, freq, number, bitrate, md5);
				m_s3cat(&tmp_pat, " ", tmp);
			}
		}
	}
	else
		count += print_mp3(NULL, fs_output, freq, number, bitrate, md5);
	
	if (do_hook(MODULE_LIST, "NAP MATCHEND %d %s", count, tmp_pat ? tmp_pat : "*"))
		nap_say("Found %d files matching \"%s\"", count, tmp_pat ? tmp_pat : "*");
	new_free(&tmp_pat);
	new_free(&fs_output);
}


static unsigned long convert_to_header(unsigned char * buf)
{
	return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}


static int mpg123_head_check(unsigned long head)
{
	if ((head & 0xffe00000) != 0xffe00000)
		return 0;
	if (!((head >> 17) & 3))
		return 0;
	if (((head >> 12) & 0xf) == 0xf)
		return 0;
	if (!((head >> 12) & 0xf))
		return 0;
	if (((head >> 10) & 0x3) == 0x3)
		return 0;
	if (((head >> 19) & 1) == 1 && ((head >> 17) & 3) == 3 && ((head >> 16) & 1) == 1)
		return 0;
	if ((head & 0xffff0000) == 0xfffe0000)
		return 0;
	
	return 1;
}

int tabsel_123[2][3][16] =
{
	{
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,},
       {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,},
       {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320,}},

	{
       {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256,},
	    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,},
	    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,}}
};

long mpg123_freqs[9] =
{44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000};

int parse_header(AUDIO_HEADER *fr, unsigned long newhead)
{
	double bpf;
	
	/* 00 2.5
	   01 reserved 
	   10 version2
	   11 version1
	*/
	if (newhead & (1 << 20))
	{
		fr->ID = (newhead & (1 << 19)) ? 0x0 : 0x1;
		fr->mpeg25 = 0;
	}
	else
	{
		fr->ID = 1;
		fr->mpeg25 = 1;
	}
	fr->layer = ((newhead >> 17) & 3);
	if (fr->mpeg25)
		fr->sampling_frequency = 6 + ((newhead >> 10) & 0x3);
	else
		fr->sampling_frequency = ((newhead >> 10) & 0x3) + (fr->ID * 3);

	fr->error_protection = ((newhead >> 16) & 0x1) ^ 0x1;

	if (fr->mpeg25)		/* allow Bitrate change for 2.5 ... */
		fr->bitrate_index = ((newhead >> 12) & 0xf);

	fr->bitrate_index = ((newhead >> 12) & 0xf);
	fr->padding = ((newhead >> 9) & 0x1);
	fr->extension = ((newhead >> 8) & 0x1);
	fr->mode = ((newhead >> 6) & 0x3);
	fr->mode_ext = ((newhead >> 4) & 0x3);
	fr->copyright = ((newhead >> 3) & 0x1);
	fr->original = ((newhead >> 2) & 0x1);
	fr->emphasis = newhead & 0x3;

	fr->stereo = (fr->mode == 3) ? 1 : 2;
	fr->true_layer = 4 - fr->layer;

	if (!fr->bitrate_index)
		return 0;

	switch (fr->true_layer)
	{
		case 1:
			fr->bitrate = tabsel_123[fr->ID][0][fr->bitrate_index];
			fr->framesize = (long) tabsel_123[fr->ID][0][fr->bitrate_index] * 12000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize = ((fr->framesize + fr->padding) << 2) - 4;
			fr->freq = mpg123_freqs[fr->sampling_frequency];
			break;
		case 2:
			fr->framesize = (long) tabsel_123[fr->ID][1][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize += fr->padding - 4;
			fr->freq = mpg123_freqs[fr->sampling_frequency];
			fr->bitrate = tabsel_123[fr->ID][1][fr->bitrate_index];
			break;
		case 3:
		{
			fr->bitrate = tabsel_123[fr->ID][2][fr->bitrate_index];
			fr->framesize = (long) tabsel_123[fr->ID][2][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency] << (fr->ID);
			fr->framesize = fr->framesize + fr->padding - 4;
			fr->freq = mpg123_freqs[fr->sampling_frequency];
			break;
		}
		default:
			return 0;
	}
	if (fr->framesize > 1792) /* supposedly max framesize */
		return 0;
	switch (fr->true_layer)
	{
		case 1:
			bpf =  tabsel_123[fr->ID][0][fr->bitrate_index];
			bpf *= 12000.0 * 4.0;
			bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->ID);
			break;
		case 2:
		case 3:
			bpf = tabsel_123[fr->ID][fr->true_layer - 1][fr->bitrate_index];
			bpf *= 144000;
			bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->ID);
			break;
		default:
			bpf = 1.0;
	}
	fr->totalframes = fr->filesize / bpf;
	return 1;
}

double compute_tpf(AUDIO_HEADER *fr)
{
	static int bs[4] = {0, 384, 1152, 1152};
	double tpf;

	tpf = (double) bs[fr->true_layer];
	tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->ID);
	return tpf;
}


long get_bitrate(int fdes, time_t *mp3_time, unsigned int *freq_rate, unsigned long *filesize, int *stereo, long *id3, int *mime_type)
{


	AUDIO_HEADER header = {0};
	unsigned long btr = 0;
	struct stat	st;
	unsigned long	head;

unsigned char	buf[1025];
unsigned char	tmp[5];
			
	if (freq_rate)
		*freq_rate = 0;

	fstat(fdes, &st);

	if (!(*filesize = st.st_size))
		return 0;
	memset(tmp, 0, sizeof(tmp));
	read(fdes, tmp, 4);

	if (!strcmp(tmp, "PK\003\004")) /* zip magic */
		return 0;
	if (!strcmp(tmp, "PE") || !strcmp(tmp, "MZ")) /* windows Exe magic */
		return 0;
	if (!strcmp(tmp, "\037\235")) /* gzip/compress */
		return 0;
	if (!strcmp(tmp, "\037\213") || !strcmp(tmp, "\037\036") || !strcmp(tmp, "BZh")) /* gzip/compress/bzip2 */
		return 0;
	if (!strcmp(tmp, "\177ELF")) /* elf binary */
		return 0;
		
	head = convert_to_header(tmp);

	if ((head == 0x000001ba) || (head == 0x000001b3)) /* got a video mpeg header */
		return 0;
	if ((head == 0xffd8ffe0) || (head == 0x47494638)) /* jpeg image  gif image */
		return 0;
	if ((head == 0xea60)) /* ARJ magic */
		return 0;

	while (!mpg123_head_check(head))
	{
		int in_buf;
		int i;
		/*
		 * The mpeg-stream can start anywhere in the file,
		 * so we check the entire file
		 */
		/* Optimize this */
		if ((in_buf = read(fdes, buf, sizeof(buf)-1)) != (sizeof(buf)-1))
			return 0;
		for (i = 0; i < in_buf; i++)
		{
			head <<= 8;
			head |= buf[i];
			if(mpg123_head_check(head))
			{
				lseek(fdes, i+1-in_buf, SEEK_CUR);
				break;
			}
		}
	}
	header.filesize = st.st_size;
	parse_header(&header, head);

	btr = header.bitrate;

	*mp3_time = (time_t) (header.totalframes * compute_tpf(&header));
	*freq_rate = header.freq;

	if (id3)
	{
		char buff[130];
		int rc;
		lseek(fdes, 0, SEEK_SET);
		*id3 = 0;
		rc = read(fdes, buff, 128);
		if (!strncmp(buff, "ID3", 3))
		{
			struct id3v2 {
				char tag[3];
				unsigned char ver[2];
				unsigned char flag;
				unsigned char size[4];
			} id3v2;
			unsigned char bytes[4];
			/* this is id3v2 */
			memcpy(&id3v2, buff, sizeof(id3v2));
			bytes[3] = id3v2.size[3] | ((id3v2.size[2] & 1 ) << 7);
			bytes[2] = ((id3v2.size[2] >> 1) & 63) | ((id3v2.size[1] & 3) << 6);
			bytes[1] = ((id3v2.size[1] >> 2) & 31) | ((id3v2.size[0] & 7) << 5);
			bytes[0] = ((id3v2.size[0] >> 3) & 15);

			*id3 = (bytes[3] | (bytes[2] << 8) | ( bytes[1] << 16) | ( bytes[0] << 24 )) + sizeof(struct id3v2);
		}
		lseek(fdes, st.st_size-128, SEEK_SET);
		rc = read(fdes, buff, 128);
		if ((rc == 128) && !strncmp(buff, "TAG", 3))
			*id3 = *id3 ? (*id3 * -1) : 1;
	}
	*stereo = header.mode;
	return btr;
}

off_t file_size (char *filename)
{
	struct stat statbuf;

	if (!stat(filename, &statbuf))
		return (off_t)(statbuf.st_size);
	else
		return -1;
}

char *calc_md5(int r, unsigned long mapsize)
{
unsigned char digest[16];			
md5_state_t state;
char buffer[BIG_BUFFER_SIZE+1];
struct stat st;
unsigned long size = DEFAULT_MD5_SIZE;
int di = 0;
int rc;

#if !defined(WINNT) && !defined(__EMX__)
	char *m;
#endif

	*buffer = 0;
	md5_init(&state);
	if ((fstat(r, &st)) == -1)
		return m_strdup("");
		
	if (!mapsize)
	{
		if (st.st_size < size)
			size = st.st_size;
	}
	else if (st.st_size < mapsize)
		size = st.st_size;
	else
		size = mapsize;

#if defined(WINNT) || defined(__EMX__)
	while (size)
	{
		unsigned char md5_buff[8 * NAP_BUFFER_SIZE+1];
		rc = (size >= (8 * NAP_BUFFER_SIZE)) ?  8 * NAP_BUFFER_SIZE : size;
		rc = read(r, md5_buff, rc);
		md5_append(&state, (unsigned char *)md5_buff, rc);
		if (size >= 8 * NAP_BUFFER_SIZE)
			size -= rc;
		else
			size = 0;
	}						
	md5_finish(digest, &state);
	{
#else
	if ((m = mmap((void *)0, size, PROT_READ, MAP_PRIVATE, r, 0)) != MAP_FAILED)
	{
		md5_append(&state, (unsigned char *)m, size);
		md5_finish(digest, &state);
		munmap(m, size);
#endif
		memset(buffer, 0, 200);
		for (di = 0, rc = 0; di < 16; ++di, rc += 2)
			snprintf(&buffer[rc], BIG_BUFFER_SIZE, "%02x", digest[di]);
		strcat(buffer, "-");
		strcat(buffer, ltoa(st.st_size));
	}
	return m_strdup(buffer);
}

unsigned int scan_mp3_dir(char *path, int recurse, int reload, int share, int search_type)
{
	int	mt = 0;
	glob_t	globpat;
	int	i = 0;
	Files	*new;
	int	count = 0;
	int	r;
	char	buffer[2*NAP_BUFFER_SIZE+1];
	
	memset(&globpat, 0, sizeof(glob_t));
	read_glob_dir(path, GLOB_MARK|GLOB_NOSORT, &globpat, recurse);
	for (i = 0; i < globpat.gl_pathc; i++)
	{
		char	*fn;
		long	id3 = 0;
		fn = globpat.gl_pathv[i];
		if (fn[strlen(fn)-1] == '/')
			continue;
		switch (search_type)
		{
			case MP3_ONLY:
				if (!(mt = wild_match(DEFAULT_FILEMASK, fn)))
					continue;
				break;
			case VIDEO_ONLY:
				if (!(mt = wild_match("*.mpg", fn)) && !(mt = wild_match("*.dat", fn)))
					continue;
				break;
			case IMAGE_ONLY:
				if (!(mt = wild_match("*.jpg", fn)) && !(mt = wild_match("*.gif", fn)))
					continue;
				break;
			case ANY_FILE:
				break; 
		}
		if (reload && find_in_list((List **)&fserv_files, globpat.gl_pathv[i], 0))
			continue;
		if ((r = open(fn, O_RDONLY)) == -1)
			continue;
		new = (Files *) new_malloc(sizeof(Files));
		new->filename = m_strdup(fn);
		new->bitrate = get_bitrate(r, &new->time, &new->freq, &new->filesize, &new->stereo, &id3, &new->type);
		if (new->filesize && new->bitrate)
		{
			unsigned long size = DEFAULT_MD5_SIZE;
			switch(id3)
			{
				case 1:
				{
					if (new->filesize < size)
						size = new->filesize - 128;
				}
				case 0:
					lseek(r, 0, SEEK_SET);
					break;
				default:
				{
					lseek(r, (id3 < 0) ? -id3 : id3, SEEK_SET);
					if (id3 > 0)
					{
						if ((new->filesize - id3) < size)
							size = new->filesize - id3;
					}
					else
					{
						/*blah. got both tags. */
						if ((new->filesize + id3 - 128) < size)
							size = new->filesize + id3 - 128;
					}
					break;
				}
			}
			new->checksum = calc_md5(r, size);
			close(r);
			r = -1;
			add_to_list((List **)&fserv_files, (List *)new);
			statistics.total_files++;
			statistics.total_filesize += new->filesize;
			count++;
			if (share && (nap_socket != -1))
			{
				sprintf(buffer, "\"%s\" %s %lu %u %u %lu", new->filename, 
					new->checksum, new->filesize, new->bitrate, new->freq, new->time);
				send_ncommand(CMDS_ADDFILE, convertnap_dos(buffer));
				statistics.shared_files++;
				statistics.shared_filesize += new->filesize;
			}
			if (!(count % 25))
			{
				lock_stack_frame();
				io("scan_mp3_dir");
				unlock_stack_frame();
				build_napster_status(NULL);
			}
		}
		else if (search_type != MP3_ONLY)
		{
			unsigned long size = DEFAULT_MD5_SIZE;
			if (new->filesize < size)
				size = new->filesize;
			new->checksum = calc_md5(r, size);
			close(r);
			r = -1;
			add_to_list((List **)&fserv_files, (List *)new);
			statistics.total_files++;
			statistics.total_filesize += new->filesize;
			count++;
		}
		else
		{
			new_free(&new->filename);
			new_free(&new);
		}
		if (r !=  -1)
			close(r);
	}
	bsd_globfree(&globpat);
	return count;
}

void load_shared(char *fname)
{
char buffer[BIG_BUFFER_SIZE+1];
char *expand = NULL;
FILE *fp;
int count = 0;
Files *new;
	if (!fname || !*fname)
		return;
	if (!strchr(fname, '/'))
		sprintf(buffer, "%s/%s", get_string_var(CTOOLZ_DIR_VAR), fname);
	else
		sprintf(buffer, "%s", fname);
	expand = expand_twiddle(buffer);
	if ((fp = fopen(expand, "r")))
	{
		char *fn, *md5, *fs, *br, *fr, *t, *args;
		while (!feof(fp))
		{
			if (!fgets(buffer, BIG_BUFFER_SIZE, fp))
				break;
			args = &buffer[0];
			fn = new_next_arg(args, &args);
			if (fn && *fn && find_in_list((List **)&fserv_files, fn, 0))
				continue;
			if (!(md5 = next_arg(args, &args)) || 
				!(fs = next_arg(args, &args)) ||
				!(br = next_arg(args, &args)) ||
				!(fr = next_arg(args, &args)) ||
				!(t = next_arg(args, &args)))
				continue;
			new = (Files *) new_malloc(sizeof(Files));
			new->filename = m_strdup(fn);
			new->checksum = m_strdup(md5);
			new->time = my_atol(t);
			new->bitrate = my_atol(br);
			new->freq = my_atol(fr);
			new->filesize = my_atol(fs);
			new->stereo = 1;			
			add_to_list((List **)&fserv_files, (List *)new);
			count++;
			statistics.total_files++;
			statistics.total_filesize += new->filesize;
		}
		fclose(fp);
	} else
		nap_say("Error loading %s[%s]", buffer, strerror(errno));
	if (count)
		nap_say("Finished loading %s/%s. Sharing %d files", get_string_var(CTOOLZ_DIR_VAR), fname, count);
	new_free(&expand);
}

void save_shared(char *fname)
{
char buffer[BIG_BUFFER_SIZE+1];
char *expand = NULL;
FILE *fp;
int count = 0;
Files *new;
	if (!fname || !*fname)
		return;
	if (!strchr(fname, '/'))
		sprintf(buffer, "%s/%s", get_string_var(CTOOLZ_DIR_VAR), fname);
	else
		sprintf(buffer, "%s", fname);
	expand = expand_twiddle(buffer);
	if ((fp = fopen(expand, "w")))
	{
		for (new = fserv_files; new; new = new->next)
		{
			fprintf(fp, "\"%s\" %s %lu %u %u %lu\n",
				new->filename, new->checksum, new->filesize, 
				new->bitrate, new->freq, new->time);
			count++;
		}
		fclose(fp);
		nap_say("Finished saving %s [%d]", buffer, count);
		statistics.total_files = 0;
		statistics.total_filesize = 0;
	} else
		nap_say("Error saving %s %s", buffer, strerror(errno));
	new_free(&expand);
}

static int in_load = 0;
BUILT_IN_DLL(load_napserv)
{
	char	*path = NULL;
	int	recurse = 1;
	char	*pch;
	int	count = 0;
	int	reload = 0;
	int	share = 0;
	int	type = MP3_ONLY;
	
	char	fname[] = "shared.dat";
		
	if (command && !my_stricmp(command, "NRELOAD"))
		reload = 1;
	
	if (in_load)
	{
		nap_say("Already loading files. Please wait");
		return;
	}
	in_load++;
	if (args && *args)
	{
		if (!my_stricmp(args, "-clear"))
		{
			Files *new;
			if (statistics.shared_files)
			{
				for (new = fserv_files; new; new = new->next)
					send_ncommand(CMDS_REMOVEFILE, new->filename);
			}
			statistics.total_files = 0;
			statistics.total_filesize = 0;
			statistics.shared_files = 0;
			statistics.shared_filesize = 0;
			clear_files(&fserv_files);
			in_load--;
			return;
		}
		else if (!my_stricmp(args, "-file"))
		{
			char *fn;
			next_arg(args, &args);
			fn = next_arg(args, &args);
			load_shared((fn && *fn) ? fn : fname);
			in_load--;
			return;
		}
		else if (!my_stricmp(args, "-save"))
		{
			char *fn;
			next_arg(args, &args);
			fn = next_arg(args, &args);
			save_shared((fn && *fn) ? fn : fname);
			in_load--;
			return;
		}
		else if (!my_strnicmp(args, "-video", 4))
		{
			next_arg(args, &args);
			type = VIDEO_ONLY;
		}
		else if (!my_strnicmp(args, "-image", 4))
		{
			next_arg(args, &args);
			type = IMAGE_ONLY;
		}
		while ((path = new_next_arg(args, &args)) && path && *path)
		{
			int len = strlen(path);
			if (!my_strnicmp(path, "-recurse", len))
			{
				recurse ^= 1;
				continue;
			}
			if (!my_strnicmp(path, "-share", len))
			{
				share ^= 1;
				continue;
			}
			count += scan_mp3_dir(path, recurse, reload, share, type);
		}
	}
	else
	{
		path = get_dllstring_var("napster_dir");

		if (!path || !*path)
		{
			nap_say("No path. /set napster_dir first.");
			in_load = 0;
			return;
		}

		pch = LOCAL_COPY(path);
		while ((path = new_next_arg(pch, &pch)) && path && *path)
			count += scan_mp3_dir(path, recurse, reload, share, type);
	}

	build_napster_status(NULL);
	if (!fserv_files || !count)
		nap_say("Could not read dir");
	else if (do_hook(MODULE_LIST, "NAP LOAD %d", count))
		nap_say("Found %d files%s", count, share ? "" : ". To share these type /nshare");
	in_load = 0;
	return;
}

static int in_sharing = 0;

BUILT_IN_DLL(share_napster)
{
unsigned long count = 0;
char buffer[2*NAP_BUFFER_SIZE+1];
Files	*new;
	if (in_sharing)
	{
		nap_say("Already Attempting share");
		return;
	}
	in_sharing++;
	for (new = fserv_files; new && (nap_socket != -1); new = new->next, count++)
	{
		int rc;
		int len;
		int cmd;
		char *name;
				
		if (!new->checksum || !new->filesize || !new->filename)
			continue;
		name = LOCAL_COPY(new->filename);
		name = convertnap_dos(name);
		
		if (new->freq && new->bitrate)
		{
			sprintf(buffer, "\"%s\" %s %lu %u %u %lu", name, 
				new->checksum, new->filesize, new->bitrate, new->freq, new->time);
			cmd = CMDS_ADDFILE;
		}
		else
		{
			char *s;
			if (!(s = find_mime_type(new->filename)))
				continue;
			sprintf(buffer, "\"%s\" %lu %s %s", name, new->filesize, new->checksum, s);
			cmd = CMDS_ADDMIMEFILE;
		}
		len = strlen(buffer);
		if ((rc = send_ncommand(cmd, buffer)) == -1)
		{
			nclose(NULL, NULL, NULL, NULL, NULL);
			in_sharing = 0;
			return;
		}
		statistics.shared_files++;
		statistics.shared_filesize += new->filesize;
		while (rc != len)
		{
			int i;
			if (!(count % 2))
			{
				lock_stack_frame();
				io("share napster");
				unlock_stack_frame();
				build_napster_status(NULL);
			}
			if ((nap_socket > -1) && (i = write(nap_socket, buffer+rc, strlen(buffer+rc))) != -1)
				rc += i;
			else
			{
				nclose(NULL, NULL, NULL, NULL, NULL);
				in_sharing = 0;
				return;
			}
		}
		if (!(count % 20))
		{
			lock_stack_frame();
			io("share napster");
			unlock_stack_frame();
			build_napster_status(NULL);
		}
	}
	build_napster_status(NULL);
	if (do_hook(MODULE_LIST, "NAP SHARE %d", count))
		nap_say("%s", cparse("Sharing $0 files", "%l", count));
	in_sharing = 0;
}

int nap_finished_file(int snum, GetFile *gf)
{
SocketList *s = NULL;
	if (snum > 0)
	{
		if ((s = get_socket(snum)))
		{
			s->is_write = 0;
			s->info = NULL;
		}
		close_socketread(snum);
	}
	if (gf)
	{
		if (gf->write > 0)
			close(gf->write);
		new_free(&gf->nick);
		new_free(&gf->filename);
		new_free(&gf->checksum);
		new_free(&gf->realfile);
		new_free(&gf->ip);
		if (gf->up == NAP_UPLOAD)
			current_sending--;
		new_free(&gf);
	}
	return 0;
}

void sendfile_timeout(int snum)
{
GetFile *f = NULL;
	if ((f = (GetFile *)get_socketinfo(snum)))
	{
		f = find_in_getfile(&napster_sendqueue, 1, f->nick, NULL, f->filename, -1, NAP_UPLOAD);
		if (do_hook(MODULE_LIST, "NAP SENDTIMEOUT %s %s", f->nick, strerror(errno)))
			nap_say("%s", cparse("Send to $0 timed out [$1-]", "%s %s", f->nick, strerror(errno)));
		if (f->socket)
			send_ncommand(CMDS_UPDATE_SEND, NULL);
	}
	nap_finished_file(snum, f);
	build_napster_status(NULL);
	return;
}

int clean_queue(GetFile **gf, int timeout)
{
GetFile *ptr;
int count = 0;
	if (!gf || !*gf || timeout <= 0)
		return 0;
	ptr = *gf;
	while (ptr)
	{
		if (ptr->addtime && (ptr->addtime <= (now - timeout)))
		{
			if (!(ptr = find_in_getfile(gf, 1, ptr->nick, NULL, ptr->filename, -1, NAP_UPLOAD)))
				continue;
			if (ptr->write > 0)
				close(ptr->write);
			if (ptr->socket > 0)
			{
				SocketList *s;
				s = get_socket(ptr->socket);
				s->is_write = 0;
				s->info = NULL;
				close_socketread(ptr->socket);
				send_ncommand(CMDS_UPDATE_SEND, NULL);
			}
			new_free(&ptr->nick);
			new_free(&ptr->filename);
			new_free(&ptr->checksum);
			new_free(&ptr->realfile);
			new_free(&ptr->ip);
			if (ptr->up == NAP_UPLOAD)
				current_sending--;
			new_free(&ptr);
			ptr = *gf;
			count++;
		} 
		else 
			ptr = ptr->next;
	}
	if (count)
		nap_say("Cleaned queue of stale entries");
	return count;
}

NAP_COMM(cmd_accepterror)
{
char *nick, *filename;
	nick = next_arg(args, &args);
	filename = new_next_arg(args, &args);
	if (nick && filename)
	{
		GetFile *gf;
		if (!(gf = find_in_getfile(&napster_sendqueue, 1, nick, NULL, filename, -1, NAP_UPLOAD)))
			return 0;
		nap_say("%s", cparse("Removing $0 from the send queue. Accept error", "%s", nick));
		nap_finished_file(gf->socket, gf);
	}
	return 0;
}

void naplink_handleconnect(int);

NAP_COMM(cmd_firewall_request) /* 501 */
{
char	*nick,
	*ip,
	*filename,
	*md5;
unsigned short port = 0;
	nick = next_arg(args, &args);
	ip = next_arg(args, &args);
	port = my_atol(next_arg(args, &args));
	filename = new_next_arg(args, &args);
	convertnap_unix(filename);
	md5 = next_arg(args, &args);
	if (!port)
		nap_say("Unable to send to a firewalled system");
	else
	{
		GetFile *gf;
		int getfd;
		struct sockaddr_in socka;
#ifndef NO_STRUCT_LINGER
		int len;
		struct linger	lin;
		len = sizeof(lin);
		lin.l_onoff = lin.l_linger = 1;
#endif
		if (!(gf = find_in_getfile(&napster_sendqueue, 1, nick, NULL, filename, -1, -1)))
		{
			nap_say("no such file requested %s %s", nick, filename);
			return 0;
		}
		gf->checksum = m_strdup(md5);
		
		getfd = socket(AF_INET, SOCK_STREAM, 0);
		socka.sin_addr.s_addr = strtoul(ip, NULL, 10);
		socka.sin_family = AF_INET;
		socka.sin_port = htons(port);
		alarm(get_int_var(CONNECT_TIMEOUT_VAR));
		if (connect(getfd, (struct sockaddr *)&socka, sizeof(struct sockaddr)) != 0) 
		{
			nap_say("ERROR connecting [%s]", strerror(errno));
			send_ncommand(CMDR_DATAPORTERROR, gf->nick);
			new_free(&gf->nick);
			new_free(&gf->filename);
			new_free(&gf->ip);
			new_free(&gf->checksum);
			new_free(&gf->realfile);
			new_free(&gf);
			return 0;
		}
		alarm(0);
#ifndef NO_STRUCT_LINGER
		setsockopt(getfd, SOL_SOCKET, SO_LINGER, (char *)&lin, len);
#endif
		gf->socket = getfd;
		gf->next = napster_sendqueue;
		napster_sendqueue = gf;
		add_socketread(getfd, getfd, 0, inet_ntoa(socka.sin_addr), naplink_handleconnect, NULL);
		set_socketinfo(getfd, gf);
		write(getfd, "1", 1);
	}
	return 0;
}

NAP_COMM(cmd_filerequest)
{
char *nick;
Files *new = NULL;
char *filename;
int count = 0;

	nick = next_arg(args, &args);
	filename = new_next_arg(args, &args);
	if (!nick || !filename || !*filename || check_nignore(nick))
		return 0;
	convertnap_unix(filename);
	for (new = fserv_files; new; new = new->next)
	{
		if (!strcmp(filename, new->filename))
			break;
	}
	if (new)
	{
		char buffer[2*NAP_BUFFER_SIZE+1];
		GetFile *gf;
		int dl_limit, dl_count;
		for (gf = napster_sendqueue; gf; gf = gf->next)
		{
			if (!gf->filename)
			{
				nap_say("ERROR in cmd_filerequest. gf->filename is null");
				return 0;
				
			}
			count++;
			if (!strcmp(filename, gf->filename) && !strcmp(nick, gf->nick))
			{
				if (do_hook(MODULE_LIST, "NAP SENDFILE already queued %s %s", gf->nick, gf->filename))
					nap_say("%s", cparse("$0 is already queued for $1-", "%s %s", gf->nick, gf->filename));
				break;
			}
		}
		dl_limit = get_dllint_var("napster_max_send_nick");
		dl_count = count_download(nick);
		if (!get_dllint_var("napster_share") || (get_dllint_var("napster_send_limit") && count > get_dllint_var("napster_send_limit")) || (dl_limit && (dl_count >= dl_limit)))
		{
			sprintf(buffer, "%s \"%s\" %d", nick, convertnap_dos(filename), (dl_limit && dl_count >= dl_limit) ? dl_limit : get_dllint_var("napster_send_limit"));
			send_ncommand(CMDS_SENDLIMIT, buffer);
			return 0;
		}
		if (do_hook(MODULE_LIST, "NAP SENDFILE %s %s", nick, filename))
			nap_say("%s", cparse("$0 has requested [$1-]", "%s %s", nick, base_name(filename)));
		sprintf(buffer, "%s \"%s\"", nick, new->filename);
		send_ncommand(CMDS_REQUESTINFO, nick);
		send_ncommand(CMDS_FILEINFO, convertnap_dos(buffer));
		if (!gf)
		{
			gf = new_malloc(sizeof(GetFile));
			gf->nick = m_strdup(nick);
			gf->checksum = m_strdup(new->checksum);
			gf->filename = m_strdup(new->filename);
			if ((gf->write = open(new->filename, O_RDONLY)) < 0)
				nap_say("Unable to open %s for sending [%s]", new->filename, strerror(errno));
			gf->filesize = new->filesize;
			gf->next = napster_sendqueue;
			gf->up = NAP_UPLOAD;
			napster_sendqueue = gf;
			current_sending++;
		}
		gf->addtime = time(NULL);
		clean_queue(&napster_sendqueue, 300);
	} 
	return 0;
}

void napfile_sendfile(int snum)
{
GetFile *gf;
unsigned char buffer[NAP_BUFFER_SIZE+1];
int rc, numread;
	if (!(gf = (GetFile *)get_socketinfo(snum)))
		return;
	gf->addtime = now;
	numread = read(gf->write, buffer, sizeof(buffer)-1);
	switch(numread)
	{
		case -1:
		case 0:
		{
			close(gf->write);
			if ((gf = find_in_getfile(&napster_sendqueue, 1, gf->nick, NULL, gf->filename, -1, NAP_UPLOAD)))
			{
				if ((gf->received + gf->resume) >= gf->filesize)
				{
					double speed;
					char speed1[80];
					statistics.files_served++;
					statistics.filesize_served += gf->received;
					speed = gf->received / 1024.0 / (time(NULL) - gf->starttime);
					if (speed > statistics.max_uploadspeed)
						statistics.max_uploadspeed = speed;
					sprintf(speed1, "%4.2fK/s", speed);
					if (do_hook(MODULE_LIST, "NAP SENDFILE FINISHED %s %s %s", gf->nick, speed1, gf->filename)) 
						nap_say("%s", cparse("Finished Sending $0 [$2-] at $1", "%s %s %s", gf->nick, speed1, base_name(gf->filename)));
				}
				else if (do_hook(MODULE_LIST, "NAP SENDFILE ERROR %s %lu %lu %s", gf->nick, gf->filesize, gf->received + gf->resume, base_name(gf->filename)))
				{
					char rs[60];
					sprintf(rs, "%4.2g%s", _GMKv(gf->received+gf->resume), _GMKs(gf->received+gf->resume));
					nap_say("%s", cparse("Error sending [$2-] to $0 ", "%s %s \"%s\"", gf->nick, rs, base_name(gf->filename)));
				}
			}
			nap_finished_file(snum, gf);
			build_napster_status(NULL);
			send_ncommand(CMDS_UPDATE_SEND, NULL);
			break;
		}
		default:
		{
			rc = send(snum, buffer, numread, 0);	
			if (rc != numread)
			{
				if (rc == -1)
				{
					if (errno == EWOULDBLOCK || errno == ENOBUFS)
						lseek(gf->write, -numread, SEEK_CUR);
					else
					{
						if ((gf = find_in_getfile(&napster_sendqueue, 1, gf->nick, NULL, gf->filename, -1, NAP_UPLOAD)))
						{
							if (do_hook(MODULE_LIST, "NAP SENDFILE ERROR %s %lu %lu \"%s\" %s", gf->nick, gf->filesize, gf->received + gf->resume, base_name(gf->filename), strerror(errno)))
							{
								char rs[60];
								sprintf(rs, "%4.2g%s", _GMKv(gf->received+gf->resume), _GMKs(gf->received+gf->resume));
								nap_say("%s", cparse("Error sending [$2-] to $0 ", "%s %s \"%s\" %s", gf->nick, rs, base_name(gf->filename), strerror(errno)));
							}
						}
						nap_finished_file(snum, gf);
						build_napster_status(NULL);
						send_ncommand(CMDS_UPDATE_SEND, NULL);
					}
					return;
				}
				else 
					lseek(gf->write, -(numread - rc), SEEK_CUR);
			}
			gf->received += rc;
			if (!(gf->received % (10 * (sizeof(buffer)-1))))
				build_napster_status(NULL);
		}
	}
}

void napfirewall_pos(int snum)
{
int rc;
char buff[80];
GetFile *gf;
unsigned long pos;
SocketList *s;
	s = get_socket(snum);
	if (!s || !(gf = (GetFile *)get_socketinfo(snum)))
		return;
	alarm(10);
	if ((rc = read(snum, buff, sizeof(buff)-1)) < 1)
	{
		alarm(0);
		return;
	}
	alarm(0);
	buff[rc] = 0;
	pos = my_atol(buff);
	gf->resume = pos;
	lseek(gf->write, SEEK_SET, pos);
	s->func_read = napfile_sendfile;
	napfile_sendfile(snum);
}

void nap_firewall_start(int snum)
{
GetFile *gf;
unsigned char buffer[NAP_BUFFER_SIZE+1];
SocketList *s;
	s = get_socket(snum);
	if (!s || !(gf = (GetFile *)get_socketinfo(snum)))
		return;
	if ((read(snum, buffer, 4)) < 1)
		return;
	
#if 0
	sprintf(buffer, "%s \"%s\" %lu", gf->nick, gf->filename, gf->filesize);
	write(snum, convertnap_dos(buffer), strlen(buffer));
#endif
	if (*buffer && !strcmp(buffer, "SEND"))
		s->func_read = napfirewall_pos;	
	else
		close_socketread(snum);
}

void napfile_read(int snum)
{
GetFile *gf;
unsigned char buffer[NAP_BUFFER_SIZE+1];
int rc;
SocketList *s;
	s = get_socket(snum);
	if (!(gf = (GetFile *)get_socketinfo(snum)))
	{
		unsigned char buff[2*NAP_BUFFER_SIZE+1];
		unsigned char fbuff[2*NAP_BUFFER_SIZE+1];
		char *nick, *filename, *args;
		
		alarm(10);
		if ((rc = read(snum, buff, sizeof(buff)-1)) < 0)
		{
			alarm(0);
			close_socketread(snum);
			return;
		}
		alarm(0);
		buff[rc] = 0;
		args = &buff[0];
		if (!*args || !strcmp(buff, "FILE NOT FOUND") || !strcmp(buff, "INVALID REQUEST"))
		{
			close_socketread(snum);
			nap_say("Error %s", *args ? args : "unknown read");
			return;
		}

		nick = next_arg(args, &args);
		if ((filename = new_next_arg(args, &args)) && *filename)
		{
			strcpy(fbuff, filename);
			convertnap_unix(fbuff);
		}
		if (!nick || !filename || !*filename || !args || !*args
			|| !(gf = find_in_getfile(&napster_sendqueue, 0, nick, NULL, fbuff, -1, NAP_UPLOAD)) 
			|| (gf->write == -1))
		{
			memset(buff, 0, 80);
			if (!gf)
				sprintf(buff, "0INVALID REQUEST");
			else
			{
				sprintf(buff, "0FILE NOT FOUND");
				if ((gf = find_in_getfile(&napster_sendqueue, 1, nick, NULL, fbuff, -1, NAP_UPLOAD)))
					gf->socket = snum;
			}
			write(snum, buff, strlen(buffer));
			nap_finished_file(snum, gf);
			return;
		}
		gf->resume = strtoul(args, NULL, 0);
		if (gf->resume >= gf->filesize)
		{
			gf = find_in_getfile(&napster_sendqueue, 1, nick, NULL, fbuff, -1, NAP_UPLOAD);
			nap_finished_file(snum, gf);
			return;
		}
		gf->socket = snum;
		lseek(gf->write, SEEK_SET, gf->resume);
		set_socketinfo(snum, gf);
		memset(buff, 0, 80);
		sprintf(buff, "%lu", gf->filesize);
		write(snum, buff, strlen(buff));
		s->func_write = s->func_read;
		s->is_write = s->is_read;
		if (do_hook(MODULE_LIST, "NAP SENDFILE %sING %s %s",gf->resume ? "RESUM" : "SEND", gf->nick, gf->filename))
			nap_say("%s", cparse("$0ing file to $1 [$2-]", "%s %s %s", gf->resume ? "Resum" : "Send", gf->nick, base_name(gf->filename)));
		add_sockettimeout(snum, 0, NULL);
		set_non_blocking(snum);
		build_napster_status(NULL);
		send_ncommand(CMDS_UPDATE_SEND1, NULL);
		return;
	} else if (!gf->starttime)
		gf->starttime = now;
	s->func_read = napfile_sendfile;
	napfile_sendfile(snum);
}

void naplink_handleconnect(int snum)
{
unsigned char buff[2*NAP_BUFFER_SIZE+1];
SocketList *s;
int rc;
	memset(buff, 0, sizeof(buff) - 1);
	switch ((rc = recv(snum, buff, 4, MSG_PEEK)))
	{

		case -1:
			nap_say("naplink_handleconnect %s", strerror(errno));
			close_socketread(snum);
		case 0:
			return;
		default:
			break;
	}

	buff[rc] = 0;
	if (!(s = get_socket(snum)))
	{
		close_socketread(snum);
		return;
	}
	if (rc == 1 && (buff[0] == '1' || buff[0] == '\n'))
	{
		read(snum, buff, 1);
/*		write(snum, "SEND", 4);*/
		s->func_read = nap_firewall_start;
	}
	else if (!strncmp(buff, "GET", 3))
	{
	/* someone has requested a non-firewalled send */
		read(snum, buff, 3);
		set_napster_socket(snum);
		s->func_read = napfile_read;
	}
	else if (!strncmp(buff, "SEND", 4))
	{
	/* we have requested a file from someone who is firewalled */
		read(snum, buff, 4);
		s->func_read = nap_firewall_get;
	}
	else
		close_socketread(snum);
}

void naplink_handlelink(int snum)
{
struct  sockaddr_in     remaddr;
int sra = sizeof(struct sockaddr_in);
int sock = -1;
	if ((sock = accept(snum, (struct sockaddr *) &remaddr, &sra)) > -1)
	{
		add_socketread(sock, snum, 0, inet_ntoa(remaddr.sin_addr), naplink_handleconnect, NULL);
		add_sockettimeout(sock, 180, sendfile_timeout);
		write(sock, "\n", 1); 
	}
}

