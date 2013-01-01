#ifndef _MODVAL_

/* include this so we have the enum table just in case someone forgets. */

#include "module.h"

/* 
 * this is a method first used in eggdrop modules.. 
 * A global table of functions is passed into the init function of module,
 * which is then assigned to the value global. This table is then indexed,
 * to access the various functions. What this means to us, is that we no 
 * longer require -rdynamic on the LDFLAGS line, which reduces the size 
 * of the client. This also makes this less compiler/environment dependant,
 * allowing modules to work on more platforms.
 * A Function_ptr *global is required in the module. The second arg to 
 * the init function is used to initialize this table. The table itself is
 * initialized in modules.c. This file should only be included once in the
 * module and also should be the last file included. It should never be 
 * included in the source itself. 
 * As long as we add new functions to the END of the list in module.h then
 * currently compiled modules will continue to function fine. If we change
 * the order of the list however, then BAD things will occur.
 * Copyright Colten Edwards July 1998.
 */

#define _MODVAL_
#ifndef BUILT_IN_DLL
#define BUILT_IN_DLL(x) \
	void x (IrcCommandDll *intp, char *command, char *args, char *subargs, char *helparg)
#endif

#if defined(WTERM_C) || defined(STERM_C)
/* If we are building wserv or scr-bx we can't use
 * the global table so we forward to the actual
 * functions instead of throught the global table.
 */
#define set_non_blocking(x) BX_set_non_blocking(x)
#define set_blocking(x) BX_set_blocking(x)
#define ip_to_host(x) BX_ip_to_host(x)
#define host_to_ip(x) BX_host_to_ip(x)
#define connect_by_number(a, b, c, d, e) BX_connect_by_number(a, b, c, d, e)
#ifndef __vars_h_
enum VAR_TYPES { unused };
#endif
int  get_int_var (enum VAR_TYPES);
void ircpanic (char *, ...);
char *my_ltoa (long);
#else

/*
 * need to undefine these particular defines. Otherwise we can't include
 * them in the table.
 */
#undef new_malloc
#undef new_free
#undef RESIZE
#undef malloc_strcat
#undef malloc_strcpy
#undef m_strdup
#undef m_strcat_ues
#undef m_strndup

#undef MODULENAME

#ifdef MAIN_SOURCE
void init_global_functions(void);
#ifndef __modules_c
extern char *_modname_;
extern Function_ptr *global;
#endif
#else

#ifdef INIT_MODULE
/* only in the first c file do we #define INIT_MODULE */
char *_modname_ = NULL;
Function_ptr *global = NULL;
#undef INIT_MODULE
#else
extern char *_modname_;
extern Function_ptr *global;
#endif

#endif /* MAIN_SOURCE */

#define MODULENAME _modname_

#define check_module_version (*(int (*)(unsigned long))global[MODULE_VERSION_CHECK])
#define set_dll_name(x) malloc_strcpy(&_modname_, x)
#define set_global_func(x) global = x;
#define initialize_module(x) { \
		global = global_table; \
		malloc_strcpy(&_modname_, x); \
		if (!check_module_version(MODULE_VERSION)) \
			return INVALID_MODVERSION; \
}

	
#ifndef MAIN_SOURCE
#define empty_string ""
#define space " "
#endif

#ifndef HAVE_VSNPRINTF
#define vsnprintf ((char * (*)())global[VSNPRINTF])
#endif
#ifndef HAVE_SNPRINTF
#define snprintf ((char * (*)())global[SNPRINTF])
#endif

/* Changed these to cast the function pointer rather than the arguments and result.  The old method wasn't portable to
 * some 64 bit platforms.  Next step - change instances of global[XYZ] to &xyzfunc in the code.
 *
 * Should also think about standardising types (eg Screen instead of struct ScreenStru, get rid of u_char).
 */
/* ircaux.c */
#define new_malloc(x) ((void * (*)(size_t, const char *, const char *, int))global[NEW_MALLOC])((x),MODULENAME, __FILE__,__LINE__)
#define new_free(x) (*(x) = ((void * (*)(void *, const char *, const char *, int))global[NEW_FREE])(*(x),MODULENAME, __FILE__,__LINE__))
#define RESIZE(x, y, z) ((x) = ((void * (*)(void *, size_t, const char *, const char *, int))global[NEW_REALLOC])((x), sizeof(y) * (z), MODULENAME, __FILE__, __LINE__))
#define malloc_strcpy(x, y) ((char * (*)(char **, const char *, const char *, const char *, int))global[MALLOC_STRCPY])((x), (y), MODULENAME, __FILE__, __LINE__)
#define malloc_strcat(x, y) ((char * (*)(char **, const char *, const char *, const char *, int))global[MALLOC_STRCAT])((x), (y), MODULENAME, __FILE__, __LINE__)
#define malloc_str2cpy (*(char * (*)(char **, const char *, const char *))global[MALLOC_STR2CPY])
#define m_3dup (*(char * (*)(const char *, const char *, const char *))global[M_3DUP])
#define m_opendup (*(char * (*)(const char *, ...))global[M_OPENDUP])
#define m_s3cat (*(char * (*)(char **, const char *, const char *))global[M_S3CAT])
#define m_s3cat_s (*(char * (*)(char **, const char *, const char *))global[M_S3CAT_S])
#define m_3cat (*(char * (*)(char **, const char *, const char *))global[M_3CAT])
#define m_2dup (*(char * (*)(const char *, const char *))global[M_2DUP])
#define m_e3cat (*(char * (*)(char **, const char *, const char *))global[M_E3CAT])

#define my_stricmp (*(int (*)(const char *, const char *))global[MY_STRICMP])
#define my_strnicmp (*(int (*)(const char *, const char *, size_t))global[MY_STRNICMP])

