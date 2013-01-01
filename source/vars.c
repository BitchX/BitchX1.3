/*
 * vars.c: All the dealing of the irc variables are handled here. 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: vars.c 160 2012-03-06 11:14:51Z keaston $";
CVS_REVISION(vars_c)
#include "struct.h"

#include "status.h"
#include "window.h"
#include "lastlog.h"
#include "log.h"
#include "encrypt.h"
#include "history.h"
#include "notify.h"
#include "vars.h"
#include "input.h"
#include "ircaux.h"
#include "who.h"
#include "ircterm.h"
#include "output.h"
#include "screen.h"
#include "list.h"
#include "server.h"
#include "window.h"
#include "misc.h"
#include "stack.h"
#include "alist.h"
#include "hash2.h"
#include "cset.h"
#include "module.h"
#include "tcl_bx.h"
void add_tcl_vars(void);


#ifdef WANT_CD
#include "cdrom.h"
#endif
#ifdef TRANSLATE
#include "translat.h"
#endif
#define MAIN_SOURCE
#include "modval.h"

#if defined(WINNT)
#include <windows.h>
extern DWORD gdwPlatform;
#endif

char	*var_settings[] =
{
	"OFF", "ON", "TOGGLE"
};


extern	char	*auto_str;

extern  Screen	*screen_list;

extern  struct	in_addr nat_address;
extern	int	use_nat_address;

int	loading_global = 0;


enum VAR_TYPES	find_variable (char *, int *);
static	void	exec_warning (Window *, char *, int);
static	void	eight_bit_characters (Window *, char *, int);
static	void	set_realname (Window *, char *, int);
static	void	set_numeric_string (Window *, char *, int);
static	void	set_user_mode (Window *, char *, int);
static	void	set_ov_mode (Window *, char *, int);
static	void	set_away_time (Window *, char *, int);
static	void	reinit_screen (Window *, char *, int);
	void	reinit_status (Window *, char *, int);
	int	save_formats (FILE *);
static	void	set_clock_format (Window *, char *, int);
	void	create_fsets (Window *, int);
	void	setup_ov_mode (int, int, int);
	void	convert_swatch (Window *, char *, int);	
	void	toggle_reverse_status(Window *, char *, int);
static	void	set_dcc_timeout(Window *, char *, int);
	void	BX_set_scrollback_size (Window *, char *, int);
static	void	set_use_socks(Window *, char *, int);

static	void	set_mangle_inbound (Window *, char *, int);
static	void	set_mangle_outbound (Window *, char *, int);
static	void	set_mangle_logfiles (Window *, char *, int);
static	void	set_mangle_operlog (Window *, char *, int);
static	void	set_nat_address (Window *, char *, int);
extern	void	debug_window (Window *, char *, int);
	
extern	void	set_detach_on_hup (Window *, char *, int);

/*
 * irc_variable: all the irc variables used.  Note that the integer and
 * boolean defaults are set here, which the string default value are set in
 * the init_variables() procedure 
 */

