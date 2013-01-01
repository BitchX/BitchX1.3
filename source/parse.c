/*
 * parse.c: handles messages from the server.   Believe it or not.  I
 * certainly wouldn't if I were you. 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 * Modified Colten Edwards 1997
 */

#include "irc.h"
static char cvsrevision[] = "$Id: parse.c 214 2012-10-19 12:25:25Z keaston $";
CVS_REVISION(parse_c)
#include "struct.h"

#include "alias.h"
#include "server.h"
#include "names.h"
#include "vars.h"
#include "cdcc.h"
#include "ctcp.h"
#include "hook.h"
#include "log.h"
#include "commands.h"
#include "ignore.h"
#include "who.h"
#include "lastlog.h"
#include "input.h"
#include "ircaux.h"
#include "funny.h"
#include "encrypt.h"
#include "input.h"
#include "ircterm.h"
#include "flood.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "numbers.h"
#include "parse.h"
#include "notify.h"
#include "status.h"
#include "list.h"
#include "userlist.h"
#include "misc.h"
#include "whowas.h"
#include "timer.h"
#include "keys.h"
#include "hash2.h"
#include "cset.h"
#include "module.h"
#include "hash2.h"
#include "gui.h"
#include "tcl_bx.h"
#define MAIN_SOURCE
#include "modval.h"


#define STRING_CHANNEL '+'
#define MULTI_CHANNEL '#'
#define LOCAL_CHANNEL '&'
#define ID_CHANNEL '!'

static	void	strip_modes (char *, char *, char *);

	char *last_split_server = NULL;
	char *last_split_from = NULL;
	int in_server_ping = 0;

/*
 * joined_nick: the nickname of the last person who joined the current
 * channel 
 */
	char	*joined_nick = NULL;

/* public_nick: nick of the last person to send a message to your channel */
	char	*public_nick = NULL;

/* User and host information from server 2.7 */
	char	*FromUserHost = empty_string;

/* doing a PRIVMSG */
	int	doing_privmsg = 0;
	

#define do_output(x) put_it("%s%s%s", get_int_var(TIMESTAMP_VAR) ? update_clock(GET_TIME):empty_string, get_int_var(TIMESTAMP_VAR) ? space: empty_string, x)

/* returns ban if the ban is on the channel already, NULL if not */
BanList *ban_is_on_channel(register char *ban, register ChannelList *chan)
{
register BanList *bans = NULL;
#if 0
register BanList *eban = NULL;
#endif	
	for (bans = chan->bans; bans; bans = bans->next) 
	{
		if (wild_match(bans->ban, ban) || wild_match(ban, bans->ban))
			break;
		else if (!my_stricmp(bans->ban, ban))
			break;
	}
	return bans;
#if 0
	for (eban = chan->exemptbans; eban; eban = eban->next)
	{
		if (wild_match(eban->ban, ban) || wild_match(ban, eban->ban))
			break;
		else if (!my_stricmp(eban->ban, ban))
			break;
	}
	return eban ? NULL : bans;
#endif
}

BanList *eban_is_on_channel(register char *ban, register ChannelList *chan)
{
register BanList *eban = NULL;
	
	for (eban = chan->exemptbans; eban; eban = eban->next)
	{
		if (wild_match(eban->ban, ban) || wild_match(ban, eban->ban))
			break;
		else if (!my_stricmp(eban->ban, ban))
			break;
	}
	return eban ? eban : NULL;
}


void fake (void) 
{
	bitchsay("--- Fake Message recieved!!! ---");
	return;
}

int check_auto_reply(char *str)
{
char *p = NULL;
char *pat;
	if (!str || !*str || !get_int_var(AUTO_RESPONSE_VAR))
		return 0;
	p = alloca(strlen(auto_str)+1);
	strcpy(p, auto_str);
	if (p && *p)
	{
		while ((pat = next_arg(p, &p)))
		{
			switch(get_int_var(NICK_COMPLETION_TYPE_VAR))
			{
				case 3:
					if (!my_stricmp(str, pat))
						goto found_auto;
					continue;
				case 2:
					if (wild_match(pat, str))
						goto found_auto;
					continue;
				case 1:
					if (stristr(str, pat))
						goto found_auto;
					continue;
				default:
				case 0:
					if (!my_strnicmp(str, pat, strlen(pat)))
						goto found_auto;
					continue;
			}
		}
	}
	return 0;
found_auto:
#ifdef GUI
	gui_activity(COLOR_HIGHLIGHT);
#endif
	return 1;
}

int annoy_kicks(int list_type, char *to, char *from, char *ptr, NickList *nick)
{
int kick_em = 0;
ChannelList *chan;
	if (nick && (nick->userlist && nick->userlist->flags))
		return 0;
	if (!check_channel_match(get_string_var(PROTECT_CHANNELS_VAR), to) || !are_you_opped(to))
		return 0;
	if (!(chan = lookup_channel(to, from_server, CHAN_NOUNLINK)))
		return 0;
	if (get_cset_int_var(chan->csets, ANNOY_KICK_CSET) && !nick_isop(nick))
	{	
		char *buffer = NULL;
		if (char_fucknut(ptr, '\002', 12))
			malloc_sprintf(&buffer, "KICK %s %s :%s",to, from, "autokick for \002bold\002");
		else if (char_fucknut(ptr, '\007', 1))
			malloc_sprintf(&buffer, "KICK %s %s :%s", to, from, "autokick for beeping");
		else if (char_fucknut(ptr, '\003', 12))
			malloc_sprintf(&buffer, "KICK %s %s :%s", to, from, "autokick for \037mirc color\037");
		else if (char_fucknut(ptr, '\037', 0))
			malloc_sprintf(&buffer, "KICK %s %s :%s", to, from, "autokick for \037underline\037");
		else if (char_fucknut(ptr, '\026', 12))
			malloc_sprintf(&buffer, "KICK %s %s :%s", to, from, "autokick for \026inverse\026");
		else if (caps_fucknut(ptr))
			malloc_sprintf(&buffer, "KICK %s %s :%s", to, from, "autokick for CAPS LOCK");
		else if (strstr(ptr, "0000027fed"))
		{
			char *host = NULL, *p;
			malloc_strcpy(&host, FromUserHost);
			p = strchr(host, '@'); *p++ = '\0';
			send_to_server("MODE %s -o+b %s *!*%s", to, from, cluster(FromUserHost));
			send_to_server("KICK %s %s :%s", to, from, "\002Zmodem rocks\002");
			if (get_int_var(AUTO_UNBAN_VAR))
				add_timer(0, empty_string, get_int_var(AUTO_UNBAN_VAR) * 1000, 1, timer_unban, m_sprintf("%d %s *!*%s", from_server, to, cluster(FromUserHost)), NULL, current_window->refnum, "auto-unban");
			new_free(&host);
			kick_em = 1;
		}
		if (buffer)
		{
			kick_em = 1;
			send_to_server("%s", buffer);
			new_free(&buffer);
		}
	}

	if (ban_words)
	{
		WordKickList *word;
		int ops = get_cset_int_var(chan->csets, KICK_OPS_CSET);
		for (word = ban_words; word; word = word->next)
		{
			if (stristr(ptr, word->string))
			{
				if (wild_match(word->channel, to))
				{
					if (!ops && (nick_isop(nick) || nick_isvoice(nick)))
						break;
					send_to_server("KICK %s %s :%s %s", to, from, "\002BitchX BWK\002: ", word->string);
					kick_em = 1;
					break;
				}
			}
		}
	}
	return kick_em;
}

/*
 * is_channel: determines if the argument is a channel.  If it's a number,
 * begins with MULTI_CHANNEL and has no '*', or STRING_CHANNEL, then its a
 * channel 
 */
int BX_is_channel(char *to)
{
	if (!to || !*to)
		return 0;

	return ( (to) && ((*to == MULTI_CHANNEL)
					  || (*to == STRING_CHANNEL)
					  || (*to == ID_CHANNEL)
					  || (*to == LOCAL_CHANNEL)));
}


char	* BX_PasteArgs(char **Args, int StartPoint)
{
	int	i;

	for (; StartPoint; Args++, StartPoint--)
		if (!*Args)
			return NULL;
	for (i = 0; Args[i] && Args[i+1]; i++)
		Args[i][strlen(Args[i])] = ' ';
	Args[1] = NULL;
	return Args[0];
}

/*
 * BreakArgs: breaks up the line from the server, in to where its from,
 * setting FromUserHost if it should be, and returns all the arguements
 * that are there.   Re-written by phone, dec 1992.
 */
