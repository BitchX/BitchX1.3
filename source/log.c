/*
 * log.c: handles the irc session logging functions 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: log.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(log_c)
#include "struct.h"

#include <sys/stat.h>

#include "log.h"
#include "vars.h"
#include "screen.h"
#include "misc.h"
#include "output.h"
#include "ircaux.h"
#define MAIN_SOURCE
#include "modval.h"

FILE	*irclog_fp = NULL;

void do_log(int flag, char *logfile, FILE **fp)
{
#ifdef PUBLIC_ACCESS
	bitchsay("This command has been disabled on a public access system");
	return;
#else
	time_t	t = now;

	if (flag)
	{
		if (*fp)
			say("Logging is already on");
		else
		{
			if (!logfile)
				return;
			if (!(logfile = expand_twiddle(logfile)))
			{
				say("SET LOGFILE: No such user");
				return;
			}

			if ((*fp = fopen(logfile, get_int_var(APPEND_LOG_VAR)?"a":"w")) != NULL)
			{
				say("Starting logfile %s", logfile);
				chmod(logfile, S_IREAD | S_IWRITE);
				fprintf(*fp, "IRC log started %.24s\n", ctime(&t));
				fflush(*fp);
			}
			else
			{
				say("Couldn't open logfile %s: %s", logfile, strerror(errno));
				*fp = NULL;
			}
			new_free(&logfile);
		}
	}
	else
	{
		if (*fp)
		{
			fprintf(*fp, "IRC log ended %.24s\n", ctime(&t));
			fflush(*fp);
			fclose(*fp);
			*fp = NULL;
			say("Logfile ended");
		}
	}
#endif
}

/* logger: if flag is 0, logging is turned off, else it's turned on */
void logger(Window *win, char *unused, int flag)
{
	char	*logfile;
	if ((logfile = get_string_var(LOGFILE_VAR)) == NULL)
	{
		say("You must set the LOGFILE variable first!");
		set_int_var(LOG_VAR, 0);
		return;
	}
	do_log(flag, logfile, &irclog_fp);
	if (!irclog_fp && flag)
		set_int_var(LOG_VAR, 0);
}

/*
 * set_log_file: sets the log file name.  If logging is on already, this
 * closes the last log file and reopens it with the new name.  This is called
 * automatically when you SET LOGFILE. 
 */
void set_log_file(Window *win, char *filename, int unused)
{
	char	*expanded;

	if (filename)
	{
		if (strcmp(filename, get_string_var(LOGFILE_VAR)))
			expanded = expand_twiddle(filename);
		else
			expanded = expand_twiddle(get_string_var(LOGFILE_VAR));
		set_string_var(LOGFILE_VAR, expanded);
		new_free(&expanded);
		if (irclog_fp)
		{
			logger(current_window, NULL, 0);
			logger(current_window, NULL, 1);
		}
	}
}

/*
 * add_to_log: add the given line to the log file.  If no log file is open
 * this function does nothing. 
 */
void BX_add_to_log(FILE *fp, time_t t, const char *line, int mangler)
{
#ifndef PUBLIC_ACCESS
	if (fp && !inhibit_logging)
	{
		char *local_line;
		int len = strlen(line) * 2 + 1;
		local_line = alloca(len);
		strcpy(local_line, line);

		/* Do this first */
		if (mangler)
			mangle_line(local_line, mangler, len);
		else if (!get_int_var(MIRCS_VAR))
		{
			char *tmp = alloca(strlen(local_line) + 1);
			strip_control(local_line, tmp);
			strcpy(local_line, tmp);
		}

		
		fprintf(fp, "%s\n", local_line);
		fflush(fp);
	}
#endif
}
