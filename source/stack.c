/*
 * stack.c - does the handling of stack functions
 *
 * written by matthew green
 * finished by Jeremy Nelson (ESL)
 * modified Colten Edwards 1996 for BitchX
 * copyright (C) 1993.
 */

#include "irc.h"
static char cvsrevision[] = "$Id: stack.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(stack_c)
#include "struct.h"

#include "vars.h"
#include "stack.h"
#include "window.h"
#include "hook.h"
#include "ircaux.h"
#include "output.h"
#include "list.h"
#include "misc.h"
#define MAIN_SOURCE
#include "modval.h"

	AliasStack1	*set_stack = NULL;
extern void do_stack_set (int, char *);

#ifdef OLD_HOOK
static	OnStack		*on_stack = NULL;

extern void do_stack_on (int, char *);

extern int Add_Remove_Check (Hook *, char *);

static	void	do_stack_on (int type, char *args)
{
	int	len, cnt, i, which = 0;
	Hook	*list = NULL;
	NumberMsg *nhook = NULL;
	char foo[10];

	if (!on_stack && (type == STACK_POP || type == STACK_LIST))
	{
		say("ON stack is empty!");
		return;
	}
	if (!args || !*args)
	{
		say("Missing event type for STACK ON");
		return;
	}
	len = strlen(args);
	for (cnt = 0, i = 0; i < NUMBER_OF_LISTS; i++)
	{
		if (!my_strnicmp(args, hook_functions[i].name, len))
		{
			if (strlen(hook_functions[i].name) == len)
			{
				cnt = 1;
				which = i;
				break;
			}
			else
			{
				cnt++;
				which = i;
			}
		}
		else if (cnt)
			break;
	}
	if (!cnt)
	{
		if (is_number(args))
		{
			which = my_atol(args);
			if (which < 1 || which > 999)
			{
				say("Numerics must be between 001 and 999");
				return;
			}
			which = -which;
		}
		else
		{
			say("No such ON function: %s", args);
			return;
		}
	}
	if (which < 0)
	{
		sprintf(foo, "%3.3u", -which);
		if ((nhook = find_name_in_hooklist(foo, numeric_list, 0, HOOKTABLE_SIZE)))
			list = nhook->list;
	}
	else
		list = hook_functions[which].list;

	if (type == STACK_PUSH)
	{
		OnStack	*new;

		new = (OnStack *) new_malloc(sizeof(OnStack));
		new->next = on_stack;
		on_stack = new;
		new->which = which;
		new->list = list;
		if (which < 0)
		{
			
			if (!nhook)
				return;
			nhook = find_name_in_hooklist(foo, numeric_list, 1, HOOKTABLE_SIZE);
			new_free(&nhook->name);
			new_free(&nhook);
		}
		else
			hook_functions[which].list = NULL;
		return;
	}
	else if (type == STACK_POP)
	{
		OnStack	*p, *tmp = NULL;

		for (p = on_stack; p; tmp = p, p = p->next)
		{
			if (p->which == which)
			{
				if (p == on_stack)
					on_stack = p->next;
				else
					tmp->next = p->next;
				break;
			}
		}
		if (!p)
		{
			say("No %s on the stack", args);
			return;
		}
		if (which >= 0 && hook_functions[which].list)
			remove_hook(which, NULL, 0, 0, 1);	/* free hooks */
		else if (which < 0 && nhook)
		{
			Hook *tmp, *next;
			if ((nhook = find_name_in_hooklist(foo, numeric_list, 1, HOOKTABLE_SIZE)))
			{
				new_free(&nhook->name);
				for (tmp = nhook->list; tmp; tmp = next)
				{
					next = tmp->next;
					tmp->not = 1;
					new_free(&(tmp->nick));
					new_free(&(tmp->stuff));
					new_free((char **)&tmp);
				}
				new_free(&nhook);
			}
		}
		if (which < 0)
		{
			/* look -- do we have any hooks already for this numeric? */
			if (p->list)
			{
				sprintf(foo, "%3.3u", -which);
				nhook = add_name_to_hooklist(foo, NULL, numeric_list, HOOKTABLE_SIZE);
				add_to_list_ext((List **)&nhook->list, (List *)p->list, (int (*)(List *, List *))Add_Remove_Check);
			}
		}
		else
			hook_functions[which].list = p->list;
		new_free((char **)&p);
		return;
	}
	else if (type == STACK_LIST)
	{
		int	slevel = 0;
		OnStack	*osptr;

		for (osptr = on_stack; osptr; osptr = osptr->next)
		{
			if (osptr->which == which)
			{
				Hook	*hptr;

				slevel++;
				say("Level %d stack", slevel);
				for (hptr = osptr->list; hptr; hptr = hptr->next)
					show_hook(hptr, args);
			}
		}
		
		if (!slevel)
			say("The STACK ON %s list is empty", args);
		return;
	}
	say("Unknown STACK ON type ??");
}
#endif

BUILT_IN_COMMAND(stackcmd)
{
	char	*arg;
	int	len, type;

	if ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		if (!my_strnicmp(arg, "PUSH", len))
			type = STACK_PUSH;
		else if (!my_strnicmp(arg, "POP", len))
			type = STACK_POP;
		else if (!my_strnicmp(arg, "LIST", len))
			type = STACK_LIST;
		else
		{
			say("%s is unknown stack verb", arg);
			return;
		}
	}
	else
		return;
	if ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		if (!my_strnicmp(arg, "ON", len))
			do_stack_on(type, args);
		else if (!my_strnicmp(arg, "ALIAS", len))
			do_stack_alias(type, args, STACK_DO_ALIAS);
		else if (!my_strnicmp(arg, "ASSIGN", len))
			do_stack_alias(type, args, STACK_DO_ASSIGN);
		else if (!my_strnicmp(arg, "SET", len))
			do_stack_set(type, args);
		else
		{
			say("%s is not a valid STACK type", arg);
			return;
		}
	}
}
