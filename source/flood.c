/*
 * flood.c: handle channel flooding. 
 *
 * This attempts to give you some protection from flooding.  Basically, it keeps
 * track of how far apart (timewise) messages come in from different people.
 * If a single nickname sends more than 3 messages in a row in under a
 * second, this is considered flooding.  It then activates the ON FLOOD with
 * the nickname and type (appropriate for use with IGNORE). 
 *
 * Thanks to Tomi Ollila <f36664r@puukko.hut.fi> for this one. 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: flood.c 26 2008-04-30 13:57:56Z keaston $";
CVS_REVISION(flood_c)
#include "struct.h"

#include "alias.h"
#include "hook.h"
#include "ircaux.h"
#include "ignore.h"
#include "flood.h"
#include "vars.h"
#include "output.h"
#include "list.h"
#include "misc.h"
#include "server.h"
#include "userlist.h"
#include "timer.h"
#include "ignore.h"
#include "status.h"
#include "hash2.h"
#include "cset.h"
#define MAIN_SOURCE
#include "modval.h"

static	char	*ignore_types[] =
{
	"",
	"MSG",
	"PUBLIC",
	"NOTICE",
	"WALL",
	"WALLOP",
	"CTCP",
	"INVITE",
	"CDCC",
	"ACTION",
	"NICK",
	"DEOP",
	"KICK",
	"JOIN"
};

#define FLOOD_HASHSIZE 31
HashEntry no_flood_list[FLOOD_HASHSIZE];
HashEntry flood_list[FLOOD_HASHSIZE];

static int remove_oldest_flood_hashlist(HashEntry *, time_t, int);




extern	char	*FromUserHost;
extern	unsigned int window_display;
extern	int	from_server;

static double allow_flood = 0.0;
static double this_flood = 0.0;

#define NO_RESET 0
#define RESET 1

char *get_flood_types(unsigned int type)
{
int x = 0;
	while (type)
	{
		type = type >> 1;
		x++;
	}
	return ignore_types[x];
}

#if 0
int get_flood_rate(int type, ChannelList * channel)
{
	int flood_rate = get_int_var(FLOOD_RATE_VAR);
	if (channel)
	{
		switch(type)
		{
			case JOIN_FLOOD:
				flood_rate = get_cset_int_var(channel->csets, JOINFLOOD_TIME_CSET);
				break;
			case PUBLIC_FLOOD:
				flood_rate = get_cset_int_var(channel->csets, PUBFLOOD_TIME_CSET);
				break;
			case NICK_FLOOD:
				flood_rate = get_cset_int_var(channel->csets, NICKFLOOD_TIME_CSET);
				break;
			case KICK_FLOOD:
				flood_rate = get_cset_int_var(channel->csets, KICKFLOOD_TIME_CSET);
				break;
			case DEOP_FLOOD:
				flood_rate = get_cset_int_var(channel->csets, DEOPFLOOD_TIME_CSET);
				break;
			default:
				break;
		}
	}
	else
	{
		switch(type)
		{
			case CDCC_FLOOD:
				flood_rate = get_int_var(CDCC_FLOOD_RATE_VAR);
				break;
			case CTCP_FLOOD:
				flood_rate = get_int_var(CTCP_FLOOD_RATE_VAR);
			case CTCP_ACTION_FLOOD:
			default:
				break;
		}
	}
	return flood_rate;
}

int get_flood_count(int type, ChannelList * channel)
{
	int flood_count = get_int_var(FLOOD_AFTER_VAR);
	if (channel) {
		switch(type)
		{
			case JOIN_FLOOD:
				flood_count = get_cset_int_var(channel->csets, KICK_ON_JOINFLOOD_CSET);
				break;
			case PUBLIC_FLOOD:
				flood_count = get_cset_int_var(channel->csets, KICK_ON_PUBFLOOD_CSET);
				break;
			case NICK_FLOOD:
				flood_count = get_cset_int_var(channel->csets, KICK_ON_NICKFLOOD_CSET);
				break;
			case KICK_FLOOD:
				flood_count = get_cset_int_var(channel->csets, KICK_ON_KICKFLOOD_CSET);
				break;
			case DEOP_FLOOD:
				flood_count = get_cset_int_var(channel->csets, KICK_ON_DEOPFLOOD_CSET);
				break;
			default:
			break;
		}
	} 
	else
	{
		switch(type)
		{
			case CDCC_FLOOD:
				flood_count = get_int_var(CDCC_FLOOD_AFTER_VAR);
				break;
			case CTCP_FLOOD:
				flood_count = get_int_var(CTCP_FLOOD_AFTER_VAR);
			case CTCP_ACTION_FLOOD:
			default:
				break;
		}
	}
	return flood_count;
}
#endif

void get_flood_val(ChannelList *chan, int type, int *flood_count, int *flood_rate)
{
	*flood_count = get_int_var(FLOOD_AFTER_VAR);
	*flood_rate = get_int_var(FLOOD_RATE_VAR);
	if (chan)
	{
		switch(type)
		{
			case JOIN_FLOOD:
				*flood_count = get_cset_int_var(chan->csets, KICK_ON_JOINFLOOD_CSET);
				*flood_rate = get_cset_int_var(chan->csets, JOINFLOOD_TIME_CSET);
				break;
			case PUBLIC_FLOOD:
				*flood_count = get_cset_int_var(chan->csets, KICK_ON_PUBFLOOD_CSET);
				*flood_rate = get_cset_int_var(chan->csets, PUBFLOOD_TIME_CSET);
				break;
			case NICK_FLOOD:
				*flood_count = get_cset_int_var(chan->csets, KICK_ON_NICKFLOOD_CSET);
				*flood_rate = get_cset_int_var(chan->csets, NICKFLOOD_TIME_CSET);
				break;
			case KICK_FLOOD:
				*flood_count = get_cset_int_var(chan->csets, KICK_ON_KICKFLOOD_CSET);
				*flood_rate = get_cset_int_var(chan->csets, KICKFLOOD_TIME_CSET);
				break;
			case DEOP_FLOOD:
				*flood_count = get_cset_int_var(chan->csets, KICK_ON_DEOPFLOOD_CSET);
				*flood_rate = get_cset_int_var(chan->csets, DEOPFLOOD_TIME_CSET);
				break;
			default:
			break;
		}
	}
	else
	{
		switch(type)
		{
			case CDCC_FLOOD:
				*flood_count = get_int_var(CDCC_FLOOD_AFTER_VAR);
				*flood_rate = get_int_var(CDCC_FLOOD_RATE_VAR);
				break;
			case CTCP_FLOOD:
				*flood_count = get_int_var(CTCP_FLOOD_AFTER_VAR);
				*flood_rate = get_int_var(CTCP_FLOOD_RATE_VAR);
			case CTCP_ACTION_FLOOD:
			default:
				break;
		}
	}
}

int set_flood(int type, time_t flood_time, int reset, NickList *tmpnick)
{
	if (!tmpnick)
		return 0;
	switch(type)
	{
		case JOIN_FLOOD:
			if (reset == RESET)
			{
				tmpnick->joincount = 1; 
				tmpnick->jointime = flood_time;
			} else tmpnick->joincount++;
			break;
		case PUBLIC_FLOOD:
			if (reset == RESET)
			{
				tmpnick->floodcount = 1;
				tmpnick->floodtime = tmpnick->idle_time = flood_time;
			} else tmpnick->floodcount++;
			break;
		case NICK_FLOOD:
			if (reset == RESET)
			{
				tmpnick->nickcount = 1;
				tmpnick->nicktime = flood_time;
			} else tmpnick->nickcount++;
			break;
		case KICK_FLOOD:
			if (reset == RESET)
			{
				tmpnick->kickcount = 1;
				tmpnick->kicktime = flood_time;
			} else tmpnick->kickcount++;
			break;
		case DEOP_FLOOD:
			if (reset == RESET)
			{
				tmpnick->dopcount = 1;
				tmpnick->doptime = flood_time;
			} else tmpnick->dopcount++;
			break;
		default:
		break;
	}
	return 1;
}

int BX_is_other_flood(ChannelList *channel, NickList *tmpnick, int type, int *t_flood)
{
time_t diff = 0, flood_time = 0;
int doit = 0;
int count = 0;
int flood_rate = 0, flood_count = 0;

	flood_time = now;
	
	
	if (!channel || !tmpnick)
		return 0;
	if (isme(tmpnick->nick))
		return 0;
	if (find_name_in_genericlist(tmpnick->nick, no_flood_list, FLOOD_HASHSIZE, 0))
		return 0;
	set_flood(type, flood_time, NO_RESET, tmpnick);
	switch(type)
	{
		case JOIN_FLOOD:
			if (!get_cset_int_var(channel->csets, JOINFLOOD_CSET))
				break;
			diff = flood_time - tmpnick->jointime;
			count = tmpnick->joincount;
			doit = 1;
			break;
		case PUBLIC_FLOOD:
			if (!get_cset_int_var(channel->csets, PUBFLOOD_CSET))
				break;
			diff = flood_time - tmpnick->floodtime;
			count = tmpnick->floodcount;
			doit = 1;
			break;
		case NICK_FLOOD:
			if (!get_cset_int_var(channel->csets, NICKFLOOD_CSET))
				break;
			diff = flood_time - tmpnick->nicktime;
			count = tmpnick->nickcount;
			doit = 1;
			break;
		case DEOP_FLOOD:
			if (!get_cset_int_var(channel->csets, DEOPFLOOD_CSET))
				break;
			diff = flood_time - tmpnick->doptime;
			count = tmpnick->dopcount;
			doit = 1;
			break;
		case KICK_FLOOD:
			if (!get_cset_int_var(channel->csets, KICKFLOOD_CSET))
				break;
			diff = flood_time - tmpnick->kicktime;
			count = tmpnick->kickcount;
			doit = 1;
			break;
		default:
			return 0;
			break;
	}
	if (doit)
	{
		int is_user = 0;
		if (!get_int_var(FLOOD_PROTECTION_VAR))
			return 0;
		get_flood_val(channel, type, &flood_count, &flood_rate);
		if ((tmpnick->userlist && (tmpnick->userlist->flags & ADD_FLOOD)))
			is_user = 1;
		if (!is_user && (count >= flood_count))
		{
			int flooded = 0;
			if (count >= flood_count)
			{
				if (!diff || (flood_rate && (diff < flood_rate)))
				{
					*t_flood = diff;
					flooded = 1;
					do_hook(FLOOD_LIST, "%s %s %s %s", tmpnick->nick, get_flood_types(type),channel?channel->channel:zero, tmpnick->host);
				}
				set_flood(type, flood_time, RESET, tmpnick);
				return flooded;
			}
			else if (diff > flood_rate)
				set_flood(type, flood_time, RESET, tmpnick);
		}
	} 
	return 0;
} 

/*
 * check_flooding: This checks for message flooding of the type specified for
 * the given nickname.  This is described above.  This will return 0 if no
 * flooding took place, or flooding is not being monitored from a certain
 * person.  It will return 1 if flooding is being check for someone and an ON
 * FLOOD is activated. 
 */

