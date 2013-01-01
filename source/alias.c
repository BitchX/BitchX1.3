/*
 * alias.c Handles command aliases for irc.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990, 1995 Michael Sandroff and others 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#define _cs_alist_hash_
#include "irc.h"
static char cvsrevision[] = "$Id: alias.c 26 2008-04-30 13:57:56Z keaston $";
CVS_REVISION(alias_c)
#include "struct.h"
#include "alias.h"
#include "alist.h"
#include "array.h"
#include "dcc.h"
#include "commands.h"
#include "files.h"
#include "history.h"
#include "hook.h"
#include "input.h"
#include "ircaux.h"
#include "names.h"
#include "notify.h"
#include "numbers.h"
#include "output.h"
#include "parse.h"
#include "screen.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "window.h"
#include "ircterm.h"
#include "server.h"
#include "list.h"
#include "keys.h"
#include "userlist.h"
#include "misc.h"
#include "newio.h"
#include "stack.h"
#include "module.h"
#include "timer.h"
#define MAIN_SOURCE
#include "modval.h"

#include <sys/stat.h>

#define LEFT_BRACE '{'
#define RIGHT_BRACE '}'
#define LEFT_BRACKET '['
#define RIGHT_BRACKET ']'
#define LEFT_PAREN '('
#define RIGHT_PAREN ')'
#define DOUBLE_QUOTE '"'

#ifdef WANT_DLL
#ifdef dll_functions
#error blah
#endif
BuiltInDllFunctions *dll_functions = NULL;
#endif



/* Used for statistics gathering */
static unsigned long alias_total_allocated = 0;
static unsigned long alias_total_bytes_allocated = 0;
static unsigned long var_cache_hits = 0;
static unsigned long var_cache_misses = 0;
static unsigned long var_cache_missed_by = 0;
static unsigned long var_cache_passes = 0;
static unsigned long cmd_cache_hits = 0;
static unsigned long cmd_cache_misses = 0;
static unsigned long cmd_cache_missed_by = 0;
static unsigned long cmd_cache_passes = 0;

int     last_function_call_level = -1;

/* Ugh. XXX Bag on the side */
ArgList	*parse_arglist (char *arglist);
void	destroy_arglist (ArgList *);


static	void	delete_all_var_alias (char *name);
static	void	delete_cmd_alias   (char *name, int owd);
static	void	list_cmd_alias     (char *name);
static	void	list_var_alias     (char *name);
static	void	list_local_alias   (char *name);
static	char *	get_variable_with_args (const char *str, const char *args, int *args_flag);
	void	add_cmd_alias (char *, ArgList *, char *);

/*
 * after_expando: This replaces some much more complicated logic strewn
 * here and there that attempted to figure out just how long an expando 
 * name was supposed to be.  Well, now this changes that.  This will slurp
 * up everything in 'start' that could possibly be put after a $ that could
 * result in a syntactically valid expando.  All you need to do is tell it
 * if the expando is an rvalue or an lvalue (it *does* make a difference)
 */
static 	char *lval[] = { "rvalue", "lvalue" };
char *after_expando (char *start, int lvalue, int *call)
{
	char	*rest;
	char	*str;

	/*
	 * One or two leading colons are allowed
	 */
	if (!*start)
		return start;
		
	str = start;
	if (*str == ':')
		if (*++str == ':')
			++str;

	/*
	 * This handles 99.99% of the cases
	 */
	while (*str && (isalpha((unsigned char)*str) || isdigit((unsigned char)*str) || *str == '_' || *str == '.'))
		str++;

	/*
	 * This handles any places where a var[var] could sneak in on
	 * us.  Supposedly this should never happen, but who can tell?
	 */
	while (*str == '[')
	{
		if (!(rest = MatchingBracket(str + 1, '[', ']')))
		{
			if (!(rest = strchr(str, ']')))
			{
				debugyell("Unmatched bracket in %s (%s)", 
						lval[lvalue], start);
				return empty_string;
			}
		}
		str = rest + 1;
	}

	/*
	 * Rvalues may include a function call, slurp up the argument list.
	 */
	if (!lvalue && *str == '(')
	{
		if (!(rest = MatchingBracket(str + 1, '(', ')')))
		{
			if (!(rest = strchr(str, ')')))
			{
				debugyell("Unmatched paren in %s (%s)", 
						lval[lvalue], start);
				return empty_string;
			}
		}
		*call = 1;
		str = rest + 1;
	}

  	/*
	 * If the entire thing looks to be invalid, perhaps its a 
	 * special built-in expando.  Check to see if it is, and if it
	 * is, then slurp up the first character as valid.
	 */
	if (str == start || (str == start + 1 && *start == ':'))
	{
		int is_builtin = 0;

		built_in_alias(*start, &is_builtin);
		if (is_builtin && (str == start))
			str++;
	}



	/*
	 * All done!
	 */
	return str;
}


/*
 * aliascmd: The user interface for /ALIAS
 */
BUILT_IN_COMMAND(aliascmd)
{
	char *name;
	char *real_name;
	char *ptr;
	void show_alias_caches(void);

	/*
	 * If no name present, list all aliases
	 */
	if (!(name = next_arg(args, &args)))
	{
		list_cmd_alias(NULL);
		return;
	}

	/*
	 * Alias can take an /s arg, which shows some data we've collected
	 */
	if (!my_strnicmp(name, "/S", 2))
	{
extern u_32int_t       bin_ints;
extern u_32int_t       lin_ints;
extern u_32int_t       bin_chars;
extern u_32int_t       lin_chars;
extern u_32int_t       alist_searches;
extern u_32int_t       char_searches;

		say("Total aliases handled: %ld", 
 			alias_total_allocated);
		say("Total bytes allocated to aliases: %ld", 
			alias_total_bytes_allocated);

		say("Var command hits/misses/passes/missed-by [%ld/%ld/%ld/%3.1f]",
			var_cache_hits, 
			var_cache_misses, 
			var_cache_passes, 
			( var_cache_misses ? (double) 
			  (var_cache_missed_by / var_cache_misses) : 0.0));
		say("Cmd command hits/misses/passes/missed-by [%ld/%ld/%ld/%3.1f]",
			cmd_cache_hits, 
			cmd_cache_misses, 
			cmd_cache_passes,
			( cmd_cache_misses ? (double)
			  (cmd_cache_missed_by / cmd_cache_misses) : 0.0));
	    say("Ints(bin/lin)/Chars(bin/lin)/Lookups: [(%d/%d)/(%d/%d)] (%d/%d)",
			 bin_ints, lin_ints, bin_chars, lin_chars,
			 alist_searches, char_searches);
		show_alias_caches();
		return;
	}

	/*
	 * Canonicalize the alias name
	 */
	real_name = remove_brackets(name, NULL, NULL);

	/*
	 * Find the argument body
	 */
	while (my_isspace(*args))
		args++;

	/*
	 * If there is no argument body, we probably want to delete alias
	 */
	if (!args || !*args)
	{
		/*
		 * If the alias name starts with a hyphen, then we are
		 * going to delete it.
		 */
		if (real_name[0] == '-')
		{
			if (real_name[1])
				delete_cmd_alias(real_name + 1, window_display);
			else
				say("You must specify an alias to be removed.");
		}

		/*
		 * Otherwise, the user wants us to list that alias
		 */
		else
			list_cmd_alias(real_name);
	}

	/*
	 * If there is an argument body, then we have to register it
	 */
	else
	{
		ArgList *arglist = NULL;

		/*
		 * Aliases may contain a parameter list, which is parsed
		 * at registration time.
		 */
		if (*args == LEFT_PAREN)
		{
			ptr = MatchingBracket(++args, LEFT_PAREN, RIGHT_PAREN);
			if (!ptr)
				say("Unmatched lparen in %s %s", 
					command, real_name);
			else
			{
				*ptr++ = 0;
				while (*ptr && my_isspace(*ptr))
					ptr++;
				if (!*ptr)
					say("Missing alias body in %s %s", 
						command, real_name);

				while (*args && my_isspace(*args))
					args++;

				arglist = parse_arglist(args);
				args = ptr;
			}
		}

		/*
		 * Aliases' bodies can be surrounded by a set of braces,
		 * which are stripped off.
		 */
		if (*args == LEFT_BRACE)
		{
			ptr = MatchingBracket(++args, LEFT_BRACE, RIGHT_BRACE);

			if (!ptr)
				say("Unmatched brace in %s %s", command, real_name);
			else 
			{
				*ptr++ = 0;
				while (*ptr && my_isspace(*ptr))
					ptr++;

				if (*ptr)
					say("Junk [%s] after closing brace in %s %s", ptr, command, real_name);

				while (*args && my_isspace(*args))
					args++;

			}
		}

		/*
		 * Register the alias
		 */
		add_cmd_alias(real_name, arglist, args);
	}

	new_free(&real_name);
	return;
}