#define my_strnstr (*(int (*)(const unsigned char *, const unsigned char *, size_t))global[MY_STRNSTR])
#define chop (*(char * (*)(char *, int))global[CHOP])
#define strmcpy (*(char * (*)(char *, const char *, int))global[STRMCPY])
#define strmcat (*(char * (*)(char *, const char *, int))global[STRMCAT])
#define scanstr (*(int (*)(char *, char *))global[SCANSTR])
#define m_dupchar (*(char * (*)(int))global[M_DUPCHAR])
#define streq (*(size_t (*)(const char *, const char *))global[STREQ])
#define strieq (*(size_t (*)(const char *, const char *))global[STRIEQ])
#define strmopencat (*(char * (*)(char *, int , ...))global[STRMOPENCAT])
#define ov_strcpy (*(char * (*)(char *, const char *))global[OV_STRCPY])
#define upper (*(char * (*)(char *))global[UPPER])
#define lower (*(char * (*)(char *))global[LOWER])
#define stristr (*(char * (*)(const char *, const char *))global[STRISTR])
#define rstristr (*(char * (*)(char *, char *))global[RSTRISTR])
#define word_count (*(int (*)(char *))global[WORD_COUNT])
#define remove_trailing_spaces (*(char * (*)(char *))global[REMOVE_TRAILING_SPACES])
#define expand_twiddle (*(char * (*)(char *))global[EXPAND_TWIDDLE])
#define check_nickname (*(char * (*)(char *))global[CHECK_NICKNAME])
#define sindex (*(char * (*)(char *, char *))global[SINDEX])
#define rsindex (*(char * (*)(char *, char *, char *, int))global[RSINDEX])
#define is_number (*(int (*)(const char *))global[ISNUMBER])
#define rfgets (*(char * (*)(char *, int , FILE *))global[RFGETS])
#define path_search (*(char * (*)(char *, char *))global[PATH_SEARCH])
#define double_quote (*(char   * (*)(const char *, const char *, char *))global[DOUBLE_QUOTE])
#define ircpanic (*(void (*)(char *, ...))global[IRCPANIC])
#define end_strcmp (*(int (*)(const char *, const char *, int))global[END_STRCMP])
#define beep_em (*(void (*)(int))global[BEEP_EM])
#define uzfopen (*(FILE * (*)(char **, char *, int))global[UZFOPEN])
#define get_time (*(struct timeval (*)(struct timeval *))global[FUNC_GET_TIME])
#define time_diff (*(double (*)(struct timeval , struct timeval))global[TIME_DIFF])
#define time_to_next_minute (*(int (*)(void))global[TIME_TO_NEXT_MINUTE])
#define plural (*(char * (*)(int))global[PLURAL])
#define my_ctime (*(char * (*)(time_t))global[MY_CTIME])
#define ccspan (*(size_t (*)(const char *, int))global[CCSPAN])

/* If we are in a module, undefine the previous define from ltoa to my_ltoa */
#ifdef ltoa
#undef ltoa
#endif
#define ltoa (*(char *(*)(long ))global[LTOA])
#define strformat (*(char *(*)(char *, const char *, int , char ))global[STRFORMAT])
#define MatchingBracket (*(char *(*)(char *, char , char ))global[MATCHINGBRACKET])
#define parse_number (*(int (*)(char **))global[PARSE_NUMBER])
#define splitw (*(int (*)(char *, char ***))global[SPLITW])
#define unsplitw (*(char *(*)(char ***, int ))global[UNSPLITW])
#define check_val (*(int (*)(char *))global[CHECK_VAL])
#define on_off (*(char *(*)(int ))global[ON_OFF])
#define strextend (*(char *(*)(char *, char , int ))global[STREXTEND])
#define strfill (*(const char *(*)(char , int ))global[STRFILL])
#define empty (*(int (*)(const char *))global[EMPTY_FUNC])
#define remove_brackets (*(char *(*)(const char *, const char *, int *))global[REMOVE_BRACKETS])
#define my_atol (*(long (*)(const char *))global[MY_ATOL])
#define strip_control (*(void (*)(const char *, char *))global[STRIP_CONTROL])
#define figure_out_address (*(int (*)(char *, char **, char **, char **, char **, int *))global[FIGURE_OUT_ADDRESS])
#define strnrchr (*(char *(*)(char *, char , int ))global[STRNRCHR])
#define mask_digits (*(void (*)(char **))global[MASK_DIGITS])
#define ccscpan (*(size_t (*)(const char *, int))global[CCSPAN])
#define charcount (*(int (*)(const char *, char ))global[CHARCOUNT])
#define strpcat (*(char *(*)(char *, const char *, ...))global[STRPCAT])
#define strcpy_nocolorcodes (*(u_char *(*)(u_char *, const u_char *))global[STRCPY_NOCOLORCODES])
#define cryptit (*(char *(*)(const char *))global[CRYPTIT])
#define stripdev (*(char *(*)(char *))global[STRIPDEV])
#define mangle_line (*(size_t (*)(char *, int, size_t))global[MANGLE_LINE])
#define m_strdup(x) (*(char *(*)(const char *, const char *, const char *, const int ))global[M_STRDUP])((x), MODULENAME, __FILE__, __LINE__)
#define m_strcat_ues(x, y, z) (*(char *(*)(char **, char *, int , const char *, const char *, const int ))global[M_STRCAT_UES])((x), (y), (z), MODULENAME, __FILE__, __LINE__)
#define m_strndup(x, y) (*(char *(*)(const char *, size_t, const char *, const char *, const int ))global[M_STRNDUP])((x), (y), MODULENAME, __FILE__, __LINE__)
#define malloc_sprintf (*(char *(*)(char **, const char *, ...))global[MALLOC_SPRINTF])
#define m_sprintf (*(char *(*)(const char *, ...))global[M_SPRINTF])
#define next_arg (*(char *(*)(char *, char **))global[NEXT_ARG])
#define new_next_arg (*(char *(*)(char *, char **))global[NEW_NEXT_ARG])
#define new_new_next_arg (*(char *(*)(char *, char **, char *))global[NEW_NEW_NEXT_ARG])
#define last_arg (*(char *(*)(char **))global[LAST_ARG])
#define next_in_comma_list (*(char *(*)(char *, char **))global[NEXT_IN_COMMA_LIST])
#define random_number (*(u_long (*)(u_long))global[RANDOM_NUMBER])


/* words.c reg.c */
#define strsearch (*(char *(*)(char *, char *, char *, int ))global[STRSEARCH])
#define move_to_abs_word (*(char *(*)(const char *, char **, int ))global[MOVE_TO_ABS_WORD])
#define move_word_rel (*(char *(*)(const char *, char **, int ))global[MOVE_WORD_REL])
#define extract (*(char *(*)(char *, int , int ))global[EXTRACT])
#define extract2 (*(char *(*)(const char *, int , int ))global[EXTRACT2])
#define wild_match (*(int (*)(const char *, const char *))global[WILD_MATCH])

/* network.c */
#define connect_by_number (*(int (*)(char *, unsigned short *, int , int , int ))global[CONNECT_BY_NUMBER])
#define lookup_host (*(struct sockaddr_foobar *(*)(const char *))global[LOOKUP_HOST])
#define resolv (*(struct sockaddr_foobar *(*)(const char *))global[LOOKUP_HOST])
#define host_to_ip (*(char *(*)(const char *))global[HOST_TO_IP])
#define ip_to_host (*(char *(*)(const char *))global[IP_TO_HOST])
#define one_to_another (*(char *(*)(const char *))global[ONE_TO_ANOTHER])
#define set_blocking (*(int (*)(int ))global[SET_BLOCKING])
#define set_non_blocking (*(int (*)(int ))global[SET_NON_BLOCKING])


