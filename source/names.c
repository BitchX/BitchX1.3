/*
 * names.c: This here is used to maintain a list of all the people currently
 * on your channel.  Seems to work 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */


#include "irc.h"
static char cvsrevision[] = "$Id: names.c 137 2011-09-06 06:48:57Z keaston $";
CVS_REVISION(names_c)
#include "struct.h"

#include "ircaux.h"
#include "names.h"
#include "flood.h"
#include "window.h"
#include "screen.h"
#include "server.h"
#include "lastlog.h"
#include "list.h"
#include "output.h"
#include "numbers.h"
#include "userlist.h"
#include "timer.h"
#include "input.h"
#include "hook.h"
#include "parse.h"
#include "whowas.h"
#include "misc.h"
#include "vars.h"
#include "keys.h"
#include "tcl_bx.h"
#include "status.h"
#include "userlist.h"
#include "hash2.h"
#include "cset.h"
#include "gui.h"
#define MAIN_SOURCE
#include "modval.h"

extern AJoinList *ajoin_list;


static	void	add_to_mode_list (char *, int, char *);
static	void	check_mode_list_join (char *, int);
static	void	show_channel (ChannelList *);
static	void	clear_channel (ChannelList *);
	char	*BX_recreate_mode (ChannelList *);
static	void	clear_mode_list (int);
static	void	apply_channel_modes (char *, char *, ChannelList *);
	void	BX_clear_bans (ChannelList *);

static	char	mode_str[] = "aciklmnprstzR";
	char	new_channel_format[BIG_BUFFER_SIZE];
	
static struct modelist
{
	char	*chan;
	int	server;
	char	*mode;
	struct modelist *next;
}	*mode_list = NULL;


static struct  joinlist
{
	char    *chan;
	int     server,
	gotinfo,
	winref;
	struct  timeval tv;
	struct  joinlist        *next;
}	*join_list = NULL;

/*
 * check_channel_type: checks if the given channel is a normal #channel
 * or a new !channel from irc2.10.  If the latter, then it reformats it
 * a bit into a more user-friendly form.
 */
char *	check_channel_type (char *channel)
{
	if (*channel != '!' || strlen(channel) < 6)
		return channel;

	sprintf(new_channel_format, "[%.6s] %s", channel, channel + 6);
	return new_channel_format;
}

                                                        
/* clear_channel: erases all entries in a nick list for the given channel */
static	void clear_channel(ChannelList *chan)
{
NickList *Nick, *n;
	while((Nick = next_nicklist(chan, NULL)))
	{
		n = find_nicklist_in_channellist(Nick->nick, chan, REMOVE_FROM_LIST);
		add_to_whowas_buffer(n, chan->channel, NULL, NULL);
	}
	clear_nicklist_hashtable(chan);
	chan->totalnicks = 0;
}

extern	ChannelList *BX_lookup_channel(char *channel, int server, int unlink)
{
	register ChannelList	*chan = NULL,
				*last = NULL;
	if (server <= -2)
		return NULL;
	if (server == -1)
	{
		server = primary_server;
		if (server == -1)
			return NULL;
	}
	if (!channel || !*channel || !strcmp(channel, "*"))
		channel = get_current_channel_by_refnum(0);
		
	chan = get_server_channels(server);
	if (!chan || !channel || !*channel)
		return NULL;
	while (chan)
	{
		if (chan->channel && !my_stricmp(chan->channel, channel))
		{
			if (unlink == CHAN_UNLINK)
			{
				if (last)
					last->next = chan->next;
				else
					set_server_channels(server, chan->next);
			}
			break;
		}
		last = chan;
		chan = chan->next;
	}
	return chan;
}

extern void set_waiting_channel (int i)
{
	Window	*tmp = NULL;

	
	while ((traverse_all_windows(&tmp)))
		if (tmp->server == i && tmp->current_channel)
		{
#if 0
			if (tmp->bind_channel)
				tmp->waiting_channel = tmp->current_channel;
			else
				new_free(&tmp->current_channel);
			tmp->current_channel = NULL;
#else
			if (tmp->waiting_channel)
				continue;
			tmp->waiting_channel = tmp->current_channel;
			tmp->current_channel = NULL;
#endif
		}
}

/* if the user is on the given channel, it returns 1. */
extern int BX_im_on_channel (char *channel, int server)
{
	
	return (channel?(lookup_channel(channel, server, 0) ? 1 : 0) : 0);
}

void add_userhost_to_channel (char *channel, char *nick, int server, char *userhost)
{
	NickList *new;
	ChannelList *chan;

	if ((chan = lookup_channel(channel, server, 0)) != NULL)
		if ((new = find_nicklist_in_channellist(nick, chan, 0)))
			malloc_strcpy(&new->host, userhost);
}


/*
 * add_channel: adds the named channel to the channel list.  If the channel
 * is already in the list, then the channel gets cleaned, and ready for use
 * again.   The added channel becomes the current channel as well.
 */
ChannelList * BX_add_channel(char *channel, int server, int refnum)
{
	ChannelList *new = NULL;
	WhowasChanList *whowaschan;
	
	
	if ((new = lookup_channel(channel, server, CHAN_NOUNLINK)) != NULL)
	{
		new->connected = 1;
		new->mode = 0;
		new->limit = 0;
		new_free(&new->s_mode);
		new->server = server;
		new->window = current_window;
		new->refnum = new->window->refnum;
		malloc_strcpy(&(new->channel), channel);
		clear_channel(new);
		clear_bans(new);
	}
	else
	{
		Window *tmp = NULL;		
		if (!(whowaschan = check_whowas_chan_buffer(channel, refnum, 1)))
		{
			
			new = (ChannelList *) new_malloc(sizeof(ChannelList));
			new->connected = 1;
			get_time(&new->channel_create);
			malloc_strcpy(&(new->channel), channel);
			add_to_list((List **)&new->csets, (List *)create_csets_for_channel(channel));
		}
		else
		{
			
			new = whowaschan->channellist;
			new_free(&whowaschan->channel);
			new_free((char **)&whowaschan);
			new_free(&(new->key));
			new->mode = 0;
			new_free(&new->s_mode);
			new->limit = new->i_mode = 0;
			clear_channel(new);
			clear_bans(new);
		}
		if ((!new->window  || !(tmp = get_window_by_refnum(new->window->refnum))) && current_window)
		{
			new->window = tmp ? tmp : current_window; 
			new->refnum = new->window->refnum;
		}
		new->server = server;
		new->flags.got_modes = new->flags.got_who = new->flags.got_bans = 1;
		get_time(&new->join_time);
		add_server_channels(server, new);
	}
	new->have_op = 0;
	new->voice = 0;
	new->hop = 0;
	
	if (!is_current_channel(channel, server, 0))
	{
		Window	*tmp = NULL;
		
		while ((traverse_all_windows(&tmp)))
		{
                        if (tmp->server != from_server)
                                continue;
			if (tmp->name && (!strcmp(tmp->name, "oper_view") || !strcmp(tmp->name, "debug")))
				continue;
                        if (!tmp->waiting_channel && !tmp->bind_channel)
                                 continue;
                        if (tmp->bind_channel && tmp->server == from_server)
                        {
				char *p, *q;
				p = LOCAL_COPY(tmp->bind_channel);
				while ((q = next_in_comma_list(p, &p)))
				{
					if (!q || !*q)
						break;
					if (my_stricmp(q, channel))
						continue;
	                                set_current_channel_by_refnum(tmp->refnum, channel);
        	                        new->window = tmp;
					new->refnum = tmp->refnum;
                        	        update_all_windows();
                                	return new;
				}
                        }
                        if (tmp->waiting_channel && tmp->server == from_server)
                        {
				char *p, *q;
				p = LOCAL_COPY(tmp->waiting_channel);
				while ((q = next_in_comma_list(p, &p)))
				{
					if (!q || !*q)
						break;
					if (my_stricmp(q, channel))
						continue;
					set_current_channel_by_refnum(tmp->refnum, channel);
					new->window = tmp;
					new->refnum = tmp->refnum;
					new_free(&tmp->waiting_channel);
					update_all_windows();
					return new;
				}
			}
		}
		set_current_channel_by_refnum(new->window?new->window->refnum:0, channel);
	}
	update_all_windows();
	return new;
}