BUILT_IN_COMMAND(purge)
{	
char *arg, *real_name;
	while ((arg = next_arg(args, &args)))
	{
		real_name = remove_brackets(arg, NULL, NULL);
		delete_all_var_alias(real_name);
		new_free(&real_name);
	}
}

/*
 * User front end to the ASSIGN command
 * Syntax to add variable:    ASSIGN name text
 * Syntax to delete variable: ASSIGN -name
 */
BUILT_IN_COMMAND(assigncmd)
{
	char *real_name;
	char *name;

	/*
	 * If there are no arguments, list all the global variables
	 */
	if (!(name = next_arg(args, &args)))
	{
		list_var_alias(NULL);
		return;
	}

	/*
	 * Canonicalize the variable name
	 */
	real_name = remove_brackets(name, NULL, NULL);

	/*
	 * Find the stuff to assign to the variable
	 */
	while (my_isspace(*args))
		args++;

	/*
	 * If there is no body, then the user probably wants to delete
	 * the variable
	 */
	if (!args || !*args)
	{
		/*
	 	 * If the variable name starts with a hyphen, then we remove
		 * the variable
		 */
		if (real_name[0] == '-')
		{
			if (real_name[1])
				delete_var_alias(real_name + 1, window_display);
			else
				say("You must specify an alias to be removed.");
		}

		/*
		 * Otherwise, the user wants us to list the variable
		 */
		else
			list_var_alias(real_name);
	}

	/*
	 * Register the variable
	 */
	else
		add_var_alias(real_name, args);

	new_free(&real_name);
	return;
}

/*
 * User front end to the STUB command
 * Syntax to stub an alias to a file:	STUB ALIAS name[,name] filename(s)
 * Syntax to stub a variable to a file:	STUB ASSIGN name[,name] filename(s)
 */
BUILT_IN_COMMAND(stubcmd)
{
	int type;
	char *cmd;
	char *name;

	/*
	 * The first argument is the type of stub to make
	 * (alias or assign)
	 */
	if (!(cmd = upper(next_arg(args, &args))))
	{
		error("Missing Stub type");
		return;
	}

	if (!strncmp(cmd, "ALIAS", strlen(cmd)))
		type = COMMAND_ALIAS;
	else if (!strncmp(cmd, "ASSIGN", strlen(cmd)))
		type = VAR_ALIAS;
	else
	{
		error("[%s] is an Unrecognized stub type", cmd);
		return;
	}

	/*
	 * The next argument is the name of the item to be stubbed.
	 * This is not optional.
	 */
	if (!(name = next_arg(args, &args)))
	{
		error("Missing alias name");
		return;
	}

	/*
	 * Find the filename argument
	 */
	while (my_isspace(*args))
		args++;

	/*
	 * The rest of the argument(s) are the files to load when the
	 * item is referenced.  This is not optional.
	 */
	if (!args || !*args)
	{
		error("Missing file name");
		return;
	}

	/*
	 * Now we iterate over the item names we were given.  For each
	 * item name, seperated from the next by a comma, stub that item
	 * to the given filename(s) specified as the arguments.
	 */
	while (name && *name)
	{
		char 	*next_name;
		char	*real_name;

		if ((next_name = strchr(name, ',')))
			*next_name++ = 0;

		real_name = remove_brackets(name, NULL, NULL);
		if (type == COMMAND_ALIAS)
			add_cmd_stub_alias(real_name, args);
		else
			add_var_stub_alias(real_name, args);

		new_free(&real_name);
		name = next_name;
	}
}

BUILT_IN_COMMAND(localcmd)
{
	char *name;

	if (!(name = next_arg(args, &args)))
	{
		list_local_alias(NULL);
		return;
	}

	while (args && *args && my_isspace(*args))
		args++;

	if (!args)
		args = empty_string;

	while (name && *name)
	{
		char 	*next_name;
		char	*real_name;

		if ((next_name = strchr(name, ',')))
			*next_name++ = 0;

		real_name = remove_brackets(name, NULL, NULL);
		add_local_alias(real_name, args);
		new_free(&real_name);
		name = next_name;
	}
}

BUILT_IN_COMMAND(dumpcmd)
{
	FILE *fp;
	char *filename = NULL;
	char *expand = NULL;
	char 	*blah;

	if (!args || !*args)
	{
		if (!command)
		{
			destroy_aliases(COMMAND_ALIAS);
			destroy_aliases(VAR_ALIAS);
			destroy_aliases(VAR_ALIAS_LOCAL);
			flush_on_hooks();
			delete_all_ext_fset();
			delete_all_timers();
			delete_all_arrays();
		}
		return;
		
	}
	while ((blah = next_arg(args, &args)))
	{
		if (!my_strnicmp(blah,"FI",2))
		{
#ifdef PUBLIC_ACCESS
			bitchsay("This command has been disabled on a public access system");
			return;
#else
			malloc_sprintf(&filename, "~/%s.dump", version);
			expand = expand_twiddle(filename);
			new_free(&filename);
			if ((fp = fopen(expand, "w")) != NULL)
			{
				save_bindings(fp, 0);
				save_hooks(fp, 0);
				save_variables(fp, 0);
				save_aliases(fp, 0);
				save_assigns(fp, 0);
				save_servers(fp);
				fclose(fp);
			}
			bitchsay("Saved to ~/%s.dump", version);
			new_free(&expand);
#endif
		}
		else if (!my_strnicmp(blah,"BIND",4) || !my_strnicmp(blah, "KEY", 3))
		{
			remove_bindings();
			init_keys();
			init_keys2();
		}
		else if (!my_strnicmp(blah, "AR", 2))
			delete_all_arrays();
		else if (!my_strnicmp(blah,"V",1))			
			destroy_aliases(VAR_ALIAS);
		else if (!my_strnicmp(blah,"ALI",3))
			destroy_aliases(COMMAND_ALIAS);
		else if (!my_strnicmp(blah,"O",1))
			flush_on_hooks();
		else if (!my_strnicmp(blah,"CH",2))
			flush_channel_stats();
		else if (!my_strnicmp(blah,"LO",2))
			destroy_aliases(VAR_ALIAS_LOCAL);
		else if (!my_strnicmp(blah, "TI", 2))
			delete_all_timers();
		else if (!my_strnicmp(blah, "FS", 2))
		{
			create_fsets(NULL, get_int_var(DISPLAY_ANSI_VAR));
			delete_all_ext_fset();
		}
		else if (!my_strnicmp(blah, "WS", 2))
		{
			Window *win = NULL;
			while (traverse_all_windows(&win))
			{
				if (win->wset)
				{
					remove_wsets_for_window(win);
					win->wset = create_wsets_for_window(win);
					build_status(win, NULL, 0);
				}
			}
		}
		else if (!my_strnicmp(blah, "CS", 2))
		{
			ChannelList *chan;
			if (from_server == -1)
				return;
			for (chan = get_server_channels(from_server); chan; chan = chan->next)
			{
				remove_csets_for_channel(chan->csets);
				chan->csets = create_csets_for_channel(chan->channel);
			}
		}
		else if (!my_strnicmp(blah,"ALL",3))
		{
			remove_bindings();
			init_keys();
			init_keys2();
			destroy_aliases(COMMAND_ALIAS);
			destroy_aliases(VAR_ALIAS);
			destroy_aliases(VAR_ALIAS_LOCAL);
			flush_on_hooks();
			delete_all_timers();
			delete_all_ext_fset();
			delete_all_arrays();
		}
	}
}

