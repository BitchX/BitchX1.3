/*
 * ctcp.c:handles the client-to-client protocol(ctcp). 
 *
 * Written By Michael Sandrof 
 * Copyright(c) 1990, 1995 Michael Sandroff and others
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Serious cleanup by jfn (August 1996)
 */


#include "irc.h"
static char cvsrevision[] = "$Id: ctcp.c 137 2011-09-06 06:48:57Z keaston $";
CVS_REVISION(ctcp_c)
#include "struct.h"

#include <pwd.h>


#ifdef HAVE_UNAME
# include <sys/utsname.h>
#endif

#include <stdarg.h>

#include "encrypt.h"
#include "ctcp.h"
#include "dcc.h"
#include "commands.h"
#include "flood.h"
#include "hook.h"
#include "if.h"
#include "ignore.h"
#include "ircaux.h"
#include "lastlog.h"
#include "list.h"
#include "names.h"
#include "output.h"
#include "parse.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "window.h"
#include "cdcc.h"
#include "misc.h"
#include "userlist.h"
#include "hash2.h"

#ifdef WANT_TCL
#include "tcl_bx.h"
#endif

#define MAIN_SOURCE
#include "modval.h"

/*
 * ctcp_entry: the format for each ctcp function.   note that the function
 * described takes 4 parameters, a pointer to the ctcp entry, who the message
 * was from, who the message was to (nickname, channel, etc), and the rest of
 * the ctcp message.  it can return null, or it can return a malloced string
 * that will be inserted into the oringal message at the point of the ctcp.
 * if null is returned, nothing is added to the original message
 */

/* forward declarations for the built in CTCP functions */
static	char	*do_sed 	(CtcpEntry *, char *, char *, char *);
static	char	*do_version 	(CtcpEntry *, char *, char *, char *);
static	char	*do_clientinfo 	(CtcpEntry *, char *, char *, char *);
static	char	*do_ping 	(CtcpEntry *, char *, char *, char *);
static	char	*do_echo 	(CtcpEntry *, char *, char *, char *);
static	char	*do_userinfo 	(CtcpEntry *, char *, char *, char *);
static	char	*do_finger 	(CtcpEntry *, char *, char *, char *);
static	char	*do_time 	(CtcpEntry *, char *, char *, char *);
static	char	*do_atmosphere 	(CtcpEntry *, char *, char *, char *);
static	char	*do_dcc 	(CtcpEntry *, char *, char *, char *);
static	char	*do_utc 	(CtcpEntry *, char *, char *, char *);
static	char	*do_dcc_reply 	(CtcpEntry *, char *, char *, char *);
static	char	*do_ping_reply 	(CtcpEntry *, char *, char *, char *);
static	char	*do_bdcc 	(CtcpEntry *, char *, char *, char *);
static	char	*do_cinvite 	(CtcpEntry *, char *, char *, char *);
static	char	*do_whoami 	(CtcpEntry *, char *, char *, char *);
static	char	*do_ctcpops 	(CtcpEntry *, char *, char *, char *);
static	char	*do_ctcpunban 	(CtcpEntry *, char *, char *, char *);
static	char	*do_botlink 	(CtcpEntry *, char *, char *, char *);
static	char	*do_botlink_rep	(CtcpEntry *, char *, char *, char *);
static	char	*do_ctcp_uptime	(CtcpEntry *, char *, char *, char *);
static	char	*do_ctcpident	(CtcpEntry *, char *, char *, char *);

static CtcpEntry ctcp_cmd[] =
{
	{ "SED",	CTCP_SED, 	CTCP_INLINE | CTCP_NOLIMIT,
		"contains simple_encrypted_data",
		do_sed, 	do_sed },
	{ "UTC",	CTCP_UTC, 	CTCP_INLINE | CTCP_NOLIMIT,
		"substitutes the local timezone",
		do_utc, 	do_utc },
	{ "ACTION",	CTCP_ACTION, 	CTCP_SPECIAL | CTCP_NOLIMIT,
		"contains action descriptions for atmosphere",
		do_atmosphere, 	do_atmosphere },

	{ "DCC",	CTCP_DCC, 	CTCP_SPECIAL,
		"requests a direct_client_connection",
		do_dcc, 	do_dcc_reply },

	{ "CDCC",	CTCP_CDCC, 	CTCP_SPECIAL | CTCP_TELLUSER,
		"checks cdcc info for you",
		do_bdcc,	NULL },
	{ "BDCC",	CTCP_CDCC1, 	CTCP_SPECIAL | CTCP_TELLUSER,
		"checks cdcc info for you",
		do_bdcc,	NULL },
	{ "XDCC",	CTCP_CDCC2, 	CTCP_SPECIAL | CTCP_TELLUSER,
		"checks cdcc info for you",
		do_bdcc,	NULL },

	{ "VERSION",	CTCP_VERSION,	CTCP_REPLY | CTCP_TELLUSER,
		"shows client type, version and environment",
		do_version, 	NULL },

	{ "CLIENTINFO",	CTCP_CLIENTINFO,CTCP_REPLY | CTCP_TELLUSER,
		"gives information about available CTCP commands",
		do_clientinfo, 	NULL },
	{ "USERINFO",	CTCP_USERINFO, 	CTCP_REPLY | CTCP_TELLUSER,
		"returns user settable information",
		do_userinfo, 	NULL },
	{ "ERRMSG",	CTCP_ERRMSG, 	CTCP_REPLY | CTCP_TELLUSER,
		"returns error messages",
		do_echo, 	NULL },
	{ "FINGER",	CTCP_FINGER, 	CTCP_REPLY | CTCP_TELLUSER,
		"shows real name, login name and idle time of user", 
		do_finger, 	NULL },
	{ "TIME",	CTCP_TIME, 	CTCP_REPLY | CTCP_TELLUSER,
		"tells you the time on the user's host",
		do_time, 	NULL },
	{ "PING", 	CTCP_PING, 	CTCP_REPLY | CTCP_TELLUSER,
		"returns the arguments it receives",
		do_ping, 	do_ping_reply },
	{ "ECHO", 	CTCP_ECHO, 	CTCP_REPLY | CTCP_TELLUSER,
		"returns the arguments it receives",
		do_echo, 	NULL },

	{ "INVITE",	CTCP_INVITE, 	CTCP_SPECIAL,
		"invite to channel specified",
		 do_cinvite, 	NULL },
	{ "WHOAMI",	CTCP_WHOAMI,	CTCP_SPECIAL,
		"user list information",
		do_whoami,	NULL },
	{ "OP",		CTCP_OPS,	CTCP_SPECIAL,
		"ops the person if on userlist",
		do_ctcpops,	NULL },
	{ "OPS",	CTCP_OPS,	CTCP_SPECIAL,
		 "ops the person if on userlist",
		do_ctcpops,	NULL },
	{ "UNBAN",	CTCP_UNBAN,	CTCP_SPECIAL,
		"unbans the person from channel",
		do_ctcpunban,	NULL },
	{ "IDENT",	CTCP_IDENT,	CTCP_SPECIAL,
		"change userhost of userlist",
		do_ctcpident,	NULL },
	{ "XLINK",	CTCP_BOTLINK,	CTCP_SPECIAL,
		"x-filez rule",
		do_botlink,	do_botlink_rep },

	{ "UPTIME",	CTCP_UPTIME,	CTCP_SPECIAL,
		"my uptime",
		do_ctcp_uptime, NULL },

	{ NULL,		CTCP_CUSTOM,	CTCP_REPLY | CTCP_TELLUSER,
		NULL,
		NULL, NULL }
};

