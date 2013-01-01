/*
 * this code is Copyright Colten Edwards (c) 96
 */
 
#include "irc.h"
static char cvsrevision[] = "$Id: banlist.c 205 2012-06-07 04:22:17Z tcava $";
CVS_REVISION(banlist_c)
#include "struct.h"
#include "commands.h"
#include "list.h"
#include "hook.h"
#include "ignore.h"
#include "ircaux.h"
#include "output.h"
#include "screen.h"
#include "server.h"
#include "window.h"
#include "who.h"
#include "whowas.h"
#include "vars.h"
#include "userlist.h"
#include "misc.h"
#include "timer.h"
#include "hash2.h"
#include "cset.h"
#define MAIN_SOURCE
#include "modval.h"

#ifndef Server2_8hybrid
#define Server2_8hybrid 5
#endif

int defban = 2;

static char *mode_buf = NULL;
static int mode_len = 0;

static char *mode_str = NULL;
static char *user = NULL;
static int mode_str_len = 0;
static int push_len = 0;
static char plus_mode[20] = "\0";

void add_mode_buffer( char *buffer, int mode_str_len)
{
	
	malloc_strcat(&mode_buf, buffer);
	mode_len += push_len;
}

void flush_mode(ChannelList *chan)
{
	
	if (mode_buf)
		my_send_to_server(chan?chan->server:from_server, "%s", mode_buf);
	new_free(&mode_buf);
	mode_len = 0;
}

int delay_flush_all (void *arg, char *sub)
{
char buffer[BIG_BUFFER_SIZE+1];
char *args = (char *)arg;
char *serv_num = NULL;
char *channel = NULL;
int ofs = from_server;

	
	channel = next_arg(args, &args);
	if ((serv_num = next_arg(args, &args)))
		from_server = atoi(serv_num);
	if (channel && *channel && mode_str && user)
	{
		sprintf(buffer, "MODE %s %s%s %s\r\n", channel, plus_mode, mode_str, user);
		push_len = strlen(buffer);
		add_mode_buffer(buffer, push_len);
		mode_str_len = 0;
		new_free(&mode_str);
		new_free(&user);
		memset(plus_mode, 0, sizeof(plus_mode));
		push_len = 0;
	}
	flush_mode(NULL);
	new_free(&arg);
	from_server = ofs;
	return 0;
}

void flush_mode_all(ChannelList *chan)
{
char buffer[BIG_BUFFER_SIZE+1];

	
	if (mode_str && user)
	{
		sprintf(buffer, "MODE %s %s%s %s\r\n", chan->channel, plus_mode, mode_str, user);
		push_len = strlen(buffer);
		add_mode_buffer(buffer, push_len);
		mode_str_len = 0;
		new_free(&mode_str);
		new_free(&user);
		memset(plus_mode, 0, sizeof(plus_mode));
		push_len = 0;
	}
	flush_mode(chan);
}


void add_mode(ChannelList *chan, char *mode, int plus, char *nick, char *reason, int max_modes)
{
char buffer[BIG_BUFFER_SIZE+1];
/*
KICK $C nick :reason
MODE $C +/-o nick
MODE $C +/-b userhost
*/

	
	if (mode_len >= (IRCD_BUFFER_SIZE-100))
	{
		flush_mode(chan);
		push_len = 0;
	}

	if (reason)
	{
		sprintf(buffer, "KICK %s %s :%s\r\n", chan->channel, nick, reason);
		push_len = strlen(buffer);
		add_mode_buffer(buffer, push_len);
	}
	else
	{
		mode_str_len++;
		strcat(plus_mode, plus ? "+" : "-");
		malloc_strcat(&mode_str, mode);
		m_s3cat(&user, space, nick);
		if (mode_str_len >= max_modes)
		{
			sprintf(buffer, "MODE %s %s%s %s\r\n", chan->channel, plus_mode, mode_str, user);
			push_len = strlen(buffer);
			add_mode_buffer(buffer, push_len);
			new_free(&mode_str);
			new_free(&user);
			memset(plus_mode, 0, sizeof(plus_mode));
			mode_str_len = push_len = 0;
		}
	}
}


BUILT_IN_COMMAND(fuckem)
{
char c;
ChannelList *chan;
int server;
char buffer[BIG_BUFFER_SIZE];
BanList *Bans;
	if (!(chan = prepare_command(&server, NULL, NEED_OP)))
		return;
	for (Bans = chan->bans; Bans; Bans = Bans->next)
		add_mode(chan, "b", 0, Bans->ban, NULL, get_int_var(NUM_BANMODES_VAR));
	for (c = 'a'; c <= 'z'; c++)
	{
		sprintf(buffer, "*!*@*%c*", c);
		add_mode(chan, "b", 1, buffer, NULL, get_int_var(NUM_BANMODES_VAR));
	}         
	flush_mode_all(chan);
}


/*
 * Lamer Kick!   Kicks All UnOpped People from Current Channel        
 */
