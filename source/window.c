/*
 * window.c: Handles the Main Window stuff for irc.  This includes proper
 * scrolling, saving of screen memory, refreshing, clearing, etc. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 * Modified 1996 Colten Edwards
 */


#include "irc.h"
static char cvsrevision[] = "$Id: window.c 87 2010-06-26 08:18:34Z keaston $";
CVS_REVISION(window_c)
#include "struct.h"

#include "screen.h"
#include "commands.h"
#include "exec.h"
#include "window.h"
#include "vars.h"
#include "server.h"
#include "list.h"
#include "ircterm.h"
#include "names.h"
#include "ircaux.h"
#include "input.h"
#include "status.h"
#include "output.h"
#include "log.h"
#include "hook.h"
#include "dcc.h"
#include "misc.h"
#include "cset.h"
#include "module.h"
#include "gui.h"
#define MAIN_SOURCE
#include "modval.h"

/* Resize relatively or absolutely? */
#define RESIZE_REL 1
#define RESIZE_ABS 2

Window	*invisible_list = NULL;		/* list of hidden windows */

const char	*who_from = NULL;
unsigned long	who_level = LOG_CRAP;		/* Log level of message being displayed */

int	in_window_command = 0;		/* set to true if we are in window().  This
					 * is used if a put_it() is called within the
					 * window() command.  We make sure all
					 * windows are fully updated before doing the
					 * put_it(). 
					 */
extern	int	dead;
static char *onoff[] = {"OFF", "ON"};

extern unsigned long current_window_level;

#ifdef WANT_DLL
WindowDll *dll_window = NULL;
#endif

/*
 * window_display: this controls the display, 1 being ON, 0 being OFF.  The
 * DISPLAY var sets this. 
 */
unsigned int	window_display = 1;

/*
 * status_update_flag: if 1, the status is updated as normal.  If 0, then all
 * status updating is suppressed 
 */
	int	status_update_flag = 1;


static 	void 	remove_from_invisible_list (Window *window);
static 	void 	swap_window (Window *v_window, Window *window);
static	Window	*get_next_window  (Window *);
static	Window	*get_previous_window (Window *);
static 	void 	revamp_window_levels (Window *window);
static	void	resize_window_display (Window *window);
static	Window	*window_next (Window *window, char **args, char *usage);
static	Window	*window_previous (Window *window, char **args, char *usage);


/*
 * this is set to the window output should appear in.
 */
	Window	*target_window = NULL;

	Window	*current_window = NULL;

void *default_output_function = BX_add_to_window;


#ifdef GUI
MenuStruct *findmenu(char *menuname);
#endif /* GUI */

/*
 * new_window: This creates a new window on the screen.  It does so by either
 * splitting the current window, or if it can't do that, it splits the
 * largest window.  The new window is added to the window list and made the
 * current window 
 */
Window	*BX_new_window(Screen *screen)
{
	Window	*new;
	Window	*tmp = NULL;
	unsigned int new_refnum = 1;
		
	if (dumb_mode && current_window)
		return NULL;

	new = (Window *) new_malloc(sizeof(Window));

	new->output_func = default_output_function;
	new->update_status = NULL;

	tmp = NULL;
	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->refnum == new_refnum)
		{
			new_refnum++;
			tmp = NULL;
		}
	}
	new->refnum = new_refnum;
	new->name = NULL;
		
	if (current_window)
		new->server = current_window->server;
	else
		new->server = primary_server;


#if 0
	if (!current_window)
		new->window_level = LOG_ALL;
	else
#endif
		new->window_level = LOG_NONE;
	
	new->lastlog_level = real_lastlog_level();
	new->lastlog_max = get_int_var(LASTLOG_VAR);
	new->status_split = 1;	
	new->scratch_line = -1;

#ifdef DEFAULT_DOUBLE_STATUS
	new->double_status = DEFAULT_DOUBLE_STATUS;
#else
 	if (new->refnum == 1)
  		new->double_status = 1;
  	else
  		new->double_status = 0;
#endif

#ifdef DEFAULT_STATUS_LINES
	new->status_lines = DEFAULT_STATUS_LINES;
#endif

	new->display_size = 1;
	new->old_size = 1;
	new->visible = 1;
	new->repaint_start = 0;
	new->repaint_end = -1;
	
	new->screen = screen;
	new->next = new->prev = NULL;
	
	new->notify_level = real_notify_level();

	new->display_buffer_max = get_int_var(SCROLLBACK_VAR);
	new->hold_mode = get_int_var(HOLD_MODE_VAR);

	new->mangler = logfile_line_mangler;
		
	create_wsets_for_window(new);
	if (screen)
	{
		if (add_to_window_list(screen, new))
		{
			set_screens_current_window(screen, new);
			term_flush();
			do_hook(WINDOW_CREATE_LIST, "%d", new->refnum);

		} 
		else
		{
			new_free(&new);
			return NULL;
		}
	}
	else
		add_to_invisible_list(new);
	build_status(new, NULL, 0);
		
	make_window_current(new);
	resize_window_display(new);
	return (new);
}

/*
 * delete_window: This deletes the given window.  It frees all data and
 * structures associated with the window, and it adjusts the other windows so
 * they will display correctly on the screen. 
 */
void BX_delete_window(Window *window)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	
	if (window == NULL)
		window = current_window;
	if (window->screen && (window->screen->visible_windows == 1))
	{
		if (invisible_list)
		{
			window->deceased = 1;
			swap_window(window, invisible_list);
			window = invisible_list;
		}
		else if (!dead)
		{
			if (!get_int_var(WINDOW_QUIET_VAR))
				say("You can't kill the last window!");
			return;
		}
		else
		{
			window->deceased = 1;
			set_screens_current_window(window->screen, NULL);
		}
	}
	if (window->name)
		strmcpy(buffer, window->name, BIG_BUFFER_SIZE - 1);
	else
		sprintf(buffer, "%u", window->refnum);

	/*
	 * If this window is the "previous" window, then we make the current
	 * window the "previous" window.  Um. right.
	 */
	if (!dead && window->screen && window->screen->last_window_refnum == window->refnum)
		window->screen->last_window_refnum = window->screen->current_window->refnum;

	move_window_channels(window);

	new_free(&window->query_nick);
	new_free(&window->query_host);
	new_free(&window->query_cmd);
	new_free(&window->current_channel);
	new_free(&window->bind_channel);
	new_free(&window->waiting_channel);
	new_free(&window->logfile);
	new_free(&window->name);
	
	/*
	 * Free off the display
	 */
	{ 
		Display *next;
		while (window->top_of_scrollback)
		{
			next = window->top_of_scrollback->next;
			new_free(&window->top_of_scrollback->line);
			new_free((char **)&window->top_of_scrollback);
			window->display_buffer_size--;
			window->top_of_scrollback = next;
		}
		window->display_ip = NULL;
		if (window->display_buffer_size != 0)
			ircpanic("display_buffer size is %d. should be 0", window->display_buffer_size);
	}

	/*
	 * Free off the lastlog
	 */
	while (window->lastlog_size)
		remove_from_lastlog(window);

	/*
	 * Free off the nick list
	 */
	{
		NickList *next;

		while (window->nicks)
		{
			next = window->nicks->next;
			new_free(&window->nicks->nick);
			new_free(&window->nicks->host);
			new_free((char **)&window->nicks);
			window->nicks = next;
		}
	}

	free_formats(window);
	
	if (window->screen)
		remove_window_from_screen(window);
	else
		remove_from_invisible_list(window);

  	/*
	 * If its the current window, choose another one.
	 */
	if (window->screen && window->screen->current_window == window)
		set_screens_current_window(window->screen, NULL);
	if (window == current_window)
	{
		if (window == last_input_screen->current_window)
			make_window_current(last_input_screen->window_list);
		else
			make_window_current(NULL);
	}
	if (target_window && (window == target_window))
		target_window = NULL;
	new_free((char **)&window);
	window_check_servers(current_window->server);
	do_hook(WINDOW_KILL_LIST, "%s", buffer);
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
	update_input(UPDATE_ALL);
}

  /*
 * new_traverse_all_windows: Based on the idea from phone that you should
 * be able to do more than one iteration simultaneously.
 *
 * To initialize, *ptr should be NULL.  The function will return 1 each time
 * *ptr is set to the next valid window.  When the function returns 0, then
 * you have iterated all windows.
 */
int BX_traverse_all_windows (Window **ptr)
{
	/*
	 * If this is the first time through...
	 */
	if (!*ptr)
	{
		Screen *screen = screen_list;
		while (screen && (!screen->alive || !screen->window_list))
			screen = screen->next;

		if (!screen && !invisible_list)
			return 0;
		else if (!screen)
			*ptr = invisible_list;
		else
 			*ptr = screen->window_list;
	}

	/*
	 * As long as there is another window on this screen, keep going.
	 */
	else if ((*ptr)->next)
	{
		*ptr = (*ptr)->next;
	}

	/*
	 * If there are no more windows on this screen, but we do belong to
	 * a screen (eg, we're not invisible), try the next screen
	 */
	else if ((*ptr)->screen)
	{
		/*
		 * Skip any dead screens
		 */
		Screen *ns = (*ptr)->screen->next;
		while (ns && (!ns->alive || !ns->window_list))
			ns = ns->next;

		/*
		 * If there are no other screens, then if there is a list
		 * of hidden windows, try that.  Otherwise we're done.
		 */
		if (!ns && !invisible_list)
			return 0;
		else if (!ns)
			*ptr = invisible_list;
		else
			*ptr = ns->window_list;
	}

	/*
	 * Otherwise there are no other windows, and we're not on a screen
	 * (eg, we're hidden), so we're all done here.
	 */
	else
		return 0;

	/*
	 * If we get here, we're in business!
	 */
	return 1;
}

static	void remove_from_invisible_list(Window *window)
{

	if (window->prev)
		window->prev->next = window->next;
	else
		invisible_list = window->next;
	if (window->next)
		window->next->prev = window->prev;
}

void BX_add_to_invisible_list(Window *window)
{
	if ((window->next = invisible_list) != NULL)
		invisible_list->prev = window;
	invisible_list = window;
	window->prev = NULL;
	window->visible = 0;
	if (window->screen)
		window->columns = window->screen->co;
	else
		window->columns = current_term->TI_cols;
	window->screen = NULL;
}

/*
 * add_to_window_list: This inserts the given window into the visible window
 * list (and thus adds it to the displayed windows on the screen).  The
 * window is added by splitting the current window.  If the current window is
 * too small, the next largest window is used.  The added window is returned
 * as the function value or null is returned if the window couldn't be added 
 */
Window	*BX_add_to_window_list(Screen *screen, Window *new)
{
	Window	*biggest = NULL,
		*tmp;

	screen->visible_windows++;
	new->screen = screen;
	new->visible = 1;
	new->miscflags &= ~WINDOW_NOTIFIED;
	if (!screen->current_window)
	{
		screen->window_list_end = screen->window_list = new;
		if (dumb_mode)
		{
			new->display_size = 24;	/* what the hell */
			set_screens_current_window(screen, new);
			return (new);
		}
		recalculate_windows(screen);
	}
	else
	{
		/* split current window, or find a better window to split */
		if ((screen->current_window->display_size < 4) || 
			get_int_var(ALWAYS_SPLIT_BIGGEST_VAR))
		{
			int	size = 0;

			for (tmp = screen->window_list; tmp; tmp = tmp->next)
			{
				if (tmp->absolute_size)
					continue;
				if (tmp->display_size > size)
				{
					size = tmp->display_size;
					biggest = tmp;
				}
			}
			if (!biggest/* || size < 4 */)
			{
				if (!get_int_var(WINDOW_QUIET_VAR))
					say("Not enough room for another window!");
				screen->visible_windows--;
				return (NULL);
			}
		}
		else
			biggest = screen->current_window;

		if ((new->prev = biggest->prev) != NULL)
			new->prev->next = new;
		else
			screen->window_list = new;

		new->next = biggest;
		biggest->prev = new;
		biggest->display_size /= 2;
		new->display_size = biggest->display_size;
		recalculate_windows(screen);
	}
	return (new);
}

/*
 * remove_from_window_list: this removes the given window from the list of
 * visible windows.  It closes up the hole created by the windows abnsense in
 * a nice way 
 */
void BX_remove_window_from_screen(Window *window)
{
	if (!window->visible || !window->screen)
		ircpanic("This window is not on a screen");

	/*
	 * We  used to go to greath lengths to figure out how to fill
	 * in the space vacated by this window.  Now we dont sweat that.
	 * we just blow away the window and then recalculate the entire
	 * screen.
	 */
	if (window->prev)
		window->prev->next = window->next;
	else
		window->screen->window_list = window->next;

	if (window->next)
		window->next->prev = window->prev;
	else
		window->screen->window_list_end = window->prev;

	if (!--window->screen->visible_windows)
		return;

	if (window->screen->current_window == window)
		set_screens_current_window(window->screen, NULL);

	if (window->refnum == window->screen->last_window_refnum)
		window->screen->last_window_refnum = window->screen->current_window->refnum;

	if (window == window->screen->current_window)
		make_window_current(last_input_screen->window_list);
	else
		make_window_current(NULL);

	recalculate_windows(window->screen);
}

/*
 * recalculate_window_positions: This runs through the window list and
 * re-adjusts the top and bottom fields of the windows according to their
 * current positions in the window list.  This doesn't change any sizes of
 * the windows 
 */
void BX_recalculate_window_positions(Screen *screen)
{
	Window	*tmp;
	int	top;

	if (!screen)
		return;
	top = 0;
	for (tmp = screen->window_list; tmp; tmp = tmp->next)
	{
		tmp->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
		tmp->top = top;
		tmp->bottom = top + tmp->display_size + tmp->status_lines;
		top += tmp->display_size + tmp->status_lines + 1 + tmp->double_status;
	}
}

/*
 * swap_window: This swaps the given window with the current window.  The
 * window passed must be invisible.  Swapping retains the positions of both
 * windows in their respective window lists, and retains the dimensions of
 * the windows as well 
 */
static	void swap_window(Window *v_window, Window *window)
{
	if (!window)
	{
		if (!get_int_var(WINDOW_QUIET_VAR))
			say("The window to be swapped in does not exist.");
		return;
	}

	if (window->visible || !v_window->visible)
	{
		if (!get_int_var(WINDOW_QUIET_VAR))
			say("You can only SWAP a hidden window with a visible window.");
		return;
	}

	v_window->screen->last_window_refnum = v_window->refnum;
	remove_from_invisible_list(window);

	window->top = v_window->top;
	
	window->display_size = v_window->display_size + 
				v_window->double_status - 
				window->double_status;

	window->bottom = window->top + window->display_size + window->status_lines;

	window->visible = 1;
	window->screen = v_window->screen;

	if (v_window->screen->current_window == v_window)
		v_window->screen->current_window = window;

	/*
	 * Put the window to be swapped into the screen list
	 */
	if ((window->prev = v_window->prev))
		window->prev->next = window;
	else
		window->screen->window_list = window;

	if ((window->next = v_window->next))
		window->next->prev = window;
	else
		window->screen->window_list_end = window;
	add_to_invisible_list(v_window);

	if (v_window == current_window)
		make_window_current(window);
		
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	window->miscflags &= ~WINDOW_NOTIFIED;
	update_input(UPDATE_ALL);
	set_screens_current_window(window->screen, window);
	recalculate_windows(window->screen);
	reset_display_target();
	do_hook(WINDOW_SWAP_LIST, "%d %d", v_window->refnum, window->refnum);
}

/*
 * move_window: This moves a window offset positions in the window list. This
 * means, of course, that the window will move on the screen as well 
 */
