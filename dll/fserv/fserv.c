#define AUTO_VERSION "1.00"

/*
 *
 * Written by Colten Edwards. (C) Nov 11/99 
 *
 */

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
#define INIT_MODULE
#include "modval.h"

#include <sys/time.h>
#include <sys/stat.h>

#define cparse convert_output_format
#define FS "%PFS%w:%n"

#define DEFAULT_IMPRESS_TIME 30
#define DEFAULT_FSERV 1
#define DEFAULT_IMPRESS 0
#define DEFAULT_MAX_MATCH 4
#define DEFAULT_RECURSE 1
#define DEFAULT_FILEMASK "*.mp3"


char fserv_version[] = "Fserv 1.000";
char *fserv_filename = NULL;
char FSstr[80] = "FS:";

typedef struct _Stats {
	unsigned long total_files;
	unsigned long total_filesize;
	unsigned long files_served;
	unsigned long filesize_served;
	double max_speed;
	time_t starttime;
} Stats;

Stats statistics = { 0, };

/*
 * ideas.
 * make it notice instead of msg the nick.
 * make it allow /msg nick !filename
 */

#if 0
<Lena-:#mp3jukebox> <><>< For My List(1069 files: 4307MB) and DCC Status Type
   @Lena- and @Lena--stats [(1/10) Slots Taken (0/10) Ques
   Taken] [Open Slot Ready] [Bandwidth in Use: 3081cps]
   [Highest Cps Record: 56.3kb/s by Grey13] [Total Files
   Served: 6045] ><><>
<Gaff> [ !Gaff Is It Too Late Now.mp3 ] [2.6MB 128Kbps 44.1Khz Joint Stereo
    (2m44s)]-SpR-@Gaff -> request my list-@Gaff-que -> check
    que-@Gaff-stats -> check DCC stats-@Gaff-remove -> cancel
    song request
#endif                                                                                                                        

typedef struct _AUDIO_HEADER {
	int IDex;
	int ID;
	int layer;
	int protection_bit;
	int bitrate_index;
	int sampling_frequency;
	int padding_bit;
	int private_bit;
	int mode; /* 0 = STEREO 1 = Joint 2 = DUAL 3 = Mono */
	int mode_extension;
	int copyright;
	int original;
	int emphasis;
	int stereo;
	int jsbound;
	int sblimit;
	int true_layer;
	int framesize;
} AUDIO_HEADER;


static unsigned char _buffer[32];
static int _bptr = 0;


typedef struct _files {
	struct _files	*next;
	char		*filename;
	unsigned long	filesize;
	time_t		time;
	int		bitrate;
	int		freq;
	int		stereo;
	int		id3;
} Files;

Files *fserv_files = NULL;


char *mode_str(int mode)
{
	switch(mode)
	{	
		case 0:
			return "Stereo";
		case 1:
			return "Joint-Stereo";
		case 2:
			return "Dual-Channel";
		case 3:
			return "Mono";
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
	static char	buffer[BIG_BUFFER_SIZE+1];
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
	char	buffer[BIG_BUFFER_SIZE+1];
	
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

unsigned int print_mp3(char *pattern, char *format, int freq, int number, int bitrate)
{
unsigned int count = 0;
Files *new;
char dir[BIG_BUFFER_SIZE];
char *fs = NULL;
	*dir = 0;
	for (new = fserv_files; new; new = new->next)
	{
		if (!pattern || (pattern && wild_match(pattern, new->filename)))
		{
			char *p;
			p = LOCAL_COPY(new->filename);
			p = strrchr(new->filename, '/');
			p++;
			if (do_hook(MODULE_LIST, "FS: File \"%s\" %s %u %lu %lu %u", p, mode_str(new->stereo), new->bitrate, new->time, new->filesize, new->freq))
			{
				if ((bitrate != -1) && (new->bitrate != bitrate))
					continue;
				if ((freq != -1) && (new->freq != freq))
					continue;
				if (!format || !*format)
					put_it("%s \"%s\" %s %dk [%s]", FSstr, p, mode_str(new->stereo), new->bitrate, print_time(new->time));
				else
				{
					if ((fs = make_mp3_string(NULL, new, format, dir)))
						put_it("%s %s", FSstr, fs);
					else
						put_it("%s %s", FSstr, make_mp3_string(NULL, new, format, dir));
				}
			}
			if ((number > 0) && (count == number))
				break;
			count++;
		}
	}
	return count;
}

BUILT_IN_DLL(print_fserv)
{
	int	count = 0;
	int	bitrate = -1;
	int	number = -1;
	int	freq = -1;
	char 	*fs_output = NULL;
	char	*tmp_pat = NULL;
	
	if ((get_dllstring_var("fserv_format")))
		fs_output = m_strdup(get_dllstring_var("fserv_format"));
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
			else if (!my_strnicmp(tmp, "-FORMAT", 3))
			{
				if ((tmp = new_next_arg(args, &args)))
					malloc_strcpy(&fs_output, tmp);
			} 
			else
			{
				count += print_mp3(tmp, fs_output, freq, number, bitrate);
				m_s3cat(&tmp_pat, " ", tmp);
			}
		}
	}
	else
		count += print_mp3(NULL, fs_output, freq, number, bitrate);
	
	if (do_hook(MODULE_LIST, "FS: Found %d %s", count, tmp_pat ? tmp_pat : "*"))
		put_it("%s found %d files matching \"%s\"", FSstr, count, tmp_pat ? tmp_pat : "*");
	new_free(&tmp_pat);
	new_free(&fs_output);
}

