/*
 * Program source is Copyright 1998 Colten Edwards.
 * Written specifically for configuring BitchX config.h
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#ifdef __EMX__
#include <sys/types.h>
#endif

#include <ncurses.h>

#include "ds_cell.h"
#include "ds_keys.h"
#include "func_pr.h"



extern int mapcolors;
/*extern _DS_CONFIG config;*/
extern int desc_changed;

/* Default answer replies for yes/no prompts*/
_default default_answer[5] = {
	{'Y',					TRUE},
	{'N',					FALSE},
	{13,					FALSE},
	{27,					FALSE},
	{'\0',					FALSE}
};


#define YN_ROW	 18
#define YN_COL	 5

#define PATHCOL	9
#define MAXLEN 280

/* Some local function prototypes */
int update_select(CELL *c);
char *fmt_long(unsigned long int val);
int main_dir (CELL *c);
int compile_dir (CELL *c);
int edit_dir (CELL *c);
int flood_dir (CELL *c);
int userlist_dir (CELL *c);
int toggle_select (CELL *c);
int edit_enter (CELL *c);
int save_file (CELL *c);
int exit_program(CELL *c);
int dcc_dir (CELL *c);
int server_dir (CELL *c);
int various_dir (CELL *c);
int load_dir (CELL *c);

#define USER_PATH 20
#define TOGGLE_FILES 21


#define NONE_TYPE 0
#define BOOL_TYPE 1
#define INT_TYPE 2
#define STR_TYPE 3

/* Function Table for file routines */
struct FuncTable file_cpy_table[] = {
	  {LS_END				, ls_end},
	  {LS_HOME				, ls_home},
	  {LS_PGUP				, ls_pgup},
	  {LS_PGDN				, ls_pgdn},
	  {CURSOR_UP				, cursor_up},
	  {CURSOR_DN				, cursor_dn},
	  {LS_QUIT				, ls_quit},
	  {WRAP_CURSOR_UP			, NULL},
	  {WRAP_CURSOR_DN			, NULL},
	  {DO_SHELL				, NULL},
	  {LS_ENTER				, update_select },
	  {UPFUNC				, NULL },
	  {TOGGLE				, NULL },
	  {PLUS_IT				, plus_it},
	  {MINUS_IT				, minus_it},
	  {0					, NULL}};

struct FuncTable compile_cpy_table[] = {
	  {LS_END				, ls_end},
	  {LS_HOME				, ls_home},
	  {LS_PGUP				, ls_pgup},
	  {LS_PGDN				, ls_pgdn},
	  {CURSOR_UP				, cursor_up},
	  {CURSOR_DN				, cursor_dn},
	  {LS_QUIT				, ls_quit},
	  {WRAP_CURSOR_UP			, NULL},
	  {WRAP_CURSOR_DN			, NULL},
	  {DO_SHELL				, NULL},
	  {LS_ENTER				, toggle_select },
	  {UPFUNC				, NULL },
	  {TOGGLE				, NULL },
	  {PLUS_IT				, plus_it},
	  {MINUS_IT				, minus_it},
	  {0					, NULL}};

struct FuncTable edit_cpy_table[] = {
	  {LS_END				, ls_end},
	  {LS_HOME				, ls_home},
	  {LS_PGUP				, ls_pgup},
	  {LS_PGDN				, ls_pgdn},
	  {CURSOR_UP				, cursor_up},
	  {CURSOR_DN				, cursor_dn},
	  {LS_QUIT				, ls_quit},
	  {WRAP_CURSOR_UP			, NULL},
	  {WRAP_CURSOR_DN			, NULL},
	  {DO_SHELL				, NULL},
	  {LS_ENTER				, edit_enter },
	  {0					, NULL}};

/* Menu Table for File routines */
struct KeyTable file_cpy_menu[] = {
	{KEY_END,LS_END,"End"},
	{KEY_HOME, LS_HOME, "Home"},
	{KEY_PPAGE, LS_PGUP, "PgUp"},
	{KEY_NPAGE, LS_PGDN, "PgDn"},
	{KEY_UP, CURSOR_UP, "Up"},
	{KEY_DOWN, CURSOR_DN, "Down"},
	{ESC, LS_QUIT, "Quit"},
	{ALT_X, LS_QUIT, "Quit"},
	{KEY_F(9), DO_SHELL, "Shell"},
	{SPACE, TOGGLE, "Toggle On/Off"},
	{'\n', LS_ENTER, "Change Dir"},
	{'\r', LS_ENTER, "Change Dir"},
	{'+',  PLUS_IT, "Add Mark"},
	{'-', MINUS_IT,"Take Mark"},
	{-1, -1, ""}};


typedef struct _config_type {
	char *option;
	char *help;
	char *out;
	int integer;
	int type;
	int (*func)(CELL *c);
} Configure; 

Configure config_type[] = {
	{ "Compile Defaults", "Compile time defaults within the client", NULL, 0, 0, compile_dir },
	{ "Flood Settings", "Change the flood protection settings", NULL, 0, 0, flood_dir },
	{ "Userlist/Shitlist", "Change settings for the Userlist Shitlist options", NULL, 0, 0, userlist_dir },
	{ "DCC Settings",	"Change various DCC settings", NULL, 0, 0, dcc_dir },
	{ "Server Settings",	"Change various server and away settings", NULL, 0, 0, server_dir },
	{ "Various Settings",	"Change Miscellaneous settings", NULL, 0, 0, various_dir },
	{ "Save .config.h", "Save .config.h", NULL, 0, 0, save_file},
	{ "Load .config.h", "Load a saved .config.h", NULL, 0, 0, load_dir},
	{ "Exit", "Exit this program", NULL, 0, 0, exit_program},
	{  NULL, NULL, NULL, 0, 0, NULL }
};

#define ON 1
#define OFF 0

