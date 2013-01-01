/*
 * functions.c -- Built-in functions for ircII
 *
 * Written by Michael Sandrof
 * Copyright(c) 1990 Michael Sandrof
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 */

#include "irc.h"
static char cvsrevision[] = "$Id: functions.c 204 2012-06-02 14:53:37Z keaston $";
CVS_REVISION(functions_c)
#include "struct.h"

#include "alias.h"
#include "alist.h"
#include "array.h"
#include "dcc.h"
#include "commands.h"
#include "files.h"
#include "history.h"
#include "hook.h"
#include "ignore.h"
#include "input.h"
#include "ircaux.h"
#include "names.h"
#include "output.h"
#include "list.h"
#include "parse.h"
#include "screen.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "window.h"
#include "ircterm.h"
#include "notify.h"
#include "misc.h"
#include "userlist.h"
#include "numbers.h"
#include "ignore.h"
#include "hash2.h"
#include "struct.h"
#include "cset.h"
#include "log.h"
#include "gui.h"
#define MAIN_SOURCE
#include "modval.h"

#include "options.h"
#include <sys/stat.h>

#ifdef HAVE_REGEX_H
#include <regex.h>
#endif
#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

#ifdef WANT_HEBREW
#include "hebrew.h"
#endif

#include "tcl_bx.h"

static	char	*alias_detected		(void);
static	char	*alias_sent_nick 	(void);
static	char	*alias_recv_nick 	(void);
static	char	*alias_msg_body 	(void);
static	char	*alias_joined_nick 	(void);
static	char	*alias_public_nick 	(void);
static	char	*alias_dollar		(void);
static	char	*alias_channel		(void);
static	char	*alias_server		(void);
static	char	*alias_query_nick	(void);
static	char	*alias_target		(void);
static	char	*alias_nick		(void);
static	char	*alias_invite		(void);
static	char	*alias_cmdchar		(void);
static	char	*alias_line		(void);
static	char	*alias_away		(void);
static	char	*alias_oper		(void);
static	char	*alias_chanop		(void);
static	char	*alias_modes		(void);
static	char	*alias_buffer		(void);
static	char	*alias_time		(void);
static	char	*alias_version		(void);
static	char	*alias_currdir		(void);
static	char	*alias_current_numeric	(void);
static	char	*alias_server_version	(void);
static  char	*alias_show_userhost	(void);
static	char	*alias_show_realname	(void);
static	char	*alias_online		(void);
static	char 	*alias_idle		(void);
static	char	*alias_version_str	(void);
static	char	*alias_version_str1	(void);
static	char	*alias_thingansi	(void);
static	char	*alias_uptime		(void);
static	char	*alias_serverport	(void);
static	char	*alias_tclsupport	(void);
static	char	*alias_current_network	(void);
static  char	*alias_bitchx		(void);
static	char	*alias_hookname		(void);
static	char	*alias_awaytime		(void);
static	char	*alias_thisaliasname	(void);
static	char	*alias_serverlag	(void);
static	char	*alias_currentwindow	(void);
static	char	*alias_serverlistsize	(void);

typedef struct
{
	char	name;
	char	*(*func) (void);
}	BuiltIns;

static	BuiltIns built_in[] =
{
	{ '.',		alias_sent_nick		},
	{ ',',		alias_recv_nick		},
	{ ':',		alias_joined_nick	},
	{ ';',		alias_public_nick	},
	{ '`',		alias_uptime		},
	{ '$',		alias_dollar		},
	{ 'A',		alias_away		},
	{ 'B',		alias_msg_body		},
	{ 'C',		alias_channel		},
	{ 'D',		alias_detected		},
	{ 'E',		alias_idle		},
	{ 'F',		alias_online		},
	{ 'G',		alias_thingansi		},
	{ 'H', 		alias_current_numeric	},
	{ 'I',		alias_invite		},
	{ 'J',		alias_version_str	},
	{ 'K',		alias_cmdchar		},
	{ 'L',		alias_line		},
	{ 'M',		alias_modes		},
	{ 'N',		alias_nick		},
	{ 'O',		alias_oper		},
	{ 'P',		alias_chanop		},
	{ 'Q',		alias_query_nick	},
	{ 'R',		alias_server_version	},
	{ 'S',		alias_server		},
	{ 'T',		alias_target		},
	{ 'U',		alias_buffer		},
	{ 'V',		alias_version		},
	{ 'W',		alias_currdir		},
	{ 'X',		alias_show_userhost	},
	{ 'Y',		alias_show_realname	},
	{ 'Z',		alias_time		},
	{ 'a',		alias_version_str1	},
	{ 'b',		alias_bitchx		},
	{ 'h',		alias_hookname		},
	{ 'l',		alias_serverlag		},
	{ 'n',		alias_current_network	},
	{ 's',		alias_serverport	},
	{ 't',		alias_thisaliasname	},
	{ 'u',		alias_awaytime		},
	{ 'v',		alias_tclsupport	},
	{ 'w',		alias_currentwindow	},
	{ 'z',		alias_serverlistsize	},
	{ 0,	 	NULL			}
};

/* the 30 "standard" functions */
static		char	*function_channels 	(char *, char *);
static		char	*function_connect 	(char *, char *);
static		char	*function_curpos 	(char *, char *);
		char	*function_decode 	(char *, unsigned char *);
static		char	*function_encode 	(char *, unsigned char *);
static		char	*function_index 	(char *, char *);
static		char	*function_ischannel 	(char *, char *);
static		char	*function_ischanop 	(char *, char *);
static		char	*function_ishalfop 	(char *, char *);
static		char	*function_left 		(char *, char *);
static		char	*function_listen 	(char *, char *);
static		char	*function_match 	(char *, char *);
static		char	*function_mid 		(char *, char *);
static		char	*function_notify	(char *, char *);
static		char	*function_pid 		(char *, char *);
static		char	*function_ppid 		(char *, char *);
static		char	*function_rand 		(char *, char *);
static		char	*function_right 	(char *, char *);
static		char	*function_rindex 	(char *, char *);
static		char	*function_rmatch 	(char *, char *);
static		char	*function_servers 	(char *, char *);
static		char	*function_srand 	(char *, char *);
static		char	*function_stime 	(char *, char *);
static		char	*function_strip 	(char *, char *);
static		char	*function_tdiff 	(char *, char *);
static		char	*function_tdiff2 	(char *, char *);
static		char	*function_time 		(char *, char *);
static		char	*function_tolower 	(char *, char *);
static		char	*function_toupper 	(char *, char *);
static		char	*function_userhost 	(char *, char *);
static		char	*function_winnum 	(char *, char *);
static		char	*function_winnam 	(char *, char *);
static		char	*function_winnames 	(char *, char *);
static		char	*function_word 		(char *, char *);
static		char	*function_utime		(char *, char *);
static		char	*function_umode		(char *, char *);
	
/* CDE added functions */
static		char 	*function_uptime	(char *, char *);
static		char	*function_cluster	(char *, char *);
static		char	*function_checkshit	(char *, char *);
static		char	*function_checkuser	(char *, char *);
static		char	*function_rot13		(char *, char *);
		char	*function_addtabkey	(char *, char *);
		char	*function_gettabkey	(char *, char *);
static		char	*function_lastnotice	(char *, char *);
static		char	*function_lastmessage	(char *, char *);
static  	char	*function_help		(char *, char *);
static		char	*function_isuser	(char *, char *);
/* Thanks Jordy */
static		char	*function_pad		(char *, char *);
static		char	*function_isban		(char *, char *);
static		char	*function_isop		(char *, char *);
static		char	*function_isvoice	(char *, char *);
static		char	*function_randomnick	(char *, char *);

static		char	*function_openserver	(char *, char *);
static		char	*function_readserver	(char *, char *);
static		char	*function_readchar	(char *, char *);
static		char	*function_writeserver	(char *, char *);
static		char	*function_closeserver	(char *, char *);
static		char	*function_getreason	(char *, char *);

/* the 53 "extended" functions */
static		char *	function_after 		(char *, char *);
static		char *	function_afterw 	(char *, char *);
static		char *	function_aliasctl	(char *, char *);
static		char *	function_ascii 		(char *, char *);
static		char *	function_before 	(char *, char *);
static		char *	function_beforew 	(char *, char *);
static		char *	function_center 	(char *, char *);
static		char *	function_cexist		(char *, char *);
static		char *	function_currchans	(char *, char *);
static		char *	function_channelmode	(char *, char *);
static		char *	function_channelnicks	(char *, char *);
static		char *	function_chngw 		(char *, char *);
static		char *	function_chop 		(char *, char *);
static		char *	function_chops 		(char *, char *);
static		char *	function_chr 		(char *, char *);
static		char *	function_close 		(char *, char *);
static		char *	function_common 	(char *, char *);
static		char *	function_convert 	(char *, char *);
static		char *	function_copattern 	(char *, char *);
static		char *	function_crypt 		(char *, char *);
static		char *	function_diff 		(char *, char *);
static		char *	function_epic 		(char *, char *);
static		char *	function_eof 		(char *, char *);
static		char *	function_fnexist	(char *, char *);
static		char *	function_glob	 	(char *, char *);
static		char *	function_fexist 	(char *, char *);
static		char *	function_filter 	(char *, char *);
static		char *	function_fromw 		(char *, char *);
static		char *	function_fsize	 	(char *, char *);
static		char *	function_geom		(char *, char *);
static		char *	function_info		(char *, char *);
static		char *	function_insertw 	(char *, char *);
static		char *	function_iptoname 	(char *, char *);
static		char *	function_isalpha 	(char *, char *);
static		char *	function_isdigit 	(char *, char *);
static		char *	function_jot 		(char *, char *);
static		char *	function_key 		(char *, char *);
static		char *	function_lastserver	(char *, char *);
static		char *	function_leftw 		(char *, char *);
static		char *	function_mkdir		(char *, char *);
static		char *	function_midw 		(char *, char *);
static		char *	function_nametoip 	(char *, char *);
static		char *	function_nochops 	(char *, char *);
static		char *	function_notw 		(char *, char *);
static		char *	function_numonchannel 	(char *, char *);
static		char *	function_numwords 	(char *, char *);
static		char *	function_numsort 	(char *, char *);
static		char *	function_onchannel 	(char *, char *);
static		char *	function_open 		(char *, char *);
static		char *	function_pass		(char *, char *);
static		char *	function_pattern 	(char *, char *);
static		char *	function_read 		(char *, char *);
static		char *	function_remw 		(char *, char *);
static		char *	function_rename 	(char *, char *);
static		char *	function_restw 		(char *, char *);
static		char *	function_reverse 	(char *, char *);
static		char *	function_revw 		(char *, char *);
static		char *	function_rfilter 	(char *, char *);
static		char *	function_rightw 	(char *, char *);
static		char *	function_rmdir 		(char *, char *);
static		char *	function_rpattern 	(char *, char *);
static		char *	function_sar 		(char *, char *);
static		char *	function_server_version (char *, char *);
static		char *	function_servername	(char *, char *);
static		char *	function_sort		(char *, char *);
static		char *	function_split 		(char *, char *);
static		char *	function_splice 	(char *, char *);
static		char *	function_stripansi	(char *, char *);
static		char *	function_stripansicodes	(char *, char *);
static		char *  function_strftime	(char *, char *);
static		char *	function_strlen		(char *, char *);
static		char *	function_tow 		(char *, char *);
static		char *	function_translate 	(char *, char *);
static		char *	function_truncate 	(char *, char *);
static		char *	function_unlink 	(char *, char *);
static		char *	function_umask		(char *, char *);
static		char *	function_which 		(char *, char *);
static		char *	function_winserv	(char *, char *);
static		char *	function_winsize	(char *, char *);
static		char *	function_write 		(char *, char *);
static		char *	function_writeb		(char *, char *);
static		char *  function_idle		(char *, char *);
static		char *	function_repeat		(char *, char *);
static		char *  function_bcopy		(char *, char *);
		char *	function_cparse		(char *, char *);
static		char *	function_chmod		(char *, char *);
static		char *	function_twiddle	(char *, char *);
static		char *	function_uniq		(char *, char *);
static		char *	function_uhost 		(char *, char *);
static		char *	function_numdiff	(char *, char *);
		char *	function_getkey		(char *, char *);
static		char *  function_winvisible	(char *, char *);
static		char *	function_mircansi	(char *, char *);
static		char *	function_banonchannel	(char *, char *);
static		char *	function_chanwin	(char *, char *);
static		char *	function_gethost	(char *, char *);
static		char *	function_getenv		(char *, char *);
static		char *	function_getvar		(char *, char *);
static		char *	function_status		(char *, char *);
		char *  function_push		(char *, char *);
		char *  function_pop		(char *, char *);
		char *  function_shift		(char *, char *);
		char *  function_unshift	(char *, char *);
static		char *	function_get_info	(char *, char *);
static		char *	function_set_info	(char *, char *);
static		char *	function_statsparse	(char *, char *);
static		char *	function_absstrlen	(char *, char *);
static		char *	function_findw		(char *, char *);
static		char *	function_countansi	(char *, char *);
static		char *	function_strstr		(char *, char *);
static		char *	function_substr		(char *, char *);
static		char *	function_longip		(char *, char *);	
static		char *	function_iplong		(char *, char *);	
static		char *	function_rword		(char *, char *);
static		char *	function_winlen		(char *, char *);
static		char *	function_isignored	(char *, char *);
static		char *	function_channel	(char *, char *);
static		char *	function_ftime		(char *, char *);
static		char *	function_irclib		(char *, char *);
static		char *	function_winbound	(char *, char *);
static		char *	function_rstrstr	(char *, char *);
static		char *	function_rsubstr	(char *, char *);
static		char *	function_country	(char *, char *);
static		char *	function_servernick	(char *, char *);
static		char *	function_fparse		(char *, char *);
static		char *	function_isconnected	(char *, char *);
static		char *	function_bmatch		(char *, char *);
static		char *	function_regcomp	(char *, char *);
static		char *	function_regexec	(char *, char *);
static		char *	function_regerror	(char *, char *);
static		char *	function_regfree	(char *, char *);
		char *	function_cdcc		(char *, char *);
		char *	function_sendcdcc	(char *, char *);
static		char *	function_count		(char *, char *);
static		char *	function_msar		(char *, char *);
static		char *	function_getcset	(char *, char *);
	
static		char *	function_leftpc		(char *, char *);
static		char *	function_mask		(char *, char *);
static		char *	function_querywin	(char *, char *);
static		char *	function_uname		(char *, char *);
static		char *	function_winrefs	(char *, char *);

static		char *	function_getgid		(char *, char *);
static		char *	function_getlogin	(char *, char *);
static		char *	function_getpgrp	(char *, char *);
static		char *	function_getuid		(char *, char *);
static		char *	function_iscurchan	(char *, char *);
static		char *	function_remws		(char *, char *);
static		char *	function_uhc		(char *, char *);	
static		char *	function_deuhc		(char *, char *);	
static		char *	function_getfsets	(char *, char *);
static		char *	function_rest		(char *, char *);
static		char *	function_isnumber	(char *, char *);
static		char *	function_getsets	(char *, char *);
static		char *	function_servref	(char *, char *);
static		char *	function_getflags	(char *, char *);
static		char *	function_numlines	(char *, char *);
static		char *	function_winlevel	(char *, char *);
static		char *	function_stripc		(char *, char *);
static		char *	function_topic		(char *, char *);
static		char *	function_stripcrap	(char *, char *);
#if 0	
static		char *	function_parse_and_return (char *, char *);
#endif
static		char *	function_dccitem	(char *, char *);
static		char *	function_winitem	(char *, char *);
static  	char *  function_servgroup      (char *, char *);
static  	char *  function_ajoinitem      (char *, char *);
static		char *	function_long_to_comma	(char *, char *);
static		char *	function_nohighlight	(char *, char *);
static		char *	function_getopt		(char *, char *);
static		char *	function_isaway		(char *, char *);
static		char *	function_banwords	(char *, char *);
static		char *	function_hash_32bit	(char *, char *);
	
static		char *	function_nickcomp	(char *, char *);
static		char *	function_filecomp	(char *, char *);
static		char *	function_log		(char *, char *);
static		char *	function_indextoword	(char *, char *);
static		char *	function_ttyname	(char *, char *);
static		char *	function_functioncall	(char *, char *);
static		char *	function_prefix		(char *, char *);
static		char *	function_stat		(char *, char *);
static		char *	function_maxlen		(char *, char *);
static		char *	function_insert		(char *, char *);
static		char *	function_realpath	(char *, char *);        
static		char *	function_serverlag	(char *, char *);
static		char *	function_servports	(char *, char *);
static		char *	function_isalnum	(char *, char *);
static		char *	function_isspace	(char *, char *);
static		char *	function_isxdigit	(char *, char *);
static		char *	function_serverpass	(char *, char *);
static		char *	function_igmask		(char *, char *);
static		char *	function_rigmask	(char *, char *);
static		char *	function_igtype		(char *, char *);
static		char *	function_rigtype	(char *, char *);
extern		char *	function_istimer	(char *, char *);
static		char *	function_strchar	(char *, char *);
static		char *	function_watch		(char *, char *);
static		char *	function_getcap		(char *, char *);
static		char *	function_isdisplaying	(char *, char *);
static		char *	function_getset		(char *, char *);
static		char *	function_builtin	(char *, char *);
extern		char *	function_timer		(char *, char *);
static		char *	function_runlevel		(char *, char *);
static		char *	function_ovserver		(char *, char *);
        
#ifdef GUI
static		char *  function_mciapi         (char *, char *);
static		char *  function_lastclickline  (char *, char *);
static		char *  function_lastclickx     (char *, char *);
static		char *  function_lastclicky     (char *, char *);
static		char *  function_screensize     (char *, char *);
static		char *  function_menucontrol    (char *, char *);
#endif

static		char *	function_ipv6		(char *, char *);

#undef BUILT_IN_FUNCTION
#define BUILT_IN_FUNCTION(x, y) static char * x (char *fn, char * y)

/* 
 * This is the built-in function list.  This list *must* be sorted because
 * it is binary searched.   See the code for each function to see how it
 * is used.  Or see the help files.  Or see both.  Or look at the code
 * and see how it REALLY works, irregardless of the documentation >;-)
 */
static BuiltInFunctions built_in_functions[] =
{
	{ "ABSSTRLEN",		function_absstrlen	},
	{ "ADDTABKEY",		function_addtabkey	},
	{ "AFTER",              function_after 		},
	{ "AFTERW",             function_afterw 	},
	{ "AJOINITEM",  	function_ajoinitem      },
	{ "ALIASCTL",		function_aliasctl	},
	{ "ASCII",              function_ascii 		},
	{ "BANONCHANNEL",	function_banonchannel	},
	{ "BANWORDS",		function_banwords	},
	{ "BCOPY",		function_bcopy		},
	{ "BEFORE",             function_before 	},
	{ "BEFOREW",            function_beforew 	},
	{ "BITCHX",		function_epic		},
	{ "BMATCH",		function_bmatch		},
	{ "BUILTIN_EXPANDO",	function_builtin	},
	{ "CENTER",		function_center 	},
	{ "CEXIST",		function_cexist		},
	{ "CHANMODE",		function_channelmode	},
	{ "CHANNEL",		function_channel	},
	{ "CHANNICKS",		function_channelnicks	},
	{ "CHANUSERS",		function_onchannel 	},
	{ "CHANWIN",		function_chanwin	},
	{ "CHANWINDOW",		function_chanwin	},
	{ "CHECKSHIT",		function_checkshit	},
	{ "CHECKUSER",		function_checkuser	},
	{ "CHMOD",		function_chmod		},
	{ "CHNGW",              function_chngw 		},
	{ "CHOP",		function_chop		},
	{ "CHOPS",              function_chops 		},
	{ "CHR",                function_chr 		},
	{ "CLOSE",		function_close 		},
#ifndef BITCHX_LITE
	{ "CLOSESOCKET",	function_closeserver	},
#endif
	{ "CLUSTER",		function_cluster	},
	{ "COMMON",             function_common 	},
	{ "CONNECT",		function_connect 	},
	{ "CONVERT",		function_convert 	},
	{ "COPATTERN",          function_copattern 	},
	{ "COUNT",		function_count		},
	{ "COUNTANSI",		function_countansi	},
	{ "COUNTRY",		function_country	},
	{ "CPARSE",		function_cparse		},
	{ "CRYPT",		function_crypt		},
	{ "CURPOS",		function_curpos 	},
	{ "CURRCHANS",		function_currchans	},
	{ "DCCITEM",		function_dccitem	},
	{ "DECODE",	  (bf *)function_decode 	},
	{ "DELARRAY",           function_delarray 	},
	{ "DELITEM",            function_delitem 	},
	{ "DEUHC",		function_deuhc		},
	{ "DIFF",               function_diff 		},
	{ "ENCODE",	  (bf *)function_encode 	},
	{ "EOF",		function_eof 		},
	{ "EPIC",		function_epic		},
	{ "FEXIST",             function_fexist 	},
	{ "FILECOMP",		function_filecomp	},
	{ "FILTER",             function_filter 	},
	{ "FINDITEM",           function_finditem 	},
	{ "FINDW",		function_findw		},
	{ "FNEXIST",		function_fnexist	},
	{ "FPARSE",		function_fparse		},
	{ "FROMW",              function_fromw 		},
	{ "FSIZE",		function_fsize		},
	{ "FTIME",		function_ftime		},
	{ "FUNCTIONCALL",	function_functioncall	},
	{ "GEOM",		function_geom		},
	{ "GETARRAYS",          function_getarrays 	},
	{ "GETCAP",		function_getcap		},
	{ "GETCDCC",		function_cdcc		},
	{ "GETCSET",		function_getcset	},
	{ "GETGID",		function_getgid		},
	{ "GETENV",		function_getenv		},
	{ "GETFLAGS",		function_getflags	},
	{ "GETFSETS",		function_getfsets	},
	{ "GETHOST",		function_gethost	},
	{ "GETINFO",		function_get_info	},
	{ "GETITEM",            function_getitem 	},
	{ "GETKEY",		function_getkey		},
	{ "GETLOGIN",		function_getlogin	},
	{ "GETMATCHES",         function_getmatches 	},
	{ "GETOPT",		function_getopt		},
	{ "GETPGRP",            function_getpgrp        },
	{ "GETREASON",		function_getreason	}, 
	{ "GETRMATCHES",        function_getrmatches 	},
	{ "GETSET",		function_getset		},
	{ "GETSETS",		function_getsets	},
	{ "GETTABKEY",		function_gettabkey	},
	{ "GETTMATCH",		function_gettmatch	},
	{ "GETUID",             function_getuid         },
	{ "GETVAR",		function_getvar		},
	{ "GLOB",		function_glob		},
#ifdef GTK
	{ "GTKBITCHX",		function_epic		},
#endif
	{ "HASH_32BIT",		function_hash_32bit	},
	{ "HELP",		function_help		},
	{ "IDLE",		function_idle		},
	{ "IFINDFIRST",         function_ifindfirst 	},
	{ "IFINDITEM",          function_ifinditem 	},
	{ "IGETITEM",           function_igetitem 	},
	{ "IGETMATCHES",	function_igetmatches	},
	{ "IGMASK",		function_igmask		},
	{ "IGTYPE",		function_igtype		},
	{ "INDEX",		function_index 		},
	{ "INDEXTOITEM",        function_indextoitem 	},
	{ "INDEXTOWORD",	function_indextoword	},
	{ "INFO",		function_info		},
	{ "INSERT",		function_insert		},
	{ "INSERTW",            function_insertw 	},
	{ "IPLONG",		function_iplong		},
	{ "IPTONAME",		function_iptoname 	},
	{ "IPV6",		function_ipv6		},
	{ "IRCLIB",		function_irclib		},
	{ "ISALNUM",		function_isalnum	},
	{ "ISALPHA",		function_isalpha 	},
	{ "ISAWAY",		function_isaway		},
	{ "ISBAN",		function_isban		},
	{ "ISCHANNEL",		function_ischannel 	},
	{ "ISCHANOP",		function_ischanop 	},
	{ "ISCHANVOICE", 	function_isvoice	},
	{ "ISCONNECTED",	function_isconnected	},
	{ "ISCURCHAN",          function_iscurchan      },
	{ "ISDIGIT",		function_isdigit 	},
	{ "ISDISPLAYING",	function_isdisplaying	},
	{ "ISHALFOP",		function_ishalfop 	},
	{ "ISIGNORED",		function_isignored	},
	{ "ISNUMBER",		function_isnumber	},
	{ "ISOP",		function_isop		},
	{ "ISSPACE",		function_isspace	},
	{ "ISTIMER",		function_istimer	},
	{ "ISUSER",		function_isuser		},
	{ "ISVOICE",		function_isvoice	},
	{ "ISXDIGIT",		function_isxdigit	},
	{ "ITEMTOINDEX",        function_itemtoindex 	},
	{ "JOT",                function_jot 		},
	{ "KEY",                function_key 		},
	{ "LAG",		function_serverlag	},

#ifdef GUI
	{ "LASTCLICKLINE",	function_lastclickline	},
	{ "LASTCLICKX",	        function_lastclickx     },
	{ "LASTCLICKY",	        function_lastclicky	},
#endif
	{ "LASTLOG",		function_lastlog	},
	{ "LASTMESSAGE",	function_lastmessage	},
	{ "LASTNOTICE",		function_lastnotice	},
	{ "LASTSERVER",		function_lastserver	},
	{ "LEFT",		function_left 		},
	{ "LEFTPC",		function_leftpc		},
	{ "LEFTW",              function_leftw 		},
	{ "LENGTH",		function_strlen		},
	{ "LINE",		function_line		},
	{ "LISTARRAY",		function_listarray	},
	{ "LISTEN",		function_listen 	},
	{ "LOG",		function_log		},
	{ "LONGCOMMA",		function_long_to_comma	},
	{ "LONGIP",		function_longip		},
	{ "MASK",		function_mask		},
	{ "MATCH",		function_match 		},
	{ "MATCHITEM",          function_matchitem 	},
	{ "MAXLEN",		function_maxlen		},
#ifdef GUI
	{ "MCIAPI",             function_mciapi         },
	{ "MENUCONTROL",        function_menucontrol    },
#endif
	{ "MID",		function_mid 		},
	{ "MIDW",               function_midw 		},
#ifndef BITCHX_LITE
	{ "MIRCANSI",		function_mircansi	},
#endif
	{ "MKDIR",		function_mkdir		},
	{ "MSAR",		function_msar		},
	{ "MYCHANNELS",		function_channels 	},
	{ "MYSERVERS",		function_servers 	},
	{ "NAMETOIP",		function_nametoip 	},
	{ "NICKCOMP",		function_nickcomp	},
	{ "NOCHOPS",            function_nochops 	},
	{ "NOHIGHLIGHT",	function_nohighlight	},
	{ "NOTIFY",		function_notify		},
	{ "NOTW",               function_notw 		},
	{ "NUMARRAYS",          function_numarrays 	},
	{ "NUMDIFF",		function_numdiff	},
	{ "NUMITEMS",           function_numitems 	},
	{ "NUMLINES",		function_numlines	},
	{ "NUMONCHANNEL",	function_numonchannel 	},
	{ "NUMSORT",		function_numsort	},
	{ "NUMWORDS",		function_numwords	},
	{ "ONCHANNEL",          function_onchannel 	},
	{ "OPEN",		function_open 		},
#ifndef BITCHX_LITE
	{ "OPENSOCKET",		function_openserver	},
#endif
	{ "OVSERVER",		function_ovserver	},
	{ "PAD",		function_pad		},
#if 0
	{ "PARSE",		function_parse_and_return},
#endif
	{ "PASS",		function_pass		},
	{ "PATTERN",            function_pattern 	},
	{ "PID",		function_pid 		},
#ifdef __EMXPM__
	{ "PMBITCHX",		function_epic		},
#endif
	{ "POP",		function_pop 		},
	{ "PPID",		function_ppid 		},
	{ "PREFIX",		function_prefix		},
	{ "PRINTLEN",		function_countansi	},
	{ "PUSH",		function_push 		},
	{ "QUERYWIN",		function_querywin	},
	{ "RAND",		function_rand 		},
	{ "RANDOMNICK",		function_randomnick	},
	{ "READ",		function_read 		},
	{ "REALPATH",		function_realpath	},
	{ "REGCOMP",		function_regcomp	},
	{ "REGERROR",		function_regerror	},
	{ "REGEXEC",		function_regexec	},
	{ "REGFREE",		function_regfree	},
#ifndef BITCHX_LITE
	{ "READCHAR",		function_readchar	},
	{ "READSOCKET",		function_readserver	},
#endif
	{ "REMW",               function_remw 		},
	{ "REMWS",		function_remws		},
	{ "RENAME",		function_rename 	},
	{ "REPEAT",		function_repeat		},
	{ "REST",		function_rest		},
	{ "RESTW",              function_restw 		},
	{ "REVERSE",            function_reverse 	},
	{ "REVW",               function_revw 		},
	{ "RFILTER",            function_rfilter 	},
	{ "RIGHT",		function_right 		},
	{ "RIGHTW",             function_rightw 	},
	{ "RIGMASK",		function_rigmask	},
	{ "RIGTYPE",		function_rigtype	},
	{ "RINDEX",		function_rindex		},
	{ "RMATCH",		function_rmatch		},
	{ "RMATCHITEM",         function_rmatchitem 	},
	{ "RMDIR",		function_rmdir 		},
	{ "ROT13",		function_rot13		},
	{ "RPATTERN",           function_rpattern 	},
	{ "RSTRSTR",		function_rstrstr	},
	{ "RSUBSTR",		function_rsubstr	},
	{ "RUNLEVEL",		function_runlevel	},
	{ "RWORD",		function_rword		},
	{ "SAR",		function_sar 		},
#ifdef GUI
	{ "SCREENSIZE",         function_screensize     },
#endif
	{ "SENDCDCC",		function_sendcdcc	},
	{ "SERVERGROUP",	function_servgroup	},
	{ "SERVERNAME",		function_servername	},
	{ "SERVERNICK",		function_servernick	},
	{ "SERVERNUM",		function_servref	},
	{ "SERVERPASS",		function_serverpass	},
	{ "SERVPORTS",		function_servports	},
	{ "SETINFO",		function_set_info	},
	{ "SERVERPORT",		function_servports	},
	{ "SETITEM",            function_setitem 	},
	{ "SHIFT",		function_shift 		},
	{ "SORT",		function_sort		},
	{ "SPLICE",		function_splice 	},
	{ "SPLIT",		function_split 		},
	{ "SRAND",		function_srand 		},
	{ "STAT",		function_stat		},
	{ "STATSPARSE",		function_statsparse	},
	{ "STATUS",		function_status		},
	{ "STIME",		function_stime 		},
	{ "STRCHR",		function_strchar	},
	{ "STRFTIME",		function_strftime	},
	{ "STRIP",		function_strip 		},
	{ "STRIPANSI",		function_stripansi	},
	{ "STRIPANSICODES",	function_stripansicodes	},
	{ "STRIPC",		function_stripc		},
	{ "STRIPCRAP",		function_stripcrap	},
#ifndef BITCHX_LITE
	{ "STRIPMIRC",		function_stripc		},
#endif
	{ "STRLEN",		function_strlen		},
	{ "STRRCHR",		function_strchar	},
	{ "STRSTR",		function_strstr		},
	{ "SUBSTR",		function_substr		},
	{ "TDIFF",		function_tdiff 		},
	{ "TDIFF2",		function_tdiff2 	},
	{ "TIME",		function_time 		},
	{ "TIMER",		function_timer		},
	{ "TIMEREXISTS",	function_istimer	},
	{ "TOLOWER",		function_tolower 	},
	{ "TOPIC",		function_topic		},
	{ "TOUPPER",		function_toupper 	},
	{ "TOW",                function_tow 		},
	{ "TR",			function_translate 	},
	{ "TRUNC",		function_truncate 	},
	{ "TTYNAME",		function_ttyname	},
	{ "TWIDDLE",		function_twiddle	},
	{ "UHC",		function_uhc		},
	{ "UHOST",		function_uhost		},
	{ "UMASK",		function_umask		},
	{ "UNAME",		function_uname		},
	{ "UNIQ",		function_uniq		},
	{ "UNLINK",		function_unlink 	},
	{ "UNSHIFT",		function_unshift 	},
	{ "UPTIME",		function_uptime		},
	{ "USERHOST",		function_userhost 	},
	{ "USERMODE",		function_umode		},
	{ "UTIME",		function_utime	 	},
	{ "VERSION",		function_server_version },
	{ "WATCH",		function_watch		},
	{ "WHICH",		function_which 		},
	{ "WINBOUND",		function_winbound	},
	{ "WINCHAN",		function_chanwin	},
	{ "WINITEM",		function_winitem	},
	{ "WINLEN",		function_winlen		},
	{ "WINLEVEL",		function_winlevel	},
	{ "WINNAM",		function_winnam 	},
	{ "WINNICKLIST",	function_winnames 	},
	{ "WINNUM",		function_winnum 	},
	{ "WINREFS",		function_winrefs	},
	{ "WINSERV",		function_winserv	},
	{ "WINSIZE",		function_winsize	},
	{ "WINVISIBLE",		function_winvisible	},
	{ "WORD",		function_word 		},
	{ "WRITE",		function_write 		},
	{ "WRITEB",		function_writeb		},
#ifndef BITCHX_LITE
	{ "WRITESOCKET",	function_writeserver	},
#endif
	{ NULL,			NULL 			}
};