/*
 * Argument lists look like this:
 *
 * LIST   := LPAREN TERM [COMMA TERM] RPAREN)
 * LPAREN := '('
 * RPAREN := ')'
 * COMMA  := ','
 * TERM   := LVAL QUAL | "..." | "void"
 * LVAL   := <An alias name>
 * QUAL   := NUM "words"
 *
 * In English:
 *   An argument list is a comma seperated list of variable descriptors (TERM)
 *   enclosed in a parenthesis set.  Each variable descriptor may contain any
 *   valid alias name followed by an action qualifier, or may be the "..."
 *   literal string, or may be the "void" literal string.  If a variable
 *   descriptor is an alias name, then that positional argument will be removed
 *   from the argument list and assigned to that variable.  If the qualifier
 *   specifies how many words are to be taken, then that many are taken.
 *   If the variable descriptor is the literal string "...", then all argument
 *   list parsing stops there and all remaining alias arguments are placed 
 *   into the $* special variable.  If the variable descriptor is the literal
 *   string "void", then the balance of the remaining alias arguments are 
 *   discarded and $* will expand to the false value.  If neither "..." nor
 *   "void" are provided in the argument list, then that last variable in the
 *   list will recieve all of the remaining arguments left at its position.
 * 
 * Examples:
 *
 *   # This example puts $0 in $channel, $1 in $mag, and $2- in $nicklist.
 *   /alias opalot (channel, mag, nicklist) {
 *	fe ($nicklist) xx yy zz {
 *	    mode $channel ${mag}ooo $xx $yy $zz
 *	}
 *   }
 *
 *   # This example puts $0 in $channel, $1 in $mag, and the new $* is old $2-
 *   /alias opalot (channel, mag, ...) {
 *	fe ($*) xx yy zz {
 *	    mode $channel ${mag}ooo $xx $yy $zz
 *	}
 *   }
 *
 *   # This example throws away any arguments passed to this alias
 *   /alias booya (void) { echo Booya! }
 */
ArgList	*parse_arglist (char *arglist)
{
	char *	this_term;
	char *	next_term;
	char *	varname;
	char *	modifier, *value;
	int	arg_count = 0;
	ArgList *args = new_malloc(sizeof(ArgList));

	args->void_flag = args->dot_flag = 0;
	for (this_term = arglist; *this_term; this_term = next_term,arg_count++)
	{
		while (isspace((unsigned char)*this_term))
			this_term++;
		next_in_comma_list(this_term, &next_term);
		if (!(varname = next_arg(this_term, &this_term)))
			continue;
		if (!my_stricmp(varname, "void")) {
			args->void_flag = 1;
			break;
		} else if (!my_stricmp(varname, "...")) {
			args->dot_flag = 1;
			break;
		} else {
			args->vars[arg_count] = m_strdup(varname);
			args->defaults[arg_count] = NULL;

			if (!(modifier = next_arg(this_term, &this_term)))
				continue;
			if (!my_stricmp(modifier, "default"))
			{
			    if (!(value = new_next_arg(this_term, &this_term)))
				continue;
			    args->defaults[arg_count] = m_strdup(value);
			}
		}
	}
	args->vars[arg_count] = NULL;
	return args;
}

void	destroy_arglist (ArgList *arglist)
{
	int	i = 0;

	if (!arglist)
		return;

	for (i = 0; ; i++)
	{
		if (!arglist->vars[i])
			break;
		new_free(&arglist->vars[i]);
		new_free(&arglist->defaults[i]);
	}
	new_free(&arglist);
}

void	prepare_alias_call (void *al, char **stuff)
{
	ArgList *args = (ArgList *)al;
	int	i;

	if (!args)
		return;

	for (i = 0; args->vars[i]; i++)
	{
		char	*next_val;
		char	*expanded = NULL;
		int	af;

		/* Last argument on the list and no ... argument? */
		if (!args->vars[i + 1] && !args->dot_flag && !args->void_flag)
		{
			next_val = *stuff;
			*stuff = empty_string;
		}

		/* Yank the next word from the arglist */
		else
			next_val = next_arg(*stuff, stuff);

		if (!next_val || !*next_val)
		{
			if ((next_val = args->defaults[i]))
				next_val = expanded = expand_alias(next_val, *stuff, &af, NULL);
			else
				next_val = empty_string;
		}

		/* Add the local variable */
		add_local_alias(args->vars[i], next_val);
		if (expanded)
			new_free(&expanded);
	}

	/* Throw away rest of args if wanted */
	if (args->void_flag)
		*stuff = empty_string;
}

/**************************** INTERMEDIATE INTERFACE *********************/
/* We define Alias here to keep it encapsulated */
/*
 * This is the description of an alias entry
 * This is an ``array_item'' structure
 */

/*
 * This is the description for a list of aliases
 * This is an ``array_set'' structure
 */
#define ALIAS_CACHE_SIZE 10

typedef struct	AliasStru
{
	Alias **list;
	int	max;
	int	max_alloc;
	alist_func func;
	Alias  **cache;
	int	cache_size;
	int	revoke_index;
}	AliasSet;

static AliasSet var_alias = 	{ NULL, 0, 0, strncmp, NULL, 0, 0 };
static AliasSet cmd_alias = 	{ NULL, 0, 0, strncmp, NULL, 0, 0 };

	Alias *	find_var_alias     (char *name);
static	Alias *	find_cmd_alias     (char *name, int *cnt);
static	Alias *	find_local_alias   (char *name, AliasSet **list);

/*
 * This is the ``stack frame''.  Each frame has a ``name'' which is
 * the name of the alias or on of the frame, or is NULL if the frame
 * is not an ``enclosing'' frame.  Each frame also has a ``current command''
 * that is being executed, which is used to help us when the client crashes.
 * Each stack also contains a list of local variables.
 */
typedef struct RuntimeStackStru
{
	char	*name;		/* Name of the stack */
	char 	*current;	/* Current cmd being executed */
	AliasSet alias;		/* Local variables */
	int	locked;
	int	parent;
}	RuntimeStack;

/*
 * This is the master stack frame.  Its size is saved in ``max_wind''
 * and the current frame being used is stored in ``wind_index''.
 */
static 	RuntimeStack *call_stack = NULL;
	int 	max_wind = -1;
	int 	wind_index = -1;

void show_alias_caches(void)
{
	int i;
	for (i = 0; i < var_alias.cache_size; i++)
	{
		if (var_alias.cache[i])
			debugyell("VAR cache [%d]: [%s] [%s]", i, var_alias.cache[i]->name, var_alias.cache[i]->stuff);
		else
			debugyell("VAR cache [%d]: empty", i);
	}

	for (i = 0; i < cmd_alias.cache_size; i++)
	{
		if (cmd_alias.cache[i])
			debugyell("CMD cache [%d]: [%s] [%s]", i, cmd_alias.cache[i]->name, cmd_alias.cache[i]->stuff);
		else
			debugyell("CMD cache [%d]: empty", i);
	}
}




Alias *make_new_Alias (char *name)
{
	Alias *tmp = (Alias *) new_malloc(sizeof(Alias));
	tmp->name = m_strdup(name);
	tmp->stuff = tmp->stub = NULL;
	alias_total_bytes_allocated += sizeof(Alias) + strlen(name);
	return tmp;
}


/*
 * add_var_alias: Add a global variable
 *
 * name -- name of the alias to create (must be canonicalized)
 * stuff -- what to have ``name'' expand to.
 *
 * If ``name'' is FUNCTION_RETURN, then it will create the local
 * return value (die die die)
 *
 * If ``name'' refers to an already created local variable, that
 * local variable is used (invisibly)
 */
void	add_var_alias	(char *name, char *stuff)
{
	char *ptr;
	Alias *tmp = NULL;
	int af;
	int local = 0;
	char *save;
	
	save = name = remove_brackets(name, NULL, &af);
	if (*name == ':')
	{
		name++, local = 1;
		if (*name == ':')
			name++, local = -1;
	}
	/*
	 * Weed out invalid variable names
	 */
	ptr = after_expando(name, 1, NULL);
	if (*ptr)
  		error("ASSIGN names may not contain '%c' (You asked for [%s])", *ptr, name);
		

	/*
	 * Weed out FUNCTION_RETURN (die die die)
	 */
	else if (!strcmp(name, "FUNCTION_RETURN"))
		add_local_alias(name, stuff);

	/*
	 * Pass the buck on local variables
	 */
	else if ((local == 1) || ((local == 0) && find_local_alias(name, NULL)))
		add_local_alias(name, stuff);

	else if (stuff && *stuff)
	{
		int cnt, loc;

		/*
		 * Look to see if the given alias already exists.
		 * If it does, and the ``stuff'' to assign to it is
		 * empty, then we should remove the variable outright
		 */
		tmp = (Alias *) find_array_item ((Array *)&var_alias, name, &cnt, &loc);
		if (!tmp || cnt >= 0)
		{
			tmp = make_new_Alias(name);
			add_to_array ((Array *)&var_alias, (Array_item *)tmp);
		}

		/*
		 * Then we fill in the interesting details
		 */
		malloc_strcpy(&(tmp->stuff), stuff);
		new_free(&tmp->stub);
		tmp->global = loading_global;

		alias_total_allocated++;
		alias_total_bytes_allocated += strlen(tmp->name) + strlen(tmp->stuff);

		/*
		 * And tell the user.
		 */
		say("Assign %s added [%s]", name, stuff);
	}
	else
		delete_var_alias(name, window_display);

	new_free(&save);
	return;
}

