/*
 * keys.c:  Keeps track of what happens whe you press a key.
 *
 * Written By Michael Sandrof
 * Copyright(c) 1990 Michael Sandrof
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Substantial re-implementation by Jeremy Nelson
 * Copyright 1998 EPIC Software Labs
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#include "irc.h"
static char cvsrevision[] = "$Id: keys.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(keys_c)
#include "struct.h"
#include "config.h"
#include "commands.h"
#include "history.h"
#include "ircaux.h"
#include "input.h"
#include "keys.h"
#include "names.h"
#include "output.h"
#include "screen.h"
#include "ircterm.h"
#include "vars.h"
#include "window.h"
#define MAIN_SOURCE
#include "modval.h"

#define KEY(meta, ch) (*keys[meta])[ch]

typedef unsigned char uc;
static 	void 	new_key 	(int, unsigned, int, int, char *);
static 	void	snew_key 	(int meta, unsigned chr, char *what);
static	uc *	display_key 	(uc c);
static	int 	lookup_function (const uc *name, int *lf_index);
static int	parse_key (const uc *sequence, uc *term);

#ifdef GUI
char *mouse_actions[] =
{
	"RCLICK",
	"STATUSRCLICK",
	"NICKLISTRCLICK",
	"LCLICK",
	"STATUSLCLICK",
	"NICKLISTLCLICK",
	"MCLICK",
	"STATUSMCLICK",
	"NICKLISTMCLICK",
	"RDBLCLICK",
	"STATUSRDBLCLICK",
	"NICKLISTRDBLCLICK",
	"LDBLCLICK",
	"STATUSLDBLCLICK",
	"NICKLISTLDBLCLICK",
	"MDBLCLICK",
	"STATUSMDBLCLICK",
	"NICKLISTMDBLCLICK"
};
#endif



/*
 * Yet again we've changed how the key maps are held.  This time, hopefully
 * its the second to last time, as we've made the entire things independant
 * of the number of meta keymaps that are available (it can change.)  The
 * only thing i see left to be done is to encapsulate all this data inside
 * a class so that different contexts can have different bindings sets.  
 * I'm sure that will come up some day.  But anyhow, on to the details!
 */

/*
 * The actual "stuff" that is in a keybinding is defined in keys.h.
 * At the time i write this, its just an int, two pointers, and a char,
 * so its 16 bytes (after padding)....
 *
 *	typedef struct
 *	{
 *		int	 	key_index;
 *		char		*stuff;
 *		char		*filename;
 *		char		changed;
 *	} KeyMap;
 *
 * Some special notes about 'key_index'.  No longer are the meta bindings
 * just normal bindings -- having them hardcoded as normal bindings limited
 * the ability of the user to set the number of meta states that they wanted.
 * Now all the negative values for 'key_index' are reserved for those meta
 * states.  If something is bound to META2_CHARACTER, then 'key_index' is
 * given the value -2.  Otherwise, 'key_index' is given the value of the 
 * entry of the binding in the 'key_names' table.  Note that 'key_index' is
 * not strictly sorted.  The first two bindings are special and are hardcoded,
 * and you must not change them.  Entry 0 must always be "NOTHING", and 
 * entry 1 must always be "SELF_INSERT".  The enum that was in keys.h
 * is now totaly obsolete -- we no longer use the symbolic names, but instead
 * always use the full string name for the binding.  This makes it much 
 * easier to add new key bindings, as there is only one place to manage.
 *
 *
 * Each meta map is a collection of 256 bindings...
 *
 *	typedef KeyMap MetaMap[256];
 *
 * But since most of the meta maps are either sparse or empty, it makes very 
 * little sense to actually allocate 4k (16 * 256) for each of the maps only 
 * to have them unused.  So instead the meta map is just 256 pointers to 
 * these objects, which then are allocated dynamically.  
 *
 *	typedef KeyMap *MetaMap[256];		(better)
 *
 * That means that the overhead for a completely empty meta map is 1k, and 
 * the map needs to be 75% full before it uses more memory than it would have
 * otherwise.  So its a reasonable win.
 *
 * Now we need to have the meta maps stored somehow.  Originally, we used
 * to just make an array of them...
 *
 *	typedef MetaMap KeyTable[MAX_META];
 *
 * but then again, many of those MetaMaps are going to be totaly empty.
 * Why should we allocate 1k to something that isnt going to be used? 
 * So instead we should keep pointers to the maps and allocate them as
 * neccesary at runtime...
 *
 *	typedef	MetaMap	*KeyTable[MAX_META];	(better)
 *
 * Which is what we had before.  This works out fine, except, that the
 * number of meta maps is hardcoded into the client at compile time.  
 * Wouldn't it be nice to be able to determine at runtime how many maps
 * we want and be able to change them as neccesary?  We can do this by
 * having a pointer to the set of pointers of MetaMaps...
 *
 *	typedef MetaMap **KeyTable;		(dyanmic now)
 *
 * And so we dynamically allocate to MetaSet enough pointers to hold the
 * number of MetaMap's we want.  Then any time we want to use a MetaMap,
 * we allocate space for it.  Then any time we want to use a binding in the
 * MetaMap, we allocate space for it and set its pointer.
 *
 * So the final dereference for a specific key binding 'X' in meta map 'Y' is:
 *
 *	KeyTable keys;
 *	keys[Y]			The pointer to the 'Y'th MetaMap
 *	(*keys[Y])		The MetaMap itself
 *	(*keys[Y])[X]		The pointer to the 'X'th character in the
 *					'Y'th MetaMap
 *	(*keys[Y])[X]->key_index
 *				What 'Y,X' is actually bound to.  This is a
 *				negative value for a transition to a new
 *				meta state Y, positive for a terminating
 *				binding.
 */

/* * * * * * * * * * * * * * * METAMAP MANAGEMENT * * * * * * * * * * * */
/*
 * This is where all the bindings are stored
 * Also we store how big it thinks it is, and how big it actually is.
 */

