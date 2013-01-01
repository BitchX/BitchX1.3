/*
 * timer.c -- handles timers in ircII
 * Copyright 1993, 1996 Matthew Green
 * This file organized/adapted by Jeremy Nelson
 *
 * This used to be in edit.c, and it used to only allow you to
 * register ircII commands to be executed later.  I needed more
 * generality then that, specifically the ability to register
 * any function to be called, so i pulled it all together into
 * this file and called it timer.c
 */

#include "irc.h"
static char cvsrevision[] = "$Id: timer.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(timer_c)
#include "struct.h"

#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "timer.h"
#include "hook.h"
#include "output.h"
#include "commands.h"
#include "misc.h"
#include "vars.h"
#include "server.h"
#include "screen.h"
#include "tcl_bx.h"
#define MAIN_SOURCE
#include "modval.h"

static	void	show_timer (char *command);
	void	BX_delete_all_timers (void);
int	timer_exists (char *);
static	TimerList *get_timer (char *ref);

#define RETURN_INT(x) return m_strdup(ltoa(x))

/*
 * timercmd: the bit that handles the TIMER command.  If there are no
 * arguements, then just list the currently pending timers, if we are
 * give a -DELETE flag, attempt to delete the timer from the list.  Else
 * consider it to be a timer to add, and add it.
 */
BUILT_IN_COMMAND(timercmd)
{
	char	*waittime,
		*flag;
	char	*want = empty_string;
	char	*ptr;
	double	interval;
	long	events = -2;
	int	update = 0;
	int	winref = current_window->refnum;
	
	while (*args == '-' || *args == '/')
	{
		flag = next_arg(args, &args);
		if (!flag || !*flag)
			break;

		if (!my_strnicmp(flag+1, "D", 1))	/* DELETE */
		{
			if (!(ptr = next_arg(args, &args)))
				say("%s: Need a timer reference number for -DELETE", command);
			else
			{
				if (timer_exists(ptr))
					delete_timer(ptr);
				else if (!my_strnicmp(ptr, "A", 1))
					delete_all_timers();
			}
			return;
		}
		else if (!my_strnicmp(flag+1, "REP", 3))
		{
			char *na = next_arg(args, &args);
			if (!na || !*na)
			{
				say("%s: Missing argument to -REPEAT", command);
				return;
			}
			if (!strcmp(na, "*") || !strcmp(na, "-1"))
				events = -1;
			else if ((events = my_atol(na)) == 0)			
				return;			
		}
		else if (!my_strnicmp(flag+1, "REF", 3))	/* REFNUM */
		{
			want = next_arg(args, &args);
			if (!want || !*want)
			{
				say("%s: Missing argument to -REFNUM", command);
				return;
			}
		}
		else if (!my_strnicmp(flag + 1, "L", 1))
			show_timer(command);
		else if (!my_strnicmp(flag + 1, "W", 1))	/* WINDOW */
		{
			char 	*na;

			if ((na = next_arg(args, &args)))
				winref = get_winref_by_desc(na);

			if (winref == -1 && my_stricmp(na, "-1"))
			{
				say("%s: That window doesnt exist!", command);
				return;
			}
		}
		else if (!my_strnicmp(flag + 1, "UPDATE", 1))	/* UPDATE */
			update = 1;
		else
			say("%s: %s no such flag", command, flag);
	}

	/* else check to see if we have no args -> list */

	waittime = next_arg(args, &args);
	if (update || waittime)
	{
		if (update && !timer_exists(want))
		{
			say("%s: To use -UPDATE you must specify a valid refnum", command);
			return;
		}

		if (!waittime)
			interval = -1;
		else
			interval = atof(waittime) * 1000.0;

		if (!update && events == -2)
			events = 1;
		else if (events == -2)
			events = -1;
		
		add_timer(update, want, interval, events, NULL, args, subargs, winref, NULL);
	} 
	else
		show_timer(command);
	return;
}

/*
 * This is put here on purpose -- we dont want any of the above functions
 * to have any knowledge of this struct.
 */
static TimerList *PendingTimers;
static char *schedule_timer (TimerList *ntimer);

static char *current_exec_timer = empty_string;

/*
 * ExecuteTimers:  checks to see if any currently pending timers have
 * gone off, and if so, execute them, delete them, etc, setting the
 * current_exec_timer, so that we can't remove the timer while its
 * still executing.
 *
 * changed the behavior: timers will not hook while we are waiting.
 */
