/*
 * lastlog.c: handles the lastlog features of irc. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: lastlog.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(lastlog_c)
#include "struct.h"

#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "vars.h"
#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "hook.h"
#include "status.h"
#define MAIN_SOURCE
#include "modval.h"

extern	int	grab_http (char *, char *, char *);
/*
 * lastlog_level: current bitmap setting of which things should be stored in
 * the lastlog.  The LOG_MSG, LOG_NOTICE, etc., defines tell more about this 
 */
static	unsigned long	lastlog_level;
static	unsigned long	notify_level;

static	unsigned long	msglog_level = 0;
	unsigned long	beep_on_level = 0;
	unsigned long	new_server_lastlog_level = 0;
	unsigned long	current_window_level = 0;
	
	
FILE	*logptr = NULL;

/*
 * msg_level: the mask for the current message level.  What?  Did he really
 * say that?  This is set in the set_lastlog_msg_level() routine as it
 * compared to the lastlog_level variable to see if what ever is being added
 * should actually be added 
 */
static	unsigned long	msg_level = LOG_CRAP;

static	char	*levels[] =
{
	"CRAP",		"PUBLIC",	"MSGS",		"NOTICES",
	"WALLS",	"WALLOPS",	"NOTES",	"OPNOTES",
	"SNOTES",	"ACTIONS",	"DCC",		"CTCP",
	"USERLOG1",	"USERLOG2",	"USERLOG3",	"USERLOG4",
	"USERLOG5",	"BEEP",		"TCL",		"SEND_MSG", 
	"KILL",		"MODEUSER",	"MODECHAN",	"KICK", 
	"KICKUSER",	"PARTS",	"INVITES",	"JOIN",
	"TOPIC",	"HELP",		"NOTIFY",	"DEBUG"
};

#define NUMBER_OF_LEVELS (sizeof(levels) / sizeof(char *))


void reset_hold_mode(Window *win)
{
	win->hold_mode = win->save_hold_mode;
	win->save_hold_mode = win->in_more = 0;
}

/* set_lastlog_msg_level: sets the message level for recording in the lastlog */
unsigned long BX_set_lastlog_msg_level(unsigned long level)
{
	unsigned long	old;

	old = msg_level;
	msg_level = level;
	return (old);
}

/*
 * bits_to_lastlog_level: converts the bitmap of lastlog levels into a nice
 * string format.  Note that this uses the global buffer, so watch out 
 */
char	* bits_to_lastlog_level(unsigned long level)
{
	static	char	buffer[481]; /* this *should* be enough for this */
	int	i;
unsigned long	p;

	if (level == LOG_ALL)
		strcpy(buffer, "ALL");
	else if (level == 0)
		strcpy(buffer, "NONE");
	else
	{
		*buffer = '\0';
		for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p <<= 1)
		{
			if (level & p)
			{
				if (*buffer)
					strmcat(buffer, space, 480);
				strmcat(buffer, levels[i],480);
			}
		}
	}
	return (buffer);
}

unsigned long parse_lastlog_level(char *str, int display)
{
	char	*ptr,
		*rest;
	int	len,
		i;
unsigned long	p,
		level;
	int	neg;

	level = 0;
	while ((str = next_arg(str, &rest)) != NULL)
	{
		while (str)
		{
			if ((ptr = strchr(str, ',')) != NULL)
				*ptr++ = '\0';
			if ((len = strlen(str)) != 0)
			{
				if (my_strnicmp(str, "ALL", len) == 0)
					level = LOG_ALL;
				else if (my_strnicmp(str, "NONE", len) == 0)
					level = 0;
				else
				{
					if (*str == '-')
					{
						str++; len--;
						neg = 1;
					}
					else
						neg = 0;
					for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p <<= 1)
					{
						if (!my_strnicmp(str, levels[i], len))
						{
							if (neg)
								level &= (LOG_ALL ^ p);
							else
								level |= p;
							break;
						}
					}
					if (i == NUMBER_OF_LEVELS)
					{
						if (display)
							say("Unknown lastlog level: %s", str);
						return LOG_ALL;
					}
				}
			}
			str = ptr;
		}
		str = rest;
	}
	return (level);
}

/*
 * set_lastlog_level: called whenever a "SET LASTLOG_LEVEL" is done.  It
 * parses the settings and sets the lastlog_level variable appropriately.  It
 * also rewrites the LASTLOG_LEVEL variable to make it look nice 
 */