BUILT_IN_COMMAND(LameKick)
{
	char *channel = NULL;
	ChannelList *chan;
	NickList *tmp;
	char	*buffer = NULL;
	char	*buf2 = NULL;
	int	old_server = from_server;

	
	if (args && *args && is_channel(args))
		channel = next_arg(args, &args);	
	if ((chan = prepare_command(&from_server, channel, NEED_OP)))
	{
		char reason[BIG_BUFFER_SIZE+1];
		int len_buffer =  0;
		int count = 0;
		int total = 0;
		*reason = 0;
		quote_it(args ? args : empty_string, NULL, reason);
		malloc_sprintf(&buffer, "KICK %%s %%s :<\002BX\002-LK> %s", reason);
		len_buffer = strlen(buffer) + 2;
		for (tmp = next_nicklist(chan, NULL); tmp; tmp = next_nicklist(chan, tmp))
		{
			int level= 0;
			if (tmp->userlist)
				level = ((tmp->userlist->flags | 0xff) & PROT_ALL);
/*			if (!tmp->chanop && !tmp->voice && ((tmp->userlist && !level) || !tmp->userlist))*/

			if (!nick_isop(tmp) && !nick_isvoice(tmp) && ((tmp->userlist && !level) || !tmp->userlist))
			{
				m_s3cat(&buf2, ",", tmp->nick);
				count++;
				total++;
				if (((get_int_var(NUM_KICKS_VAR)) && 
					(count >= get_int_var(NUM_KICKS_VAR))) || 
					(strlen(buf2) + len_buffer) >= (IRCD_BUFFER_SIZE - (NICKNAME_LEN + 5)))
				{
					send_to_server(buffer, chan->channel, buf2);
					new_free(&buf2);
					count = 0;
				}
			}
		}
		if (buf2)
			send_to_server(buffer, chan->channel, buf2);
		new_free(&buffer);
		new_free(&buf2);
		say("Sent the Server all the Lamer Kicks, Sit back and Watch %d kicks!", total);
	}
	from_server = old_server;
}

static void shitlist_erase(ShitList **clientlist)
{
	ShitList	*Client, *tmp;
	
	for (Client = *clientlist; Client;)
	{
		new_free(&Client->filter);
		new_free(&Client->reason);
		tmp = Client->next;
		new_free((char **)&Client);
		Client = tmp;
	}
	*clientlist = NULL;
}

static char *screw(char *user)
{
char *p;
	for (p = user; p && *p;)
	{
		switch(*p)
		{
			case '.':
			case ':':
			case '*':
			case '@':
			case '!':
				p+=1;
				break;
			default:
				*p = '?';
				if (*(p+1) && *(p+2))
					p+=2;
				else
					p++;
		}
	}
	return user;
}

char * ban_it(char *nick, char *user, char *host, char *ip)
{
static char banstr[BIG_BUFFER_SIZE/4+1];
char *t = user;
char *t1 = user;
char *tmp;
	
	*banstr = 0;
	while (strlen(t1)>9)
		t1++;
	t1 = clear_server_flags(t1);
	switch (defban) 
	{
		case 7:
			if (ip)
			{
				snprintf(banstr, sizeof banstr, "*!*@%s",
					cluster(ip));
				break;
			}
		case 2: /* Better 	*/
			snprintf(banstr, sizeof banstr, "*!*%s@%s", t1, 
				cluster(host));
			break;
		case 3: /* Host 	*/
			snprintf(banstr, sizeof banstr, "*!*@%s", host);
			break;
		case 4: /* Domain	*/
			tmp = strrchr(host, '.');
			if (tmp) {
				snprintf(banstr, sizeof banstr, "*!*@*%s",
					tmp);
			} else {
				snprintf(banstr, sizeof banstr, "*!*@%s", 
					host);
			}
			break;	
		case 5: /* User		*/
			snprintf(banstr, sizeof banstr, "*!%s@%s", t, 
				cluster(host));
			break;
		case 6: /* Screw 	*/
			snprintf(banstr, sizeof banstr, "*!*%s@%s", t1, host);
			screw(banstr);
			break;
		case 1:	/* Normal 	*/
		default:
			snprintf(banstr, sizeof banstr, "%s!*%s@%s", nick, t1,
				host);
			break;
	}
	return banstr;
}

void userhost_unban(UserhostItem *stuff, char *nick1, char *args)
{
char *tmp;
ChannelList *chan;
char *channel = NULL;
BanList *bans;

char *host = NULL;
char *ip_str = NULL;
WhowasList *whowas = NULL;
NickList *n = NULL;
int count = 0;
int old_server = from_server;

	
	if (!stuff || !stuff->nick || !nick1 || 
		!strcmp(stuff->user, "<UNKNOWN>") || 
		my_stricmp(stuff->nick, nick1))
	{
		if (nick1 && (whowas = check_whowas_nick_buffer(nick1, args, 0)))
		{
			malloc_sprintf(&host, "%s!%s", whowas->nicklist->nick, whowas->nicklist->host);
			bitchsay("Using WhoWas info for unban of %s ", nick1);
			n = whowas->nicklist;
		}
		else if (nick1)
		{
			bitchsay("No match for the unban of %s on %s", nick1, args);
			return;
		}
		if (!nick1)
			return;
	}
	else
	{
		tmp = clear_server_flags(stuff->user);
		malloc_sprintf(&host, "%s!%s@%s",stuff->nick, tmp, stuff->host); 
	}

	channel = next_arg(args, &args);
	if (args && *args)
		from_server = atoi(args);
	if (!(chan = prepare_command(&from_server, channel, NEED_OP)))
	{
		new_free(&host);
		return;
	}
	if (!n)
		n = find_nicklist_in_channellist(stuff->nick, chan, 0);

	if (n && n->ip)
	{
		size_t len = strlen(n->nick)+strlen(n->host)+strlen(n->ip)+10;
		ip_str = alloca(len); 
		*ip_str = 0;
		strmopencat(ip_str, len, stuff->nick, "!", stuff->user, "@", n->ip, NULL);
	}
	for (bans = chan->bans; bans; bans = bans->next)
	{
		if (!bans->sent_unban && (wild_match(bans->ban, host) || (ip_str && wild_match(bans->ban, ip_str))) )
		{
			add_mode(chan, "b", 0, bans->ban, NULL, get_int_var(NUM_BANMODES_VAR));
			bans->sent_unban++;
			count++;
		}			
	}	

	flush_mode_all(chan);
	if (!count)
		bitchsay("No match for Unban of %s on %s", nick1, args);
	new_free(&host);
	from_server = old_server;
}


