/*
 * status.c: handles the status line updating, etc for IRCII 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: status.c 139 2011-09-07 10:13:18Z keaston $";
CVS_REVISION(status_c)
#include "struct.h"

#include "ircterm.h"
#include "status.h"
#include "server.h"
#include "vars.h"
#include "hook.h"
#include "input.h"
#include "commands.h"
#include "window.h"
#include "screen.h"
#include "mail.h"
#include "output.h"
#include "names.h"
#include "ircaux.h"
#include "misc.h"
#include "notify.h"
#include "hash2.h"
#include "cset.h"
#ifdef TRANSLATE
#include "translat.h"
#endif
#define MAIN_SOURCE
#include "modval.h"
#ifdef WANT_HEBREW
#include "hebrew.h"
#endif

#define MY_BUFFER 120

extern char *DCC_get_current_transfer (void);
extern	long	oper_kills;
extern	long	nick_collisions;

static	char	*convert_format (Window *, char *, int);
static	char	*status_nickname (Window *);
static	char	*status_query_nick (Window *);
static	char	*status_right_justify (Window *);
static	char	*status_chanop (Window *);
static	char	*status_channel (Window *);
static	char	*status_server (Window *);
static	char	*status_mode (Window *);
static	char	*status_umode (Window *);
static	char	*status_insert_mode (Window *);
static	char	*status_overwrite_mode (Window *);
static	char	*status_away (Window *);
static	char	*status_oper (Window *);
static	char	*status_users (Window *);
static	char	*status_user0s (Window *);
static	char	*status_user1s (Window *);
static	char	*status_user2s (Window *);
static	char	*status_user3s (Window *);
static	char	*status_user4s (Window *);
static	char	*status_user5s (Window *);
static	char	*status_user6s (Window *);
static	char	*status_user7s (Window *);
static	char	*status_user8s (Window *);
static	char	*status_user9s (Window *);
static	char	*status_user10s (Window *);
static	char	*status_user11s (Window *);
static	char	*status_user12s (Window *);
static	char	*status_user13s (Window *);
static	char	*status_user14s (Window *);
static	char	*status_user15s (Window *);
static	char	*status_user16s (Window *);
static	char	*status_user17s (Window *);
static	char	*status_user18s (Window *);
static	char	*status_user19s (Window *);
static	char	*status_user20s (Window *);
static	char	*status_user21s (Window *);
static	char	*status_user22s (Window *);
static	char	*status_user23s (Window *);
static	char	*status_user24s (Window *);
static	char	*status_user25s (Window *);
static	char	*status_user26s (Window *);
static	char	*status_user27s (Window *);
static	char	*status_user28s (Window *);
static	char	*status_user29s (Window *);
static	char	*status_user30s (Window *);
static	char	*status_user31s (Window *);
static	char	*status_user32s (Window *);
static	char	*status_user33s (Window *);
static	char	*status_user34s (Window *);
static	char	*status_user35s (Window *);
static	char	*status_user36s (Window *);
static	char	*status_user37s (Window *);
static	char	*status_user38s (Window *);
static	char	*status_user39s (Window *);
static	char	*status_lag   (Window *);
static	char	*status_dcc (Window *);
static	char	*status_oper_kills (Window *);
static	char	*status_msgcount (Window *);
static	char	*status_hold (Window *);
static	char	*status_version (Window *);
static	char	*status_clock (Window *);
static	char	*status_hold_lines (Window *);
static	char	*status_window (Window *);
static	char	*status_mail (Window *);
static	char	*status_refnum (Window *);
static	char	*status_topic (Window *);
static	char	*status_null_function (Window *);
static	char	*status_notify_windows (Window *);
static	char	*convert_sub_format (char *, char, char *);
static	char	*status_voice (Window *);
static	char	*status_cpu_saver_mode (Window *);
static	char	*status_dcccount (Window *);
static	char	*status_cdcccount (Window *);
static	char	*status_position (Window *);
static	char	*status_lastjoin (Window *);
static	char	*status_userlist (Window *);
static	char	*status_shitlist (Window *);
static	char	*status_nethack (Window *);
static	char	*status_aop (Window *);
static	char	*status_bitch (Window *);
static	char	*status_newserver (Window *);
static	char	*status_scrollback (Window *);
static	char	*status_percent (Window *);
static	char	*status_windowspec (Window *);
static	char	*status_halfop (Window *);
static	char	*status_notifyusers(Window *);
#define cparse(format, str) convert_output_format(fget_string_var(format), "%s", str)

#if 1
/*
 * This is how we store status line expandos.
 */
struct status_formats {
	int	map;
	char 	key;
	char	*(*callback_function)(Window *);
	char	**format_var;
	int	format_set;
};


char	*away_format = NULL,
	*hold_lines_format = NULL,
	*channel_format = NULL,
	*notify_format = NULL,
	*cpu_saver_format = NULL,
	*kill_format = NULL,
	*lag_format = NULL,
	*mail_format = NULL,
	*nick_format = NULL,
	*query_format = NULL,
	*server_format = NULL,
	*clock_format = NULL,
	*dcccount_format = NULL,
	*cdcc_format = NULL,
	*msgcount_format = NULL,
	*umode_format = NULL,
	*mode_format = NULL,
	*topic_format = NULL,
	*user_format = NULL;
	
/*
 * This is the list of possible expandos.  Note that you should not use
 * the '{' character, as it would be confusing.  It is already used for 
 * specifying the map.
 */
