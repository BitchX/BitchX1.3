/*
 * output.c: handles a variety of tasks dealing with the output from the irc
 * program 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#include "irc.h"
static char cvsrevision[] = "$Id: output.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(output_c)
#include "struct.h"              
#include <sys/stat.h>

#include <stdarg.h>

#include "output.h"
#include "vars.h"
#include "input.h"
#include "ircaux.h"
#include "ircterm.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "server.h"
#include "hook.h"
#include "ctcp.h"
#include "log.h"
#include "misc.h"
#define MAIN_SOURCE
#include "modval.h"

	int	in_help = 0;

/* make this buffer *much* bigger than needed */

#define LARGE_BIG_BUFFER_SIZE BIG_BUFFER_SIZE * 10

static	char	*putbuf = NULL; /*[LARGE_BIG_BUFFER_SIZE + 1];*/
extern	LastMsg	last_servermsg[];

char three_stars[4] = "***";

/* unflash: sends a ^[c to the screen */
/* Must be defined to be useful, cause some vt100s really *do* reset when
   sent this command. >;-) */

/* functions which switch the character set on the console */
/* ibmpc is not available on the xterm */

void charset_ibmpc (void)
{
	fwrite("\033(U", 3, 1, current_ftarget);	/* switch to IBM code page 437 */
}

void charset_lat1 (void)
{
	fwrite("\033(B", 3, 1, current_ftarget);	/* switch to Latin-1 (ISO 8859-1) */
}

void charset_cst(void)
{
	fwrite("\033(K", 3, 1, current_ftarget); /* switch too user-defined */
}

/* currently not used. */


/* Now that you can send ansi sequences, this is much less inportant.. */
void unflash (void)
{
#if !defined(WINNT) && !defined(__EMX__)

#if defined(HARD_UNFLASH) && !defined(CHARSET_CUSTOM)
	fwrite("\033c", 5, 1, current_ftarget);		/* hard reset */
#else
	fwrite("\033)0", 6, 1, current_ftarget);		/* soft reset */
#endif

#if defined(LATIN1)
	charset_lat1();
#elif defined(CHARSET_CUSTOM)
	charset_cst();
#else
	charset_ibmpc();
#endif

#endif
}

/*
 * refresh_screen: Whenever the REFRESH_SCREEN function is activated, this
 * swoops into effect 
 */
void refresh_screen (unsigned char dumb, char *dumber)
{
extern int need_redraw;

#if !defined(WINNT) && !defined(__EMX__)
	term_clear_screen();
	unflash();
#else
	xterm_settitle();
	term_clear_screen();
#endif

#if 0
	for (tmp = screen_list; tmp; tmp = tmp->next)
		tmp->co = TI_cols, tmp->li = TI_lines;
#endif
	if (term_resize())
		recalculate_windows(current_window->screen);
	else
		redraw_all_windows();
	if (need_redraw)
		need_redraw = 0;
	update_all_windows();
	update_input(UPDATE_ALL);
}

/*
 * refresh_window_screen: Updates and redraws only the window's 
 * screen that was passed as a parameter. 
 */

#ifdef GUI
void refresh_window_screen(Window *window)
{
   xterm_settitle();
   recalculate_windows(current_window->screen);
   update_all_windows();
   update_input(UPDATE_ALL);
}
#endif

int init_output(void)
{
	if (!putbuf)
		putbuf = new_malloc(LARGE_BIG_BUFFER_SIZE+1);
	return 0;
}

/* init_windows:  */
int init_screen (void)
{
extern int term_initialized;
	term_initialized = 1;
	term_clear_screen();
	term_resize();
	create_new_screen();
	new_window(main_screen);
	update_all_windows();
	term_move_cursor(0, 0);
	return 0;
}

void put_echo (char *str)
{
	add_to_log(irclog_fp, 0, str, logfile_line_mangler);
	add_to_screen(str);
}


/*
 * put_it: the irc display routine.  Use this routine to display anything to
 * the main irc window.  It handles sending text to the display or stdout as
 * needed, add stuff to the lastlog and log file, etc.  Things NOT to do:
 * Dont send any text that contains \n, very unpredictable.  Tabs will also
 * screw things up.  The calling routing is responsible for not overwriting
 * the 1K buffer allocated.  
 *
 * For Ultrix machines, you can't call put_it() with floating point arguements.
 * It just doesn't work.  - phone, jan 1993.
 */