static	IrcVariable irc_variable[] =
{
	{ "AINV",0,			INT_TYPE_VAR,	DEFAULT_AINV, NULL, NULL, 0, VIF_BITCHX },
	{ "ALTNICK",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "ALT_CHARSET", 0,		BOOL_TYPE_VAR,	DEFAULT_ALT_CHARSET, NULL, NULL, 0, VIF_BITCHX },
	{ "ALWAYS_SPLIT_BIGGEST",0,	BOOL_TYPE_VAR,	DEFAULT_ALWAYS_SPLIT_BIGGEST, NULL, NULL, 0, 0 },
	{ "ANNOY_KICK",0,		BOOL_TYPE_VAR,	DEFAULT_ANNOY_KICK, NULL, NULL, 0, VIF_BITCHX },
	{ "AOP",0,			BOOL_TYPE_VAR,	DEFAULT_AOP_VAR, NULL, NULL, 0, VIF_BITCHX  },
	{ "APPEND_LOG",0,		BOOL_TYPE_VAR,	1, NULL, NULL, 0, VIF_BITCHX }, 
	
	{ "AUTOKICK_ON_VERSION",0,	BOOL_TYPE_VAR,  0, NULL, NULL, 0, VIF_BITCHX },
	{ "AUTO_AWAY",0,		BOOL_TYPE_VAR,  DEFAULT_AUTO_AWAY, NULL, NULL, 0, VIF_BITCHX },
	{ "AUTO_AWAY_TIME",0,		INT_TYPE_VAR,	DEFAULT_AUTO_AWAY_TIME, NULL, set_away_time, 0, VIF_BITCHX  },
	{ "AUTO_JOIN_ON_INVITE",0,	BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "AUTO_LIMIT",0,		INT_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "AUTO_NSLOOKUP",0,		BOOL_TYPE_VAR,  DEFAULT_AUTO_NSLOOKUP, NULL, NULL, 0, VIF_BITCHX  },
	{ "AUTO_RECONNECT",0,		BOOL_TYPE_VAR,	DEFAULT_AUTO_RECONNECT, NULL, NULL, 0, VIF_BITCHX },
	{ "AUTO_REJOIN",0,		INT_TYPE_VAR,   DEFAULT_AUTO_REJOIN, NULL, NULL, 0, VIF_BITCHX  },
	{ "AUTO_RESPONSE",0,		BOOL_TYPE_VAR,	1, NULL, NULL, 0, VIF_BITCHX },
	{ "AUTO_RESPONSE_STR",0,	STR_TYPE_VAR,	0, NULL, reinit_autoresponse, 0, VIF_BITCHX },
	{ "AUTO_UNBAN",0,		INT_TYPE_VAR,	DEFAULT_AUTO_UNBAN, NULL, NULL, 0, VIF_BITCHX },
	{ "AUTO_UNMARK_AWAY",0,		BOOL_TYPE_VAR,	DEFAULT_AUTO_UNMARK_AWAY, NULL, NULL, 0, 0 },
	{ "AUTO_WHOWAS",0,		BOOL_TYPE_VAR,	DEFAULT_AUTO_WHOWAS, NULL, NULL, 0, 0 },
	{ "BANTIME", 0, 		INT_TYPE_VAR,	DEFAULT_BANTIME, NULL, NULL, 0, VIF_BITCHX }, 
	{ "BEEP",0,			BOOL_TYPE_VAR,	DEFAULT_BEEP, NULL, NULL, 0, VIF_BITCHX },
	{ "BEEP_ALWAYS",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "BEEP_MAX",0,			INT_TYPE_VAR,	DEFAULT_BEEP_MAX, NULL, NULL, 0, 0 },
	{ "BEEP_ON_MSG",0,		STR_TYPE_VAR,	0, NULL, set_beep_on_msg, 0, VIF_BITCHX },
	{ "BEEP_WHEN_AWAY",0,		INT_TYPE_VAR,	DEFAULT_BEEP_WHEN_AWAY, NULL, NULL, 0, VIF_BITCHX },
	{ "BITCH",0,			BOOL_TYPE_VAR,  0, NULL, NULL, 0, VIF_BITCHX },
	{ "BITCHX_HELP",0,		STR_TYPE_VAR,   0, NULL, NULL, 0, VIF_BITCHX },
	{ "BLINK_VIDEO",0,		BOOL_TYPE_VAR,	DEFAULT_BLINK_VIDEO, NULL, NULL, 0, 0 },
	{ "BOLD_VIDEO",0,		BOOL_TYPE_VAR,	DEFAULT_BOLD_VIDEO, NULL, NULL, 0, 0 },
	{ "BOT_LOG",0,			BOOL_TYPE_VAR,	1, NULL, NULL, 0, VIF_BITCHX  },
	{ "BOT_LOGFILE",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX  },
	{ "BOT_MODE",0,			BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "BOT_PASSWD",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "BOT_RETURN",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "BOT_TCL",0,			BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "CDCC",0,			BOOL_TYPE_VAR,	DEFAULT_CDCC, NULL, NULL, 0, 0 },
	{ "CDCC_FLOOD_AFTER",0,		INT_TYPE_VAR,	DEFAULT_CDCC_FLOOD_AFTER, NULL, NULL, 0, VIF_BITCHX  },
	{ "CDCC_FLOOD_RATE",0,		INT_TYPE_VAR,	DEFAULT_CDCC_FLOOD_RATE,NULL, NULL, 0, VIF_BITCHX  },
	{ "CDCC_PROMPT",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "CDCC_SECURITY",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX  },
#ifdef WANT_CD
	{ "CD_DEVICE",0,		STR_TYPE_VAR,	0, NULL, set_cd_device, 0, VIF_BITCHX },
#else
	{ "CD_DEVICE",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
#endif
	{ "CHANGE_NICK_ON_KILL",0,	BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "CHANMODE",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "CHANNEL_NAME_WIDTH",0,	INT_TYPE_VAR,	DEFAULT_CHANNEL_NAME_WIDTH, NULL, BX_update_all_status, 0, 0 },
	{ "CHECK_BEEP_USERS",0,		BOOL_TYPE_VAR,  0, NULL, NULL, 0, 0 },
	{ "CLIENT_INFORMATION",0,	STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "CLOAK",0,			INT_TYPE_VAR,	DEFAULT_CLOAK, NULL, BX_update_all_status, 0, VIF_BITCHX },
	{ "CLOCK",0,			BOOL_TYPE_VAR,	DEFAULT_CLOCK, NULL, BX_update_all_status, 0, 0 },
	{ "CLOCK_24HOUR",0,		BOOL_TYPE_VAR,	DEFAULT_CLOCK_24HOUR, NULL, reset_clock, 0, VIF_BITCHX },
	{ "CLOCK_FORMAT",0,		STR_TYPE_VAR,	0, NULL, set_clock_format, 0, 0 },
	{ "CLONE_CHECK",0,		INT_TYPE_VAR,	0, NULL, NULL, 0, 0 }, 
	{ "CLONE_COUNT",0,		INT_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "CMDCHARS",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "COLOR",0,			BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
	{ "COMMAND_MODE",0,		BOOL_TYPE_VAR,	DEFAULT_COMMAND_MODE, NULL, NULL, 0, 0 },
	{ "COMMENT_BREAKAGE",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "COMPRESS_MODES",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "CONNECT_DELAY",0,		INT_TYPE_VAR,	DEFAULT_CONNECT_DELAY, NULL, NULL, 0, VIF_BITCHX },
	{ "CONNECT_TIMEOUT",0,		INT_TYPE_VAR,	DEFAULT_CONNECT_TIMEOUT, NULL, NULL, 0, 0 },
	{ "CONTINUED_LINE",0,		STR_TYPE_VAR,	0, NULL, BX_set_continued_lines, 0, 0 },
	{ "CPU_SAVER_AFTER",0,		INT_TYPE_VAR,	DEFAULT_CPU_SAVER_AFTER, NULL, NULL, 0, VIF_BITCHX },
	{ "CPU_SAVER_EVERY",0,		INT_TYPE_VAR,	DEFAULT_CPU_SAVER_EVERY, NULL, NULL, 0, VIF_BITCHX },
	{ "CTCP_DELAY",0,		INT_TYPE_VAR,	DEFAULT_CTCP_DELAY, NULL, NULL, 0, VIF_BITCHX },
	{ "CTCP_FLOOD_AFTER",0,		INT_TYPE_VAR,	DEFAULT_CTCP_FLOOD_AFTER, NULL, NULL, 0, VIF_BITCHX },
	{ "CTCP_FLOOD_BAN",0,		BOOL_TYPE_VAR,	DEFAULT_CTCP_FLOOD_BAN, NULL, NULL, 0, 0 },
	{ "CTCP_FLOOD_PROTECTION",0,	BOOL_TYPE_VAR,	DEFAULT_CTCP_FLOOD_PROTECTION, NULL, NULL, 0, 0 },
	{ "CTCP_FLOOD_RATE",0,		INT_TYPE_VAR,	DEFAULT_CTCP_FLOOD_RATE, NULL, NULL, 0, VIF_BITCHX},
	{ "CTCP_VERBOSE",0,		BOOL_TYPE_VAR,	DEFAULT_VERBOSE_CTCP, NULL, NULL, 0, 0 },
	{ "CTOOLZ_DIR",0,		STR_TYPE_VAR,   0, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_AUTOGET",0,		BOOL_TYPE_VAR,	DEFAULT_DCC_AUTOGET, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_AUTORENAME",0,		BOOL_TYPE_VAR,	DEFAULT_DCC_AUTORENAME, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_AUTORESUME",0,		BOOL_TYPE_VAR,	DEFAULT_DCC_AUTORESUME, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_BAR_TYPE",0,		INT_TYPE_VAR,	DEFAULT_DCC_BAR_TYPE, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_BLOCK_SIZE",0,		INT_TYPE_VAR,	DEFAULT_DCC_BLOCK_SIZE, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_DLDIR",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_FAST",0,			BOOL_TYPE_VAR,	DEFAULT_DCC_FAST, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_FORCE_PORT",0,		INT_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_GET_LIMIT",0,		INT_TYPE_VAR,	DEFAULT_DCC_GET_LIMIT, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_MAX_AUTOGET_SIZE",0,	INT_TYPE_VAR,	DEFAULT_MAX_AUTOGET_SIZE, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_QUEUE_LIMIT",0,		INT_TYPE_VAR,	DEFAULT_DCC_QUEUE_LIMIT, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_SEND_LIMIT",0,		INT_TYPE_VAR,	DEFAULT_DCC_SEND_LIMIT, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_TIMEOUT",0,		INT_TYPE_VAR,	DEFAULT_DCCTIMEOUT, NULL, set_dcc_timeout, 0, VIF_BITCHX },
	{ "DCC_ULDIR",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "DCC_USE_GATEWAY_ADDR",0,	BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "DEBUG",0,			STR_TYPE_VAR,	0, NULL, debug_window, 0, 0 },
#if defined(__EMXPM__) || defined(WIN32)
	{ "DEFAULT_CODEPAGE",0,		INT_TYPE_VAR,	DEFAULT_CODEPAGE, NULL, NULL, 0, VIF_BITCHX },
#endif
	{ "DEFAULT_FONT",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "DEFAULT_MENU",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "DEFAULT_NICK",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "DEFAULT_REASON",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "DEOPFLOOD",0,		BOOL_TYPE_VAR,	DEFAULT_DEOPFLOOD, NULL, NULL, 0, VIF_BITCHX },
	{ "DEOPFLOOD_TIME",0,		INT_TYPE_VAR,	DEFAULT_DEOPFLOOD_TIME, NULL, NULL, 0, VIF_BITCHX },
	{ "DEOP_ON_DEOPFLOOD",0,	INT_TYPE_VAR,	DEFAULT_DEOP_ON_DEOPFLOOD, NULL, NULL, 0, VIF_BITCHX },
	{ "DEOP_ON_KICKFLOOD",0,	INT_TYPE_VAR,	DEFAULT_DEOP_ON_KICKFLOOD, NULL, NULL, 0, VIF_BITCHX },
	{ "DETACH_ON_HUP",0,		BOOL_TYPE_VAR,	DEFAULT_DETACH_ON_HUP, NULL, set_detach_on_hup, 0, 0 },
	{ "DISPATCH_UNKNOWN_COMMANDS",0,BOOL_TYPE_VAR,	DEFAULT_DISPATCH_UNKNOWN_COMMANDS, NULL, NULL, 0, 0 },
	{ "DISPLAY",0,			BOOL_TYPE_VAR,	DEFAULT_DISPLAY, NULL, NULL, 0, 0 },
	{ "DISPLAY_ANSI",0,		BOOL_TYPE_VAR,	DEFAULT_DISPLAY_ANSI, NULL, toggle_reverse_status, 0, 0 },
	{ "DISPLAY_PC_CHARACTERS",0,	INT_TYPE_VAR,	DEFAULT_DISPLAY_PC_CHARACTERS, NULL, reinit_screen, 0, VIF_BITCHX },
	{ "DOUBLE_STATUS_LINE",0,	INT_TYPE_VAR,	DEFAULT_DOUBLE_STATUS_LINE, NULL, reinit_status, 0, VIF_BITCHX },
	{ "EIGHT_BIT_CHARACTERS",0,	BOOL_TYPE_VAR,	DEFAULT_EIGHT_BIT_CHARACTERS, NULL, eight_bit_characters, 0, VIF_BITCHX },
	{ "EXEC_PROTECTION",0,		BOOL_TYPE_VAR,	DEFAULT_EXEC_PROTECTION, NULL, exec_warning, 0, VF_NODAEMON },
	{ "FAKE_SPLIT_PATTERNS",0,	STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },

	{ "FLOATING_POINT_MATH_VAR",0,	BOOL_TYPE_VAR,	DEFAULT_FLOATING_POINT_MATH, NULL, NULL, 0, 0 },
	{ "FLOOD_AFTER",0,		INT_TYPE_VAR,	DEFAULT_FLOOD_AFTER, NULL, NULL, 0, VIF_BITCHX },
	{ "FLOOD_KICK",0,		BOOL_TYPE_VAR,	DEFAULT_FLOOD_KICK, NULL, NULL, 0, VIF_BITCHX },
	{ "FLOOD_PROTECTION",0,		BOOL_TYPE_VAR,	DEFAULT_FLOOD_PROTECTION, NULL, NULL, 0, VIF_BITCHX },
	{ "FLOOD_RATE",0,		INT_TYPE_VAR,	DEFAULT_FLOOD_RATE, NULL, NULL, 0, VIF_BITCHX },
	{ "FLOOD_USERS",0,		INT_TYPE_VAR,	DEFAULT_FLOOD_USERS, NULL, NULL, 0, VIF_BITCHX },
	{ "FLOOD_WARNING",0,		BOOL_TYPE_VAR,	DEFAULT_FLOOD_WARNING, NULL, NULL, 0, VIF_BITCHX },

	{ "FTP_GRAB",0,			BOOL_TYPE_VAR,	DEFAULT_FTP_GRAB, NULL, NULL, 0, VIF_BITCHX },
	{ "FULL_STATUS_LINE",0,		BOOL_TYPE_VAR,	DEFAULT_FULL_STATUS_LINE, NULL, BX_update_all_status, 0, 0 },
	{ "HACKING",0,			INT_TYPE_VAR,	DEFAULT_HACKING, NULL, NULL, 0, VIF_BITCHX },
	{ "HACK_OPS",0,			BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "HEBREW_TOGGLE",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "HELP_PAGER",0,		BOOL_TYPE_VAR,	DEFAULT_HELP_PAGER, NULL, NULL, 0, 0 },
	{ "HELP_PATH",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_EXPAND_PATH|VF_NODAEMON },
	{ "HELP_PROMPT",0,		BOOL_TYPE_VAR,	DEFAULT_HELP_PROMPT, NULL, NULL, 0, 0 },
	{ "HELP_WINDOW",0,		BOOL_TYPE_VAR,	DEFAULT_HELP_WINDOW, NULL, NULL, 0, 0 },
	{ "HIDE_PRIVATE_CHANNELS",0,	BOOL_TYPE_VAR,	DEFAULT_HIDE_PRIVATE_CHANNELS, NULL, BX_update_all_status, 0, 0 },
	{ "HIGHLIGHT_CHAR",0,		STR_TYPE_VAR,	0, NULL, set_highlight_char, 0, 0 },
	{ "HIGH_BIT_ESCAPE",0,		INT_TYPE_VAR,	DEFAULT_HIGH_BIT_ESCAPE, NULL, set_meta_8bit, 0, 0 }, 
	{ "HISTORY",0,			INT_TYPE_VAR,	DEFAULT_HISTORY, NULL, set_history_size, 0, VF_NODAEMON },
	{ "HOLD_MODE",0,		BOOL_TYPE_VAR,	DEFAULT_HOLD_MODE, NULL, BX_reset_line_cnt, 0, 0 },
	{ "HOLD_MODE_MAX",0,		INT_TYPE_VAR,	DEFAULT_HOLD_MODE_MAX, NULL, NULL, 0, 0 },
	{ "HTTP_GRAB",0,		BOOL_TYPE_VAR,  DEFAULT_HTTP_GRAB, NULL, NULL, 0, VIF_BITCHX }, 
	{ "IDENT_HACK",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "IDLE_CHECK",0,		INT_TYPE_VAR,	120, NULL, NULL, 0, VIF_BITCHX },
	{ "IGNORE_TIME",0,		INT_TYPE_VAR,	DEFAULT_IGNORE_TIME, NULL, NULL, 0, VIF_BITCHX },
	{ "INDENT",0,			BOOL_TYPE_VAR,	DEFAULT_INDENT, NULL, NULL, 0, 0 },
	{ "INPUT_ALIASES",0,		BOOL_TYPE_VAR,	DEFAULT_INPUT_ALIASES, NULL, NULL, 0, 0 },
	{ "INPUT_GLOB",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "INPUT_PROMPT",0,		STR_TYPE_VAR,	0, NULL, BX_set_input_prompt, 0, 0 },
	{ "INSERT_MODE",0,		BOOL_TYPE_VAR,	DEFAULT_INSERT_MODE, NULL, BX_update_all_status, 0, 0 },
	{ "INVERSE_VIDEO",0,		BOOL_TYPE_VAR,	DEFAULT_INVERSE_VIDEO, NULL, NULL, 0, 0 },
	{ "JOINFLOOD",0,		BOOL_TYPE_VAR,	DEFAULT_JOINFLOOD, NULL, NULL, 0, VIF_BITCHX },
	{ "JOINFLOOD_TIME",0,		INT_TYPE_VAR,	DEFAULT_JOINFLOOD_TIME, NULL, NULL, 0, VIF_BITCHX },
	{ "JOIN_NEW_WINDOW",0,          BOOL_TYPE_VAR,  DEFAULT_JOIN_NEW_WINDOW, NULL, NULL, 0, VIF_BITCHX },
	{ "JOIN_NEW_WINDOW_TYPE",0,     STR_TYPE_VAR,   0, NULL, NULL, 0, VIF_BITCHX },
	{ "KICKFLOOD",0,		BOOL_TYPE_VAR,	DEFAULT_KICKFLOOD, NULL, NULL, 0, VIF_BITCHX  },
	{ "KICKFLOOD_TIME",0,		INT_TYPE_VAR,	DEFAULT_KICKFLOOD_TIME, NULL, NULL, 0, VIF_BITCHX  },
	{ "KICK_IF_BANNED",0,		BOOL_TYPE_VAR,  DEFAULT_KICK_IF_BANNED, NULL, NULL, 0, VIF_BITCHX },
	{ "KICK_ON_DEOPFLOOD",0,	INT_TYPE_VAR,   DEFAULT_KICK_ON_DEOPFLOOD, NULL, NULL, 0, VIF_BITCHX },
	{ "KICK_ON_JOINFLOOD",0,	INT_TYPE_VAR,	DEFAULT_KICK_ON_JOINFLOOD, NULL, NULL, 0, VIF_BITCHX },
	{ "KICK_ON_KICKFLOOD",0,	INT_TYPE_VAR,   DEFAULT_KICK_ON_KICKFLOOD, NULL, NULL, 0, VIF_BITCHX  },
	{ "KICK_ON_NICKFLOOD",0,	INT_TYPE_VAR,   DEFAULT_KICK_ON_NICKFLOOD, NULL, NULL, 0, VIF_BITCHX  },
	{ "KICK_ON_PUBFLOOD",0,		INT_TYPE_VAR,   DEFAULT_KICK_ON_PUBFLOOD, NULL, NULL, 0, VIF_BITCHX  },
	{ "KICK_OPS",0,			BOOL_TYPE_VAR,	DEFAULT_KICK_OPS, NULL, NULL, 0, VIF_BITCHX  },
	{ "LAMEIDENT",0,		BOOL_TYPE_VAR,	DEFAULT_LAME_IDENT, NULL, NULL, 0, VIF_BITCHX },
	{ "LAMELIST",0,			BOOL_TYPE_VAR,	DEFAULT_LAMELIST, NULL, NULL, 0, VIF_BITCHX },
	{ "LASTLOG",0,			INT_TYPE_VAR,	DEFAULT_LASTLOG, NULL, set_lastlog_size, 0, 0 },
	{ "LASTLOG_ANSI",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "LASTLOG_LEVEL",0,		STR_TYPE_VAR,	0, NULL, set_lastlog_level, 0, 0 },
	{ "LLOOK",0,			BOOL_TYPE_VAR,	DEFAULT_LLOOK, NULL, NULL, 0, 0 },
	{ "LLOOK_DELAY",0,		INT_TYPE_VAR,	DEFAULT_LLOOK_DELAY, NULL, NULL, 0, VIF_BITCHX  },
	{ "LOAD_PATH",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "LOG",0,			BOOL_TYPE_VAR,	DEFAULT_LOG, NULL, logger, 0, 0 },
	{ "LOGFILE",0,			STR_TYPE_VAR,	0, NULL, set_log_file, 0, VF_NODAEMON },
	{ "MAIL",0,			INT_TYPE_VAR,	DEFAULT_MAIL, NULL, BX_update_all_status, 0, VIF_BITCHX },

	{ "MANGLE_INBOUND",0,		STR_TYPE_VAR,	0, NULL, set_mangle_inbound, 0, VIF_BITCHX },
	{ "MANGLE_LOGFILES",0,		STR_TYPE_VAR,	0, NULL, set_mangle_logfiles, 0, VIF_BITCHX },
	{ "MANGLE_OPERLOG",0,		STR_TYPE_VAR,	0, NULL, set_mangle_operlog, 0, VIF_BITCHX },
	{ "MANGLE_OUTBOUND",0,		STR_TYPE_VAR,	0, NULL, set_mangle_outbound, 0, VIF_BITCHX },

	{ "MAX_DEOPS",0,		INT_TYPE_VAR,	DEFAULT_MAX_DEOPS, NULL, NULL, 0, VIF_BITCHX  },
	{ "MAX_IDLEKICKS",0,		INT_TYPE_VAR,	DEFAULT_MAX_IDLEKICKS, NULL, NULL, 0, VIF_BITCHX  },
	{ "MAX_SERVER_RECONNECT",0,	INT_TYPE_VAR,	DEFAULT_MAX_SERVER_RECONNECT, NULL, NULL, 0, VIF_BITCHX },
	{ "MAX_URLS",0,			INT_TYPE_VAR,	DEFAULT_MAX_URLS, NULL, NULL, 0, VIF_BITCHX },
#ifdef GUI
	{ "MDI",0,				BOOL_TYPE_VAR,	DEFAULT_MDI, NULL, gui_mdi, 0, VIF_BITCHX },
#else
	{ "MDI",0,				BOOL_TYPE_VAR,	DEFAULT_MDI, NULL, NULL, 0, VIF_BITCHX },
#endif
	{ "META_STATES_VAR",0,		INT_TYPE_VAR,	DEFAULT_META_STATES, NULL, NULL, 0, 0 },
	{ "MIRCS",0,			BOOL_TYPE_VAR,  1, NULL, NULL, 0, VIF_BITCHX  },
	{ "MODE_STRIPPER",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "MSGCOUNT",0,			INT_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "MSGLOG",0,			BOOL_TYPE_VAR,	DEFAULT_MSGLOG, NULL, NULL, 0, 0 },
	{ "MSGLOG_FILE",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX|VF_EXPAND_PATH },
	{ "MSGLOG_LEVEL",0,		STR_TYPE_VAR,	0, NULL, set_msglog_level, 0, VIF_BITCHX},
	{ "NAMES_COLUMNS",0,		INT_TYPE_VAR,	5, NULL, NULL, 0, VIF_BITCHX },
	{ "NAT_ADDRESS",0,		STR_TYPE_VAR,	0, NULL, set_nat_address, 0, VIF_BITCHX },
	{ "ND_SPACE_MAX",0,		INT_TYPE_VAR,	DEFAULT_ND_SPACE_MAX, NULL, NULL, 0, 0 },
	{ "NEW_SERVER_LASTLOG_LEVEL",0,	STR_TYPE_VAR,	0, NULL, set_new_server_lastlog_level, 0, VIF_BITCHX },
	{ "NEXT_SERVER_ON_LOCAL_KILL",0,  BOOL_TYPE_VAR,  0, NULL, NULL, 0, VIF_BITCHX },

	{ "NICKFLOOD",0,		BOOL_TYPE_VAR,	DEFAULT_NICKFLOOD, NULL, NULL, 0, VIF_BITCHX  },
	{ "NICKFLOOD_TIME",0,		INT_TYPE_VAR,	DEFAULT_NICKFLOOD_TIME, NULL, NULL, 0, VIF_BITCHX  },
	{ "NICKLIST",0,		        INT_TYPE_VAR,	DEFAULT_NICKLIST, NULL, NULL, 0, VIF_BITCHX  },
	{ "NICKLIST_SORT",0,		        INT_TYPE_VAR,	DEFAULT_NICKLIST_SORT, NULL, NULL, 0, VIF_BITCHX  },
	{ "NICK_COMPLETION",0,		BOOL_TYPE_VAR,	DEFAULT_NICK_COMPLETION, NULL, NULL, 0, VIF_BITCHX  },
	{ "NICK_COMPLETION_CHAR",0,	CHAR_TYPE_VAR,  DEFAULT_NICK_COMPLETION_CHAR, NULL, NULL, 0, VIF_BITCHX },
	{ "NICK_COMPLETION_LEN",0,	INT_TYPE_VAR,	DEFAULT_NICK_COMPLETION_LEN, NULL, NULL, 0, VIF_BITCHX  },
	{ "NICK_COMPLETION_TYPE",0,	INT_TYPE_VAR,	DEFAULT_NICK_COMPLETION_TYPE, NULL, NULL, 0, VIF_BITCHX  },
	{ "NOTIFY",0,			BOOL_TYPE_VAR,	DEFAULT_NOTIFY, NULL, NULL, 0, VIF_BITCHX },
	{ "NOTIFY_HANDLER",0,		STR_TYPE_VAR, 	0, NULL, set_notify_handler, 0, 0 },
	{ "NOTIFY_INTERVAL",0,		INT_TYPE_VAR,   DEFAULT_NOTIFY_INTERVAL, NULL, NULL, 0, VIF_BITCHX },
	{ "NOTIFY_LEVEL",0,		STR_TYPE_VAR,	0, NULL, set_notify_level, 0, 0 },
	{ "NOTIFY_ON_TERMINATION",0,	BOOL_TYPE_VAR,	DEFAULT_NOTIFY_ON_TERMINATION, NULL, NULL, 0, VF_NODAEMON },
	{ "NO_CTCP_FLOOD",0,		BOOL_TYPE_VAR,	DEFAULT_NO_CTCP_FLOOD, NULL, NULL, 0, 0 },
	{ "NO_FAIL_DISCONNECT",0,	BOOL_TYPE_VAR,	DEFAULT_NO_FAIL_DISCONNECT, NULL, NULL, 0, VIF_BITCHX },
	{ "NUM_BANMODES",0,		INT_TYPE_VAR,   DEFAULT_NUM_BANMODES, NULL, NULL, 0, VIF_BITCHX },
	{ "NUM_KICKS",0,		INT_TYPE_VAR,   DEFAULT_NUM_KICKS, NULL, NULL, 0, VIF_BITCHX },
	{ "NUM_KILLS",0,		INT_TYPE_VAR,	1, NULL, NULL, 0, VIF_BITCHX },
	{ "NUM_OF_WHOWAS",0,		INT_TYPE_VAR,	DEFAULT_NUM_OF_WHOWAS, NULL, NULL, 0, VIF_BITCHX },
	{ "NUM_OPMODES",0,		INT_TYPE_VAR,   DEFAULT_NUM_OPMODES, NULL, NULL, 0, VIF_BITCHX },
	{ "OPER_MODES",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "ORIGNICK_TIME",0,		INT_TYPE_VAR,	5, NULL, NULL, 0, VIF_BITCHX },
#ifdef WANT_OPERVIEW
	{ "OV",0,			BOOL_TYPE_VAR,  DEFAULT_OPER_VIEW, NULL, set_ov_mode, 0, VIF_BITCHX },
#else
	{ "OV",0,			BOOL_TYPE_VAR,  0, NULL, NULL, 0, VIF_BITCHX },
#endif
	{ "PAD_CHAR",0,			CHAR_TYPE_VAR,	DEFAULT_PAD_CHAR, NULL, NULL, 0, 0 },
	{ "PING_TYPE",0,		INT_TYPE_VAR,   DEFAULT_PING_TYPE, NULL, NULL, 0, VIF_BITCHX  },
	{ "PROTECT_CHANNELS",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "PUBFLOOD",0,			BOOL_TYPE_VAR,	DEFAULT_PUBFLOOD, NULL, NULL, 0, VIF_BITCHX },
	{ "PUBFLOOD_TIME",0,		INT_TYPE_VAR,	DEFAULT_PUBFLOOD_TIME, NULL, NULL, 0, VIF_BITCHX  },
	{ "QUERY_NEW_WINDOW",0,         BOOL_TYPE_VAR,  DEFAULT_QUERY_NEW_WINDOW, NULL, NULL, 0, VIF_BITCHX },
	{ "QUERY_NEW_WINDOW_TYPE",0,    STR_TYPE_VAR,   0, NULL, NULL, 0, VIF_BITCHX },
	{ "QUEUE_SENDS",0,		INT_TYPE_VAR,	DEFAULT_QUEUE_SENDS, NULL, NULL, 0, VIF_BITCHX },
	{ "RANDOM_LOCAL_PORTS",0,	BOOL_TYPE_VAR,	DEFAULT_RANDOM_LOCAL_PORTS, NULL, NULL, 0, VIF_BITCHX },
	{ "RANDOM_SOURCE",0,		INT_TYPE_VAR,	DEFAULT_RANDOM_SOURCE, NULL, NULL, 0, VIF_BITCHX },
	{ "REALNAME",0,			STR_TYPE_VAR,	0, NULL, set_realname, 0, VF_NODAEMON },
	{ "REVERSE_STATUS",0,		BOOL_TYPE_VAR,	0, NULL, reinit_status, 0, 0 },
	{ "SAVEFILE",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX  },
	{ "SCREEN_OPTIONS",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SCRIPT_HELP",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX  },
	{ "SCROLLBACK",0,		INT_TYPE_VAR,	DEFAULT_SCROLLBACK_LINES, NULL, BX_set_scrollback_size, 0, 0 },
	{ "SCROLLBACK_RATIO",0,		INT_TYPE_VAR,	DEFAULT_SCROLLBACK_RATIO, NULL, NULL, 0, 0 },
	{ "SCROLL_LINES",0,		INT_TYPE_VAR,	DEFAULT_SCROLL_LINES, NULL, BX_set_scroll_lines, 0, 0 },
	{ "SEND_AWAY_MSG",0,		BOOL_TYPE_VAR,	DEFAULT_SEND_AWAY_MSG, NULL, NULL, 0, VIF_BITCHX },
	{ "SEND_CTCP_MSG",0,		BOOL_TYPE_VAR,	DEFAULT_SEND_CTCP_MSG, NULL, NULL, 0, VIF_BITCHX },
	{ "SEND_IGNORE_MSG",0,		BOOL_TYPE_VAR,	DEFAULT_SEND_IGNORE_MSG, NULL, NULL, 0, VIF_BITCHX },
	{ "SEND_OP_MSG",0,		BOOL_TYPE_VAR,	DEFAULT_SEND_OP_MSG, NULL, NULL, 0, VIF_BITCHX },
	{ "SERVER_GROUPS",0,            BOOL_TYPE_VAR,  DEFAULT_SERVER_GROUPS, NULL, NULL, 0, VIF_BITCHX },
	{ "SERVER_PROMPT",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "SHELL",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL_FLAGS",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL_LIMIT",0,		INT_TYPE_VAR,	DEFAULT_SHELL_LIMIT, NULL, NULL, 0, VF_NODAEMON },
	{ "SHITLIST",0,			BOOL_TYPE_VAR,  DEFAULT_SHITLIST, NULL, NULL, 0, VIF_BITCHX  },
	{ "SHITLIST_REASON",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX  },
	{ "SHOW_AWAY_ONCE",0,		BOOL_TYPE_VAR,	DEFAULT_SHOW_AWAY_ONCE, NULL, NULL, 0, VIF_BITCHX },
	{ "SHOW_CHANNEL_NAMES",0,	BOOL_TYPE_VAR,	DEFAULT_SHOW_CHANNEL_NAMES, NULL, NULL, 0, 0 },
	{ "SHOW_END_OF_MSGS",0,		BOOL_TYPE_VAR,	DEFAULT_SHOW_END_OF_MSGS, NULL, NULL, 0, 0 },
	{ "SHOW_NUMERICS",0,		BOOL_TYPE_VAR,	DEFAULT_SHOW_NUMERICS, NULL, NULL, 0, 0 },
	{ "SHOW_NUMERICS_STR",0,	STR_TYPE_VAR,	0, NULL, set_numeric_string, 0, 0 },
	{ "SHOW_STATUS_ALL",0,		BOOL_TYPE_VAR,	DEFAULT_SHOW_STATUS_ALL, NULL, BX_update_all_status, 0, 0 },
	{ "SHOW_WHO_HOPCOUNT",0, 	BOOL_TYPE_VAR,	DEFAULT_SHOW_WHO_HOPCOUNT, NULL, NULL, 0, 0 },
	{ "SOCKS_HOST",0,		STR_TYPE_VAR,	0, NULL, set_use_socks, 0, VIF_BITCHX },
	{ "SOCKS_PORT",0,		INT_TYPE_VAR,	DEFAULT_SOCKS_PORT, NULL, NULL, 0, VIF_BITCHX },
	{ "STATUS_AWAY",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_CDCCCOUNT",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_CHANNEL",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_CHANOP",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_CLOCK",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_CPU_SAVER",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_DCCCOUNT",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_DOES_EXPANDOS",0,	BOOL_TYPE_VAR,	DEFAULT_STATUS_DOES_EXPANDOS, NULL, BX_build_status, 0, 0 },
	{ "STATUS_FLAG",0,		STR_TYPE_VAR,   0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_FORMAT",0,            STR_TYPE_VAR,   0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_FORMAT1",0,           STR_TYPE_VAR,   0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_FORMAT2",0,           STR_TYPE_VAR,   0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_FORMAT3",0,           STR_TYPE_VAR,   0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_HALFOP",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_HOLD",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_HOLD_LINES",0,	STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_INSERT",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_LAG",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_MAIL",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, VF_NODAEMON },
	{ "STATUS_MODE",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_MSGCOUNT",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_NICK",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_NOTIFY",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_NO_REPEAT",0,		BOOL_TYPE_VAR,	DEFAULT_STATUS_NO_REPEAT, NULL, NULL, 0, 0 },
	{ "STATUS_OPER",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_OPER_KILLS",0,	STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_OVERWRITE",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_QUERY",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_SCROLLBACK",0,	STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 }, 
	{ "STATUS_SERVER",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_TOPIC",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_UMODE",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER0",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER1",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER10",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER11",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER12",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER13",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER14",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER15",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER16",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER17",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER18",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER19",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER2",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER20",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER21",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER22",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER23",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER24",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER25",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER26",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER27",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER28",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER29",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER3",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER30",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER31",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER32",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER33",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER34",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER35",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER36",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER37",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER38",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER39",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER4",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER5",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER6",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER7",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER8",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USER9",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_USERS",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_VOICE",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "STATUS_WINDOW",0,		STR_TYPE_VAR,	0, NULL, BX_build_status, 0, 0 },
	{ "SUPPRESS_SERVER_MOTD",0,	BOOL_TYPE_VAR,	DEFAULT_SUPPRESS_SERVER_MOTD, NULL, NULL, 0, VF_NODAEMON },
#ifdef WANT_OPERVIEW
	{ "SWATCH",0,			STR_TYPE_VAR,	0, NULL, convert_swatch, 0, VIF_BITCHX },
#else
	{ "SWATCH",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
#endif
	{ "TAB",0,			BOOL_TYPE_VAR,	DEFAULT_TAB, NULL, NULL, 0, 0 },
	{ "TAB_MAX",0,			INT_TYPE_VAR,	DEFAULT_TAB_MAX, NULL, NULL, 0, 0 },
	{ "TIMESTAMP",0,		BOOL_TYPE_VAR,	DEFAULT_TIMESTAMP, NULL, NULL, 0, VIF_BITCHX },
	{ "TIMESTAMP_AWAYLOG_HOURLY",0,	BOOL_TYPE_VAR,	DEFAULT_TIMESTAMP_AWAYLOG_HOURLY, NULL, NULL, 0, VIF_BITCHX },
	{ "TIMESTAMP_STRING",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },

#ifdef TRANSLATE
	{ "TRANSLATION",0,		STR_TYPE_VAR,	0, NULL, set_translation, 0, 0 },
#else
	{ "TRANSLATION",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
#endif

	{ "UNDERLINE_VIDEO",0,		BOOL_TYPE_VAR,	DEFAULT_UNDERLINE_VIDEO, NULL, NULL, 0, 0 },
	{ "USERLIST",0,			BOOL_TYPE_VAR,  DEFAULT_USERLIST, NULL, NULL, 0, VIF_BITCHX },
	{ "USERMODE",0,			STR_TYPE_VAR,	0, NULL, set_user_mode, 0, VIF_BITCHX},
	{ "USER_FLAG_OPS",0, 		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "USER_FLAG_PROT",0, 		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "USER_INFORMATION",0, 	STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "WINDOW_DESTROY_PART",0,      BOOL_TYPE_VAR,  DEFAULT_WINDOW_DESTROY_PART, NULL, NULL, 0, VIF_BITCHX },
	{ "WINDOW_QUIET",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "WORD_BREAK",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "XTERM",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "XTERM_OPTIONS",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "XTERM_TITLE",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX }, 
	{ "_CDCC_CLOSE_IDLE_SENDS_TIME",0,INT_TYPE_VAR,	55,NULL, NULL, 0, VIF_BITCHX },
	{ "_CDCC_MINSPEED_TIME",0,	INT_TYPE_VAR,	0, NULL, NULL, 0, VIF_BITCHX },
	{ "_CDCC_PACKS_OFFERED",0,	INT_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ NULL, 0,			0, 		0, NULL, NULL, 0, 0 }
};

/*
 * get_string_var: returns the value of the string variable given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
char	*	BX_get_string_var(enum VAR_TYPES var)
{
	return (irc_variable[var].string);
}

/*
 * get_int_var: returns the value of the integer string given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
int BX_get_int_var(enum VAR_TYPES var)
{
	return (irc_variable[var].integer);
}

/*
 * set_string_var: sets the string variable given as an index into the
 * variable table to the given string.  If string is null, the current value
 * of the string variable is freed and set to null 
 */
void BX_set_string_var(enum VAR_TYPES var, char *string)
{
	if (string)
		malloc_strcpy(&(irc_variable[var].string), string);
	else
		new_free(&(irc_variable[var].string));
}

/*
 * set_int_var: sets the integer value of the variable given as an index into
 * the variable table to the given value 
 */
void BX_set_int_var(enum VAR_TYPES var, unsigned int value)
{
	irc_variable[var].integer = value;
}

/*
 * init_variables: initializes the string variables that can't really be
 * initialized properly above 
 */
void init_variables()
{
#if defined(WINNT) || defined(__EMX__)
	char *shell;
#endif
	int old_display = window_display;
	char *s;
#if 1
/* def VAR_DEBUG*/
 	int i;
	
	for (i = 1; i < NUMBER_OF_VARIABLES - 1; i++)
		if (strcmp(irc_variable[i-1].name, irc_variable[i].name) >= 0)
			ircpanic("Variable [%d] (%s) is out of order.", i, irc_variable[i].name);
#endif

	window_display = 0;

	set_string_var(SCRIPT_HELP_VAR, DEFAULT_SCRIPT_HELP_FILE);
	set_string_var(BITCHX_HELP_VAR, DEFAULT_BITCHX_HELP_FILE);
	set_string_var(IDENT_HACK_VAR, DEFAULT_IDENT_HACK);
	set_string_var(AUTO_RESPONSE_STR_VAR, nickname);
	set_string_var(XTERM_OPTIONS_VAR, DEFAULT_XTERM);
        set_string_var(XTERM_OPTIONS_VAR, DEFAULT_XTERM_OPTIONS);
        
	set_string_var(SHOW_NUMERICS_STR_VAR, DEFAULT_SHOW_NUMERICS_STR);
	set_numeric_string(current_window, DEFAULT_SHOW_NUMERICS_STR, 0);

	reinit_autoresponse(current_window, nickname, 0);
	set_string_var(MSGLOGFILE_VAR, DEFAULT_MSGLOGFILE);

	set_string_var(MSGLOG_LEVEL_VAR, DEFAULT_MSGLOG_LEVEL);
	set_string_var(CTOOLZ_DIR_VAR, DEFAULT_CTOOLZ_DIR);
	set_int_var(LLOOK_DELAY_VAR, DEFAULT_LLOOK_DELAY);
	set_int_var(MSGCOUNT_VAR, 0);

	set_string_var(SHITLIST_REASON_VAR, DEFAULT_SHITLIST_REASON);
	set_string_var(PROTECT_CHANNELS_VAR, DEFAULT_PROTECT_CHANNELS);
	set_string_var(DEFAULT_REASON_VAR, DEFAULT_KICK_REASON);
	set_string_var(DCC_DLDIR_VAR, DEFAULT_DCC_DLDIR);
	set_string_var(DCC_ULDIR_VAR, DEFAULT_DCC_DLDIR);	

	set_string_var(CMDCHARS_VAR, DEFAULT_CMDCHARS);
	set_string_var(LOGFILE_VAR, DEFAULT_LOGFILE);
	set_string_var(WORD_BREAK_VAR, DEFAULT_WORD_BREAK);

        set_string_var(JOIN_NEW_WINDOW_TYPE_VAR, DEFAULT_JOIN_NEW_WINDOW_TYPE);
        set_string_var(QUERY_NEW_WINDOW_TYPE_VAR, DEFAULT_QUERY_NEW_WINDOW_TYPE);

#if defined(__EMX__)
	if (getenv("SHELL"))
	{
		shell = getenv("SHELL");
		set_string_var(SHELL_VAR, path_search(shell, getenv("PATH")));
        	if (getenv("SHELL_FLAGS"))
        		set_string_var(SHELL_FLAGS_VAR, getenv("SHELL_FLAGS"));
	}
	else
	{
	        shell = "cmd.exe";
        	set_string_var(SHELL_FLAGS_VAR, "/c");
		set_string_var(SHELL_VAR, convert_dos(path_search(shell, getenv("PATH"))));
	}
#elif WINNT
	if (getenv("SHELL"))
	{
		shell = getenv("SHELL");
		set_string_var(SHELL_VAR, path_search(shell, getenv("PATH")));
        	if (getenv("SHELL_FLAGS"))
        		set_string_var(SHELL_FLAGS_VAR, getenv("SHELL_FLAGS"));
	}
 	else if (gdwPlatform == VER_PLATFORM_WIN32_WINDOWS)
	{
		shell = "command.com";
        	set_string_var(SHELL_FLAGS_VAR, "/c");
		set_string_var(SHELL_VAR, convert_dos(path_search(shell, getenv("PATH"))));
	}
	else
	{
	        shell = "cmd.exe";
        	set_string_var(SHELL_FLAGS_VAR, "/c");
		set_string_var(SHELL_VAR, convert_dos(path_search(shell, getenv("PATH"))));
	}
#else
	set_string_var(SHELL_VAR, DEFAULT_SHELL);
	set_string_var(SHELL_FLAGS_VAR, DEFAULT_SHELL_FLAGS);
#endif
	set_string_var(CONTINUED_LINE_VAR, DEFAULT_CONTINUED_LINE);
	set_string_var(INPUT_PROMPT_VAR, DEFAULT_INPUT_PROMPT);
	set_string_var(HIGHLIGHT_CHAR_VAR, DEFAULT_HIGHLIGHT_CHAR);
	set_string_var(LASTLOG_LEVEL_VAR, DEFAULT_LASTLOG_LEVEL);
	set_string_var(NOTIFY_HANDLER_VAR, DEFAULT_NOTIFY_HANDLER);
	set_string_var(NOTIFY_LEVEL_VAR, DEFAULT_NOTIFY_LEVEL);
	set_string_var(REALNAME_VAR, realname);

	set_string_var(STATUS_FLAG_VAR, DEFAULT_STATUS_FLAG);
	set_string_var(STATUS_FORMAT_VAR, DEFAULT_STATUS_FORMAT);
	set_string_var(STATUS_FORMAT1_VAR, DEFAULT_STATUS_FORMAT1);
	set_string_var(STATUS_FORMAT2_VAR, DEFAULT_STATUS_FORMAT2);
	set_string_var(STATUS_FORMAT3_VAR, DEFAULT_STATUS_FORMAT3);

	set_string_var(STATUS_DCCCOUNT_VAR, DEFAULT_STATUS_DCCCOUNT);
	set_string_var(STATUS_CDCCCOUNT_VAR, DEFAULT_STATUS_CDCCCOUNT);

	set_string_var(STATUS_AWAY_VAR, DEFAULT_STATUS_AWAY);
	set_string_var(STATUS_CHANNEL_VAR, DEFAULT_STATUS_CHANNEL);
	set_string_var(STATUS_CHANOP_VAR, DEFAULT_STATUS_CHANOP);
	set_string_var(STATUS_CLOCK_VAR, DEFAULT_STATUS_CLOCK);
	set_string_var(STATUS_CPU_SAVER_VAR, DEFAULT_STATUS_CPU_SAVER);
	set_string_var(STATUS_HALFOP_VAR, DEFAULT_STATUS_HALFOP);
	set_string_var(STATUS_HOLD_VAR, DEFAULT_STATUS_HOLD);
	set_string_var(STATUS_HOLD_LINES_VAR, DEFAULT_STATUS_HOLD_LINES);
	set_string_var(STATUS_INSERT_VAR, DEFAULT_STATUS_INSERT);
	set_string_var(STATUS_LAG_VAR, DEFAULT_STATUS_LAG);
	set_string_var(STATUS_MAIL_VAR, DEFAULT_STATUS_MAIL);
	set_string_var(STATUS_MODE_VAR, DEFAULT_STATUS_MODE);
	set_string_var(STATUS_MSGCOUNT_VAR, DEFAULT_STATUS_MSGCOUNT);
	set_string_var(STATUS_OPER_VAR, DEFAULT_STATUS_OPER);
	set_string_var(STATUS_VOICE_VAR, DEFAULT_STATUS_VOICE);
	set_string_var(STATUS_NICK_VAR, DEFAULT_STATUS_NICK);
	set_string_var(STATUS_NOTIFY_VAR, DEFAULT_STATUS_NOTIFY);
	set_string_var(STATUS_OPER_KILLS_VAR, DEFAULT_STATUS_OPER_KILLS);
	set_string_var(STATUS_OVERWRITE_VAR, DEFAULT_STATUS_OVERWRITE);
	set_string_var(STATUS_QUERY_VAR, DEFAULT_STATUS_QUERY);
	set_string_var(STATUS_SERVER_VAR, DEFAULT_STATUS_SERVER);
	set_string_var(STATUS_TOPIC_VAR, DEFAULT_STATUS_TOPIC);
	set_string_var(STATUS_UMODE_VAR, DEFAULT_STATUS_UMODE);
	set_string_var(STATUS_USERS_VAR, DEFAULT_STATUS_USERS);
	set_string_var(STATUS_USER0_VAR, DEFAULT_STATUS_USER);
	set_string_var(STATUS_USER1_VAR, DEFAULT_STATUS_USER1);
	set_string_var(STATUS_USER2_VAR, DEFAULT_STATUS_USER2);
	set_string_var(STATUS_USER3_VAR, DEFAULT_STATUS_USER3);
	set_string_var(STATUS_USER4_VAR, DEFAULT_STATUS_USER4);
	set_string_var(STATUS_USER5_VAR, DEFAULT_STATUS_USER5);
	set_string_var(STATUS_USER6_VAR, DEFAULT_STATUS_USER6);
	set_string_var(STATUS_USER7_VAR, DEFAULT_STATUS_USER7);
	set_string_var(STATUS_USER8_VAR, DEFAULT_STATUS_USER8);
	set_string_var(STATUS_USER9_VAR, DEFAULT_STATUS_USER9);
	set_string_var(STATUS_USER10_VAR, DEFAULT_STATUS_USER10);
	set_string_var(STATUS_USER11_VAR, DEFAULT_STATUS_USER11);
	set_string_var(STATUS_USER12_VAR, DEFAULT_STATUS_USER12);
	set_string_var(STATUS_USER13_VAR, DEFAULT_STATUS_USER13);
	set_string_var(STATUS_USER14_VAR, DEFAULT_STATUS_USER14);
	set_string_var(STATUS_USER15_VAR, DEFAULT_STATUS_USER15);
	set_string_var(STATUS_USER16_VAR, DEFAULT_STATUS_USER16);
	set_string_var(STATUS_USER17_VAR, DEFAULT_STATUS_USER17);
	set_string_var(STATUS_USER18_VAR, DEFAULT_STATUS_USER18);
	set_string_var(STATUS_USER19_VAR, DEFAULT_STATUS_USER19);
	set_string_var(STATUS_USER20_VAR, DEFAULT_STATUS_USER20);
	set_string_var(STATUS_USER21_VAR, DEFAULT_STATUS_USER21);
	set_string_var(STATUS_USER22_VAR, DEFAULT_STATUS_USER22);
	set_string_var(STATUS_USER23_VAR, DEFAULT_STATUS_USER23);
	set_string_var(STATUS_USER24_VAR, DEFAULT_STATUS_USER24);
	set_string_var(STATUS_USER25_VAR, DEFAULT_STATUS_USER25);
	set_string_var(STATUS_USER26_VAR, DEFAULT_STATUS_USER26);
	set_string_var(STATUS_USER27_VAR, DEFAULT_STATUS_USER27);
	set_string_var(STATUS_USER28_VAR, DEFAULT_STATUS_USER28);
	set_string_var(STATUS_USER29_VAR, DEFAULT_STATUS_USER29);
	set_string_var(STATUS_USER30_VAR, DEFAULT_STATUS_USER30);
	set_string_var(STATUS_USER31_VAR, DEFAULT_STATUS_USER31);
	set_string_var(STATUS_USER32_VAR, DEFAULT_STATUS_USER32);
	set_string_var(STATUS_USER33_VAR, DEFAULT_STATUS_USER33);
	set_string_var(STATUS_USER34_VAR, DEFAULT_STATUS_USER34);
	set_string_var(STATUS_USER35_VAR, DEFAULT_STATUS_USER35);
	set_string_var(STATUS_USER36_VAR, DEFAULT_STATUS_USER36);
	set_string_var(STATUS_USER37_VAR, DEFAULT_STATUS_USER37);
	set_string_var(STATUS_USER38_VAR, DEFAULT_STATUS_USER38);
	set_string_var(STATUS_USER39_VAR, DEFAULT_STATUS_USER39);
	set_string_var(STATUS_WINDOW_VAR, DEFAULT_STATUS_WINDOW);
	set_string_var(STATUS_SCROLLBACK_VAR, DEFAULT_STATUS_SCROLLBACK);
	set_string_var(TIMESTAMP_STRING_VAR, DEFAULT_TIMESTAMP_STR);
	
	set_string_var(USERINFO_VAR, DEFAULT_USERINFO);

	set_string_var(USERMODE_VAR, !send_umode ? DEFAULT_USERMODE : send_umode);
	set_user_mode(current_window, !send_umode? DEFAULT_USERMODE : send_umode, 0);

	set_beep_on_msg(current_window, DEFAULT_BEEP_ON_MSG, 0);

	set_var_value(SWATCH_VAR, DEFAULT_SWATCH, NULL);
	set_string_var(CLIENTINFO_VAR, IRCII_COMMENT);
	set_string_var(FAKE_SPLIT_PATS_VAR, "*fuck* *shit* *suck* *dick* *penis* *cunt* *haha* *fake* *split* *hehe* *bogus* *yawn* *leet* *blow* *screw* *dumb* *fbi* *l33t* *gov");
	set_string_var(CHANMODE_VAR, DEFAULT_CHANMODE);
	
	set_string_var(CDCC_PROMPT_VAR, "%GC%gDCC");
	set_string_var(DEFAULT_MENU_VAR, "none");
	set_string_var(DEFAULT_FONT_VAR, DEFAULT_FONT);
		
	set_string_var(SERVER_PROMPT_VAR, DEFAULT_SERVER_PROMPT);

	set_string_var(BOT_LOGFILE_VAR, "tcl.log");

	s = m_strdup(irc_lib);
	malloc_strcat(&s, "/help");
	set_string_var(HELP_PATH_VAR, s);
	new_free(&s);

#ifdef WANT_CD
	set_cd_device(current_window, "/dev/cdrom", 0);
#endif
	set_string_var(OPER_MODES_VAR, DEFAULT_OPERMODE);	

	set_string_var(MANGLE_INBOUND_VAR, NULL);
	set_string_var(MANGLE_OUTBOUND_VAR, NULL);
	set_string_var(MANGLE_LOGFILES_VAR, NULL);
	set_mangle_operlog(current_window, "ALL", 0);
	set_string_var(NEW_SERVER_LASTLOG_LEVEL_VAR, DEFAULT_NEW_SERVER_LASTLOG_LEVEL);
	
	set_lastlog_size(current_window, NULL, irc_variable[LASTLOG_VAR].integer);
	set_history_size(current_window, NULL, irc_variable[HISTORY_VAR].integer);

	set_highlight_char(current_window, irc_variable[HIGHLIGHT_CHAR_VAR].string, 0);
	set_lastlog_level(current_window, irc_variable[LASTLOG_LEVEL_VAR].string, 0);
	set_notify_level(current_window, irc_variable[NOTIFY_LEVEL_VAR].string, 0);
	set_msglog_level(current_window, irc_variable[MSGLOG_LEVEL_VAR].string, 0);

#ifdef TRANSLATE
	set_string_var(TRANSLATION_VAR, "LATIN1");
	set_translation(current_window, "LATIN1", 0);
#endif
	create_fsets(current_window, get_int_var(DISPLAY_ANSI_VAR));
	set_input_prompt(current_window, DEFAULT_INPUT_PROMPT, 0);
	if (current_window->wset)
		remove_wsets_for_window(current_window);
	current_window->wset = create_wsets_for_window(current_window);
	build_status(current_window, NULL, 0);
	window_display = old_display;
}


BUILT_IN_COMMAND(init_window_vars)
{
	if (current_window->wset)
		remove_wsets_for_window(current_window);
	current_window->wset = create_wsets_for_window(current_window);
	update_all_windows();
	build_status(current_window, NULL, 0);
	update_all_status(current_window, NULL, 0);
}

BUILT_IN_COMMAND(init_vars)
{
	init_variables();
	init_window_vars(NULL, NULL, NULL, NULL);
}

/*
 * do_boolean: just a handy thing.  Returns 1 if the str is not ON, OFF, or
 * TOGGLE 
 */
int do_boolean(char *str, int *value)
{
	upper(str);
	if (strcmp(str, var_settings[ON]) == 0)
		*value = 1;
	else if (strcmp(str, var_settings[OFF]) == 0)
		*value = 0;
	else if (strcmp(str, "TOGGLE") == 0)
	{
		if (*value)
			*value = 0;
		else
			*value = 1;
	}
	else
		return (1);
	return (0);
}

/*
 * set_var_value: Given the variable structure and the string representation
 * of the value, this sets the value in the most verbose and error checking
 * of manors.  It displays the results of the set and executes the function
 * defined in the var structure 
 */

void set_var_value(int var_index, char *value, IrcVariableDll *dll)
{
	char	*rest;
	IrcVariable *var;
	int	old;
#ifdef WANT_DLL
	if (dll)
	{
		var = (IrcVariable *) new_malloc(sizeof(IrcVariable));
		var->type = dll->type;
		var->string = dll->string;
		var->integer = dll->integer;
		var->int_flags = dll->int_flags;
		var->flags = dll->flags;
		var->name = dll->name;
		var->func = dll->func;
	}
	else
#endif
		var = &(irc_variable[var_index]);
	switch (var->type)
	{
	case BOOL_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			old = var->integer;
			if (do_boolean(value, &(var->integer)))
			{
				say("Value must be either ON, OFF, or TOGGLE");
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (old != var->integer)
					var->int_flags |= VIF_CHANGED;
			}
			if (loading_global)
				var->int_flags |= VIF_GLOBAL;
			if (var->func)
				(var->func) (current_window, NULL, var->integer);
			say("Value of %s set to %s", var->name,
				var->integer ? var_settings[ON]
					     : var_settings[OFF]);
		}
		else
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SET_FSET), "%s %s", var->name, var->integer?var_settings[ON] : var_settings[OFF]));
		break;
	case CHAR_TYPE_VAR:
		if (!value)
		{
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (var->integer)
					var->int_flags |= VIF_CHANGED;
			}
			if (loading_global)
				var->int_flags |= VIF_GLOBAL;
			var->integer = ' ';
			if (var->func)
				(var->func) (current_window, NULL, var->integer);
			say("Value of %s set to '%c'", var->name, var->integer);
		}

		else if (value && *value && (value = next_arg(value, &rest)))
		{
			if (strlen(value) > 1)
				say("Value of %s must be a single character",
					var->name);
			else
			{
				if (!(var->int_flags & VIF_CHANGED))
				{
					if (var->integer != *value)
						var->int_flags |= VIF_CHANGED;
				}
				if (loading_global)
					var->int_flags |= VIF_GLOBAL;
				var->integer = *value;
				if (var->func)
					(var->func) (current_window, NULL, var->integer);
				say("Value of %s set to '%c'", var->name,
					var->integer);
			}
		}
		else
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SET_FSET), "%s %c", var->name, var->integer));
		break;
	case INT_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			int	val;

			if (!is_number(value))
			{
				say("Value of %s must be numeric!", var->name);
				break;
			}
			if ((val = my_atol(value)) < 0)
			{
				say("Value of %s must be greater than 0", var->name);
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (var->integer != val)
					var->int_flags |= VIF_CHANGED;
			}
			if (loading_global)
				var->int_flags |= VIF_GLOBAL;
			var->integer = val;
			if (var->func)
				(var->func) (current_window, NULL, var->integer);
			say("Value of %s set to %d", var->name, var->integer);
		}
		else
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SET_FSET), "%s %d", var->name, var->integer));
		break;
	case STR_TYPE_VAR:
		if (value)
		{
			if (*value)
			{
				char	*temp = NULL;

				if (var->flags & VF_EXPAND_PATH)
				{
					temp = expand_twiddle(value);
					if (temp)
						value = temp;
					else
						say("SET: no such user");
				}
				if ((!var->int_flags & VIF_CHANGED))
				{
					if ((var->string && ! value) ||
					    (! var->string && value) ||
					    my_stricmp(var->string, value))
						var->int_flags |= VIF_CHANGED;
				}
				if (loading_global)
					var->int_flags |= VIF_GLOBAL;
				malloc_strcpy(&(var->string), value);
				if (temp)
					new_free(&temp);
			}
			else
			{
				put_it("%s", convert_output_format(fget_string_var(var->string?FORMAT_SET_FSET:FORMAT_SET_NOVALUE_FSET), "%s %s", var->name, var->string));
#ifdef WANT_DLL
				goto got_dll;
#else
				return;
#endif
			}
		}
		else
			new_free(&(var->string));
		if (var->func && !(var->int_flags & VIF_PENDING))
		{
			var->int_flags |= VIF_PENDING;
			(var->func) (current_window, var->string, 0);
			var->int_flags &= ~VIF_PENDING;
		}
		say("Value of %s set to %s", var->name, var->string ?
			var->string : "<EMPTY>");
		break;
	}
