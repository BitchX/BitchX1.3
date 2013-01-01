/*
 * if.c: handles the IF command for IRCII 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990, 1991 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: if.c 32 2008-05-07 08:38:12Z keaston $";
CVS_REVISION(if_c)
#include "struct.h"

#include "alias.h"
#include "ircaux.h"
#include "window.h"
#include "vars.h"
#include "output.h"
#include "if.h"
#include "commands.h"
#include "misc.h"
#define MAIN_SOURCE
#include "modval.h"

/*
 * next_expr finds the next expression delimited by brackets. The type
 * of bracket expected is passed as a parameter. Returns NULL on error.
 */
char	*my_next_expr(char **args, char type, int whine)
{
	char	*ptr,
		*ptr2,
		*ptr3;

	if (!*args)
		return NULL;
	ptr2 = *args;
	if (!*ptr2)
		return 0;
	if (*ptr2 != type)
	{
		if (whine)
			say("Expression syntax");
		return 0;
	}							/* { */
	ptr = MatchingBracket(ptr2 + 1, type, (type == '(') ? ')' : '}');
	if (!ptr)
	{
		say("Unmatched '%c'", type);
		return 0;
	}
	*ptr = '\0';

	do
		ptr2++;
	while (my_isspace(*ptr2));

	ptr3 = ptr+1;
	while (my_isspace(*ptr3))
		ptr3++;
	*args = ptr3;
	if (*ptr2)
	{
		ptr--;
		while (my_isspace(*ptr))
			*ptr-- = '\0';
	}
	return ptr2;
}

extern char *next_expr_failok (char **args, char type)
{
	return my_next_expr (args, type, 0);
}

extern char *next_expr (char **args, char type)
{
	return my_next_expr (args, type, 1);
}

/*
 * All new /if command -- (jfn, 1997)
 *
 * Here's the plan:
 *
 *		if (expr) ......
 *		if (expr) {......}
 *		if (expr) {......} {......}
 *		if (expr) {......} else {......}
 *		if (expr) {......} elsif (expr2) {......}
 * etc.
 */

BUILT_IN_COMMAND(ifcmd)
{
	int unless_cmd;
	char *current_expr;
	char *current_expr_val;
	int result;
	char *current_line = NULL;
	int flag = 0;

	unless_cmd = (*command == 'U');
	if (!subargs)
		subargs = empty_string;

	while (args && *args)
	{
		while (my_isspace(*args))
			args++;

		current_expr = next_expr(&args, '(');
		if (!current_expr)
		{
			error("IF: Missing expression");
			return;
		}
		current_expr_val = parse_inline(current_expr, subargs, &flag);
		if (internal_debug & DEBUG_EXPANSIONS && !in_debug_yell)
			debugyell("%s expression expands to: (%s)", command, current_expr_val);
		result = check_val(current_expr_val);
		new_free(&current_expr_val);

		if (*args == '{')
		{
			current_line = next_expr(&args, '{');
		}
		else
			current_line = args, args = NULL;

		/* If the expression was FALSE for IF, and TRUE for UNLESS */
		if (unless_cmd == result)
		{
			if (args)
			{
				if (!my_strnicmp(args, "elsif ", 6))
				{
					args += 6;
					continue;
				}
				else if (!my_strnicmp(args, "else ", 5))
					args += 5;

				while (my_isspace(*args))
					args++;

				if (*args == '{')
					current_line = next_expr(&args, '{');
				else
					current_line = args, args = NULL;

			}
			else
				current_line = NULL;
		}

		if (current_line)
			parse_line(NULL, current_line, subargs, 0, 0, 1);

		break;
	}
}

BUILT_IN_COMMAND(docmd)
{
	char *body, *expr, *cmd, *ptr;
	char *newexp = NULL;
	int args_used = 0;
	int result;

	if (*args == '{')
	{
		if (!(body = next_expr(&args, '{')))
		{
			error("DO: unbalanced {");
			return;
		}	
		if (args && *args && (cmd = next_arg(args, &args)) && 
		     !my_stricmp (cmd, "while"))
		{
			if (!(expr = next_expr(&args, '(')))
			{
				error("DO: unbalanced (");
				return;
			}
			will_catch_break_exceptions++;
			will_catch_return_exceptions++;

			while (1)
			{
				parse_line (NULL, body, subargs ? subargs : empty_string, 0, 0, 1);
				if (break_exception)
				{
					break_exception = 0;
					break;
				}
				if (continue_exception)
				{
					continue_exception = 0;
					continue;
				}

				if (return_exception)
					break;
				malloc_strcpy(&newexp, expr);
				ptr = parse_inline(newexp, subargs ? subargs : empty_string,
					&args_used);
				result = check_val(ptr);
				new_free(&ptr);
				if (!result)
					break;
			}	
			new_free(&newexp);
			will_catch_break_exceptions--;
			will_catch_continue_exceptions--;
			return;
		}
		/* falls through to here if its /do {...} */
		parse_line (NULL, body, subargs ? subargs : empty_string, 0, 0, 1);
	}
	/* falls through to here if it its /do ... */
	parse_line (NULL, args, subargs ? subargs : empty_string, 0, 0, 1);
}