struct status_formats status_expandos[] = {
{ 0, 'A', status_away,           NULL,		STATUS_AWAY_WSET },
{ 0, 'B', status_hold_lines,     NULL,		STATUS_HOLD_LINES_WSET },
{ 0, 'C', status_channel,        NULL,		STATUS_CHANNEL_WSET },
{ 0, 'D', status_dcc, 	         NULL, 			-1 },
{ 0, 'E', status_scrollback,     NULL, 			-1 },
{ 0, 'F', status_notify_windows, NULL,		STATUS_NOTIFY_WSET },
{ 0, 'H', status_hold,		 NULL,			-1 },
{ 0, 'G', status_halfop,	 NULL,			-1 },
{ 0, 'I', status_insert_mode,    NULL,			-1 },
{ 0, 'J', status_cpu_saver_mode, NULL,		STATUS_CPU_SAVER_WSET },
{ 0, 'K', status_oper_kills,	 NULL,		STATUS_OPER_KILLS_WSET },
{ 0, 'L', status_lag, 		 NULL,		STATUS_LAG_WSET },

{ 0, 'M', status_mail,		 NULL,		STATUS_MAIL_WSET },
{ 0, 'N', status_nickname,	 NULL,		STATUS_NICKNAME_WSET },
{ 0, 'O', status_overwrite_mode, NULL,			-1 },
{ 0, 'P', status_position,       NULL,			-1 },
{ 0, 'Q', status_query_nick,     NULL,		STATUS_QUERY_WSET },
{ 0, 'R', status_refnum,         NULL, 			-1 },
{ 0, 'S', status_server,         NULL,     	STATUS_SERVER_WSET },
{ 0, 'T', status_clock,          NULL,      	STATUS_CLOCK_WSET },
{ 0, 'U', status_user0s,		 NULL, 			-1 },
{ 0, 'V', status_version,	 NULL, 			-1 },
{ 0, 'W', status_window,	 NULL, 			-1 },
{ 0, 'X', status_user1s,		 NULL, 			-1 },
{ 0, 'Y', status_user2s,		 NULL, 			-1 },
{ 0, 'Z', status_user3s,		 NULL, 			-1 },
{ 0, '&', status_dcccount,	 NULL,		STATUS_DCCCOUNT_WSET },
{ 0, '|', status_cdcccount,	 NULL,		STATUS_CDCCCOUNT_WSET },
{ 0, '^', status_msgcount,	 NULL,		STATUS_MSGCOUNT_WSET },
{ 0, '#', status_umode,		 NULL,	     	STATUS_UMODE_WSET },
{ 0, '%', status_percent,	 NULL, 			-1 },
{ 0, '*', status_oper,		 NULL, 			-1 },
{ 0, '+', status_mode,		 NULL,       	STATUS_MODE_WSET },
{ 0, '.', status_windowspec,	 NULL, 			-1 },
{ 0, '=', status_voice,		 NULL, 			-1 },
{ 0, '>', status_right_justify,	 NULL, 			-1 },
{ 0, '-', status_topic,		 NULL,		STATUS_TOPIC_WSET },
{ 0, '!', status_users,		 NULL,		STATUS_USERS_WSET },
{ 0, '@', status_chanop,	 NULL, 			-1 },
{ 0, '0', status_user0s,		 NULL, 			-1 },
{ 0, '1', status_user1s,		 NULL, 			-1 },
{ 0, '2', status_user2s,		 NULL, 			-1 },
{ 0, '3', status_user3s,		 NULL, 			-1 },
{ 0, '4', status_user4s,		 NULL, 			-1 },
{ 0, '5', status_user5s,		 NULL, 			-1 },
{ 0, '6', status_user6s,		 NULL, 			-1 },
{ 0, '7', status_user7s,		 NULL, 			-1 },
{ 0, '8', status_user8s,		 NULL, 			-1 },
{ 0, '9', status_user9s,		 NULL, 			-1 },
{ 0, 'f', status_shitlist,		 NULL,			-1 },
{ 0, 'a', status_aop,			 NULL,			-1 },
{ 0, 'b', status_bitch,			 NULL,			-1 },
{ 0, 'h', status_nethack,		 NULL,			-1 },
{ 0, 'l', status_lastjoin,		 NULL,			-1 },
{ 0, 'n', status_notifyusers,		 NULL,			-1 },
{ 0, 's', status_newserver,		 NULL,			-1 },
{ 0, 'u', status_userlist,		 NULL,			-1 },

{ 1, '0', status_user10s,		NULL,			-1 },
{ 1, '1', status_user11s,		NULL,			-1 },
{ 1, '2', status_user12s,		NULL,			-1 },
{ 1, '3', status_user13s,		NULL,			-1 },
{ 1, '4', status_user14s,		NULL,			-1 },
{ 1, '5', status_user15s,		NULL,			-1 },
{ 1, '6', status_user16s,		NULL,			-1 },
{ 1, '7', status_user17s,		NULL,			-1 },
{ 1, '8', status_user18s,		NULL,			-1 },
{ 1, '9', status_user19s,		NULL,			-1 },

{ 2, '0', status_user20s,		NULL,			-1 },
{ 2, '1', status_user21s,		NULL,			-1 },
{ 2, '2', status_user22s,		NULL,			-1 },
{ 2, '3', status_user23s,		NULL,			-1 },
{ 2, '4', status_user24s,		NULL,			-1 },
{ 2, '5', status_user25s,		NULL,			-1 },
{ 2, '6', status_user26s,		NULL,			-1 },
{ 2, '7', status_user27s,		NULL,			-1 },
{ 2, '8', status_user28s,		NULL,			-1 },
{ 2, '9', status_user29s,		NULL,			-1 },

{ 3, '0', status_user30s,		NULL,			-1 },
{ 3, '1', status_user31s,		NULL,			-1 },
{ 3, '2', status_user32s,		NULL,			-1 },
{ 3, '3', status_user33s,		NULL,			-1 },
{ 3, '4', status_user34s,		NULL,			-1 },
{ 3, '5', status_user35s,		NULL,			-1 },
{ 3, '6', status_user36s,		NULL,			-1 },
{ 3, '7', status_user37s,		NULL,			-1 },
{ 3, '8', status_user38s,		NULL,			-1 },
{ 3, '9', status_user39s,		NULL,			-1 },

};

#define NUMBER_OF_EXPANDOS (sizeof(status_expandos) / sizeof(struct status_formats))
#endif

void *default_status_output_function = make_status;

/*
 * convert_sub_format: This is used to convert the formats of the
 * sub-portions of the status line to a format statement specially designed
 * for that sub-portions.  convert_sub_format looks for a single occurence of
 * %c (where c is passed to the function). When found, it is replaced by "%s"
 * for use is a sprintf.  All other occurences of % followed by any other
 * character are left unchanged.  Only the first occurence of %c is
 * converted, all subsequence occurences are left unchanged.  This routine
 * mallocs the returned string. 
 */
