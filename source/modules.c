
/*
 * Routines within this files are Copyright Colten Edwards 1996
 * Aka panasync on irc.
 * Thanks to Shiek and Flier for helpful hints and suggestions. As well 
 * as code in some cases.
 */

#define __modules_c

#include "irc.h"
static char cvsrevision[] = "$Id: modules.c 87 2010-06-26 08:18:34Z keaston $";
CVS_REVISION(modules_c)
#include "struct.h"
#include "alias.h"
#include "encrypt.h"
#include "commands.h"
#include "dcc.h"
#include "input.h"
#include "ircaux.h"
#include "flood.h"
#include "hook.h"
#include "list.h"
#include "output.h"
#include "log.h"
#include "ctcp.h"
#include "cdcc.h"
#include "misc.h"
#include "module.h"
#include "names.h"
#include "hash2.h"
#include "vars.h"
#include "screen.h"
#include "parse.h"
#include "server.h"
#include "timer.h"
#include "status.h"
#include "window.h"
#include "tcl_bx.h"
#include "ircterm.h"
#define MAIN_SOURCE
#include "modval.h"

IrcVariableDll *dll_variable = NULL;
extern void *default_output_function;
extern DCC_dllcommands dcc_dllcommands;
extern void *default_status_output_function;

Function_ptr global_table[NUMBER_OF_GLOBAL_FUNCTIONS] = { NULL };
Function_ptr *global = global_table;
char *_modname_ = NULL;

#ifdef WANT_DLL

#ifdef NO_DLFCN_H
#   include "../include/dlfcn.h"
#else
#if defined(HPUX)
#include <dl.h>
#else
#   include <dlfcn.h>
#endif
#endif

#ifndef RTLD_NOW
#   define RTLD_NOW 1
#endif

#ifndef RTLD_GLOBAL
#   define RTLD_GLOBAL 0
#endif


Packages *install_pack = NULL;
#endif /* WANT_DLL */

extern int BX_read_sockets();
extern int identd;
extern int doing_notice;

extern int (*dcc_open_func) (int, int, unsigned long, int);
extern int (*dcc_output_func) (int, int, char *, int);
extern int (*dcc_input_func)  (int, int, char *, int, int);
extern int (*dcc_close_func) (int, unsigned long, int);

int (*serv_open_func) (int, unsigned long, int);
extern int (*serv_output_func) (int, int, char *, int);
extern int (*serv_input_func)  (int, char *, int, int, int);
extern int (*serv_close_func) (int, unsigned long, int);
extern int (*check_ext_mail_status)(void);
extern char *(*check_ext_mail)(void);

int BX_check_module_version(unsigned long);

#ifdef GUI
extern char *lastclicklinedata;
extern int contextx, contexty;
extern int guiipc[2];
#endif

void null_function(void)
{
}