/*
 * add_to_channel: adds the given nickname to the given channel.  If the
 * nickname is already on the channel, nothing happens.  If the channel is
 * not on the channel list, nothing happens (although perhaps the channel
 * should be addded to the list?  but this should never happen) 
 */
ChannelList *BX_add_to_channel(char *channel, char *nick, int server, int oper, int voice, char *userhost, char *server1, char *away, int serv_split, int server_hops)
{
	NickList *new = NULL;
	ChannelList *chan = NULL;
	WhowasList *whowas;

	int	ischop = oper;
	
	
	if (!(*channel == '*'))
	{
		if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
		{
			if (*nick == '+')
			{
				nick++;
				if (!my_stricmp(nick, get_server_nickname(server)))
				{
					check_mode_list_join(channel, server);
					chan->voice = 1;
				}
			}
			if (ischop && ischop != -1)
			{
				if (!my_stricmp(nick, get_server_nickname(server)))
				{
					check_mode_list_join(channel, server);
					chan->have_op = 1;
				}
			}	
			if (!(new = find_nicklist_in_channellist(nick, chan,  0)))
			{
			
				if (!(whowas = check_whowas_buffer(nick, userhost, channel)))
				{
					new = (NickList *) new_malloc(sizeof(NickList));

					new->idle_time = new->kicktime = 
					new->doptime = new->nicktime = 
					new->floodtime = new->bantime = now;

					new->joincount = 1;
					malloc_strcpy(&(new->nick), nick);
#ifdef WANT_USERLIST
					new->userlist = lookup_userlevelc("*", userhost, channel, NULL);
					new->shitlist = nickinshit(nick, userhost);
#endif
					new->serverhops = server_hops;
				} 
				else
				{
					new = whowas->nicklist;
					new_free(&whowas->channel);
					new_free(&whowas->server1);
					new_free(&whowas->server2);
					new_free((char **)&whowas);
					malloc_strcpy(&(new->nick), nick);

					new->idle_time = new->kicktime = 
					new->doptime = new->nicktime =
					new->floodtime = new->bantime = now;
	
					new->sent_reop = new->sent_deop = 
					new->bancount = new->nickcount = 
					new->dopcount = new->kickcount = 
					new->floodcount = new->ip_count = 
					new->sent_voice = 0;
					new->flags = 0;
					new->serverhops = server_hops;
					new->next = NULL;
				}
				if (server1)
					malloc_strcpy(&new->server, server1);			
				add_nicklist_to_channellist(new, chan);				
				update_stats(JOINLIST, new, chan, serv_split);
			}
			malloc_strcpy(&new->host, userhost);
			if (ischop > 0)
				new->flags |= NICK_CHANOP;
			if (voice > 0)
				new->flags |= NICK_VOICE;
			if (away)
			{
				if (strchr(away,'H'))
					new->flags &= ~NICK_AWAY;
				else
					new->flags |= NICK_AWAY;

				if (strchr(away, '*'))
					new->flags |= NICK_IRCOP;
                
                if (strchr(away, '%'))
                    new->flags |= NICK_HALFOP;
			}
		}
		else if (check_whowas_chan_buffer(channel, -1, 0))
		{
			if (*nick == '+') nick++;
			new = (NickList *) new_malloc(sizeof(NickList));
			new->idle_time = new->kicktime = 
			new->doptime = new->nicktime = 
			new->floodtime = new->bantime = now;
			new->joincount = 1;
			malloc_strcpy(&(new->nick), nick);
			malloc_strcpy(&new->host, userhost);
			new->serverhops = server_hops;
#ifdef WANT_USERLIST
			new->userlist = lookup_userlevelc("*", userhost, channel, NULL);
			new->shitlist = nickinshit(nick, userhost);
#endif
			if (ischop > 0)
				new->flags |= NICK_CHANOP;
			if (voice > 0)
				new->flags |= NICK_VOICE;
			if (away)
			{
				if (strchr(away,'H'))
					new->flags &= ~NICK_AWAY;
				else
					new->flags |= NICK_AWAY;

				if (strchr(away, '*'))
					new->flags |= NICK_IRCOP;
                
                if (strchr(away, '%'))
                    new->flags |= NICK_HALFOP;
			}
			if (server1)
				malloc_strcpy(&new->server, server1);			
			add_to_whowas_buffer(new, channel, server1, NULL);
		}
	}
	return chan;
}

char *BX_get_channel_key (char *channel, int server)
{
	ChannelList *tmp;

	if ((tmp = lookup_channel(channel, server, 0)) != NULL)
		return (tmp->key ? tmp->key : empty_string);
	return empty_string;
}



/*
 * recreate_mode: converts the bitmap representation of a channels mode into
 * a string 
 *
 * This malloces what it returns, but nothing that calls this function
 * is expecting to have to free anything.  Therefore, this function
 * should not malloc what it returns.  (hop)
 *
 * but this leads to horrible race conditions, so we add a bit to
 * the channel structure, and cache the string value of mode, and
 * the u_long value of the cached string, so that each channel only
 * has one copy of the string.  -mrg, june '94.
 */
char	*BX_recreate_mode(ChannelList *chan)
{
	int	mode_pos = 0,
		mode;
	static	char	*s;
	char	buffer[BIG_BUFFER_SIZE + 1];

	chan->i_mode = chan->mode;
	buffer[0] = 0;
	s = buffer;
	mode = chan->mode;
	
	if (!mode)
		return NULL;
	while (mode)
	{
		if (mode % 2)
			*s++ = mode_str[mode_pos];
		mode /= 2;
		mode_pos++;
	}
	if (chan->key && *chan->key)
	{
		*s++ = ' ';
		strcpy(s, chan->key);
		s += strlen(chan->key);
	}
	if (chan->limit)
		sprintf(s, " %d", chan->limit);
	else
		*s = 0;

	malloc_strcpy(&chan->s_mode, buffer);
	return chan->s_mode;
}
#ifdef COMPRESS_MODES
/* some structs to help along the process, NickList is way too much of a memory
   hog */

typedef struct _UserChanModes {
   struct _UserChanModes *next;
   char *nick;
   int o_ed;
   int v_ed;
   int deo_ed;
   int dev_ed;
   int b_ed;
   int deb_ed;
} UserChanModes;