/* KeyMapNames: the structure of the keymap to realname array */
typedef struct
{
	char	*	name;
	KeyBinding 	func;
}	KeyMapNames;
static KeyMapNames 	key_names[] =
{
	/* The first two here are "magic" binds */
	{ "NOTHING",			NULL 				},
	{ "SELF_INSERT",		input_add_character 		},
	{ "ALTCHARSET",			insert_altcharset		},
	{ "AUTOREPLY",			input_autoreply },
	{ "AUTOREPLY_BACK",		input_autoreplyback },
	{ "BACKSPACE",			input_backspace 		},
	{ "BACKWARD_CHARACTER",		backward_character 		},
	{ "BACKWARD_HISTORY",		backward_history 		},
	{ "BACKWARD_WORD",		input_backward_word 		},
	{ "BEGINNING_OF_LINE",		input_beginning_of_line 	},
	{ "BLINK",			insert_blink 			},
	{ "BOLD",			insert_bold 			},
#ifdef WANT_CDCC
	{ "CDCC_PLIST",			cdcc_plist },
#endif
	{ "CHANNEL_CHOPS",		channel_chops },
	{ "CHANNEL_NONOPS",		channel_nonops },
	{ "CHANGE_TO_SPLIT",		change_to_split }, 
#ifdef WANT_CHELP
	{ "CHELP",			do_chelp },
#endif
	{ "CLEAR_SCREEN",		clear_screen 			},
	{ "COMMAND_COMPLETION",		command_completion 		},
	{ "CPU_SAVER",			cpu_saver_on 			},
	{ "DCC_PLIST",			dcc_plist },
	{ "DCC_STATS",			dcc_ostats },
	{ "DELETE_CHARACTER",		input_delete_character 		},
	{ "DELETE_NEXT_WORD",		input_delete_next_word 		},
	{ "DELETE_PREVIOUS_WORD",	input_delete_previous_word 	},
	{ "DELETE_TO_PREVIOUS_SPACE",	input_delete_to_previous_space 	},
	{ "END_OF_LINE",		input_end_of_line 		},
#ifdef TRANSLATE
	{ "ENTER_DIGRAPH",		enter_digraph },
#endif
	{ "ERASE_LINE",			input_clear_line 		},
	{ "ERASE_TO_BEG_OF_LINE",	input_clear_to_bol 		},
	{ "ERASE_TO_END_OF_LINE",	input_clear_to_eol 		},
	{ "FORWARD_CHARACTER",		forward_character 		},
	{ "FORWARD_HISTORY",		forward_history 		},
	{ "FORWARD_WORD",		input_forward_word 		},
	{ "HIGHLIGHT_OFF",		highlight_off 			},
	{ "IGNORE_NICK",		ignore_last_nick },
	{ "JOIN_LAST_INVITE",		join_last_invite },
	{ "NEW_BEGINNING_OF_LINE",	new_input_beginning_of_line },
	{ "NEW_SCROLL_BACKWARD",	my_scrollback },
	{ "NEW_SCROLL_END",		my_scrollend },
	{ "NEW_SCROLL_FORWARD",		my_scrollforward},
	{ "NEXT_WINDOW",		BX_next_window 			},
	{ "NICK_COMPLETION",		nick_completion },
	{ "PARSE_COMMAND",		parse_text 			},
#ifdef GUI
	{ "PASTE_TO_INPUT",             paste_to_input },
#endif
	{ "PREVIOUS_WINDOW",		BX_previous_window 		},
	{ "QUIT_IRC",			irc_quit 			},
	{ "QUOTE_CHARACTER",		quote_char 			},
	{ "REFRESH_INPUTLINE",		refresh_inputline 		},
	{ "REFRESH_SCREEN",		(KeyBinding) refresh_screen 	},
	{ "REFRESH_STATUS",		(KeyBinding) BX_update_all_status 	},
	{ "REVERSE",			insert_reverse 			},
	{ "SCROLL_BACKWARD",		BX_scrollback_backwards 		},
	{ "SCROLL_END",			BX_scrollback_end 			},
	{ "SCROLL_FORWARD",		BX_scrollback_forwards 		},
	{ "SCROLL_START",		BX_scrollback_start 		},
	{ "SEND_LINE",			send_line 			},
	{ "SHOVE_TO_HISTORY",		shove_to_history 		},
#ifdef ALLOW_STOP_IRC
	{ "STOP_IRC",			term_pause 			},
#endif
	{ "SWAP_LAST_WINDOW",		BX_swap_last_window 		},
	{ "SWAP_NEXT_WINDOW",		BX_swap_next_window 		},
	{ "SWAP_PREVIOUS_WINDOW",	BX_swap_previous_window 		},
	{ "SWITCH_CHANNELS",		switch_channels 		},
#ifdef WANT_TABKEY
	{ "TAB_COMPLETION",		tab_completion },
#endif
	{ "TAB_MSG",			input_msgreply },
	{ "TAB_MSG_BACK",		input_msgreplyback },
	{ "TOGGLE_CLOAK",		toggle_cloak },
	{ "TOGGLE_INSERT_MODE",		toggle_insert_mode 		},
	{ "TOGGLE_STOP_SCREEN",		BX_toggle_stop_screen 		},
	{ "TRANSPOSE_CHARACTERS",	input_transpose_characters 	},
	{ "TYPE_TEXT",			type_text 			},
	{ "UNCLEAR_SCREEN",		input_unclear_screen 		},
	{ "UNDERLINE",			insert_underline 		},
	{ "UNSTOP_ALL_WINDOWS",		BX_unstop_all_windows 		},
	{ "WHOLEFT",			wholeft },
	{ "WINDOW_BALANCE",		window_key_balance },
	{ "WINDOW_GROW_ONE",		window_grow_one },
#ifdef WANT_CHELP
	{ "WINDOW_HELP",		w_help },
#endif
	{ "WINDOW_HIDE",		window_key_hide },
	{ "WINDOW_KILL",		window_key_kill },
	{ "WINDOW_LIST",		window_key_list },
	{ "WINDOW_MOVE",		window_key_move },
	{ "WINDOW_SHRINK_ONE",		window_shrink_one },

	{ "WINDOW_SWAP_1",		window_swap1 },
	{ "WINDOW_SWAP_2",		window_swap2 },
	{ "WINDOW_SWAP_3",		window_swap3 },
	{ "WINDOW_SWAP_4",		window_swap4 },
	{ "WINDOW_SWAP_5",		window_swap5 },
	{ "WINDOW_SWAP_6",		window_swap6 },
	{ "WINDOW_SWAP_7",		window_swap7 },
	{ "WINDOW_SWAP_8",		window_swap8 },
	{ "WINDOW_SWAP_9",		window_swap9 },
	{ "WINDOW_SWAP_10",		window_swap10 },
	{ "YANK_FROM_CUTBUFFER",	input_yank_cut_buffer 		},
	{ "NULL",			NULL 				}
};
#define NUMBER_OF_FUNCTIONS (sizeof(key_names) / sizeof(KeyMapNames)) - 1