Configure compile_default[] = {
{ "Lite BitchX",	"Disables ALOT of options below to make the client smaller", "BITCHX_LITE",			OFF, BOOL_TYPE, NULL },
{ "Link Looker",	"Enable LinkLook. This is deprecated as well as dangerous", "WANT_LLOOK",			ON, BOOL_TYPE, NULL },
{ "Plugin support",	"Enable plugins on supported OS\'s", "WANT_DLL",				ON, BOOL_TYPE, NULL },
{ "OperView",		"Enable OperView window support", "WANT_OPERVIEW",			ON, BOOL_TYPE, NULL },
{ "CDCC support",	"Enable builtin cdcc which is a XDCC file offer clone", "WANT_CDCC",			ON, BOOL_TYPE, NULL },
{ "DCC ftp support",	"Enable builtin ftp support", "WANT_FTP",				ON, BOOL_TYPE, NULL },
{ "Internal Screen",	"Enable builtin screen utility for /detach", "WANT_DETACH",			ON, BOOL_TYPE, NULL },
{ "EPIC help file support", "Enable epic helpfile support and /ehelp", "WANT_EPICHELP",		ON, BOOL_TYPE, NULL},
{ "Humble\'s Hades color","Another color option from the dark days of BX.", "HUMBLE",				OFF, BOOL_TYPE, NULL },
{ "Xwindows Window create","Xwindows/screen can /window create to create a new \"screen\"", "WINDOW_CREATE",			ON, BOOL_TYPE, NULL},
{ "Flow Control",	"Leave this enabled, unless you know it's needed","USE_FLOW_CONTROL",			ON, BOOL_TYPE, NULL},
{ "Allow Ctrl-Z",	"Does Ctrl-Z background BX.", "ALLOW_STOP_IRC",			ON, BOOL_TYPE, NULL},
{ "Only Std Chars",	"Use only standard chars instead of ibmpc charset", "ONLY_STD_CHARS",			OFF, BOOL_TYPE, NULL},
{ "Latin1 Char Set",	"Use only the latin1 charset.", "LATIN1", OFF, BOOL_TYPE, NULL},
{ "Ascii Logos",	"Use the alternate ASCII logos", "ASCII_LOGO", OFF, BOOL_TYPE, NULL},
{ "Reverse Color",	"Reverse black and white colors for Xterms", "REVERSE_WHITE_BLACK", OFF, BOOL_TYPE, NULL},
{ "Emacs Keybinds",	"If you have trouble with your keyboard, change this", "EMACS_KEYBINDS", OFF, BOOL_TYPE, NULL},
{ "Identd Faking",	"Allow identd faking", "IDENT_FAKE",			OFF, BOOL_TYPE, NULL},
{ "cidentd support",	"Use cidentd .authlie for fake username", "CIDENTD",				OFF, BOOL_TYPE, NULL},
{ "wdidentd support",	"Use wdidentd for fake username. Do no enable both cidentd and this", "WDIDENT",				OFF, BOOL_TYPE, NULL},
{ "Include glob support","File Globbing support","INCLUDE_GLOB_FUNCTION",		ON, BOOL_TYPE, NULL},
{ "Exec Command",	"Enable the /exec command", "EXEC_COMMAND",			ON, BOOL_TYPE, NULL},
{ "Public Access Enabled","This disables the dangerous commands, making them unsable", "PUBLIC_ACCESS", OFF, BOOL_TYPE, NULL},
{ "Hebrew Language support", "Hebrew Language Support", "HEBREW",			ON, BOOL_TYPE, NULL},
{ "Translation Tables", "Keyboard Translation tables", "TRANSLATE",			ON, BOOL_TYPE, NULL},
{ "Mirc resume support"," Support Mirc\'s Broken resume","MIRC_BROKEN_DCC_RESUME",	ON, BOOL_TYPE, NULL},
{ "Mode Compression",	"Code for performing mode compression on mass mode changes", "COMPRESS_MODES", ON, BOOL_TYPE, NULL},
{ "MAX # of urls in list", "Max Number of URLS to save in memory", "DEFAULT_MAX_URLS",   30, INT_TYPE, edit_dir },
{ "ChatNet Support", "Support chatnet\'s numeric 310", "WANT_CHATNET",			OFF,BOOL_TYPE, NULL},
{ "Notify BitchX.com", "Notify BitchX.com of our version", "SHOULD_NOTIFY_BITCHX_COM",	ON, BOOL_TYPE, NULL},
{ "Want Userlist",		"", "WANT_USERLIST",		ON,  BOOL_TYPE, NULL },
{ NULL, NULL, NULL, 0, 0, NULL }
};

Configure userlist_default[] = {
{ "Userlist",			"", "DEFAULT_USERLIST",		ON,  BOOL_TYPE, NULL },
{ "Auto-op",			"", "DEFAULT_AOP_VAR",		 OFF, BOOL_TYPE, NULL },
{ "Auto Invite",		"", "DEFAULT_AINV",		0, INT_TYPE, edit_dir },
{ "Kick ops",			"", "DEFAULT_KICK_OPS",		 ON, BOOL_TYPE, NULL },
{ "Annoy Kick",			"", "DEFAULT_ANNOY_KICK",	 OFF, BOOL_TYPE, NULL },

{ "Lamenick",			"", "DEFAULT_LAMELIST", 	ON, BOOL_TYPE, NULL },
{ "Lame Ident",			"", "DEFAULT_LAME_IDENT", 	OFF, BOOL_TYPE, NULL },
{ "Shitlist",			"", "DEFAULT_SHITLIST", 	ON, BOOL_TYPE, NULL },
{ "Auto Rejoin Channel",	"", "DEFAULT_AUTO_REJOIN",	 ON, BOOL_TYPE, NULL },

{ "Deop Flood",			"", "DEFAULT_DEOPFLOOD",	 ON, BOOL_TYPE, NULL },
{ "Deop Flood Time",		"", "DEFAULT_DEOPFLOOD_TIME",	 30,INT_TYPE, edit_dir },
{ "Deop After \'X\' deops",	"", "DEFAULT_DEOP_ON_DEOPFLOOD", 3,INT_TYPE, edit_dir },
{ "Deop After \'X\' kicks",	"", "DEFAULT_DEOP_ON_KICKFLOOD", 3,INT_TYPE, edit_dir },

{ "Join Flood",			"", "DEFAULT_JOINFLOOD",	 ON, BOOL_TYPE, NULL },
{ "Join Flood Time",		"", "DEFAULT_JOINFLOOD_TIME",	 50,INT_TYPE, edit_dir },

{ "Kick Flood",			"", "DEFAULT_KICKFLOOD",	 ON, BOOL_TYPE, NULL },
{ "Kick Flood Time",		"", "DEFAULT_KICKFLOOD_TIME",	 30,INT_TYPE, edit_dir },
{ "Kick on \'X\' Kicks",		"", "DEFAULT_KICK_ON_KICKFLOOD", 4,INT_TYPE, edit_dir },
{ "Kick on \'X\' Deops",	"", "DEFAULT_KICK_ON_DEOPFLOOD", 3,INT_TYPE, edit_dir },
{ "Kick On \'X\' Joins",	"", "DEFAULT_KICK_ON_JOINFLOOD", 4,INT_TYPE, edit_dir },

{ "Nick Flood",			"", "DEFAULT_NICKFLOOD",	 ON, BOOL_TYPE, NULL },
{ "Nick Flood Time",		"", "DEFAULT_NICKFLOOD_TIME",	 30,INT_TYPE, edit_dir },
{ "Kick on \'X\' Nick Changes",	"", "DEFAULT_KICK_ON_NICKFLOOD", 3,INT_TYPE, edit_dir },

{ "Public Flood",		"", "DEFAULT_PUBFLOOD",		 OFF, BOOL_TYPE, NULL },
{ "Public Flood Time",		"", "DEFAULT_PUBFLOOD_TIME",	 20,INT_TYPE, edit_dir },
{ "Kick on Public flood",	"", "DEFAULT_KICK_ON_PUBFLOOD", 30,INT_TYPE, edit_dir },

{ "Kick on ban match",		"", "DEFAULT_KICK_IF_BANNED",	 OFF, BOOL_TYPE, NULL },
{ "Server op protection",	"Values 0 for none, 1 for deop, 2 for announce only", "DEFAULT_HACKING",		 0,INT_TYPE, edit_dir },  /* 0 1 2 */
{"Auto-Unban time",		"", "DEFAULT_AUTO_UNBAN",   600, INT_TYPE, edit_dir },
{"Default Ban time",		"", "DEFAULT_BANTIME",   600, INT_TYPE, edit_dir },
{"Send ctcp msg",		"Send notice when ctcp command recieved", "DEFAULT_SEND_CTCP_MSG",	 ON, BOOL_TYPE, NULL },
{"Send Op msg",			"Send notice when auto-op sent", "DEFAULT_SEND_OP_MSG",	 ON, BOOL_TYPE, NULL },
{ NULL, NULL, NULL, 0, 0, NULL }
};