static	char	* convert_sub_format(char *format, char c, char *padded)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	static	char	bletch[] = "%% ";
	char	*ptr = NULL;
	int	dont_got_it = 1;
	
	if (format == NULL)
		return (NULL);
	*buffer = (char) 0;
	memset(buffer, 0, sizeof(buffer));
	while (format)
	{
#if 0
/wset status_mode +#%+%
#endif
		if ((ptr = (char *) strchr(format, '%')) != NULL)
		{
			*ptr = (char) 0;
			strmcat(buffer, format, BIG_BUFFER_SIZE);
			*(ptr++) = '%';
			if ((*ptr == c)/* && dont_got_it*/)
			{
				dont_got_it = 0;
				if (*padded)
				{
					strmcat(buffer, "%", BIG_BUFFER_SIZE);
					strmcat(buffer, padded, BIG_BUFFER_SIZE);
					strmcat(buffer, "s", BIG_BUFFER_SIZE);
				}
				else
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
			}
			else if (*ptr == '<')
			{
				char *s = ptr + 1;
				while(*ptr && *ptr != '>') ptr++;				
				if (*ptr)
				{
					ptr++;
					if (!*ptr)
						continue;
					else if (*ptr == c)
					{

						strmcat(buffer, "%", BIG_BUFFER_SIZE);
						strmcat(buffer, s, (ptr - s));
						strmcat(buffer, "s", BIG_BUFFER_SIZE);
					}
				}
			
			}
			else
			{
				bletch[2] = *ptr;
				strmcat(buffer, bletch, BIG_BUFFER_SIZE);
				if (!*ptr)
				{
					format = ptr;
					continue;
				}
			}
			ptr++;
		}
		else
			strmcat(buffer, format, BIG_BUFFER_SIZE);
		format = ptr;
	}
	malloc_strcpy(&ptr, buffer);
	return (ptr);
}

static	char	*convert_format(Window *win, char *format, int k)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	padded[BIG_BUFFER_SIZE + 1];
	int	pos = 0;
	int	*cp;
	int	map;
	char	key;
	int	i;

	cp = &win->func_cnt[k];
	while (*format && pos < BIG_BUFFER_SIZE - 4)
	{
		*padded = 0;
		if (*format != '%')
		{
			buffer[pos++] = *format++;
			continue;
		}

		/* Its a % */
		map = 0;

		/* Find the map, if neccesary */
		if (*++format == '{')
		{
			char	*endptr;

			format++;
			map = strtoul(format, &endptr, 10);
			if (*endptr != '}')
			{
				/* Unrecognized */
				continue;
			}
			format = endptr + 1;
		}
		else if (*format == '<')
		{
			char *p = padded;
			format++;
			while(*format && *format != '>') 
				*p++ = *format++;
			*p = 0;
			format++;
		}
		key = *format++;

		/* Choke once we get to the maximum number of expandos */
		if (*cp >= MAX_FUNCTIONS)
			continue;

		for (i = 0; i < NUMBER_OF_EXPANDOS; i++)
		{
			char **s = NULL;
			if (status_expandos[i].map != map ||
			    status_expandos[i].key != key)
				continue;

			if (status_expandos[i].format_set != -1)
			{
				s = get_wset_format_var_address(win->wset, status_expandos[i].format_set);
				if (s)
					new_free(s);
				if (s)
					*s = convert_sub_format(get_wset_string_var(win->wset, status_expandos[i].format_set), key, padded);
			}
			buffer[pos++] = '%';
			buffer[pos++] = 's';

			win->status_func[k][(*cp)++] = 
				status_expandos[i].callback_function;
			break;
		}
	}

	win->func_cnt[k] = *cp;
	buffer[pos] = 0;
	return m_strdup(buffer);
}

void fix_status_buffer(Window *win, char *buffer, int in_status)
{
unsigned char	rhs_buffer[3*BIG_BUFFER_SIZE + 1];
unsigned char	lhs_buffer[3*BIG_BUFFER_SIZE + 1];
unsigned char	lhs_fillchar[6],
		rhs_fillchar[6],
		*fillchar = lhs_fillchar,
		*lhp = lhs_buffer,
		*rhp = rhs_buffer,
		*cp,
		*start_rhs = 0,
		*str = NULL, *ptr = NULL;
int		in_rhs = 0,
		pr_lhs = 0,
		pr_rhs = 0,
		*prc = &pr_lhs;

	lhs_buffer[0] = 0;
	rhs_buffer[0] = 0;
	if (!buffer || !*buffer)
		return;
	if (get_int_var(STATUS_DOES_EXPANDOS_VAR))
	{
		int  af = 0;
		char *stuff;
		Window *old = current_window;
		current_window = win;
		stuff = expand_alias(buffer, empty_string, &af, NULL);
		strmcpy(buffer, stuff, BIG_BUFFER_SIZE);
		new_free(&stuff);
		current_window = old;
	}
	/*
	 * This converts away any ansi codes in the status line
	 * in accordance with the current settings.  This leaves us
	 * with nothing but logical characters, which are then easy
	 * to count. :-)
	 */
	str = strip_ansi(buffer);

	/*
	 * Count out the characters.
	 * Walk the entire string, looking for nonprintable
	 * characters.  We count all the printable characters
	 * on both sides of the %> tag.
	 */
	ptr = str;
	cp = lhp;
	lhs_buffer[0] = rhs_buffer[0] = 0;
	while (*ptr)
	{
		/*
		 * The FIRST such %> tag is used.
		 * Using multiple %>s is bogus.
		 */
		if (*ptr == '\f' && start_rhs == NULL)
		{
			ptr++;
			start_rhs = ptr;
			fillchar = rhs_fillchar;
			in_rhs = 1;
			*cp = 0;
			cp = rhp;
			prc = &pr_rhs;
		}
		/*
		 * Skip over color attributes if we're not
		 * doing color.
		 */
		else if (*ptr == '\003')
		{
			const u_char *end = skip_ctl_c_seq(ptr, NULL, NULL, 0);
			while (ptr < end)
				*cp++ = *ptr++;
		}
		/*
		 * If we have a ROM character code here, we need to
		 * copy it to the output buffer, as well as the fill
		 * character, and increment the printable counter by
		 * only 1.
		 */
		else if (*ptr == ROM_CHAR)
		{
			fillchar[0] = *cp++ = *ptr++;
			fillchar[1] = *cp++ = *ptr++;
			fillchar[2] = *cp++ = *ptr++;
			fillchar[3] = *cp++ = *ptr++;
			fillchar[4] = 0;
			*prc += 1;
		}
		/*
		 * Is it NOT a printable character?
		 */
		else if ((*ptr == REV_TOG) || (*ptr == UND_TOG) ||
			 (*ptr == ALL_OFF) || (*ptr == BOLD_TOG) ||
			 (*ptr == BLINK_TOG))
				*cp++ = *ptr++;
#if 0
		else if (*ptr == 9)	/* TAB */
		{
			fillchar[0] = ' ';
			fillchar[1] = 0;
			do
				*cp++ = ' ';
			while (++(*prc) % 8);
			ptr++;
		}
#endif
		/*
		 * So it is a printable character.
		 * Or maybe its a tab. ;-)
		 */
		else
		{
			*prc += 1;
			fillchar[0] = *cp++ = *ptr++;
			fillchar[1] = 0;
		}
		/*
		 * Dont allow more than CO printable characters
		 */
		if (pr_lhs + pr_rhs >= win->screen->co)
		{
			*cp = 0;
			break;
		}
	}
	*cp = 0;
#if 0
	/* What will we be filling with? */
	if (get_int_var(STATUS_NO_REPEAT_VAR))
	{
		lhs_fillchar[0] = ' ';
		lhs_fillchar[1] = 0;
		rhs_fillchar[0] = ' ';
		rhs_fillchar[1] = 0;
	}

	/*
	 * Now if we have a rhs, then we have to adjust it.
	 */
	if (start_rhs)
	{
		int numf = 0;
			numf = win->screen->co - pr_lhs - pr_rhs  -1;
		while (numf-- >= 0)
			strmcat(lhs_buffer, lhs_fillchar, 
					BIG_BUFFER_SIZE);
	}
	/*
	 * No rhs?  If the user wants us to pad it out, do so.
	 */
	else if (get_int_var(FULL_STATUS_LINE_VAR))
	{
		int chars = win->screen->co - pr_lhs - 1;
		while (chars-- >= 0)
			strmcat(lhs_buffer, lhs_fillchar, 
					BIG_BUFFER_SIZE);
	}
#endif
	strcpy(buffer, lhs_buffer);
	strmcat(buffer, rhs_buffer, BIG_BUFFER_SIZE);
	new_free(&str);
}