void init_global_functions(void)
{
static int already_done = 0;

	if (already_done)
		return;
	already_done++;

/* ircaux.c */
	global_table[MODULE_VERSION_CHECK]	= (Function_ptr) BX_check_module_version;
	global_table[VSNPRINTF]			= (Function_ptr) vsnprintf;
	global_table[SNPRINTF]			= (Function_ptr) snprintf;
	global_table[NEW_MALLOC] 		= (Function_ptr) n_malloc;
	global_table[NEW_FREE]			= (Function_ptr) n_free;
	global_table[NEW_REALLOC] 		= (Function_ptr) n_realloc;
	global_table[MALLOC_STRCPY] 		= (Function_ptr) n_malloc_strcpy;
	global_table[MALLOC_STR2CPY] 		= (Function_ptr) BX_malloc_str2cpy;
	global_table[M_3DUP]	 		= (Function_ptr) BX_m_3dup;
	global_table[M_OPENDUP]	 		= (Function_ptr) BX_m_opendup;
	global_table[M_S3CAT]	 		= (Function_ptr) BX_m_s3cat;
	global_table[M_S3CAT_S]	 		= (Function_ptr) BX_m_s3cat_s;
	global_table[M_3CAT]	 		= (Function_ptr) BX_m_3cat;
	global_table[UPPER]	 		= (Function_ptr) BX_upper;
	global_table[LOWER]	 		= (Function_ptr) BX_lower;
	global_table[STRISTR]	 		= (Function_ptr) BX_stristr;
	global_table[RSTRISTR]	 		= (Function_ptr) BX_rstristr;
	global_table[WORD_COUNT]	 	= (Function_ptr) BX_word_count;
	global_table[REMOVE_TRAILING_SPACES] 	= (Function_ptr) BX_remove_trailing_spaces;

	global_table[MY_STRICMP]		= (Function_ptr) BX_my_stricmp;
	global_table[MY_STRNICMP]		= (Function_ptr) BX_my_strnicmp;

	global_table[MY_STRNSTR]		= (Function_ptr) BX_my_strnstr;
	global_table[CHOP]			= (Function_ptr) BX_chop;
	global_table[STRMCPY]			= (Function_ptr) BX_strmcpy;
	global_table[STRMCAT]			= (Function_ptr) BX_strmcat;
	global_table[SCANSTR]			= (Function_ptr) BX_scanstr;
	global_table[EXPAND_TWIDDLE]		= (Function_ptr) BX_expand_twiddle;
	global_table[CHECK_NICKNAME]		= (Function_ptr) BX_check_nickname;
	global_table[SINDEX]			= (Function_ptr) BX_sindex;
	global_table[RSINDEX]			= (Function_ptr) BX_rsindex;
	global_table[ISNUMBER]			= (Function_ptr) BX_is_number;
	global_table[RFGETS]			= (Function_ptr) BX_rfgets;
	global_table[PATH_SEARCH]		= (Function_ptr) BX_path_search;
	global_table[DOUBLE_QUOTE]		= (Function_ptr) BX_double_quote;
	global_table[IRCPANIC]			= (Function_ptr) BX_ircpanic;
	global_table[END_STRCMP]		= (Function_ptr) BX_end_strcmp;
	global_table[BEEP_EM]			= (Function_ptr) BX_beep_em;
	global_table[UZFOPEN]			= (Function_ptr) BX_uzfopen;
	global_table[FUNC_GET_TIME]		= (Function_ptr) BX_get_time;
	global_table[TIME_DIFF]			= (Function_ptr) BX_time_diff;
	global_table[TIME_TO_NEXT_MINUTE]	= (Function_ptr) BX_time_to_next_minute;
	global_table[PLURAL]			= (Function_ptr) BX_plural;
	global_table[MY_CTIME]			= (Function_ptr) BX_my_ctime;
	global_table[LTOA]			= (Function_ptr) BX_my_ltoa;
	global_table[STRFORMAT]			= (Function_ptr) BX_strformat;
	global_table[MATCHINGBRACKET]		= (Function_ptr) BX_MatchingBracket;
	global_table[PARSE_NUMBER]		= (Function_ptr) BX_parse_number;
	global_table[SPLITW]			= (Function_ptr) BX_splitw;
	global_table[UNSPLITW]			= (Function_ptr) BX_unsplitw;
	global_table[M_2DUP]			= (Function_ptr) BX_m_2dup;
	global_table[M_E3CAT]			= (Function_ptr) BX_m_e3cat;
	global_table[CHECK_VAL]			= (Function_ptr) BX_check_val;
	global_table[ON_OFF]			= (Function_ptr) BX_on_off;
	global_table[STREXTEND]			= (Function_ptr) BX_strextend;
	global_table[STRFILL]			= (Function_ptr) BX_strfill;
	global_table[EMPTY_FUNC]		= (Function_ptr) BX_empty;
	global_table[REMOVE_BRACKETS]		= (Function_ptr) BX_remove_brackets;
	global_table[MY_ATOL]			= (Function_ptr) BX_my_atol;
	global_table[M_DUPCHAR]			= (Function_ptr) BX_m_dupchar;
	global_table[STREQ]			= (Function_ptr) BX_streq;
	global_table[STRIEQ]			= (Function_ptr) BX_strieq;
	global_table[STRMOPENCAT]		= (Function_ptr) BX_strmopencat;
	global_table[OV_STRCPY]			= (Function_ptr) BX_ov_strcpy;
	global_table[STRIP_CONTROL]		= (Function_ptr) BX_strip_control;
	global_table[FIGURE_OUT_ADDRESS]	= (Function_ptr) BX_figure_out_address;
	global_table[STRNRCHR]			= (Function_ptr) BX_strnrchr;
	global_table[MASK_DIGITS]		= (Function_ptr) BX_mask_digits;
	global_table[CCSPAN]			= (Function_ptr) BX_ccspan;
	global_table[CHARCOUNT]			= (Function_ptr) BX_charcount;
	global_table[STRPCAT]			= (Function_ptr) BX_strpcat;
	global_table[STRCPY_NOCOLORCODES]	= (Function_ptr) BX_strcpy_nocolorcodes;
	global_table[CRYPTIT]			= (Function_ptr) BX_cryptit;
	global_table[STRIPDEV]			= (Function_ptr) BX_stripdev;
	global_table[MANGLE_LINE]		= (Function_ptr) BX_mangle_line;
	global_table[MALLOC_STRCAT] 		= (Function_ptr) n_malloc_strcat;
	global_table[M_STRDUP] 			= (Function_ptr) n_m_strdup;
	global_table[M_STRCAT_UES] 		= (Function_ptr) n_m_strcat_ues;
	global_table[M_STRNDUP] 		= (Function_ptr) n_m_strndup;
	global_table[MALLOC_SPRINTF] 		= (Function_ptr) BX_malloc_sprintf;
	global_table[M_SPRINTF] 		= (Function_ptr) BX_m_sprintf;
	global_table[NEXT_ARG] 			= (Function_ptr) BX_next_arg;
	global_table[NEW_NEXT_ARG] 		= (Function_ptr) BX_new_next_arg;
	global_table[NEW_NEW_NEXT_ARG] 		= (Function_ptr) BX_new_new_next_arg;
	global_table[LAST_ARG] 			= (Function_ptr) BX_last_arg;
	global_table[NEXT_IN_COMMA_LIST]	= (Function_ptr) BX_next_in_comma_list;
	global_table[RANDOM_NUMBER]		= (Function_ptr) BX_random_number;
	
/* words.c reg.c */
	global_table[STRSEARCH]			= (Function_ptr) BX_strsearch;
	global_table[MOVE_TO_ABS_WORD]		= (Function_ptr) BX_move_to_abs_word;
	global_table[MOVE_WORD_REL]		= (Function_ptr) BX_move_word_rel;
	global_table[EXTRACT]			= (Function_ptr) BX_extract;
	global_table[EXTRACT2]			= (Function_ptr) BX_extract2;
	global_table[WILD_MATCH]		= (Function_ptr) BX_wild_match;


/* list.c */
	global_table[ADD_TO_LIST] 		= (Function_ptr) BX_add_to_list;
	global_table[ADD_TO_LIST_EXT] 		= (Function_ptr) BX_add_to_list_ext;
	global_table[FIND_IN_LIST] 		= (Function_ptr) BX_find_in_list;
	global_table[FIND_IN_LIST_EXT] 		= (Function_ptr) BX_find_in_list_ext;
	global_table[REMOVE_FROM_LIST_]		= (Function_ptr) BX_remove_from_list;
	global_table[REMOVE_FROM_LIST_EXT]	= (Function_ptr) BX_remove_from_list_ext;
	global_table[REMOVEWILD_FROM_LIST]	= (Function_ptr) BX_removewild_from_list;
	global_table[LIST_LOOKUP] 		= (Function_ptr) BX_list_lookup;
	global_table[LIST_LOOKUP_EXT] 		= (Function_ptr) BX_list_lookup_ext;

/* alist.c */
	global_table[ADD_TO_ARRAY]		= (Function_ptr) BX_add_to_array;
	global_table[REMOVE_FROM_ARRAY]		= (Function_ptr) BX_remove_from_array;
	global_table[ARRAY_POP]			= (Function_ptr) BX_array_pop;
	global_table[REMOVE_ALL_FROM_ARRAY]	= (Function_ptr) BX_remove_all_from_array;
	global_table[ARRAY_LOOKUP]		= (Function_ptr) BX_array_lookup;
	global_table[FIND_ARRAY_ITEM]		= (Function_ptr) BX_find_array_item;
	global_table[FIND_FIXED_ARRAY_ITEM]	= (Function_ptr) BX_find_fixed_array_item;	


/* server.c */
	global_table[SEND_TO_SERVER]		= (Function_ptr) BX_send_to_server;
	global_table[QUEUE_SEND_TO_SERVER]	= (Function_ptr) BX_queue_send_to_server;
	global_table[MY_SEND_TO_SERVER]		= (Function_ptr) BX_my_send_to_server;

/* connecting to the server */
	global_table[GET_CONNECTED]		= (Function_ptr) BX_get_connected;
	global_table[CONNECT_TO_SERVER_BY_REFNUM]=(Function_ptr) BX_connect_to_server_by_refnum;
	global_table[CLOSE_SERVER]		= (Function_ptr) BX_close_server;
	global_table[IS_SERVER_CONNECTED]	= (Function_ptr) BX_is_server_connected;
	global_table[FLUSH_SERVER]		= (Function_ptr) BX_flush_server;
	global_table[SERVER_IS_CONNECTED]	= (Function_ptr) BX_server_is_connected;
	global_table[IS_SERVER_OPEN]		= (Function_ptr) BX_is_server_open;
	global_table[CLOSE_ALL_SERVER]		= (Function_ptr) BX_close_all_server;
/* server file reading */
	global_table[READ_SERVER_FILE]		= (Function_ptr) BX_read_server_file;
	global_table[ADD_TO_SERVER_LIST]	= (Function_ptr) BX_add_to_server_list;
	global_table[BUILD_SERVER_LIST]		= (Function_ptr) BX_build_server_list;
	global_table[DISPLAY_SERVER_LIST]	= (Function_ptr) BX_display_server_list;
	global_table[CREATE_SERVER_LIST]	= (Function_ptr) BX_create_server_list;
	global_table[PARSE_SERVER_INFO]		= (Function_ptr) BX_parse_server_info;
	global_table[SERVER_LIST_SIZE]		= (Function_ptr) BX_server_list_size;
/* misc server/nickname functions */
	global_table[FIND_SERVER_REFNUM]	= (Function_ptr) BX_find_server_refnum;
	global_table[FIND_IN_SERVER_LIST]	= (Function_ptr) BX_find_in_server_list;
	global_table[PARSE_SERVER_INDEX]	= (Function_ptr) BX_parse_server_index;
	global_table[GET_SERVER_REDIRECT]	= (Function_ptr) BX_get_server_redirect;
	global_table[SET_SERVER_REDIRECT]	= (Function_ptr) BX_set_server_redirect;
	global_table[CHECK_SERVER_REDIRECT]	= (Function_ptr) BX_check_server_redirect;
	global_table[FUDGE_NICKNAME]		= (Function_ptr) BX_fudge_nickname;
	global_table[RESET_NICKNAME]		= (Function_ptr) BX_reset_nickname;
/* various set server struct functions */
	global_table[SET_SERVER_COOKIE]		= (Function_ptr) BX_set_server_cookie;
	global_table[SET_SERVER_FLAG]		= (Function_ptr) BX_set_server_flag;
	global_table[SET_SERVER_MOTD]		= (Function_ptr) BX_set_server_motd;
	global_table[SET_SERVER_OPERATOR]	= (Function_ptr) BX_set_server_operator;
	global_table[SET_SERVER_ITSNAME]	= (Function_ptr) BX_set_server_itsname;
	global_table[SET_SERVER_VERSION]	= (Function_ptr) BX_set_server_version;
	global_table[SET_SERVER_LAG]		= (Function_ptr) BX_set_server_lag;
	global_table[SET_SERVER_PASSWORD]	= (Function_ptr) BX_set_server_password;
	global_table[SET_SERVER_NICKNAME]	= (Function_ptr) BX_set_server_nickname;
	global_table[SET_SERVER2_8]		= (Function_ptr) BX_set_server2_8;
	global_table[SET_SERVER_AWAY]		= (Function_ptr) BX_set_server_away;

/* various get server struct functions */
	global_table[GET_SERVER_COOKIE]		= (Function_ptr) BX_get_server_cookie;
	global_table[GET_SERVER_NICKNAME]	= (Function_ptr) BX_get_server_nickname;
	global_table[GET_SERVER_NAME]		= (Function_ptr) BX_get_server_name;
	global_table[GET_SERVER_ITSNAME]	= (Function_ptr) BX_get_server_itsname;
	global_table[GET_SERVER_MOTD]		= (Function_ptr) BX_get_server_motd;
	global_table[GET_SERVER_OPERATOR]	= (Function_ptr) BX_get_server_operator;
	global_table[GET_SERVER_VERSION]	= (Function_ptr) BX_get_server_version;
	global_table[GET_SERVER_FLAG]		= (Function_ptr) BX_get_server_flag;
	global_table[GET_POSSIBLE_UMODES]	= (Function_ptr) BX_get_possible_umodes;
	global_table[GET_SERVER_PORT]		= (Function_ptr) BX_get_server_port;
	global_table[GET_SERVER_LAG]		= (Function_ptr) BX_get_server_lag;
	global_table[GET_SERVER2_8]		= (Function_ptr) BX_get_server2_8;
	global_table[GET_UMODE]			= (Function_ptr) BX_get_umode;
	global_table[GET_SERVER_AWAY]		= (Function_ptr) BX_get_server_away;
	global_table[GET_SERVER_NETWORK]	= (Function_ptr) BX_get_server_network;
	global_table[GET_PENDING_NICKNAME]	= (Function_ptr) BX_get_pending_nickname;
	global_table[SERVER_DISCONNECT]		= (Function_ptr) BX_server_disconnect;

	global_table[GET_SERVER_LIST]		= (Function_ptr) BX_get_server_list;
	global_table[GET_SERVER_CHANNELS]	= (Function_ptr) BX_get_server_channels;
	global_table[SET_SERVER_LAST_CTCP_TIME]	= (Function_ptr) BX_set_server_last_ctcp_time;
	global_table[GET_SERVER_LAST_CTCP_TIME]	= (Function_ptr) BX_get_server_last_ctcp_time;
	global_table[SET_SERVER_TRACE_FLAG]	= (Function_ptr) BX_set_server_trace_flag;
	global_table[GET_SERVER_TRACE_FLAG]	= (Function_ptr) BX_get_server_trace_flag;
	global_table[SET_SERVER_STAT_FLAG]	= (Function_ptr) BX_set_server_stat_flag;
	global_table[GET_SERVER_STAT_FLAG]	= (Function_ptr) BX_get_server_stat_flag;
	global_table[GET_SERVER_READ]		= (Function_ptr) BX_get_server_read;
	global_table[GET_SERVER_LINKLOOK]	= (Function_ptr) BX_get_server_linklook;
	global_table[SET_SERVER_LINKLOOK]	= (Function_ptr) BX_set_server_linklook;
	global_table[GET_SERVER_LINKLOOK_TIME]	= (Function_ptr) BX_get_server_linklook_time;
	global_table[SET_SERVER_LINKLOOK_TIME]	= (Function_ptr) BX_set_server_linklook_time;
	global_table[GET_SERVER_TRACE_KILL]	= (Function_ptr) BX_get_server_trace_kill;
	global_table[SET_SERVER_TRACE_KILL]	= (Function_ptr) BX_set_server_trace_kill;
	global_table[ADD_SERVER_CHANNELS]	= (Function_ptr) BX_add_server_channels;
	global_table[SET_SERVER_CHANNELS]	= (Function_ptr) BX_set_server_channels;
	global_table[SEND_MSG_TO_CHANNELS]	= (Function_ptr) BX_send_msg_to_channels;
	global_table[SEND_MSG_TO_NICKS]		= (Function_ptr) BX_send_msg_to_nicks;
	global_table[IS_SERVER_QUEUE]		= (Function_ptr) BX_is_server_queue;		

/* glob.c */
	global_table[BSD_GLOB]			= (Function_ptr) BX_bsd_glob;
	global_table[BSD_GLOBFREE]		= (Function_ptr) BX_bsd_globfree;
		
/* output.c */
	global_table[PUT_IT] 			= (Function_ptr) BX_put_it;
	global_table[BITCHSAY]			= (Function_ptr) BX_bitchsay;
	global_table[YELL]			= (Function_ptr) BX_yell;

/* screen.c */
	global_table[ADD_TO_SCREEN]		= (Function_ptr) BX_add_to_screen;
	global_table[XTERM_SETTITLE]		= (Function_ptr) BX_xterm_settitle;
	global_table[PREPARE_DISPLAY]		= (Function_ptr) BX_prepare_display;
	global_table[ADD_TO_WINDOW]		= (Function_ptr) BX_add_to_window;
	global_table[SKIP_INCOMING_MIRC]	= (Function_ptr) BX_skip_incoming_mirc;
	global_table[SPLIT_UP_LINE]		= (Function_ptr) BX_split_up_line;
	global_table[OUTPUT_LINE]		= (Function_ptr) BX_output_line;
	global_table[OUTPUT_WITH_COUNT]		= (Function_ptr) BX_output_with_count;
	global_table[SCROLL_WINDOW]		= (Function_ptr) BX_scroll_window;
	global_table[CURSOR_NOT_IN_DISPLAY]	= (Function_ptr) BX_cursor_not_in_display;
	global_table[CURSOR_IN_DISPLAY]		= (Function_ptr) BX_cursor_in_display;
	global_table[IS_CURSOR_IN_DISPLAY]	= (Function_ptr) BX_is_cursor_in_display;
	global_table[REPAINT_WINDOW]		= (Function_ptr) BX_repaint_window;
	global_table[CREATE_NEW_SCREEN]		= (Function_ptr) BX_create_new_screen;
#ifdef WINDOW_CREATE
	global_table[CREATE_ADDITIONAL_SCREEN]	= (Function_ptr) BX_create_additional_screen;
	global_table[KILL_SCREEN]		= (Function_ptr) BX_kill_screen;
#endif
	global_table[ADD_WAIT_PROMPT]		= (Function_ptr) BX_add_wait_prompt;
	global_table[SKIP_CTL_C_SEQ]		= (Function_ptr) BX_skip_ctl_c_seq;
	global_table[STRIP_ANSI]		= (Function_ptr) BX_strip_ansi;

/* status.c */
	global_table[BUILD_STATUS]		= (Function_ptr) BX_build_status;

/* window.c */
	global_table[GET_WINDOW_BY_NAME]	= (Function_ptr) BX_get_window_by_name;
	global_table[GET_CURRENT_CHANNEL_BY_REFNUM]=(Function_ptr) BX_get_current_channel_by_refnum;
	global_table[NEW_WINDOW]		= (Function_ptr) BX_new_window;
	global_table[GET_WINDOW_SERVER]		= (Function_ptr) BX_get_window_server;
	global_table[RESIZE_WINDOW]		= (Function_ptr) BX_resize_window;
	global_table[UPDATE_ALL_WINDOWS]	= (Function_ptr) BX_update_all_windows;
	global_table[SET_SCREENS_CURRENT_WINDOW]= (Function_ptr) BX_set_screens_current_window;
	global_table[DELETE_WINDOW]		= (Function_ptr) BX_delete_window;
	global_table[FREE_FORMATS]		= (Function_ptr) BX_free_formats;
	global_table[REMOVE_WINDOW_FROM_SCREEN]	= (Function_ptr) BX_remove_window_from_screen;
	global_table[TRAVERSE_ALL_WINDOWS]	= (Function_ptr) BX_traverse_all_windows;
	global_table[ADD_TO_INVISIBLE_LIST]	= (Function_ptr) BX_add_to_invisible_list;
	global_table[ADD_TO_WINDOW_LIST]	= (Function_ptr) BX_add_to_window_list;
	global_table[RECALCULATE_WINDOW_POSITIONS]= (Function_ptr) BX_recalculate_window_positions;
	global_table[MOVE_WINDOW]		= (Function_ptr) BX_move_window;
	global_table[REDRAW_ALL_WINDOWS]	= (Function_ptr) BX_redraw_all_windows;
	global_table[REBALANCE_WINDOWS]		= (Function_ptr) BX_rebalance_windows;
	global_table[RECALCULATE_WINDOWS]	= (Function_ptr) BX_recalculate_windows;
	global_table[GOTO_WINDOW]		= (Function_ptr) BX_goto_window;
	global_table[HIDE_BX_WINDOW]		= (Function_ptr) BX_hide_window;
	global_table[FUNC_SWAP_LAST_WINDOW]	= (Function_ptr) BX_swap_last_window;
	global_table[FUNC_SWAP_NEXT_WINDOW]	= (Function_ptr) BX_swap_next_window;
	global_table[FUNC_SWAP_PREVIOUS_WINDOW]	= (Function_ptr) BX_swap_previous_window;
	global_table[SHOW_WINDOW]		= (Function_ptr) BX_show_window;
	global_table[GET_STATUS_BY_REFNUM]	= (Function_ptr) BX_get_status_by_refnum;
	global_table[GET_WINDOW_BY_DESC]	= (Function_ptr) BX_get_window_by_desc;
	global_table[GET_WINDOW_BY_REFNUM]	= (Function_ptr) BX_get_window_by_refnum;
	global_table[GET_VISIBLE_BY_REFNUM]	= (Function_ptr) BX_get_visible_by_refnum;
	global_table[FUNC_NEXT_WINDOW]		= (Function_ptr) BX_next_window;
	global_table[FUNC_PREVIOUS_WINDOW]	= (Function_ptr) BX_previous_window;
	global_table[UPDATE_WINDOW_STATUS]	= (Function_ptr) BX_update_window_status;
	global_table[UPDATE_ALL_STATUS]		= (Function_ptr) BX_update_all_status;
	global_table[UPDATE_WINDOW_STATUS_ALL]	= (Function_ptr) BX_update_window_status_all;
	global_table[STATUS_UPDATE]		= (Function_ptr) BX_status_update;
	global_table[SET_PROMPT_BY_REFNUM]	= (Function_ptr) BX_set_prompt_by_refnum;
	global_table[GET_PROMPT_BY_REFNUM]	= (Function_ptr) BX_get_prompt_by_refnum;
	global_table[QUERY_NICK]		= (Function_ptr) null_function; /* DEFUNCT */
	global_table[QUERY_HOST]		= (Function_ptr) null_function; /* DEFUNCT */
	global_table[QUERY_CMD]			= (Function_ptr) null_function; /* DEFUNCT */
	global_table[GET_TARGET_BY_REFNUM]	= (Function_ptr) BX_get_target_by_refnum;
	global_table[GET_TARGET_CMD_BY_REFNUM]	= (Function_ptr) BX_get_target_cmd_by_refnum;
	global_table[GET_WINDOW_TARGET_BY_DESC]	= (Function_ptr) BX_get_window_target_by_desc;
	global_table[IS_CURRENT_CHANNEL]	= (Function_ptr) BX_is_current_channel;
	global_table[SET_CURRENT_CHANNEL_BY_REFNUM]= (Function_ptr) BX_set_current_channel_by_refnum;
	global_table[GET_REFNUM_BY_WINDOW]	= (Function_ptr) BX_get_refnum_by_window;
	global_table[IS_BOUND_TO_WINDOW]	= (Function_ptr) BX_is_bound_to_window;
	global_table[GET_WINDOW_BOUND_CHANNEL]	= (Function_ptr) BX_get_window_bound_channel;
	global_table[IS_BOUND_ANYWHERE]		= (Function_ptr) BX_is_bound_anywhere;
	global_table[IS_BOUND]			= (Function_ptr) BX_is_bound;
	global_table[UNBIND_CHANNEL]		= (Function_ptr) BX_unbind_channel;
	global_table[GET_BOUND_CHANNEL]		= (Function_ptr) BX_get_bound_channel;
	global_table[SET_WINDOW_SERVER]		= (Function_ptr) BX_set_window_server;
	global_table[WINDOW_CHECK_SERVERS]	= (Function_ptr) BX_window_check_servers;
	global_table[CHANGE_WINDOW_SERVER]	= (Function_ptr) BX_change_window_server;
	global_table[SET_LEVEL_BY_REFNUM]	= (Function_ptr) BX_set_level_by_refnum;
	global_table[MESSAGE_TO]		= (Function_ptr) BX_message_to;
	global_table[CLEAR_WINDOW]		= (Function_ptr) BX_clear_window;
	global_table[CLEAR_ALL_WINDOWS]		= (Function_ptr) BX_clear_all_windows;
	global_table[CLEAR_WINDOW_BY_REFNUM]	= (Function_ptr) BX_clear_window_by_refnum;
	global_table[UNCLEAR_WINDOW_BY_REFNUM]	= (Function_ptr) BX_unclear_window_by_refnum;
	global_table[SET_SCROLL_LINES]		= (Function_ptr) BX_set_scroll_lines;
	global_table[SET_CONTINUED_LINES]	= (Function_ptr) BX_set_continued_lines;
	global_table[CURRENT_REFNUM]		= (Function_ptr) BX_current_refnum;
	global_table[NUMBER_OF_WINDOWS_ON_SCREEN]= (Function_ptr) BX_number_of_windows_on_screen;
	global_table[SET_SCROLLBACK_SIZE]	= (Function_ptr) BX_set_scrollback_size;
	global_table[IS_WINDOW_NAME_UNIQUE]	= (Function_ptr) BX_is_window_name_unique;
	global_table[GET_NICKLIST_BY_WINDOW]	= (Function_ptr) BX_get_nicklist_by_window;
	global_table[SCROLLBACK_BACKWARDS_LINES]= (Function_ptr) BX_scrollback_backwards_lines;
	global_table[SCROLLBACK_FORWARDS_LINES]	= (Function_ptr) BX_scrollback_forwards_lines;
	global_table[SCROLLBACK_FORWARDS]	= (Function_ptr) BX_scrollback_forwards;
	global_table[SCROLLBACK_BACKWARDS]	= (Function_ptr) BX_scrollback_backwards;
	global_table[SCROLLBACK_END]		= (Function_ptr) BX_scrollback_end;
	global_table[SCROLLBACK_START]		= (Function_ptr) BX_scrollback_start;
	global_table[SET_HOLD_MODE]		= (Function_ptr) BX_set_hold_mode;
	global_table[UNHOLD_WINDOWS]		= (Function_ptr) BX_unhold_windows;
	global_table[FUNC_UNSTOP_ALL_WINDOWS]	= (Function_ptr) BX_unstop_all_windows;
	global_table[RESET_LINE_CNT]		= (Function_ptr) BX_reset_line_cnt;
	global_table[FUNC_TOGGLE_STOP_SCREEN]	= (Function_ptr) BX_toggle_stop_screen;
	global_table[FLUSH_EVERYTHING_BEING_HELD]= (Function_ptr) BX_flush_everything_being_held;
	global_table[UNHOLD_A_WINDOW]		= (Function_ptr) BX_unhold_a_window;
	global_table[RECALCULATE_WINDOW_CURSOR]	= (Function_ptr) BX_recalculate_window_cursor;
	global_table[MAKE_WINDOW_CURRENT]	= (Function_ptr) BX_make_window_current;
	global_table[CLEAR_SCROLLBACK]		= (Function_ptr) BX_clear_scrollback;
	
	global_table[RESET_DISPLAY_TARGET]	= (Function_ptr) BX_reset_display_target;
	global_table[SET_DISPLAY_TARGET]	= (Function_ptr) BX_set_display_target;
	global_table[SAVE_DISPLAY_TARGET]	= (Function_ptr) BX_save_display_target;
	global_table[RESTORE_DISPLAY_TARGET]	= (Function_ptr) BX_restore_display_target;

	global_table[MY_ENCRYPT]		= (Function_ptr) BX_my_encrypt;
	global_table[MY_DECRYPT]		= (Function_ptr) BX_my_decrypt;
	global_table[PREPARE_COMMAND] 		= (Function_ptr) BX_prepare_command;
	global_table[CONVERT_OUTPUT_FORMAT] 	= (Function_ptr) BX_convert_output_format;
	global_table[BREAKARGS]			= (Function_ptr) BX_BreakArgs;
	global_table[PASTEARGS]			= (Function_ptr) BX_PasteArgs;
	global_table[USERAGE]			= (Function_ptr) BX_userage;
	global_table[SEND_TEXT]			= (Function_ptr) BX_send_text;
	global_table[SPLIT_CTCP]		= (Function_ptr) BX_split_CTCP;
	global_table[RANDOM_STR]		= (Function_ptr) BX_random_str;
	global_table[DCC_PRINTF]		= (Function_ptr) BX_dcc_printf;
	global_table[ADD_TO_LOG]		= (Function_ptr) BX_add_to_log;
	global_table[SET_LASTLOG_MSG_LEVEL]	= (Function_ptr) BX_set_lastlog_msg_level;


/* module.c */
#ifdef WANT_DLL
	global_table[REMOVE_MODULE_PROC]	= (Function_ptr) BX_remove_module_proc;
	global_table[ADD_MODULE_PROC] 		= (Function_ptr) BX_add_module_proc;
#else
	global_table[REMOVE_MODULE_PROC]	= (Function_ptr) null_function;
	global_table[ADD_MODULE_PROC] 		= (Function_ptr) null_function;
#endif

/* this one should be differant. */
	global_table[FUNC_LOAD]			= (Function_ptr) BX_load;

	global_table[HOOK]			= (Function_ptr) BX_do_hook;

/* irc.c */
	global_table[UPDATE_CLOCK] 		= (Function_ptr) BX_update_clock;
	global_table[IRC_IO_FUNC]		= (Function_ptr) BX_io;
	global_table[IRC_EXIT_FUNC]		= (Function_ptr) BX_irc_exit;

/* alias.c */
	global_table[LOCK_STACK_FRAME]		= (Function_ptr) BX_lock_stack_frame;
	global_table[UNLOCK_STACK_FRAME]	= (Function_ptr) BX_unlock_stack_frame;
			
/* input.c */	
	global_table[FUNC_UPDATE_INPUT]		= (Function_ptr) BX_update_input;
	global_table[CURSOR_TO_INPUT]		= (Function_ptr) BX_cursor_to_input;
	global_table[SET_INPUT]			= (Function_ptr) BX_set_input;
	global_table[GET_INPUT]			= (Function_ptr) BX_get_input;
	global_table[SET_INPUT_PROMPT]		= (Function_ptr) BX_set_input_prompt;
	global_table[GET_INPUT_PROMPT]		= (Function_ptr) BX_get_input_prompt;
	global_table[ADDTABKEY]			= (Function_ptr) BX_addtabkey;
	global_table[GETTABKEY]			= (Function_ptr) BX_gettabkey;
	global_table[GETNEXTNICK]		= (Function_ptr) BX_getnextnick;
	global_table[GETCHANNICK]		= (Function_ptr) BX_getchannick;
	global_table[LOOKUP_NICKCOMPLETION]	= (Function_ptr) BX_lookup_nickcompletion;
	global_table[ADD_COMPLETION_TYPE]	= (Function_ptr) BX_add_completion_type;
		
/* names.c */			
	global_table[IS_CHANOP]			= (Function_ptr) BX_is_chanop;
	global_table[IS_HALFOP]			= (Function_ptr) BX_is_halfop;
	global_table[IS_CHANNEL]		= (Function_ptr) BX_is_channel;
	global_table[MAKE_CHANNEL]		= (Function_ptr) BX_make_channel; /* this is really in misc.c */
	global_table[IM_ON_CHANNEL]		= (Function_ptr) BX_im_on_channel;
	global_table[IS_ON_CHANNEL]		= (Function_ptr) BX_is_on_channel;
	global_table[ADD_CHANNEL]		= (Function_ptr) BX_add_channel;
	global_table[ADD_TO_CHANNEL]		= (Function_ptr) BX_add_to_channel;
	global_table[GET_CHANNEL_KEY]		= (Function_ptr) BX_get_channel_key;
	global_table[FUNC_RECREATE_MODE]	= (Function_ptr) BX_recreate_mode;
#ifdef COMPRESS_MODES
	global_table[FUNC_COMPRESS_MODES]	= (Function_ptr) BX_do_compress_modes;
#endif
	global_table[FUNC_GOT_OPS]		= (Function_ptr) BX_got_ops;
	global_table[GET_CHANNEL_BANS]		= (Function_ptr) BX_get_channel_bans;
	global_table[GET_CHANNEL_MODE]		= (Function_ptr) BX_get_channel_mode;
	global_table[CLEAR_BANS]		= (Function_ptr) BX_clear_bans;
	global_table[REMOVE_CHANNEL]		= (Function_ptr) BX_remove_channel;
	global_table[REMOVE_FROM_CHANNEL]	= (Function_ptr) BX_remove_from_channel;
	global_table[RENAME_NICK]		= (Function_ptr) BX_rename_nick;
	global_table[GET_CHANNEL_OPER]		= (Function_ptr) BX_get_channel_oper;
	global_table[GET_CHANNEL_HALFOP]	= (Function_ptr) BX_get_channel_halfop;
	global_table[FETCH_USERHOST]		= (Function_ptr) BX_fetch_userhost;
	global_table[GET_CHANNEL_VOICE]		= (Function_ptr) BX_get_channel_voice;
	global_table[CREATE_CHANNEL_LIST]	= (Function_ptr) BX_create_channel_list;
	global_table[FLUSH_CHANNEL_STATS]	= (Function_ptr) BX_flush_channel_stats;
	global_table[LOOKUP_CHANNEL]		= (Function_ptr) BX_lookup_channel;
				
/* hash.c */
	global_table[FIND_NICKLIST_IN_CHANNELLIST]= (Function_ptr) BX_find_nicklist_in_channellist;
	global_table[ADD_NICKLIST_TO_CHANNELLIST]= (Function_ptr) BX_add_nicklist_to_channellist;
	global_table[NEXT_NICKLIST] 		= (Function_ptr) BX_next_nicklist;
	global_table[NEXT_NAMELIST]		= (Function_ptr) BX_next_namelist;
	global_table[ADD_NAME_TO_GENERICLIST]	= (Function_ptr) BX_add_name_to_genericlist;
	global_table[FIND_NAME_IN_GENERICLIST]	= (Function_ptr) BX_find_name_in_genericlist;
	global_table[ADD_WHOWAS_USERHOST_CHANNEL]=(Function_ptr) BX_add_whowas_userhost_channel;
	global_table[FIND_USERHOST_CHANNEL]	= (Function_ptr) BX_find_userhost_channel;			
	global_table[NEXT_USERHOST]		= (Function_ptr) BX_next_userhost;
	global_table[SORTED_NICKLIST]		= (Function_ptr) BX_sorted_nicklist;
	global_table[CLEAR_SORTED_NICKLIST] 	= (Function_ptr) BX_clear_sorted_nicklist;
	global_table[ADD_NAME_TO_FLOODLIST]	= (Function_ptr) BX_add_name_to_floodlist;
	global_table[FIND_NAME_IN_FLOODLIST]	= (Function_ptr) BX_find_name_in_floodlist;		
	global_table[REMOVE_OLDEST_WHOWAS_HASHLIST]=(Function_ptr) BX_remove_oldest_whowas_hashlist;

/* vars.h cset.c fset.c */
	global_table[FGET_STRING_VAR]		= (Function_ptr) BX_fget_string_var;
	global_table[FSET_STRING_VAR]		= (Function_ptr) BX_fset_string_var;
	global_table[GET_WSET_STRING_VAR]	= (Function_ptr) BX_get_wset_string_var;
	global_table[SET_WSET_STRING_VAR]	= (Function_ptr) BX_set_wset_string_var;
	global_table[SET_CSET_INT_VAR]		= (Function_ptr) BX_set_cset_int_var;
	global_table[GET_CSET_INT_VAR]		= (Function_ptr) BX_get_cset_int_var;
	global_table[SET_CSET_STR_VAR]		= (Function_ptr) BX_set_cset_str_var;
	global_table[GET_CSET_STR_VAR]		= (Function_ptr) BX_get_cset_str_var;
#ifdef WANT_DLL
	global_table[GET_DLLINT_VAR]		= (Function_ptr) BX_get_dllint_var;
	global_table[SET_DLLINT_VAR]		= (Function_ptr) BX_set_dllint_var;
	global_table[GET_DLLSTRING_VAR]		= (Function_ptr) BX_get_dllstring_var;
	global_table[SET_DLLSTRING_VAR]		= (Function_ptr) BX_set_dllstring_var;
#else
	global_table[GET_DLLINT_VAR]		= (Function_ptr) null_function;
	global_table[SET_DLLINT_VAR]		= (Function_ptr) null_function;
	global_table[GET_DLLSTRING_VAR]		= (Function_ptr) null_function;
	global_table[SET_DLLSTRING_VAR]		= (Function_ptr) null_function;
#endif
	global_table[GET_INT_VAR]		= (Function_ptr) BX_get_int_var;
	global_table[SET_INT_VAR]		= (Function_ptr) BX_set_int_var;
	global_table[GET_STRING_VAR]		= (Function_ptr) BX_get_string_var;
	global_table[SET_STRING_VAR]		= (Function_ptr) BX_set_string_var;

/* timer.c */
	global_table[ADD_TIMER]			= (Function_ptr) BX_add_timer;
	global_table[DELETE_TIMER]		= (Function_ptr) BX_delete_timer;
	global_table[DELETE_ALL_TIMERS]		= (Function_ptr) BX_delete_all_timers;	

/* socket functions */
	global_table[ADD_SOCKETREAD]		= (Function_ptr) BX_add_socketread;
	global_table[ADD_SOCKETTIMEOUT]		= (Function_ptr) BX_add_sockettimeout;
	global_table[CLOSE_SOCKETREAD]		= (Function_ptr) BX_close_socketread;
	global_table[GET_SOCKET]		= (Function_ptr) BX_get_socket;
	global_table[SET_SOCKETFLAGS]		= (Function_ptr) BX_set_socketflags;
	global_table[GET_SOCKETFLAGS]		= (Function_ptr) BX_get_socketflags;
	global_table[GET_SOCKETINFO]		= (Function_ptr) BX_get_socketinfo;
	global_table[SET_SOCKETINFO]		= (Function_ptr) BX_set_socketinfo;
	global_table[SET_SOCKETWRITE]		= (Function_ptr) BX_set_socketwrite;
	global_table[CHECK_SOCKET]		= (Function_ptr) BX_check_socket;
	global_table[READ_SOCKETS]		= (Function_ptr) BX_read_sockets;
	global_table[WRITE_SOCKETS]		= (Function_ptr) BX_write_sockets;
	global_table[GET_MAX_FD]		= (Function_ptr) BX_get_max_fd;
	global_table[NEW_CLOSE]			= (Function_ptr) BX_new_close;
	global_table[NEW_OPEN]			= (Function_ptr) BX_new_open;	
	global_table[DGETS]			= (Function_ptr) BX_dgets;

/* network.c */
	global_table[CONNECT_BY_NUMBER]		= (Function_ptr) BX_connect_by_number;
	global_table[RESOLV]			= (Function_ptr) BX_lookup_host;
	global_table[LOOKUP_HOST]		= (Function_ptr) BX_lookup_host;
	global_table[LOOKUP_IP]			= (Function_ptr) BX_host_to_ip;
	global_table[HOST_TO_IP]		= (Function_ptr) BX_host_to_ip;
	global_table[IP_TO_HOST]		= (Function_ptr) BX_ip_to_host;
	global_table[ONE_TO_ANOTHER]		= (Function_ptr) BX_one_to_another;
	global_table[SET_BLOCKING]		= (Function_ptr) BX_set_blocking;
	global_table[SET_NON_BLOCKING]		= (Function_ptr) BX_set_non_blocking;
	
/* flood.c */
	global_table[IS_OTHER_FLOOD]		= (Function_ptr) BX_is_other_flood;
	global_table[CHECK_FLOODING]		= (Function_ptr) BX_check_flooding;
	global_table[FLOOD_PROT]		= (Function_ptr) BX_flood_prot;

/* alias.c */
	global_table[NEXT_UNIT]			= (Function_ptr) BX_next_unit;
	global_table[EXPAND_ALIAS]		= (Function_ptr) BX_expand_alias;
	global_table[PARSE_INLINE]		= (Function_ptr) BX_parse_inline;
	global_table[ALIAS_SPECIAL_CHAR]	= (Function_ptr) BX_alias_special_char;
	global_table[PARSE_LINE]		= (Function_ptr) BX_parse_line;
	global_table[PARSE_COMMAND_FUNC]	= (Function_ptr) BX_parse_command;
	global_table[MAKE_LOCAL_STACK]		= (Function_ptr) BX_make_local_stack;
	global_table[DESTROY_LOCAL_STACK]	= (Function_ptr) BX_destroy_local_stack;

/* dcc.c */
	global_table[DCC_CREATE_FUNC]		= (Function_ptr) BX_dcc_create;
	global_table[FIND_DCC_FUNC]		= (Function_ptr) BX_find_dcc;
	global_table[ERASE_DCC_INFO]		= (Function_ptr) BX_erase_dcc_info;
	global_table[ADD_DCC_BIND]		= (Function_ptr) BX_add_dcc_bind;
	global_table[REMOVE_DCC_BIND]		= (Function_ptr) BX_remove_dcc_bind;
	global_table[REMOVE_ALL_DCC_BINDS]	= (Function_ptr) BX_remove_all_dcc_binds;
	global_table[GET_ACTIVE_COUNT]		= (Function_ptr) BX_get_active_count;
	global_table[DCC_FILESEND]		= (Function_ptr) BX_dcc_filesend;
	global_table[DCC_RESEND]		= (Function_ptr) BX_dcc_resend;

/* cdcc.c */
	global_table[GET_NUM_QUEUE]		= (Function_ptr) BX_get_num_queue;
	global_table[ADD_TO_QUEUE]		= (Function_ptr) BX_add_to_queue;

/* who.c */
	global_table[WHOBASE]			= (Function_ptr) BX_whobase;
	global_table[ISONBASE]			= (Function_ptr) BX_isonbase;
	global_table[USERHOSTBASE]		= (Function_ptr) BX_userhostbase;
	
			
	global_table[SERV_OPEN_FUNC]		= (Function_ptr) &serv_open_func;
	global_table[SERV_OUTPUT_FUNC]		= (Function_ptr) &serv_output_func;
	global_table[SERV_INPUT_FUNC]		= (Function_ptr) &serv_input_func;
	global_table[SERV_CLOSE_FUNC]		= (Function_ptr) &serv_close_func;
	global_table[DEFAULT_OUTPUT_FUNCTION]	= (Function_ptr) &default_output_function;
	global_table[DEFAULT_STATUS_OUTPUT_FUNCTION]=(Function_ptr) &default_status_output_function;


/* important variables */
	global_table[NICKNAME]			= (Function_ptr) &nickname;
	global_table[IRC_VERSION]		= (Function_ptr) &irc_version;
	global_table[FROM_SERVER]		= (Function_ptr) &from_server;
	global_table[CONNECTED_TO_SERVER]	= (Function_ptr) &connected_to_server;
	global_table[PRIMARY_SERVER]		= (Function_ptr) &primary_server;
	global_table[PARSING_SERVER_INDEX]	= (Function_ptr) &parsing_server_index;
	global_table[NOW]			= (Function_ptr) &now;
	global_table[START_TIME]		= (Function_ptr) &start_time;
	global_table[IDLE_TIME]			= (Function_ptr) &idle_time;

	global_table[LOADING_GLOBAL]		= (Function_ptr) &loading_global;

	global_table[TARGET_WINDOW]		= (Function_ptr) &target_window;
	global_table[CURRENT_WINDOW]		= (Function_ptr) &current_window;
	global_table[INVISIBLE_LIST]		= (Function_ptr) &invisible_list;
	global_table[MAIN_SCREEN]		= (Function_ptr) &main_screen;
	global_table[LAST_INPUT_SCREEN]		= (Function_ptr) &last_input_screen;
	global_table[OUTPUT_SCREEN]		= (Function_ptr) &output_screen;
	global_table[SCREEN_LIST]		= (Function_ptr) &screen_list;
	global_table[DOING_NOTICE]      = (Function_ptr) &doing_notice;
	global_table[SENT_NICK]         = 0;	/* No longer used */
	global_table[LAST_SENT_MSG_BODY]    = 0;	/* No longer used */
                                
	global_table[IRCLOG_FP]			= (Function_ptr) &irclog_fp;
#ifdef WANT_DLL
	global_table[DLL_FUNCTIONS]		= (Function_ptr) &dll_functions;
	global_table[DLL_NUMERIC]		= (Function_ptr) &dll_numeric_list;
	global_table[DLL_COMMANDS]		= (Function_ptr) &dll_commands;
	global_table[DLL_WINDOW]		= (Function_ptr) &dll_window;
	global_table[DLL_VARIABLE]		= (Function_ptr) &dll_variable;
	global_table[DLL_CTCP]			= (Function_ptr) &dll_ctcp;
#endif
	global_table[WINDOW_DISPLAY]		= (Function_ptr) &window_display;
	global_table[STATUS_UPDATE_FLAG]	= (Function_ptr) &status_update_flag;
	global_table[TABKEY_ARRAY]		= (Function_ptr) &tabkey_array;
	global_table[AUTOREPLY_ARRAY]		= (Function_ptr) &autoreply_array;
	global_table[IDENTD_SOCKET]		= (Function_ptr) &identd;
		
#ifdef WANT_TCL
	global_table[VAR_TCL_INTERP]		= (Function_ptr) tcl_interp;
#endif

#ifdef GUI
	global_table[LASTCLICKLINEDATA]         = (Function_ptr) &lastclicklinedata;
	global_table[CONTEXTX]                  = (Function_ptr) &contextx;
	global_table[CONTEXTY]                  = (Function_ptr) &contexty;
	global_table[GUIIPC]                    = (Function_ptr) &(guiipc[1]);
	global_table[GUI_MUTEX_LOCK]		= (Function_ptr) BX_gui_mutex_lock;
	global_table[GUI_MUTEX_UNLOCK]		= (Function_ptr) BX_gui_mutex_unlock;
#else
	global_table[GUI_MUTEX_LOCK]		= (Function_ptr) null_function;
	global_table[GUI_MUTEX_UNLOCK]		= (Function_ptr) null_function;
#endif
/* commands.c */
	global_table[FIND_COMMAND_FUNC]		= (Function_ptr) BX_find_command;
	
#ifdef MEM_DEBUG
	{
		int i;
		for (i = 0; i < NUMBER_OF_GLOBAL_FUNCTIONS; i++)
			if (global_table[i] == NULL)
				put_it("global table %d is NULL", i);
	}
#endif
}