/* KeyMap: the structure of the irc keymaps */
typedef struct
{
	int		key_index;
	char *		stuff;
	char *		filename;
	char		changed;
}	KeyMap;
typedef	KeyMap	*	MetaMap[256];
typedef MetaMap **	KeyTable;
static	KeyTable 	keys 		= NULL;
	int		curr_keys_size 	= 0;
static	int		max_keys_size 	= 0;
#define MAX_META 	curr_keys_size - 1

static void 	delete_metamap (int i);

/*
 * resize_metamap -- When we need to increase or decrease the number of
 * metamaps that the system is handling, you call this function with the
 * new size, and everything automagically adjusts from there.   This function
 * always succeeds if it returns.  This function is the callback for the
 * /SET META_STATES action.
 */
void	resize_metamap (int new_size)
{
	int	old_size = curr_keys_size;
	int	i, j;

	/*
	 * Sorry, just too much will break if you go lower than 5.
	 */
	if (new_size < 5)
	{
		say("You can't set META_STATES to less than 5.");
		set_int_var(META_STATES_VAR, 5);
	}

	if (old_size == new_size)
		return;		/* What-EVER */

	/*
	 * If we're growing the meta table, resize and copy the data.
	 */
	if (old_size < new_size)
	{
		/*
		 * Realloc and copy if neccesary
		 */
		if (new_size > max_keys_size)
		{
			KeyTable 	new_keys;
			new_keys = new_malloc(sizeof(KeyTable *) * new_size);

			for (i = 0; i < old_size; i++)
				new_keys[i] = keys[i];
			for (i = old_size; i < new_size; i++)
				new_keys[i] = NULL;
			new_free((void **)&keys);
			keys = new_keys;
			max_keys_size = new_size;
		}
		curr_keys_size = new_size;
	}

	/*
	 * If we're shrinking the meta table, just garbage collect all
	 * the old bindings, dont actually bother resizing the table.
	 */
	else
	{
		for (i = new_size; i < old_size; i++)
			delete_metamap(i);
		curr_keys_size = new_size;

		/*
		 * This is a bit tricky -- There might be meta transitions
		 * in other states that point to the now defunct states.
		 * If we leave those bindings around, then they will point
		 * to either meaningless, or bogus data, and either cause
		 * undefined behavior or a total program crash.  So we walk
		 * all of the remaining states and garbage collect any 
		 * meta transisions that are out of bounds.
		 */
		for (i = 0; i < new_size; i++)
		{
		    if (!keys[i])
			continue;
		    for (j = 0; j < 256; j++)
			if (KEY(i, j) && (KEY(i, j)->key_index <= -new_size))
				snew_key(i, j, NULL);
		}
	}

	set_int_var(META_STATES_VAR, curr_keys_size);
}


/*
 * new_metamap -- When you "touch" a metamap for the first time,
 * the table for the 256 bindings in that metamap must be created, so
 * you call this function to do that.  You must never call this function
 * unless the metamap does not exist, or it will panic.
 */
static void 	new_metamap (int which)
{
	int j;

	if (keys[which])
		ircpanic("metamap already exists");

	keys[which] = new_malloc(sizeof(MetaMap));

	for (j = 0; j <= 255; j++)
		KEY(which, j) = NULL;
}

/*
 * delete_metamap -- When you're all done with a metamap you can call
 * this function to garbage collect it.  If there are any bindings in the
 * metamap when you call this, they will be summarily disposed of.
 */
static void 	delete_metamap (int i)
{
	int j;

	/* This is cheating, but do i care? ;-) */
	for (j = 0; j <= 255; j++)
		snew_key(i, j, NULL);

	new_free((char **)&keys[i]);
}




/* * * * * * * * * * * * * KEY BINDING MANAGEMENT * * * * * * * * * * * * */
/* special interface to new_key for the default key bindings */
static void	snew_key (int meta, unsigned chr, char *what)
{
	int	i;
	int	j;

	if ((j = lookup_function(what, &i)) == 1)
		new_key(meta, chr, i, 0, NULL);
#if 0
	else
		ircpanic("Something bogus passed to snew_key");
#endif
}

static	void	snew_key_from_str (uc *string, char *what)
{
	int	i;
	int	meta;
	int	old_display;
	uc	chr;

	old_display = window_display;
	window_display = 0;
	if ((meta = parse_key(string, &chr)) == -1)
		return;
	window_display = old_display;

	if (lookup_function(what, &i) == 1)
		new_key(meta, chr, i, 0, NULL);

	return;
}


static void 	new_key (int meta, unsigned chr, int type, int change, char *stuff)
{
	/*
	 * Create a map first time we bind into it.  We have to do this
	 * Because its possible to do /bind METAX-f when there is not
	 * otherwise any key bound to METAX.
	 */
	if (!keys)
		return;
	if (!keys[meta])
		new_metamap(meta);

	if (KEY(meta, chr))
	{
		if (KEY(meta, chr)->stuff)
			new_free(&(KEY(meta, chr)->stuff));
		if (KEY(meta, chr)->filename)
			new_free(&(KEY(meta, chr)->filename));
		new_free(&(KEY(meta, chr)));
		KEY(meta, chr) = NULL;
	}

	if (type != 0)
	{
		KEY(meta, chr) = (KeyMap *)new_malloc(sizeof(KeyMap));
		KEY(meta, chr)->key_index = type;
		KEY(meta, chr)->changed = change;
/*		KEY(meta, chr)->filename = m_strdup(current_package());*/
		if (stuff)
			KEY(meta, chr)->stuff = m_strdup(stuff);
		else
			KEY(meta, chr)->stuff = NULL;
	}
}

/*
 * show_binding:  Given an unsigned character 'X' in the meta map 'Y', this
 * function will display to the screen the status of that bindings in a 
 * human-readable way.
 */
static void 	show_binding (int meta, uc c)
{
	char	meta_str[8];

	*meta_str = 0;
	if (meta < 1 || meta > MAX_META)
		meta = 0;
	else
		sprintf(meta_str, "META%d-", meta);

	if (keys[meta] && KEY(meta, c))
	{
#ifdef GUI
		if(meta == MAX_META && c < MAX_MOUSE)
			say("%s is bound to %s %s", mouse_actions[c],
				key_names[(*keys[meta])[c]->key_index].name,
				SAFE(KEY(meta, c)->stuff));
		else if (KEY(meta, c)->key_index < 0)
#else
			if (KEY(meta, c)->key_index < 0)
#endif
			say("%s%s is bound to META%d_CHARACTER",
				meta_str, 
				display_key(c),
				-(KEY(meta, c)->key_index));
		else
			say("%s%s is bound to %s %s", 
				meta_str, 
				display_key(c),
				key_names[KEY(meta, c)->key_index].name, 
				SAFE(KEY(meta, c)->stuff));
	}
	else
		say("%s%s is bound to NOTHING", meta_str, display_key(c));
}