Configure flood_default[] = {
{ "Flood Prot",		"", "DEFAULT_FLOOD_PROTECTION",		ON,  BOOL_TYPE, NULL },
{ "Ctcp Flood Prot",	"", "DEFAULT_CTCP_FLOOD_PROTECTION",	ON,  BOOL_TYPE, NULL },
{ "Ctcp Flood after",	"", "DEFAULT_CTCP_FLOOD_AFTER",		10, INT_TYPE, edit_dir },
{ "Ctcp Flood rate",	"", "DEFAULT_CTCP_FLOOD_RATE",		3,  INT_TYPE, edit_dir},
{ "Flood Kick",		"", "DEFAULT_FLOOD_KICK",		ON,  BOOL_TYPE, NULL },
{ "Flood Rate",		"", "DEFAULT_FLOOD_RATE",		5,  INT_TYPE, edit_dir },
{ "Flood After",	"", "DEFAULT_FLOOD_AFTER",		4,  INT_TYPE, edit_dir },
{ "Flood Users",	"", "DEFAULT_FLOOD_USERS",		10, INT_TYPE, edit_dir },
{ "Flood Warning",	"", "DEFAULT_FLOOD_WARNING",		OFF,  BOOL_TYPE, NULL },
{ "No CTCP flood",	"", "DEFAULT_NO_CTCP_FLOOD",		ON,  BOOL_TYPE, NULL },
{ "CDCC flood after",	"", "DEFAULT_CDCC_FLOOD_AFTER",		3,  INT_TYPE, edit_dir },
{ "CDCC flood rate",	"", "DEFAULT_CDCC_FLOOD_RATE",		4,  INT_TYPE, edit_dir },
{ "CTCP delay",		"", "DEFAULT_CTCP_DELAY",		3,  INT_TYPE, edit_dir },
{ "CTCP flood ban",	"", "DEFAULT_CTCP_FLOOD_BAN",		ON,  BOOL_TYPE, NULL },
{ "CTCP verbose",	"", "DEFAULT_VERBOSE_CTCP",		ON,  BOOL_TYPE, NULL },
{ "CDCC on/off",	"", "DEFAULT_CDCC",			ON,  BOOL_TYPE, NULL },
{"Ignore time on flood","", "DEFAULT_IGNORE_TIME",		600, INT_TYPE, edit_dir },
{ NULL, NULL, NULL, 0, 0, NULL }
};

Configure dcc_default[] = {
{ "DCC Fast", "Enable DCC fast support on supported hosts.", "DEFAULT_DCC_FAST",	ON, BOOL_TYPE, NULL},
{ "DCC block size", "", "DEFAULT_DCC_BLOCK_SIZE", 2048, INT_TYPE, edit_dir},
{ "DCC auto-rename", "", "DEFAULT_DCC_AUTORENAME",   ON, BOOL_TYPE, NULL},
{ "DCC auto-resume", "", "DEFAULT_DCC_AUTORESUME",   OFF, BOOL_TYPE, NULL},
{ "DCC display bar", " 0 for color bargraph, 1 for non-color bar", "DEFAULT_DCC_BAR_TYPE",   0, INT_TYPE, edit_dir },
{ "DCC Autoget", "", "DEFAULT_DCC_AUTOGET",   OFF, BOOL_TYPE, NULL},
{ "DCC Max DCC gets", "", "DEFAULT_DCC_GET_LIMIT",    0, INT_TYPE, edit_dir},
{ "DCC Max DCC sends", "", "DEFAULT_DCC_SEND_LIMIT",   5, INT_TYPE, edit_dir},
{ "DCC Queue Limit", "", "DEFAULT_DCC_QUEUE_LIMIT",   10, INT_TYPE, edit_dir},
{ "DCC Limit get/send", "", "DEFAULT_DCC_LIMIT",   10, INT_TYPE, edit_dir },
{ "DCC Idle dcc timeout", "", "DEFAULT_DCCTIMEOUT",   600, INT_TYPE, edit_dir },
{ "CDCC Queue timeout", "", "DEFAULT_QUEUE_SENDS", 0, INT_TYPE, edit_dir },
{ "DCC MAX dcc filesize get", "", "DEFAULT_MAX_AUTOGET_SIZE", 2000000, INT_TYPE, edit_dir },
{ NULL, NULL, NULL, 0, 0, NULL }
};

Configure server_default[] = {
{"Auto-Away system on/off", "", "DEFAULT_AUTO_AWAY",     ON, BOOL_TYPE, NULL },
{"Send Away to Channel", "", "DEFAULT_SEND_AWAY_MSG",   OFF, BOOL_TYPE, NULL },
{"# of seconds for auto-away", "", "DEFAULT_AUTO_AWAY_TIME",   600, INT_TYPE, edit_dir },
{"auto-unmark away", "", "DEFAULT_AUTO_UNMARK_AWAY",    OFF, BOOL_TYPE, NULL },
{"Channel Name width", "", "DEFAULT_CHANNEL_NAME_WIDTH", 10, INT_TYPE, NULL },
{"Socks Port default", "", "DEFAULT_SOCKS_PORT",   1080, INT_TYPE, edit_dir },
{"Turn Notify on/off", "", "DEFAULT_NOTIFY",   ON, BOOL_TYPE, NULL},
{"Max # of server reconnects", "", "DEFAULT_MAX_SERVER_RECONNECT",   2, INT_TYPE, edit_dir },
{"# of Seconds for connect", "", "DEFAULT_CONNECT_TIMEOUT",   30, INT_TYPE, edit_dir },
{"Send unknown commands", "", "DEFAULT_DISPATCH_UNKNOWN_COMMANDS",   OFF, BOOL_TYPE, NULL},
{"Ignore Server write errors", "", "DEFAULT_NO_FAIL_DISCONNECT",   OFF, BOOL_TYPE, NULL},
{"Enable Server groups", "", "DEFAULT_SERVER_GROUPS",   OFF, BOOL_TYPE, NULL},
{"Kill window on part OS/2", "", "DEFAULT_WINDOW_DESTROY_PART",   OFF, BOOL_TYPE, NULL},
{"Display server MOTD", "", "DEFAULT_SUPPRESS_SERVER_MOTD",   ON, BOOL_TYPE, NULL},
{"Reconnect on kill", "", "DEFAULT_AUTO_RECONNECT",   ON, BOOL_TYPE, NULL},
{"Show away once", "", "DEFAULT_SHOW_AWAY_ONCE",   ON, BOOL_TYPE, NULL},
{"Show Nicks on channel join", "", "DEFAULT_SHOW_CHANNEL_NAMES",   ON, BOOL_TYPE, NULL},
{"Show end of server msgs", "", "DEFAULT_SHOW_END_OF_MSGS",   OFF, BOOL_TYPE, NULL},
{"Display Numerics", "", "DEFAULT_SHOW_NUMERICS",   OFF, BOOL_TYPE, NULL},
{"Show ALL status vars", "", "DEFAULT_SHOW_STATUS_ALL",   OFF, BOOL_TYPE, NULL},
{"Show who hop count", "", "DEFAULT_SHOW_WHO_HOPCOUNT",   OFF, BOOL_TYPE, NULL},
{"Send ignore msg on ignore", "", "DEFAULT_SEND_IGNORE_MSG",   OFF, BOOL_TYPE, NULL},
{"Hide Private channels", "", "DEFAULT_HIDE_PRIVATE_CHANNELS",   OFF, BOOL_TYPE, NULL},
{"Default IRC server port", "", "IRC_PORT",   6667, INT_TYPE, edit_dir },
{"After # of second turn on", "", "DEFAULT_CPU_SAVER_AFTER",   0, INT_TYPE, edit_dir },
{"Every # of seconds allow", "", "DEFAULT_CPU_SAVER_EVERY",   0, INT_TYPE, edit_dir },
{ NULL, NULL, NULL, 0, 0, NULL }
};