void BX_move_window(Window *window, int offset)
{
	Window	*tmp,
	    *last;
	int	win_pos,
	pos;

	if (offset == 0 || !window->screen)
		return;
	last = NULL;
	for (win_pos = 0, tmp = window->screen->window_list; tmp;
	    tmp = tmp->next, win_pos++)
	{
		if (window == tmp)
			break;
		last = tmp;
	}
	if (!tmp)
		return;
	if (!last)
		window->screen->window_list = tmp->next;
	else
		last->next = tmp->next;
	if (tmp->next)
		tmp->next->prev = last;
	else
		window->screen->window_list_end = last;

	win_pos = (offset + win_pos) % window->screen->visible_windows;
	if (win_pos < 0)
		win_pos = window->screen->visible_windows + win_pos;

	last = NULL;
	for (pos = 0, tmp = window->screen->window_list;
	    pos != win_pos; tmp = tmp->next, pos++)
		last = tmp;
	if (!last)
		window->screen->window_list = window;
	else
		last->next = window;

	if (tmp)
		tmp->prev = window;
	else
		window->screen->window_list_end = window;

	window->prev = last;
	window->next = tmp;
	recalculate_window_positions(window->screen);
}

/*
 * resize_window: if 'how' is RESIZE_REL, then this will increase or decrease
 * the size of the given window by offset lines (positive offset increases,
 * negative decreases).  If 'how' is RESIZE_ABS, then this will set the 
 * absolute size of the given window.
 * Obviously, with a fixed terminal size, this means that some other window
 * is going to have to change size as well.  Normally, this is the next
 * window in the window list (the window below the one being changed) unless
 * the window is the last in the window list, then the previous window is
 * changed as well 
 */
void BX_resize_window(int how, Window *window, int offset)
{
	Window	*other;
	int	after,
		window_size,
		other_size;

	if (!window)
		window = current_window;

	if (!window->visible)
	{
		if (!get_int_var(WINDOW_QUIET_VAR))
			say("You cannot change the size of hidden windows!");
		return;
	}

	if (how == RESIZE_ABS)
	{
		offset -= window->display_size;
		how = RESIZE_REL;
	}

	after = 1;
	other = window;

	do
	{
		if (other->next)
			other = other->next;
		else
		{
			other = window->screen->window_list;
			after = 0;
		}

		if (other == window)
		{
			say("Can't change the size of this window!");
			return;
		}

		if (other->absolute_size)
			continue;
	}
	while (/*other->absolute_size || */other->display_size < offset);

	window_size = window->display_size + offset;
	other_size = other->display_size - offset;

#if 0
	if (how == RESIZE_REL)
	{
		window_size = window->display_size + offset;
		other_size = other->display_size - offset;
	}
	else /* absolute size */
	{
		/* 
		 * How much its growing/shrinking by.  if
		 * offset > display_size, then window_size < 0.
		 * and other window is shrinking.  If offset < display_size,
		 * the window_size > 0, and other_window is growing.
		 */
		window_size = offset;
		offset -= window->display_size;
		other_size = other->display_size - offset;
	}
#endif

	if ((window_size < 0) || (other_size < 0))
	{
		if (!get_int_var(WINDOW_QUIET_VAR))
			say("Not enough room to resize this window!");
		return;
	}

	window->display_size = window_size;
	other->display_size = other_size;
	recalculate_windows(window->screen);
}

/*
 * resize_display: After determining that the screen has changed sizes, this
 * resizes all the internal stuff.  If the screen grew, this will add extra
 * empty display entries to the end of the display list.  If the screen
 * shrank, this will remove entries from the end of the display list.  By
 * doing this, we try to maintain as much of the display as possible. 
 *
 * This has now been improved so that it returns enough information for
 * redraw_resized to redisplay the contents of the window without having
 * to redraw too much.
 */
void resize_window_display(Window *window)
{
	int	cnt = 0, i;
	Display *tmp;

	if (dumb_mode)
		return;
	/*
	 * This is called in new_window to initialize the
	 * display the first time
	 */
	if (!window->top_of_scrollback)
	{
		window->top_of_scrollback = new_display_line(NULL);
		window->top_of_scrollback->line = NULL;
		window->top_of_scrollback->next = NULL;
		window->display_buffer_size = 1;
		window->display_ip = window->top_of_scrollback;
		window->top_of_display = window->top_of_scrollback;
		window->ceiling_of_display = window->top_of_display; 
		window->old_size = 1;
	}
	else if (window->scrollback_point)
		;
	else
	{

		/*
		 * Find out how much the window has changed by
		 */
		cnt = window->display_size - window->old_size;
		tmp = window->top_of_display;

		/*
		 * If it got bigger, move the top_of_display back.
		 */
		if (cnt > 0)
		{
			for (i = 0; i < cnt; i++)
			{
				if (!tmp || !tmp->prev || tmp == window->ceiling_of_display)
					break;
				tmp = tmp->prev;
			}
		}

		/*
		 * If it got smaller, then move the top_of_display up
		 */
		else if (cnt < 0)
		{
			/* Use any whitespace we may have lying around */
			cnt += (window->old_size - window->distance_from_display);
			for (i = 0; i > cnt; i--)
			{
				if (tmp == window->display_ip)
					break;
				tmp = tmp->next;
			}
		}
		window->top_of_display = tmp;
		recalculate_window_cursor(window);
	}

	/*
	 * Mark the window for redraw and store the new window size.
	 */
	window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	window->old_size = window->display_size;
	return;
}

/*
 * redraw_all_windows: This basically clears and redraws the entire display
 * portion of the screen.  All windows and status lines are draws.  This does
 * nothing for the input line of the screen.  Only visible windows are drawn 
 */
void BX_redraw_all_windows(void)
{
	Window	*tmp = NULL;

	if (dumb_mode)
		return;
	while (traverse_all_windows(&tmp))
		tmp->update = REDRAW_STATUS | REDRAW_DISPLAY_FAST;
}

/*
 * Rebalance_windows: this is called when you want all the windows to be
 * rebalanced, except for those who have a set size.
 */
void BX_rebalance_windows (Screen *screen)
{
	Window *tmp;
	int each, extra;
	int window_resized = 0, window_count = 0;

	if (dumb_mode)
		return;

	/*
	 * Two passes -- first figure out how much we need to balance,
	 * and how many windows there are to balance
	 */
	for (tmp = screen->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->absolute_size)
			continue;
		window_resized += tmp->display_size;
		window_count++;
	}

	if (!window_count)
	{
		yell("All the windows on this screen are fixed");
		return;
	}

	each = window_resized / window_count;
	extra = window_resized % window_count;

	/*
	 * And then go through and fix everybody
	 */
	for (tmp = screen->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->absolute_size)
			;
		else
		{
			tmp->display_size = each;
			if (extra)
				tmp->display_size++, extra--;
		}
	}
	recalculate_window_positions(screen);
}



/*
 * recalculate_windows: this is called when the terminal size changes (as
 * when an xterm window size is changed).  It recalculates the sized and
 * positions of all the windows.  Currently, all windows are rebalanced and
 * window size proportionality is lost 
 */
void BX_recalculate_windows (Screen *screen)
{
	int	old_li = 1;
	int	excess_li = 0;
	Window	*tmp;
	int	window_count = 0;
	int	window_resized = 0;
	int	offset;
	int	split = 0;
	
	if (dumb_mode)
		return;
#ifdef GUI
	current_term->TI_lines = screen->li;
	current_term->TI_cols = screen->co;
#endif
                
	if (!screen) /* it's a hidden window. ignore this */
		return;
	/*
	 * If its a new window, just set it and be done with it.
	 */
	if (screen && !screen->current_window)
	{
		screen->window_list->top = 0;
		screen->window_list->display_size = current_term->TI_lines - 2 - screen->window_list->double_status;
		screen->window_list->bottom = current_term->TI_lines - 2 - screen->window_list->double_status;
		old_li = current_term->TI_lines;
		return;
	}

	/* 
	 * Expanding the screen takes two passes.  In the first pass,
	 * We figure out how many windows will be resized.  If none can
	 * be rebalanced, we add the whole shebang to the last one.
	 */
	for (tmp = screen->window_list; tmp; tmp = tmp->next)
	{
		old_li += tmp->display_size + tmp->double_status + 1;
		if (tmp->absolute_size && (window_count || tmp->next))
			continue;
		window_resized += tmp->display_size;
		window_count++;
		if (tmp->status_lines)
			split += tmp->status_lines;
	}

	excess_li = current_term->TI_lines - old_li - split;

	for (tmp = screen->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->absolute_size && tmp->next)
			;
		else
		{
			/*
			 * The number of lines this window gets is:
			 * The number of lines available for resizing times 
			 * the percentage of the resizeable screen the window 
			 * covers.
			 */
			if (tmp->next && window_resized)
				offset = (tmp->display_size * excess_li) / window_resized;
			else
				offset = excess_li;

			tmp->display_size += offset;
			if (tmp->display_size < 0)
				tmp->display_size = 1;
			excess_li -= offset;
			tmp->bottom = tmp->bottom - tmp->status_lines;
		}
	}

	recalculate_window_positions(screen);
}

/*
 * update_all_windows: This goes through each visible window and draws the
 * necessary portions according the the update field of the window. 
 */
void BX_update_all_windows()
{
	Window	*tmp = NULL;
	if (in_window_command)
		return;

	while (traverse_all_windows(&tmp))
	{
		if (tmp->display_size != tmp->old_size)
			resize_window_display(tmp);
		if (tmp->visible && tmp->update)
		{
			int fast_window = tmp->update & REDRAW_DISPLAY_FAST;
			int full_window = tmp->update & REDRAW_DISPLAY_FULL;
			int r_status = tmp->update & REDRAW_STATUS;
			int u_status = tmp->update & UPDATE_STATUS;

			if (full_window || fast_window)
				repaint_window(tmp, tmp->repaint_start, tmp->repaint_end);

			if (tmp->update_status)
				(tmp->update_status)(tmp);
			else if (r_status)
				update_window_status(tmp, 1);
			else if (u_status)
				update_window_status(tmp, 0);
		}
		tmp->update = 0;
		tmp->repaint_start = 0;
		tmp->repaint_end = -1;
	}
	update_input(UPDATE_JUST_CURSOR);
}

/*
 * goto_window: This will switch the current window to the window numbered
 * "which", where which is 0 through the number of visible windows on the
 * screen.  The which has nothing to do with the windows refnum. 
 */
void BX_goto_window(Screen *s, int which)
{
	Window	*tmp;
	int	i;


	if (!s || which == 0)
		return;

	if ((which < 0) || (which > s->visible_windows))
	{
		if (!get_int_var(WINDOW_QUIET_VAR))
			say("GOTO: Illegal value");
		return;
	}
	tmp = s->window_list;
	for (i = 1; i < which; i++)
		tmp = tmp->next;

	set_screens_current_window(s, tmp);
	make_window_current(tmp);
}

/*
 * hide_window: sets the given window to invisible and recalculates remaing
 * windows to fill the entire screen 
 */
void BX_hide_window(Window *window)
{
	if (!window->screen)
	{
		say("You can't hide an invisible window.");
		return;
	}
	if (window->screen->visible_windows == 1)
	{
		if (!get_int_var(WINDOW_QUIET_VAR))
			say("You can't hide the last window.");
		return;
	}
	if (window->screen)
	{
		remove_window_from_screen(window);
		add_to_invisible_list(window);
	}
}

/*
 * swap_last_window:  This swaps the current window with the last window
 * that was hidden.
 */

void BX_swap_last_window(char key, char *ptr)
{
	if (!invisible_list || !current_window->screen)
		return;

	swap_window(current_window, invisible_list);
	reset_display_target();
	update_all_windows();
	cursor_to_input();
}

/*
 * swap_next_window:  This swaps the current window with the next hidden
 * window.
 */

void BX_swap_next_window(char key, char *ptr)
{
	window_next(current_window, NULL, NULL);
	update_all_windows();
}

/*
 * swap_previous_window:  This swaps the current window with the next 
 * hidden window.
 */

void BX_swap_previous_window(char key, char *ptr)
{
	window_previous(current_window, NULL, NULL);
	cursor_to_input();
	update_all_windows();
}

/* show_window: This makes the given window visible.  */
void BX_show_window(Window *window)
{
	if (!window->screen)
	{
		remove_from_invisible_list(window);
		if ((window == current_window) && !current_window->screen)
			window->screen = last_input_screen; /* What the hey */
		if (!add_to_window_list(current_window->screen, window))
			add_to_invisible_list(window);
	}
	make_window_current(window);
	if (!window->screen)
	{
		yell("ERROR ERROR ERROR. screen == NULL");
		return;
	}
	set_screens_current_window(window->screen, window); 
	return;
}

/* 
 * XXXX i have no idea if this belongs here.
 */
char *BX_get_status_by_refnum(unsigned refnum, unsigned line)
{
	Window *the_window;

	if ((the_window = get_window_by_refnum(refnum)))
	{
		if (line > the_window->double_status)
			return empty_string;

		return the_window->wset->status_line[line];
	}
	else
		return empty_string;
}

/*
 * get_window_by_desc: Given either a refnum or a name, find that window
 */
Window *BX_get_window_by_desc (const char *stuff)
{
Window *w = NULL;
	do 
	{
		if ((w = get_window_by_name(stuff)))
			break;
		if (is_number(stuff) && (w = get_window_by_refnum(my_atol(stuff))))
			break;
		if (*stuff == '#')
		{
			stuff++;
			continue;
		}	
	} while (0);
	return w;
}

/*
 * get_window_by_refnum: Given a reference number to a window, this returns a
 * pointer to that window if a window exists with that refnum, null is
 * returned otherwise.  The "safe" way to reference a window is throught the
 * refnum, since a window might be delete behind your back and and Window
 * pointers might become invalid.  
 */
Window	* BX_get_window_by_refnum(unsigned int refnum)
{
	Window	*tmp = NULL;

	if (refnum < 0)
		return NULL;
	if (refnum == 0)
		return current_window;
	else while ((traverse_all_windows(&tmp)))
	{
		if (tmp->refnum == refnum)
			return (tmp);
	}
	return NULL;
}

/*
 * get_window: this parses out any window (visible or not) and returns a
 * pointer to it 
 */
int BX_get_visible_by_refnum (char *args)
{
	char	*arg;
	Window	*tmp;

	if ((arg = next_arg(args, &args)) != NULL)
	{
		if (is_number(arg))
		{
			if ((tmp = get_window_by_refnum(my_atol(arg))) != NULL)
				return tmp->visible;
		}
		if ((tmp = get_window_by_name(arg)) != NULL)
			return tmp->visible;

		return -1;
	}
	return -1;
}

/*
 * get_window_by_name: returns a pointer to a window with a matching logical
 * name or null if no window matches 
 */
Window	* BX_get_window_by_name(const char *name)
{
	Window	*tmp = NULL;

	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->name && (my_stricmp(tmp->name, name) == 0))
			return (tmp);
	}
	return (NULL);
}


/*
 * get_next_window: This returns a pointer to the next *visible* window in
 * the window list.  It automatically wraps at the end of the list back to
 * the beginning of the list 
 */
static	Window	* get_next_window (Window *w)
{
	Window *last = w;
	Window *new = w;

	if (!w || !w->screen)
		last = new = w = current_window;

	do
	{
		if (new->next)
			new = new->next;
		else
			new = w->screen->window_list;
	}
	while (new && new->skip && new != last);
	return new;
}

/*
 * get_previous_window: this returns the previous *visible* window in the
 * window list.  This automatically wraps to the last window in the window
 * list 
 */
static	Window	* get_previous_window (Window *w)
{
	Window *last = w;
	Window *new = w;

	if (!w || !w->screen)
		last = new = w = current_window;

	do
	{
		if (new->prev)
			new = new->prev;
		else
			new = w->screen->window_list_end;
	}
	while (new->skip && new != last);

	return new;
}

/*
 * next_window: This switches the current window to the next visible window 
 */
void BX_next_window(char key, char *ptr)
{
	Window *w;
	if (!last_input_screen)
		return;
	if (last_input_screen->visible_windows == 1)
		return;
	w = get_next_window(last_input_screen->current_window);
	make_window_current(w);
	set_screens_current_window(last_input_screen, w);
	update_all_windows();
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
}


/*
 * previous_window: This switches the current window to the previous visible
 * window 
 */
void BX_previous_window(char key, char *ptr)
{
	Window *w;
	
	if (!last_input_screen || last_input_screen->visible_windows == 1)
		return;
	w = get_previous_window(last_input_screen->current_window);
	make_window_current(w);
	set_screens_current_window(last_input_screen, w);
	update_all_windows();
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
}