int BX_check_module_version(unsigned long number)
{
	if (number != MODULE_VERSION)
		return 0;
	return 1;
}

#ifdef WANT_DLL
char *BX_get_dllstring_var(char *typestr)
{
IrcVariableDll *dll = NULL;
	if (typestr)
		dll = (IrcVariableDll *) find_in_list((List **)&dll_variable, typestr, 0);
	return (dll?dll->string:NULL);
}


int BX_get_dllint_var(char *typestr)
{
IrcVariableDll *dll = NULL;
	if (typestr)
		dll = (IrcVariableDll *) find_in_list((List **)&dll_variable, typestr, 0);
	return (dll?dll->integer:-1);
}

void BX_set_dllstring_var(char *typestr, char *string)
{
	if (typestr)
	{
		IrcVariableDll *dll = NULL;
		if (typestr)
			dll = (IrcVariableDll *) find_in_list((List **)&dll_variable, typestr, 0);
		if (!dll)
			return;
		if (string)
			malloc_strcpy(&dll->string, string);
		else
			new_free(&dll->string);
	}
}

void BX_set_dllint_var(char *typestr, unsigned int value)
{
	if (typestr)
	{
		IrcVariableDll *dll = NULL;
		if (typestr)
			dll = (IrcVariableDll *) find_in_list((List **)&dll_variable, typestr, 0);
		if (!dll)
			return;
		dll->integer = value;
	}
}