Configure various_default[] = {
{"Ping type", "0 1 2", "DEFAULT_PING_TYPE",   1, INT_TYPE, edit_dir },
{"Cloak", "Which ctcp\'s do we respond too. 0 all. 1. ping 2. none", "DEFAULT_CLOAK",	1, INT_TYPE, NULL },
{"Msg Logging while away", "", "DEFAULT_MSGLOG",   ON, BOOL_TYPE, NULL},
{"Auto-nslookup", "", "DEFAULT_AUTO_NSLOOKUP",   OFF, BOOL_TYPE, NULL},
{"LinkLook on/off", "", "DEFAULT_LLOOK",	OFF, BOOL_TYPE, NULL},
{"LinkLook Delay", "", "DEFAULT_LLOOK_DELAY",   120, INT_TYPE, edit_dir },
{"Split biggest window", "", "DEFAULT_ALWAYS_SPLIT_BIGGEST",   ON, BOOL_TYPE, NULL},
{"Auto whowas", "", "DEFAULT_AUTO_WHOWAS",   OFF, BOOL_TYPE, NULL},
{"Command Mode", "", "DEFAULT_COMMAND_MODE",   OFF, BOOL_TYPE, NULL},
{"Comment Hack for /*", "", "DEFAULT_COMMENT_HACK",   ON, BOOL_TYPE, NULL},
{"Display on/off", "", "DEFAULT_DISPLAY",   ON, BOOL_TYPE, NULL},
{"8-bit chars", "", "DEFAULT_EIGHT_BIT_CHARACTERS",   ON, BOOL_TYPE, NULL},
{"Enable exec protection", "", "DEFAULT_EXEC_PROTECTION",   ON, BOOL_TYPE, NULL},
{"Escape chars > 127", "", "DEFAULT_HIGH_BIT_ESCAPE",   OFF, BOOL_TYPE, NULL},
{"History", "", "DEFAULT_HISTORY",   100, INT_TYPE, edit_dir },
{"Screen paging", "", "DEFAULT_HOLD_MODE",   OFF, BOOL_TYPE, NULL},
{"Max lines to hold", "", "DEFAULT_HOLD_MODE_MAX",   0, INT_TYPE, edit_dir },

{"Lastlog items", "", "DEFAULT_LASTLOG",   1000, INT_TYPE, edit_dir },
{"Logging on/off", "", "DEFAULT_LOG",   OFF, BOOL_TYPE, NULL},
{"Enable Mail Check", " 0 1 2", "DEFAULT_MAIL",   2, INT_TYPE, edit_dir },
{"Notify on Terminate", "", "DEFAULT_NOTIFY_ON_TERMINATION",   OFF, BOOL_TYPE, NULL},
{"Scroll Enabled", "", "DEFAULT_SCROLL_LINES",   ON, BOOL_TYPE, NULL},
{"Process limit", "", "DEFAULT_SHELL_LIMIT",   0, INT_TYPE, edit_dir },
{"Number of Key Meta\'s", "", "DEFAULT_META_STATES",   5, INT_TYPE, edit_dir },

{"Max Deops", "", "DEFAULT_MAX_DEOPS",   2, INT_TYPE, edit_dir },
{"Max Idle check kicks", "", "DEFAULT_MAX_IDLEKICKS",   2, INT_TYPE, edit_dir },
{"Max Number bans", "", "DEFAULT_NUM_BANMODES",   4, INT_TYPE, edit_dir },
{"Num of kicks", "", "DEFAULT_NUM_KICKS",   4, INT_TYPE, edit_dir },
{"Num of Whowas", "", "DEFAULT_NUM_OF_WHOWAS",   4, INT_TYPE, edit_dir },
{"Num of OPmodes", "", "DEFAULT_NUM_OPMODES",   4, INT_TYPE, edit_dir },

{"Enable Help Pager", "", "DEFAULT_HELP_PAGER",   ON, BOOL_TYPE, NULL},
{"Enable Help Prompt", "", "DEFAULT_HELP_PROMPT",   ON, BOOL_TYPE, NULL},
{"Enable Help Window", "", "DEFAULT_HELP_WINDOW",   OFF, BOOL_TYPE, NULL},

{"FTP address grab", "", "DEFAULT_FTP_GRAB",   OFF, BOOL_TYPE, NULL},
{"HTTP address grab", "", "DEFAULT_HTTP_GRAB",   OFF, BOOL_TYPE, NULL},

{"Nick Completion on/off", "", "DEFAULT_NICK_COMPLETION",   ON, BOOL_TYPE, NULL},
{"Len of nick for complete", "", "DEFAULT_NICK_COMPLETION_LEN",   2, INT_TYPE, edit_dir },
{"Nick complete type", " 0 1 2", "DEFAULT_NICK_COMPLETION_TYPE",   0, INT_TYPE, edit_dir },

{"Full Status bar", "", "DEFAULT_FULL_STATUS_LINE",   ON, BOOL_TYPE, NULL},
{"Allow %> to repeat", "", "DEFAULT_STATUS_NO_REPEAT",   ON, BOOL_TYPE, NULL},
{"Double Status line", "", "DEFAULT_DOUBLE_STATUS_LINE",   ON, BOOL_TYPE, NULL},
{"Expand $ vars in status", "", "DEFAULT_STATUS_DOES_EXPANDOS", OFF, BOOL_TYPE, NULL},


{"Operview window hide", "", "DEFAULT_OPERVIEW_HIDE",   0, INT_TYPE, edit_dir },
{"Operview on/off", "", "DEFAULT_OPER_VIEW",   OFF, BOOL_TYPE, NULL},

{"Enable tabs", "", "DEFAULT_TAB",   ON, BOOL_TYPE, NULL},
{"# of Chars to a tab", "", "DEFAULT_TAB_MAX",   8, INT_TYPE, edit_dir },
{"Beep on/off", "", "DEFAULT_BEEP",   ON, BOOL_TYPE, NULL},
{"Beep MAX", "", "DEFAULT_BEEP_MAX",   3, INT_TYPE, edit_dir },
{"Beep when away", "", "DEFAULT_BEEP_WHEN_AWAY",   OFF, BOOL_TYPE, NULL},
{"Enable Bold video", "", "DEFAULT_BOLD_VIDEO",   ON, BOOL_TYPE, NULL},
{"Enable Blink video", "", "DEFAULT_BLINK_VIDEO",   ON, BOOL_TYPE, NULL},
{"Enable Inverse video", "", "DEFAULT_INVERSE_VIDEO",   ON, BOOL_TYPE, NULL},
{"Enable Underline video", "", "DEFAULT_UNDERLINE_VIDEO",   ON, BOOL_TYPE, NULL},
{"Clock display on/off", "", "DEFAULT_CLOCK",   ON, BOOL_TYPE, NULL},
{"24-Hour Clock display", "", "DEFAULT_CLOCK_24HOUR",   OFF, BOOL_TYPE, NULL},
{"Floating point math", "", "DEFAULT_FLOATING_POINT_MATH",   OFF, BOOL_TYPE, NULL},
{"Indent on/off", "", "DEFAULT_INDENT",   ON, BOOL_TYPE, NULL},
{"Enable Input aliases", "", "DEFAULT_INPUT_ALIASES",   OFF, BOOL_TYPE, NULL},
{"Enable insert mode", "", "DEFAULT_INSERT_MODE",   ON, BOOL_TYPE, NULL},

{"Display Ansi on/off", "", "DEFAULT_DISPLAY_ANSI",   ON, BOOL_TYPE, NULL},
{"Display char Mangling", "", "DEFAULT_DISPLAY_PC_CHARACTERS",   4, INT_TYPE, edit_dir },

{"# of Scrollback lines", "", "DEFAULT_SCROLLBACK_LINES",   512, INT_TYPE, edit_dir },
{"Scrollback ratio", "", "DEFAULT_SCROLLBACK_RATIO",   50, INT_TYPE, edit_dir },
{"MAX ND space", "", "DEFAULT_ND_SPACE_MAX",   160, INT_TYPE, edit_dir },
{"TabKey", "", "WANT_TABKEY",   ON, BOOL_TYPE, NULL },
{"NSlookup", "", "WANT_NSLOOKUP",   ON, BOOL_TYPE, NULL },
{"Bhelp command", "", "WANT_CHELP",   ON, BOOL_TYPE, NULL },
{"TimeStamp", "", "DEFAULT_TIMESTAMP", OFF, BOOL_TYPE, NULL },
{ "Default Alt Charset","Use the default ALT charset", "DEFAULT_ALT_CHARSET",			ON, BOOL_TYPE, NULL},
{ NULL, NULL, NULL, 0, 0, NULL }
};