int _get_input(int file, unsigned char *bp, int size)
{
	if (read(file,  bp, size) != size)
		return -1;
	return 0;
}
                                        
static inline int readsync(int file)
{
	_bptr=0;
	_buffer[0]=_buffer[1];
	_buffer[1]=_buffer[2];
	_buffer[2]=_buffer[3];
	return _get_input(file, &_buffer[3], 1);
}


static inline int _fillbfr(int file, unsigned int size)
{
	_bptr=0;
        return _get_input(file, _buffer, size);
}


static inline unsigned int _getbits(int n)
{
	unsigned int	pos,
			ret_value;

        pos = _bptr >> 3;
	ret_value = _buffer[pos] << 24 |
		    _buffer[pos+1] << 16 |
		    _buffer[pos+2] << 8 |
		    _buffer[pos+3];
        ret_value <<= _bptr & 7;
        ret_value >>= 32 - n;
        _bptr += n;
        return ret_value;
}       


/*
 * header and side info parsing stuff ******************************************
 */
static inline void parse_header(AUDIO_HEADER *header) 
{
        header->IDex=_getbits(1);
        header->ID=_getbits(1);
        header->layer=_getbits(2);
        header->protection_bit=_getbits(1);
        header->bitrate_index=_getbits(4);
        header->sampling_frequency=_getbits(2);
        header->padding_bit=_getbits(1);
        header->private_bit=_getbits(1);
        header->mode=_getbits(2);
        header->mode_extension=_getbits(2);
        if (!header->mode) 
        	header->mode_extension=0;
        header->copyright=_getbits(1);
        header->original=_getbits(1);
        header->emphasis=_getbits(2);

	header->stereo = (header->mode == 3) ? 1 : 2;
	header->true_layer = 4 - header->layer;
}

int gethdr(int file, AUDIO_HEADER *header)
{
	int	retval;

	if ((retval=_fillbfr(file, 4))) 
		return retval;

	while (_getbits(11) != 0x7ff) 
	{
		if ((retval=readsync(file))!=0) 
			return retval;
	}
	parse_header(header);
	return 0;
}