void	add_local_alias	(char *name, char *stuff)
{
	char *ptr;
	Alias *tmp = NULL;
	AliasSet *list = NULL;
	int af;

	name = remove_brackets(name, NULL, &af);

	/*
	 * Weed out invalid variable names
	 */
	ptr = after_expando(name, 1, NULL);
	if (*ptr)
	{
		error("LOCAL names may not contain '%c' (You asked for [%s])", *ptr, name);	
		new_free(&name);
		return;
	}
	/*
	 * Now we see if this local variable exists anywhere
	 * within our view.  If it is, we dont care where.
	 * If it doesnt, then we add it to the current frame,
	 * where it will be reaped later.
	 */
	if (!(tmp = find_local_alias (name, &list)))
	{
		tmp = make_new_Alias(name);
		add_to_array ((Array *)list, (Array_item *)tmp);
	}

	/* Fill in the interesting stuff */
	malloc_strcpy(&(tmp->stuff), stuff);
	alias_total_allocated++;
	alias_total_bytes_allocated += strlen(tmp->stuff);
	if (x_debug & DEBUG_LOCAL_VARS)
		debugyell("Assign %s (local) added [%s]", name, stuff);
	else
		say("Assign %s (local) added [%s]", name, stuff);

	new_free(&name);
	return;
}

void	add_cmd_alias	(char *name, ArgList *arglist, char *stuff)
{
	Alias *tmp = NULL;
	int cnt, af, loc;

	name = remove_brackets(name, NULL, &af);

	tmp = (Alias *) find_array_item ((Array *)&cmd_alias, name, &cnt, &loc);
	if (!tmp || cnt >= 0)
	{
		tmp = make_new_Alias(name);
		add_to_array ((Array *)&cmd_alias, (Array_item *)tmp);
	}

	malloc_strcpy(&(tmp->stuff), stuff);
	new_free(&tmp->stub);
	tmp->global = loading_global;
	tmp->arglist = arglist;
	
	alias_total_allocated++;
	alias_total_bytes_allocated += strlen(tmp->stuff);
	say("Alias	%s added [%s]", name, stuff);
	new_free(&name);
	return;
}

void	add_var_stub_alias  (char *name, char *stuff)
{
	Alias *tmp = NULL;
	char *ptr;
	int af;

	name = remove_brackets(name, NULL, &af);

	ptr = after_expando(name, 1, NULL);
	if (*ptr)
		error("Assign names may not contain '%c' (You asked for [%s])", *ptr, name);

	else if (!strcmp(name, "FUNCTION_RETURN"))
		error("You may not stub the FUNCTION_RETURN variable.");

	else 
	{
		int cnt, loc;

		tmp = (Alias *) find_array_item ((Array *)&var_alias, name, &cnt, &loc);
		if (!tmp || cnt >= 0)
		{
			tmp = make_new_Alias(name);
			add_to_array ((Array *)&var_alias, (Array_item *)tmp);
		}

		malloc_strcpy(&(tmp->stub), stuff);
		new_free(&tmp->stuff);
		tmp->global = loading_global;

		alias_total_allocated++;
		alias_total_bytes_allocated += strlen(tmp->stub);
		say("Assign %s stubbed to file %s", name, stuff);
	}

	new_free(&name);
	return;
}


void	add_cmd_stub_alias  (char *name, char *stuff)
{
	Alias *tmp = NULL;
	int cnt, af;

	name = remove_brackets(name, NULL, &af);
	if (!(tmp = find_cmd_alias(name, &cnt)) || cnt >= 0)
	{
		tmp = make_new_Alias(name);
		add_to_array ((Array *)&cmd_alias, (Array_item *)tmp);
	}

	malloc_strcpy(&(tmp->stub), stuff);
	new_free(&tmp->stuff);
	tmp->global = loading_global;

	alias_total_allocated++;
	alias_total_bytes_allocated += strlen(tmp->stub);
	say("Assign %s stubbed to file %s", name, stuff);
	new_free(&name);
	return;
}


/************************ LOW LEVEL INTERFACE *************************/
/* XXXX */
static void unstub_alias (Alias *item);

static void resize_cache (AliasSet *set, int newsize)
{
	int 	c, d;
	int 	oldsize = set->cache_size;

	set->cache_size = newsize;
	if (newsize < oldsize)
	{
		for (d = oldsize; d < newsize; d++)
			set->cache[d]->cache_revoked = ++set->revoke_index;
	}

	RESIZE(set->cache, Alias *, set->cache_size);
	for (c = oldsize; c < set->cache_size; c++)
		set->cache[c] = NULL;
}


/*
 * 'name' is expected to already be in canonical form (uppercase, dot notation)
 */
Alias *	find_var_alias (char *name)
{
	Alias *item = NULL;
	register int cache;
	int loc;
	int cnt = 0;
	register int i;
	u_32int_t       mask;
	u_32int_t hash = cs_alist_hash(name, &mask);
	
	if (!strcmp(name, "::"))
		name +=2;
		
	if (var_alias.cache_size == 0)
		resize_cache(&var_alias, ALIAS_CACHE_SIZE);

	for (cache = 0; cache < var_alias.cache_size; cache++)
	{
		if (var_alias.cache[cache] && 
		    var_alias.cache[cache]->name &&
		    (var_alias.cache[cache]->hash == hash) &&
		    !strcmp(name, var_alias.cache[cache]->name))
		{
			item = var_alias.cache[cache];
			cnt = -1;
			var_cache_hits++;
			break;
		}
	}
	if (!item)
	{
		cache = var_alias.cache_size - 1;
		if ((item = (Alias *) find_array_item ((Array *)&var_alias, name, &cnt, &loc)))
			var_cache_misses++;
		else
			var_cache_passes++;
	}

	if (cnt < 0)
	{
		if (var_alias.cache[cache])
			var_alias.cache[cache]->cache_revoked = ++var_alias.revoke_index;

		for (i = cache; i > 0; i--)
			var_alias.cache[i] = var_alias.cache[i - 1];
		var_alias.cache[0] = item;

		if (item->cache_revoked)
			var_cache_missed_by += var_alias.revoke_index - item->cache_revoked;

		if (item->stub)
		{
			unstub_alias(item);
			if (!(item = find_var_alias(name)))
				return NULL;
			if (!item->stuff)
			{
				delete_var_alias(item->name, 0);
				return NULL;
			}
		}

		return item;
	}

	return NULL;
}

static Alias *	find_cmd_alias (char *name, int *cnt)
{
	Alias *item = NULL;
	int loc;
	register int i;
	register int cache;
	u_32int_t       mask;
	u_32int_t hash = cs_alist_hash(name, &mask);

	if (cmd_alias.cache_size == 0)
		resize_cache(&cmd_alias, ALIAS_CACHE_SIZE);

	for (cache = 0; cache < cmd_alias.cache_size; cache++)
	{
		if (cmd_alias.cache[cache] && cmd_alias.cache[cache]->name &&
			(cmd_alias.cache[cache]->hash == hash) &&
			!strcmp(name, cmd_alias.cache[cache]->name))
		{
			item = cmd_alias.cache[cache];
			*cnt = -1;
			cmd_cache_hits++;
			break;
		}
		if (cmd_alias.cache[cache] && !cmd_alias.cache[cache]->name)
			cmd_alias.cache[cache] = NULL;
	}

	if (!item)
	{
		cache = cmd_alias.cache_size - 1;
		if ((item = (Alias *) find_array_item ((Array *)&cmd_alias, name, cnt, &loc)))
			cmd_cache_misses++;
		else
			cmd_cache_passes++;
	}

	if (*cnt < 0 || *cnt == 1)
	{
		if (cmd_alias.cache[cache])
			cmd_alias.cache[cache]->cache_revoked = ++cmd_alias.revoke_index;

		for (i = cache; i > 0; i--)
			cmd_alias.cache[i] = cmd_alias.cache[i - 1];
		cmd_alias.cache[0] = item;

		if (item->cache_revoked)
			cmd_cache_missed_by += cmd_alias.revoke_index - item->cache_revoked;

		if (item->stub)
		{
			unstub_alias(item);
			if (!(find_cmd_alias(name, cnt)))
				return NULL; 
			if (!item->stuff)
			{
				delete_cmd_alias(item->name, 0);
				*cnt = 0;
				return NULL;
			}
		}

		if (item->stuff)
			return item;
		/* XXXXXXXXXXXXXXXXXXXXXXXXX */
	}

	return NULL;
}