/* Mark file and operated on special chars */
int mark_char = '*'; /* '*';  0x10;*/
int old_mark_char = '-'; /*= 0xfe;*/





int changed = 0;


/* Current path and filename storage areas */
char current_path[MAXPATH + 1];
char current_filename[MAXPATH + 1];

#define COLOR_MAIN 1
#define COLOR_DIALOG 2
#define COLOR_STATUS 3
#define COLOR_HELP 4
#define COLOR_DIRECTORY 5
#define COLOR_ONVOL 6
#define COLOR_SELECTED 7
#define COLOR_BOTTOM 8
#define COLOR_TITLE 9
char main_foreground = COLOR_WHITE, main_background = COLOR_BLUE, 
	 dialog_foreground, dialog_background;
char status_foreground, status_background, 
	 help_foreground, help_background;
char directory_foreground, directory_background, 
	 onvol_foreground, onvol_background;
char selected_foreground = COLOR_BLUE, selected_background = COLOR_WHITE, 
	 bottom_foreground, bottom_background;
char title_foreground, title_background;


#include "save.c"
#include "load.c"

int load_dir(CELL *c)
{
	load_file(current_path);
	return TRUE;
}




int exit_program (CELL *c)
{
	changed = 0;
	c->termkey = ALT_X;
	return TRUE;
}



void setup_colors()
{
/* Sets up color */
	
	if (has_colors()) 
	{
		init_pair(COLOR_MAIN, main_foreground, main_background);
		init_pair(COLOR_DIALOG, dialog_foreground, dialog_background);
		init_pair(COLOR_STATUS, status_foreground, status_background);
		init_pair(COLOR_HELP, help_foreground, help_background);
		init_pair(COLOR_DIRECTORY, directory_foreground, directory_background);
		init_pair(COLOR_ONVOL, onvol_foreground, onvol_background);
		init_pair(COLOR_SELECTED, selected_foreground, selected_background);
		init_pair(COLOR_BOTTOM, bottom_foreground, bottom_background);
		init_pair(COLOR_TITLE, title_foreground, title_background);
	}
}


/* Initialize the linked list to NULL */
void init_dlist ( CELL *c ) 
{
	c->start = c->end = c->list_start = c->list_end = c->current = NULL;
}

int toggle_select (CELL *c)
{
	if (c->current->datainfo.type == BOOL_TYPE)
	{
		if (c->current->datainfo.integer)
			c->current->datainfo.integer = 0;
		else
			c->current->datainfo.integer = 1;
		c->redraw = TRUE;
		return TRUE;
	} 
	if (c->current->datainfo.func)
		return (*c->current->datainfo.func) (c);
	return TRUE;
}

/* Add mark to current highlighted item */
int plus_it(CELL *c) 
{

	if (c->current->datainfo.type == INT_TYPE)
	{
		c->current->datainfo.integer++;
		c->redraw = TRUE;
	}
	return TRUE;
}

/* Take away mark from current highlighted item */
int minus_it(CELL *c) {
	if (c->current->datainfo.type == INT_TYPE && c->current->datainfo.integer)
	{
		c->current->datainfo.integer--;
		c->redraw = TRUE;
	}
	return TRUE;
}


int update_select(CELL *c)
{
	if (c->current->datainfo.func)
		return (*c->current->datainfo.func) (c);
	return TRUE;
}


/* clear list of files to play.. Make sure user wants this behavior */
int clear_marks(CELL *c) 
{
dlistptr ptr;
	ptr = c->start;
	while (ptr) 
	{
		ptr->datainfo.mark = 0;
		ptr = ptr->nextlistptr;
	}
	c->redraw = TRUE;
	return TRUE;
}

/* cleate all parts ot the the linked list */

int clear_dlist (CELL *c) 
{
	 dlistptr ptr;
	 while (c->start != NULL ) {
		ptr = c->start;
		c->start = c->start->nextlistptr;
		if (ptr->datainfo.option)
			free(ptr->datainfo.option);
		if (ptr->datainfo.help)
			free(ptr->datainfo.help);
		free(ptr);
	 }
	 c->end = NULL;
	return TRUE;
}

int List_Exit(CELL *c) {
	clear_dlist(c);
	return TRUE;
}


/*
 * Update status information including number of files, number of marked files
 * etc.
 */
int status_update(CELL *c) {
char tmp[(200 + 1) * 2];
int center;
	center = ((c->ecol - 2) / 2) - (strlen(c->filename) / 2);
	memset(tmp, 0, sizeof(tmp));
#if 0
	memset(tmp, ' ', center);
	strcat(tmp, c->filename);
	mvwaddstr(c->window, c->srow - 2, c->scol , tmp);
#else
	memset(tmp, ' ', c->ecol - 2);
	mvwaddstr (c->window, c->srow - 2 , c->scol, tmp);
	wattron(c->window,A_REVERSE);
	mvwaddstr (c->window, c->srow - 2 , center, c->filename);
	wattroff(c->window,A_REVERSE);
	
#endif
	if (c->current->datainfo.help)
	{
		sprintf(tmp, " %-75s ", c->current->datainfo.help);
		mvwaddstr(c->window, c->max_rows - 3, c->scol, tmp);
	}
	else
		mvwaddstr(c->window, c->max_rows - 2, c->scol, tmp);
	return TRUE;
}