extern	void ExecuteTimers (void)
{
	TimerList	*current;
        static int parsingtimer = 0;
	int		old_from_server = from_server;
	struct timeval	now1;
		
        /* We do NOT want to parse timers while waiting
         * cause it gets icky recursive
         */
        if (!PendingTimers || parsingtimer)
                return;
	get_time(&now1);
	
        parsingtimer = 1;
	while (PendingTimers && BX_time_diff(now1, PendingTimers->time) < 0)
	{
		int old_refnum = current_window->refnum;

		current = PendingTimers;
		PendingTimers = current->next;

		make_window_current_by_winref(current->window);		
		/*
		 * Restore from_server and curr_scr_win from when the
		 * timer was registered
		 */

		if (is_server_connected(current->server))
			from_server = current->server;
		else if (is_server_connected(get_window_server(0)))
			from_server = get_window_server(0);
		else
			from_server = -1;
						
		/* 
		 * If a callback function was registered, then
		 * we use it.  If no callback function was registered,
		 * then we use ''parse_line''.
		 */
		current_exec_timer = current->ref;
		if (current->callback)
			(*current->callback)(current->command, current->subargs);
		else
			parse_line("TIMER",(char *)current->command, current->subargs, 0, 0, 1);

		current_exec_timer = empty_string;
		from_server = old_from_server;
		make_window_current_by_winref(old_refnum);

		/*
		 * Clean up or reschedule the timer
		 */
		switch (current->events)
		{
			case 0:
			case 1:
			{
				/* callback cleans up command */
				if (!current->callback)
					new_free(&current->command);
				new_free(&current->subargs);
				new_free(&current->whom);
				new_free(&current);
				break;
			}
			default:
				current->events--;
			case -1:
			{
#if 1
				double milli, seconds;
				milli = current->interval * 1000 * 1000 + current->time.tv_usec;
				seconds = current->time.tv_sec + (milli / 1000000);
				milli = ((unsigned long)current) % 1000000;
				current->time.tv_sec = seconds;
				current->time.tv_usec = milli;
#else
				current->time.tv_usec += (current->interval * 1000);
				current->time.tv_sec += (current->time.tv_usec / 1000000);
				current->time.tv_usec %= 1000000;
#endif
				schedule_timer(current);
				break;	
			}
		}
		if (current && current->delete)
		{
			if (!current->callback)
				new_free(&current->command);
			new_free(&current->subargs);
			new_free(&current->whom);
			new_free(&current);
		}
	}
        parsingtimer = 0;
}

/*
 * show_timer:  Display a list of all the TIMER commands that are
 * pending to be executed.
 */
static	void	show_timer (char *command)
{
	TimerList	*tmp;
	struct timeval	current;
	double		time_left;
	int count = 0;
	
	for (tmp = PendingTimers; tmp; tmp = tmp->next)
		count++;

	if (!count)
	{
		say("%s: No commands pending to be executed", command);
		return;
	}

	get_time(&current);
	put_it("%s", convert_output_format(fget_string_var(FORMAT_TIMER_FSET), "%s %s %s %s","Timer","Seconds","Events","Command"));
	for (tmp = PendingTimers; tmp; tmp = tmp->next)
	{
		char buf[40];
		time_left = BX_time_diff(current, tmp->time);
		if (time_left < 0)
			time_left = 0;
		sprintf(buf, "%0.3f", time_left);
		put_it("%s", convert_output_format(fget_string_var(FORMAT_TIMER_FSET), "%s %s %d %s %s", tmp->ref, buf, tmp->events, tmp->callback? "(internal callback)" : (tmp->command? tmp->command : ""), tmp->whom ? tmp->whom : empty_string ));
	}
}

/*
 * create_timer_ref:  returns the lowest unused reference number for a timer
 *
 * This will never return 0 for a refnum because that is what atol() returns
 * on case of error, so that it can never happen that a timer has a refnum
 * of zero which would be tripped if the user did say,
 *	/TIMER -refnUm foobar 3 blah blah blah
 * which should elicit an error, not be silently punted.
 */