void userhost_ban(UserhostItem *stuff, char *nick1, char *args)
{
	char *temp;
	char *str= NULL;
	char *channel;
	ChannelList *c = NULL;
	NickList *n = NULL;

	char *ob = "-o+b";
	char *b = "+b";

	char *host = NULL, *nick = NULL, *user = NULL, *chan = NULL;
	WhowasList *whowas = NULL;
		
	int fuck = 0;
	int set_ignore = 0;
	
	
	channel = next_arg(args, &args);
	temp = next_arg(args, &args);

	fuck = !my_stricmp("FUCK", args);
	set_ignore = !my_stricmp("BKI", args);
	
	if (!stuff || !stuff->nick || !nick1 || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick1))
	{
		if (nick1 && channel && (whowas = check_whowas_nick_buffer(nick1, channel, 0)))
		{
			nick = whowas->nicklist->nick;
			user = m_strdup(clear_server_flags(whowas->nicklist->host));
			host = strchr(user, '@');
			*host++ = 0;
			bitchsay("Using WhoWas info for ban of %s ", nick1);
			n = whowas->nicklist;
		}
		else if (nick1)
		{
			bitchsay("No match for the %s of %s on %s", fuck ? "Fuck":"Ban", nick1, channel);
			return;
		}
	} 
	else
	{
		nick = stuff->nick;
		user = m_strdup(clear_server_flags(stuff->user));
		host = stuff->host;
	}

	if (!(my_stricmp(nick, get_server_nickname(from_server))))
	{
		bitchsay("Try to kick yourself again!!");
		new_free(&user);
		return;
	}

	if (is_on_channel(channel, from_server, nick))
		chan = channel;
	c = lookup_channel(channel, from_server, 0);
	if (c && !n)
		n = find_nicklist_in_channellist(nick, c, 0);
	send_to_server("MODE %s %s %s %s", channel, chan ? ob : b, chan?nick:empty_string, ban_it(nick, user, host, (n && n->ip)?n->ip:NULL));
	if (fuck)
	{
		malloc_sprintf(&str, "%s!*%s@%s %s 3 Auto-Shit", nick, user, host, channel);
#ifdef WANT_USERLIST
		add_shit(NULL, str, NULL, NULL);
#endif
		new_free(&str);
	} else if (set_ignore)
		ignore_nickname(ban_it("*", user, host, NULL)	, IGNORE_ALL, 0);
	new_free(&user);
}

BUILT_IN_COMMAND(multkick)
{
	char *to = NULL, *temp = NULL, *reason = NULL;
	ChannelList *chan;
	int server = from_server;
	int	filter = 0;
	

	if (command && *command)
		filter = 1;
		
	if (!(to = next_arg(args, &args)))
		to = NULL;
	
	if (to && !is_channel(to))
	{
		temp = to;
		if (args && *args)
			*(temp + strlen(temp)) = ' ';
		to = NULL;
	}
	else
		temp = args;
		
	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;

	if (!temp || !*temp)
		return;

	reason = strchr(temp, ':');

	if (reason)
		*reason++ = 0;

	if (!reason || !*reason)
		reason = get_reason(NULL, NULL);

	while (temp && *temp)
	{
		my_send_to_server(server, "KICK %s %s :\002%s\002", chan->channel,
			       next_arg(temp, &temp), reason);
	}
}

BUILT_IN_COMMAND(massdeop)
{
	ChannelList *chan;

register NickList *nicks;

	char *spec, *rest, *to;
	int maxmodes, count, all = 0;
	char	buffer[BIG_BUFFER_SIZE + 1];
	int isvoice = 0;
	int old_server = from_server;
			
	
	maxmodes = get_int_var(NUM_OPMODES_VAR);

	if (command && !my_stricmp(command, "mdvoice"))
		isvoice = 1;
	
	rest = NULL;
	spec = NULL;

	if (!(to = next_arg(args, &args)))
		to = NULL;
	if (to && !is_channel(to))
	{
		spec = to;
		to = NULL;
	}
	if (!(chan = prepare_command(&from_server, to, NEED_OP)))
		return;


	if (!spec && !(spec = next_arg(args, &args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		rest = spec;
		spec = "*!*@*";
	}
	else
		rest = args;
	if (rest && !my_stricmp(rest, "-all"))
		all = 1;

	count = 0;
	for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks))
	{
		sprintf(buffer, "%s!%s", nicks->nick, nicks->host);
#if 0
		if ((all || (!isvoice && nicks->chanop) || (isvoice && nicks->voice)) &&
		    my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
		    wild_match(spec, buffer))
#endif
		if ((all || (!isvoice && nick_isop(nicks)) || (isvoice && nick_isvoice(nicks))) &&
		    my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
		    wild_match(spec, buffer))
		{
			add_mode(chan, isvoice? "v":"o", 0, nicks->nick, NULL, maxmodes);
			count++;
		}

	}
	flush_mode_all(chan);
	from_server = old_server;
	if (!count)
		say("No matches for %s of %s on %s", command?command:"massdeop", spec, chan->channel);
}