static void unstub_alias (Alias *item)
{
	static int already_looking = 0;
	char *copy;

	copy = LOCAL_COPY(item->stub);
	new_free((char **)&item->stub);

	/* 
	 * If already_looking is 1, then
	 * we are un-stubbing this alias
	 * because we are loading a file
	 * that presumably it must be in.
	 * So we dont load it again (duh).
	 */
	if (already_looking)
		return;

	already_looking = 1;
	load("LOAD", copy, empty_string, NULL);
	already_looking = 0;
}


/*
 * An example will best describe the semantics:
 *
 * A local variable will be returned if and only if there is already a
 * variable that is exactly ``name'', or if there is a variable that
 * is an exact leading subset of ``name'' and that variable ends in a
 * period (a dot).
 */
static Alias *	find_local_alias (char *name, AliasSet **list)
{
	Alias *alias = NULL;
	int c = wind_index;
	char *ptr;
	int implicit = -1;
	int function_return = 0;
	
	/* No name is an error */
	if (!name)
		return NULL;

	ptr = after_expando(name, 1, NULL);
	if (*ptr || !call_stack)
		return NULL;
	if (!my_stricmp(name, "FUNCTION_RETURN"))
		function_return = 1;

	/*
	 * Search our current local variable stack, and wind our way
	 * backwards until we find a NAMED stack -- that is the enclosing
	 * alias or ON call.  If we find a variable in one of those enclosing
	 * stacks, then we use it.  If we dont, we progress.
	 *
	 * This needs to be optimized for the degenerate case, when there
	 * is no local variable available...  It will be true 99.999% of
	 * the time.
	 */
	for (c = wind_index; c >= 0; c = call_stack[c].parent)
	{
		 /* XXXXX */
		if (function_return && last_function_call_level != -1)
			c = last_function_call_level;

		if (x_debug & DEBUG_LOCAL_VARS)
			debugyell("Looking for [%s] in level [%d]", name, c);

		if (call_stack[c].alias.list)
		{
			register int x;

			/* XXXX - This is bletcherous */
			for (x = 0; x < call_stack[c].alias.max; x++)
			{
				size_t len = strlen(call_stack[c].alias.list[x]->name);

				if (streq(call_stack[c].alias.list[x]->name, name) == len)
				{
					if (call_stack[c].alias.list[x]->name[len-1] == '.')
						implicit = c;
					else if (strlen(name) == len)
					{
						alias = call_stack[c].alias.list[x];
						break;
					}
				}
				else
				{
					if (my_stricmp(call_stack[c].alias.list[x]->name, name) > 0)
						continue;
				}
			}

			if (!alias && implicit >= 0)
			{
				alias = make_new_Alias(name);
				add_to_array ((Array *)&call_stack[implicit].alias, (Array_item *)alias);
			}
		}

		if (alias)
		{
			if (x_debug & DEBUG_LOCAL_VARS) 
				debugyell("I found [%s] in level [%d] (%s)", name, c, alias->stuff);
			break;
		}

		if (*call_stack[c].name || call_stack[c].parent == -1)
		{
			if (x_debug & DEBUG_LOCAL_VARS) 
				debugyell("I didnt find [%s], stopped at level [%d]", name, c);
			break;
		}
	}

	if (alias)
	{
		if (list)
			*list = &call_stack[c].alias;
		return alias;
	}
	else if (list)
		*list = &call_stack[wind_index].alias;

	return NULL;
}




static void	delete_all_var_alias (char *name)
{
	Alias *item;
	int i;
	int count = 0;
	upper(name);
	while ((item = (Alias *) remove_all_from_array((Array *)&var_alias, name)))
	{
		for (i = 0; i < ALIAS_CACHE_SIZE; i++)
		{
			if (var_alias.cache[i] == item)
				var_alias.cache[i] = NULL;
		}

		new_free(&(item->name));
		new_free(&(item->stuff));
		new_free(&(item->stub));
		new_free((char **)&item);
		count++;
	}
}

void	delete_var_alias (char *name, int owd)
{
	Alias *item;
	int i;

	upper(name);
	if ((item = (Alias *)remove_from_array ((Array *)&var_alias, name)))
	{
		for (i = 0; i < var_alias.cache_size; i++)
		{
			if (var_alias.cache[i] == item)
				var_alias.cache[i] = NULL;
		}

		new_free(&(item->name));
		new_free(&(item->stuff));
		new_free(&(item->stub));
		new_free((char **)&item);
		if (owd)
			say("Assign %s removed", name);
	}
	else if (owd)
		say("No such assign: %s", name);
}

static void	delete_cmd_alias (char *name, int owd)
{
	Alias *item;
	int i;

	upper(name);
	if ((item = (Alias *)remove_from_array ((Array *)&cmd_alias, name)))
	{
		for (i = 0; i < cmd_alias.cache_size; i++)
		{
			if (cmd_alias.cache[i] == item)
				cmd_alias.cache[i] = NULL;
		}

		new_free(&(item->name));
		new_free(&(item->stuff));
		new_free(&(item->stub));
		destroy_arglist(item->arglist);
		new_free((char **)&item);
		if (owd)
			say("Alias	%s removed", name);
	}
	else if (owd)
		say("No such alias: %s", name);
}





static void	list_local_alias (char *name)
{
	int 	len = 0, cnt;
	int	DotLoc,	LastDotLoc = 0;
	char	*LastStructName = NULL;
	char	*s;
	
	say("Visible Local Assigns:");
	if (name)
	{
		upper(name);
		len = strlen(name);
	}

	for (cnt = wind_index; cnt >= 0; cnt = call_stack[cnt].parent)
	{
		int x;
		if (!call_stack[cnt].alias.list)
			continue;
		for (x = 0; x < call_stack[cnt].alias.max; x++)
		{
			if (!name || !strncmp(call_stack[cnt].alias.list[x]->name, name, len))
			{
				if ((s = strchr(call_stack[cnt].alias.list[x]->name + len, '.')))
				{
					DotLoc = s - call_stack[cnt].alias.list[x]->name;
					if (!LastStructName || (DotLoc != LastDotLoc) || strncmp(call_stack[cnt].alias.list[x]->name, LastStructName, DotLoc))
					{
						put_it("\t%*.*s\t<Structure>", DotLoc, DotLoc, call_stack[cnt].alias.list[x]->name);
						LastStructName = call_stack[cnt].alias.list[x]->name;
						LastDotLoc = DotLoc;
					}
				}
				else
				{
					if (call_stack[cnt].alias.list[x]->stub)
						put_it("\t%s STUBBED TO %s", call_stack[cnt].alias.list[x]->name, call_stack[cnt].alias.list[x]->stub);
					else
						put_it("\t%s\t%s", call_stack[cnt].alias.list[x]->name, call_stack[cnt].alias.list[x]->stuff);
				}
			}
		}
	}
}

/*
 * This function is strictly O(N).  Its possible to address this.
 */
static void	list_var_alias (char *name)
{
	int	len = 0;
	int	DotLoc,
		LastDotLoc = 0;
	char	*LastStructName = NULL;
	int	cnt;
	char	*s;

	say("Assigns:");

	if (name)
	{
		upper(name);
		len = strlen(name);
	}

	for (cnt = 0; cnt < var_alias.max; cnt++)
	{
		if (!name || !strncmp(var_alias.list[cnt]->name, name, len))
		{
			if ((s = strchr(var_alias.list[cnt]->name + len, '.')))
			{
				DotLoc = s - var_alias.list[cnt]->name;
				if (!LastStructName || (DotLoc != LastDotLoc) || strncmp(var_alias.list[cnt]->name, LastStructName, DotLoc))
				{
					put_it("\t%*.*s\t<Structure>", DotLoc, DotLoc, var_alias.list[cnt]->name);
					LastStructName = var_alias.list[cnt]->name;
					LastDotLoc = DotLoc;
				}
			}
			else
			{
				if (var_alias.list[cnt]->stub)
					put_it("\t%s STUBBED TO %s", var_alias.list[cnt]->name, var_alias.list[cnt]->stub);
				else
					put_it("\t%s\t%s", var_alias.list[cnt]->name, var_alias.list[cnt]->stuff);
			}
		}
	}
}

/*
 * This function is strictly O(N).  Its possible to address this.
 */