#ifdef GUI
void wm_process(int param)
{
void	(*func) (char, char *) = NULL;
char	*ptr = NULL;

	if (keys[MAX_META] && (*keys[MAX_META]) && (*keys[MAX_META])[param])
	{
		func = key_names[(*keys[MAX_META])[param]->key_index].func;
		ptr = (*keys[MAX_META])[param]->stuff;
	}
	if (func)
		func(param, ptr ? ptr : empty_string);
}
#endif

/*
 * save_bindings: This writes all the key bindings for ircII to the given
 * FILE pointer suitable for being /LOADed again in the future.
 */
void 	save_bindings (FILE *fp, int do_all)
{
	int	meta, j;
	int	charsize = charset_size();
	char	meta_str[10];

	*meta_str = 0;
	for (meta = 0; meta <= MAX_META; meta++)
	{
		if (meta != 0)
			sprintf(meta_str, "META%d-", meta);

		for (j = 0; j < charsize; j++)
		{
			if (keys[meta] && KEY(meta, j) && KEY(meta, j)->changed)
			{
				if (KEY(meta, j)->key_index < 0)
				    fprintf(fp, "BIND %s%s META%d\n", 
					meta_str, 
					display_key(j), 
					-(KEY(meta, j)->key_index));
				else
				    fprintf(fp, "BIND %s%s %s %s\n", 
					meta_str, 
					display_key(j), 
					key_names[KEY(meta, j)->key_index].name,
					SAFE(KEY(meta, j)->stuff));
			}
		}
	}
}

/*
 * This is a function used by edit_char to retreive the details for a
 * specific key binding.  This function provides the only external access
 * to the key bindings.  The arguments are the meta state and the character
 * whose information you want to retreive.  That information is stored into
 * the 'func' and 'name' pointers you pass in.
 *
 * The function will return 0 if the binding you request is a "normal" one.
 *	If the binding is "NOTHING", then func will be set to NULL
 *	If the binding is an action, the func will be set to its callback.
 *
 * The function will return a positive number if the binding you request is
 * a "meta" character.
 *	The value of 'func' will be NULL but you should not depend on that.
 */
int	get_binding (int meta, uc c, KeyBinding *func, char **name)
{
	*func = NULL;
	*name = NULL;

	if (meta >= 0 && meta <= MAX_META)
	{
		if (keys[meta] && KEY(meta, c))
		{
			/*
			 * If this is a meta binding, return the new meta
			 * state -- this is a "special" value.
			 */
			if (KEY(meta, c)->key_index < 0)
				return -(KEY(meta, c)->key_index);

			/*
			 * Otherwise, assign to 'func' and 'name' the
			 * appropriate values.
			 */
			*func = key_names[KEY(meta, c)->key_index].func;
			*name = KEY(meta, c)->stuff;
		}
	}
	return 0;
}


void 	remove_bindings (void)
{
	int i;

	for (i = 0; i <= MAX_META; i++)
		delete_metamap(i);
}

void	unload_bindings (const char *filename)
{
	int 	i, j;

	for (i = 0; i <= MAX_META; i++)
	{
		if (!keys[i])
			continue;

		for (j = 0; j < 256; j++)
			if (KEY(i, j) && !strcmp(KEY(i, j)->filename, filename))
				snew_key(i, j, NULL);
	}
}


static void	show_all_bindings (int meta)
{
	int	i, j, k;

	if (meta == -1)
	{
		for (i = 0; i <= MAX_META; i++)
			show_all_bindings(i);
		return;
	}

	if (meta > MAX_META || !keys[meta])
		return;

	k = charset_size();
	for (j = 0; j < k; j++)
		if (KEY(meta, j) && KEY(meta, j)->key_index != 1)
			show_binding(meta, j);
}

/* * * * * * * * * * * * * * PARSEKEY * * * * * * * * * * * * */
/* parsekeycmd: does the PARSEKEY command.  */
BUILT_IN_COMMAND(parsekeycmd)
{
	int	i;
	char	*arg;

	if ((arg = next_arg(args, &args)) != NULL)
	{
                int	keyval = lookup_function(arg, &i);

		switch (keyval)
		{
			case 0:
				say("No such function %s", arg);
				break;
			case 1:
				if (i < 0)
					last_input_screen->meta_hit = -i;
				else if (key_names[i].func)
					key_names[i].func(0, args);
				break;
			default:
				say("Ambigious function %s", arg);
				break;
		}
	}
}

/* * * * * * * * * * * * * * RBIND * * * * * * * * * * * * * * */
/*
 * rbindcmd:  This is the /RBIND command.  If you give it a bind action,
 * it will show you all of the key bindings that have that action.  You
 * probably cannot lookup the action NOTHING as it is a magic action.
 */
BUILT_IN_COMMAND(rbindcmd)
{
	int	f;
	char	*arg;
	int	i, j;
	int	charsize = charset_size();

	if ((arg = next_arg(args, &args)) == NULL)
		return;		/* No args is a no-op */

	switch (lookup_function(arg, &f))
	{
		case 0:
			say("No such function %s", arg);
			return;

		case 1:
			break;

		default:
			say("Ambigious function %s", arg);
			return;
	}

	for (i = 0; i <= MAX_META; i++)
	{
		if (!keys[i])
			continue;

		for (j = 0; j < charsize; j++)
			if (KEY(i, j) && KEY(i, j)->key_index == f)
				show_binding(i, j);
	}
}


/* * * * * * * * * * * * * * BIND  * * * * * * * * * * * * * */
static int	grok_meta (const uc *ptr, const uc **end)
{
	int		meta = -1;
	const uc *	str;

	/*
	 * Well, if it is going to be anywhere, META has to be out front,
	 * so lets slurp it up if its there.
	 */
	if (!my_strnicmp(ptr, "META", 4))
	{
		str = ptr = ptr + 4;
		while (isdigit(*ptr))
			ptr++;
		if (*ptr == '_' && !my_strnicmp(ptr, "_CHARACTER", 10))
			ptr = ptr + 10;
		if (*ptr == '-')
			ptr++;
		meta = atol(str);
	}

	*end = ptr;
	return meta;
}