/*
 * compress_modes: This will return a list of modes which removes duplicates
 * 
 * for instance:
 * +oooo Jordy Jordy Jordy Jordy
 *
 * would return:
 * +o Jordy
 *
 * +oo-oo Sheik Jordy Sheik Jordy
 *
 * would return:
 * -oo Sheik jordy
 *
 * also if *!*@* isn't banned on the channel
 * -b *!*@*
 *
 * will return NULL
 *
 * This is a frigging ugly function and because I'm not going to sit and
 * learn every shortcut function in irc, I don't care :)
 *
 * Written by Jordy (jordy@wserv.com)
 */
char *BX_do_compress_modes(ChannelList *chan, int server, char *channel, char *modes) {
int		add = 0, 
		isbanned = 0, 
		isopped = 0, 
		isvoiced = 0, 
		mod = -1;
char		*tmp = NULL, 
		*rest = NULL, 
		nmodes[16], 
		nargs[100];
UserChanModes	*ucm = NULL, 
		*tucm = NULL;
BanList		*tbl = NULL;
NickList	*tnl = NULL;

   /* now, modes contains the actual modes, and rest contains the arguments
      to those modes */

	if (!chan || !(modes = next_arg(modes, &rest)))
		return NULL;

   	*nmodes = 0;
   	*nargs = 0;
	for (; *modes && (strlen(nmodes) + 2) < sizeof nmodes; modes++) 
	{
		isbanned = isopped = isvoiced = 0;
		switch (*modes) 
		{
			case '+':
				add = 1;
				break;  
			case '-':
				add = 0;
				break;
			case 'o':
				if (!(tmp = next_arg(rest, &rest)))
					break;
				if (!(tucm = (UserChanModes *)find_in_list((List **)&ucm, tmp, 0)))
				{
					tucm = (UserChanModes *) alloca(sizeof(UserChanModes));
					memset(tucm, 0, sizeof(UserChanModes));
					tucm->nick = LOCAL_COPY(tmp);
					add_to_list((List **)&ucm, (List *)tucm);
				}

				tnl = find_nicklist_in_channellist(tmp, chan, 0);
				if (tnl && (nick_isop(tnl) || tucm->o_ed))
					isopped = 1;
				else if (tnl)
					isopped = 0;

				if (add && !isopped) 
				{
					tucm->o_ed = 1;
					tucm->deo_ed = 0;
				} 
				else if (!add && isopped) 
				{
					tucm->o_ed = 0;
					tucm->deo_ed = 1;
				} 
				break;
			case 'v':
				if (!(tmp = next_arg(rest, &rest)))
					break;
				if (!(tucm = (UserChanModes *)find_in_list((List **)&ucm, tmp, 0)))
				{
					tucm = (UserChanModes *) alloca(sizeof(UserChanModes));
					memset(tucm, 0, sizeof(UserChanModes));
					tucm->nick = LOCAL_COPY(tmp);
					add_to_list((List **)&ucm, (List *)tucm);
				}

				tnl = find_nicklist_in_channellist(tmp, chan, 0);
				if (tnl && (nick_isvoice(tnl) || tucm->v_ed))
					isvoiced = 1;
				else if (tnl)
					isvoiced = 0;

				if (add && !isvoiced) 
				{
					tucm->v_ed = 1;
					tucm->dev_ed = 0;
				} 
				else if (!add & isvoiced) 
				{
					tucm->v_ed = 0;
					tucm->dev_ed = 1;
				} 
				break;
			case 'b':
				if (!(tmp = next_arg(rest, &rest)))
					break;
				if (!(tucm = (UserChanModes *)find_in_list((List **)&ucm, tmp, 0)))
				{
					tucm = (UserChanModes *) alloca(sizeof(UserChanModes));
					memset(tucm, 0, sizeof(UserChanModes));
					tucm->nick = LOCAL_COPY(tmp);
					add_to_list((List **)&ucm, (List *)tucm);
				} 
				if (!tucm->b_ed)
				{
					for (tbl = chan->bans; tbl && !isbanned; tbl = tbl->next) 
					{
						if (!my_stricmp(tbl->ban, tmp))
							isbanned = 1;
						else
							isbanned = 0;
					}
				} 
				if (add && !isbanned) 
				{
					tucm->b_ed = 1;
					tucm->deb_ed = 0;
				} 
				else if (!add && isbanned) 
				{
					tucm->b_ed = 0;
					tucm->deb_ed = 1;
				}
	                	break;

			case 'l':
			case 'k':
				tmp = next_arg(rest, &rest);

				if (add) 
				{
					if (mod == 1) 
					{
						strcat(nmodes, "-");
						strextend(nmodes, *modes, 1);
					} 
					else 
					{
						strcat(nmodes, "+");
						strextend(nmodes, *modes, 1);
						mod = 1;
					}
				} 
				else 
				{
					if (!mod) 
					{
						strcat(nmodes, "+");
						strextend(nmodes, *modes, 1);
					} 
					else 
					{
						strcat(nmodes, "-");
						strextend(nmodes, *modes, 1);
						mod = 0;
					}
				}
				if (tmp && *tmp)
					strmopencat(nargs, sizeof(nargs)-1, space, tmp, NULL);
				break;

			case 'i':
			case 's':
			case 'n':
			case 't':
			case 'm':
			{
                		if (add) 
				{
					if (mod == 1)
						strextend(nmodes, *modes, 1);
					else 
					{
						strextend(nmodes, '+', 1);
						strextend(nmodes, *modes, 1);
			                        mod = 1;
					}
				} 
				else 
				{
					if (!mod)
						strextend(nmodes, *modes, 1);
					else 
					{
						strextend(nmodes, '-', 1);
						strextend(nmodes, *modes, 1);
	                		        mod = 0;
					}
				}
				break;
			}
		}
	}

   /* modes which can be done multiple times are added here */

	for (tucm = ucm; tucm && (strlen(nmodes) + 2) < sizeof nmodes; 
		tucm = tucm->next) 
	{
		if (tucm->o_ed) 
		{
			if (mod == 1) 
				strcat(nmodes, "o");
			else 
			{
				strcat(nmodes, "+o");
				mod = 1;
			}
			strmopencat(nargs, sizeof(nargs)-1, space, tucm->nick, NULL);
		} 
		else if (tucm->deo_ed) 
		{
			if (!mod) 
				strcat(nmodes, "o");
			else 
			{
				strcat(nmodes, "-o");
				mod = 0;
			}
			strmopencat(nargs, sizeof(nargs)-1, space, tucm->nick, NULL);
		}
		if (tucm->v_ed) 
		{
			if (mod == 1) 
				strcat(nmodes, "v");
			else 
			{
				strcat(nmodes, "+v");
				mod = 1;
			}
			strmopencat(nargs, sizeof(nargs)-1, space, tucm->nick, NULL);
		} 
		else if (tucm->dev_ed) 
		{
			if (!mod) 
				strcat(nmodes, "v");
			else 
			{
				strcat(nmodes, "-v");
				mod = 0;
			}
			strmopencat(nargs, sizeof(nargs)-1, space, tucm->nick, NULL);
		}
		if (tucm->b_ed) 
		{
			if (mod == 1) 
				strcat(nmodes, "b");
			else 
			{
				strcat(nmodes, "+b");
				mod = 1;
			}
			strmopencat(nargs, sizeof(nargs)-1, space, tucm->nick, NULL);
		} 
		else if (tucm->deb_ed) 
		{
			if (!mod) 
				strcat(nmodes, "b");
			else 
			{
				strcat(nmodes, "-b");
				mod = 0;
			}
			strmopencat(nargs, sizeof(nargs)-1, space, tucm->nick, NULL);
		}
	}

	if (strlen(nmodes) || strlen(nargs))
		return m_sprintf("%s%s", nmodes, nargs);
	return NULL;
}
#endif

