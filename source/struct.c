/* 
 * Written for BitchX by Colten Edwards (c) Feb 1999
 */

#include "irc.h"
static char cvsrevision[] = "$Id: struct.c 110 2011-02-02 12:34:13Z keaston $";
CVS_REVISION(struct_c)
#include "struct.h"
#include "hash.h"
#include "hash2.h"
#include "misc.h"
#include "names.h"
#include "window.h"
#include "server.h"
#define MAIN_SOURCE
#include "modval.h"

extern char *after_expando(char *, int, int *);

/* the types of IrcVariables (repeated in vars.h) */
#define BOOL_TYPE_VAR		0
#define CHAR_TYPE_VAR		1
#define INT_TYPE_VAR		2
#define STR_TYPE_VAR		3
#define SPECIAL_TYPE_VAR	4

#define VAR_READ_WRITE		0
#define VAR_READ_ONLY		1

static char *struct_name[] = {"WINDOW", "CHANNEL", "NICK", "DCC", "CSET", "USERLIST", "SHITLIST", "BANS", "EXEMPTBANS", "SERVER", "" };

#define WINDOW_LOOKUP		0
#define CHANNEL_LOOKUP		1
#define NICKLIST_LOOKUP 	2
#define DCC_LOOKUP		3
#define CSET_LOOKUP		4
#define USERLIST_LOOKUP		5
#define SHITLIST_LOOKUP		6
#define BANS_LOOKUP		7
#define EXEMPT_LOOKUP		8
#define SERVER_LOOKUP		9

typedef struct _lookup_struct
{
	char *code;
	int offset;
	int type;
	int readwrite;
} LookupStruct;

static LookupStruct server_struct[] = {
	{ "NAME",		offsetof(Server, name), STR_TYPE_VAR, VAR_READ_ONLY },
	{ "ITSNAME",		offsetof(Server, itsname), STR_TYPE_VAR, VAR_READ_ONLY },
	{ "PASSWORD",		offsetof(Server, password), STR_TYPE_VAR, VAR_READ_WRITE },
	{ "SNETWORK",		offsetof(Server, snetwork), STR_TYPE_VAR, VAR_READ_WRITE },
	{ "COOKIE",		offsetof(Server, cookie), STR_TYPE_VAR, VAR_READ_ONLY },
	{ "PORT",		offsetof(Server, port), INT_TYPE_VAR, VAR_READ_ONLY },

	{ "NICKNAME",		offsetof(Server, nickname), STR_TYPE_VAR, VAR_READ_ONLY },
	{ "USERHOST",		offsetof(Server, userhost), STR_TYPE_VAR, VAR_READ_WRITE },
	{ "NICKNAME_PENDING",	offsetof(Server, nickname_pending), INT_TYPE_VAR, VAR_READ_ONLY },
	{ "ORIGNICK_PENDING",	offsetof(Server, orignick_pending), INT_TYPE_VAR, VAR_READ_ONLY },

	{ "AWAY",		offsetof(Server, away), STR_TYPE_VAR, VAR_READ_WRITE },
	{ "AWAYTIME",		offsetof(Server, awaytime), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "OPERATOR",		offsetof(Server, operator), BOOL_TYPE_VAR, VAR_READ_ONLY },
	{ "SERVER2_8",		offsetof(Server, server2_8), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "VERSION",		offsetof(Server, version), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "VERSION_STRING",	offsetof(Server, version_string), STR_TYPE_VAR, VAR_READ_ONLY },

	{ "UMODES",		offsetof(Server, umodes), STR_TYPE_VAR, VAR_READ_ONLY },
	{ "UMODE",		offsetof(Server, umode),  CHAR_TYPE_VAR, VAR_READ_ONLY },

	{ "CONNECTED",		offsetof(Server, connected), BOOL_TYPE_VAR, VAR_READ_ONLY },

	{ "WRITE",		offsetof(Server, write), INT_TYPE_VAR, VAR_READ_ONLY },
	{ "READ",		offsetof(Server, read), INT_TYPE_VAR, VAR_READ_ONLY },
	{ "EOF",		offsetof(Server, eof), INT_TYPE_VAR, VAR_READ_ONLY },


	{ "CHANNEL",		offsetof(Server, chan_list), SPECIAL_TYPE_VAR, VAR_READ_ONLY },
	{ "ORIGNICK",		offsetof(Server, orignick), STR_TYPE_VAR, VAR_READ_WRITE },
	{ "LAG",		offsetof(Server, lag),	INT_TYPE_VAR, VAR_READ_ONLY },
	{ NULL,			0,				0, 0 }
};