/* list.c */
#define add_to_list (*(void (*)(List **, List *))global[ADD_TO_LIST])
#define add_to_list_ext (*(void (*)(List **, List *, int (*)(List *, List *)))global[ADD_TO_LIST_EXT])
#define	find_in_list (*(List *(*)(List **, char *, int))global[FIND_IN_LIST])
#define	find_in_list_ext (*(List *(*)(List **, char *, int, int (*)(List *, char *)))global[FIND_IN_LIST_EXT])
#define	remove_from_list (*(List *(*)(List **, char *))global[REMOVE_FROM_LIST_])
#define	remove_from_list_ext (*(List *(*)(List **, char *, int (*)(List *, char *)))global[REMOVE_FROM_LIST_EXT])
#define	removewild_from_list (*(List *(*)(List **, char *))global[REMOVEWILD_FROM_LIST])
#define	list_lookup (*(List *(*)(List **, char *, int, int))global[LIST_LOOKUP])
#define	list_lookup_ext (*(List *(*)(List **, char *, int, int, int (*)(List *, char *)))global[LIST_LOOKUP_EXT])

/* alist.c */
#define add_to_array (*(Array_item *(*)(Array *, Array_item *))global[ADD_TO_ARRAY])
#define remove_from_array (*(Array_item *(*)(Array *, char *))global[REMOVE_FROM_ARRAY])
#define array_pop (*(Array_item *(*)(Array *, int))global[ARRAY_POP])

#define remove_all_from_array (*(Array_item *(*)(Array *, char *))global[REMOVE_ALL_FROM_ARRAY])
#define array_lookup (*(Array_item *(*)(Array *, char *, int, int ))global[ARRAY_LOOKUP])
#define find_array_item (*(Array_item *(*)(Array *, char *, int *, int *))global[FIND_ARRAY_ITEM])

#define find_fixed_array_item (*(void *(*)(void *, size_t, int, char *, int *, int *))global[FIND_FIXED_ARRAY_ITEM])

/* output.c */
#define put_it (*(void (*)(const char *, ...))global[PUT_IT])
#define bitchsay (*(void (*)(const char *, ...))global[BITCHSAY])
#define yell (*(void (*)(const char *, ...))global[YELL])
#define add_to_screen (*(void (*)(unsigned char *))global[ADD_TO_SCREEN])
#define add_to_log (*(void (*)(FILE *, time_t, const char *, int ))global[ADD_TO_LOG])

#define bsd_glob (*(int (*)(const char *, int, int (*)(const char *, int), glob_t *))global[BSD_GLOB])
#define bsd_globfree (*(void (*)(glob_t *))global[BSD_GLOBFREE])

/* misc commands */
#define my_encrypt (*(void (*)(char *, int , char *))global[MY_ENCRYPT])
#define my_decrypt (*(void (*)(char *, int , char *))global[MY_DECRYPT])
#define prepare_command (*(ChannelList *(*)(int *, char *, int))global[PREPARE_COMMAND])
#define convert_output_format (*(char *(*)(const char *, const char *, ...))global[CONVERT_OUTPUT_FORMAT])
#define userage (*(void (*)(char *, char *))global[USERAGE])
#define send_text (*(void (*)(const char *, const char *, char *, int , int ))global[SEND_TEXT])
/* this needs to be worked out. it's passed in the IrcVariable * to _Init */
#define load (*(void (*)(char *, char *, char *, char *))global[FUNC_LOAD])
#define update_clock (*(char *(*)(int ))global[UPDATE_CLOCK])
#define PasteArgs (*(char *(*)(char **, int ))global[PASTEARGS])
#define BreakArgs (*(int (*)(char *, char **, char **, int ))global[BREAKARGS])

#define set_lastlog_msg_level (*(unsigned long (*)(unsigned long ))global[SET_LASTLOG_MSG_LEVEL])
#define split_CTCP (*(void (*)(char *, char *, char *))global[SPLIT_CTCP])
#define random_str (*(char *(*)(int , int ))global[RANDOM_STR])
#define dcc_printf (*(int (*)(int, char *, ...))global[DCC_PRINTF])

/* screen.c */
#define prepare_display (*(unsigned char **(*)(const unsigned char *, int , int *, int ))global[PREPARE_DISPLAY])
#define add_to_window (*(void (*)(Window *, const unsigned char *))global[ADD_TO_WINDOW])
#define skip_incoming_mirc (*(unsigned char *(*)(unsigned char *))global[SKIP_INCOMING_MIRC])
#define add_to_screen (*(void (*)(unsigned char *))global[ADD_TO_SCREEN])
#define split_up_line (*(unsigned char **(*)(const unsigned char *, int ))global[SPLIT_UP_LINE])
#define output_line (*(int (*)(const unsigned char *))global[OUTPUT_LINE])
#define output_with_count (*(int (*)(const unsigned char *, int , int ))global[OUTPUT_WITH_COUNT])
#define scroll_window (*(void (*)(Window *))global[SCROLL_WINDOW])
/* Previous broken definitions - yet it still seemed to work?
#define cursor_not_in_display(x) ((void) (global[CURSOR_IN_DISPLAY]((Screen *)x)))
#define cursor_in_display(x) ((void) (global[CURSOR_IN_DISPLAY]((Screen *)x)))
*/
#define cursor_not_in_display (*(void (*)(Screen *))global[CURSOR_NOT_IN_DISPLAY])
#define cursor_in_display (*(void (*)(Window *))global[CURSOR_IN_DISPLAY])
#define is_cursor_in_display (*(int (*)(Screen *))global[IS_CURSOR_IN_DISPLAY])
#define repaint_window (*(void (*)(Window *, int, int))global[REPAINT_WINDOW])

#define kill_screen (*(void (*)(Screen *))global[KILL_SCREEN])
#define xterm_settitle (*(void (*)(void))global[XTERM_SETTITLE])
#define add_wait_prompt (*(void (*)(char *, void (*)(char *, char *), char *, int , int ))global[ADD_WAIT_PROMPT])
#define skip_ctl_c_seq (*(const unsigned char *(*)(const unsigned char *, int *, int *, int ))global[SKIP_CTL_C_SEQ])
#define strip_ansi (*(unsigned char *(*)(const unsigned char *))global[STRIP_ANSI])
#define create_new_screen ((Screen * (*)(void))global[CREATE_NEW_SCREEN])
#define create_additional_screen ((Window * (*)(void))global[CREATE_ADDITIONAL_SCREEN])