BUILT_IN_COMMAND(doop)
{
	char *to = NULL, 
		*temp = NULL;
	ChannelList	*chan = NULL;
	int	count,
		max = get_int_var(NUM_OPMODES_VAR);
	int	old_server = from_server;

	/* command is mode char to use - if none given, default to op */
	if (!command)
		command = "o";

	count = 0;

	if (!(to = next_arg(args, &args)))
		to = NULL;
	if (to && !is_channel(to))
	{
		temp = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&from_server, to, NEED_OP)))
		return;

	if (!temp)
		temp = next_arg(args, &args);

	while (temp && *temp)
	{
		count++;
		add_mode(chan, command, 1, temp, NULL, max);
		temp = next_arg(args, &args);
	}
	flush_mode_all(chan);
	from_server = old_server;
}

BUILT_IN_COMMAND(dodeop)
{
	char *to = NULL, *temp;
	int count, max;
	ChannelList *chan;
	int server = from_server;
		
	count = 0;
	temp = NULL;
	max = get_int_var(NUM_OPMODES_VAR);

	/* command is mode char to use - if none given, default to deop */
	if (!command)
		command = "o";
	
	if (!(to = next_arg(args, &args)))
		to = NULL;
	if (to && !is_channel(to))
	{
		temp = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;

	if (!temp)
		temp = next_arg(args, &args);

	while (temp && *temp)
	{
		count++;
		add_mode(chan, command, 0, temp, NULL, max);
		temp = next_arg(args, &args);
	}
	flush_mode_all(chan);
}

BUILT_IN_COMMAND(massop)
{
	ChannelList *chan;
	
	register NickList *nicks;

	char	*to = NULL, 
		*spec, 
		*rest;
	char	buffer[BIG_BUFFER_SIZE + 1];
	
	int	maxmodes = get_int_var(NUM_OPMODES_VAR), 
		count, 
		i, 
		all = 0,
		massvoice =0;
	int	server = 0;
		
	
	if (command)
		massvoice = 1;

	rest = NULL;
	spec = NULL;

	if (!(to = next_arg(args, &args)))
		to = NULL;
	if (to && !is_channel(to) )
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;

	if (!spec && !(spec = next_arg(args, &args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		rest = spec;
		spec = "*!*@*";
	}
	else
		rest = args;

	if (rest && !my_stricmp(rest, "-all"))
		all = 1;

	count = 0;
	for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks))
	{
		i = 0;
		sprintf(buffer, "%s!%s", nicks->nick, nicks->host);
		if ((my_stricmp(nicks->nick, get_server_nickname(from_server)) && wild_match(spec, buffer)))
		{
			if ((massvoice && !nick_isvoice(nicks) && !nick_isop(nicks)) || !nick_isop(nicks))
			{
				add_mode(chan, massvoice?"v":"o", 1, nicks->nick, NULL, maxmodes);
				count++;
			}
		}
	}
	flush_mode_all(chan);
	if (!count)
		say("No matches for %s of %s on %s", command? command : "massop", spec, chan->channel);
}

BUILT_IN_COMMAND(masskick)
{
	ChannelList *chan;
register NickList *nicks;
	ShitList *masskick_list = NULL, *new = NULL;
	char *to, *spec, *rest, *buffer = NULL, *q;
	int server = from_server;

	int all = 0;
	int ops = 0;
	
	
	rest = NULL;
	spec = NULL;

	if (!(to = next_arg(args, &args)))
		to = NULL;

	if (to && !is_channel(to))
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;

	if (spec && !strncmp(spec, "-all", 4))
	{
		all = 1;
		spec =  next_arg(args, &args);
	}

	if (spec && !strncmp(spec, "-ops", 4))
	{
		ops = 1;
		spec = next_arg(args, &args);
	}
	if (!spec)
		return;
	rest = args;
	if (rest && !*rest)
		rest = NULL;

	for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks))
	{
		int doit = 0;
		q = clear_server_flags(nicks->host);
		malloc_sprintf(&buffer, "%s!*%s", nicks->nick, q);		
		if (all)
			doit = 1;
		else if (ops && nick_isop(nicks))
			doit = 1;
		else if (!nick_isop(nicks))
			doit = 1;
		else if (get_cset_int_var(chan->csets, KICK_OPS_CSET))
			doit = 1;
		if (doit && !isme(nicks->nick) && (wild_match(spec, buffer) || wild_match(nicks->nick, spec)))
#if 0
		if ((all || (ops && nicks->chanop) || !nicks->chanop || get_cset_int_var(chan->csets, KICK_OPS_CSET)) &&
		    my_stricmp(nicks->nick, get_server_nickname(from_server)) &&
		    (wild_match(spec, buffer) || wild_match(nicks->nick, spec)))
#endif
		{
			new = (ShitList *)new_malloc(sizeof(ShitList));
			malloc_sprintf(&new->filter, "%s", nicks->nick);
			add_to_list((List **)&masskick_list, (List *)new);
		}
		new_free(&buffer);
	}

	if (masskick_list)
	{
		int len = 0, num = 0;
		char *send_buf = NULL;
		char buf[BIG_BUFFER_SIZE + 1];
		char reason[BIG_BUFFER_SIZE+1];
		*reason = 0;
		quote_it(rest ? rest : "MassKick", NULL, reason);						
		bitchsay("Performing (%s) Mass Kick on %s", all? "opz/non-opz" : ops ? "ops":"non-opz", chan->channel);
		sprintf(buf, "KICK %%s %%s :\002%s\002", reason);
		len = strlen(buf);
		for (new = masskick_list; new; new = new->next)
		{
			send_buf = m_s3cat(&send_buf, ",", new->filter);
			num++;
			if ((get_int_var(NUM_KICKS_VAR) && (num == get_int_var(NUM_KICKS_VAR))) || (strlen(send_buf)+len) >= (IRCD_BUFFER_SIZE - (NICKNAME_LEN + 5)))
			{
				num = 0;
				send_to_server(buf, chan->channel, send_buf);
				new_free(&send_buf);
			}
		}
		if (send_buf)
			send_to_server(buf, chan->channel, send_buf);
		new_free(&send_buf);
		shitlist_erase(&masskick_list);
	}
	else
		bitchsay("No matches for mass kick of %s on %s", spec, chan->channel);
}