int BX_check_flooding(char *nick, int type, char *line, char *channel)
{
static	int	users = 0,
		pos = 0;
time_t flood_time = now,
		diff = 0;

Flooding 	*tmp;
int		flood_rate, 
		flood_count;


	if (!(users = get_int_var(FLOOD_USERS_VAR)) || !*FromUserHost)
		return 1;
	if (find_name_in_genericlist(nick, no_flood_list, FLOOD_HASHSIZE, 0))
		return 1;
	if (!(tmp = find_name_in_floodlist(nick, FromUserHost, flood_list, FLOOD_HASHSIZE, 0)))
	{
		if (pos >= users)
		{
			pos -= remove_oldest_flood_hashlist(&flood_list[0], 0, (users + 1 - pos));
		}
		tmp = add_name_to_floodlist(nick, FromUserHost, channel, flood_list, FLOOD_HASHSIZE);
		tmp->type = type;
		tmp->cnt = 1;
		tmp->start = flood_time;
		tmp->flood = 0;
		pos++;
		return 1;
	} 
	if (!(tmp->type & type))
	{
		tmp->type |= type; 
		return 1;
	}

#if 0
	flood_count = get_flood_count(type, NULL); /* FLOOD_AFTER_VAR */
	flood_rate = get_flood_rate(type, NULL); /* FLOOD_RATE_VAR */
#endif
	get_flood_val(NULL, type, &flood_count, &flood_rate);
	if (!flood_count || !flood_rate)
		return 1;
	tmp->cnt++;
	if (tmp->cnt > flood_count)
	{
		int ret;
		diff = flood_time - tmp->start;
		if (diff != 0)
			this_flood = (double)tmp->cnt / (double)diff;
		else
			this_flood = 0;
		allow_flood = (double)flood_count / (double)flood_rate;
		if (!diff || !this_flood || (this_flood > allow_flood))
		{
			if (tmp->flood == 0)
			{
				tmp->flood = 1;
				if ((ret = do_hook(FLOOD_LIST, "%s %s %s %s", nick, get_flood_types(type),channel?channel:zero, line)) != 1)
					return ret;
				switch(type)
				{
					case WALL_FLOOD:
					case MSG_FLOOD:
					case NOTICE_FLOOD:
					case CDCC_FLOOD:
					case CTCP_FLOOD:
						if (flood_prot(nick, FromUserHost, get_flood_types(type), type, get_int_var(IGNORE_TIME_VAR), channel))
							return 0;
						break;
					case CTCP_ACTION_FLOOD:
						if (flood_prot(nick, FromUserHost, get_flood_types(CTCP_FLOOD), type, get_int_var(IGNORE_TIME_VAR), channel))
							return 0;
					default:
						break;
				}
				if (get_int_var(FLOOD_WARNING_VAR))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_FLOOD_FSET), "%s %s %s %s %s", update_clock(GET_TIME), get_flood_types(type), nick, FromUserHost, channel?channel:"unknown"));
			}
			return 1;
		}
		else
		{
			tmp->flood = 0;
			tmp->cnt = 1;
			tmp->start = flood_time;
		}
	}
	return 1;
}