#define	NUMBER_OF_FUNCTIONS (sizeof(built_in_functions) / sizeof(BuiltInFunctions)) - 2

#include "hash2.h"

#define FUNCTION_HASHSIZE 251
HashEntry functions[FUNCTION_HASHSIZE] = { { NULL } };

int done_init_functions = 0;


static int	func_exist (char *name)
{
	int cnt, pos;
	char *tmp;

	tmp = LOCAL_COPY(name);
	upper(tmp);

	find_fixed_array_item(built_in_functions, sizeof(BuiltInFunctions), NUMBER_OF_FUNCTIONS + 1, tmp, &cnt, &pos);
	if (cnt < 0)
		return 1;

	return 0;
}

int in_cparse  = 0;

#ifdef WANT_TCL

void add_tcl_alias (Tcl_Interp *tcl_interp, void *func1, void *func2)
{
int i = 0;
char str[80];
	while (built_in_functions[i].func)
	{
		sprintf(str, "_%s", built_in_functions[i].name);
		Tcl_CreateCommand(tcl_interp, lower(str), func1?func1:built_in_functions[i].func, NULL, NULL);
		i++;
	}
	i = 0;
	while (built_in[i].func)
	{
		sprintf(str, "_%c", built_in[i].name);
		Tcl_CreateCommand(tcl_interp, str, func2?func2:built_in[i].func, NULL, NULL);
		i++;
	}
}

#endif

typedef struct _built_in_new {
	struct _built_in_new *next;
	char *name;
	char *(*func)(char *, char *);
} NewBuiltInFunctions;
	
static inline void move_link_to_top(NewBuiltInFunctions *tmp, NewBuiltInFunctions *prev, HashEntry *location)
{
	if (prev) 
	{
		NewBuiltInFunctions *old_list;
		old_list = (NewBuiltInFunctions *) location->list;
		location->list = (void *) tmp;
		prev->next = tmp->next;
		tmp->next = old_list;
	}
}

static void init_functions(void)
{
int i;
unsigned long hash;
NewBuiltInFunctions *new;

	for (i = 0; i <= NUMBER_OF_FUNCTIONS; i++)
	{
		if (!built_in_functions[i].func)
			break;
		hash = hash_nickname(built_in_functions[i].name, FUNCTION_HASHSIZE);
		new = (NewBuiltInFunctions *)new_malloc(sizeof(NewBuiltInFunctions));
		new->name = built_in_functions[i].name;
		new->func = built_in_functions[i].func;
		new->next = (NewBuiltInFunctions *)functions[hash].list;
		functions[hash].list = (void *) new;
		functions[hash].links++;
	}
	done_init_functions++;
}

BUILT_IN_COMMAND(debugfunc)
{
NewBuiltInFunctions *ptr;
int i;
	for (i = 0; i < FUNCTION_HASHSIZE; i++)
	{
		if (!(ptr = (NewBuiltInFunctions *)functions[i].list))
			continue;
		while (ptr)
		{
			put_it("DEBUG_FUNC[%d]: %s  %d links  %d hits", i, ptr->name, functions[i].links, functions[i].hits);
			ptr = ptr->next;
		}
	}
	
}

BuiltIns *find_func_aliasvar(char *name)
{
BuiltIns *tmp = NULL;
int i = 0;
	while (built_in[i].func)
	{
		if (!(*name == built_in[i].name))
		{
			i++;
			continue;
		}
		tmp = &built_in[i];
		break;
	}
	return tmp; 
}

BuiltInFunctions *find_func_alias(char *name)
{
unsigned long hash = 0;
HashEntry *location;
register NewBuiltInFunctions *tmp, *prev = NULL;
static BuiltInFunctions new;

	if (!done_init_functions)
		init_functions();
	hash = hash_nickname(name, FUNCTION_HASHSIZE);
	location = &functions[hash];	
	for (tmp = (NewBuiltInFunctions *)location->list; tmp; tmp = tmp->next)
	{
		if (!my_stricmp(name, tmp->name))
		{
			move_link_to_top(tmp, prev, location);
			location->hits++;
			new.name = tmp->name;
			new.func = tmp->func;
			return &new;
		}
	}
	return NULL;
#if 0
	while (built_in_functions[i].func && i <= NUMBER_OF_FUNCTIONS)
	{
		if (!my_stricmp(name, built_in_functions[i].name))
			return &built_in_functions[i];
		i++;
	}
#endif
	return NULL;
}

char **get_builtins(char *name, int *cnt)
{
char *last_match = NULL;
int matches_size = 5;
int i = 0;
int len;
char **matches = NULL;
#ifdef WANT_DLL	
BuiltInDllFunctions *dll = NULL;
#endif	

	len = strlen(name);
	*cnt = 0;
	RESIZE(matches, char *, matches_size);
        while (built_in_functions[i].func && i <= NUMBER_OF_FUNCTIONS)
	{
		if (strncmp(name, built_in_functions[i].name, len) == 0)
		{
			matches[*cnt] = NULL;
			malloc_strcpy(&(matches[*cnt]), built_in_functions[i].name);
			last_match = matches[*cnt];
			if (++(*cnt) == matches_size)
			{
				matches_size += 5;
				RESIZE(matches, char *, matches_size);
			}
		}
		else if (*cnt)
			break;
		i++;
	}
#ifdef WANT_DLL
	for (dll = dll_functions; dll; dll = dll->next)
	{
		if (strncasecmp(name, dll->name, len) == 0)
		{
			matches[*cnt] = NULL;
			malloc_strcpy(&(matches[*cnt]), dll->name);
			if (++(*cnt) == matches_size)
			{
				matches_size += 5;
				RESIZE(matches, char *, matches_size);
			}
		}
	}
#endif
	return matches;
}

char	*built_in_alias (char c, int *returnval)
{
	BuiltIns	*tmp;

	for (tmp = built_in;tmp->name;tmp++)
	{
		if (c == tmp->name)
		{
			if (returnval)
			{
				*returnval = 1;
				return NULL;
			}
			else
				return tmp->func();
		}
	}
	return NULL;
}

char	*call_function (char *name, const char *args, int *args_flag)
{
extern	char	*check_tcl_alias (char *command, char *args);
	char	*tmp;
	char	*result = NULL;
	char	*debug_copy = NULL;
	BuiltInFunctions *funcptr = NULL;
#ifdef WANT_DLL	
	register BuiltInDllFunctions *dll = NULL;
#endif	
	char	*lparen, *rparen;

	if ((lparen = strchr(name, '(')))
	{
		if ((rparen = MatchingBracket(lparen + 1, '(', ')')))
			*rparen++ = 0;
		else
			debugyell("Unmatched lparen in function call [%s]", name);

		*lparen++ = 0;
 	}
	else
		lparen = empty_string;

	tmp = expand_alias(lparen, args, args_flag, NULL);
	if ((internal_debug & DEBUG_FUNC) && !in_debug_yell)
		debug_copy = LOCAL_COPY(tmp);

	upper(name);
#ifdef WANT_DLL
	for (dll = dll_functions; dll; dll = dll->next)
		if (!strcasecmp(name, dll->name))
			break;
	if (dll)
		result = (dll->func)(name, tmp);
	else 
#endif
	{
#ifdef WANT_TCL
		if (!(result = check_tcl_alias(name, tmp)))
#endif
		{
			if ((funcptr = find_func_alias(name)))
				result = funcptr->func(name, tmp);
			else
				result = call_user_function(name, tmp);
		}
	}
	if (debug_copy && alias_debug)
	{
#if 0
		if (!alias_debug)
			debugyell("Function %s(%s) returned %s", name, debug_copy, result);
		else
#endif
		debugyell("%3d %s(%s) -> %s", debug_count++, name, debug_copy, result);
	}
	new_free(&tmp);
	return result;
}


extern char hook_name[];

/* built in expando functions */
static	char	*alias_version_str1	(void) { return m_strdup(_VERSION_); }
static	char	*alias_line 		(void) { return m_strdup(get_input()); }
static	char	*alias_buffer 		(void) { return m_strdup(cut_buffer); }
static	char	*alias_time 		(void) { return m_strdup(update_clock(GET_TIME)); }
static	char	*alias_dollar 		(void) { return m_strdup("$"); }
static	char	*alias_detected 	(void) { return m_strdup(last_notify_nick); }
static	char	*alias_nick 		(void) { return m_strdup((current_window->server != -1? get_server_nickname(current_window->server) : empty_string)); }
	char	*alias_away		(void) { return m_strdup(get_server_away(from_server)); }
static	char	*alias_sent_nick 	(void) { return m_strdup(get_server_sent_nick(from_server)); }
static	char	*alias_recv_nick 	(void) { return m_strdup(get_server_recv_nick(from_server)); }
static	char	*alias_msg_body 	(void) { return m_strdup(get_server_sent_body(from_server)); }
static	char	*alias_joined_nick 	(void) { return m_strdup((joined_nick) ? joined_nick : empty_string); }
static	char	*alias_public_nick 	(void) { return m_strdup((public_nick) ? public_nick : empty_string); }
static  char    *alias_show_realname 	(void) { return m_strdup(realname); }
static	char	*alias_version_str 	(void) { return m_strdup(irc_version); }
static	char	*alias_invite 		(void) { return m_strdup((invite_channel) ? invite_channel : empty_string); }
static	char	*alias_oper 		(void) { return m_strdup(get_server_operator(from_server) ?  get_string_var(STATUS_OPER_VAR) : empty_string); }
static	char	*alias_version 		(void) { return m_strdup(internal_version); }
static  char    *alias_online 		(void) { return m_sprintf("%ld",(long)start_time); }
static  char    *alias_idle 		(void) { return m_sprintf("%ld",now-idle_time); }
static  char    *alias_show_userhost 	(void) { return m_strdup(get_server_userhost(from_server)); }
static	char	*alias_current_numeric	(void) { return m_sprintf("%03d", -current_numeric); }
static	char	*alias_hookname		(void) { return m_sprintf("%s", *hook_name?hook_name:empty_string); }
static	char	*alias_thingansi	(void) { return m_strdup(numeric_banner()); }
static	char	*alias_uptime		(void) { return m_sprintf("%s", convert_time(now-start_time)); }
static	char	*alias_bitchx		(void) { return m_strdup("[BX]"); }
extern char *return_this_alias (void);
static	char	*alias_thisaliasname	(void) { return m_strdup(return_this_alias()); }
static	char	*alias_serverlag	(void) { return m_sprintf("%ld", get_server_lag(from_server)); }
static	char	*alias_currentwindow	(void) { return m_sprintf("%d", current_window ? current_window->refnum : 0); }
static	char	*alias_serverlistsize	(void) { return m_sprintf("%d", server_list_size()); }

#ifdef WANT_TCL
extern char tcl_versionstr[];
static	char	*alias_tclsupport	(void) { return m_strdup(tcl_versionstr); }
#else
static	char	*alias_tclsupport	(void) { return m_strdup(empty_string); }
#endif
static	char	*alias_currdir  	(void)
{
	char 	*tmp = (char *)new_malloc(MAXPATHLEN+1);
	return getcwd(tmp, MAXPATHLEN);
}

static	char	*alias_channel 		(void) 
{ 
	char	*tmp; 
	char	buffer[BIG_BUFFER_SIZE+1];
	if ((tmp = get_current_channel_by_refnum(0)))
	{
		strlcpy(buffer, tmp, BIG_BUFFER_SIZE);
		tmp = double_quote(tmp, "{}()\"", buffer);
#ifdef WANT_HEBREW
		if (get_int_var(HEBREW_TOGGLE_VAR))
			hebrew_process(tmp);
#endif
		return m_strdup(tmp);
	}
	else
		return m_strdup(zero);	
}

static	char	*alias_server 		(void) 
{
	return m_strdup((parsing_server_index != -1) ?
					get_server_itsname(parsing_server_index) :
					(get_window_server(0) != -1) ?
					get_server_itsname(get_window_server(0)) : empty_string);
}

static	char	*alias_awaytime		(void) 
{ 
	return m_sprintf("%lu", parsing_server_index != -1 ? 
					get_server_awaytime(parsing_server_index):
					get_window_server(0) != -1 ? 
					get_server_awaytime(get_window_server(0)):
					0); 
}

static	char	*alias_current_network	(void) 
{
	return m_strdup((parsing_server_index != -1) ?
		         get_server_network(parsing_server_index) :
		         (get_window_server(0) != -1) ?
			        get_server_network(get_window_server(0)) : empty_string);
}

static	char	*alias_serverport 		(void) 
{
	return m_sprintf("%d", (parsing_server_index != -1) ?
			get_server_port(parsing_server_index):
		         (get_window_server(0) != -1) ?
				get_server_port(get_window_server(0)) : 0);
}

static	char	*alias_query_nick 	(void)
{
	char	*tmp;
	return m_strdup((tmp = current_window->query_nick) ? tmp : empty_string);
}

static	char	*alias_target 		(void)
{
	char	*tmp;
	return m_strdup((tmp = get_target_by_refnum(0)) ? tmp : empty_string);
}

static	char	*alias_cmdchar 		(void)
{
	char	*cmdchars, tmp[2];

	if ((cmdchars = get_string_var(CMDCHARS_VAR)) == NULL)
		cmdchars = DEFAULT_CMDCHARS;
	tmp[0] = cmdchars[0];
	tmp[1] = 0;
	return m_strdup(tmp);
}

static	char	*alias_chanop 		(void)
{
	char	*tmp;
	return m_strdup(((tmp = get_current_channel_by_refnum(0)) && get_channel_oper(tmp, from_server)) ?
		"@" : empty_string);
}

static	char	*alias_modes 		(void)
{
	char	*tmp;
	return m_strdup((tmp = get_current_channel_by_refnum(0)) ?
		get_channel_mode(tmp, from_server) : empty_string);
}

static	char	*alias_server_version  (void)
{
	int s = from_server;

	if (s == -1)
	{
		if (primary_server != -1)
			s = primary_server;
		else
			return m_strdup(empty_string);
	}

	return m_strdup(get_server_version_string(s));
}


/*	*	*	*	*	*	*	*	*	*
		These are the built-in functions.

	About 80 of them are here, the rest are in array.c.  All of the
	stock client's functions are supported, as well as about 60 more.
	Most of the 30 stock client's functions have been re-written for
	optimization reasons, and also to further distance ircii's code
	from EPIC.
 *	*	*	*	*	*	*	*	*	*/

/* 
 * These are defined to make the construction of the built-in functions
 * easier and less prone to bugs and unexpected behaviors.  As long as
 * you consistently use these macros to do the dirty work for you, you
 * will never have to do bounds checking as the macros do that for you. >;-) 
 *
 * Yes, i realize it makes the code slightly less efficient, but i feel that 
 * the cost is minimal compared to how much time i have spent over the last 
 * year debugging these functions and the fact i wont have to again. ;-)
 */

#define EMPTY empty_string
#define EMPTY_STRING m_strdup(EMPTY)
#define RETURN_EMPTY return EMPTY_STRING
#define RETURN_IF_EMPTY(x) if (empty( x )) RETURN_EMPTY
#define GET_INT_ARG(x, y) {RETURN_IF_EMPTY(y); x = atol(safe_new_next_arg(y, &y));}
#define GET_FLOAT_ARG(x, y) {RETURN_IF_EMPTY(y); x = atof(safe_new_next_arg(y, &y));}
#define GET_STR_ARG(x, y) {RETURN_IF_EMPTY(y); x = new_next_arg(y, &y);RETURN_IF_EMPTY(x);}
#define RETURN_STR(x) return m_strdup(x ? x : EMPTY)
#define RETURN_MSTR(x) return ((x) ? (x) : EMPTY_STRING);
#define RETURN_INT(x) return m_strdup(ltoa(x))

#undef BUILT_IN_FUNCTION
#define BUILT_IN_FUNCTION(x, y) static char * x (char *fn, char * y)

/*
 * Usage: $left(number text)
 * Returns: the <number> leftmost characters in <text>.
 * Example: $left(5 the quick brown frog) returns "the q"
 *
 * Note: the difference between $[10]foo and $left(10 foo) is that the former
 * is padded and the latter is not.
 */
BUILT_IN_FUNCTION(function_left, input)
{
	long	count;

	GET_INT_ARG(count, input);
	RETURN_IF_EMPTY(input);
	if (count < 0)
		RETURN_EMPTY;
		
	if (strlen(input) > count)
		input[count] = 0;

	RETURN_STR(input);
}

char *function_getkey(char *n, char *input)
{
	char *temp;
	RETURN_IF_EMPTY(input);
	temp = get_channel_key(input, from_server);
	RETURN_STR(temp);
}

/*
 * Usage: $right(number text)
 * Returns: the <number> rightmost characters in <text>.
 * Example: $right(5 the quick brown frog) returns " frog"
 */
BUILT_IN_FUNCTION(function_right, word)
{
	long	count;

	GET_INT_ARG(count, word);
	RETURN_IF_EMPTY(word);
	if (count < 0)
		RETURN_EMPTY;
		
	if (strlen(word) > count)
		word += strlen(word) - count;

	RETURN_STR(word);
}

/*
 * Usage: $mid(start number text)
 * Returns: the <start>th through <start>+<number>th characters in <text>.
 * Example: $mid(3 4 the quick brown frog) returns " qui"
 *
 * Note: the first character is numbered zero.
 */
BUILT_IN_FUNCTION(function_mid, word)
{
	long	start, length;

	GET_INT_ARG(start, word);
	GET_INT_ARG(length, word);
	RETURN_IF_EMPTY(word);

	if (start < strlen(word))
	{
		word += start;
		if (length < 0)
			RETURN_EMPTY;
		if (length < strlen(word))
			word[length] = 0;
	}
	else
		word = EMPTY;

	RETURN_STR(word);
}


/*
 * Usage: $rand(max)
 * Returns: A random number from zero to max-1.
 * Example: $rand(10) might return any number from 0 to 9.
 */
BUILT_IN_FUNCTION(function_rand, word)
{
	long	tempin;
	int 	result;

	GET_INT_ARG(tempin, word);
	if (tempin == 0)
		tempin = (unsigned long) -1;	/* This is cheating. :P */
	result = random_number(0L) % tempin;
	RETURN_INT(result);
}

/*
 * Usage: $srand(seed)
 * Returns: Nothing.
 * Side effect: seeds the random number generater.
 * Note: the argument is ignored.
 */
BUILT_IN_FUNCTION(function_srand, word)
{
	random_number((long) now);
	RETURN_EMPTY;
}

/*
 * Usage: $time()
 * Returns: The number of seconds that has elapsed since Jan 1, 1970, GMT.
 * Example: $time() returned something around 802835348 at the time I
 * 	    wrote this comment.
 */
BUILT_IN_FUNCTION(function_time, input)
{
	RETURN_INT(time(NULL));
}

/*
 * Usage: $stime(time)
 * Returns: The human-readable form of the date based on the <time> argument.
 * Example: $stime(1000) returns what time it was 1000 seconds froM the epoch.
 * 
 * Note: $stime() is really useful when you givE it the argument $time(), ala
 *       $stime($time()) is the human readable form for now.
 */
BUILT_IN_FUNCTION(function_stime, input)
{
	time_t	ltime;

	GET_INT_ARG(ltime, input);
	RETURN_STR(my_ctime(ltime));
}

/*
 * Usage: $tdiff(seconds)
 * Returns: The time that has elapsed represented in days/hours/minutes/seconds
 *          corresponding to the number of seconds passed as the argument.
 * Example: $tdiff(3663) returns "1 hour 1 minute 3 seconds"
 */