/* window.c */
#define free_formats (*(void (*)(Window *))global[FREE_FORMATS])
#define set_screens_current_window (*(void (*)(Screen *, Window *))global[SET_SCREENS_CURRENT_WINDOW])
#define new_window (*(Window *(*)(struct ScreenStru *))global[NEW_WINDOW])
#define delete_window (*(void (*)(Window *))global[DELETE_WINDOW])
#define traverse_all_windows (*(int (*)(Window **))global[TRAVERSE_ALL_WINDOWS])
#define add_to_invisible_list (*(void (*)(Window *))global[ADD_TO_INVISIBLE_LIST])
#define remove_window_from_screen (*(void (*)(Window *))global[REMOVE_WINDOW_FROM_SCREEN])
#define recalculate_window_positions (*(void (*)(struct ScreenStru *))global[RECALCULATE_WINDOW_POSITIONS])
#define move_window (*(void (*)(Window *, int))global[MOVE_WINDOW])
#define resize_window (*(void (*)(int, Window *, int))global[RESIZE_WINDOW])
#define redraw_all_windows (*(void (*)(void))global[REDRAW_ALL_WINDOWS])
#define rebalance_windows (*(void (*)(struct ScreenStru *))global[REBALANCE_WINDOWS])
#define recalculate_windows (*(void (*)(struct ScreenStru *))global[RECALCULATE_WINDOWS])

#define update_all_windows (*(void (*)(void))global[UPDATE_ALL_WINDOWS])

/* Several of these are never used! */
#define goto_window (*(void (*)(Screen *, int))global[GOTO_WINDOW])
#define hide_window (*(void (*)(Window *))global[HIDE_BX_WINDOW])
#define swap_last_window (*(void (*)(char , char *))global[FUNC_SWAP_LAST_WINDOW])
#define swap_next_window (*(void (*)(char , char *))global[FUNC_SWAP_NEXT_WINDOW])
#define swap_previous_window (*(void (*)(char , char *))global[FUNC_SWAP_PREVIOUS_WINDOW])
#define show_window (*(void (*)(Window *))global[SHOW_WINDOW])
#define get_status_by_refnum (*(char *(*)(unsigned , unsigned ))global[GET_STATUS_BY_REFNUM])
#define get_visible_by_refnum (*(int (*)(char *))global[GET_VISIBLE_BY_REFNUM])
#define get_window_by_desc (*(Window *(*)(const char *))global[GET_WINDOW_BY_DESC])
#define get_window_by_refnum (*(Window *(*)(unsigned ))global[GET_WINDOW_BY_REFNUM])
#define get_window_by_name (*(Window *(*)(const char *))global[GET_WINDOW_BY_NAME])
#define next_window (*(void (*)(char , char *))global[FUNC_NEXT_WINDOW])
#define previous_window (*(void (*)(char , char *))global[FUNC_PREVIOUS_WINDOW])
#define update_window_status (*(void (*)(Window *, int ))global[UPDATE_WINDOW_STATUS])
#define update_all_status (*(void (*)(Window *, char *, int))global[UPDATE_ALL_STATUS])
#define update_window_status_all (*(void (*)(void ))global[UPDATE_WINDOW_STATUS_ALL])
#define status_update (*(int (*)(int ))global[STATUS_UPDATE])

#define set_prompt_by_refnum (*(void (*)(unsigned , char *))global[SET_PROMPT_BY_REFNUM])
#define get_prompt_by_refnum (*(char *(*)(unsigned ))global[GET_PROMPT_BY_REFNUM])
#define get_target_by_refnum (*(char *(*)(unsigned ))global[GET_TARGET_BY_REFNUM])
#define get_target_cmd_by_refnum (*(char *(*)(u_int))global[GET_TARGET_CMD_BY_REFNUM])
#define get_window_target_by_desc (*(Window *(*)(char *))global[GET_WINDOW_TARGET_BY_DESC])
#define is_current_channel (*(int (*)(char *, int , int ))global[IS_CURRENT_CHANNEL])
#define set_current_channel_by_refnum (*(const char *(*)(unsigned , char *))global[SET_CURRENT_CHANNEL_BY_REFNUM])
#define get_current_channel_by_refnum (*(char *(*)(unsigned ))global[GET_CURRENT_CHANNEL_BY_REFNUM])
#define get_refnum_by_window (*(char *(*)(const Window *))global[GET_REFNUM_BY_WINDOW])
#define is_bound_to_window (*(int (*)(const Window *, const char *))global[IS_BOUND_TO_WINDOW])
#define get_window_bound_channel (*(Window *(*)(const char *))global[GET_WINDOW_BOUND_CHANNEL])
#define is_bound_anywhere (*(int (*)(const char *))global[IS_BOUND_ANYWHERE])
#define is_bound (*(int (*)(const char *, int ))global[IS_BOUND])
#define unbind_channel (*(void (*)(const char *, int ))global[UNBIND_CHANNEL])
#define get_bound_channel (*(char *(*)(Window *))global[GET_BOUND_CHANNEL])
#define get_window_server (*(int (*)(unsigned ))global[GET_WINDOW_SERVER])
#define set_window_server (*(void (*)(int , int , int ))global[SET_WINDOW_SERVER])
#define window_check_servers (*(void (*)(int ))global[WINDOW_CHECK_SERVERS])
#define change_window_server (*(void (*)(int , int ))global[CHANGE_WINDOW_SERVER])
#define set_level_by_refnum (*(void (*)(unsigned , unsigned long ))global[SET_LEVEL_BY_REFNUM])
#define message_to (*(void (*)(unsigned long ))global[MESSAGE_TO])
#define clear_window (*(void (*)(Window *))global[CLEAR_WINDOW])
#define clear_all_windows (*(void (*)(int , int ))global[CLEAR_ALL_WINDOWS])
#define clear_window_by_refnum (*(void (*)(unsigned ))global[CLEAR_WINDOW_BY_REFNUM])
#define unclear_window_by_refnum (*(void (*)(unsigned ))global[UNCLEAR_WINDOW_BY_REFNUM])
#define set_scroll_lines (*(void (*)(Window *, char *, int))global[SET_SCROLL_LINES])
#define set_continued_lines (*(void (*)(Window *, char *, int))global[SET_CONTINUED_LINES])
#define current_refnum (*(unsigned (*)(void ))global[CURRENT_REFNUM])
#define number_of_windows_on_screen (*(int (*)(Window *))global[NUMBER_OF_WINDOWS_ON_SCREEN])
#define set_scrollback_size (*(void (*)(Window *, char *, int))global[SET_SCROLLBACK_SIZE])
#define is_window_name_unique (*(int (*)(char *))global[IS_WINDOW_NAME_UNIQUE])
#define get_nicklist_by_window (*(char *(*)(Window *))global[GET_NICKLIST_BY_WINDOW])
#define scrollback_backwards_lines (*(void (*)(int ))global[SCROLLBACK_BACKWARDS_LINES])
#define scrollback_forwards_lines (*(void (*)(int ))global[SCROLLBACK_FORWARDS_LINES])
#define scrollback_forwards (*(void (*)(char , char *))global[SCROLLBACK_FORWARDS])
#define scrollback_backwards (*(void (*)(char , char *))global[SCROLLBACK_BACKWARDS])
#define scrollback_end (*(void (*)(char , char *))global[SCROLLBACK_END])
#define scrollback_start (*(void (*)(char , char *))global[SCROLLBACK_START])
#define set_hold_mode (*(void (*)(Window *, int, int))global[SET_HOLD_MODE])
#define unhold_windows (*(int (*)(void ))global[UNHOLD_WINDOWS])
#define unstop_all_windows (*(void (*)(char , char *))global[FUNC_UNSTOP_ALL_WINDOWS])
#define reset_line_cnt (*(void (*)(Window *, char *, int))global[RESET_LINE_CNT])
#define toggle_stop_screen (*(void (*)(char , char *))global[FUNC_TOGGLE_STOP_SCREEN])
#define flush_everything_being_held (*(void (*)(Window *))global[FLUSH_EVERYTHING_BEING_HELD])
#define unhold_a_window (*(int (*)(Window *))global[UNHOLD_A_WINDOW])
#define recalculate_window_cursor (*(void (*)(Window *))global[RECALCULATE_WINDOW_CURSOR])
#define make_window_current (*(void (*)(Window *))global[MAKE_WINDOW_CURRENT])
#define clear_scrollback (*(void (*)(Window *))global[CLEAR_SCROLLBACK])