static void	list_cmd_alias (char *name)
{
	int	len = 0;
	int	DotLoc,
		LastDotLoc = 0;
	char	*LastStructName = NULL;
	int	cnt;
	char	*s;

	say("Aliases:");

	if (name)
	{
		upper(name);
		len = strlen(name);
	}

	for (cnt = 0; cnt < cmd_alias.max; cnt++)
	{
		if (!name || !strncmp(cmd_alias.list[cnt]->name, name, len))
		{
			if ((s = strchr(cmd_alias.list[cnt]->name + len, '.')))
			{
				DotLoc = s - cmd_alias.list[cnt]->name;
				if (!LastStructName || (DotLoc != LastDotLoc) || strncmp(cmd_alias.list[cnt]->name, LastStructName, DotLoc))
				{
					put_it("\t%*.*s\t<Structure>", DotLoc, DotLoc, cmd_alias.list[cnt]->name);
					LastStructName = cmd_alias.list[cnt]->name;
					LastDotLoc = DotLoc;
				}
			}
			else
			{
				if (cmd_alias.list[cnt]->stub)
					put_it("\t%s STUBBED TO %s", cmd_alias.list[cnt]->name, cmd_alias.list[cnt]->stub);
				else
					put_it("\t%s\t%s", cmd_alias.list[cnt]->name, cmd_alias.list[cnt]->stuff);
			}
		}
	}
}


/************************* DIRECT VARIABLE EXPANSION ************************/
/*
 * get_variable: This simply looks up the given str.  It first checks to see
 * if its a user variable and returns it if so.  If not, it checks to see if
 * it's an IRC variable and returns it if so.  If not, it checks to see if
 * its and environment variable and returns it if so.  If not, it returns
 * null.  It mallocs the returned string 
 */
char 	*get_variable 	(char *str)
{
	int af = 0;
	return get_variable_with_args(str, NULL, &af);
}


static  char	*get_variable_with_args (const char *str, const char *args, int *args_flag)
{
	Alias	*alias = NULL;
	char	*ret = NULL;
	char	*name = NULL;
	char	*freep = NULL;
	int	copy = 0;
	int	local = 0;
		
	freep = name = remove_brackets(str, args, args_flag);

	if (*name == ':' && name[1])
	{
		name++, local = 1;
		if (*name == ':')
			name++, local = -1;
	}

	/*
	 * local == -1  means "local variables not allowed"
	 * local == 0   means "locals first, then globals"
	 * local == 1   means "global variables not allowed"
	 */

	if ((local != -1) && (alias = find_local_alias(name, NULL)))
		copy = 1, ret = alias->stuff;
	else if (local == 1)
		;
	else if ((alias = find_var_alias(name)) != NULL)
		copy = 1, ret = alias->stuff;
	else if ((strlen(str) == 1) && (ret = built_in_alias(*str, NULL)))
		;
	else if ((ret = make_string_var(str)))
		;
	else if ((ret = make_fstring_var(str)))
		;
	else
		copy = 1, ret = getenv(str);

	if (x_debug & DEBUG_UNKNOWN && ret == NULL)
		debugyell("Variable lookup to non-existant assign [%s]", name);
	if ((internal_debug & DEBUG_VARIABLE) && alias_debug && !in_debug_yell)
		debugyell("%3d \t@%s == %s", debug_count++, str, ret ? ret : empty_string); 
	new_free(&freep);
	return (copy ? m_strdup(ret) : ret);
}

void debug_alias(char *name, int x)
{
	Alias *item;
	int cnt;
	if (name && *name)
	{
		if ((item = find_cmd_alias(name, &cnt)) && cnt < 0)
			item->debug = DEBUG_CMDALIAS;
	}
}

char *	get_cmd_alias (char *name, int *howmany, char **complete_name, void **args)
{
	Alias *item;

	if ((item = find_cmd_alias(name, howmany)))
	{
		alias_debug += (item->debug ? 1 : 0);
		if (complete_name)
			malloc_strcpy(complete_name, item->name);
		if (args)
			*args = (void *)item->arglist;
		return item->stuff;
	}
	return NULL;
}

/*
 * This function is strictly O(N).  This should probably be addressed.
 */
char **	glob_cmd_alias (char *name, int *howmany)
{
	int	cnt;
	int	cmp;
	int 	len;
	char 	**matches = NULL;
	int 	matches_size = 5;

	len = strlen(name);
	*howmany = 0;
	RESIZE(matches, char *, matches_size);

	for (cnt = 0; cnt < cmd_alias.max; cnt++)
	{
		if (!(cmp = strncmp(name, cmd_alias.list[cnt]->name, len)))
		{
			if (strchr(cmd_alias.list[cnt]->name + len, '.'))
				continue;

			matches[*howmany] = m_strdup(cmd_alias.list[cnt]->name);
			*howmany += 1;
			if (*howmany == matches_size)
			{
				matches_size += 5;
				RESIZE(matches, char *, matches_size);
			}
		}
		else if (cmp < 0)
			break;
	}

	if (*howmany)
		matches[*howmany] = NULL;
	else
		new_free((char **)&matches);

	return matches;
}

/*
 * This function is strictly O(N).  This should probably be addressed.
 */
char **	glob_assign_alias (char *name, int *howmany)
{
	int    	cnt;
	int     cmp;
	int     len;
	char    **matches = NULL;
	int     matches_size = 5;

	len = strlen(name);
	*howmany = 0;
	RESIZE(matches, char *, matches_size);

	for (cnt = 0; cnt < var_alias.max; cnt++)
	{
		if (!(cmp = strncmp(name, var_alias.list[cnt]->name, len)))
		{
			if (strchr(var_alias.list[cnt]->name + len, '.'))
				continue;

			matches[*howmany] = m_strdup(var_alias.list[cnt]->name);
			*howmany += 1;
			if (*howmany == matches_size)
			{
				matches_size += 5;
				RESIZE(matches, char *, matches_size);
			}
		}
		else if (cmp < 0)
			break;
	}

	if (*howmany)
		matches[*howmany] = NULL;
	else
		new_free((char **)&matches);

	return matches;
}

/*
 * This function is strictly O(N).  This should probably be addressed.
 */
char **	pmatch_cmd_alias (char *name, int *howmany)
{
	int	cnt;
	int 	len;
	char 	**matches = NULL;
	int 	matches_size = 5;

	len = strlen(name);
	*howmany = 0;
	RESIZE(matches, char *, matches_size);

	for (cnt = 0; cnt < cmd_alias.max; cnt++)
	{
		if (wild_match(name, cmd_alias.list[cnt]->name))
		{
			matches[*howmany] = m_strdup(cmd_alias.list[cnt]->name);
			*howmany += 1;
			if (*howmany == matches_size)
			{
				matches_size += 5;
				RESIZE(matches, char *, matches_size);
			}
		}
	}

	if (*howmany)
		matches[*howmany] = NULL;
	else
		new_free((char **)&matches);

	return matches;
}

/*
 * This function is strictly O(N).  This should probably be addressed.
 */
char **	pmatch_assign_alias (char *name, int *howmany)
{
	int    	cnt;
	int     len;
	char    **matches = NULL;
	int     matches_size = 5;

	len = strlen(name);
	*howmany = 0;
	RESIZE(matches, char *, matches_size);

	for (cnt = 0; cnt < var_alias.max; cnt++)
	{
		if (wild_match(name, var_alias.list[cnt]->name))
		{
			matches[*howmany] = m_strdup(var_alias.list[cnt]->name);
			*howmany += 1;
			if (*howmany == matches_size)
			{
				matches_size += 5;
				RESIZE(matches, char *, matches_size);
			}
		}
	}

	if (*howmany)
		matches[*howmany] = NULL;
	else
		new_free((char **)&matches);

	return matches;
}


/*
 * This function is strictly O(N).  This should probably be addressed.
 */
char **	get_subarray_elements (char *root, int *howmany, int type)
{
	AliasSet *as;		/* XXXX */

	register int len, len2;
	register int cnt;
	int cmp = 0;
	char **matches = NULL;
	int matches_size = 5;
	size_t end;
	char *last = NULL;
	
	if (type == COMMAND_ALIAS)
		as = &cmd_alias;
	else
		as = &var_alias;

	len = strlen(root);
	*howmany = 0;
	RESIZE(matches, char *, matches_size);
	for (cnt = 0; cnt < as->max; cnt++)
	{
		len2 = strlen(as->list[cnt]->name);
		if ( (len < len2) &&
		     ((cmp = streq(root, as->list[cnt]->name)) == len))
		{
			if (as->list[cnt]->name[cmp] == '.')
			{
				end = strcspn(as->list[cnt]->name + cmp + 1, ".");
				if (last && !my_strnicmp(as->list[cnt]->name, last, cmp + 1 + end))
					continue;
				matches[*howmany] = m_strndup(as->list[cnt]->name, cmp + 1 + end);
				last = matches[*howmany];
				*howmany += 1;
				if (*howmany == matches_size)
				{
					matches_size += 5;
					RESIZE(matches, char *, matches_size);
				}
			}
		}
	}

	if (*howmany)
		matches[*howmany] = NULL;
	else
		new_free((char **)&matches);

	return matches;
}