/*
 * update_window_status: This updates the status line for the given window.
 * If the refresh flag is true, the entire status line is redrawn.  If not,
 * only some of the changed portions are redrawn 
 */
void BX_update_window_status(Window *window, int refresh)
{
	if (!window)
		window = current_window;
	if (!window || !window->visible || !status_update_flag || never_connected)
		return;
	if (refresh) 
	{
		new_free(&(window->wset->status_line[0]));
		new_free(&(window->wset->status_line[1]));
		new_free(&(window->wset->status_line[2]));
	}
	make_status(window);
}

/*
 * update_all_status: This updates all of the status lines for all of the
 * windows.  By updating, it only draws from changed portions of the status
 * line to the right edge of the screen 
 */
void BX_update_all_status(Window *win, char *unused, int unused1)
{
	Window	*window = NULL;
	extern	int foreground;
	
	if (dumb_mode || !status_update_flag || never_connected || !foreground)
		return;
	while (traverse_all_windows(&window))
	{
#if 0
		if (window->update & UPDATE_STATUS)
			continue;
#endif
		if (!window->visible)
			break;
		make_status(window);
	}
	update_input(UPDATE_JUST_CURSOR);
}

void BX_update_window_status_all(void)
{
Window *win  = NULL;
	while ((traverse_all_windows(&win)))
	{
		remove_wsets_for_window(win);
		win->wset = create_wsets_for_window(win);
	}
	update_all_status(win, NULL, 0);
	update_all_windows();
}


/*
 * status_update: sets the status_update_flag to whatever flag is.  This also
 * calls update_all_status(), which will update the status line if the flag
 * was true, otherwise it's just ignored 
 */
int BX_status_update(int flag)
{
	int old_flag = status_update_flag;
	status_update_flag = flag;
	update_all_status(current_window, NULL, 0);
	cursor_to_input();
	return old_flag;
}


/*
 * set_prompt_by_refnum: changes the prompt for the given window.  A window
 * prompt will be used as the target in place of the query user or current
 * channel if it is set 
 */
void BX_set_prompt_by_refnum(unsigned int refnum, char *prompt)
{
	Window	*tmp;

	if (!(tmp = get_window_by_refnum(refnum)))
		tmp = current_window;
	malloc_strcpy(&(tmp->prompt), prompt);
}

/* get_prompt_by_refnum: returns the prompt for the given window refnum */
char	*BX_get_prompt_by_refnum(unsigned int refnum)
{
	Window	*tmp;

	if (!(tmp = get_window_by_refnum(refnum)))
		tmp = current_window;
	if (tmp->prompt)
		return (tmp->prompt);
	else
		return (empty_string);
}

/*
 * get_target_by_refnum: returns the target for the window with the given
 * refnum (or for the current window).  The target is either the query nick
 * or current channel for the window 
 */
char	*BX_get_target_by_refnum(unsigned int refnum)
{
	Window	*tmp;

	if (!(tmp = get_window_by_refnum(refnum)))
		if (!(tmp = last_input_screen->current_window))
			return NULL;
	if (tmp->query_nick)
		return tmp->query_nick;
	else if (tmp->current_channel)
		return tmp->current_channel;

	return NULL;
}

char	*BX_get_target_cmd_by_refnum(unsigned int refnum)
{
	Window *tmp;
	if (!(tmp = get_window_by_refnum(refnum)))
		if (!(tmp = last_input_screen->current_window))
			return NULL;
	return tmp->query_cmd ? tmp->query_cmd : NULL;
}

Window *BX_get_window_target_by_desc(char *name)
{
Window *tmp = NULL;
unsigned long level = 0;
	level = parse_lastlog_level(name, 0);
	while ((traverse_all_windows(&tmp)))
	{
		if (is_channel(name) && tmp->server != -1)
		{
			ChannelList *chan, *chan2;
			chan2 = get_server_channels(tmp->server);
			if ((chan = (ChannelList *)find_in_list((List **)&chan2, (char *)name, 0)))
				if (chan->window && (chan->window == tmp))
					return tmp;
		} 
		else if (tmp->query_nick && !my_stricmp(tmp->query_nick, name))
			return tmp;
		else if (find_in_list((List **)&tmp->nicks, (char *)who_from, 0))
			return tmp;
		else if (level && tmp->window_level && (tmp->window_level & level))
			return tmp;
	}
	return NULL;
}

#if 0
/* set_query_nick: sets the query nick for the current channel to nick */
void set_query_nick(char *nick, char *host, char *cmd)
{
	NickList *tmp;
	char	*old_nick;
	if (!nick && !host && cmd)
	{
		malloc_strcpy(&current_window->query_cmd, cmd);
		return;
	}
	if ((old_nick = current_window->query_nick))
	{
		char	*n;
		while ((n = next_in_comma_list(old_nick, &old_nick)))
		{
			if (!*n)
				break;
			if ((tmp = (NickList *) remove_from_list((List **) &(current_window->nicks), n)) != NULL)
			{
				new_free(&tmp->nick);
				new_free(&tmp->host);
				new_free((char **)&tmp);
			}
		}
		new_free(&current_window->query_nick);
		new_free(&current_window->query_host);
		new_free(&current_window->query_cmd);
	}
	if (nick)
	{
		malloc_strcpy(&current_window->query_nick, nick);
		malloc_strcpy(&current_window->query_host, host);
		if (cmd)
			malloc_strcpy(&current_window->query_cmd, cmd);
		current_window->update |= UPDATE_STATUS;
		while ((old_nick = next_in_comma_list(nick, &nick)))
		{
			if (!*old_nick)
				break;
			tmp = (NickList *) new_malloc(sizeof(NickList));
			malloc_strcpy(&tmp->nick, old_nick);
			malloc_strcpy(&tmp->host, host);
			add_to_list((List **) &(current_window->nicks), (List *) tmp);
		}
	}
	update_window_status(current_window, 0);
}
#endif
/*
 * is_current_channel: Returns true is channel is a current channel for any
 * winDow.  If the delete flag is not 0, then unset channel as the current
 * channel and attempt to replace it by a non-current channel or the 
 * current_channel of window specified by value of delete
 */
int BX_is_current_channel(char *channel, int server, int delete)
{

	Window	*tmp = NULL;
	int	found = 0;

	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->current_channel &&
		    !my_stricmp(channel, tmp->current_channel) &&
		    (tmp->server == from_server))
		{
			found = 1;
			if (delete)
			{
				new_free(&(tmp->current_channel));
				tmp->update |= UPDATE_STATUS;
			}
		}
	}
	return (found);
}

/*
 * set_current_channel_by_refnum: This sets the current channel for the current
 * window. It returns the current channel as it's value.  If channel is null,
 * the * current channel is not changed, but simply reported by the function
 * result.  This treats as a special case setting the current channel to
 * channel "0".  This frees the current_channel for the
 * output_screen->current_window, * setting it to null 
 */
const char	*BX_set_current_channel_by_refnum(unsigned int refnum, char *channel)
{
	Window	*tmp;
	char *oldc;
	if ((tmp = get_window_by_refnum(refnum)) == NULL)
		tmp = current_window;

	oldc = tmp->current_channel;
	if (!channel || (channel && !strcmp(channel, zero)))
		tmp->current_channel = NULL;
	else
		tmp->current_channel = m_strdup(channel);

	new_free(&tmp->waiting_channel);
	tmp->update |= UPDATE_STATUS;
	set_channel_window(tmp, tmp->current_channel, tmp->server);
	do_hook(SWITCH_CHANNELS_LIST, "%d %s %s", refnum, 
			oldc ? oldc : zero, 
			tmp->current_channel ? tmp->current_channel : zero);
#ifdef GUI
	if(tmp->current_channel)
		gui_update_nicklist(tmp->current_channel);
#endif
	new_free(&oldc);
	return (channel);
}

/* get_current_channel_by_refnum: returns the current channel for window refnum */
char	* BX_get_current_channel_by_refnum(unsigned int refnum)
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == NULL)
		tmp = current_window;
	return (tmp ? tmp->current_channel : NULL);
}

char *BX_get_refnum_by_window(const Window *w)
{
	return w ? ltoa(w->refnum) : NULL;
}
 

int	BX_is_bound_to_window (const Window *window, const char *channel)
{
	if (window->bind_channel)
	{
		char *p, *q;
		q = p = LOCAL_COPY(window->bind_channel);
		while ((p = next_in_comma_list(q, &q)))
		{
			if (!p || !*p) break;
			if (!my_stricmp(p, channel))
				return 1;
		}
	}
	return 0;
}

Window *BX_get_window_bound_channel (const char *channel)
{
	Window *tmp = NULL;

	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->bind_channel)
		{
			char *p, *q;
			if (!channel) 
				return tmp;
			q = p = LOCAL_COPY(tmp->bind_channel);
			while ((p = next_in_comma_list(q, &q)))
			{
				if (!p || !*p) break;
				if (!my_stricmp(p, channel))
					return tmp;
			}
		}
	}
	return NULL;
}

int BX_is_bound_anywhere (const char *channel)
{
	Window *tmp = NULL;

	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->bind_channel)
		{
			char *p, *q;
			q = p = LOCAL_COPY(tmp->bind_channel);
			while ((p = next_in_comma_list(q, &q)))
			{
				if (!p || !*p) break;
				if (!my_stricmp(p, channel))
					return 1;
			}
		}
	}
	return 0;
}

extern int BX_is_bound (const char *channel, int server)
{
	Window *tmp = NULL;

	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->server == server && tmp->bind_channel)
		{
			char *p, *q;
			q = p = LOCAL_COPY(tmp->bind_channel);
			while ((p = next_in_comma_list(q, &q)))
			{
				if (!p || !*p) break;
				if (!my_stricmp(p, channel))
					return 1;
			}
		}
	}
	return 0;
}

void BX_unbind_channel (const char *channel, int server)
{
	Window *tmp = NULL;

	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->server == server && tmp->bind_channel)
		{
			char *p, *q, *new_bind = NULL;
			if (!strchr(tmp->bind_channel, ','))
			{
				new_free(&tmp->bind_channel);
				tmp->bind_channel = NULL;
				return;
			}
			q = p = LOCAL_COPY(tmp->bind_channel);
			while ((p = next_in_comma_list(q, &q)))
			{
				if (!p || !*p) 
					break;
				if (!my_stricmp(p, channel))
					continue;
				m_s3cat(&new_bind, comma, p);
			}
			malloc_strcpy(&tmp->bind_channel, new_bind);
			new_free(&new_bind);
			return;
		}
	}
}

char *BX_get_bound_channel (Window *window)
{
	return window ? window->bind_channel : NULL;
}

/*
 * get_
 window_server: returns the server index for the window with the given
 * refnum 
 */
int BX_get_window_server(unsigned int refnum)
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == NULL)
		tmp = current_window;
	return tmp ? tmp->server : -1;
}

/*
 * set_window_server:  This sets the server of the given window to server. 
 * If refnum is -1 then we are setting the primary server and all windows
 * that are set to the current primary server are changed to server.  The misc
 * flag is ignored in this case.  If refnum is not -1, then that window is
 * set to the given server.  If the misc flag is set as well, then all windows
 * with the same server as renum are set to the new server as well 
 * if refnum == -2, then, we are setting the server group passed in the misc
 * variable to the server.
 */
void BX_set_window_server(int refnum, int server, int misc)
{
	Window	*tmp = NULL,
		*window;
	int	old;

	if (refnum == -1)
	{
		while ((traverse_all_windows(&tmp)))
		{
			if (tmp->server == primary_server)
				tmp->server = server;
		}
		window_check_servers(server);
		primary_server = server;
	}
	else
	{
		if ((window = get_window_by_refnum(refnum)) == NULL)
			window = current_window;
		old = window->server;
		if (misc || old == WINDOW_SERVER_CLOSED)
		{
			while ((traverse_all_windows(&tmp)))
			{
				if (tmp->server == old)
					tmp->server = server;
			}
		}
		else
			window->server = server;
		window_check_servers(window->server);
	}
}

/*
 * window_check_servers: this checks the validity of the open servers vs the
 * current window list.  Every open server must have at least one window
 * associated with it.  If a window is associated with a server that's no
 * longer open, that window's server is set to the primary server.  If an
 * open server has no assicatiate windows, that server is closed.  If the
 * primary server is no more, a new primary server is picked from the open
 * servers 
 */

void BX_window_check_servers(int unused)
{
	Window	*tmp;
	int	cnt, max, i, not_connected, prime = -1;

	connected_to_server = 0;
	max = server_list_size();
	for (i = 0; i < max; i++)
	{
		not_connected = !is_server_open(i);
		tmp = NULL;
		cnt = 0;
		while ((traverse_all_windows(&tmp)))
		{
			if (tmp->server == i)
			{
				if (not_connected)
					tmp->server = primary_server;
				else
				{
					prime = tmp->server;
					cnt++;
				}
			}
		}
		if (cnt == 0)
		{
			if(!not_connected)
			{
				if(!get_server_change_pending(i))
					close_server(i, "No windows for this server");
				else
					close_unattached_server(i);
			}
		}
		else
			connected_to_server++;
	}
	
		
	if (!is_server_open(primary_server))
	{
		tmp = NULL;
		while ((traverse_all_windows(&tmp)))
			if (tmp->server == primary_server)
				tmp->server = prime;
		primary_server = prime;
	}
	update_all_status(current_window, NULL, 0);
	cursor_to_input();
}

/*
 * Changes any windows that are currently using "old_server" to instead
 * use "new_server".
 */
void	BX_change_window_server (int old_server, int new_server)
{
	Window *tmp = NULL;

	while (traverse_all_windows(&tmp))
	{
		if (tmp->server == old_server)
			tmp->server = new_server;
	}
	window_check_servers(old_server);
}

/*
 * set_level_by_refnum: This sets the window level given a refnum.  It
 * revamps the windows levels as well using revamp_window_levels() 
 */
void BX_set_level_by_refnum(unsigned int refnum, unsigned long level)
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == NULL)
		tmp = current_window;
	tmp->window_level = level;
	revamp_window_levels(tmp);
}

/*
 * revamp_window_levels: Given a level setting for the current window, this
 * makes sure that that level setting is unused by any other window. Thus
 * only one window in the system can be set to a given level.  This only
 * revamps levels for windows with servers matching the given window 
 * it also makes sure that only one window has the level `DCC', as this is
 * not dependant on a server.
 */
void revamp_window_levels(Window *window)
{
	Window	*tmp = NULL;
	int	got_dcc;

	got_dcc = (LOG_DCC & window->window_level) ? 1 : 0;
	while ((traverse_all_windows(&tmp)))
	{
		if (tmp == window)
			continue;
		if (LOG_DCC & tmp->window_level)
		{
			if (0 != got_dcc)
				tmp->window_level &= ~LOG_DCC;
			got_dcc = 1;
		}
		if (window->server == tmp->server)
			tmp->window_level ^= (tmp->window_level & window->window_level);
	}
}

/*
 * message_to: This allows you to specify a window (by refnum) as a
 * destination for messages.  Used by EXEC routines quite nicely 
 */
void BX_message_to(unsigned long refnum)
{
	target_window = (refnum) ? get_window_by_refnum(refnum) : NULL;
}

#if 0
/*
 * save_message_from: this is used to save (for later restoration) the
 * who_from variable.  This comes in handy very often when a routine might
 * call another routine that might change who_from. 
 */
void save_message_from(char **saved_who_from, unsigned long *saved_who_level)
{
	*saved_who_from = who_from;
	*saved_who_level = who_level;
}

/* restore_message_from: restores a previously saved who_from variable */
void restore_message_from (char *saved_who_from, unsigned long saved_who_level)
{
	who_from = saved_who_from;
	who_level = saved_who_level;
}

/*
 * message_from: With this you can the who_from variable and the who_level
 * variable, used by the display routines to decide which window messages
 * should go to.  
 */