#define set_display_target (*(void (*)(const char *, unsigned long ))global[SET_DISPLAY_TARGET])
#define save_display_target (*(void (*)(const char **, unsigned long *))global[SAVE_DISPLAY_TARGET])
#define restore_display_target (*(void (*)(const char *, unsigned long ))global[RESTORE_DISPLAY_TARGET])
#define reset_display_target (*(void (*)(void ))global[RESET_DISPLAY_TARGET])

#define build_status (*(void (*)(Window *, char *, int))global[BUILD_STATUS])



#define do_hook (*(int (*)(int, char *, ...))global[HOOK])

/* input.c */
#define update_input (*(void (*)(int ))global[FUNC_UPDATE_INPUT])
#define cursor_to_input (*(void (*)(void ))global[CURSOR_TO_INPUT])
#define set_input (*(void (*)(char *))global[SET_INPUT])
#define get_input (*(char *(*)(void ))global[GET_INPUT])
#define get_input_prompt (*(char *(*)(void ))global[GET_INPUT_PROMPT])
#define set_input_prompt (*(void (*)(Window *, char *, int))global[SET_INPUT_PROMPT])
#define addtabkey (*(void (*)(char *, char *, int ))global[ADDTABKEY])
#define gettabkey (*(NickTab *(*)(int, int, char *))global[GETTABKEY])
#define getnextnick (*(NickTab *(*)(int, char *, char *, char *))global[GETNEXTNICK])
#define getchannick (*(char *(*)(char *, char *))global[GETCHANNICK])
#define lookup_nickcompletion (*(NickList *(*)(ChannelList *, char *))global[LOOKUP_NICKCOMPLETION])
#define add_completion_type (*(int (*)(char *, int , enum completion ))global[ADD_COMPLETION_TYPE])

/* names.c */
#define is_channel (*(int (*)(char *))global[IS_CHANNEL])
#define make_channel (*(char *(*)(char *))global[MAKE_CHANNEL])
#define is_chanop (*(int (*)(char *, char *))global[IS_CHANOP])
#define is_halfop (*(int (*)(char *, char *))global[IS_HALFOP])
#define im_on_channel (*(int (*)(char *, int ))global[IM_ON_CHANNEL])
#define is_on_channel (*(int (*)(char *, int , char *))global[IS_ON_CHANNEL])
#define add_channel (*(ChannelList *(*)(char *, int, int))global[ADD_CHANNEL])
#define add_to_channel (*(ChannelList *(*)(char *, char *, int, int, int, char *, char *, char *, int, int))global[ADD_TO_CHANNEL])
#define get_channel_key (*(char *(*)(char *, int ))global[GET_CHANNEL_KEY])
#define recreate_mode (*(char *(*)(ChannelList *))global[FUNC_RECREATE_MODE])
#define do_compress_modes (*(char *(*)(ChannelList *, int, char *, char *))global[FUNC_COMPRESS_MODES])
#define got_ops (*(int (*)(int, ChannelList *))global[FUNC_GOT_OPS])
#define get_channel_bans (*(char *(*)(char *, int , int ))global[GET_CHANNEL_BANS])
#define get_channel_mode (*(char *(*)(char *, int ))global[GET_CHANNEL_MODE])
#define clear_bans (*(void (*)(ChannelList *))global[CLEAR_BANS])
#define remove_channel (*(void (*)(char *, int ))global[REMOVE_CHANNEL])
#define remove_from_channel (*(void (*)(char *, char *, int , int , char *))global[REMOVE_FROM_CHANNEL])
#define rename_nick (*(void (*)(char *, char *, int ))global[RENAME_NICK])
#define get_channel_oper (*(int (*)(char *, int ))global[GET_CHANNEL_OPER])
#define get_channel_halfop (*(int (*)(char *, int ))global[GET_CHANNEL_HALFOP])
#define get_channel_voice (*(int (*)(char *, int ))global[GET_CHANNEL_VOICE])
#define fetch_userhost (*(char *(*)(int , char *))global[FETCH_USERHOST])
#define create_channel_list (*(char *(*)(Window *))global[CREATE_CHANNEL_LIST])
#define flush_channel_stats (*(void (*)(void ))global[FLUSH_CHANNEL_STATS])
#define lookup_channel (*(ChannelList *(*)(char *, int, int))global[LOOKUP_CHANNEL])