/*ARGSUSED*/
BUILT_IN_COMMAND(whilecmd)
{
	char	*exp = NULL,
		*ptr = NULL,
		*body = NULL,
		*newexp = NULL;
	int	args_used;	/* this isn't used here, but is passed
				 * to expand_alias() */
	int whileval = !strcmp(command, "WHILE");

	if (!(ptr = next_expr(&args, '(')))
	{
		error("WHILE: missing boolean expression");
		return;
	}
	exp = LOCAL_COPY(ptr);
	if ((ptr = next_expr_failok(&args, '{')) == (char *) 0)
		ptr = args;

	body = LOCAL_COPY(ptr);

	will_catch_break_exceptions++;
	will_catch_continue_exceptions++;
	make_local_stack(NULL);
	while (1)
	{
		newexp = LOCAL_COPY(exp);
		ptr = parse_inline(newexp, subargs ? subargs : empty_string, &args_used);
		if (check_val(ptr) != whileval)
			break;
		new_free(&ptr);
		parse_line(NULL, body, subargs ?  subargs : empty_string, 0, 0, 0);
		if (continue_exception)
		{
			continue_exception = 0;
			continue;
		}
		if (break_exception)
		{
			break_exception = 0;
			break;
		}
		if (return_exception)
			break;
	}
	will_catch_break_exceptions--;
	will_catch_continue_exceptions--;
	destroy_local_stack();
	new_free(&ptr);
}

BUILT_IN_COMMAND(foreach)
{
	char	*struc = NULL,
		*ptr,
		*body = NULL,
		*var = NULL;
	char	**sublist;
	int	total;
	int	i;
	int	slen;
	int	old_display;
	int     list = VAR_ALIAS;
	int	af;
	
        while (args && my_isspace(*args))
        	args++;

        if (*args == '-')
                args++, list = COMMAND_ALIAS;

	if ((ptr = new_next_arg(args, &args)) == NULL)
	{
		error("FOREACH: missing structure expression");
		return;
	}
	struc = upper(remove_brackets(ptr, subargs, &af));

	if ((var = next_arg(args, &args)) == NULL)
	{
		new_free(&struc);
		error("FOREACH: missing variable");
		return;
	}
	while (my_isspace(*args))
		args++;

	if ((body = next_expr(&args, '{')) == NULL)	/* } */
	{
		new_free(&struc);
		error("FOREACH: missing statement");
		return;
	}

	if ((sublist = get_subarray_elements(struc, &total, list)) == NULL)
	{
		new_free(&struc);
		return;		/* Nothing there. */
	}

	slen=strlen(struc);
	old_display=window_display;
	make_local_stack(NULL);
	for (i=0;i<total;i++)
	{
		window_display=0;
		add_local_alias(var, sublist[i]+slen+1);
		window_display=old_display;
		parse_line(NULL, body, subargs ? subargs:empty_string, 0, 0, 0);
		new_free(&sublist[i]);
	}
	destroy_local_stack();
	new_free((char **)&sublist);
	new_free(&struc);
}

/*
 * FE:  Written by Jeremy Nelson (jnelson@iastate.edu)
 *
 * FE: replaces recursion
 *
 * The thing about it is that you can nest variables, as this command calls
 * expand_alias until the list doesnt change.  So you can nest lists in
 * lists, and hopefully that will work.  However, it also makes it 
 * impossible to have $s anywhere in the list.  Maybe ill change that
 * some day.
 */