int BX_BreakArgs(char *Input, char **Sender, char **OutPut, int ig_sender)
{
	int	ArgCount = 0;

	/*
	 * The RFC describes it fully, but in a short form, a line looks like:
	 * [:sender[!user@host]] COMMAND ARGUMENT [[:]ARGUMENT]{0..14}
	 */

	/*
	 * Look to see if the optional :sender is present.
	 */
	if (!ig_sender)
	{
		if (*Input == ':')
		{
			*Sender = ++Input;
			while (*Input && *Input != *space)
				Input++;
			if (*Input == *space)
				*Input++ = 0;

		/*
		 * Look to see if the optional !user@host is present.
		 * look for @host only as services use it.
		 */
			FromUserHost = *Sender;
			while (*FromUserHost && *FromUserHost != '!' && *FromUserHost != '@')
				FromUserHost++;
			if (*FromUserHost == '!' || *FromUserHost == '@')
				*FromUserHost++ = 0;
		}
		/*
		 * No sender present.
		 */
		else
			*Sender = FromUserHost = empty_string;
	}
	/*
	 * Now we go through the argument list...
	 */
	for (;;)
	{
		while (*Input && *Input == *space)
			Input++;

		if (!*Input)
			break;

		if (*Input == ':')
		{
			OutPut[ArgCount++] = ++Input;
			break;
		}

		OutPut[ArgCount++] = Input;
		if (ArgCount >= MAXPARA)
			break;

		while (*Input && *Input != *space)
			Input++;
		if (*Input == *space)
			*Input++ = 0;
	}
	OutPut[ArgCount] = NULL;
	return ArgCount;
}

/* in response to a TOPIC message from the server */
static	void p_topic(char *from, char **ArgList)
{
ChannelList *tmp;

	
	if (!ArgList[1])
		{ fake(); return; }
	if ((tmp = lookup_channel(ArgList[0], from_server, CHAN_NOUNLINK)))
	{
		update_stats(TOPICLIST, find_nicklist_in_channellist(from, tmp, 0), tmp, 0);
		if (tmp->topic_lock)
		{
			if (my_stricmp(from, get_server_nickname(from_server)))
			{
				if (tmp->topic)
				{
					if (!ArgList[1] || my_stricmp(ArgList[1], tmp->topic))
						send_to_server("TOPIC %s :%s", tmp->channel, tmp->topic);
				}
			} else 
				malloc_strcpy(&tmp->topic, ArgList[1]);
		} else
			malloc_strcpy(&tmp->topic, ArgList[1]);
		add_last_type(&last_topic[0], 1, from, FromUserHost, tmp->channel, ArgList[1]);
		do_logchannel(LOG_TOPIC, tmp, "%s %s %s", from, ArgList[0], ArgList[1] ? ArgList[1] : empty_string);
	}
	if (tmp && check_ignore(from, FromUserHost, tmp->channel, IGNORE_TOPICS, NULL) != IGNORED)
	{
		set_display_target(ArgList[0], LOG_CRAP);
		if (do_hook(TOPIC_LIST, "%s %s %s", from, ArgList[0], ArgList[1]))
		{
			if (ArgList[1] && *ArgList[1])
			{
				if (fget_string_var(FORMAT_TOPIC_CHANGE_HEADER_FSET))
					put_it("%s",convert_output_format(fget_string_var(FORMAT_TOPIC_CHANGE_HEADER_FSET), "%s %s %s %s", update_clock(GET_TIME), from, ArgList[0], ArgList[1]));
				put_it("%s",convert_output_format(fget_string_var(FORMAT_TOPIC_CHANGE_FSET), "%s %s %s %s", update_clock(GET_TIME), from, ArgList[0], ArgList[1]));
			} else
				put_it("%s",convert_output_format(fget_string_var(FORMAT_TOPIC_UNSET_FSET), "%s %s %s", update_clock(GET_TIME), from, ArgList[0]));
			
		}
		reset_display_target();
	}
	logmsg(LOG_TOPIC, from, 0, "%s %s", ArgList[0], ArgList[1] ? ArgList[1]:empty_string);
#ifdef GUI
	xterm_settitle();
#endif
	update_all_status(current_window, NULL, 0);
}

static	void p_wallops(char *from, char **ArgList)
{
	char	*line;
	int	autorep = 0;
	int	from_server = strchr(from, '.') ? 1 : 0;

	
	if (!(line = PasteArgs(ArgList, 0)))
		{ fake(); return; }

	if (from_server || check_flooding(from, WALLOP_FLOOD,line, NULL))
	{
		/* The old server check, don't use the whois stuff for servers */
		int	level;
		char	*high;
		switch (check_ignore(from, FromUserHost, NULL, IGNORE_WALLOPS, NULL))
		{
		case (IGNORED):
			return;
		case (HIGHLIGHTED):
			high = highlight_char;
			break;
		default:
			high = empty_string;
			break;
		}
		set_display_target(from, LOG_WALLOP);
		level = set_lastlog_msg_level(LOG_WALLOP);
		autorep = check_auto_reply(line);
		add_last_type(&last_wall[0], MAX_LAST_MSG, from, NULL, from_server ? "S":"*", line);
		if (do_hook(WALLOP_LIST, "%s %c %s", from, from_server ? 'S': '*',line))
			put_it("%s",convert_output_format(fget_string_var(from_server? FORMAT_WALLOP_FSET: (autorep? FORMAT_WALL_AR_FSET:FORMAT_WALL_FSET)), 
			"%s %s %s %s", update_clock(GET_TIME),
			 from, from_server?"!":"*", line));
		if (beep_on_level & LOG_WALLOP)
			beep_em(1);
		logmsg(LOG_WALLOP, from, 0, "%s", line);
		set_lastlog_msg_level(level);
		reset_display_target();
	}
}