/*
 * copy_redux:
 * This converts an ordinary sequence into something more suitable to
 * work with, including the redux of ^X into X-64.
 * You can then work with the sequence after processing.
 */
void	copy_redux (const uc *orig, uc *result)
{
	const  uc	*ptr;
	*result = 0;
	
	for (ptr = orig; ptr && *ptr; ptr++, result++)
	{
		if (*ptr != '^')
		{
			*result = *ptr;
			continue;
		}

		ptr++;
		switch (toupper(*ptr))
		{
			case 0:			/* ^<nul> is ^ */
				*result = '^';
				return;
			case '?':		/* ^? is DEL */
				*result = 0177;
				break;
			default:
				if (toupper(*ptr) < 64)
				{
					say("Illegal key sequence: ^%c", *ptr);
					*result = 0;
					return;
				}
				*result = toupper(*ptr) - 64;
				break;
		}
	}
	*result = 0;
	return;
}

/*
 * find_meta_map: Finds a meta map that does not already contain a 
 * binding to the specified character.
 */
int	find_meta_map	(uc key)
{
	int	curr = MAX_META;

	for (curr = MAX_META; curr > 4; curr--)
	{
		if (!keys[curr])
			return curr;

		if (!KEY(curr, key))
			return curr;
	}

	resize_metamap(curr_keys_size + 1);
	return MAX_META;		/* Well, its empty now */
}

/*
 * Purpose:
 * To make sure that the key sequence X is valid upon return.
 * Composition of X is:  [<key>]*<key>
 * Where <key> is  an ascii char > 32, or a caret followed by an ascii char.
 *
 * First, remove the last character
 * Second, remove the leading part of X that is a valid meta descriptor
 *
 * At this point, we have  <META> + [<unbound-key>]* + <final-key>
 *
 * If unbound-key is present, then we have to bind it.  The first thing to
 * do is find a place where <final-key> can be stashed.  Look for the highest
 * metamap that has an open spot for <final-key>
 *
 * Now we have   <META> + [<unbound-key>]* + <unbound-key> + <final-key>
 * And we now know what meta map the first three parts have to conclude to.
 * So that is the return value.
 *
 * Now we need to build up to that.  Repeat this process until there is
 * nothing left in the unbound-key segment.
 */
/*
 * A lot of magic goes on in this function.  The general purpose of this
 * function is to take a "key-description" of any form, and canonicalize
 * it down into a resulting meta map (which is the return value), and return
 * the final character in 'term'.  Older algorithms only allowed you to 
 * specify META(X)-(Y), where X is the meta value to be returned and Y is
 * the character, and anything else was an error.  Now we allow you to specify
 * any arbitrary string -- if the leading part of the string is already bound
 * to a META key, then we can deal with that.  If the leading part of the
 * string is NOT bound to anything in particular, then we will bind it FOR
 * you, and the resulting meta state is returned.  This allows things like
 * this to work:
 *
 *	/BIND META2-C	BIND-ACTION	(Specify a meta map directly)
 *	/BIND ^[[A	BIND-ACTION	(^[[ is bound to META2 by default)
 *	/BIND ^[[11~	BIND-ACTION	(Force us to make suer ^[[11 is bound
 *					 to a meta map before returning.)
 */
static int	parse_key (const uc *sequence, uc *term)
{
	uc	*copy;
	uc	*end;
	int	return_meta = 0;
	int	meta;
	uc	last_character;
	uc	terminal_character;
	int	last;
	int	somethingN;
#ifdef GUI
	int mouse;
#endif

	/*
	 * Make a local copy of the string to be bound.  Redux all of
	 * the ^x modifers to their literal control characters.
	 */
	copy = alloca(strlen(sequence) + 4);
	copy_redux(sequence, copy);
	end = copy + strlen(copy) - 1;

#ifdef GUI
	for( mouse = 0; mouse < MAX_MOUSE; mouse++)
	{
		if (!my_strnicmp(sequence, mouse_actions[mouse], strlen(mouse_actions[mouse])))
		{
			*term=(char)mouse;
			return MAX_META;
		}
	}
#endif

	if (x_debug & DEBUG_AUTOKEY)
		yell("Starting with COPY := [%s]", copy);
	/*
	 * Remove any leading META description
	 */
	if ((meta = grok_meta(copy, (const uc **)&copy)) == -1)
		meta = 0;

	if (x_debug & DEBUG_AUTOKEY)
		yell("After META grokked, COPY := [%s]", copy);

	/*
	 * Remove any leading characters that also comprise a META
	 * description
	 */
	while (copy[0] && copy[1])
	{
		if (keys[meta] && KEY(meta, *copy) &&
				  KEY(meta, *copy)->key_index < 0)
		{
			meta = -(KEY(meta, *copy)->key_index);
			copy++;
			if (x_debug & DEBUG_AUTOKEY)
			{
				yell("First character of COPY switches to meta [%d]", meta);
				yell("After META grokked, COPY := [%s]", copy);
			}
			continue;
		}
		break;
	}

	if (x_debug & DEBUG_AUTOKEY)
		yell("After ALL META grokked, COPY := [%s]", copy);

	/*
	 * Check to see if the entire sequence was just a meta modifier
	 * or if it is a META-KEY modifier.  Either way, we're done.
	 */
	if (!copy[0] || !copy[1])
	{
		*term = copy[0];
		return meta;
	}

	/*
	 * Right now the input boils down to this:
	 *
	 * input := SOME_CHARACTERS + TERMINAL_CHARACTER + LAST_CHARACTER
	 * SOME_CHARACTERS 	:= <key>*
	 * TERMINAL_CHARACTER 	:= <key>
	 * LAST_CHARACTER 	:= <key>
	 *
	 * The previous check assures that 'terminal character' is not
	 * an empty value at this point.
	 */
	last_character = *end;
	*end-- = 0;
	terminal_character = *end;
	*end-- = 0;

	if (x_debug & DEBUG_AUTOKEY)
	{
		yell("Starting to work on the string:");
		yell("SOME_CHARACTERS := [%s] (%d)", copy, strlen(copy));
		yell("TERMINAL_CHARACTER := [%c]", terminal_character);
		yell("LAST_CHARACTER := [%c]", last_character);
	}


	/*
	 * Our ultimate goal is to return when the operation:
	 *	/bind META<something>-LAST_CHARACTER  <binding>
	 * will succeed.  So we need to find a place to put LAST_CHARACTER.
	 */
	last = return_meta = find_meta_map(last_character);
	if (x_debug & DEBUG_AUTOKEY)
	{
		yell("FIND_META_MAP says we can put [%c] in META [%d]", 
				last_character, return_meta);
	}

	/*
	 * So now we need to work backwards through the string linking
	 * each of the characters to the next one.  Starting with
	 * TERMINAL_CHARACTER, we find a meta map where that can be linked
	 * from (that map is somethingN1).  We then do:
	 *
	 *	/bind META<somethingN1>-TERMINAL_CHARACTER META<LAST>
	 *
	 * Where 'last' is the most previous meta map we linked to, starting
	 * with 'something'.
	 */
	while (*copy)
	{
		if (x_debug & DEBUG_AUTOKEY)
		{
			yell("COPY: [%s] (%d)", copy, strlen(copy));
			yell("Now we are going to bind the [%c] character to meta [%d] somehow.",
					terminal_character, last);
		}

		/*
		 * <something> is any meta map such that:
		 * /bind META<somethingN>-[TERMINAL CHARACTER] META<something>
		 */
		somethingN = find_meta_map(terminal_character);
		if (x_debug & DEBUG_AUTOKEY)
			yell("FIND_META_MAP says we can do this in META [%d]", somethingN);

		new_key(somethingN, terminal_character, -last, 1, NULL);
		show_binding(somethingN, terminal_character);

		/*
		 * Now we walk backwards in the string:  'last' now becomes
	 	 * the meta map we just linked, and we pop TERMINAL_CHARACTER
		 * off the end of SOME_CHARACTERS.  We repeat this until
		 * SOME_CHARACTERS is empty.
		 */
		last = somethingN;
		terminal_character = *end;
		*end-- = 0;
	}

	/*
	 * Make the final link from the initial meta state to our newly
	 * constructed chain...
	 */
	new_key(meta, terminal_character, -last, 1, NULL);
	show_binding(meta, terminal_character);

	/*
	 * Return the interesting information
	 */
	*term = last_character;
	return return_meta;

#if 0
	/* The rest of this isnt finished, hense is unsupported */
	say("The bind cannot occur because the character sequence to bind contains a leading substring that is bound to something else.");
	return -1;
#endif
}