void message_from(char *who, unsigned long level)
{
	static unsigned long saved_lastlog_level = LOG_ALL;

	if (level == LOG_CURRENT)
		set_lastlog_msg_level(saved_lastlog_level);
	else
		saved_lastlog_level = set_lastlog_msg_level(level);
	who_from = who;
	who_level = level;
}

/*
 * message_from_level: Like set_lastlog_msg_level, except for message_from.
 * this is needed by XECHO, because we could want to output things in moRe
 * than one level.
 */
int message_from_level(unsigned long level)
{
	int	temp;

	temp = who_level;
	who_level = level;
	return temp;
}
#else

/*
 * save_message_from: this is used to save (for later restoration) the
 * who_from variable.  This is needed when a function (do_hook) is about 
 * to call another function (parse_line) it knows will change who_from.
 * The values are saved on the stack so it will be recursive-safe.
 *
 * NO CHEATING when you call this function to get the value of who_from! ;-)
 */
void 	BX_save_display_target (const char **saved_from, unsigned long *saved_level)
{
	*saved_from = who_from;
	*saved_level = who_level;
}

/* restore_message_from: restores a previously saved who_from variable */
void 	BX_restore_display_target (const char *saved_from, unsigned long saved_level)
{
	who_from = saved_from;
	who_level = saved_level;
}

/*
 * is "word" in comma-separated "list" ?
 *  --einride
 */
int wordinlist(const char * word, const char * list)
{
	char * nw;
	int wl;
	if (!word || !list || !word[0] || !list[0])
		return 0;
	wl = strlen(word);
	nw = (char *) list;
	while (nw) {
		if (!my_strnicmp(word, nw, wl) && (!nw[wl] || (nw[wl]==',')))
			return 1;
		nw = strchr(nw, ',');
		if (nw)
			nw++;
	}
	return 0;
}

/*
 * message_from: With this you can set the who_from variable and the 
 * who_level variable, used by the display routines to decide which 
 * window messages should go to.  
 */
static	unsigned long	saved_lastlog_level = -1;

void 	BX_set_display_target (const char *who, unsigned long level)
{
	Window	*tmp;

#ifdef NO_CHEATING
	if (who)
		malloc_strcpy(&who_from, who);
	else
		new_free(&who_from);
#else
	who_from = who;
#endif
	who_level = level;
	saved_lastlog_level = set_lastlog_msg_level(level);


	/*
	 * Now we try to find the window that this output level would
	 * be directed to.  This was transplanted from add_to_screen.
	 */

	if (who_level == LOG_DEBUG && debugging_window)
	{
		target_window = debugging_window;
		return;
	}
	/*
	 * LOG_CURRENT means everything goes to the current window.
	 */
	if (who_level == LOG_CURRENT && current_window->server == from_server)
	{
		target_window = current_window;
		return;
	}
	/*
	 * Next priority is to honor who_from (using /window bind, 
	 * /window channel, or /window add)
	 */
	if (who_from)
	{
		tmp = NULL;
		while (traverse_all_windows(&tmp))
		{
			/*
			 * Check for /WINDOW CHANNELs that apply.
			 * (Any current channel will do)
			 */
			if (tmp->server != from_server && level != LOG_DCC)
				continue;
			if ((tmp->window_level & LOG_DEBUG) == LOG_DEBUG)
				continue;
			if (tmp->current_channel &&
				wordinlist(who_from, tmp->current_channel))
			{
				if (tmp->server == from_server)
				{
					target_window = tmp;
					return;
				}
			}
			if (tmp->bind_channel &&
				wordinlist(who_from, tmp->bind_channel))
			{
				if (tmp->server == from_server)
				{
					target_window = tmp;
					return;
				}
			}
			/*
			 * Check for /WINDOW QUERYs that apply.
			 */
			if (tmp->query_nick)
			{
			    if ((who_level == LOG_MSG || who_level == LOG_NOTICE
				|| who_level == LOG_DCC || who_level == LOG_CTCP
				|| who_level == LOG_ACTION)
				&& wordinlist(who_from, tmp->query_nick)
				&& from_server == tmp->server)
			    {
					target_window = tmp;
					return;
			    }
			    if ((who_level == LOG_DCC || who_level == LOG_CTCP
				|| who_level == LOG_ACTION)
				&& (*tmp->query_nick == '=' || *tmp->query_nick == '-')
				&& wordinlist(who_from, tmp->query_nick + 1))
			    {
					target_window = tmp;
					return;
			    }
			    if ((who_level == LOG_DCC || who_level == LOG_CTCP
			  	|| who_level == LOG_ACTION)
				&& *tmp->query_nick == '='
				&& wordinlist(who_from, tmp->query_nick))
			    {
					target_window = tmp;
					return;
			    }
			}
		}

		/*
		 * Check for /WINDOW NICKs that apply
		 */
		tmp = NULL;
		while (traverse_all_windows(&tmp))
		{
			if (tmp->nicks && from_server == tmp->server)
			{
				if (find_in_list((List **)&(tmp->nicks), 
					(char *)who_from, !USE_WILDCARDS))
				{
					target_window = tmp;
					return;
				}
			}
		}

		/*
		 * We'd better check to see if this should go to a
		 * specific window (i dont agree with this, though)
		 */
		if (is_channel((char *)who_from) && from_server > -1)
		{
			ChannelList *chan, *chan2;
			chan2 = get_server_channels(from_server);
			if ((chan = (ChannelList *)find_in_list((List **)&chan2,
				(char *)who_from, !USE_WILDCARDS)))
			{
				tmp = NULL;
				while ((traverse_all_windows(&tmp)))
				{
					if ((tmp->window_level & LOG_DEBUG) == LOG_DEBUG || !chan->window)
						continue;
					if (tmp->server != from_server)
						continue;
					if ((chan->refnum == tmp->refnum) && (tmp->server == chan->server))
					{
						target_window = tmp;
						return;
					}
				}
			}
		}
	}

	/*
	 * Check to see if this level should go to current window
	 */
	if ((current_window_level & who_level) && 
		current_window->server == from_server)
	{
		target_window = current_window;
		return;
	}
	/*
	 * Check to see if any window can claim this level
	 */
	tmp = NULL;
	while (traverse_all_windows(&tmp))
	{
		if ((tmp->window_level & LOG_DEBUG) == LOG_DEBUG)
			continue;
		/*
		 * Check for /WINDOW LEVELs that apply
		 */
		if (((from_server == tmp->server) || (from_server <= -1)) &&
		    (who_level & tmp->window_level))
		{
			target_window = tmp;
			return;
		}
	}

	/*
	 * If all else fails, if the current window is connected to the
	 * given server, use the current window.
	 */
	if (level == LOG_DCC || (from_server == current_window->server &&
		(current_window->window_level & LOG_DEBUG) != LOG_DEBUG))
	{
		target_window = current_window;
		return;
	}

	/*
	 * And if that fails, look for ANY window that is bound to the
	 * given server (this never fails if we're connected.)
	 */
	tmp = NULL;
	while (traverse_all_windows(&tmp))
	{
		if ((tmp->window_level & LOG_DEBUG) == LOG_DEBUG)
			continue;
		if (tmp->server == from_server)
		{
			target_window = tmp;
			return;
		}
	}

	/*
	 * No window found for a server is usually because we're
	 * disconnected or not yet connected.
	 */
	target_window = current_window;
	return;
}

void	BX_reset_display_target (void)
{
	set_display_target(NULL, LOG_CRAP);
}

void	BX_set_display_target_by_desc (char *desc)
{
	if (!desc)
		target_window = current_window;
	else if (!strcmp(desc, "-1"))
		return;
	else if (!(target_window = get_window_by_desc(desc)))
	{
		say("Window [%s] does not exist", desc);
		target_window = current_window;
	}
}

void	set_display_target_by_winref (unsigned int refnum)
{
	Window *w;

	if (refnum == -1)
		return;
	if (!(w = get_window_by_refnum(refnum)))
		say("Window [%d] does not exist", refnum);
	else
		target_window = w;
}

#endif


/*
 * message_from_level: Like set_lastlog_msg_level, except for message_from.
 * this is needed by XECHO, because we could want to output things in more
 * than one level.
 */
unsigned long message_from_level (unsigned long level)
{
	unsigned long temp;
	temp = who_level;
	who_level = level;
	return temp;
}



/*
 * clear_window: This clears the display list for the given window, or
 * current window if null is given.  
 */
void BX_clear_window(Window *window)
{
	if (dumb_mode)
		return;
	if (window->scratch_line != -1)
	{
		Display *curr_line;
		 /* Just walk every line and nuke whatever is in it */
		curr_line = window->top_of_display;
		while (curr_line && curr_line != window->display_ip)
		{
			malloc_strcpy(&curr_line->line, empty_string);
			curr_line = curr_line->next;
		}
	}
	else
	{
		window->top_of_display = window->display_ip;
		window->ceiling_of_display = window->top_of_display;
		window->cursor = 0;
		window->lines_scrolled_back = 0;
		window->scrollback_point = NULL;
		window->held_displayed = 0;
		if (window->miscflags & WINDOW_NOTIFIED)
			window->miscflags &= ~WINDOW_NOTIFIED;
	}
	repaint_window(window, 0, -1);
	update_window_status(window, 1);
#ifdef GUI
	gui_activity(COLOR_INACTIVE);
#endif
}

/* clear_all_windows: This clears all *visible* windows */
void BX_clear_all_windows(int unhold, int scrollback)
{
	Window	*tmp = NULL;

	while (traverse_all_windows(&tmp))
	{
		if (unhold)
			set_hold_mode(tmp, OFF, 1);
		if (scrollback)
			clear_scrollback(tmp);
		clear_window(tmp);
	}
}

/*
 * clear_window_by_refnum: just like clear_window(), but it uses a refnum. If
 * the refnum is invalid, the current window is cleared. 
 */
void BX_clear_window_by_refnum(unsigned int refnum)
{
	Window	*tmp;

	if ((tmp = get_window_by_refnum(refnum)) == NULL)
		tmp = current_window;
	clear_window(tmp);
}

static void	unclear_window (Window *window)
{
	int i;

	if (dumb_mode)
		return;
	if (window->scratch_line != -1)
		return;
	window->top_of_display = window->display_ip;
	for (i = 0; i < window->display_size; i++)
	{
		window->top_of_display = window->top_of_display->prev;
		if (window->top_of_display == window->top_of_scrollback)
			break;
	}
	window->ceiling_of_display = window->top_of_display;
	repaint_window(window, 0, -1);
	update_window_status(window, 0);
}

void	unclear_all_windows (int unhold, int visible, int hidden)
{
	Window *tmp = NULL;

	while (traverse_all_windows(&tmp))
	{
		if (visible && !hidden && !tmp->visible)
			continue;
		if (!visible && hidden && tmp->visible)
			continue;

		if (unhold)
			set_hold_mode(tmp, OFF, 1);
		unclear_window(tmp);
	}
}

void	BX_unclear_window_by_refnum (unsigned refnum)
{
	Window *tmp;

	if (!(tmp = get_window_by_refnum(refnum)))
		tmp = current_window;
	unclear_window(tmp);
}

/*
 * set_scroll_lines: called by /SET SCROLL_LINES to check the scroll lines
 * value 
 */
void BX_set_scroll_lines(Window *win, char *unused, int size)
{
	if (size == 0)
	{
		return;
	}
	else if (size > current_window->display_size)
	{
		say("Maximum lines that may be scrolled is %d", 
		    current_window->display_size);
		set_int_var(SCROLL_LINES_VAR, current_window->display_size);
	}
}

/*
 * set_continued_lines: checks the value of CONTINUED_LINE for validity,
 * altering it if its no good 
 */
void BX_set_continued_lines(Window *win, char *value, int unused)
{
	if (value && ((int) strlen(stripansicodes(value)) > (current_term->TI_cols / 2)))
		value[current_term->TI_cols / 2] = '\0';
}



void BX_free_formats(Window *window)
{
	remove_wsets_for_window(window);
}

/* current_refnum: returns the reference number for the current window */
unsigned int BX_current_refnum (void)
{
	return current_window->refnum;
}

int BX_number_of_windows_on_screen (Window *w)
{
	return w->screen->visible_windows;
}

/*
 * set_lastlog_size: sets up a lastlog buffer of size given.  If the lastlog
 * has gotten larger than it was before, all previous lastlog entry remain.
 * If it get smaller, some are deleted from the end. 
 */
void    BX_set_scrollback_size (Window *w, char *unused, int size)
{
        Window  *window = NULL;

        while (traverse_all_windows(&window))
        {
		if (size < window->display_size * 2)
			window->display_buffer_max = window->display_size * 2;
		else
			window->display_buffer_max = size;
        }
}

/*
 * is_window_name_unique: checks the given name vs the names of all the
 * windows and returns true if the given name is unique, false otherwise 
 */
int BX_is_window_name_unique(name)
	char	*name;
{
	Window	*tmp = NULL;

	if (name)
	{
		while ((traverse_all_windows(&tmp)))
		{
			if (tmp->name && !my_stricmp(tmp->name, name))
				return (0);
		}
	}
	return (1);
}

char  *BX_get_nicklist_by_window (Window *win)
{
	NickList *nick = win->nicks;
	char *stuff = NULL;
	for (; nick; nick = nick->next)
		m_s3cat(&stuff, space, nick->nick);
	if (!stuff)
		return m_strdup(empty_string);
	return stuff;
}


#define WIN_FORM "%-4s %*.*s %*.*s %*.*s %-9.9s %-10.10s %s%s"
static	void list_a_window(Window *window, int len)
{
	int	cnw = get_int_var(CHANNEL_NAME_WIDTH_VAR);

	say(WIN_FORM,           ltoa(window->refnum),
		      12, 12,   get_server_nickname(window->server),
		      len, len, window->name ? window->name : "<None>",
		      cnw, cnw, window->current_channel ? window->current_channel : "<None>",
		                window->query_nick ? window->query_nick : "<None>",
		                get_server_itsname(window->server) ? get_server_itsname(window->server) : "<None>",
		                bits_to_lastlog_level(window->window_level),
		                window->visible ? empty_string : " Hidden");
}

/* below is stuff used for parsing of WINDOW command */

/*
 * get_window: this parses out any window (visible or not) and returns a
 * pointer to it 
 */
static	Window	* get_window(char *name, char **args)
{
	char	*arg;
	Window	*tmp;

	if ((arg = next_arg(*args, args)) != NULL)
	{
		if (is_number(arg))
		{
			if ((tmp = get_window_by_refnum(my_atol(arg))) != NULL)
				return tmp;
		}
		if ((tmp = get_window_by_name(arg)) != NULL)
			return tmp;
		if (!get_int_var(WINDOW_QUIET_VAR))
			say("%s: No such window: %s", name, arg);
	}
	else
		say("%s: Please specify a window refnum or name", name);
	return NULL;
}

/*
 * get_invisible_window: parses out an invisible window by reference number.
 * Returns the pointer to the window, or null.  The args can also be "LAST"
 * indicating the top of the invisible window list (and thus the last window
 * made invisible) 
 */
static Window	*get_invisible_window(char *name, char **args)
{
	char	*arg;
	Window	*tmp;

	if ((arg = next_arg(*args, args)) != NULL)
	{
		if (my_strnicmp(arg, "LAST", strlen(arg)) == 0)
		{
			if (invisible_list == NULL)
			{
				if (!get_int_var(WINDOW_QUIET_VAR))
					say("%s: There are no hidden windows", name);
			}
			return invisible_list;
		}
		if ((tmp = get_window(name, &arg)) != NULL)
		{
			if (!tmp->visible)
				return (tmp);
			else if (!get_int_var(WINDOW_QUIET_VAR))
			{
				if (tmp->name)
					say("%s: Window %s is not hidden!",
						name, tmp->name);
				else
					say("%s: Window %d is not hidden!",
						name, tmp->refnum);
			}
		}
	}
	else
		say("%s: Please specify a window refnum or LAST", name);
	return NULL;
}