char *	parse_line_alias_special (char *name, char *what, char *args, int d1, int d2, void *arglist, int function)
{
	int	old_window_display = window_display;
	int	old_last_function_call_level = last_function_call_level;
	char	*result = NULL;

	window_display = 0;
	
	make_local_stack(name);
	prepare_alias_call(arglist, &args);
	if (function)
	{
		last_function_call_level = wind_index;
		add_local_alias("FUNCTION_RETURN", empty_string);
	}
	window_display = old_window_display;

	will_catch_return_exceptions++;
	parse_line(NULL, what, args, d1, d2, 1);
	will_catch_return_exceptions--;
	return_exception = 0;

	if (function)
	{
		result = get_variable("FUNCTION_RETURN");
		last_function_call_level = old_last_function_call_level;
	}
	destroy_local_stack();

	return result;
}

char *	parse_line_with_return (char *name, char *what, char *args, int d1, int d2)
{
	return parse_line_alias_special(name, what, args, d1, d2, NULL, 1);
}


/************************************************************************/
/*
 * call_user_function: Executes a user alias (by way of parse_command.
 * The actual function ends up being routed through execute_alias (below)
 * and we just keep track of the retval and stuff.  I dont know that anyone
 * depends on command completion with functions, so we can save a lot of
 * CPU time by just calling execute_alias() directly.
 */
char 	*call_user_function	(char *alias_name, char *args)
{
	char	*result = NULL;
	char	*sub_buffer;
	int	cnt;
	void	*arglist = NULL;
	
	sub_buffer = get_cmd_alias(alias_name, &cnt, NULL, &arglist);
	if (cnt < 0)
		result = parse_line_alias_special(alias_name, sub_buffer, args, 0, 1, arglist, 1);
	else if (x_debug & DEBUG_UNKNOWN)
		debugyell("Function call to non-existant alias [%s]", alias_name);

	if (!result)
		result = m_strdup(empty_string);

	return result;
}
/* XXX Ugh. */
void	call_user_alias	(char *alias_name, char *alias_stuff, char *args, void *arglist)
{
	parse_line_alias_special(alias_name, alias_stuff, args,	0, 1, arglist, 0);
}



/*
 * save_aliases: This will write all of the aliases to the FILE pointer fp in
 * such a way that they can be read back in using LOAD or the -l switch 
 */

void 	save_assigns	(FILE *fp, int do_all)
{
	int cnt = 0;

	for (cnt = 0; cnt < var_alias.max; cnt++)
	{
		if (!var_alias.list[cnt]->global || do_all)
		{
			if (var_alias.list[cnt]->stub)
				fprintf(fp, "STUB ");
			fprintf(fp, "ASSIGN %s %s\n", var_alias.list[cnt]->name, var_alias.list[cnt]->stuff);
		}
	}
}

void 	save_aliases	(FILE *fp, int do_all)
{
	int cnt = 0;

#if 0
	for (cnt = 0; cnt < var_alias.max; cnt++)
	{
		if (!var_alias.list[cnt]->global || do_all)
		{
			if (var_alias.list[cnt]->stub)
				fprintf(fp, "STUB ");
			fprintf(fp, "ASSIGN %s %s\n", var_alias.list[cnt]->name, var_alias.list[cnt]->stuff);
		}
	}
#endif
	for (cnt = 0; cnt < cmd_alias.max; cnt++)
	{
		if (!cmd_alias.list[cnt]->global || do_all)
		{
			if (cmd_alias.list[cnt]->stub)
				fprintf(fp, "STUB ");
			fprintf(fp, "ALIAS %s %s\n", cmd_alias.list[cnt]->name, cmd_alias.list[cnt]->stuff);
		}
	}	
}

void 	destroy_aliases (int type)
{
	int cnt = 0;
	AliasSet *my_array = NULL;

	if (type == COMMAND_ALIAS)
		my_array = &cmd_alias;
	else if (type == VAR_ALIAS)
		my_array = &var_alias;
	else if (type == VAR_ALIAS_LOCAL)
		my_array = &call_stack[wind_index].alias;

	for (cnt = 0; cnt < my_array->max; cnt++)
	{
		new_free(&(my_array->list[cnt]->stuff));
		new_free(&(my_array->list[cnt]->name));
		new_free(&(my_array->list[cnt]->stub));
		new_free(&(my_array->list[cnt]));
		new_free((void **)&(my_array->list[cnt]));      /* XXX Hrm. */
	}
	for (cnt = 0; cnt < my_array->cache_size; cnt++)
		my_array->cache[cnt] = NULL;
	new_free(&my_array->list);
	my_array->max = my_array->max_alloc = 0;
}

/******************* RUNTIME STACK SUPPORT **********************************/

void 	BX_make_local_stack 	(char *name)
{
	wind_index++;

	if (wind_index >= max_wind)
	{
		int tmp_wind = wind_index;

		if (max_wind == -1)
			max_wind = 8;
		else
			max_wind <<= 1;

		RESIZE(call_stack, RuntimeStack, max_wind+1);
		for (; wind_index <= max_wind; wind_index++)
		{
			call_stack[wind_index].alias.max = 0;
			call_stack[wind_index].alias.max_alloc = 0;
			call_stack[wind_index].alias.list = NULL;
			call_stack[wind_index].current = NULL;
			call_stack[wind_index].name = NULL;
			call_stack[wind_index].alias.func = strncmp;
			call_stack[wind_index].parent = -1;
			call_stack[wind_index].alias.cache = NULL;
			call_stack[wind_index].alias.cache_size = 0;
			call_stack[wind_index].alias.revoke_index = 0;
		}
		wind_index = tmp_wind;
	}

	/* Just in case... */
	destroy_local_stack();
	wind_index++;		/* XXXX - chicanery */

	if (name)
	{
		call_stack[wind_index].name = name;
		call_stack[wind_index].parent = -1;
	}
	else
	{
		call_stack[wind_index].name = empty_string;
		call_stack[wind_index].parent = wind_index - 1;
	}
	call_stack[wind_index].locked =  0;
}

int	find_locked_stack_frame	(void)
{
	int i;
	for (i = 0; i < wind_index; i++)
		if (call_stack[i].locked)
 			return i;

	return -1;
}

void	bless_local_stack	(void)
{
	call_stack[wind_index].name = empty_string;
	call_stack[wind_index].parent = find_locked_stack_frame();
}

char    *return_this_alias(void)
{
	int ind = wind_index;
	if (ind != -1)
	{
		if (call_stack[ind].name[0])
			return call_stack[ind].name;
		while (!(call_stack[ind].name[0]) && ind)
			ind--;
		return call_stack[ind].name;
	}
	return empty_string;
}

void 	BX_destroy_local_stack	(void)
{
	/*
	 * We clean up as best we can here...
	 */
	if (call_stack[wind_index].alias.list)
		destroy_aliases(VAR_ALIAS_LOCAL);
	if (call_stack[wind_index].current)
		call_stack[wind_index].current = 0;
	if (call_stack[wind_index].name)
		call_stack[wind_index].name = 0;

	wind_index--;
}

void 	set_current_command 	(char *line)
{
	call_stack[wind_index].current = line;
}

void 	unset_current_command 	(void)
{
	call_stack[wind_index].current = NULL;
}

void	BX_lock_stack_frame 	(void)
{
	call_stack[wind_index].locked = 1;
}

void	BX_unlock_stack_frame	(void)
{
	int lock = find_locked_stack_frame();
	if (lock >= 0)
		call_stack[lock].locked = 0;
}
  

void 	dump_call_stack 	(void)
{
	int my_wind_index = wind_index;
	if (my_wind_index >= 0)
	{
		say("Call stack");
		while (my_wind_index--)
			say("[%3d] %s", my_wind_index, call_stack[my_wind_index].current);
		say("End of call stack");
	}
}