/*
 * bindcmd:  The /BIND command.  The general syntax is:
 *
 *	/BIND ([key-descr] ([bind-command] ([args])))
 * Where:
 *	KEY-DESCR    := ([^]C | META[num])
 *	BIND-COMMAND := <Any string in the key_names lookup table>
 *
 * If given no arguments, this command shows all non-empty bindings
 * current registered.
 *
 * If given one argument, that argument is to be a description of a valid
 * key sequence.  The command will show the binding of that sequence.
 *
 * If given two arguments, the first argument is to be a description of a
 * valid key sequence and the second argument is to be a valid binding
 * command followed by any optionally appropriate arguments.  The key
 * sequence is then bound to that action.
 *
 * The special binding command "NOTHING" actually unbinds the key.
 */
BUILT_IN_COMMAND(bindcmd)
{
	uc	*key,
		*function;
	uc	*newkey;
	int	meta;
	uc	dakey;
	int	bi_index;
	int	cnt,
		i;

	/*
	 * See if they specified a key argument.  If they didnt, show all
	 * binds and return
	 */
	if ((key = new_next_arg(args, &args)) == NULL)
	{
		show_all_bindings(-1);
		return;
	}

	/*
	 * Grok any flags (only one, for now)
	 */
	if (*key == '-')
	{
		if (!my_strnicmp(key + 1, "DEFAULTS", 1))
		{
			init_keys();
			init_keys2();
		}
		return;
	}


	/*
	 * Grok the key argument and see what we can make of it
	 * If there is an error at this point, dont continue.
	 * Most of the work is done here.
	 */
	
	newkey = get_term_capability(key, 0, 1);
	if ((meta = parse_key(newkey ? newkey : key, &dakey)) == -1)
		if (!newkey || (parse_key(key, &dakey) == -1))
			return;

	/*
	 * See if they specified an action argument.  If they didnt, then
	 * check to see if they specified /bind METAX or if they specified
	 * /bind <char sequence>, and output as is appropriate.
	 */
	if ((function = next_arg(args, &args)) == NULL)
	{
		/* They did /bind ^C */
		if (dakey)
			show_binding(meta, dakey);

		/* They did /bind meta2 */
		else
			show_all_bindings(meta);

		return;
	}

	/*
	 * Look up the action they want to take.  If it is invalid, tell
	 * them so, if it is ambiguous, show the possible choices, and if
	 * if it valid, then actually do the bind action.  Note that if we
	 * do the bind, we do a show() so the user knows we took the action.
	 */
	switch ((cnt = lookup_function(function, &bi_index)))
	{
		case 0:
			say("No such function: %s", function);
			break;
		case 1:
			if (meta < 1 || meta > MAX_META)
				meta = 0;
			new_key(meta, dakey, bi_index, 1, *args ? args : NULL);
			show_binding(meta, dakey);
			break;
		default:
			say("Ambiguous function name: %s", function);
			for (i = 0; i < cnt; i++, bi_index++)
				put_it("%s", key_names[bi_index].name);
			break;
	}
}

/* * * * * * * * * * * BINDING ACTIONS  * * * * * * * * * */
/* I hate typedefs... */

/*
 * lookup_function:  When you want to convert a "binding" name (such as
 * BACKSPACE or SELF_INSERT) over to its offset in the binding lookup table,
 * you must call this function to retreive that offset.  The first argument
 * is the name you want to look up, and the second argument is where the
 * offset is to be stored.  
 *
 * Return value: (its tricky)
 *	-1	  -- The name is a META binding that is invalid.
 *	Zero	  -- The name is not a valid binding name.
 *	One	  -- The name is a valid, unambiguous binding name.
 *		     If it is a META binding, lf_index will be negative,
 *		     Otherwise, lf_index will be positive.
 *	Other	  -- The name is an ambiguous (therefore invalid) binding name.
 *
 * In the case of a return value of any positive value, "lf_index" will be
 * set to the first item that matches the 'name'.  For all other return
 * values, "lf_index" will have the value -1.
 */