BUILT_IN_COMMAND(dll_load)
{
#if defined(HPUX)  /* 	HP machines */
	shl_t handle = NULL;
#elif defined(__EMX__)
	ULONG ulerror;
	HMODULE handle;
#elif defined(WINNT)
	HINSTANCE handle;
#else		     /*		linux SunOS AIX etc */
	void *handle = NULL;
#endif
    
char *filename = NULL;
Irc_PackageInitProc *proc1Ptr;
Irc_PackageVersionProc *proc2Ptr;
char *f, *p, *procname = NULL;
int code = 0;

	if (command)
	{
		if (install_pack)
		{
			Packages *pkg = install_pack;
			List *pk;
			for ( ; pkg; pkg = pkg->next)
				put_it("DLL [%s%s%s] installed", pkg->name, pkg->version?space:empty_string, pkg->version?pkg->version:empty_string);
			for (pk = (List *)dll_commands; pk; pk = pk->next)
				put_it("\t%10s\t%s", "Command", pk->name);
			for (pk = (List *)dll_functions; pk; pk = pk->next)
				put_it("\t%10s\t%s", "Alias", pk->name);
			for (pk = (List *)dll_ctcp; pk; pk = pk->next)
				put_it("\t%10s\t%s", "Ctcp", pk->name);
			for (pk = (List *)dll_variable; pk; pk = pk->next)
				put_it("\t%10s\t%s", "Variable", pk->name);		
			for (pk = (List *)dll_window; pk; pk = pk->next)
				put_it("\t%10s\t %s", "Window", pk->name);
		}
		else
			bitchsay("No modules loaded");
		return;
	}
	if (!args || !*args)
		return;

	filename = next_arg(args, &args);
	f = expand_twiddle(filename);
	
	if ((p = strrchr(filename, '/')))	
		p++;
	else
	{
		new_free(&f);
		if (!(p = path_search(filename, PLUGINDIR)))
			if (!(p = path_search(filename, get_string_var(LOAD_PATH_VAR))))
			{
				char file_buf[BIG_BUFFER_SIZE];
				strcpy(file_buf, filename);
				strcat(file_buf, SHLIB_SUFFIX);
				if (!(p = path_search(file_buf, get_string_var(LOAD_PATH_VAR))))
					if (!(p = path_search(file_buf, PLUGINDIR)))
						p = filename;
			}
		f = expand_twiddle(p);
		p = filename;
		
	}

	procname  = m_strdup(p);
	if ((p = strchr(procname, '.')))
		*p = 0;

	p = procname;
	*p = toupper(*p);
	p++;
	while (p && *p)
	{
		*p = tolower(*p);
		p++;
	}

	if (!procname || find_in_list((List **)&install_pack, procname, 0))
	{
		if (procname)
			bitchsay("Module [%s] Already installed", procname);
		new_free(&f);
		new_free(&procname);
		return;
	}
	malloc_strcat(&procname, "_Init");

#if defined(HPUX)
	handle = shl_load(f, BIND_IMMEDIATE, 0L);
#elif defined(__osf1__)
	handle = dlopen(f, RTLD_NOW);
#elif defined(__EMX__)
        malloc_strcat(&f, ".dll");
        convert_dos(f);
	ulerror = DosLoadModule(NULL, 0, f, &handle);
#elif defined(WINNT)
        malloc_strcat(&f, ".dll");
	convert_dos(f);
	handle = LoadLibrary(f);
#else
	handle = dlopen(f, RTLD_NOW | RTLD_GLOBAL);
#endif
#if defined(__EMX__)
	if (ulerror)
#else
	if (handle == NULL)
#endif
	{
#if defined(__EMX__)
		bitchsay("couldn't load file: DosLoadModule() failed %d", ulerror);
#elif defined(WINNT)
		bitchsay("could't load file %s", f);
#else
		bitchsay("couldn't load file: %s", dlerror());
#endif
		new_free(&procname);
		new_free(&f);
		return;
	}

#if defined(HPUX) 
	if (!shl_findsym(&handle, procname, (short) TYPE_PROCEDURE, (void *) proc1Ptr))
#elif defined(__EMX__)
	if(DosQueryProcAddr(handle, 0, procname, (PFN *) &proc1Ptr)) 
#elif defined(WINNT)
	if (!(proc1Ptr = (Irc_PackageInitProc *) GetProcAddress(handle, procname)))
#else
	if (!(proc1Ptr = (Irc_PackageInitProc *) dlsym(handle, (char *) procname)))
#endif
		bitchsay("UnSuccessful module load");
	else
		code = (proc1Ptr)(&dll_commands, global_table);

	if (!code && proc1Ptr)
	{
		Packages *new;
		new = (Packages *) new_malloc(sizeof(Packages));
		new->name = m_strdup(procname);
		new->handle = handle;
		new->major = bitchx_numver / 10000;
		new->minor = (bitchx_numver / 100) % 100;

		if ((p = strrchr(new->name, '_')))
			*p = 0;
		p = m_sprintf("%s_Version", new->name);
#if defined(__EMX__)
		if (!DosQueryProcAddr(handle, 0, p, (PFN *) &proc2Ptr))
#elif defined(WINNT)
		if ((proc2Ptr = (Irc_PackageVersionProc *) GetProcAddress(handle, p)))
#elif defined(HPUX)
               if (shl_findsym(&handle, p, (short) TYPE_PROCEDURE, (void *)proc2Ptr))
#else
		if ((proc2Ptr = (Irc_PackageVersionProc *) dlsym(handle, p)))
#endif
			new->version = m_strdup(((proc2Ptr)(&dll_commands)));
		new_free(&p);
		
		if ((p = strrchr(new->name, '_')))
			*p = 0;
		p = m_sprintf("%s_Cleanup", new->name);
#if defined(__EMX__)
		if(!DosQueryProcAddr(handle, 0, p, (PFN *) &proc1Ptr))
#elif defined(WINNT)
		if ((proc1Ptr = (Irc_PackageInitProc *) GetProcAddress(handle, p)))
#elif defined(HPUX)
		if (shl_findsym(&handle, p, (short) TYPE_PROCEDURE, (void *)proc1Ptr))
#else
		if ((proc1Ptr = (Irc_PackageInitProc *) dlsym(handle, p)))
#endif
			new->cleanup = proc1Ptr;

		new_free(&p);
		if ((p = strrchr(new->name, '_')))
			*p = 0;
		p = m_sprintf("%s_Lock", new->name);
#if defined(__EMX__)
		if(!DosQueryProcAddr(handle, 0, p, (PFN *) &proc1Ptr))
#elif defined(WINNT)
		if ((proc1Ptr = (Irc_PackageInitProc *) GetProcAddress(handle, p)))
#elif defined(HPUX)
		if (shl_findsym(&handle, p, (short) TYPE_PROCEDURE, (void *)proc1Ptr))
#else
		if ((proc1Ptr = (Irc_PackageInitProc *) dlsym(handle, p)))
#endif
			new->lock = 1;
		new_free(&p);
		add_to_list((List **)&install_pack, (List *)new);
	}
	else 
	{
		if (code == INVALID_MODVERSION)
			bitchsay("Error module version is wrong for [%s]", procname);
		else
			bitchsay("Error initiliziing module [%s:%d]", procname, code);
		if (handle)
#if defined(__EMX__)
			DosFreeModule(handle);
#elif defined(WINNT)
			FreeLibrary(handle);
#elif defined(HPUX)
			shl_unload(handle);
#else
			dlclose(handle);
#endif
	}
	new_free(&procname);
	new_free(&f);
}