BUILT_IN_COMMAND(fe)
{
	char    *list = NULL,
		*templist = NULL,
		*placeholder,
		*sa,
		*vars,
        *varmem = NULL,
		*var[255],
		*word = NULL,
		*todo = NULL,
		fec_buffer[2] = { 0 };
	int     ind, y, args_flag;
	int     old_display;
	int	doing_fe = !my_stricmp(command, "FE");

	list = next_expr(&args, '(');

	if (!list)
	{
		error("%s: Missing List for /%s", command, command);
		return;
	}

	sa = subargs ? subargs : space;

	templist = expand_alias(list, sa, &args_flag, NULL);
	if (!templist || !*templist)
	{
		new_free(&templist);
		return;
	}

	vars = args;
	if (!(args = strchr(args, '{')))		/* } */
	{
		error("%s: Missing commands", command);
		new_free(&templist);
		return;
	}

    /* This is subtle - we have to create a duplicate of the string
     * containing the var names, in case there's no space between
     * it and the commands. */
	args[0] = '\0';
    malloc_strcpy(&varmem, vars);
    vars = varmem;
    args[0] = '{';
    
	ind = 0;
	while ((var[ind] = next_arg(vars, &vars)))
	{
        ind++;

		if (ind == 255)
		{
			error("%s: Too many variables", command);
			new_free(&templist);
            new_free(&varmem);
			return;
		}
	}

	if (ind < 1)
	{
		error("%s: You did not specify any variables", command);
		new_free(&templist);
        new_free(&varmem);
		return;
	}

	if (!(todo = next_expr(&args, '{')))		/* } { */
	{
		error("%s: Missing }", command);		
		new_free(&templist);
        new_free(&varmem);
		return;
	}

	old_display = window_display;

	placeholder = templist;

	will_catch_break_exceptions++;
	will_catch_continue_exceptions++;

	make_local_stack(NULL);

    if (doing_fe) {
        /* FE */
        word = new_next_arg(templist, &templist);
    } else {
        /* FEC */
        word = fec_buffer; 
        word[0] = *templist++;
        if (word[0] == '\0')
            word = NULL;
    }

    while (word)
    {
        window_display = 0;
        for ( y = 0 ; y < ind ; y++ )
        {
            if (word) {
                add_local_alias(var[y], word);

                if (doing_fe) {
                    /* FE */
                    word = new_next_arg(templist, &templist);
                } else {
                    /* FEC */
                    word[0] = *templist++;
                    if (word[0] == '\0')
                        word = NULL;
                }
            } else {
                add_local_alias(var[y], empty_string);
            }
        }
        window_display = old_display;
        parse_line(NULL, todo, subargs?subargs:empty_string, 0, 0, 0);
        if (continue_exception)
        {
            continue_exception = 0;
            continue;
        }
        if (break_exception)
        {
            break_exception = 0;
            break;
        }
        if (return_exception)
            break;
    }

	destroy_local_stack();
	will_catch_break_exceptions--;
	will_catch_continue_exceptions--;

	window_display = old_display;
	new_free(&placeholder);
    new_free(&varmem);
}

/* FOR command..... prototype: 
 *  for (commence,evaluation,iteration)
 * in the same style of C's for, the for loop is just a specific
 * type of WHILE loop.
 *
 * IMPORTANT: Since ircII uses ; as a delimeter between commands,
 * commas were chosen to be the delimiter between expressions,
 * so that semicolons may be used in the expressions (think of this
 * as the reverse as C, where commas seperate commands in expressions,
 * and semicolons end expressions.
 */
/*  I suppose someone could make a case that since the
 *  foreach_handler() routine weeds out any for command that doesnt have
 *  two commans, that checking for those 2 commas is a waste.  I suppose.
 */
BUILT_IN_COMMAND(forcmd)
{
	char        *working        = NULL;
	char        *commence       = NULL;
	char        *evaluation     = NULL;
	char        *lameeval       = NULL;
	char        *iteration      = NULL;
	char        *sa             = NULL;
	int         argsused        = 0;
	char        *blah           = NULL;
	char        *commands       = NULL;

	/* Get the whole () thing */
	if ((working = next_expr(&args, '(')) == NULL)	/* ) */
	{
		error("FOR: missing closing parenthesis");
		return;
	}
	commence = LOCAL_COPY(working);

	/* Find the beginning of the second expression */

	evaluation = strchr(commence, ',');
	if (!evaluation)
	{
		error("FOR: no components!");
		return;
	}
	do 
		*evaluation++ = '\0';
	while (my_isspace(*evaluation));

	/* Find the beginning of the third expression */
	iteration = strchr(evaluation, ',');
	if (!iteration)
	{
		error("FOR: Only two components!");
		return;
	}
	do 
	{
		*iteration++ = '\0';
	}
	while (my_isspace(*iteration));

	working = args;
	while (my_isspace(*working))
		*working++ = '\0';

	if ((working = next_expr(&working, '{')) == NULL)		/* } */
	{
		error("FOR: badly formed commands");
		return;
	}

	make_local_stack(NULL);

	commands = LOCAL_COPY(working);

	sa = subargs?subargs:empty_string;
	parse_line(NULL, commence, sa, 0, 0, 0);

	will_catch_break_exceptions++;
	will_catch_continue_exceptions++;
	
	while (1)
	{
		lameeval = LOCAL_COPY(evaluation);

		blah = parse_inline(lameeval,sa,&argsused);
		if (!check_val(blah))
		{
			new_free(&blah);
			break;
		}

		new_free(&blah);
		parse_line(NULL, commands, sa, 0, 0, 0);
		if (break_exception)
		{
			break_exception = 0;
			break;
		}
		if (continue_exception)
			continue_exception = 0;	/* Dont continue here! */
		if (return_exception)
			break;
		parse_line(NULL, iteration, sa, 0, 0, 0);
	}

	destroy_local_stack();
	will_catch_break_exceptions--;
	will_catch_continue_exceptions--;

	new_free(&blah);
}

