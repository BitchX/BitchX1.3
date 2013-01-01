/*
 * history.c: stuff to handle command line history 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: history.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(history_c)
#include "struct.h"

#include "ircaux.h"
#include "vars.h"
#include "history.h"
#include "output.h"
#include "input.h"
#define MAIN_SOURCE
#include "modval.h"

static	char	*history_match (char *);
static	void	add_to_history_list (int, char *);
static	char	*get_from_history_buffer (int);

typedef struct	HistoryStru
{
	int	number;
	char	*stuff;
	struct	HistoryStru *next;
	struct	HistoryStru *prev;
}	History;

/* command_history: pointer to head of command_history list */
static	History *command_history_head = NULL;
static	History *command_history_tail = NULL;
static	History *command_history_pos = NULL;

/* hist_size: the current size of the command_history array */
static	int	hist_size = 0;

/* hist_count: the absolute counter for the history list */
static	int	hist_count = 0;

/*
 * last_dir: the direction (next or previous) of the last get_from_history()
 * call.... reset by add to history 
 */
static	int	last_dir = -1;

/*
 * history pointer
 */
static	History	*tmp = NULL;

/*
 * history_match: using wild_match(), this finds the latest match in the
 * history file and returns it as the function result.  Returns null if there
 * is no match.  Note that this sticks a '*' at the end if one is not already
 * there. 
 */
static	char *history_match (char *match)
{
	char	*ptr;
	char	*match_str = NULL;

	if (*(match + strlen(match) - 1) == '*')
		match_str = LOCAL_COPY(match);
	else
	{
		match_str = alloca(strlen(match) + 3);
		strcpy(match_str, match);
		strcat(match_str, "*");
	}
	if (get_int_var(HISTORY_VAR))
	{
		if ((last_dir == -1) || (tmp == NULL))
			tmp = command_history_head;
		else
			tmp = tmp->next;
		if (tmp)
		{		
			for (; tmp; tmp = tmp->next)
			{
				ptr = tmp->stuff;
				while (ptr && strchr(get_string_var(CMDCHARS_VAR), *ptr))
					ptr++;

				if (wild_match(match_str, ptr))
				{
					last_dir = PREV;
					return (tmp->stuff);
				}
			}
		}
	}
	last_dir = -1;
	return NULL;
}

/* shove_to_history: a key binding that saves the current line into
 * the history and then deletes the whole line.  Useful for when you
 * are in the middle of a big line and need to "get out" to do something
 * else quick for just a second, and you dont want to have to retype
 * everything all over again
 */
extern void	shove_to_history (char unused, char *not_used)
{
	add_to_history(get_input());
	input_clear_line(unused, not_used);
}

static	void add_to_history_list(int cnt, char *stuff)
{
	History *new;

	if (get_int_var(HISTORY_VAR) == 0)
		return;
	if ((hist_size == get_int_var(HISTORY_VAR)) && command_history_tail)
	{
		if (hist_size == 1)
		{
			malloc_strcpy(&command_history_tail->stuff, stuff);
			return;
		}
		new = command_history_tail;
		command_history_tail = command_history_tail->prev;
		command_history_tail->next = NULL;
		new_free(&new->stuff);
		new_free((char **)&new);
		if (command_history_tail == NULL)
			command_history_head = NULL;
	}
	else
		hist_size++;
	new = (History *) new_malloc(sizeof(History));
	new->stuff = NULL;
	new->number = cnt;
	new->next = command_history_head;
	new->prev = NULL;
	malloc_strcpy(&(new->stuff), stuff);
	if (command_history_head)
		command_history_head->prev = new;
	command_history_head = new;
	if (command_history_tail == NULL)
		command_history_tail = new;
	command_history_pos = NULL;
}

/*
 * set_history_size: adjusts the size of the command_history to be size. If
 * the array is not yet allocated, it is set to size and all the entries
 * nulled.  If it exists, it is resized to the new size with a realloc.  Any
 * new entries are nulled. 
 */
void set_history_size(Window *win, char *unused, int size)
{
	int	i,
		cnt;
	History *ptr;

	if (size < hist_size)
	{
		cnt = hist_size - size;
		for (i = 0; i < cnt; i++)
		{
			ptr = command_history_tail;
			command_history_tail = ptr->prev;
			new_free(&(ptr->stuff));
			new_free((char **)&ptr);
		}
		if (command_history_tail == NULL)
			command_history_head = NULL;
		else
			command_history_tail->next = NULL;
		hist_size = size;
	}
}

/*
 * add_to_history: adds the given line to the history array.  The history
 * array is a circular buffer, and add_to_history handles all that stuff. It
 * automagically allocted and deallocated memory as needed 
 */