static	int	create_timer_ref (char *refnum_want, char *refnum_gets)
{
	TimerList       *tmp;
	int             refnum = 0;
                
	/* Max of 10 characters. */
	if (strlen(refnum_want) > REFNUM_MAX)
		refnum_want[REFNUM_MAX] = 0;

	/* If the user doesnt care */
	if (!strcmp(refnum_want, empty_string))
	{
		/* Find the lowest refnum available */
		for (tmp = PendingTimers; tmp; tmp = tmp->next)
		{
			long ref;
			ref = my_atol(tmp->ref);
			if (refnum < ref)
				refnum = ref;
		}
		strmcpy(refnum_gets, ltoa(refnum+1), REFNUM_MAX);
	}
	else
	{
		/* See if the refnum is available */
		for (tmp = PendingTimers; tmp; tmp = tmp->next)
		{
			if (!my_stricmp(tmp->ref, refnum_want))
				return -1;
		}
		strmcpy(refnum_gets, refnum_want, REFNUM_MAX);
	}

	return 0;
}

/*
 * Deletes a refnum.  This does cleanup only if the timer is a 
 * user-defined timer, otherwise no clean up is done (the caller
 * is responsible to handle it)  This shouldnt output an error,
 * it should be more general and return -1 and let the caller
 * handle it.  Probably will be that way in a future release.
 */
extern int BX_delete_timer (char *ref)
{
	TimerList	*tmp,
			*prev;

	if (current_exec_timer != empty_string)
	{
		say("You may not remove a TIMER from another TIMER");
		return -1;
	}

	for (prev = tmp = PendingTimers; tmp; prev = tmp, tmp = tmp->next)
	{
		/* can only delete user created timers */
		if (!my_stricmp(tmp->ref, ref))
		{
			if (tmp == prev)
				PendingTimers = PendingTimers->next;
			else
				prev->next = tmp->next;
			if (!tmp->callback)
				new_free(&tmp->command);
			new_free(&tmp->subargs);
			new_free(&tmp->whom);
			new_free((char **)&tmp);
			return 0;
		}
	}
	say("TIMER: Can't delete %s, no such refnum", ref);
	return -1;
}

int kill_timer(char *ref)
{
	TimerList	*tmp;
	for (tmp = PendingTimers; tmp; tmp = tmp->next)
	{
		/* can only delete user created timers */
		if (!my_stricmp(tmp->ref, ref))
		{
			tmp->delete = 1;
			return 0;
		}
	}
	return -1;
}

int get_delete_timer(char *ref)
{
	TimerList	*tmp;
	for (tmp = PendingTimers; tmp; tmp = tmp->next)
	{
		/* can only delete user created timers */
		if (!my_stricmp(tmp->ref, ref))
			return tmp->delete;
	}
	return -1;

}

void BX_delete_all_timers (void)
{
	while (PendingTimers)
		delete_timer(PendingTimers->ref);
	return;
}

int timer_exists (char *ref)
{
	if (get_timer(ref))
		return 1;
	return 0;
}

BUILT_IN_FUNCTION(function_istimer)
{
char *timer = NULL;

	if ((timer = next_arg(input, &input)))
		if (timer_exists(timer))
			RETURN_INT(1);
	RETURN_INT(0);
}

int timer_callback_exists (void *ref)
{
TimerList *t;
	for (t = PendingTimers; t; t = t->next)
	{
		if (t->callback && ref == t->callback)
			return 1;
	}
	return 0;
}

static	TimerList *get_timer (char *ref)
{
	TimerList *tmp;

	for (tmp = PendingTimers; tmp; tmp = tmp->next)
	{
		if (!my_stricmp(tmp->ref, ref) || (tmp->whom && !my_stricmp(tmp->whom, ref)))
			return tmp;
	}

	return NULL;
}

char *function_timer(char *n, char *args)
{
char *ref;
char *out = NULL;
TimerList *tmp;
	ref = next_arg(args, &args);
	if (ref && *ref && (tmp = get_timer(ref)))
	{
		double time_left;
		struct timeval current;
		char buf[40];
		get_time(&current);
		time_left = BX_time_diff(current, tmp->time);
		if (time_left < 0)
			time_left = 0.0;
		sprintf(buf, "%0.3f", time_left);
		malloc_sprintf(&out, "%s %d %d %d %d %s %s %s", tmp->ref, tmp->server, tmp->window, tmp->interval, tmp->events, buf, tmp->callback? "(internal callback)" : (tmp->command? tmp->command : ""), tmp->whom ? tmp->whom : empty_string );
		return ref;
	}
	return m_strdup(empty_string);
}