/* Insert an item into the list. IN this case a filename and it's current
 * attribute's are stored in the list. Item is added to the end of the list.
 * and pointers are updated.
 */
int insert_fdlist (CELL *c, struct _config_type *current) {
	dlistptr ptr = NULL;

	ptr = (dlistptr) calloc(1, sizeof(struct dlistmem));
	if (c->start == NULL) /* start of new list */
		c->start = ptr;
	else 
	{
		ptr->prevlistptr = c->end;
		c->end->nextlistptr = ptr;
	}
	c->end = ptr;

	ptr->datainfo.option = (char *) malloc(strlen(current->option)+1);
	strcpy(ptr->datainfo.option, current->option);
	ptr->datainfo.help = (char *) malloc(strlen(current->help)+1);
	strcpy(ptr->datainfo.help, current->help);
	ptr->datainfo.func = current->func;
	ptr->datainfo.integer = current->integer;
	ptr->datainfo.type = current->type;
	return TRUE;

}

/* on entry to file list routine, setup screen windows and put menu options
 * on screen.
 * initilize the list.
 * Use findfirst/findnext to add filenames into list
 * Set display begin/end to start of list.
 * finally sort the list
 */

int File_Entry(CELL *c) {
	wmove(c->window,c->srow - 2, c->scol - 1);
	wborder(c->window,0,0,0,0,0,0,0,0);

	mvwaddch(c->window, c->srow - 1, c->max_cols, ACS_RTEE);
	mvwaddch(c->window, c->srow - 1, c->scol - 1, ACS_LTEE);
	wmove(c->window,c->srow - 1, c->scol);	
	whline(c->window, ACS_HLINE, c->max_cols - c->scol);

	mvwaddch(c->window, c->erow + 1, c->scol - 1, ACS_LTEE);
	mvwaddch(c->window, c->erow + 1, c->max_cols, ACS_RTEE);
	wmove(c->window,c->erow + 1, c->scol);	
	whline(c->window, ACS_HLINE, c->max_cols - c->scol);

	wrefresh(c->window);
	c->current = c->list_start = c->list_end = c->start;
	return TRUE;
}

/*
 * screen Display function, formats file info into displayable format
 * Uses some magic numbers for masking information.
 */
char *fDisplay (dlistptr *ptr)
{
	static char p[100];
	sprintf(p, " %-36s ", (*ptr)->datainfo.option);
	return p;
}

/*
 * File redraw routine. Draws current list on screen.
 */
int fredraw (CELL * c)
{
	register int row = c->srow;
	dlistptr p = c->list_start;	
	int i = 0;
	char buff[200];
	if (c->ecol - c->scol)
		sprintf(buff, "%*s",c->ecol - c->scol + 1, " ");
	while (i <= c->erow - c->srow && p != NULL) 
	{
		if (p == c->current) wattron(c->window,A_REVERSE);
			mvaddstr (row , c->scol, fDisplay(&p));
		if (p == c->current) wattroff(c->window,A_REVERSE);
			row++; i++;
		p = p->nextlistptr;
	}
	if (row <= c -> erow)
		for (; row <= c -> erow ; row++)
			mvaddstr(row, c->scol, buff);
	wrefresh(c->window);
	c -> redraw = FALSE;
	return TRUE;
}


char *cDisplay (dlistptr *ptr)
{
	static char p[100];
	if ((*ptr)->datainfo.type == BOOL_TYPE)
		sprintf(p, " %-28s %8s", (*ptr)->datainfo.option, (*ptr)->datainfo.integer? "On":"Off");
	else if ((*ptr)->datainfo.type == INT_TYPE)
		sprintf(p, " %-28s %8d", (*ptr)->datainfo.option, (*ptr)->datainfo.integer);
	return p;
}

/*
 * File redraw routine. Draws current list on screen.
 */
int credraw (CELL * c)
{
register int row = c->srow;
dlistptr p = c->list_start;	
int i = 0;
char buff[200];
	if (c->ecol - c->scol)
		sprintf(buff, "%*s",c->ecol - c->scol + 1, " ");

	while (i <= c->erow - c->srow && p != NULL) 
	{
		if (p == c->current) wattron(c->window,A_REVERSE);
			mvaddstr (row , c->scol, cDisplay(&p));
		if (p == c->current) wattroff(c->window,A_REVERSE);
			row++; i++;
		p = p->nextlistptr;
	}
	if (row <= c -> erow)
		for (; row <= c -> erow ; row++)
			mvaddstr(row, c->scol, buff);
	wrefresh(c->window);
	c -> redraw = FALSE;
	return TRUE;
}

/*
 * Basically setup values for where the box is to appear.
 * setup function pointers to default.
 * save the screen.
 *	loop
 *	  execute dispatch key routine
 *	until ESC or E or termkey is not equal to 0
 *	retore screen
 * and return to caller
 * true indicates that the function was successful in executing whatever
 * it was supposed to do.
 */
 /* In this case also
  * setup the old drive and path information,
  * set the current path to Quote Master export directory and
  * filename to worksheet extension
  */
int main_dir(CELL *c) {
int i;

	getcwd(current_path, sizeof(current_path));
	strcat(current_path, "/.config.h");
	load_file(current_path);
	
	c->keytable = file_cpy_menu;
	c->func_table = file_cpy_table;
	c->ListEntryProc = File_Entry;
	c->UpdateStatusProc = status_update;
	c->redraw = TRUE;
	c->ListExitProc = List_Exit;
	c->ListPaintProc = fredraw;

	c->srow = 3;
	c->scol = 1;
	c->erow = c->window->_maxy - 5;
	c->ecol = c->window->_maxx - 1;
	c->max_rows = c->window->_maxy;
	c->max_cols = c->window->_maxx;

	c->filename = "[ BitchX Config ]";
	c->menu_bar = 0;
	c->normcolor = 0x07;
	c->barcolor = 0x1f;
	init_dlist(c);
	for (i = 0; config_type[i].option; i++)
		insert_fdlist(c, &config_type[i]);


	/*
	* Go Do It
	*/
	do {
		c->redraw = TRUE;
		ls_dispatch(c);
	} while (c->termkey != ESC && c->termkey != ALT_X);
	return TRUE;
}

int Compile_Exit(CELL *c) {
dlistptr ptr = c->start;
int i;
	for (i = 0; ptr; ptr = ptr->nextlistptr, i++)
		compile_default[i].integer = ptr->datainfo.integer;
	clear_dlist(c);
	return TRUE;
}