void set_lastlog_level(Window *win, char *str, int unused)
{
	lastlog_level = parse_lastlog_level(str, 1);
	set_string_var(LASTLOG_LEVEL_VAR, bits_to_lastlog_level(lastlog_level));
	current_window->lastlog_level = lastlog_level;
}

/*
 * set_msglog_level: called whenever a "SET MSGLOG_LEVEL" is done.  It
 * parses the settings and sets the msglog_level variable appropriately.  It
 * also rewrites the MSGLOG_LEVEL variable to make it look nice 
 */
void set_msglog_level(Window *win, char *str, int unused)
{
	msglog_level = parse_lastlog_level(str, 1);
	set_string_var(MSGLOG_LEVEL_VAR, bits_to_lastlog_level(msglog_level));
}

void	set_new_server_lastlog_level (Window *win, char *str, int unused)
{
	new_server_lastlog_level = parse_lastlog_level(str, 1);
	set_string_var(NEW_SERVER_LASTLOG_LEVEL_VAR, 
			bits_to_lastlog_level(new_server_lastlog_level));
}

void remove_from_lastlog(Window *window)
{
	Lastlog *tmp, *end_holder;

	if (window->lastlog_tail)
	{
		end_holder = window->lastlog_tail;
		tmp = window->lastlog_tail->prev;
		window->lastlog_tail = tmp;
		if (tmp)
			tmp->next = NULL;
		else
			window->lastlog_head = window->lastlog_tail;
		window->lastlog_size--;
		new_free(&end_holder->msg);
		new_free((char **)&end_holder);
	}
	else
		window->lastlog_size = 0;
}

/*
 * set_lastlog_size: sets up a lastlog buffer of size given.  If the lastlog
 * has gotten larger than it was before, all previous lastlog entry remain.
 * If it get smaller, some are deleted from the end. 
 */
void set_lastlog_size(Window *win_unused, char *unused, int size)
{
	int	i,
		diff;
	Window	*win = NULL;

	while ((traverse_all_windows(&win)))
	{
		if (win->lastlog_size > size)
		{
			diff = win->lastlog_size - size;
			for (i = 0; i < diff; i++)
				remove_from_lastlog(win);
		}
		win->lastlog_max = size;
	}
}

void free_lastlog(Window *win)
{
Lastlog *ptr;
	for (ptr = win->lastlog_head; ptr;)
	{
		Lastlog *next = ptr->next;
		new_free(&ptr->msg);
		new_free(&ptr);
		ptr = next;
	}
	win->lastlog_head = NULL;
	win->lastlog_tail = NULL;
	win->lastlog_size = 0;
}
/*
 * lastlog: the /LASTLOG command.  Displays the lastlog to the screen. If
 * args contains a valid integer, only that many lastlog entries are shown
 * (if the value is less than lastlog_size), otherwise the entire lastlog is
 * displayed 
 */