static LookupStruct channel_struct[] = {
	{ "CHANNEL",		offsetof(ChannelList, channel), STR_TYPE_VAR, VAR_READ_ONLY },
	{ "SERVER",		offsetof(ChannelList, server), INT_TYPE_VAR, VAR_READ_ONLY  },
	{ "MODE",		offsetof(ChannelList, s_mode), STR_TYPE_VAR, VAR_READ_ONLY  },
	{ "TOPIC",		offsetof(ChannelList, topic), STR_TYPE_VAR, VAR_READ_WRITE  },
	{ "TOPIC_LOCK",		offsetof(ChannelList, topic_lock), INT_TYPE_VAR, VAR_READ_WRITE  },
		
	{ "LIMIT",		offsetof(ChannelList, limit), INT_TYPE_VAR, VAR_READ_ONLY  },
	{ "KEY",		offsetof(ChannelList, key), STR_TYPE_VAR, VAR_READ_ONLY  },
	{ "CHOP",		offsetof(ChannelList, have_op), INT_TYPE_VAR, VAR_READ_ONLY  },
	{ "HOP",		offsetof(ChannelList, hop), INT_TYPE_VAR, VAR_READ_ONLY  },
	{ "VOICE",		offsetof(ChannelList, voice), INT_TYPE_VAR, VAR_READ_ONLY  },
	{ "BOUND",		offsetof(ChannelList, bound), INT_TYPE_VAR, VAR_READ_ONLY  },
	{ "CHANPASS",		offsetof(ChannelList, chanpass), STR_TYPE_VAR, VAR_READ_ONLY  },
	{ "CONNECTED",		offsetof(ChannelList, connected), INT_TYPE_VAR, VAR_READ_ONLY  },
	{ "REFNUM",		offsetof(ChannelList, refnum), INT_TYPE_VAR, VAR_READ_ONLY  },
	{ "WINDOW",		offsetof(ChannelList, window), SPECIAL_TYPE_VAR, VAR_READ_ONLY },

	{ "NICK",		offsetof(ChannelList, NickListTable), SPECIAL_TYPE_VAR, VAR_READ_ONLY },

	{ "MAXIDLE",		offsetof(ChannelList, max_idle), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "TOG_LIMIT",		offsetof(ChannelList, tog_limit), BOOL_TYPE_VAR, VAR_READ_WRITE  },
	{ "CHECK_IDLE",		offsetof(ChannelList, check_idle), BOOL_TYPE_VAR, VAR_READ_WRITE  },
	{ "DO_SCAN",		offsetof(ChannelList, do_scan), INT_TYPE_VAR, VAR_READ_ONLY  },
#if 0
	struct timeval	channel_create;		/* time for channel creation */
	struct timeval	join_time;		/* time of last join */
#endif

	{ "STATS_OPS",		offsetof(ChannelList, stats_ops), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_DOPS",		offsetof(ChannelList, stats_dops), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_BANS",		offsetof(ChannelList, stats_bans), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_UNBANS",	offsetof(ChannelList, stats_unbans), INT_TYPE_VAR, VAR_READ_WRITE  },

	{ "STATS_SOPS",		offsetof(ChannelList, stats_sops), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_SDOPS",	offsetof(ChannelList, stats_sdops), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_SHOPS",	offsetof(ChannelList, stats_shops), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_SDEHOPS",	offsetof(ChannelList, stats_sdehops), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_SEBANS",	offsetof(ChannelList, stats_sebans), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_SUNEBANS",	offsetof(ChannelList, stats_sunebans), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_SBANS",	offsetof(ChannelList, stats_sbans), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_SUNBANS",	offsetof(ChannelList, stats_sunbans), INT_TYPE_VAR, VAR_READ_WRITE  },

	{ "STATS_TOPICS",	offsetof(ChannelList, stats_topics), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_KICKS",	offsetof(ChannelList, stats_kicks), INT_TYPE_VAR, VAR_READ_WRITE  },
	{ "STATS_PUBS",		offsetof(ChannelList, stats_pubs), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STATS_PARTS",	offsetof(ChannelList, stats_parts), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STATS_SIGNOFFS",	offsetof(ChannelList, stats_signoffs), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STATS_JOINS",	offsetof(ChannelList, stats_joins), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STATS_EBANS",	offsetof(ChannelList, stats_ebans), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STATS_UNEBANS",	offsetof(ChannelList, stats_unebans), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STATS_CHANPASS",	offsetof(ChannelList, stats_chanpass), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STATS_HOPS",		offsetof(ChannelList, stats_hops), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STATS_DHOPS",	offsetof(ChannelList, stats_dhops), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "CSET",		offsetof(ChannelList, csets), SPECIAL_TYPE_VAR, VAR_READ_ONLY },

	{ "MSGLOG",		offsetof(ChannelList, msglog_on), BOOL_TYPE_VAR , VAR_READ_WRITE },
	{ "MSGLOG_FILE",	offsetof(ChannelList, logfile), STR_TYPE_VAR , VAR_READ_WRITE },

	{ "TOTALNICKS",		offsetof(ChannelList, totalnicks), INT_TYPE_VAR , VAR_READ_ONLY },
	{ "MAXNICKS",		offsetof(ChannelList, maxnicks), INT_TYPE_VAR , VAR_READ_ONLY },
	{ "MAXNICKSTIME",	offsetof(ChannelList, maxnickstime), INT_TYPE_VAR , VAR_READ_ONLY },
	{ "TOTALBANS",		offsetof(ChannelList, totalbans), INT_TYPE_VAR , VAR_READ_ONLY },
	{ "MAXBANS",		offsetof(ChannelList, maxbans), INT_TYPE_VAR , VAR_READ_ONLY },
	{ "MAXBANSTIME",	offsetof(ChannelList, maxbanstime), INT_TYPE_VAR , VAR_READ_ONLY },
	{ "BANS",		offsetof(ChannelList, bans), SPECIAL_TYPE_VAR, VAR_READ_ONLY },
	{ "EXEMPTBANS",		offsetof(ChannelList, exemptbans), SPECIAL_TYPE_VAR, VAR_READ_ONLY },
	{ NULL,			0,				0, 0 }
};

static LookupStruct user_struct[] = {
	{ "NICK",		offsetof(UserList, nick),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "HOST",		offsetof(UserList, host),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "COMMENT",		offsetof(UserList, comment),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "CHANNELS",		offsetof(UserList, channels),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "PASSWORD",		offsetof(UserList, password),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "FLAGS",		offsetof(UserList, flags),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "TIME",		offsetof(UserList, time),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ NULL,			0,				0, 0 }
};

static LookupStruct shit_struct[] = {
	{ "FILTER",		offsetof(ShitList, filter),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "LEVEL",		offsetof(ShitList, level),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "CHANNELS",		offsetof(ShitList, channels),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "REASON",		offsetof(ShitList, reason),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "TIME",		offsetof(ShitList, time),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ NULL,			0,				0, 0 }
};

static LookupStruct bans_struct[] = {
	{ "BAN",		offsetof(BanList, ban),	STR_TYPE_VAR, VAR_READ_ONLY },
	{ "SETBY",		offsetof(BanList, setby), STR_TYPE_VAR, VAR_READ_WRITE },
	{ "SENT_UNBAN",		offsetof(BanList, sent_unban),INT_TYPE_VAR, VAR_READ_WRITE },
	{ "SENT_UNBAN_TIME",	offsetof(BanList, sent_unban_time),INT_TYPE_VAR, VAR_READ_WRITE },
	{ "TIME",		offsetof(BanList, time), INT_TYPE_VAR, VAR_READ_ONLY },
	{ "COUNT",		offsetof(BanList, count), INT_TYPE_VAR, VAR_READ_ONLY },
	{ NULL,			0,				0, 0 }
};

static LookupStruct dcc_struct[] = {
	{ "USER",		offsetof(DCC_int, user),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "USERHOST",		offsetof(DCC_int, userhost),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "SERVER",		offsetof(DCC_int, server),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "ENCRYPT",		offsetof(DCC_int, encrypt),	STR_TYPE_VAR , VAR_READ_ONLY },
	{ "FILENAME",		offsetof(DCC_int, filename),	STR_TYPE_VAR , VAR_READ_ONLY },
	{ "OTHERNAME",		offsetof(DCC_int, othername),	STR_TYPE_VAR , VAR_READ_ONLY },
	{ "BYTES_READ",		offsetof(DCC_int, bytes_read),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "BYTES_SENT",		offsetof(DCC_int, bytes_sent),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "START_OFFSET",	offsetof(DCC_int, transfer_orders.byteoffset), INT_TYPE_VAR , VAR_READ_ONLY },
	{ "FILESIZE",		offsetof(DCC_int, filesize),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "PACKETS",		offsetof(DCC_int, packets),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "BLOCKSIZE",		offsetof(DCC_int, blocksize),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "DCC_FAST",		offsetof(DCC_int, dcc_fast),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "REMPORT",		offsetof(DCC_int, remport),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "LOCALPORT",		offsetof(DCC_int, localport),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "DCCNUM",		offsetof(DCC_int, dccnum),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ NULL,			0, 				0, 0}
};

static LookupStruct win_struct[] = {
	{ "NAME",		offsetof(Window, name),		STR_TYPE_VAR , VAR_READ_ONLY },
	{ "REFNUM",		offsetof(Window, refnum),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "SERVER",		offsetof(Window, server),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "TOP",		offsetof(Window, top),		INT_TYPE_VAR , VAR_READ_ONLY },
	{ "BOTTOM",		offsetof(Window, bottom),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "CURSOR",		offsetof(Window, cursor),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "LINE_CNT",		offsetof(Window, line_cnt),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "SCROLL",		offsetof(Window, noscroll),	BOOL_TYPE_VAR , VAR_READ_WRITE },
	{ "SCRATCH",		offsetof(Window, scratch_line), BOOL_TYPE_VAR , VAR_READ_WRITE },	
	{ "COLUMNS",		offsetof(Window, columns),	INT_TYPE_VAR , VAR_READ_ONLY },
	{ "NOTIFY_LEVEL",	offsetof(Window, notify_level), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "WINDOW_LEVEL",	offsetof(Window, window_level), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "CURRENT_CHANNEL",	offsetof(Window, current_channel),STR_TYPE_VAR , VAR_READ_ONLY },
	{ "WAITING_CHANNEL",	offsetof(Window, waiting_channel),STR_TYPE_VAR , VAR_READ_ONLY },
	{ "BIND_CHANNEL",	offsetof(Window, bind_channel), STR_TYPE_VAR , VAR_READ_ONLY },
	{ "QUERY_NICK",		offsetof(Window, query_nick),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "QUERY_HOST",		offsetof(Window, query_host),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "QUERY_CMD",		offsetof(Window, query_cmd),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "LOG",		offsetof(Window, log),		BOOL_TYPE_VAR , VAR_READ_WRITE },
	{ "LOGFILE",		offsetof(Window, logfile),	STR_TYPE_VAR , VAR_READ_WRITE },
	{ "LASTLOG_LEVEL",	offsetof(Window, lastlog_level),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "LASTLOG_SIZE",	offsetof(Window, lastlog_size), INT_TYPE_VAR , VAR_READ_WRITE },
	{ "LASTLOG_MAX",	offsetof(Window, lastlog_max),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "HOLD_MODE",		offsetof(Window, hold_mode),	BOOL_TYPE_VAR , VAR_READ_WRITE },
	{ "MANGLER",		offsetof(Window, mangler),	BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "PROMPT",		offsetof(Window, prompt),	STR_TYPE_VAR, VAR_READ_WRITE },
	{ NULL,			0, 				0, 0}
};			

static LookupStruct nicklist_struct[] = {
	{ "NICK",		offsetof(NickList, nick),	STR_TYPE_VAR , VAR_READ_ONLY }, 
	{ "HOST",		offsetof(NickList, host),	STR_TYPE_VAR , VAR_READ_ONLY }, 
	{ "IP",			offsetof(NickList, ip),		STR_TYPE_VAR , VAR_READ_WRITE }, 
	{ "SERVER",		offsetof(NickList, server),	STR_TYPE_VAR , VAR_READ_ONLY }, 
	{ "IP_COUNT",		offsetof(NickList, ip_count),	INT_TYPE_VAR , VAR_READ_ONLY },

	{ "USERLIST",		offsetof(NickList, userlist),	SPECIAL_TYPE_VAR , VAR_READ_ONLY },
	{ "SHITLIST",		offsetof(NickList, shitlist),	SPECIAL_TYPE_VAR , VAR_READ_ONLY },

	{ "FLAGS",		offsetof(NickList, flags),	INT_TYPE_VAR , VAR_READ_ONLY },

	{ "IDLE_TIME",		offsetof(NickList, idle_time),	INT_TYPE_VAR , VAR_READ_WRITE },

	{ "FLOODCOUNT",		offsetof(NickList, floodcount),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "FLOODTIME",		offsetof(NickList, floodtime),	INT_TYPE_VAR , VAR_READ_WRITE },

	{ "NICKCOUNT",		offsetof(NickList, nickcount),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "NICKTIME",		offsetof(NickList, nicktime),	INT_TYPE_VAR , VAR_READ_WRITE },

	{ "KICKCOUNT",		offsetof(NickList, kickcount),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "KICKTIME",		offsetof(NickList, kicktime),	INT_TYPE_VAR , VAR_READ_WRITE },

	{ "JOINCOUNT",		offsetof(NickList, joincount),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "JOINTIME",		offsetof(NickList, jointime),	INT_TYPE_VAR , VAR_READ_WRITE },


	{ "DOPCOUNT",		offsetof(NickList, dopcount),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "DOPTIME",		offsetof(NickList, doptime),	INT_TYPE_VAR , VAR_READ_WRITE },

	{ "KICKCOUNT",		offsetof(NickList, bancount),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "KICKTIME",		offsetof(NickList, bantime),	INT_TYPE_VAR , VAR_READ_WRITE },

	{ "CREATED",		offsetof(NickList, created),	INT_TYPE_VAR , VAR_READ_ONLY },


	{ "STAT_KICKS", 	offsetof(NickList, stat_kicks),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_DOPS",		offsetof(NickList, stat_dops),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_OPS",		offsetof(NickList, stat_ops),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_HOPS",		offsetof(NickList, stat_hops),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_DHOPS",		offsetof(NickList, stat_dhops),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_EBAN",		offsetof(NickList, stat_eban),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_UNEBAN",	offsetof(NickList, stat_uneban),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_BANS",		offsetof(NickList, stat_bans),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_UNBANS",	offsetof(NickList, stat_unbans),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_NICKS",		offsetof(NickList, stat_nicks),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_PUB",		offsetof(NickList, stat_pub),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "STAT_TOPICS",	offsetof(NickList, stat_topics),INT_TYPE_VAR , VAR_READ_WRITE },

	{ "SENT_REOP",		offsetof(NickList, sent_reop),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "SENT_REOP_TIME",	offsetof(NickList, sent_reop_time),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "SENT_VOICE",		offsetof(NickList, sent_voice),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "SENT_VOICE_TIME",	offsetof(NickList, sent_voice_time),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "SENT_DEOP",		offsetof(NickList, sent_deop),	INT_TYPE_VAR , VAR_READ_WRITE },
	{ "SENT_DEOP_TIME",	offsetof(NickList, sent_deop_time),INT_TYPE_VAR , VAR_READ_WRITE },
	{ "NEED_USERHOST",	offsetof(NickList, need_userhost), INT_TYPE_VAR , VAR_READ_ONLY },
	{ NULL,			0, 				0, 0}
};

static LookupStruct cset_struct[] = {
	{ "AINV",		offsetof(CSetList, set_ainv), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "ANNOY_KICK",		offsetof(CSetList, set_annoy_kick), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "AOP",		offsetof(CSetList, set_aop), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "AUTO_JOIN_ON_INVITE",offsetof(CSetList, set_auto_join_on_invite), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "AUTO_LIMIT",		offsetof(CSetList, set_auto_limit), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "AUTO_REJOIN",	offsetof(CSetList, set_auto_rejoin), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "BANTIME",		offsetof(CSetList, set_bantime), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "BITCH",		offsetof(CSetList, bitch_mode), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "CHANMODE",		offsetof(CSetList, chanmode), STR_TYPE_VAR, VAR_READ_WRITE },

	{ "CHANNEL_LOG",	offsetof(CSetList, channel_log), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "CHANNEL_LOG_FILE",	offsetof(CSetList, channel_log_file), STR_TYPE_VAR, VAR_READ_WRITE },
	{ "CHANNEL_LOG_LEVEL",	offsetof(CSetList, log_level), INT_TYPE_VAR, VAR_READ_WRITE },

	{ "COMPRESS_MODES",	offsetof(CSetList, compress_modes), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "CTCP_FLOOD_BAN",	offsetof(CSetList, set_ctcp_flood_ban), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "DEOPFLOOD",		offsetof(CSetList, set_deopflood), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "DEOPFLOOD_TIME",	offsetof(CSetList, set_deopflood_time), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "DEOP_ON_DEOPFLOOD",	offsetof(CSetList, set_deop_on_deopflood), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "DEOP_ON_KICKFLOOD",	offsetof(CSetList, set_deop_on_kickflood), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "HACKING",		offsetof(CSetList, set_hacking), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "JOINFLOOD",		offsetof(CSetList, set_joinflood), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "JOINFLOOD_TIME",	offsetof(CSetList, set_joinflood_time), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "KICKFLOOD",		offsetof(CSetList, set_kickflood), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "KICKFLOOD_TIME",	offsetof(CSetList, set_kickflood_time), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "KICK_IF_BANNED",	offsetof(CSetList, set_kick_if_banned), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "KICK_ON_DEOPFLOOD",	offsetof(CSetList, set_kick_on_deopflood), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "KICK_ON_JOINFLOOD",	offsetof(CSetList, set_kick_on_joinflood), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "KICK_ON_KICKFLOOD",	offsetof(CSetList, set_kick_on_kickflood), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "KICK_ON_NICKFLOOD",	offsetof(CSetList, set_kick_on_nickflood), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "KICK_ON_PUBFLOOD",	offsetof(CSetList, set_kick_on_pubflood), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "KICK_OPS",		offsetof(CSetList, set_kick_ops), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "LAMEIDENT",		offsetof(CSetList, set_lame_ident), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "LAMELIST",		offsetof(CSetList, set_lamelist), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "NICKFLOOD",		offsetof(CSetList, set_nickflood), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "NICKFLOOD_TIME",	offsetof(CSetList, set_nickflood_time), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "PUBFLOOD",		offsetof(CSetList, set_pubflood), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "PUBFLOOD_IGNORE_TIME",offsetof(CSetList, set_pubflood_ignore), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "PUBFLOOD_TIME",	offsetof(CSetList, set_pubflood_time), INT_TYPE_VAR, VAR_READ_WRITE },
	{ "SHITLIST",		offsetof(CSetList, set_shitlist), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "USERLIST",		offsetof(CSetList, set_userlist), BOOL_TYPE_VAR, VAR_READ_WRITE },
	{ "CHANNEL",		offsetof(CSetList, channel), STR_TYPE_VAR, VAR_READ_ONLY },
	{ NULL,			0, 				0, 0}
};

int find_structure(char *name)
{
int i;
	for (i = 0; *struct_name[i]; i++)
		if (!my_stricmp(struct_name[i], name))
			return i;	
	return -1;
}

int setup_structure(char *name, char *which, Window **win, DCC_int **dcc, ChannelList **chan, NickList **nick, int *server)
{
	int i;
	int serv = -1;
	i = find_structure(name);
	switch (i)
	{
		case WINDOW_LOOKUP:
		{
			*win = get_window_by_desc(which);
			if (!*win)
				*win = current_window;
			return WINDOW_LOOKUP;
		}
		case NICKLIST_LOOKUP:
		{
			char *ch = NULL;
			if (!*chan)
			{
				if (!(ch = get_current_channel_by_refnum(0)))
					return -1;
				if (!(*chan = lookup_channel(ch, current_window->server, 0)))
					return -1;
			}
			if (!ch && !*chan)
				return -1;
			*nick = find_nicklist_in_channellist(which, *chan, 0);
			return NICKLIST_LOOKUP;
		}
		case DCC_LOOKUP:
		{
			SocketList *s;
			DCC_int *n;
			int l = my_atol(which);
			s = get_socket(l);
			if (!s || !(n = (DCC_int *)s->info))
				break;
			*dcc = n;
			return DCC_LOOKUP;
		}
		case CHANNEL_LOOKUP:
		{
			char *ch = which;
			if (!*ch)
			{
				if (!(ch = get_current_channel_by_refnum(0)))
					return -1;
			}
			if (is_channel(ch))
				*chan = lookup_channel(ch, current_window->server, 0);
			else
			{
				if ((ch = make_channel(ch)))
					*chan = lookup_channel(ch, current_window->server, 0);
			}
			return CHANNEL_LOOKUP;
		}
		case SERVER_LOOKUP:
			if (*which && ((serv = my_atol(which)) != -1))
				*server = serv;
			return SERVER_LOOKUP;
		default:
			break;
	}
	return -1;
}


static inline int get_offset_int(void *tmp, int offset)
{
	int val = *(int *)((char *)tmp + offset);
	return val;
}

static inline char *get_offset_str(void *tmp, int offset)
{
	char *s = *(char **)((char *)tmp + offset);
	return s;
}

static inline char *get_offset_char(void *tmp, int offset)
{
	char *s = (char *)((char *)tmp + offset);
	return s;
}

static inline void set_offset_int(void *tmp, int offset, int val)
{
	int *ptr = (int *)((char *)tmp + offset);
	*ptr = val;
}

static inline void set_offset_str(void *tmp, int offset, const char *val)
{
	char **ptr = (char **)((char *)tmp + offset);
	if (val && *val)
		malloc_strcpy(ptr, val);
	else
		new_free(ptr);
}

static inline void set_offset_char(void *tmp, int offset, const char *val)
{
	char *ptr = (char *)((char *)tmp + offset);
	if (val && *val)
		strcpy(ptr, val);
	else
		*ptr = 0;
}

int lookup_code(LookupStruct *t, char *name)
{
	int i;
	for (i = 0; t[i].code; i++)
	{
		if (!my_stricmp(t[i].code, name))
			break;
	}
	if (!t[i].code)
		return -1;
	return i;
}

char *return_structure(LookupStruct *name, void *user)
{
int j;
char *ret = NULL;
char *s;
	if (!user)
		return NULL;
	for (j = 0; name[j].code; j++)
	{
		if (name[j].type == INT_TYPE_VAR)
			m_s3cat(&ret, ",", ltoa(get_offset_int(user, name[j].offset)));
		else if (name[j].type == STR_TYPE_VAR)
		{
			s = get_offset_str(user, name[j].offset);
			m_s3cat(&ret, ",", s ? s : empty_string);
		}
		else if (name[j].type == BOOL_TYPE_VAR)
			m_s3cat(&ret, ",", on_off(get_offset_int(user, name[j].offset)));
		else if (name[j].type == CHAR_TYPE_VAR)
			m_s3cat(&ret, ",", get_offset_char(user, name[j].offset));
		else if (name[j].type == SPECIAL_TYPE_VAR)
		{
			s = LOCAL_COPY(name[j].code);
			lower(s);
			m_s3cat(&ret, ",", "<");
			malloc_strcat(&ret, s);
			malloc_strcat(&ret, ">");
		}
	}
	return ret;
}

char *lookup_structure_item(int idx, LookupStruct *name, void *user, char *arg)
{
	if (!user || (idx == -1))
		return NULL;
	switch (name[idx].type)
	{
		case STR_TYPE_VAR:
			if (arg && !name[idx].readwrite)
				set_offset_str(user, name[idx].offset, arg);
			return m_strdup(get_offset_str(user, name[idx].offset));
		case CHAR_TYPE_VAR:
			if (arg && !name[idx].readwrite)
				set_offset_char(user, name[idx].offset, arg);
			return m_strdup(get_offset_char(user, name[idx].offset));
		case INT_TYPE_VAR:
			if (arg && *arg && !name[idx].readwrite)
				set_offset_int(user, name[idx].offset, my_atol(arg));
			return m_strdup(ltoa(get_offset_int(user, name[idx].offset)));
		case BOOL_TYPE_VAR:
			if (arg && *arg && !name[idx].readwrite)
			{
				int val = 0;
				if (!my_stricmp(arg, "on"))
					val = 1;
				else if (!my_stricmp(arg, "off"))
					val = 0;
				else
					val = my_atol(arg);
				set_offset_int(user, name[idx].offset, val);
			}
			return m_strdup(on_off(get_offset_int(user, name[idx].offset)));
		default:
			break;
	}
	return NULL;
}

char *lookup_structure_member(int type, char *name, char *rest, NickList *n, ChannelList *ch, Window *w, DCC_int *dcc, int serv)
{
LookupStruct *tmp = NULL;
int i = -1;
char *ret = NULL;
void *user = NULL;
int code = -1;
	switch (type)
	{
		case WINDOW_LOOKUP:
			tmp = win_struct;
			user = w;
			break;
		case NICKLIST_LOOKUP:
			tmp = nicklist_struct;
			user = n;
			break;
		case DCC_LOOKUP:
			tmp = dcc_struct;
			user = dcc;
			break;
		case CHANNEL_LOOKUP:
			tmp = channel_struct;
			user = ch;
			break;
		case CSET_LOOKUP:
			tmp = cset_struct;
			if (ch)
				user = ch->csets;
			break;
		case SERVER_LOOKUP:
			tmp = server_struct;
			if (serv >= 0 && serv < server_list_size())
			{
				Server *slist = get_server_list();

				user = &slist[serv];
				ch = slist[serv].chan_list;
			}
		default:
			break;
	}
	if (!tmp || !user) 
		return NULL;
	if (!name || !*name)
		return return_structure(tmp, user);
 	if ((i = lookup_code(tmp, name)) == -1)
		return NULL;
 	switch(tmp[i].type)
	{
		case INT_TYPE_VAR:
		case STR_TYPE_VAR:
		case BOOL_TYPE_VAR:
		case CHAR_TYPE_VAR:
		{
			char *lparen;
			if (rest && !*rest)
				rest = NULL;
#if 0
			if ((lparen = strchr(rest+1, '[')))
			{
				if ((rparen = MatchingBracket(lparen+1, '[', ']')))
					*rparen++ = 0;
				*lparen++ = 0;
				if (!*lparen)
					lparen = NULL;
			} else if (*rest)
#endif
				lparen = rest;
			ret = lookup_structure_item(i, tmp, user, lparen);
			break;
		}
		case SPECIAL_TYPE_VAR:
		{
			code = find_structure(name);
			switch (code)
			{
				case NICKLIST_LOOKUP: /* this is a special case */
					tmp = nicklist_struct;
					n = sorted_nicklist(ch, NICKSORT_NONE);
					for (; n; n = n->next)
						if (!rest || !*rest)
							m_s3cat(&ret, ",", n->nick);
						else if (*rest == '>' &&  *(rest+1) && !my_stricmp(rest+1, n->nick))
							ret = return_structure(tmp, n); 
					clear_sorted_nicklist(&n);
					return ret;
					break;
				case USERLIST_LOOKUP:
					tmp = user_struct;
					if (n)
						user = n->userlist;
					break;
				case SHITLIST_LOOKUP:
					tmp = shit_struct;
					if (n)
						user = n->shitlist;
					break;
				case CSET_LOOKUP:
					tmp = cset_struct;
					if (ch)
						user = ch->csets;
					break;
				case WINDOW_LOOKUP:
					tmp = win_struct;
					if (ch)
						user = ch->window;
					break;
				case EXEMPT_LOOKUP:
					tmp = bans_struct;
					if (ch)
						user = ch->exemptbans;
					break;
				case BANS_LOOKUP:
				{
					BanList *b;
					tmp = bans_struct;
					if (ch)
						user = ch->bans;
					for (b = ch->bans; b; b= b->next)
						if (!rest || !*rest)
							m_s3cat(&ret, ",", b->ban);
						else if (*rest == '>' &&  *(rest+1) && !my_stricmp(rest+1, b->ban))
							ret = return_structure(tmp, b);
					return ret;
					break;
				}
				case CHANNEL_LOOKUP:
					tmp = channel_struct;
					ch = ((Server *)user)->chan_list;
					for (; ch; ch = ch->next)
						if (!rest || !*rest)
							m_s3cat(&ret, ",", ch->channel);
						else if (*rest == '>' &&  *(rest+1) && !my_stricmp(rest+1, ch->channel))
							ret = return_structure(tmp, ch);
					return ret;
					break;
			}
			if (rest && *rest == '>' && *(rest + 1))
			{
				char *lparen, *rparen;
				if ((lparen = strchr(rest+1, '[')))
				{
					if ((rparen = MatchingBracket(lparen+1, '[', ']')))
						*rparen++ = 0;
					*lparen++ = 0;
				} else if (rest && !*rest)
					rest = NULL;
				i = lookup_code(tmp, rest+1);
				ret = lookup_structure_item(i, tmp, user, lparen);
			}
			else
				ret = return_structure(tmp, user);
		}
		default:
			break;
	}
	return ret;
}


char *call_structure_internal(char *name, char *args, char *rest, char *rest1)
{
	Window		*win	= NULL; 
	ChannelList	*chan	= NULL;
	DCC_int		*dcc	= NULL;
	NickList	*nick	= NULL;
	char		*ret	= NULL;
	int		type =  -1;
	int		server = -1;
	char		*lparen, *rparen;
	type = setup_structure(name, args, &win, &dcc, &chan, &nick, &server);
	if ((lparen = strchr(rest, '[')))
	{
		if ((rparen = MatchingBracket(lparen+1, '[', ']')))
			*rparen++ = 0;
		*lparen++ = 0;
		rest1 = lparen;
	}
	if (type != -1)
		ret = lookup_structure_member(type, rest, rest1, nick, chan, win ? win : current_window, dcc, server);
	return ret;
}