/*

  Need to support something like this:

	switch (text to be matched)
	{
		(sample text)
		{
			...
		}
		(sample text2)
		(sample text3)
		{
			...
		}
		...
	}

How it works:

	The command is technically made up a single (...) expression and
	a single {...} expression.  The (...) expression is taken to be
	regular expando-text (much like the (...) body of /fe.

	The {...} body is taken to be a series of [(...)] {...} pairs.
	The [(...)] expression is taken to be one or more consecutive
	(...) structures, which are taken to be text expressions to match
	against the header text.  If any of the (...) expressions are found
	to match, then the commands in the {...} body are executed.

	There may be as many such [(...)] {...} pairs as you need.  However,
	only the *first* pair found to be matching is actually executed,
	and the others are ignored, so placement of your switches are
	rather important:  Put your most general ones last.

*/
BUILT_IN_COMMAND(switchcmd)
{
	char *control, *body, *header, *commands;
	int af;
	int found_def = 0;
	char *def = NULL;
	
	if (!(control = next_expr(&args, '(')))
	{
		error("SWITCH: String to be matched not found where expected");
		return;
	}

	control = expand_alias(control, subargs, &af, NULL);
	if (internal_debug & DEBUG_EXPANSIONS && !in_debug_yell)
		debugyell("%s expression expands to: (%s)", command, control);

	if (!(body = next_expr(&args, '{')))
		error("SWITCH: Execution body not found where expected");

	make_local_stack(NULL);
	while (body && *body)
	{
		int hooked = 0;

		while (*body == '(')
		{
			if (!(header = next_expr(&body, '(')))
			{
				error("SWITCH: Case label not found where expected");
				new_free(&control);
				return;
			}
			if (!strcmp(header, "default"))
			{
				if (def)
				{
					error("SWITCH: No more than one \"default\" case");
					new_free(&control);
					return;
				}
				found_def = 1;
			}
			header = expand_alias(header, subargs, &af, NULL);
			if (internal_debug & DEBUG_EXPANSIONS && !in_debug_yell)
				debugyell("%s expression expands to: (%s)", command, header);
			if (wild_match(header, control))
				hooked = 1;
			new_free(&header);
			if (*body == ';')
				body++;		/* ugh. */
		}

		if (!(commands = next_expr(&body, '{')))
		{
			error("SWITCH: case body not found where expected");
			break;
		}

		if (hooked)
		{
			parse_line(NULL, commands, subargs, 0, 0, 0);
			def = NULL;
			break;
		} 
		else if (!def && found_def)
		{
			def = LOCAL_COPY(commands);
			found_def = 0;
		}

		if (*body == ';')
			body++;		/* grumble */
	}
	if (def && *def)
		parse_line(NULL, def, subargs, 0, 0, 0);
	destroy_local_stack();
	new_free(&control);
}

BUILT_IN_COMMAND(repeatcmd)
{
	char *num_expr = NULL;
	int value;

	while (isspace((unsigned char)*args))
		args++;

	if (*args == '(')
	{
		char *tmp_val;
		char *dumb_copy;
		int argsused;
		char *sa = subargs ? subargs : empty_string;

		num_expr = next_expr(&args, '(');
		dumb_copy = LOCAL_COPY(num_expr);
		tmp_val = parse_inline(dumb_copy,sa,&argsused);
		value = my_atol(tmp_val);
		new_free(&tmp_val);
	}
	else
	{
		char *tmp_val;
		int af;

		num_expr = new_next_arg(args, &args);
		tmp_val = expand_alias(num_expr, subargs, &af, NULL);
		value = my_atol(tmp_val);
		new_free(&tmp_val);
	}

	if (value <= 0)
		return;
	while (value--)
		parse_line(NULL, args, subargs ? subargs : empty_string, 0, 0, 1);

	return;
}