#ifdef WANT_DLL
got_dll:
	if (dll)
	{
		dll->integer = var->integer;
		if (var->string)
			dll->string = m_strdup(var->string);
		else
			dll->string = NULL;
		new_free(&var->string);
		new_free((char **)&var);
	}
#endif

}

extern AliasStack1 *set_stack;
void do_stack_set(int type, char *args)
{
	AliasStack1 *aptr = set_stack;
	AliasStack1 **aptrptr = &set_stack;
		
	if (!*aptrptr && (type == STACK_POP || type == STACK_LIST))
	{
		say("Set stack is empty!");
		return;
	}

	if (STACK_PUSH == type)
	{
		enum VAR_TYPES var_index;
		int cnt = 0;
		char *copy;
		copy = LOCAL_COPY(args);
		/* Dont need to unstub it, we're not actually using it. */
		upper(copy);
		find_fixed_array_item(irc_variable, sizeof(IrcVariable), NUMBER_OF_VARIABLES, copy, &cnt, (int *)&var_index);
		if (cnt < 0)
		{	
			aptr = (AliasStack1 *)new_malloc(sizeof(AliasStack1));
			aptr->next = aptrptr ? *aptrptr : NULL;
			*aptrptr = aptr;
			aptr->set = (IrcVariable *) new_malloc(sizeof(IrcVariable));
			memcpy(aptr->set, &irc_variable[var_index], sizeof(IrcVariable));
			aptr->name = m_strdup(irc_variable[var_index].name);
			if (irc_variable[var_index].string)
				aptr->set->string = m_strdup(irc_variable[var_index].string);
			aptr->var_index = var_index;
		}
		else if ((cnt == 0))
			say("No such Set [%s]", args);
		else
			say("Set is ambiguous %s", args);
		return;
	}

	if (STACK_POP == type)
	{
		AliasStack1 *prev = NULL;
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

				new_free(&(irc_variable[aptr->var_index].string));
				memcpy(&irc_variable[aptr->var_index], aptr->set, sizeof(IrcVariable));
				/* free it */
				new_free((char **)&aptr->name);
				new_free((char **)&aptr->set);
				new_free((char **)&aptr);
				return;
			}
		}
		say("%s is not on the %s stack!", args, "Set");
		return;
	}
	if (STACK_LIST == type)
	{
		AliasStack1 *prev = NULL;
		for (aptr = *aptrptr; aptr; prev = aptr, aptr = aptr->next)
		{
			switch(aptr->set->type)
			{
				case BOOL_TYPE_VAR:
					say("Variable [%s] = %s", aptr->set->name, on_off(aptr->set->integer));
					break;
				case INT_TYPE_VAR:
					say("Variable [%s] = %d", aptr->set->name, aptr->set->integer);
					break;
				case CHAR_TYPE_VAR:
					say("Variable [%s] = %c", aptr->set->name, aptr->set->integer);
					break;
				case STR_TYPE_VAR:
					say("Variable [%s] = %s", aptr->set->name, aptr->set->string?aptr->set->string:"<Empty String>");
					break;
				default:
					bitchsay("Error in do_stack_set: unknown set type");
			}
		}
		return;
	}
	say("Unknown STACK type ??");
}