char	*stat_convert_format(Window *win, char *form)
{
unsigned char	buffer[2 * BIG_BUFFER_SIZE + 1];
	 char   *ptr = form;
	 int	pos = 0;
	 int	map;
	 char	key;
	 int	i;


	if (!form || !*form)
		return m_strdup(empty_string);	
	*buffer = 0;

	while (*ptr && pos < (2 * BIG_BUFFER_SIZE) - 4)
	{
		if (*ptr != '%')
		{
			buffer[pos++] = *ptr++;
			continue;
		}

		/* Its a % */
		map = 0;

		/* Find the map, if neccesary */
		if (*++ptr == '{')
		{
			char	*endptr;

			ptr++;
			map = strtoul(ptr, &endptr, 10);
			if (*endptr != '}')
			{
				/* Unrecognized */
				continue;
			}
			ptr = endptr + 1;
		}

		key = *ptr++;

		/* Choke once we get to the maximum number of expandos */
		for (i = 0; i < NUMBER_OF_EXPANDOS; i++)
		{
			if (status_expandos[i].map != map || status_expandos[i].key != key)
				continue;
			strmcat(buffer, (status_expandos[i].callback_function)(win), BIG_BUFFER_SIZE);
			pos = strlen(buffer);
			break;
		}
	}

	buffer[pos] = 0;
	fix_status_buffer(win, buffer, 0);
	return m_strdup(buffer);
}

void BX_build_status(Window *win, char *format, int unused)
{
	int	i, k;
	char	*f = NULL;
	int	ofs = from_server;
	if (!win)
		return;
	if (win->server > -1)
		from_server = win->server;
	for (k = 0; k < 3; k++) 
	{
		if (!win->wset)
			continue;
		new_free(&(win->wset->status_format[k]));
		win->func_cnt[k] = 0;
		switch(k)
		{
			case 0:
				f = win->wset->format_status[1];
				break;
			case 1:
				f = win->wset->format_status[2];
				break;
			case 2:
				f = win->wset->format_status[3];
				break;
		}			

		if (f)
			win->wset->status_format[k] = convert_format(win, f, k);

		for (i = win->func_cnt[k]; i < MAX_FUNCTIONS; i++)
			win->status_func[k][i] = status_null_function;
	}
	update_all_status(win, NULL, 0);
	from_server = ofs;
}