/* hash.c */
#define find_nicklist_in_channellist (*(NickList *(*)(char *, ChannelList *, int))global[FIND_NICKLIST_IN_CHANNELLIST])
#define add_nicklist_to_channellist (*(void (*)(NickList *, ChannelList *))global[ADD_NICKLIST_TO_CHANNELLIST])
#define next_nicklist (*(NickList *(*)(ChannelList *, NickList *))global[NEXT_NICKLIST])
#define next_namelist (*(List *(*)(HashEntry *, List *, unsigned int))global[NEXT_NAMELIST])
#define add_name_to_genericlist (*(void (*)(char *, HashEntry *, unsigned int))global[ADD_NAME_TO_GENERICLIST])
#define find_name_in_genericlist (*(List *(*)(char *, HashEntry *, unsigned int, int))global[FIND_NAME_IN_GENERICLIST])
#define add_whowas_userhost_channel (*(void (*)(WhowasList *, WhowasWrapList *))global[ADD_WHOWAS_USERHOST_CHANNEL])
#define find_userhost_channel (*(WhowasList *(*)(char *, char *, int, WhowasWrapList *))global[FIND_USERHOST_CHANNEL])
#define next_userhost (*(WhowasList *(*)(WhowasWrapList *, WhowasList *))global[NEXT_USERHOST])
#define sorted_nicklist (*(NickList *(*)(ChannelList *, int))global[SORTED_NICKLIST])
#define clear_sorted_nicklist (*(void (*)(NickList **))global[CLEAR_SORTED_NICKLIST])
#define add_name_to_floodlist (*(Flooding *(*)(char *, char *, char *, HashEntry *, unsigned int))global[ADD_NAME_TO_FLOODLIST])
#define find_name_in_floodlist (*(Flooding *(*)(char *, char *, HashEntry *, unsigned int, int))global[FIND_NAME_IN_FLOODLIST])

#define remove_oldest_whowas_hashlist (*(int (*)(WhowasWrapList *, time_t, int))global[REMOVE_OLDEST_WHOWAS_HASHLIST])



/* cset.c fset.c vars.c set string and set int ops */
#define fget_string_var (*(char *(*)(enum FSET_TYPES ))global[FGET_STRING_VAR])
#define fset_string_var (*(void (*)(enum FSET_TYPES , char *))global[FSET_STRING_VAR])
#define get_wset_string_var (*(char *(*)(WSet *, int))global[GET_WSET_STRING_VAR])
#define set_wset_string_var (*(void (*)(WSet *, int, char *))global[SET_WSET_STRING_VAR])
#define get_cset_int_var (*(int (*)(CSetList *, int))global[GET_CSET_INT_VAR])
#define set_cset_int_var (*(void (*)(CSetList *, int, int))global[SET_CSET_INT_VAR])
#define get_cset_str_var (*(char *(*)(CSetList *, int))global[GET_CSET_STR_VAR])
#define set_cset_str_var (*(void (*)(CSetList *, int, const char *))global[SET_CSET_STR_VAR])

#define get_dllint_var (*(int (*)(char *))global[GET_DLLINT_VAR])
#define set_dllint_var (*(void (*)(char *, unsigned int ))global[SET_DLLINT_VAR])
#define get_dllstring_var (*(char *(*)(char *))global[GET_DLLSTRING_VAR])
#define set_dllstring_var (*(void (*)(char *, char *))global[SET_DLLSTRING_VAR])

#define get_int_var (*(int (*)(enum VAR_TYPES ))global[GET_INT_VAR])
#define set_int_var (*(void (*)(enum VAR_TYPES , unsigned int ))global[SET_INT_VAR])
#define get_string_var (*(char *(*)(enum VAR_TYPES ))global[GET_STRING_VAR])
#define set_string_var (*(void (*)(enum VAR_TYPES , char *))global[SET_STRING_VAR])


/* module.c */
#define add_module_proc (*(int (*)(unsigned int , char *, char *, char *, int , int , void *, void *))global[ADD_MODULE_PROC])
#define remove_module_proc (*(int (*)(unsigned int , char *, char *, char *))global[REMOVE_MODULE_PROC])


/* timer.c */
#define add_timer (*(char *(*)(int , char *, double , long , int (*)(void *, char *), char *, char *, int , char *))global[ADD_TIMER])
#define delete_timer (*(int (*)(char *))global[DELETE_TIMER])
#define delete_all_timers (*(void (*)(void ))global[DELETE_ALL_TIMERS])



/* server.c */
#define send_to_server (*(void (*)(const char *, ...))global[SEND_TO_SERVER])
#define queue_send_to_server (*(void (*)(int, const char *, ...))global[QUEUE_SEND_TO_SERVER])
#define my_send_to_server (*(void (*)(int, const char *, ...))global[MY_SEND_TO_SERVER])
#define get_connected (*(void (*)(int , int ))global[GET_CONNECTED])
#define connect_to_server_by_refnum (*(int (*)(int , int ))global[CONNECT_TO_SERVER_BY_REFNUM])
#define close_server (*(void (*)(int , char *))global[CLOSE_SERVER])
#define is_server_connected (*(int (*)(int ))global[IS_SERVER_CONNECTED])
#define flush_server (*(void (*)(void ))global[FLUSH_SERVER])
#define server_is_connected (*(void (*)(int , int ))global[SERVER_IS_CONNECTED])
#define is_server_open (*(int (*)(int ))global[IS_SERVER_OPEN])
#define close_all_server (*(void (*)(void ))global[CLOSE_ALL_SERVER])

#define read_server_file (*(int (*)(char *))global[READ_SERVER_FILE])
#define add_to_server_list (*(void (*)(char *, int , char *, char *, char *, char *, char *, int , int ))global[ADD_TO_SERVER_LIST])
#define build_server_list (*(int (*)(char *))global[BUILD_SERVER_LIST])
#define display_server_list (*(void (*)(void ))global[DISPLAY_SERVER_LIST])
#define create_server_list (*(char *(*)(char *))global[CREATE_SERVER_LIST])
#define parse_server_info (*(void (*)(char *, char **, char **, char **, char **, char **, char **))global[PARSE_SERVER_INFO])
#define server_list_size (*(int (*)(void ))global[SERVER_LIST_SIZE])

#define find_server_refnum (*(int (*)(char *, char **))global[FIND_SERVER_REFNUM])
#define find_in_server_list (*(int (*)(char *, int ))global[FIND_IN_SERVER_LIST])
#define parse_server_index (*(int (*)(char *))global[PARSE_SERVER_INDEX])

#define set_server_redirect (*(void (*)(int , const char *))global[SET_SERVER_REDIRECT])
#define get_server_redirect (*(char *(*)(int ))global[GET_SERVER_REDIRECT])
#define check_server_redirect (*(int (*)(char *))global[CHECK_SERVER_REDIRECT])
#define fudge_nickname (*(void (*)(int , int ))global[FUDGE_NICKNAME])
#define reset_nickname (*(void (*)(int ))global[RESET_NICKNAME])