int compile_dir(CELL *c) {
CELL compile = { 0 };
int i;
	compile.window = c->window;
	compile.keytable = file_cpy_menu;
	compile.func_table = compile_cpy_table;
	compile.ListEntryProc = File_Entry;
	compile.UpdateStatusProc = status_update;
	compile.redraw = TRUE;
	compile.ListExitProc = Compile_Exit;
	compile.ListPaintProc = credraw;

	compile.srow = 3;
	compile.scol = 1;
	compile.erow = compile.window->_maxy - 5;
	compile.ecol = compile.window->_maxx - 1;
	compile.max_rows = compile.window->_maxy;
	compile.max_cols = compile.window->_maxx;

	compile.filename = "[ BitchX Config ]";
	compile.menu_bar = 0;
	compile.normcolor = 0x07;
	compile.barcolor = 0x1f;
	init_dlist(&compile);
	for (i = 0; compile_default[i].option; i++)
		insert_fdlist(&compile, &compile_default[i]);


	/*
	* Go Do It
	*/
	do {
		compile.redraw = TRUE;
		ls_dispatch(&compile);
	} while (compile.termkey != ESC && compile.termkey != ALT_X);
	c->redraw = TRUE;
	return TRUE;
}

int userlist_Exit (CELL *c)
{
int i;
dlistptr ptr = c->start;
	for (i = 0; ptr; ptr = ptr->nextlistptr, i++)
		userlist_default[i].integer = ptr->datainfo.integer;
	clear_dlist(c);
	return TRUE;
}

int userlist_dir(CELL *c) {
CELL compile = { 0 };
int i;
	compile.window = c->window;
	compile.keytable = file_cpy_menu;
	compile.func_table = compile_cpy_table;
	compile.ListEntryProc = File_Entry;
	compile.UpdateStatusProc = status_update;
	compile.redraw = TRUE;
	compile.ListExitProc = userlist_Exit;
	compile.ListPaintProc = credraw;

	compile.srow = 3;
	compile.scol = 1;
	compile.erow = compile.window->_maxy - 5;
	compile.ecol = compile.window->_maxx - 1;
	compile.max_rows = compile.window->_maxy;
	compile.max_cols = compile.window->_maxx;

	compile.filename = "[ BitchX CSET/Userlist Config ]";
	compile.menu_bar = 0;
	compile.normcolor = 0x07;
	compile.barcolor = 0x1f;
	init_dlist(&compile);
	for (i = 0; userlist_default[i].option; i++)
		insert_fdlist(&compile, &userlist_default[i]);


	/*
	* Go Do It
	*/
	do {
		compile.redraw = TRUE;
		ls_dispatch(&compile);
	} while (compile.termkey != ESC && compile.termkey != ALT_X);
	c->redraw = TRUE;
	return TRUE;
}

int flood_Exit (CELL *c)
{
int i;
dlistptr ptr = c->start;
	for (i = 0; ptr; ptr = ptr->nextlistptr, i++)
		flood_default[i].integer = ptr->datainfo.integer;
	clear_dlist(c);
	return TRUE;
}

int flood_dir(CELL *c) {
CELL compile = { 0 };
int i;
	compile.window = c->window;
	compile.keytable = file_cpy_menu;
	compile.func_table = compile_cpy_table;
	compile.ListEntryProc = File_Entry;
	compile.UpdateStatusProc = status_update;
	compile.redraw = TRUE;
	compile.ListExitProc = flood_Exit;
	compile.ListPaintProc = credraw;

	compile.srow = 3;
	compile.scol = 1;
	compile.erow = compile.window->_maxy - 5;
	compile.ecol = compile.window->_maxx - 1;
	compile.max_rows = compile.window->_maxy;
	compile.max_cols = compile.window->_maxx;

	compile.filename = "[ BitchX Flood Config ]";
	compile.menu_bar = 0;
	compile.normcolor = 0x07;
	compile.barcolor = 0x1f;
	init_dlist(&compile);
	for (i = 0; flood_default[i].option; i++)
		insert_fdlist(&compile, &flood_default[i]);


	/*
	* Go Do It
	*/
	do {
		compile.redraw = TRUE;
		ls_dispatch(&compile);
	} while (compile.termkey != ESC && compile.termkey != ALT_X);

	c->redraw = TRUE;
	return TRUE;
}


int dcc_Exit (CELL *c)
{
int i;
dlistptr ptr = c->start;
	for (i = 0; ptr; ptr = ptr->nextlistptr, i++)
		dcc_default[i].integer = ptr->datainfo.integer;
	clear_dlist(c);
	return TRUE;
}

int dcc_dir(CELL *c) {
CELL compile = { 0 };
int i;
	compile.window = c->window;
	compile.keytable = file_cpy_menu;
	compile.func_table = compile_cpy_table;
	compile.ListEntryProc = File_Entry;
	compile.UpdateStatusProc = status_update;
	compile.redraw = TRUE;
	compile.ListExitProc = dcc_Exit;
	compile.ListPaintProc = credraw;

	compile.srow = 3;
	compile.scol = 1;
	compile.erow = compile.window->_maxy - 5;
	compile.ecol = compile.window->_maxx - 1;
	compile.max_rows = compile.window->_maxy;
	compile.max_cols = compile.window->_maxx;

	compile.filename = "[ BitchX DCC Config ]";
	compile.menu_bar = 0;
	compile.normcolor = 0x07;
	compile.barcolor = 0x1f;
	init_dlist(&compile);
	for (i = 0; dcc_default[i].option; i++)
		insert_fdlist(&compile, &dcc_default[i]);


	/*
	* Go Do It
	*/
	do {
		compile.redraw = TRUE;
		ls_dispatch(&compile);
	} while (compile.termkey != ESC && compile.termkey != ALT_X);

	c->redraw = TRUE;
	return TRUE;
}

int server_Exit (CELL *c)
{
int i;
dlistptr ptr = c->start;
	for (i = 0; ptr; ptr = ptr->nextlistptr, i++)
		server_default[i].integer = ptr->datainfo.integer;
	clear_dlist(c);
	return TRUE;
}

int server_dir(CELL *c) {
CELL compile = { 0 };
int i;
	compile.window = c->window;
	compile.keytable = file_cpy_menu;
	compile.func_table = compile_cpy_table;
	compile.ListEntryProc = File_Entry;
	compile.UpdateStatusProc = status_update;
	compile.redraw = TRUE;
	compile.ListExitProc = server_Exit;
	compile.ListPaintProc = credraw;

	compile.srow = 3;
	compile.scol = 1;
	compile.erow = compile.window->_maxy - 5;
	compile.ecol = compile.window->_maxx - 1;
	compile.max_rows = compile.window->_maxy;
	compile.max_cols = compile.window->_maxx;

	compile.filename = "[ BitchX Server Config ]";
	compile.menu_bar = 0;
	compile.normcolor = 0x07;
	compile.barcolor = 0x1f;
	init_dlist(&compile);
	for (i = 0; server_default[i].option; i++)
		insert_fdlist(&compile, &server_default[i]);


	/*
	* Go Do It
	*/
	do {
		compile.redraw = TRUE;
		ls_dispatch(&compile);
	} while (compile.termkey != ESC && compile.termkey != ALT_X);

	c->redraw = TRUE;
	return TRUE;
}

int various_Exit (CELL *c)
{
int i;
dlistptr ptr = c->start;
	for (i = 0; ptr; ptr = ptr->nextlistptr, i++)
		various_default[i].integer = ptr->datainfo.integer;
	clear_dlist(c);
	return TRUE;
}