int BX_got_ops(int add, ChannelList *channel)
{
int have_op = 0;
register NickList *tmp;
int in_join = 0;
	in_join = in_join_list(channel->channel, from_server);
	if (add && add != channel->have_op && !in_join)
	{
		have_op = channel->have_op = add;
#ifdef WANT_USERLIST
		for(tmp = next_nicklist(channel, NULL); tmp; tmp = next_nicklist(channel, tmp))
			check_auto(channel->channel,tmp, channel);
#endif
		if ((get_server_version(from_server) == Server2_8hybrid6))
			send_to_server("MODE %s e", channel->channel);
	}
	else if (!add && add != channel->have_op && !in_join)
 	{
		for(tmp = next_nicklist(channel, NULL); tmp; tmp = next_nicklist(channel, tmp))
			tmp->sent_reop = tmp->sent_deop = tmp->sent_voice = 0;
	}
	return have_op;
}

/*
 * apply_channel_modes
 *
 * This looks at a set of channel mode changes from a MODE command and
 * applies them to the Channel structure (old decifer_mode()).
 */
static void apply_channel_modes(char *from, char *mode_str, 
	ChannelList *channel)
{

	register char	*person;
	int	add = 0;
	int	splitter = 0;
	char	*rest;
	
	NickList *ThisNick = NULL;
	BanList *new;
	unsigned int	value = 0;
	int	its_me = 0;
		
	if (!(mode_str = next_arg(mode_str, &rest)))
		return;

	its_me = !my_stricmp(from, get_server_nickname(from_server)) ? 1 : 0;
	splitter = wild_match("*.*.*", from);

	for (; *mode_str; mode_str++)
	{
		switch (*mode_str)
		{
		case '+':
			add = 1;
			value = 0;
			break;
		case '-':
			add = 0;
			value = 0;
			break;
		case 'p':
			value = MODE_PRIVATE;
			break;
		case 'l':
			value = MODE_LIMIT;
			if (add)
			{
				char *limit  = next_arg(rest, &rest);

				if (limit)
				{
					channel->limit = atoi(limit);
					
					/* Setting +l 0 is the same as setting -l */
					if (channel->limit == 0)
						add = 0;
				}
				else
				{
					/* +l with no argument - broken server, just ignore
					 * it. */
					value = 0;
				}
			}
			else
			{
				channel->limit = 0;
			}
			break;
		case 'a':
			value = MODE_ANONYMOUS;
			break;
		case 't':
			value = MODE_TOPIC;
			break;
		case 'i':
			value = MODE_INVITE;
			break;
		case 'n':
			value = MODE_MSGS;
			break;
		case 'r':
			value = MODE_REGISTERED;
			break;
		case 's':
			value = MODE_SECRET;
			break;
		case 'm':
			value = MODE_MODERATED;
			break;
		case 'R':
			value = MODE_RESTRICTED;
			break;
		case 'z':
			value = MODE_Z;
			break;
		case 'h':
			if (!(person = next_arg(rest, &rest)))
				break;
			if (!my_stricmp(person, get_server_nickname(from_server)))
			{
				channel->hop = add;
			}			
			if ((ThisNick = find_nicklist_in_channellist(person, channel, 0)))
			{
/*				ThisNick->halfop=add;*/
				if (add)
					ThisNick->flags |= NICK_HALFOP;
				else
					ThisNick->flags &= ~NICK_HALFOP;
			}			
			ThisNick = find_nicklist_in_channellist(from, channel, 0);
			update_stats(add ? MODEHOPLIST: MODEDEHOPLIST, ThisNick, channel, splitter);
			break;
		case 'o':
		{
			if (!(person = next_arg(rest, &rest))) /* av 2.9 */
				person = get_server_nickname(from_server);
			if (!my_stricmp(person, get_server_nickname(from_server)))
			{
				got_ops(add, channel);
				channel->have_op = add;
				if (add)
					do_hook(CHANOP_LIST, "%s", channel->channel);
			}
			ThisNick = find_nicklist_in_channellist(from, channel, 0);
			update_stats(add ? MODEOPLIST: MODEDEOPLIST, ThisNick, channel, splitter);
			if ((ThisNick = find_nicklist_in_channellist(person, channel, 0)))
			{
				if (add)
					ThisNick->flags |= NICK_CHANOP;
				else
					ThisNick->flags &= ~NICK_CHANOP;
				if (add)
				{
					if (ThisNick->sent_reop)
				 		ThisNick->sent_reop--;
				} 	
				else if (ThisNick->sent_deop)
				        ThisNick->sent_deop--;
			}			

			if (!its_me && channel->have_op)
			{
				if (add && splitter)
					check_hack(person, channel, ThisNick, from);
#ifdef WANT_USERLIST
				else if (!add)
					check_prot(from, person, channel, NULL, ThisNick);
				else if (ThisNick)
					check_auto(channel->channel,ThisNick,channel);
#endif
			}
			break;
		}			
		case 'c':
			value = MODE_C;
			if (add)
			{
				char *pass;
				pass = next_arg(rest, &rest);
				if (pass)
					malloc_strcpy(&channel->chanpass, pass);
			}
			else
				new_free(&channel->chanpass);
			break;
		case 'k':
			value = MODE_KEY;
			if (add)
				malloc_strcpy(&channel->key, next_arg(rest, &rest));
			else
			{
				if (rest)
					next_arg(rest, &rest);

				new_free(&channel->key);
			}
			channel->i_mode = -1;
			break;	
		case 'v':
			if ((person = next_arg(rest, &rest)))
			{
				if ((ThisNick = find_nicklist_in_channellist(person, channel, 0)))
				{
					if (add)
						ThisNick->flags |= NICK_VOICE;
					else
						ThisNick->flags &= ~NICK_VOICE;
					if (add)
					{
						if (ThisNick->sent_voice)
					 		ThisNick->sent_voice--;
					} 	
					else
						ThisNick->sent_voice = 0;
				}
				if (!my_stricmp(person, get_server_nickname(from_server)))
					channel->voice = add;
			}
			break;
		case 'b':
			if (!(person = next_arg(rest, &rest)))
				break;

			ThisNick = find_nicklist_in_channellist(from, channel, 0);
			update_stats(add?MODEBANLIST:MODEUNBANLIST, ThisNick, channel, splitter);
			if (add)
			{
				ThisNick = find_nicklist_in_channellist(person, channel, 0);
				if (!(new = (BanList *)find_in_list((List **)&channel->bans, person, 0)) || my_stricmp(person, new->ban))
				{
					new = (BanList *) new_malloc(sizeof(BanList));
					malloc_strcpy(&new->ban, person);
					add_to_list((List **)&channel->bans, (List *)new);
				} 
				new->sent_unban = 0;
				if (!new->setby)
					malloc_strcpy(&new->setby, from?from:get_server_name(from_server));
				new->time = now;
#ifdef WANT_USERLIST
				if (!its_me && channel->have_op)
					check_prot(from, person, channel, new, ThisNick);
#endif
			} 
			else
			{
				if ((new = (BanList *)remove_from_list((List **)&channel->bans, person)))
				{
					new_free(&new->setby);
					new_free(&new->ban);
					new_free((char **)&new);
				}
			}
			break;
		case 'e':
			if (!(person = next_arg(rest, &rest)))
				break;

			ThisNick = find_nicklist_in_channellist(from, channel, 0);
			update_stats(add?MODEEBANLIST:MODEUNEBANLIST, ThisNick, channel, splitter);
			if (add)
			{
				ThisNick = find_nicklist_in_channellist(person, channel, 0);
				if (!(new = (BanList *)find_in_list((List **)&channel->exemptbans, person, 0)) || my_stricmp(person, new->ban))
				{
					new = (BanList *) new_malloc(sizeof(BanList));
					malloc_strcpy(&new->ban, person);
					add_to_list((List **)&channel->exemptbans, (List *)new);
				} 
				new->sent_unban = 0;
				if (!new->setby)
					malloc_strcpy(&new->setby, from?from:get_server_name(from_server));
				new->time = now;
			} 
			else
			{
				if ((new = (BanList *)remove_from_list((List **)&channel->exemptbans, person)))
				{
					new_free(&new->setby);
					new_free(&new->ban);
					new_free((char **)&new);
				}
			}
			break;
		}
		if (add)
			channel->mode |= value;
		else
			channel->mode &= ~value;
	}
#ifdef WANT_USERLIST
	check_shit(channel);
#endif
	flush_mode_all(channel);
}