static int 	lookup_function (const uc *orig_name, int *lf_index)
{
	int	len,
		cnt,
		i;
	uc	*name, *breakage;

	if (!orig_name)
	{
		*lf_index = 0;
		return 1;
	}

	breakage = name = LOCAL_COPY(orig_name);
	upper(name);
	len = strlen(name);

	*lf_index = -1;

	/* Handle "META" descriptions especially. */
	if (!strncmp(name, "META", 4))
	{
		const uc *	endp;
		int		meta;

		if ((meta = grok_meta(name, &endp)) < 0)
			return meta;
		else
		{
			*lf_index = -meta;
			return 1;
		}
	}

	for (cnt = 0, i = 0; i < NUMBER_OF_FUNCTIONS; i++)
	{
		if (strncmp(name, key_names[i].name, len) == 0)
		{
			cnt++;
			if (*lf_index == -1)
				*lf_index = i;
		}
	}
	if (*lf_index == -1)
		return 0;
	if (strcmp(name, key_names[*lf_index].name) == 0)
		return 1;
	else
		return cnt;
}


/* I dont know where this belongs. */
/*
 * display_key: Given a (possibly unprintable) unsigned character 'c', 
 * convert that character into a printable string.  For characters less
 * than 32, and the character 127, they will be converted into the "control"
 * sequence by having a prepended caret ('^').  Other characters will be
 * left alone.  The return value belongs to the function -- dont mangle it.
 */
static uc *	display_key (uc c)
{
	static	uc key[3];

	key[2] = (char) 0;
	if (c < 32)
	{
		key[0] = '^';
		key[1] = c + 64;
	}
	else if (c == '\177')
	{
		key[0] = '^';
		key[1] = '?';
	}
	else
	{
		key[0] = c;
		key[1] = (char) 0;
	}
	return (key);
}

char *convert_to_keystr(char *key)
{
int ret, loc;
static char keyloc[80];
	ret = lookup_function(key, &loc);
	*keyloc = 0;
	if  (ret == 1)
	{
		char meta_str[8];
		int i, j;
		int charsize = charset_size();
		*meta_str = 0;

		for (i = 0; i <= MAX_META; i++)
		{
			if (!keys[i])
				continue;
			for (j = 0; j < charsize; j++)
				if (KEY(i, j) && KEY(i, j)->key_index == loc)
				{
					if (i > 0)
						sprintf(meta_str, "META%d-", i);
					sprintf(keyloc, "%s%s", meta_str, display_key(j));
					return keyloc;  
				}
		}
	}
	return keyloc;
}

/* * * * * * * * * * * * * * * * * * INITIALIZATION * * * * * * * * * * * */
/* 
 * This is where you put all the default key bindings.  This is a lot
 * simpler, just defining those you need, instead of all of them, isnt
 * it?  And it takes up so much less memory, too...
 */