void make_status(Window *win)
{
	u_char	buffer	    [BIG_BUFFER_SIZE + 1];
	u_char	lhs_buffer  [BIG_BUFFER_SIZE + 1];
	u_char	rhs_buffer  [BIG_BUFFER_SIZE + 1];
	char	*func_value[MAX_FUNCTIONS+10] = {NULL};
	u_char	*ptr;
	
	int	len = 1,
		status_line,
		ansi = get_int_var(DISPLAY_ANSI_VAR);

	/* The second status line is only displayed in the bottom window
	 * and should always be displayed, no matter what SHOW_STATUS_ALL
	 * is set to - krys
	 */
	for (status_line = 0 ; status_line < 1+win->double_status + win->status_lines; status_line++)
	{
		u_char	lhs_fillchar[6],
			rhs_fillchar[6],
			*fillchar = lhs_fillchar,
			*lhp = lhs_buffer,
			*rhp = rhs_buffer,
			*cp,
			*start_rhs = 0,
			*str;
		int	in_rhs = 0,
			pr_lhs = 0,
			pr_rhs = 0,
			line = status_line,
			*prc = &pr_lhs, 
			i;

		fillchar[0] = fillchar[1] = 0;

		if (!win->wset || !win->wset->status_format[line])
			continue;
			
		for (i = 0; i < MAX_FUNCTIONS; i++)
			func_value[i] = (win->status_func[line][i]) (win);
		len = 1;
		
		if (get_int_var(REVERSE_STATUS_VAR))
			buffer[0] = get_int_var(REVERSE_STATUS_VAR) ? REV_TOG : ' ';
		else
			len = 0;
		str = &buffer[len];                                        
		snprintf(str, BIG_BUFFER_SIZE - 1, 
			win->wset->status_format[line],
			func_value[0], func_value[1], func_value[2],
			func_value[3], func_value[4], func_value[5],
			func_value[6], func_value[7], func_value[8],
			func_value[9], func_value[10], func_value[11],
			func_value[12], func_value[13], func_value[14],
			func_value[15], func_value[16], func_value[17],
			func_value[18], func_value[19], func_value[20],
			func_value[21], func_value[22], func_value[23],
			func_value[24], func_value[25], func_value[26],
			func_value[27], func_value[28], func_value[29],
			func_value[30], func_value[31],func_value[32],
			func_value[33], func_value[34],func_value[35],
			func_value[36], func_value[37],func_value[38]);

			/*  Patched 26-Mar-93 by Aiken
			 *  make_window now right-justifies everything 
			 *  after a %>
			 *  it's also more efficient.
			 */

		/*
		 * If the user wants us to, pass the status bar through the
		 * expander to pick any variables/function calls.
		 * This is horribly expensive, but what do i care if you
		 * want to waste cpu cycles? ;-)
		 */
		if (get_int_var(STATUS_DOES_EXPANDOS_VAR))
		{
			int  af = 0;

			str = expand_alias(buffer, empty_string, &af, NULL);
			strmcpy(buffer, str, BIG_BUFFER_SIZE);
			new_free(&str);
		}

		/*
		 * This converts away any ansi codes in the status line
		 * in accordance with the current settings.  This leaves us
		 * with nothing but logical characters, which are then easy
		 * to count. :-)
		 */
		str = strip_ansi(buffer);

		/*
		 * Count out the characters.
		 * Walk the entire string, looking for nonprintable
		 * characters.  We count all the printable characters
		 * on both sides of the %> tag.
		 */
		ptr = str;
		cp = lhp;
		lhs_buffer[0] = rhs_buffer[0] = 0;

		while (*ptr)
		{
			/*
			 * The FIRST such %> tag is used.
			 * Using multiple %>s is bogus.
			 */
			if (*ptr == '\f' && start_rhs == NULL)
			{
				ptr++;
				start_rhs = ptr;
				fillchar = rhs_fillchar;
				in_rhs = 1;
				*cp = 0;
				cp = rhp;
				prc = &pr_rhs;
			}

			/*
			 * Skip over color attributes if we're not
			 * doing color.
			 */
			else if (*ptr == '\003')
			{
				const u_char *end = skip_ctl_c_seq(ptr, NULL, NULL, 0);
				while (ptr < end)
					*cp++ = *ptr++;
			}

			/*
			 * If we have a ROM character code here, we need to
			 * copy it to the output buffer, as well as the fill
			 * character, and increment the printable counter by
			 * only 1.
			 */
			else if (*ptr == ROM_CHAR)
			{
				fillchar[0] = *cp++ = *ptr++;
				fillchar[1] = *cp++ = *ptr++;
				fillchar[2] = *cp++ = *ptr++;
				fillchar[3] = *cp++ = *ptr++;
				fillchar[4] = 0;
				*prc += 1;
			}

			/*
			 * Is it NOT a printable character?
			 */
			else if ((*ptr == REV_TOG) || (*ptr == UND_TOG) ||
				 (*ptr == ALL_OFF) || (*ptr == BOLD_TOG) ||
				 (*ptr == BLINK_TOG))
					*cp++ = *ptr++;
#if 0
			else if (*ptr == 9)	/* TAB */
			{
				fillchar[0] = ' ';
				fillchar[1] = 0;
				do
					*cp++ = ' ';
				while (++(*prc) % 8);
				ptr++;
			}
#endif
			/*
			 * So it is a printable character.
			 * Or maybe its a tab. ;-)
			 */
			else
			{
				*prc += 1;
				fillchar[0] = *cp++ = *ptr++;
				fillchar[1] = 0;
			}

			/*
			 * Dont allow more than CO printable characters
			 */
			if (pr_lhs + pr_rhs >= win->screen->co)
			{
				*cp = 0;
				break;
			}
		}
		*cp = 0;

		/* What will we be filling with? */
		if (get_int_var(STATUS_NO_REPEAT_VAR))
		{
			lhs_fillchar[0] = ' ';
			lhs_fillchar[1] = 0;
			rhs_fillchar[0] = ' ';
			rhs_fillchar[1] = 0;
		}

		/*
		 * Now if we have a rhs, then we have to adjust it.
		 */
		if (start_rhs)
		{
			int numf = 0;

			numf = win->screen->co - pr_lhs - pr_rhs  -1;
			while (numf-- >= 0)
				strmcat(lhs_buffer, lhs_fillchar, 
						BIG_BUFFER_SIZE);
		}

		/*
		 * No rhs?  If the user wants us to pad it out, do so.
		 */
		else if (get_int_var(FULL_STATUS_LINE_VAR))
		{
			int chars = win->screen->co - pr_lhs - 1;

			while (chars-- >= 0)
				strmcat(lhs_buffer, lhs_fillchar, 
						BIG_BUFFER_SIZE);
		}

		strcpy(buffer, lhs_buffer);
		strmcat(buffer, rhs_buffer, BIG_BUFFER_SIZE);
		strmcat(buffer, ALL_OFF_STR, BIG_BUFFER_SIZE);
		new_free(&str);

		do_hook(STATUS_UPDATE_LIST, "%d %d %s", 
			win->refnum, 
			status_line, 
			buffer);
		if (dumb_mode)
		{
			/* lets see what happens here */
			continue;
		}
		
		if (!win->wset->status_line[status_line] ||
			strcmp(buffer, win->wset->status_line[status_line]))

		{
			char *st = NULL;
			malloc_strcpy(&win->wset->status_line[status_line], buffer);
			output_screen = win->screen;
			st = cparse((line==3)?FORMAT_STATUS3_FSET:(line==2)?FORMAT_STATUS2_FSET:(line==1)?FORMAT_STATUS1_FSET:FORMAT_STATUS_FSET, buffer);
			if (!ansi)
				st = stripansicodes(st);
			if (win->status_lines && (line == win->double_status+win->status_lines) && win->status_split)
				term_move_cursor(0,win->top);
			else if (win->status_lines && !win->status_split)
				term_move_cursor(0,win->bottom+status_line-win->status_lines);
			else
				term_move_cursor(0,win->bottom+status_line);

			output_line(st);
			cursor_in_display(win);
			term_bold_off();
		} 
	}
	cursor_to_input();
}


/* Some useful macros */
/*
 * This is used to get the current window on a window's screen
 */
#define CURRENT_WINDOW window->screen->current_window

/*
 * This tests to see if the window IS the current window on its screen
 */
#define IS_CURRENT_WINDOW (window->screen->current_window == window)

/*
 * This tests to see if all expandoes are to appear in all status bars
 */
#define SHOW_ALL_WINDOWS (get_int_var(SHOW_STATUS_ALL_VAR))

/*
 * "Current-type" window expandoes occur only on the current window for a 
 * screen.  However, if /set show_status_all is on, then ALL windows act as
 * "Current-type" windows.
 */
#define DISPLAY_ON_WINDOW (IS_CURRENT_WINDOW || SHOW_ALL_WINDOWS)

#define RETURN_EMPTY  return empty_string