/*
 * get_channel_mode: returns the current mode string for the given channel
 */
char	*BX_get_channel_mode(char *channel, int server)
{
	ChannelList *tmp;
	
	if ((tmp = lookup_channel(channel, server, CHAN_NOUNLINK)))
		return recreate_mode(tmp);
	return empty_string;
}

char	*BX_get_channel_bans(char *channel, int server, int type_mode)
{
	ChannelList *tmp;
	
	if ((tmp = lookup_channel(channel, server, CHAN_NOUNLINK)))
	{
		BanList *b;
		char *temp = NULL;
		switch(type_mode)
		{
			case 1: /* ban */
				for (b = tmp->bans; b; b = b->next)
					m_s3cat(&temp, space, b->ban);
				break;
			case 2: /* ban who time */
				for (b = tmp->bans; b; b = b->next)
				{
					m_s3cat(&temp, space, b->ban);
					m_s3cat(&temp, space, b->setby);
					m_s3cat(&temp, space, ltoa(b->time));
				}
				break;
			case 3: /* exemptions [ts4] */
				for (b = tmp->exemptbans; b; b = b->next)
					m_s3cat(&temp, space, b->ban);
				break;
		}
		return temp;
	}
	return m_strdup(empty_string);
}

/*
 * update_channel_mode: This will modify the mode for the given channel
 * according the the new mode given.  
 */
void update_channel_mode(char *from, char *channel, int server, char *mode, ChannelList *tmp)
{
	if (!tmp && channel)
	{
		tmp = lookup_channel(channel, server, CHAN_NOUNLINK);
	}

	if (tmp)
	{
		apply_channel_modes(from, mode, tmp);
	}
}

/*
 * is_channel_mode: returns the logical AND of the given mode with the
 * channels mode.  Useful for testing a channels mode 
 */
int is_channel_mode(char *channel, int mode, int server_index)
{
	ChannelList *tmp;
	 
	if ((tmp = lookup_channel(channel, server_index, CHAN_NOUNLINK)))
		return (tmp->mode & mode);
	return 0;
}

void BX_clear_bans(ChannelList *channel)
{
	BanList *bans, 
		*next;
	
	if (!channel)
		return;
	for (bans = channel->bans; bans; bans = next)
	{
		next = bans->next;
		new_free(&bans->setby);
		new_free(&bans->ban);
		new_free((char **)&bans);
	}
	channel->bans = NULL;
	for (bans = channel->exemptbans; bans; bans = next)
	{
		next = bans->next;
		new_free(&bans->setby);
		new_free(&bans->ban);
		new_free((char **)&bans);
	}
	channel->exemptbans = NULL;
}

/*
 * remove_channel: removes the named channel from the
 * server_list[server].chan_list.  If the channel is not on the
 * server_list[server].chan_list, nothing happens.  If the channel was
 * the current channel, this will select the top of the
 * server_list[server].chan_list to be the current_channel, or 0 if the
 * list is empty. 
 */

void BX_remove_channel (char *channel, int server)
{
	ChannelList *tmp;
	int old_from_server = from_server;
	
	if (channel)
	{
		if (*channel == '*')
			return;
		from_server = server;
		from_server = old_from_server;
		if ((tmp = lookup_channel(channel, server, CHAN_UNLINK)))
		{
			clear_bans(tmp);
			clear_channel(tmp);
			add_to_whowas_chan_buffer(tmp);
		}
		if (is_current_channel(channel, server, 1))
			switch_channels(0, NULL);
	}
	else
	{
		ChannelList *next;

		for (tmp = get_server_channels(server); tmp; tmp = next)
		{
			next = tmp->next;
			clear_channel(tmp);
			clear_bans(tmp);
			add_to_whowas_chan_buffer(tmp);
		}
		set_server_channels(server, NULL);
	}
	xterm_settitle();
	update_all_windows();
}

/*
 * remove_from_channel: removes the given nickname from the given channel. If
 * the nickname is not on the channel or the channel doesn't exist, nothing
 * happens. 
 */
void BX_remove_from_channel(char *channel, char *nick, int server, int netsplit, char *reason)
{
	ChannelList *chan;
	NickList *tmp = NULL;
	extern char *last_split_server;
	extern char *last_split_from;
	char buf[BIG_BUFFER_SIZE+1];
	char *server1 = NULL, *server2 = NULL;
	
	if (netsplit && reason && *reason)
	{
		char *p = NULL;
		strncpy(buf, reason, sizeof(buf)-1);
		if ((p = strchr(buf, ' ')))
		{
			*p++ = '\0';
			server2 = buf;
			server1 = p;
		}		
		if (server1 && !check_split_server(server1))
		{
			add_split_server(server1, server2, 0);
			set_display_target(channel, LOG_CRAP);
			malloc_strcpy(&last_split_server, server1);
			malloc_strcpy(&last_split_from, server2);
			if (do_hook(LLOOK_SPLIT_LIST, "%s %s", server2, server1))
			{
				char *s1, *s = NULL, *t = NULL;
				if ((s = convert_to_keystr("WHOLEFT")))
					t = LOCAL_COPY(s);
				s = NULL;
				if ((s1 = convert_to_keystr("CHANGE_TO_SPLIT")))
					s = s1;
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NETSPLIT_FSET), "%s %s %s", update_clock(GET_TIME), server1, server2));
				bitchsay("%s \002%s\002 to see who left \002%s\002 to change to [%s]", t ? "Press" : "", t ? t : "/wholeft", s ? s : "/server", server1);
			}
			reset_display_target();
		}
	}

	if (channel)
	{
		if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
		{
			if ((tmp = find_nicklist_in_channellist(nick, chan, REMOVE_FROM_LIST)))
			{
				do_hook(NETSPLIT_LIST, "%d", netsplit);
				add_to_whowas_buffer(tmp, channel, server1, server2);
				if (netsplit)
				{
					if (server1 && strchr(server1, '*') && tmp->server)
						server1 = tmp->server;
					add_to_whosplitin_buffer(tmp, channel, server1, server2);
				}
			}
		}
	}
	else
	{
		for (chan = get_server_channels(server); chan; chan = chan->next)
		{
			if ((tmp = find_nicklist_in_channellist(nick, chan, REMOVE_FROM_LIST)))
			{
				add_to_whowas_buffer(tmp, chan->channel, server1, server2);
				if (netsplit)
				{
					if (server1 && strchr(server1, '*') && tmp->server)
						server1 = tmp->server;
					add_to_whosplitin_buffer(tmp, chan->channel, server1, server2);
				}
			}
		}
	}
}