BUILT_IN_COMMAND(mknu)
{
	ChannelList *chan;
	NickList *nicks;
	char *to = NULL, *rest;
	int count;
	int server = from_server;
	int kickops;
	
	if (args && (is_channel(args) || !strncmp(args, "* ", 2) || !strcmp(args, "*")))
		to = next_arg(args, &args);
	
	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;

	rest = args;
	if (rest && !*rest)
		rest = NULL;

	kickops = get_cset_int_var(chan->csets, KICK_OPS_CSET);
	count = 0;
	for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks))
	{
		if (nicks->userlist || (nick_isop(nicks) && !kickops) || isme(nicks->nick))
			continue;
		count++;
		send_to_server("KICK %s %s :(non-users) \002%s\002", chan->channel, nicks->nick, rest ? rest : get_reason(nicks->nick, NULL));
	}
	if (!count)
		say("No matches for masskick of non-users on %s", chan->channel);
}

BUILT_IN_COMMAND(masskickban)
{
	ChannelList *chan;
register NickList *nicks;
	char *to = NULL, *spec, *rest;
	int count, all = 0;
	int server = from_server;
	char	buffer[BIG_BUFFER_SIZE + 1];
	char tempbuf[BIG_BUFFER_SIZE+1];
	
	
	rest = NULL;
	spec = NULL;

	if (!(to = next_arg(args, &args)))
		to = NULL;
	if (to && !is_channel(to))
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server,to, NEED_OP)))
		return;

	if (!spec && !(spec = next_arg(args, &args)))
		return;
	if (args && !strncmp(args, "-all", 4))
	{
		all = 1;
		next_arg(args, &args);
	}
	rest = args;
	if (rest && !*rest)
		rest = NULL;

	count = 0;
	if (!strchr(spec, '!'))
	{
		strcpy(tempbuf, "*!");
		if (!strchr(spec, '@'))
			strcat(tempbuf, "*@");
		strcat(tempbuf, spec);
	}
	else
		strcpy(tempbuf, spec);

	send_to_server("MODE %s +b %s", chan->channel, tempbuf);
	for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks))
	{
		*buffer = '\0';
		strmopencat(buffer, IRCD_BUFFER_SIZE, nicks->nick, "!", nicks->host, NULL);
		if ((all || !nick_isop(nicks) || get_cset_int_var(chan->csets, KICK_OPS_CSET)) &&
		   !isme(nicks->nick) && wild_match(tempbuf, buffer))
		{
			count++;
			send_to_server("KICK %s %s :(%s) \002%s\002", chan->channel, nicks->nick, spec, rest ? rest : get_reason(nicks->nick, NULL));
		}
	}
	if (!count)
		say("No matches for masskickban of %s on %s", spec, chan->channel);
}

BUILT_IN_COMMAND(massban)
{
	ChannelList *chan;
register NickList *nicks;
	ShitList *massban_list = NULL, *tmp;
	char *to = NULL, *spec, *rest;
	char *buffer = NULL;
	int server = from_server;

	int maxmodes, all = 0;

	
	maxmodes = get_int_var(NUM_BANMODES_VAR);

	rest = NULL;
	spec = NULL;

	if (!(to = next_arg(args, &args)))
		to = NULL;
	if (to && !is_channel(to))
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;

	if (!spec && !(spec = next_arg(args, &args)))
		spec = "*!*@*";
	if (*spec == '-')
	{
		rest = spec;
		spec = "*!*@*";
	}
	else
		rest = args;

	if (rest && !my_stricmp(rest, "-all"))
		all = 1;

	for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks))
	{
		new_free(&buffer);
		malloc_sprintf(&buffer, "%s!%s", nicks->nick, nicks->host);

		if ((all || !nick_isop(nicks) || get_cset_int_var(chan->csets, KICK_OPS_CSET)) &&
		    !isme(nicks->nick) && wild_match(spec, buffer))
		{
			char *temp = NULL;
			char *p, *q;
			ShitList *new;
			temp = LOCAL_COPY(nicks->host);

			q = clear_server_flags(temp);
			p = strchr(temp, '@');
			*p++ = 0;
		
			new = (ShitList *)new_malloc(sizeof(ShitList));
			malloc_sprintf(&new->filter, "*!*%s@%s ", q, cluster(p));
			add_to_list((List **)&massban_list, (List *)new);
		}
	}
	new_free(&buffer);
	if (massban_list)
	{
		char modestr[100];
		int i = 0;
		
		bitchsay("Performing Mass Bans on %s", chan->channel);
		for (tmp = massban_list; tmp; tmp = tmp->next)
		{
			malloc_strcat(&buffer, tmp->filter);
			modestr[i] = 'b';
			i++;
			if (i > maxmodes)
			{
				modestr[i] = '\0';
				send_to_server("MODE %s +%s %s", chan->channel, modestr, buffer);
				i = 0;
				new_free(&buffer);
			}
		}
		modestr[i] = '\0';
		if (buffer && *buffer)
		{
			send_to_server("MODE %s +%s %s", chan->channel, modestr, buffer);
			new_free(&buffer);
		}
		shitlist_erase(&massban_list);
	} else
		say("No matches for massban of %s on %s", spec, chan->channel);
}