static	void p_privmsg(char *from, char **Args)
{
	int	level,
		list_type,
		flood_type,
		log_type,
		ar_true = 0,
		no_flood = 1,
		do_beep = 0;

	unsigned char	ignore_type;

	char	*ptr = NULL,
		*to,
		*high;

	static int com_do_log, com_lines = 0;

	ChannelList *channel = NULL;
	NickList *tmpnick = NULL;
	

	
	if (!from)
		return;
	PasteArgs(Args, 1);
	to = Args[0];
	ptr = Args[1];
	if (!to || !ptr)
		{ fake(); return; }
	doing_privmsg = 1;

	ptr = do_ctcp(from, to, ptr);
	if (!ptr || !*ptr)
	{
		doing_privmsg = 0;
		return;
	} 
	
	if (is_channel(to) && im_on_channel(to, from_server))
	{
		set_display_target(to, LOG_PUBLIC);
		malloc_strcpy(&public_nick, from);
		log_type = LOG_PUBLIC;
		ignore_type = IGNORE_PUBLIC;
		flood_type = PUBLIC_FLOOD;

		if (!is_on_channel(to, from_server, from))
			list_type = PUBLIC_MSG_LIST;
		else
		{
			if (is_current_channel(to, from_server, 0))
				list_type = PUBLIC_LIST;
			else
				list_type = PUBLIC_OTHER_LIST;
			channel = lookup_channel(to, from_server, CHAN_NOUNLINK);
			if (channel)
				tmpnick = find_nicklist_in_channellist(from, channel, 0);
		}
	}
	else
	{
		set_display_target(from, LOG_MSG);
		flood_type = MSG_FLOOD;
		if (my_stricmp(to, get_server_nickname(from_server)))
		{
			log_type = LOG_WALL;
			ignore_type = IGNORE_WALLS;
			list_type = MSG_GROUP_LIST;
			flood_type = WALL_FLOOD;
		}
		else
		{
			log_type = LOG_MSG;
			ignore_type = IGNORE_MSGS;
			list_type = MSG_LIST;
		}
	}
	switch (check_ignore(from, FromUserHost, to, ignore_type, ptr))
	{
	case IGNORED:
		if ((list_type == MSG_LIST) && get_int_var(SEND_IGNORE_MSG_VAR))
			send_to_server("NOTICE %s :%s is ignoring you", from, get_server_nickname(from_server));
		doing_privmsg = 0;
		return;
	case HIGHLIGHTED:
		high = highlight_char;
		break;
	case CHANNEL_GREP:
		high = highlight_char;
		break;
	default:
		high = empty_string;
		break;
	}

#ifdef WANT_TCL
	{
		int x = 0;
		char *cmd = NULL;
		switch(list_type)
		{
			case MSG_LIST:
			case MSG_GROUP_LIST:
			{
				char *ctcp_ptr;
				ctcp_ptr = LOCAL_COPY(ptr);
				cmd = next_arg(ctcp_ptr, &ctcp_ptr);
				x = check_tcl_msg(cmd, from, FromUserHost, from, ctcp_ptr);
				if (!x)
					check_tcl_msgm(cmd, from, FromUserHost, from, ctcp_ptr);
				break;
			}
			case PUBLIC_MSG_LIST:
			case PUBLIC_LIST:
			case PUBLIC_OTHER_LIST:
			{
				x = check_tcl_pub(from, FromUserHost, to, ptr);
				if (!x)
					check_tcl_pubm(from, FromUserHost, to, ptr);
				break;
			}
		}
	}
#endif
	update_stats(PUBLICLIST, tmpnick, channel, 0);

	level = set_lastlog_msg_level(log_type);
	com_do_log = 0;
	if (flood_type == PUBLIC_FLOOD)
	{
		int blah = 0;
		if (is_other_flood(channel, tmpnick, PUBLIC_FLOOD, &blah))
		{
			no_flood = 0;
			flood_prot(tmpnick->nick, FromUserHost, "PUBLIC", flood_type, get_cset_int_var(channel->csets, PUBFLOOD_IGNORE_TIME_CSET), channel->channel);
		}
	}
	else
		no_flood = check_flooding(from, flood_type, ptr, NULL);

	if (sed == 1)
	{
		if (do_hook(ENCRYPTED_PRIVMSG_LIST,"%s %s %s",from, to, ptr))
			put_it("%s",convert_output_format(fget_string_var(FORMAT_ENCRYPTED_PRIVMSG_FSET), "%s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, to, ptr));
			sed = 0;
	}
	else
	{
		int added_to_tab = 0;
		if (list_type == PUBLIC_LIST || list_type == PUBLIC_OTHER_LIST || list_type == PUBLIC_MSG_LIST)
		{
			if (check_auto_reply(ptr))
			{
				addtabkey(from, "msg", 1);
				com_do_log = 1;
				com_lines = 0;
				ar_true = 1;
				added_to_tab = 1;
			}
		}
		switch (list_type)
		{
		case PUBLIC_MSG_LIST:
		{
			if (!no_flood)
				break;
			if (do_hook(list_type, "%s %s %s", from, to, ptr))
			{
				logmsg(LOG_PUBLIC, from, 0, "%s %s", to, ptr);
				put_it("%s",convert_output_format(fget_string_var(ar_true?FORMAT_PUBLIC_MSG_AR_FSET:FORMAT_PUBLIC_MSG_FSET), "%s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, to, ptr));
				do_beep = 1;
			}
			break;
		}
		case MSG_GROUP_LIST:
		{
			if (!no_flood)
				break;
			if (do_hook(list_type, "%s %s %s", from, to, ptr))
			{
				logmsg(LOG_PUBLIC, from, 0,"%s %s", FromUserHost, ptr);
				put_it("%s", convert_output_format(fget_string_var(FORMAT_MSG_GROUP_FSET), "%s %s %s %s", update_clock(GET_TIME), from, to, ptr));
				do_beep = 1;
			}
			break;
		}
		case MSG_LIST:
		{
			if (!no_flood)
				break;
			set_server_recv_nick(from_server, from);
#ifdef WANT_CDCC
			if ((msgcdcc(from, to, ptr)) == NULL)
				break;
#endif
			if (!strncmp(ptr, "PASS", 4) && change_pass(from, ptr))
				break;
			if (forwardnick)
				send_to_server("NOTICE %s :*%s* %s", forwardnick, from, ptr);

			if (do_hook(list_type, "%s %s", from, ptr))
			{
				if (get_server_away(from_server))
				{
					do_beep = 0;
					beep_em(get_int_var(BEEP_WHEN_AWAY_VAR));
					set_int_var(MSGCOUNT_VAR, get_int_var(MSGCOUNT_VAR)+1);
				}
				else
					do_beep = 1;
				put_it("%s", convert_output_format(fget_string_var(FORMAT_MSG_FSET), "%s %s %s %s", update_clock(GET_TIME), from, FromUserHost, ptr));
				if (!added_to_tab)
					addtabkey(from, "msg", 0);
				logmsg(LOG_MSG, from,  0,"%s %s", FromUserHost, ptr);
			}
			add_last_type(&last_msg[0], MAX_LAST_MSG, from, FromUserHost, to, ptr);
			if (get_server_away(from_server) && get_int_var(SEND_AWAY_MSG_VAR))
			{
				if (!check_last_type(&last_msg[0], MAX_LAST_MSG, from, FromUserHost))
					my_send_to_server(from_server, "NOTICE %s :%s", from, stripansicodes(convert_output_format(fget_string_var(FORMAT_SEND_AWAY_FSET), "%l %l %s", now, get_server_awaytime(from_server), get_int_var(MSGLOG_VAR)?"On":"Off")));
			}
			break;
		}
		case PUBLIC_LIST:
		{
			if (!no_flood)
				break;
               		annoy_kicks(list_type, to, from, ptr, tmpnick);
			if (ar_true)
				list_type = PUBLIC_AR_LIST;
			if (do_hook(list_type, "%s %s %s", from, to, ptr))
			{
				logmsg(LOG_PUBLIC, from, 0,"%s %s", to, ptr);
				do_logchannel(LOG_PUBLIC, channel, "%s", convert_output_format(fget_string_var((list_type == PUBLIC_AR_LIST)? FORMAT_PUBLIC_AR_FSET:FORMAT_PUBLIC_FSET), "%s %s %s %s", update_clock(GET_TIME), from, to, ptr));
				put_it("%s", convert_output_format(fget_string_var((list_type == PUBLIC_AR_LIST)? FORMAT_PUBLIC_AR_FSET:FORMAT_PUBLIC_FSET), "%s %s %s %s", update_clock(GET_TIME), from, to, ptr));
				do_beep = 1;
			}
			break;
		}
		case PUBLIC_OTHER_LIST:
		{
			if (!no_flood)
				break;
                	annoy_kicks(list_type, to, from, ptr, tmpnick);
			if (ar_true)
				list_type = PUBLIC_OTHER_AR_LIST;
			if (do_hook(list_type, "%s %s %s", from, to, ptr))
			{
				logmsg(LOG_PUBLIC, from, 0,"%s %s", to, ptr);
				do_logchannel(LOG_PUBLIC, channel, "%s", convert_output_format(fget_string_var(list_type==PUBLIC_OTHER_AR_LIST?FORMAT_PUBLIC_OTHER_AR_FSET:FORMAT_PUBLIC_OTHER_FSET), "%s %s %s %s", update_clock(GET_TIME), from, to, ptr));
				put_it("%s", convert_output_format(fget_string_var(list_type==PUBLIC_OTHER_AR_LIST?FORMAT_PUBLIC_OTHER_AR_FSET:FORMAT_PUBLIC_OTHER_FSET), "%s %s %s %s", update_clock(GET_TIME), from, to, ptr));
				do_beep = 1;
			}
			break;
		} /* case */
		} /* switch */
	}

	if ((beep_on_level & log_type) && do_beep)
		beep_em(1);

	if (no_flood)
		grab_http(from, to, ptr);
	set_lastlog_msg_level(level);
	reset_display_target();
	doing_privmsg = 0;
}

static	void p_quit(char *from, char **ArgList)
{
	int	one_prints = 0;
	char *reason;
	char *chanlist = NULL;
	ChannelList *chan;
	int netsplit = 0;
	int ignore;
				
	PasteArgs(ArgList, 0);
	if (ArgList[0])
	{
		reason = ArgList[0];
		netsplit = check_split(from, reason);
	}
	else
		reason = "?";

	for (chan = walk_channels(from, 1, from_server); chan; 
		chan = walk_channels(from, 0, -1))
	{
		update_stats(CHANNELSIGNOFFLIST, 
			find_nicklist_in_channellist(from, chan, 0), chan, netsplit);

#ifdef WANT_TCL
		if (netsplit)
			check_tcl_split(from, FromUserHost, from, chan->channel);
		else
			check_tcl_sign(from, FromUserHost, from, chan->channel, reason);
#endif

		if (!netsplit)
		{
			do_logchannel(LOG_PART, chan, "%s %s %s %s", from, FromUserHost, 
				chan->channel, reason);
			check_channel_limit(chan);
		}

		if (chanlist)
			m_3cat(&chanlist, ",", chan->channel);
		else
			malloc_strcpy(&chanlist, chan->channel);

		ignore = check_ignore(from, FromUserHost, chan->channel, 
			(netsplit?IGNORE_SPLITS:IGNORE_QUITS), NULL);
		if (ignore != IGNORED)
		{
			set_display_target(chan->channel, LOG_CRAP);
			if (do_hook(CHANNEL_SIGNOFF_LIST, "%s %s %s", chan->channel, 
				from, reason))
				one_prints = 1;
		}
	}

	if (one_prints)
	{
		char *channel = what_channel(from, from_server);
		ignore = check_ignore(from, FromUserHost, channel, 
			(netsplit?IGNORE_SPLITS:IGNORE_QUITS), NULL);
		set_display_target(channel, LOG_CRAP);
		if ((ignore != IGNORED) && do_hook(SIGNOFF_LIST, "%s %s", from, reason)
			&& !netsplit)
			put_it("%s", convert_output_format(
				fget_string_var(FORMAT_CHANNEL_SIGNOFF_FSET), 
				"%s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, 
				chanlist, reason));
	}

	logmsg(LOG_PART, from, 0, "%s %s", chanlist ? chanlist : "<NONE>", reason);
	check_orig_nick(from);
	notify_mark(from, FromUserHost, 0, 0);
	remove_from_channel(NULL, from, from_server, netsplit, reason);
	update_all_status(current_window, NULL, 0);
	new_free(&chanlist);
	reset_display_target();
#ifdef GUI
	gui_update_nicklist(NULL);
#endif
}

static	void p_pong(char *from, char **ArgList)
{
	int is_server = 0;
	int i;	

	
	if (!ArgList[0])
		return;
	is_server = wild_match("*.*", ArgList[0]);
	if (in_server_ping && is_server)
	{
		int old_from_server = from_server;
		for (i = 0; i < server_list_size(); i++)
		{
			if ((!my_stricmp(ArgList[0], get_server_name(i)) || !my_stricmp(ArgList[0], get_server_itsname(i))) && is_server_open(i))
			{
				int old_lag = get_server_lag(i);
				from_server = i;
				set_server_lag(i, now - get_server_lagtime(i));
				in_server_ping--;
				if (old_lag != get_server_lag(i))
					status_update(1);
				from_server = old_from_server;
				return;
			}
		}
		from_server = old_from_server;
	}
	if (check_ignore(from, FromUserHost, NULL, IGNORE_PONGS, NULL) != IGNORED)
	{
		if (!is_server)
			return;
		reset_display_target();
		if (!ArgList[1])
			say("%s: PONG received from %s", ArgList[0], from);
		else if (!strncmp(ArgList[1], "LAG", 3))
		{
			char *p = empty_string;
			char buff[50];
			struct timeval timenow = {0};
			struct timeval timethen;
#ifdef HAVE_GETTIMEOFDAY
			if ((p = strchr(ArgList[1], '.')))
			{
				*p++ = 0;
				timethen.tv_usec = my_atol(p);
			} else
				timethen.tv_usec = 0;
			timethen.tv_sec = my_atol(ArgList[1]+3);
#else
			timethen.tv_sec = my_atol(ArgList[1]+3);
#endif
			get_time(&timenow);
			sprintf(buff, "%2.4f", BX_time_diff(timethen, timenow));
			put_it("%s", convert_output_format("$G Server pong from %W$0%n $1 seconds", "%s %s", ArgList[0], buff));
			clear_server_sping(from_server, ArgList[0]);
		}
		else if (!my_stricmp(ArgList[1], get_server_nickname(from_server)))
		{
			char buff[50];
			Sping *tmp;
			if ((tmp = get_server_sping(from_server, ArgList[0])))
			{
#ifdef HAVE_GETTIMEOFDAY
				struct timeval timenow = {0};
				get_time(&timenow);
				sprintf(buff, "%2.4f", BX_time_diff(tmp->in_sping, timenow));
				put_it("%s", convert_output_format("$G Server pong from %W$0%n $1 seconds", "%s %s", ArgList[0], buff));
#else
				sprintf(buff, "%2ld.x", now - tmp->in_sping);
				put_it("%s", convert_output_format("$G Server pong from %W$0%n $1 seconds", "%s %s", ArgList[0], buff));
#endif
				clear_server_sping(from_server, ArgList[0]);
				if (is_server_connected(from_server))
				{
					int old_lag = get_server_lag(from_server);
					set_server_lag(from_server, now - get_server_lagtime(from_server));
					if (old_lag != get_server_lag(from_server))
						status_update(1);
				}
			}
		}
		else
			say("%s: PING received from %s %s", ArgList[0], from, ArgList[1]);
	}
	return;
}
		

static	void p_error(char *from, char **ArgList)
{

	PasteArgs(ArgList, 0);
	if (!ArgList[0])
	{
		fake();
		return;
	}

	say("%s", ArgList[0]);
}

/*
 * This only handles negotiating the SASL capability with the PLAIN method. It would
 * be good to add DH-BLOWFISH, and later, full capability support.
 */
static	void p_cap(char *from, char **ArgList)
{
	char *caps, *p;

	if (!strcmp(ArgList[1], "ACK"))
	{
		caps = LOCAL_COPY(ArgList[2]);
		while ((p = next_arg(caps, &caps)) != NULL)
		{
			/* Only AUTHENTICATE before registration */
			if (!strcmp(p, "sasl") && !is_server_connected(from_server))
			{
				my_send_to_server(from_server, "AUTHENTICATE PLAIN");
				break;
			}
		}
	}
	else if (!strcmp(ArgList[1], "NAK"))
	{
		caps = LOCAL_COPY(ArgList[2]);
		while ((p = next_arg(caps, &caps)) != NULL)
		{
			/* End capability negotiation to continue registration */
			if (!strcmp(p, "sasl") && !is_server_connected(from_server))
			{
				my_send_to_server(from_server, "CAP END");
				break;
			}
		}
	}
}

static	void p_authenticate(char *from, char **ArgList)
{
	char buf[512];
	char *output = NULL;
	char *nick, *pass;

	/* "AUTHENTICATE command MUST be used before registration is complete" */
	if (is_server_connected(from_server))
		return;

	if (!strcmp(ArgList[0], "+"))
	{
		nick = get_server_sasl_nick(from_server);
		pass = get_server_sasl_pass(from_server);

		/* "The client can abort an authentication by sending an asterisk as the data" */
		if (!nick || !pass)
		{
			my_send_to_server(from_server, "AUTHENTICATE *");
			return;
		}

		strlcpy(buf, nick, sizeof buf);
		strlcpy(buf + strlen(nick) + 1, nick, sizeof buf);
		strlcpy(buf + strlen(nick) * 2 + 2, pass, sizeof buf);

		if (my_base64_encode(buf, strlen(nick) * 2 + strlen(pass) + 2, &output) != -1)
		{
			my_send_to_server(from_server, "AUTHENTICATE %s", output);
// XXX			new_free(&output);
			free(output);
		}
		else
			my_send_to_server(from_server, "AUTHENTICATE *");
	}
}

void add_user_who (WhoEntry *w, char *from, char **ArgList)
{
	char *userhost;
	ChannelList *chan;
	int op = 0, voice = 0;

	/* Obviously this is safe. */
	userhost = alloca(strlen(ArgList[1]) + strlen(ArgList[2]) + 2);
	sprintf(userhost, "%s@%s", ArgList[1], ArgList[2]);
	voice = (strchr(ArgList[5], '+') != NULL);
	op = (strchr(ArgList[5], '@') != NULL);
	chan = add_to_channel(ArgList[0], ArgList[4], from_server, op, voice, userhost, ArgList[3], ArgList[5], 0, ArgList[6] ? my_atol(ArgList[6]) : 0);
#ifdef WANT_NSLOOKUP
	if (get_int_var(AUTO_NSLOOKUP_VAR))
		do_nslookup(ArgList[2], ArgList[4], ArgList[1], ArgList[0], from_server, auto_nslookup, NULL);
#endif	
}

void add_user_end (WhoEntry *w, char *from, char **ArgList)
{
	got_info(ArgList[0], from_server, GOTWHO);
	/* Nothing to do! */
}

static	void p_channel(char *from, char **ArgList)
{
	char	*channel;
	ChannelList *chan = NULL;
	NickList *tmpnick = NULL;		
	WhowasList *whowas = NULL;
	int	its_me = 0;				
	int op = 0, vo = 0;
	char	extra[80];
	register char *c;
	Window *old_window = current_window;
	int switched = 0;
	irc_server *irc_serv = NULL;
	
	
	if (!strcmp(ArgList[0], zero))
	{
		fake();
		return;
	}

	channel = ArgList[0];
	set_display_target(channel, LOG_CRAP);
	malloc_strcpy(&joined_nick, from);

	/*
	 * Workaround for gratuitous protocol change in ef2.9
	 */
	*extra = 0;
	if ((c = strchr(channel, '\007')))
	{
		for (*c++ = 0; *c; c++)
		{
			     if (*c == 'o') op = 1;
			else if (*c == 'v') vo = 1;
		}
	}
	if (op)
		strcat(extra, " (+o)");
	if (vo)
		strcat(extra, " (+v)");
                                                
	if (!my_stricmp(from, get_server_nickname(from_server)))
	{
		int refnum;
		if (!in_join_list(channel, from_server))
		{
			add_to_join_list(channel, from_server, current_window->refnum);
			refnum = current_window->refnum;
		}
		else
		{

			if (current_window->refnum != (refnum = get_win_from_join_list(channel, from_server)))
			{
				switched = 1;
				make_window_current(get_window_by_refnum(refnum));
			}
		}

		its_me = 1;
		chan = add_channel(channel, from_server, refnum);
		do_hook(JOIN_ME_LIST, "%s %d", channel, refnum);
		if (*channel == '+')
		{
			got_info(channel, from_server, GOTBANS);
			got_info(channel, from_server, GOTMODE);
			if ((get_server_version(from_server) == Server2_8ts4) 
				|| (get_server_version(from_server) == Server2_10)) 
				got_info(channel, from_server, GOTEXEMPT); 
		}
		else
		{
			int ver = get_server_version(from_server);
			if ((ver == Server2_8ts4) || (ver == Server2_10))
				send_to_server("MODE %s\r\nMODE %s b\r\nMODE %s e", channel, channel, channel);
			else
				send_to_server("MODE %s\r\nMODE %s b", channel, channel);
		}
                whobase(channel, add_user_who, add_user_end, NULL);
	}
	else 
	{
		if ((whowas = check_whosplitin_buffer(from, FromUserHost, channel, 0)))
			irc_serv = check_split_server(whowas->server1);
		chan = add_to_channel(channel, from, from_server, op, vo, FromUserHost, NULL, NULL, whowas && irc_serv ? 1 : 0, 0);
		if (whowas && whowas->server2 && irc_serv)
			new_free(&whowas->server2);

#ifdef WANT_TCL
		check_tcl_join(from, FromUserHost, from, channel);
#endif
		logmsg(LOG_JOIN, from, 0, "%s %s %s", FromUserHost, channel, extra);
		do_logchannel(LOG_JOIN, chan, "%s, %s %s %s", from, FromUserHost, channel, extra);
		if (!irc_serv)
			check_channel_limit(chan);
	}
			
#ifdef WANT_USERLIST
	if (!in_join_list(channel, from_server) && chan)
		tmpnick = check_auto(channel, find_nicklist_in_channellist(from, chan, 0), chan);
#endif
	flush_mode_all(chan);

	if (tmpnick && !tmpnick->ip && get_int_var(AUTO_NSLOOKUP_VAR))
	{
		char *user;
#ifdef WANT_NSLOOKUP
		char *host;
#endif
		user = alloca(strlen(FromUserHost)+1);
		strcpy(user, FromUserHost);

#ifdef WANT_NSLOOKUP
		if ((host = strchr(user, '@')))
		{
			*host++ = 0;
			do_nslookup(host, from, user, channel, from_server, auto_nslookup, NULL);
		}
#endif
	}
	set_display_target(channel, LOG_CRAP);
	if (whowas)
	{
		if (irc_serv)
		{
		 	if ((do_hook(LLOOK_JOIN_LIST, "%s %s", irc_serv->name, irc_serv->link)))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NETJOIN_FSET), "%s %s %s %d", update_clock(GET_TIME), irc_serv->name, irc_serv->link, 0));
			remove_split_server(CHAN_SPLIT, irc_serv->name);
		}
#ifdef WANT_TCL
		check_tcl_rejoin(from, FromUserHost, from, channel);
#endif
	}
	if (check_ignore(from, FromUserHost, channel, IGNORE_JOINS, NULL) != IGNORED && chan)
	{
		if (do_hook(JOIN_LIST, "%s %s %s %s", from, channel, FromUserHost? FromUserHost : "UnKnown", extra))
		{
			if (chan && (tmpnick = find_nicklist_in_channellist(from, chan, 0)))
			{
				if (tmpnick->userlist)
					put_it("%s",convert_output_format(fget_string_var(FORMAT_FRIEND_JOIN_FSET), "%s %s %s %s %s %s",update_clock(GET_TIME),from,FromUserHost?FromUserHost:"UnKnown",channel, tmpnick->userlist?(tmpnick->userlist->comment?tmpnick->userlist->comment:empty_string):empty_string, extra));
				else 
					put_it("%s",convert_output_format(fget_string_var(FORMAT_JOIN_FSET), "%s %s %s %s %s",update_clock(GET_TIME),from,FromUserHost?FromUserHost:"UnKnown",channel, extra));
			}
			else 
				put_it("%s",convert_output_format(fget_string_var(FORMAT_JOIN_FSET), "%s %s %s %s %s",update_clock(GET_TIME),from,FromUserHost?FromUserHost:"UnKnown",channel, extra));

			if (!its_me && chan && chan->have_op)
			{
				char lame_chars[] = "\x01\x02\x03\x04\x05\x06\x07\x08\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a";
				register char *p;
				if (get_cset_int_var(chan->csets, LAMELIST_CSET))
				{
					if (lame_list && find_in_list((List **)&lame_list, from, 0))
					{
						send_to_server("MODE %s -o+b %s %s!*", chan->channel, from, from);
						send_to_server("KICK %s %s :\002Lame Nick detected\002", chan->channel, from);
						if (get_int_var(AUTO_UNBAN_VAR))
							add_timer(0, empty_string, get_int_var(AUTO_UNBAN_VAR) * 1000, 1, timer_unban, m_sprintf("%d %s %s!*", from_server, chan->channel, from), NULL, current_window->refnum, "auto-unban");
					}
				}
				if (get_cset_int_var(chan->csets, LAMEIDENT_CSET))
				{
					for (p = FromUserHost; *p; p++)
					{
						char *user, *host;
						if (!strchr(lame_chars, *p))
							continue;
						user = LOCAL_COPY(FromUserHost);
						host = strchr(FromUserHost, '@');
						host++;
						send_to_server("MODE %s +b *!*@%s\r\nKICK %s %s :\002Lame Ident detected\002", chan->channel, cluster(host), chan->channel, from);
						break;
					}
				}
			}
		}
	}
	reset_display_target();
#ifdef GUI
	gui_update_nicklist(channel);
#endif
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
	update_all_status(current_window, NULL, 0);
	notify_mark(from, FromUserHost, 1, 0);
	if (switched)
		make_window_current(old_window);
}

void check_auto_join(int server, char *from, char *channel, char *key)
{
ChannelList *chan = NULL;
WhowasChanList *w_chan = NULL;
UserList *u = NULL;
CSetList *cset = NULL;
	if (in_join_list(channel, from_server))
		return;
	if ((w_chan = check_whowas_chan_buffer(channel, -1, 0)))
	{
		chan = w_chan->channellist;
#ifdef WANT_USERLIST
		if (((get_cset_int_var(chan->csets, AUTO_REJOIN_CSET)) || (!chan && get_int_var(AUTO_REJOIN_VAR))) && (channel && ((u = lookup_userlevelc(from, FromUserHost, channel, NULL)) != NULL)))
		{
			if ((u->flags & ADD_BOT))
				goto got_request;
		}
		else
#endif
		if (get_cset_int_var(chan->csets, AUTO_JOIN_ON_INVITE_CSET))
			goto got_request;
	} 
	else if ((cset = (CSetList *) check_cset_queue(channel, 0)))
	{
		if (get_cset_int_var(cset, AUTO_JOIN_ON_INVITE_CSET))
			goto got_request;
	}
	return;
got_request:
	bitchsay("Auto-joining %s on invite", channel);
	add_to_join_list(channel, from_server, current_window->refnum);
	send_to_server("JOIN %s", channel);

}

static	void p_invite(char *from, char **ArgList)
{
	char	*high;

	
	switch (check_ignore(from, FromUserHost, ArgList[1] ? ArgList[1] : NULL, IGNORE_INVITES, NULL))
	{
		case IGNORED:
			if (get_int_var(SEND_IGNORE_MSG_VAR))
				send_to_server("NOTICE %s :%s is ignoring you",
					from, get_server_nickname(from_server));
			return;
		case HIGHLIGHTED:
			high = highlight_char;
			break;
		default:
			high = empty_string;
			break;
	}
	if (ArgList[0] && ArgList[1])
	{
		ChannelList *chan = NULL;
		set_display_target(from, LOG_CRAP);
		malloc_strcpy(&invite_channel, ArgList[1]);
		if (check_flooding(from, INVITE_FLOOD, ArgList[1], NULL) && 
		   do_hook(INVITE_LIST, "%s %s %s", from, ArgList[1], ArgList[2]?ArgList[2]:empty_string))
		{
			char *s;
			put_it("%s", convert_output_format(fget_string_var(FORMAT_INVITE_FSET), "%s %s %s",update_clock(GET_TIME), from, ArgList[1]));
			if ((s = convert_to_keystr("JOIN_LAST_INVITE")) && *s)
			{
				if (!get_int_var(AUTO_JOIN_ON_INVITE_VAR))
				{
					if (ArgList[2])
						bitchsay("Press %s to join %s (%s)", s, invite_channel, ArgList[2]);
					else
						bitchsay("Press %s to join %s", s, invite_channel);
				}
			}
			logmsg(LOG_INVITE, from, 0, "%s", invite_channel);
		}
		if (!(chan = lookup_channel(invite_channel, from_server, 0)))
			check_auto_join(from_server, from, invite_channel, ArgList[2]);
		add_last_type(&last_invite_channel[0], 1, from, FromUserHost, ArgList[1], ArgList[2]?ArgList[2]:empty_string);
		reset_display_target();
	}
}

static void p_silence (char *from, char **ArgList)
{
	char *target = ArgList[0];
	char *mag = target++;

	
	if (do_hook(SILENCE_LIST, "%c %s", *mag, target))
		put_it("%s", convert_output_format(fget_string_var(FORMAT_SILENCE_FSET), "%s %c %s", update_clock(GET_TIME), *mag, target));
}


static	void p_kill(char *from, char **ArgList)
{
	int 	port;
	int 	localkill;
	int 	serverkill = strchr(from, '.') != NULL;
	int 	next_server;
	char	sc[20];

	/*
	 * Bogorific Microsoft Exchange ``IRC'' server sends out a KILL
	 * protocol message instead of a QUIT protocol message when
	 * someone is killed on your server.  Do the obviously appropriate
	 * thing and reroute this misdirected protocol message to
	 * p_quit, where it should have been sent in the first place.
	 * Die Microsoft, Die.
	 */
	if (!isme(ArgList[0]))
	{
		/* I don't care if this doesn't work.  */
		p_quit(from, ArgList);  /* Die Microsoft, Die */
		return;
	}

	port = get_server_port(from_server);
	snprintf(sc, 19, "+%i %d", from_server, port);
	
	localkill = !serverkill && ArgList[1] && 
		strstr(ArgList[1], get_server_name(from_server));

	next_server = localkill && get_int_var(NEXT_SERVER_ON_LOCAL_KILL_VAR);
	
	if (serverkill || (get_int_var(AUTO_RECONNECT_VAR) && !next_server))
		set_server_reconnecting(from_server, 1);
	    
	close_server(from_server,empty_string);
	clean_server_queues(from_server);
	window_check_servers(from_server);
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
	if (serverkill)
	{
		say("Server [%s] has rejected you (probably due to a nick collision)", from);
		servercmd(NULL, sc, empty_string, NULL);
	}
	else
	{
		if (localkill)
		{
			int i = from_server + 1;
			if (i >= server_list_size())
				i = 0;
			snprintf(sc, 19, "+%i", i);
			from_server = -1;
		}
		if (do_hook(DISCONNECT_LIST,"Killed by %s (%s)",from,
				ArgList[1] ? ArgList[1] : "(No Reason)"))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_KILL_FSET), "%s %s %s", update_clock(GET_TIME), from, ArgList[1]? ArgList[1] : "You have been Killed"));
		if (get_int_var(CHANGE_NICK_ON_KILL_VAR))
			fudge_nickname(from_server, 1);
		if (get_int_var(AUTO_RECONNECT_VAR))
			servercmd (NULL, sc, empty_string, NULL);
		logmsg(LOG_KILL, from, 0, "%s", ArgList[1]?ArgList[1]:"(No Reason)");
	}
	update_all_status(current_window, NULL, 0);
}