void 	panic_dump_call_stack 	(void)
{
	int my_wind_index = wind_index;
	printf("Call stack\n");
	if (wind_index >= 0)
	{
		while (my_wind_index--)
			printf("[%3d] %s\n", my_wind_index, call_stack[my_wind_index].current);
	} else
		printf("call stack corruption\n");
	printf("End of call stack\n");
}


/*
 * You may NOT call this unless youre about to exit.
 * If you do (call this when youre not about to exit), and you do it
 * very often, max_wind will get absurdly large.  So dont do it.
 *
 * XXXX - this doesnt clean up everything -- but do i care?
 */
void 	destroy_call_stack 	(void)
{
	wind_index = 0;
	new_free((char **)&call_stack);
}

#if 0
char *lookup_member(char *varname, char *var_args, char *ptr, char *args)
{
	char *structs[] = { "WINDOW", "DCC", "CHANNEL", "NICK", NULL };
	int i;
	
	for (i = 0; structs[i]; i++)
		if (!my_stricmp(varname, structs[i]))
			break;
	if (!structs[i]) 
		return NULL;
	switch (i)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		default:
			break;
	}
	return NULL;
}
#endif

#include "expr2.c"
#include "expr.c"

/****************************** ALIASCTL ************************************/
#define EMPTY empty_string
#define RETURN_EMPTY return m_strdup(EMPTY)
#define RETURN_IF_EMPTY(x) if (empty( x )) RETURN_EMPTY
#define GET_INT_ARG(x, y) {RETURN_IF_EMPTY(y); x = my_atol(safe_new_next_arg(y, &y));}
#define GET_FLOAT_ARG(x, y) {RETURN_IF_EMPTY(y); x = atof(safe_new_next_arg(y, &y));}
#define GET_STR_ARG(x, y) {RETURN_IF_EMPTY(y); x = new_next_arg(y, &y);RETURN_IF_EMPTY(x);}
#define RETURN_STR(x) return m_strdup(x ? x : EMPTY)
#define RETURN_INT(x) return m_strdup(ltoa(x));

/* Used by function_aliasctl */
/* MUST BE FIXED */
BUILT_IN_FUNCTION(aliasctl)
{
	int list = -1;
	char *listc;
	enum { GET, SET, MATCH, PMATCH } op;

	GET_STR_ARG(listc, input);
	if (!my_strnicmp(listc, "AS", 2))
		list = VAR_ALIAS;
	else if (!my_strnicmp(listc, "AL", 2))
		list = COMMAND_ALIAS;
	else if (!my_strnicmp(listc, "LO", 2))
		list = VAR_ALIAS_LOCAL;
	else if (!my_strnicmp(listc, "CO", 2))
		list = -1;
	else
		RETURN_EMPTY;

	GET_STR_ARG(listc, input);
	if (!my_strnicmp(listc, "G", 1))
		op = GET;
	else if (!my_strnicmp(listc, "S", 1))
		op = SET;
	else if (!my_strnicmp(listc, "M", 1))
		op = MATCH;
	else if (!my_strnicmp(listc, "P", 1))
		op = PMATCH;
	else
		RETURN_EMPTY;

	if (list == -1 && op != MATCH)
		RETURN_EMPTY;
		
	GET_STR_ARG(listc, input);

	switch (op)
	{
		case (GET) :
		{
			Alias *alias = NULL;
			AliasSet *a_list;
			int dummy;

			upper(listc);
			if (list == VAR_ALIAS_LOCAL)
				alias = find_local_alias(listc, &a_list);
			else if (list == VAR_ALIAS)
				alias = find_var_alias(listc);
			else
				alias = find_cmd_alias(listc, &dummy);

			if (alias)
				RETURN_STR(alias->stuff);
			else
				RETURN_EMPTY;
		}
		case (SET) :
		{
			upper(listc);
			if (list == VAR_ALIAS_LOCAL)
				add_local_alias(listc, input);
			else if (list == VAR_ALIAS)
				add_var_alias(listc, input);
			else
				add_cmd_alias(listc, NULL, input);

			RETURN_INT(1)
		}
		case (MATCH) :
		{
			char **mlist = NULL;
			char *mylist = NULL;
			int num = 0, ctr;

			if (!my_stricmp(listc, "*"))
				listc = empty_string;

			upper(listc);

			if (list == COMMAND_ALIAS)
				mlist = glob_cmd_alias(listc, &num);
			else if (list == VAR_ALIAS)
				mlist = glob_assign_alias(listc, &num);
			else if (list == -1)
				return glob_commands(listc, &num, 0);

			for (ctr = 0; ctr < num; ctr++)
			{
				m_s3cat(&mylist, space, mlist[ctr]);
				new_free((char **)&mlist[ctr]);
			}
			new_free((char **)&mlist);
			if (mylist)
				return mylist;
			RETURN_EMPTY;
		}
		case (PMATCH) : 
		{
			char **	mlist = NULL;
			char *	mylist = NULL;
			int	num = 0,
				ctr;

			if (list == COMMAND_ALIAS)
				mlist = pmatch_cmd_alias(listc, &num);
			else if (list == VAR_ALIAS)
				mlist = pmatch_assign_alias(listc, &num);
			else if (list == -1)
				return glob_commands(listc, &num, 1);

			for (ctr = 0; ctr < num; ctr++)
			{
				m_s3cat(&mylist, space, mlist[ctr]);
				new_free((char **)&mlist[ctr]);
			}
			new_free((char **)&mlist);
			if (mylist)
				return mylist;
			RETURN_EMPTY;
		}
		default :
			error("aliasctl: Error");
			RETURN_EMPTY;
	}
	RETURN_EMPTY;
}

/*************************** stacks **************************************/
typedef	struct	aliasstacklist
{
	int	which;
	char	*name;
	Alias	*list;
	struct aliasstacklist *next;
}	AliasStack;

static  AliasStack *	alias_stack = NULL;
static	AliasStack *	assign_stack = NULL;

void	do_stack_alias (int type, char *args, int which)
{
	char		*name;
	AliasStack	*aptr, **aptrptr;
	Alias		*alptr;
	int		cnt;
	int 		my_which = 0;
	
	if (which == STACK_DO_ALIAS)
	{
		name = "ALIAS";
		aptrptr = &alias_stack;
		my_which = COMMAND_ALIAS;
	}
	else
	{
		name = "ASSIGN";
		aptrptr = &assign_stack;
		my_which = VAR_ALIAS;
	}

	if (!*aptrptr && (type == STACK_POP || type == STACK_LIST))
	{
		say("%s stack is empty!", name);
		return;
	}

	if (type == STACK_PUSH)
	{
		if (which == STACK_DO_ALIAS)
		{
			if ((alptr = find_var_alias(args)))
				delete_var_alias(args, window_display);
		}
		else
		{
			if ((alptr = find_cmd_alias(args, &cnt)))
				delete_cmd_alias(args, window_display);
		}

		aptr = (AliasStack *)new_malloc(sizeof(AliasStack));
		aptr->list = alptr;
		aptr->name = m_strdup(args);
		aptr->next = aptrptr ? *aptrptr : NULL;
		*aptrptr = aptr;
		return;
	}

	if (type == STACK_POP)
	{
		AliasStack *prev = NULL;

		for (aptr = *aptrptr; aptr; prev = aptr, aptr = aptr->next)
		{
			/* have we found it on the stack? */
			if (!my_stricmp(args, aptr->name))
			{
				/* remove it from the list */
				if (prev == NULL)
					*aptrptr = aptr->next;
				else
					prev->next = aptr->next;

				/* throw away anything we already have */
				delete_cmd_alias(args, window_display);

				/* put the new one in. */
				if (aptr->list)
				{
					if (which == STACK_DO_ALIAS)
						add_to_array((Array *)&cmd_alias, (Array_item *)(aptr->list));
					else
						add_to_array((Array *)&var_alias, (Array_item *)(aptr->list));
				}

				/* free it */
				new_free((char **)&aptr->name);
				new_free((char **)&aptr);
				return;
			}
		}
		say("%s is not on the %s stack!", args, name);
		return;
	}
	if (STACK_LIST == type)
	{
		AliasStack	*tmp;

		say("%s STACK LIST", name);
		for (tmp = *aptrptr; tmp; tmp = tmp->next)
		{
			if (!tmp->list)
				say("\t%s\t<Placeholder>", tmp->name);

			else if (tmp->list->stub)
				say("\t%s STUBBED TO %s", tmp->name, tmp->list->stub);

			else
				say("\t%s\t%s", tmp->name, tmp->list->stuff);
		}
		return;
	}
	say("Unknown STACK type ??");
}