#ifdef WANT_DLL
CtcpEntryDll *dll_ctcp = NULL;
#endif


/* CDE do ops and unban logging */

static char	*ctcp_type[] =
{
	"PRIVMSG",
	"NOTICE"
};

/* This is set to one if we parsed an SED */
int     sed = 0;

/*
 * in_ctcp_flag is set to true when IRCII is handling a CTCP request.  This
 * is used by the ctcp() sending function to force NOTICEs to be used in any
 * CTCP REPLY 
 */
int	in_ctcp_flag = 0;

#define CTCP_HANDLER(x) \
	static char * x (CtcpEntry *ctcp, char *from, char *to, char *cmd)

/**************************** CTCP PARSERS ****************************/

/********** INLINE EXPANSION CTCPS ***************/


#ifdef WANT_USERLIST
void print_ctcp(char *from, char *uh, char *to, char *str, char *cmd)
{
	put_it("%s", convert_output_format(fget_string_var(is_channel(to)?FORMAT_CTCP_CLOAK_FUNC_FSET:FORMAT_CTCP_CLOAK_FUNC_USER_FSET),
		"%s %s %s %s %s %s",update_clock(GET_TIME), from, uh, to, str, *cmd? cmd : empty_string));
}

void log_denied(char *type, char *from, char *uh, char *to, char *cmd)
{
	logmsg(LOG_CTCP, from, 0, "%s %s %s [%s]", type, "! Access Denied from ", uh, cmd);
}
#endif

CTCP_HANDLER(do_ctcpident)
{
#if defined(WANT_USERLIST)
UserList *ThisNick = NULL;
char *nick, *passwd = NULL;
char *channel = "*";
int ok = 0;
	print_ctcp(from, FromUserHost, to, "IDENT ", cmd);
	if (is_channel(to))
		return NULL;
	nick = next_arg(cmd, &cmd);
	passwd = next_arg(cmd, &cmd);
	if (cmd && *cmd)
		channel = next_arg(cmd, &cmd);
	ThisNick = lookup_userlevelc(nick, "*", channel, passwd);
	if (nick && passwd && ThisNick && ThisNick->password)
	{
		if (!checkpass(passwd, ThisNick->password))
		{
			char *bang = NULL;
			char *u, *h;
			ok = 1;
			bang = LOCAL_COPY(FromUserHost);
			u = bang;
			if ((h = strchr(bang, '@')))
				*h++ = 0;
			if (!h || !*h)
				return NULL;
			malloc_sprintf(&ThisNick->host, "*%s@%s", clear_server_flags(u), cluster(h));
		}
	}
	if (!ok && !get_int_var(CLOAK_VAR))
		log_denied("IDENT", from, FromUserHost, to, cmd);
	else if (ok || (ThisNick && (ThisNick->flags & ADD_CTCP)))
	{
		if (get_int_var(SEND_CTCP_MSG_VAR))
			send_to_server("NOTICE %s :UserHost updated to %s", from, FromUserHost);
		logmsg(LOG_CTCP, from, 0, "Successful %s from %s!%s", "IDENT", from, FromUserHost);
	}
#endif
	return NULL;
}




/* parse a remote ctcp CDCC command */
CTCP_HANDLER(do_bdcc)
{
#ifdef WANT_CDCC
	int i;
	char *secure = NULL;
extern	pack *offerlist;
extern	remote_cmd remote[];
	char *rest, *arg, *temp = NULL;
		
	if ((check_ignore(from, FromUserHost, to, IGNORE_CDCC, NULL) == IGNORED))
		return NULL;
		
	if (!offerlist || !get_int_var(CDCC_VAR))
		return NULL;

	if ((secure = get_string_var(CDCC_SECURITY_VAR)))
	{
		UserList *tmp;
		char *pass = NULL;
/* workaround for var.c which had a int for a var */
		if (*secure == '0' && strlen(secure) == 1)
			goto got_good_pass1;
		if (cmd && *cmd)
			pass = strrchr(cmd, ' ');
		if (pass && *pass)
		{
			pass++;
			if (*pass && !my_stricmp(pass, secure))
			{
				*pass-- = 0;
				goto got_good_pass1;
			}
		}	
#if defined(WANT_USERLIST)
		if (!(tmp = lookup_userlevelc("*", FromUserHost, "*", NULL)) || !(tmp->flags & ADD_DCC))
			return NULL;
#else
		return NULL;
#endif
	}

got_good_pass1:
	temp = LOCAL_COPY(cmd);
	arg = next_arg(temp, &temp);
	if (!arg) 
		return NULL;
	rest = temp;
	for (i = 0; *remote[i].name; i++) 
	{
		if (!my_stricmp(arg, remote[i].name)) 
		{
			remote[i].function(from, rest);
			return NULL;
		}
	}
	send_to_server("NOTICE %s :try /ctcp %s cdcc help", from, get_server_nickname(from_server));
#endif	 
	return NULL;
}

CTCP_HANDLER(do_botlink)
{
#if !defined(BITCHX_LITE)
#ifdef WANT_USERLIST
char *nick = NULL, *password = NULL, *port = NULL;
	nick = next_arg(cmd, &cmd);
	password = next_arg(cmd, &cmd);
	port = next_arg(cmd, &cmd);
	print_ctcp(from, FromUserHost, to, "XLINK ", cmd);
	if (nick && password)
	{
		char *t = NULL;
		UserList *n = NULL;
		if (get_string_var(BOT_PASSWD_VAR) || (n = lookup_userlevelc(nick, FromUserHost, "*", password)))
		{
			char *pass;
			if (!n)
				pass = get_string_var(BOT_PASSWD_VAR);
			else
				pass = n->password;
			if (pass && !my_stricmp(pass, password))
			{
				if (port)
					malloc_sprintf(&t, "%s -p %s -e %s", nick, port, password);
				else
					malloc_sprintf(&t, "%s -e %s", nick, password);
				dcc_chat("BOT", t);
				new_free(&t);
				set_int_var(BOT_MODE_VAR, 1);
				logmsg(LOG_CTCP, from, 0, "Successful %s from %s!%s to %s [%s]", "XLINK", from, FromUserHost, to, cmd?cmd:empty_string);
			} else 
				log_denied("XLINK", from, FromUserHost, to, cmd);
		} else
			log_denied("XLINK", from, FromUserHost, to, cmd);
	}
	else
	{
		log_denied("XLINK", from, FromUserHost, to, cmd);
		return m_sprintf("Invalid Bot Link request %s %s %s", nick?nick:"null", password?password:"-", port?port:"*");
	}
#endif
#endif

	return NULL;
}

CTCP_HANDLER(do_botlink_rep)
{
#ifndef BITCHX_LITE
char *type, *description, *inetaddr, *port, *extra_flags;
	if (my_stricmp(to, get_server_nickname(from_server)))
		return NULL;

	if     (!(type = next_arg(cmd, &cmd)) ||
		!(description = next_arg(cmd, &cmd)) ||
		!(inetaddr = next_arg(cmd, &cmd)) ||
		!(port = next_arg(cmd, &cmd)))
			return NULL;

	extra_flags = next_arg(cmd, &cmd);
	set_int_var(BOT_MODE_VAR, 1);
	register_dcc_type(from, type, description, inetaddr, port, NULL, extra_flags, FromUserHost, NULL);
#endif
	return NULL;
}