int various_dir(CELL *c) {
CELL compile = { 0 };
int i;
	compile.window = c->window;
	compile.keytable = file_cpy_menu;
	compile.func_table = compile_cpy_table;
	compile.ListEntryProc = File_Entry;
	compile.UpdateStatusProc = status_update;
	compile.redraw = TRUE;
	compile.ListExitProc = various_Exit;
	compile.ListPaintProc = credraw;

	compile.srow = 3;
	compile.scol = 1;
	compile.erow = compile.window->_maxy - 5;
	compile.ecol = compile.window->_maxx - 1;
	compile.max_rows = compile.window->_maxy;
	compile.max_cols = compile.window->_maxx;

	compile.filename = "[ BitchX Various Config ]";
	compile.menu_bar = 0;
	compile.normcolor = 0x07;
	compile.barcolor = 0x1f;
	init_dlist(&compile);
	for (i = 0; various_default[i].option; i++)
		insert_fdlist(&compile, &various_default[i]);


	/*
	* Go Do It
	*/
	do {
		compile.redraw = TRUE;
		ls_dispatch(&compile);
	} while (compile.termkey != ESC && compile.termkey != ALT_X);

	c->redraw = TRUE;
	return TRUE;
}

char *eDisplay (dlistptr *ptr)
{
	static char p[100];
	char str[40];
	sprintf(str, "%d", (*ptr)->datainfo.integer);
	sprintf(p, "%14s", str);
	return p;
}

/*
 * File redraw routine. Draws current list on screen.
 */
int eredraw (CELL * c)
{
register int row = c->srow;
dlistptr p = c->list_start;	
int i = 0;
char buff[200];

	if (c->ecol - c->scol)
		sprintf(buff, "%*s",c->ecol - c->scol + 1, " ");
	
	while (i <= c->erow - c->srow && p != NULL) 
	{
		if (p == c->current) wattron(c->window,A_REVERSE);
			mvaddstr (row , c->scol+4, eDisplay(&p));
		if (p == c->current) wattroff(c->window,A_REVERSE);
			row++; i++;
		p = p->nextlistptr;
	}
	if (row <= c -> erow)
		for (; row <= c -> erow ; row++)
			mvaddstr(row, c->scol, buff);
	wrefresh(c->window);
	c -> redraw = FALSE;
	return TRUE;
}

int Edit_Entry(CELL *c) {
char tmp[180];
	memset(tmp, ' ', sizeof(tmp)-1);
	tmp[c->ecol - 2 - c->scol - 4] = 0;
	mvwaddstr (c->window, c->srow - 1 , c->scol, tmp);
	mvwaddstr (c->window, c->srow - 1, c->scol + 4, c->start->datainfo.option);
	wrefresh(c->window);
	c->current = c->list_start = c->list_end = c->start;
	return TRUE;
}

int edit_enter (CELL *c)
{
char tmp[180];
	memset(tmp, ' ', sizeof(tmp)-1);
	tmp[c->ecol - 2 - c->scol - 4] = 0;
	if (c->current->datainfo.type == INT_TYPE)
	{
		c->redraw = TRUE;
		c->termkey = '\n';
	}
	mvwaddstr (c->window, c->srow , c->scol, tmp);
	return TRUE;
}

int edit_line(CELL *c)
{
int found = 0;
int end = 0;
	if (c->current->datainfo.type == INT_TYPE)
		found = INT_TYPE;
	else if (c->current->datainfo.type == STR_TYPE)
		found = STR_TYPE;
	if (!found || ((found == INT_TYPE) && !isdigit(c->key)) || ((found == STR_TYPE) && !isalpha(c->key)))
		if (!iscntrl(c->key))
			return TRUE;
	end = strlen(c->current->datainfo.save);
	switch(c->key)
	{
		case KEY_BACKSPACE:
			if (*c->current->datainfo.save)
				c->current->datainfo.save[end - 1] = 0;
			break;
		default:
			if (iscntrl(c->key))
				return TRUE;
			if (end < MAXLEN)
				c->current->datainfo.save[end] = c->key;
			else
				beep();
	}
	if (found == INT_TYPE)
	{
		unsigned long temp = 0;
		if (*c->current->datainfo.save)
		{
			temp = strtoul(c->current->datainfo.save, NULL, 10);
			if (temp < 2147483647)
				c->current->datainfo.integer = temp;
			else
				c->current->datainfo.save[end] = 0;
		}
		else
			c->current->datainfo.integer = 0;
	}
	c->redraw = TRUE;
	return TRUE;
}

int edit_exit(CELL *c)
{
char tmp[180];
	memset(tmp, ' ', sizeof(tmp)-1);
	tmp[c->ecol - 2 - c->scol - 4] = 0;
	mvwaddstr (c->window, c->srow - 1 , c->scol, tmp);
	mvwaddstr (c->window, c->srow , c->scol, tmp);
	return TRUE;
}

int edit_dir(CELL *c) 
{
CELL edit = { 0 };
dlistptr ptr = NULL;

	edit.window = c->window;
	edit.keytable = file_cpy_menu;
	edit.func_table = edit_cpy_table;
	edit.ListEntryProc = Edit_Entry;
	edit.redraw = TRUE;
	edit.ListPaintProc = eredraw;
	edit.ListExitProc = edit_exit;
	
	edit.current_event = edit_line;

	edit.srow = edit.window->_maxy - 2;
	edit.scol = 1;
	edit.erow = edit.window->_maxy - 2;
	edit.ecol = edit.window->_maxx - 1;
	edit.max_rows = edit.window->_maxy;
	edit.max_cols = edit.window->_maxx;
	
	edit.menu_bar = 0;
	edit.normcolor = 0x07;
	edit.barcolor = 0x1f;
	
	ptr = (dlistptr) calloc(1, sizeof(struct dlistmem));
	edit.start = ptr;
	ptr->datainfo.option = (char *) malloc(strlen(c->current->datainfo.option)+1);
	strcpy(ptr->datainfo.option, c->current->datainfo.option);
	ptr->datainfo.integer = c->current->datainfo.integer;
	ptr->datainfo.type = c->current->datainfo.type;
	ptr->datainfo.save = (char *) malloc(MAXLEN+1);
	memset(ptr->datainfo.save, 0, MAXLEN);	

	/*
	* Go Do It
	*/
	do 
	{
		edit.redraw = TRUE;
		ls_dispatch(&edit);
	} while (edit.termkey != ESC && edit.termkey != ALT_X && edit.termkey != '\n');
	c->redraw = TRUE;
	if (edit.termkey == '\n')	
		c->current->datainfo.integer = edit.current->datainfo.integer;
	clear_dlist(&edit);
	return TRUE;
}

int main() {
CELL file_cpy = {0};
WINDOW *mainwin;
	mainwin = initscr();
	start_color();
	setup_colors();
	cbreak(); 
	noecho(); 
	keypad(mainwin, TRUE);
	meta(mainwin, TRUE);
	raw(); 

	leaveok(mainwin, TRUE);
	wbkgd(mainwin, COLOR_PAIR(COLOR_MAIN));
	wattron(mainwin, COLOR_PAIR(COLOR_MAIN));
	werase(mainwin);
	refresh();

	file_cpy.window = mainwin;

	main_dir(&file_cpy);

	wbkgd(mainwin, A_NORMAL);
	werase(mainwin);
	echo(); 
	nocbreak(); 
	noraw();
	refresh();
	endwin();
	return TRUE;
}