Packages *find_module(char *name)
{
Packages *new = NULL;
	if (name) 
		new = (Packages *)find_in_list((List **)&install_pack, name, 0);
	return new;
}

#define RAWHASH_SIZE 20

HashEntry RawHash[RAWHASH_SIZE] = {{ NULL, 0, 0 }};

RawDll *find_raw_proc(char *comm, char **ArgList)
{
	RawDll *tmp = NULL;
	if ((tmp = (RawDll *)find_name_in_genericlist(comm, RawHash, RAWHASH_SIZE, 0)))
		return tmp;
	return NULL;
}




int BX_add_module_proc(unsigned int mod_type, char *modname, char *procname, char *desc, int id, int flag, void *func1, void *func2)
{
	switch(mod_type)
	{
		case COMMAND_PROC:
		{
			IrcCommandDll *new;
			new = (IrcCommandDll *) new_malloc(sizeof(IrcCommandDll));
			new->name = m_strdup(procname);
			if (desc)
				new->server_func = m_strdup(desc);
			new->func = func1;
			if (func2)
				new->help = m_strdup((char *)func2);
			new->module = m_strdup(modname);
			add_to_list((List **)&dll_commands, (List *)new);
			break;
		}
		case WINDOW_PROC:
		{
			WindowDll *new;
			new = (WindowDll *) new_malloc(sizeof(WindowDll));
			new->name = m_strdup(procname);
			if (desc)
				new->help = m_strdup(desc);
			new->func = func1;
			new->module = m_strdup(modname);
			add_to_list((List **)&dll_window, (List *)new);
			break;
		}
		case OUTPUT_PROC:
		{
			Screen *screen;
			Window *window;
			if (func1)
			{
				default_output_function = func1;
				for (screen = screen_list; screen; screen = screen->next)
				{
					for (window = screen->window_list; window; window = window->next)
						window->output_func = func1;
				}
			}
			if (func2)
			{
				default_status_output_function = func2;
				for (screen = screen_list; screen; screen = screen->next)
				{
					for (window = screen->window_list; window; window = window->next)
						window->status_output_func = func2;
				}
			}
			break;
		}
		case ALIAS_PROC:
		{
			BuiltInDllFunctions *new = NULL;
			new = (BuiltInDllFunctions *) new_malloc(sizeof(BuiltInDllFunctions));
			new->name = m_strdup(procname);
			new->func = func1;
			new->module = m_strdup(modname);
			add_to_list((List **)&dll_functions, (List *)new);
			break;
		}
		case CTCP_PROC:
		{
			CtcpEntryDll *new = NULL;
			new = (CtcpEntryDll *) new_malloc(sizeof(CtcpEntryDll));
			new->name = m_strdup(procname);
			new->desc = m_strdup(desc);
			new->id = id;
			new->flag = flag;
			new->func = func1;
			new->repl = func2;
			new->module = m_strdup(modname);
			add_to_list((List **)&dll_ctcp, (List *)new);
			break;
		}
		case VAR_PROC:
		{
			IrcVariableDll *new = NULL;
			new = (IrcVariableDll *) new_malloc(sizeof(IrcVariableDll));
			new->type = id;
			new->integer = flag;
			new->name = m_strdup(procname);
			if (desc)
				new->string = m_strdup(desc);
			new->module = m_strdup(modname);
			new->func = func1;
			add_to_list((List **)&dll_variable, (List *)new);
			break;
		}
		case RAW_PROC:
		{
			RawDll *raw;
			unsigned long hvalue = hash_nickname(procname, RAWHASH_SIZE);
			raw = (RawDll *)new_malloc(sizeof(RawDll)); 
			raw->next = (RawDll *)RawHash[hvalue].list;
			RawHash[hvalue].list = (void *)raw;
			RawHash[hvalue].links++;
			RawHash[hvalue].hits++;
			raw->name = m_strdup(procname);
			raw->module = m_strdup(modname);
			raw->func = func1;
			break;
		}
		case HOOK_PROC:
		{
extern void add_dll_hook(int , int, char *, char *, int (*func1)(int, char *, char **),int (*func2)(char *, char *, char **));
			add_dll_hook(id, flag, desc? desc : "*", modname, func1, func2);
			break;
		}
		case DCC_PROC:
		{
			DCC_dllcommands *new = NULL;
			new = (DCC_dllcommands *) new_malloc(sizeof(DCC_dllcommands));
			new->name = m_strdup(procname);
			new->help = m_strdup(desc);
			new->module = m_strdup(modname);
			new->function = func1;
			add_to_list((List **)&dcc_dllcommands, (List *)new);
			break;
		}
		default:
		{
			if ((mod_type & ~TABLE_PROC) > 0 && (mod_type & ~TABLE_PROC) < NUMBER_OF_GLOBAL_FUNCTIONS)
				global_table[mod_type & ~TABLE_PROC] = func1;
		}
	}
	return 0;
}