CTCP_HANDLER(do_cinvite)
{
#ifdef WANT_USERLIST
UserList *tmp;
char *channel;
char *password = NULL;
ChannelList *chan;
int serv = from_server;
	print_ctcp(from, FromUserHost, to, "Invite to ", cmd);
	channel = next_arg(cmd, &cmd);
	if (cmd && *cmd)
		password = next_arg(cmd, &cmd);

	if (is_channel(to) || !channel || (channel && *channel && !is_channel(channel)))
		return NULL;

	tmp = lookup_userlevelc("*", FromUserHost, channel, password);
	chan = prepare_command(&serv, channel, PC_SILENT);
	if (chan && tmp && (tmp->flags & ADD_INVITE) && (check_channel_match(tmp->channels, channel)))
	{
		if (tmp->password && !password)
		{
			log_denied("INVITE", from, FromUserHost, to, cmd);
			return NULL;
		}
		if (im_on_channel(channel, from_server) && is_chanop(channel, get_server_nickname(from_server)))
		{
			if (chan->key)
				my_send_to_server(serv, "NOTICE %s :\002Ctcp-inviting you to %s. Key is [%s]\002", from, channel, chan->key);
			else
				my_send_to_server(serv, "NOTICE %s :\002Ctcp-inviting you to %s\002", from, channel);
			my_send_to_server(serv, "INVITE %s %s", from, channel);
			logmsg(LOG_CTCP, from, 0, "Successful INVITE from %s!%s to %s", from, FromUserHost, channel);
		}
		else
		{
			if (get_int_var(SEND_CTCP_MSG_VAR))
				my_send_to_server(serv, "NOTICE %s :\002%s\002: I'm not on %s, or I'm not opped", from, version, channel);
			log_denied("INVITE", from, FromUserHost, to, cmd);
		}
	} 
	else if (!get_int_var(CLOAK_VAR) || (tmp && (tmp->flags & ADD_CTCP)))
	{
		if (!chan)
		{
			if (get_int_var(SEND_CTCP_MSG_VAR))
				my_send_to_server(serv, "NOTICE %s :\002%s\002: I'm not on that channel", from, version);
			log_denied("INVITE", from, FromUserHost, to, cmd);
		}
		else
		{
			if (get_int_var(SEND_CTCP_MSG_VAR))
				my_send_to_server(serv, "NOTICE %s :\002%s\002: Access Denied", from, version);
			log_denied("INVITE", from, FromUserHost, to, cmd);
		}
	}
	else
		log_denied("INVITE", from, FromUserHost, to, cmd);
#endif
	return NULL;
}

CTCP_HANDLER(do_whoami)
{
#ifdef WANT_USERLIST
UserList *Nick = NULL;
	print_ctcp(from, FromUserHost, to, "WhoAmI", cmd);
	if (is_channel(to))
		return NULL;

	Nick = lookup_userlevelc("*", FromUserHost, "*", NULL);
	if (Nick && Nick->flags)
	{
		send_to_server("NOTICE %s :\002%s\002: Userlevel - %s ",from, version, convert_flags_to_str(Nick->flags));
		send_to_server("NOTICE %s :\002%s\002: Host Mask - %s Channels Allowed - %s   %s", 
			from, version, Nick->host, Nick->channels, Nick->password?"Password Required":empty_string);
		logmsg(LOG_CTCP, from, 0, "Successful %s from %s!%s to %s", "WHOAMI", from, FromUserHost, to);
	} 
	else if (!get_int_var(CLOAK_VAR) || (Nick && (Nick->flags & ADD_CTCP)))
	{
		send_to_server("NOTICE %s :\002%s\002: Access Denied", from, version);
		if (get_int_var(SEND_CTCP_MSG_VAR))
			log_denied("WHOAMI", from, FromUserHost, to, cmd);
	} else 	if (get_int_var(SEND_CTCP_MSG_VAR))
		log_denied("WHOAMI", from, FromUserHost, to, cmd);
#endif
	return NULL;
}

CTCP_HANDLER(do_ctcp_uptime)
{
#ifdef WANT_USERLIST
UserList *Nick = NULL;
	print_ctcp(from, FromUserHost, to, "Uptime", cmd);
	Nick = lookup_userlevelc("*", FromUserHost, "*", NULL);
	if (Nick)
	{
		send_to_server("NOTICE %s :\002%s\002: Current Uptime is %s", from, version, convert_time(now-start_time)); 
		logmsg(LOG_CTCP, from, 0, "Successful %s from %s!%s to %s", "UPTIME", from, FromUserHost, to);
	} else 	if (get_int_var(SEND_CTCP_MSG_VAR))
		log_denied("UPTIME", from, FromUserHost, to, cmd);
#endif
	return NULL;
}

CTCP_HANDLER(do_ctcpops)
{
#ifdef WANT_USERLIST
UserList *Nick = NULL;
char *channel;
char *password = NULL;
ChannelList *chan;
int serv = from_server;
	print_ctcp(from, FromUserHost, to, "Ops on", cmd);

	if (is_channel(to))
		return NULL;
	channel = next_arg(cmd, &cmd);
	if (!channel || !*channel)
		return NULL;
	if (cmd && *cmd)
		password = next_arg(cmd, &cmd);
	Nick = lookup_userlevelc("*", FromUserHost, channel, password);
	chan = prepare_command(&serv, channel, PC_SILENT);
	if (chan && get_cset_int_var(chan->csets, USERLIST_CSET) && Nick)
	{
		if (Nick->flags & ADD_OPS)
		{
			if (Nick->password && !password)
			{
				log_denied("OPS", from, FromUserHost, to, cmd);
				return NULL;
			}
			if (im_on_channel(channel, from_server) && is_chanop(channel, get_server_nickname(from_server)))
			{
				my_send_to_server(serv, "MODE %s +o %s", channel, from); 
				logmsg(LOG_CTCP, from, 0, "Successful %s from %s!%s", "OPS", from, FromUserHost);
			}
			else
			{
				if (get_int_var(SEND_CTCP_MSG_VAR))
					my_send_to_server(serv, "NOTICE %s :\002%s\002: I'm not on %s, or I'm not opped", from, version, channel);
				log_denied("OPS", from, FromUserHost, to, cmd);
			}
		}
		else if (Nick->flags & ADD_VOICE)
		{
			if (im_on_channel(channel, from_server) && is_chanop(channel, get_server_nickname(from_server)))
			{
				my_send_to_server(serv, "MODE %s +v %s", channel, from); 
				logmsg(LOG_CTCP, from, 0, "Successful %s from %s!%s", "VOICE", from, FromUserHost);
			}
			else
			{
				if (get_int_var(SEND_CTCP_MSG_VAR))
					my_send_to_server(serv, "NOTICE %s :\002%s\002: I'm not on %s, or I'm not opped", from, version, channel);
				log_denied("OPS", from, FromUserHost, to, cmd);
			}
		}
		else
		{
			if ((!get_int_var(CLOAK_VAR) || (Nick && (Nick->flags & ADD_CTCP))) && get_int_var(SEND_CTCP_MSG_VAR))
				my_send_to_server(serv, "NOTICE %s :\002%s\002: I'm not on %s, or I'm not opped", from, version, channel?channel:empty_string);
			log_denied("OPS", from, FromUserHost, to, cmd);
		}
	}
	else if (!get_int_var(CLOAK_VAR) || (Nick && (Nick->flags & ADD_CTCP)))
	{
		if (get_int_var(SEND_CTCP_MSG_VAR))
			my_send_to_server(serv, "NOTICE %s :\002%s\002: I'm not on %s, or I'm not opped", from, version, channel?channel:empty_string);
		log_denied("OPS", from, FromUserHost, to, cmd);
	}
#endif
	return NULL;
}