static	void p_ping(char *from, char **ArgList)
{

	
	PasteArgs(ArgList, 0);
	send_to_server("PONG %s", ArgList[0]);
}

static	void p_nick(char *from, char **ArgList)
{
	int	one_prints = 0,
		its_me = 0;
ChannelList	*chan;
	char	*line;


	
	line = ArgList[0];
	if (!my_stricmp(from, get_server_nickname(from_server)))
	{
		accept_server_nickname(from_server, line);
  		its_me = 1;
		nick_command_is_pending(from_server, 0);
	}
	if (!check_ignore(from, FromUserHost, NULL, IGNORE_NICKS, NULL))
		goto do_nick_rename;
	for (chan = get_server_channels(from_server); chan; chan = chan->next)
	{
		if (find_nicklist_in_channellist(from, chan, 0)) {
#ifdef WANT_TCL
			if (!its_me)
				check_tcl_nick(from, FromUserHost, from, chan->channel, line);
#endif
			set_display_target(chan->channel, LOG_CRAP);
			if (do_hook(CHANNEL_NICK_LIST, "%s %s %s", chan->channel, from, line))
				one_prints = 1;
			do_logchannel(LOG_CRAP, chan, "%s %s", from, line);
		}
	}
	if (one_prints)
	{
		if (its_me)
		{
			set_string_var(AUTO_RESPONSE_STR_VAR, line);
			reset_display_target();
		} else
			set_display_target(what_channel(from, from_server), LOG_CRAP);
		if (do_hook(NICKNAME_LIST, "%s %s", from, line))
			put_it("%s",convert_output_format(
				fget_string_var(its_me?FORMAT_NICKNAME_USER_FSET:
				im_on_channel(what_channel(from, from_server), from_server)?
				FORMAT_NICKNAME_FSET:
				FORMAT_NICKNAME_OTHER_FSET), 
				"%s %s %s %s", 
				update_clock(GET_TIME),from, "-", line));
	}

do_nick_rename:

	rename_nick(from, line, from_server);
#ifdef WANT_NSLOOKUP
	ar_rename_nick(from, line, from_server);
#endif
	if (!its_me)
	{
		notify_mark(from, FromUserHost, 0, 0);
		notify_mark(line, FromUserHost, 1, 0);
	}
#ifdef GUI
	gui_update_nicklist(NULL);
#endif
}