void 	init_keys (void)
{
	int i;

	/*
	 * Make sure the meta map is big enough to hold all these bindings.
	 */
	remove_bindings();
	resize_metamap(40);	/* Whatever. */

	/* 
	 * All the "default" bindings are self_insert unless we bind
	 * them differently
	 */
	for (i = 1; i <= 255; i++)
		snew_key(0, i, "SELF_INSERT");

	/* "default" characters that arent self_insert */
	snew_key(0,  1, "BEGINNING_OF_LINE");		/* ^A */
	snew_key(0,  2, "BOLD");			/* ^B */
	snew_key(0,  4, "DELETE_CHARACTER");		/* ^D */
	snew_key(0,  5, "CHANGE_TO_SPLIT");		/* ^E */
	snew_key(0,  6, "WHOLEFT");			/* ^F */
	snew_key(0,  8, "BACKSPACE");			/* ^H (delete) */

	snew_key(0,  9, "TAB_COMPLETION");		/* ^I (tab) */
	snew_key(0, 10, "SEND_LINE");			/* ^J (enter) */
	snew_key(0, 11, "JOIN_LAST_INVITE");		/* ^K */
	snew_key(0, 12, "REFRESH_SCREEN");		/* ^L (linefeed) */
	snew_key(0, 13, "SEND_LINE");			/* ^M (return) */
	snew_key(0, 14, "QUOTE_CHARACTER");		/* ^N */
	snew_key(0, 15, "IGNORE_NICK");			/* ^O */
	snew_key(0, 16, "BACKWARD_HISTORY");		/* ^P */

	snew_key(0, 17, "QUOTE_CHARACTER");		/* ^Q */
	snew_key(0, 18, "NICK_COMPLETION");		/* ^R */
	snew_key(0, 19, "TOGGLE_STOP_SCREEN");		/* ^S */
	snew_key(0, 20, "TRANSPOSE_CHARACTERS");	/* ^T */
	snew_key(0, 21, "ERASE_LINE");			/* ^U */
	snew_key(0, 22, "REVERSE");			/* ^V */
	snew_key(0, 23, "META2_CHARACTER");		/* ^W */
	snew_key(0, 24, "SWITCH_CHANNELS");		/* ^X */

	snew_key(0, 25, "YANK_FROM_CUTBUFFER");		/* ^Y */
#ifdef ALLOW_STOP_IRC
#ifndef PUBLIC_ACCESS
	snew_key(0, 26, "STOP_IRC");			/* ^Z */
#endif
#endif
	snew_key(0, 27, "META1_CHARACTER");		/* ^[ (escape) */
	snew_key(0, 29, "AUTOREPLY");			/* ^] */
	snew_key(0, 31, "UNDERLINE");			/* ^_ */

	snew_key(0, 127, "BACKSPACE");			/* ^? (delete) */

	/* 
	 * european keyboards (and probably others) use the eigth bit
	 * for extended characters.  Having these keys bound by default
	 * causes them lots of grief, so unless you really want to use
	 * these, they are commented out.
	 */
#ifdef EMACS_KEYBINDS
	snew_key(0, 188, "SCROLL_START");
	snew_key(0, 190, "SCROLL_END");
	snew_key(0, 226, "BACKWARD_WORD");
	snew_key(0, 228, "DELETE_NEXT_WORD");
	snew_key(0, 229, "SCROLL_END");
	snew_key(0, 230, "FORWARD_WORD");
	snew_key(0, 232, "DELETE_PREVIOUS_WORD");
	snew_key(0, 255, "DELETE_PREVIOUS_WORD");
#endif

	/* meta 1 characters */
	snew_key(1,  27, "COMMAND_COMPLETION");
	snew_key(1,  46, "CLEAR_SCREEN");
	snew_key(1,  60, "SCROLL_START");
	snew_key(1,  62, "SCROLL_END");
	snew_key(1,  79, "META2_CHARACTER");
	snew_key(1,  91, "META2_CHARACTER");
	snew_key(1,  98, "BACKWARD_WORD");
	snew_key(1, 100, "DELETE_NEXT_WORD");
	snew_key(1, 101, "SCROLL_END");
	snew_key(1, 102, "FORWARD_WORD");
	snew_key(1, 104, "DELETE_PREVIOUS_WORD");
	snew_key(1, 110, "SCROLL_FORWARD");
	snew_key(1, 112, "SCROLL_BACKWARD");
	snew_key(1, 127, "DELETE_PREVIOUS_WORD");

	snew_key(1, '1', "WINDOW_SWAP_1");
	snew_key(1, '2', "WINDOW_SWAP_2");
	snew_key(1, '3', "WINDOW_SWAP_3");
	snew_key(1, '4', "WINDOW_SWAP_4");
	snew_key(1, '5', "WINDOW_SWAP_5");
	snew_key(1, '6', "WINDOW_SWAP_6");
	snew_key(1, '7', "WINDOW_SWAP_7");
	snew_key(1, '8', "WINDOW_SWAP_8");
	snew_key(1, '9', "WINDOW_SWAP_9");
	snew_key(1, '0', "WINDOW_SWAP_10");

	/* meta 2 characters */
#ifdef ALLOW_STOP_IRC
#ifndef PUBLIC_ACCESS
	snew_key(2,  26, "STOP_IRC");
#endif
#endif

	snew_key(2, 110, "SWAP_NEXT_WINDOW");
	snew_key(2, 112, "PREVIOUS_WINDOW");

#ifdef __EMX__
	/* meta 3 characters */
	snew_key(2, '[', "META3_CHARACTER");
	snew_key(2, '1', "META3_CHARACTER");

    snew_key(1, 71, "NEW_BEGINNING_OF_LINE");
    snew_key(1, 83, "TOGGLE_CLOAK");
    snew_key(1, 79, "NEW_SCROLL_END");
    snew_key(1, 73, "NEW_SCROLL_BACKWARD");
    snew_key(1, 81, "NEW_SCROLL_FORWARD");

	snew_key(2, '?', "WINDOW_HELP");
	snew_key(2, '+', "WINDOW_GROW_ONE");
	snew_key(2, '-', "WINDOW_SHRINK_ONE");
	snew_key(2, 'm', "WINDOW_MOVE");
	snew_key(2, 'l', "WINDOW_LIST");
	snew_key(2, 'k', "WINDOW_KILL");
	snew_key(2, 'b', "WINDOW_BALANCE");
	snew_key(2, 'h', "WINDOW_HIDE");
		
	snew_key(2, '[', "META3_CHARACTER");
	snew_key(2, '1', "META3_CHARACTER");

#else /* __EMX__ */
	snew_key(2, '?', "WINDOW_HELP");
	snew_key(2, '+', "WINDOW_GROW_ONE");
	snew_key(2, '-', "WINDOW_SHRINK_ONE");
	snew_key(2, 'm', "WINDOW_MOVE");
	snew_key(2, 'l', "WINDOW_LIST");
	snew_key(2, 'k', "WINDOW_KILL");
	snew_key(2, 'b', "WINDOW_BALANCE");
	snew_key(2, 'h', "WINDOW_HIDE");
	snew_key(2, '[', "META3_CHARACTER");
	snew_key(2,  70, "SCROLL_START");	/* Freebsd home */
	snew_key(2,  71, "SCROLL_FORWARD");	/* Freebsd pgdown */
	snew_key(2,  72, "SCROLL_END");		/* Freebsd end */
	snew_key(2,  73, "SCROLL_BACKWARD");	/* Freebsd pgup */
	snew_key(2, '1', "META32_CHARACTER");
	snew_key(2, '4', "META33_CHARACTER");
	snew_key(2, '5', "META30_CHARACTER");
	snew_key(2, '6', "META31_CHARACTER");
#endif

	/* meta 4 characters -- vi key mappings */
	snew_key(4,   8, "BACKWARD_CHARACTER");
	snew_key(4,  32, "FORWARD_CHARACTER");
	snew_key(4,  65, "META4");
	snew_key(4,  72, "BACKWARD_CHARACTER");
	snew_key(4,  73, "META4");
	snew_key(4,  74, "FORWARD_HISTORY");
	snew_key(4,  75, "BACKWARD_HISTORY");
	snew_key(4,  76, "FORWARD_CHARACTER");
	snew_key(4,  88, "DELETE_CHARACTER");
	snew_key(4,  97, "META4");
	snew_key(4, 104, "BACKWARD_CHARACTER");
	snew_key(4, 105, "META4");
	snew_key(4, 106, "FORWARD_HISTORY");
	snew_key(4, 107, "BACKWARD_HISTORY");
	snew_key(4, 108, "FORWARD_CHARACTER");
	snew_key(4, 120, "DELETE_CHARACTER");

	/* I used 30-something to keep them out of others' way */
	snew_key(30, '~', "SCROLL_BACKWARD");
	snew_key(31, '~', "SCROLL_FORWARD");
	snew_key(32, '~', "SCROLL_START");
	snew_key(33, '~', "SCROLL_END");
}

#define LKEY(x, y) \
{									\
	char *l = get_term_capability(#x, 0, 1);			\
	if (l)								\
		snew_key_from_str(l, #y);				\
}

void	init_keys2 (void)
{
	/* keys bound from terminfo/termcap */
	LKEY(key_up, BACKWARD_HISTORY)
	LKEY(key_down, FORWARD_HISTORY)
	LKEY(key_left, BACKWARD_CHARACTER)
	LKEY(key_right, FORWARD_CHARACTER)
	LKEY(key_ppage, SCROLL_BACKWARD)
	LKEY(key_npage, SCROLL_FORWARD)
	LKEY(key_home, SCROLL_START)
	LKEY(key_end, SCROLL_END)
	LKEY(key_ic, TOGGLE_INSERT_MODE)
	/*LKEY(key_dc, DELETE_CHARACTER)*/
	LKEY(key_f1, CHELP)
	LKEY(key_f2, CHANNEL_CHOPS)
	LKEY(key_f3, CHANNEL_NONOPS)
#ifdef WANT_CDCC
	LKEY(key_f4, CDCC_PLIST)
#endif
	LKEY(key_f5, DCC_PLIST)
	LKEY(key_f6, DCC_STATS)
	LKEY(key_dc, TOGGLE_CLOAK)
}


void disable_stop(void)
{
	snew_key(0, 26, "SELF_INSERT");
}