/* get_number: parses out an integer number and returns it */
static	int get_number(char *name, char **args, char *msg)
{
	char	*arg;

	if ((arg = next_arg(*args, args)) != NULL)
		return my_atol(arg);
	else if (!get_int_var(WINDOW_QUIET_VAR))
	{
		if (msg)
			say("%s: %s", name, msg);
		else
			say("%s: You must specify the number of lines", name);
	}
	return 0;
}

/*
 * get_boolean: parses either ON, OFF, or TOGGLE and sets the var
 * accordingly.  Returns 0 if all went well, -1 if a bogus or missing value
 * was specified 
 */
static	int get_boolean(char *name, char **args, int *var)
{
	char	*arg;

	if (((arg = next_arg(*args, args)) == NULL) || do_boolean(arg, var))
	{
		if (!get_int_var(WINDOW_QUIET_VAR))
			say("Value for %s must be ON, OFF, or TOGGLE", name);
		return (-1);
	}
	else
	{
		say("Window %s is %s", name, onoff[*var]);
		return (0);
	}
}






/*
 * /WINDOW ADD nick<,nick>
 * Adds a list of one or more nicknames to the current list of usupred
 * targets for the current window.  These are matched up with the nick
 * argument for message_from().
 */
static Window *window_add (Window *window, char **args, char *usage)
{
	char		*ptr;
	NickList 	*new;
	char 		*arg = next_arg(*args, args);

	if (!arg)
		say("ADD: Add nicknames to be redirected to this window");

	else while (arg)
	{
		if ((ptr = strchr(arg, ',')))
			*ptr++ = 0;
		if (!find_in_list((List **)&window->nicks, arg, !USE_WILDCARDS))
		{
			say("Added %s to window name list", arg);
			new = (NickList *)new_malloc(sizeof(NickList));
			new->nick = m_strdup(arg);
			add_to_list((List **)&(window->nicks), (List *)new);
		}
		else
			say("%s already on window name list", arg);

		arg = ptr;
	}

	return window;
}

/*
 * /WINDOW BACK
 * Changes the current window pointer to the window that was most previously
 * the current window.  If that window is now hidden, then it is swapped with
 * the current window.
 */
static Window *window_back (Window *window, char **args, char *usage)
{
	Window *tmp;

	tmp = get_window_by_refnum(last_input_screen->last_window_refnum);
	if (!tmp)
		tmp = last_input_screen->window_list;

	make_window_current(tmp);
	if (tmp->visible)
		set_screens_current_window(last_input_screen, tmp);
	else
	{
		swap_window(window, tmp);
		reset_display_target();
	}

	return window;
}

/*
 * /WINDOW BALANCE
 * Causes all of the windows on the current screen to be adjusted so that 
 * the largest window on the screen is no more than one line larger than
 * the smallest window on the screen.
 */
static Window *window_balance (Window *window, char **args, char *usage)
{
	if (window->screen)
		rebalance_windows(window->screen);
	else
		yell("cannot rebalance invisible windows!");
	return window;
}

/*
 * /WINDOW BEEP_ALWAYS ON|OFF
 * Indicates that when this window is HIDDEN (sorry, thats not what it seems
 * like it should do, but that is what it does), beeps to this window should
 * not be suppressed like they normally are for hidden windows.  The beep
 * occurs EVEN IF /set beep is OFF.
 */
static Window *window_beep_always (Window *window, char **args, char *usage)
{
	if (get_boolean("BEEP_ALWAYS", args, &window->beep_always))
		return NULL;
	return window;
}

/*
 * /WINDOW BIND <#channel>
 * Indicates that the window should be "bound" to the specified channel.
 * "binding" a channel to a window means that the channel will always 
 * belong to this window, no matter what.  For example, if a channel is
 * bound to a window, you can do a /join #channel in any window, and it 
 * will always "join" in this window.  This is especially useful when
 * you are disconnected from a server, because when you reconnect, the client
 * often loses track of which channel went to which window.  Binding your
 * channels gives the client a hint where channels belong.
 *
 * You can rebind a channel to a new window, even after it has already
 * been bound elsewhere.
 */
static Window *window_bind (Window *window, char **args, char *usage)
{
	char *arg;
	Window *w = NULL;
	
	if ((arg = next_arg(*args, args)))
	{
		char *channel;
		channel = make_channel(arg);
		if (!channel || !is_channel(channel))
		{
			say("BIND: %s is not a valid channel name", channel ? channel :"");
			return window;
		}
		/*
		 * If its already bound, no point in continuing.
		 */
		if (window->bind_channel && is_bound_to_window(window, channel))
		{
			say("Window is already bound to channel %s", channel);
			return window;
		}
                
#if 0
		/*
		 * You must either bind the current channel to a window, or
		 * you must be binding to a window without a current channel
		 */
		if (window->current_channel)
		{
			if (!my_stricmp(window->current_channel, channel))
				m_s3cat(&window->bind_channel, comma, channel);
			else
				say("You may only /WINDOW BIND the current channel for this window");
			return window;
		}
#endif

		/*
		 * So we know this window doesnt have a current channel.
		 * So we have to find the window where it IS the current
		 * channel (if it is at all)
		 */
		while (traverse_all_windows(&w))
		{
			/*
			 * If we have found a window where this channel
			 * is currently bound, then we unbind it from
			 * that oTher window and bind it here.
			 */
			if (w->bind_channel && w->server && is_bound_to_window(w, channel))
			{
				char *p, *q, *new_bind = NULL;
				m_s3cat(&window->bind_channel, comma, channel);
				q = p = LOCAL_COPY(w->bind_channel);
				while ((p = next_in_comma_list(q, &q)))
				{
					if (!p || !*p)
						break;
					if (!my_stricmp(p, channel))
						continue;
					m_s3cat(&new_bind, comma, p);
				}
				if (new_bind && *new_bind)
					malloc_strcpy(&w->bind_channel, new_bind);
				else
					new_free(&w->bind_channel);
				new_free(&new_bind);
			}

			/*
			 * If we have found a window where this channel
			 * is the current channel, then we make it so that
			 * it is the current channel here.
			 */
			if (w->current_channel && w->server == window->server)
			{
				if (is_bound_to_window(w, channel))
					unset_window_current_channel(w);
			}
		}

		/*
		 * Now we mark this channel as being bound here.
		 * and as being our current channel.
		 */
		m_s3cat(&window->bind_channel, comma, channel);
		say("Window is bound to channel %s", channel);

		if (im_on_channel(channel, window->server))
		{
			set_current_channel_by_refnum(window->refnum, channel);
			say("Current channel for window now %s", channel);
		}
	}

	else if ((arg = get_bound_channel(window)))
		say("Window is bound to channel %s", arg);
	else
		say("Window is not bound to any channel");

	return window;
}

/*
 * /WINDOW CHANNEL <#channel>
 * Directs the client to make a specified channel the current channel for
 * the window -- it will JOIN the channel if you are not already on it.
 * If the channel you wish to activate is bound to a different window, you
 * will be notified.  If you are already on the channel in another window,
 * then the channel's window will be switched.  If you do not specify a
 * channel, or if you specify the channel "0", then the window will drop its
 * connection to whatever channel it is in.
 */
Window *window_channel (Window *window, char **args, char *usage)
{
	char	*arg; 

	if ((arg = new_next_arg(*args, args)))
	{
		char *channel, *ch;
		while ((ch = next_in_comma_list(arg, &arg)))
		{
			if (!my_strnicmp(ch, "-i", 2))
			{
				if (invite_channel)
					ch = invite_channel;
				else
				{
					say("You have not been invited to a channel!");
					return window;
				}
			}
			if (!ch || !*ch)
				break;
			if (!(channel = make_channel(ch)))
				break;
			if (is_bound(channel, window->server))
				say("Channel %s is already bound elsewhere", channel);
			else if (is_current_channel(channel, window->server, 1) ||
				is_on_channel(channel, window->server, get_server_nickname(window->server)))
			{
				say("You are now talking to channel %s", channel);
				set_current_channel_by_refnum(window->refnum, channel);
			}
			else if (channel[1] == '0' && channel[2] == 0)
				set_current_channel_by_refnum(window->refnum, NULL);
			else
			{
				my_send_to_server(window->server, "JOIN %s", channel);
				malloc_strcpy(&window->waiting_channel, channel);
			}
		}
	}
	else
		set_current_channel_by_refnum(window->refnum, zero);

	return window;
}

/* For JOIN_NEW_WINDOW .... */
void win_create(int var, int test)
{
	if (get_int_var(var) && (test ||
		current_window->current_channel || current_window->query_nick))
	{
		char *args = NULL;
		switch (var)
		{
			case JOIN_NEW_WINDOW_VAR:
				args = LOCAL_COPY(SAFE(get_string_var(JOIN_NEW_WINDOW_TYPE_VAR)));
			break;
			case QUERY_NEW_WINDOW_VAR:
				args = LOCAL_COPY(SAFE(get_string_var(QUERY_NEW_WINDOW_TYPE_VAR)));
			break;
			default:
				return;
			break;
		}
		if (args && *args)
			windowcmd("WINDOW", args, empty_string, empty_string);
	}
}

/*
 * /WINDOW CREATE
 * This directs the client to open up a new physical screen and create a
 * new window in it.  This feature depends on the external "wserv" utility
 * and requires a multi-processing system, since it actually runs the new
 * screen in a seperate process.  Please note that the external screen is
 * not actually controlled by the client, but rather by "wserv" which acts
 * as a pass-through filter on behalf of the client.
 *
 * Since the external screen is outside the client's process, it is not really
 * possible for the client to know when the external screen is resized, or
 * what that new size would be.  For this reason, you should not resize any
 * screen when you have external screens open.  If you do, the client will
 * surely become confused and the output will probably be garbled.  You can
 * restore some sanity by making sure that ALL external screens have the same
 * geometry, and then redrawing each screen.
 */
static Window *window_create (Window *window, char **args, char *usage)
{
#ifdef WINDOW_CREATE
	Window *tmp;
	if ((tmp = (Window *)create_additional_screen()))
	{
		last_input_screen = tmp->screen;
		window = tmp;
	}
	else
#endif
		say("Cannot create new screen!");
	return window;
}

/*
 * /WINDOW DELETE
 * This directs the client to close the current external physical screen
 * and to re-parent any windows onto other screens.  You are not allowed
 * to delete the "main" window because that window belongs to the process
 * group of the client itself.
 */
static Window *window_delete (Window *window, char **args, char *usage)
{
#ifdef WINDOW_CREATE
	if(window->screen)
		kill_screen(window->screen);
#endif
	return current_window;
}

/*
 * /WINDOW DESCRIBE
 * Directs the client to tell you a bit about the current window.
 * This is the 'default' argument to the /window command.
 */
static Window *window_describe (Window *window, char **args, char *usage)
{
	if (window->name)
		say("Window %s (%u)", window->name, window->refnum);
	else
		say("Window %u", window->refnum);

	say("\tServer: [%d] %s",
				window->server, 
				window->server <= -1 ? 
				get_server_name(window->server) : "<None>");
	say("\tScreen: %p", window->screen);
	say("\tGeometry Info: [%d %d %d %d %d %d]", 
				window->top, window->bottom, 
				window->held_displayed, window->display_size,
				window->cursor, window->distance_from_display);

#ifndef GUI
	say("\tCO, LI are [%d %d]", current_term->TI_cols, current_term->TI_lines);
#else
	say("\tCO, LI are [%d %d]", output_screen->co, output_screen->li);
#endif
	say("\tCurrent channel: %s", 
				window->current_channel ? 
				window->current_channel : "<None>");

if (window->waiting_channel)
	say("\tWaiting channel: %s", 
				window->waiting_channel);

if (window->bind_channel)
	say("\tBound channel: %s", 
				window->bind_channel);
	say("\tQuery User: %s %s", 
				window->query_nick ? 
				window->query_nick : "<None>", 
				window->query_cmd ? 
				window->query_cmd : empty_string);
	say("\tPrompt: %s", 
				window->prompt ? 
				window->prompt : "<None>");
	say("\tSecond status line is %s", onoff[window->double_status]);
	say("\tSplit line is %s triple is %s", onoff[window->status_split], onoff[window->status_lines]);
	say("\tLogging is %s", 	 onoff[window->log]);

	if (window->logfile)
		say("\tLogfile is %s", window->logfile);
	else
		say("\tNo logfile given");

	say("\tNotification is %s", 
			      onoff[window->miscflags & WINDOW_NOTIFY]);
	say("\tHold mode is %s", 
				onoff[window->hold_mode]);
	say("\tWindow level is %s", 
				bits_to_lastlog_level(window->window_level));
	say("\tLastlog level is %s", 
				bits_to_lastlog_level(window->lastlog_level));
	say("\tNotify level is %s", 
				bits_to_lastlog_level(window->notify_level));

	if (window->nicks)
	{
		NickList *tmp;
		say("\tName list:");
		for (tmp = window->nicks; tmp; tmp = tmp->next)
			say("\t  %s", tmp->nick);
	}

	return window;
}

/*
 * /WINDOW DISCON
 * This disassociates a window with all servers.
 */
static Window *window_discon (Window *window, char **args, char *usage)
{
	if (window->server != -1)
		server_disconnect(window->server, NULL);
	window->server = -1;
	return window;
}

/*
 * /WINDOW DOUBLE ON|OFF
 * This directs the client to enable or disable the supplimentary status bar.
 * When the "double status bar" is enabled, the status formats are taken from
 * /set STATUS_FORMAT1 or STATUS_FORMAT2.  When it is disabled, the format is
 * taken from /set STATUS_FORMAT.
 */
static Window *window_double (Window *window, char **args, char *usage)
{
	int current = window->double_status;

	if (get_boolean("DOUBLE", args, &window->double_status))
		return NULL;

	window->display_size += current - window->double_status;
	recalculate_window_positions(window->screen);
	redraw_all_windows();
	build_status(window, NULL, 0);
	return window;
}

/*
 * alias wc {^window new double on split on hide_others}
 */
static Window *window_split (Window *window, char **args, char *usage)
{
int booya = window->status_lines;
Window *tmp;
	if (get_boolean("SPLIT", args, &booya))
		return NULL;
	for (tmp = screen_list->window_list; tmp; tmp = tmp->next)
	{
		if (tmp->status_lines && booya)
		{
			if (!get_int_var(WINDOW_QUIET_VAR))
				yell("Already a split window");
			return window;
		}
		else if (tmp->status_lines && !booya)
			window = tmp;
	}
	window->status_lines = booya;
	recalculate_window_positions(window->screen);
	recalculate_windows(window->screen);
	update_all_windows();
	build_status (window, NULL, 0);
	return window;
}

static Window *window_triple (Window *window, char **args, char *usage)
{
int booya = window->status_lines;
	if (get_boolean("TRIPLE", args, &booya))
		return NULL;
	if (!booya)
	{
		window->status_split = 1;
		window->status_lines = 0;
	}
	else
	{
		window->status_split = 0;
		window->status_lines = 1;
	}
	recalculate_windows(window->screen);
	update_all_windows();
	build_status (window, NULL, 0);
	return window;
}

/*
 * WINDOW ECHO <text>
 *
 * Text must either be surrounded with double-quotes (")'s or it is assumed
 * to terminate at the end of the argument list.  This sends the given text
 * to the current window.
 */
static	Window *window_echo (Window *window, char **args, char *usage)
{
	const char *to_echo;

	if (**args == '"')
		to_echo = new_next_arg(*args, args);
	else
		to_echo = *args, *args = NULL;

	add_to_window(window, (const unsigned char *)to_echo);
	return window;
}

/*
 * /WINDOW FIXED (ON|OFF)
 *
 * When this is ON, then this window will never be used as the implicit
 * partner for a window resize.  That is to say, if you /window grow another
 * window, this window will not be considered for the corresponding shrink.
 * You may /window grow a fixed window, but if you do not have other nonfixed
 * windows, the grow will fail.
 */
static	Window *window_fixed (Window *window, char **args, char *usage)
{
	if (get_boolean("FIXED", args, &window->absolute_size))
		return NULL;
	return window;
}

/*
 * /WINDOW GOTO refnum
 * This switches the current window selection to the window as specified
 * by the numbered refnum.
 */