CTCP_HANDLER(do_ctcpunban)
{
#ifdef WANT_USERLIST
UserList *Nick = NULL;
char *channel;
char *password = NULL;
char ban[BIG_BUFFER_SIZE];
ChannelList *chan;
int server;

	print_ctcp(from, FromUserHost, to, "UnBan on ", cmd);

	if (is_channel(to))
		return NULL;

	channel = next_arg(cmd, &cmd);
	if (!channel || !*channel)
		return NULL;
	if (cmd && *cmd)
		password = next_arg(cmd, &cmd);

	Nick = lookup_userlevelc("*", FromUserHost, channel, password);
	chan = prepare_command(&server, channel, PC_SILENT);

	if (chan && get_cset_int_var(chan->csets, USERLIST_CSET) && Nick && (Nick->flags & ADD_UNBAN))
	{
		BanList *b = NULL;
		if (Nick->password && !password)
		{
			log_denied("UNBAN", from, FromUserHost, to, cmd);
			return NULL;
		}
		sprintf(ban, "%s!%s", from, FromUserHost);
		if (chan && chan->have_op)
		{
			if ((b = ban_is_on_channel(ban, chan)))
			{
				my_send_to_server(server, "MODE %s -b %s", channel, b->ban); 
				logmsg(LOG_CTCP, from, 0, "Successful %s from %s!%s on %s", "UNBAN", from, FromUserHost, channel);
			}
			else
			{
				if (get_int_var(SEND_CTCP_MSG_VAR))
					my_send_to_server(server, "NOTICE %s :\002%s\002: you %s are not banned on %s", from, version, ban, channel);
				log_denied("OPS", from, FromUserHost, to, cmd);
			}		
		} 
		else
		{
			my_send_to_server(server, "NOTICE %s :\002%s\002: I'm not on %s, or I'm not opped", from, version, channel);
			log_denied("OPS", from, FromUserHost, to, cmd);
		}
	}
	else if (!get_int_var(CLOAK_VAR) || (Nick && (Nick->flags & ADD_CTCP)))
	{
		if (!chan)
			my_send_to_server(server, "NOTICE %s :\002%s\002: I'm not on that channel", from, version);
		else
			my_send_to_server(server, "NOTICE %s :\002%s\002: Access Denied", from, version);
		log_denied("OPS", from, FromUserHost, to, cmd);
	}
#endif
	return NULL;
}



/*
 * do_sed: Performs the Simple Encrypted Data trasfer for ctcp.  Returns in a
 * malloc string the decryped message (if a key is set for that user) or the
 * text "[ENCRYPTED MESSAGE]" 
 */
CTCP_HANDLER(do_sed)
{
	char	*key = NULL,
		*crypt_who;
	char	*ret = NULL, *ret2 = NULL;

	if (*from == '=')
		crypt_who = from;
	if (my_stricmp(to, get_server_nickname(from_server)))
		crypt_who = to;
	else
		crypt_who = from;

	if ((key = is_crypted(crypt_who)))
		ret = decrypt_msg(cmd, key);

	if (!key || !ret)
		malloc_strcpy(&ret2, "[ENCRYPTED MESSAGE]");
	else
	{
		/* 
		 * There might be a CTCP message in there,
		 * so we see if we can find it.
		 */
		ret2 = m_strdup(do_ctcp(from, to, ret));
		sed = 1;
	}

	new_free(&ret);
	return ret2;
}

CTCP_HANDLER(do_utc)
{

	if (get_int_var(CLOAK_VAR))
		return m_strdup(empty_string);
	if (!cmd || !*cmd)
		return m_strdup(empty_string);

	return m_strdup(my_ctime(my_atol(cmd)));
}


/*
 * do_atmosphere: does the CTCP ACTION command --- done by lynX
 * Changed this to make the default look less offensive to people
 * who don't like it and added a /on ACTION. This is more in keeping
 * with the design philosophy behind IRCII
 */