BUILT_IN_COMMAND(unban)
{
	char *to, *spec, *host;
	ChannelList *chan;
	BanList *bans;
	int count = 0;
	int server = from_server;
			
	
	to = spec = host = NULL;

	if (!(to = new_next_arg(args, &args)))
		to = NULL;
	if (to && !is_channel(to))
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;

	set_display_target(chan->channel, LOG_CRAP);
	if (!spec && !(spec = next_arg(args, &args)))
	{
		spec = "*";
		host = "*@*";
	}

	if (spec && *spec == '#')
	{
		count = atoi(spec);	
	}
	if (!strchr(spec, '*'))
	{
		userhostbase(spec, userhost_unban, 1, "%s %d", chan->channel, current_window->refnum);
		reset_display_target();
		return;
	}

	if (count)
	{
		int tmp = 1;
		for (bans = chan->bans; bans; bans = bans->next)
		{
			if (tmp == count)
			{
				if (bans->sent_unban == 0)
				{
					send_to_server("MODE %s -b %s", chan->channel, bans->ban);
					bans->sent_unban++;
					tmp = 0;
					break;
				}
			} else
				tmp++;
		}
		if (tmp != 0)
			count = 0;
	} 
	else 
	{
		char *banstring = NULL;
		int num = 0;
		count = 0;	
		for (bans = chan->bans; bans; bans = bans->next)
		{
			if (wild_match(bans->ban, spec) || wild_match(spec, bans->ban))
			{
				if (bans->sent_unban == 0)
				{
					m_s3cat(&banstring, space, bans->ban);
					bans->sent_unban++;
					count++;
					num++;
				}
			}
			if (count && (count % get_int_var(NUM_BANMODES_VAR) == 0))
			{
				send_to_server("MODE %s -%s %s", chan->channel, strfill('b', num), banstring);
				new_free(&banstring);
				num = 0;
			}
		}
		if (banstring && num)
			send_to_server("MODE %s -%s %s", chan->channel, strfill('b', num), banstring);
		new_free(&banstring);
	}
	if (!count)
		bitchsay("No ban matching %s found", spec);
	reset_display_target();
}

BUILT_IN_COMMAND(dokick)
{
	char	*to = NULL, 
		*spec = NULL,
		*reason = NULL;
	ChannelList *chan;
	int server = from_server;

	
	if (!(to = next_arg(args, &args)))
		to = NULL;

	if (to && !is_channel(to))
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server, (to && !strcmp(to, "*"))? NULL : to, NEED_OP)))
		return;
	set_display_target(chan->channel, LOG_KICK);

	if (!spec && !(spec = next_arg(args, &args)))
		return;
	if (args && *args)
		reason = args;
	else
		reason = get_reason(spec, NULL);

	send_to_server("KICK %s %s :%s", chan->channel, spec, reason);
	reset_display_target();
}

BUILT_IN_COMMAND(kickban)
{
	char	*to = NULL, 
		*tspec = NULL,
		*spec = NULL, 
		*tnick = NULL,
		*rest = NULL;

	ChannelList *chan;
	NickList *nicks;
	int count = 0;
	int server = from_server;
	int set_ignore = 0;
	int kick_first = 0;		
	time_t time = 0;
	
	
	if (!(to = next_arg(args, &args)))
		to = NULL;

	if (to && !is_channel(to))
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;
	set_display_target(chan->channel, LOG_KICK);
	if (command)
	{
		if (!strstr(command, "KB"))
			kick_first = 1;
		set_ignore = (command[strlen(command)-1] == 'I') ? 1 : 0;
	}
	if (!spec && !(spec = new_next_arg(args, &args)))
	{
		reset_display_target();
		return;
	}
	if (command && (!my_stricmp(command, "TBK") || !my_stricmp(command, "TKB")))
	{
		char *string_time;
		time = get_cset_int_var(chan->csets, BANTIME_CSET);
		if ((string_time = next_arg(args, &args)))
			time = atoi(string_time);
		malloc_sprintf(&rest, "Timed kickban for %s", convert_time(time));
		rest = args;
		if (rest && !*rest)
			rest = NULL;
	}
	else
	{
		rest = args;
		if (rest && !*rest)
			rest = NULL;
	}
	tspec = alloca(strlen(spec)+1);
	strcpy(tspec, spec);
	while ((tnick = next_in_comma_list(tspec, &tspec)))
	{
		int exact = 1;
		if (!tnick || !*tnick) break;
		exact = strchr(tnick, '*') ? 0 : 1;
		for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks))
		{
			char *p, *user, *host;
			if (isme(nicks->nick))
				continue;
			if (exact && my_stricmp(nicks->nick, tnick))
				continue;
			else if (!exact && !wild_match(tnick, nicks->nick))
				continue;
			p = LOCAL_COPY(nicks->host);
			user = clear_server_flags(p);
			host = strchr(user, '@');
			*host++ = 0;
			if (kick_first)
				send_to_server("KICK %s %s :%s\r\nMODE %s +b %s", 
					chan->channel, nicks->nick, rest ? rest : get_reason(nicks->nick, NULL),
					chan->channel, ban_it(nicks->nick, user, host, nicks->ip));
			else
				send_to_server("MODE %s -o+b %s %s\r\nKICK %s %s :%s", 
					chan->channel, nicks->nick, ban_it(nicks->nick, user, host, nicks->ip),
					chan->channel, nicks->nick, rest ? rest : get_reason(nicks->nick, NULL));
			count++;
			if (command && (!my_stricmp(command, "TKB") || !my_stricmp(command, "TBK")))
				add_timer(0, empty_string, time*1000, 1, timer_unban, m_sprintf("%d %s %s", from_server, chan->channel, ban_it(nicks->nick, user, host, nicks->ip)), NULL, -1, "timed-unban");
			else if (command && !my_stricmp(command, "FUCK"))
			{
				char *temp = NULL;
				malloc_sprintf(&temp, "%s!*%s@%s %s 3 Auto-Shit", nicks->nick, user, host, chan->channel);
#ifdef WANT_USERLIST
				add_shit(NULL, temp, NULL, NULL);
#endif
				new_free(&temp);
			} else if (set_ignore)
				ignore_nickname(ban_it("*", user, host, NULL), IGNORE_ALL, 0);
		}
	}
	if (!count && !isme(spec))
		userhostbase(spec, userhost_ban, 1, "%s %s %s", chan->channel, spec, command ? (!strcmp(command, "FUCK") ? "FUCK": set_ignore ? "BKI":empty_string):empty_string);
	reset_display_target();
}