static Window *window_goto (Window *window, char **args, char *usage)
{
	goto_window(window->screen, get_number("GOTO", args, NULL));
	from_server = get_window_server(0);
	return current_window;
}

/*
 * /WINDOW GROW lines
 * This directs the client to expand the specified window by the specified
 * number of lines.  The number of lines should be a positive integer, and
 * the window's growth must noT cause another window to be smaller than
 * the minimum of 3 lines.
 */
static Window *window_grow (Window *window, char **args, char *usage)
{
	resize_window(RESIZE_REL, window, get_number("GROW", args, NULL));
	return window;
}

/*
 * /WINDOW HIDE
 * This directs the client to remove the specified window from the current
 * (visible) screen and place the window on the client's invisible list.
 * A hidden window has no "screen", and so can not be seen, and does not
 * have a size.  It can be unhidden onto any screen.
 */
static Window *window_hide (Window *window, char **args, char *usage)
{
	hide_window(window);
	return current_window;
}

/*
 * /WINDOW HIDE_OTHERS
 * This directs the client to place *all* windows on the current screen,
 * except for the current window, onto the invisible list.
 */
static Window *window_hide_others (Window *window, char **args, char *usage)
{
	Window *tmp, *next;

	if (window->screen)
		tmp = window->screen->window_list;
	else
		tmp = invisible_list;
	while (tmp)
	{
		next = tmp->next;
		if (tmp != window)
			hide_window(tmp);
		tmp = next;
	}
	return window;
}

/*
 * /WINDOW HOLD_MODE
 * This arranges for the window to "hold" any output bound for it once
 * a full page of output has been completed.  Setting the global value of
 * HOLD_MODE is truly bogus and should be changed. XXXX
 */
static Window *window_hold_mode (Window *window, char **args, char *usage)
{
	if (get_boolean("HOLD_MODE", args, &window->hold_mode))
		return NULL;

	set_int_var(HOLD_MODE_VAR, window->hold_mode);
	return window;
}

/*
 * /WINDOW KILL
 * This arranges for the current window to be destroyed.  Once a window
 * is killed, it cannot be recovered.  Because every server must have at
 * least one window "connected" to it, if you kill the last window for a
 * server, the client will drop your connection to that server automatically.
 */
static Window *window_kill (Window *window, char **args, char *usage)
{
	delete_window(window);
	redraw_all_windows();
	return current_window;
}

/*
 * /WINDOW KILL_OTHERS
 * This arranges for all windows on the current screen, other than the 
 * current window to be destroyed.  Obviously, the current window will be
 * the only window left on the screen.  Connections to servers other than
 * the server for the current window will be implicitly closed.
 */
static Window *window_kill_others (Window *window, char **args, char *usage)
{
	Window *tmp, *next;

	if (window->screen)
		tmp = window->screen->window_list;
	else
		tmp = invisible_list;
	while (tmp)
	{
		next = tmp->next;
		if (tmp != window)
			delete_window(tmp);
		tmp = next;
	}
	return window;
}

/*
 * /WINDOW KILLSWAP
 * This arranges for the current window to be replaced by the last window
 * to be hidden, and also destroys the current window.
 */
static Window *window_killswap (Window *window, char **args, char *usage)
{
	if (invisible_list)
	{
		swap_window(window, invisible_list);
		delete_window(window);
	}
	else
		say("There are no hidden windows!");

	return current_window;
}

/*
 * /WINDOW LAST
 * This changes the current window focus to the window that was most recently
 * the current window *but only if that window is still visible*.  If the 
 * window is no longer visible (having been HIDDEN), then the next window
 * following the current window will be made the current window.
 */
static Window *window_last (Window *window, char **args, char *usage)
{
	set_screens_current_window(window->screen, NULL);
	return current_window;
}

/*
 * /WINDOW LASTLOG <size>
 * This changes the size of the window's lastlog buffer.  The default value
 * foR a window's lastlog is the value of /set LASTLOG, but each window may
 * be independantly tweaked with this command.
 */
static Window *window_lastlog (Window *window, char **args, char *usage)
{
	char *arg = next_arg(*args, args);

	if (arg)
	{
		int size = my_atol(arg);
		if (window->lastlog_size > size)
		{
			int i, diff;
			diff = window->lastlog_size - size;
			for (i = 0; i < diff; i++)
				remove_from_lastlog(window);
		}
		window->lastlog_max = size;
	}
	say("Lastlog size is %d", window->lastlog_max);
	return window;
}

/*
 * /WINDOW LASTLOG_LEVEL <level-description>
 * This changes the types of lines that will be placed into this window's
 * lastlog.  It is useful to note that the window's lastlog will contain
 * a subset (possibly a complete subset) of the lines that have appeared
 * in the window.  This setting allows you to control which lines are
 * "thrown away" by the window.
 */
static Window *window_lastlog_level (Window *window, char **args, char *usage)
{
	char *arg = next_arg(*args, args);;

	if (arg)
		window->lastlog_level = parse_lastlog_level(arg, 1);
	say("Lastlog level is %s", bits_to_lastlog_level(window->lastlog_level));
	return window;
}

/*
 * /WINDOW LEVEL <level-description>
 * This changes the types of output that will appear in the specified window.
 * Note that for the given set of windows connected to a server, each level
 * may only appear once in that set.  When you add a level to a given window,
 * then it will be removed from whatever window currently holds it.  The
 * exception to this is the "DCC" level, which may only be set to one window
 * for the entire client.
 */
static Window *window_level (Window *window, char **args, char *usage)
{
	char 	*arg;
	int	add = 0;
	int	newlevel = 0;

	if ((arg = next_arg(*args, args)))
	{
		if (*arg == '+')
			add = 1, arg++;
		else if (*arg == '-')
			add = -1, arg++;

		newlevel = parse_lastlog_level(arg, 1);
		if (add == 1)
			window->window_level |= newlevel;
		else if (add == 0)
			window->window_level = newlevel;
		else if (add == -1)
			window->window_level &= ~newlevel;

		revamp_window_levels(window);
	}
	say("Window level is %s", bits_to_lastlog_level(window->window_level));
	return window;
}

/*
 * /WINDOW LIST
 * This lists all of the windows known to the client, and a breif summary
 * of their current state.
 */
Window *window_list (Window *window, char **args, char *usage)
{
	Window	*tmp = NULL;
	int	len = 6;
	int	cnw = get_int_var(CHANNEL_NAME_WIDTH_VAR);

	while ((traverse_all_windows(&tmp)))
	{
		if (tmp->name && (strlen(tmp->name) > len))
			len = strlen(tmp->name);
	}

	say(WIN_FORM,      	"Ref",
		      12, 12,	"Nick",	
		      len, len, "Name",
		      cnw, cnw, "Channel",
				"Query",
				"Server",
				"Level",
				empty_string);

	tmp = NULL;
	while ((traverse_all_windows(&tmp)))
		list_a_window(tmp, len);

	return window;
}

/*
 * /WINDOW LOG ON|OFF
 * This sets the current state of the logfile for the given window.  When the
 * logfile is on, then any lines that appear on the window are written to the
 * logfile 'as-is'.  The name of the logfile can be controlled with
 * /WINDOW LOGFILE.  The default logfile name is <windowname>.<target|refnum>
 */
static Window *window_log (Window *window, char **args, char *usage)
{
	char *logfile;
	int add_ext = 1;
	char buffer[BIG_BUFFER_SIZE + 1];

	if (get_boolean("LOG", args, &window->log))
		return NULL;

	if ((logfile = window->logfile))
		add_ext = 0;
	else if (!(logfile = get_string_var(LOGFILE_VAR)))
		logfile = empty_string;

	strmcpy(buffer,  logfile, BIG_BUFFER_SIZE);

	if (add_ext)
	{
		char *title = empty_string;

		strmcat(buffer, ".", BIG_BUFFER_SIZE);
		if ((title = window->current_channel))
			strmcat(buffer, title, BIG_BUFFER_SIZE);
		else if ((title = window->query_nick))
			strmcat(buffer, title, BIG_BUFFER_SIZE);
		else
		{
			strmcat(buffer, "Window_", BIG_BUFFER_SIZE);
			strmcat(buffer, ltoa(window->refnum), BIG_BUFFER_SIZE);
		}
	}
	strip_chars(buffer, "|\\:", '-'); 
	do_log(window->log, buffer, &window->log_fp);
	if (!window->log_fp)
		window->log = 0;

	return window;
}

/*
 * /WINDOW LOGFILE <filename>
 * This sets the current value of the log filename for the given window.
 * When you activate the log (with /WINDOW LOG ON), then any output to the
 * window also be written to the filename specified.
 */
static Window *window_logfile (Window *window, char **args, char *usage)
{
	char *arg = next_arg(*args, args);

	if (arg)
	{
		malloc_strcpy(&window->logfile, arg);
		say("Window LOGFILE set to %s", arg);
	}
	else if (window->logfile)
		say("Window LOGFILE is %s", window->logfile);
	else
		say("Window LOGFILE is not set.");

	return window;
}

static Window *window_move (Window *window, char **args, char *usage)
{
	move_window(window, get_number("MOVE", args, NULL));
	return window;
}

static Window *window_name (Window *window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg(*args, args)))
	{
		if (is_window_name_unique(arg))
		{
			malloc_strcpy(&window->name, arg);
			window->update |= UPDATE_STATUS;
		}
		else
			say("%s is not unique!", arg);
	}
	else
		say("You must specify a name for the window!");

	return window;
}

static Window *window_new (Window *window, char **args, char *usage)
{
	Window *tmp;
	if ((tmp = new_window(window->screen)))
		window = tmp;

	return window;
}

static Window *window_new_hide (Window *window, char **args, char *usage)
{
	new_window(NULL);
	return window;
}

static Window *window_next (Window *window, char **args, char *usage)
{
	Window	*tmp;
	Window	*next = NULL;
	Window	*smallest = NULL;

	if (!invisible_list)
	{
		say("There are no hidden windows");
		return NULL;
	}

	smallest = window;
	for (tmp = invisible_list; tmp; tmp = tmp->next)
	{
		if (tmp->refnum < smallest->refnum)
			smallest = tmp;
		if ((tmp->refnum > window->refnum)
		    && (!next || (tmp->refnum < next->refnum)))
			next = tmp;
	}

	if (!next)
		next = smallest;

	swap_window(window, next);
	reset_display_target();
	return current_window;
}

static        Window *window_noserv (Window *window, char **args, char *usage)
{
	window->server = -1;
	return window;
}

static Window *window_notify (Window *window, char **args, char *usage)
{
	window->miscflags ^= WINDOW_NOTIFY;
	say("Notification when hidden set to %s",
		window->miscflags & WINDOW_NOTIFY ? on : off);
	return window;
}

static Window *window_notify_level (Window *window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg(*args, args)))
		window->notify_level = parse_lastlog_level(arg, 1);
	say("Window notify level is %s", bits_to_lastlog_level(window->notify_level));
	return window;
}

static Window *window_number (Window *window, char **args, char *usage)
{
	Window 	*tmp;
	char 	*arg;
	int 	i;

	if ((arg = next_arg(*args, args)))
	{
		if ((i = my_atol(arg)) > 0)
		{
			if ((tmp = get_window_by_refnum(i)))
				tmp->refnum = window->refnum;
			window->refnum = i;
		}
		else
			say("Window number must be greater than 0");
	}
	else
		say("Window number missing");

	return window;
}

/*
 * /WINDOW POP
 * This changes the current window focus to the most recently /WINDOW PUSHed
 * window that still exists.  If the window is hidden, then it will be made
 * visible.  Any windows that are found along the way that have been since
 * KILLed will be ignored.
 */
static Window *window_pop (Window *window, char **args, char *usage)
{
	int 		refnum;
	WindowStack 	*tmp;
	Window		*win = NULL;

	while (window->screen->window_stack)
	{
		refnum = window->screen->window_stack->refnum;
		tmp = window->screen->window_stack->next;
		new_free(&window->screen->window_stack);
		window->screen->window_stack = tmp;

		win = get_window_by_refnum(refnum);
		if (!win)
			continue;

		if (win->visible)
			set_screens_current_window(win->screen, win);
		else
			show_window(win);
	}

	if (!window->screen->window_stack && !win)
		say("The window stack is empty!");

	return win;
}

static Window *window_previous (Window *window, char **args, char *usage)
{
	Window	*tmp;
	Window	*previous = NULL, *largest;

	if (!invisible_list)
	{
		say("There are no hidden windows");
		return NULL;
	}

	largest = window;
	for (tmp = invisible_list; tmp; tmp = tmp->next)
	{
		if (tmp->refnum > largest->refnum)
			largest = tmp;
		if ((tmp->refnum < window->refnum)
		    && (!previous || tmp->refnum > previous->refnum))
			previous = tmp;
	}

	if (!previous)
		previous = largest;

	swap_window(window, previous);
	reset_display_target();
	return current_window;
}

static Window *window_prompt (Window *window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg(*args, args)))
	{
		malloc_strcpy(&window->prompt, arg);
		window->update |= UPDATE_STATUS;
	}
	else
		say("You must specify a prompt for the window!");

	return window;
}

static Window *window_push (Window *window, char **args, char *usage)
{
	WindowStack *new;

	new = (WindowStack *) new_malloc(sizeof(WindowStack));
	new->refnum = window->refnum;
	new->next = window->screen->window_stack;
	window->screen->window_stack = new;
	return window;
}

Window *window_query (Window *window, char **args, char *usage)
{
	NickList *tmp;
	char	  *nick = NULL;
	char	  *host = NULL;
	char	  *cmd = NULL;
	char	  *ptr;

	/*
	 * Nuke the old query list
	 */
	if (*args && !my_strnicmp(*args, "-cmd", 4))
	{
		cmd = next_arg(*args, args);
		cmd = next_arg(*args, args);
	}
	if ((nick = window->query_nick))
	{
		say("Ending conversation with %s", current_window->query_nick);
		window->update |= UPDATE_STATUS;
		while ((ptr = next_in_comma_list(nick, &nick)))
		{
			if (!ptr || !*ptr)
				break;

			if ((tmp = (NickList *)remove_from_list(
					(List **)&window->nicks, ptr)))
			{
				new_free(&tmp->nick);
				new_free(&tmp->host);
				new_free((char **)&tmp);
			}
		}
		new_free(&window->query_nick);
		new_free(&window->query_host);
		new_free(&window->query_cmd);
	}

	if (cmd)
		malloc_strcpy(&window->query_cmd, cmd);

	if ((nick = next_arg(*args, args)))
	{

		if (args && *args)
			host = *args;
		if (!strcmp(nick, "."))
		{
			if (!(nick = get_server_sent_nick(window->server)))
				say("You have not messaged anyone yet");
		}
		else if (!strcmp(nick, ","))
		{
			if (!(nick = get_server_recv_nick(window->server)))
				say("You have not recieved a message yet");
		}
		else if (!strcmp(nick, "*") && 
			!(nick = get_current_channel_by_refnum(0)))
		{
			say("You are not on a channel");
		}
		else if (*nick == '%')
		{
			if (!is_valid_process(nick))
				nick = NULL;
		}

		if (!nick)
			return window;



		/*
		 * Create the new query list
		 */
		say("Starting conversation with %s", nick);
		malloc_strcpy(&window->query_nick, nick);
		malloc_strcpy(&window->query_host, host);

		window->update |= UPDATE_STATUS;
		ptr = nick;
		while ((ptr = next_in_comma_list(nick, &nick)))
		{
			if (!ptr || !*ptr)
				break;
			tmp = (NickList *) new_malloc(sizeof(NickList));
			tmp->nick = m_strdup(ptr);
			if (host)
				tmp->host = m_strdup(host);
			add_to_list((List **)&window->nicks,(List *)tmp);
		}
	}
	update_input(UPDATE_ALL);
#ifdef GUI
	xterm_settitle();
#endif
	return window;
}

static Window *window_refresh (Window *window, char **args, char *usage)
{
	int oiwc = in_window_command;
	in_window_command = 0;
	update_all_windows();
	update_all_status(window, NULL, 0);
	in_window_command = oiwc;
	return window;
}