/*
 * set_variable: The SET command sets one of the irc variables.  The args
 * should consist of "variable-name setting", where variable name can be
 * partial, but non-ambbiguous, and setting depends on the variable being set 
 */
BUILT_IN_COMMAND(setcmd)
{
	char	*var;
	int	cnt = 0;
	int	hook = 0;

enum VAR_TYPES	var_index = 0;

	IrcVariableDll *dll = NULL;

	if ((var = next_arg(args, &args)) != NULL)
	{
		if (*var == '-')
		{
			var++;
			args = NULL;
		}
#ifdef WANT_DLL
		if (dll_variable)
		{
			int dll_cnt = 0;
			if ((dll = (IrcVariableDll *) find_in_list_ext((List **)&dll_variable, var, 0, list_strnicmp)))
			{
				IrcVariableDll *tmp;
				for (tmp = dll; tmp; tmp = tmp->next)
				{
					if (!my_strnicmp(tmp->name, var, strlen(var)))
						dll_cnt++;
				}
				if (my_stricmp(dll->name, var))
				{
					cnt = dll_cnt;
					if (cnt == 1)
						cnt = -1;
				}
				else
					cnt = -1;
			}
		}
		if (cnt == 0)
#endif
		{
			upper(var);
			find_fixed_array_item (irc_variable, sizeof(IrcVariable), NUMBER_OF_VARIABLES, var, &cnt, (int *)&var_index);
		}

		if (cnt == 1)
			cnt = -1;
		if ((cnt >= 0) || (!dll && !(irc_variable[var_index].int_flags & VIF_PENDING)) || (dll && !(dll->int_flags & VIF_PENDING)))
			hook = 1;

		if (cnt < 0)
		{
			if (dll)
				dll->int_flags |= VIF_PENDING;
			else
				irc_variable[var_index].int_flags |= VIF_PENDING;
		}

		if (hook)
		{
			hook = do_hook(SET_LIST, "%s %s", var, args ? args : "<unset>"/*empty_string*/);
			if (hook && (cnt < 0))
			{
				hook = do_hook(SET_LIST, "%s %s",
					dll? dll->name : irc_variable[var_index].name,
					args ? args : "<unset>" /*empty_string*/);
			}
		}
		if (cnt < 0)
		{
			if (dll)
				dll->int_flags &= ~VIF_PENDING;
			else
				irc_variable[var_index].int_flags &= ~VIF_PENDING;
		}
		if (hook)
		{
			if (cnt < 0)
				set_var_value(var_index, args, dll);
			else if (cnt == 0)
			{
				if (!dll)
				{
					if (do_hook(SET_LIST, "set-error No such variable \"%s\"", var))
						say("No such variable \"%s\"", var);
				}
				else
					set_var_value(-1, args, dll);
			}
			else
			{
				if (do_hook(SET_LIST, "set-error %s is ambiguous", var))
				{
					say("%s is ambiguous", var);
					if (dll)
					{
						IrcVariableDll *tmp;
						for (tmp = dll; tmp; tmp = tmp->next)
						{
							if (!my_strnicmp(tmp->name, var, strlen(var)))
								set_var_value(-1, empty_string, tmp);
							else
								break;
						}
					}
					else
					{
						for (cnt += var_index; var_index < cnt; var_index++)
							set_var_value(var_index, empty_string, dll);
					}
				}
			}	
		}
	}
	else
        {
		int var_index;
		for (var_index = 0; var_index < NUMBER_OF_VARIABLES; var_index++)
			set_var_value(var_index, empty_string, NULL);
#ifdef WANT_DLL
		for (dll = dll_variable; dll; dll = dll->next)
			set_var_value(-1, empty_string, dll);
#endif
	}
}