CTCP_HANDLER(do_atmosphere)
{
	const char *old_message_from;
	unsigned long old_message_level;
	int ac_reply;
	char *ptr1 = cmd;
	
	if (!cmd || !*cmd)
		return NULL;
	
	if ((ac_reply = check_auto_reply(cmd)))
		addtabkey(from,"msg", 1);

	save_display_target(&old_message_from, &old_message_level);
	logmsg(LOG_ACTION, from, 0, "%s %s", to, cmd);
	
	if (is_channel(to))
	{
		int r = 0;
		ChannelList *chan = NULL;
		chan = lookup_channel(to, from_server, CHAN_NOUNLINK);
		do_logchannel(LOG_CTCP, chan, "%s %s %s %s", from, FromUserHost, to, cmd);
#ifdef WANT_USERLIST
		r = (lookup_userlevelc("*", FromUserHost, to, NULL)) ? 1 : 0;
#endif
		set_display_target(to, LOG_ACTION);
		if (do_hook(ACTION_LIST, "%s %s %s", from, to, cmd))
		{
			char *s;
	s = fget_string_var((ac_reply && r)?FORMAT_ACTION_USER_AR_FSET : 
		ac_reply ? FORMAT_ACTION_OTHER_AR_FSET : 
		r ? FORMAT_ACTION_USER_FSET :FORMAT_ACTION_OTHER_FSET);
			if (is_current_channel(to, from_server, 0))
			{
	s = fget_string_var((r && ac_reply)?FORMAT_ACTION_USER_AR_FSET: 
		(ac_reply) ? FORMAT_ACTION_AR_FSET:
		r ? FORMAT_ACTION_USER_FSET : FORMAT_ACTION_CHANNEL_FSET);
				put_it("%s", convert_output_format(s, "%s %s %s %s %s",update_clock(GET_TIME), from, FromUserHost, to, ptr1));
			}
			else
				put_it("%s", convert_output_format(s, "%s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, to, ptr1));
		}
	}
	else
	{
		set_display_target(from, LOG_ACTION);
		if (do_hook(ACTION_LIST, "%s %s %s", from, to, cmd))
			put_it("%s", convert_output_format(fget_string_var(ac_reply?FORMAT_ACTION_AR_FSET:FORMAT_ACTION_FSET), "%s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, to, ptr1));
	}

	restore_display_target(old_message_from, old_message_level);
	return NULL;
}

/*
 * do_dcc: Records data on an incoming DCC offer. Makes sure it's a
 *	user->user CTCP, as channel DCCs don't make any sense whatsoever
 */
CTCP_HANDLER(do_dcc)
{
	char	*type;
	char	*description = NULL;
	char	*inetaddr = NULL;
	char	*port = NULL;
	char	*size = NULL;
	char	*extra_flags = NULL;
	if (my_stricmp(to, get_server_nickname(from_server)))
		return NULL;
	if (!(type = next_arg(cmd, &cmd)))
		return NULL;
#if 1
	if (!(description = new_next_arg(cmd, &cmd)) || !*description)
		return NULL;
	if     (!(inetaddr = next_arg(cmd, &cmd)) ||
		!(port = next_arg(cmd, &cmd)))
			return NULL;

	size = next_arg(cmd, &cmd);
	extra_flags = next_arg(cmd, &cmd);
#else
	size = last_arg(&cmd);
	port = last_arg(&cmd);
	inetaddr = last_arg(&cmd);
	if (!size || !port || !inetaddr || !description)
		return NULL;
#endif
	register_dcc_type(from, type, description, inetaddr, port, size, extra_flags, FromUserHost, NULL);
	return NULL;
}

/*************** REPLY-GENERATING CTCPS *****************/

/*
 * do_clientinfo: performs the CLIENTINFO CTCP.  If cmd is empty, returns the
 * list of all CTCPs currently recognized by IRCII.  If an arg is supplied,
 * it returns specific information on that CTCP.  If a matching CTCP is not
 * found, an ERRMSG ctcp is returned 
 */
CTCP_HANDLER(do_clientinfo)
{
	int	i;
#ifdef WANT_DLL
	CtcpEntryDll *dll = NULL;
#endif	

	if (get_int_var(CLOAK_VAR))
		return NULL;
	if (cmd && *cmd)
	{
#ifdef WANT_DLL
		for (dll = dll_ctcp; dll; dll = dll->next)
		{
			if (!my_stricmp(cmd, dll->name))
			{
				send_ctcp(CTCP_NOTICE, from, CTCP_CLIENTINFO, 
					"%s %s", 
					dll->name, dll->desc?dll->desc:"none");
				return NULL;
			}
		}
#endif
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			if (!my_stricmp(cmd, ctcp_cmd[i].name))
			{
				send_ctcp(CTCP_NOTICE, from, CTCP_CLIENTINFO, 
					"%s %s", 
					ctcp_cmd[i].name, ctcp_cmd[i].desc);
				return NULL;
			}
		}
		send_ctcp(CTCP_NOTICE, from, CTCP_ERRMSG,
				"%s: %s is not a valid function",
				ctcp_cmd[CTCP_CLIENTINFO].name, cmd);
	}
	else
	{
		char buffer[BIG_BUFFER_SIZE + 1];
		*buffer = '\0';

		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			strmcat(buffer, ctcp_cmd[i].name, BIG_BUFFER_SIZE);
			strmcat(buffer, space, BIG_BUFFER_SIZE);
		}
#ifdef WANT_DLL
		for (dll = dll_ctcp; dll; dll = dll->next)
		{
			strmcat(buffer, dll->name, BIG_BUFFER_SIZE);
			strmcat(buffer, space, BIG_BUFFER_SIZE);
		}
#endif
		send_ctcp(CTCP_NOTICE, from, CTCP_CLIENTINFO,
			"%s :Use CLIENTINFO <COMMAND> to get more specific information", 
			buffer);
	}
	return NULL;
}

/* do_version: does the CTCP VERSION command */
CTCP_HANDLER(do_version)
{
	char	*tmp;
	char	*version_reply = NULL;
extern	char	tcl_versionstr[];
	/*
	 * The old way seemed lame to me... let's show system name and
	 * release information as well.  This will surely help out
	 * experts/gurus answer newbie questions.  -- Jake [WinterHawk] Khuon
	 *
	 * For the paranoid, UNAME_HACK hides the gory details of your OS.
	 */


#if defined(HAVE_UNAME) && !defined(UNAME_HACK)
	struct utsname un;
	char	*the_unix,
		*the_version;

	if (get_int_var(AUTOKICK_ON_VERSION_VAR))
	{
		char *channel = get_current_channel_by_refnum(0);
		if (channel)
		{
			ChannelList *chan;
			if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
			{
				NickList *nick;
				if ((nick = find_nicklist_in_channellist(from, chan, 0)) && !isme(from))
					send_to_server("KICK %s %s :%s", channel, from, "/VER is lame");
			}
		}
		return NULL;
	}
	if (get_int_var(CLOAK_VAR))
		return NULL;
	if (uname(&un) < 0)
	{
		the_version = empty_string;
		the_unix = "unknown";
	}
	else
	{
		the_version = un.release;
		the_unix = un.sysname;
	}
	malloc_strcpy(&version_reply, stripansicodes(convert_output_format(fget_string_var(FORMAT_VERSION_FSET), "%s %s %s %s %s", irc_version, internal_version, the_unix, the_version, tcl_versionstr)));
	send_ctcp(CTCP_NOTICE, from, CTCP_VERSION, "%s :%s", version_reply, 
#else
	if (get_int_var(AUTOKICK_ON_VERSION_VAR))
	{
		char *channel = get_current_channel_by_refnum(0);
		if (channel)
		{
			ChannelList *chan;
			if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
			{
				NickList *nick;
				if ((nick = find_nicklist_in_channellist(from, chan, 0)))
					send_to_server("KICK %s %s :%s", channel, from, "/VER is lame");
			}
		}
		return NULL;
	}
	if (get_int_var(CLOAK_VAR))
		return NULL;
	malloc_strcpy(&version_reply, stripansicodes(convert_output_format(fget_string_var(FORMAT_VERSION_FSET), "%s %s %s %s %s", irc_version, internal_version, "*IX", "ÿ", tcl_versionstr)));
	send_ctcp(CTCP_NOTICE, from, CTCP_VERSION, "%s :%s", version_reply, 
#endif
		(tmp = get_string_var(CLIENTINFO_VAR)) ?  tmp : IRCII_COMMENT);
	new_free(&version_reply);
	return NULL;
}

/* do_time: does the CTCP TIME command --- done by Veggen */
CTCP_HANDLER(do_time)
{


	if (get_int_var(CLOAK_VAR))
		return NULL;
	send_ctcp(CTCP_NOTICE, from, CTCP_TIME, 
			"%s", my_ctime(now));
	return NULL;
}

/* do_userinfo: does the CTCP USERINFO command */
CTCP_HANDLER(do_userinfo)
{


	if (get_int_var(CLOAK_VAR))
		return NULL;
	send_ctcp(CTCP_NOTICE, from, CTCP_USERINFO,
			"%s", get_string_var(USERINFO_VAR));
	return NULL;
}

/*
 * do_echo: does the CTCP ERRMSG and CTCP ECHO commands. Does
 * not send an error for ERRMSG and if the CTCP was sent to a channel.
 */
CTCP_HANDLER(do_echo)
{


	if (get_int_var(CLOAK_VAR))
		return NULL;
	if (!is_channel(to))
	{
		if (strlen(cmd) > 60)
		{
			yell("WARNING ctcp echo request longer than 60 chars. truncating");
			cmd[60] = 0;
		}
		send_ctcp(CTCP_NOTICE, from, ctcp->id, "%s", cmd);
	}
	return NULL;
}

CTCP_HANDLER(do_ping)
{


	if (get_int_var(CLOAK_VAR) == 2)
		return NULL;
	send_ctcp(CTCP_NOTICE, from, CTCP_PING, "%s", cmd ? cmd : empty_string);
	return NULL;
}


/* 
 * Does the CTCP FINGER reply 
 */
CTCP_HANDLER(do_finger)
{
	struct	passwd	*pwd;
	time_t	diff;
	char	*tmp;
	char	*ctcpuser,
		*ctcpfinger;
const	char	*my_host;


	if (get_int_var(CLOAK_VAR))
		return NULL;
	if ((my_host = get_server_userhost(from_server)) && strchr(my_host, '@'))
		my_host = strchr(my_host, '@') + 1;
	else
		my_host = hostname;
		
	diff = now - idle_time;

	if (!(pwd = getpwuid(getuid())))
		return NULL;

#ifndef GECOS_DELIMITER
#define GECOS_DELIMITER ','
#endif

	if ((tmp = strchr(pwd->pw_gecos, GECOS_DELIMITER)) != NULL)
		*tmp = '\0';

/* 
 * Three optionsn for handling CTCP replies
 *  + Fascist Bastard Way -- normal, non-hackable fashion
 *  + Winterhawk way (default) -- allows hacking through IRCUSER and
 *	IRCFINGER environment variables
 *  + hop way -- returns a blank always
 */
	/* 
	 * It would be pretty pointless to allow for customisable
	 * usernames if they can track via IRCNAME from the
	 * /etc/passwd file...  We therefore need to either disable
	 * CTCP_FINGER or also make it customisable.  Let's do the
	 * latter because it invokes less suspicion in the long run
	 *				-- Jake [WinterHawk] Khuon
	 */
	if ((ctcpuser = getenv("IRCUSER"))) 
		strmcpy(pwd->pw_name, ctcpuser, NAME_LEN);
	if ((ctcpfinger = getenv("IRCFINGER"))) 
		strmcpy(pwd->pw_gecos, ctcpfinger, NAME_LEN);

	send_ctcp(CTCP_NOTICE, from, CTCP_FINGER, 
		"%s (%s@%s) Idle %ld second%s", 
		pwd->pw_gecos, pwd->pw_name, my_host, diff, plural(diff));
	return NULL;
}


/* 
 * If we recieve a CTCP DCC REJECT in a notice, then we want to remove
 * the offending DCC request
 */
CTCP_HANDLER(do_dcc_reply)
{
	char *subcmd = NULL;
	char *type = NULL;

	if (is_channel(to))
		return NULL;

	if (cmd && *cmd)
		subcmd = next_arg(cmd, &cmd);
	if (cmd && *cmd)
		type = next_arg(cmd, &cmd);

	if (subcmd && type && cmd && !strcmp(subcmd, "REJECT"))
		dcc_reject (from, type, cmd);

	return NULL;
}


/*
 * Handles CTCP PING replies.
 */
CTCP_HANDLER(do_ping_reply)
{
	char *sec, *usec = NULL;
	struct timeval t;
	time_t tsec = 0, tusec = 0, orig;

	if (!cmd || !*cmd)
		return NULL;		/* This is a fake -- cant happen. */

	orig = my_atol(cmd);
	
	get_time(&t);
	if (orig < start_time || orig > t.tv_sec)
		return NULL;
	
       /* We've already checked 'cmd' here, so its safe. */
        sec = cmd;
	tsec = t.tv_sec - my_atol(sec);
        
	if ((usec = strchr(sec, ' ')))
	{
		*usec++ = 0;
		tusec = t.tv_usec - my_atol(usec);
	}
                        
	/*
	 * 'cmd', a variable passed in from do_notice_ctcp()
	 * points to a buffer which is MUCH larger than the
	 * string 'cmd' points at.  So this is safe, even
	 * if it looks "unsafe".
	 */
	sprintf(cmd, "%5.3f seconds", (float)(tsec + (tusec / 1000000.0)));
	return NULL;
}


/************************************************************************/
/*
 * do_ctcp: a re-entrant form of a CTCP parser.  The old one was lame,
 * so i took a hatchet to it so it didnt suck.
 */
extern 	char *do_ctcp (char *from, char *to, char *str)
{
	int 	flag;

	char 	local_ctcp_buffer [BIG_BUFFER_SIZE + 1],
		the_ctcp          [IRCD_BUFFER_SIZE + 1],
		last              [IRCD_BUFFER_SIZE + 1];
	char	*ctcp_command,
		*ctcp_argument;
	int	i;
	char	*ptr = NULL;
	int	allow_ctcp_reply = 1;

#ifdef WANT_DLL
	CtcpEntryDll  *dll = NULL;
#endif
	int delim_char = charcount(str, CTCP_DELIM_CHAR);

	if (delim_char < 2)
		return str;             /* No CTCPs. */

	if (delim_char > 8)
		allow_ctcp_reply = 0;   /* Historical limit of 4 CTCPs */


	flag = check_ignore(from, FromUserHost, to, IGNORE_CTCPS, NULL);

	in_ctcp_flag++;
	strmcpy(local_ctcp_buffer, str, IRCD_BUFFER_SIZE-2);

	for (;;strmcat(local_ctcp_buffer, last, IRCD_BUFFER_SIZE-2))
	{
		split_CTCP(local_ctcp_buffer, the_ctcp, last);

		if (!*the_ctcp)
			break;		/* all done! */
		/*
		 * Apply some integrety rules:
		 * -- If we've already replied to a CTCP, ignore it.
		 * -- If user is ignoring sender, ignore it.
		 * -- If we're being flooded, ignore it.
		 * -- If CTCP was a global msg, ignore it.
		 */

		/*
		 * Yes, this intentionally ignores "unlimited" CTCPs like
		 * UTC and SED.  Ultimately, we have to make sure that
		 * CTCP expansions dont overrun any buffers that might
		 * contain this string down the road.  So by allowing up to
		 * 4 CTCPs, we know we cant overflow -- but if we have more
		 * than 40, it might overflow, and its probably a spam, so
		 * no need to shed tears over ignoring them.  Also makes
		 * the sanity checking much simpler.
		 */
		if (!allow_ctcp_reply)
			continue;

		/*
		 * Check to see if the user is ignoring person.
		 */

		if (flag == IGNORED)
		{
			allow_ctcp_reply = 0;
			continue;
		}
					
		ctcp_command = the_ctcp;
		ctcp_argument = strchr(the_ctcp, ' ');
		if (ctcp_argument)
			*ctcp_argument++ = 0;
		else
			ctcp_argument = empty_string;

		/* Set up the window level/logging */
		if (im_on_channel(to, from_server))
			set_display_target(to, LOG_CTCP);
		else
			set_display_target(from, LOG_CTCP);

		/* Global messages -- just drop the CTCP */
		if (*to == '$' || (*to == '#' && !lookup_channel(to, from_server, 0)))
		{
			allow_ctcp_reply = 0;
			continue;
		}
#ifdef WANT_DLL
		/* Find the correct CTCP and run it. */
		for (dll = dll_ctcp; dll; dll = dll->next)
			if (!strcmp(dll->name, ctcp_command))
				break;  			
#endif
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
			if (!strcmp(ctcp_command, ctcp_cmd[i].name))
				break;

		/* Set up the window level/logging */
		if (im_on_channel(to, from_server))
			set_display_target(to, LOG_CTCP);
		else
			set_display_target(NULL, LOG_CTCP);

#ifdef WANT_DLL
		if (!dll && ctcp_cmd[i].id == CTCP_ACTION)
			check_flooding(from, CTCP_ACTION_FLOOD, ctcp_argument, is_channel(to)?to:NULL);
		else if (!dll && ctcp_cmd[i].id == CTCP_DCC)
			check_flooding(from, CTCP_FLOOD, ctcp_argument, is_channel(to)?to:NULL);
#else
		if (ctcp_cmd[i].id == CTCP_ACTION)
			check_flooding(from, CTCP_ACTION_FLOOD, ctcp_argument, is_channel(to)?to:NULL);
		else if (ctcp_cmd[i].id == CTCP_DCC)
			check_flooding(from, CTCP_FLOOD, ctcp_argument, is_channel(to)?to:NULL);
#endif
		else
		{
			check_flooding(from, CTCP_FLOOD, ctcp_argument, is_channel(to)?to:NULL);
			if (get_int_var(NO_CTCP_FLOOD_VAR) && (now - get_server_last_ctcp_time(from_server) < get_int_var(CTCP_DELAY_VAR)))
			{
				if (get_int_var(FLOOD_WARNING_VAR))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_FLOOD_FSET), "%s %s %s %s %s", update_clock(GET_TIME),ctcp_command,from, FromUserHost, to));
				set_server_last_ctcp_time(from_server, time(NULL));
				allow_ctcp_reply = 0;
				continue;
			}
		}
		/* Did the CTCP search work? */
#ifdef WANT_DLL
		if (i == NUMBER_OF_CTCPS && !dll)
#else
		if (i == NUMBER_OF_CTCPS)
#endif
		{
			/*
			 * Offer it to the user.
			 * Maybe they know what to do with it.
			 */
#ifdef WANT_TCL
			if (check_tcl_ctcp(from, FromUserHost, from, to, ctcp_command, ctcp_argument))
				continue;
#endif

			if (do_hook(CTCP_LIST, "%s %s %s %s", from, to, ctcp_command, ctcp_argument) && allow_ctcp_reply)
			{
				if (get_int_var(CTCP_VERBOSE_VAR))
				{
#ifdef WANT_USERLIST
					if (lookup_userlevelc("*", FromUserHost, "*", NULL))
						put_it("%s", convert_output_format(fget_string_var(get_int_var(CLOAK_VAR)? FORMAT_CTCP_CLOAK_UNKNOWN_USER_FSET:FORMAT_CTCP_UNKNOWN_USER_FSET),
							"%s %s %s %s %s %s",update_clock(GET_TIME), from, FromUserHost, to, ctcp_command, *ctcp_argument? ctcp_argument : empty_string));
					else
#endif
						put_it("%s", convert_output_format(fget_string_var(get_int_var(CLOAK_VAR)? FORMAT_CTCP_CLOAK_UNKNOWN_FSET:FORMAT_CTCP_UNKNOWN_FSET),
							"%s %s %s %s %s %s",update_clock(GET_TIME), from, FromUserHost, to, ctcp_command, *ctcp_argument? ctcp_argument : empty_string));
				}

				add_last_type(&last_ctcp[0], 1, from, FromUserHost, to, ctcp_command);
			}
			allow_ctcp_reply = 0;
			continue;
		}

#ifdef WANT_TCL
		check_tcl_ctcp(from, FromUserHost, from, to, ctcp_command, ctcp_argument);
#endif

#ifdef WANT_DLL
		if (dll)
			ptr = (dll->func) (dll, from, to, ctcp_argument);
		else
#endif
			ptr = ctcp_cmd[i].func(ctcp_cmd + i, from, to, ctcp_argument);

#ifdef WANT_DLL
		if (!(ctcp_cmd[i].flag & CTCP_NOLIMIT) || (dll && !(dll->flag & CTCP_NOLIMIT)))
#else
		if (!(ctcp_cmd[i].flag & CTCP_NOLIMIT))
#endif
		{
			set_server_last_ctcp_time(from_server, time(NULL));
			allow_ctcp_reply = 0;
		}

		/*
		 * We've only gotten to this point if its a valid CTCP
		 * query and we decided to parse it.
		 */

		/* If its an inline CTCP paste it back in */
#ifdef WANT_DLL
		if ((ctcp_cmd[i].flag & CTCP_INLINE) || (dll && (dll->flag & CTCP_INLINE)))
			strmcat(local_ctcp_buffer, ptr, BIG_BUFFER_SIZE);
#else
		if ((ctcp_cmd[i].flag & CTCP_INLINE))
			strmcat(local_ctcp_buffer, ptr, BIG_BUFFER_SIZE);
#endif
		/* If its interesting, tell the user. */
#ifdef WANT_DLL
		if ((ctcp_cmd[i].flag & CTCP_TELLUSER) || (dll && (dll->flag & CTCP_TELLUSER)))
#else
		if ((ctcp_cmd[i].flag & CTCP_TELLUSER))
#endif
		{
			if (do_hook(CTCP_LIST, "%s %s %s %s", from, to, ctcp_command, ctcp_argument))
			{
				if (get_int_var(CTCP_VERBOSE_VAR))
#ifdef WANT_USERLIST
					put_it("%s", convert_output_format(fget_string_var((!lookup_userlevelc("*",FromUserHost, "*", NULL))? get_int_var(CLOAK_VAR)?FORMAT_CTCP_CLOAK_FSET:FORMAT_CTCP_FSET:get_int_var(CLOAK_VAR)?FORMAT_CTCP_CLOAK_USER_FSET:FORMAT_CTCP_USER_FSET),
						"%s %s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, to, ctcp_command, *ctcp_argument? ctcp_argument : empty_string));
#else
					put_it("%s", convert_output_format(fget_string_var((get_int_var(CLOAK_VAR)?FORMAT_CTCP_CLOAK_FSET:FORMAT_CTCP_FSET)),
						"%s %s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, to, ctcp_command, *ctcp_argument? ctcp_argument : empty_string));
#endif
				add_last_type(&last_ctcp[0], 1, from, FromUserHost, to, ctcp_command);
			}
		}

		new_free(&ptr);
	}

	/* Reset the window level/logging */
	reset_display_target();

	in_ctcp_flag--;
	if (*local_ctcp_buffer)
		return strcpy(str, local_ctcp_buffer);
	else
		return empty_string;
}



/*
 * do_notice_ctcp: a re-entrant form of a CTCP reply parser.
 */
extern 	char *do_notice_ctcp (char *from, char *to, char *str)
{
	int 	flag;
	char 	local_ctcp_buffer [BIG_BUFFER_SIZE + 1],
		the_ctcp          [IRCD_BUFFER_SIZE + 1],
		last              [IRCD_BUFFER_SIZE + 1];
	char	*ctcp_command,
		*ctcp_argument;
	int	i;
	char	*ptr;
	char	*tbuf = NULL;
	int	allow_ctcp_reply = 1;

#ifdef WANT_DLL
	CtcpEntryDll  *dll = NULL;
#endif

	int delim_char = charcount(str, CTCP_DELIM_CHAR);

	if (delim_char < 2)
		return str;		/* No CTCPs. */
	if (delim_char > 8)
		allow_ctcp_reply = 0;	/* Ignore all the CTCPs. */

	flag = check_ignore(from, FromUserHost, to, IGNORE_CTCPS, NULL);
	if (!in_ctcp_flag)
		in_ctcp_flag = -1;

	tbuf = stripansi(str);
	strmcpy(local_ctcp_buffer, tbuf, IRCD_BUFFER_SIZE-2);
	new_free(&tbuf);
		
	for (;;strmcat(local_ctcp_buffer, last, IRCD_BUFFER_SIZE-2))
	{
		split_CTCP(local_ctcp_buffer, the_ctcp, last);
		if (!*the_ctcp)
			break;		/* all done! */

		if (!allow_ctcp_reply)
			continue;
			
		if (flag == IGNORED)
		{
			allow_ctcp_reply = 0;
			continue;
		}

		/* Global messages -- just drop the CTCP */
		if (*to == '$' || (*to == '#' && !lookup_channel(to, from_server, 0)))
		{
			allow_ctcp_reply = 0;
			continue;
		}

		ctcp_command = the_ctcp;
		ctcp_argument = strchr(the_ctcp, ' ');
		if (ctcp_argument)
			*ctcp_argument++ = 0;
		else
			ctcp_argument = empty_string;
		
#ifdef WANT_DLL
		/* Find the correct CTCP and run it. */
		for (dll = dll_ctcp; dll; dll = dll->next)
			if (!strcmp(dll->name, ctcp_command))
				break;  			
#endif
		/* Find the correct CTCP and run it. */
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
			if (!strcmp(ctcp_command, ctcp_cmd[i].name))
				break;

		/* 
		 * If we've already parsed one, and there is a limit
		 * on this CTCP, then just punt it.
		 */
#ifdef WANT_DLL
		if (i < NUMBER_OF_CTCPS && !dll && ctcp_cmd[i].repl)
#else
		if (i < NUMBER_OF_CTCPS && ctcp_cmd[i].repl)
#endif
		{
			if ((ptr = ctcp_cmd[i].repl(ctcp_cmd + i, from, to, ctcp_argument)))
			{
				strmcat(local_ctcp_buffer, ptr, BIG_BUFFER_SIZE);
				new_free(&ptr);
				continue;
			}
		}
#ifdef WANT_DLL
		if (dll && dll->repl)
		{
			if ((ptr = dll->repl(dll, from, to, ctcp_argument)))
			{
				strmcat(local_ctcp_buffer, ptr, BIG_BUFFER_SIZE);
				new_free(&ptr);
				continue;
			}
		}
#endif
#ifdef WANT_TCL
		check_tcl_ctcr(from, FromUserHost, from, to, ctcp_command, ctcp_argument?ctcp_argument:empty_string);
#endif
		/* Set up the window level/logging */
		set_display_target(from, LOG_CTCP);
		/* Toss it at the user.  */
		if (do_hook(CTCP_REPLY_LIST, "%s %s %s", from, ctcp_command, ctcp_argument))
		{
			put_it("%s", convert_output_format(fget_string_var(FORMAT_CTCP_REPLY_FSET),"%s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, ctcp_command, ctcp_argument));
			add_last_type(&last_ctcp_reply[0], 1, from, FromUserHost, ctcp_command, ctcp_argument);
		}

		allow_ctcp_reply = 0;
	}

	if (in_ctcp_flag == -1)
		in_ctcp_flag = 0;
	/* Reset the window level/logging */
	reset_display_target();

	return strcpy(str, local_ctcp_buffer);
}



/* in_ctcp: simply returns the value of the ctcp flag */
extern int	in_ctcp (void) { return (in_ctcp_flag); }



/*
 * This is no longer directly sends information to its target.
 * As part of a larger attempt to consolidate all data transmission
 * into send_text, this function was modified so as to use send_text().
 * This function can send both direct CTCP requests, as well as the
 * appropriate CTCP replies.  By its use of send_text(), it can send
 * CTCPs to DCC CHAT and irc nickname peers, and handles encryption
 * transparantly.  This greatly reduces the logic, complexity, and
 * possibility for error in this function.
 */
extern	void	send_ctcp (int type, char *to, int datatag, char *format, ...)
{
	char putbuf [BIG_BUFFER_SIZE + 1],
	     *putbuf2;
	int len;
	
	if ((len = IRCD_BUFFER_SIZE - (12 + strlen(to))) < 0)
		return;

	if (len < strlen(ctcp_cmd[datatag].name) + 3)
		return;
	
	putbuf2 = alloca(len);

	if (format)
	{
		va_list args;
		va_start(args, format);
		vsnprintf(putbuf, BIG_BUFFER_SIZE, format, args);
		va_end(args);
		
		do_hook(SEND_CTCP_LIST, "%s %s %s %s", 
				ctcp_type[type], to, 
				ctcp_cmd[datatag].name, putbuf);
		snprintf(putbuf2, len, "%c%s %s%c", 
				CTCP_DELIM_CHAR, 
				ctcp_cmd[datatag].name, putbuf, 
				CTCP_DELIM_CHAR);
	}
	else
	{
		do_hook(SEND_CTCP_LIST, "%s %s %s", 
				ctcp_type[type], to, 
				ctcp_cmd[datatag].name);
		snprintf(putbuf2, len, "%c%s%c", 
				CTCP_DELIM_CHAR, 
				ctcp_cmd[datatag].name, 
				CTCP_DELIM_CHAR);
	}

	putbuf2[len - 2] = CTCP_DELIM_CHAR;
	putbuf2[len - 1] = 0;
	send_text(to, putbuf2, ctcp_type[type], 0, 0);
}


/*
 * quote_it: This quotes the given string making it sendable via irc.  A
 * pointer to the length of the data is required and the data need not be
 * null terminated (it can contain nulls).  Returned is a malloced, null
 * terminated string.   
 */
extern 	char	*ctcp_quote_it (char *str, int len)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	*ptr;
	int	i;

	ptr = buffer;
	for (i = 0; i < len; i++)
	{
		switch (str[i])
		{
			case CTCP_DELIM_CHAR:	*ptr++ = CTCP_QUOTE_CHAR;
						*ptr++ = 'a';
						break;
			case '\n':		*ptr++ = CTCP_QUOTE_CHAR;
						*ptr++ = 'n';
						break;
			case '\r':		*ptr++ = CTCP_QUOTE_CHAR;
						*ptr++ = 'r';
						break;
			case CTCP_QUOTE_CHAR:	*ptr++ = CTCP_QUOTE_CHAR;
						*ptr++ = CTCP_QUOTE_CHAR;
						break;
			case '\0':		*ptr++ = CTCP_QUOTE_CHAR;
						*ptr++ = '0';
						break;
			default:		*ptr++ = str[i];
						break;
		}
	}
	*ptr = '\0';
	return m_strdup(buffer);
}

/*
 * ctcp_unquote_it: This takes a null terminated string that had previously
 * been quoted using ctcp_quote_it and unquotes it.  Returned is a malloced
 * space pointing to the unquoted string.  NOTE: a trailing null is added for
 * convenied, but the returned data may contain nulls!.  The len is modified
 * to contain the size of the data returned. 
 */
extern	char	*ctcp_unquote_it (char *str, int *len)
{
	char	*buffer;
	char	*ptr;
	char	c;
	int	i,
		new_size = 0;

	buffer = (char *) new_malloc((sizeof(char) * *len) + 1);
	ptr = buffer;
	i = 0;
	while (i < *len)
	{
		if ((c = str[i++]) == CTCP_QUOTE_CHAR)
		{
			switch (c = str[i++])
			{
				case CTCP_QUOTE_CHAR:
					*ptr++ = CTCP_QUOTE_CHAR;
					break;
				case 'a':
					*ptr++ = CTCP_DELIM_CHAR;
					break;
				case 'n':
					*ptr++ = '\n';
					break;
				case 'r':
					*ptr++ = '\r';
					break;
				case '0':
					*ptr++ = '\0';
					break;
				default:
					*ptr++ = c;
					break;
			}
		}
		else
			*ptr++ = c;
		new_size++;
	}
	*ptr = '\0';
	*len = new_size;
	return (buffer);
}

int get_ctcp_val (char *str)
{
	int i;

	for (i = 0; i < NUMBER_OF_CTCPS; i++)
		if (!strcmp(str, ctcp_cmd[i].name))
			return i;

	/*
	 * This is *dangerous*, but it works.  The only place that
	 * calls this function is edit.c:ctcp(), and it immediately
	 * calls send_ctcp().  So the pointer that is being passed
	 * to us is globally allocated at a level higher then ctcp().
	 * so it wont be bogus until some time after ctcp() returns,
	 * but at that point, we dont care any more.
	 */
	ctcp_cmd[CTCP_CUSTOM].name = str;
	return CTCP_CUSTOM;
}



/*
 * XXXX -- some may call this a hack, but if youve got a better
 * way to handle this job, id love to use it.
 */
void BX_split_CTCP(char *raw_message, char *ctcp_dest, char *after_ctcp)
{
	char *ctcp_start, *ctcp_end;

	*ctcp_dest = *after_ctcp = 0;
	ctcp_start = strchr(raw_message, CTCP_DELIM_CHAR);
	if (!ctcp_start)
		return;		/* No CTCPs present. */

	*ctcp_start++ = 0;
	ctcp_end = strchr(ctcp_start, CTCP_DELIM_CHAR);
	if (!ctcp_end)
	{
		*--ctcp_start = CTCP_DELIM_CHAR;
		return;		/* Thats _not_ a CTCP. */
	}

	*ctcp_end++ = 0;
	strmcpy(ctcp_dest, ctcp_start, IRCD_BUFFER_SIZE-2);
	strmcpy(after_ctcp, ctcp_end, IRCD_BUFFER_SIZE-2);

	return;		/* All done! */
}