static int check_mode_change(NickList *nick, char type_mode, char *from, char *this_nick, char *channel)
{
time_t right_now = now;
int found = 0;
	if (!nick->userlist && !isme(nick->nick))
	{
		if ((!nick_isop(nick) && type_mode == '+') || (nick_isop(nick) && type_mode == '-'))
		{
			switch(type_mode)
			{
				case '-':
					if (nick->sent_deop > 4 && right_now - nick->sent_deop_time < 10)
						return 0;
					nick->sent_deop++;
					nick->sent_deop_time = right_now;	
					break;
				case '+':
					if (nick->sent_reop > 4 && right_now - nick->sent_reop_time < 10)
						return 0;
					nick->sent_reop++;
					nick->sent_reop_time = right_now;
					break;
				default:
					break;
			}
			if (my_stricmp(this_nick, from))
			{
				send_to_server("MODE %s %co %s", channel, type_mode, this_nick);
				found++;
			}
		}
	}
	return found;
}

static void check_bitch_mode(char *from, char *uh, char *channel, char *line, ChannelList *chan)
{
NickList *nick;
char *new_mode = NULL;
char *n = NULL;
time_t right_now;

	
	if (!from || !chan || (chan && (!get_cset_int_var(chan->csets, BITCH_CSET) || !chan->have_op)))
		return;
	if (!get_int_var(HACK_OPS_VAR) && wild_match("%.%", from))
		return;
	if (!(nick = find_nicklist_in_channellist(from, chan, 0)))
		return;
	set_display_target(channel, LOG_CRAP);
	new_mode = LOCAL_COPY(line);
	new_mode = next_arg(new_mode, &n);
	if (!nick->userlist || !check_channel_match(nick->userlist->channels, channel))
	{
		char *p;
		char type_mode = '%' , *this_nick, *list_nicks;
		int found = 0;
		list_nicks = LOCAL_COPY(n);
		right_now = now;
		for (p = new_mode; *p; p++)
		{
			switch(*p)
			{
				case '-':
					type_mode = '+';
					break;
				case '+':
					type_mode = '-';
					break;
				case 'o':
					this_nick = next_arg(list_nicks, &list_nicks);
					nick = find_nicklist_in_channellist(this_nick, chan, 0);
					found += check_mode_change(nick, type_mode, from, this_nick, channel);
					break;
				default:
					break;
			} 
		}
		if (found)
			put_it("%s", convert_output_format(fget_string_var(FORMAT_BITCH_FSET), "%s %s %s %s %s %s", update_clock(GET_TIME), from, uh, channel, new_mode, n));
	}
	reset_display_target();
}