void handle_nickflood(char *old_nick, char *new_nick, register NickList *nick, register ChannelList *chan, int flood_time)
{
time_t floodtime = now - nick->bantime;
	if (isme(new_nick))
		return;
	if ((!nick->bancount) || (nick->bancount && (floodtime > 3)))
	{
		if (!nick->kickcount++ && ((nick->userlist && (nick->userlist->flags & ADD_FLOOD)) || !nick->userlist))
			send_to_server("KICK %s %s :\002Niq flood (%d nicks in %dsecs of %dsecs)\002", chan->channel, new_nick, get_cset_int_var(chan->csets, KICK_ON_NICKFLOOD_CSET), flood_time, get_cset_int_var(chan->csets, NICKFLOOD_TIME_CSET));
	}
}

/*
 * rename_nick: in response to a changed nickname, this looks up the given
 * nickname on all you channels and changes it the new_nick 
 */
void BX_rename_nick(char *old_nick, char *new_nick, int server)
{
	register ChannelList *chan;
	register NickList *tmp;
	int t = 0;
		
	
	for (chan = get_server_channels(server); chan; chan = chan->next)
	{
		if ((chan->server == server))
		{
			if ((tmp = find_nicklist_in_channellist(old_nick, chan, REMOVE_FROM_LIST)))
			{
				tmp->stat_nicks++;
				if (chan->have_op && !isme(new_nick))
				{
					if (is_other_flood(chan, tmp, NICK_FLOOD, &t))
						handle_nickflood(old_nick, new_nick, tmp, chan, t); 
					else if (get_cset_int_var(chan->csets, LAMELIST_CSET) && lame_list)
					{
						if (find_in_list((List **)&lame_list, new_nick, 0))
						{
							send_to_server("MODE %s -o+b %s %s!*@*", chan->channel, new_nick, new_nick);
							send_to_server("KICK %s %s :\002Lame Nick detected\002", chan->channel, new_nick);
							tmp->bancount++;
							tmp->bantime = now;
						}
					}
				}
				check_orig_nick(new_nick);
				xterm_settitle();
				malloc_strcpy(&tmp->nick, new_nick);
				add_nicklist_to_channellist(tmp, chan);
			}
		}
	}
}


/*
 * is_on_channel: returns true if the given nickname is in the given channel,
 * false otherwise.  Also returns false if the given channel is not on the
 * channel list. 
 */
int BX_is_on_channel(char *channel, int server, char *nick)
{
	ChannelList *chan;
	
	if (nick && (chan = lookup_channel(channel, server, CHAN_NOUNLINK)) && chan->connected)
		if (find_nicklist_in_channellist(nick, chan, 0))
			return 1;
	return 0;
}