/*
 * save_variables: this writes all of the IRCII variables to the given FILE
 * pointer in such a way that they can be loaded in using LOAD or the -l switch 
 */
void save_variables(FILE *fp, int do_all)
{
	IrcVariable *var;

	for (var = irc_variable; var->name; var++)
	{
		if (!(var->int_flags & VIF_CHANGED))
			continue;
		if ((do_all == 1) || !(var->int_flags & VIF_GLOBAL))
		{
			if (strcmp(var->name, "DISPLAY") == 0 || strcmp(var->name, "CLIENT_INFORMATION") == 0)
				continue;
			fprintf(fp, "SET ");
			switch (var->type)
			{
			case BOOL_TYPE_VAR:
				fprintf(fp, "%s %s\n", var->name, var->integer ?
					var_settings[ON] : var_settings[OFF]);
				break;
			case CHAR_TYPE_VAR:
				fprintf(fp, "%s %c\n", var->name, var->integer);
				break;
			case INT_TYPE_VAR:
				fprintf(fp, "%s %u\n", var->name, var->integer);
				break;
			case STR_TYPE_VAR:
				if (var->string)
					fprintf(fp, "%s %s\n", var->name,
						var->string);
				else
					fprintf(fp, "-%s\n", var->name);
				break;
			}
		}
	}
}