long get_bitrate(char *filename, time_t *mp3_time, unsigned int *freq_rate, int *id3, unsigned long *filesize, int *stereo)
{
	short t_bitrate[2][3][15] = {{
	{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
	{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
	{0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}
	},{
	{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
	{0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
	{0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}
	}};

	int t_sampling_frequency[2][2][3] = {
		{ /* MPEG 2.5 samplerates */ 
  			{ 11025, 12000, 8000},
			{ 0,0,0 }
		},{ /* MPEG 2.0/1.0 samplerates */
			{ 22050 , 24000 , 16000},
			{ 44100 , 48000 , 32000}
		}
	};

	AUDIO_HEADER header;
	unsigned long btr = 0;
	int l = -1;
	struct stat	st;
	unsigned long	framesize = 0,
			totalframes = 0;
	
	if (freq_rate)
		*freq_rate =0;
	if (id3)
		*id3 = 0;
	if ((l = open(filename, O_RDONLY)) == -1) 
		return 0;
	gethdr(l, &header);
	if (header.ID > 1 || header.layer > 2 || header.bitrate_index > 14) 
	{
		close(l);
		return 0;
	}
	btr = t_bitrate[header.ID][3-header.layer][header.bitrate_index];

	fstat(l, &st);
	if (t_sampling_frequency[header.IDex][header.ID][header.sampling_frequency] > 0)
		framesize = (header.ID ? 144000 : 72000) * btr / t_sampling_frequency[header.IDex][header.ID][header.sampling_frequency];
	totalframes = (st.st_size / (framesize + 1)) - 1;
	if (t_sampling_frequency[header.IDex][header.ID][header.sampling_frequency] > 0)
		*mp3_time = (time_t) (totalframes * (header.ID==0?576:1152)/t_sampling_frequency[header.IDex][header.ID][header.sampling_frequency]);
	*filesize = st.st_size;

	if (freq_rate)
		*freq_rate = t_sampling_frequency[header.IDex][header.ID][header.sampling_frequency];
	if (id3)
	{
		char	buffer[200];
		lseek(l, SEEK_END, -128);
		if (read(l, buffer, 128) > 0)
			if (!strncmp(buffer, "TAG", 3))
				*id3 = 1;
	}
	*stereo = header.mode;
	close(l);	        
	return btr;
}



BUILT_IN_DLL(unload_fserv)
{
	Files	*new, *tmp;
	int	count = 0;
	if (!args || !*args)
	{
		while ((new = fserv_files))
		{
			tmp = fserv_files->next;
			new_free(&new->filename);
			statistics.total_filesize -= new->filesize;
			new_free(&new);
			fserv_files = tmp;
			count++;
		}
	} 
	else 
	{
		char	*pat;

		while ((pat = new_next_arg(args, &args)))	
		{
			if (!pat || !*pat)
				break;
			if ((new = (Files *)remove_from_list((List **)&fserv_files, pat)))
			{
				new_free(&new->filename);
				statistics.total_filesize -= new->filesize;
				new_free(&new);
				count++;
			}
		}
	}
	if (do_hook(MODULE_LIST, "FS: Clear %d", count))
		put_it("%s cleared %d entries", FSstr, count);
	statistics.total_files -= count;
}

off_t file_size (char *filename)
{
	struct stat statbuf;

	if (!stat(filename, &statbuf))
		return (off_t)(statbuf.st_size);
	else
		return -1;
}

unsigned int scan_mp3_dir(char *path, int recurse, int reload)
{
	int	mt = 0;
	glob_t	globpat;
	int	i = 0;
	Files	*new;
	int	count = 0;
	memset(&globpat, 0, sizeof(glob_t));
	read_glob_dir(path, GLOB_MARK|GLOB_NOSORT, &globpat, recurse);
	for (i = 0; i < globpat.gl_pathc; i++)
	{
		char	*fn;
		fn = globpat.gl_pathv[i];
		if (fn[strlen(fn)-1] == '/')
			continue;
		if (!(mt = wild_match(DEFAULT_FILEMASK, fn)))
			continue;
		if (reload && find_in_list((List **)&fserv_files, globpat.gl_pathv[i], 0))
			continue;
		new = (Files *) new_malloc(sizeof(Files));
		new->filename = m_strdup(fn);
		new->bitrate = get_bitrate(fn, &new->time, &new->freq, &new->id3, &new->filesize, &new->stereo);
		if (new->filesize)
		{
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
	}
	bsd_globfree(&globpat);
	return count;
}

BUILT_IN_DLL(load_fserv)
{
	char	*path = NULL;
	int	recurse = DEFAULT_RECURSE;
	char	*pch;
	int	count = 0;
	int	reload = 0;
	
	if (command && !my_stricmp(command, "FSRELOAD"))
		reload = 1;
	if (args && *args)
	{
		while ((path = next_arg(args, &args)) && *path)
		{
			int len = strlen(path);
			if (!my_strnicmp(path, "-recurse", len))
			{
				recurse ^= 1;
				continue;
			}
			count += scan_mp3_dir(path, recurse, reload);
		}
		goto fs_print_out;
	}

	path = get_dllstring_var("fserv_dir");

	if (!path || !*path)
	{
		if (do_hook(MODULE_LIST, "FS: Error no fserv_dir path"))
			put_it("%s No path. /set fserv_dir first.", FSstr);
		return;
	}

	pch = LOCAL_COPY(path);
	while ((path = next_arg(pch, &pch)))
		count += scan_mp3_dir(path, recurse, reload);
fs_print_out:
	if (do_hook(MODULE_LIST, "FS: Load %d", count))
	{
		if (!fserv_files || !count)
			put_it("%s Could not read dir", FSstr);
		else
			put_it("%s found %d files", FSstr, count);
	}
	return;
}

Files *search_list(char *nick, char *pat, int wild)
{
	Files	*new;
	int	dcc_send_num = 0;
	int	dcc_queue_num = 0;
	char	*p;
	char	buffer[BIG_BUFFER_SIZE+1];
	int	num_to_match;

	num_to_match = get_dllint_var("fserv_max_match");
	if (wild)
	{
		int	count = 0;

		sprintf(buffer, "*%s*", pat);
		while ((p = strchr(buffer, ' ')))
			*p = '*';
		dcc_send_num = get_active_count();
		dcc_queue_num = get_num_queue();
		for (new = fserv_files; new; new = new->next)
		{
			p = strrchr(new->filename, '/');			
			p++;
			if (!wild_match(buffer, p))
				continue;

			if (!count)
				if (do_hook(MODULE_LIST, "FS: SearchHeader %s %s %d %d %d %d", nick, buffer, dcc_send_num, get_int_var(DCC_SEND_LIMIT_VAR), dcc_queue_num, get_int_var(DCC_QUEUE_LIMIT_VAR)))
					queue_send_to_server(from_server, "PRIVMSG %s :Matches for %s. Copy and Paste in channel to request. (Slots:%d/%d), (Queue:%d/%d)", nick, buffer, dcc_send_num, get_int_var(DCC_SEND_LIMIT_VAR), dcc_queue_num, get_int_var(DCC_QUEUE_LIMIT_VAR));
			count++;
			if (!num_to_match || (count < num_to_match))
			{
				if (do_hook(MODULE_LIST, "FS: SearchList %s \"%s\" %u %u %lu %lu", nick, p, new->bitrate, new->freq, new->filesize, new->time)) 
					queue_send_to_server(from_server, "PRIVMSG %s :!%s %s %dk [%s]", nick, get_server_nickname(from_server), p, new->bitrate, print_time(new->time));
			}
		}
		if (num_to_match && (count > num_to_match))
		{
			if (do_hook(MODULE_LIST, "FS: SearchTooMany %s %d", nick, count))
				queue_send_to_server(from_server, "PRIVMSG %s :Too Many Matches[%d]", nick, count);
		}
		else if (count)
		{
			if (do_hook(MODULE_LIST, "FS: SearchResults %s %d", nick, count)) 
				queue_send_to_server(from_server, "PRIVMSG %s :..... Total %d files found", nick, count);
		}
		return NULL;
	}
	for (new = fserv_files; new; new = new->next)
	{
		p = strrchr(new->filename, '/');			
		p++;
		if (my_stricmp(pat, p))
			continue;
		return new;
	}
	return NULL;
}


char *make_temp_list(char *nick)
{
	char	*nam;
	char	*real_nam;
	FILE	*fp;
	
	if (!(nam = get_dllstring_var("fserv_filename")) || !*nam)
		nam = tmpnam(NULL);
	real_nam = expand_twiddle(nam);
	if (!fserv_files || !real_nam || !*real_nam)
	{
		new_free(&real_nam);
		return NULL;
	}
	if ((fp = fopen(real_nam, "w")))
	{
		char	buffer2[BIG_BUFFER_SIZE+1];
		char	*fs;
		int	count = 0;
		
		time_t t = now;
		Files *new;
		strftime(buffer2, 200, "%X %d/%m/%Y", localtime(&t));
		for (new = fserv_files; new; new = new->next)
			count++;
		fprintf(fp, "Temporary mp3 list created for %s by %s on %s with %d mp3's\n\n", nick, get_server_nickname(from_server), buffer2, count);
		*buffer2 = 0;
		if (!(fs = get_dllstring_var("fserv_format")) || !*fs)
			fs = " %6.3s %3b [%t]\t %f\n";
		for (new = fserv_files; new; new = new->next)
			make_mp3_string(fp, new, fs, buffer2);
		fclose(fp);
		new_free(&real_nam);
		return nam;
	}
	new_free(&real_nam);
	return NULL;
}

BUILT_IN_DLL(list_fserv)
{
	char *nam;
	if (!get_dllstring_var("fserv_filename"))
	{
		put_it("%s /set fserv_filename first", FSstr);
		return;
	}
	if ((nam = make_temp_list(get_server_nickname(from_server))))
		malloc_strcpy(&fserv_filename, nam);
	return;
}

int search_proc(char *which, char *str, char **unused)
{
	char	*loc,
		*chan = NULL,
		*nick,
		*command,
		*chan_list;
	char	buffer[BIG_BUFFER_SIZE+1];

	loc = LOCAL_COPY(str);
	chan_list = get_dllstring_var("fserv_chan");
	nick = next_arg(loc, &loc);
	
	if (my_stricmp(which, "MSG"))
	{
		chan = next_arg(loc, &loc);
		command = next_arg(loc, &loc);
	}
	else
		command = next_arg(loc, &loc);

	if (!get_dllint_var("fserv"))
		return 1;
	if (chan_list && *chan_list && chan)
	{
		char	*t, 
			*ch;
		int got_it = 0;
		ch = LOCAL_COPY(chan_list);
		if (*ch == '*')
			got_it = 1;
		else
		{
			while ((t = next_in_comma_list(ch, &ch)) && *t)
				if (!my_stricmp(t, chan))
					got_it = 1;
		}
		if (!got_it)
			return 1;
	}
	if (command && *command == '@')
	{
		char *p;
		command++;
		if (!*command)
			return 1;
		if (loc && *loc && (!my_stricmp(command, "locate") || !my_stricmp(command, "find")))
		{
			search_list(nick, loc, 1);
			if (do_hook(MODULE_LIST, "FS: Search %s %s \"%s\"", nick, chan ? chan : "*", loc))
				put_it("%s got nick %s in %s searching for \"%s\"" , FSstr, nick, chan ? chan : "*", loc);
			return 1;
		} 
		if ((p = strchr(command, '-')))
		{
			*p++ = 0;
			if (!*p || my_stricmp(command, get_server_nickname(from_server)))
				return 1;
			if (!my_stricmp("que", command))
			{
				/* check queue */
				return 1;			
			}
			if (!my_stricmp("stats", command))
			{
				/* print stats */
				return 1;			
			}
			if (!my_stricmp("remove", command))
			{
				/* remove from queue */
				return 1;			
			}
		}
	}
	if (command && *command == '!')
	{
		command++;
		if (!*command) return 1;
		if (!my_stricmp(get_server_nickname(from_server), command) && loc && *loc)
		{
			Files	*fn = NULL;
			if ((fn = search_list(nick, loc, 0)))
			{
				int	send_num,
					queue_num;
				send_num = get_active_count();
				queue_num = get_num_queue();
				if (do_hook(MODULE_LIST, "FS: Sending %s \"%s\" $lu", nick, fn->filename, fn->filesize))
					put_it("%s sending %s \"%s\" %lu", FSstr, nick, fn->filename, fn->filesize);
				sprintf(buffer, "%s \"%s\"", nick, fn->filename);
				if (send_num > get_int_var(DCC_SEND_LIMIT_VAR))
				{
					pack *ptr = NULL;
					if (queue_num < get_int_var(DCC_QUEUE_LIMIT_VAR))
					{
						sprintf(buffer, "\"%s\"", fn->filename);
						ptr = (pack *)alloca(sizeof(pack));
						memset(ptr, 0, sizeof(pack));
						ptr->file = LOCAL_COPY(buffer);
						ptr->desc = LOCAL_COPY(buffer);
						ptr->numfiles = 1;
						ptr->size = fn->filesize;
						ptr->server = from_server;
						do_hook(MODULE_LIST, "FS: Queue Add %s %s", nick, buffer);

						if (add_to_queue(nick, "SEND", ptr))
						{
							statistics.files_served++;
							statistics.filesize_served += fn->filesize;
						}
						else if (do_hook(MODULE_LIST, "FS: QueueFile %s %s", nick, buffer))
							queue_send_to_server(from_server, "PRIVMSG %s :Queued File %s", nick, buffer);
					} else if (do_hook(MODULE_LIST, "FS: Queue Full %s", nick))
						queue_send_to_server(from_server, "PRIVMSG %s :Queue is full, try again later.", nick);
				}
				else
				{
					dcc_filesend("SEND", buffer);				
					statistics.files_served++;
					statistics.filesize_served += fn->filesize;
				}
			}
		}
		else if (!my_stricmp(get_server_nickname(from_server), command))
		{
			char	*name = NULL;
			if (fserv_filename || (name = make_temp_list(nick)))
			{
				sprintf(buffer, "%s %s", nick, fserv_filename ? fserv_filename : name);
				dcc_filesend("SEND", buffer);
			}
		}
	}
	return 1;
}

void impress_me(void *args)
{
	int		timer;
	char		*ch = NULL;
	ChannelList	*chan= NULL;

	timer = get_dllint_var("fserv_time");
	if (timer < DEFAULT_IMPRESS_TIME)
		timer = DEFAULT_IMPRESS_TIME;
	if (!(ch = get_dllstring_var("fserv_chan")) || !*ch)
		ch = NULL;
	else
		ch = m_strdup(ch);
	chan = get_server_channels(from_server);
	if (!ch)
		ch = m_strdup(get_current_channel_by_refnum(0));
	else
	{
		char	*c,
			*p;

		c = LOCAL_COPY(ch);
		ch = NULL;
		if (*c == '*')
		{
			ChannelList *chan;
			for (chan = get_server_channels(from_server); chan; chan=chan->next)
				m_s3cat(&ch, ",", chan->channel);
		}
		else
		{
			ChannelList *tmpchan;
			while ((p = next_in_comma_list(c, &c)) && *p)
			{
				if ((tmpchan = (ChannelList *)find_in_list((List **)&chan, p, 0)))
					m_s3cat(&ch, ",", p);
			}
		}
	}
	if (fserv_files && get_dllint_var("fserv_impress"))
	{
		unsigned long	l;
		Files		*new;

		l = random_number(0L) % statistics.total_files;
		for (new = fserv_files; new && l; new = new->next, l--)
			;
		if (new && new->bitrate)
		{
			char	frq[30],
				size[40],
				*p;
			p = strrchr(new->filename, '/');
			p++; 
			if (do_hook(MODULE_LIST, "FS: Impress %s \"%s\" %lu %u %u %s %lu",ch, p, new->filesize, new->bitrate, new->freq, mode_str(new->stereo), new->time))
			{
				sprintf(frq, "%3.1f", ((double)new->freq)/1000.0);
				sprintf(size, "%4.3f%s", _GMKv(new->filesize), _GMKs(new->filesize));
				queue_send_to_server(from_server, "PRIVMSG %s :[  !%s %s  ] [%s %uKbps %sKhz %s]-[%s]",
					ch, get_server_nickname(from_server), p, 
					size, new->bitrate, frq, 
					mode_str(new->stereo), 
					print_time(new->time));
			}
		}
	}
	add_timer(0, empty_string, timer * 1000, 1, impress_me, NULL, NULL, -1, "fserv");
	new_free(&ch);
}

BUILT_IN_FUNCTION(func_convert_mp3time)
{
	int	hours, 
		minutes, 
		seconds;
	if (!input)
		return m_strdup(empty_string);
	seconds = my_atol(input);
	hours = seconds / ( 60 * 60 );
	minutes = seconds / 60;
	seconds = seconds % 60;
	return m_sprintf("[%02d:%02d:%02d]", hours, minutes, seconds);
}

BUILT_IN_DLL(stats_fserv)
{
	put_it("%s\t File Server Statistics From %s", FSstr, my_ctime(statistics.starttime));
	put_it("%s\t Fserv is [%s] Impress is [%s] %d seconds with %d matches allowed", FSstr, on_off(get_dllint_var("fserv")), on_off(get_dllint_var("fserv_impress")), get_dllint_var("fserv_time"), get_dllint_var("fserv_max_match"));
	put_it("%s\t Files available %lu for %4.3f%s",FSstr, statistics.total_files, _GMKv(statistics.total_filesize), _GMKs(statistics.total_filesize));
	put_it("%s\t Files served %lu for %4.3f%s", FSstr, statistics.files_served, _GMKv(statistics.filesize_served), _GMKs(statistics.filesize_served));
}



BUILT_IN_DLL(save_fserv)
{
char bogus[] = "fserv";
FILE *fp;
char buffer[BIG_BUFFER_SIZE];
char *p;
char *fserv_savname = NULL;
	sprintf(buffer, "%s/fserv.sav", get_string_var(CTOOLZ_DIR_VAR));
	fserv_savname  = expand_twiddle(buffer);
	if (!(fp = fopen(fserv_savname, "w")))
	{
		new_free(&fserv_savname);
		return;
	}
	fprintf(fp, "%s %s\n", bogus, on_off(get_dllint_var("fserv")));
	if ((p = get_dllstring_var("fserv_dir")))
		fprintf(fp, "%s%s %s\n", bogus, "_dir", p);
	if ((p = get_dllstring_var("fserv_chan")))
		fprintf(fp, "%s%s %s\n", bogus, "_chan", p);
	if ((p = get_dllstring_var("fserv_filename")))
		fprintf(fp, "%s%s %s\n", bogus, "_filename", p);
	if ((p = get_dllstring_var("fserv_format")))
		fprintf(fp, "%s%s %s\n", bogus, "_format", p);
	fprintf(fp, "%s%s %u\n", bogus, "_time", get_dllint_var("fserv_time"));
	fprintf(fp, "%s%s %u\n", bogus, "_max_match", get_dllint_var("fserv_max_match"));
	fprintf(fp, "%s%s %s\n", bogus, "_impress", on_off(get_dllint_var("fserv_impress")));
	if (statistics.files_served)
	{
		fprintf(fp, "%s%s %lu\n", bogus, "_totalserved", statistics.files_served);
		fprintf(fp, "%s%s %lu\n", bogus, "_totalstart", statistics.starttime);
		fprintf(fp, "%s%s %lu\n", bogus, "_totalsizeserved", statistics.filesize_served);
	}
	fclose(fp);
	if ((do_hook(MODULE_LIST, "FS: Save")))
		put_it("%s Done Saving.", FSstr);
	new_free(&fserv_savname);
}

void fserv_read(char *filename)
{
FILE *fp;
char buff[IRCD_BUFFER_SIZE+1];
char *fserv_savname = NULL;
	fserv_savname  = expand_twiddle(filename);
	if (!(fp = fopen(fserv_savname, "r")))
	{
		new_free(&fserv_savname);
		return;
	}
	fgets(buff, IRCD_BUFFER_SIZE, fp);
	while (!feof(fp))
	{
		char *p;
		chop(buff, 1);
		if ((p = strchr(buff, ' ')))
		{
			*p++ = 0;

			if (!my_strnicmp(buff, "fserv_totalserved", 17))
				statistics.files_served = strtoul(p, NULL, 0);
			else if (!my_strnicmp(buff, "fserv_totalsizeserved", 17))
				statistics.filesize_served = strtoul(p, NULL, 0);
			else if (!my_strnicmp(buff, "fserv_totalserved", 17)) 
				statistics.starttime = strtoul(p, NULL, 0);
			else
			{
				if (*p > '0' && *p < '9')
				{
					int val;
					val = my_atol(p);
					set_dllint_var(buff, val);
				}
				else if (!my_stricmp(p, "ON"))
					set_dllint_var(buff, 1);
				else if (!my_stricmp(p, "OFF"))
					set_dllint_var(buff, 0);
				else
					set_dllstring_var(buff, p);
			}
		}
		fgets(buff, IRCD_BUFFER_SIZE, fp);
	}
	fclose(fp);
}

BUILT_IN_DLL(help_fserv)
{
	put_it("%s FServ %s by Colten Edwards aka panasync", FSstr, AUTO_VERSION); 
	put_it("%s [Sets]", FSstr);
	put_it("%s fserv on/off  fserv functions. Default is %s", FSstr, on_off(DEFAULT_FSERV));
	put_it("%s fserv_dir path [path]", FSstr);
	put_it("%s fserv_chan #chan[,#chan2]", FSstr);
	put_it("%s fserv_time seconds between displays of random mp3. Default is %d", FSstr, DEFAULT_IMPRESS_TIME);
	put_it("%s fserv_max_match defines how many matches allowed. Default is %d", FSstr, DEFAULT_MAX_MATCH);
	put_it("%s fserv_impress on/off public display of random mp3. Default is %s", FSstr, on_off(DEFAULT_IMPRESS));
	put_it("%s", FSstr);
	put_it("%s channel commands are @find pattern or @locate pattern", FSstr);
	put_it("%s !nick filename to send a file to nick requesting", FSstr);
	put_it("%s a /msg to the nick can be used instead of a public", FSstr);
	put_it("%s a $mp3time() function as well as a hook are provided. /on module \"FS:*\"", FSstr);
	put_it("%s    more help available with /help", FSstr);
}

int Fserv_Lock(IrcCommandDll **intp, Function_ptr *global_table)
{
	return 1;
}

char *Fserv_Version(IrcCommandDll **intp)
{
	return AUTO_VERSION;
}


int Fserv_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
char buffer[BIG_BUFFER_SIZE+1];
	initialize_module("Fserv");
	add_module_proc(VAR_PROC, "Fserv", "fserv", NULL, BOOL_TYPE_VAR, DEFAULT_FSERV, NULL, NULL);
	add_module_proc(VAR_PROC, "Fserv", "fserv_dir", NULL, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, "Fserv", "fserv_chan", NULL, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, "Fserv", "fserv_filename", NULL, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, "Fserv", "fserv_format", NULL, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, "Fserv", "fserv_time", NULL, INT_TYPE_VAR, DEFAULT_IMPRESS_TIME, NULL, NULL);
	add_module_proc(VAR_PROC, "Fserv", "fserv_max_match", NULL, INT_TYPE_VAR, DEFAULT_MAX_MATCH, NULL, NULL);
	add_module_proc(VAR_PROC, "Fserv", "fserv_impress", NULL, BOOL_TYPE_VAR, DEFAULT_IMPRESS, NULL, NULL);

	sprintf(buffer, " [-recurse] [path [path]] to load all files -recurse is a \ntoggle and can appear anywhere. Default is [%s]", on_off(DEFAULT_RECURSE));
	add_module_proc(COMMAND_PROC, "Fserv", "fsload", NULL, 0, 0, load_fserv, buffer);

	sprintf(buffer, " [-count #] [-freq #] [-bitrate #] [pattern] to search database locally");
	add_module_proc(COMMAND_PROC, "Fserv", "fsprint", NULL, 0, 0, print_fserv, buffer);

	sprintf(buffer, " to remove all files or [pat [pat]] to remove specific");
	add_module_proc(COMMAND_PROC, "Fserv", "fsunload", NULL, 0, 0, unload_fserv, buffer);

	add_module_proc(COMMAND_PROC, "Fserv", "fshelp", NULL, 0, 0, help_fserv, " to provide help for fserv plugin");

	sprintf(buffer, " [-recurse] [path [path]] to reload all files");
	add_module_proc(COMMAND_PROC, "Fserv", "fsreload", NULL, 0, 0, load_fserv, buffer);

	add_module_proc(COMMAND_PROC, "Fserv", "fsstats", NULL, 0, 0, stats_fserv, " provides fserv statistics");

	sprintf(buffer, " Creates list of mp3");
	add_module_proc(COMMAND_PROC, "Fserv", "fslist", NULL, 0, 0, list_fserv, buffer);

	sprintf(buffer, " to save your stats and settings to %s/fserv.sav", get_string_var(CTOOLZ_DIR_VAR));
	add_module_proc(COMMAND_PROC, "Fserv", "fssave", NULL, 0, 0, save_fserv, buffer);


	add_module_proc(ALIAS_PROC, "Fserv", "mp3time", NULL, 0, 0, func_convert_mp3time, NULL);
        add_module_proc(HOOK_PROC, "Fserv", NULL, "*", PUBLIC_LIST, 1, NULL, search_proc);
        add_module_proc(HOOK_PROC, "Fserv", NULL, "*", MSG_LIST, 1, NULL, search_proc);
        add_module_proc(HOOK_PROC, "Fserv", NULL, "*", PUBLIC_OTHER_LIST, 1, NULL, search_proc);

	add_completion_type("fsload", 3, FILE_COMPLETION);

	add_timer(0, empty_string, get_dllint_var("fserv_time"), 1, impress_me, NULL, NULL, -1, "fserv");
	strcpy(FSstr, cparse(FS, NULL, NULL));
	put_it("%s %s", FSstr, convert_output_format("$0 v$1 by panasync.", "%s %s", fserv_version, AUTO_VERSION));
	sprintf(buffer, "$0+%s by panasync - $2 $3", fserv_version);
	fset_string_var(FORMAT_VERSION_FSET, buffer);
	statistics.starttime = time(NULL);

	sprintf(buffer, "%s/fserv.sav", get_string_var(CTOOLZ_DIR_VAR));
	fserv_read(buffer);

	put_it("%s for help with this fserv, /fshelp", FSstr);
	return 0;
}