BUILT_IN_FUNCTION(function_tdiff, input)
{
	long	ltime;

	long	days = 0,
		hours = 0,
		minutes = 0,
		seconds = 0;
	char	*tstr, *tmp;
	char	*after = NULL;

	tmp = alloca(strlen(input) + 180);
	*tmp = 0;

	ltime = strtol(input, &after, 10);
	tstr = tmp;

	if (after > input)
	{
		seconds = ltime % 60;
		ltime = (ltime - seconds) / 60;
		minutes = ltime % 60;
		ltime = (ltime - minutes) / 60;
		hours = ltime % 24;
		days = (ltime - hours) / 24;

		if (days)
		{
			sprintf(tstr, "%ld day%s ", days, plural(days));
			tstr += strlen(tstr);
		}
		if (hours)
		{
			sprintf(tstr, "%ld hour%s ", hours, plural(hours));
			tstr += strlen(tstr);
		}
		if (minutes)
		{
			sprintf(tstr, "%ld minute%s ", minutes, plural(minutes));
			tstr += strlen(tstr);
		}
	}
	if (seconds || (!days && !hours && !minutes) || (*after == '.' && is_number(after + 1)))
	{
		long number = 0;
		/*
		 * If we have a decmial point, and is_number() returns 1,
		 * then we know that we have a real, authentic number AFTER
		 * the decmial point.  As long as it isnt zero, we want it.
		 */
		if (*after == '.')
			number = atol(after + 1);

		/*
		 * *IF* and *ONLY IF* either the seconds OR the decmial
		 * part of the number is non zero do we tack on the seconds
		 * argument.  If both are zero, we do nothing.
		 */
		if (seconds != 0 || number != 0)
		{
			if (number == 0)
				sprintf(tstr, "%ld second%s", seconds, plural(seconds));
			else
				sprintf(tstr, "%ld%s seconds", seconds, after);
		}
	}
	else
		*--tstr = 0;

	RETURN_STR(tmp);
}

/*
 * Usage: $index(characters text)
 * Returns: The number of leading characters in <text> that do not occur 
 *          anywhere in the <characters> argument.
 * Example: $index(f three fine frogs) returns 6 (the 'f' in 'fine')
 *          $index(frg three fine frogs) returns 2 (the 'r' in 'three')
 */
BUILT_IN_FUNCTION(function_index, input)
{
	char	*schars;
	char	*iloc;

	GET_STR_ARG(schars, input);
	iloc = sindex(input, schars);
	RETURN_INT(iloc ? iloc - input : -1);
}

/*
 * Usage: $rindex(characters text)
 * Returns: The number of leading characters in <text> that occur before the
 *          *last* occurance of any of the characters in the <characters> 
 *          argument.
 * Example: $rindex(f three fine frogs) returns 12 (the 'f' in 'frogs')
 *          $rindex(frg three fine frogs) returns 15 (the 'g' in 'froGs')
 */
BUILT_IN_FUNCTION(function_rindex, word)
{
	char	*chars, *last;

	/* need to find out why ^x doesnt work */
	GET_STR_ARG(chars, word);
	last = rsindex(word + strlen(word) - 1, word, chars, 1);
	RETURN_INT(last ? last - word : -1);
}

/*
 * Usage: $match(pattern list of words)
 * Returns: if no words in the list match the pattern, it returns 0.
 *	    Otherwise, it returns the number of the word that most
 *	    exactly matches the pattern (first word is numbered one)
 * Example: $match(f*bar foofum barfoo foobar) returns 3
 *	    $match(g*ant foofum barfoo foobar) returns 0
 *
 * Note: it is possible to embed spaces inside of a word or pattern simply
 *       by including the entire word or pattern in quotation marks. (")
 */
BUILT_IN_FUNCTION(function_match, input)
{
	char	*pattern, 	*word;
	long	current_match,	best_match = 0,	match = 0, match_index = 0;

	GET_STR_ARG(pattern, input);

	while (input && *input)
	{
		while (input && my_isspace(*input))
			input++;
		match_index++;
		GET_STR_ARG(word, input);
		if ((current_match = wild_match(pattern, word)) > best_match)
		{
			match = match_index;
			best_match = current_match;
		}
	}

	RETURN_INT(match);
}

/*
 * Usage: $rmatch(word list of patterns)
 * Returns: if no pattern in the list matches the word, it returns 0.
 *	    Otherwise, it returns the number of the pattern that most
 *	    exactly matches the word (first word is numbered one)
 * Example: $rmatch(foobar f*bar foo*ar g*ant) returns 2 
 *	    $rmatch(booya f*bar foo*ar g*ant) returns 0
 * 
 * Note: It is possible to embed spaces into a word or pattern simply by
 *       including the entire word or pattern within quotation marks (")
 */
BUILT_IN_FUNCTION(function_rmatch, input)
{
	char	*pattern,	*word;
	int	current_match,	best_match = 0,	match = 0, rmatch_index = 0;

	GET_STR_ARG(word, input);

	while (input && *input)
	{
		while (input && my_isspace(*input))
			input++;
		rmatch_index++;
		GET_STR_ARG(pattern, input);
		if ((current_match = wild_match(pattern, word)) > best_match)
		{
			match = rmatch_index;
			best_match = current_match;
		}
		/* WARNING WARNING HACK IN PROGRESS WARNING WARNING */
		while (input && my_isspace(*input))
			input++;
	}

	RETURN_INT(match);
}

/*
 * Usage: $userhost()
 * Returns: the userhost (if any) of the most previously recieved message.
 * Caveat: $userhost() changes with every single line that appears on
 *         your screen, so if you want to save it, you will need to assign
 *         it to a variable.
 */
BUILT_IN_FUNCTION(function_userhost, input)
{
	if (input && *input)
	{
		char *retval = NULL;
		char *nick;
		const char *userhost;

		while (input && *input)
		{
			GET_STR_ARG(nick, input);
			if ((userhost = fetch_userhost(from_server, nick)))
				m_s3cat(&retval, space, userhost);
			else
				m_s3cat(&retval, space, unknown_userhost);
		}
		return retval;		/* DONT USE RETURN_STR HERE! */
	}
	RETURN_STR(FromUserHost);
}

/* 
 * Usage: $strip(characters text)
 * Returns: <text> with all instances of any characters in the <characters>
 *	    argument removed.
 * Example: $strip(f free fine frogs) returns "ree ine rogs"
 *
 * Note: it can be difficult (actually, not possible) to remove spaces from
 *       a string using this function.  To remove spaces, simply use this:
 *		$tr(/ //$text)
 *
 *	 Actually, i recommend not using $strip() at all and just using
 *		$tr(/characters//$text)
 *	 (but then again, im biased. >;-)
 */
BUILT_IN_FUNCTION(function_strip, input)
{
	char	*result;
	char	*chars;
	char	*cp, *dp;

	GET_STR_ARG(chars, input);
	RETURN_IF_EMPTY(input);

	result = (char *)new_malloc(strlen(input) + 1);
	for (cp = input, dp = result; *cp; cp++)
	{
		/* This is expensive -- gotta be a better way */
		if (!strchr(chars, *cp))
			*dp++ = *cp;
	}
	*dp = '\0';

	return result;		/* DONT USE RETURN_STR HERE! */
}

/*
 * Usage: $encode(text)
 * Returns: a string, uniquely identified with <text> such that the string
 *          can be used as a variable name.
 * Example: $encode(fe fi fo fum) returns "GGGFCAGGGJCAGGGPCAGGHFGN"
 *
 * Note: $encode($decode(text)) returns text (most of the time)
 *       $decode($encode(text)) also returns text.
 */
static char * function_encode (char *n, unsigned char * input)
{
	char	*result;
	int	i = 0;

	result = (char *)new_malloc(strlen((char *)input) * 2 + 1);
	while (*input)
	{
		result[i++] = (*input >> 4) + 0x41;
		result[i++] = (*input & 0x0f) + 0x41;
		input++;
	}
	result[i] = '\0';

	return result;		/* DONT USE RETURN_STR HERE! */
}


/*
 * Usage: $decode(text)
 * Returns: If <text> was generated with $encode(), it returns the string
 *          you originally encoded.  If it wasnt, you will probably get
 *	    nothing useful in particular.
 * Example: $decode(GGGFCAGGGJCAGGGPCAGGHFGN) returns "fe fi fo fum"
 *
 * Note: $encode($decode(text)) returns "text"
 *       $decode($encode(text)) returns "text" too.
 *
 * Note: Yes.  $decode(plain-text) does compress the data by a factor of 2.
 *       But it ignores non-ascii text, so use this as compression at your
 *	 own risk and peril.
 */
char *function_decode(char *n, unsigned char * input)
{
	unsigned char	*result;
	int	i = 0;

	result = (unsigned char *)new_malloc(strlen((char *)input) / 2 + 1);

	while (input[0] && input[1])
	{
		/* oops, this isnt quite right. */
		result[i] = ((input[0] - 0x41) << 4) | (input[1] - 0x41);
		input += 2;
		i++;
	}
	result[i] = '\0';

	return result;		/* DONT USE RETURN_STR HERE! */
}


/*
 * Usage: $ischannel(text)
 * Returns: If <text> could be a valid channel name, 1 is returned.
 *          If <text> is an illegal channel name, 0 is returned.
 *
 * Note: Contrary to popular belief, this function does NOT determine
 * whether a given channel name is in use!
 */
BUILT_IN_FUNCTION(function_ischannel, input)
{
	RETURN_INT(is_channel(input));
}

/*
 * Usage: $ischanop(nick channel)
 * Returns: 1 if <nick> is a channel operator on <channel>
 *          0 if <nick> is not a channel operator on <channel>
 *			* O R *
 *	      if you are not on <channel>
 *
 * Note: Contrary to popular belief, this function can only tell you
 *       who the channel operators are for channels you are already on!
 *
 * Boo Hiss:  This should be $ischanop(channel nick <nick...nick>)
 *	      and return a list (1 1 ... 0), which would allow us to
 *	      call is_chanop() without ripping off the nick, and allow 
 *	      us to abstract is_chanop() to take a list. oh well... 
 *	      Too late to change it now. :/
 */
BUILT_IN_FUNCTION(function_ischanop, input)
{
	char	*nick;

	GET_STR_ARG(nick, input);
	if (is_channel(nick))
	{
		char *blah;
		blah = is_chanoper(nick, input);
		return blah ? blah : m_strdup(empty_string);
	}
	else
		RETURN_INT(is_chanop(input, nick));
}

/*
 * Usage: $ishalfop(nick channel)
 * Returns: 1 if <nick> is a channel half-op on <channel>
 *          0 if <nick> is not a channel half-op on <channel>
 *			* O R *
 *	      if you are not on <channel>
 *
 */
BUILT_IN_FUNCTION(function_ishalfop, input)
{
	char	*nick;

	GET_STR_ARG(nick, input);
	RETURN_INT(is_halfop(input, nick));
}


/*
 * Usage: $word(jUmber text)
 * Returns: the <number>th word in <text>.  The first word is numbered zero.
 * Example: $word(3 one two three four five) returns "four" (think about it)
 */
BUILT_IN_FUNCTION(function_word, word)
{
	int	cvalue;
	char	*w_word;

	GET_INT_ARG(cvalue, word);
	if (cvalue < 0)
		RETURN_EMPTY;

	while (cvalue-- > 0)
		GET_STR_ARG(w_word, word);

	GET_STR_ARG(w_word, word);
	RETURN_STR(w_word);
}


/*
 * Usage: $winnum()
 * Returns: the index number for the current window
 * 
 * Note: returns -1 if there are no windows open (ie, in dumb mode)
 */
BUILT_IN_FUNCTION(function_winnum, input)
{
	Window *win = NULL;
	if (!(win = *input ? get_window_by_desc(input) : current_window))
		RETURN_INT(-1);
	RETURN_INT(win->refnum);
}

BUILT_IN_FUNCTION(function_winnam, input)
{
	Window *win = NULL;
	if (!(win = *input ? get_window_by_desc(input) : current_window))
		RETURN_EMPTY;
	RETURN_STR(win->name);
}

BUILT_IN_FUNCTION(function_winnames, input)
{
Window *win = NULL;
char *s = NULL;
	if (*input)
		win = get_window_by_desc(input);
	else
		win = current_window;
	if (!win)
		RETURN_EMPTY;

	return (s = get_nicklist_by_window(win)) ? s : m_strdup(empty_string);
}

BUILT_IN_FUNCTION(function_connect, input)
{
	char	*host;
	int	port;

	GET_STR_ARG(host, input);
	GET_INT_ARG(port, input);

	return dcc_raw_connect(host, port);	/* DONT USE RETURN_STR HERE! */
}


BUILT_IN_FUNCTION(function_listen, input)
{
	int	port = 0;
	char	*result;

	/* Oops. found by CrowMan, listen() has a default. erf. */
	if (input && *input)
	{
		char *tmp, *ptr;
		if ((tmp = new_next_arg(input, &input)))
		{
			port = strtoul(tmp, &ptr, 10);
			if (ptr == tmp)
				RETURN_EMPTY;	/* error. */
		}
	}

	result = dcc_raw_listen(port);
	RETURN_STR(result);			/* DONT REMOVE RESULT! */
}

BUILT_IN_FUNCTION(function_toupper, input)
{
	return (upper(m_strdup(input)));
}

BUILT_IN_FUNCTION(function_tolower, input)
{
	return (lower(m_strdup(input)));
}

BUILT_IN_FUNCTION(function_curpos, input)
{
	RETURN_INT(current_window->screen->buffer_pos);
}

BUILT_IN_FUNCTION(function_channels, input)
{
	long	winnum;
	Window  *window = current_window;

	if (isdigit((unsigned char)*input))
	{
		GET_INT_ARG(winnum, input);
		window = get_window_by_refnum(winnum);
	}
	if (window->server <= -1)
		RETURN_EMPTY;
	return create_channel_list(window);	/* DONT USE RETURN_STR HERE! */
}

BUILT_IN_FUNCTION(function_servers, input)
{
	return create_server_list(input);		/* DONT USE RETURN_STR HERE! */
}

BUILT_IN_FUNCTION(function_pid, input)
{
	RETURN_INT(getpid());
}

BUILT_IN_FUNCTION(function_ppid, input)
{
	RETURN_INT(getppid());
}

/*
 * strftime() patch from hari (markc@arbld.unimelb.edu.au)
 */
BUILT_IN_FUNCTION(function_strftime, input)
{
	char		result[128];
	time_t		ltime;
	struct tm	*tm;

	if (isdigit((unsigned char)*input))
		ltime = strtoul(input, &input, 10);
	else
		ltime = now;

	while (*input && my_isspace(*input))
		++input; 

	if (!*input)
		return m_strdup(empty_string);


	tm = localtime(&ltime);

	if (!strftime(result, 128, input, tm))
		return m_strdup(empty_string);

	return m_strdup(result);
}

BUILT_IN_FUNCTION(function_idle, input)
{
	return alias_idle();
}



/* The new "added" functions */

/* $before(chars string of text)
 * returns the part of "string of text" that occurs before the
 * first instance of any character in "chars"
 * EX:  $before(! hop!jnelson@iastate.edu) returns "hop"
 */
BUILT_IN_FUNCTION(function_before, word)
{
	char	*pointer = NULL;
	char	*chars;
	char	*tmp;
	long	numint;

	GET_STR_ARG(tmp, word);			/* DONT DELETE TMP! */
	numint = atol(tmp);

	if (numint)
	{
		GET_STR_ARG(chars, word);
	}
	else
	{
		numint = 1;
		chars = tmp;
	}

	if (numint < 0 && strlen(word))
		pointer = word + strlen(word) - 1;

	pointer = strsearch(word, pointer, chars, numint);

	if (!pointer)
		RETURN_EMPTY;

	*pointer = '\0';
	RETURN_STR(word);
}

/* $after(chars string of text)
 * returns the part of "string of text" that occurs after the 
 * first instance of any character in "chars"
 * EX: $after(! hop!jnelson@iastate.edu)  returns "jnelson@iastate.edu"
 */
BUILT_IN_FUNCTION(function_after, word)
{
	char	*chars;
	char	*pointer = NULL;
	char 	*tmp;
	long	numint;

	GET_STR_ARG(tmp, word);
	numint = atol(tmp);

	if (numint)
		chars = new_next_arg(word, &word);
	else
	{
		numint = 1;
		chars = tmp;
	}

	if (numint < 0 && strlen(word))
		pointer = word + strlen(word) - 1;

	pointer = strsearch(word, pointer, chars, numint);

	if (!pointer || !*pointer)
		RETURN_EMPTY;

	RETURN_STR(pointer + 1);
}

/* $leftw(num string of text)
 * returns the left "num" words in "string of text"
 * EX: $leftw(3 now is the time for) returns "now is the"
 */
BUILT_IN_FUNCTION(function_leftw, word)
{
	int value;
 
	GET_INT_ARG(value, word);
	if (value < 1)
		RETURN_EMPTY;

	return (extract(word, 0, value-1));	/* DONT USE RETURN_STR HERE! */
}

/* $rightw(num string of text)
 * returns the right num words in "string of text"
 * EX: $rightw(3 now is the time for) returns "the time for"
 */
BUILT_IN_FUNCTION(function_rightw, word)
{
	int     value;

	GET_INT_ARG(value, word);
	if (value < 1)
		RETURN_EMPTY;
		
	return extract2(word, -value, EOS); 
}


/* $midw(start num string of text)
 * returns "num" words starting at word "start" in the string "string of text"
 * NOTE: The first word is word #0.
 * EX: $midw(2 2 now is the time for) returns "the time"
 */
BUILT_IN_FUNCTION(function_midw, word)
{
	int     start, num;

	GET_INT_ARG(start, word);
	GET_INT_ARG(num, word);

	if (num < 1)
		RETURN_EMPTY;

	return extract(word, start, (start + num - 1));
}

/* $notw(num string of text)
 * returns "string of text" with word number "num" removed.
 * NOTE: The first word is numbered 0.
 * EX: $notw(3 now is the time for) returns "now is the for"
 */
BUILT_IN_FUNCTION(function_notw, word)
{
	char    *booya = NULL;
	int     where;
	
	GET_INT_ARG(where, word);

	/* An illegal word simpli returns the string as-is */
	if (where < 0)
		RETURN_STR(word);

	if (where > 0)
	{
		char *part1, *part2;
		part1 = extract(word, 0, (where - 1));
		part2 = extract(word, (where + 1), EOS);

		booya = m_strdup(part1);
		m_s3cat_s(&booya, space, part2);
		new_free(&part1);
		new_free(&part2);
	}
	else /* where == 0 */
		booya = extract(word, 1, EOS);

	return booya;				/* DONT USE RETURN_STR HERE! */
}

/* $restw(num string of text)
 * returns "string of text" that occurs starting with and including
 * word number "num"
 * NOTE: the first word is numbered 0.
 * EX: $restw(3 now is the time for) returns "time for"
 */
BUILT_IN_FUNCTION(function_restw, word)
{
	int     where;
	
	GET_INT_ARG(where, word);
	if (where < 0)
		RETURN_EMPTY;
	return extract(word, where, EOS);
}

/* $remw(word string of text)
 * returns "string of text" with the word "word" removed
 * EX: $remw(the now is the time for) returns "now is time for"
 */
BUILT_IN_FUNCTION(function_remw, word)
{
	char 	*word_to_remove;
	int	len;
	char	*str;

	GET_STR_ARG(word_to_remove, word);
	len = strlen(word_to_remove);

	str = stristr(word, word_to_remove);
	for (; str && *str; str = stristr(str + 1, word_to_remove))
	{
		if (str == word || isspace((unsigned char)str[-1]))
		{
			if (!str[len] || isspace((unsigned char)str[len]))
			{
				if (!str[len])
				{
					if (str != word)
						str--;
					*str = 0;
				}
				else if (str > word)
				{
					char *safe = (char *)alloca(strlen(str));
					strcpy(safe, str + len);
					strcpy(str - 1, safe);
				}
				else 
				{
					char *safe = (char *)alloca(strlen(str));
					strcpy(safe, str + len + 1);
					strcpy(str, safe);
				}
				break;
			}
		}
	}

	RETURN_STR(word);
}

/* $insertw(num word string of text)
 * returns "string of text" such that "word" is the "num"th word
 * in the string.
 * NOTE: the first word is numbered 0.
 * EX: $insertw(3 foo now is the time for) returns "now is the foo time for"
 */
BUILT_IN_FUNCTION(function_insertw, word)
{
	int     where;
	char    *what;
	char    *booya= NULL;
	char 	*str1, *str2;

	GET_INT_ARG(where, word);
	
	/* If the word goes at the front of the string, then it
	   already is: return it. ;-) */
	if (where < 1)
		booya = m_strdup(word);
	else
	{
		GET_STR_ARG(what, word);
		str1 = extract(word, 0, (where - 1));
		str2 = extract(word, where, EOS);
		booya = m_strdup(str1);
		if (str1 && *str1)
			booya = m_2dup(str1, space);
		malloc_strcat(&booya, what);
		m_s3cat_s(&booya, space, str2);
		new_free(&str1);
		new_free(&str2);
	}

	return booya;				/* DONT USE RETURN_STR HERE! */
}

/* $chngw(num word string of text)
 * returns "string of text" such that the "num"th word is removed 
 * and replaced by "word"
 * NOTE: the first word is numbered 0
 * EX: $chngw(3 foo now is the time for) returns "now is the foo for"
 */
BUILT_IN_FUNCTION(function_chngw, word)
{
	int     which;
	char    *what;
	char    *booya= NULL;
	char	*str1, *str2;
	
	GET_INT_ARG(which, word);
	GET_STR_ARG(what, word);

	if (which < 0)
		RETURN_STR(word);

	/* hmmm. if which is 0, extract does the wrong thing. */
	str1 = extract(word, 0, which - 1);
	str2 = extract(word, which + 1, EOS);
	booya = m_strdup(str1);
	if (str1 && *str1)
		booya = m_2dup(str1, space);
	malloc_strcat(&booya, what);
	m_s3cat_s(&booya, space, str2);
	new_free(&str1);
	new_free(&str2);

	return (booya);
}


/* $common (string of text / string of text)
 * Given two sets of words seperated by a forward-slash '/', returns
 * all words that are found in both sets.
 * EX: $common(one two three / buckle my two shoe one) returns "one two"
 * NOTE: returned in order found in first string.
 */
BUILT_IN_FUNCTION(function_common, word)
{
	char    *left = NULL, **leftw = NULL,
		*right = NULL, **rightw = NULL,	*booya = NULL;
	int	leftc, lefti;
	int	rightc, righti;
	
	left = word;
	if ((right = strchr(word,'/')) == NULL)
		RETURN_EMPTY;

 	*right++ = 0;
	leftc = splitw(left, &leftw);
	rightc = splitw(right, &rightw);

	for (lefti = 0; lefti < leftc; lefti++)
	{
		for (righti = 0; righti < rightc; righti++)
		{
			if (rightw[righti] && !my_stricmp(leftw[lefti], rightw[righti]))
			{
				m_s3cat(&booya, space, leftw[lefti]);
				rightw[righti] = NULL;
			}
		}
	}
	new_free((char **)&leftw);
	new_free((char **)&rightw);
	if (!booya)
		RETURN_EMPTY;

	return (booya);				/* DONT USE RETURN_STR HERE! */
}

/* $diff(string of text / string of text)
 * given two sets of words, seperated by a forward-slash '/', returns
 * all words that are not found in both sets
 * EX: $diff(one two three / buckle my two shoe)
 * returns "one two three buckle my shoe"
 */
BUILT_IN_FUNCTION(function_diff, word)
{
	char 	*left = NULL, **leftw = NULL,
	     	*right = NULL, **rightw = NULL, *booya = NULL;
	int 	lefti, leftc,
	    	righti, rightc;
	int 	found;

	left = word;
	if ((right = strchr(word,'/')) == NULL)
		RETURN_EMPTY;

	*right++ = 0;
	leftc = splitw(left, &leftw);
	rightc = splitw(right, &rightw);

	for (lefti = 0; lefti < leftc; lefti++)
	{
		found = 0;
		for (righti = 0; righti < rightc; righti++)
		{
			if (rightw[righti] && !my_stricmp(leftw[lefti], rightw[righti]))
			{
				found = 1;
				rightw[righti] = NULL;
			}
		}
		if (!found)
			m_s3cat(&booya, space, leftw[lefti]);
	}

	for (righti = 0; righti < rightc; righti++)
	{
		if (rightw[righti])
			m_s3cat(&booya, space, rightw[righti]);
	}

	new_free((char **)&leftw);
	new_free((char **)&rightw);

	if (!booya)
		RETURN_EMPTY;

	return (booya);
}

/* $pattern(pattern string of words)
 * givEn a pattern and a string of words, returns all words that
 * are matched by the pattern
 * EX: $pattern(f* one two three four five) returns "four five"
 */
BUILT_IN_FUNCTION(function_pattern, word)
{
	char    *blah;
	char    *booya = NULL;
	char    *pattern; 

	GET_STR_ARG(pattern, word);
	while (((blah = new_next_arg(word, &word)) != NULL))
	{
		if (wild_match(pattern, blah))
			m_s3cat(&booya, space, blah);
	}
	RETURN_MSTR(booya);
}

/* $filter(pattern string of words)
 * given a pattern and a string of words, returns all words that are 
 * NOT matched by the pattern
 * $filter(f* one two three four five) returns "one two three"
 */
BUILT_IN_FUNCTION(function_filter, word)
{
	char    *blah;
	char    *booya = NULL;
	char    *pattern;

	GET_STR_ARG(pattern, word);
	while ((blah = new_next_arg(word, &word)) != NULL)
	{
		if (!wild_match(pattern, blah))
			m_s3cat(&booya, space, blah);
	}
	RETURN_MSTR(booya);
}

/* $rpattern(word list of patterns)
 * Given a word and a list of patterns, return all patterns that
 * match the word.
 * EX: $rpattern(jnelson@iastate.edu *@* jnelson@* f*@*.edu)
 * returns "*@* jnelson@*"
 */
BUILT_IN_FUNCTION(function_rpattern, word)
{
	char    *blah;
	char    *booya = NULL;
	char    *pattern;

	GET_STR_ARG(blah, word);
	while ((pattern = new_next_arg(word, &word)) != NULL)
	{
		if (wild_match(pattern, blah))
			m_s3cat(&booya, space, pattern);
	}
	RETURN_MSTR(booya);
}

/* $rfilter(word list of patterns)
 * given a word and a list of patterns, return all patterns that
 * do NOT match the word
 * EX: $rfilter(jnelson@iastate.edu *@* jnelson@* f*@*.edu)
 * returns "f*@*.edu"
 */