static	char	*status_nickname(Window *window)
{
static char my_buffer[MY_BUFFER/2+1];
	snprintf(my_buffer, MY_BUFFER/2, window->wset->nick_format, get_server_nickname(window->server));
	return my_buffer;
}

static	char	*status_server(Window *window)
{
	char	*name;
static	char	my_buffer[MY_BUFFER+1];
	if (connected_to_server)
	{
		if (window->server != -1)
		{
			if (window->wset->server_format)
			{
				if (!(name = get_server_itsname(window->server)))
					name = get_server_name(window->server);
				snprintf(my_buffer, MY_BUFFER, window->wset->server_format, name);
			}
			else
				RETURN_EMPTY;
		}
		else
			strcpy(my_buffer, " No Server");
	}
	else
		RETURN_EMPTY;
	return my_buffer;
}

static	char	*status_query_nick(Window *window)
{
static	char my_buffer[BIG_BUFFER_SIZE+1];

	if (window->query_nick && window->wset->query_format)
	{
		snprintf(my_buffer, BIG_BUFFER_SIZE, window->wset->query_format, window->query_nick);
		return my_buffer;
	}
	else
		RETURN_EMPTY;
}

static	char	*status_right_justify(Window *window)
{
static	char	my_buffer[] = "\f";

	return my_buffer;
}

static	char	*status_notify_windows(Window *window)
{
	int	doneone = 0;
	char	buf2[MY_BUFFER+2];
static	char	my_buffer[MY_BUFFER/2+1];
	if (get_int_var(SHOW_STATUS_ALL_VAR) || window == window->screen->current_window)
	{
		*buf2='\0';
		window = NULL;
		while ((traverse_all_windows(&window)))
		{
			if (window->miscflags & WINDOW_NOTIFIED)
			{
				if (doneone++)
					strmcat(buf2, ",", MY_BUFFER/2);
				strmcat(buf2, ltoa(window->refnum), MY_BUFFER/2);
			}
		}
	}
	if (doneone && window->wset->notify_format)
	{
		snprintf(my_buffer, MY_BUFFER/2, window->wset->notify_format, buf2);
		return (my_buffer);
	}
	RETURN_EMPTY;
}