#define set_server_cookie (*(void (*)(int , char *))global[SET_SERVER_COOKIE])
#define set_server_flag (*(void (*)(int , int , int ))global[SET_SERVER_FLAG])
#define set_server_motd (*(void (*)(int , int ))global[SET_SERVER_MOTD])
#define set_server_operator (*(void (*)(int , int ))global[SET_SERVER_OPERATOR])
#define set_server_itsname (*(void (*)(int , char *))global[SET_SERVER_ITSNAME])
#define set_server_version (*(void (*)(int , int ))global[SET_SERVER_VERSION])
#define set_server_lag (*(void (*)(int , int ))global[SET_SERVER_LAG])
#define set_server_password (*(char *(*)(int , char *))global[SET_SERVER_PASSWORD])
#define set_server_nickname (*(void (*)(int , char *))global[SET_SERVER_NICKNAME])
#define set_server2_8 (*(void (*)(int , int ))global[SET_SERVER2_8])
#define set_server_away (*(void (*)(int , char *, int ))global[SET_SERVER_AWAY])

#define get_server_cookie (*(char *(*)(int ))global[GET_SERVER_COOKIE])
#define get_server_nickname (*(char *(*)(int ))global[GET_SERVER_NICKNAME])
#define get_server_name (*(char *(*)(int ))global[GET_SERVER_NAME])
#define get_server_itsname (*(char *(*)(int ))global[GET_SERVER_ITSNAME])
#define get_server_motd (*(int (*)(int ))global[GET_SERVER_MOTD])
#define get_server_operator (*(int (*)(int ))global[GET_SERVER_OPERATOR])
#define get_server_version (*(int (*)(int ))global[GET_SERVER_VERSION])
#define get_server_flag (*(int (*)(int , int ))global[GET_SERVER_FLAG])
#define get_possible_umodes (*(char *(*)(int ))global[GET_POSSIBLE_UMODES])
#define get_server_port (*(int (*)(int ))global[GET_SERVER_PORT])
#define get_server_lag (*(int (*)(int ))global[GET_SERVER_LAG])
#define get_server2_8 (*(int (*)(int ))global[GET_SERVER2_8])
#define get_umode (*(char *(*)(int ))global[GET_UMODE])
#define get_server_away (*(char *(*)(int ))global[GET_SERVER_AWAY])
#define get_server_network (*(char *(*)(int ))global[GET_SERVER_NETWORK])
#define get_pending_nickname (*(char *(*)(int ))global[GET_PENDING_NICKNAME])
#define server_disconnect (*(void (*)(int , char *))global[SERVER_DISCONNECT])

#define get_server_list (*(Server *(*)(void))global[GET_SERVER_LIST])
#define get_server_channels (*(ChannelList *(*)(int))global[GET_SERVER_CHANNELS])

#define set_server_last_ctcp_time (*(void (*)(int , time_t))global[SET_SERVER_LAST_CTCP_TIME])
#define get_server_last_ctcp_time (*(time_t (*)(int))global[GET_SERVER_LAST_CTCP_TIME])
#define set_server_trace_flag (*(void (*)(int , int ))global[SET_SERVER_TRACE_FLAG])
#define get_server_trace_flag (*(int (*)(int ))global[GET_SERVER_TRACE_FLAG])
#define get_server_read (*(int (*)(int ))global[GET_SERVER_READ])
#define get_server_linklook (*(int (*)(int ))global[GET_SERVER_LINKLOOK])
#define set_server_linklook (*(void (*)(int , int ))global[SET_SERVER_LINKLOOK])
#define get_server_stat_flag (*(int (*)(int ))global[GET_SERVER_STAT_FLAG])
#define set_server_stat_flag (*(void (*)(int , int ))global[SET_SERVER_STAT_FLAG])
#define get_server_linklook_time (*(time_t (*)(int ))global[GET_SERVER_LINKLOOK_TIME])
#define set_server_linklook_time (*(void (*)(int , time_t))global[SET_SERVER_LINKLOOK_TIME])
#define get_server_trace_kill (*(int (*)(int ))global[GET_SERVER_TRACE_KILL])
#define set_server_trace_kill (*(void (*)(int , int ))global[SET_SERVER_TRACE_KILL])
#define add_server_channels (*(void (*)(int, ChannelList *))global[ADD_SERVER_CHANNELS])
#define set_server_channels (*(void (*)(int, ChannelList *))global[SET_SERVER_CHANNELS])
#define send_msg_to_channels (*(void (*)(ChannelList *, int, char *))global[SEND_MSG_TO_CHANNELS])
#define send_msg_to_nicks (*(void (*)(ChannelList *, int, char *))global[SEND_MSG_TO_NICKS])
#define is_server_queue (*(int (*)(void ))global[IS_SERVER_QUEUE])


/* sockets */
#define add_socketread (*(int (*)(int, int, unsigned long, char *, void (*)(int), void (*)(int)))global[ADD_SOCKETREAD])
#define add_sockettimeout (*(void (*)(int , time_t, void *))global[ADD_SOCKETTIMEOUT])
#define close_socketread (*(void (*)(int ))global[CLOSE_SOCKETREAD])
#define get_socket (*(SocketList *(*)(int ))global[GET_SOCKET])
#define set_socketflags (*(unsigned long (*)(int , unsigned long ))global[SET_SOCKETFLAGS])
#define get_socketflags (*(unsigned long (*)(int ))global[GET_SOCKETFLAGS])
#define check_socket (*(int (*)(int ))global[CHECK_SOCKET])
#define read_sockets (*(int (*)(int , unsigned char *, int ))global[READ_SOCKETS])
#define write_sockets (*(int (*)(int , unsigned char *, int , int ))global[WRITE_SOCKETS])
#define get_max_fd (*(int (*)(void ))global[GET_MAX_FD])
#define new_close (*(int (*)(int ))global[NEW_CLOSE])
#define new_open (*(int (*)(int ))global[NEW_OPEN])
#define dgets (*(int (*)(char *, int , int , int , void *))global[DGETS])
#define get_socketinfo (*(void *(*)(int ))global[GET_SOCKETINFO])
#define set_socketinfo (*(void (*)(int , void *))global[SET_SOCKETINFO])
#define set_socket_write (*(int (*)(int ))global[SET_SOCKETWRITE])


/* flood.c */
#define is_other_flood (*(int (*)(ChannelList *, NickList *, int, int *))global[IS_OTHER_FLOOD])
#define check_flooding (*(int (*)(char *, int , char *, char *))global[CHECK_FLOODING])
#define flood_prot (*(int (*)(char *, char *, char *, int , int , char *))global[FLOOD_PROT])

/* expr.c */
#define next_unit (*(char *(*)(char *, const char *, int *, int ))global[NEXT_UNIT])
#define parse_inline (*(char *(*)(char *, const char *, int *))global[PARSE_INLINE])
#define expand_alias (*(char *(*)(const char *, const char *, int *, char **))global[EXPAND_ALIAS])
#define alias_special_char (*(char *(*)(char **, char *, const char *, char *, int *))global[ALIAS_SPECIAL_CHAR])
#define parse_line (*(void (*)(const char *, char *, const char *, int , int , int ))global[PARSE_LINE])
#define parse_command (*(int (*)(char *, int , char *))global[PARSE_COMMAND_FUNC])
#define make_local_stack (*(void (*)(char *))global[MAKE_LOCAL_STACK])
#define destroy_local_stack (*(void (*)(void ))global[DESTROY_LOCAL_STACK])