static Window *window_refnum (Window *window, char **args, char *usage)
{
	Window *tmp;
	if ((tmp = get_window("REFNUM", args)))
	{
		window = tmp;
		make_window_current(tmp);
		if (tmp->visible)
		{
			set_screens_current_window(tmp->screen, tmp);
			window = tmp;
		}
	}
	else
	{
		say("No such window!");
		window = NULL;
	}
	return window;
}

static Window *window_remove (Window *window, char **args, char *usage)
{
	char *arg;

	if ((arg = next_arg(*args, args)))
	{
		char	*ptr;
		NickList *new;

		while (arg)
		{
			if ((ptr = strchr(arg, ',')) != NULL)
				*ptr++ = 0;

			if ((new = (NickList *)remove_from_list((List **)&(window->nicks), arg)))
			{
				say("Removed %s from window name list", new->nick);
				new_free(&new->nick);
				new_free((char **)&new);
			}
			else
				say("%s is not on the list for this window!", arg);

			arg = ptr;
		}
	}
	else
		say("REMOVE: Do something!  Geez!");

	return window;
}

BUILT_IN_WINDOW(window_server)
{
	char *arg;
#ifdef HAVE_SSL
  	int withSSL = 0;
#endif

	if ((arg = next_arg(*args, args)))
	{
#ifdef HAVE_SSL
		if (!my_strnicmp(arg, "-SSL", strlen(arg)))
		{
			withSSL = 1;

			arg = next_arg(*args, args);
		}
		if(arg)
		{
#endif
			int i = find_server_refnum(arg, NULL);

#ifdef HAVE_SSL
			if(i != -1)
			{
				if(withSSL)
					set_server_ssl(i, 1);
				else
					set_server_ssl(i, 0);
			}
#endif
			close_unattached_servers();
			set_server_old_server(i, -2);
			set_server_try_once(i, 1);
			set_server_change_refnum(i, window->refnum);
			if (!connect_to_server_by_refnum(i, -1))
			{
#ifndef NON_BLOCKING_CONNECTS
				set_window_server(window->refnum, from_server, 0);
				set_level_by_refnum(window->refnum, new_server_lastlog_level);
				if (window->current_channel)
					new_free(&window->current_channel);
#endif
			}
			window_check_servers(from_server);
#ifdef HAVE_SSL
		}
#endif
	}
	else
		say("SERVER: You must specify a server");

	return window;
}

static Window *window_show (Window *window, char **args, char *usage)
{
	Window *tmp;

	if ((tmp = get_window("SHOW", args)))
	{
		show_window(tmp);
		window = window->screen->current_window;
		update_input(UPDATE_ALL);
#ifdef GUI
                gui_setfocus(tmp->screen);
#endif
	}
	return window;
}

static	Window *window_scratch (Window *window, char **args, char *usage)
{
	int scratch = 0;

	if (get_boolean("SCRATCH", args, &scratch))
		return NULL;
 
	if (scratch == 1)
		window->scratch_line = 0;
	else
	{
		window->scratch_line = -1;
 		window->top_of_display = window->display_ip;
		window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	}
	return window;
}

Window *window_scroll (Window *window, char **args, char *usage)
{
	int scroll = 0;

	if (get_boolean("SCROLL", args, &scroll))
		return NULL;

	if (scroll == 1 && window->scratch_line == -1)
		return window;
	if (scroll == 0 && window->scratch_line == 0)
		return window;

	if (scroll == 1)
	{
		window->scratch_line = -1;
		window->top_of_display = window->display_ip;
		window->update |= REDRAW_DISPLAY_FULL | REDRAW_STATUS;
	}
	else
	{
		window->scratch_line = 0;
		window->noscroll = 1;
	}
	return window;
}

static	Window *window_scrollback (Window *window, char **args, char *usage)
{
	int val = get_number("SCROLLBACK", args, NULL);

	if (!val)
		return NULL;
	if (val < window->display_size * 2)
		window->display_buffer_max = window->display_size * 2;
	else
		window->display_buffer_max = val;

	say("Window scrollback size set to %d", window->display_buffer_max);
	return window;
}

static Window *window_skip (Window *window, char **args, char *usage)
{
	if (get_boolean("SKIP", args, &window->skip))
		return NULL;

	return window;
}

static Window *window_show_all (Window *window, char **args, char *usage)
{
	while (invisible_list)
		show_window(invisible_list);
	return window;
}

static Window *window_shrink (Window *window, char **args, char *usage)
{
	resize_window(RESIZE_REL, window, -get_number("SHRINK", args, NULL));
	return window;
}

static Window *window_size (Window *window, char **args, char *usage)
{
	char *ptr = *args;
	int number;

	number = parse_number(args);
	if(ptr == *args)
		say("Window size is %d", window->display_size);
	else
		resize_window(RESIZE_ABS, window, number);

	return window;
}

static Window *window_stack (Window *window, char **args, char *usage)
{
	WindowStack 	*last, *tmp, *crap;
	Window 		*win = NULL;
	int		len = 4;

	while ((traverse_all_windows(&win)))
	{
		if (win->name && (strlen(win->name) > len))
			len = strlen(win->name);
	}

	say("Window stack:");
	last = NULL;
	tmp = window->screen->window_stack;
	while (tmp)
	{
		if ((win = get_window_by_refnum(tmp->refnum)) != NULL)
		{
			list_a_window(win, len);
			last = tmp;
			tmp = tmp->next;
		}
		else
		{
			crap = tmp->next;
			new_free((char **)&tmp);
			if (last)
				last->next = crap;
			else
				window->screen->window_stack = crap;

			tmp = crap;
		}
	}

	return window;
}

static Window *window_status_special (Window *window, char **args, char *usage)
{
	char *arg;

	arg = new_next_arg(*args, args);
	if (window->wset)
		malloc_strcpy(&window->wset->window_special_format, arg);
	window->update |= REDRAW_STATUS;

	return window;
}

static Window *window_swap (Window *window, char **args, char *usage)
{
	Window *tmp;

	if ((tmp = get_invisible_window("SWAP", args)))
		swap_window(window, tmp);

	return current_window;
}

static Window *window_unbind (Window *window, char **args, char *usage)
{
	char *arg, *channel;

	if ((arg = next_arg(*args, args)))
	{
		channel = make_channel(arg);
		if (channel && is_bound(channel, from_server))
		{
			say("Channel %s is no longer bound", channel);
			unbind_channel(channel, from_server);
		}
		else
			say("Channel %s is not bound", channel ? channel : empty_string);
	}
	else
	{
		say("Channel %s is no longer bound", window->bind_channel);
		new_free(&window->bind_channel);
	}
	return window;
}

static Window *window_set (Window *window, char **args, char *usage)
{
	wset_variable(NULL, *args, NULL, NULL);
	*args = NULL;
	return window;
}

static Window *window_update (Window *window, char **args, char *usage)
{
	update_window_status_all();
	return window;
}


/* 
 * Associates a Menu with a Window 
 */

static Window *window_menu(Window *window, char **args, char *usage)
{
#ifdef GUI
MenuStruct *menutoadd = NULL;
Screen *menuscreen;
char *addthismenu;

	if(window->screen != NULL)
		menuscreen=window->screen;
	else
		menuscreen=main_screen;

	addthismenu = m_strdup(next_arg(*args, args));

	if (my_stricmp(addthismenu, "-delete") && !(menutoadd = (MenuStruct *)findmenu(addthismenu)))
	{
		say("Menu not found!");
		return window;
	}

        gui_menu(menuscreen, addthismenu);
#endif /* GUI */
	return window;
}

#ifdef GUI
static Window *window_setpos(Window *window, char **args, char *usage)
{
int 	x = 0, 
	y = 0, 
	cx = 0, 
	cy = 0, 
	min = 0, 
	max = 0, 
	restore = 0, 
	activate = 0, 
	bottom = 0, 
	top = 0, 
	size = 0, 
	position = 0, 
	t = 0;
char *tmp;

	if (!(tmp = next_arg(*args, args)))
		return window;
	for (t = 0; t < strlen(tmp); t++)
	{
		switch (tmp[t])
		{
			case 's':
				size = 1;
				break;
			case 'p':
				position = 1;
				break;
			case 'm':
				min = 1;
				break;
			case 'M':
				max = 1;
				break;
			case 'r':
				restore = 1;
				break;
			case 'a':
				activate = 1;
				break;
			case 'T':
				top = 1;
				break;
			case 'B':
				bottom = 1;
				break;
		}
	}
	if (!(tmp = next_arg(*args, args)))
		return window;
	x = my_atol(tmp);

	if (!(tmp = next_arg(*args, args)))
		return window;
	y = my_atol(tmp);

	if (!(tmp = next_arg(*args, args)))
		return window;
	cx = my_atol(tmp);

	if (!(tmp = next_arg(*args, args)))
		return window;
	cy = my_atol(tmp);

	gui_setwindowpos(window->screen, x, y, cx, cy, top, bottom, min, max, restore, activate, size, position);
	return window;
}

static Window *window_font(Window *window, char **args, char *usage)
{
char *tmp;

	if ((tmp = next_arg(*args, args)))
		if (window && window->screen)
			gui_font_set(tmp, window->screen);
	return window;
}

static Window *window_nicklist(Window *window, char **args, char *usage)
{
char *tmp;

	if ((tmp = next_arg(*args, args)))
		if (window && window->screen)
			gui_nicklist_width(my_atol(tmp), window->screen);
	return window;
}
#endif

static Window *window_help (Window *, char **, char *);

static window_ops options [] = {
	{ "ADD",		window_add, 		"[nick|#channel|nick1,nick2,...]\n- Adds [nick|#channel|nick1,nick2,...] to the nicks to display in a window" },
	{ "BACK",		window_back, 		"\n- Moves you back one window" },
	{ "BALANCE",		window_balance, 	"\n- Balances the displayed windows" },
	{ "BEEP_ALWAYS",	window_beep_always, 	"[on|off|toggle]\n- Should this window beep always on/off/toggle" },
	{ "BIND",		window_bind, 		"[#channel]\n- Binds a channel to a window" },
	{ "CHANNEL",		window_channel, 	"[#channel|#chan1,#chan2,...]\n- Attempts to join a channel in current window" },
	{ "CREATE",		window_create, 		"\n- Create a new screen using wserv in X-windows" },
	{ "DELETE",		window_delete, 		"\n- Used to kill a screen (wserv) in X-windows" },
	{ "DESCRIBE",		window_describe,	"[text]\n- Displays useful information about the current window" },
	{ "DISCON",		window_discon,		"Disconnects window from a server" },
	{ "DOUBLE",		window_double, 		"[on|off|toggle]\n- Turns double status bar on/off/toggle" },
	{ "ECHO",		window_echo,		"[text]\n- echo [text] which is in quotes to a window" },
	{ "FIXED",		window_fixed,		"[on|off|toggle]\n- Turns fixed window sizing on/off/toggle" },
#ifdef GUI
        { "FONT",		window_font,		"[fontname]\n- Sets the font for the window" },
#endif
	{ "GOTO",		window_goto, 		"[refnum]\n- Go's to a N'th window" },
	{ "GROW",		window_grow, 		"[number]\n- Grows the current window by # of lines" },
	{ "HELP",		window_help,		"- All of this commands work on the current window" },
	{ "HIDE",		window_hide, 		"\n- Hides the current window" },
	{ "HIDE_OTHERS",	window_hide_others, 	"\n- Hides all windows except the current one" },
	{ "HOLD_MODE",		window_hold_mode, 	"[on|off|toggle]\n- Sets the current window's hold mode status" },
	{ "KILL",		window_kill, 		"\n- Kills the current window" },
	{ "KILL_OTHERS",	window_kill_others, 	"\n- Kills all visible windows except the current one" },
	{ "KILLSWAP",		window_killswap, 	"\n- Swaps the current window with the last one hidden and kills the swapped out window" },
	{ "LAST", 		window_last, 		"\n- Places you in the last window you were in" },
	{ "LASTLOG",		window_lastlog, 	"[number]\n- Sets this windows lastlog size to #" },
	{ "LASTLOG_LEVEL",	window_lastlog_level, 	"[level]\n- Sets the current windows lastlog level to [levels]" },
	{ "LEVEL",		window_level, 		"[level]\n- Sets the window's level to [levels]" },
	{ "LIST",		window_list, 		"\n- Displays a list of windows" },
	{ "LOG",		window_log, 		"[on|off|toggle]\n- Turns window logging on/off/toggle" },
	{ "LOGFILE",		window_logfile, 	"[filename]\n- Sets the filename for the current windows logfile" },
	{ "MENU",		window_menu,		"[name]\n- Associate a menu with a window"},
	{ "MOVE",		window_move, 		"[number]\n- Moves the current window on the display [number] of times" },
	{ "NAME",		window_name, 		"[text]\n- Sets the name for the current window" },
	{ "NEW",		window_new, 		"\n- Creates a new window" },
	{ "NEW_HIDE",		window_new_hide, 		"\n- Creates a new window" },
	{ "NEXT",		window_next, 		"\n- Sets the current window to the next numbered window" },
#ifdef GUI
	{ "NICKLIST",		window_nicklist,       	"\n- Sets the current window nicklist width" },
#endif
	{ "NOSERV",		window_noserv,		"\n- turns the server off for this window" },
	{ "NOTIFY",		window_notify, 		"[on|off|toggle]\n- Turns notification on/off/toggle for the current window." },
	{ "NOTIFY_LEVEL",	window_notify_level, 	"[level]\n- Set's current window notification level to [level]" },
	{ "NUMBER",		window_number, 		"[number]\n- Set's the current windows refnum to #" },
	{ "POP",		window_pop, 		"\n- Pops the top window off the window stack. if hidden it's made visible" },
	{ "PREVIOUS",		window_previous, 	"\n- Sets the current window to the previous numbered windows" },
	{ "PROMPT",		window_prompt, 		"[text]\n- Sets the current input prompt to [text]" },
	{ "PUSH",		window_push, 		"[refnum]\n- Places the current window or specified window on the window stack" },
	{ "QUERY",		window_query,		"Adds/remove query for current window" },
	{ "REFNUM",		window_refnum, 		"[refnum]\n- Changes the current window to [refnum]" },
	{ "REFRESH",		window_refresh,		"refreshes the current window" },
	{ "REMOVE",		window_remove, 		"[nick|#channel|nick1,nick2,...]\n- Removes nick|#channel from this window's display" },
	{ "SERVER",		window_server, 		"[servername[:[<port>][:[<password>][:[<nickname>]]]]]\n- Attempts to connect current window with a new server" },
	{ "SET",		window_set,		"[text]\n- Displays or sets window specific set's" },
#ifdef GUI
	{ "SETWINDOWPOS",	window_setpos,		"[options]\n- Set's windows size, position and state." },
#endif
	{ "SCRATCH",		window_scratch,		"\n- Create a scratch window. this window has no scrollback or scroll" },
	{ "SCROLL",		window_scroll,		"\n- toggle scratch window non-scroll" },
	{ "SCROLLBACK",		window_scrollback,	"[lines]\n- Sets the scrollback size for this window" },
	{ "SHOW",		window_show, 		"[refnum]\n- Makes the specified [refnum] window visible again" },
	{ "SHOW_ALL",		window_show_all,	"\n- Makes all hidden windows visibile again" },
	{ "SHRINK",		window_shrink, 		"[lines]\n- Shrinks the current window by # of lines" },
	{ "SIZE",		window_size, 		"[lines]\n- Set's the current window to # lines" },
	{ "SKIP",		window_skip,		"[on|off|toggle]\n- Set's the current windows skip status on/off/toggle" },
	{ "SPLIT",		window_split,		"[on|off|toggle]\n- Places a third status line at the top of the screen if possible on/off/toggle" },
	{ "STACK",		window_stack, 		"\n- Shows the current window stack which have been added using /window push" },
	{ "STATUS_SPECIAL",	window_status_special,	"[text]\n- A special window format"},
	{ "SWAP",		window_swap, 		"[refnum]\n- Swap the current window with the specified hidden [refnum] window" },
	{ "TRIPLE",		window_triple,		"[on|off|toggle]\n- Turns on/off/toggle a triple status bar" },
	{ "UNBIND",		window_unbind, 		"[#channel]\n- Removes a channel bound to a window. If not specified then the current channel is used" },
	{ "UPDATE",		window_update,		"\n- Updates the window status on all windows" },
	{ NULL,			NULL, 			NULL }
};