static	void p_mode(char *from, char **ArgList)
{
	char *target;
	char *line;
	int	flag;
	
	ChannelList *chan = NULL;
	ChannelList *chan2 = get_server_channels(from_server);
	char buffer[BIG_BUFFER_SIZE+1];		
	char *smode;
	char *display_uh = FromUserHost[0] ? FromUserHost : "*";
#ifdef COMPRESS_MODES
	char *tmpbuf = NULL;
#endif
	
	PasteArgs(ArgList, 1);
	target = ArgList[0];
	line = ArgList[1];
	smode = strchr(from, '.');

	flag = check_ignore(from, FromUserHost, target, (smode?IGNORE_SMODES : IGNORE_MODES) | IGNORE_CRAP, NULL);

	set_display_target(target, LOG_CRAP);
	if (target && line)
	{
		strcpy(buffer, line);
		if (get_int_var(MODE_STRIPPER_VAR))
			strip_modes(from, target, line);
		if (is_channel(target))
		{
#ifdef COMPRESS_MODES
			if (chan2)
				chan = (ChannelList *)find_in_list((List **)&chan2, target, 0);
			if (chan && get_cset_int_var(chan->csets, COMPRESS_MODES_CSET))
			{
				tmpbuf = do_compress_modes(chan, from_server, target, line);
				if (tmpbuf)
					strcpy(line, tmpbuf);
				else
					flag = IGNORED;
			}
#endif
			/* CDE handle mode protection here instead of later */
			update_channel_mode(from, target, from_server, buffer, chan);
#ifdef WANT_TCL
			check_tcl_mode(from, FromUserHost, from, target, line);
#endif			
			if (my_stricmp(from, get_server_nickname(from_server))) 
			{
				check_mode_lock(target, line, from_server);
				check_bitch_mode(from, FromUserHost, target, line, chan);
			}

			if (flag != IGNORED && do_hook(MODE_LIST, "%s %s %s", from, target, line))
			{
				enum FSET_TYPES fset = smode ? FORMAT_SMODE_FSET : FORMAT_MODE_FSET;
				put_it("%s", convert_output_format(fget_string_var(fset), "%s %s %s %s %s", update_clock(GET_TIME), from, display_uh, target, line));
			}
			logmsg(LOG_MODE_CHAN, from, 0, "%s %s", target, line);
			do_logchannel(LOG_MODE_CHAN, chan, "%s %s, %s", from, target, line);
		}
		else
		{
			if (flag != IGNORED && do_hook(MODE_LIST, "%s %s %s", from, target, line))
			{
				/* User mode changes where from != target don't occur on 
				 * standard servers, but are used by services on some networks. */
				enum FSET_TYPES fset = my_stricmp(from, target) ? FORMAT_USERMODE_OTHER_FSET : FORMAT_USERMODE_FSET;
				put_it("%s", convert_output_format(fget_string_var(fset), "%s %s %s %s %s", update_clock(GET_TIME), from, display_uh, target, line));
			}
			if (!my_stricmp(target, get_server_nickname(from_server)))
				update_user_mode(line);
			logmsg(LOG_MODE_USER, from, 0, "%s %s", target, line);
		}
		update_all_status(current_window, NULL, 0);
	}
#ifdef GUI
	gui_update_nicklist(target);
#endif
	reset_display_target();
}