int BX_is_chanop(char *channel, char *nick)
{
	ChannelList *chan;
	NickList *Nick;
	
	if (nick && (chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
	{
		if ((Nick = find_nicklist_in_channellist(nick, chan, 0)))
			if (nick_isop(Nick))
				return 1;
	}
	return 0;
}

int BX_is_halfop(char *channel, char *nick)
{
	ChannelList *chan;
	NickList *Nick;
	
	if (nick && (chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
	{
		if ((Nick = find_nicklist_in_channellist(nick, chan, 0)))
			if (nick_ishalfop(Nick))
				return 1;
	}
	return 0;
}

char *is_chanoper(char *channel, char *nick)
{
	ChannelList *chan;
	NickList *Nick;
	char *ret = NULL;
	if (nick && (chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
	{
		char *n;
		while ((n = next_in_comma_list(nick, &nick)))
		{
			if (!n || !*n) break;
			if ((Nick = find_nicklist_in_channellist(n, chan, 0)))
				m_s3cat(&ret, space, nick_isop(Nick) ?one:zero);
			else
				m_s3cat(&ret, space, zero);
		}
	}
	return ret;
}

static	void show_channel(ChannelList *chan)
{
	NickList *tmp;
	char	*ptr, *s;
	char	buffer[BIG_BUFFER_SIZE * 10 + 1];
	size_t	nick_len = BIG_BUFFER_SIZE * 10;
	int len = 0;
		
	*buffer = 0;
	s = recreate_mode(chan);
	ptr = buffer;
		
	for (tmp = next_nicklist(chan, NULL); tmp; tmp = next_nicklist(chan, tmp))
	{
		strlcpy(ptr, tmp->nick, nick_len);
		if (tmp->host)
		{
			strlcat(ptr, "!", nick_len);
			strlcat(ptr, tmp->host, nick_len);
		}
		strlcat(ptr, space, nick_len);
		len = strlen(ptr);
		nick_len -= len;
		ptr += len;
		if (nick_len <= 0)
			break;
	}
	say("\t%s %c%s (%s) (Win: %d): %s", 
		chan->channel, s ? '+':' ', 
		s ? s : "<none>", 
		get_server_name(chan->server), 
		chan->window ? chan->window->refnum : -1,
		buffer);
}

/* list_channels: displays your current channel and your channel list */
void list_channels(void)
{
	ChannelList *tmp;
	int	server,
		no = 1;
	
	if (get_server_channels(from_server))
	{
		if (get_current_channel_by_refnum(0))
			say("Current channel %s", get_current_channel_by_refnum(0));
		else
			say("No current channel for this window");
		if ((tmp = get_server_channels(get_window_server(0))))
		{
			for (; tmp; tmp = tmp->next)
				show_channel(tmp);
			no = 0;
		}
		if (connected_to_server != 1)
		{
			for (server = 0; server < server_list_size(); server++)
			{
				if (server == get_window_server(0))
					continue;
				say("Other servers:");
				for (tmp = get_server_channels(server); tmp; tmp = tmp->next)
					show_channel(tmp);
				no = 0;
			}
		}
	}
	else
		say("You are not on any channels");
}

void switch_channels(char key, char *ptr)
{
	ChannelList *	tmp = NULL;
	char *nc = get_current_channel_by_refnum(0);
	
	if (from_server == -1 || !get_server_channels(from_server))
		return;

	if (nc)
		if ((tmp = lookup_channel(nc, from_server, CHAN_NOUNLINK)))
			tmp = tmp->next;
	
	if (!tmp)
		tmp = get_server_channels(from_server);
	
	if (current_window->name && !strcmp(current_window->name, "oper_view"))
		return;
	for (; tmp; tmp = tmp->next)
	{
		if (tmp->server != from_server)
			continue;
		if (is_current_channel(tmp->channel, from_server, 0))
			continue;
		if (current_window == tmp->window)
		{
			set_current_channel_by_refnum(0, tmp->channel);
			update_all_windows();
			do_hook(CHANNEL_SWITCH_LIST, "%s", tmp->channel);
			set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
			update_input(UPDATE_ALL);
#ifdef WANT_TCL
			Tcl_SetVar(tcl_interp, "curchan", tmp->channel, TCL_GLOBAL_ONLY);
#endif
			xterm_settitle();
			return;
		}
	}

	for (tmp = get_server_channels(from_server); tmp; tmp = tmp->next)
	{
		if (tmp->server != from_server)
			continue;
		if (is_current_channel(tmp->channel, from_server, 0))
			continue;
		if (current_window == tmp->window)
		{
			set_current_channel_by_refnum(0, tmp->channel);
			update_all_windows();
			do_hook(CHANNEL_SWITCH_LIST, "%s", tmp->channel);
			set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
			update_input(UPDATE_ALL);
#ifdef WANT_TCL
			Tcl_SetVar(tcl_interp, "curchan", tmp->channel, TCL_GLOBAL_ONLY);
#endif
			xterm_settitle();
			return;
		}
	}
}

void change_server_channels(int old, int new)
{
	ChannelList *tmp;

	
	if (new == old)
		return;
	if (new > -1 && (tmp = get_server_channels(new)))
		tmp->server = new;
	if (new > -1 && old > -1)
	{
		tmp = get_server_channels(old);
		set_server_channels(new, tmp);
		for (; tmp; tmp = tmp->next)
			tmp->server = new;
		set_server_channels(old, NULL);
	}
}

void clear_channel_list(int server)
{
	ChannelList *tmp = NULL,
		*next;
	Window		*ptr = NULL;
	
	while ((traverse_all_windows(&ptr)))
		if (ptr->server == server && ptr->current_channel)
			new_free(&ptr->current_channel);
	
	for (tmp = get_server_channels(server); tmp; tmp = next)
	{
		next = tmp->next;
		clear_channel(tmp);
		add_to_whowas_chan_buffer(tmp);
	}
	set_server_channels(server, NULL);
	clear_mode_list(server);
	return;
}

/*
 * reconnect_all_channels: used after you get disconnected from a server, 
 * clear each channel nickname list and re-JOINs each channel in the 
 * channel_list ..  
 */
void reconnect_all_channels(int server)
{
	ChannelList *tmp;
	char	*mode;
	char	*channel = NULL;
	char	*keys	= NULL;

	for (tmp = get_server_channels(server); tmp; tmp = tmp->next)
	{
		if ((mode = recreate_mode(tmp)))
			add_to_mode_list(tmp->channel, server, mode);
		add_to_join_list(tmp->channel, server, tmp->refnum);
		m_s3cat(&channel, ",", tmp->channel);
		m_s3cat(&keys, ",", tmp->key? tmp->key:"-");
		clear_channel(tmp);
		clear_bans(tmp);
		tmp->server = server;
	}
	if (channel)
		my_send_to_server(server, "JOIN %s %s", channel, keys ? keys : empty_string);
	clear_channel_list(from_server);
	new_free(&channel);
	new_free(&keys);
	reset_display_target();
}

char *what_channel(char *nick, int server)
{
	ChannelList *tmp;

	
	if (current_window->current_channel && is_on_channel(current_window->current_channel, current_window->server, nick))
		return current_window->current_channel;

	for (tmp = get_server_channels(from_server); tmp; tmp = tmp->next)
	{
		if (find_nicklist_in_channellist(nick, tmp, 0))
			return tmp->channel;
	}
	return NULL;
}

ChannelList *walk_channels(char *nick, int init, int server)
{
	static	ChannelList *tmp = NULL;

	if (init)
		tmp = get_server_channels(server);
	else if (tmp)
		tmp = tmp->next;
	
	for (;tmp ; tmp = tmp->next)
	{
		if (find_nicklist_in_channellist(nick, tmp, 0))
			return tmp;
	}
	return NULL;
}

int BX_get_channel_oper(char *channel, int server)
{
	ChannelList *chan;

	
	if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
		return chan->have_op;
	return 1;
}

int BX_get_channel_halfop(char *channel, int server)
{
	ChannelList *chan;

	
	if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
		return chan->hop;
	return 1;
}

char *BX_fetch_userhost (int server, char *nick)
{
	ChannelList *tmp = NULL;
	NickList *user = NULL;

	for (tmp = get_server_channels(server); tmp; tmp = tmp->next)
	{
		if (((user = (NickList *)find_nicklist_in_channellist(nick, tmp, 0))))
			return user->host;
	}
	return NULL;
}

int BX_get_channel_voice(char *channel, int server)
{
	ChannelList *chan;

	
	if ((chan = lookup_channel(channel, server, CHAN_NOUNLINK)))
		return chan->voice;
	return 1;
}

extern	void set_channel_window(Window *window, char *channel, int server)
{
	ChannelList	*tmp;

	
	if (!channel || server < 0)
		return;
	for (tmp = get_server_channels(server); tmp; tmp = tmp->next)
	{
		if (!my_stricmp(channel, tmp->channel) && tmp->server == server)
		{
			tmp->window = window;
			return;
		}
	}
}

extern	char	* BX_create_channel_list(Window *window)
{
	ChannelList	*tmp;
	char	buffer[BIG_BUFFER_SIZE + 1];
	
	
	*buffer = 0;
	for (tmp = get_server_channels(window->server); tmp; tmp = tmp->next)
	{
		if (tmp->server == from_server)
		{
			strmcat(buffer, tmp->channel, BIG_BUFFER_SIZE);
			strmcat(buffer, space, BIG_BUFFER_SIZE);
		}
	}
	return m_strdup(buffer);
}

extern void channel_server_delete(int server)
{
	ChannelList     *tmp;
	int	i;
	
	for (i = server + 1; i < server_list_size(); i++)
		for (tmp = get_server_channels(i); tmp; tmp = tmp->next)
			if (tmp->server >= server)
				tmp->server--;
}


/* remove_from_join_list: called when mode and names have been received or
   when access has been denied */
void remove_from_join_list(char *chan, int server)
{
	struct	joinlist	*tmp,
				*next,
				*prev = NULL;

	for (tmp = join_list; tmp; tmp = tmp->next)
	{
		next = tmp->next;
		if (!my_stricmp(chan, tmp->chan) && tmp->server == server)
		{
			if (tmp == join_list)
				join_list = next;
			else
				prev->next = next;
			new_free(&tmp->chan);
			new_free((char **)&tmp);
			return;
		}
		else
			prev = tmp;
	}
}

/* get_win_from_join_list: returns the window refnum associated to the channel
   we're trying to join */
int get_win_from_join_list(char *chan, int server)
{
	struct	joinlist	*tmp;
	int			found = 0;

	for (tmp = join_list; tmp; tmp = tmp->next)
	{
		if (!my_stricmp(tmp->chan, chan) && tmp->server == server)
			found = tmp->winref;
	}
	return found;
}

/* get_chan_from_join_list: returns a pointer on the name of the first channel
   entered into join_list for the specified server */
char	*get_chan_from_join_list(int server)
{
	struct	joinlist	*tmp;
	char			*found = NULL;

	for (tmp = join_list; tmp; tmp = tmp->next)
	{
		if (tmp->server == server)
			found = tmp->chan;
	}
	return found;
}

/* in_join_list: returns 1 if we're attempting to join the channel */
int in_join_list(char *chan, int server)
{
	struct	joinlist	*tmp;

	for (tmp = join_list; tmp; tmp = tmp->next)
	{
		if (!tmp->chan) continue;
		if (!my_stricmp(tmp->chan, chan) && tmp->server == server)
			return 1;
	}
	return 0;
}

void show_channel_sync(struct joinlist *tmp, char *chan)
{
struct timeval tv;
	get_time(&tv);
	set_display_target(chan, LOG_CRAP);
	if (do_hook(CHANNEL_SYNCH_LIST, "%s %1.3f", chan, BX_time_diff(tmp->tv,tv)))
		bitchsay("Join to %s was synched in %1.3f secs!!", chan, BX_time_diff(tmp->tv,tv));
#ifdef WANT_USERLIST
	delay_check_auto(chan);
#endif
	update_all_status(current_window, NULL, 0);
	reset_display_target();
	xterm_settitle();
#ifdef GUI
	gui_update_nicklist(chan);
#endif
}

/* got_info: increments the gotinfo field when receiving names and mode
   and removes the channel if both have been received */
int got_info(char *chan, int server, int type)
{
	struct	joinlist	*tmp;

	for (tmp = join_list; tmp; tmp = tmp->next)
		if (!my_stricmp(tmp->chan, chan) && tmp->server == server)
		{
			int what_info = (GOTNAMES | GOTMODE | GOTBANS | GOTWHO);
			int ver;

			ver = get_server_version(server);
			if ((ver == Server2_8ts4) || (ver == Server2_10))
				what_info |= GOTEXEMPT;

			if ((tmp->gotinfo |= type) == what_info)
			{
				if (prepare_command(&tmp->server, chan, PC_SILENT))
					show_channel_sync(tmp, chan);
				remove_from_join_list(chan, tmp->server);
				return 1;
			}
			return 0;
		}
	return -1;
}

/* add_to_join_list: registers a channel we're trying to join */
void add_to_join_list(char *chan, int server, int winref)
{
	struct	joinlist	*tmp;

	for (tmp = join_list; tmp; tmp = tmp->next)
	{
		if (!my_stricmp(chan, tmp->chan) && (tmp->server == server) && (tmp->winref == winref))
		{
			tmp->winref = winref;
			return;
		}
	}
	tmp = (struct joinlist *) new_malloc(sizeof(struct joinlist));
	tmp->chan = m_strdup(chan);
	tmp->server = server;
	tmp->gotinfo = 0;
	tmp->winref = winref;
	tmp->next = join_list;
	get_time(&tmp->tv);
	join_list = tmp;
}

static	void add_to_mode_list(char *channel, int server, char *mode)
{
	struct modelist	*mptr;

	if (!channel || !*channel || !mode || !*mode)
		return;
	
	mptr = (struct modelist *) new_malloc(sizeof(struct modelist));
	mptr->chan = m_strdup(channel);
	mptr->server = server;
	mptr->mode =m_strdup(mode);
	mptr->next = mode_list;
	mode_list = mptr;
}

static	void check_mode_list_join(char *channel, int server)
{
	struct modelist *mptr;

	
	if (!channel)
		return;
	for (mptr = mode_list; mptr; mptr = mptr->next)
	{
		if (!my_stricmp(mptr->chan, channel) && mptr->server == server)
		{
			my_send_to_server(mptr->server, "MODE %s %s", mptr->chan, mptr->mode);
			remove_from_mode_list(channel, server);
			return;
		}
	}
}

extern	void remove_from_mode_list(char *channel, int server)
{
	struct modelist	*curr, *next, 	*prev = NULL;

	
	for (next = mode_list; next; )
	{
		curr = next;
		next = curr->next;
		if (!my_stricmp(curr->chan, channel) && curr->server == server)
		{
			if (curr == mode_list)
				mode_list = curr->next;
			else
				prev->next = curr->next;
			prev = curr;
			new_free(&curr->chan);
			new_free(&curr->mode);
			new_free((char **)&curr);
		}
		else
			prev = curr;
	}
}

extern	void clear_mode_list(int server)
{
	struct modelist	*curr, *next, *prev = NULL;

	
	for (next = mode_list; next; )
	{
		curr = next;
		next = curr->next;
		if (curr == mode_list)
			mode_list = curr->next;
		else
			prev->next = curr->next;
		prev = curr;
		new_free(&curr->chan);
		new_free(&curr->mode);
		new_free((char **)&curr);
	}
}

void BX_flush_channel_stats(void)
{
ChannelList *chan;
	for (chan = get_server_channels(from_server); chan; chan = chan->next)
	{
		chan->stats_ops = 0;
		chan->stats_dops = 0;
		chan->stats_bans = 0;
		chan->stats_unbans = 0;
		chan->stats_sops = 0;
		chan->stats_sdops = 0;
		chan->stats_sbans = 0;
		chan->stats_sunbans = 0;
		chan->stats_topics = 0;
		chan->stats_kicks = 0;
		chan->stats_pubs = 0;
		chan->stats_parts = 0;
		chan->stats_signoffs = 0;
		chan->stats_joins = 0;
		chan->maxnicks = 0;
		chan->maxnickstime = 0;
	}
}
/*
 * For any given window, re-assign all of the channels that are connected
 * to that window.
 */
void	unset_window_current_channel (Window *window)
{
	ChannelList *tmp;
	if (window->server <= -1)
		return;
	for (tmp = get_server_channels(window->server); tmp; tmp = tmp->next)
	{
		if (tmp->window == window && window->current_channel && !my_stricmp(tmp->channel, window->current_channel))
		{
			new_free(&window->current_channel);
			tmp->window = NULL;
		}
	}
}

/*
 * For any given window, re-assign all of the channels that are connected
 * to that window.
 */
void	move_window_channels (Window *window)
{
	ChannelList *tmp;
	if (window->server <= -1)
		return;
	for (tmp = get_server_channels(window->server); tmp; tmp = tmp->next)
	{
		if (tmp->window == window)
		{
			Window *w = NULL;
			tmp->window = NULL;
			while (traverse_all_windows(&w))
			{
				if (w->server == window->server && w != window)
				{
					tmp->window = w;
					break;
				}
			}
		}
	}
}

void check_channel_limits()
{
int i;
ChannelList *chan;
	for (i = 0; i < server_list_size(); i++)
	{
		for (chan = get_server_channels(i); chan; chan = chan->next)
		{
			if (!chan->have_op || !chan->csets || !chan->csets->set_auto_limit)
				continue;
			if (chan->totalnicks + chan->csets->set_auto_limit != chan->limit)
				my_send_to_server(i, "MODE %s +l %d", chan->channel, chan->totalnicks + chan->csets->set_auto_limit);
		}
	}
}