BUILT_IN_FUNCTION(function_rfilter, word)
{
	char    *blah;
	char    *booya = NULL;
	char    *pattern;

	GET_STR_ARG(blah, word);
	while ((pattern = new_next_arg(word, &word)) != NULL)
	{
		if (!wild_match(pattern, blah))
			m_s3cat(&booya, space, pattern);
	}
	RETURN_MSTR(booya);
}

/* $copattern(pattern var_1 var_2)
 * Given a pattern and two variable names, it returns all words
 * in the variable_2 corresponding to any words in variable_1 that
 * are matched by the pattern
 * EX: @nicks = [hop IRSMan skip]
 *     @userh = [jnelson@iastate.edu irsman@iastate.edu sanders@rush.cc.edu]
 *     $copattern(*@iastate.edu userh nicks) 
 *	returns "hop IRSMan"
 */
BUILT_IN_FUNCTION(function_copattern, word)
{
	char	*booya = NULL,
		*pattern = NULL,
		*firstl = NULL, *firstlist = NULL, *firstel = NULL,
		*secondl = NULL, *secondlist = NULL, *secondel = NULL;
	char 	*sfirstl, *ssecondl;

	GET_STR_ARG(pattern, word);
	GET_STR_ARG(firstlist, word);
	GET_STR_ARG(secondlist, word);

	firstl = get_variable(firstlist);
	secondl = get_variable(secondlist);
                
	sfirstl = firstl;
	ssecondl = secondl;

	while ((firstel = new_next_arg(firstl, &firstl)))
	{
		if (!(secondel = new_next_arg(secondl, &secondl)))
			break;

		if (wild_match(pattern, firstel))
			m_s3cat(&booya, space, secondel);
	}
	new_free(&sfirstl);
	new_free(&ssecondl);

	if (!booya) 
		RETURN_EMPTY;

	return (booya);
}

/* $beforew(pattern string of words)
 * returns the portion of "string of words" that occurs before the 
 * first word that is matched by "pattern"
 * EX: $beforew(three one two three o leary) returns "one two"
 */
BUILT_IN_FUNCTION(function_beforew, word)
{
	int     where;
	char	*lame = NULL;
	char	*placeholder;

	lame = LOCAL_COPY(word);
	where = my_atol((placeholder = function_rmatch(NULL, word)));
	new_free(&placeholder);

	if (where < 1)
	{
		RETURN_EMPTY;
	}
	placeholder = extract(lame, 1, where - 1);
	return placeholder;
}
		
/* Same as above, but includes the word being matched */
BUILT_IN_FUNCTION(function_tow, word)
{
	int     where;
	char	*lame = NULL;
	char	*placeholder;

	lame = LOCAL_COPY(word);
	where = my_atol((placeholder = function_rmatch(NULL, word)));
	new_free(&placeholder);

	if (where < 1)
		RETURN_EMPTY;
	placeholder = extract(lame, 1, where);
	return placeholder;
}

/* Returns the string after the word being matched */
BUILT_IN_FUNCTION(function_afterw, word)
{
	int     where;
	char	*lame = NULL;
	char	*placeholder;

	lame = LOCAL_COPY(word);
	where = my_atol((placeholder = function_rmatch(NULL, word)));
	new_free(&placeholder);

	if (where < 1)
		RETURN_EMPTY;
	placeholder = extract(lame, where + 1, EOS);
	return placeholder;
}

/* Returns the string starting with the word being matched */
BUILT_IN_FUNCTION(function_fromw, word)
{
	int     where;
	char 	*lame = NULL;
	char	*placeholder;

	lame = LOCAL_COPY(word);
	where = my_atol((placeholder = function_rmatch(NULL, word)));
	new_free(&placeholder);

	if (where < 1)
		RETURN_EMPTY;

	placeholder = extract(lame, where, EOS);
	return placeholder;
}

/* Cut and paste a string */
BUILT_IN_FUNCTION(function_splice, word)
{
	char    *variable;
	int	start;
	int	length;
	char 	*left_part = NULL;
	char	*middle_part = NULL;
	char	*right_part = NULL;
	char	*old_value = NULL;
	char	*new_value = NULL;
	int 	old_display = window_display;
	int	num_words;

	GET_STR_ARG(variable, word);
	GET_INT_ARG(start, word);
	GET_INT_ARG(length, word);

	old_value = get_variable(variable);
	num_words = word_count(old_value);

	if (start < 0)
	{
		if ((length += start) <= 0)
			RETURN_EMPTY;
		start = 0;
	}

	if (start >= num_words)
	{
		left_part = m_strdup(old_value);
		middle_part = m_strdup(empty_string);
		right_part = m_strdup(empty_string);
	}

	else if (start + length >= num_words)
	{
		left_part = extract(old_value, 0, start - 1);
		middle_part = extract(old_value, start, EOS);
		right_part = m_strdup(empty_string);
	}

	else
	{
		left_part = extract(old_value, 0, start - 1);
		middle_part = extract(old_value, start, start + length - 1);
		right_part = extract(old_value, start + length, EOS);
	}

	new_value = NULL;
	malloc_strcpy(&new_value, left_part);
	if (new_value && *new_value && word && *word)
		malloc_strcat(&new_value, space);
	if (word && *word)
		malloc_strcat(&new_value, word);
	if (new_value && *new_value && *right_part && *right_part)
		malloc_strcat(&new_value, space);
	if (right_part && *right_part)
		malloc_strcat(&new_value, right_part);

	window_display = 0;
	add_var_alias(variable, new_value);
	window_display = old_display;

	new_free(&old_value);
	new_free(&new_value);
	new_free(&left_part);
	new_free(&right_part);
	return middle_part;
}
	
BUILT_IN_FUNCTION(function_numonchannel, word)
{
	char		*channel;
	ChannelList	*chan = NULL; /* XXX */
	NickList	*tmp = NULL;
	int             counter = 0;

	/* this does the right thing CDE */	
	channel = next_arg(word, &word);
	if ((chan = lookup_channel(channel, from_server, 0)))
	{
		for (tmp = next_nicklist(chan, NULL); tmp; tmp = next_nicklist(chan, tmp))
			counter++;
	}
	RETURN_INT(counter);
}

BUILT_IN_FUNCTION(function_onchannel, word)
{
	char		*channel;
	ChannelList     *chan = NULL; /* XXX */
	NickList        *tmp = NULL;
	char		*nicks = NULL;
	int		sort_type = NICKSORT_NORMAL;
	
	/* DO NOT use new_next_arg() in here. NULL is a legit value to pass CDE*/
	channel = next_arg(word, &word);
	if ((chan = lookup_channel(channel, from_server, 0)))
	{
		NickList *list=NULL;
		if (word && *word)
			GET_INT_ARG(sort_type, word);
		list = sorted_nicklist(chan, sort_type);
		for (tmp = list; tmp; tmp = tmp->next)
			m_s3cat(&nicks, space, tmp->nick);
		clear_sorted_nicklist(&list);
		return (nicks ? nicks : m_strdup(empty_string));
	}
	else
	{
		nicks = channel;
		channel = next_arg(word, &word);
		RETURN_INT(is_on_channel(channel, from_server, nicks) ? 1 : 0);
	}
}

BUILT_IN_FUNCTION(function_channelnicks, word)
{
	char		*channel;
	ChannelList     *chan = NULL; /* XXX */
	NickList        *tmp = NULL;
	char		*nicks = NULL;
	int		sort_type = NICKSORT_NORMAL;
	
	channel = next_arg(word, &word);
	if ((chan = lookup_channel(channel, from_server, 0)))
	{
		NickList *list = NULL;
		if (word && *word)
			GET_INT_ARG(sort_type, word);
		list = sorted_nicklist(chan, NICKSORT_NORMAL);
		for (tmp = list; tmp; tmp = tmp->next)
			m_s3cat(&nicks, ",", tmp->nick);
		clear_sorted_nicklist(&list);
		return (nicks ? nicks : m_strdup(empty_string));
	}
	else
	{
		nicks = channel;
		channel = next_arg(word, &word);
		RETURN_INT(is_on_channel(channel, from_server, nicks) ? 1 : 0);
	}
}

BUILT_IN_FUNCTION(function_servports, input)
{
	int	servnum = from_server;

	if (*input)
		GET_INT_ARG(servnum, input);

	if (servnum == -1)
		servnum = from_server;
	if (servnum < 0 || servnum > server_list_size())
		RETURN_EMPTY;

	return m_sprintf("%d %d", get_server_port(servnum),
				  get_server_local_port(servnum));
}

BUILT_IN_FUNCTION(function_chop, input)
{
	char *buffer;
	int howmany = 1;

	if (my_atol(input))
		GET_INT_ARG(howmany, input);

	buffer = m_strdup(input);
	chop(buffer, howmany);
	return buffer;
}

BUILT_IN_FUNCTION(function_winlevel, input)
{
	Window	*win;
	char	*desc;

	if (input && *input)
	{
		GET_STR_ARG(desc, input);
		win = get_window_by_desc(desc);
	}
	else
		win = current_window;

	if (!win)
		RETURN_EMPTY;

	RETURN_STR(bits_to_lastlog_level(win->window_level));
}

BUILT_IN_FUNCTION(function_chops, word)
{
	char		*channel;
	ChannelList     *chan;
	NickList        *tmp;
	char            *nicks = NULL;

	channel = next_arg(word, &word);
	if ((chan = lookup_channel(channel,from_server,0)))
	{
		NickList *list = NULL;
		list = sorted_nicklist(chan, NICKSORT_NORMAL);
		for (tmp = list; tmp; tmp = tmp->next)
			if (nick_isop(tmp)) 
				m_s3cat(&nicks, space, tmp->nick);
		clear_sorted_nicklist(&list);
	}
	return (nicks ? nicks : m_strdup(empty_string));
}        

BUILT_IN_FUNCTION(function_nochops, word)
{
	ChannelList     *chan;
	NickList        *tmp;
	char            *nicks = NULL;
	char		*channel;

	channel = next_arg(word, &word);
	if ((chan = lookup_channel(channel,from_server,0)))
	{
		NickList *list = NULL;
		list = sorted_nicklist(chan, NICKSORT_NORMAL);
		for (tmp = list; tmp; tmp = tmp->next)
			if (!nick_isop(tmp)) 
				m_s3cat(&nicks, space, tmp->nick);
		clear_sorted_nicklist(&list);
	}
	return (nicks ? nicks : m_strdup(empty_string));
}

BUILT_IN_FUNCTION(function_key, word)
{
	char	*channel;
	char    *booya = NULL;
	char 	*key;

	do
	{
		channel = next_arg(word, &word);
		if ((!channel || !*channel) && booya)
			break;

		key = get_channel_key(channel, from_server);
		m_s3cat(&booya, space, (key && *key) ? key : "*");
	}
	while (word && *word);

	return (booya ? booya : m_strdup(empty_string));
}

BUILT_IN_FUNCTION(function_revw, words)
{
	char	*booya = NULL;

	while (words && *words)
		m_s3cat(&booya, space, last_arg(&words));
	if (!booya)
		RETURN_EMPTY;

	return booya;
}

BUILT_IN_FUNCTION(function_reverse, words)
{
	int     length = strlen(words);
	char    *booya = NULL;
	int     x = 0;

	booya = (char *)new_malloc(length+1);
	for (length--; length >= 0; length--,x++)
		booya[x] = words[length];
	booya[x] = '\0';
	return (booya);
}

BUILT_IN_FUNCTION(function_substr, input)
{
	char *search;
	char *ptr;

	GET_STR_ARG(search, input);

	if ((ptr = stristr(input, search)))
		RETURN_INT((unsigned long)(ptr - input));
	else
		RETURN_INT(-1);
}

BUILT_IN_FUNCTION(function_rsubstr, input)
{
	char *search;
	char *ptr;

	GET_STR_ARG(search, input);

	if ((ptr = rstristr(input, search)))
		RETURN_INT((unsigned long)(ptr - input));
	else
		RETURN_INT(-1);
}

BUILT_IN_FUNCTION(function_nohighlight, input)
{
	char *outbuf, *ptr;

	ptr = outbuf = alloca(strlen(input) * 3 + 1);
	while (*input)
	{
		switch (*input)
		{
			case REV_TOG:
			case UND_TOG:
			case BOLD_TOG:
			case BLINK_TOG:
			case ALL_OFF:
			case '\003':
			case '\033':
			{
				*ptr++ = REV_TOG;
				*ptr++ = (*input++ | 0x40);
				*ptr++ = REV_TOG;
				break;
			}
			default:
			{
				*ptr++ = *input++;
				break;
			}
		}
	}

	*ptr = 0;
	RETURN_STR(outbuf);
}


BUILT_IN_FUNCTION(function_strstr, input)
{
char *p, *word;
	GET_STR_ARG(word, input);
	RETURN_IF_EMPTY(input);
	if ((p = stristr(input, word)))
		RETURN_STR(p);
	else
		RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_rstrstr, input)
{
char *p, *word;
	GET_STR_ARG(word, input);
	RETURN_IF_EMPTY(input);
	if ((p = rstristr(input, word)))
		RETURN_STR(p);
	else
		RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_jot, input)
{
	int     start = 0;
	int     stop = 0;
	int     interval = 1;
	int     counter;
	char    *booya = NULL;
	int	range;
	size_t  size;
	
        GET_INT_ARG(start,input)
        GET_INT_ARG(stop, input)
        if (input && *input)
                GET_INT_ARG(interval, input)
        else
                interval = 1;

        if (interval < 0) 
                interval = -interval;

	range = abs(stop - start) + 1;
	size = range * 10;
	booya = new_malloc(size);
	size--;

        if (start < stop)
	{
		strlcpy(booya, ltoa(start), size);
		for (counter = start + interval; counter <= stop; counter += interval)
		{
			strlcat(booya, space, size);
			strlcat(booya, ltoa(counter), size);
		}
	}
        else
	{
		strlcpy(booya, ltoa(start),size);
		for (counter = start - interval; counter >= stop; counter -= interval)
		{
			strlcat(booya, space, size);
			strlcat(booya, ltoa(counter), size);
		}
	}

        return booya;
}

char    *function_shift(char *n, char *word)
{
	char    *value = NULL;
	char    *var    = NULL;
	char	*booya 	= NULL;
	int     old_display = window_display;
	char    *placeholder;

	GET_STR_ARG(var, word);

	if (word && *word)
		RETURN_STR(var);

	value = get_variable(var);

	if (!value || !*value)
	{
		new_free(&value);
		RETURN_EMPTY;
	}

	placeholder = value;
	booya = m_strdup(new_next_arg(value, &value));
	/* do not free value at this point */
	if (var)
	{
		window_display = 0;
		add_var_alias(var, value);
		window_display = old_display;
	}
	new_free(&placeholder);
	if (!booya)
		RETURN_EMPTY;
	return booya;
}

char    *function_unshift(char *n, char *word)
{
	char    *value = NULL;
	char    *var    = NULL;
	char	*booya  = NULL;
	int     old_display = window_display;

	GET_STR_ARG(var, word);
	value = get_variable(var);
	if (!word || !*word)
		return value;

	booya = m_strdup(word);
	m_s3cat_s(&booya, space, value);

	window_display = 0;
	add_var_alias(var, booya);
	window_display = old_display;
	new_free(&value);
	return booya;
}

char    *function_push(char *n, char *word)
{
	char    *value = NULL;
	char    *var    = NULL;
	int     old_display = window_display;

	GET_STR_ARG(var, word);
	value = get_variable(var);
	m_s3cat(&value, space, word);
	window_display = 0;
	add_var_alias(var, value);
	window_display = old_display;
	return value;
}

char	*function_pop(char *n, char *word)
{
	char *value	= NULL;
	char *var	= NULL;
	char *pointer	= NULL;
	int   old_display = window_display;
	char *blech     = NULL;

	GET_STR_ARG(var, word);

	if (word && *word)
	{
		pointer = strrchr(word, ' ');
		RETURN_STR(pointer ? pointer : word);
	}

	value = get_variable(var);
	if (!value || !*value)
	{
		new_free(&value);
		RETURN_EMPTY;
	}

	if (!(pointer = strrchr(value, ' ')))
	{
		window_display = 0;
		add_var_alias(var, empty_string); /* dont forget this! */
		window_display = old_display;
		return value;	/* one word -- return it */
	}

	*pointer++ = '\0';
	window_display = 0;
	add_var_alias(var, value);
	window_display = old_display;

	/* because pointer points to value, we *must* make a copy of it
	 * *before* we free value! (And we cant forget to free value, either) 
	 */
	blech = m_strdup(pointer);
	new_free(&value);
	return blech;
}


/* Search and replace function --
   Usage:   $sar(c/search/replace/data)
   Commands:
		r - treat data as a variable name and 
		    return the replaced data to the variable
		g - Replace all instances, not just the first one
   The delimiter may be any character that is not a command (typically /)
   The delimiter MUST be the first character after the command
   Returns emppy string on error
*/
BUILT_IN_FUNCTION(function_sar, word)
{
	register char    delimiter;
	register char	*pointer	= NULL;
	char    *search         = NULL;
	char    *replace        = NULL;
	char    *data		= NULL;
	char	*value		= NULL;
	char	*booya		= NULL;
	int	variable = 0,this_global = 0,searchlen,oldwindow = window_display;
	char *(*func) (const char *, const char *) = strstr;
	char 	*svalue;

	while (((*word == 'r') && (variable = 1)) || ((*word == 'g') && (this_global = 1)) || ((*word == 'i') && (func = (char *(*)(const char *, const char *))global[STRISTR])))
		word++;

	RETURN_IF_EMPTY(word);

	delimiter = *word;
	search = word + 1;
	if ((replace = strchr(search, delimiter)) == 0)
		RETURN_EMPTY;

	*replace++ = 0;
	if ((data = strchr(replace,delimiter)) == 0)
		RETURN_EMPTY;

	*data++ = '\0';

	value = (variable == 1) ? get_variable(data) : m_strdup(data);

	if (!value || !*value)
	{
		new_free(&value);
		RETURN_EMPTY;
	}

	pointer = svalue = value;
	searchlen = strlen(search) - 1;
	if (searchlen < 0)
		searchlen = 0;
	if (this_global)
	{
		while ((pointer = func(pointer,search)) != NULL)
		{
			pointer[0] = pointer[searchlen] = 0;
			pointer += searchlen + 1;
			m_e3cat(&booya, value, replace);
			value = pointer;
			if (!*pointer)
				break;
		}
	} 
	else
	{
		if ((pointer = func(pointer,search)) != NULL)
		{
			pointer[0] = pointer[searchlen] = 0;
			pointer += searchlen + 1;
			m_e3cat(&booya, value, replace);
			value = pointer;
		}
	}

	malloc_strcat(&booya, value);
	if (variable) 
	{
		window_display = 0;
		add_var_alias(data, booya);
		window_display = oldwindow;
	}
	new_free(&svalue);
	return (booya);
}

/* Search and replace function --
   Usage:   $msar(c/search/replace/data)
   Commands:
		r - treat data as a variable name and 
		    return the replaced data to the variable
		g - Replace all instances, not just the first one
   The delimiter may be any character that is not a command (typically /)
   The delimiter MUST be the first character after the command
   Returns empty string on error
*/
#if 0
BUILT_IN_FUNCTION(function_msar, word)
{
	register char    delimiter;
	register char	*pointer	= NULL;
	char    *search         = NULL;
	char    *replace        = NULL;
	char    *data		= NULL;
	char	*value		= NULL;
	char	*booya		= NULL;
	char	*p		= NULL;
	int	variable = 0,this_global = 0,searchlen,oldwindow = window_display;
	char *(*func) (const char *, const char *) = strstr;
	char 	*svalue = NULL;

	while (((*word == 'r') && (variable = 1)) || ((*word == 'g') && (this_global = 1)) || ((*word == 'i') && (func = (char *(*)(const char *, const char *))global[STRISTR])))
		word++;

	RETURN_IF_EMPTY(word);

	delimiter = *word;
	search = word + 1;
	if (!(replace = strchr(search, delimiter)))
		RETURN_EMPTY;

	*replace++ = 0;
	if (!(data = strchr(replace,delimiter)))
		RETURN_EMPTY;

	*data++ = 0;

	if (!(p = strrchr(data, delimiter)))
		value = (variable == 1) ? get_variable(data) : m_strdup(data);
	else
	{
		*p++ = 0;
		value = (variable == 1) ? get_variable(p) : m_strdup(p);
	}
	
	if (!value || !*value)
	{
		new_free(&value);
		RETURN_EMPTY;
	}

	pointer = svalue = value;

	do 
	{
		if ( (searchlen = (strlen(search) - 1)) < 0)
			searchlen = 0;
		if (this_global)
		{
			while ((pointer = func(pointer,search)))
			{
				pointer[0] = pointer[searchlen] = 0;
				pointer += searchlen + 1;
				m_e3cat(&booya, value, replace);
				value = pointer;
				if (!*pointer)
					break;
			}
		} 
		else
		{
			if ((pointer = func(pointer,search)))
			{
				pointer[0] = pointer[searchlen] = 0;
				pointer += searchlen + 1;
				m_e3cat(&booya, value, replace);
				value = pointer;
			}
		}
		malloc_strcat(&booya, value);
		if (data && *data)
		{
			new_free(&svalue);
			search = data;
			if ((replace = strchr(data, delimiter)))
			{
				*replace++ = 0;
				if ((data = strchr(replace, delimiter)))
					*data++ = 0;
			}
			/* patch from RoboHak */
			if (!replace || !search)
			{
				pointer = value = svalue;
				break;
			}
			pointer = value = svalue = booya;
			booya = NULL;
		} else 
			break;
	} while (1);

	if (variable) 
	{
		window_display = 0;
		add_var_alias(data, booya);
		window_display = oldwindow;
	}
	new_free(&svalue);
	return booya ? booya : m_strdup(empty_string);
}
#endif
BUILT_IN_FUNCTION(function_msar, word)
{
	char    delimiter;
	char    *pointer        = NULL;
	char    *search         = NULL;
	char    *replace        = NULL;
	char    *data           = NULL;
	char    *value          = NULL;
	char    *booya          = NULL;
	char    *p              = NULL;
	int     variable 	= 0,
		this_global 		= 0,
		searchlen,
		oldwindow 	= window_display;
	char 	*(*func) (const char *, const char *) = strstr;
	char    *svalue;

        while (((*word == 'r') && (variable = 1)) || ((*word == 'g') && (this_global = 1)) || ((*word == 'i') && (func = (char *(*)(const char *, const char *))global[STRISTR])))
                word++;

        RETURN_IF_EMPTY(word);

        delimiter = *word;
        search = word + 1;
        if (!(replace = strchr(search, delimiter)))
                RETURN_EMPTY;

        *replace++ = 0;
        if (!(data = strchr(replace,delimiter)))
                RETURN_EMPTY;

        *data++ = 0;

        if (!(p = strrchr(data, delimiter)))
                value = (variable == 1) ? get_variable(data) : m_strdup(data);
        else
        {
                *p++ = 0;
                value = (variable == 1) ? get_variable(p) : m_strdup(p);
        }

        if (!value || !*value)
        {
                new_free(&value);
                RETURN_EMPTY;
        }

        pointer = svalue = value;

        do 
        {
                searchlen = strlen(search) - 1;
                if (searchlen < 0)
                        searchlen = 0;
                if (this_global)
                {
                        while ((pointer = func(pointer,search)))
                        {
                                pointer[0] = pointer[searchlen] = 0;
                                pointer += searchlen + 1;
                                m_e3cat(&booya, value, replace);
                                value = pointer;
                                if (!*pointer)
                                        break;
                        }
                } 
                else
                {
                        if ((pointer = func(pointer,search)))
                        {
                                pointer[0] = pointer[searchlen] = 0;
                                pointer += searchlen + 1;
                                m_e3cat(&booya, value, replace);
                                value = pointer;
                        }
                }
                malloc_strcat(&booya, value);
                if (data && *data)
                {
			new_free(&svalue);
                        search = data;
                        if ((replace = strchr(data, delimiter)))
                        {
                                *replace++ = 0;
                                if ((data = strchr(replace, delimiter)))
                                        *data++ = 0;
                        }
			/* patch from RoboHak */
			if (!replace || !search)
			{
				pointer = value = svalue;
				break;
			}
			pointer = value = svalue = booya;
			booya = NULL;
                } else 
                        break;
        } while (1);

        if (variable) 
        {
                window_display = 0;
                add_var_alias(data, booya);
                window_display = oldwindow;
        }
        new_free(&svalue);
        return (booya);
}


BUILT_IN_FUNCTION(function_center, word)
{
	int	length,pad,width;
	char 	*padc;

	if (!word || !*word)
		RETURN_EMPTY;

	width = atoi(new_next_arg(word, &word));
	RETURN_IF_EMPTY(word);
	length = strlen(word);
	
	if ((pad = width - length) < 0)
		RETURN_STR(word);

	pad /= 2;
	padc = (char *)new_malloc(width+1);
	memset(padc, ' ', pad);
	padc[pad] = '\0';

	return strcat(padc, word);
}

BUILT_IN_FUNCTION(function_split, word)
{
	char	*chrs;
	register char	*pointer;

	chrs = next_arg(word, &word);
	pointer = word;
	while ((pointer = sindex(pointer,chrs)))
		*pointer++ = ' ';

	RETURN_STR(word);
}

BUILT_IN_FUNCTION(function_chr, word)
{
	char aboo[BIG_BUFFER_SIZE];
	unsigned char *ack = aboo;
	char *blah;

	while ((blah = next_arg(word, &word)))
		*ack++ = (unsigned char)atoi(blah);

	*ack = '\0';
	RETURN_STR(aboo);
}

BUILT_IN_FUNCTION(function_ascii, word)
{
	char *aboo = NULL;
	unsigned char *w = word;
	if (!word || !*word)
		RETURN_EMPTY;

	aboo = m_strdup(ltoa((unsigned long) *w));
	while (*++w)
		m_3cat(&aboo, space, ltoa((unsigned long) *w));
		
	return (aboo);
}

BUILT_IN_FUNCTION(function_which, word)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	char *file1;

	GET_STR_ARG(file1, word);

	if ((file1 = path_search(file1, (word && *word) ? word :
		get_string_var(LOAD_PATH_VAR))))
	{
		RETURN_STR(file1);
	}
	else
	{
		new_free(&file1);
		RETURN_EMPTY;
	}
#endif
}