static Window *window_help (Window *window, char **args, char *usage)
{
int i, c = 0;
char buffer[BIG_BUFFER_SIZE+1];
char *arg = NULL;
int done = 0;
	*buffer = 0;
	if (!(arg = next_arg(*args, args)))
	{
		for (i = 0; options[i].command; i++)
		{
			strmcat(buffer, options[i].command, BIG_BUFFER_SIZE);
			strmcat(buffer, space, BIG_BUFFER_SIZE);
			if (++c == 5)
			{
				put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
				*buffer = 0;
				c = 0;
			}
		}
		if (c)
			put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
		userage("WINDOW help", "%R[%ncommand%R]%n to get help on specific commands");
	} 
	else
	{
		for (i = 0; options[i].command; i++)
		{
			if (!my_stricmp(options[i].command, arg))
			{
				sprintf(buffer, "WINDOW %s", arg);
				userage(buffer, options[i].usage?options[i].usage:" - No help available");
				done++;
			}
		}
		if (!done)
			put_it("%s", convert_output_format("$G WINDOW - No such command", NULL, NULL));
	}
	return window;
}

BUILT_IN_COMMAND(windowcmd)
{
	char *arg;
	int nargs = 0;
	int old_status_update = status_update_flag;
	Window *window;
	int oiwc = in_window_command;
#ifdef WANT_DLL
	WindowDll *dll = NULL;
#endif	
	in_window_command = 1;
	reset_display_target();
	
	window = current_window;

	while ((arg = next_arg(args, &args)))
	{
		int i;
		int len = strlen(arg);

		if (*arg == '-' || *arg == '/')
			arg++, len--;
#ifdef WANT_DLL
		if (dll_window && (dll = (WindowDll *)find_in_list((List **)&dll_window, arg, 0)))
		{
			window = dll->func(window, &args, dll->help);			
			nargs++;
			if (!window)
				args = NULL;
		}
		else
#endif
		{
			for (i = 0; options[i].func ; i++)
			{
				if (!my_strnicmp(arg, options[i].command, len))
				{
					window = options[i].func(window, &args, options[i].usage); 
					nargs++;
					if (!window)
						args = NULL;
					break;
				}
			}

			if (!options[i].func)
			{
				Window *s_window;
				if ((s_window = get_window_by_desc(arg)))
				{
					nargs++;
					window = s_window;
				}
				else
					yell("WINDOW: Invalid option: [%s]", arg);
			}
		}
	}

	if (!nargs)
		window_describe(current_window, NULL, NULL);
	in_window_command = oiwc;
	status_update_flag = old_status_update;

	update_all_windows();
	update_all_status(window, NULL, 0);
	update_input(UPDATE_ALL);
	cursor_to_input();
}



/* * * * * * * * * * * SCROLLBACK BUFFER * * * * * * * * * * * * * * */
/* 
 * XXXX Dont you DARE touch this XXXX 
 *
 * Most of the time, a delete_display_line() is followed somewhat
 * immediately by a new_display_line().  So most of the time we just
 * cache that one item and re-use it.  That saves us thousands of
 * malloc()s.  In the cases where its not, then we just do things the
 * normal way.  
 */
static Display *recycle = NULL;

void 	delete_display_line (Display *stuff)
{
	if (recycle == stuff)
		ircpanic("error in delete_display_line");
	if (recycle)
		new_free(&recycle);

	recycle = stuff;
	new_free(&recycle->line);
}

Display *new_display_line (Display *prev)
{
	Display *stuff;

	if (recycle)
	{
		stuff = recycle;
		recycle = NULL;
	}
	else
		stuff = (Display *)new_malloc(sizeof(Display));

	stuff->line = NULL;
	stuff->prev = prev;
	stuff->next = NULL;
	return stuff;
}

/* * * * * * * * * * * Scrollback functionality * * * * * * * * * * */
void 	BX_scrollback_backwards_lines (int lines)
{
	Window	*window = current_window;
	Display *new_top = window->top_of_display;
	int	new_lines;
#ifdef GUI
	int position;
	float bleah = get_int_var(SCROLLBACK_VAR);
#endif

	if (new_top == window->top_of_scrollback)
	{
#ifndef GUI
		term_beep();
#endif
		return;
	}

	if (!window->scrollback_point)
		window->scrollback_point = window->top_of_display;

	for (new_lines = 0; new_lines < lines; new_lines++)
	{
		if (new_top == window->top_of_scrollback)
			break;
		new_top = new_top->prev;
	}

	window->top_of_display = new_top;
	window->lines_scrolled_back += new_lines;

	recalculate_window_cursor(window);
	repaint_window(window, 0, -1);
	update_window_status(window, 0);
	cursor_not_in_display(window->screen);
	update_input(UPDATE_JUST_CURSOR);
#ifdef GUI
	position = (int)(bleah-((((float)(window->distance_from_display-window->display_size))/(float)(window->display_buffer_size-window->display_size))*bleah));
	gui_scrollerchanged(window->screen, position);
#endif
}

void 	BX_scrollback_forwards_lines (int lines)
{
	Window	*window = current_window;
	Display *new_top = window->top_of_display;
	int	new_lines = 0;
#ifdef GUI
	int position;
	float bleah = get_int_var(SCROLLBACK_VAR);
#endif

	if (new_top == window->display_ip || !window->scrollback_point)
	{
#ifndef GUI
		term_beep();
#endif	
		return;
	}

	for (new_lines = 0; new_lines < lines; new_lines++)
	{
		if (new_top == window->scrollback_point/*display_ip*/)
			break;
		new_top = new_top->next;
	}

	window->top_of_display = new_top;
	window->lines_scrolled_back -= new_lines;
	recalculate_window_cursor(window);
	repaint_window(window, 0, -1);
	update_window_status(window, 0);
	cursor_not_in_display(window->screen);
	update_input(UPDATE_JUST_CURSOR);
	if (window->lines_scrolled_back <= 0)
		scrollback_end (0, NULL);
#ifdef GUI
	position = (int)(bleah-((((float)(window->distance_from_display-window->display_size))/(float)(window->display_buffer_size-window->display_size))*bleah));
	gui_scrollerchanged(window->screen, position);
#endif
}

void 	BX_scrollback_forwards (char dumb, char *dumber)
{
	int 	ratio = get_int_var(SCROLLBACK_RATIO_VAR);
	int	lines;

	if (ratio < 10 ) 
		ratio = 10;
	if (ratio > 100) 
		ratio = 100;

	lines = current_window->display_size * ratio / 100;
	scrollback_forwards_lines(lines);
}

void 	BX_scrollback_backwards (char dumb, char *dumber)
{
	int 	ratio = get_int_var(SCROLLBACK_RATIO_VAR);
	int	lines;

	if (ratio < 10 ) 
		ratio = 10;
	if (ratio > 100) 
		ratio = 100;

	lines = current_window->display_size * ratio / 100;
	scrollback_backwards_lines(lines);
}


void 	BX_scrollback_end (char dumb, char *dumber)
{
	Window	*window = current_window;

	if (!window->scrollback_point)
	{
		term_beep();
		return;
	}

	/* Adjust the top of window only if we would move forward. */
	if (window->lines_scrolled_back > 0)
		window->top_of_display = window->scrollback_point;

	window->lines_scrolled_back = 0;
	window->scrollback_point = NULL;
	repaint_window(window, 0, -1);
	update_window_status(window, 0);
	cursor_not_in_display(window->screen);
	update_input(UPDATE_JUST_CURSOR);
#ifdef GUI
	gui_scrollerchanged(window->screen, get_int_var(SCROLLBACK_VAR));
#endif
}

void 	BX_scrollback_start (char dumb, char *dumber)
{
	Window	*window = current_window;

	if (window->display_buffer_size <= window->display_size)
	{
		term_beep();
		return;
	}

	if (!window->scrollback_point)
		window->scrollback_point = window->top_of_display;

	while (window->top_of_display != window->top_of_scrollback)
	{
		window->top_of_display = window->top_of_display->prev;
		window->lines_scrolled_back++;
	}

	repaint_window(window, 0, -1);
	update_window_status(window, 0);
	cursor_not_in_display(window->screen);
	update_input(UPDATE_JUST_CURSOR);
#ifdef GUI
	gui_scrollerchanged(window->screen, 0);
#endif
}


/* HOLD MODE STUFF */
/*
 * hold_mode: sets the "hold mode".  Really.  If the update flag is true,
 * this will also update the status line, if needed, to display the hold mode
 * state.  If update is false, only the internal flag is set.  
 */
void	BX_set_hold_mode (Window *window, int flag, int update)
{
	if (window == NULL)
		window = current_window;

	if (flag != ON && window->scrollback_point)
		return;

	if (flag == TOGGLE)
	{
		if (window->hold_mode == OFF)
			window->hold_mode = ON;
		else
			window->hold_mode = OFF;
	}
	else
		window->hold_mode = flag;


	if (update)
	{
		if (window->lines_held != window->last_lines_held)
		{
			window->last_lines_held = window->lines_held;
			update_window_status(window, 0);
			if (window->update | UPDATE_STATUS)
				window->update -= UPDATE_STATUS;
			cursor_in_display(window);
			update_input(NO_UPDATE);
		}
	}
	else
		window->last_lines_held = -1;
}

/*
 * This checks to see if any windows need to be unheld or not
 */
int	BX_unhold_windows(void)
{
	Window 	*w = NULL;
	int	retval = 0;

	while (traverse_all_windows(&w))
	{
		if (!w->hold_mode && w->lines_held)
		{
			if ((unhold_a_window(w)))
				retval++;
		}
	}

	return retval;
}

void 	BX_unstop_all_windows (char dumb, char *dumber)
{
	Window	*tmp = NULL;

	while (traverse_all_windows(&tmp))
		set_hold_mode(tmp, OFF, 1);
}

/*
 * reset_line_cnt: called by /SET HOLD_MODE to reset the line counter so we
 * always get a held screen after the proper number of lines 
 */
void 	BX_reset_line_cnt (Window *win, char *unused, int value)
{
	win->hold_mode = value;
	win->held_displayed = 0;
	if (!win->hold_mode && win->in_more)
		reset_hold_mode(win);
}

/* toggle_stop_screen: the BIND function TOGGLE_STOP_SCREEN */
void 	BX_toggle_stop_screen (char unused, char *not_used)
{
	set_hold_mode(NULL, TOGGLE, 1);
	update_all_windows();
}

/* 
 * If "scrollback_point" is set, then anything below the bottom of the screen
 * at that point gets nuked.
 * If "scrollback_point" is not set, anything below the current position of
 * the screen gets nuked.
 */
void 	BX_flush_everything_being_held (Window *window)
{
	Display *ptr, *save;
	int count;

	if (!window)
		window = current_window;

	count = window->display_size;

	if (window->scrollback_point)
		ptr = window->scrollback_point;
	else
		ptr = window->top_of_display;

	while (--count > 0)
	{
		ptr = ptr->next;
		if (ptr == window->display_ip)
			return;		/* Nothing to flush */
	}
	save = ptr->next;
	ptr->next = window->display_ip;
	window->display_ip->prev = ptr;
	ptr = save;
	
	while (ptr != window->display_ip)
	{
		Display *next = ptr->next;
		delete_display_line(ptr);
		window->lines_held--;
		ptr = next;
	}

	if (window->lines_held != 0)
		ircpanic("erf. fix this.");

	window->holding_something = 0;
	set_hold_mode(window, OFF, 1);
}


/*
 * This adjusts the viewport up one full screen.  This calls rite() 
 * indirectly, because repaint_window() uses rite() to do the work.
 * This belongs somewhere else.
 */
int	BX_unhold_a_window (Window *window)
{
	int amount = window->display_size;

	if (window->holding_something &&
		(window->distance_from_display < window->display_size))
	{
		window->holding_something = 0;
		window->lines_held = 0;
	}

	if (!window->lines_held || window->scrollback_point)
		return 0;		/* Right. */

	if (window->lines_held < amount)
		amount = window->lines_held;

	window->lines_held -= amount;
	window->held_displayed = 0;

	if (!window->lines_held)
		window->holding_something = 0;

	if (!amount)
		return 0;		/* Whatever */

	while (amount--)
		window->top_of_display = window->top_of_display->next;

	repaint_window(window, 0, -1);
	update_window_status(window, 0);
	return 1;
}

void BX_recalculate_window_cursor (Window *window)
{
	Display *tmp;

	window->cursor = window->distance_from_display = 0;
	for (tmp = window->top_of_display; tmp != window->display_ip;
				tmp = tmp->next)
		window->cursor++, window->distance_from_display++;

	if (window->cursor > window->display_size)
 		window->cursor = window->display_size;
}

void BX_set_screens_current_window (Screen *screen, Window *window)
{
	if (!window)
	{
		window = get_window_by_refnum(screen->last_window_refnum);
		if (window && window->screen != screen)
			window = NULL;
	}
	if (!window)
		window = screen->window_list;
	if (window->deceased)
		ircpanic("This window is dead");
	if (window->screen != screen || !screen)
		ircpanic("The window is not on that screen");

	if (screen->current_window != window)
	{
		if (screen->current_window)
		{
			screen->current_window->update |= UPDATE_STATUS;
			screen->last_window_refnum = screen->current_window->refnum;
		}
		screen->current_window = window;
		screen->current_window->update |= UPDATE_STATUS;
	}
	if (current_window != window)
		make_window_current(window);
	update_all_windows();
	xterm_settitle();
}

void BX_make_window_current (Window *window)
{
	Window *old_current_window = current_window;
	int	old_screen, old_win;
	int	new_screen, new_win; 

	if (!window)
		current_window = last_input_screen->current_window;
	else if (current_window != window)
 		current_window = window;
	if (current_window->deceased)
		ircpanic("This window is dead. Cannot continue.");

	if (current_window == old_current_window)
		return;

	if (!old_current_window)
		old_screen = old_win = -1;
	else if (!old_current_window->screen)
		old_screen = -1, old_win = old_current_window->refnum;
	else
		old_screen = old_current_window->screen->screennum,
		old_win = old_current_window->refnum;

	new_win = current_window->refnum;
	if (!current_window->screen)
		new_screen = -1;
	else
		new_screen = current_window->screen->screennum;

        do_hook(WINDOW_FOCUS_LIST, "%d %d %d %d", 
        		old_screen, old_win, 
			new_screen, new_win);
}

void BX_clear_scrollback(Window *window)
{
	while (window->display_buffer_size > 0)
	{
		Display *next;
		if (window->top_of_scrollback)
		{
			next = window->top_of_scrollback->next;
			new_free(&window->top_of_scrollback->line);
			new_free(&window->top_of_scrollback);
			window->top_of_scrollback = next;
		}
		window->display_buffer_size--;
	}
	window->cursor = window->distance_from_display = 0;
	resize_window_display(window);
}

void	make_to_window_by_desc (char *desc)
{
	Window	*new_win = get_window_by_desc(desc);

	if (new_win)
		target_window = new_win;
	else
		say("Window [%s] doesn't exist any more.  Punting.", desc);
}

int	get_current_winref (void)
{
	return current_window->refnum;
}

int	get_winref_by_desc (const char *desc)
{
	Window *w;

	if ((w = get_window_by_desc(desc)))
		return w->refnum;

	return -1;
}

void	make_window_current_by_desc (char *desc)
{
	Window	*new_win = get_window_by_desc(desc);

	if (new_win)
		make_window_current(new_win);
	else
		say("Window [%s] doesn't exist any more.  Punting.", desc);
}

void	make_window_current_by_winref (int refnum)
{
	Window	*new_win;

	if (refnum == -1)
		return;

	if ((new_win = get_window_by_refnum(refnum)))
		make_window_current(new_win);
	else
		say("Window [%d] doesn't exist any more.  Punting.", refnum);
}