static void strip_modes (char *from, char *channel, char *line)
{
	char	*mode;
	char 	*pointer;
	char	mag = '+'; /* XXXX Bogus */
        char    *copy = NULL;
	char	free_copy[BIG_BUFFER_SIZE+1];

	strcpy(free_copy, line);
	
	copy = free_copy;
	mode = next_arg(copy, &copy);
	if (is_channel(channel))
	{
		for (pointer = mode; *pointer; pointer++)
		{
			char	c = *pointer;
			switch (c) 
			{
				case '+' :
				case '-' : mag = c; break;
				case 'l' : if (mag == '+')
						do_hook(MODE_STRIPPED_LIST,"%s %s %c%c %s",
						  from,channel,mag,c,next_arg(copy,&copy));
					   else
						do_hook(MODE_STRIPPED_LIST,"%s %s %c%c",
						  from,channel,mag,c);
					   break;
				case 'a' :
				case 'i' :
				case 'm' :
				case 'n' :
				case 'p' :
				case 's' :
				case 't' : 
				case 'z' :
				case 'c' :
				case 'r' :
				case 'R' :
				do_hook(MODE_STRIPPED_LIST,"%s %s %c%c",from,
						channel,mag,c);
					   break;
				case 'b' :
				case 'k' :
				case 'o' :
				case 'e' :
				case 'I' :
				case 'v' : do_hook(MODE_STRIPPED_LIST,"%s %s %c%c %s",from,
						channel,mag,c,next_arg(copy,&copy));
					   break;
			}
		}
	}
	else /* User mode */
	{
		for (pointer = mode; *pointer; pointer++)
		{
			char	c = *pointer;
			switch (c) 
			{
				case '+' :
				case '-' : mag = c; break;
				default  : do_hook(MODE_STRIPPED_LIST,"%s %s %c%c",from, channel, mag, c);
					   break;
			}
		}
	}
}

static	void p_kick(char *from, char **ArgList)
{
	char	*channel,
		*who,
		*comment;
	char	*chankey = NULL;
	ChannelList *chan = NULL;
	NickList *tmpnick = NULL;
	int	t = 0;
	

	
	channel = ArgList[0];
	who = ArgList[1];
	comment = ArgList[2] ? ArgList[2] : "(no comment)";

	if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
		tmpnick = find_nicklist_in_channellist(from, chan, 0);
	set_display_target(channel, LOG_CRAP);
	if (channel && who && chan)
	{
		update_stats(KICKLIST, tmpnick, chan, 0);
#ifdef WANT_TCL
		check_tcl_kick(from, FromUserHost, from, channel, who, comment);
#endif
		if (!my_stricmp(who, get_server_nickname(from_server)))
		{

			Window *window = get_window_by_refnum(chan->refnum);/*chan->window;*/
			int rejoin = 0;  
			if (chan->key)
				malloc_strcpy(&chankey, chan->key);
	
			rejoin = get_cset_int_var(chan->csets, AUTO_REJOIN_CSET);
			switch(rejoin)
			{
				case 0:
				case 1:
					break;
				case 2:
					if (FromUserHost)
					{
						char *username;
						char *ptr;
						username = LOCAL_COPY(FromUserHost);
						if ((ptr = strchr(username, '@')))
						{
							*ptr = 0;
							ptr = username;
							ptr = clear_server_flags(username);
						} else
							ptr = username;
						do_newuser(NULL, ptr, NULL);
					}
					break;
					
				case 3:
					send_to_server("NICK %s", random_str(3,9));
					break;
				case 4:
					do_newuser(NULL, random_str(2,9), NULL);
				case 5:
				default:
						send_to_server("NICK %s", random_str(3,9));
					break;
			}
			do_logchannel(LOG_KICK_USER, chan, "%s %s, %s %s %s", from, FromUserHost, who, channel, comment);
			if (rejoin)
				send_to_server("JOIN %s%s%s", channel, chankey? space : empty_string, chankey ? chankey: empty_string);
			new_free(&chankey);
			if (do_hook(KICK_LIST, "%s %s %s %s", who, from, channel, comment?comment:empty_string))
				put_it("%s",convert_output_format(fget_string_var(FORMAT_KICK_USER_FSET),"%s %s %s %s %s",update_clock(GET_TIME),from, channel, who, comment));
			remove_channel(channel, from_server);
			update_all_status(window ? window : current_window, NULL, 0);
			update_input(UPDATE_ALL);
			logmsg(LOG_KICK_USER, from, 0, "%s %s %s %s", FromUserHost, who, channel, comment);
			if (rejoin)
				add_to_join_list(channel, from_server, window ? window->refnum : 0);
		}
		else
		{
			NickList *f_nick = NULL;
			int itsme = !my_stricmp(get_server_nickname(from_server), from) ? 1: 0;

			if ((check_ignore(from, FromUserHost, channel, IGNORE_KICKS, NULL) != IGNORED) && 
			     do_hook(KICK_LIST, "%s %s %s %s", who, from, channel, comment))
				put_it("%s",convert_output_format(fget_string_var(FORMAT_KICK_FSET),"%s %s %s %s %s",update_clock(GET_TIME),from, channel, who, comment));
			/* if it's me that's doing the kick don't flood check */
			if (!itsme)
			{
				f_nick = find_nicklist_in_channellist(who, chan, 0);
				if (chan->have_op && tmpnick && is_other_flood(chan, tmpnick, KICK_FLOOD, &t))
				{
					if (get_cset_int_var(chan->csets, KICK_ON_KICKFLOOD_CSET) > get_cset_int_var(chan->csets, DEOP_ON_KICKFLOOD_CSET))
						send_to_server("MODE %s -o %s", chan->channel, from);
					else if (!f_nick->kickcount++)
						send_to_server("KICK %s %s :\002Mass kick detected - (%d kicks in %dsec%s)\002", chan->channel, from, get_cset_int_var(chan->csets, KICK_ON_KICKFLOOD_CSET), t, plural(t));
				} 
#ifdef WANT_USERLIST
				check_prot(from, who, chan, NULL, f_nick);
#endif
			}
			remove_from_channel(channel, who, from_server, 0, NULL);
			logmsg(LOG_KICK, from, 0, "%s %s %s %s", FromUserHost, who, channel, comment);
			do_logchannel(LOG_KICK, chan, "%s %s %s %s %s", from, FromUserHost, who, channel, comment);
		}

	}
	update_all_status(current_window, NULL, 0);
	reset_display_target();
#ifdef GUI
	gui_update_nicklist(channel);
#endif
}