BUILT_IN_COMMAND(ban)
{
	char	*to = NULL, 
		*spec = NULL, 
		*rest = NULL;
	ChannelList *chan;
	NickList *nicks;
	int server = from_server;
	int found = 0;
	
	
	if (!(to = next_arg(args, &args)))
		to = NULL;

	if (to && !is_channel(to))
	{
		spec = to;
		to = NULL;
	}

	if (!(chan = prepare_command(&server, to, NEED_OP)))
		return;

	if (!spec && !(spec = new_next_arg(args, &args)))
		return;
	rest = args;

	if (rest && !*rest)
		rest = NULL;

	for (nicks = next_nicklist(chan, NULL); nicks; nicks = next_nicklist(chan, nicks))
	{
		if (!my_stricmp(spec, nicks->nick))
		{
			char *t, *host, *user;
			t = LOCAL_COPY(nicks->host);
			user = clear_server_flags(t);
			host = strchr(user, '@');
			*host++ = 0;

			send_to_server("MODE %s -o+b %s %s", chan->channel, nicks->nick, ban_it(nicks->nick, user, host, nicks->ip));
			found++;
			
		}
	}
	if (!found)
	{
		if (strchr(spec, '!') && strchr(spec, '@'))
			send_to_server("MODE %s +b %s", chan->channel, spec);
		else
			userhostbase(spec, userhost_ban, 1, "%s %s", chan->channel, spec);
	}
}

BUILT_IN_COMMAND(banstat)
{
char *channel = NULL, *tmp = NULL, *check = NULL;
ChannelList *chan;
BanList *tmpc;
int count = 1;
int server;

	
	if (args && *args)
	{
		tmp = next_arg(args, &args);
		if (*tmp == '#' && is_channel(tmp))
			malloc_strcpy(&channel, tmp);
		else
			malloc_strcpy(&check, tmp);
		if (args && *args && channel)
		{
			tmp = next_arg(args, &args);			
			malloc_strcpy(&check, tmp);
		}		
	}

	if ((chan = prepare_command(&server, channel, NO_OP)))
	{
		if (!chan->bans && !chan->exemptbans)
		{
			bitchsay("No bans on %s", chan->channel);
			return;
		}
		if (chan->bans)
		{
			if ((do_hook(BANS_HEADER_LIST, "%s %s %s %s %s", "#", "Channel", "Ban", "SetBy", "Seconds")))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_HEADER_FSET), NULL));
			for (tmpc = chan->bans; tmpc; tmpc = tmpc->next, count++)
			{
				if (check && (!wild_match(check, tmpc->ban) || !wild_match(tmpc->ban, check)))
					continue;
				if (do_hook(BANS_LIST, "%d %s %s %s %lu", count, chan->channel, tmpc->ban, tmpc->setby?tmpc->setby:get_server_name(from_server), (unsigned long)tmpc->time))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_FSET), "%d %s %s %s %l", count, chan->channel, tmpc->ban, tmpc->setby?tmpc->setby:get_server_name(from_server), (unsigned long)tmpc->time));
			}
			if (do_hook(BANS_FOOTER_LIST, "%d %s", count, chan->channel))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_FOOTER_FSET), "%d %s", count, chan->channel));
		}
		if (chan->exemptbans)
		{
			count = 1;
			if ((do_hook(EBANS_HEADER_LIST, "%s %s %s %s %s", "#", "Channel", "ExBan", "SetBy", "Seconds")))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_EBANS_HEADER_FSET), NULL));
			for (tmpc = chan->exemptbans; tmpc; tmpc = tmpc->next, count++)
			{
				if (check && (!wild_match(check, tmpc->ban) || !wild_match(tmpc->ban, check)))
					continue;
				if (do_hook(EBANS_LIST, "%d %s %s %s %lu", count, chan->channel, tmpc->ban, tmpc->setby?tmpc->setby:get_server_name(from_server), (unsigned long)tmpc->time))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_EBANS_FSET), "%d %s %s %s %l", count, chan->channel, tmpc->ban, tmpc->setby?tmpc->setby:get_server_name(from_server), (unsigned long)tmpc->time));
			}
			if (do_hook(EBANS_FOOTER_LIST, "%d %s", count, chan->channel))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_EBANS_FOOTER_FSET), "%d %s", count, chan->channel));
		}
		new_free(&check);
		new_free(&channel);
	} 
	else if (channel)
		send_to_server("MODE %s b", channel);
}