void savebitchx_variables(FILE *fp)
{
	IrcVariable *var;
	int count = 0;
	for (var = irc_variable; var->name; var++)
	{
		if (!(var->flags & VIF_BITCHX))
			continue;
		count++;
		fprintf(fp, "SET ");
		switch (var->type)
		{
		case BOOL_TYPE_VAR:
			fprintf(fp, "%s %s\n", var->name, var->integer ?
				var_settings[ON] : var_settings[OFF]);
			break;
		case CHAR_TYPE_VAR:
			fprintf(fp, "%s %c\n", var->name, var->integer);
			break;
		case INT_TYPE_VAR:
			fprintf(fp, "%s %u\n", var->name, var->integer);
			break;
		case STR_TYPE_VAR:
			if (var->string)
				fprintf(fp, "%s %s\n", var->name,
					var->string);
			else
				fprintf(fp, "-%s\n", var->name);
			break;
		}
	}
	bitchsay("Saved %d variables", count);
}

char	*make_string_var(const char *var_name)
{
	int	cnt,
		msv_index;
	char	*ret = NULL;
	char	*copy;
	
	copy = LOCAL_COPY(var_name);
	upper(copy);

	if ((find_fixed_array_item (irc_variable, sizeof(IrcVariable), NUMBER_OF_VARIABLES, copy, &cnt, &msv_index) == NULL))
		return NULL;
	if (cnt >= 0)
		return NULL;
	switch (irc_variable[msv_index].type)
	{
		case STR_TYPE_VAR:
			ret = m_strdup(irc_variable[msv_index].string);
			break;
		case INT_TYPE_VAR:
			ret = m_strdup(ltoa(irc_variable[msv_index].integer));
			break;
		case BOOL_TYPE_VAR:
			ret = m_strdup(var_settings[irc_variable[msv_index].integer]);
			break;
		case CHAR_TYPE_VAR:
			ret = m_dupchar(irc_variable[msv_index].integer);
			break;
	}
	return ret;
}

