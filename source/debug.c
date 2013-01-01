/*
 * debug.c -- controll the values of x_debug.
 *
 * Written by Jeremy Nelson
 * Copyright 1997 EPIC Software Labs
 * See the COPYRIGHT file for more information
 */

#include "irc.h"
static char cvsrevision[] = "$Id: debug.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(debug_c)
#include "struct.h"

#include "ircaux.h"
#include "output.h"
#include "misc.h"
#include "window.h"
#include "hook.h"
#include "lastlog.h"
#include "cset.h"
#include "screen.h"
#include "input.h"
#include "status.h"
#include "vars.h"
#define MAIN_SOURCE
#include "modval.h"

unsigned long x_debug = 0;
unsigned long internal_debug = 0;
unsigned long alias_debug = 0;
unsigned int debug_count = 1;
int in_debug_yell = 0;

struct debug_opts
{
	char 	*command;
	int	flag;
};

static struct debug_opts opts[] = 
{
	{ "LOCAL_VARS",		DEBUG_LOCAL_VARS },
	{ "CTCPS",		DEBUG_CTCPS },
	{ "DCC_SEARCH",		DEBUG_DCC_SEARCH },
	{ "OUTBOUND",		DEBUG_OUTBOUND },
	{ "INBOUND",		DEBUG_INBOUND },
	{ "DCC_XMIT",		DEBUG_DCC_XMIT },
	{ "WAITS",		DEBUG_WAITS },
	{ "MEMORY",		DEBUG_MEMORY },
	{ "SERVER_CONNECT",	DEBUG_SERVER_CONNECT },
	{ "CRASH",		DEBUG_CRASH },
	{ "COLOR",		DEBUG_COLOR },
	{ "NOTIFY",		DEBUG_NOTIFY },
	{ "REGEX",		DEBUG_REGEX },
	{ "REGEX_DEBUG",	DEBUG_REGEX_DEBUG },
	{ "BROKEN_CLOCK",	DEBUG_BROKEN_CLOCK },
	{ "UNKNOWN",		DEBUG_UNKNOWN },
	{ "DEBUG",		DEBUG_DEBUGGER },
	{ "NEW_MATH",		DEBUG_NEW_MATH },
	{ "NEW_MATH_DEBUG",	DEBUG_NEW_MATH_DEBUG },
	{ "AUTOKEY",		DEBUG_AUTOKEY },
	{ "STRUCTURES",		DEBUG_STRUCTURES },
	{ "ALL",		DEBUG_ALL },
	{ NULL,			0 },
};



BUILT_IN_COMMAND(xdebugcmd)
{
	int cnt;
	int remove = 0;
	char *this_arg;

	if (!args || !*args)
	{
		char buffer[540];
		char *q;
		int i = 0;

		buffer[0] = 0;
		strmcat(buffer, "[-][+][option(s)] ", 511);
		q = &buffer[strlen(buffer)];
		for (i = 0; opts[i].command; i++)
		{
			if (q)
				strmcat(q, ", ", 511);
			strmcat(q, opts[i].command, 511);
		}
		return;
	}

	while (args && *args)
	{
		this_arg = upper(next_arg(args, &args));
		if (*this_arg == '-')
			remove = 1, this_arg++;
		else if (*this_arg == '+')
			this_arg++;

		for (cnt = 0; opts[cnt].command; cnt++)
		{
			if (!strncmp(this_arg, opts[cnt].command, strlen(this_arg)))
			{
				if (remove)
					x_debug &= ~opts[cnt].flag;
				else
					x_debug |= opts[cnt].flag;
				break;
			}
		}
		if (!opts[cnt].command)
			say("Unrecognized XDEBUG option '%s'", this_arg);
	}
}

void	debugyell(const char *format, ...)
{
const char *save_from;
unsigned long save_level;
unsigned long old_alias_debug = alias_debug;
	alias_debug = 0;
	save_display_target(&save_from, &save_level);
	set_display_target(NULL, LOG_DEBUG);
	if (format)
	{
		char debugbuf[BIG_BUFFER_SIZE+1];
		va_list args;
		va_start (args, format);
		*debugbuf = 0;
		vsnprintf(debugbuf, BIG_BUFFER_SIZE, format, args);
		va_end(args);
		in_debug_yell = 1;
		if (*debugbuf && do_hook(DEBUG_LIST, "%s", debugbuf))
			put_echo(debugbuf);
		in_debug_yell = 0;
	}
	alias_debug = old_alias_debug;
	reset_display_target();
	restore_display_target(save_from, save_level);
}