void remove_bans (char *stuff, char *line)
{
ChannelList *chan;
int count = 1;
BanList *tmpc, *next;
char *banstring = NULL;
int num = 0;
int server = from_server;

	
	if (stuff && (chan = prepare_command(&server, stuff, NEED_OP)))
	{
		int done = 0;	
		if (!chan->bans)
		{
			bitchsay("No bans on %s", stuff);
			return;
		}

		for (tmpc = chan->bans; tmpc; tmpc = tmpc->next, count++)
			if (!tmpc->sent_unban && (matchmcommand(line, count)))
			{
				malloc_strcat(&banstring, tmpc->ban);
				malloc_strcat(&banstring, space);
				num++;
				tmpc->sent_unban++;
				if (num % get_int_var(NUM_BANMODES_VAR) == 0)
				{
					send_to_server("MODE %s -%s %s", stuff, strfill('b', num), banstring);
					new_free(&banstring);
					num = 0;
				}
				done++;
			}
		if (banstring && num)
			send_to_server("MODE %s -%s %s", stuff, strfill('b', num), banstring);
		new_free(&banstring);
		num = 0;
		if (!done)
		{
			for (tmpc = chan->exemptbans; tmpc; tmpc = tmpc->next, count++)
				if (!tmpc->sent_unban && (matchmcommand(line, count)))
				{
					malloc_strcat(&banstring, tmpc->ban);
					malloc_strcat(&banstring, space);
					num++;
					tmpc->sent_unban++;
					if (num % get_int_var(NUM_BANMODES_VAR) == 0)
					{
						send_to_server("MODE %s -%s %s", stuff, strfill('e', num), banstring);
						new_free(&banstring);
						num = 0;
					}
					done++;
				}
		}
		if (banstring && num)
			send_to_server("MODE %s -%s %s", stuff, strfill('b', num), banstring);
		for (tmpc = chan->bans; tmpc; tmpc = next)
		{
			next = tmpc->next;
			if (tmpc->sent_unban)
			{
				if ((tmpc = (BanList *)remove_from_list((List**)&chan->bans, tmpc->ban)))
				{
					new_free(&tmpc->ban);
					new_free(&tmpc->setby);
					new_free((char **)&tmpc);
					tmpc = NULL;
				}
			}
		}
		for (tmpc = chan->exemptbans; tmpc; tmpc = next)
		{
			next = tmpc->next;
			if (tmpc->sent_unban)
			{
				if ((tmpc = (BanList *)remove_from_list((List**)&chan->exemptbans, tmpc->ban)))
				{
					new_free(&tmpc->ban);
					new_free(&tmpc->setby);
					new_free((char **)&tmpc);
					tmpc = NULL;
				}
			}
		}
		new_free(&banstring);
	}	
}

BUILT_IN_COMMAND(tban)
{
ChannelList *chan;
int count = 1;
BanList *tmpc;
int server;

	
	if ((chan = prepare_command(&server, NULL, NEED_OP)))
	{
	
		if (!chan->bans)
		{
			bitchsay("No bans on %s", chan->channel);
			return;
		}
		if ((do_hook(BANS_HEADER_LIST, "%s %s %s %s %s", "#", "Channel", "Ban", "SetBy", "Seconds")))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_HEADER_FSET), NULL));
		for (tmpc = chan->bans; tmpc; tmpc = tmpc->next, count++)
			if (do_hook(BANS_LIST, "%d %s %s %s %lu", count, chan->channel, tmpc->ban, tmpc->setby?tmpc->setby:get_server_name(from_server), (unsigned long)tmpc->time))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_FSET), "%d %s %s %s %l", count, chan->channel, tmpc->ban, tmpc->setby?tmpc->setby:get_server_name(from_server), (unsigned long)tmpc->time));
		for (tmpc = chan->exemptbans; tmpc; tmpc = tmpc->next, count++)
			if (do_hook(BANS_LIST, "%d %s %s %s %lu", count, chan->channel, tmpc->ban, tmpc->setby?tmpc->setby:get_server_name(from_server), (unsigned long)tmpc->time))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_BANS_FSET), "%d %s %s %s %l", count, chan->channel, tmpc->ban, tmpc->setby?tmpc->setby:get_server_name(from_server), (unsigned long)tmpc->time));
		add_wait_prompt("Which ban to delete (-2, 2-5, ...) ? ", remove_bans, chan->channel, WAIT_PROMPT_LINE, 1);
	} 
}

static const char *bantypes[] = { "*Unknown*", "\002N\002ormal", 
	"\002B\002etter", "\002H\002ost", "\002D\002omain", 
	"\002U\002ser", "\002S\002crew", "\002I\002P" };

static void set_default_bantype(char value, char *helparg)
{
	switch(toupper(value))
	{
		case 'B':
			defban = 2;
			break;
		case 'H':
			defban = 3;
			break;
		case 'D':
			defban = 4;
			break;
		case 'I':
			defban = 7;
			break;
		case 'S':
			defban = 6;
			break;
		case 'U':
			defban = 5;
			break;
		case 'N':
			defban = 1;
			break;
		default :
			return;
	}
	bitchsay("BanType set to %s", 
		bantypes[defban >= 1 && defban <= 7 ? defban : 0]);
}

BUILT_IN_COMMAND(bantype)
{
	if (args && *args)
		set_default_bantype(*args, helparg);
	else
		bitchsay("Current BanType is %s", 
			bantypes[defban >= 1 && defban <= 7 ? defban : 0]);
}