IrcVariable *get_var_address(char *var_name)
{
	int	cnt,
		msv_index;
	char	*copy;
	
	copy = LOCAL_COPY(var_name);
	upper(copy);
	if ((find_fixed_array_item (irc_variable, sizeof(IrcVariable), NUMBER_OF_VARIABLES, copy, &cnt, &msv_index) == NULL))
		return NULL;
	if (cnt >= 0)
		return NULL;
	return &irc_variable[msv_index];
}

/* exec_warning: a warning message displayed whenever EXEC_PROTECTION is turned off.  */
static	void exec_warning(Window *win, char *unused, int value)
{
	if (value == OFF)
	{
		bitchsay("Warning!  You have turned EXEC_PROTECTION off");
		bitchsay("Please read the /HELP SET EXEC_PROTECTION documentation");
	}
}

/* returns the size of the character set */
int charset_size(void)
{
	return get_int_var(EIGHT_BIT_CHARACTERS_VAR) ? 256 : 128;
}

static	void eight_bit_characters(Window *win, char *unused, int value)
{
	if (value == ON && !term_eight_bit())
		say("Warning!  Your terminal says it does not support eight bit characters");
	set_term_eight_bit(value);
}


static	void set_realname(Window *win, char *value, int unused)
{
	if (value)
		strmcpy(realname, value, REALNAME_LEN);
	else
		strmcpy(realname, empty_string, REALNAME_LEN);
}

void reinit_autoresponse(Window *win, char *value, int unused)
{
int old_window = window_display;
	window_display = 1;
	if (value)
		malloc_strcpy(&auto_str, value);
	else 
		bitchsay("Auto Response is set to - %s", auto_str);
	window_display = old_window;
}

static void set_numeric_string(Window *win, char *value, int unused)
{
	malloc_strcpy(&thing_ansi, value);
}

static void set_user_mode(Window *win, char *value, int unused)
{
	malloc_strcpy(&send_umode, value);
}

int old_ov_mode = -1;

#ifdef WANT_OPERVIEW
static void set_ov_mode (Window *win, char *value, int unused)
{
int old_window_display = window_display;
	if (!current_window)
		return;
	window_display = 0;
	if (old_ov_mode == -1)
		setup_ov_mode(unused ? 0 : 1, 0, -1);
	else if (old_ov_mode != unused)
		setup_ov_mode(unused ? 0 : 1, 0, -1);
	old_ov_mode = unused;
	window_display = old_window_display;
}
#endif