static	void p_part(char *from, char **ArgList)
{
	char	*channel;
	ChannelList *tmpc;
	

	
	if (!from || !*from)
		return;
	channel = ArgList[0];

	PasteArgs(ArgList, 1);
	set_display_target(channel, LOG_CRAP);

	if ((tmpc = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
		update_stats(LEAVELIST, find_nicklist_in_channellist(from, tmpc, 0), tmpc, 0);

	if ((check_ignore(from, FromUserHost, channel, IGNORE_PARTS, NULL) != IGNORED) &&
	    do_hook(LEAVE_LIST, "%s %s %s %s", from, channel, FromUserHost, ArgList[1]?ArgList[1]:empty_string))
		put_it("%s",convert_output_format(fget_string_var(FORMAT_LEAVE_FSET), "%s %s %s %s %s", update_clock(GET_TIME), from, FromUserHost, channel, ArgList[1]?ArgList[1]:empty_string));
	if (!my_stricmp(from, get_server_nickname(from_server)))
	{
		remove_channel(channel, from_server);
		remove_from_mode_list(channel, from_server);
		remove_from_join_list(channel, from_server);
		set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
		do_hook(LEAVE_ME_LIST, "%s", channel);
	}
	else
	{
#ifdef WANT_TCL
		check_tcl_part(from, FromUserHost, from, channel);
#endif
		remove_from_channel(channel, from, from_server, 0, NULL);
		logmsg(LOG_PART, from, 0, "%s %s", channel, ArgList[1] ? ArgList[1]:empty_string);
		do_logchannel(LOG_PART, tmpc, "%s %s %s", channel, from, ArgList[1] ? ArgList[1]:empty_string);
	}
	update_all_status(current_window, NULL, 0);
	update_input(UPDATE_ALL);
	reset_display_target();
#ifdef GUI
	gui_update_nicklist(channel);
#endif
}

static void rfc1459_odd (char *from, char *comm, char **ArgList)
{
	PasteArgs(ArgList, 0);
	if (do_hook(ODD_SERVER_STUFF_LIST, "%s %s %s", from ? from : "*", comm, ArgList[0]))
	{
		if (from)
			say("Odd server stuff: \"%s %s\" (%s)", comm, ArgList[0], from);
		else
			say("Odd server stuff: \"%s %s\"", comm, ArgList[0]);
	}
}
static void p_rpong (char *from, char **ArgList)
{
	if (!ArgList[3])
	{
		PasteArgs(ArgList, 0);
		say("RPONG %s (from %s)", ArgList[0], from);
	}
	else
	{
		time_t delay = now - atol(ArgList[3]);

		say("Pingtime %s - %s : %s ms (total delay: %ld s)",
			from, ArgList[1], ArgList[2], delay);
	}
}


protocol_command rfc1459[] = {
{	"ADMIN",	NULL,		NULL,		0,		0, 0},
{	"AUTHENTICATE",	p_authenticate,	NULL,		0,		0, 0},
{	"AWAY",		NULL,		NULL,		0,		0, 0},
{	"CAP",		p_cap,		NULL,		0,		0, 0},
{ 	"CONNECT",	NULL,		NULL,		0,		0, 0},
{	"ERROR",	p_error,	NULL,		0,		0, 0},
{	"ERROR:",	p_error,	NULL,		0,		0, 0},
{	"INVITE",	p_invite,	NULL,		0,		0, 0},
{	"INFO",		NULL,		NULL,		0,		0, 0},
{	"ISON",		NULL,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"JOIN",		p_channel,	NULL,		PROTO_DEPREC,	0, 0},
{	"KICK",		p_kick,		NULL,		0,		0, 0},
{	"KILL",		p_kill,		NULL,		0,		0, 0},
{	"LINKS",	NULL,		NULL,		0,		0, 0},
{	"LIST",		NULL,		NULL,		0,		0, 0},
{	"MODE",		p_mode,		NULL,		0,		0, 0},
{	"NAMES",	NULL,		NULL,		0,		0, 0},
{	"NICK",		p_nick,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"NOTICE",	parse_notice,	NULL,		0,		0, 0},
{	"OPER",		NULL,		NULL,		0,		0, 0},
{	"PART",		p_part,		NULL,		PROTO_DEPREC,	0, 0},
{	"PASS",		NULL,		NULL,		0, 		0, 0},
{	"PING",		p_ping,		NULL,		0,		0, 0},
{	"PONG",		p_pong,		NULL,		0,		0, 0},
{	"PRIVMSG",	p_privmsg,	NULL,		0,		0, 0},
{	"QUIT",		p_quit,		NULL,		PROTO_DEPREC,	0, 0},
{	"REHASH",	NULL,		NULL,		0,		0, 0},
{	"RESTART",	NULL,		NULL,		0,		0, 0},
{	"RPONG",	p_rpong,	NULL,		0,		0, 0},
{	"SERVER",	NULL,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"SILENCE",	p_silence,	NULL,		0,		0, 0},
{	"SQUIT",	NULL,		NULL,		0,		0, 0},
{	"STATS",	NULL,		NULL,		0,		0, 0},
{	"SUMMON",	NULL,		NULL,		0,		0, 0},
{	"TIME",		NULL,		NULL,		0,		0, 0},
{	"TRACE",	NULL,		NULL,		0,		0, 0},
{	"TOPIC",	p_topic,	NULL,		0,		0, 0},
{	"USER",		NULL,		NULL,		0,		0, 0},
{	"USERHOST",	NULL,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"USERS",	NULL,		NULL,		0,		0, 0},
{	"VERSION",	NULL,		NULL,		0,		0, 0},
{	"WALLOPS",	p_wallops,	NULL,		0,		0, 0},
{	"WHO",		NULL,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"WHOIS",	NULL,		NULL,		0,		0, 0},
{	"WHOWAS",	NULL,		NULL,		0,		0, 0},
{	NULL,		NULL,		NULL,		0,		0, 0}
};
#define NUMBER_OF_COMMANDS (sizeof(rfc1459) / sizeof(protocol_command)) - 2;
int 	num_protocol_cmds = -1;

BUILT_IN_COMMAND(debugmsg)
{
int i;
unsigned long total = 0;
	for (i = 0; rfc1459[i].command; i++)
	{
		put_it("DEBUG_MSG: %10s[%03lu] # %ld -> %ld bytes", rfc1459[i].command, i, rfc1459[i].count, rfc1459[i].bytes);
		total += rfc1459[i].bytes;
	}
	put_it("DEBUG_MSG: Total bytes received %ld", total);
}

void parse_server(char *orig_line)
{
	char	*from,
		*comm,
		*end;
	int	numeric;
	char	*line = NULL;
	int	len = 0;
	char	**ArgList;
	char	copy[BIG_BUFFER_SIZE+1];
	char	*TrueArgs[MAXPARA + 1] = {NULL};

#ifdef WANT_DLL
	RawDll	*raw = NULL;
#endif
	protocol_command *retval;
	int	loc;
	int	cnt;

	if (num_protocol_cmds == -1)
		num_protocol_cmds = NUMBER_OF_COMMANDS;

	
	if (!orig_line || !*orig_line)
		return;

	len = strlen(orig_line);

	end = len + orig_line;
	if (*--end == '\n')
		*end-- = 0;
	if (*end == '\r')
		*end-- = 0;

	if (x_debug & DEBUG_INBOUND)
		yell("[%d] <- [%s]", get_server_read(from_server), orig_line);
                                                                                                                                                                                                    
	if (*orig_line == ':')
	{
		if (!do_hook(RAW_IRC_LIST, "%s", orig_line + 1))
			return;
	}
	else if (!do_hook(RAW_IRC_LIST, "* %s", orig_line))
		return;

	if (inbound_line_mangler)
	{
		len = strlen(orig_line) * 3;
		line = alloca(len + 1);
		strcpy(line, orig_line);
		if (mangle_line(line, inbound_line_mangler, len) > len)
			yell("mangle_line truncated its result. Ack.");
	}
	else
		line = orig_line;

	ArgList = TrueArgs;

	strncpy(copy, line, BIG_BUFFER_SIZE);
	BreakArgs(line, &from, ArgList, 0);

	/* XXXX - i dont think 'from' can be null here.  */
	if (!(comm = (*ArgList++)) || !from || !*ArgList)
		return;		/* Serious protocol violation -- ByeBye  */
#ifdef WANT_TCL
	if (check_tcl_raw(copy, comm))
		return;
#endif

#ifdef WANT_DLL
	if ((raw = find_raw_proc(comm, ArgList)))
		if ((raw->func(comm, from, FromUserHost, ArgList)))
			return;
#endif

#if 0
	if (translation)
	{
		unsigned char *q, *p;
		int i = 0;
		q = ArgList[0];
		while (q && *q)
		{
			for (p = q; *p; p++)
				*p = transToClient[(int)*p];
			q = ArgList[++i];
		}
	}
#endif

	/*
	 * I reformatted these in may '96 by using the output of /stats m
	 * from a few busy servers.  They are arranged so that the most
	 * common types are high on the list (to save the average number
	 * of compares.)  I will be doing more testing in the future on
	 * a live client to see if this is a reasonable order.
	 */
	if ((numeric = atoi(comm)) > 0) /* numbered_command can't handle -ves */
		numbered_command(from, numeric, ArgList);
	else
	{
		retval = (protocol_command *)find_fixed_array_item(
			(void *)rfc1459, sizeof(protocol_command), 
			num_protocol_cmds + 1, comm, &cnt, &loc);

		if (cnt < 0 && rfc1459[loc].inbound_handler)
			rfc1459[loc].inbound_handler(from, ArgList);
		else
			rfc1459_odd(from, comm, ArgList);
		rfc1459[loc].bytes += len;
		rfc1459[loc].count++;
	}
	FromUserHost = empty_string;
	from_server = -1;
}