BUILT_IN_COMMAND(lastlog)
{
	int	cnt,
		from = 0,
		p,
		i,
		level = 0,
		msg_level,
		len,
		mask = 0,
		header = 1,
		lines = 0,
		reverse = 0,
		time_log = 1,
		remove = 0;

	Lastlog *start_pos;
	char	*match = NULL,
		*arg;
	char	*file_open[] = { "wt", "at" };
	int	file_open_type = 0;
	char	*blah = NULL;
	FILE	*fp = NULL;
	
	reset_display_target();
	cnt = current_window->lastlog_size;

	while ((arg = new_next_arg(args, &args)) != NULL)
	{
		if (*arg == '-')
		{
			arg++;
			if (!(len = strlen(arg)))
			{
				header = 0;
				continue;
			}
			else if (!my_strnicmp(arg, "MAX", len))
			{
				char *ptr = NULL;
				ptr = new_next_arg(args, &args);
				if (ptr)
					lines = atoi(ptr);
				if (lines < 0)
					lines = 0;
			}
			else if (!my_strnicmp(arg, "LITERAL", len))
			{
				if (match)
				{
					say("Second -LITERAL argument ignored");
					(void) new_next_arg(args, &args);
					continue;
				}
				if ((match = new_next_arg(args, &args)) != NULL)
					continue;
				say("Need pattern for -LITERAL");
				return;
			}
			else if (!my_strnicmp(arg, "REVERSE", len))
				reverse = 1;
			else if (!my_strnicmp(arg, "TIME", len))
				time_log = 0;
			else if (!my_strnicmp(arg, "BEEP", len))
			{
				if (match)
				{
					say("-BEEP is exclusive; ignored");
					continue;
				}
				else
					match = "\007";
			}
			else if (!my_strnicmp(arg, "CLEAR", len))
			{
				free_lastlog(current_window);
				say("Cleared lastlog");
				return;
			}
			else if (!my_strnicmp(arg, "APPEND", len))
				file_open_type = 1;
			else if (!my_strnicmp(arg, "FILE", len))
			{
#ifdef PUBLIC_ACCESS
				bitchsay("This command has been disabled on a public access system");
				return;
#else
				if (args && *args)
				{
					char *filename;
					filename = next_arg(args, &args);
					if (!(fp = fopen(filename, file_open[file_open_type])))
					{
						bitchsay("cannot open file %s", filename);
						return;
					}
				} 
				else
				{
					bitchsay("Filename needed for save");
					return;
				}
#endif
			}
			else if (!my_strnicmp(arg, "MORE", len))
			{
				current_window->save_hold_mode = current_window->hold_mode;
				current_window->in_more = 1;
				reset_line_cnt(current_window, NULL, 1);
			}
			else
			{
				if (*arg == '-')
					remove = 1, arg++;
				else
					remove = 0;
					
				/*
				 * Which can be combined with -ALL, which 
				 * turns on all levels.  Use --MSGS or
				 * whatever to turn off ones you dont want.
				 */
				if (!my_strnicmp(arg, "ALL", len))
				{
					if (remove)
						mask = 0;
					else
						mask = LOG_ALL;
					continue;	/* Go to next arg */
				}

				/*
				 * Find the lastlog level in our list.
				 */
				for (i = 0, p = 1; i < NUMBER_OF_LEVELS; i++, p *= 2)
				{
					if (!my_strnicmp(levels[i], arg, len))
					{
						if (remove)
							mask &= ~p;
						else
							mask |= p;
						break;
					}
				}

				if (i == NUMBER_OF_LEVELS)
				{
					bitchsay("Unknown flag: %s", arg);
					reset_display_target();
					return;
				}
			}
		}
		else
		{
			if (level == 0)
			{
				if (match || isdigit((unsigned char)*arg))
				{
					cnt = atoi(arg);
					level++;
				}
				else
					match = arg;
			}
			else if (level == 1)
			{
				from = atoi(arg);
				level++;
			}
		}
	}
	start_pos = current_window->lastlog_head;
	level = current_window->lastlog_level;
	msg_level = set_lastlog_msg_level(0);

	if (!reverse)
	{
		for (i = 0; (i < from) && start_pos; start_pos = start_pos->next)
			if (!mask || (mask & start_pos->level))
				i++;

		for (i = 0; (i < cnt) && start_pos; start_pos = start_pos->next)
			if (!mask || (mask & start_pos->level))
				i++;

		start_pos = (start_pos) ? start_pos->prev : current_window->lastlog_tail;
	} else
		start_pos = current_window->lastlog_head;
		
	/* Let's not get confused here, display a seperator.. -lynx */
	strip_ansi_in_echo = 0;
	if (header && !fp)
		say("Lastlog:");

	if (match)
	{

		blah = (char *) alloca(strlen(match)+4);
		sprintf(blah, "*%s*", match);
	}
	for (i = 0; (i < cnt) && start_pos; start_pos = (reverse ? start_pos->next : start_pos->prev))
	{
		if (!mask || (mask & start_pos->level))
		{
			i++;
			if (!match || wild_match(blah, start_pos->msg))
			{
				char *s = !get_int_var(LASTLOG_ANSI_VAR)?stripansicodes(start_pos->msg):start_pos->msg;

				if (!fp)
				{
					put_it("%s", !time_log ? s : convert_output_format(fget_string_var(FORMAT_LASTLOG_FSET), "%l %s", start_pos->time, s));
					grab_http("*", "*", start_pos->msg);
				} 
				else
				{
					if (time_log)
					{
						s = convert_output_format(fget_string_var(FORMAT_LASTLOG_FSET), "%l %s", start_pos->time, s);
						chop(s, 3);
					}
					fprintf(fp, "%s\n", s);
				}
				if (lines == 0)
					continue;
				else if (lines == 1)
					break;
				lines--;
			}
		}
	}
	if (header && !fp)
		say("End of Lastlog");
	if (fp)
		fclose(fp);
	strip_ansi_in_echo = 1;
	current_window->lastlog_level = level;
	set_lastlog_msg_level(msg_level);
}

Lastlog *get_lastlog_current_head(Window *win)
{
	return win->lastlog_head;
}

/*
 * add_to_lastlog: adds the line to the lastlog.  If the LASTLOG_CONVERSATION
 * variable is on, then only those lines that are user messages (private
 * messages, channel messages, wall's, and any outgoing messages) are
 * recorded, otherwise, everything is recorded 
 */