void check_ctcp_ban_flood(char *channel, char *nick)
{
NickList *Nick = NULL;
ChannelList *chan = NULL;
	for (chan = get_server_channels(from_server); chan; chan = chan->next)
		if ((Nick = find_nicklist_in_channellist(nick, chan, 0)))
			break;
	if (chan && chan->have_op && get_cset_int_var(chan->csets, CTCP_FLOOD_BAN_CSET) && Nick)
	{
		if (!Nick->userlist || (Nick->userlist && !(Nick->userlist->flags & ADD_FLOOD)))
		{
			if (!nick_isop(Nick) || get_cset_int_var(chan->csets, KICK_OPS_CSET))
			{
				char *ban, *u, *h;
				u = alloca(strlen(Nick->host)+1);
				strcpy(u, Nick->host);
				h = strchr(u, '@');
				*h++ = 0;
				ban = ban_it(Nick->nick, u, h, Nick->ip);
				if (!ban_is_on_channel(ban, chan) && !eban_is_on_channel(ban, chan))
					send_to_server("MODE %s +b %s", chan->channel, ban);
			}
		}
	}
}

int BX_flood_prot (char *nick, char *userhost, char *type, int ctcp_type, int ignoretime, char *channel)
{
ChannelList *chan;
NickList *Nick;
char tmp[BIG_BUFFER_SIZE+1];
char *uh;
int	old_window_display;
int	kick_on_flood = 1;

	if ((ctcp_type == CDCC_FLOOD || ctcp_type == CTCP_FLOOD || ctcp_type == CTCP_ACTION_FLOOD) && !get_int_var(CTCP_FLOOD_PROTECTION_VAR))
		return 0;
	else if (!get_int_var(FLOOD_PROTECTION_VAR))
		return 0;
	else if (!my_stricmp(nick, get_server_nickname(from_server)))
		return 0;
	switch (ctcp_type)
	{
		case WALL_FLOOD:
		case MSG_FLOOD:
			break;
		case NOTICE_FLOOD:
			break;
		case PUBLIC_FLOOD:
			if (channel)
			{
				if ((chan = lookup_channel(channel, from_server, 0)))
				{
					kick_on_flood = get_cset_int_var(chan->csets, PUBFLOOD_CSET);
					if (kick_on_flood && (Nick = find_nicklist_in_channellist(nick, chan, 0)))
					{
						if (chan->have_op && (!Nick->userlist || (Nick->userlist && !(Nick->userlist->flags & ADD_FLOOD))))
							if (!nick_isop(Nick) || get_cset_int_var(chan->csets, KICK_OPS_CSET))
								send_to_server("KICK %s %s :\002%s\002 flooder", chan->channel, nick, type);
					} 
				}
			}
			break;
		case CTCP_FLOOD:
		case CTCP_ACTION_FLOOD:
			check_ctcp_ban_flood(channel, nick);
		default:
			if (get_int_var(FLOOD_KICK_VAR) && kick_on_flood && channel)
			{
				for (chan = get_server_channels(from_server); chan; chan = chan->next)
				{
					if (chan->have_op && (Nick = find_nicklist_in_channellist(nick, chan, 0)))
					{
						if ((!Nick->userlist || (Nick->userlist && !(Nick->userlist->flags & ADD_FLOOD))))
							if (!nick_isop(Nick) || get_cset_int_var(chan->csets, KICK_OPS_CSET))
								send_to_server("KICK %s %s :\002%s\002 flooder", chan->channel, nick, type);
					}
				}
			}
	}
	if (!ignoretime)
		return 0;
	uh = clear_server_flags(userhost);
	sprintf(tmp, "*!*%s", uh);
	old_window_display = window_display;
	window_display = 0;
	ignore_nickname(tmp, ignore_type(type, strlen(type)), 0);
	window_display = old_window_display;
	sprintf(tmp, "%d ^IGNORE *!*%s NONE", ignoretime, uh);
	timercmd("TIMER", tmp, NULL, NULL);
	bitchsay("Auto-ignoring %s for %d minutes [\002%s\002 flood]", nick, ignoretime/60, type);
	return 1;
}