static	char	*status_clock(Window *window)
{
static	char	my_buf[MY_BUFFER+1];

	if ((get_int_var(CLOCK_VAR) && window->wset->clock_format)  &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
		snprintf(my_buf, MY_BUFFER, window->wset->clock_format, update_clock(GET_TIME));
	else
		RETURN_EMPTY;
	return my_buf;
}

static	char	*status_mode(Window *window)
{
char		*mode = NULL;
static  char	my_buffer[MY_BUFFER+1];
	if (window->current_channel)
	{
		mode = get_channel_mode(window->current_channel,window->server);
		if (mode && *mode && window->wset->mode_format)
		{
			if (get_int_var(STATUS_DOES_EXPANDOS_VAR))
			{
				char *mode2 = alloca(strlen(mode) * 2 + 1);
				double_quote(mode, "$", mode2);
				mode = mode2;
			}
			snprintf(my_buffer, MY_BUFFER, window->wset->mode_format, mode);
		}
		else
			RETURN_EMPTY;
	} else
		RETURN_EMPTY;
	return my_buffer;
}


static	char	*status_umode(Window *window)
{
	char	localbuf[MY_BUFFER+1];
static char my_buffer[MY_BUFFER/2+1];

	if ((connected_to_server) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		*localbuf = 0;
	else {
		if (window->server > -1)
			strmcpy(localbuf, get_umode(window->server), MY_BUFFER);
		else
			*localbuf = 0;
	}
	
	if (*localbuf && window->wset->umode_format)
		snprintf(my_buffer, MY_BUFFER/2, window->wset->umode_format, localbuf);
	else
		RETURN_EMPTY;
	return my_buffer;
}

static	char	*status_chanop(Window *window)
{
    char	*text;

	if (window->current_channel && 
        get_channel_oper(window->current_channel, window->server) && 
        (text = get_wset_string_var(window->wset, STATUS_CHANOP_WSET)))
        return text;

    RETURN_EMPTY;
}
static	char	*status_halfop(Window *window)
{
    char	*text;

    if (window->current_channel && 
        get_channel_halfop(window->current_channel, window->server) &&
        (text = get_wset_string_var(window->wset, STATUS_HALFOP_WSET)))
        return text;

    RETURN_EMPTY;
}


static	char	*status_hold_lines(Window *window)
{
	int	num;
static  char	my_buffer[MY_BUFFER/2+1];
	
	if ((num = (window->lines_held /10) * 10))
	{
		snprintf(my_buffer, MY_BUFFER/2, window->wset->hold_lines_format, ltoa(num));
		return(my_buffer);
	}
	RETURN_EMPTY;
}

static	char	*status_msgcount(Window *window)
{
static  char	my_buffer[MY_BUFFER/2+1];

	if (get_int_var(MSGCOUNT_VAR) && window->wset->msgcount_format)
	{
		snprintf(my_buffer, MY_BUFFER/2, window->wset->msgcount_format, ltoa(get_int_var(MSGCOUNT_VAR)));
		return my_buffer;
	}
	RETURN_EMPTY;
}

static	char	*status_channel(Window *window)
{
	char	channel[IRCD_BUFFER_SIZE + 1];
static	char	my_buffer[IRCD_BUFFER_SIZE + 1];

	if (window->current_channel/* && chan_is_connected(s, window->server)*/)
	{
		int num;
		if (get_int_var(HIDE_PRIVATE_CHANNELS_VAR) &&
		    is_channel_mode(window->current_channel,
				MODE_PRIVATE | MODE_SECRET,
				window->server))
			strmcpy(channel, "*private*", IRCD_BUFFER_SIZE);
		else
			strmcpy(channel, window->current_channel, IRCD_BUFFER_SIZE);

		#ifdef WANT_HEBREW
		if (get_int_var(HEBREW_TOGGLE_VAR))
			hebrew_process((char *)&channel);
		#endif

		if ((num = get_int_var(CHANNEL_NAME_WIDTH_VAR)) &&
		    ((int) strlen(channel) > num))
			channel[num] = (char) 0;
		snprintf(my_buffer, IRCD_BUFFER_SIZE, window->wset->channel_format, channel);
		return my_buffer;
	}
	RETURN_EMPTY;
}

static	char	*status_mail(Window *window)
{
	char	*number;
static	char	my_buffer[MY_BUFFER/2+1];

	if (window && (get_int_var(MAIL_VAR) && (number = check_mail()) && window->wset->mail_format) &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
	{
		snprintf(my_buffer, MY_BUFFER/2, window->wset->mail_format, number);
		return my_buffer;
	}
	RETURN_EMPTY;
}

static	char	*status_insert_mode(Window *window)
{
char	*text;

	if (get_int_var(INSERT_MODE_VAR) && (get_int_var(SHOW_STATUS_ALL_VAR)
	    || (window->screen->current_window == window)))
		if ((text = get_string_var(STATUS_INSERT_VAR)))
			return text;
	RETURN_EMPTY;
}

static	char	*status_overwrite_mode(Window *window)
{
char	*text;

	if (!get_int_var(INSERT_MODE_VAR) && (get_int_var(SHOW_STATUS_ALL_VAR)
	    || (window->screen->current_window == window)))
	{
	    if ((text = get_string_var(STATUS_OVERWRITE_VAR)))
		return text;
	}
	RETURN_EMPTY;
}

static	char	*status_away(Window *window)
{
static char	my_buffer[MY_BUFFER+1];

	if (window && connected_to_server && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		RETURN_EMPTY;
	else if (window)
	{
		if (window->server != -1 && get_server_away(window->server))
		{
			snprintf(my_buffer, MY_BUFFER, window->wset->away_format, ltoa(get_int_var(MSGCOUNT_VAR)));
			return my_buffer;
		}
		else
			RETURN_EMPTY;
	}
	RETURN_EMPTY;
}


/*
 * This is a generic status_userX variable.  All of these go to the
 * current-type window, although i think they should go to all windows.
 */
#define STATUS_VAR(x) \
static	char	*status_user ## x ## s (Window *window)			\
{									\
	char	*text;							\
									\
	if ((text = get_wset_string_var(window->wset, STATUS_USER ## x ## _WSET)) && 	\
	     DISPLAY_ON_WINDOW)						\
		return text;						\
	else								\
		RETURN_EMPTY;					\
}

STATUS_VAR(0)
STATUS_VAR(1)
STATUS_VAR(2)
STATUS_VAR(3)
STATUS_VAR(4)
STATUS_VAR(5)
STATUS_VAR(6)
STATUS_VAR(7)
STATUS_VAR(8)
STATUS_VAR(9)
STATUS_VAR(10)
STATUS_VAR(11)
STATUS_VAR(12)
STATUS_VAR(13)
STATUS_VAR(14)
STATUS_VAR(15)
STATUS_VAR(16)
STATUS_VAR(17)
STATUS_VAR(18)
STATUS_VAR(19)
STATUS_VAR(20)
STATUS_VAR(21)
STATUS_VAR(22)
STATUS_VAR(23)
STATUS_VAR(24)
STATUS_VAR(25)
STATUS_VAR(26)
STATUS_VAR(27)
STATUS_VAR(28)
STATUS_VAR(29)
STATUS_VAR(30)
STATUS_VAR(31)
STATUS_VAR(32)
STATUS_VAR(33)
STATUS_VAR(34)
STATUS_VAR(35)
STATUS_VAR(36)
STATUS_VAR(37)
STATUS_VAR(38)
STATUS_VAR(39)

static	char	*status_hold(Window *window)
{
char	*text;

	if (window->holding_something && (text = get_wset_string_var(window->wset, STATUS_HOLD_WSET)))
		return(text);
	RETURN_EMPTY;
}

static	char	*status_lag (Window *window)
{
static  char	my_buffer[MY_BUFFER/2+1];
	if ((window->server > -1) && window->wset->lag_format)
	{
		if (get_server_lag(window->server) > -1)
		{
			char p[40];
			sprintf(p, "%2d",get_server_lag(window->server)); 
			snprintf(my_buffer,MY_BUFFER/2, window->wset->lag_format, p);
		}
		else
			snprintf(my_buffer, MY_BUFFER/2, window->wset->lag_format, "??");
		return(my_buffer);
	}
	RETURN_EMPTY;
}

static	char	*status_topic (Window *window)
{
static  char	my_buffer[MY_BUFFER+41];
	if (window && window->current_channel && window->wset->topic_format)
	{
		ChannelList *chan;
		if ((chan = lookup_channel(window->current_channel, window->server, 0)))
		{
			char *t;
			if ((t = chan->topic))
			{
				char *t2 = alloca(strlen(t) * 2 + 1 );
				if (get_int_var(STATUS_DOES_EXPANDOS_VAR))
					double_quote(t, "()[]$\"", t2);
				else
					strcpy(t2, t);
				snprintf(my_buffer, MY_BUFFER, window->wset->topic_format, stripansicodes(t2));
			}
			else
				strmcpy(my_buffer, "No Topic", MY_BUFFER);
			return(my_buffer);
		}
	}
	RETURN_EMPTY;
}

static	char	*status_oper(Window *window)
{
char	*text;

	if (get_server_operator(window->server) &&
			(text = get_string_var(STATUS_OPER_VAR)) &&
			(get_int_var(SHOW_STATUS_ALL_VAR) ||
			connected_to_server || 
			(window->screen->current_window == window)))
		return(text);
	RETURN_EMPTY;
}

static	char	*status_voice(Window *window)
{
char	*text;
	if (window->current_channel &&
	    get_channel_voice(window->current_channel, window->server) &&
	    (text = get_wset_string_var(window->wset, STATUS_VOICE_WSET)))
		return text;
	RETURN_EMPTY;
}

static	char	*status_window(Window *window)
{
char	*text;
	if ((number_of_windows_on_screen(window) > 1) && (window->screen->current_window == window) &&
	    (text = get_wset_string_var(window->wset, STATUS_WINDOW_WSET)))
		return(text);
	RETURN_EMPTY;
}

static	char	*status_refnum(Window *window)
{
static char my_buffer[MY_BUFFER/3+1];
	strmcpy(my_buffer, window->name ? window->name : ltoa(window->refnum), MY_BUFFER/3);
	return (my_buffer);
}

static	char	*status_version(Window *window)
{
	if ((connected_to_server) && !get_int_var(SHOW_STATUS_ALL_VAR)
	    && (window->screen->current_window != window))
		return(empty_string);
	return ((char *)irc_version);
}

static	char	* status_oper_kills (Window *window)
{
static char my_buffer[MY_BUFFER+1];
	if (window->wset->kills_format && (nick_collisions || oper_kills))
	{
		char tmp[30];
		snprintf(tmp, 29, "%ld", nick_collisions); 
		snprintf(my_buffer, MY_BUFFER, window->wset->kills_format, tmp, ltoa(oper_kills));
		return my_buffer;
	}
	RETURN_EMPTY;	
}

static char *status_dcccount (Window *window)
{
extern int get_count_stat, send_count_stat;
static char my_buffer[2 * MY_BUFFER+1];
	if (window->wset->dcccount_format && send_count_stat)
	{
		char tmp[30];
		strcpy(tmp, ltoa(send_count_stat));
		snprintf(my_buffer, 2 * MY_BUFFER, window->wset->dcccount_format, ltoa(get_count_stat), tmp);
		return my_buffer;
	}
	RETURN_EMPTY;
}

static char *status_cdcccount (Window *window)
{
#ifdef WANT_CDCC
extern int cdcc_numpacks, send_numpacks;
static char my_buffer[2 * MY_BUFFER+1];
	if (window->wset->cdcc_format && cdcc_numpacks)
	{
		char tmp[30];
		strcpy(tmp, ltoa(cdcc_numpacks));
		snprintf(my_buffer, 2 * MY_BUFFER, window->wset->cdcc_format, ltoa(send_numpacks), tmp);
		return my_buffer;
	}
#endif
	RETURN_EMPTY;
}

static char *status_cpu_saver_mode (Window *window)
{
static char my_buffer[MY_BUFFER/2+1];
	if (cpu_saver && window->wset->cpu_saver_format)
	{
		snprintf(my_buffer, MY_BUFFER/2, window->wset->cpu_saver_format, "cpu");
		return my_buffer;
	}

	RETURN_EMPTY;
}

static	char	* status_users (Window *window)
{
static char my_buffer[MY_BUFFER * 2 + 1];
ChannelList *chan;
NickList *nick;
int serv = window->server;
	if (window->server != -1 && window->wset->status_users_format)
	{
		if ((chan = prepare_command(&serv, NULL, PC_SILENT)))
		{
			int ops = 0, nonops = 0, voice = 0, ircop = 0, friends = 0;
			char buff[40], buff1[40], buff2[40], buff3[40], buff4[40];
			 
			for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
			{
				if (nick_isop(nick))
					ops++;
				else
					nonops++;
				if (nick_isvoice(nick))
					voice++;
				if (nick_isircop(nick))
					ircop++;
				if (nick->userlist)
					friends++;
			}
			strcpy(buff, ltoa(ops)); 
			strcpy(buff1, ltoa(nonops));
			strcpy(buff2,ltoa(ircop)); 
			strcpy(buff3, ltoa(voice));
			strcpy(buff4, ltoa(friends));
			snprintf(my_buffer, MY_BUFFER*2, window->wset->status_users_format, buff, buff1, buff2, buff3, buff4);
			return my_buffer;
		}
	}
	RETURN_EMPTY;	
}

static	char	*status_null_function(Window *window)
{
	RETURN_EMPTY;
}


static char *status_dcc(Window *window)
{
	if (window->window_level & LOG_DCC || !window->window_level)
		return(DCC_get_current_transfer());
	RETURN_EMPTY;
}

static char *status_position (Window *window)
{
static char my_buffer[MY_BUFFER/2+1];

	snprintf(my_buffer, MY_BUFFER/2, "(%d-%d)", window->lines_scrolled_back,
					window->distance_from_display);
	return my_buffer;
}
 
static	char	*status_userlist (Window *window)
{
ChannelList *chan;
int serv = window->server;
static char my_buffer[3] = "\0";
	if (window->server != -1)
	{
		if ((chan = prepare_command(&serv, NULL, PC_SILENT)))
		{
			my_buffer[0] = chan->csets->set_userlist ? 'U':'u';
			return my_buffer;
		}
	}
	RETURN_EMPTY;
}

static	char	*status_shitlist (Window *window)
{
ChannelList *chan;
int serv = window->server;
static char my_buffer[3] = "\0";
	if (window->server != -1)
	{
		if ((chan = prepare_command(&serv, NULL, PC_SILENT)))
		{
			my_buffer[0] = chan->csets->set_shitlist ? 'S':'s';
			return my_buffer;
		}
	}
	RETURN_EMPTY;
}

static	char	*status_nethack (Window *window)
{
ChannelList *chan;
int serv = window->server;
static char my_buffer[3] = "\0";
	if (window->server != -1)
	{
		if ((chan = prepare_command(&serv, NULL, PC_SILENT)))
		{
			my_buffer[0] = chan->csets->set_hacking ? 'H':'h';
			return my_buffer;
		}
	}
	RETURN_EMPTY;
}

static	char	*status_aop (Window *window)
{
ChannelList *chan;
int serv = window->server;
static char my_buffer[3] = "\0";
	if (window->server != -1)
	{
		if ((chan = prepare_command(&serv, NULL, PC_SILENT)))
		{
			my_buffer[0] = chan->csets->set_aop ? 'A':'a';
			return my_buffer;
		}
	}
	RETURN_EMPTY;
}

static	char	*status_bitch (Window *window)
{
ChannelList *chan;
int serv = window->server;
static char my_buffer[3] = "\0";
	if (window->server != -1)
	{
		if ((chan = prepare_command(&serv, NULL, PC_SILENT)))
		{
			my_buffer[0] = chan->csets->bitch_mode ? 'B':'b';
			return my_buffer;
		}
	}
	RETURN_EMPTY;
}

static char *status_notifyusers (Window *window)
{
int serv = window->server;
static char my_buffer[40] = "\0";
int on, off;
	notify_count(serv, &on, &off);
	sprintf(my_buffer, "%d %d", on, off);
	return my_buffer;
}

static char *status_lastjoin (Window *window)
{
	if (window->server != -1)
	{
		if (joined_nick)
			return joined_nick;
	}
	RETURN_EMPTY;
}

static char *status_newserver (Window *window)
{
extern char *ov_server(int);
	if (window->server != -1)	
		return ov_server(window->server);
	RETURN_EMPTY;
}

static char *status_scrollback(Window *win)
{
char *stuff;
	if (win->scrollback_point &&
	    (stuff = get_wset_string_var(win->wset, STATUS_SCROLLBACK_WSET)))
		return stuff;
	else
		RETURN_EMPTY;
}

static	char	*status_windowspec	(Window *window)
{
static char my_buffer[81];
	if (window->wset->window_special_format)
		strmcpy(my_buffer, window->wset->window_special_format, 80);
	else
		*my_buffer = 0;

	return my_buffer;
}

static	char	*status_percent		(Window *window)
{
	static	char	percent[] = "%";
	return	percent;
}