void add_to_history(char *line)
{
	char	*ptr;

	if (line && *line)
	{
		while (line && *line)
		{
			if ((ptr = sindex(line, "\n\r")) != NULL)
				*(ptr++) = '\0';
			add_to_history_list(hist_count, line);
			last_dir = PREV;
			hist_count++;
			line = ptr;
		}
	}
}

static	char	*get_from_history_buffer(int which)
{
	if ((get_int_var(HISTORY_VAR) == 0) || (hist_size == 0))
		return NULL;
	/*
	 * if (last_dir != which) { last_dir = which; get_from_history(which); }
	 */
	if (which == NEXT)
	{
		if (command_history_pos)
		{
			if (command_history_pos->prev)
				command_history_pos = command_history_pos->prev;
			else
				command_history_pos = command_history_tail;
		}
		else
		{
			add_to_history(get_input());
			command_history_pos = command_history_tail;
		}
		return (command_history_pos->stuff);
	}
	else
	{
		if (command_history_pos)
		{
			if (command_history_pos->next)
				command_history_pos = command_history_pos->next;
			else
				command_history_pos = command_history_head;
		}
		else
		{
			add_to_history(get_input());
			command_history_pos = command_history_head;
		}
		return (command_history_pos->stuff);
	}
}

/*
 * get_history: gets the next history entry, either the PREV entry or the
 * NEXT entry, and sets it to the current input string 
 */
extern void	get_history (int which)
{
	char	*ptr;

	if ((ptr = get_from_history(which)) != NULL)
	{
		set_input(ptr);
		update_input(UPDATE_ALL);
	}

}

char	*get_from_history(int which)
{
	return(get_from_history_buffer(which));
}

/* history: the /HISTORY command, shows the command history buffer. */
BUILT_IN_COMMAND(history)
{
	int	cnt,
		max = 0;
	char	*value;
	char	*match = NULL;
	
	if (get_int_var(HISTORY_VAR))
	{
		say("Command History:");
		if ((value = next_arg(args, &args)) != NULL)
		{
			if (my_strnicmp(value, "-CLEAR", 3))
			{
				for (tmp = command_history_head; command_history_head; tmp = command_history_head)
				{
					new_free(&tmp->stuff);
					command_history_head = tmp->next;
					new_free(&tmp);
				}
				hist_size = hist_count = 0;
				command_history_pos = NULL;
				command_history_tail = NULL;
				command_history_head = NULL;
				return;
			}
			if (isdigit((unsigned char)*value))
			{
				max = my_atol(value);
				if (max > get_int_var(HISTORY_VAR))
					max = get_int_var(HISTORY_VAR);
			} 
			else
				match = value;
		}
		else
			max = get_int_var(HISTORY_VAR);
		for (tmp = command_history_tail, cnt = 0; tmp && (match || (cnt < max));
				tmp = tmp->prev, cnt++)
		{
			if (!match || (match && wild_match(match, tmp->stuff)))
				put_it("%d: %s", tmp->number, tmp->stuff);
		}
	}
}

/*
 * do_history: This finds the given history entry in either the history file,
 * or the in memory history buffer (if no history file is given). It then
 * returns the found entry as its function value, or null if the entry is not
 * found for some reason.  Note that this routine mallocs the string returned  
 */
char	*do_history (char *com, char *rest)
{
	int	hist_num;
	char	*ptr, *ret = NULL;
	static	char	*last_com = NULL;

	if (!com || !*com)
	{
		if (last_com)
			com = last_com;
		else
			com = empty_string;
	}
	else
		malloc_strcpy(&last_com, com);

	if (!is_number(com))
	{
		if ((ptr = history_match(com)))
		{
			ret = m_strdup(ptr);
			m_s3cat_s(&ret, space, rest);
			return ret;
		}
		say("No Match");
	}
	else 
	{
		hist_num = my_atol(com);
		if (hist_num > 0)
		{
			for (tmp = command_history_head; tmp; tmp = tmp->next)
			{
				if (tmp->number == hist_num)
				{
					ret = m_strdup(tmp->stuff);
					m_s3cat_s(&ret, space, rest);
					return ret;
				}
			}
		}
		else
		{
			hist_num++;
			for (tmp = command_history_head; tmp && hist_num < 0; )
				tmp = tmp->next, hist_num++;
			if (tmp)
			{
				ret = m_strdup(tmp->stuff);
				m_s3cat_s(&ret, space, rest);
				return (ret);
			}
		}
		say("No such history entry: %d", hist_num);
	}
	return NULL;
}