typedef struct _list2 
{
	struct _list2 *next;
	char	*command;
	char	*name;
} List2;

static List2	*remove_module(List2 **list, char *name)
{
register	List2	*tmp;
		List2 	*last;

	last = NULL;
	for (tmp = *list; tmp; tmp = tmp->next)
	{
		if (tmp->name && !strcasecmp(tmp->name, name))
		{
			if (last)
				last->next = tmp->next;
			else
				*list = tmp->next;
			return (tmp);
		}
		last = tmp;
	}
	return NULL;
}


int BX_remove_module_proc(unsigned int mod_type, char *modname, char *procname, char *desc)
{
int count = 0;
	switch(mod_type)
	{
		case COMMAND_PROC:
		{
			IrcCommandDll *ptr;
			while ((ptr = (IrcCommandDll *)remove_module((List2 **)&dll_commands, modname)))
			{
				new_free(&ptr->name);
				new_free(&ptr->module);
				new_free(&ptr->server_func);
				new_free(&ptr->result);
				new_free(&ptr->help);
				new_free((char **)&ptr);
				count++;
			}
			break;
		}
		case WINDOW_PROC:
		{
			WindowDll *ptr;
			while ((ptr = (WindowDll *)remove_module((List2 **)&dll_window, modname)))
			{
				new_free(&ptr->name);
				new_free(&ptr->module);
				new_free(&ptr->help);
				new_free((char **)&ptr);
				count++;
			}
			break;
		}
		case OUTPUT_PROC:
		{
			Screen *screen;
			Window *window;
			default_output_function = BX_add_to_window;
			for (screen = screen_list; screen; screen = screen->next)
			{
				for (window = screen->window_list; window; window = window->next)
					window->output_func = BX_add_to_window;
			}
			default_status_output_function = make_status;
			for (screen = screen_list; screen; screen = screen->next)
			{
				for (window = screen->window_list; window; window = window->next)
					window->status_output_func = make_status;
			}
			count++;
			break;
		}
		case ALIAS_PROC:
		{
			BuiltInDllFunctions *ptr;
			while ((ptr = (BuiltInDllFunctions *)remove_module((List2 **)&dll_functions, modname)))
			{
				new_free(&ptr->name);
				new_free(&ptr->module);
				new_free((char **)&ptr);
				count++;
			}
			break;
		}
		case CTCP_PROC:
		{
			CtcpEntryDll *ptr;
			while ((ptr = (CtcpEntryDll *)remove_module((List2 **)&dll_ctcp, modname)))
			{
				new_free(&ptr->name);
				new_free(&ptr->module);
				new_free(&ptr->desc);
				new_free((char **)&ptr);
				count++;
			}
			break;
		}
		case VAR_PROC:
		{
			IrcVariableDll *ptr;
			while ((ptr = (IrcVariableDll *)remove_module((List2 **)&dll_variable, modname)))
			{
				new_free(&ptr->name);
				new_free(&ptr->module);
				new_free(&ptr->string);
				new_free((char **)&ptr);
				count++;
			}
			break;
		}
		case RAW_PROC:
		{
			int i;
			RawDll *ptr = NULL;
			for (i = 0; i < RAWHASH_SIZE; i++)
			{
				while ((ptr = (RawDll *)remove_module((List2 **)&(RawHash[i].list), modname)))
				{
					new_free(&ptr->module);
					new_free(&ptr->name);
					new_free((char **)&ptr);
					count++;
				}
			}
			break;
		}
		case HOOK_PROC:
		{
			extern int remove_dll_hook(char *);
			count = remove_dll_hook(modname);
			break;
		}
		case DCC_PROC:
		{
			DCC_dllcommands *ptr;
			while ((ptr = (DCC_dllcommands *)remove_module((List2 **)&dcc_dllcommands, modname)))
			{
				new_free(&ptr->name);
				new_free(&ptr->module);
				new_free(&ptr->help);
				new_free((char **)&ptr);
				count++;
			}
			break;
		}
		default:
		{
			if ((mod_type & ~TABLE_PROC) > 0 && (mod_type & ~TABLE_PROC)< NUMBER_OF_GLOBAL_FUNCTIONS)
			{
				global_table[mod_type & ~TABLE_PROC] = NULL;
				count++;
			}
		}
	}
	return count;
}