static void set_away_time(Window *win, char *unused, int value)
{
	if (value == 0)
		set_int_var(AUTO_AWAY_TIME_VAR, 0);
	else if ((value / 60) == 0)		
		set_int_var(AUTO_AWAY_TIME_VAR, value * 60);
	else if (value < 60 * 10)
		set_int_var(AUTO_AWAY_TIME_VAR, 60 * 10);
	else
		set_int_var(AUTO_AWAY_TIME_VAR, value);
}

void clear_sets(void)
{
int i = 0;
	for(i = 0; irc_variable[i].name; i++)
		new_free(&irc_variable->string);
}

static void reinit_screen(Window *win, char *unused, int value)
{
	set_input_prompt(current_window, NULL, 0);
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	update_input(UPDATE_ALL);
}

void reinit_status(Window *win, char *unused, int value)
{
	update_all_windows();
	update_all_status(current_window, NULL, 0);
}

void toggle_reverse_status(Window *win, char *unused, int value)
{
	if (!value)
		set_int_var(REVERSE_STATUS_VAR, 1);
	else
		set_int_var(REVERSE_STATUS_VAR, 0);
	create_fsets(win, value);
	set_string_var(SHOW_NUMERICS_STR_VAR, value ? DEFAULT_SHOW_NUMERICS_STR : NO_NUMERICS_STR);
	set_numeric_string(current_window, value ? DEFAULT_SHOW_NUMERICS_STR : NO_NUMERICS_STR, 0);
	reinit_status(win, unused, value);
}

#ifdef WANT_TCL
char *ircii_rem_str(ClientData *cd, Tcl_Interp *intp, char *name1, char *name2, int flags)
{
char *s;
IrcVariable *n;
	n = (IrcVariable *)cd;
	if ((s = Tcl_GetVar(intp, name1, TCL_GLOBAL_ONLY)))
	{
		Tcl_UnlinkVar(intp, name1);
		if (s && *s)
			malloc_strcpy(&n->string, s);
		else
			new_free(&n->string);
		Tcl_LinkVar(intp, name1, (char *)&n->string, TCL_LINK_STRING);
	}
	return NULL;
}

void add_tcl_vars(void)
{
char varname[80];
int i = 0;
	for(i = 0; irc_variable[i].name; i++)
	{
		int type_of = -1;
		switch(irc_variable[i].type)
		{
			case INT_TYPE_VAR:
				type_of = TCL_LINK_INT;
				break;
			case STR_TYPE_VAR:
				type_of = TCL_LINK_STRING;
				break;
			case BOOL_TYPE_VAR:
				type_of = TCL_LINK_BOOLEAN;
				break;
			default:
				continue;
		}
		strcpy(varname, irc_variable[i].name);
		lower(varname);
		Tcl_LinkVar(tcl_interp, varname, 
			(irc_variable[i].type == STR_TYPE_VAR) ? 
				(char *)&irc_variable[i].string : 
				(char *)&irc_variable[i].integer,
			type_of);
		if (irc_variable[i].type == STR_TYPE_VAR)
			Tcl_TraceVar(tcl_interp, varname, TCL_TRACE_WRITES,
				(Tcl_VarTraceProc *)ircii_rem_str, 
				(ClientData)&irc_variable[i]);
	}
}
#endif

void clear_variables(void)
{
int i;
	for(i = 0; irc_variable[i].name; i++)
	{
		if (irc_variable[i].string)
			new_free(&irc_variable[i].string);
	}
}

static void set_clock_format (Window *win, char *value, int unused)
{
	extern char *time_format; /* XXXX bogus XXXX */
	malloc_strcpy(&time_format, value);
	update_clock(RESET_TIME);
	update_all_status(current_window, NULL, 0);
}

char *get_all_sets(void)
{
	int i;
	char *ret = NULL;
	for (i = 0; irc_variable[i].name; i++)
		m_s3cat(&ret, space, irc_variable[i].name);
	return ret;
}

char *get_set(char *str)
{
	int i;
	char *ret = NULL;
	if (!str || !*str)
		return get_all_sets();
	for (i = 0; irc_variable[i].name; i++)
		if (wild_match(str, irc_variable[i].name))
			m_s3cat(&ret, space, irc_variable[i].name);
	return ret ? ret : m_strdup(empty_string);
}

static void	set_dcc_timeout (Window *win, char *unused, int value)
{
extern time_t dcc_timeout;
	if (value == 0)
		dcc_timeout = (time_t) -1;
	else
		dcc_timeout = value;
}

static void	set_use_socks(Window *win, char *value, int unused)
{
extern int use_socks;
	if (value && *value)
		use_socks = 1;
	else
		use_socks = 0;
}


int	parse_mangle (char *value, int nvalue, char **rv)
{
	char	*str1, *str2;
	char	*copy;
	char	*nv = NULL;

	if (rv)
		*rv = NULL;

	if (!value)
		return 0;

	copy = LOCAL_COPY(value);

	while ((str1 = new_next_arg(copy, &copy)))
	{
		while (*str1 && (str2 = next_in_comma_list(str1, &str1)))
		{
		     if (!my_strnicmp(str2, "ALL", 3))
				nvalue = (0x7FFFFFFF - (MANGLE_ESCAPES));
			else if (!my_strnicmp(str2, "-ALL", 4))
				nvalue = 0;
			else if (!my_strnicmp(str2, "ALL_OFF", 4))
				nvalue |= STRIP_ALL_OFF;
			else if (!my_strnicmp(str2, "-ALL_OFF", 5))
				nvalue &= ~(STRIP_ALL_OFF);
			else if (!my_strnicmp(str2, "ANSI", 2))
				nvalue |= MANGLE_ANSI_CODES;
			else if (!my_strnicmp(str2, "-ANSI", 3))
				nvalue &= ~(MANGLE_ANSI_CODES);
			else if (!my_strnicmp(str2, "BLINK", 2))
				nvalue |= STRIP_BLINK;
			else if (!my_strnicmp(str2, "-BLINK", 3))
				nvalue &= ~(STRIP_BLINK);
			else if (!my_strnicmp(str2, "BOLD", 2))
				nvalue |= STRIP_BOLD;
			else if (!my_strnicmp(str2, "-BOLD", 3))
				nvalue &= ~(STRIP_BOLD);
			else if (!my_strnicmp(str2, "COLOR", 1))
				nvalue |= STRIP_COLOR;
			else if (!my_strnicmp(str2, "-COLOR", 2))
				nvalue &= ~(STRIP_COLOR);
			else if (!my_strnicmp(str2, "ESCAPE", 1))
				nvalue |= MANGLE_ESCAPES;
			else if (!my_strnicmp(str2, "-ESCAPE", 2))
				nvalue &= ~(MANGLE_ESCAPES);
			else if (!my_strnicmp(str2, "ND_SPACE", 2))
				nvalue |= STRIP_ND_SPACE;
			else if (!my_strnicmp(str2, "-ND_SPACE", 3))
				nvalue &= ~(STRIP_ND_SPACE);
			else if (!my_strnicmp(str2, "NONE", 2))
				nvalue = 0;
			else if (!my_strnicmp(str2, "REVERSE", 1))
				nvalue |= STRIP_REVERSE;
			else if (!my_strnicmp(str2, "-REVERSE", 2))
				nvalue &= ~(STRIP_REVERSE);
			else if (!my_strnicmp(str2, "ROM_CHAR", 1))
				nvalue |= STRIP_ROM_CHAR;
			else if (!my_strnicmp(str2, "-ROM_CHAR", 2))
				nvalue &= ~(STRIP_ROM_CHAR);
			else if (!my_strnicmp(str2, "UNDERLINE", 1))
				nvalue |= STRIP_UNDERLINE;
			else if (!my_strnicmp(str2, "-UNDERLINE", 2))
				nvalue &= ~(STRIP_UNDERLINE);
		}
	}

	if (rv)
	{
		if (nvalue & MANGLE_ESCAPES)
			m_s3cat(&nv, comma, "ESCAPE");
		if (nvalue & MANGLE_ANSI_CODES)
			m_s3cat(&nv, comma, "ANSI");
		if (nvalue & STRIP_COLOR)
			m_s3cat(&nv, comma, "COLOR");
		if (nvalue & STRIP_REVERSE)
			m_s3cat(&nv, comma, "REVERSE");
		if (nvalue & STRIP_UNDERLINE)
			m_s3cat(&nv, comma, "UNDERLINE");
		if (nvalue & STRIP_BOLD)
			m_s3cat(&nv, comma, "BOLD");
		if (nvalue & STRIP_BLINK)
			m_s3cat(&nv, comma, "BLINK");
		if (nvalue & STRIP_ROM_CHAR)
			m_s3cat(&nv, comma, "ROM_CHAR");
		if (nvalue & STRIP_ND_SPACE)
			m_s3cat(&nv, comma, "ND_SPACE");
		if (nvalue & STRIP_ALL_OFF)
			m_s3cat(&nv, comma, "ALL_OFF");

		*rv = nv;
	}

	return nvalue;
}

static	void	set_mangle_inbound (Window *w, char *value, int unused)
{
	char *nv = NULL;
	inbound_line_mangler = parse_mangle(value, inbound_line_mangler, &nv);
	set_string_var(MANGLE_INBOUND_VAR, nv);
	new_free(&nv);
}

static	void	set_mangle_outbound (Window *w, char *value, int unused)
{
	char *nv = NULL;
	outbound_line_mangler = parse_mangle(value, outbound_line_mangler, &nv);
	set_string_var(MANGLE_OUTBOUND_VAR, nv);
	new_free(&nv);
}

static	void	set_mangle_logfiles (Window *w, char *value, int unused)
{
	char *nv = NULL;
	logfile_line_mangler = parse_mangle(value, logfile_line_mangler, &nv);
	set_string_var(MANGLE_LOGFILES_VAR, nv);
	if (w) 
		w->mangler = logfile_line_mangler;
	new_free(&nv);
}

static	void	set_mangle_operlog (Window *w, char *value, int unused)
{
	char *nv = NULL;
	operlog_line_mangler = parse_mangle(value, operlog_line_mangler, &nv);
	set_string_var(MANGLE_OPERLOG_VAR, nv);
	new_free(&nv);
}

static	void	set_nat_address (Window *w, char *value, int unused)
{
	char *nv = NULL, *h;
	if (value && *value)
	{
		nv = next_arg(value, &value);
		if (isdigit((unsigned char)*nv))
			h = nv;
		else
			h = host_to_ip(nv);
		set_string_var(NAT_ADDRESS_VAR, h);
        	nat_address.s_addr = inet_addr(h);
		use_nat_address = 2;
	}
	else
	{
		set_string_var(NAT_ADDRESS_VAR, NULL);
		use_nat_address = 0;
	}
}

#ifdef GUI
IrcVariable *return_irc_var(int nummer)
{
   return &irc_variable[nummer];
}
#endif