void BX_put_it(const char *format, ...)
{
	if (window_display && format)
	{
		va_list args;
		memset(putbuf, 0, 200);
		va_start (args, format);
		vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		if (*putbuf)
			put_echo(putbuf);
	}
}

/* This is an alternative form of put_it which writes three asterisks
 * before actually putting things out.
 */
void say (const char *format, ...)
{
int len = 0;
	if (window_display && format)
	{
		va_list args;
		va_start (args, format);
		if (thing_ansi)
			len = strlen(thing_ansi);
		else
			len = 3;
		vsnprintf(&(putbuf[len+1]), LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		strcpy(putbuf, thing_ansi?thing_ansi:three_stars);
		putbuf[len] = ' ';
		if (strip_ansi_in_echo) 
		{
			register char *ptr;
			for (ptr = putbuf + len; *ptr; ptr++)
				if (*ptr < 31 && *ptr > 13)
					if (*ptr != 15 && *ptr != 22)
						*ptr = (*ptr & 127) | 64;
		}
		put_echo(putbuf);
	}
}

void BX_bitchsay (const char *format, ...)
{
int len;
	if (window_display && format)
	{
		va_list args;
		va_start (args, format);
		sprintf(putbuf, "%s \002%s\002: ", thing_ansi?thing_ansi:three_stars, version);
		len = strlen(putbuf);
		vsnprintf(&(putbuf[len]), LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		if (strip_ansi_in_echo) 
		{
			register char *ptr;
			for (ptr = putbuf+len; *ptr; ptr++)
				if (*ptr < 31 && *ptr > 13)
					if (*ptr != 15 && *ptr != 22)
						*ptr = (*ptr & 127) | 64;
		}
		put_echo(putbuf);
	}
}

void	BX_yell(const char *format, ...)
{
	if (format)
	{
		va_list args;
		va_start (args, format);
		*putbuf = 0;
		vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		if (*putbuf && do_hook(YELL_LIST, "%s", putbuf))
			put_echo(putbuf);
	}
}


void	log_put_it (const char *topic, const char *format, ...)
{
	if (format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);

		in_help = 1;
		set_display_target(NULL, LOG_CURRENT);
		if (window_display)
			put_echo(putbuf);
		reset_display_target();
		in_help = 0;
	}
}

char *ov_server(int server)
{
	char *c;
	char *d;
	static char tmpstr[61];
	char *string = get_server_itsname(server);
	    
	if (!string || !*string)
		string = get_server_name(server);
	if (!string || !*string)
		return  empty_string;
	strmcpy(tmpstr, string, 60);
	if (!(c = strrchr(tmpstr,'.')))
		return(string);
	*c = 0;
	if (!(d = strrchr(tmpstr, '.'))) 
		d = ++c; /* Extract domain */
	d++;
	return(d);
}

void serversay(int save, int from_server, const char *format, ...)
{
	Window	*old_target_window = target_window;
	char 	servername[200];
	int	len = 0;	
	char	*out = NULL;
	if (get_int_var(OV_VAR))
		target_window = get_window_by_name("OPER_VIEW");
        if (window_display && format)
        {
		va_list args;
		va_start (args, format);
		vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		strmcpy(servername, convert_output_format(get_string_var(SERVER_PROMPT_VAR), "%s", ov_server(from_server)?ov_server(from_server):empty_string), 79);
		len = strlen(putbuf);
		out = alloca(strlen(servername)+len+5);
		len = strlen(servername);
		strcpy(out, servername); out[len] = ' '; out[len+1] = 0;
		strcat(out, putbuf);
		if (*out)
			put_echo(out);
	}
	target_window = old_target_window;
	if (save && out)
		add_last_type(&last_servermsg[0], MAX_LAST_MSG, NULL, NULL, NULL, out);
}
/*
 * Error is exactly like yell, except that if the error occured while
 * you were loading a script, it tells you where it happened.
 */
void 	error (const char *format, ...)
{
	dump_load_stack(0);
	if (format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		do_hook(YELL_LIST, "%s", putbuf);
		put_echo(putbuf);
	}
}