static int remove_oldest_flood_hashlist(HashEntry *list, time_t timet, int count)
{
Flooding *ptr;
register time_t t;
int total = 0;
register unsigned long x;
	t = now;
	if (!count)
	{
		for (x = 0; x < FLOOD_HASHSIZE; x++)
		{
			ptr = (Flooding *) (list + x)->list;
			if (!ptr || !*ptr->name)
				continue;
			while (ptr)
			{
				if ((ptr->start + timet) <= t)
				{
					if (!(ptr = find_name_in_floodlist(ptr->name, ptr->host, flood_list, FLOOD_HASHSIZE, 1)))
						continue;
					new_free(&(ptr->channel));
					new_free(&(ptr->name));
					new_free(&ptr->host);
					new_free((char **)&ptr);
					total++;
					ptr = (Flooding *) (list + x)->list;
				} else ptr = ptr->next;
			}
		}
	}
	else
	{
		for (x = 0; x < FLOOD_HASHSIZE; x++)
		{
			Flooding *next = NULL;
			ptr = (Flooding *) (list + x)->list;
			if (!ptr || !*ptr->name)
				continue;
			while(ptr && count)
			{
				if ((ptr = find_name_in_floodlist(ptr->name, ptr->host, flood_list, FLOOD_HASHSIZE, 1)))
				{
					next = ptr->next;
					new_free(&(ptr->channel));
					new_free(&(ptr->name));
					new_free(&ptr->host);
					new_free((char **)&ptr);
					total++; count--;			
					ptr = (Flooding *) (list + x)->list;
					ptr = next;
				}
			}
		}
	}
	return total;
}

void clean_flood_list()
{
	remove_oldest_flood_hashlist(&flood_list[0], get_int_var(FLOOD_RATE_VAR)+1, 0);
}