int parse_debug(char *value, int nvalue, char **rv)
{
	char	*str1, *str2;
	char	 *copy;
	char	*nv = NULL;

	if (rv)
		*rv = NULL;

	if  (!value)
		return 0;

	copy = alloca(strlen(value) + 1);
	strcpy(copy, value);
	
	while ((str1 = new_next_arg(copy, &copy)))
	{
		while (*str1 && (str2 = next_in_comma_list(str1, &str1)))
		{
			if (!my_strnicmp(str2, "ALL", 3))
				nvalue = (0x7F - (DEBUG_TCL));
			else if (!my_strnicmp(str2, "-ALL", 4))
				nvalue = 0;
			else if (!my_strnicmp(str2, "COMMANDS", 4))
				nvalue |= DEBUG_COMMANDS;
			else if (!my_strnicmp(str2, "-COMMANDS", 4))
				nvalue &= ~(DEBUG_COMMANDS);
			else if (!my_strnicmp(str2, "EXPANSIONS", 4))
				nvalue |= DEBUG_EXPANSIONS;
			else if (!my_strnicmp(str2, "-EXPANSIONS", 4))
				nvalue &= ~(DEBUG_EXPANSIONS);
			else if (!my_strnicmp(str2, "TCL", 3))
				nvalue |= DEBUG_TCL;
			else if (!my_strnicmp(str2, "-TCL", 3))
				nvalue &= ~(DEBUG_TCL);
			else if (!my_strnicmp(str2, "ALIAS", 3))
				nvalue |= DEBUG_CMDALIAS;
			else if (!my_strnicmp(str2, "-ALIAS", 3))
				nvalue &= ~(DEBUG_CMDALIAS);
			else if (!my_strnicmp(str2, "HOOK", 3))
				nvalue |= DEBUG_HOOK;
			else if (!my_strnicmp(str2, "-HOOK", 3))
				nvalue &= ~(DEBUG_HOOK);
			else if (!my_strnicmp(str2, "VARIABLES", 3))
				nvalue |= DEBUG_VARIABLE;
			else if (!my_strnicmp(str2, "-VARIABLES", 3))
				nvalue &= ~(DEBUG_VARIABLE);
			else if (!my_strnicmp(str2, "FUNCTIONS", 3))
				nvalue |= DEBUG_FUNC;
			else if (!my_strnicmp(str2, "-FUNCTIONS", 3))
				nvalue &= ~(DEBUG_FUNC);
			else if (!my_strnicmp(str2, "STRUCTURES", 3))
				nvalue |= DEBUG_STRUCTURES;
			else if (!my_strnicmp(str2, "-STRUCTURES", 3))
				nvalue &= ~(DEBUG_STRUCTURES);
		}
	}
	if (rv)
	{
		if (nvalue & DEBUG_COMMANDS)
			m_s3cat(&nv, comma, "COMMANDS");
		if (nvalue & DEBUG_EXPANSIONS)
			m_s3cat(&nv, comma, "EXPANSIONS");
		if (nvalue & DEBUG_TCL)
			m_s3cat(&nv, comma, "TCL");
		if (nvalue & DEBUG_CMDALIAS)
			m_s3cat(&nv, comma, "ALIAS");
		if (nvalue & DEBUG_HOOK)
			m_s3cat(&nv, comma, "HOOK");
		if (nvalue & DEBUG_VARIABLE)
			m_s3cat(&nv, comma, "VARIABLES");
		if (nvalue & DEBUG_FUNC)
			m_s3cat(&nv, comma, "FUNCTIONS");
		if (nvalue & DEBUG_STRUCTURES)
			m_s3cat(&nv, comma, "STRUCTURES");
		*rv = nv;
	}
	return nvalue;
}

void debug_window(Window *win, char *value, int unused)
{
	Window	*old_win = win;
	char	*nv = NULL;

	internal_debug = parse_debug(value, internal_debug, &nv);
	set_string_var(DEBUG_VAR, nv);

	if (internal_debug)
	{
		Window *tmp = NULL;
		if (!get_window_by_name("debug") && (tmp = new_window(win->screen)))
		{
			malloc_strcpy(&tmp->name, "debug");
			tmp->double_status = 0;
			hide_window(tmp);
			tmp->window_level = LOG_DEBUG;
			tmp->absolute_size = 1;
			tmp->skip = 1;
			debugging_window = tmp;
			set_wset_string_var(tmp->wset, STATUS_FORMAT1_WSET, DEFAULT_FORMAT_DEBUG_FSET);
			build_status(tmp, NULL, 0);
			update_all_windows();
			set_input_prompt(win, get_string_var(INPUT_PROMPT_VAR), 0);
			cursor_to_input();
			set_screens_current_window(old_win->screen, old_win);
		}
	}
	else
	{
		if ((old_win = get_window_by_name("debug")))
		{
			delete_window(old_win);
			debugging_window = NULL;
			update_all_windows();
			set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
			cursor_to_input();
		}
	}
	new_free(&nv);
}