void add_to_lastlog(Window *window, const char *line)
{
	Lastlog *new;

	if (window == NULL)
		window = current_window;
	if (window->lastlog_level & msg_level)
	{
		/* no nulls or empty lines (they contain "> ") */
		if (line && (strlen(line) > 2))
		{
			new = (Lastlog *) new_malloc(sizeof(Lastlog));
			new->next = window->lastlog_head;
			new->prev = NULL;
			new->level = msg_level;
			new->msg = NULL;
			new->time = now;
			new->msg = m_strdup(line);

			if (window->lastlog_head)
				window->lastlog_head->prev = new;
			window->lastlog_head = new;

			if (window->lastlog_tail == NULL)
				window->lastlog_tail = window->lastlog_head;

			if (window->lastlog_size++ >= window->lastlog_max)
				remove_from_lastlog(window);
		}
	}
}

unsigned long real_notify_level(void)
{
	return (notify_level);
}

unsigned long real_lastlog_level(void)
{
	return (lastlog_level);
}

void set_notify_level(Window *win, char *str, int unused)
{
	notify_level = parse_lastlog_level(str, 1);
	set_string_var(NOTIFY_LEVEL_VAR, bits_to_lastlog_level(notify_level));
	current_window->notify_level = notify_level;
}

int logmsg(unsigned long log_type, char *from, int flag, char *format, ...)
{
#ifdef PUBLIC_ACCESS
	return 0;
#else
	char *timestr;
	time_t t;
	char *filename = NULL;
	char *expand = NULL;
	char *type = NULL;
	unsigned char **lines = NULL;
	char msglog_buffer[BIG_BUFFER_SIZE+1];			
	
	
	if (!get_string_var(MSGLOGFILE_VAR) || !get_string_var(CTOOLZ_DIR_VAR))
		return 0;

	t = now;
	timestr = update_clock(GET_TIME);


	if (format)
	{
		va_list ap;
		va_start(ap, format);
		vsnprintf(msglog_buffer, BIG_BUFFER_SIZE, format, ap);
		va_end(ap);
	}


	switch (flag)
	{
	case 0:
		if (!(type = bits_to_lastlog_level(log_type)))
			type = "Unknown";
		if (msglog_level & log_type && format)
		{
			char *format;
			
			if (!do_hook(MSGLOG_LIST, "%s %s %s %s", timestr, type, from, msglog_buffer))
				break;
			if (!logptr)
				return 0;
			if (!(format = fget_string_var(FORMAT_MSGLOG_FSET)))
				format = "[$[10]0] [$1] - $2-";
			lines = split_up_line(stripansicodes(convert_output_format(format, "%s %s %s %s", type, timestr, from, msglog_buffer)), 80);
			for ( ; *lines; lines++)
			{
				char *local_copy;
				int len = strlen(*lines) * 2 + 1;
				if (!*lines || !**lines) break;
				local_copy = alloca(len);
				strcpy(local_copy, *lines);

				if (local_copy[strlen(local_copy)-1] == ALL_OFF)
					local_copy[strlen(local_copy)-1] = 0;
				if (logfile_line_mangler)
					mangle_line(local_copy, logfile_line_mangler, len);
				if (*local_copy)
					fprintf(logptr, "%s\n", local_copy);
			}
			fflush(logptr);
		}
		break;
	case 1:
		malloc_sprintf(&filename, "%s/%s", get_string_var(CTOOLZ_DIR_VAR), get_string_var(MSGLOGFILE_VAR));
		expand = expand_twiddle(filename);
		new_free(&filename);
		if (!do_hook(MSGLOG_LIST, "%s %s %s %s", timestr, "On", expand, empty_string))
		{
			new_free(&expand);
			return 1;
		}
		if (logptr)
		{
			new_free(&expand);
			return 1;
		}
		if (!(logptr = fopen(expand, get_int_var(APPEND_LOG_VAR)?"at":"wt")))
		{
			set_int_var(MSGLOG_VAR, 0);
			new_free(&expand);
			return 0;
		} 

		fprintf(logptr, "MsgLog started [%s]\n", my_ctime(t));
		fflush(logptr);
		if (format)
		{
			int i;

			i = logmsg(LOG_CURRENT, from, 0, "%s", msglog_buffer);
			return i;
		}
		bitchsay("Now logging messages to: %s", expand);
		new_free(&expand);
		break;
	case 2:
		if (!do_hook(MSGLOG_LIST, "%s %s %s %s", timestr, "Off", empty_string, empty_string))
			return 1;
		if (!logptr)
			return 1;
		fprintf(logptr, "MsgLog ended [%s]\n", my_ctime(t));
		fclose(logptr);
		logptr = NULL;
		break;
	case 3:
		return logptr ? 1 : 0;
		break;
	case 4:
		if (!logptr)
			return 1;
		fprintf(logptr, "[TimeStamp %s]\n", my_ctime(t));
		fflush(logptr);
		break;
	default:
		bitchsay("Bad Flag passed to logmsg");
		return 0;
	}
	return 1;
#endif
}