/*
 * You call this to register a timer callback.
 *
 * The arguments:
 *  refnum_want: The refnUm requested.  This should only be sepcified
 *		 by the user, functions wanting callbacks should specify
 *		 the value -1 which means "dont care".
 * The rest of the arguments are dependant upon the value of "callback"
 *	-- if "callback" is NULL then:
 *  callback:	 NULL
 *  what:	 some ircII commands to run when the timer goes off
 *  subargs:	 what to use to expand $0's, etc in the 'what' variable.
 *
 *	-- if "callback" is non-NULL then:
 *  callback:	 function to call when timer goes off
 *  what:	 argument to pass to "callback" function.  Should be some
 *		 non-auto storage, perhaps a struct or a malloced char *
 *		 array.  The caller is responsible for disposing of this
 *		 area when it is called, since the timer mechanism does not
 *		 know anything of the nature of the argument.
 * subargs:	 should be NULL, its ignored anyhow.
 */
char *BX_add_timer(int update, char *refnum_want, double when, long events, int (callback) (void *, char *), char *what, char *subargs, int winref, char *whom)
{
	TimerList	*ntimer, *otimer = NULL;
	char		refnum_got[REFNUM_MAX+1] = "";
	double		seconds = 0.0, milli = 0.0;	
extern	double		fmod(double, double);
	
	ntimer = (TimerList *) new_malloc(sizeof(TimerList));

	get_time(&ntimer->time);
#if 1
	milli = when * 1000 + ntimer->time.tv_usec;
	seconds = ntimer->time.tv_sec + (milli / 1000000);

	milli = ((unsigned long)milli) % 1000000;
	ntimer->time.tv_sec = seconds;
	ntimer->time.tv_usec = milli;
#else
	ntimer->time.tv_usec += (unsigned long)when;
	ntimer->time.tv_sec += ((when + ntimer->time.tv_usec) / 1000);
	ntimer->time.tv_usec %= 1000;
#endif

	ntimer->interval = when / 1000;
	ntimer->events = events;
	ntimer->server = from_server;
	ntimer->window = winref;
	if (whom) 
		ntimer->whom = m_strdup(whom);
	if (update)
		otimer = get_timer(refnum_want);
                        
	if (otimer)
	{
		if (when == -1)
		{
			ntimer->time = otimer->time;
			ntimer->interval = otimer->interval;
		}

		if (events == -1)
			ntimer->events = otimer->events;

		ntimer->callback = otimer->callback;
		if (what && *what)
		{
			ntimer->command = (void *)what;
			if (subargs)
				ntimer->subargs = m_strdup(subargs);
		}
		else
		{
			ntimer->command = m_strdup(otimer->command);
			ntimer->subargs = m_strdup(otimer->subargs);
		}

		delete_timer(refnum_want);
	}
	else 
	{
		if ((ntimer->callback = callback) != NULL)
			ntimer->command = (void *)what;
		else
			ntimer->command = m_strdup(what);
		if (subargs)
			ntimer->subargs = m_strdup(subargs);
	}

	if (create_timer_ref(refnum_want, refnum_got) == -1)
	{
		say("TIMER: Refnum %s already exists", refnum_want);
		new_free(&ntimer->command);
		new_free(&ntimer->subargs);
		new_free(&ntimer->whom);
		new_free(&ntimer);
		return NULL;
	}

	strcpy(ntimer->ref, refnum_got);	

	/* we've created it, now put it in order */
	return schedule_timer(ntimer);
}

static char *schedule_timer (TimerList *ntimer)
{
	TimerList **slot;

	/* we've created it, now put it in order */
	for (slot = &PendingTimers; *slot; slot = &(*slot)->next)
	{
		if (BX_time_diff((*slot)->time, ntimer->time) < 0)
			break;
	}
	ntimer->next = *slot;
	*slot = ntimer;
	return ntimer->ref;
}

static	struct timeval current;


/*
 * TimerTimeout:  Called from irc_io to help create the timeout
 * part of the call to select.
 */

time_t TimerTimeout (void)
{
	time_t	timeout_in;
	time_t t = MAGIC_TIMEOUT;
	
	if (is_server_queue() && (t = get_int_var(QUEUE_SENDS_VAR)))
		;
	else
		t = MAGIC_TIMEOUT;
	get_time(&current);
	if (!PendingTimers)
		timeout_in = tclTimerTimeout(MAGIC_TIMEOUT);
	else
	{
		timeout_in = (time_t)(BX_time_diff(current, PendingTimers->time) * 1000);
		timeout_in = (time_t)(tclTimerTimeout((timeout_in < 0) ? 0 : timeout_in));
	}
	
	if (t < timeout_in)
		timeout_in = t * 1000;
		
	return (timeout_in < 0) ? 0 : timeout_in + 100;
}