/* dcc.c */
#define dcc_create (*(DCC_int *(*)(char *, char *, char *, unsigned long, int, int, unsigned long, void (*)(int)))global[DCC_CREATE_FUNC])
#define find_dcc (*(SocketList *(*)(char *, char *, char *, int, int, int, int))global[FIND_DCC_FUNC])
#define erase_dcc_info (*(void (*)(int, int, char *, ...))global[ERASE_DCC_INFO])
#define add_dcc_bind (*(int (*)(char *, char *, void *, void *, void *, void *, void *))global[ADD_DCC_BIND])
#define remove_dcc_bind (*(int (*)(char *, int ))global[REMOVE_DCC_BIND])
#define remove_all_dcc_binds (*(int (*)(char *))global[REMOVE_ALL_DCC_BINDS])
#define get_active_count (*(int (*)(void ))global[GET_ACTIVE_COUNT])
#define get_num_queue (*(int (*)(void ))global[GET_NUM_QUEUE])
#define add_to_queue (*(int (*)(char *, char *, pack *))global[ADD_TO_QUEUE])
#define dcc_filesend (*(void (*)(char *, char *))global[DCC_FILESEND])
#define dcc_resend (*(void (*)(char *, char *))global[DCC_RESEND])

/* irc.c */
#define irc_exit (*(void (*)(int, char *, char *, ...))global[IRC_EXIT_FUNC])
#define io (*(void (*)(const char *))global[IRC_IO_FUNC])

/* commands.c */
#define find_command (*(IrcCommand *(*)(char *, int *))global[FIND_COMMAND_FUNC])

#define lock_stack_frame (*(void (*)(void ))global[LOCK_STACK_FRAME])
#define unlock_stack_frame (*(void (*)(void ))global[UNLOCK_STACK_FRAME])

/* who.c */
#define userhostbase (*(void (*)(char *, void (*)(UserhostItem *, char *, char *), int, char *, ...))global[USERHOSTBASE])
#define isonbase (*(void (*)(char *, void (*)(char *, char *)))global[ISONBASE])
#define whobase (*(void (*)(char *, void (*)(WhoEntry *, char *, char **), void (*)(WhoEntry *, char *, char **), char *, ...))global[WHOBASE])

#define add_to_window_list (*(Window *(*)(struct ScreenStru *, Window *))global[ADD_TO_WINDOW_LIST])

/*
 * Rest of these are all variables of various sorts.
 */

#ifndef MAIN_SOURCE

#define nickname ((char *) *global[NICKNAME])
#define irc_version ((char *) *global[IRC_VERSION])

#define from_server (*(int *)global[FROM_SERVER])
#define connected_to_server ((int) *((int *)global[CONNECTED_TO_SERVER]))
#define primary_server ((int) *((int *)global[PRIMARY_SERVER]))
#define parsing_server_index ((int) *((int *)global[PARSING_SERVER_INDEX]))
#define now ((time_t) *((time_t *)global[NOW]))
#define start_time ((time_t) *((time_t *)global[START_TIME]))
#define idle_time() ((time_t) *((time_t *)global[IDLE_TIME]()))

#define loading_global (*((int *)global[LOADING_GLOBAL]))
#define target_window (*((Window **)global[TARGET_WINDOW]))
#define current_window (*((Window **)global[CURRENT_WINDOW]))
#define invisible_list (*((Window **)global[INVISIBLE_LIST]))
#define main_screen (*((Screen **)global[MAIN_SCREEN]))
#define last_input_screen (*((Screen **)global[LAST_INPUT_SCREEN]))
#define output_screen (*((Screen **)global[OUTPUT_SCREEN]))
#define screen_list (*((Screen **)global[SCREEN_LIST]))
#define irclog_fp (*((FILE **)global[IRCLOG_FP]))
#define dll_functions (*((BuiltInDllFunctions **)global[DLL_FUNCTIONS]))
#define dll_numeric (*((NumericFunction **)global[DLL_NUMERIC]))
#define dll_commands (*((IrcCommandDll **)global[DLL_COMMANDS]))
#define dll_variable (*((IrcVariableDll **)global[DLL_VARIABLE]))
#define dll_ctcp (*((CtcpEntryDll **)global[DLL_CTCP]))
#define dll_window (*((WindowDll **)global[DLL_WINDOW]))
#define window_display ((int) *((int *)global[WINDOW_DISPLAY]))
#define status_update_flag ((int) *((int *)global[STATUS_UPDATE_FLAG]))
#define tabkey_array (*((NickTab **)global[TABKEY_ARRAY]))
#define autoreply_array (*((NickTab *)global[AUTOREPLY_ARRAY]))
#define identd (*((int *)global[IDENTD_SOCKET]))
#define doing_notice ((int) *((int *)global[DOING_NOTICE]))

#define default_output_function (*(void (**)(char *))global[DEFAULT_OUTPUT_FUNCTION])

#define serv_open_func (*(int (**)(int, unsigned long, int))global[SERV_OPEN_FUNC])
#define serv_input_func (*(int (**)(int, char *, int, int, int))global[SERV_INPUT_FUNC])
#define serv_output_func (*(int (**)(int, int, char *, int))global[SERV_OUTPUT_FUNC])
#define serv_close_func (*(int (**)(int, unsigned long, int))global[SERV_CLOSE_FUNC])
#if 0
#define check_ext_mail_status (*(int (**)()) global[CHECK_EXT_MAIL_STATUS])
#define check_ext_mail (*(char *(**)())global[CHECK_EXT_MAIL])
#endif

#ifdef WANT_TCL
#define tcl_interp ((Tcl_Interp *)((Tcl_Interp **)(global[VAR_TCL_INTERP])))
#else
#define tcl_interp NULL
#endif
#else
#undef get_time
#define get_time(a) BX_get_time(a)
#endif /* MAIN_SOURCE */

#ifdef GUI
#ifndef MAIN_SOURCE
#define lastclicklinedata ((char *) *global[LASTCLICKLINEDATA])
#define contextx ((int) *((int *)global[CONTEXTX]))
#define contexty ((int) *((int *)global[CONTEXTY]))
#define guiipc ((int) *((int *)global[GUIIPC]))
#endif
#define gui_mutex_lock() ((void (*)(void)) global[GUI_MUTEX_LOCK])()
#define gui_mutex_unlock() ((void (*)(void))global[GUI_MUTEX_UNLOCK])()
#endif

#endif /* WTERM_C || STERM_C */
#endif