int remove_package(char *name)
{
Packages *new = NULL;
	if ((new = (Packages *) remove_from_list((List **)&install_pack, name)))
	{
#if defined(__EMX__)
		DosFreeModule(new->handle);
#elif defined(WINNT)
		FreeLibrary(new->handle);
#elif defined(HPUX)
		shl_unload(new->handle);
#else
		dlclose(new->handle);
#endif
		new_free(&new->name);
		new_free(&new->version);
		new_free((char **)&new);
		return 1;
	}
	return  0;
}

int check_version(unsigned long required)
{
unsigned long major, minor, need_major, need_minor;
	major = bitchx_numver / 10000;
	minor = (bitchx_numver / 100) % 100;
	need_major = required / 10000;
	need_minor = (required / 100) % 100;
	if ((major > need_major) || (major == need_major && minor >= need_minor))
		return 1;
	return INVALID_MODVERSION;
}

BUILT_IN_COMMAND(unload_dll)
{
Packages *new = NULL;
int success = 0;
char *name;
	name = next_arg(args, &args);
	if (name && (new = find_module(name)))
	{
		if (new->cleanup)
			success = (new->cleanup)(&dll_commands, global_table);
		else
		{
			if (new->lock)
			{
				bitchsay("Module is locked");
				return;
			}
			success += remove_module_proc(COMMAND_PROC, name, NULL, NULL);
			success += remove_module_proc(ALIAS_PROC, name, NULL, NULL);
			success += remove_module_proc(CTCP_PROC, name, NULL, NULL);
			success += remove_module_proc(VAR_PROC, name, NULL, NULL);
			success += remove_module_proc(HOOK_PROC, name, NULL, NULL);
			success += remove_module_proc(RAW_PROC, name, NULL, NULL);
			success += remove_module_proc(WINDOW_PROC, name, NULL, NULL);
			success += remove_module_proc(OUTPUT_PROC, name, NULL, NULL);
			success += remove_module_proc(DCC_PROC, name, NULL, NULL);
			success += remove_all_dcc_binds(name);
		}
		if (success)
		{
			remove_package(name);
			put_it("%s", convert_output_format("$G Successfully removed [$0 ($1)]", "%s %d", name, success));
		} else
			put_it("%s", convert_output_format("$G Unsuccessful module unload", NULL, NULL));
	} else
		bitchsay("No such module loaded");
}

int add_module(unsigned int mod_type, Function * table, char *modname)
{
Function *p = table;
int count = 0;

	while(p)
	{
		add_module_proc(mod_type, modname, p->name, p->desc, p->id, p->flag, p->func1, p->func2);
		p++;count++;
	}
	return count;
}
#endif

