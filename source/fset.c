#include "irc.h"
static char cvsrevision[] = "$Id: fset.c 144 2011-10-13 12:45:27Z keaston $";
CVS_REVISION(fset_c)
#include "struct.h"

#include "alist.h"
#include "ircaux.h"
#include "screen.h"
#include "hook.h"
#include "output.h"
#include "misc.h"
#include "list.h"
#include "vars.h"
#define MAIN_SOURCE
#include "modval.h"

extern void reinit_status (Window *, char *, int);

typedef struct _fset_number_ {
	struct _fset_number_ *next;
	int	numeric;
	char	*format;
} FsetNumber;

static FsetNumber *numeric_fset = NULL;

IrcVariable fset_array[] = 
{
	{ "ACTION",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ACTION_AR",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ACTION_CHANNEL",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ACTION_OTHER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ACTION_OTHER_AR",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ACTION_USER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ACTION_USER_AR",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ALIAS",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ASSIGN",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "AWAY",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BACK",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BANS",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BANS_FOOTER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BANS_HEADER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BITCH",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BOT",			0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BOT_FOOTER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BOT_HEADER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "BWALL",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},

	{ "CHANNEL_SIGNOFF",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CHANNEL_URL",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	
	{ "COMPLETE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CONNECT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CSET",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_CLOAK",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_CLOAK_FUNC",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_CLOAK_FUNC_USER",0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_CLOAK_UNKNOWN",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_CLOAK_UNKNOWN_USER", 0,STR_TYPE_VAR,0,NULL, NULL, 0, 0},
	{ "CTCP_CLOAK_USER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_FUNC",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_FUNC_USER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_REPLY",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_UNKNOWN",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_UNKNOWN_USER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "CTCP_USER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "DCC",			0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "DCC_CHAT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "DCC_CONNECT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "DCC_ERROR",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "DCC_LOST",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "DCC_REQUEST",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "DESYNC",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	
	{ "DISCONNECT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "EBANS",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "EBANS_FOOTER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "EBANS_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ENCRYPTED_NOTICE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "ENCRYPTED_PRIVMSG",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "FLOOD",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "FRIEND_JOIN",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "HELP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "HOOK",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},

	{ "IGNORE_INVITE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "IGNORE_MSG",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "IGNORE_MSG_AWAY",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "IGNORE_NOTICE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "IGNORE_WALL",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},

	{ "INVITE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "INVITE_USER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "JOIN",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "KICK",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "KICK_USER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "KILL",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "LASTLOG",		0,STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{ "LEAVE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "LINKS",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "LIST",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "MAIL",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "MODE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "MODE_CHANNEL",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "MSG",			0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "MSGCOUNT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "MSGLOG",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "MSG_GROUP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_BANNER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_BOT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_FOOTER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_FRIEND",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_IRCOP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_NICK",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_NICK_BOT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_NICK_FRIEND",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_NICK_ME",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_NICK_SHIT",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_NONOP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_OP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_SHIT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_USER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_USER_CHANOP",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_USER_IRCOP",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_USER_VOICE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NAMES_VOICE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},

	{ "NETADD",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NETJOIN",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NETSPLIT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NETSPLIT_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NICKNAME",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NICKNAME_OTHER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NICKNAME_USER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NICK_AUTO",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NICK_COMP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NICK_MSG",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NONICK",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NOTE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NOTICE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NOTIFY_OFF",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NOTIFY_ON",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NOTIFY_SIGNOFF",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "NOTIFY_SIGNON",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "OPER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "OV",			0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PASTE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PUBLIC",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PUBLIC_AR",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PUBLIC_MSG",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PUBLIC_MSG_AR",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PUBLIC_NOTICE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PUBLIC_NOTICE_AR",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PUBLIC_OTHER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "PUBLIC_OTHER_AR",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "REL",			0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "RELM",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "RELN",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "RELS",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "RELSM",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "RELSN",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_ACTION",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_ACTION_OTHER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_AWAY",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_CTCP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_DCC_CHAT",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_ENCRYPTED_MSG",	0,STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{ "SEND_ENCRYPTED_NOTICE",0,STR_TYPE_VAR,0, NULL, NULL, 0, 0},
	{ "SEND_MSG",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_NOTICE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_PUBLIC",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SEND_PUBLIC_OTHER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_MSG1",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_MSG1_FROM",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_MSG2",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_MSG2_FROM",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},

	{ "SERVER_NOTICE_BOT",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_BOT1",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_BOT_ALARM",0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_CLIENT_CONNECT",0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_CLIENT_EXIT",0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_CLIENT_INVALID",0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_CLIENT_TERM",0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_FAKE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_GLINE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_KILL",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_KILL_LOCAL",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_KLINE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_NICKC",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_OPER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_REHASH",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_STATS",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_TRAFFIC_HIGH",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_TRAFFIC_NORM",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SERVER_NOTICE_UNAUTH",0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},

	{ "SET",			0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SET_NOVALUE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SHITLIST",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SHITLIST_FOOTER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SHITLIST_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SIGNOFF",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SILENCE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "SMODE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "STATUS",		0,STR_TYPE_VAR,	0, NULL, reinit_status, 0, 0},
	{ "STATUS1",		0,STR_TYPE_VAR,	0, NULL, reinit_status, 0, 0},
	{ "STATUS2",		0,STR_TYPE_VAR,	0, NULL, reinit_status, 0, 0},
	{ "STATUS3",		0,STR_TYPE_VAR,	0, NULL, reinit_status, 0, 0},
	{ "TIMER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "TOPIC",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "TOPIC_CHANGE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "TOPIC_CHANGE_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "TOPIC_SETBY",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "TOPIC_UNSET",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "TRACE_OPER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "TRACE_SERVER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "TRACE_USER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USAGE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERLIST",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERLIST_FOOTER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERLIST_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERMODE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERMODE_OTHER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERS",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERS_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERS_SHIT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "USERS_TITLE",		0,STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{ "USERS_USER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "VERSION",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WALL",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WALLOP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WALL_AR",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WATCH_SIGNON",	0,STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{ "WATCH_SIGNOFF",	0,STR_TYPE_VAR, 0, NULL, NULL, 0, 0},
	{ "WHO",			0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_ACTUALLY",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_ADMIN",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_AWAY",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_BOT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_CALLERID",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_CHANNELS",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_FOOTER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_FRIEND",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_HELP",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_IDLE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_NAME",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_NICK",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_OPER",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_REGISTER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_SECURE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_SERVER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_SERVICE",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_SHIT",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOIS_SIGNON",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOLEFT_FOOTER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOLEFT_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOLEFT_USER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOWAS_HEADER",	0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WHOWAS_NICK",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WIDELIST",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "WINDOW_SET",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ "XTERM_TITLE",		0,STR_TYPE_VAR,	0, NULL, NULL, 0, 0},
	{ NULL,				0, 0,		0, NULL, NULL, 0, 0}
};

typedef struct FSet_struc
{
	IrcVariable **list;
	int max;
	int max_set;
	alist_func func;
	unsigned long hash;
} FSet;

int strncasecmp(const char *s1, const char *s2, size_t n);
       
FSet ext_fset_list = { NULL, 0, 0, strncasecmp, 0};

void delete_all_ext_fset(void)
{
IrcVariable *ptr;
int index;

	for (index = 0; index < ext_fset_list.max; index++)
	{
		ptr = ext_fset_list.list[index];
		new_free(&ptr->name);
		new_free(&ptr->string);
		new_free(&ptr);
	}
	new_free(&ext_fset_list.list);
	ext_fset_list.max = 0;
	ext_fset_list.max_set = 0;
}

void add_new_fset(char *name, char *args)
{
	if (args && *args)
	{
		IrcVariable *tmp = NULL;
		int cnt, loc;
		tmp = (IrcVariable *)find_array_item((Array *)&ext_fset_list, name, &cnt, &loc);
		if (!tmp || cnt >= 0)
		{
			tmp = new_malloc(sizeof(IrcVariable));
			tmp->name = m_strdup(name);
			tmp->type = STR_TYPE_VAR;
			add_to_array((Array *)&ext_fset_list, (Array_item *)tmp);
		}
		malloc_strcpy(&tmp->string, args);
	}
	else 
	{
		IrcVariable *tmp;
		if ((tmp = (IrcVariable *)remove_from_array((Array *)&ext_fset_list, name)))
		{
			new_free(&tmp->name);
			new_free(&tmp->string);
			new_free(&tmp);
		}
	}
}

IrcVariable *find_ext_fset_var(char *name)
{
IrcVariable *tmp = NULL;
int loc, cnt;
	tmp = (IrcVariable *)find_array_item((Array *)&ext_fset_list, name, &cnt, &loc);
	if (tmp && cnt < 0)
		return tmp;
	return NULL;
}



char *BX_fget_string_var(enum FSET_TYPES var)
{
IrcVariable *tmp = NULL;
	if ((tmp = find_ext_fset_var(fset_array[var].name)))
		return tmp->string;
	return (fset_array[var].string);
}

void BX_fset_string_var(enum FSET_TYPES var, char *value)
{
	if (value && *value)
		malloc_strcpy(&(fset_array[var].string), value);
	else
		new_free(&(fset_array[var].string));
}

static int find_fset_variable(IrcVariable *array, char *org_name, int *cnt)
{
	IrcVariable *v, *first;
	int     len, var_index;
	char    *name = NULL;

	name = LOCAL_COPY(org_name);
	upper(name);
	len = strlen(name);
	var_index = 0;
	for (first = array; first->name; first++, var_index++) 
	{
		if (strncmp(name, first->name, len) == 0) 
		{
			*cnt = 1;
			break;
		}
	}
	if (first->name) 
	{
		if (strlen(first->name) != len) 
		{
			v = first;
			for (v++; v->name; v++, (*cnt)++) 
			{
				if (strncmp(name, v->name, len) != 0)
					break;
			}
		}
		return (var_index);
	}
	else 
	{
		*cnt = 0;
		return (-1);
	}
}


static void set_fset_var_value(int var_index, char *name, char *value)
{
	IrcVariable *var = NULL;
	if (name)
		var = find_ext_fset_var(name);
	if (!var)
		var = &(fset_array[var_index]);
	
	switch (var->type) 
	{
	case STR_TYPE_VAR:
		{
			if (value)
			{
				if (*value)
					malloc_strcpy(&(var->string), value);
				else
				{
					put_it("%s", convert_output_format(fget_string_var(FORMAT_SET_FSET), "%s %s", var->name, var->string?var->string:empty_string));
					return;
				}
			} else
				new_free(&(var->string));
			if (var->func)
				(var->func) (current_window, var->string, 0);
			say("Value of %s set to %s", var->name, var->string ?
				var->string : "<EMPTY>");
		}
		break;
	default:
		say("FSET_type not supported");
	}
}

static inline void fset_variable_case1(char *name, int var_index, char *args)
{
	set_fset_var_value(var_index, name, args);
}

static inline void fset_variable_casedef(char *name, int cnt, int var_index, char *args)
{
FsetNumber *tmp;
	for (cnt += var_index; var_index < cnt; var_index++)
		set_fset_var_value(var_index, NULL, args);
	if (!is_number(name))
		return;
	for (tmp = numeric_fset; tmp; tmp = tmp->next)
		if (my_atol(name) == tmp->numeric)
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SET_FSET), "%d %s", tmp->numeric, tmp->format));
}

static inline void fset_variable_noargs(char *name)
{
int var_index = 0;
FsetNumber *tmp;
	for (var_index = 0; var_index < NUMBER_OF_FSET; var_index++)
		set_fset_var_value(var_index, NULL, empty_string);
	for (var_index = 0; var_index < ext_fset_list.max; var_index++)
		set_fset_var_value(var_index, (*ext_fset_list.list[var_index]).name, empty_string);
	for (tmp = numeric_fset; tmp; tmp = tmp->next)
		put_it("%s", convert_output_format(fget_string_var(FORMAT_SET_FSET), "%d %s", tmp->numeric, tmp->format));
}

BUILT_IN_COMMAND(fset_variable)
{
	char    *var;
	char 	*name = NULL;
	int     cnt, var_index;
	int	hook = 0, remove = 0;
	
	if ((var = next_arg(args, &args)) != NULL) 
	{
		if (*var == '-') 
		{
			var++;
			remove = 1;
		}
		else if (*var == '+')
		{
			var++;
			add_new_fset(var, args);
			return;
		}
		if (!my_strnicmp(var, "FORMAT_", 7))
			var = var + 7;
		if (is_number(var))
		{
			add_numeric_fset(var, remove, args, 1);
			return;
		}
		else if (remove)
			args = NULL;
		var_index = find_fset_variable(fset_array, var, &cnt);

		if ((cnt >= 0) || !(fset_array[var_index].int_flags & VIF_PENDING))
			hook = 1;

		if (cnt < 0)
			fset_array[var_index].int_flags |= VIF_PENDING;

		if (hook)
			hook = do_hook(SET_LIST, "%s %s", var, args ? args : empty_string);
		else
			hook = 1;

		if (cnt < 0)
			fset_array[var_index].int_flags &= ~VIF_PENDING;
		
		if (hook)
		{
			switch (cnt) 
			{
				case 0:
					say("No such variable \"%s\"", var);
					return;
				case 1:
					fset_variable_case1(name, var_index, args);
					return;
				default:
					say("%s is ambiguous", var);
					fset_variable_casedef(name, cnt, var_index, empty_string);
					return;
			}
		}
	} else
		fset_variable_noargs(name);
}


void create_fsets(Window *win, int ansi)
{
#ifdef VAR_DEBUG
 	int i;
	
	for (i = 1; i < NUMBER_OF_FSET - 1; i++)
		if (strcmp(fset_array[i-1].name, fset_array[i].name) >= 0)
			ircpanic("Variable [%d] (%s) is out of order.", i, fset_array[i].name);
#endif

	if (sizeof fset_array / sizeof fset_array[0] != NUMBER_OF_FSET + 1)
		ircpanic("Inconsistent fset_array - %d != %d", 
			sizeof fset_array / sizeof fset_array[0], NUMBER_OF_FSET + 1);
	
	add_numeric_fset("381", 0, DEFAULT_FORMAT_381_FSET, 0);
	add_numeric_fset("391", 0, DEFAULT_FORMAT_391_FSET, 0);
	add_numeric_fset("471", 0, DEFAULT_FORMAT_471_FSET, 0);
	add_numeric_fset("473", 0, DEFAULT_FORMAT_473_FSET, 0);
	add_numeric_fset("474", 0, DEFAULT_FORMAT_474_FSET, 0);
	add_numeric_fset("475", 0, DEFAULT_FORMAT_475_FSET, 0);
	add_numeric_fset("476", 0, DEFAULT_FORMAT_476_FSET, 0);

	fset_string_var(FORMAT_ACTION_FSET, DEFAULT_FORMAT_ACTION_FSET);
	fset_string_var(FORMAT_ACTION_AR_FSET, DEFAULT_FORMAT_ACTION_AR_FSET);
	fset_string_var(FORMAT_ACTION_CHANNEL_FSET, DEFAULT_FORMAT_ACTION_CHANNEL_FSET);
	fset_string_var(FORMAT_ACTION_OTHER_FSET, DEFAULT_FORMAT_ACTION_OTHER_FSET);
	fset_string_var(FORMAT_ACTION_OTHER_AR_FSET, DEFAULT_FORMAT_ACTION_OTHER_AR_FSET);
	fset_string_var(FORMAT_ACTION_USER_FSET, DEFAULT_FORMAT_ACTION_USER_FSET);
	fset_string_var(FORMAT_ACTION_USER_AR_FSET, DEFAULT_FORMAT_ACTION_USER_AR_FSET);
	fset_string_var(FORMAT_ACTION_FSET, DEFAULT_FORMAT_ACTION_FSET);
	fset_string_var(FORMAT_ACTION_AR_FSET, DEFAULT_FORMAT_ACTION_AR_FSET);
	fset_string_var(FORMAT_ACTION_CHANNEL_FSET, DEFAULT_FORMAT_ACTION_CHANNEL_FSET);
	fset_string_var(FORMAT_ACTION_OTHER_FSET, DEFAULT_FORMAT_ACTION_OTHER_FSET);
	fset_string_var(FORMAT_ACTION_OTHER_AR_FSET, DEFAULT_FORMAT_ACTION_OTHER_AR_FSET);
	fset_string_var(FORMAT_ACTION_USER_FSET, DEFAULT_FORMAT_ACTION_USER_FSET);
	fset_string_var(FORMAT_ACTION_USER_AR_FSET, DEFAULT_FORMAT_ACTION_USER_AR_FSET);
	fset_string_var(FORMAT_ALIAS_FSET, DEFAULT_FORMAT_ALIAS_FSET);
	fset_string_var(FORMAT_ASSIGN_FSET, DEFAULT_FORMAT_ASSIGN_FSET);
	fset_string_var(FORMAT_AWAY_FSET, DEFAULT_FORMAT_AWAY_FSET);
	fset_string_var(FORMAT_BACK_FSET, DEFAULT_FORMAT_BACK_FSET);

	fset_string_var(FORMAT_BANS_HEADER_FSET, DEFAULT_FORMAT_BANS_HEADER_FSET);
	fset_string_var(FORMAT_BANS_FSET, DEFAULT_FORMAT_BANS_FSET);
#if defined(DEFAULT_FORMAT_BANS_FOOTER_FSET)
	fset_string_var(FORMAT_BANS_FOOTER_FSET, DEFAULT_FORMAT_BANS_FOOTER_FSET);
#endif

	fset_string_var(FORMAT_BITCH_FSET, DEFAULT_FORMAT_BITCH_FSET);
	fset_string_var(FORMAT_BOT_HEADER_FSET, DEFAULT_FORMAT_BOT_HEADER_FSET);
	fset_string_var(FORMAT_BOT_FOOTER_FSET, DEFAULT_FORMAT_BOT_FOOTER_FSET);
	fset_string_var(FORMAT_BOT_FSET, DEFAULT_FORMAT_BOT_FSET);
	fset_string_var(FORMAT_BWALL_FSET, DEFAULT_FORMAT_BWALL_FSET);
	fset_string_var(FORMAT_CHANNEL_SIGNOFF_FSET, DEFAULT_FORMAT_CHANNEL_SIGNOFF_FSET);
	fset_string_var(FORMAT_CHANNEL_URL_FSET, DEFAULT_FORMAT_CHANNEL_URL_FSET);
	fset_string_var(FORMAT_CONNECT_FSET, DEFAULT_FORMAT_CONNECT_FSET);
	fset_string_var(FORMAT_COMPLETE_FSET, DEFAULT_FORMAT_COMPLETE_FSET);
	fset_string_var(FORMAT_CTCP_FSET, DEFAULT_FORMAT_CTCP_FSET);
	fset_string_var(FORMAT_CTCP_CLOAK_FSET, DEFAULT_FORMAT_CTCP_CLOAK_FSET);
	fset_string_var(FORMAT_CTCP_CLOAK_FUNC_FSET, DEFAULT_FORMAT_CTCP_CLOAK_FUNC_FSET);
	fset_string_var(FORMAT_CTCP_CLOAK_FUNC_USER_FSET, DEFAULT_FORMAT_CTCP_CLOAK_FUNC_USER_FSET);
	fset_string_var(FORMAT_CTCP_CLOAK_UNKNOWN_FSET, DEFAULT_FORMAT_CTCP_CLOAK_UNKNOWN_FSET);
	fset_string_var(FORMAT_CTCP_CLOAK_UNKNOWN_USER_FSET, DEFAULT_FORMAT_CTCP_CLOAK_UNKNOWN_USER_FSET);
	fset_string_var(FORMAT_CTCP_CLOAK_USER_FSET, DEFAULT_FORMAT_CTCP_CLOAK_USER_FSET);
	fset_string_var(FORMAT_CTCP_FUNC_FSET, DEFAULT_FORMAT_CTCP_FUNC_FSET);
	fset_string_var(FORMAT_CTCP_FUNC_USER_FSET, DEFAULT_FORMAT_CTCP_FUNC_USER_FSET);
	fset_string_var(FORMAT_CTCP_UNKNOWN_FSET, DEFAULT_FORMAT_CTCP_UNKNOWN_FSET);
	fset_string_var(FORMAT_CTCP_UNKNOWN_USER_FSET, DEFAULT_FORMAT_CTCP_UNKNOWN_USER_FSET);
	fset_string_var(FORMAT_CTCP_USER_FSET, DEFAULT_FORMAT_CTCP_USER_FSET);
	fset_string_var(FORMAT_CTCP_REPLY_FSET, DEFAULT_FORMAT_CTCP_REPLY_FSET);
	fset_string_var(FORMAT_DCC_CHAT_FSET, DEFAULT_FORMAT_DCC_CHAT_FSET);
	fset_string_var(FORMAT_DCC_CONNECT_FSET, DEFAULT_FORMAT_DCC_CONNECT_FSET);
	fset_string_var(FORMAT_DCC_ERROR_FSET, DEFAULT_FORMAT_DCC_ERROR_FSET);
	fset_string_var(FORMAT_DCC_LOST_FSET, DEFAULT_FORMAT_DCC_LOST_FSET);
	fset_string_var(FORMAT_DCC_REQUEST_FSET, DEFAULT_FORMAT_DCC_REQUEST_FSET);
	fset_string_var(FORMAT_DESYNC_FSET, DEFAULT_FORMAT_DESYNC_FSET);
	fset_string_var(FORMAT_DISCONNECT_FSET, DEFAULT_FORMAT_DISCONNECT_FSET);
	fset_string_var(FORMAT_ENCRYPTED_NOTICE_FSET, DEFAULT_FORMAT_ENCRYPTED_NOTICE_FSET);
	fset_string_var(FORMAT_ENCRYPTED_PRIVMSG_FSET, DEFAULT_FORMAT_ENCRYPTED_PRIVMSG_FSET);
	fset_string_var(FORMAT_FLOOD_FSET, DEFAULT_FORMAT_FLOOD_FSET);
	fset_string_var(FORMAT_FRIEND_JOIN_FSET, DEFAULT_FORMAT_FRIEND_JOIN_FSET);
	fset_string_var(FORMAT_HELP_FSET, DEFAULT_FORMAT_HELP_FSET);
	fset_string_var(FORMAT_HOOK_FSET, DEFAULT_FORMAT_HOOK_FSET);
	fset_string_var(FORMAT_INVITE_FSET, DEFAULT_FORMAT_INVITE_FSET);
	fset_string_var(FORMAT_INVITE_USER_FSET, DEFAULT_FORMAT_INVITE_USER_FSET);
	fset_string_var(FORMAT_JOIN_FSET, DEFAULT_FORMAT_JOIN_FSET);
	fset_string_var(FORMAT_KICK_FSET, DEFAULT_FORMAT_KICK_FSET);
	fset_string_var(FORMAT_KICK_USER_FSET, DEFAULT_FORMAT_KICK_USER_FSET);
	fset_string_var(FORMAT_KILL_FSET, DEFAULT_FORMAT_KILL_FSET);
	fset_string_var(FORMAT_LEAVE_FSET, DEFAULT_FORMAT_LEAVE_FSET);
	fset_string_var(FORMAT_LINKS_FSET, DEFAULT_FORMAT_LINKS_FSET);
	fset_string_var(FORMAT_LINKS_FSET, DEFAULT_FORMAT_LINKS_FSET);
	fset_string_var(FORMAT_LIST_FSET, DEFAULT_FORMAT_LIST_FSET);
	fset_string_var(FORMAT_MAIL_FSET, DEFAULT_FORMAT_MAIL_FSET);
	fset_string_var(FORMAT_MSGCOUNT_FSET, DEFAULT_FORMAT_MSGCOUNT_FSET);
	fset_string_var(FORMAT_MSGLOG_FSET, DEFAULT_FORMAT_MSGLOG_FSET);
	fset_string_var(FORMAT_MODE_FSET, DEFAULT_FORMAT_MODE_FSET);
	fset_string_var(FORMAT_SMODE_FSET, DEFAULT_FORMAT_SMODE_FSET);
	fset_string_var(FORMAT_MODE_CHANNEL_FSET, DEFAULT_FORMAT_MODE_CHANNEL_FSET);
	fset_string_var(FORMAT_MSG_FSET, DEFAULT_FORMAT_MSG_FSET);
	fset_string_var(FORMAT_OPER_FSET, DEFAULT_FORMAT_OPER_FSET);
	fset_string_var(FORMAT_IGNORE_INVITE_FSET, DEFAULT_FORMAT_IGNORE_INVITE_FSET);
	fset_string_var(FORMAT_IGNORE_MSG_FSET, DEFAULT_FORMAT_IGNORE_MSG_FSET);
	fset_string_var(FORMAT_IGNORE_MSG_AWAY_FSET, DEFAULT_FORMAT_IGNORE_MSG_AWAY_FSET);
	fset_string_var(FORMAT_IGNORE_NOTICE_FSET, DEFAULT_FORMAT_IGNORE_NOTICE_FSET);
	fset_string_var(FORMAT_IGNORE_WALL_FSET, DEFAULT_FORMAT_IGNORE_WALL_FSET);
	fset_string_var(FORMAT_MSG_GROUP_FSET, DEFAULT_FORMAT_MSG_GROUP_FSET);
	fset_string_var(FORMAT_NAMES_FSET, DEFAULT_FORMAT_NAMES_FSET);
	fset_string_var(FORMAT_NAMES_BOT_FSET, DEFAULT_FORMAT_NAMES_BOT_FSET);
	fset_string_var(FORMAT_NAMES_FRIEND_FSET, DEFAULT_FORMAT_NAMES_FRIEND_FSET);
	fset_string_var(FORMAT_NAMES_NICK_FSET, DEFAULT_FORMAT_NAMES_NICK_FSET);
	fset_string_var(FORMAT_NAMES_NICK_BOT_FSET, DEFAULT_FORMAT_NAMES_NICK_BOT_FSET);
	fset_string_var(FORMAT_NAMES_NICK_FRIEND_FSET, DEFAULT_FORMAT_NAMES_NICK_FRIEND_FSET);
	fset_string_var(FORMAT_NAMES_NICK_ME_FSET, DEFAULT_FORMAT_NAMES_NICK_ME_FSET);
	fset_string_var(FORMAT_NAMES_NICK_SHIT_FSET, DEFAULT_FORMAT_NAMES_NICK_SHIT_FSET);
	fset_string_var(FORMAT_NAMES_NONOP_FSET, DEFAULT_FORMAT_NAMES_NONOP_FSET);
	fset_string_var(FORMAT_NAMES_OP_FSET, DEFAULT_FORMAT_NAMES_OP_FSET);
	fset_string_var(FORMAT_NAMES_IRCOP_FSET, DEFAULT_FORMAT_NAMES_IRCOP_FSET);
	fset_string_var(FORMAT_NAMES_SHIT_FSET, DEFAULT_FORMAT_NAMES_SHIT_FSET);
	fset_string_var(FORMAT_NAMES_USER_FSET, DEFAULT_FORMAT_NAMES_USER_FSET);
	fset_string_var(FORMAT_NAMES_USER_CHANOP_FSET, DEFAULT_FORMAT_NAMES_USER_CHANOP_FSET);
	fset_string_var(FORMAT_NAMES_USER_IRCOP_FSET, DEFAULT_FORMAT_NAMES_USER_IRCOP_FSET);
	fset_string_var(FORMAT_NAMES_USER_VOICE_FSET, DEFAULT_FORMAT_NAMES_USER_VOICE_FSET);
	fset_string_var(FORMAT_NAMES_VOICE_FSET, DEFAULT_FORMAT_NAMES_VOICE_FSET);
	fset_string_var(FORMAT_NETADD_FSET, DEFAULT_FORMAT_NETADD_FSET);
	fset_string_var(FORMAT_NETJOIN_FSET, DEFAULT_FORMAT_NETJOIN_FSET);
	fset_string_var(FORMAT_NETSPLIT_FSET, DEFAULT_FORMAT_NETSPLIT_FSET);
	fset_string_var(FORMAT_NICKNAME_FSET, DEFAULT_FORMAT_NICKNAME_FSET);
	fset_string_var(FORMAT_NICKNAME_OTHER_FSET, DEFAULT_FORMAT_NICKNAME_OTHER_FSET);
	fset_string_var(FORMAT_NICKNAME_USER_FSET, DEFAULT_FORMAT_NICKNAME_USER_FSET);
	fset_string_var(FORMAT_NONICK_FSET, DEFAULT_FORMAT_NONICK_FSET);
	fset_string_var(FORMAT_NOTE_FSET, DEFAULT_FORMAT_NOTE_FSET);
	fset_string_var(FORMAT_NOTICE_FSET, DEFAULT_FORMAT_NOTICE_FSET);
	fset_string_var(FORMAT_REL_FSET, DEFAULT_FORMAT_REL_FSET);
	fset_string_var(FORMAT_RELN_FSET, DEFAULT_FORMAT_RELN_FSET);
	fset_string_var(FORMAT_RELM_FSET, DEFAULT_FORMAT_RELM_FSET);
	fset_string_var(FORMAT_RELSN_FSET, DEFAULT_FORMAT_RELSN_FSET);
	fset_string_var(FORMAT_RELS_FSET, DEFAULT_FORMAT_RELS_FSET);
	fset_string_var(FORMAT_RELSM_FSET, DEFAULT_FORMAT_RELSM_FSET);
	fset_string_var(FORMAT_NOTIFY_SIGNOFF_FSET, DEFAULT_FORMAT_NOTIFY_SIGNOFF_FSET);
	fset_string_var(FORMAT_NOTIFY_SIGNON_FSET, DEFAULT_FORMAT_NOTIFY_SIGNON_FSET);
	fset_string_var(FORMAT_PASTE_FSET, DEFAULT_FORMAT_PASTE_FSET);
	fset_string_var(FORMAT_PUBLIC_FSET, DEFAULT_FORMAT_PUBLIC_FSET);
	fset_string_var(FORMAT_PUBLIC_AR_FSET, DEFAULT_FORMAT_PUBLIC_AR_FSET);
	fset_string_var(FORMAT_PUBLIC_MSG_FSET, DEFAULT_FORMAT_PUBLIC_MSG_FSET);
	fset_string_var(FORMAT_PUBLIC_MSG_AR_FSET, DEFAULT_FORMAT_PUBLIC_MSG_AR_FSET);
	fset_string_var(FORMAT_PUBLIC_NOTICE_FSET, DEFAULT_FORMAT_PUBLIC_NOTICE_FSET);
	fset_string_var(FORMAT_PUBLIC_NOTICE_AR_FSET, DEFAULT_FORMAT_PUBLIC_NOTICE_AR_FSET);
	fset_string_var(FORMAT_PUBLIC_OTHER_FSET, DEFAULT_FORMAT_PUBLIC_OTHER_FSET);
	fset_string_var(FORMAT_PUBLIC_OTHER_AR_FSET, DEFAULT_FORMAT_PUBLIC_OTHER_AR_FSET);
	fset_string_var(FORMAT_SEND_ACTION_FSET, DEFAULT_FORMAT_SEND_ACTION_FSET);
	fset_string_var(FORMAT_SEND_ACTION_OTHER_FSET, DEFAULT_FORMAT_SEND_ACTION_OTHER_FSET);
	fset_string_var(FORMAT_SEND_ACTION_FSET, DEFAULT_FORMAT_SEND_ACTION_FSET);
	fset_string_var(FORMAT_SEND_ACTION_OTHER_FSET, DEFAULT_FORMAT_SEND_ACTION_OTHER_FSET);
	fset_string_var(FORMAT_SEND_AWAY_FSET, DEFAULT_FORMAT_SEND_AWAY_FSET);
	fset_string_var(FORMAT_SEND_CTCP_FSET, DEFAULT_FORMAT_SEND_CTCP_FSET);
	fset_string_var(FORMAT_SEND_DCC_CHAT_FSET, DEFAULT_FORMAT_SEND_DCC_CHAT_FSET);
	fset_string_var(FORMAT_SEND_ENCRYPTED_NOTICE_FSET, DEFAULT_FORMAT_SEND_ENCRYPTED_NOTICE_FSET);
	fset_string_var(FORMAT_SEND_ENCRYPTED_MSG_FSET, DEFAULT_FORMAT_SEND_ENCRYPTED_MSG_FSET);

	fset_string_var(FORMAT_SEND_MSG_FSET, DEFAULT_FORMAT_SEND_MSG_FSET);
	fset_string_var(FORMAT_SEND_NOTICE_FSET, DEFAULT_FORMAT_SEND_NOTICE_FSET);
	fset_string_var(FORMAT_SEND_PUBLIC_FSET, DEFAULT_FORMAT_SEND_PUBLIC_FSET);
	fset_string_var(FORMAT_SEND_PUBLIC_OTHER_FSET, DEFAULT_FORMAT_SEND_PUBLIC_OTHER_FSET);
	fset_string_var(FORMAT_SERVER_FSET, DEFAULT_FORMAT_SERVER_FSET);
	fset_string_var(FORMAT_SERVER_MSG1_FSET, DEFAULT_FORMAT_SERVER_MSG1_FSET);
	fset_string_var(FORMAT_SERVER_MSG1_FROM_FSET, DEFAULT_FORMAT_SERVER_MSG1_FROM_FSET);
	fset_string_var(FORMAT_SERVER_MSG2_FSET, DEFAULT_FORMAT_SERVER_MSG2_FSET);
	fset_string_var(FORMAT_SERVER_MSG2_FROM_FSET, DEFAULT_FORMAT_SERVER_MSG2_FROM_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_FSET, DEFAULT_FORMAT_SERVER_NOTICE_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_BOT_FSET, DEFAULT_FORMAT_SERVER_NOTICE_BOT_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_BOT1_FSET, DEFAULT_FORMAT_SERVER_NOTICE_BOT1_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_BOT_ALARM_FSET, DEFAULT_FORMAT_SERVER_NOTICE_BOT_ALARM_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_CLIENT_CONNECT_FSET, DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_CONNECT_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_CLIENT_EXIT_FSET, DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_EXIT_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_CLIENT_INVALID_FSET, DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_INVALID_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_CLIENT_TERM_FSET, DEFAULT_FORMAT_SERVER_NOTICE_CLIENT_TERM_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_FAKE_FSET, DEFAULT_FORMAT_SERVER_NOTICE_FAKE_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_KILL_FSET, DEFAULT_FORMAT_SERVER_NOTICE_KILL_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_KILL_LOCAL_FSET, DEFAULT_FORMAT_SERVER_NOTICE_KILL_LOCAL_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_KLINE_FSET, DEFAULT_FORMAT_SERVER_NOTICE_KLINE_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_GLINE_FSET, DEFAULT_FORMAT_SERVER_NOTICE_GLINE_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_NICK_COLLISION_FSET, DEFAULT_FORMAT_SERVER_NOTICE_NICK_COLLISION_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_OPER_FSET, DEFAULT_FORMAT_SERVER_NOTICE_OPER_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_REHASH_FSET, DEFAULT_FORMAT_SERVER_NOTICE_REHASH_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_STATS_FSET, DEFAULT_FORMAT_SERVER_NOTICE_STATS_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_TRAFFIC_HIGH_FSET, DEFAULT_FORMAT_SERVER_NOTICE_TRAFFIC_HIGH_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_TRAFFIC_NORM_FSET, DEFAULT_FORMAT_SERVER_NOTICE_TRAFFIC_NORM_FSET);
	fset_string_var(FORMAT_SERVER_NOTICE_UNAUTH_FSET, DEFAULT_FORMAT_SERVER_NOTICE_UNAUTH_FSET);
	fset_string_var(FORMAT_SET_FSET, DEFAULT_FORMAT_SET_FSET);
	fset_string_var(FORMAT_CSET_FSET, DEFAULT_FORMAT_CSET_FSET);
	fset_string_var(FORMAT_SET_NOVALUE_FSET, DEFAULT_FORMAT_SET_NOVALUE_FSET);
	fset_string_var(FORMAT_SHITLIST_FSET, DEFAULT_FORMAT_SHITLIST_FSET);
	fset_string_var(FORMAT_SHITLIST_FOOTER_FSET, DEFAULT_FORMAT_SHITLIST_FOOTER_FSET);
	fset_string_var(FORMAT_SHITLIST_HEADER_FSET, DEFAULT_FORMAT_SHITLIST_HEADER_FSET);
	fset_string_var(FORMAT_SIGNOFF_FSET, DEFAULT_FORMAT_SIGNOFF_FSET);
	fset_string_var(FORMAT_SILENCE_FSET, DEFAULT_FORMAT_SILENCE_FSET);
	fset_string_var(FORMAT_TRACE_OPER_FSET, DEFAULT_FORMAT_TRACE_OPER_FSET);
	fset_string_var(FORMAT_TRACE_SERVER_FSET, DEFAULT_FORMAT_TRACE_SERVER_FSET);
	fset_string_var(FORMAT_TRACE_USER_FSET, DEFAULT_FORMAT_TRACE_USER_FSET);
	fset_string_var(FORMAT_TIMER_FSET, DEFAULT_FORMAT_TIMER_FSET);
	fset_string_var(FORMAT_TOPIC_FSET, DEFAULT_FORMAT_TOPIC_FSET);
	fset_string_var(FORMAT_TOPIC_CHANGE_FSET, DEFAULT_FORMAT_TOPIC_CHANGE_FSET);
	fset_string_var(FORMAT_TOPIC_SETBY_FSET, DEFAULT_FORMAT_TOPIC_SETBY_FSET);
	fset_string_var(FORMAT_TOPIC_UNSET_FSET, DEFAULT_FORMAT_TOPIC_UNSET_FSET);
	fset_string_var(FORMAT_USAGE_FSET, DEFAULT_FORMAT_USAGE_FSET);
	fset_string_var(FORMAT_USERMODE_FSET, DEFAULT_FORMAT_USERMODE_FSET);
	fset_string_var(FORMAT_USERMODE_OTHER_FSET, DEFAULT_FORMAT_USERMODE_OTHER_FSET);
	fset_string_var(FORMAT_USERLIST_FSET, DEFAULT_FORMAT_USERLIST_FSET);
	fset_string_var(FORMAT_USERLIST_FOOTER_FSET, DEFAULT_FORMAT_USERLIST_FOOTER_FSET);
	fset_string_var(FORMAT_USERLIST_HEADER_FSET, DEFAULT_FORMAT_USERLIST_HEADER_FSET);
	fset_string_var(FORMAT_USERS_FSET, DEFAULT_FORMAT_USERS_FSET);
	fset_string_var(FORMAT_USERS_USER_FSET, DEFAULT_FORMAT_USERS_USER_FSET);
	fset_string_var(FORMAT_USERS_TITLE_FSET, DEFAULT_FORMAT_USERS_TITLE_FSET);
	fset_string_var(FORMAT_USERS_SHIT_FSET, DEFAULT_FORMAT_USERS_SHIT_FSET);
	fset_string_var(FORMAT_USERS_HEADER_FSET, DEFAULT_FORMAT_USERS_HEADER_FSET);
	fset_string_var(FORMAT_VERSION_FSET, DEFAULT_FORMAT_VERSION_FSET);
	fset_string_var(FORMAT_WALL_FSET, DEFAULT_FORMAT_WALL_FSET);
	fset_string_var(FORMAT_WALL_AR_FSET, DEFAULT_FORMAT_WALL_AR_FSET);
	fset_string_var(FORMAT_WALLOP_FSET, DEFAULT_FORMAT_WALLOP_FSET);
	fset_string_var(FORMAT_WHO_FSET, DEFAULT_FORMAT_WHO_FSET);
	fset_string_var(FORMAT_WHOIS_ACTUALLY_FSET, DEFAULT_FORMAT_WHOIS_ACTUALLY_FSET);
	fset_string_var(FORMAT_WHOIS_AWAY_FSET, DEFAULT_FORMAT_WHOIS_AWAY_FSET);
	fset_string_var(FORMAT_WHOIS_BOT_FSET, DEFAULT_FORMAT_WHOIS_BOT_FSET);
	fset_string_var(FORMAT_WHOIS_CALLERID_FSET, DEFAULT_FORMAT_WHOIS_CALLERID_FSET);
	fset_string_var(FORMAT_WHOIS_CHANNELS_FSET, DEFAULT_FORMAT_WHOIS_CHANNELS_FSET);
	fset_string_var(FORMAT_WHOIS_FRIEND_FSET, DEFAULT_FORMAT_WHOIS_FRIEND_FSET);
	fset_string_var(FORMAT_WHOIS_HEADER_FSET, DEFAULT_FORMAT_WHOIS_HEADER_FSET);
	fset_string_var(FORMAT_WHOIS_IDLE_FSET, DEFAULT_FORMAT_WHOIS_IDLE_FSET);
	fset_string_var(FORMAT_WHOIS_SHIT_FSET, DEFAULT_FORMAT_WHOIS_SHIT_FSET);
	fset_string_var(FORMAT_WHOIS_SIGNON_FSET, DEFAULT_FORMAT_WHOIS_SIGNON_FSET);
	fset_string_var(FORMAT_WHOIS_NAME_FSET, DEFAULT_FORMAT_WHOIS_NAME_FSET);
	fset_string_var(FORMAT_WHOIS_NICK_FSET, DEFAULT_FORMAT_WHOIS_NICK_FSET);
	fset_string_var(FORMAT_WHOIS_OPER_FSET, DEFAULT_FORMAT_WHOIS_OPER_FSET);
	fset_string_var(FORMAT_WHOIS_SECURE_FSET, DEFAULT_FORMAT_WHOIS_SECURE_FSET);
	fset_string_var(FORMAT_WHOIS_SERVER_FSET, DEFAULT_FORMAT_WHOIS_SERVER_FSET);
	fset_string_var(FORMAT_WHOLEFT_HEADER_FSET, DEFAULT_FORMAT_WHOLEFT_HEADER_FSET);
	fset_string_var(FORMAT_WHOLEFT_USER_FSET, DEFAULT_FORMAT_WHOLEFT_USER_FSET);
	fset_string_var(FORMAT_WHOWAS_HEADER_FSET, DEFAULT_FORMAT_WHOWAS_HEADER_FSET);
	fset_string_var(FORMAT_WHOWAS_NICK_FSET, DEFAULT_FORMAT_WHOWAS_NICK_FSET);
	fset_string_var(FORMAT_WHOIS_ADMIN_FSET, DEFAULT_FORMAT_WHOIS_ADMIN_FSET);
	fset_string_var(FORMAT_WHOIS_SERVICE_FSET, DEFAULT_FORMAT_WHOIS_SERVICE_FSET);
	fset_string_var(FORMAT_WHOIS_HELP_FSET, DEFAULT_FORMAT_WHOIS_HELP_FSET);
	fset_string_var(FORMAT_WHOIS_REGISTER_FSET, DEFAULT_FORMAT_WHOIS_REGISTER_FSET);
	fset_string_var(FORMAT_WIDELIST_FSET, DEFAULT_FORMAT_WIDELIST_FSET);
	fset_string_var(FORMAT_WINDOW_SET_FSET, DEFAULT_FORMAT_WINDOW_SET_FSET);
	fset_string_var(FORMAT_NICK_MSG_FSET, DEFAULT_FORMAT_NICK_MSG_FSET);
	fset_string_var(FORMAT_NICK_COMP_FSET, DEFAULT_FORMAT_NICK_COMP_FSET);
	fset_string_var(FORMAT_NICK_AUTO_FSET, DEFAULT_FORMAT_NICK_AUTO_FSET);
	fset_string_var(FORMAT_STATUS_FSET, DEFAULT_FORMAT_STATUS_FSET);
	fset_string_var(FORMAT_STATUS1_FSET, DEFAULT_FORMAT_STATUS1_FSET);
	fset_string_var(FORMAT_STATUS2_FSET, DEFAULT_FORMAT_STATUS2_FSET);
	fset_string_var(FORMAT_STATUS3_FSET, DEFAULT_FORMAT_STATUS3_FSET);
	fset_string_var(FORMAT_NOTIFY_OFF_FSET, DEFAULT_FORMAT_NOTIFY_OFF_FSET);
	fset_string_var(FORMAT_NOTIFY_ON_FSET, DEFAULT_FORMAT_NOTIFY_ON_FSET);
	fset_string_var(FORMAT_OV_FSET, DEFAULT_FORMAT_OV_FSET);

	fset_string_var(FORMAT_WHOIS_FOOTER_FSET, NULL);
	fset_string_var(FORMAT_XTERM_TITLE_FSET, NULL);
	fset_string_var(FORMAT_DCC_FSET, NULL);
	fset_string_var(FORMAT_NAMES_FOOTER_FSET, NULL);
	fset_string_var(FORMAT_NETSPLIT_HEADER_FSET, NULL);
	fset_string_var(FORMAT_TOPIC_CHANGE_HEADER_FSET, NULL);
	fset_string_var(FORMAT_WHOLEFT_FOOTER_FSET, NULL);
	fset_string_var(FORMAT_LASTLOG_FSET, DEFAULT_FORMAT_LASTLOG_FSET);	
	fset_string_var(FORMAT_EBANS_FSET, DEFAULT_FORMAT_EBANS_FSET);
	fset_string_var(FORMAT_EBANS_HEADER_FSET, DEFAULT_FORMAT_EBANS_HEADER_FSET);

	fset_string_var(FORMAT_WATCH_SIGNOFF_FSET, DEFAULT_FORMAT_WATCH_SIGNOFF_FSET);
	fset_string_var(FORMAT_WATCH_SIGNON_FSET, DEFAULT_FORMAT_WATCH_SIGNON_FSET);
	
#if defined(DEFAULT_FORMAT_EBANS_FOOTER_FSET)
	fset_string_var(FORMAT_EBANS_FOOTER_FSET, DEFAULT_FORMAT_EBANS_FOOTER_FSET);
#endif
}

int save_formats(FILE *outfile)
{
char thefile[BIG_BUFFER_SIZE+1];
char *p;
int i;
int count = 0;
FsetNumber *tmp;

#if defined(__EMX__) || defined(WINNT)
	sprintf(thefile, "%s/%s.fmt", get_string_var(CTOOLZ_DIR_VAR), version);
#else
	sprintf(thefile, "%s/%s.formats", get_string_var(CTOOLZ_DIR_VAR), version);
#endif
	p = expand_twiddle(thefile);
	outfile = fopen(p, "w");
	if (!outfile)
	{
		bitchsay("Cannot open file %s for saving!", thefile);
		new_free(&p);
		return 1;
	}
	for (i = 0; i < NUMBER_OF_FSET; i++)	
	{

		if (fset_array[i].string)
			fprintf(outfile, "FSET %s %s\n", fset_array[i].name, fset_array[i].string);
		else
			fprintf(outfile, "FSET -%s\n", fset_array[i].name);
	}
	count = NUMBER_OF_FSET;
	for (tmp = numeric_fset; tmp; tmp = tmp->next) {
		fprintf(outfile, "FSET %d %s\n", tmp->numeric, tmp->format);
		count++;
	}
	
	fclose(outfile);
	bitchsay("Saved %d formats to %s", count, thefile);
	new_free(&p);
	return 0;
}

void clear_fset(void)
{
int i;
	for (i = 0; i < NUMBER_OF_FSET; i++)	
	{
		if (fset_array[i].string)
			new_free(&fset_array[i].string);
	}
}

char *get_all_fset(void)
{
	int i;
	char *ret = NULL;
	IrcVariable *ptr;
	FsetNumber *tmp = numeric_fset;
	for (i = 0; i < NUMBER_OF_FSET; i++)
		m_s3cat(&ret, space, fset_array[i].name);
	for (i = 0; i < ext_fset_list.max; i++)
	{
		ptr = ext_fset_list.list[i];
		m_s3cat(&ret, space, ptr->name);
	}
	for (tmp = numeric_fset; tmp; tmp = tmp->next)
		m_s3cat(&ret, space, ltoa(tmp->numeric));
	return ret;
}

char *get_fset(char *str)
{
	int i;
	char *ret = NULL;
	IrcVariable *ptr;
	FsetNumber *tmp;
	if (!str || !*str)
		return get_all_fset();
	for (i = 0; i < NUMBER_OF_FSET; i++)
		if (wild_match(str, fset_array[i].name))
			m_s3cat(&ret, space, fset_array[i].name);
	for (i = 0; i < ext_fset_list.max; i++)
	{
		ptr = ext_fset_list.list[i];
		if (wild_match(str, ptr->name))
			m_s3cat(&ret, space, ptr->name);
	}
	for (tmp = numeric_fset; tmp; tmp = tmp->next)
		if (wild_match(str, ltoa(tmp->numeric)))
			m_s3cat(&ret, space, ltoa(tmp->numeric));
	return ret ? ret : m_strdup(empty_string);
}

IrcVariable *fget_var_address(char *var_name)
{
	IrcVariable *var = NULL;
	int	cnt,
		msv_index;
	char	*tmp_var = alloca(strlen(var_name)+1);
	strcpy(tmp_var, var_name);
	upper(tmp_var);
	if ((var = find_ext_fset_var(tmp_var)))
		return var;
	if ((find_fixed_array_item (fset_array, sizeof(IrcVariable), NUMBER_OF_FSET, tmp_var, &cnt, &msv_index) == NULL))
		return NULL;
	if (cnt >= 0)
		return NULL;
	return &fset_array[msv_index];
}

char	*make_fstring_var(const char *var_name)
{
	IrcVariable *var = NULL;
	int	cnt,
		msv_index;
	char	*ret = NULL;
	char	*tmp_var;

	tmp_var = LOCAL_COPY(var_name);
	upper(tmp_var);
	if ((var = find_ext_fset_var(tmp_var)))
		return m_strdup(var->string);

	if (!strncmp(tmp_var, "FORMAT_", 7))
		tmp_var += 7;
	if ((find_fixed_array_item (fset_array, sizeof(IrcVariable), NUMBER_OF_FSET, tmp_var, &cnt, &msv_index) == NULL))
	{
		if ((ret = find_numeric_fset(my_atol(tmp_var))))
			return m_strdup(ret);
		return NULL;
	}
	if (cnt >= 0)
		return NULL;
	switch (fset_array[msv_index].type)
	{
		case STR_TYPE_VAR:
			ret = m_strdup(fset_array[msv_index].string);
			break;
		case INT_TYPE_VAR:
			ret = m_strdup(ltoa(fset_array[msv_index].integer));
			break;
		case BOOL_TYPE_VAR:
			ret = m_strdup(var_settings[fset_array[msv_index].integer]);
			break;
		case CHAR_TYPE_VAR:
			ret = m_dupchar(fset_array[msv_index].integer);
			break;
	}
	return ret;
}

#ifdef WANT_TCL
char *fset_rem_str(ClientData *cd, Tcl_Interp *intp, char *name1, char *name2, int flags)
{
char *s;
IrcVariable *n;
	n = (IrcVariable *)cd;
	if ((s = Tcl_GetVar(intp, name1, TCL_GLOBAL_ONLY)))
	{
		malloc_strcpy(&n->string, s);
	}
	return NULL;
}

void add_tcl_fset(Tcl_Interp *irp)
{
char varname[180];
int i = 0;
	for(i = 0; fset_array[i].name; i++)
	{
		int type_of = -1;
		switch(fset_array[i].type)
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
		strncpy(varname, fset_array[i].name, 80);
		lower(varname);
		type_of |= TCL_LINK_READ_ONLY;
		Tcl_LinkVar(irp, varname, 
			(fset_array[i].type == STR_TYPE_VAR) ? 
				(char *)&fset_array[i].string : 
				(char *)&fset_array[i].integer,
			type_of);
#if 0
		if (fset_array[i].type == STR_TYPE_VAR)
		{
			Tcl_TraceVar(irp, varname, TCL_TRACE_WRITES,
				(Tcl_VarTraceProc *)fset_rem_str, (ClientData)&fset_array[i]);
		}
#endif
	}
}
#endif


#ifdef GUI
IrcVariable *return_fset_var(int nummer)
{
   return &fset_array[nummer];
}
#endif

/* This is a horrible hack so that the standard List functions can be used
 * for the numeric list.
 */
int compare_number(List *item1, List *item2)
{
	FsetNumber *real1 = (FsetNumber *)item1;
	FsetNumber *real2 = (FsetNumber *)item2;

	return real1->numeric - real2->numeric;	
}

char *find_numeric_fset(int numeric)
{
FsetNumber *tmp = numeric_fset;
	while (tmp)
	{
		if (tmp->numeric == numeric)
			return tmp->format;
		tmp = tmp->next;
	}		
	return NULL;
}

void add_numeric_fset(char *name, int remove, char *args, int verbose)
{
FsetNumber *tmp = numeric_fset, *last = NULL;
int num = my_atol(name);
	for (tmp = numeric_fset; tmp; tmp = tmp->next)
	{
		if (num == tmp->numeric)
		{
			if (remove)
			{
				if (last)
					last->next = tmp->next;
				else
					numeric_fset = tmp->next;
				new_free(&tmp->format);
				new_free(&tmp);
				return;
			} 
			else 
			{
				if (args && *args)
					malloc_strcpy(&tmp->format, args);
				if (verbose)
					bitchsay("Numeric %d is %s", num, tmp->format);
				return;
			}
		}
		last = tmp;
	}		
	if (!args || !*args)
	{
		if (verbose)
			bitchsay("No such Numeric Fset %d", num);
		return;
	}
	tmp = (FsetNumber *) new_malloc(sizeof(FsetNumber));
	tmp->numeric = num;
	tmp->format = m_strdup(args);
	add_to_list_ext((List **)&numeric_fset, (List *)tmp, compare_number);
	if (verbose)
		bitchsay("Added Numeric %d as %s", num, tmp->format);
}