BUILT_IN_FUNCTION(function_isalpha, input)
{
	if (isalpha((unsigned char)*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isdigit, input)
{
	if (isdigit((unsigned char)*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isalnum, input)
{
	if (isalpha((unsigned char)*input) || isdigit((unsigned char)*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isspace, input)
{
	if (isspace((unsigned char)*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isxdigit, input)
{
	#define ishex(x) \
		((x >= 'A' && x <= 'F') || (x >= 'a' && x <= 'f'))

	if (isdigit((unsigned char)*input) || ishex(*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);	
}

BUILT_IN_FUNCTION(function_serverpass, input)
{
	int	servnum = from_server;

	if (*input)
		GET_INT_ARG(servnum, input);

	if (servnum == -1)
		servnum = from_server;
	if (servnum < 0 || servnum > server_list_size())
		RETURN_EMPTY;

	return m_sprintf("%d", get_server_pass(servnum));	
}

BUILT_IN_FUNCTION(function_open, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	char *filename;
	char *mode;
	char *bin_mode = NULL;
	
	GET_STR_ARG(filename, words);
	GET_STR_ARG(mode, words);
	if (in_cparse)
		RETURN_EMPTY;
	if (words && *words)
		bin_mode = words;
		
	if (*mode == 'R' || *mode == 'r')
	{
		if (bin_mode && (*bin_mode == 'B' || *bin_mode == 'b'))
			RETURN_INT(open_file_for_read(filename));
		else
			RETURN_INT(open_file_for_read(filename));
	}
	else if (*mode == 'W' || *mode == 'w')
	{
		if (bin_mode && (*bin_mode == 'B' || *bin_mode == 'b'))
			RETURN_INT(open_file_for_bwrite(filename));
		else
			RETURN_INT(open_file_for_write(filename));
	}
	RETURN_EMPTY;	
#endif
}

BUILT_IN_FUNCTION(function_close, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	RETURN_IF_EMPTY(words);
	if (in_cparse)
		RETURN_INT(0);
	RETURN_INT(file_close(atoi(new_next_arg(words, &words))));
#endif
}	

BUILT_IN_FUNCTION(function_write, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	char *fdc;
	if (in_cparse)
		RETURN_INT(0);
	GET_STR_ARG(fdc, words);
	RETURN_INT(file_write(atoi(fdc), words));
#endif
}

BUILT_IN_FUNCTION(function_writeb, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	char *fdc;
	if (in_cparse)
		RETURN_INT(0);
	GET_STR_ARG(fdc, words);
	RETURN_INT(file_writeb(atoi(fdc), words));
#endif
}

BUILT_IN_FUNCTION(function_read, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	char *fdc = NULL, *numb = NULL;
	if (in_cparse)
		RETURN_INT(0);

	GET_STR_ARG(fdc, words);
	if (words && *words)
		GET_STR_ARG(numb, words);

	if (numb)
		return file_readb (atoi(fdc), atoi(numb));
	else
		return file_read (atoi(fdc));
#endif
}

BUILT_IN_FUNCTION(function_eof, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	RETURN_IF_EMPTY(words);
	RETURN_INT(file_eof(atoi(new_next_arg(words, &words))));
#endif
}


BUILT_IN_FUNCTION(function_iptoname, words)
{
	char *ret = ip_to_host(words);
	RETURN_STR(ret);
}

BUILT_IN_FUNCTION(function_nametoip, words)
{
	char *ret = host_to_ip(words);
	RETURN_STR(ret);
}

BUILT_IN_FUNCTION(function_convert, words)
{
	char *ret = one_to_another(words);
	RETURN_STR(ret);
}

BUILT_IN_FUNCTION(function_translate, words)
{
register char *	oldc;
	char *	newc, 
	     *	text,
		delim;
register char *	ptr;
	int 	size_old, 
		size_new,
		x;

	RETURN_IF_EMPTY(words);

	oldc = words;
	/* First character can be a slash.  If it is, we just skip over it */
	delim = *oldc++;
	newc = strchr(oldc, delim);

	if (!newc)
		RETURN_EMPTY;	/* no text in, no text out */

	text = strchr(++newc, delim);

	if (newc == oldc)
		RETURN_EMPTY;

	if (!text)
		RETURN_EMPTY;
	*text++ = '\0';

	if (newc == text)
	{
		*newc = '\0';
		newc = empty_string;
	}
	else
		newc[-1] = 0;

	/* this is cheating, but oh well, >;-) */
	text = m_strdup(text);

	size_new = strlen(newc);
	size_old = strlen(oldc);

	for (ptr = text; ptr && *ptr; ptr++)
	{
		for (x = 0;x < size_old;(void)x++)
		{
			if (*ptr == oldc[x])
			{
				/* Check to make sure we arent
				   just eliminating the character.
				   If we arent, put in the new char,
				   otherwise strcpy it away */
				if (size_new)
					*ptr = newc[(x<size_new)?x:size_new-1];
				else
				{
					ov_strcpy (ptr, ptr+1);
					ptr--;				
				}
				break;
			}
		}
	}
	return text;
}

BUILT_IN_FUNCTION(function_server_version, word)
{
	int servnum;
	int version;

	servnum = ((word && *word) ? atoi(next_arg(word, &word)) : primary_server);

	if (servnum > server_list_size())
		RETURN_STR("unknown");

	version = get_server_version(servnum);

	if (version == Server2_8) 		RETURN_STR("2.8");
	else if (version == Server2_9) 		RETURN_STR("2.9");
	else if (version == Server2_10)		RETURN_STR("2.10");
	else if (version == Server2_8ts4)	RETURN_STR("2.8ts4");
	else if (version == Server2_8hybrid)	RETURN_STR("2.8hybrid5");
	else if (version == Server2_8hybrid6)	RETURN_STR("2.8hybrid6");
	else if (version == Server2_8comstud)	RETURN_STR("2.8comstud");

	else if (version == Server_u2_8) 	RETURN_STR("u2.8");
	else if (version == Server_u2_9) 	RETURN_STR("u2.9");
	else if (version == Server_u2_10)	RETURN_STR("u2.10");
	else if (version == Server_u3_0)	RETURN_STR("u3.0");

	RETURN_STR("Unknown");
}

BUILT_IN_FUNCTION(function_unlink, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	char *	expanded;
	int 	failure = 0;
	if (in_cparse)
		RETURN_INT(0);

	while (words && *words)
	{
		expanded = expand_twiddle(new_next_arg(words, &words));
		failure -= unlink(expanded);	
		new_free(&expanded);
	}
	RETURN_INT(failure);
#endif
}

BUILT_IN_FUNCTION(function_rename, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	char *	filename1, 
	     *	filename2;
	char *expanded1, *expanded2;
	int 	failure = 0;
	if (in_cparse)
		RETURN_INT(failure);

	GET_STR_ARG(filename1, words);
	GET_STR_ARG(filename2, words);
	expanded1 = expand_twiddle(filename1);
	expanded2 = expand_twiddle(filename2);
	failure = rename(expanded1, expanded2);
	new_free(&expanded1); new_free(&expanded2);
	RETURN_INT(failure);
#endif
}

BUILT_IN_FUNCTION(function_rmdir, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	int 	failure = 0;
	char	*expanded;
	
	if (in_cparse)
		RETURN_INT(failure);

	while (words && *words)
	{
		expanded = expand_twiddle(new_next_arg(words, &words));
		failure -= rmdir(expanded);
		new_free(&expanded);
	}
	
	RETURN_INT(failure);
#endif
}

BUILT_IN_FUNCTION(function_truncate, words)
{
	int		num = 0;
	double		value = 0;
	char		buffer[BIG_BUFFER_SIZE],
			format[BIG_BUFFER_SIZE];

	GET_INT_ARG(num, words);
	GET_FLOAT_ARG(value, words);

	if (num < 0)
	{
		float foo;
		int end;

		sprintf(format, "%%.%de", -num-1);
		sprintf(buffer, format, value);
		foo = atof(buffer);
		sprintf(buffer, "%f", foo);
		end = strlen(buffer) - 1;
		if (end == 0)
			RETURN_EMPTY;
		while (buffer[end] == '0')
			end--;
		if (buffer[end] == '.')
			end--;
		buffer[end+1] = 0;
	}
	else if (num > 0)
	{
		sprintf(format, "%%10.%dlf", num);
		sprintf(buffer, format, value);
	}
	else
		RETURN_EMPTY;

	while (*buffer == ' ')
		ov_strcpy(buffer, buffer+1);

	RETURN_STR(buffer);
}


/*
 * Apprantly, this was lifted from a CS client.  I reserve the right
 * to replace this code in future versions. (hop)
 */
/*
	I added this little function so that I can have stuff formatted
	into days, hours, minutes, seconds; but with d, h, m, s abreviations.
		-Taner
*/

BUILT_IN_FUNCTION(function_tdiff2, input)
{
	time_t	ltime;
	time_t	days,
		hours,
		minutes,
		seconds;
	char	tmp[80];
	char	*tstr;

	GET_INT_ARG(ltime, input);

	seconds = ltime % 60;
	ltime = (ltime - seconds) / 60;
	minutes = ltime%60;
	ltime = (ltime - minutes) / 60;
	hours = ltime % 24;
	days = (ltime - hours) / 24;
	tstr = tmp;

	if (days)
	{
		sprintf(tstr, "%ldd ", days);
		tstr += strlen(tstr);
	}
	if (hours)
	{
		sprintf(tstr, "%ldh ", hours);
		tstr += strlen(tstr);
	}
	if (minutes)
	{
		sprintf(tstr, "%ldm ", minutes);
		tstr += strlen(tstr);
	}
	if (seconds || (!days && !hours && !minutes))
	{
		sprintf(tstr, "%lds", seconds);
		tstr += strlen(tstr);
	}
	else
		*--tstr = 0;	/* chop off that space! */

	RETURN_STR(tmp);
}


/* 
 * Apparantly, this was lifted from a CS client.  I reserve the right 
 * to replace this code in a future release.
 */
BUILT_IN_FUNCTION(function_utime, input)
{
	struct  timeval         tp;
	get_time(&tp);
	return m_sprintf("%ld %ld",(unsigned long)tp.tv_sec, (unsigned long)tp.tv_usec);
}


/*
 * This inverts any ansi sequence present in the string
 * from: Scott H Kilau <kilau@prairie.NoDak.edu>
 */
BUILT_IN_FUNCTION(function_stripansi, input)
{
	register unsigned char	*cp;
	for (cp = input; *cp; cp++)
		if (*cp < 31 && *cp > 13)
			if (*cp != 15 && *cp !=22)
				*cp = (*cp & 127) | 64;
	RETURN_STR(input);
}

#if 0
BUILT_IN_FUNCTION(function_stripansicodes, inPut)
{
	RETURN_IF_EMPTY(input);
	RETURN_STR(stripansicodes(input));
}
#endif

BUILT_IN_FUNCTION(function_stripc, input)
{
	char	*retval;
	retval = LOCAL_COPY(input);
	strcpy_nocolorcodes(retval, input);
	RETURN_STR(retval);
}

BUILT_IN_FUNCTION(function_servername, input)
{
	int sval = from_server;
	const	char *which;

	if(*input)
		GET_INT_ARG(sval, input);

	/* garbage in, garbage out. */
	if (sval < 0 || sval >= server_list_size())
		RETURN_EMPTY;

	/* First we try to see what the server thinks it name is */
	which = get_server_itsname(sval);
	
	/* Next we try what we think its name is */
	if (!which)
		which = get_server_name(sval);

	/* Ok. i give up, return a null. */
	RETURN_STR(which);
}

BUILT_IN_FUNCTION(function_lastserver, input)
{
	RETURN_INT(last_server);
}

/*
 * Date: Sat, 26 Apr 1997 13:41:11 +1200
 * Author: IceKarma (ankh@canuck.gen.nz)
 * Contributed by: author
 *
 * Usage: $winchan(#channel <server refnum|server name>)
 * Given a channel name and either a server refnum or a direct server
 * name or an effective server name, this function will return the 
 * refnum of the window where the channel is the current channel (on that
 * server if appropriate.)
 *
 * Returns -1 (Too few arguments specified, or Window Not Found) on error.
 */
BUILT_IN_FUNCTION(function_chanwin, input)
{
	char *arg1 = NULL;

	if (input && *input)
		GET_STR_ARG(arg1, input);

	/*
	 * Return window refnum by channel
	 */
	if (arg1 && is_channel(arg1))
	{
		int 	servnum = from_server;
		char 	*chan, 
			*serv = NULL;
		ChannelList *ch = NULL;

		chan = arg1;
		if ((serv = new_next_arg(input, &input)))
		{
			if (my_isdigit(serv))
				servnum = my_atol(serv);
			else
				servnum = find_in_server_list(serv, 0);
		}

		if((ch = lookup_channel(chan, servnum, CHAN_NOUNLINK)) && ch->window)
			RETURN_INT(ch->window->refnum);

		if (fn && !strcmp(fn, "CHANWINDOW"))
			RETURN_INT(0);
		else
			RETURN_INT(-1);
	}

	/*
	 * Return channel by window refnum/desc
	 */
	else
	{
		Window *win = current_window;

		if (arg1 && *arg1)
			win = get_window_by_desc(arg1);

		if (!win)
			RETURN_EMPTY;

		RETURN_STR(win->current_channel);
	}
	if (fn && !strcmp(fn, "CHANWINDOW"))
		RETURN_INT(0);
	RETURN_INT(-1);
}

BUILT_IN_FUNCTION(function_winserv, input)
{
	int win = 0;
	char *tmp;
	Window *winp;

	if (input && *input)
	{
		if ((tmp = new_next_arg(input, &input)))
			win = atoi(tmp);
	}
	if ((winp = get_window_by_refnum(win)))
		RETURN_INT(winp->server);

	RETURN_INT(-1);
}

BUILT_IN_FUNCTION(function_numwords, input)
{
	RETURN_INT(word_count(input));
}

BUILT_IN_FUNCTION(function_strlen, input)
{
	RETURN_INT(strlen(input));
}

/* 
 * Next two contributed by Scott H Kilau (sheik), who for some reason doesnt 
 * want to take credit for them. *shrug* >;-)
 *
 * Deciding not to be controversial, im keeping the original (contributed)
 * semantics of these two functions, which is to return 1 on success and
 * -1 on error.  If you dont like it, then tough. =)  I didnt write it, and
 * im not going to second guess any useful contributions. >;-)
 */
BUILT_IN_FUNCTION(function_fexist, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
        char	FileBuf[BIG_BUFFER_SIZE+1];
	char	*filename, *fullname;

	*FileBuf = 0;
	if ((filename = new_next_arg(words, &words)))
	{
		if (*filename == '/')
			strlcpy(FileBuf, filename, BIG_BUFFER_SIZE);

		else if (*filename == '~') 
		{
			if (!(fullname = expand_twiddle(filename)))
				RETURN_INT(-1);

			strmcpy(FileBuf, fullname, BIG_BUFFER_SIZE);
			new_free(&fullname);
		}
#if defined(__EMX__) || defined(WINNT)
		else if (is_dos(filename))
			strmcpy(FileBuf, filename, BIG_BUFFER_SIZE);
#endif
		else 
		{
			getcwd(FileBuf, BIG_BUFFER_SIZE);
			strmcat(FileBuf, "/", BIG_BUFFER_SIZE);
			strmcat(FileBuf, filename, BIG_BUFFER_SIZE);
		}
#if defined(__EMX__) || defined(WINNT)
		convert_dos(FileBuf);
#endif
		if (access(FileBuf, R_OK) == -1)
			RETURN_INT(-1);

		else
			RETURN_INT(1);
	}
	RETURN_INT(-1);
#endif
}

/* XXXX - ugh. do we really have to do a access() call first? */
BUILT_IN_FUNCTION(function_fsize, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else 
        char	FileBuf[BIG_BUFFER_SIZE+1];
	char	*filename, *fullname;
        struct  stat    stat_buf;
	off_t	filesize = 0;

	filename = new_next_arg(words, &words);
	*FileBuf = 0;
	if (filename && *filename) 
	{
		if (*filename == '/')
			strlcpy(FileBuf, filename, BIG_BUFFER_SIZE);

		else if (*filename == '~') 
		{
			if (!(fullname = expand_twiddle(filename)))
				RETURN_INT(-1);

			strmcpy(FileBuf, fullname, BIG_BUFFER_SIZE);
			new_free(&fullname);
		}
#if defined(__EMX__) || defined(WINNT)
		else if (is_dos(filename))
			strmcpy(FileBuf, filename, BIG_BUFFER_SIZE);
#endif
		else 
		{
			getcwd(FileBuf, BIG_BUFFER_SIZE);
			strmcat(FileBuf, "/", BIG_BUFFER_SIZE);
			strmcat(FileBuf, filename, BIG_BUFFER_SIZE);
		}

#if defined(__EMX__) || defined(WINNT)
		convert_dos(FileBuf);
#endif
		if ((stat(FileBuf, &stat_buf)) != -1)
		{
			filesize = stat_buf.st_size;
			RETURN_INT(filesize);
		}
	}
	RETURN_INT(-1);
#endif
}

/* 
 * Contributed by CrowMan
 * I changed two instances of "RETURN_INT(result)"
 * (where result was a null pointer) to RETURN_STR(empty_string)
 * because i dont think he meant to return a null pointer as an int value.
 */
/*
 * $crypt(password seed)
 * What it does: Returns a 13-char encrypted string when given a seed and
 *    password. Returns zero (0) if one or both args missing. Additional
 *    args ignored.
 * Caveats: Password truncated to 8 chars. Spaces allowed, but password
 *    must be inside "" quotes.
 * Credits: Thanks to Strongbow for showing me how crypt() works.
 * This cheap hack by: CrowMan
 */
BUILT_IN_FUNCTION(function_crypt, words)
{
#if defined(WINNT)
	RETURN_STR(empty_string);
#else
        char pass[9] = "\0";
        char seed[3] = "\0";
        char *blah, *bleh, *crypt (const char *, const char *);

	GET_STR_ARG(blah, words)
	GET_STR_ARG(bleh, words)
	strmcpy(pass, blah, 8);
	strmcpy(seed, bleh, 2);

	RETURN_STR(crypt(pass, seed));
#endif
}

BUILT_IN_FUNCTION(function_info, words)
{
/*
	char *which;
	extern char *compile_info;
	extern char *info_c_sum;

	GET_STR_ARG(which, words);

	     if (!my_strnicmp(which, "C", 1))
		RETURN_STR(compile_info);
	else if (!my_strnicmp(which, "S", 1))
		RETURN_STR(info_c_sum);

	else
*/
		return m_sprintf("%s+%s", version, compile_time_options);
	/* more to be added as neccesary */
}

/*
 * Based on a contribution made a very long time ago by wintrhawk
 */
BUILT_IN_FUNCTION(function_channelmode, word)
{
	char	*channel;
	char    *booya = NULL;
	char	*mode;
	int	type_mode = 0;
	do
	{
		channel = new_next_arg(word, &word);
		if ((!channel || !*channel) && booya)
			break;
		if (word && *word && isdigit((unsigned char)*word))
			GET_INT_ARG(type_mode, word);
		switch(type_mode)
		{
			case 1: /* bans */
			case 2: /* ban who time */
			case 3: /* exemptions [ts4] */
				mode = get_channel_bans(channel, from_server, type_mode);
				m_s3cat(&booya, space, (mode && *mode) ? mode : "*");
				new_free(&mode);
				continue;
			case 0:
			default:
				mode = get_channel_mode(channel, from_server);
			break;
		}
		m_s3cat(&booya, space, (mode && *mode) ? mode : "*");
	}
	while (word && *word);

	return (booya ? booya : m_strdup(empty_string));
}

BUILT_IN_FUNCTION(function_geom, words)
{
	/* Erf. CO and LI are ints. (crowman) */
	return m_sprintf("%d %d", current_term->TI_cols, current_term->TI_lines);
}

BUILT_IN_FUNCTION(function_pass, words)
{
	char *lookfor;
	char *final, *ptr;

	GET_STR_ARG(lookfor, words);
	final = (char *)new_malloc(strlen(words) + 1);
	ptr = final;

 	while (*words)
	{
		if (strchr(lookfor, *words))
			*ptr++ = *words;
		words++;
	}

	*ptr = 0;
	return final;
}

BUILT_IN_FUNCTION(function_uptime, input)
{
	time_t  ltime;
	time_t  days,hours,minutes,seconds;
	struct  timeval         tp;
	static time_t timestart = 0;
	time_t timediff;
	char buffer[BIG_BUFFER_SIZE+1];

	*buffer = '\0';

	get_time(&tp);
	if (timestart == 0)  
	{
		timestart = tp.tv_sec;
		timediff = 0;
	} else
		timediff = tp.tv_sec - timestart;

	ltime = timediff;
	seconds = ltime % 60;
	ltime = (ltime - seconds) / 60;
	minutes = ltime%60;
	ltime = (ltime - minutes) / 60;
	hours = ltime % 24;
	days = (ltime - hours) / 24;
	sprintf(buffer, "%ldd %ldh %ldm %lds", days, hours, minutes, seconds);
	RETURN_STR(buffer);
}

BUILT_IN_FUNCTION(function_cluster, input)
{
char *q;
	RETURN_IF_EMPTY(input);
	if ((q = cluster(input)))
		RETURN_STR(q);
	else
		RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_checkshit, input)
{
char *uh = NULL;
char *channel = NULL;
register ShitList *tmp;
int matched = 0;
	GET_STR_ARG(uh, input);
	GET_STR_ARG(channel, input);	

	for (tmp = shitlist_list; tmp; tmp = tmp->next)
	{
		if (wild_match(tmp->filter, uh) && check_channel_match(tmp->channels, channel))
		{
			matched = 1;
			break;
		}
	}

	if (tmp && matched)
		return m_sprintf("%d %s %s %s", tmp->level, tmp->channels, tmp->filter, tmp->reason);
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_get_info, input)
{
#ifdef WANT_USERLIST
	char *nick;
	register UserList *tmp;
	void *needed = NULL;
	int size = -1;
	GET_STR_ARG(nick, input);
	for (tmp = next_userlist(NULL, &size, &needed); tmp; tmp = next_userlist(tmp, &size, &needed))
	{
		if (!my_stricmp(nick, tmp->nick) || wild_match(nick, tmp->nick))
		{
			if (tmp->comment)
				RETURN_STR(tmp->comment);
		}
	}
#endif
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_set_info, input)
{
#ifdef WANT_USERLIST
	char *nick;
	register UserList *tmp;
	void *location = NULL;
	int size = -1;
	int done = 0;
	GET_STR_ARG(nick, input);
	for (tmp = next_userlist(NULL, &size, &location); tmp; tmp = next_userlist(tmp, &size, &location))
	{
		if (!my_stricmp(tmp->nick, nick) || wild_match(nick, tmp->nick))
		{
			if (input && *input)
				malloc_strcpy(&tmp->comment, input);
			else
				new_free(&tmp->comment);
			done++;
		}
	}
	if (done)
		RETURN_INT(1);
#endif
	RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_checkuser, input)
{
#ifdef WANT_USERLIST
char *uh = NULL;
char *channel = NULL;
register UserList *tmp;
	GET_STR_ARG(uh, input);
	GET_STR_ARG(channel, input);	
	if (!uh || !*uh || !channel || !*channel)
		RETURN_EMPTY;
	if ((tmp = find_bestmatch("*", uh, channel, NULL)))
		return m_sprintf("%d %s %s %s", tmp->flags, tmp->host, tmp->channels, tmp->password?tmp->password:empty_string);
#endif
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_rot13, input)
{
	char temp[BIG_BUFFER_SIZE+1];
	register char *p = NULL;
	int rotate = 13;
	strmcpy(temp, input, BIG_BUFFER_SIZE);
	for (p = temp; *p; p++) {
		if (*p >= 'A' && *p <='Z')
			*p = (*p - 'A' + rotate) % 26 + 'A';
		else if (*p >= 'a' && *p <= 'z')
			*p = (*p - 'a' + rotate) % 26 + 'a';
	}
	RETURN_STR(temp);
}

BUILT_IN_FUNCTION(function_repeat, words)
{
	register int num;
	char *final = NULL;

	GET_INT_ARG(num, words);
	if (num < 1)
		RETURN_EMPTY;

	final = (char *)new_malloc(strlen(words) * num + 1);
	for (; num > 0; num--)
		strcat(final, words);
	if (strlen(final) > BIG_BUFFER_SIZE)
		final[BIG_BUFFER_SIZE] = 0;
	return final;
}


BUILT_IN_FUNCTION(function_bcopy, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	int from, to;
	if (in_cparse)
		RETURN_INT(0);
	GET_INT_ARG(from, words);
	GET_INT_ARG(to, words);	
	RETURN_INT(file_copy(from, to)); 	
#endif
}

BUILT_IN_FUNCTION(function_epic, words)
{
	RETURN_INT(1);
}

BUILT_IN_FUNCTION(function_runlevel, words)
{
	extern int run_level;

	RETURN_INT(run_level);
}

BUILT_IN_FUNCTION(function_ovserver, words)
{
	int ovserver;
	char *servname = NULL;

	GET_INT_ARG(ovserver, words);
	if(ovserver > -1)
		servname = ov_server(ovserver);

	if(servname)
		RETURN_STR(servname);

	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_winsize, words)
{
	int refnum;
	Window *win;

	if (words && *words)
	{
		GET_INT_ARG(refnum, words);
		win = get_window_by_refnum(refnum);
	}
	else
		win = current_window;

	if (!win)
		RETURN_EMPTY;
		
	RETURN_INT(win->display_size);
}

BUILT_IN_FUNCTION(function_umode, words)
{
	int servnum;

	if (words && *words)
	{
		GET_INT_ARG(servnum, words);
	}
	else
		servnum = from_server;

	RETURN_STR(get_umode(servnum));
}


BUILT_IN_FUNCTION(function_lastnotice, words)
{
	int count = 0;
	char *str = NULL;
	GET_INT_ARG(count, words);
	if (count >= MAX_LAST_MSG)
		count = 0;
	RETURN_IF_EMPTY(last_notice[count].last_msg);
	malloc_sprintf(&str, "%s %s %s %s %s", last_notice[count].time, last_notice[count].from, last_notice[count].uh, last_notice[count].to, last_notice[count].last_msg);
	return str;
}

BUILT_IN_FUNCTION(function_lastmessage, words)
{
	int count = 0;
	char *str = NULL;
	GET_INT_ARG(count, words);
	if (count >= MAX_LAST_MSG)
		count = 0;
	RETURN_IF_EMPTY(last_msg[count].last_msg);
	malloc_sprintf(&str, "%s %s %s %s %s", last_msg[count].time, last_msg[count].from, last_msg[count].uh, last_msg[count].to, last_msg[count].last_msg);
	return str;
}

char *function_addtabkey (char *n, char *words)
{
char *arrayname = NULL;
char *nick = NULL;

	GET_STR_ARG(nick, words);
	if (words && *words)
		arrayname = new_next_arg(words, &words);
	addtabkey(nick, NULL, arrayname ? 1: 0);
	RETURN_EMPTY;
}

char *function_gettabkey (char *n, char *words)
{
char *arrayname = NULL;
NickTab *nick = NULL;
int direction;

	GET_INT_ARG(direction, words);
	
	if (words && *words)
		arrayname = new_next_arg(words, &words);
	
	if (arrayname && !my_stricmp(arrayname, "AUTOREPLY"))
		nick = gettabkey(direction, 1, NULL);
	else
		nick = gettabkey(direction, 0, NULL);
	
	return nick ? m_strdup(nick->nick) : m_strdup(empty_string);
}

static int sort_it (const void *one, const void *two)
{
	return my_stricmp(*(char **)one, *(char **)two);
}

BUILT_IN_FUNCTION(function_sort, words)
{
	int wordc;
	char **wordl;

	wordc = splitw(words, &wordl);
	qsort((void *)wordl, wordc, sizeof(char *), sort_it);
	return unsplitw(&wordl, wordc);	/* DONT USE RETURN_STR() HERE */
}

BUILT_IN_FUNCTION(function_notify, words)
{
	int showon = -1, showserver = from_server;
	char *firstw;
	int len = 0;
	char *buf = NULL, *ret = NULL;
		
	while (words && *words)
	{
		firstw = new_next_arg(words, &words);
		len = strlen(firstw);
		if (*firstw == '+')
		{
			if (len == 1)
				showon = 1;
			else
				buf = get_notify_nicks(showserver, 1, firstw+1, 0);
		}
		else if (*firstw == '-')
		{
			if (len == 1)
				showon = 0;
			else
				buf = get_notify_nicks(showserver, 1, firstw+1, 0);
		}
		else if (*firstw == '!')
		{
			if (len != 1)
				buf = get_notify_nicks(showserver, showon, firstw+1, 1); 
		}
		else if (!my_strnicmp(firstw, "serv", 4))
			GET_INT_ARG(showserver, words);

		if (buf)
		{
			m_s3cat(&ret, space, buf);
			new_free(&buf);
		}
	}
 
	/* dont use RETURN_STR() here. */
	return ret ? ret : get_notify_nicks(showserver, showon, NULL, 0);
}

BUILT_IN_FUNCTION(function_watch, words)
{
	int showon = -1, showserver = from_server;
	char *firstw;
	int len = 0;
	char *buf = NULL, *ret = NULL;
		
	while (words && *words)
	{
		firstw = new_next_arg(words, &words);
		len = strlen(firstw);
		if (*firstw == '+')
		{
			if (len == 1)
				showon = 1;
			else
				buf = get_watch_nicks(showserver, 1, firstw+1, 0);
		}
		else if (*firstw == '-')
		{
			if (len == 1)
				showon = 0;
			else
				buf = get_watch_nicks(showserver, 1, firstw+1, 0);
		}
		else if (*firstw == '!')
		{
			if (len != 1)
				buf = get_watch_nicks(showserver, showon, firstw+1, 1); 
		}
		else if (!my_strnicmp(firstw, "serv", 4))
			GET_INT_ARG(showserver, words);

		if (buf)
		{
			m_s3cat(&ret, space, buf);
			new_free(&buf);
		}
	}
 
	/* dont use RETURN_STR() here. */
	return ret ? ret : get_watch_nicks(showserver, showon, NULL, 0);
}

static int num_sort_it (const void *one, const void *two)
{
	char *oneptr = *(char **)one;
	char *twoptr = *(char **)two;
	long val1, val2;

	while (*oneptr && *twoptr)
	{
		while (*oneptr && *twoptr && !isdigit((unsigned char)*oneptr) && !isdigit((unsigned char)*twoptr))
		{
			if (*oneptr != *twoptr)
				return (*oneptr - *twoptr);
			oneptr++, twoptr++;
		}

		if (!*oneptr || !*twoptr)
			break;

		val1 = strtol(oneptr, (char **)&oneptr, 10);
		val2 = strtol(twoptr, (char **)&twoptr, 10);
		if (val1 != val2)
			return val1 - val2;
	}
	return (*oneptr - *twoptr);
}

BUILT_IN_FUNCTION(function_numsort, words)
{
	int wordc;
	char **wordl;

	wordc = splitw(words, &wordl);
	qsort((void *)wordl, wordc, sizeof(char *), num_sort_it);

	return unsplitw(&wordl, wordc);	/* DONT USE RETURN_STR() HERE */
}

#ifdef NEED_GLOB
#define glob bsd_glob
#define globfree bsd_globfree
#endif

BUILT_IN_FUNCTION(function_glob, word)
{
#if defined(INCLUDE_GLOB_FUNCTION) && !defined(PUBLIC_ACCESS)
	char	*path, 
		*path2 = NULL, 
		*retval = NULL;
	int	numglobs,
		i;
	glob_t	globbers;

	memset(&globbers, 0, sizeof(glob_t));
	while (word && *word)
	{
		GET_STR_ARG(path, word);
		path2 = expand_twiddle(path);
		if (!path2)
			path2 = m_strdup(path);

		numglobs = glob(path2, GLOB_MARK, NULL, &globbers);
		if (numglobs < 0)
		{
			new_free(&path2);
			RETURN_INT(numglobs);
		}
		for (i = 0; i < globbers.gl_pathc; i++)
		{
			if (strchr(globbers.gl_pathv[i], ' '))
			{
				int len = strlen(globbers.gl_pathv[i])+4;
				char *b = alloca(len+1);
				*b = 0;
				strmopencat(b, len, "\"", globbers.gl_pathv[i], "\"", NULL);
				m_s3cat(&retval, space, b);
			}
			else
				m_s3cat(&retval, space, globbers.gl_pathv[i]);
		}
		globfree(&globbers);
		new_free(&path2);
	}
	return retval ? retval : m_strdup(empty_string);
#else
	RETURN_EMPTY;
#endif
}

BUILT_IN_FUNCTION(function_mkdir, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_EMPTY;
#else
	int 	failure = 0;
	char *expanded;

	while (words && *words)
	{
		expanded = expand_twiddle(new_next_arg(words, &words));
		failure -= mkdir(expanded, 0777);
		new_free(&expanded);
	}

	RETURN_INT(failure);
#endif
}

BUILT_IN_FUNCTION(function_umask, words)
{
	int new_umask;
	GET_INT_ARG(new_umask, words);
	RETURN_INT(umask(new_umask));
}

BUILT_IN_FUNCTION(function_help, words)
{
#if defined(WANT_CHELP) && !defined(PUBLIC_ACCESS)
	extern void get_help_topic(const char *, int);
	extern int read_file(FILE *, int);

	static int first_time = 1;
	char *subject;

	GET_STR_ARG(subject, words);

	if (first_time)
	{
		FILE *help_file;
		char *new_file = NULL;

		malloc_strcpy(&new_file, get_string_var(SCRIPT_HELP_VAR));
		help_file = uzfopen(&new_file, get_string_var(LOAD_PATH_VAR), 0);
		new_free(&new_file);

		if (!help_file)
			RETURN_EMPTY;

		first_time = 0;
		read_file(help_file, 1);
		fclose(help_file);
	}

	get_help_topic(subject, 1);
#endif
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_isuser, words)
{
#ifdef WANT_USERLIST
char *nick = NULL;
char *uhost = NULL;
char *channel = NULL;
register UserList *User;
register ShitList *Shit;

	GET_STR_ARG(nick, words);
	GET_STR_ARG(uhost, words);
	if (words && *words)
		channel = words;
	else
		channel = "*";
	if ((User = lookup_userlevelc("*", uhost, channel, NULL)))
		return m_sprintf("USER %d %s %s", User->flags, User->host, User->channels);
	if ((Shit = nickinshit(nick, uhost)) && check_channel_match(Shit->channels, channel))
		return m_sprintf("SHIT %s %d %s", Shit->filter, Shit->level, Shit->channels);
#endif
	RETURN_EMPTY;
}

/*
$pad(N string of text goes here)
if N is negative, it'll pad to the right 
if N is positive, it'll pad to the left

so something like: $pad(20 letters) would output:
            letters
and $pad(20 some string) would be:
        some string
GREAT way to allign shit, and if you use $curpos() can can add that and
figure out the indent :p
hohoho, aren't we ingenious, better yet, you can do a strlen() on the
string youw anna output, then - the curpos, and if its over, grab all
words to the right of that position and output them [so its on one line]
then indent, grab the next section of words( and then the next and then
next

   Jordy (jordy@thirdwave.net) 19960622 
*/

BUILT_IN_FUNCTION(function_pad, word)
{
	int	length = 0;
	GET_INT_ARG(length, word);
	return m_sprintf("%*s", length, word);
}

BUILT_IN_FUNCTION(function_isban, word)
{
	char *channel;
	char *ban;
	ShitList *Ban;
	GET_STR_ARG(channel, word);
	GET_STR_ARG(ban, word);
	if ((Ban = (ShitList *)find_in_list((List **)&shitlist_list, ban, 0)) && check_channel_match(channel, Ban->channels))
		RETURN_INT(1);
	RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isignored, word)
{
char *nick;
	GET_STR_ARG(nick, word);
	if (check_is_ignored(nick))
		RETURN_INT(1);
	RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_banonchannel, word)
{
	ChannelList *chan;
	char *channel;
	char *ban;
	GET_STR_ARG(channel, word);
	GET_STR_ARG(ban, word);
	if ((chan = lookup_channel(channel, from_server, 0)))
	{
		if (ban_is_on_channel(ban, chan))
			RETURN_INT(1);
	}
	RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isop, word)
{
	char *channel;
	char *nick;
	ChannelList *chan;
	NickList *Nick;
	GET_STR_ARG(channel, word);
	GET_STR_ARG(nick, word);
	if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
	{
		if ((Nick = find_nicklist_in_channellist(nick, chan, 0)) &&
				(Nick->userlist && (Nick->userlist->flags & ADD_OPS)))
			RETURN_INT(1);
	}
	RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isvoice, word)
{
	char *channel, *nickname;
	ChannelList *chan;
	NickList *Nick;
	char *ret = NULL;
	
	GET_STR_ARG(channel, word);
	/* Note: This is the old way, maintained for compatibility */
	if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
	{
		while ((nickname = next_in_comma_list(word, &word)))
		{
			if (!nickname || !*nickname) break;
			Nick = find_nicklist_in_channellist(nickname, chan, 0);
			m_s3cat(&ret, comma, Nick? (nick_isvoice(Nick) ? one : zero) : zero);
		}
		return ret;
	} else {
		/* args are in proper order, channel is actually the nick */
		nickname = channel;
		GET_STR_ARG(channel, word);
		
		if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK))) {
			Nick = find_nicklist_in_channellist(nickname, chan, 0);
			RETURN_STR(Nick ? (nick_isvoice(Nick) ? one : zero) : zero);
		}
	}
	
	RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_randomnick, word)
{
int len1 = 3, len2 = 9;
	if (word && *word)
		GET_INT_ARG(len1, word);
	if (word && *word)
		GET_INT_ARG(len2, word);
	RETURN_STR(random_str(len1,len2));
}


#ifndef BITCHX_LITE
BUILT_IN_FUNCTION(function_openserver, word)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
int port = -1;
char *servern = NULL;
int socket_num = -1;
unsigned short this_sucks;
int notify_mode = 0;
	GET_STR_ARG(servern, word);
	GET_INT_ARG(port, word);
	if (word && *word && toupper(*word) == 'N')
		notify_mode = 1;
	if (port > 0)
	{		
		this_sucks = (unsigned short)port;
		if ((socket_num = connect_by_number(servern, &this_sucks, SERVICE_CLIENT, PROTOCOL_TCP, 1)) < 0)
			RETURN_INT(-1);
		if ((add_socketread(socket_num, port, 0, servern, !notify_mode ? read_clonelist :read_clonenotify, NULL)) < 0)
		{
			close_socketread(socket_num);
			RETURN_INT(-1);
		}
	}
	RETURN_INT(socket_num);
#endif
}

BUILT_IN_FUNCTION(function_closeserver, word)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
int socket_num = -1;
	GET_INT_ARG(socket_num, word);
	if (socket_num > 0 && check_socket(socket_num))
		close_socketread(socket_num);
	RETURN_INT(socket_num);
#endif
}

BUILT_IN_FUNCTION(function_readserver, word)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
int socket_num = -1;
	GET_INT_ARG(socket_num, word);
	if ((socket_num > 0) && check_socket(socket_num))
	{
		char buffer[IRCD_BUFFER_SIZE+1];
		char *str;
		char *s;
		*buffer = 0;
		str = buffer;
		switch(dgets(str, socket_num, 0, IRCD_BUFFER_SIZE, NULL))
		{
			case 0:
				socket_num = 0;
				break;
			case -1:
				close_socketread(socket_num);
				socket_num = -1;
				break;
			default:
				if ((buffer[strlen(buffer)-1] == '\r') || (buffer[strlen(buffer)-1] == '\n'))
					buffer[strlen(buffer)-1] = 0;
				if ((buffer[strlen(buffer)-1] == '\r') || (buffer[strlen(buffer)-1] == '\n'))
					buffer[strlen(buffer)-1] = 0;
				s = m_sprintf("%d %s", strlen(buffer), buffer);
				return s;
		}
	}
	RETURN_INT(socket_num);
#endif
}


BUILT_IN_FUNCTION(function_readchar, word)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
int socket_num = -1;
	GET_INT_ARG(socket_num, word);
	if ((socket_num >= 0) && check_socket(socket_num))
	{
		char buffer[10];
		char *str;
		char *s;
		*buffer = 0;
		*(buffer+1) = 0;
		str = buffer;
		if ((read(socket_num, str, 1) == 1))
			s = m_sprintf("%c", *buffer);
		else
			s = m_sprintf(empty_string);
		return s;
	}
	RETURN_INT(socket_num);
#endif
}

BUILT_IN_FUNCTION(function_writeserver, word)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
int socket_num = -1;
int len = -1;
	GET_INT_ARG(socket_num, word);
	if ((socket_num > 0) && check_socket(socket_num))
		len = write_sockets(socket_num, word, strlen(word), 1);
	RETURN_INT(len);
#endif
}
#endif

char *function_cparse(char *n, char *word)
{
char *format = NULL;
char *blah;
	GET_STR_ARG(format, word);
	in_cparse++;
	blah = convert_output_format(format, word && *word? "%s":NULL, word&&*word?word:NULL);
	in_cparse--;
	RETURN_STR(blah);
}

BUILT_IN_FUNCTION(function_getreason, word)
{
char *nick = NULL;
	GET_STR_ARG(nick, word);
	RETURN_STR(get_reason(nick, word));
}

BUILT_IN_FUNCTION(function_chmod, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
	char *filearg, *after;
	int fd = -1;
	char *perm_s;
	mode_t perm;

	GET_STR_ARG(filearg, words);
	fd = (int) strtoul(filearg, &after, 10);
	GET_STR_ARG(perm_s, words);
	perm = (mode_t) strtol(perm_s, &perm_s, 8);

	if (after != words && *after == 0)
	{
		if (file_valid(fd))
#if defined(__EMX__) || defined(__OPENNT)
			RETURN_INT(0644);
#else
			RETURN_INT(fchmod(fd, perm));
#endif
		else
			RETURN_EMPTY;
	}
	else
	{
		char *expanded;
		int retval;
		expanded = expand_twiddle(filearg);
		retval = chmod(expanded, perm);
		new_free(&expanded);
		RETURN_INT(retval);
	}
#endif
}

BUILT_IN_FUNCTION(function_twiddle, words)
{
	if (words && *words)
		return expand_twiddle(new_next_arg(words, &words));

	RETURN_EMPTY;
}


/* 
 * Date: Sun, 29 Sep 1996 19:17:25 -0700
 * Author: Thomas Morgan <tmorgan@pobox.com>
 * Submitted-by: Archon <nuance@twri.tamu.edu>
 *
 * $uniq (string of text)
 * Given a set of words, returns a new set of words with duplicates
 * removed.
 * EX: $uniq(one two one two) returns "one two"
 */
BUILT_IN_FUNCTION(function_uniq, word)
{
        char    **list = NULL, *booya = NULL;
        int     listc, listi;
	char	*input, *tval;
	
	RETURN_IF_EMPTY(word);
        listc = splitw(word, &list);
	if (!listc)
		RETURN_EMPTY;
		
	booya = m_strdup(list[0]);
        for (listi = 1; listi < listc; listi++)
        {
		input = alloca(strlen(list[listi]) + strlen(booya) + 2);
		strcpy(input, list[listi]);
		strcat(input, space);
		strcat(input, booya);

		tval = function_findw(NULL, input);
		if (my_atol(tval) == -1)
			m_s3cat(&booya, space, list[listi]);
		new_free(&tval);
	}
        new_free((char **)&list);
	RETURN_MSTR(booya);
}

BUILT_IN_FUNCTION(function_uhost, word)
{
char *nick;
char *answer = NULL;
int i;
register ChannelList *chan;
NickList *n1;
	GET_STR_ARG(nick, word);
	for (i = 0; i < server_list_size(); i++)
	{
		for (chan = get_server_channels(i); chan; chan = chan->next)
		{
			if ((n1 = find_nicklist_in_channellist(nick, chan, 0)))
			{
				malloc_strcat(&answer, n1->host);
				malloc_strcat(&answer, space);
			}
		}
	}
	if (!answer)
		RETURN_EMPTY;
	return answer;
}

BUILT_IN_FUNCTION(function_numdiff, word)
{
int value1 = 0, value2 = 0;
	GET_INT_ARG(value1, word);
	GET_INT_ARG(value2, word);
	RETURN_INT(value1-value2);
}

BUILT_IN_FUNCTION(function_winvisible, word)
{
	RETURN_INT(get_visible_by_refnum(word));
}

#ifndef BITCHX_LITE
BUILT_IN_FUNCTION(function_mircansi, word)
{
extern char *mircansi(char *);
	RETURN_STR(mircansi(word));
}
#endif

BUILT_IN_FUNCTION(function_getenv, word)
{
char *p;
char *q;
	GET_STR_ARG(q, word);
	p = getenv(q);
	if (p && *p)
		RETURN_STR(p);
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_gethost, word)
{
register ChannelList *chan;
register NickList *nick;
char *arg;
	GET_STR_ARG(arg, word);
	if (from_server != -1)
	{
		for (chan = get_server_channels(from_server); chan; chan = chan->next)
		{
			for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
			{
				if (!my_stricmp(arg, nick->nick))
					RETURN_STR(nick->host);
			}
		}
	}
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_getvar, words)
{
	char *alias;
	int num = 0;
	int old_alias_debug = alias_debug;
	if ((alias = (char *)get_cmd_alias(upper(words), &num, NULL, NULL)))
	{
		alias_debug = old_alias_debug;
		RETURN_STR(alias);
	}
	RETURN_EMPTY;	
}


BUILT_IN_FUNCTION(function_aliasctl, input)
{
	extern char *aliasctl (char *, char *);
	return aliasctl(NULL, input);
}

BUILT_IN_FUNCTION(function_status, word)
{
	int window_refnum;
	int status_line;

	GET_INT_ARG(window_refnum, word);
	GET_INT_ARG(status_line, word);
	RETURN_STR(get_status_by_refnum(window_refnum, status_line));
}
extern char *stat_convert_format (Window *, char *);
BUILT_IN_FUNCTION(function_statsparse, word)
{
	return stat_convert_format(current_window, word);
}

BUILT_IN_FUNCTION(function_absstrlen, word)
{
char *tmp;
	if (!word || !*word)
		RETURN_INT(0);
	tmp = stripansicodes(word);
	RETURN_INT(strlen(tmp));
}

BUILT_IN_FUNCTION(function_findw, input)
{
	char	*word, *this_word;
	int	word_cnt = 0;

	GET_STR_ARG(word, input);

	while (input && *input)
	{
		GET_STR_ARG(this_word, input);
		if (!my_stricmp(this_word, word))
			RETURN_INT(word_cnt);

		word_cnt++;
	}

	RETURN_INT(-1);
}


BUILT_IN_FUNCTION(function_countansi, input)
{
	RETURN_INT(output_with_count(input, 0, 0));		
}

BUILT_IN_FUNCTION(function_iplong, word)
{
	char *blah;
	struct in_addr addr = {0};
	GET_STR_ARG(blah, word);
	if (inet_aton(blah, &addr))
		return m_sprintf("%lu", (unsigned long)htonl(addr.s_addr));
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_longip, word)
{
	char *blah = NULL;
	struct in_addr addr = {0};
	GET_STR_ARG(blah, word);
	addr.s_addr = atol(blah);
	RETURN_STR(inet_ntoa(addr));
}

#include  <stdlib.h>
BUILT_IN_FUNCTION(function_rword, word)
{
int num = 0;
long rn;
char *p;
#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif
	RETURN_IF_EMPTY(word);
	num = word_count(word);
	rn = (int)(((float)num)*rand()/(RAND_MAX+1.0));
	p = extract(word, rn, rn);		
	return p;
}

BUILT_IN_FUNCTION(function_winlen, word)
{
int refnum;
int len = 0;
Window *win = current_window;
	if (word && *word)
	{
		refnum = my_atol(word);
		win = get_window_by_refnum(refnum);
		if (!win)
			RETURN_INT(-1);
	}
	len = win->display_size;
	RETURN_INT(len);
}

BUILT_IN_FUNCTION(function_channel, word)
{
char *chan;
ChannelList     *channel = NULL; /* XXX */
NickList        *tmp = NULL;
char		*nicks = NULL;
int		sort_order = NICKSORT_NORMAL;
	chan = next_arg(word, &word);
	if (!chan) 
		chan = get_current_channel_by_refnum(0);
	RETURN_IF_EMPTY(chan);
	if ((channel = lookup_channel(chan, from_server, 0)))
	{
		NickList *list = NULL;
		if (word && *word)
			GET_INT_ARG(sort_order, word);
		list = sorted_nicklist(channel, sort_order);
		for (tmp = list; tmp; tmp = tmp->next)
		{
			if (nick_isircop(tmp))
				malloc_strcat(&nicks, "*");
			if (tmp->userlist)
				malloc_strcat(&nicks, "&");
			else if (tmp->shitlist)
				malloc_strcat(&nicks, "!");
			if (nick_isop(tmp)) 
				m_3cat(&nicks, "@", tmp->nick);
            else if (nick_ishalfop(tmp))
                m_3cat(&nicks, "%", tmp->nick);
			else if (nick_isvoice(tmp))
				m_3cat(&nicks, "+", tmp->nick);
			else
				m_3cat(&nicks, ".", tmp->nick);
			malloc_strcat(&nicks, space);
		}		
		clear_sorted_nicklist(&list);
	}
	return (nicks ? nicks : m_strdup(empty_string));
	
}

/* 
 * Given a channel, tells you what window its bound to.
 * Given a window, tells you what channel its bound to.
 */
BUILT_IN_FUNCTION(function_winbound, input)
{
	Window *foo;
	char *stuff;

	stuff = new_next_arg(input, &input);
	if (my_atol(stuff) && (foo = get_window_by_refnum((unsigned)my_atol(stuff))))
		RETURN_STR(get_bound_channel(foo));
	else if ((foo = get_window_by_name(stuff)))
		RETURN_STR(get_bound_channel(foo));
	else if ((foo = get_window_bound_channel(stuff)))
		RETURN_STR(get_refnum_by_window(foo));

	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_ftime, words)
{
#ifdef PUBLIC_ACCESS
	RETURN_INT(0);
#else
        char	FileBuf[BIG_BUFFER_SIZE+1];
	char	*filename, *fullname;
	struct stat s;

	*FileBuf = 0;
	if ((filename = new_next_arg(words, &words)))
	{
		if (*filename == '/')
			strlcpy(FileBuf, filename, BIG_BUFFER_SIZE);

		else if (*filename == '~') 
		{
			if (!(fullname = expand_twiddle(filename)))
				RETURN_EMPTY;

			strmcpy(FileBuf, fullname, BIG_BUFFER_SIZE);
			new_free(&fullname);
		}
#if defined(__EMX__) || defined(WINNT)
		else if (is_dos(filename))
			strmcpy(FileBuf, filename, BIG_BUFFER_SIZE);
#endif
		else 
		{
			getcwd(FileBuf, BIG_BUFFER_SIZE);
			strmcat(FileBuf, "/", BIG_BUFFER_SIZE);
			strmcat(FileBuf, filename, BIG_BUFFER_SIZE);
		}
#if defined(__EMX__) || defined(WINNT)
		convert_dos(FileBuf);
#endif
		if (stat(FileBuf, &s) == -1)
			RETURN_EMPTY;
		else
			RETURN_INT(s.st_mtime);
	}
	RETURN_EMPTY;
#endif
}

BUILT_IN_FUNCTION(function_irclib, input)
{
	RETURN_STR(irc_lib);
}

BUILT_IN_FUNCTION(function_country, input)
{
	RETURN_STR(country(input));
}

BUILT_IN_FUNCTION(function_servernick, input)
{
	char *servdesc;
	int refnum;

	if(*input)
	{
		GET_STR_ARG(servdesc, input);
		if ((refnum = parse_server_index(servdesc)) == -1)
			if ((refnum = find_in_server_list(servdesc, 0)) == -1)
				RETURN_EMPTY;
	} else if(from_server != -1)
		refnum = from_server;
	else
		RETURN_EMPTY;

	RETURN_STR(get_server_nickname(refnum));
}
 
BUILT_IN_FUNCTION(function_fparse, input)
{
char *format;
char *f = NULL;
Alias *a = NULL;
int owd = window_display;
	GET_STR_ARG(f, input);
	if (!f || !*f)
		RETURN_EMPTY;
	if ((format = make_fstring_var(f)))
	{
		f = convert_output_format(format, "%s", input?input:empty_string);
		new_free(&format);
		RETURN_STR(f);
	} 
	window_display = 0;
	if ((a = find_var_alias(f)) && a->stuff && input && *input)
		f = convert_output_format(a->stuff, "%s", input?input:empty_string);
	else if (input && *input)
		add_var_alias(f, input);
	else
		delete_var_alias(f, 0);
	window_display = owd;
	RETURN_STR(f);
}

BUILT_IN_FUNCTION(function_isconnected, input)
{
int server = 0;
	if (!input || !*input)
		server = from_server;
	else
		GET_INT_ARG(server, input);
	RETURN_INT(is_server_connected(server));
}

BUILT_IN_FUNCTION(function_bmatch, input)
{
char *to = NULL;
char *p, *q, *m;
	GET_STR_ARG(to, input);
	p = q = m_strdup(input);
	while((m = next_arg(p, &p)))
	{
		if (wild_match(m, to))
		{
			char *ret = m_strdup(m);
			new_free(&q);
			return ret;
		}
	}
	new_free(&q);	
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_currchans, input)
{
	int server = -1;
	Window *blah = NULL;
	char *retval = NULL;

	if (input && *input)
		GET_INT_ARG(server, input)
	else
		server = -2;

	if (server == -1)
		server = from_server;

	while (traverse_all_windows(&blah))
 	{
		if (server != -2 && blah->server != server)
			continue;
		if (!blah->current_channel)
			continue;

		m_s3cat(&retval, space, "\"");
		malloc_strcat(&retval, ltoa(blah->server));
		malloc_strcat(&retval, space);
		malloc_strcat(&retval, blah->current_channel);
		malloc_strcat(&retval, "\"");
	}
	RETURN_MSTR(retval);
}

/*
 * Returns 1 if the specified function name is a built in function,
 * Returns 0 if not.
 */
BUILT_IN_FUNCTION(function_fnexist, input)
{
	char *fname;

	GET_STR_ARG(fname, input);
	RETURN_INT(func_exist(fname));
}

BUILT_IN_FUNCTION(function_cexist, input)
{
	char *fname;

	GET_STR_ARG(fname, input);
	RETURN_INT(command_exist(fname));
}

#if defined(HAVE_REGEX_H) && !defined(__EMX__) && !defined(WINNT)
/*
 * These are used as an interface to regex support.  Here's the plan:
 *
 * $regcomp(<pattern>) 
 *	will return an $encode()d string that is suitable for
 * 		assigning to a variable.  The return value of this
 *		function must at some point be passed to $regfree()!
 *
 * $regexec(<compiled> <string>)
 *	Will return "0" or "1" depending on whether or not the given string
 *		was matched by the previously compiled string.  The value
 *		for <compiled> must be a value previously returned by $regex().
 *		Failure to do this will result in undefined chaos.
 *
 * $regerror(<compiled>)
 *	Will return the error string for why the previous $regmatch() or 
 *		$regex() call failed.
 *
 * $regfree(<compiled>)
 *	Will free off any of the data that might be contained in <compiled>
 *		You MUST call $regfree() on any value that was previously
 *		returned by $regex(), or you will leak memory.  This is not
 *		my problem (tm).  It returns the FALSE value.
 */

static int last_error = 0; 		/* XXX */

BUILT_IN_FUNCTION(function_regcomp, input)
{
	regex_t preg = {0};
	if (input && *input)
	{
		last_error = regcomp(&preg, input, REG_EXTENDED | REG_ICASE | REG_NOSUB);
		return encode((char *)&preg, sizeof(regex_t));
	}
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_regexec, input)
{
	char *unsaved;
	regex_t *preg = NULL;
	int retval = -1;

	GET_STR_ARG(unsaved, input);
	if (input && *input)
	{
		char *p;
		if ((p = decode(unsaved)) && *p)
		{
			preg = (regex_t *)p;
			retval = regexec(preg, input, 0, NULL, 0);
			new_free((char **)&preg);
		}
	}
	RETURN_INT(retval);		/* DONT PASS FUNC CALL TO RETURN_INT */
}

BUILT_IN_FUNCTION(function_regerror, input)
{
	char *unsaved, *p;
	regex_t *preg = NULL;
	char error_buf[1024];
	int errcode;

	GET_INT_ARG(errcode, input);
	GET_STR_ARG(unsaved, input);
	*error_buf = 0;
	if ((p = decode(unsaved)) && *p)
	{
		preg = (regex_t *)p;

		if (errcode == -1)
			errcode = last_error;
		regerror(errcode, preg, error_buf, 1023);
		new_free((char **)&preg);
	}
	RETURN_STR(error_buf);
}

BUILT_IN_FUNCTION(function_regfree, input)
{
	char *unsaved, *p;
	regex_t *preg;

	GET_STR_ARG(unsaved, input);
	if ((p = decode(unsaved)) && *p)
	{
		preg = (regex_t *)p;
		regfree(preg);
		new_free((char **)&preg);
	}
	RETURN_EMPTY;
}
#else
BUILT_IN_FUNCTION(function_regexec, input)  { RETURN_EMPTY; }
BUILT_IN_FUNCTION(function_regcomp, input)  { RETURN_EMPTY; }
BUILT_IN_FUNCTION(function_regerror, input) { RETURN_STR("no regex support"); }
BUILT_IN_FUNCTION(function_regfree, input)  { RETURN_EMPTY; }
#endif

/*
 * Usage: $igmask(pattern)
 * Returns: All the ignore patterns that are matched by the pattern
 * Example: $igmask(*) returns your entire ignore list
 * Based on work contributed by pavlov
 */
BUILT_IN_FUNCTION(function_igmask, input)
{
	return get_ignores_by_pattern(input, 0);	/* DONT MALLOC THIS */
}

/*
 * Usage: $rigmask(pattern)
 * Returns: All the ignore patterns that would trigger on the given input
 * Example: $igmask(wc!bhauber@frenzy.com) would return a pattern like
 *		*!*@*frenzy.com  or like *!bhauber@*
 */
BUILT_IN_FUNCTION(function_rigmask, input)
{
	return get_ignores_by_pattern(input, 1);	/* DONT MALLOC THIS */
}

BUILT_IN_FUNCTION(function_count, input)
{
	char 	*str, *str2;
	int 	count = 0;

	GET_STR_ARG(str, input);
	str2 = input;

	for (;;)
	{
		if ((str2 = stristr(str2, str)))
			count++, str2++;
		else
			break;
	}

	RETURN_INT(count);
}

/*
 * Usage: $leftpc(COUNT TEXT)
 * Return the *longest* initial part of TEXT that includes COUNT printable
 * characters.
 */
BUILT_IN_FUNCTION(function_leftpc, word)
{
	u_char  **prepared = NULL;
	int	lines = 1;
	int	count;

	GET_INT_ARG(count, word);
	if (count <= 0 || !*word)
		RETURN_EMPTY;

	prepared = prepare_display(word, count, &lines, PREPARE_NOWRAP);
	RETURN_STR((char *)prepared[0]);
}
/*
 * $uname()
 * Returns system information.  Expandoes %a, %s, %r, %v, %m, and %n
 * correspond to uname(1)'s -asrvmn switches.  %% expands to a literal
 * '%'.  If no arguments are given, the function returns %s and %r (the
 * same information given in the client's ctcp version reply).
 *
 * Contributed by Matt Carothers (CrackBaby) on Dec 20, 1997
 */
BUILT_IN_FUNCTION(function_uname, input)
{
#ifndef HAVE_UNAME
	RETURN_STR("unknown");
#else
	struct utsname un;
	char 	tmp[BIG_BUFFER_SIZE+1];
	char	*ptr = tmp;
	size_t	size = BIG_BUFFER_SIZE;
	int 	i;
	int	len;

	if (uname(&un) == -1)
		RETURN_STR("unknown");

	if (!*input)
		input = "%s %r";

	*tmp = 0;
	for (i = 0, len = strlen(input); i < len; i++)
	{
		if (ptr - tmp >= size)
			break;

		if (input[i] == '%') 
		{
		    switch (input[++i]) 
		    {
			case 'm':	strlcpy(ptr, un.machine, size);
					break;
			case 'n':	strlcpy(ptr, un.nodename, size);
					break;
			case 'r':	strlcpy(ptr, un.release, size);
					break;
			case 's':	strlcpy(ptr, un.sysname, size);
					break;
			case 'v':	strlcpy(ptr, un.version, size);
					break;
			case 'a':	
					snprintf(ptr, size, "%s %s %s %s %s",
						un.sysname, un.nodename,
						un.release, un.version,
						un.machine);
					break;
			case '%':	strlcpy(ptr, "%", size);
		    }
		    ptr += strlen(ptr);
		}
		else
		    *ptr++ = input[i];
	}
	*ptr = 0;

	RETURN_STR(tmp);
#endif
}

BUILT_IN_FUNCTION(function_querywin, args)
{
	Window *w = NULL;

	while (traverse_all_windows(&w))
		if (w->query_nick && !my_stricmp(w->query_nick, args))
			RETURN_INT(w->refnum);

	RETURN_INT(-1);
}

BUILT_IN_FUNCTION(function_winrefs, args)
{
	Window *w = NULL;
	char *retval = NULL;

	while (traverse_all_windows(&w))
		m_s3cat(&retval, space, ltoa(w->refnum));

	return retval;		/* DONT MALLOC THIS! */
}


#define DOT (*host ? dot : empty_string)

BUILT_IN_FUNCTION(function_uhc, input)
{
	char 	*nick, *user, *host, *domain;
	int	ip;

	if (!input || !*input)
		RETURN_EMPTY;

	if (figure_out_address(input, &nick, &user, &host, &domain, &ip))
		RETURN_STR(input);
	else if (ip == 0)
		return m_sprintf("%s!%s@%s%s%s", nick, user, host, DOT, domain);
	else
		return m_sprintf("%s!%s@%s%s%s", nick, user, domain, DOT, host);
}

/*
 * Opposite of $uhc().  Breaks down a complete nick!user@host
 * into smaller components, based upon absolute wildcards.
 * Eg, *!user@host becomes user@host.  And *!*@*.host becomes *.host
 * Based on the contribution and further changes by Peter V. Evans 
 * (arc@anet-chi.com)
 */
BUILT_IN_FUNCTION(function_deuhc, input)
{
	char	*buf;
 
 	buf = LOCAL_COPY(input);
 
	if (!strchr(buf, '!') || !strchr(buf, '@'))
		RETURN_EMPTY;

	if (!strncmp(buf, "*!", 2))
	{
		buf += 2;
		if (!strncmp(buf, "*@", 2))
			buf += 2;
	}

	RETURN_STR(buf);
}


/*
 * $mask(type address)      OR
 * $mask(address type)
 * Returns address with a mask specified by type.
 *
 * $mask(1 nick!khaled@mardam.demon.co.uk)  returns *!*khaled@mardam.demon.co.uk
 * $mask(2 nick!khaled@mardam.demon.co.uk)  returns *!*@mardam.demon.co.uk
 *
 * The available types are:
 *
 * 0: *!user@host.domain
 * 1: *!*user@host.domain
 * 2: *!*@host.domain
 * 3: *!*user@*.domain
 * 4: *!*@*.domain
 * 5: nick!user@host.domain
 * 6: nick!*user@host.domain
 * 7: nick!*@host.domain
 * 8: nick!*user@*.domain
 * 9: nick!*@*.domain
 * 10: *!*@<number-masked-host>.domain		(matt)
 * 11: *!*user@<nember-masked-host>.domain	(matt)
 * 12: nick!*@<number-masked-host>.domain	(matt)
 * 13: nick!*user@<number-masked-host>.domain	(matt)
 */
BUILT_IN_FUNCTION(function_mask, args)
{
	char *	nuh;
	char *	nick = NULL;
	char *	user = NULL;
	char *	host = NULL;
	char *	domain = NULL;
	int   	which;
	char	stuff[BIG_BUFFER_SIZE + 1];
	int	ip;
	char	*first_arg;
	char	*ptr;

	first_arg = new_next_arg(args, &args);
	if (is_number(first_arg))
	{
		which = my_atol(first_arg);
		nuh = args;
	}
	else
	{
		nuh = first_arg;
		GET_INT_ARG(which, args);
	}

	if (figure_out_address(nuh, &nick, &user, &host, &domain, &ip))
		RETURN_EMPTY;

	/*
	 * Deal with ~jnelson@acronet.net for exapmle, and all sorts
	 * of other av2.9 breakage.
	 */
	if (strchr("~^-+=", *user))
		user++;

	/* Make sure 'user' isnt too long for a ban... */
	if (strlen(user) > 7)
	{
		user[7] = '*';
		user[8] = 0;
	}

#define USER (user == star ? empty_string : user)
#define MASK1(x, y) snprintf(stuff, BIG_BUFFER_SIZE, x, y); break;
#define MASK2(x, y, z) snprintf(stuff, BIG_BUFFER_SIZE, x, y, z); break;
#define MASK3(x, y, z, a) snprintf(stuff, BIG_BUFFER_SIZE, x, y, z, a); break;
#define MASK4(x, y, z, a, b) snprintf(stuff, BIG_BUFFER_SIZE, x, y, z, a, b); break;
#define MASK5(x, y, z, a, b, c) snprintf(stuff, BIG_BUFFER_SIZE, x, y, z, a, b, c); break;

	if (ip == 0) 
	switch (which)
	{
		case 0:  MASK4("*!%s@%s%s%s",         user, host, DOT, domain)
		case 1:  MASK4("*!*%s@%s%s%s",        USER, host, DOT, domain)
		case 2:  MASK3("*!*@%s%s%s",                host, DOT, domain)
		case 3:  MASK3("*!*%s@*%s%s",         USER,       DOT, domain)
		case 4:  MASK2("*!*@*%s%s",                       DOT, domain)
		case 5:  MASK5("%s!%s@%s%s%s",  nick, user, host, DOT, domain)
		case 6:  MASK5("%s!*%s@%s%s%s", nick, USER, host, DOT, domain)
		case 7:  MASK4("%s!*@%s%s%s",   nick,       host, DOT, domain)
		case 8:  MASK4("%s!*%s@*%s%s",  nick, USER,       DOT, domain)
		case 9:  MASK3("%s!*@*%s%s",    nick,             DOT, domain)
		case 10: mask_digits(&host);
			 MASK3("*!*@%s%s%s",                host, DOT, domain)
		case 11: mask_digits(&host);
			 MASK4("*!*%s@%s%s%s",        USER, host, DOT, domain)
		case 12: mask_digits(&host);
			 MASK4("%s!*@%s%s%s",   nick,       host, DOT, domain)
		case 13: mask_digits(&host);
			 MASK5("%s!*%s@%s%s%s", nick, USER, host, DOT, domain)
	}
	else
	switch (which)
	{
		case 0:  MASK3("*!%s@%s.%s",          user, domain, host)
		case 1:  MASK3("*!*%s@%s.%s",         USER, domain, host)
		case 2:  MASK2("*!*@%s.%s",                 domain, host)
		case 3:  MASK2("*!*%s@%s.*",          USER, domain)
		case 4:  MASK1("*!*@%s.*",                  domain)
		case 5:  MASK4("%s!%s@%s.%s",   nick, user, domain, host)
		case 6:  MASK4("%s!*%s@%s.%s",  nick, USER, domain, host)
		case 7:  MASK3("%s!*@%s.%s",    nick,       domain, host)
		case 8:  MASK3("%s!*%s@%s.*",   nick, USER, domain)
		case 9:  MASK2("%s!*@%s.*",     nick,       domain)
		case 10: MASK1("*!*@%s.*",                  domain)
		case 11: MASK2("*!*%s@%s.*",          USER, domain)
		case 12: MASK2("%s!*@%s.*",     nick,       domain)
		case 13: MASK3("%s!*%s@%s.*",   nick, USER, domain)
	}

	/* Clean up any non-printable chars */
	for (ptr = stuff; *ptr; ptr++)
		if (!isgraph((unsigned char)*ptr))
			*ptr = '?';

	RETURN_STR(stuff);
}

/*
 * $remws(word word word / word word word)
 * Returns the right hand side unchanged, except that any word on the right
 * hand side that is also found on the left hand side will be removed.
 * Space is *not* retained.  So there.
 */
BUILT_IN_FUNCTION(function_remws, word)
{
	char    *left = NULL,
		**lhs = NULL,
		*right = NULL,
		**rhs = NULL, 
		*booya = NULL;
	int	leftc, 	
		lefti,
		rightc,
		righti;
	int	found = 0;

	left = word;
	if (!(right = strchr(word,'/')))
	RETURN_EMPTY;

	*right++ = 0;
	leftc = splitw(left, &lhs);
	rightc = splitw(right, &rhs);

	for (righti = 0; righti < rightc; righti++)
	{
		found = 0;
		for (lefti = 0; lefti < leftc; lefti++)
		{
			if (rhs[righti] && lhs[lefti]  &&
			    !my_stricmp(lhs[lefti], rhs[righti]))
			{
				found = 1;
				break;
			}
		}
		if (!found)
			m_s3cat(&booya, space, rhs[righti]);
		rhs[righti] = NULL;
	}

	new_free((char **)&lhs);
	new_free((char **)&rhs);

	if (!booya)
		RETURN_EMPTY;

	return (booya);				/* DONT USE RETURN_STR HERE! */
}

BUILT_IN_FUNCTION(function_stripansicodes, input)
{
#if 0
	return strip_ansi(input);
#else
	RETURN_STR(stripansicodes(input));
#endif
}

BUILT_IN_FUNCTION(function_igtype, input)
{
	RETURN_STR(get_ignore_types_by_pattern(input));
}

BUILT_IN_FUNCTION(function_rigtype, input)
{
	 return get_ignore_patterns_by_type(input);	/* DONT MALLOC! */
}

BUILT_IN_FUNCTION(function_getuid, input)
{
	RETURN_INT(getuid());
}

BUILT_IN_FUNCTION(function_getgid, input)
{
	RETURN_INT(getgid());
}

BUILT_IN_FUNCTION(function_getlogin, input)
{
#ifdef HAVE_GETLOGIN
	RETURN_STR(getlogin());
#else
	RETURN_STR(getenv("LOGNAME"));
#endif
}

BUILT_IN_FUNCTION(function_getpgrp, input)
{
	RETURN_INT(getpgrp());
}

BUILT_IN_FUNCTION(function_iscurchan, input)
{
	char 	*chan;
	Window 	*w = NULL;

	GET_STR_ARG(chan, input);
	while (traverse_all_windows(&w))
	{
		/*
		 * Check to see if the channel specified is the current
		 * channel on *any* window for the current server.
		 */
		if (w->current_channel &&
			!my_stricmp(chan, w->current_channel) &&
			w->server == from_server)
				RETURN_INT(1);
	}

	RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_getcset, input)
{
char *var = NULL;
char *channel = NULL;
ChannelList *chan = NULL;
	GET_STR_ARG(var, input);
	if (input && *input)
		channel = next_arg(input, &input);
	else
		channel = get_current_channel_by_refnum(0);
	chan = lookup_channel(channel, from_server, 0);
	if (!channel || !chan)
		RETURN_EMPTY;
	if (input && *input)
		return get_cset(var, chan, input);
	else
		return get_cset(var, chan, NULL);
}

BUILT_IN_FUNCTION(function_getfsets, input)
{
extern char *get_fset(char *);
char *s = NULL;
char *ret = NULL;
	if (!input || !*input)
		return get_fset(NULL);
	while ((s = next_arg(input, &input)))
	{
		char *r;
		r = get_fset(s);
		m_s3cat(&ret, space, r);
		new_free(&r);
	}
	return ret;
}

BUILT_IN_FUNCTION(function_getsets, input)
{
extern char *get_set(char *);
char *s = NULL;
char *ret = NULL;
	if (!input || !*input)
		return get_set(NULL);
	while ((s = next_arg(input, &input)))
	{
		char *r;
		r = get_set(s);
		m_s3cat(&ret, space, r);
		new_free(&r);
	}
	return ret;
}

/*
 * $isnumber(number base)
 * returns the empty value if nothing is passed to it
 * returns 0 if the value passed is not a number
 * returns 1 if the value passed is a number.
 *
 * The "base" number is optional and should be prefixed by the 
 * 	character 'b'.  ala,   $isnumber(0x0F b16) for hexadecimal.
 *	the special base zero (b0) means to 'auto-detect'.  Base must
 *	be between 0 and 36, inclusive.  If not, it defaults to 0.
 */
BUILT_IN_FUNCTION(function_isnumber, input)
{
	int	base = 0;
	char	*endptr;
	char	*barg;
	char	*number = NULL;

	/*
	 * See if the first arg is the base
	 */
	barg = new_next_arg(input, &input);

	/*
	 * If it is, the number is the 2nd arg
	 */
	if (barg && *barg == 'b' && is_number(barg + 1))
	{
		GET_STR_ARG(number, input);
	}
	/*
	 * Otherwise, the first arg is the number,
	 * the 2nd arg probably is the base
	 */
	else
	{
		number = barg;
		barg = new_next_arg(input, &input);
	}

	/*
	 * If the user specified a base, parse it.
	 * Must be between 0 and 36.
	 */
	if (barg && *barg == 'b')
	{
		base = my_atol(barg + 1);
		if (base < 0 || base > 36)
			base = 0;
	}

	/*
	 * Must have specified a number, though.
	 */
	RETURN_IF_EMPTY(number);

	strtol(number, &endptr, base);
	if (*endptr == '.')
		strtol(endptr + 1, &endptr, base);

	if (*endptr)
		RETURN_INT(0);
	else
		RETURN_INT(1);
}

/*
 * $rest(index string)
 * Returns 'string' starting at the 'index'th character
 * Just like $restw() does for words.
 */
BUILT_IN_FUNCTION(function_rest, input)
{
	int	start = 1;

	if (my_atol(input))
		GET_INT_ARG(start, input);

	if (start <= 0)
		RETURN_STR(input);

	if (start >= strlen(input))
		RETURN_EMPTY;

	RETURN_STR(input + start);
}

/*
 * take a servername and return it's refnum or -1 
 */
BUILT_IN_FUNCTION(function_servref, input)
{
	char *servname = NULL;
	int i;

	if (input)
		servname = new_next_arg(input, &input);

	if (empty(servname))
		RETURN_INT(from_server);

	for (i = 0; i < server_list_size(); i++)
	{
		if (!my_stricmp(get_server_itsname(i), servname) ||
			!my_stricmp(get_server_name(i), servname))
		{
			RETURN_INT(i);
		}
	}

	RETURN_INT(-1);
}

/*
 * sd_(sd@pm3-25.netgate.net) needed the following function to return a 
 * set of flags from the userlist. we pass in a user@host and a channel
 * and that user's flags are returned.
 */

BUILT_IN_FUNCTION(function_getflags, input)
{
#ifdef WANT_USERLIST
	char *uh = NULL, *channel = NULL;
	register UserList *tmp;
	void *location = NULL;
	int size = -1;
		
	GET_STR_ARG(uh, input);
	GET_STR_ARG(channel, input);

	if ((tmp = find_bestmatch("*", uh, channel, NULL)))
		RETURN_STR(convert_flags_to_str(tmp->flags));
	for (tmp = next_userlist(NULL, &size, &location); tmp;  tmp = next_userlist(tmp, &size, &location))
	{
		register UserList *blah = NULL;
		if (!wild_match(uh, tmp->host) && !wild_match(tmp->host, uh))
			continue;

		uh = tmp->host;

		if ((blah = find_bestmatch("*", uh, channel, NULL)))
			RETURN_STR(convert_flags_to_str(blah->flags));
	}
#endif
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_numlines, input)
{
int count = 0;
unsigned char **lines = NULL;
char *s = NULL;
	if (input && *input)
	{
		int cols;
		s = LOCAL_COPY(input);
		if (current_window->screen)
			cols = current_window->screen->co;
		else
			cols = current_window->columns;
		for (lines = split_up_line(s, cols + 1); *lines; lines++)
			count++;
	}
	RETURN_INT(count);
}

BUILT_IN_FUNCTION(function_topic, input)
{
char *ch = NULL;
ChannelList *chan;
	if (input && *input)
		ch = next_arg(input, &input);
	if (!ch || !*ch)
		ch = get_current_channel_by_refnum(0);
	if (ch && *ch && (chan = lookup_channel(ch, from_server, 0)))
		RETURN_STR(chan->topic);
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_stripcrap, input)
{
	char	*how;
	int	mangle;
	char	*output;

	GET_STR_ARG(how, input);
	mangle = parse_mangle(how, 0, NULL);

	output = new_malloc(strlen(input) * 2 + 1);
	strcpy(output, input);
	mangle_line(output, mangle, strlen(input) * 2);
	return output;			/* DONT MALLOC THIS */
}



#if 0
BUILT_IN_FUNCTION(function_parse_and_return, str)
{
unsigned char buffer[3*BIG_BUFFER_SIZE+1];
register unsigned char *s;
char *copy = NULL;
char *tmpc = NULL;
int arg_flags;

	if (!str)
		return m_strdup(empty_string);

	memset(buffer, 0, BIG_BUFFER_SIZE);
	copy = tmpc = str;
        s = buffer;
	while (*tmpc)
	{
		if (*tmpc == '$')
		{
			char *new_str = NULL;
			tmpc++;
			in_cparse++;
			tmpc = alias_special_char(&new_str, tmpc, copy, NULL, &arg_flags);
			in_cparse--;
			if (new_str)
				strcat(s, new_str);
			new_free(&new_str);
			if (!tmpc) break;
			continue;
		} else
			*s = *tmpc;
		tmpc++; s++;
	}
	*s = 0;
	return m_strdup(buffer);
}
#endif

BUILT_IN_FUNCTION(function_long_to_comma, word)
{
long l = 0;
	if (word && *word)
		l = strtol(word, NULL, 10);
	RETURN_STR(longcomma(l));
}

BUILT_IN_FUNCTION(function_dccitem, word)
{
int dcc_item;
SocketList *s = NULL;
DCC_int *n1 = NULL;
int i = 0, found = 0;
char *str;
	if (!(str = next_arg(word, &word)))
		RETURN_EMPTY;
	if (*str == '#' && *(str+1))
	{
		dcc_item = my_atol(str+1);
		if (check_dcc_socket(dcc_item) && (s = get_socket(dcc_item)))
		{
			found++;	
			n1 = (DCC_int *) s->info;
			i = dcc_item;
		}
	}		
	else if (isdigit((unsigned char)*str))
	{
		dcc_item = my_atol(str);
		for (i = 0; i < get_max_fd()+1; i++)
		{
			if (check_dcc_socket(i) && (s = get_socket(i)))
			{
				n1 = (DCC_int *) s->info;
				if (n1 && (n1->dccnum == dcc_item))
				{
					found++;
					break;
				}
			}
		}
	}
	if (!found || !s || !n1)
		RETURN_EMPTY;
	return get_dcc_info(s, n1, i);
}

BUILT_IN_FUNCTION(function_winitem, word)
{
int win_item = -1;
char *str;
Window *window = NULL;
char none[] = "<none>";
char flags[10];
int j = 0;
#ifdef GUI
char fontinfo[100];
int winx, winy, wincx, wincy;
#endif

	if ((str = next_arg(word, &word)))
	{
		if (*str == '%' && *(str+1))
		{
			win_item = my_atol(str+1);
			window = get_window_by_refnum(win_item);
		} 
		else if (isdigit((unsigned char)*str))
		{
			if (!(win_item = my_atol(str)))
				window = get_window_by_refnum(win_item);
			else
			{
				while (traverse_all_windows(&window) && ++j != win_item) ;
				if (win_item > j)
					RETURN_EMPTY;
			}
		}
	}
	else
	{
		win_item = 0;
		window = get_window_by_refnum(win_item);
	}
	if (!window || win_item == -1)
		RETURN_EMPTY;
	memset(flags, 0, sizeof(flags));
	if (window->log)
		strcat(flags, "L");
#ifdef GUI
	fontinfo[0] = 0;
	gui_query_window_info(window->screen, fontinfo, &winx, &winy, &wincx, &wincy);
	return m_sprintf("%u %s %d %s %s %s %s %d %d %d %s %d %d %d %d %s \"%s\" \"%s\" %s %s %s",
#else
	return m_sprintf("%u %s %d %s %s %s %s %d %d %d %s %s %s %s %s %s \"%s\" \"%s\" %s %s %s",
#endif
		window->refnum, 
		window->name?window->name:none, 
		window->server, 
		window->current_channel?window->current_channel:none,
		window->query_nick?window->query_nick:none,
		window->waiting_channel?window->waiting_channel:none,
		(window->screen && window->screen->menu)?window->screen->menu:none, /* oirc menu name */
		window->visible, 
		window->screen ? window->screen->co : 0, 
		window->screen ? window->screen->li : 0,
#ifdef GUI
		fontinfo,
		winx,
		winy,
		wincx,
		wincy,
#else
		none, none, none, none, none, /* font windowx windowy window-width window-height */
#endif
		window->logfile?window->logfile:none, flags, 
		window->nicks?"n":empty_string, 
		none, none,
		bits_to_lastlog_level(window->window_level));
}

BUILT_IN_FUNCTION(function_ajoinitem, input)
{
extern AJoinList *ajoin_list;
AJoinList *new = NULL;
int ajl_num, count = 0;
char *ret = NULL;
	GET_INT_ARG(ajl_num, input);

	for (new = ajoin_list; new; new = new->next)
	{
		if ((ajl_num == -1) || (count == ajl_num))
		{
			m_s3cat(&ret, ",", new->name);
			m_s3cat(&ret, space, new->key ? new->key : "<none>");
		}
		if ((count == ajl_num))
			break;
		count++;
	}
	if (!ret)
		RETURN_EMPTY;
	return ret;
}

BUILT_IN_FUNCTION(function_servgroup, input)
{
int snum;
char *s;
	GET_INT_ARG(snum, input);
	s = get_server_network(snum);
	RETURN_STR(s ? s : empty_string);
}


#ifdef GUI
BUILT_IN_FUNCTION(function_mciapi, input)
{
char *firstarg;
static char szReturnString[BIG_BUFFER_SIZE] = "";
	if (input && *input)
	{
		GET_STR_ARG(firstarg, input)
		if (!my_stricmp(firstarg, "raw"))
			RETURN_INT(gui_send_mci_string(input, szReturnString));
		else if (!my_stricmp(firstarg, "result"))
			RETURN_STR(szReturnString);
		else if (!my_stricmp(firstarg, "playfile"))
		{
			char *fullfile;
			if (input && *input)
			{
				fullfile = expand_twiddle(input);
				gui_play_sound(fullfile);
				new_free(&fullfile);
			}
			RETURN_STR(empty_string);
		}
		else if (!my_stricmp(firstarg, "errstring"))
		{
			int errnum;
			char szErrorString[BIG_BUFFER_SIZE];
			*szErrorString = 0;
			GET_INT_ARG(errnum, input);
			gui_get_sound_error(errnum, szErrorString);
			RETURN_STR(szErrorString);
		}
	}
	RETURN_INT(1);
}

BUILT_IN_FUNCTION(function_menucontrol, input)
{
char *menuname, *what, *param;
int refnum = -1;

	GET_STR_ARG(menuname, input);
	GET_INT_ARG(refnum, input);
	GET_STR_ARG(what, input);
	GET_STR_ARG(param, input);

	if (menuname && *menuname && what && *what && param && *param)
	{
		if (refnum > -1 && gui_setmenuitem(menuname, refnum, what, param))
			RETURN_STR("1");
	}
	RETURN_STR(empty_string);
}

BUILT_IN_FUNCTION(function_lastclickline, input)
{
extern char *lastclicklinedata;
extern int lastclickcol;
int info=0,i,e,c,d;
char	retinfo[BIG_BUFFER_SIZE],

	newclicklinedata[BIG_BUFFER_SIZE + 1];

	*retinfo = 0;
	if (input && *input)
		GET_INT_ARG(info, input)
	else
		info=0;

	if (!lastclicklinedata)
		RETURN_EMPTY;

	/* Remove color codes if necessary */
	strncpy(newclicklinedata, stripansicodes(lastclicklinedata), BIG_BUFFER_SIZE);

	switch(info)
	{
		case 0:
			RETURN_STR(newclicklinedata);
			break;
		case 1:
			/* Color codes */
			RETURN_STR(lastclicklinedata);
			break;
		case 2:
			/* Color codes (replaced with ^O)....same as case 1 */
			RETURN_STR(lastclicklinedata);
			break;
		case 3:
			if (newclicklinedata[lastclickcol] != ' ')
			{
				i = lastclickcol;
				while (newclicklinedata[i] != ' ' && i > -1)
					i--;
				i++;
				e = lastclickcol;
				while (newclicklinedata[e] != ' ' && e < strlen(newclicklinedata))
					e++;
				if (newclicklinedata[i]=='[' || newclicklinedata[i]=='(' || newclicklinedata[i]=='<')
					i++;
				if (newclicklinedata[e-1]==']' || newclicklinedata[e-1]==')' || newclicklinedata[e-1]=='>')
					e--;
				for (c = i; c < e; c++)
					retinfo[(c-i)] = newclicklinedata[c];
				c++;
				retinfo[c-i] = 0;
				RETURN_STR(retinfo);
			}
			break;
		case 4:
			if (newclicklinedata[lastclickcol] != ' ')
			{
				i = lastclickcol;
				while (newclicklinedata[i] != ' ' && i > -1)
					i--;
				i++;
				e = lastclickcol;
				while (newclicklinedata[e] != ' ' && e < strlen(newclicklinedata))
					e++;
				if (newclicklinedata[i] == '[' || newclicklinedata[i]=='(' || newclicklinedata[i]=='<')
					i++;
				if (newclicklinedata[e-1] == ']' || newclicklinedata[e-1]==')' || newclicklinedata[e-1]=='>')
					e--;
			again:
				if (newclicklinedata[i] == '#' || newclicklinedata[i]=='@' || newclicklinedata[i]=='+' || newclicklinedata[i]=='%' || newclicklinedata[i]=='*')
				{
					i++;
					goto again;
				}
				d = i;
				while (newclicklinedata[d]!='@' && d < e)
					d++;
				for (c = i; c < d; c++)
					retinfo[(c-i)] = newclicklinedata[c];
				retinfo[(c-i)] = 0;
				RETURN_STR(retinfo);
			}
			break;
		default:
			RETURN_EMPTY;
			break;
	}
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_lastclickx, input)
{
extern int lastclickcol;
	RETURN_INT(lastclickcol);
}

BUILT_IN_FUNCTION(function_lastclicky, input)
{
extern int lastclickrow;
	RETURN_INT(lastclickrow);
}

BUILT_IN_FUNCTION(function_screensize, input)
{
char retbuffer[50];

	if (!my_stricmp(input, "cx"))
		sprintf(retbuffer, "%d", gui_screen_width());
	else if (!my_stricmp(input, "cy"))
		sprintf(retbuffer, "%d", gui_screen_height());
	else
		sprintf(retbuffer, "%d %d", gui_screen_width(), gui_screen_height());

	RETURN_STR(retbuffer);
}
#endif

/*
 * Date: Wed, 2 Sep 1998 18:20:34 -0500 (CDT)
 * From: CrackBaby <crack@feeding.frenzy.com>
 */
/* 
 * $getopt(<optopt var> <optarg var> <opt string> <argument list>)
 *
 * Processes a list of switches and args.  Returns one switch char each time
 * it's called, sets $<optopt var> to the char, and sets $<optarg var> to the
 * value of the next argument if the switch needs one.
 *
 * Syntax for <opt string> and <argument list> should be identical to
 * getopt(3).  A ':' in <opt string> is a required argument, and a "::" is an
 * optional one.  A '-' by itself in <argument list> is a null argument, and
 * switch parsing stops after a "--"
 *
 * If a switch requires an argument, but the argument is missing, $getopt()
 * returns a '-'  If a switch is given which isn't in the opt string, the
 * function returns a '!'  $<optopt var> is still set in both cases.
 *
 * Example usage:
 * while (option = getopt(optopt optarg "ab:c:" $*)) {
 *	switch ($option) {
 * 		(a) {echo * option "$optopt" used}
 * 		(b) {echo * option "$optopt" used - $optarg}
 * 		(c) {echo * option "$optopt" used - $optarg}
 * 		(!) {echo * option "$optopt" is an invalid option}
 * 		(-) {echo * option "$optopt" is missing an argument}
 *	}
 * }
 */
BUILT_IN_FUNCTION(function_getopt, input)
{
static 	char	switches	[INPUT_BUFFER_SIZE+1] = "",
		args		[INPUT_BUFFER_SIZE+1] = "",
		last_input	[INPUT_BUFFER_SIZE+1] = "",
		*sptr 	= switches,
		*aptr 	= args;
	char	*optopt_var, 
		*optarg_var,
		*optstr,
		*optptr,
		*tmp;
	char	extra_args	[INPUT_BUFFER_SIZE+1] = "";
	int	old_window_display = window_display,
		arg_flag = 0;

	if (strcmp(last_input, input)) 
	{
		strlcpy(last_input, input, INPUT_BUFFER_SIZE);
		
		*switches = 0;
		*args = 0;
		sptr = switches;
		aptr = args;
	}	

	GET_STR_ARG(optopt_var, input); 
	GET_STR_ARG(optarg_var, input); 
	GET_STR_ARG(optstr, input);

	if (!(optopt_var || optarg_var || optstr)) 
		RETURN_EMPTY;

	if (!*switches && !*args && strcmp(last_input, input))
	{
		/* Process each word in the input string */
		while ((tmp = next_arg(input, &input)))
		{
			/* Word is a switch or a group of switches */
			if (tmp[0] == '-' && tmp[1] && tmp[1] != '-' 
				&& !arg_flag)
			{
				/* Look at each char after the '-' */
				for (++tmp; *tmp; tmp++)
				{
					/* If the char is found in optstr
					   and doesn't need an argument,
					   it's added to the switches list.
					   switches are stored as "xy" where
					   x is the switch letter and y is
					   '_' normal switch
					   ':' switch with arg
					   '-' switch with missing arg
					   '!' unrecognized switch
					 */
					strlcat(switches, space, INPUT_BUFFER_SIZE);
					strncat(switches, tmp, 1);
					/* char is a valid switch */
					if ((optptr = strchr(optstr, *tmp)))
					{
						/* char requires an argument */
						if (*(optptr + 1) == ':')
						{
							/* -xfoo, argument is
							   "foo" */
							if (*(tmp + 1))
							{
								tmp++;
								strlcat(args, tmp, INPUT_BUFFER_SIZE);
								strlcat(args, space, INPUT_BUFFER_SIZE);
								strlcat(switches, ":", INPUT_BUFFER_SIZE);
								break;
							}
							/* Otherwise note that
							   the next word in 
							   the input is our
							   arg. */
							else if (*(optptr + 2) == ':')
								arg_flag = 2;
							else
								arg_flag = 1;
						}
						/* Switch needs no argument */
						else strlcat(switches, "_", INPUT_BUFFER_SIZE);
					}
					/* Switch is not recognized */
					else strlcat(switches, "!", INPUT_BUFFER_SIZE);
				}
			}
			else
			{
				/* Everything after a "--" is added to
				   extra_args */
				if (*tmp == '-' && *(tmp + 1) == '-')
				{
					tmp += 2;
					strlcat(extra_args, tmp, INPUT_BUFFER_SIZE);
					strlcat(extra_args, input, INPUT_BUFFER_SIZE);
					*input = 0;
				}
				/* A '-' by itself is a null arg */
				else if (*tmp == '-' && !*(tmp + 1))
				{
					if (arg_flag == 1)
						strlcat(switches, "-", INPUT_BUFFER_SIZE);
					else if (arg_flag == 2)
						strlcat(switches, "_", INPUT_BUFFER_SIZE);
					*tmp = 0;
					arg_flag = 0;
				}
			/* If the word doesn't start with a '-,' it must be
			   either the argument of a switch or just extra
			   info. */
				else if (arg_flag)
				{
				/* If arg_flag is positive, we've processes
				   a switch which requires an arg and we can
				   just tack the word onto the end of args[] */
					
					strlcat(args, tmp, INPUT_BUFFER_SIZE);
					strlcat(args, space, INPUT_BUFFER_SIZE);
					strlcat(switches, ":", INPUT_BUFFER_SIZE);
					arg_flag = 0;
				}
				else
				{
				/* If not, we'll put it aside and add it to
				   args[] after all the switches have been
				   looked at. */
					
					strlcat(extra_args, tmp, INPUT_BUFFER_SIZE);
					strlcat(extra_args, space, INPUT_BUFFER_SIZE);
				}
			}
		}
		/* If we're expecting an argument to a switch, but we've
		   reached the end of our input, the switch is missing its
		   arg. */
		if (arg_flag == 1)
			strlcat(switches, "-", INPUT_BUFFER_SIZE);
		else if (arg_flag == 2)
			strlcat(switches, "_", INPUT_BUFFER_SIZE);
		strlcat(args, extra_args, INPUT_BUFFER_SIZE);
	}

	window_display = 0;

	if ((tmp = next_arg(sptr, &sptr)))
	{
		switch (*(tmp + 1))
		{
			case '_':	
				*(tmp + 1) = 0;
				
				add_var_alias(optopt_var, tmp);
				add_var_alias(optarg_var, NULL);
		
				window_display = old_window_display;
				RETURN_STR(tmp);
			case ':':
				*(tmp + 1) = 0;
				
				add_var_alias(optopt_var, tmp);
				add_var_alias(optarg_var, next_arg(aptr, &aptr));
				window_display = old_window_display;
				RETURN_STR(tmp);
			case '-':
				*(tmp + 1) = 0;
				
				add_var_alias(optopt_var, tmp);
				add_var_alias(optarg_var, NULL);
				
				window_display = old_window_display;
				RETURN_STR("-");
			case '!':
				*(tmp + 1) = 0;
			
				add_var_alias(optopt_var, tmp);
				add_var_alias(optarg_var, NULL);
			
				window_display = old_window_display;
				RETURN_STR("!");
			default:
				/* This shouldn't happen */
				debugyell("*** getopt switch broken: %s", tmp);
				RETURN_EMPTY;
		}
	}
	else
	{
		add_var_alias(optopt_var, NULL);
		add_var_alias(optarg_var, aptr);
		window_display = old_window_display;

		*switches = 0;
		*args = 0;
		sptr = switches;
		aptr = args;

		RETURN_EMPTY;
	}
}

BUILT_IN_FUNCTION(function_isaway, input)
{
	int     refnum = -1;

	if (!*input)
		refnum = from_server;
	else
		GET_INT_ARG(refnum, input);

	if (get_server_away(refnum))
		RETURN_INT(1);

	RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_banwords, input)
{
char *ch = NULL;
char *ret = NULL;
extern WordKickList *ban_words;
	if (input && *input)
	{
		GET_STR_ARG(ch, input);
	}
	else
		ch = get_current_channel_by_refnum(0);
	if (ban_words)
	{
		WordKickList *ban;
		for (ban = ban_words; ban; ban = ban->next)
		{
			if (wild_match(ban->channel, ch))
				m_s3cat(&ret, space, ban->string);
		}
	}
	RETURN_STR(ret ? ret : empty_string);
}

/*
 * Creates a 32 bit hash value for a specified string
 * Usage: $hash_32bit(word length)
 *
 * "word" is the value to be hashed, and "length" is the number
 * of characters to hash.  If "length" is omitted or not 0 < length <= 64,
 * then it defaults to 20.
 *
 * This was contributed by srfrog (srfrog@lice.com).  I make no claims about
 * the usefulness of this hashing algorithm.
 *
 * The name was chosen just in case we decided to implement other hashing
 * algorithms for different sizes or types of return values.
 */
BUILT_IN_FUNCTION(function_hash_32bit, input)
{
	unsigned char *	u_word;
	char *		word;
	char		c;
	unsigned	bit_hi = 0;
	int		bit_low = 0;
	int		h_val;
	int		len;

	GET_STR_ARG(word, input)
	len = my_atol(input);

	if (len <= 0 || len > 64)
		len = 20;

	for (u_word = (u_char *)word; *u_word && len > 0; ++u_word, --len)
	{
		c = tolower(*u_word);
		bit_hi = (bit_hi << 1) + c;
		bit_low = (bit_low >> 1) + c;
	}
	h_val = ((bit_hi & 2047) << 3) + (bit_low & 0x7);
	RETURN_INT(h_val);
}

BUILT_IN_FUNCTION(function_filecomp, input)
{
#ifdef WANT_TABKEY
char *ret, *possible;
int count = 0; /* 6 */
	possible = next_arg(input, &input);
	ret = get_completions(3, possible, &count, NULL);
	return m_sprintf("%d %s", count, ret ? ret : empty_string);
#else
	RETURN_EMPTY;
#endif
}

BUILT_IN_FUNCTION(function_nickcomp, input)
{
#ifdef WANT_TABKEY
char *ret, *possible;
int count = 0;/* 4 */
	possible = next_arg(input, &input);
	ret = get_completions(7, possible, &count, NULL);
	return m_sprintf("%d %s", count, ret ? ret : empty_string);
#else
	RETURN_EMPTY;
#endif
}

BUILT_IN_FUNCTION(function_log, args)
{
extern FILE *irclog_fp;
	add_to_log(irclog_fp, time(NULL), args, logfile_line_mangler);
	if (irclog_fp)
		RETURN_INT(0);
	else
		RETURN_INT(-1);
}

/* 
 * KnghtBrd requested this. Returns the length of the longest word
 * in the word list.
 */
BUILT_IN_FUNCTION(function_maxlen, input)
{
	size_t	maxlen = 0;
	size_t	len;
	char	*arg;

	while (input && *input)
	{
		GET_STR_ARG(arg, input)

		if ((len = strlen(arg)) > maxlen)
			maxlen = len;
	}

	RETURN_INT(maxlen);
}


/*
 * KnghtBrd requested this.  It returns a string that is the leading
 * substring of ALL the words in the word list.   If no string is common
 * to all words in the word list, then the FALSE value is returned.
 *
 * I didn't give this a whole lot of thought about the optimal way to
 * do this, so if you have a better idea, let me know.  All i do here is
 * start at 1 character and walk my way longer and longer until i find a 
 * length that is not common, and then i stop there.  So the expense of this
 * algorithm is (M*N) where M is the number of words and N is the number of
 * characters they have in common when we're all done.
 */
BUILT_IN_FUNCTION(function_prefix, input)
{
	char	**words = NULL;
	int	numwords;
	int	max_len;
	int	len_index;
	int	word_index;
	char	*retval = NULL;

	RETURN_IF_EMPTY(input);

	numwords = splitw(input, &words);
	max_len = strlen(words[0]);

	for (len_index = 1; len_index <= max_len; len_index++)
	{
	    for (word_index = 1; word_index < numwords; word_index++)
	    {
		/*
		 * Basically, our stopping point is the first time we 
		 * see a string that does NOT share the first "len_index"
		 * characters with words[0] (which is chosen arbitrarily).
		 * As long as all words start with the same leading chars,
		 * we march along trying longer and longer substrings,
		 * until one of them doesnt work, and then we exit right
		 * there.
		 */
		if (my_strnicmp(words[0], words[word_index], len_index))
		{
			retval = new_malloc(len_index + 1);
			strmcpy(retval, words[0], len_index - 1);
			new_free((char **)&words);
			return retval;
		}
	    }
	}

	retval = m_strdup(words[0]);
	new_free((char **)&words);
	return retval;
}

/*
 * I added this so |rain| would stop asking me for it. ;-)  I reserve the
 * right to change the implementation of this in the future. 
 */
BUILT_IN_FUNCTION(function_functioncall, input)
{
	/*
	 * These two variables are supposed to be private inside alias.c
	 * Even though i export them here, it is EVIL EVIL EVIL to use
	 * them outside of alias.c.  So do not follow my example and extern
	 * these for your own use -- because who knows, one day i may make
	 * them static and your use of them will not be my fault! :p
	 */
	extern	int	wind_index;		 	/* XXXXX Ick */
	extern	int	last_function_call_level;	/* XXXXX Ick */

	/*
	 * If the last place we slapped down a FUNCTION_RETURN is in the
	 * current operating stack frame, then we're in a user-function
	 * call.  Otherwise we're not.  Pretty simple.
	 */
	if (last_function_call_level == wind_index)
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

/*
 * Usage: $indextoword(position text)
 *
 * This function returns the word-number (counting from 0) that contains
 * the 'position'th character in 'text' (counting from 0), such that the
 * following expression:
 *
 *		$word($indextoword($index(. $*) $*))
 *
 * would return the *entire word* in which the first '.' in $* is found.
 * If 'position' is -1, or past the end of the string, -1 is returned as
 * an error flag.  Note that this function can be used anywhere that a
 * word-number would be used, such as $chngw().  The empty value is returned
 * if a syntax error returns.
 *
 * DONT ASK ME to support 'position' less than 0 to indicate a position
 * from the end of the string.  Either that can be supported, or you can 
 * directly use $index() as the first argument; you can't do both.  I chose
 * the latter intentionally.  If you really want to calculate from the end of
 * your string, then just add your negative value to $strlen(string) and
 * pass that.
 */
BUILT_IN_FUNCTION(function_indextoword, input)
{
	size_t	pos;
	size_t	len;

	GET_INT_ARG(pos, input);
	if (pos < 0)
		RETURN_EMPTY;
	len = strlen(input);
	if (pos < 0 || pos >= len)
		RETURN_EMPTY;

	/* 
	 * XXX
	 * Using 'word_count' to do this is a really lazy cop-out, but it
	 * renders the desired effect and its pretty cheap.  Anyone want
	 * to bicker with me about it?
	 */
	/* Truncate the string if neccesary */
	if (pos + 1 < len)
		input[pos + 1] = 0;
	RETURN_INT(word_count(input));
}

BUILT_IN_FUNCTION(function_realpath, input)
{
#ifdef HAVE_REALPATH
	char	resolvedname[MAXPATHLEN];
	char	*retval;

	if ((retval = realpath(input, resolvedname)))
		RETURN_STR(resolvedname);
	else
		RETURN_EMPTY;
#else
	RETURN_STR(input);
#endif
}

BUILT_IN_FUNCTION(function_ttyname, input)
{
	RETURN_STR(ttyname(0));
}

/* 
 * $insert(num word string of text)
 * returns "string of text" such that "word" begins in the "num"th position
 * in the string ($index()-wise)
 * NOTE: Positions are numbered from 0
 * EX: $insert(3 baz foobarbooya) returns "foobazbarbooya"
 */
BUILT_IN_FUNCTION(function_insert, word)
{
	int	where;
	char *	inserted;
	char *	result;

	GET_INT_ARG(where, word);
	GET_STR_ARG(inserted, word);

	if (where <= 0)
		result = m_strdup(empty_string);
	else
		result = strext(word, word + where);

	m_3cat(&result, inserted, word + where);

	return result;				/* DONT USE RETURN_STR HERE! */
}

BUILT_IN_FUNCTION(function_stat, input)
{
	char *		filename;
	char 		retval[BIG_BUFFER_SIZE];
	struct stat	sb;

	GET_STR_ARG(filename, input);
	if (stat(filename, &sb) < 0)
		RETURN_EMPTY;

	snprintf(retval, BIG_BUFFER_SIZE,
		"%d %d %o %d %d %d %d %lu %lu %lu %ld %ld %ld",
		(int)	sb.st_dev,		/* device number */
		(int)	sb.st_ino,		/* Inode number */
		(int)	sb.st_mode,		/* Permissions */
		(int)	sb.st_nlink,		/* Hard links */
		(int)	sb.st_uid,		/* Owner UID */
		(int)	sb.st_gid,		/* Owner GID */
		(int)	sb.st_rdev,		/* Device type */
		(u_long)sb.st_size,		/* Size of file */
#ifdef __EMX__
		(u_long)sb.st_attr,            /* OS/2 Attributes */
		(u_long)sb.st_reserved,        /* OS/2 reserved */
#else
		(u_long)sb.st_blksize,		/* Size of each block */
		(u_long)sb.st_blocks,		/* Blocks used in file */
#endif
		(long)	sb.st_atime,		/* Last-access time */
		(long)	sb.st_mtime,		/* Last-modified time */
		(long)	sb.st_ctime		/* Last-change time */
			);

	RETURN_STR(retval);
}

BUILT_IN_FUNCTION(function_serverlag, input)
{
int server = from_server;
	if (input && *input)
		server = my_atol(input);		
	RETURN_INT(get_server_lag(server));
}

BUILT_IN_FUNCTION(function_strchar, input)
{
char *chr, *ret = NULL;
char *(*func)(const char *, int);
	GET_STR_ARG(chr, input);
	if (!my_stricmp("STRCHR", fn))
		func = strchr;
	else
		func = strrchr;
	if (chr && *chr)
		ret = func(input, *chr);
	RETURN_STR(ret);
}

BUILT_IN_FUNCTION(function_isdisplaying, input)
{
	RETURN_INT(window_display);
}

BUILT_IN_FUNCTION(function_getcap, input)
{
	char *	type;

	GET_STR_ARG(type, input);
	if (!my_stricmp(type, "TERM"))
	{
		char *	retval;
		char *	term = NULL;
		int	querytype = 0;
		int	mangle = 1;
		
		GET_STR_ARG(term, input);
		if (*input)
			GET_INT_ARG(querytype, input);
		if (*input)
			GET_INT_ARG(mangle, input);

		if (!term)			/* This seems spurious */
			RETURN_EMPTY;
		
		if ((retval = get_term_capability(term, querytype, mangle)))
			RETURN_STR(retval);

		RETURN_EMPTY;
	}

	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_getset, input)
{
	RETURN_MSTR(make_string_var(input));
}

BUILT_IN_FUNCTION(function_builtin, input)
{
	RETURN_MSTR(built_in_alias (*input, NULL));
}

BUILT_IN_FUNCTION(function_ipv6, input)
{
#ifdef IPV6
	RETURN_INT(1);
#else
	RETURN_INT(0);
#endif
}