void 	set_beep_on_msg (Window *win, char *str, int unused)
{
	beep_on_level = parse_lastlog_level(str, 1);
	set_string_var(BEEP_ON_MSG_VAR, bits_to_lastlog_level(beep_on_level));
}

BUILT_IN_COMMAND(awaylog)
{
	if (args && *args)
	{
		msglog_level = parse_lastlog_level(args, 1);
		set_string_var(MSGLOG_LEVEL_VAR, bits_to_lastlog_level(msglog_level));
		put_it("%s", convert_output_format("$G Away logging set to: $0-", "%s", get_string_var(MSGLOG_LEVEL_VAR)));
	}
	else
		put_it("%s", convert_output_format("$G Away logging currently: $0-", "%s", bits_to_lastlog_level(msglog_level)));		
}

#define EMPTY empty_string
#define RETURN_EMPTY return m_strdup(EMPTY)
#define RETURN_IF_EMPTY(x) if (empty( x )) RETURN_EMPTY
#define GET_INT_ARG(x, y) {RETURN_IF_EMPTY(y); x = my_atol(safe_new_next_arg(y, &y));}
#define GET_STR_ARG(x, y) {RETURN_IF_EMPTY((y)); x = new_next_arg((y), &(y));RETURN_IF_EMPTY((x));}
#define RETURN_STR(x) return m_strdup(x ? x : EMPTY);

/*
 * $line(<line number> [window number])
 * Returns the text of logical line <line number> from the lastlog of 
 * window <window number>.  If no window number is supplied, the current
 * window will be used.  If the window number is invalid, the function
 * will return the false value.
 *
 * Lines are numbered from 1, starting at the most recent line in the buffer.
 * Contributed by Crackbaby (Matt Carothers) on March 19, 1998.
 */

BUILT_IN_FUNCTION(function_line)
{
	int	line = 0;
	char *	windesc = zero;
	Lastlog	*start_pos;
	Window	*win;
	char	*extra;
	int	do_level = 0;


	GET_INT_ARG(line, input);

	while (input && *input)
	{
		GET_STR_ARG(extra, input);

		if (!my_stricmp(extra, "-LEVEL"))
			do_level = 1;
		else
			windesc = extra;
	}

	/* Get the current window, default to current window */
	if (!(win = get_window_by_desc(windesc)))
		RETURN_EMPTY;

	/* Make sure that the line request is within reason */
	if (line < 1 || line > win->lastlog_size)
		RETURN_EMPTY;

	/* Get the line from the lastlog */
	for (start_pos = win->lastlog_head; line; start_pos = start_pos->next)
		line--;

	if (!start_pos)
		start_pos = win->lastlog_tail;
	else
		start_pos = start_pos->prev;

	if (do_level)
		return m_sprintf("%s %s", start_pos->msg, 
					levels[start_pos->level]);
	else
		RETURN_STR(start_pos->msg);
}

/*
 * $lastlog(<window description> <lastlog levels>)
 * Returns all of the lastlog lines (suitable for use with $line()) on the
 * indicated window (0 for the current window) that have any of the lastlog 
 * levels as represented by the lastlog levels. If the window number is 
 * invalid, the function will return the false value.
 */

BUILT_IN_FUNCTION(function_lastlog)
{
	char *	windesc = zero;
	char *	pattern = NULL;
	char *	retval = NULL;
	Lastlog	*iter;
	Window *win;
	int	levels;
	int	line = 1;

	GET_STR_ARG(windesc, input);
	GET_STR_ARG(pattern, input);
	levels = parse_lastlog_level(input, 0);

	/* Get the current window, default to current window */
	if (!(win = get_window_by_desc(windesc)))
		RETURN_EMPTY;

	for (iter = win->lastlog_head; iter; iter = iter->next, line++)
	{
		if (iter->level & levels)
			if (wild_match(pattern, iter->msg))
				m_s3cat(&retval, space, ltoa(line));
	}

	if (retval)
		return retval;

	RETURN_EMPTY;
}


