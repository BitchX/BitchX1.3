/* 
 *  Copyright Colten Edwards (c) 1996
 */
#include "irc.h"
#include "struct.h"

#include "server.h"
#include "dcc.h"
#include "commands.h"
#include "crypt.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "window.h"
#include "screen.h"
#include "who.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "alias.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "list.h"
#include "timer.h"
#include "userlist.h"
#include "misc.h"
#include "gui.h"
#include "flood.h"
#include "parse.h"
#include "whowas.h"
#include "hash2.h"
#include "cset.h"

#include <stdio.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#ifndef __OPENNT
#include <sys/resource.h>
#endif
#include <unistd.h>

#if defined(sparc) && defined(sun4c)
#include <sys/rusage.h>
#endif

#ifdef GUI
extern int guiipc[2];
extern int newscrollerpos, lastscrollerpos, lastscrollerwindow;
#endif


extern int user_count;
extern int shit_count;
extern int bot_count;
extern int in_server_ping;

int serv_action = 0;
int first_time = 0;

LastMsg last_msg[MAX_LAST_MSG+1] = { { NULL } };
LastMsg last_dcc[MAX_LAST_MSG+1] = { { NULL } };
LastMsg last_notice[MAX_LAST_MSG+1] = { { NULL } };
LastMsg last_servermsg[MAX_LAST_MSG+1] = { { NULL } };
LastMsg last_sent_msg[MAX_LAST_MSG+1] = {{ NULL }};
LastMsg last_sent_notice[MAX_LAST_MSG+1] = {{ NULL }};
LastMsg last_sent_topic[2] = {{ NULL }};
LastMsg last_sent_wall[2] = {{ NULL }};
LastMsg last_topic[2] =  {{ NULL }};
LastMsg last_wall[MAX_LAST_MSG+1] =  {{ NULL }};
LastMsg last_invite_channel[2] = {{ NULL }};
LastMsg last_ctcp[2] = {{ NULL }};
LastMsg last_sent_ctcp[2] = {{ NULL }};
LastMsg last_sent_dcc[MAX_LAST_MSG+1] = {{ NULL }};

extern int in_cparse;

#define SPLIT 1


ChannelList *idlechan_list = NULL;

extern NickTab *tabkey_array, *autoreply_array;


extern Ignore *ignored_nicks;

#ifdef REVERSE_WHITE_BLACK
char *color_str[] = {
"[0m","[0;34m","[0;32m","[0;36m","[0;31m","[0;35m","[0;33m","[0;30m",
"[1;37m","[1;34m","[1;32m","[1;36m","[1;31m","[1;35m","[1;33m","[1;30m", "[0m",
"[0;47m", "[0;41m", "[0;42m","[0;43m", "[0;44m","[0;45m","[0;46m", "[0;40m",
"[1;47m", "[1;41m", "[1;42m","[1;43m", "[1;44m","[1;45m","[1;46m", "[1;40m",
"[7m", "[1m", "[5m", "[4m"};

#else

char *color_str[] = {
"[0;30m","[0;34m","[0;32m","[0;36m","[0;31m","[0;35m","[0;33m","[0m",
"[1;30m","[1;34m","[1;32m","[1;36m","[1;31m","[1;35m","[1;33m","[1;37m", "[0m",
"[0;40m", "[0;41m", "[0;42m","[0;43m", "[0;44m","[0;45m","[0;46m", "[0;47m",
"[1;40m", "[1;41m", "[1;42m","[1;43m", "[1;44m","[1;45m","[1;46m", "[1;47m",
"[7m", "[1m", "[5m", "[4m"};
#endif

irc_server /**tmplink = NULL, *server_last = NULL, *split_link = NULL,*/ *map = NULL;

#define getrandom(min, max) ((rand() % (int)(((max)+1) - (min))) + (min))

char *awaymsg = NULL;

char *convert_time (time_t ltime)
{
	time_t  days = 0,hours = 0,minutes = 0,seconds = 0;
	static char buffer[40];

	
	*buffer = '\0';
	seconds = ltime % 60;
	ltime = (ltime - seconds) / 60;
	minutes = ltime%60;
	ltime = (ltime - minutes) / 60;
	hours = ltime % 24;
	days = (ltime - hours) / 24;
	sprintf(buffer, "%2ldd %2ldh %2ldm %2lds", days, hours, minutes, seconds);
	return(*buffer ? buffer : empty_string);
}

BUILT_IN_COMMAND(do_uptime)
{
	
#ifdef ONLY_STD_CHARS
	put_it("%s",convert_output_format("%G--[ %WBitchX%g-%wClient%g-%RStatistics %G]------------------------------------------",NULL));
	put_it("%s",convert_output_format("%G| %CClient Version: %W$0 $1","%s %s", irc_version, internal_version));
	put_it("%s",convert_output_format("%G| %CClient Running Since %W$0-","%s",my_ctime(start_time)));
	put_it("%s",convert_output_format("%G| %CClient Uptime: %W$0-","%s",convert_time(now-start_time)));
	put_it("%s",convert_output_format("%G| %CCurrent UserName: %W$0-","%s", username));
	put_it("%s",convert_output_format("%G| %CCurrent RealName: %W$0-","%s", realname));
	put_it("%s",convert_output_format("%G| %CLast Recv Message: %W$0-","%s",last_msg[0].last_msg?last_msg[0].last_msg:"None"));
	put_it("%s",convert_output_format("%G| %CLast Recv Notice: %W$0-","%s",last_notice[0].last_msg?last_notice[0].last_msg:"None"));
	put_it("%s",convert_output_format("%G| %CLast Sent Msg: %W$0-","%s",last_sent_msg[0].last_msg?last_sent_msg[0].last_msg:"None"));
	put_it("%s",convert_output_format("%G| %CLast Sent Notice: %W$0-","%s",last_sent_notice[0].last_msg?last_sent_notice[0].last_msg:"None"));
	put_it("%s",convert_output_format("%G| %CLast Channel invited to: %R$0-","%s",invite_channel?invite_channel:"None"));
	put_it("%s",convert_output_format("%G| %cTotal Users on Userlist: %K[%R$0%K]","%d",user_count));
	put_it("%s",convert_output_format("%G| %cTotal Users on Shitlist: %K[%R$0%K]","%d",shit_count));

#else
	put_it("%s",convert_output_format("%GÚÄ[ %WBitchX%gÄ%wClient%gÄ%RStatistics %G]ÄÄÄÄ---%gÄ--ÄÄ%K-%gÄÄÄÄÄ--%GÄ--ÄÄ%K-%gÄÄÄÄÄÄÄ--- %K--%g  -",NULL));
	put_it("%s",convert_output_format("%G| %CClient Version: %W$0 $1","%s %s", irc_version, internal_version));
	put_it("%s",convert_output_format("%G³ %CClient Running Since %W$0-","%s",my_ctime(start_time)));
	put_it("%s",convert_output_format("%G| %CClient Uptime: %W$0-","%s",convert_time(now-start_time)));
	put_it("%s",convert_output_format("%G³ %CCurrent UserName: %W$0-","%s", username));
	put_it("%s",convert_output_format("%G: %CCurrent RealName: %W$0-","%s", realname));
	put_it("%s",convert_output_format("%G. %CLast Recv Message: %W$0-","%s",last_msg[0].last_msg?last_msg[0].last_msg:"None"));
	put_it("%s",convert_output_format("%G: %CLast Recv Notice: %W$0-","%s",last_notice[0].last_msg?last_notice[0].last_msg:"None"));
	put_it("%s",convert_output_format("%G. %CLast Sent Msg: %W$0-","%s",last_sent_msg[0].last_msg?last_sent_msg[0].last_msg:"None"));
	put_it("%s",convert_output_format("%G: %CLast Sent Notice: %W$0-","%s",last_sent_notice[0].last_msg?last_sent_notice[0].last_msg:"None"));
	put_it("%s",convert_output_format("%G³ %CLast Channel invited to: %R$0-","%s",invite_channel?invite_channel:"None"));
	put_it("%s",convert_output_format("%G| %cTotal Users on Userlist: %K[%R$0%K]","%d",user_count));
	put_it("%s",convert_output_format("%G³ %cTotal Users on Shitlist: %K[%R$0%K]","%d",shit_count));

#endif
}

/* extern_write -- controls whether others may write to our terminal or not. */
/* This is basically stolen from bsd -- so its under the bsd copyright */
BUILT_IN_COMMAND(extern_write)
{
	char *tty;
	struct stat sbuf;
	const int OTHER_WRITE = 020;
	int on = 0;
	  
	if (!(tty = ttyname(2)))
	{
		yell("Internal error: notify %s", "edwac@sasknet.sk.ca");
		yell("Error in ttyname()");
		return;
	}
	if (stat(tty, &sbuf) < 0)
	{
		yell("Internal error: noTify %s", "edwac@sasknet.sk.ca");
		yell("Error in stat()");
		return;
	}
	if (!args || !*args)
	{
		if (sbuf.st_mode & 020)
			bitchsay("Mesg is \002On\002");
		else
			bitchsay("Mesg is \002Off\002");
		return;
	}
	if (!my_stricmp(args, "ON") || !my_stricmp(args, "YES"))
		on = 1;
	else if (!my_stricmp(args, "OFF") || !my_stricmp(args, "NO"))
		on = 0;
	else
	{
		userage("Mesg", helparg);
		return;
	}		
	switch (on)
	{
		case 1 :
			if (chmod(tty, sbuf.st_mode | OTHER_WRITE) < 0)
			{
				yell("Sorry, couldnt set your tty's mode");
				return;
			}
			bitchsay("Mesg is \002On\002");
			break;
		case 0 :
			if (chmod(tty, sbuf.st_mode &~ OTHER_WRITE) < 0)
			{
				yell("Sorry, couldnt set your tty's mode");
				return;
			}
			bitchsay("Mesg is \002Off\002");
			break;
	}
	
}


int check_serverlag (void)
{
	int i;
	time_t new_t = now;
		
	for (i = 0; i < number_of_servers; i++)
	{
		if (is_server_connected(i) && (new_t != server_list[i].lag_time))
		{
			server_list[i].lag_time = new_t;
			my_send_to_server(i, "PING %lu %s", server_list[i].lag_time, get_server_itsname(i) ? get_server_itsname(i): get_server_name(i));
			in_server_ping++;
			server_list[i].lag = -1;
		}
	}
	return 0;
}

int timer_unban (void *args)
{
	char *p = (char *)args;
	char *channel;
	ChannelList *chan;
	char *ban;
	char *serv;
	int server = from_server;
	
	serv = next_arg(p, &p);
	if (my_atol(serv) != server)
		server = my_atol(serv);
	if (server < 0 || server > number_of_servers || !server_list[server].connected)
		server = from_server;
	channel = next_arg(p, &p);
	ban = next_arg(p, &p);
	if ((chan = (ChannelList *)find_in_list((List **)&server_list[server].chan_list, channel, 0)) && ban_is_on_channel(ban, chan))
		my_send_to_server(server, "MODE %s -b %s", channel, ban);
	new_free(&serv);
	return 0;
}

int timer_idlekick (void *args)
{
char *channel = (char *)args;
ChannelList *tmp = NULL;
int kick_count = 0;
UserList *user = NULL;

	
	if (channel && (tmp = lookup_channel(channel, from_server, CHAN_NOUNLINK)) && tmp->chop && tmp->max_idle && tmp->check_idle)
	{
		NickList *nick;
		for (nick = next_nicklist(tmp, NULL); nick; nick = next_nicklist(tmp, nick))
		{
			if (!my_stricmp(nick->nick, get_server_nickname(from_server)))
				continue;
			if ((nick->chanop || nick->voice) && !get_cset_int_var(tmp->csets, KICK_OPS_CSET))
				continue;
			if ((user=nick->userlist) && check_channel_match(user->channels, channel))
				continue;			
			if (now - nick->idle_time >= tmp->max_idle)
			{
				if (kick_count <= get_int_var(MAX_IDLEKICKS_VAR))
				{
					char *p = NULL;
					malloc_sprintf(&p, "%s %s*!*%s", channel, nick->nick, nick->host);
					send_to_server("MODE %s +b %s*!*%s", channel, nick->nick, nick->host);
					send_to_server("KICK %s %s :\002%s\002: (Idle Channel User)", channel, nick->nick, version);
					add_timer(0, "", 60, 1, timer_unban, m_sprintf("%d %s", from_server, p), NULL, current_window);
					new_free(&p);
				}
				else
					break;
				kick_count++;
			}
		}
	}		
	if (tmp && tmp->max_idle && tmp->check_idle)
		add_timer(0, "", get_int_var(IDLE_CHECK_VAR), 1, timer_idlekick, channel, NULL, current_window);
	else
		new_free(&channel);

	return 0;
}

BUILT_IN_COMMAND(addidle)
{
time_t default_idle = 10 * 60;
char *channel = NULL, *p;
time_t seconds = 0;
ChannelList *tmp, *new = NULL;

	
	if ((p = next_arg(args, &args)))
	{
		malloc_strcpy(&channel, make_channel(p));
		if (args && *args)
			seconds = atol(args);

		if (seconds < default_idle)
			seconds = default_idle;

		if (!(new = (ChannelList *)find_in_list((List **)&idlechan_list, channel, 0)))
		{
			new = (ChannelList *)new_malloc(sizeof(ChannelList));
			malloc_strcpy(&new->channel, channel);
			add_to_list((List **)&idlechan_list, (List *)new);
		} 
		new->max_idle = seconds;
		new->check_idle = (my_strnicmp(command, "UN", 2) == 0) ? 0: 1;

		if (!new->check_idle)
		{
			bitchsay("Idle checking turned %s for %s",on_off(new->check_idle), channel); 
			if ((tmp = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
				tmp->check_idle = tmp->max_idle = 0;
			new_free(&channel);
			new->max_idle = 0;
		} 
		else
		{
			if ((tmp = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
			{
				if (new && new->check_idle)
				{
					tmp->max_idle = new->max_idle;
					tmp->check_idle = new->check_idle;
					add_timer(0, "", get_int_var(IDLE_CHECK_VAR), 1, timer_idlekick, channel, NULL, current_window);
					bitchsay("Idle checking turned %s for %s %d mins",on_off(tmp->check_idle), channel, (int)(tmp->max_idle/60)); 
				} 
			} else
				new_free(&channel);
		}
	} else
		userage(command, helparg);
}

BUILT_IN_COMMAND(showidle)
{
ChannelList *tmp;
char *channel = NULL;
int count = 0;
NickList *nick, *ntmp;
time_t ltime;
int server;
int sorted = 0;
	while (args && *args)
	{	
		if (!args || !*args)
			break;
		if ((*args == '-') && !my_strnicmp(args, "-sort", 3))
		{
			next_arg(args, &args);
			sorted = NICKSORT_NONE;
		}
		else if (!my_strnicmp(args, "nick", 3) && sorted)
		{
			next_arg(args, &args);
			sorted = NICKSORT_NICK;
		}
		else if (!my_strnicmp(args, "host", 3) && sorted)
		{
			next_arg(args, &args);
			sorted = NICKSORT_HOST;
		}
		else if (!my_strnicmp(args, "time", 3) && sorted)
		{
			next_arg(args, &args);
			sorted = NICKSORT_TIME;
		}
		else if (!my_strnicmp(args, "ip", 2) && sorted)
		{
			next_arg(args, &args);
			sorted = NICKSORT_IP;
		}
		else
			channel = next_arg(args, &args);
	}
	if (!(tmp = prepare_command(&server, channel, NO_OP)))
	{
		userage(command, helparg);
		return;
	}
	ntmp = sorted_nicklist(tmp, sorted);
	for (nick = ntmp; nick; nick = nick->next)
	{
		if (!count && do_hook(SHOWIDLE_HEADER_LIST, "%s %ld", tmp->channel, tmp->max_idle))
			put_it("%s", convert_output_format("$G %W$a%n: Idle check for %W$0%n Max Idle Time %K[%W$1- %K]", "%s %s", tmp->channel, convert_time(tmp->max_idle)));
		ltime = now - nick->idle_time;
		if (do_hook(SHOWIDLE_LIST, "%s %s %d %ld", nick->nick, nick->host, find_user_level(nick->nick, nick->host, tmp->channel), ltime))
			put_it("%s", convert_output_format("$[20]0 Idle%W: %K[%n$1- %K]", "%s %s", nick->nick, convert_time(ltime)));
		count++;
	}
	if (count)
		do_hook(SHOWIDLE_FOOTER_LIST, "%s", "End of idlelist");
	clear_sorted_nicklist(&ntmp);	
}

BUILT_IN_COMMAND(kickidle)
{
char *channel = NULL;
ChannelList *tmp;
int kick_count = 0;
int server = from_server;

	
	if (args && *args)
		channel = next_arg(args, &args);

	if ((tmp = prepare_command(&server, channel, NEED_OP)) && tmp->max_idle)
	{
		NickList *nick;
		for (nick = next_nicklist(tmp, NULL); nick; nick = next_nicklist(tmp, nick))
		{
			if (!my_stricmp(nick->nick, get_server_nickname(from_server)))
				continue;
			if (nick->userlist && check_channel_match(nick->userlist->channels, tmp->channel))
				continue;			
			if (now - nick->idle_time >= tmp->max_idle)
			{
				if (kick_count <= get_int_var(MAX_IDLEKICKS_VAR))
					my_send_to_server(server, "KICK %s %s :\002%s\002: (Idle Channel User)", tmp->channel, nick->nick, version);
				else
					bitchsay(" found idle user %-12s channel %s", nick->nick, tmp->channel);
				kick_count++;
			}
		}
	} else
		userage(command, helparg);
}

void save_idle(FILE *output)
{
ChannelList *chan;
int count = 0;

	
	if (!output)
		return;
	if (idlechan_list)
	{
		fprintf(output, "# %s Idle Channel list\n", version);
		for (chan = idlechan_list; chan; chan = chan->next)
		{
			if (chan->max_idle)
			{
				fprintf(output, "ADDIDLE %s %d\n", chan->channel, (int)chan->max_idle); 
				count++;
			}
		}
	}
	if (count && do_hook(SAVEFILE_LIST, "Idle %d", count))
		bitchsay("Saved %d Idle channels", count);
}

BUILT_IN_COMMAND(channel_stats)
{
ChannelList *new = NULL;
char *channel = NULL;
WhowasChanList *new1 = NULL;
int numircops = 0;
int usershere = 0;
int usersaway = 0;
int chanops = 0;
int chanunop = 0;
char *ircops = NULL;
int server = -1;

NickList *l; 
unsigned long	nick_mem = 0, 
		ban_mem = 0; 
BanList *b;


	

	if (args && *args)
	{
		channel = next_arg(args, &args);
		if (my_strnicmp(channel, "-ALL", strlen(channel)))
		{
			channel = make_channel(channel);
			if (!(new = prepare_command(&server, channel, 3)))
				if ((channel && !(new1 = check_whowas_chan_buffer(channel, 0))))
					return;
		}
		else
		{
	
			int stats_ops= 0, stats_dops = 0, stats_bans = 0, stats_unbans = 0;
			int stats_topics = 0, stats_kicks = 0, stats_pubs = 0, stats_parts = 0;
			int stats_signoffs = 0, stats_joins = 0;
			int total_nicks = 0, max_nicks = 0, total_bans = 0, max_bans = 0;
			int stats_sops = 0, stats_sdops = 0, stats_sbans = 0, stats_sunbans = 0;	
		
			NickList *l; 
			BanList *b;
			unsigned long chan_mem = 0;
			channel =  NULL;
				
			if (from_server != -1)
			{
				for (new = server_list[from_server].chan_list; new; new = new->next)
				{		
					if (!channel)
						malloc_strcpy(&channel, new->channel);
					else
					{
						malloc_strcat(&channel, ",");
						malloc_strcat(&channel, new->channel);
					}
					for (l = next_nicklist(new, NULL); l; l = next_nicklist(new, l))
					{
						switch(l->away)
						{
						case 'H':
							usershere++;
							break;
						case 'G':
							usersaway++;
						default:
							break;
						}
						if (l->ircop)
						{
							numircops++;
							malloc_strcat(&ircops, " (");
							malloc_strcat(&ircops, l->nick);
							malloc_strcat(&ircops, ")");
						}
						if (l->chanop)
							chanops++;
						else
							chanunop++;					
						nick_mem += sizeof(NickList);
					}
					for (b = new->bans; b; b = b->next)
						ban_mem += sizeof(BanList);
					chan_mem += sizeof(ChannelList);
					stats_ops += new->stats_ops;
					stats_dops += new->stats_dops;
					stats_bans += new->stats_bans;
					stats_unbans += new->stats_unbans;
					stats_topics += new->stats_topics;
					stats_kicks += new->stats_kicks;
					stats_pubs += new->stats_pubs;
					stats_parts += new->stats_parts;
					stats_signoffs += new->stats_signoffs;
					stats_joins += new->stats_joins;

					total_nicks += new->totalnicks;
					max_nicks += new->maxnicks;
					total_bans += new->totalbans;
					max_bans += new->maxbans;
					stats_sops += new->stats_sops;
					stats_sdops += new->stats_sdops;
					stats_sbans += new->stats_sbans;
					stats_sunbans += new->stats_sunbans;						
				}
			}
			if (!ircops)
				malloc_strcat(&ircops, empty_string);
if (do_hook(CHANNEL_STATS_LIST, "%s %s %s %lu %lu %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %s", 
	channel, empty_string, empty_string,
	nick_mem+chan_mem+ban_mem, nick_mem, 
	(unsigned long)sizeof(ChannelList),ban_mem, 
	stats_ops, stats_dops, stats_bans, stats_unbans,
	stats_topics, stats_kicks, stats_pubs, stats_parts,
	stats_signoffs, stats_joins, total_bans, max_bans,
	stats_sops, stats_sdops,stats_sbans, stats_sunbans,
	usershere, usersaway, chanops, chanunop,total_nicks,max_nicks,
	numircops, ircops))
{
put_it("%s", convert_output_format("$G %CInformation for channels %K: %W$0", "%s", channel)); 
put_it("%s", convert_output_format("     MEM usage%K:%w Total%K:%w %c$0 bytes   %K[%cNicks $1 b Chan $2 b Bans $3 b%K]", "%d %d %d %d", (int)(nick_mem+chan_mem+ban_mem), (int)nick_mem, (int)sizeof(ChannelList), (int)ban_mem));
put_it("%s", convert_output_format("Ops        %K[%W$[-5]0%K]%w  De-Ops     %K[%W$[-5]1%K]%w  Bans       %K[%W$[-5]2%K]%w  Unbans     %K[%W$[-5]3%K]%w", "%d %d %d %d", stats_ops, stats_dops, stats_bans, stats_unbans));
put_it("%s", convert_output_format("Topics     %K[%W$[-5]0%K]%w  Kicks      %K[%W$[-5]1%K]%w  Publics    %K[%W$[-5]2%K]%w  Parts      %K[%W$[-5]3%K]%w", "%d %d %d %d", stats_topics, stats_kicks, stats_pubs, stats_parts));
put_it("%s", convert_output_format("Signoffs   %C[%W$[-5]0%K]%w  Joins      %K[%W$[-5]1%K]%w  TotalBans  %K[%W$[-5]2%K]%w  MaxBans    %K[%W$[-5]3%K]%w", "%d %d %d %d", stats_signoffs, stats_joins, total_bans, max_bans));
put_it("%s", convert_output_format("ServOps    %K[%W$[-5]0%K]%w  ServDeop   %K[%W$[-5]1%K]%w  ServBans   %K[%W$[-5]2%K]%w  ServUB     %K[%W$[-5]3%K]%w", "%d %d %d %d", stats_sops, stats_sdops,stats_sbans, stats_sunbans));
put_it("%s", convert_output_format("Users Here %K[%W$[-5]0%K]%w  Users Away %K[%W$[-5]1%K]%w  Opped      %K[%W$[-5]2%K]%w  Unopped    %K[%W$[-5]3%K]%w", "%d %d %d %d", usershere, usersaway, chanops, chanunop));
put_it("%s", convert_output_format("TotalNicks %K[%W$[-5]0%K]%w  MaxNicks   %K[%W$[-5]1%K]%w", "%d %d", total_nicks,max_nicks));
put_it("%s", convert_output_format("IRCops     %K[%W$[3]0%K]%w$1-", "%d %s", numircops, ircops));
}
			new_free(&ircops);
			new_free(&channel);
			return;		
		}
	}
	else 
	{
		if (!(new = prepare_command(&server, channel, 3)))
			if ((channel && !(new1 = check_whowas_chan_buffer(channel, 0))))
				return;
	}

	if (!new && new1)
		new = new1->channellist;
	if (!new)
	{
		bitchsay("Try joining a channel first");
		return;
	}		
	if (new)
	{
		for (l = next_nicklist(new, NULL); l; l = next_nicklist(new, l))
		{
			nick_mem += sizeof(NickList);
			switch(l->away)
			{
				case 'H':
					usershere++;
					break;
				case 'G':
					usersaway++;
				default:
					break;
			}
			if (l->ircop)
			{
				numircops++;
				malloc_strcat(&ircops, " (");
				malloc_strcat(&ircops, l->nick);
				malloc_strcat(&ircops, ")");
			}
			if (l->chanop)
				chanops++;
			else
				chanunop++;					
		}
		for (b = new->bans; b; b = b->next)
			ban_mem += sizeof(BanList);
	}
	if (!ircops)
		malloc_strcat(&ircops, empty_string);
	if (do_hook(CHANNEL_STATS_LIST, "%s %s %s %ld %ld %ld %ld %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %s", 
		new->channel, my_ctime(new->channel_create.tv_sec), convert_time(now-new->join_time.tv_sec),
		nick_mem+sizeof(ChannelList)+ban_mem, nick_mem, 
		(unsigned long)sizeof(ChannelList),ban_mem, 
		new->stats_ops, new->stats_dops, new->stats_bans, new->stats_unbans,
		new->stats_topics, new->stats_kicks, new->stats_pubs, new->stats_parts,
		new->stats_signoffs, new->stats_joins, new->totalbans, new->maxbans,
		new->stats_sops, new->stats_sdops,new->stats_sbans, new->stats_sunbans,
		usershere, usersaway, chanops, chanunop,new->totalnicks,new->maxnicks,
		numircops, ircops))
	{
put_it("%s", convert_output_format("$G %CInformation for channel %K: %W$0", "%s", new->channel)); 
put_it("%s", convert_output_format("$G %CChannel created %K: %W$0 $1 $2 $3%n in memory %W$4-", "%s %s", convert_time(now-new->channel_create.tv_sec), my_ctime(new->join_time.tv_sec)));

put_it("%s", convert_output_format("     MEM usage%K:%w Total%K:%w %c$0 bytes   %K[%cNicks $1 b Chan $2 b Bans $3 b%K]", "%d %d %d %d", (int)(nick_mem+sizeof(ChannelList)+ban_mem), (int)nick_mem, (int)sizeof(ChannelList), (int)ban_mem));
put_it("%s", convert_output_format("Ops        %K[%W$[-5]0%K]%w  De-Ops     %K[%W$[-5]1%K]%w  Bans       %K[%W$[-5]2%K]%w  Unbans     %K[%W$[-5]3%K]%w", "%d %d %d %d", new->stats_ops, new->stats_dops, new->stats_bans, new->stats_unbans));
put_it("%s", convert_output_format("Topics     %K[%W$[-5]0%K]%w  Kicks      %K[%W$[-5]1%K]%w  Publics    %K[%W$[-5]2%K]%w  Parts      %K[%W$[-5]3%K]%w", "%d %d %d %d", new->stats_topics, new->stats_kicks, new->stats_pubs, new->stats_parts));
put_it("%s", convert_output_format("Signoffs   %K[%W$[-5]0%K]%w  Joins      %K[%W$[-5]1%K]%w  TotalBans  %K[%W$[-5]2%K]%w  MaxBans    %K[%W$[-5]3%K]%w", "%d %d %d %d", new->stats_signoffs, new->stats_joins, new->totalbans, new->maxbans));
put_it("%s", convert_output_format("ServOps    %K[%W$[-5]0%K]%w  ServDeop   %K[%W$[-5]1%K]%w  ServBans   %K[%W$[-5]2%K]%w  ServUB     %K[%W$[-5]3%K]%w", "%d %d %d %d", new->stats_sops, new->stats_sdops,new->stats_sbans, new->stats_sunbans));
put_it("%s", convert_output_format("Users Here %K[%W$[-5]0%K]%w  Users Away %K[%W$[-5]1%K]%w  Opped      %K[%W$[-5]2%K]%w  Unopped    %K[%W$[-5]3%K]%w", "%d %d %d %d", usershere, usersaway, chanops, chanunop));
put_it("%s", convert_output_format("TotalNicks %K[%W$[-5]0%K]%w  MaxNicks   %K[%W$[-5]1%K]%w", "%d %d", new->totalnicks,new->maxnicks));
put_it("%s", convert_output_format("IRCops     %K[%W$[3]0%K]%w$1-", "%d %s", numircops, ircops));

put_it("%s", convert_output_format("  %CThere is %R$0%C limit and limit checking is %R$1-", "%s %s", new->limit ? ltoa(new->limit): "no", new->tog_limit?"Enabled":"Disabled"));
put_it("%s", convert_output_format("  %CIdle user check is %K[%R$0-%K]", "%s", new->check_idle?"Enabled":"Disabled"));
/*put_it("%s", convert_output_format("$G End of channel stats for $0", "%s", new->channel));*/
			/* wtf is do_scan in the channel struct */
	}
	new_free(&ircops);

}

void update_stats(int what, char *channel, NickList *nick, ChannelList *chan, int splitter)
{
time_t this_time = now;
int t = 0;
	

	if (!chan || !chan->channel)
		return;

	switch (what)
	{
		case KICKLIST:
		{
			chan->stats_kicks++;
			chan->totalnicks--;
			if (nick) nick->stat_kicks++;
			break;
		}

		case LEAVELIST:
		{
			chan->stats_parts++;
			chan->totalnicks--;
			break;
		}
		case JOINLIST:
		{
			chan->stats_joins++;
			chan->totalnicks++;
			if (chan->totalnicks > chan->maxnicks)
			{
				chan->maxnicks = chan->totalnicks;
				chan->maxnickstime = this_time;
			}
			if (!splitter)
			{
				if (chan->chop && is_other_flood(chan, nick, JOIN_FLOOD, &t))
				{
					if (get_cset_int_var(chan->csets, JOINFLOOD_CSET) && get_cset_int_var(chan->csets, KICK_ON_JOINFLOOD_CSET) && !nick->kickcount++)
					{
						send_to_server("MODE %s -o+b %s *!*%s", chan->channel, nick->nick, clear_server_flags(nick->host)); 
						send_to_server("KICK %s %s :\002Join flood\002 (%d joins in %dsecs of %dsecs)", chan->channel, nick->nick, get_cset_int_var(chan->csets, KICK_ON_JOINFLOOD_CSET)/*chan->set_kick_on_joinflood*/, t, get_cset_int_var(chan->csets, JOINFLOOD_TIME_CSET));
						if (get_int_var(AUTO_UNBAN_VAR))
							add_timer(0, "", get_int_var(AUTO_UNBAN_VAR), 1, timer_unban, m_sprintf("%d %s *!*%s", from_server, chan->channel, clear_server_flags(nick->host)), NULL, current_window);
					}
				} 
			}
			break;
		}
		case CHANNELSIGNOFFLIST:
		{
			chan->stats_signoffs++;
			chan->totalnicks--;
			break;
		}
		case PUBLICLIST:
		case PUBLICOTHERLIST:
		case PUBLICNOTICELIST:
		case NOTICELIST:
		{
			chan->stats_pubs++;
			if (nick)
			{	
				nick->stat_pub++;
				nick->idle_time = this_time;
			}
			break;
		}
		case TOPICLIST:
		{
			chan->stats_topics++;
			break; 
		}
		case MODEOPLIST:
			if (splitter)
				chan->stats_sops++;
			else
			{
				if (nick) nick->stat_ops++;		
				chan->stats_ops++;
			}
			break;
		case MODEHOPLIST:
			if (splitter)
				chan->stats_shops++;
			else
			{
				if (nick) nick->stat_hops++;		
				chan->stats_hops++;
			}
			break;
		case MODEDEHOPLIST:
			if (splitter)
				chan->stats_sdehops++;
			else
			{
				if (nick) nick->stat_dhops++;		
				chan->stats_dhops++;
			}
			break;
		case MODEDEOPLIST:
			if (splitter)
				chan->stats_sdops++;
			else
			{
				chan->stats_dops++;		
				if (nick) nick->stat_dops++;
			}

			if (chan->chop && is_other_flood(chan, nick, DEOP_FLOOD, &t))
			{
				if (get_cset_int_var(chan->csets, DEOP_ON_DEOPFLOOD_CSET) < get_cset_int_var(chan->csets, KICK_ON_DEOPFLOOD_CSET))
					send_to_server("MODE %s -o %s", chan->channel, nick->nick);
				else if (!nick->kickcount++)
					send_to_server("KICK %s %s :\002De-op flood\002 (%d de-ops in %dsecs of %dsecs)", chan->channel, nick->nick, get_cset_int_var(chan->csets, KICK_ON_DEOPFLOOD_CSET), t, get_cset_int_var(chan->csets, DEOPFLOOD_TIME_CSET)); 
			} 
			break;
		case MODEBANLIST:
			if (splitter)
				chan->stats_sbans++;
			else
			{
				if (nick) nick->stat_bans++;		
				chan->stats_bans++;
			}
			chan->totalbans++;
			if (chan->stats_bans > chan->maxbans)
			{
				chan->maxbans = chan->stats_bans;
				chan->maxbanstime = this_time;
			}
			break;
		case MODEUNBANLIST:
			if (splitter)
				chan->stats_sunbans++;
			else
			{
				if (nick) nick->stat_unbans++;
				chan->stats_unbans++;
			}
			if (chan->totalbans) chan->totalbans--;
			break;
		default:
			break;
	}
}

BUILT_IN_COMMAND(usage)
{
#if defined(HAVE_GETRUSAGE)
struct rusage r_usage;

	
	if ((0 == getrusage(RUSAGE_SELF, &r_usage)))
	{
		/* struct timeval ru_utime; user time used 
                 * struct timeval ru_stime;  system time used 
		 */                
		int secs = r_usage.ru_utime.tv_sec + r_usage.ru_stime.tv_sec;
		if (secs == 0)
			secs =1;
		
#ifdef ONLY_STD_CHARS
		put_it("%s", convert_output_format("%G--%WBitchX%G-%WUsage%G-%WStatistics%G------------------------------------", NULL));
		put_it("%s",convert_output_format("%G| %CCPU %cUsage:  Secs %W$[-2]0%n:%W$[-2]1%n     %K[%CU%cser %W$[-2]2%n:%W$[-2]3   %CS%cystem %W$[-2]4%n:%W$[-2]5%K]","%d %d %d %d %d %d", secs/60,secs%60,r_usage.ru_utime.tv_sec/60, r_usage.ru_utime.tv_sec%60,r_usage.ru_stime.tv_sec/60, r_usage.ru_stime.tv_sec%60));
		put_it("%s",convert_output_format("%G| %CMEM %cUsage:  MaXRSS %W$0   %cShMem %W$1  %cData %W$2  %cStack %W$3","%l %l %l %l", r_usage.ru_maxrss, r_usage.ru_ixrss, r_usage.ru_idrss,r_usage.ru_isrss));
		put_it("%s",convert_output_format("%G| %CSwaps %W$[-8]0   %CReclaims %W$[-8]1   %CFaults %W$[-8]2","%l %l %l", r_usage.ru_nswap, r_usage.ru_minflt, r_usage.ru_majflt));
		put_it("%s",convert_output_format("%G| %CBlock %K[%cin  %W$[-8]0  %cout %W$[-8]1%K]","%l %l", r_usage.ru_inblock, r_usage.ru_oublock));
		put_it("%s",convert_output_format("%G| %CMsg   %K[%cRcv %W$[-8]0 %cSend %W$[-8]1%K]","%l %l", r_usage.ru_msgrcv, r_usage.ru_msgsnd));
		put_it("%s",convert_output_format("%G| %CSignals %W$[-8]0   %CContext %cVol. %W$[-8]1   %cInvol %W$[-8]2","%l %l %l", r_usage.ru_nsignals, r_usage.ru_nvcsw, r_usage.ru_nivcsw));

#else
		put_it("%s", convert_output_format("%GÚÄ%WBitchX%GÄ%WUsage%GÄ%WStatistics%GÄÄÄÄ%g---%GÄ%g--%GÄÄ%g-%GÄÄÄÄÄÄ%g---%KÄ%g--%KÄÄ%g-%KÄÄÄÄÄÄÄÄ", NULL));
		put_it("%s",convert_output_format("%G| %CCPU %cUsage:  Secs %W$[-2]0%n:%W$[-2]1%n     %K[%CU%cser %W$[-2]2%n:%W$[-2]3   %CS%cystem %W$[-2]4%n:%W$[-2]5%K]","%d %d %d %d %d %d", secs/60,secs%60,r_usage.ru_utime.tv_sec/60, r_usage.ru_utime.tv_sec%60,r_usage.ru_stime.tv_sec/60, r_usage.ru_stime.tv_sec%60));
		put_it("%s",convert_output_format("%g³ %CMEM %cUsage:  MaXRSS %W$0   %cShMem %W$1  %cData %W$2  %cStack %W$3","%l %l %l %l", r_usage.ru_maxrss, r_usage.ru_ixrss, r_usage.ru_idrss,r_usage.ru_isrss));
		put_it("%s",convert_output_format("%g| %CSwaps %W$[-8]0   %CReclaims %W$[-8]1   %CFaults %W$[-8]2","%l %l %l", r_usage.ru_nswap, r_usage.ru_minflt, r_usage.ru_majflt));
		put_it("%s",convert_output_format("%K³ %CBlock %K[%cin  %W$[-8]0  %cout %W$[-8]1%K]","%l %l", r_usage.ru_inblock, r_usage.ru_oublock));
		put_it("%s",convert_output_format("%K: %CMsg   %K[%cRcv %W$[-8]0 %cSend %W$[-8]1%K]","%l %l", r_usage.ru_msgrcv, r_usage.ru_msgsnd));
		put_it("%s",convert_output_format("%K. %CSignals %W$[-8]0   %CContext %cVol. %W$[-8]1   %cInvol %W$[-8]2","%l %l %l", r_usage.ru_nsignals, r_usage.ru_nvcsw, r_usage.ru_nivcsw));
#endif
	}
#else
	bitchsay("Lack of getrusage(). This function needed to be disabled on your client");
#endif
}

char *clear_server_flags (char *userhost)
{
register char *uh = userhost;
	while(uh && (*uh == '~' || *uh == '#' || *uh == '+' || *uh == '-' || *uh == '=' || *uh == '^'))
		uh++;
	return uh;
}


/*  
 * (max server send) and max mirc color change is 256
 * so 256 * 8 should give us a safety margin for hackers.
 * BIG_BUFFER is 1024 * 3 is 3072 whereas 256*8 is 2048
 */
 
static unsigned char newline1[BIG_BUFFER_SIZE+1];

#ifndef BITCHX_LITE1
char *mircansi(unsigned char *line)
{
/* mconv v1.00 (c) copyright 1996 Ananda, all rights reserved.	*/
/* -----------------------------------------------------------	*/
/* mIRC->ansi color code convertor:	12.26.96		*/
/* map of mIRC color values to ansi color codes			*/
/* format: ansi fg color	ansi bg color			*/
/* modified Colten Edwards 					*/
struct {
	char *fg, *bg;
} codes[16] = {

	{ "[1;37m",   "[47m"        },      /* white                */
	{ "[0;30m",   "[40m"        },      /* black (grey for us)  */
	{ "[0;34m",   "[44m"        },      /* blue                 */
	{ "[0;32m",   "[42m"        },      /* green                */
	{ "[0;31m",   "[41m"        },      /* red                  */
	{ "[0;33m",   "[43m"        },      /* brown                */

	{ "[0;35m",   "[45m"        },      /* magenta              */
	{ "[1;31m",   "[41m"        },      /* bright red           */
	{ "[1;33m",   "[43m"        },      /* yellow               */

	{ "[1;32m",   "[42m"            },      /* bright green         */
	{ "[0;36m",   "[46m"            },      /* cyan                 */
	{ "[1;36m",   "[46m"            },      /* bright cyan          */
	{ "[1;34m",   "[44m"            },      /* bright blue          */
	{ "[1;35m",   "[45m"            },      /* bright magenta       */
	{ "[1;30m",   "[40m"            },      /* dark grey            */
	{ "[0;37m",   "[47m"            }       /* grey                 */
};
	register unsigned char *sptr = line, *dptr = newline1;
	short code;
	
	if (!*line)
		return empty_string;
	*newline1 = 0;
	while (*sptr) {
		if (*sptr == '' && isdigit(sptr[1])) 
		{
			sptr++;
			code = *sptr - '0';
			sptr++;
			if isdigit (*sptr) {
			  code = code * 10 + *sptr - '0';
			  sptr++;
			}
			if (code > 15 || code <= 0) code = code % 16;
			strcpy(dptr, codes[code].fg);
			while (*dptr) dptr++;
			if (*sptr == ',') 
			{
				sptr++;
             			code = *sptr - '0';
				sptr++;
	        		if isdigit (*sptr) {
		          	  code = code * 10 + *sptr - '0';
				  sptr++;
         			}
	           		if (code > 15 || code <= 0) code = code % 16;
				strcpy(dptr, codes[code].bg);
				while (*dptr) dptr++;
			}
		} 
		else if (*sptr == '')
		{
			strcpy(dptr, "[0m");
			while(*dptr) dptr++;
			sptr++;
		}
		else *dptr++ = *sptr++;
	}
	*dptr = 0;
	return (char *)newline1;
}
#endif

/* Borrowed with permission from FLiER */
char *stripansicodes(const unsigned char *line)
{
register unsigned char *tstr;
register unsigned char *nstr;
int  gotansi=0;

	tstr=(char *)line;
	nstr=newline1;
	while (*tstr) 
	{
		if (*tstr==0x1B || *tstr == 0x9b) 
			gotansi=1;
		if (gotansi && isalpha(*tstr)) 
			gotansi = 0;
		else if (!gotansi) 
		{
			*nstr = *tstr;
			nstr++;
		}
		tstr++;
	}
	*nstr = 0;
	return (char *)newline1;
}

char *stripansi(unsigned char *line)
{
register unsigned char    *cp;
unsigned char *newline;
	newline = m_strdup(line);        
	for (cp = newline; *cp; cp++)
		if ((*cp < 31 && *cp > 13))
			if (*cp != 1 && *cp != 15 && *cp !=22 && *cp != 0x9b)
				*cp = (*cp & 127) | 64;
	return (char *)newline;
}


int check_split(char *nick, char *reason, char *chan)
{
char *bogus = get_string_var(FAKE_SPLIT_PATS_VAR);
char *Reason = m_strdup(reason);
char *tmp;
	
	tmp = Reason;
	if (word_count(Reason) > 3)
		goto fail_split;
	if (wild_match("%.% %.%", Reason) && !strstr(Reason, "))") )
	{
		char *host1 = next_arg(Reason, &Reason);
		char *host2 = next_arg(Reason, &Reason);
		char *x = NULL;

		if (!my_stricmp(host1, host2))
			goto fail_split;
		if (wild_match(host1, "*..*") || wild_match(host2, "*..*"))
			goto fail_split;
		if ((x = strrchr(host1, '.')))
			x++;
		if (!x || strlen(x) < 2 || strlen(x) > 3)
			goto fail_split;
		if ((x = strrchr(host2, '.')))
			x++;
		if (!x || strlen(x) < 2 || strlen(x) > 3)
			goto fail_split;
		
		if (bogus)
		{
			char *copy = NULL;
			char *b_check;
			char *temp;
			malloc_strcpy(&copy, bogus);
			temp = copy;
			while((b_check = next_arg(copy, &copy)))
			{
				if (wild_match(b_check, host1) || wild_match(b_check, host2))
				{
					new_free(&temp);
					goto fail_split;
				}
			}
			new_free(&temp);
		}
		new_free(&tmp);
		return 1;
	} 
fail_split:
	new_free(&tmp);
	return 0;
}

void clear_array(NickTab **tmp)
{
NickTab *t, *q;
	
	for (t = *tmp; t; )
	{
		q = t->next;
		new_free(&t->nick);
		new_free(&t->type);
		new_free((char **)&t);
		t = q;
	}
	*tmp = NULL;
}

BUILT_IN_COMMAND(clear_tab)
{
NickTab **tmp = &tabkey_array;
	if (command && *command && !my_stricmp(command, "CLEARAUTO"))
		tmp = &autoreply_array;		
	clear_array(tmp);
}

void userage(char *command, char *use)
{
	if (command)
	{
		if (do_hook(USAGE_LIST, "%s %s", command, use ? use : "No Help Available for this command"))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_USAGE_FSET), "%s %s", command, convert_output_format(use ? use : "%WNo Help available for this command", NULL, NULL)));
	}
	else
		put_it("Please return the command you just typed to #bitchx on efnet");
}

char *random_str(int min, int max)
{
	int i, ii;
	static char str[BIG_BUFFER_SIZE/4+1];


	
	while ((i = getrandom(min, max)) > BIG_BUFFER_SIZE/4)
		;
	for (ii = 0; ii < i; ii++)
		str[ii] = (char) getrandom(97, 122);
	str[ii] = '\0';
	return str;
}


void auto_away(unsigned long value)
{
	
	if (get_int_var(AUTO_AWAY_VAR) && !get_server_away(from_server))
	{
		char *msg = NULL;
		if (awaymsg)
			malloc_sprintf(&msg, "%s: [%d mins]", convert_output_format(awaymsg, NULL), get_int_var(AUTO_AWAY_TIME_VAR)/60);
		else
			malloc_sprintf(&msg, "Auto-Away after %d mins", get_int_var(AUTO_AWAY_TIME_VAR)/60);
		away(NULL, msg, NULL, NULL);
		new_free(&msg);		
	}
}

/*char *logfile[] = { "tcl.log", "msg.log", NULL };*/

/* putlog(level,channel_name,format,...);  */
void putlog(int type, ...)
{
va_list va; 
time_t	t;
char	*format,
	*chname,
	*logfilen = NULL, 
	s[BIG_BUFFER_SIZE+1],
	s1[40],
	s2[BIG_BUFFER_SIZE+1];
FILE	*f; 
	if (!get_int_var(BOT_LOG_VAR))
		return;
	if (!(logfilen = get_string_var(BOT_LOGFILE_VAR)))
		return;
#ifdef PUBLIC_ACCESS
		return;
#endif
			
	va_start(va, type); 
	t = now;
	strftime(s1, 30, "%I:%M%p", localtime(&t));
	chname=va_arg(va,char *);
	format=va_arg(va,char *);
	vsprintf(s,format,va);
	
	if (!*s) 
		strcpy(s2,empty_string);
	else 
		sprintf(s2,"[%s] %s",s1,s); 

	if (chname && *chname =='*')
	{
		if ((f=fopen(logfilen, "a+")) != NULL)
		{
			fprintf(f,"%s\n",s2); 
			fclose(f);
		}
	}
}

int rename_file (char *old_file, char **new_file)
{
	char *tmp = NULL;
	char buffer[BIG_BUFFER_SIZE+4];
	char *c = NULL;
	FILE *fp;
			

	
	malloc_sprintf(&c,"%03i.",getrandom(0, 999));
	if (get_string_var(DCC_DLDIR_VAR))
		malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), &c);
	else
		malloc_sprintf(&tmp, "%s", &c); 
	malloc_strcat(&tmp, *new_file);
	strmcpy(buffer, *new_file, BIG_BUFFER_SIZE);
	while ((fp = fopen(tmp, "r")) != NULL)
	{
		fclose(fp);
		new_free(&c);
                malloc_sprintf(&c,"%03i.",getrandom(0, 999));
		if (get_string_var(DCC_DLDIR_VAR))
			malloc_sprintf(&tmp, "%s/%s", get_string_var(DCC_DLDIR_VAR), &c);
		else
			malloc_sprintf(&tmp, "%s", &c); 
		malloc_strcat(&tmp, buffer);
	}
	if (fp != NULL)
		fclose(fp);
	malloc_sprintf(new_file, "%s", c);
	malloc_strcat(new_file, buffer);
	new_free(&c);
	new_free(&tmp);
	return 0;
}

int isme(char *nick)
{
	return ((my_stricmp(nick, get_server_nickname(from_server)) == 0) ? 1 : 0);
}


void clear_link(irc_server **serv1)
{
irc_server *temp = *serv1, *hold;

	while (temp != NULL)
	{
		hold = temp->next;
		new_free(&temp->name);
		new_free(&temp->link);
		new_free(&temp->time);
		new_free((char **) &temp);
		temp = hold;
	}
	*serv1 = NULL;
}

irc_server *add_server(irc_server **serv1, char *channel, char *arg, int hops, char *time)
{
irc_server *serv2;
	serv2 = (irc_server *) new_malloc(sizeof (irc_server));
	serv2->next = *serv1;
	malloc_strcpy(&serv2->name, channel);
	malloc_strcpy(&serv2->link, arg);
	serv2->hopcount = hops;
	malloc_strcpy(&serv2->time, time);
	*serv1 = serv2;
	return serv2;
}

int find_server(irc_server *serv1, char *channel)
{
register irc_server *temp;

	for (temp = serv1; temp; temp = temp->next)
	{
		if (!my_stricmp(temp->name, channel))
			return 1;
	}
	return 0;
}

void add_split_server(char *name, char *link, int hops)
{
irc_server *temp;
	temp = add_server(&(server_list[from_server].split_link), name, link, hops, update_clock(GET_TIME));
	temp->status = SPLIT;
}

irc_server *check_split_server(char *server)
{
register irc_server *temp;
	if (!server) return NULL;
	for (temp = server_list[from_server].split_link; temp; temp = temp->next)
		if (!my_stricmp(temp->name, server))
			return temp;
	return NULL;
}

void remove_split_server(char *server)
{
irc_server *temp;
	if ((temp = (irc_server *) remove_from_list((List **)&server_list[from_server].split_link, server)))
	{
		new_free(&temp->name);
		new_free(&temp->link);
		new_free(&temp->time);
		new_free((char **) &temp);
	}
}

void parse_364(char *channel, char *args, char *subargs)
{
	if (!*channel || !*args || from_server < 0)
		return;

	add_server(&server_list[from_server].tmplink, channel, args, atol(subargs), update_clock(GET_TIME));
}

void parse_365(char *channel, char *args, char *subargs)
{
register irc_server *serv1;

	for (serv1 = server_list[from_server].server_last; serv1; serv1 = serv1->next)
	{
		if (!find_server(server_list[from_server].tmplink, serv1->name))
		{
			if (!(serv1->status & SPLIT))
				serv1->status = SPLIT;
			if (serv1->count)
				continue;
			malloc_strcpy(&serv1->time, update_clock(GET_TIME));
			if (do_hook(LLOOK_SPLIT_LIST, "%s %s %d %s", serv1->name, serv1->link, serv1->hopcount, serv1->time))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NETSPLIT_FSET), "%s %s %s %d", serv1->time, serv1->name, serv1->link, serv1->hopcount));
			serv1->count++;
		}
		else
		{
			if (serv1->status & SPLIT)
			{
				serv1->status = ~SPLIT;
				if (do_hook(LLOOK_JOIN_LIST, "%s %s %d %s", serv1->name, serv1->link, serv1->hopcount, serv1->time))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NETJOIN_FSET), "%s %s %s %d", serv1->time, serv1->name, serv1->link, serv1->hopcount));
				serv1->count = 0;
			}
		}
	}
	for (serv1 = server_list[from_server].tmplink; serv1; serv1 = serv1->next)
	{
		if (!find_server(server_list[from_server].server_last, serv1->name)) 
		{
			if (first_time == 1)
			{
				if (do_hook(LLOOK_ADDED_LIST, "%s %s %d", serv1->name, serv1->link, serv1->hopcount))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NETADD_FSET), "%s %s %s %d", serv1->time, serv1->name, serv1->link, serv1->hopcount));
				serv1->count = 0;
			}
			add_server(&server_list[from_server].server_last, serv1->name, serv1->link, serv1->hopcount, update_clock(GET_TIME));
		}
	}
	first_time = 1;
	clear_link(&server_list[from_server].tmplink);
}

/*
 * find split servers we hope 
 */
BUILT_IN_COMMAND(linklook)
{
struct server_split *serv;
int count;

	if (from_server == -1 || !(serv = server_list[from_server].server_last))
	{
		bitchsay("No active splits");
		return;
	}
		
	count = 0;
	while (serv)
	{
		if (serv->status & SPLIT)
		{
			if (!count)
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NETSPLIT_HEADER_FSET), "%s %s %s %s", "time","server","uplink","hops"));
			if (do_hook(LLOOK_SPLIT_LIST, "%s %s %d", serv->name, serv->link, serv->hopcount))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NETSPLIT_FSET), "%s %s %s %d", serv->time, serv->name, serv->link, serv->hopcount));
			count++;
		}
		serv = serv->next;
	}
	if (count)
		bitchsay("There %s %d split servers", (count == 1) ? "is": "are", count);
	else
		bitchsay("No split servers found");
}

enum REDIR_TYPES { PRIVMSG = 0, KICK, TOPIC, WALL, WALLOP, NOTICE, KBOOT, KILL, DCC, LIST};
void userhost_ban(UserhostItem *stuff, char *nick1, char *args);

int redirect_msg(char *to, enum REDIR_TYPES what, char *str, int showansi)
{
char *new_str;
	if (showansi)
		new_str = str;
	else
		new_str = stripansicodes(str);
	switch(what)
	{
		case PRIVMSG:
			if (is_channel(to))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_PUBLIC_FSET), "%s %s %s %s", update_clock(GET_TIME), "*", to, str));
			else if ((*to == '=') && dcc_activechat(to+1))
				;
			else
				put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_MSG_FSET), "%s %s %s %s", update_clock(GET_TIME), to, "*", str));
			if ((*to == '=') && dcc_activechat(to+1))
				dcc_chat_transmit(to+1, new_str, str, "PRIVMSG", 1);
			else
				send_to_server("PRIVMSG %s :%s", to, new_str);
			break;
		case KILL:
			send_to_server("KILL %s :%s", to, new_str);
			break;
		case KBOOT:
			userhostbase(to, userhost_ban, 1, "%s %s %s", get_current_channel_by_refnum(0), to, "");
		case KICK:
			send_to_server("KICK %s %s :%s", get_current_channel_by_refnum(0), to, new_str);
			break;
		case TOPIC:
			send_to_server("TOPIC %s :%s", to, new_str);
			break;
		case WALL:
		{
			ChanWallOp(NULL, new_str, NULL, NULL);
			break;
		}
		case WALLOP:
			put_it("!! %s", str); 
			send_to_server("WALLOPS :%s", new_str);
			break;
		case NOTICE:
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SEND_NOTICE_FSET), "%s %s %s %s", update_clock(GET_TIME), to, "*", str));
			send_to_server("NOTICE %s :%s", to, new_str);
			break;
		case LIST:
		default:
			break;
	}
	return 1;
}

BUILT_IN_COMMAND(do_dirlasttype)
{
	char *channel = NULL; int count = -1;
	LastMsg *t = NULL;
	char *form = NULL;
	char *sform;
	int numargs = 5;
	int size = 1;
	int len = strlen(command);
	int showansi = 0;
	enum REDIR_TYPES what = PRIVMSG;

	if (!my_strnicmp(command, "RELCR", 5))
	{
		t = &last_ctcp[0];
		form = fget_string_var(FORMAT_CTCP_REPLY_FSET);
		sform = "%s %s %s %s %s";
		if (len == 6 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELC", 4))
	{
		t = &last_sent_ctcp[0];
		form = fget_string_var(FORMAT_SEND_CTCP_FSET);
		sform = "%s %s %s %s %s";
		if (len > 4 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELD", 4))
	{
		t = &last_dcc[0];
		form = fget_string_var(FORMAT_DCC_CHAT_FSET);
		sform = "%s %s %s %s";
		if (len > 4 && command[len-1] == 'T')
			what = TOPIC;
		numargs = 4;
	}
	else if (!my_strnicmp(command, "RELSD", 5))
	{
		t = &last_sent_dcc[0];
		form = fget_string_var(FORMAT_DCC_CHAT_FSET);
		sform = "%s %s %s %s";
		if (len > 5 && command[len-1] == 'T')
			what = TOPIC;
		numargs = 4;
	}
	else if (!my_strnicmp(command, "RELI", 4))
	{
		t = &last_invite_channel[0];
		form = fget_string_var(FORMAT_INVITE_FSET);
		numargs = 4;
		sform = "%s %s %s";
		if (len > 4 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELM", 4))
	{
		/* ??? */
		t = &last_msg[0]; size = MAX_LAST_MSG;
		form = fget_string_var(FORMAT_RELM_FSET);
		sform = "%s %s %s %s %s";
		if (len > 4 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELN", 4))
	{
		/* ??? */
		t = &last_notice[0]; size = MAX_LAST_MSG;
		form = fget_string_var(FORMAT_RELN_FSET);
		sform = "%s %s %s %s %s";
		if (len > 4 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELSM", 5))
	{
		/* ??? */
		t = &last_sent_msg[0]; size = MAX_LAST_MSG;
		form = fget_string_var(FORMAT_RELSM_FSET);
		sform = "%s %s %s %s %s";
		if (len > 5 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELSN", 5))
	{
		/* ??? */
		t = &last_sent_notice[0]; size = MAX_LAST_MSG;
		form = fget_string_var(FORMAT_SEND_NOTICE_FSET);
		sform = "%s %s %s %s";
		numargs = 4;
		if (len > 5 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELST", 5))
	{
		/* ??? */
		t = &last_sent_topic[0];
		form = fget_string_var(FORMAT_TOPIC_FSET);
		sform = "%s %s %s";
		numargs = 2;
		if (len > 5 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELSW", 5))
	{
		/* ??? */
		t = &last_sent_wall[0];
		form = fget_string_var(FORMAT_WALLOP_FSET);
		sform = "%s %s %s %s %s";
		if (len > 5 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELS", 4))
	{
		t = &last_servermsg[0]; size = MAX_LAST_MSG;
		form = fget_string_var(FORMAT_RELS_FSET);
		sform = "%s %s %s %s";
		numargs = 4;
		if (len > 4 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELT", 4))
	{
		/* ??? */
		t = &last_topic[0];
		form = fget_string_var(FORMAT_TOPIC_FSET);
		sform = "%s %s %s";
		numargs = 2;
		if (len > 4 && command[len-1] == 'T')
			what = TOPIC;
	}
	else if (!my_strnicmp(command, "RELW", 4))
	{
		/* ??? */
		t = &last_wall[0]; size = MAX_LAST_MSG;
		form = fget_string_var(FORMAT_WALLOP_FSET);
		sform = "%s %s %s %s";
		numargs = 4;
		if (len > 4 && command[len-1] == 'T')
			what = TOPIC;
	} 
	else
	{
		what = LIST; size = MAX_LAST_MSG;
		t = &last_msg[0];
		for (count = size - 1; count != -1; count--)
		{
			if (t[count].last_msg)
#if 0
			{
				bitchsay("No such msg #%d for /%s received", count, command);
				break;
			} else
#endif
				put_it("%2d %s", count, convert_output_format(fget_string_var(FORMAT_RELM_FSET), "%s %s %s %s %s", t[count].time, t[count].from, t[count].uh, t[count].to, t[count].last_msg));
		}
		return;
	}

	while (args && *args)
	{
		char *comm;
		if (!(comm = next_arg(args, &args)))
			break;
		if (!my_strnicmp(comm, "-list", strlen(comm)))
		{
			for (count = size - 1; count != -1; count--)
			{
#if 0
				if (!count && !t[count].last_msg)
				{
					bitchsay("No such msg #%d for /%s received", count, command);
					break;
				} else if (!t[count].last_msg)
					break;
#endif
				if (!t[count].last_msg)
					continue;
				switch(numargs)
				{
					case 2:
						put_it("%2d %s", count, convert_output_format(form, sform, t[count].time, t[count].to, t[count].last_msg));
						break;
					case 3:
						put_it("%2d %s", count, convert_output_format(form, sform, t[count].time, t[count].from, t[count].last_msg));
						break;
					case 4:
						put_it("%2d %s", count, convert_output_format(form, sform, t[count].time, t[count].from, t[count].to, t[count].last_msg));
						break;
					case 5:
						put_it("%2d %s", count, convert_output_format(form, sform, t[count].time, t[count].from, t[count].uh, t[count].to, t[count].last_msg));
				}
			}
			return;
		}
		else if (!my_strnicmp(comm, "-kick", strlen(comm)))
			what = KICK;
		else if (!my_strnicmp(comm, "-wall", strlen(comm)))
			what = WALL;
		else if (!my_strnicmp(comm, "-wallop", strlen(comm)))
			what = WALLOP;
		else if (!my_strnicmp(comm, "-msg", strlen(comm)))
			what = PRIVMSG;
		else if (!my_strnicmp(comm, "-notice", strlen(comm)))
			what = NOTICE;
		else if (!my_strnicmp(comm, "-topic", strlen(comm)))
			what = TOPIC;
		else if (!my_strnicmp(comm, "-kboot", strlen(comm)))
			what = KBOOT;
		else if (!my_strnicmp(comm, "-kill", strlen(comm)))
			what = KILL;
		else if (!my_strnicmp(comm, "-ansi", strlen(comm)))
			showansi++;
		else if (!my_strnicmp(comm, "-help", strlen(comm)))
		{
			userage(command, helparg/*"<-list|-kick|-wall|-wallop|-msg|-notice|-topic|-kboot|-ansi|-kill|-help> <#|channel|nick> <channel|nick>"*/);
			return;
		}
		else 
		{
			if (is_number(comm))
			{
				count = my_atol(comm);
				if (count < 0)
					count *= -1;
			}
			else
				channel = comm;
		}
		if (count > size)
			count = size;
	}
	if (count == -1)
		count = 0;
	if (!channel)
		channel = get_current_channel_by_refnum(0);
	if (channel || what == WALLOP)
	{
		char *p = NULL;
		if (!t[count].last_msg)
		{
			bitchsay("No such msg #%d for /%s received", count, command);
			return;
		}
		switch(numargs)
		{
			case 2:
				malloc_strcpy(&p, convert_output_format(form, sform, t[count].time, t[count].to, t[count].last_msg));
				break;
			case 3:
				malloc_strcpy(&p, convert_output_format(form, sform, t[count].time, t[count].from, t[count].last_msg));
				break;
			case 4:
				malloc_strcpy(&p, convert_output_format(form, sform, t[count].time, t[count].from, t[count].to, t[count].last_msg));
				break;
			case 5:
				malloc_strcpy(&p, convert_output_format(form, sform, t[count].time, t[count].from, t[count].uh, t[count].to, t[count].last_msg));
		}
		redirect_msg(channel, what, p, showansi);
		new_free(&p);
	}
	else
		bitchsay("Can not %s what you requested", command);
}

/*
 * Wallop   Sends NOTICE to all ops of Current Channel!       
 */
BUILT_IN_COMMAND(ChanWallOp)
{
	char *channel = NULL;
	char *chops = NULL;
	char *include = NULL;
	char *exclude = NULL;
	ChannelList *chan;
	NickList *tmp;
	int ver = 0;
	int enable_all = 0;
	
	char	buffer[BIG_BUFFER_SIZE + 1];
	
	if (!args || (args && !*args))
	{
		userage("WALLMSG", helparg);
		return;
	}
	if (get_current_channel_by_refnum(0))
	{
		int count = 0;
		int i = 0;
		char *nick = NULL;
		malloc_strcpy(&channel, get_current_channel_by_refnum(0));
		chan = lookup_channel(channel, current_window->server, 0);
		if (((ver = get_server_version(current_window->server)) == Server2_8hybrid6))
			enable_all = 1;
		else if (((ver = get_server_version(current_window->server)) < Server_u2_8))
		{
			while (args && (*args == '-' || *args == '+'))
			{
				nick = next_arg(args, &args);
				if (*nick == '-')
				{
					malloc_strcat(&exclude, nick+1);
					malloc_strcat(&exclude, " ");
				} 
				else 
				{
					malloc_strcat(&include, nick+1);
					malloc_strcat(&include, " ");
				}
			}
		}
		if (!args || !*args)
		{
			bitchsay("NO Wallmsg included");
			new_free(&exclude);
			new_free(&include);
			new_free(&channel);
		}
		message_from(channel, LOG_WALL);
		sprintf(buffer, "[\002BX-Wall\002/\002%s\002] %s", channel, args);
		if (ver >= Server_u2_10 || enable_all)
		{
			send_to_server(enable_all?"NOTICE @%s :%s":"WALLCHOPS %s :%s", channel, buffer);
			put_it("%s", convert_output_format(fget_string_var(FORMAT_BWALL_FSET), "%s %s %s %s %s", update_clock(GET_TIME), channel, "*", "*", args));
			add_last_type(&last_sent_wall[0], 1, NULL, NULL, channel, buffer);
			new_free(&channel);
			message_from(NULL, LOG_CRAP);
			return;
		}
		for (tmp = next_nicklist(chan, NULL); tmp; tmp = next_nicklist(chan, tmp))
		{
			if (!my_stricmp(tmp->nick, get_server_nickname(from_server)))
				continue;
			if (exclude && stristr(exclude, tmp->nick))
				continue;
			if (tmp->chanop == 1 || (include && stristr(include, tmp->nick)))
			{
				if (chops)
					malloc_strcat(&chops, ",");
				malloc_strcat(&chops, tmp->nick);
				count++;
			}
			if (count >= 8 && chops)
			{
				send_to_server("%s %s :%s", "NOTICE", chops, buffer);
				i+=count;
				count = 0;
				new_free(&chops);
			}
		}
		i+=count;
		if (chops)
			send_to_server("%s %s :%s", "NOTICE", chops, buffer);
		if (i) 
		{
			put_it("%s", convert_output_format(fget_string_var(FORMAT_BWALL_FSET), "%s %s %s %s %s", update_clock(GET_TIME), channel, "*", "*", args));
			add_last_type(&last_sent_wall[0], 1, NULL, NULL, channel, buffer);
			if (exclude)
				bitchsay("Excluded <%s> from wallmsg", exclude);
			if (include)
				bitchsay("Included <%s> in wallmsg", include);
		}
		else
			put_it("%s", convert_output_format("$G No ops on $0", "%s", channel));
		message_from(NULL, LOG_CRAP);
	}
	else
		say("No Current Channel for this Window.");
	new_free(&include);
	new_free(&channel);
	new_free(&chops);
	new_free(&exclude);
}

void log_toggle(int flag, ChannelList *chan)
{
	char *logfile;

	if (((logfile = get_string_var(MSGLOGFILE_VAR)) == NULL) || !get_string_var(CTOOLZ_DIR_VAR))
	{
		bitchsay("You must set the MSGLOGFILE and CTOOLZ_DIR variables first!");
		set_int_var(MSGLOG_VAR, 0);
		return;
	}
	logmsg(LOG_CURRENT, NULL, flag ? 1 : 2, NULL);
}

void not_on_a_channel(Window *win)
{
	if (win)
		message_to(win->refnum);
	bitchsay("You're not on a channel!");
	message_to(0);
}

int are_you_opped(char *channel)
{
	return is_chanop(channel, get_server_nickname(from_server));
}

void error_not_opped(char *channel)
{
	say("You're not opped on %s", channel);
}

int freadln(FILE *stream, char *lin)
{
	char *p;

	do
		p = fgets(lin, BIG_BUFFER_SIZE/4, stream);
	while (p && (*lin == '#'));

	if (!p)
		return 0;
	chop(lin, 1);
	if (!*lin)
		return 0;
	return 1;
}

char *randreason(char *filename)
{
	int count, min, i;
	FILE *bleah;
	static char buffer[BIG_BUFFER_SIZE/4 + 1];

	min = 1;
	count = 0;

	buffer[0] = '\0';

	if (!filename || !(bleah = fopen(filename, "r")))
		return NULL;

	while (!feof(bleah))
		if (freadln(bleah, buffer))
			count++;
	if (!count)
	{
		strcpy(buffer, "No Reason");
		return buffer;
	}
	fseek(bleah, 0, 0);
	i = getrandom(1, count);
	count = 0;

	while (!feof(bleah) && (count < i))
		if (freadln(bleah, buffer))
			count++;
	fclose(bleah);

	if (*buffer)
		return buffer;
	return NULL;
}

char *get_reason(char *nick, char *file)
{
	char *temp, *p;
	char *filename = NULL;
	if (file && *file)
		malloc_sprintf(&filename, "%s", file);
	else
#if defined(WINNT) || defined(__EMX__)
		malloc_sprintf(&filename, "%s/%s.%s", get_string_var(CTOOLZ_DIR_VAR), version, "kck");
#else
		malloc_sprintf(&filename, "%s/%s.%s", get_string_var(CTOOLZ_DIR_VAR), version, "reasons");
#endif
	p = expand_twiddle(filename);
	temp = randreason(p);
	new_free(&filename);
	new_free(&p);
	if ((!temp || !*temp) && get_string_var(DEFAULT_REASON_VAR))
		temp = get_string_var(DEFAULT_REASON_VAR);
	return (stripansicodes(convert_output_format(temp, "%s %s", nick? nick: "error", get_server_nickname(from_server) )));
}

char *get_realname(char *nick)
{
	char *temp, *p;
	char *filename = NULL;

#if defined(WINNT) || defined(__EMX__)
	malloc_sprintf(&filename, "%s/%s.%s", get_string_var(CTOOLZ_DIR_VAR), version, "nam");
#else
	malloc_sprintf(&filename, "%s/%s.%s", get_string_var(CTOOLZ_DIR_VAR), version, "ircnames");
#endif
	p = expand_twiddle(filename);
	temp = randreason(p);
	new_free(&filename);
	new_free(&p);
	if ((!temp || !*temp))
		temp = "Who cares?";
	return (stripansicodes(convert_output_format(temp, "%s %s", nick? nick: "error", get_server_nickname(from_server) )));
}

char *get_signoffreason(char *nick)
{
	char *temp, *p;
	char *filename = NULL;

#if defined(WINNT) || defined(__EMX__)
	malloc_sprintf(&filename, "%s/%s.%s", get_string_var(CTOOLZ_DIR_VAR), version, "qt");
#else
	malloc_sprintf(&filename, "%s/%s.%s", get_string_var(CTOOLZ_DIR_VAR), version, "quit");
#endif
	p = expand_twiddle(filename);
	temp = randreason(p);
	new_free(&filename);
	new_free(&p);
	if (!temp || !*temp)
		temp = "$0 has no reason";

	return (stripansicodes(convert_output_format(temp, "%s %s", nick? nick: "error", get_server_nickname(from_server))));
}


#ifndef WINNT

/*
 * arlib.c (C)opyright 1993 Darren Reed. All rights reserved.
 * This file may not be distributed without the author's permission in any
 * shape or form. The author takes no responsibility for any damage or loss
 * of property which results from the use of this software.
 * heavily modified for use in a irc client.
 */
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/nameser.h>
#include <resolv.h>

#define HOSTLEN 100
extern	int	h_errno;
static	char	ar_hostbuf[HOSTLEN+1], ar_domainname[HOSTLEN+1];
static	char	ar_dot[] = ".";
static	int	ar_resfd = -1, ar_vc = 0;
static	struct	reslist	*ar_last = NULL, *ar_first = NULL;


static	int	do_query_name(struct resinfo *, char *, register struct reslist *, char *, char *, char *, char *, int , void (*func)());
static	int	do_query_number(struct resinfo *, char *, register struct reslist *, char *, char *, char *, char *, int , void (*func)());
static	int	ar_resend_query(struct reslist *);

#define MAX_RETRIES 4

#ifndef HFIXEDSZ
#define HFIXEDSZ 12
#endif

#ifndef INT32SZ
#define INT32SZ 4
#endif


/*
 * Statistics structure.
 */
static	struct	resstats {
	int	re_errors;
	int	re_nu_look;
	int	re_na_look;
	int	re_replies;
	int	re_requests;
	int	re_resends;
	int	re_sent;
	int	re_timeouts;
} ar_reinfo;

/*
 * ar_init
 *
 * Initializes the various ARLIB internal varilables and related DNS
 * options for res_init().
 *
 * Returns 0 or the socket opened for use with talking to name servers
 * if 0 is passed or ARES_INITSOCK is set.
 */
int	ar_init(int op)
{
	int	ret = 0;

	if (op & ARES_INITLIST)
	{
		memset(&ar_reinfo, 0, sizeof(ar_reinfo));
		ar_first = ar_last = NULL;
	}

	if (op & ARES_CALLINIT && !(_res.options & RES_INIT))
	{
		ret = res_init();
		(void)strcpy(ar_domainname, ar_dot);
		(void)strncat(ar_domainname, _res.defdname, HOSTLEN-2);
		if (!_res.nscount)
		{
			_res.nscount = 1;
			_res.nsaddr_list[0].sin_addr.s_addr = inet_addr("127.0.0.1");
		}
	}

	if (op & ARES_INITSOCK)
		ret = ar_resfd = ar_open();

	if (op & ARES_INITDEBG)
		_res.options |= RES_DEBUG;

	if (op == 0)
		ret = ar_resfd;

	return ret;
}

/*
 * ar_open
 *
 * Open a socket to talk to a name server with.
 * Check _res.options to see if we use a TCP or UDP socket.
 */
int	ar_open(void)
{
	if (ar_resfd == -1)
	{
		if (_res.options & RES_USEVC)
		{
			struct	sockaddr_in *sip;
			int	i = 0;

			sip = _res.nsaddr_list;
			ar_vc = 1;
			ar_resfd = socket(AF_INET, SOCK_STREAM, 0);

			/*
			 * Try each name server listed in sequence until we
			 * succeed or run out.
			 */
			while (connect(ar_resfd, (struct sockaddr *)sip++,
					sizeof(struct sockaddr)))
			{
				(void)close(ar_resfd);
				ar_resfd = -1;
				if (i >= _res.nscount)
					break;
				ar_resfd = socket(AF_INET, SOCK_STREAM, 0);
			}
		}
		else
		{
			int on = 0;
			ar_resfd = socket(AF_INET, SOCK_DGRAM, 0);
			(void) setsockopt(ar_resfd, SOL_SOCKET, SO_BROADCAST,(char *)&on, sizeof(on));
		}
	}
	if (ar_resfd >= 0)
	{	/* Need one of these two here - and it MUST work!! */
		if (set_non_blocking(ar_resfd) < 0)
		{
			(void)close(ar_resfd);
			ar_resfd = -1;
		}
	}
	return ar_resfd;
}

/*
 * ar_close
 *
 * Closes and flags the ARLIB socket as closed.
 */
void	ar_close(void)
{
	(void)close(ar_resfd);
	ar_resfd = -1;
	return;
}

/*
 * ar_add_request
 *
 * Add a new DNS query to the end of the query list.
 */
static	int	ar_add_request(struct reslist *new)
{
	if (!new)
		return -1;
	if (!ar_first)
		ar_first = ar_last = new;
	else 
	{
		ar_last->re_next = new;
		ar_last = new;
	}
	new->re_next = NULL;
	ar_reinfo.re_requests++;
	return 0;
}

/*
 * ar_remrequest
 *
 * Remove a request from the list. This must also free any memory that has
 * been allocated for temporary storage of DNS results.
 *
 * Returns -1 if there are anyy problems removing the requested structure
 * or 0 if the remove is successful.
 */
static	int	ar_remrequest(struct reslist *old)
{
	struct	reslist	*rptr, *r2ptr;
	register char	**s;

	if (!old)
		return -1;
	for (rptr = ar_first, r2ptr = NULL; rptr; rptr = rptr->re_next)
	{
		if (rptr == old)
			break;
		r2ptr = rptr;
	}

	if (!rptr)
		return -1;
	if (rptr == ar_first)
		ar_first = ar_first->re_next;
	else if (rptr == ar_last)
	{
		if ((ar_last = r2ptr))
			ar_last->re_next = NULL;
	}
	else
		r2ptr->re_next = rptr->re_next;

	if (!ar_first)
		ar_last = ar_first;
	if (rptr->re_he.h_name)
		new_free(&rptr->re_he.h_name);
	if ((s = rptr->re_he.h_aliases))
	{
		for (; *s; s++)
			new_free(s);
		new_free(&rptr->re_he.h_aliases);
	}
#if 0
	if ((s = rptr->re_he.h_addr_list))
	{
		/* CDE memleak detected here */
		for (; *s; s++)
			new_free(s);
		new_free(&rptr->re_he.h_addr_list);
	}
#endif
	if (rptr->re_rinfo.ri_ptr)
		new_free(&rptr->re_rinfo.ri_ptr);

	new_free(&rptr->nick);
	new_free(&rptr->user);
	new_free(&rptr->host);
	new_free(&rptr->channel);
	new_free(&rptr);

	return 0;
}

/*
 * ar_make_request
 *
 * Create a DNS query recorded for the request being made and place it on the
 * current list awaiting replies.  Initialization of the record with set
 * values should also be done.
 */
static	struct	reslist	*ar_make_request(register struct resinfo *resi, char *nick, char *user, char *h, char *chan, int server, void (*func)())
{
	register struct	reslist	*rptr;
	register struct resinfo *rp;

	rptr = (struct reslist *)new_malloc(sizeof(struct reslist));
	rp = &rptr->re_rinfo;

	rptr->re_next    = NULL; /* where NULL is non-zero ;) */
	rptr->re_sentat  = now;
	rptr->re_retries = MAX_RETRIES;/*_res.retry + 2;*/
	rptr->re_sends = 1;
	rptr->re_resend  = 1;
	rptr->re_timeout = rptr->re_sentat + _res.retrans;
	rptr->re_he.h_name = NULL;
	rptr->re_he.h_addrtype   = AF_INET;
	rptr->re_he.h_aliases[0] = NULL;
	rp->ri_ptr = resi->ri_ptr;
	rp->ri_size = resi->ri_size;
	if (nick)
		malloc_strcpy(&rptr->nick, nick);
	if (user)
		malloc_strcpy(&rptr->user, user);
	if (h)
		malloc_strcpy(&rptr->host, h);
	if (chan)
		rptr->channel = m_strdup(chan);
	rptr->server = server;
	if (func)
		rptr->func = func;
	(void)ar_add_request(rptr);

	return rptr;
}

void ar_rename_nick(char *old_nick, char *new_nick, int server)
{
	register struct reslist *rptr;
	for (rptr = ar_first; rptr; rptr = rptr->re_next)
	{
		if (!rptr->nick) continue;
		if (!strcmp(rptr->nick, old_nick) && rptr->server == server)
			malloc_strcpy(&rptr->nick, new_nick);
	}
}

/*
 * ar_timeout
 *
 * Remove queries from the list which have been there too long without
 * being resolved.
 */
long	ar_timeout(time_t now, char *info, int size, void (*func)(struct reslist *) )
{
	register struct	reslist	*rptr, *r2ptr;
	register long	next = 0;

	for (rptr = ar_first, r2ptr = NULL; rptr; rptr = r2ptr)
	{
		r2ptr = rptr->re_next;
		if (now >= rptr->re_timeout)
		{
			/*
			 * If the timeout for the query has been exceeded,
			 * then resend the query if we still have some
			 * 'retry credit' and reset the timeout. If we have
			 * used it all up, then remove the request.
			 */
			if (--rptr->re_retries <= 0)
			{
				ar_reinfo.re_timeouts++;
				if (info && rptr->re_rinfo.ri_ptr)
					bcopy(rptr->re_rinfo.ri_ptr, info,
						MIN(rptr->re_rinfo.ri_size, size));
				if (rptr->func)
					(*rptr->func)(rptr);
				else if (func)
					(*func)(rptr);
				(void)ar_remrequest(rptr);
				return now;
			}
			else
			{
				rptr->re_sends++;
				rptr->re_sentat = now;
				rptr->re_timeout = now + _res.retrans;
				(void)ar_resend_query(rptr);
			}
		}
		if (!next || rptr->re_timeout < next)
			next = rptr->re_timeout;
	}
	return next;
}

/*
 * ar_send_res_msg
 *
 * When sending queries to nameservers listed in the resolv.conf file,
 * don't send a query to every one, but increase the number sent linearly
 * to match the number of resends. This increase only occurs if there are
 * multiple nameserver entries in the resolv.conf file.
 * The return value is the number of messages successfully sent to 
 * nameservers or -1 if no successful sends.
 */
static	int	ar_send_res_msg(char *msg, int len, int rcount)
{
	register int	i;
	int	sent = 0;

	if (!msg)
		return -1;

	rcount = (_res.nscount > rcount) ? rcount : _res.nscount;
	if (_res.options & RES_PRIMARY)
		rcount = 1;
	if (!rcount)
		rcount = 1;
	if (ar_vc)
	    {
		ar_reinfo.re_sent++;
		sent++;
		if (write(ar_resfd, msg, len) == -1)
		    {
			int errtmp = errno;
			(void)close(ar_resfd);
			errno = errtmp;
			ar_resfd = -1;
		    }
	    }
	else
		for (i = 0; i < rcount; i++)
		{
			_res.nsaddr_list[i].sin_family = AF_INET;
			if (sendto(ar_resfd, msg, len, 0,
				   (struct sockaddr *)&(_res.nsaddr_list[i]),
				sizeof(struct sockaddr_in)) == len)
			{
				ar_reinfo.re_sent++;
				sent++;
			}
		}
	return (sent) ? sent : -1;
}

/*
 * ar_find_id
 *
 * find a dns query record by the id (id is determined by dn_mkquery)
 */
static	struct	reslist	*ar_find_id(int id)
{
	register struct	reslist	*rptr;

	for (rptr = ar_first; rptr; rptr = rptr->re_next)
		if (rptr->re_id == id)
			return rptr;
	return NULL;
}

/*
 * ar_delete
 *
 * Delete a request from the waiting list if it has a data pointer which
 * matches the one passed.
 */
int	ar_delete(char *ptr, int size)
{
	register struct	reslist	*rptr;
	register struct	reslist	*r2ptr;
	int	removed = 0;

	for (rptr = ar_first; rptr; rptr = r2ptr)
	    {
		r2ptr = rptr->re_next;
		if (rptr->re_rinfo.ri_ptr && ptr && size &&
		    memcmp(rptr->re_rinfo.ri_ptr, ptr, size) == 0)
		    {
			(void)ar_remrequest(rptr);
			removed++;
		    }
	    }
	return removed;
}

/*
 * ar_query_name
 *
 * generate a query based on class, type and name.
 */
static	int	ar_query_name(char *name, int class, int type, struct reslist *rptr)
{
	static	char buf[MAXPACKET];
	int	r,s;
	HEADER	*hptr;

	memset(buf, 0, sizeof(buf));
	r = res_mkquery(QUERY, name, class, type, NULL, 0, NULL,
			buf, sizeof(buf));
	if (r <= 0)
	    {
		h_errno = NO_RECOVERY;
		return r;
	    }
	hptr = (HEADER *)buf;
	rptr->re_id = ntohs(hptr->id);

	s = ar_send_res_msg(buf, r, rptr->re_sends);

	if (s == -1)
	    {
		h_errno = TRY_AGAIN;
		return -1;
	    }
	else
		rptr->re_sent += s;
	return 0;
}

/*
 * ar_gethostbyname
 *
 * Replacement library function call to gethostbyname().  This one, however,
 * doesn't return the record being looked up but just places the query in the
 * queue to await answers.
 */
int	ar_gethostbyname(char *name, char *info, int size, char *nick, char *user, char *h, char *chan, int server, void (*func)())
{
	char	host[HOSTLEN+1];
	struct	resinfo	resi;
	register struct resinfo *rp = &resi;

	if (size && info)
	{
		rp->ri_ptr = (char *)new_malloc(size+1);
		if (*info)
			bcopy(info, rp->ri_ptr, size);
		rp->ri_size = size;
	}
	else
		memset((char *)rp, 0, sizeof(resi));
	ar_reinfo.re_na_look++;
	(void)strncpy(host, name, 64);
	host[64] = '\0';

	return (do_query_name(rp, host, NULL, nick, user, h, chan, server, func));
}

static	int	do_query_name(struct resinfo *resi, char *name, register struct reslist *rptr, char *nick, char *user, char *h, char *chan, int server, void (*func)())
{
	char	hname[HOSTLEN];
	int	len;

	len = strlen((char *)strncpy(hname, name, sizeof(hname)-1));

	if (rptr && (hname[len-1] != '.'))
	{
		(void)strncat(hname, ar_dot, sizeof(hname)-len-1);
		/*
		 * NOTE: The logical relationship between DNSRCH and DEFNAMES
		 * is implies. ie no DEFNAES, no DNSRCH.
		 */
		if ((_res.options & (RES_DEFNAMES|RES_DNSRCH)) ==
		    (RES_DEFNAMES|RES_DNSRCH))
		{
			if (_res.dnsrch[(int)rptr->re_srch])
				(void)strncat(hname, _res.dnsrch[(int)rptr->re_srch],
					sizeof(hname) - ++len -1);
		}
		else if (_res.options & RES_DEFNAMES)
			(void)strncat(hname, ar_domainname, sizeof(hname) - len -1);
	}

	/*
	 * Store the name passed as the one to lookup and generate other host
	 * names to pass onto the nameserver(s) for lookups.
	 */
	if (!rptr)
	{
		rptr = ar_make_request(resi, nick, user, h, chan, server, func);
		rptr->re_type = T_A;
		(void)strncpy(rptr->re_name, name, sizeof(rptr->re_name)-1);
	}
	return (ar_query_name(hname, C_IN, T_A, rptr));
}

/*
 * ar_gethostbyaddr
 *
 * Generates a query for a given IP address.
 */
int	ar_gethostbyaddr(char *addr, char *info, int size, char *nick, char *user, char *h, char *chan, int server, void (*func)())
{
	struct	resinfo	resi;
	register struct resinfo *rp = &resi;

	if (size && info)
	{
		rp->ri_ptr = (char *)new_malloc(size+1);
		if (*info)
			bcopy(info, rp->ri_ptr, size);
		rp->ri_size = size;
	}
	else
		memset((char *)rp, 0, sizeof(resi));
	ar_reinfo.re_nu_look++;
	return (do_query_number(rp, addr, NULL, nick, user, h, chan, server, func));
}

/*
 * do_query_number
 *
 * Use this to do reverse IP# lookups.
 */
static	int	do_query_number(struct resinfo *resi, char *numb, register struct reslist *rptr, char *nick, char *user, char *h, char *chan, int server, void (*func)())
{
	register unsigned char	*cp;
	static	char	ipbuf[32];

	/*
	 * Generate name in the "in-addr.arpa" domain.  No addings bits to this
	 * name to get more names to query!.
	 */
	cp = (unsigned char *)numb;
	(void)sprintf(ipbuf,"%u.%u.%u.%u.in-addr.arpa.",
			(unsigned int)(cp[3]), (unsigned int)(cp[2]),
			(unsigned int)(cp[1]), (unsigned int)(cp[0]));

	if (!rptr)
	    {
		rptr = ar_make_request(resi, nick, user, h, chan, server, func);
		rptr->re_type = T_PTR;
		rptr->re_he.h_length = sizeof(struct in_addr);
		memcpy((char *)&rptr->re_addr, numb, rptr->re_he.h_length);
		memcpy((char *)&rptr->re_he.h_addr_list[0].s_addr, numb,
			rptr->re_he.h_length);
	    }
	return (ar_query_name(ipbuf, C_IN, T_PTR, rptr));
}

/*
 * ar_resent_query
 *
 * resends a query.
 */
static	int	ar_resend_query(struct reslist *rptr)
{
	if (!rptr->re_resend)
		return -1;

	switch(rptr->re_type)
	{
	case T_PTR:
		ar_reinfo.re_resends++;
		return do_query_number(NULL, (char *)&rptr->re_addr, rptr, NULL, NULL, NULL, NULL, -1, NULL);
	case T_A:
		ar_reinfo.re_resends++;
		return do_query_name(NULL, rptr->re_name, rptr, NULL, NULL, NULL, NULL, -1, NULL);
	default:
		break;
	}

	return -1;
}

/*
 * ar_procanswer
 *
 * process an answer received from a nameserver.
 */
static	int	ar_procanswer(struct reslist *rptr, HEADER *hptr, char *buf, char *eob)
{
	char	*cp, **alias;
	int	class, type, dlen, len, ans = 0, n;
	unsigned int ttl, dr, *adr;
	struct	hent	*hp;

	cp = buf + HFIXEDSZ;
	adr = (unsigned int *)rptr->re_he.h_addr_list;

	while (*adr)
		adr++;

	alias = rptr->re_he.h_aliases;
	while (*alias)
		alias++;

	hp = &rptr->re_he;


	/*
	 * Skip over the original question.
	 */
	while (hptr->qdcount-- > 0)
		cp += dn_skipname(cp, eob) + QFIXEDSZ;
	/*
	 * proccess each answer sent to us. blech.
	 */
	while (hptr->ancount-- > 0 && cp < eob) {
		n = dn_expand(buf, eob, cp, ar_hostbuf, sizeof(ar_hostbuf)-1);
		cp += n;
		if (n <= 0)
			return ans;

		ans++;
		/*
		 * 'skip' past the general dns crap (ttl, class, etc) to get
		 * the pointer to the right spot.  Some of thse are actually
		 * useful so its not a good idea to skip past in one big jump.
		 */
		type = (int)_getshort(cp);
		cp += sizeof(short);
		class = (int)_getshort(cp);
		cp += sizeof(short);
		ttl = (unsigned int)_getlong(cp);
		cp += INT32SZ;
		dlen =  (int)_getshort(cp);
		cp += sizeof(short);
		rptr->re_type = type;

		switch(type)
		{
		case T_A :
			rptr->re_he.h_length = dlen;
			if (ans == 1)
				rptr->re_he.h_addrtype=(class == C_IN) ?
							AF_INET : AF_UNSPEC;
			memcpy(&dr, cp, dlen);
			*adr++ = dr;
			*adr = 0;
			cp += dlen;
			len = strlen(ar_hostbuf);
			if (!rptr->re_he.h_name)
				malloc_strcpy(&rptr->re_he.h_name, ar_hostbuf);
 			break;
		case T_PTR :
			if ((n = dn_expand(buf, eob, cp, ar_hostbuf,
					   sizeof(ar_hostbuf)-1 )) < 0)
			    {
				cp += n;
				continue;
			    }
			ar_hostbuf[HOSTLEN] = 0;
			cp += n;
			len = strlen(ar_hostbuf)+1;
			/*
			 * copy the returned hostname into the host name
			 * or alias field if there is a known hostname
			 * already.
			 */
			if (!rptr->re_he.h_name)
				malloc_strcpy(&rptr->re_he.h_name, ar_hostbuf);
			else
			{
				*alias = (char *)new_malloc(len);
				strcpy(*alias++, ar_hostbuf);
				*alias = NULL;
			}
			break;
		case T_CNAME :
			cp += dlen;
			if (alias >= &(rptr->re_he.h_aliases[MAXALIASES-1]))
				continue;
			n = strlen(ar_hostbuf)+1;
			*alias = (char *)new_malloc(n);
			(void)strcpy(*alias++, ar_hostbuf);
			*alias = NULL;
			break;
		default :
			break;
		}
	}

	return ans;
}

/*
 * ar_answer
 *
 * Get an answer from a DNS server and process it.  If a query is found to
 * which no answer has been given to yet, copy its 'info' structure back
 * to where "reip" points and return a pointer to the hostent structure.
 */
struct	hostent	*ar_answer(char *reip, int size, void (*func)(struct reslist *) )
{
	static	char	ar_rcvbuf[HFIXEDSZ + MAXPACKET];
	static	struct	hostent	ar_host;

	register HEADER	*hptr;
	register struct	reslist	*rptr = NULL;
	register struct hostent *hp;
	register char **s;
	unsigned long	*adr;
	int	rc, i, n, a;

	rc = recv(ar_resfd, ar_rcvbuf, sizeof(ar_rcvbuf), 0);
	if (rc <= 0)
		goto getres_err;

	ar_reinfo.re_replies++;
	hptr = (HEADER *)ar_rcvbuf;
	/*
	 * convert things to be in the right order.
	 */
	hptr->id = ntohs(hptr->id);
	hptr->ancount = ntohs(hptr->ancount);
	hptr->arcount = ntohs(hptr->arcount);
	hptr->nscount = ntohs(hptr->nscount);
	hptr->qdcount = ntohs(hptr->qdcount);
	/*
	 * response for an id which we have already received an answer for
	 * just ignore this response.
	 */
	rptr = ar_find_id(hptr->id);
	if (!rptr)
		goto getres_err;

	if ((hptr->rcode != NOERROR) || (hptr->ancount == 0))
	    {
		switch (hptr->rcode)
		{
		case NXDOMAIN:
			h_errno = HOST_NOT_FOUND;
			break;
		case SERVFAIL:
			h_errno = TRY_AGAIN;
			break;
		case NOERROR:
			h_errno = NO_DATA;
			break;
		case FORMERR:
		case NOTIMP:
		case REFUSED:
		default:
			h_errno = NO_RECOVERY;
			break;
		}
		ar_reinfo.re_errors++;
		/*
		** If a bad error was returned, we stop here and dont send
		** send any more (no retries granted).
		*/
		if (h_errno != TRY_AGAIN)
		    {
			rptr->re_resend = 0;
			rptr->re_retries = 0;
		    }
		goto getres_err;
	    }

	a = ar_procanswer(rptr, hptr, ar_rcvbuf, ar_rcvbuf+rc);

	if ((rptr->re_type == T_PTR) && (_res.options & RES_CHECKPTR))
	    {
		/*
		 * For reverse lookups on IP#'s, lookup the name that is given
		 * for the ip# and return with that as the official result.
		 * -avalon
		 */
		rptr->re_type = T_A;
		/*
		 * Clean out the list of addresses already set, even though
		 * there should only be one :)
		 */
		adr = (unsigned long *)rptr->re_he.h_addr_list;
		while (*adr)
			*adr++ = 0L;
		/*
		 * Lookup the name that we were given for the ip#
		 */
		ar_reinfo.re_na_look++;
		(void)strncpy(rptr->re_name, rptr->re_he.h_name,
			sizeof(rptr->re_name)-1);
		rptr->re_he.h_name = NULL;
		rptr->re_retries = MAX_RETRIES;/*_res.retry + 2;*/
		rptr->re_sends = 1;
		rptr->re_resend = 1;
		if (rptr->re_he.h_name)
			new_free(&rptr->re_he.h_name);
		ar_reinfo.re_na_look++;
		(void)ar_query_name(rptr->re_name, C_IN, T_A, rptr);
		return NULL;
	    }

	if (reip && rptr->re_rinfo.ri_ptr && size)
		memcpy(reip, rptr->re_rinfo.ri_ptr,
			MIN(rptr->re_rinfo.ri_size, size));

	/*
	 * Clean up structure from previous usage.
	 */
	hp = &ar_host;
	if (hp->h_name)
		new_free(&hp->h_name);
	if ((s = hp->h_aliases))
	{
		while (*s)
		{
			new_free(s);
			s++;
		}
		new_free(&hp->h_aliases);
	}
	if ((s = hp->h_addr_list))
	{
	
		while (*s)
		{
			new_free(s);
			s++;
		}
		new_free(&hp->h_addr_list);
		hp->h_addr_list = NULL;
	}
	memset ((char *)hp, 0, sizeof(*hp));
	/*
	 * Setup and copy details for the structure we return a pointer to.
	 */
	hp->h_addrtype = AF_INET;
	hp->h_length = sizeof(struct in_addr);
	malloc_strcpy(&hp->h_name, rptr->re_he.h_name);
	/*
	 * Count IP#'s.
	 */
	for (i = 0, n = 0; i < MAXADDRS; i++, n++)
		if (!rptr->re_he.h_addr_list[i].s_addr)
			break;
	s = hp->h_addr_list = (char **)new_malloc((n + 1) * sizeof(char *));
	if (n)
	{
		*s = (char *)new_malloc(sizeof(struct in_addr));
		memcpy(*s, (char *)&rptr->re_he.h_addr_list[0].s_addr,
			sizeof(struct in_addr));
		s++;
		for (i = 1; i < n; i++, s++)
		{
			*s = (char *)new_malloc(sizeof(struct in_addr));
			memcpy(*s, (char *)&rptr->re_he.h_addr_list[i].s_addr,
				sizeof(struct in_addr));
		}
	}
	*s = NULL;
	/*
	 * Count CNAMEs
	 */
	for (i = 0, n = 0; i < MAXADDRS; i++, n++)
		if (!rptr->re_he.h_aliases[i])
			break;
	s = hp->h_aliases = (char **)new_malloc((n + 1) * sizeof(char *));
	for (i = 0; i < n; i++)
	{
		*s++ = rptr->re_he.h_aliases[i];
		rptr->re_he.h_aliases[i] = NULL;
	}
	*s = NULL;
	if (rptr->func)
		(*rptr->func)(rptr);
	else if (func)
		(*func)(rptr);
	if (a > 0)
		(void)ar_remrequest(rptr);
	else
		if (!rptr->re_sent)
			(void)ar_remrequest(rptr);
	return hp;

getres_err:
	if (rptr)
	{
		if (reip && rptr->re_rinfo.ri_ptr && size)
			memcpy(reip, rptr->re_rinfo.ri_ptr,
				MIN(rptr->re_rinfo.ri_size, size));
		if ((h_errno != TRY_AGAIN) &&
		    ((_res.options & (RES_DNSRCH|RES_DEFNAMES)) ==
		     (RES_DNSRCH|RES_DEFNAMES) ))
			if (_res.dnsrch[(int)rptr->re_srch])
			{
				rptr->re_retries = MAX_RETRIES; /*_res.retry + 2;*/
				rptr->re_sends = 1;
				rptr->re_resend = 1;
				(void)ar_resend_query(rptr);
				rptr->re_srch++;
			}
		return NULL;
	}
	return NULL;
}

static int ar_seq = 0;
static int ar_lookup = 0;

void auto_nslookup(struct reslist *rptr)
{
NickList *nick;
ChannelList *chan;
struct in_addr ip;
	if ((chan = lookup_channel(rptr->channel, rptr->server, 0)))
	{
		if ((nick = find_nicklist_in_channellist(rptr->nick, chan, 0)))
		{
			if (rptr->re_he.h_addr_list[0].s_addr)
			{
				bcopy(&rptr->re_he.h_addr_list[0], (char *)&ip, sizeof(ip));
				nick->ip = m_strdup(inet_ntoa(ip));
				check_auto(chan->channel, nick, chan);
			}
			else if (++nick->ip_count < 2)
				do_nslookup(rptr->host, rptr->nick, rptr->user, rptr->channel, rptr->server, auto_nslookup);
#ifdef PANA_DEBUG
			else
				put_it("got here. nslookup failure %s!%s", nick->nick, nick->host);
#endif
		}	
	}
	return;
}

void print_ns_succede(struct reslist *rptr)
{
char *u, *n, *h;
	u = rptr->user ? rptr->user : empty_string;
	h = rptr->host;
	n = rptr->nick ? rptr->nick : empty_string;
	if (do_hook(NSLOOKUP_LIST, "%s %s %s %s %s", h, rptr->re_he.h_name?rptr->re_he.h_name:"", (char *)inet_ntoa(rptr->re_he.h_addr_list[0]), n, u))
	{
		int i;
		char buffer[BIG_BUFFER_SIZE];
		struct in_addr ip;
		*buffer = 0;
		if (rptr->nick && rptr->user)
			bitchsay("[%s!%s@%s]: %s", n, u, h, rptr->re_he.h_name ? rptr->re_he.h_name: "invalid hostname");
		else
			bitchsay("%s is %s (%s)", h, rptr->re_he.h_name ? rptr->re_he.h_name:"invalid hostname", (char *)inet_ntoa(rptr->re_he.h_addr_list[0]));
		for (i = 0; rptr->re_he.h_addr_list[i].s_addr; i++)
		{
			bcopy(&rptr->re_he.h_addr_list[i], (char *)&ip, sizeof(ip));
			strpcat(buffer, "[%s] ", inet_ntoa(ip));
			if (strlen(buffer) > 490)
				break;
		}
		bitchsay("IPs: %s", buffer);
	        for (i = 0; rptr->re_he.h_aliases[i]; i++)
			bitchsay("\talias %d = %s", i+1, rptr->re_he.h_aliases[i]);
	}

}

void print_ns_fail(struct reslist *rptr)
{
	if (do_hook(NSLOOKUP_LIST, "%s %s %s", rptr->host, rptr->nick?rptr->nick:empty_string, rptr->user?rptr->user:empty_string))
	{
		if (rptr->nick && rptr->user)
			bitchsay("nslookup of %s!%s@%s failed.", rptr->nick, rptr->user, rptr->host);
		else
			bitchsay("nslookup of host %s failed.", rptr->host);
	}
}

void set_nslookupfd(fd_set *rd)
{
int s;
	if ((s = ar_init(0)) != -1)
		FD_SET(s, rd);
}

long print_nslookup(fd_set *rd)
{
struct hostent *hp = NULL;
int ar_del = 0;
int s;
	if ((s = ar_init(0)) == -1)
		return -1;
	if (!(FD_ISSET(s, rd)))
	{
		unsigned long when = 0;
		when = ar_timeout(now, (char *)&ar_del, sizeof(ar_del), print_ns_fail);
		if (ar_del)
			ar_lookup--;
		return when;
	}
	if ((hp = ar_answer((char *)&ar_del, sizeof(ar_del), print_ns_succede)))
	{
		char **s;
		ar_lookup--;
		new_free(&hp->h_name);
		if ((s = hp->h_aliases))
		{
			while (*s)
			{
				new_free(s);
				s++;
			}
			new_free(&hp->h_aliases);
		}
		if ((s = hp->h_addr_list))
		{
			while (*s)
			{
				new_free(s);
				s++;
			}
			new_free(&hp->h_addr_list);
		}
	}
	return -1;
}
#else
long print_nslookup(fd_set *rd)
{
	return -1;
}
void set_nslookupfd(fd_set *rd)
{
}

void auto_nslookup(struct reslist *res)
{
}
#endif

void ns_init(void)
{
#ifndef WINNT
	ar_init(ARES_INITLIST|ARES_CALLINIT|ARES_INITSOCK);
#endif
}

char *do_nslookup(char *host, char *nick, char *user, char *chan, int server, void (*func)())
{
#ifndef WINNT
struct in_addr temp1;
int s;

	if (!host)
		return NULL;

	if ((s = ar_init(0)) == -1)
		ar_init(ARES_INITLIST|ARES_INITSOCK|ARES_CALLINIT);

	ar_lookup++;
	if (isdigit(*(host + strlen(host) - 1)))
	{
		ar_seq++;
		temp1.s_addr = inet_addr(host);
		ar_gethostbyaddr((char *)&temp1.s_addr, (char *)&ar_seq, sizeof(ar_seq), nick, user, host, chan, server, func);
	}
	else
	{
		ar_seq++;
		ar_gethostbyname(host, (char *)&ar_seq, sizeof(ar_seq), nick, user, host, chan, server, func);
	}
#endif
	return NULL;
}

#ifndef WINNT
void userhost_nsl(UserhostItem *stuff, char *nick, char *args)
{
	char *nsl;

	if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick))
	{
		say("No information for %s", nick);
		return;
	}
	nsl = do_nslookup(stuff->host, stuff->nick, stuff->user, NULL, from_server, NULL);
}
#endif

BUILT_IN_COMMAND(nslookup)
{
#ifndef WINNT
	char *host, *hostname;

	if ((host = next_arg(args, &args)))
	{
		bitchsay("Checking tables...");
		if (!strchr(host, '.'))
			userhostbase(host, userhost_nsl, 1, "%s", host);
		else
			hostname = do_nslookup(host, NULL, NULL, NULL, from_server, NULL);
	}
	else
		userage(command, helparg);
#endif
}


char *rights(string, num)
char *string;
int num;
{
	if (strlen(string) < num)
		return string;
	return (string + strlen(string) - num);
}

int numchar(char *string, char c)
{
	int num = 0;

	while (*string)
	{
		if (tolower(*string) == tolower(c))
			num++;
		string++;
	}
	return num;
}

char *cluster (char *hostname)
{
	static char result[BIG_BUFFER_SIZE/4 + 1];
	char temphost[BIG_BUFFER_SIZE + 1];
	char *host;

	if (!hostname)
		return NULL;
	host = temphost;
	*result = 0;
	memset(result, 0, sizeof(result));
	memset(temphost, 0, sizeof(temphost));
	if (strchr(hostname, '@'))
	{
		if (*hostname == '~')
			hostname++;
		strcpy(result, hostname);
		*strchr(result, '@') = '\0';
		if (strlen(result) > 9)
		{
			result[8] = '*';
			result[9] = '\0';
		}
		strcat(result, "@");
		if (!(hostname = strchr(hostname, '@')))
			return NULL;
		hostname++;
	}
	strcpy(host, hostname);

	if (*host && isdigit(*(host + strlen(host) - 1)))
	{
		/* Thanks icebreak for this small patch which fixes this function */
                int i;
                char *tmp;
                char count=0;

                tmp = host;
                while((tmp-host)<strlen(host))
                {
	                if((tmp=strchr(tmp,'.'))==NULL) 
				break;
        	        count++;
                	tmp++;
                }
                tmp = host;
                for (i = 0; i < count; i++)
                        tmp = strchr(tmp, '.') + 1;
                *tmp = '\0';
                strcat(result, host);
                strcat(result, "*");
	}
	else
	{
		char *tmp;
		int num;

		num = 1;
		tmp = rights(host, 3);
		if (my_stricmp(tmp, "com") &&
		    my_stricmp(tmp, "edu") &&
		    my_stricmp(tmp, "net") &&
		    (stristr(host, "com") ||
		     stristr(host, "edu")))
			num = 2;
		while (host && *host && (numchar(host, '.') > num))
		{
			if ((host = strchr(host, '.')) != NULL)
				host++;
			else
				return (char *) NULL;
		}
		strcat(result, "*");
		if (my_stricmp(host, temphost))
			strcat(result, ".");
		strcat(result, host);
	}
	return result;
}












struct _sock_manager
{
	int init;
	int count;
	int max_fd;
	SocketList sockets[FD_SETSIZE];
} sock_manager =  {0, 0, -1, {{ 0, 0, 0, NULL, 0, 0, NULL, NULL, NULL }}};

#define SOCKET_TIMEOUT 120


#ifdef GUI
char *checkmenu(MenuList *check, int menuid)
{
MenuList *tmpml;
MenuStruct *tmpms;
char *tmpalias;

	tmpml=check;
	while(tmpml!=NULL)
	{
		if (tmpml->menutype==GUISUBMENU || tmpml->menutype==GUISHARED || tmpml->menutype==GUIBRKSUBMENU)
		{
			tmpms = (MenuStruct *)findmenu(tmpml->submenu);
			if ((tmpalias=checkmenu(tmpms->menuorigin, menuid))!=NULL)
				return tmpalias;
		}
		if (tmpml->menuid==menuid)
			return tmpml->alias;
		tmpml=tmpml->next;
	}
	return NULL;
}

void scan_gui(fd_set *rd) 
{
char guicmd[3];
extern MenuStruct *morigin;
extern unsigned long menucmd;
MenuStruct *tmpms;
int newpos, cursb;
char *alias;
Window *scrollwin, *newfocuswindow;
float bleah;
Screen *newfocusscreen;

#ifdef __EMXPM__
extern int just_resized;
extern Screen *just_resized_screen;
#endif

#ifdef __EMXPM__
    if(just_resized == 1)
        WinSendMsg(just_resized_screen->hwndFrame, 0x041e, 0, 0);
#endif
    
    /* Read the data from the pipe and decide what to do */
    if(FD_ISSET(guiipc[0], rd))
      {
       read(guiipc[0], guicmd, 2);
       switch(guicmd[0])
          {
           case '2':
             tmpms = morigin;
             while(tmpms!=NULL)
                {
                 if((alias=checkmenu(tmpms->menuorigin, menucmd))!=NULL)
                      {
                      make_window_current(get_window_by_refnum((int)guicmd[1]));
                      parse_line(NULL, alias, empty_string, 0, 0, 0);
                      break;
                      }
                 tmpms=tmpms->next;
                }
              break;
           case '3':
              gui_flush();
              newfocuswindow=get_window_by_refnum((int)guicmd[1]);
              if(newfocuswindow)
                 newfocusscreen=newfocuswindow->screen;
              else
                 newfocusscreen=NULL;
              if (newfocusscreen && newfocusscreen->current_window)
                 {
                 make_window_current(newfocusscreen->current_window);
                 output_screen=last_input_screen=newfocusscreen;
                 set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 1);
                 }
              update_input(UPDATE_ALL);
              break;
           case '4':
              refresh_window_screen(get_window_by_refnum((int)guicmd[1]));
              break;
           case '5':
              if(newscrollerpos != lastscrollerpos || lastscrollerwindow != (int)guicmd[1])
                 {
                    lastscrollerpos=newscrollerpos;
                    lastscrollerwindow=(int)guicmd[1];
                    scrollwin=get_window_by_refnum((int)guicmd[1]);
                    bleah = get_int_var(SCROLLBACK_VAR);
                    newpos = (int)(((bleah-newscrollerpos) / bleah) * (scrollwin->display_buffer_size));
                    cursb = scrollwin->distance_from_display;
                    printf("lsp = %d lsw = %d newpos = %d cursb = %d\n", lastscrollerpos, lastscrollerwindow, newpos, cursb);
                    if (newpos > cursb) 
                       scrollback_backwards_lines(newpos-cursb);
                    if (newpos < cursb)
                       scrollback_forwards_lines(cursb-newpos);
                 }
              break;
           case '6':
              scrollback_backwards_lines(1);
              break;
           case '7':
              scrollback_forwards_lines(1);
              break;
           case '8':
              scrollback_backwards((char)NULL, NULL);
              break;
           case '9':
              scrollback_forwards((char)NULL, NULL);
              break;
           case 'A':
              wm_process((int)guicmd[1]);
              break;
          }
      }
}
#endif


void read_clonelist(int s)
{
char buffer[IRCD_BUFFER_SIZE + 1];
char *str = buffer;
	switch(dgets(str, s, 0, IRCD_BUFFER_SIZE))
	{
		case -1:
		{
			do_hook(SOCKET_LIST, "%d %s %d", s, sock_manager.sockets[s].server, sock_manager.sockets[s].port);
			close_socketread(s);
			break;
		}
		case 0:
			break;
		default:
		{
			if ((buffer[strlen(buffer)-1] == '\r') || (buffer[strlen(buffer)-1] == '\n'))
				buffer[strlen(buffer)-1] = 0;
			if ((buffer[strlen(buffer)-1] == '\r') || (buffer[strlen(buffer)-1] == '\n'))
				buffer[strlen(buffer)-1] = 0;
			if (*buffer)
				do_hook(SOCKET_LIST, "%d %s %d %s", s, sock_manager.sockets[s].server, sock_manager.sockets[s].port, buffer);
		}
	}
}

void read_clonenotify(int s)
{
unsigned long flags = 0;
	flags = get_socketflags(s);
	do_hook(SOCKET_NOTIFY_LIST, "%d %s %d %d", s, sock_manager.sockets[s].server, sock_manager.sockets[s].port, (unsigned int)flags);
}

unsigned long set_socketflags(int s, unsigned long flags)
{
	if (check_socket(s))
	{
		sock_manager.sockets[s].flags = flags;
		return sock_manager.sockets[s].flags;
	}
	return 0;
}

unsigned long get_socketflags(int s)
{
	if (check_socket(s))
		return sock_manager.sockets[s].flags;
	return 0;
}

char *get_socketserver(int s)
{
	if (check_socket(s))
		return sock_manager.sockets[s].server;
	return NULL;
}

void *get_socketinfo(int s)
{
	if (check_socket(s))
		return sock_manager.sockets[s].info;
	return NULL;
}

void set_socketinfo(int s, void *info)
{
	if (check_socket(s))
		sock_manager.sockets[s].info = info;
}

int get_max_fd(void)
{
	return sock_manager.max_fd + 1;
}

int add_socketread(int s, int port, unsigned long flags, char *server, void (*func_read)(int), void (*func_write)(int))
{
	if (!sock_manager.init)
	{
		fd_set rd;
		FD_ZERO(&rd);
		set_socket_read(&rd, &rd);
	}
	if (s > FD_SETSIZE)
		return -1;
	if (s > sock_manager.max_fd)
		sock_manager.max_fd = s;
	sock_manager.count++;
	sock_manager.sockets[s].is_read = s;
	sock_manager.sockets[s].port = port;
	sock_manager.sockets[s].flags = flags;
	if (server)
		sock_manager.sockets[s].server = m_strdup(server);
	sock_manager.sockets[s].func_read = func_read;
	sock_manager.sockets[s].func_write = func_write;
	new_open(s);
	return s;
}

int set_socketwrite(int s)
{
	if (s > FD_SETSIZE)
		return -1;
	if (s > sock_manager.max_fd)
		sock_manager.max_fd = s;
	sock_manager.sockets[s].is_write = s;	
	new_open_write(s);
	return s;
}

void add_sockettimeout(int s, time_t timeout)
{
	if (timeout < 0)
	{
		timeout = get_int_var(CONNECT_TIMEOUT_VAR);
		if (timeout <= 0)
			timeout = SOCKET_TIMEOUT;
	}
	if (timeout)
		sock_manager.sockets[s].time = now + timeout;
	else
		sock_manager.sockets[s].time = 0;
}
	
void close_socketread(int s)
{
	if (!sock_manager.count)
		return;
	if (sock_manager.sockets[s].is_read)
	{
		new_free(&sock_manager.sockets[s].server);
		if (sock_manager.sockets[s].is_write)
			new_close_write(s);
		new_close(s);
		memset(&sock_manager.sockets[s], 0, sizeof(SocketList));
		sock_manager.count--;
		if (s == sock_manager.max_fd)
		{
			int i;
			sock_manager.max_fd = -1;
			for (i = 0; i < FD_SETSIZE; i++)
				if (sock_manager.sockets[i].is_read)
					sock_manager.max_fd = i;
		}
	}
}

int check_socket(int s)
{
	if (s != -1 && sock_manager.count && sock_manager.sockets[s].is_read)
		return 1;
	return 0;
}

int check_dcc_socket(int s)
{
	if (sock_manager.count && sock_manager.sockets[s].is_read && sock_manager.sockets[s].info)
	{
		if ( ((dcc_struct_type *)sock_manager.sockets[s].info)->struct_type == DCC_STRUCT_TYPE)
			return 1;
	}
	return 0;
}

int write_sockets(int s, unsigned char *str, int len, int nl)
{
	if (s < 1)
		return -1;
	if (nl)
	{
		unsigned char *buf;
		buf = alloca(strlen(str)+4);
		strcpy(buf, str);
		strcat(buf, "\r\n");
		len += 2;
		return write(s, buf, len);
	}
	return write(s, str, len);
}

int read_sockets(int s, unsigned char *str, int len)
{
	if (s < 1)
		return -1;
	return read(s, str, len);
}

void set_socket_read (fd_set *rd, fd_set *wr)
{
register int i;
static int socket_init = 0;
	if (!socket_init)
	{
		memset(&sock_manager, 0, sizeof(sock_manager));
		socket_init++;
		sock_manager.init++;
	}
	if (!sock_manager.count) return;
	for (i = 0; i < sock_manager.max_fd + 1; i++)
	{
		if (sock_manager.sockets[i].is_read)
			FD_SET(sock_manager.sockets[i].is_read, rd);
		if (sock_manager.sockets[i].is_write)
			FD_SET(sock_manager.sockets[i].is_write, wr);
	}
}

void scan_sockets(fd_set *rd, fd_set *wr)
{
register int i;
time_t t = now;
	if (!sock_manager.count) return;
	for (i = 0; i < sock_manager.max_fd+1; i++)
	{
		if (sock_manager.sockets[i].is_read && FD_ISSET(sock_manager.sockets[i].is_read, rd))
			(sock_manager.sockets[i].func_read) (sock_manager.sockets[i].is_read);
		if (sock_manager.sockets[i].func_write && sock_manager.sockets[i].is_write && FD_ISSET(sock_manager.sockets[i].is_write, wr))
			(sock_manager.sockets[i].func_write) (sock_manager.sockets[i].is_write);
		if (sock_manager.sockets[i].time && (t >= sock_manager.sockets[i].time))
			close_socketread(i);
	}
}

SocketList *get_socket(int s)
{
	if (check_socket(s))
		return &sock_manager.sockets[s];
	return NULL;
}

extern int dgets_errno;

void read_netfinger(int s)
{
char tmpstr[BIG_BUFFER_SIZE+1];
register unsigned char *p = tmpstr;
	*tmpstr = 0;
	switch(dgets(tmpstr, s, 0, BIG_BUFFER_SIZE))
	{
		case 0:
			break;
		case -1:
			if (do_hook(SOCKET_LIST, "%d %d %s Remote closed connection", s, sock_manager.sockets[s].port, sock_manager.sockets[s].server))
			{
				if (dgets_errno == -1)
					bitchsay("Remote closed connection");
			}
			close_socketread(s);
			break;
		default:
		{
			chop(tmpstr, 1);
			while (*p)
			{
				switch(*p)
				{
					case 0210:
					case 0211:
					case 0212:
					case 0214:
						*p -= 0200;
						break;
					case 0x9b:
					case '\t':
						break;
					case '\n':
					case '\r':
						*p = '\0';
						break;
					default:
						if (!isprint(*p))
							*p = (*p & 0x7f) | 0x40;
						break;
				}
				p++;
			}
			if (do_hook(SOCKET_LIST, "%d %d %s %s", s, sock_manager.sockets[s].port, sock_manager.sockets[s].server, tmpstr))
				put_it("%s", tmpstr);
		}
	}
}

void netfinger (char *name)
{
	char *host = NULL;
	unsigned short port = 79;
	int s;
	
	if (name)
	{
		if ((host = strrchr(name, '@')))
			*host++ = 0;
		else
			host = name;
	}
	if (!host || !*host)
	{
		say("Invalid @host or user@host.");
		return;
	}

	if (name && *name)
	{	
		if ((s = connect_by_number(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 1)) < 0)
		{
			bitchsay("Finger connect error on %s@%s", name?name:empty_string, host);
			return;
		}
		if ((add_socketread(s, port, 0, name, read_netfinger, NULL)) > -1)
		{
			write_sockets(s, name, strlen(name), 1);
			add_sockettimeout(s, 120);
		} else
			close_socketread(s);
	}
	return;
}

void start_finger (UserhostItem *stuff, char *nick, char *args)
{
	char *finger_userhost = NULL;
	char *str;
	if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick))
	{
		say("No information for %s", nick);
		return;
	}

	malloc_sprintf(&finger_userhost, "%s@%s", stuff->user, stuff->host);
	str = finger_userhost;
	str = clear_server_flags(finger_userhost);
	say("Launching finger for %s (%s)", nick, finger_userhost);
	netfinger(str);
	new_free(&finger_userhost);
}

BUILT_IN_COMMAND(finger)
{
	char *userhost;

	if ((userhost = next_arg(args, &args)))
	{
		if (!strchr(userhost, '@'))
		{
			userhostbase(userhost, start_finger, 1, "%s", userhost);
			return;
		}
		netfinger(userhost);
	}
	else
		userage(command, helparg);
}

static void handle_socket_connect(int rc)
{
struct servent *serv;
struct sockaddr_in      addr;
struct hostent *host;
        int             address_len;

	address_len = sizeof(struct sockaddr_in);
        if ((getpeername(rc, (struct sockaddr *) &addr, &address_len)) != -1)
	{
		serv = getservbyport(addr.sin_port,"tcp");
		address_len = sizeof(addr.sin_addr);
		host = gethostbyaddr((char *)&addr.sin_addr, address_len, AF_INET);
		put_it("Hostname %s:%s  port %d is running (%s)", host->h_name, inet_ntoa(addr.sin_addr), htons(addr.sin_port), serv == NULL? "UNKNOWN":serv->s_name);
	}
	close_socketread(rc);
}

static int scan(char *remote_host, int low_port, int high_port, struct hostent *host)
{
	unsigned short int port;
	int rc;
	if (low_port == 0) low_port = 1;	
	for (port = low_port;port <= high_port;port++) 
	{
		if ((rc = connect_by_number(remote_host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 1)) < 0)
			continue;
		if ((add_socketread(rc, port, 0, NULL, handle_socket_connect, NULL)) > -1)
			add_sockettimeout(rc, 120);
		else
			close_socketread(rc);
	}

	return 1;
}

void userhost_scanport(UserhostItem *stuff, char *nick, char *args)
{
char	*t;
int	low_port = 0,
	high_port = 0;
struct	hostent *host;

        if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick))
        {
        	bitchsay("No such nick [%s] found", nick);
                return;
        }
	next_arg(args, &args);
	t = next_arg(args, &args);
	low_port = atol(t);
	t = next_arg(args, &args);
	high_port = atol(t);	
	if ((host = resolv(stuff->host)))
	{
		bitchsay("Scanning %s ports %d to %d", stuff->host, low_port, high_port);
		scan(stuff->host, low_port, high_port, host);
		return;
	}
	bitchsay("Cannot resolv host %s for %s", stuff->host, stuff->nick);
}

BUILT_IN_COMMAND(findports)
{
char *remote_host;
int low_port = 6660;
int high_port = 7000;
struct hostent *host;

	
	if (args && *args)
	{
		char *tmp = NULL;
		remote_host = next_arg(args, &args);
		if (args && *args)
		{
			tmp = next_arg(args, &args);
			low_port = strtoul(tmp, NULL, 10);	
			if (args && *args)
			{
				tmp = next_arg(args, &args);
				high_port = strtoul(tmp, NULL, 10);	
			}
			else
				high_port = low_port;
		}
		if (strchr(remote_host, '.'))
		{
			if ((host = resolv(remote_host)))
			{
				bitchsay("Scanning %s's tcp ports %d through %d",remote_host, low_port,high_port);
				scan(remote_host, low_port, high_port, host);
			} else
				bitchsay("No such host %s", remote_host);
		}
		else
			userhostbase(remote_host, userhost_scanport, 1, "%s %d %d", remote_host, low_port, high_port);
	} else
		userage("FPORT", helparg);
	return;
}












void userhost_ignore (UserhostItem *stuff, char *nick1, char *args)
{
	char *p, *arg;
	char *nick = NULL, *user = NULL, *host = NULL;
	int old_window_display;
	char ignorebuf[BIG_BUFFER_SIZE+1];
	Ignore *igptr, *igtmp;
	WhowasList *whowas;

	arg = next_arg(args, &args);
	if (!stuff || !stuff->nick || !nick1 || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick1))
	{
                if ((whowas = check_whowas_nick_buffer(nick1, arg, 0)))
		{
			bitchsay("Using WhoWas info for %s of %s ", arg, nick1);
			user = host; host = strchr(host, '@'); *host++ = 0;
			nick = whowas->nicklist->nick;
		}
		else
		{                                                                                                                
			say("No match for user %s", nick1);
			return;
		}
	}
	else
	{
		user = clear_server_flags(stuff->user);
		host = stuff->host; nick = stuff->nick;
	}
	if (!arg || !*arg || !my_stricmp(arg, "+HOST"))
		sprintf(ignorebuf, "*!*@%s ALL -CRAP -PUBLIC", cluster(host));
	else if (!my_stricmp(arg, "+USER"))
		sprintf(ignorebuf, "*%s@%s ALL -CRAP -PUBLIC", user, cluster(host));
	else if (!my_stricmp(arg, "-USER") || !my_stricmp(arg, "-HOST"))
	{
		int found = 0;
		if (!my_stricmp(arg, "-HOST"))
			sprintf(ignorebuf, "*!*@%s", cluster(host));
		else
			sprintf(ignorebuf, "%s!%s@%s", nick, user, host);
		igptr = ignored_nicks;
		while (igptr != NULL)
		{
			igtmp = igptr->next;
			if (wild_match(igptr->nick, ignorebuf) ||
			    wild_match(nick, igptr->nick))
			{
				sprintf(ignorebuf, "%s NONE", igptr->nick);
				old_window_display = window_display;
				window_display = 0;
				ignore(NULL, ignorebuf, ignorebuf, NULL);
				window_display = old_window_display;
				bitchsay("Unignored %s!%s@%s", nick, user, host);
				found++;
			}
			igptr = igtmp;
		}
		if (!found)
			bitchsay("No Match for ignorance of %s", nick);
		return;
	}
	old_window_display = window_display;
	window_display = 0;
	ignore(NULL, ignorebuf, ignorebuf, NULL);
	if ((arg = next_arg(args, &args)))
	{
		char tmp[BIG_BUFFER_SIZE+1];
		sprintf(tmp, "%s ^IGNORE %s NONE", arg, ignorebuf);
		timercmd("TIMER", tmp, NULL, NULL);
	}
	window_display = old_window_display;
	if ((p = strchr(ignorebuf, ' ')))
		*p = 0;
	say("Now ignoring ALL except CRAP and PUBLIC from %s", ignorebuf);
	return;
}

BUILT_IN_COMMAND(reset)
{
	refresh_screen(0, NULL);
}


extern char *channel_key (char *);
BUILT_IN_COMMAND(cycle)
{
	char *to = NULL;
	int server = from_server;
	ChannelList *chan;
	
	if (args && args)
		to = next_arg(args, &args);
		
	if (!(chan = prepare_command(&server, to, NO_OP)))
		return;		
	my_send_to_server(server, "PART %s\nJOIN %s%s%s", chan->channel, chan->channel, chan->key?" ":"", chan->key?chan->key:"");
/*	my_send_to_server(server, "JOIN %s%s%s", chan->channel, chan->key?" ":"", chan->key?chan->key:"");*/
}


int do_newuser(char *command, char *args, char *subargs)
{
char *newusername = NULL;
	if ((newusername = next_arg(args, &args)))
	{
#ifdef IDENT_FAKE
		FILE *outfile;
		char *p = NULL, *q = NULL;
		malloc_sprintf(&p, "~/%s", get_string_var(IDENT_HACK_VAR));
		q = expand_twiddle(p);
		outfile = fopen(q,"w");
#ifdef CIDENTD
		fprintf(outfile,"hideme\nmynameis %s\n", newusername);
#else
		fprintf(outfile,"%s", newusername);
#endif
		fclose(outfile);
#endif
		strmcpy(username, newusername, NAME_LEN);
		if (subargs && *subargs)
                        strmcpy(realname, subargs, REALNAME_LEN);
#ifdef IDENT_FAKE
		new_free(&p); new_free(&q);
#endif
		reconnect_cmd(NULL, newusername, NULL, NULL);
	}
	else
		return 0;
	return 1;
}

BUILT_IN_COMMAND(newnick)
{
	char *newnick, *newusername;

	if ((newnick = next_arg(args, &args)) &&
	    (newusername = next_arg(args, &args)))
		do_newuser(newnick, newusername, args);
	else
		say("You must specify a nick and username");
}


BUILT_IN_COMMAND(newuser)
{
	char *newusername;

	if ((newusername = next_arg(args, &args)))
	{
		if ((do_newuser(NULL, newusername, args)))
			say("You must specify a username.");
	} else
		userage(command, helparg);
}

BUILT_IN_COMMAND(do_ig)
{
	char *nick;
	char ignore_type[6];
	int got_ignore_type = 0;
	int need_time = 0;
	if (!args || !*args)
		goto bad_ignore;

	while ((nick = next_arg(args, &args)))
	{
		if (!nick || !*nick)
			goto bad_ignore;
		if (*nick == '-' || *nick == '+')
		{
			if (!my_stricmp(nick, "-USER") || !my_stricmp(nick, "+HOST") || !my_stricmp(nick, "+USER") || !my_stricmp(nick, "-HOST"))
				strcpy(ignore_type, nick);
			if (!args || !*args)
				goto bad_ignore;
			got_ignore_type++;
			continue;
		}
		else if (!got_ignore_type)
		{
			if (command && !my_strnicmp(command, "IGH",3))
				strcpy(ignore_type, "+HOST");
			else if (command && !my_strnicmp(command, "IG",2))
				strcpy(ignore_type, "+USER");
			if (command && !my_strnicmp(command, "UNIGH", 5))
				strcpy(ignore_type, "-HOST");
			else if (command && !my_strnicmp(command, "UNIG", 4))
				strcpy(ignore_type, "-USER");
			if (command && toupper(command[strlen(command)-1]) == 'T')
				need_time ++;
		}
		if (need_time)
			userhostbase(nick, userhost_ignore, 1, "%s %d", ignore_type, get_int_var(IGNORE_TIME_VAR));
		else
			userhostbase(nick, userhost_ignore, 1, "%s", ignore_type);
	}
	return;
bad_ignore:
	userage(command, helparg);
}



BUILT_IN_COMMAND(users)
{
ChannelList *chan;
NickList *nicks;
NickList *sortl = NULL;
char	*to = NULL, 
	*spec = "*!*@*", 
	*temp1 = NULL;
char	modebuf[BIG_BUFFER_SIZE + 1];
char	msgbuf[BIG_BUFFER_SIZE +1];
int	count = 0, 
	ops = 0, 
	msg = 0,
	num_kicks = 0,
	hook = 0,
	not =  0,
	server = from_server,
	sorted = NICKSORT_NORMAL;
		
	*msgbuf = 0;
	*modebuf = 0;
		
	while (args && *args)
	{
		if (!args || !*args)
			break;
		temp1 = next_arg(args, &args);
		if (*temp1 == '-')
		{
			if (!my_strnicmp(temp1, "-ops", strlen(temp1)))
				ops = 1;
			else if (!my_strnicmp(temp1, "-nonops", strlen(temp1)))
				ops = 2;
			else if (!my_strnicmp(temp1, "-msg", strlen(temp1)))
				msg = 1;
			else if (!my_strnicmp(temp1, "-notice", strlen(temp1)))
				msg = 2;
			else if (!my_strnicmp(temp1, "-nkill", strlen(temp1)))
				msg = 3;
			else if (!my_strnicmp(temp1, "-kill", strlen(temp1)))
				msg = 4;
			else if (!my_strnicmp(temp1, "-kick", strlen(temp1)))
				msg = 5;
			else if (!my_strnicmp(temp1, "-stats", strlen(temp1)))
				msg = 6;
			else if (!my_strnicmp(temp1, "-ips", strlen(temp1)))
				msg = 7;
			else if (!my_strnicmp(temp1, "-sort", strlen(temp1)) && args && *args)
			{
				if (!my_strnicmp(args, "none", 4))
					sorted = NICKSORT_NONE;
				else if (!my_strnicmp(args, "host", 4))
					sorted = NICKSORT_HOST;
				else if (!my_strnicmp(args, "nick", 4))
					sorted = NICKSORT_NICK;
				else if (!my_strnicmp(args, "ip", 3))
					sorted = NICKSORT_IP;
				else if (!my_strnicmp(args, "time", 3))
					sorted = NICKSORT_TIME;
				if (sorted != NICKSORT_NORMAL)
					next_arg(args, &args);
			}
			else if (strpbrk(temp1, "*!@."))
			{
				spec = temp1+1;
				not = 1;
			}
		}
		else if (temp1 && is_channel(temp1))
			to = temp1;
		else if (strpbrk(temp1, "*!@."))
			spec = temp1;
		else
		{
			if (args && *args)
				temp1[strlen(temp1)] = ' ';
			args = temp1;
			break;
		}
	}
	if (!spec || !*spec)
		spec = "*!*@*";
	if (!(chan = prepare_command(&server, to, NO_OP)))
	{
		bitchsay(to?"Not on that channel %s":"No such channel %s", to?to:empty_string);
		return;
	}
	message_from(chan->channel, LOG_CRAP);
	if (command && !my_stricmp(command, "CHOPS"))
		ops = 1;
	if (command && !my_stricmp(command, "NOPS"))
		ops = 2;

	if ((msg == 1 || msg == 2) && (!args || !*args))
	{
		say("No message given");
		message_from(NULL, LOG_CRAP);
		return;
	}

	switch (msg)
	{
		case 6:
			if (do_hook(STAT_HEADER_LIST, "%s %s %s %s %s", "Nick", "dops", "kicks","nicks","publics"))
				put_it("Nick        dops  kicks  nicks  publics");
			break;
		case 0:
		{
			char *f;
			if ((f = fget_string_var(FORMAT_USERS_TITLE_FSET)))
				put_it("%s", convert_output_format(f, "%s %s", update_clock(GET_TIME), chan->channel));
		}
		default:
			break;
	}

	sortl = sorted_nicklist(chan, sorted);
	for (nicks = sortl; nicks; nicks = nicks->next)
	{
		sprintf(modebuf, "%s!%s", nicks->nick,
		      nicks->host ? nicks->host : "<UNKNOWN@UNKNOWN>");
		if (msg == 7 && nicks->ip)
		{
			strcat(modebuf, " "); 
			strcat(modebuf, nicks->ip);
		}
		if (((!not && wild_match(spec, modebuf)) || (not && !wild_match(spec, modebuf))) && 
					(!ops ||
					 ((ops == 1) && nicks->chanop) ||
					 ((ops == 2) && !nicks->chanop) ))
		{
			switch(msg)
			{
				case 3: /* nokill */
					count--;
					break;
				case 4: /* kill */
				{
					if (!isme(nicks->nick))
						my_send_to_server(server, "KILL %s :%s (%i", nicks->nick,
							       args && *args ? args : get_reason(nicks->nick, NULL),
							       count + 1);
					else
						count--;
					break;
				}
				case 5:	 /* mass kick */
				{
					if (!isme(nicks->nick))
					{
						if (*msgbuf)
							strcat(msgbuf, ",");
						strcat(msgbuf, nicks->nick);
						num_kicks++;
					} else 
						count--;
					if ((get_int_var(NUM_KICKS_VAR) && (num_kicks == get_int_var(NUM_KICKS_VAR))) || strlen(msgbuf) >= 450)
					{
						my_send_to_server(server, "KICK %s %s :%s", chan->channel,
							       msgbuf, (args && *args) ? args :
							       "-=punt=-");
						*msgbuf = 0;
						num_kicks = 0;
					}
					break;
				}
				case 6:
				{
					if (!isme(nicks->nick))
					{
						if (do_hook(STAT_LIST, "%s %d %d %d %d", nicks->nick, nicks->dopcount, nicks->kickcount, 
							nicks->nickcount, nicks->floodcount))
							put_it("%-10s  %4d   %4d   %4d     %4d", 
								nicks->nick, nicks->dopcount, nicks->kickcount, 
								nicks->nickcount, nicks->floodcount);
					}
					break;
				}	
				case 7:
					if (do_hook(USERS_IP_LIST, "%s %s %s", nicks->nick, nicks->host, nicks->ip?nicks->ip:"Unknown"))
						put_it("%s!%s = %s", nicks->nick, nicks->host, nicks->ip?nicks->ip:"Unknown");
					break;
				case 1:
				case 2:
				{
					if (*msgbuf)
						strcat(msgbuf, ",");
					strcat(msgbuf, nicks->nick);
					if (strlen(msgbuf)+strlen(args) >= 490)
					{
						put_it("%s", convert_output_format(fget_string_var((msg == 1)?FORMAT_SEND_MSG_FSET:FORMAT_SEND_NOTICE_FSET), "%s %s %s %s", update_clock(GET_TIME),msgbuf, get_server_nickname(from_server), args));
						my_send_to_server(server, "%s %s :%s", (msg == 1) ? "PRIVMSG" : "NOTICE", msgbuf, args);
						*msgbuf = 0;
					}
					break;
				}
				default:
				{
					if (!count && do_hook(USERS_HEADER_LIST, "%s %s %s %s %s %s %s", "Level", "aop", "prot", "Channel", "Nick", "+o", "UserHost") && fget_string_var(FORMAT_USERS_HEADER_FSET))
						put_it("%s", convert_output_format(fget_string_var(FORMAT_USERS_HEADER_FSET), "%s", chan->channel));
				
					if ((hook = do_hook(USERS_LIST, "%lu %s %s %s %c",
						nicks->userlist ? nicks->userlist->flags:nicks->shitlist?nicks->shitlist->level:0, 
						chan->channel, nicks->nick, 
						nicks->host, 
						nicks->chanop ? '@' : ' ')))
					{
						put_it("%s", convert_output_format(fget_string_var(nicks->userlist?FORMAT_USERS_USER_FSET:nicks->shitlist?FORMAT_USERS_SHIT_FSET:FORMAT_USERS_FSET), "%s %s %s %s %s", 
							nicks->userlist ? convert_flags(nicks->userlist->flags) : nicks->shitlist?ltoa(nicks->shitlist->level):"n/a            ",
							chan->channel, nicks->nick,
							nicks->host,
							nicks->chanop ? "@" : (nicks->voice? "v" : "ÿ")));
					}
				}
			}
			count++;
		} 
		else if (msg == 3)
		{
			count++;
			if (!isme(nicks->nick))
			{
				my_send_to_server(server, "KILL %s :%s (%i", nicks->nick,
					       args && *args ? args : get_reason(nicks->nick, NULL),
					       count);
			}
			else
				count--;
		}
	}
	clear_sorted_nicklist(&sortl);
	if (!msg && do_hook(USERS_FOOTER_LIST, "%s", "End of Users"))
		;
	else if (msg == 6 && do_hook(STAT_FOOTER_LIST, "%s", "End of stats"))
		;
	else if (!count)
	{
		if (chan)
		{
			if (!command)
				say("No match of %s on %s", spec, chan->channel);
			else
				say("There are no [\002%s\002] on %s", command, chan->channel);
		}
	}
	else if (!msg && !hook)
		bitchsay("End of UserList on %s %d counted", chan->channel, count);

	if (count && *msgbuf)
	{
		switch(msg)
		{
			case 1:
			case 2:
			{
				put_it("%s", convert_output_format(fget_string_var((msg == 1)?FORMAT_SEND_MSG_FSET:FORMAT_SEND_NOTICE_FSET), "%s %s %s %s", update_clock(GET_TIME),msgbuf, get_server_nickname(from_server), args));
				my_send_to_server(server, "%s %s :%s", (msg == 1) ? "PRIVMSG" : "NOTICE", msgbuf, args);
				break;
			}
			case 5:
			{
				if (chan)
				my_send_to_server(server, "KICK %s %s :%s", chan->channel,
					       msgbuf, (args && *args) ? args : "-=punt=-");
			}		
			default:
				break;
		}
	}
	message_from(NULL, LOG_CRAP);

}

int caps_fucknut (register unsigned char *crap)
{
	int total = 0, allcaps = 0;
/* removed from ComStud client */
	while (*crap)
	{
		if (isalpha(*crap))
		{
			total++;
			if (isupper(*crap))
				allcaps++;
		}
		crap++;
	}
	if (total > 12)
	{
		if ( ((unsigned int)(((float) allcaps / (float) total) * 100) >= 75))
			return (1);
	}
	return (0);
}

int char_fucknut (register unsigned char *crap, char looking, int max)
{
	int total = strlen(crap), allchar = 0;

	while (*crap)
	{
		if ((*crap == looking))
		{
			crap++;
			while(*crap && *crap != looking)
			{
				allchar++;
				crap++;
			}
		}
		if (*crap)
			crap++;
	}
	if (total > 12)
	{
		if ( ((unsigned int)(((float) allchar / (float) total) * 100)) >= 75)
			return (1);
	}
	return (0);
}

static int cparse_recurse = -1;
#define MAX_RECURSE 5
#define RECURSE_CPARSE

char *convert_output_format_raw(const char *format, const char *str, va_list args)
{
static unsigned char buffer[MAX_RECURSE*BIG_BUFFER_SIZE+1];
char buffer2[3*BIG_BUFFER_SIZE+1];
enum color_attributes this_color = BLACK;
register unsigned char *s;
char *copy = NULL;
char *tmpc = NULL;
register char *p;
int old_who_level = who_level;
int bold = 0;
extern int in_chelp;
/*va_list args;*/
int arg_flags;
char color_mod[] = "kbgcrmywKBGCRMYWn"; 

	if (!format)
		return empty_string;
	copy = alloca(strlen(format)+2);
	strcpy(copy, format);

	memset(buffer2, 0, BIG_BUFFER_SIZE);

	if (cparse_recurse < MAX_RECURSE)
		cparse_recurse++;

	if (str)
	{

		p = (char *)str;
/*		va_start(args, str);*/
		while(p && *p)
		{
			if (*p == '%')
			{
				switch(*++p)
				{
				case 's':
				{
					char *q, *s = (char *)va_arg(args, char *);
#ifdef RECURSE_CPARSE
					char buff[BIG_BUFFER_SIZE];
					q = buff;
					while (s && *s)
					{
						if (*s == '%')
							*q++ = '%';
						else if (*s == '$')
							*q++ = '$';
						*q++ = *s++;
					}
					*q = 0;
					if (*buff)
						strcat(buffer2, buff);
#else
					if (s)
						strcat(buffer2, s);
#endif
					break;
				}
				case 'd':
				{
					int d = (int) va_arg(args, int);
					strcat(buffer2, ltoa((long)d));
					break;
				}
				case 'c':
				{
					char c = (char )va_arg(args, int);
					buffer2[strlen(buffer2)] = c;
					break;
				}
				case 'u':
				{
					unsigned int d = (unsigned int) va_arg(args, unsigned int);
					strcat(buffer2, ltoa(d));
					break;
				}
				case 'l':
				{
					unsigned long d = (unsigned long) va_arg(args, unsigned long);
					strcat(buffer2, ltoa(d));
					break;
				}
				case '%':
				{
					buffer2[strlen(buffer2)] = '%';
					p++;
					break;
				}
				default:
					strcat(buffer2, "%");
					buffer2[strlen(buffer2)] = *p;
				}
				p++;
			} else 
			{
				buffer2[strlen(buffer2)] = *p;
				p++;
			}
		}
/*		va_end(args);*/
	} 
	else if (str)
		strcpy(buffer2, str);

	s = buffer + (BIG_BUFFER_SIZE * cparse_recurse);
	memset(s, 0, BIG_BUFFER_SIZE);
	tmpc = copy;
	if (!tmpc)
		goto done;
	while (*tmpc)
	{
		if (*tmpc == '%')
		{
			char *cs;
			tmpc++;
			this_color = BLACK;
			if (*tmpc == '%')
			{
				*s++ = *tmpc++;
				continue;
			}
			else if (isdigit(*tmpc))
			{
				char background_mod[] = "01234567";
				if ((cs = strchr(background_mod, *tmpc)))
				{
					this_color = ((int)cs - (int)&background_mod) + (bold ? BACK_BBLACK : BACK_BLACK);
					bold = 0;
				}
				else if (*tmpc == '8')
				{
					this_color = REVERSE_COLOR;
					bold = 0;
				} 
				else
				{
					this_color = BOLD_COLOR;
					bold ^= 1;
				}
			}
			else if ((cs = strchr(color_mod, *tmpc)))
				this_color = ((int)cs - (int)&color_mod);
			else if (*tmpc == 'F')
				this_color = BLINK_COLOR;
			else if (*tmpc == 'U')
				this_color = UNDERLINE_COLOR;
			else if (*tmpc == 'A')
				this_color = (int) (((float)UNDERLINE_COLOR * rand())/RAND_MAX);
			else if (*tmpc == 'P')
				this_color = MAGENTAB;
			else if (*tmpc == 'p')
				this_color = MAGENTA;
			else
			{
				*s++ = *tmpc;
				continue;
			}
			strcpy(s, color_str[this_color]);
			while(*s) s++;
			tmpc++;
			continue;
		}
		else if (*tmpc == '$' && !in_chelp)
		{
			char *new_str = NULL;
			tmpc++;
			if (*tmpc == '$')
			{
				*s++ = *tmpc++;
				continue;
			}
			in_cparse++;
			tmpc = alias_special_char(&new_str, tmpc, buffer2, NULL, &arg_flags);
			in_cparse--;
			if (new_str)
#ifdef RECURSE_CPARSE
				strcat(s, convert_output_format_raw(new_str, NULL, NULL));
#else
				strcat(s, new_str);
#endif
			new_free(&new_str);
			while (*s) { if (*s == 255) *s = ' '; s++; }
			if (!tmpc) break;
			continue;
		} else
			*s = *tmpc;
		tmpc++; s++;
	}
	*s = 0;

done:
	s = buffer + (BIG_BUFFER_SIZE * cparse_recurse);
/*	if (*s) strcat(s, color_str[NO_COLOR]);*/

	who_level = old_who_level;
	cparse_recurse--;

	return s;
}

char *convert_output_format(const char *format, const char *str, ...)
{
	char *ret;
	va_list args;
	va_start(args, str);
	ret = convert_output_format_raw(format, str, args);
	va_end(args);
	if (*ret)
		strcat(ret, color_str[NO_COLOR]);
	return ret;
}


char *convert_output_format2(const char *str)
{
unsigned char buffer[3*BIG_BUFFER_SIZE+1];
unsigned char buffer2[3*BIG_BUFFER_SIZE+1];
register unsigned char *s;
char *copy = NULL;
char *tmpc = NULL;
int arg_flags;

	if (!str)
		return m_strdup(empty_string);

	memset(buffer, 0, BIG_BUFFER_SIZE);
        strcpy(buffer2, str);
	copy = tmpc = buffer2;
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
#ifdef RECURSE_CPARSE
				strcat(s, convert_output_format(new_str, NULL, NULL));
#else
				strcat(s, new_str);
#endif
                        s=s+(strlen(new_str));
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

void add_last_type (LastMsg *array, int size, char *from, char *uh, char *to, char *str)
{
int i;
	for (i = size - 1; i > 0; i--)
	{
		
		malloc_strcpy(&(array[i].last_msg), array[i - 1].last_msg);
		malloc_strcpy(&(array[i].from), array[i - 1].from);
		malloc_strcpy(&(array[i].uh), array[i - 1].uh);
		malloc_strcpy(&(array[i].to), array[i - 1].to);
		malloc_strcpy(&(array[i].time), array[i - 1].time);
	}
	malloc_strcpy(&array->last_msg, str);
	malloc_strcpy(&array->from, from);
	malloc_strcpy(&array->to, to);
	malloc_strcpy(&array->uh, uh);
	malloc_strcpy(&array->time, update_clock(GET_TIME));
}

int check_last_type(LastMsg *array, int size, char *from, char *uh)
{
int i;
	for (i = 0; i < size-1; i++)
	{
		if (array[i].from && array[i].uh && !my_stricmp(from, array[i].from) && !my_stricmp(uh, array[i].uh))
			return 1;
	}
	return 0;
}

int matchmcommand(char *origline,int count)
{
    int  startnum=0;
    int  endnum=0;
    char *tmpstr;
    char tmpbuf[BIG_BUFFER_SIZE];

	strncpy(tmpbuf,origline, BIG_BUFFER_SIZE-1);
	tmpstr=tmpbuf;
	if (*tmpstr=='*') return(1);
	while (tmpstr && *tmpstr) 
	{
		startnum=0;
		endnum=0;
		if (tmpstr && *tmpstr && *tmpstr=='-') 
		{
			while (tmpstr && *tmpstr && !isdigit(*tmpstr)) 
				tmpstr++;
			endnum=atoi(tmpstr);
			startnum=1;
			while (tmpstr && *tmpstr && isdigit(*tmpstr)) 
				tmpstr++;
		}
		else 
		{
			while (tmpstr && *tmpstr && !isdigit(*tmpstr)) 
				tmpstr++;
			startnum=atoi(tmpstr);
			while (tmpstr && *tmpstr && isdigit(*tmpstr)) 
				tmpstr++;
			if (tmpstr && *tmpstr && *tmpstr=='-') {
				while (tmpstr && *tmpstr && !isdigit(*tmpstr)) 
					tmpstr++;
				endnum=atoi(tmpstr);
				if (!endnum) 
					endnum=1000;
				while (tmpstr && *tmpstr && isdigit(*tmpstr)) 
					tmpstr++;
			}
		}
		if (count==startnum || (count>=startnum && count<=endnum)) 
			return(1);
	}
	if (count==startnum || (count>=startnum && count<=endnum)) 
		return(1);
	return(0);
}

ChannelList *prepare_command(int *active_server, char *channel, int need_op)
{
int server = 0;
ChannelList *chan = NULL;

	if (!channel && !get_current_channel_by_refnum(0))
	{
		
		if (need_op != 3) 
			not_on_a_channel(current_window);
		return NULL;
	}
	server = current_window->server;
	*active_server = server;
	if (!(chan = lookup_channel(channel? channel : get_current_channel_by_refnum(0), server, 0)))
	{
		
		if (need_op != 3) 
			not_on_a_channel(current_window);
		return NULL;
	}
	if (need_op == NEED_OP && chan && !chan->chop) 
	{
		
		error_not_opped(chan->channel);
		return NULL;
	}
	return chan;
}

char *make_channel (char *chan)
{
static char buffer[IRCD_BUFFER_SIZE+1];
	*buffer = 0;
        if (*chan != '#' && *chan != '&' && *chan != '+' && *chan != '!')
		snprintf(buffer, IRCD_BUFFER_SIZE-2, "#%s", chan);
	else
		strmcpy(buffer, chan, IRCD_BUFFER_SIZE-1);
	return buffer;
}

BUILT_IN_COMMAND(do_map)
{
	if (server_list[from_server].link_look == 0)
	{
		bitchsay("Generating irc server map");
		send_to_server("LINKS");
		server_list[from_server].link_look = 2;
	} else
		bitchsay("Wait until previous %s is done", server_list[from_server].link_look == 2? "MAP":"LLOOK");	
}

void add_to_irc_map(char *server1, char *distance)
{
irc_server *tmp, *insert, *prev;
int dist = 0;
	if (distance)
		dist = atoi(distance);
	tmp = (irc_server *) new_malloc(sizeof(irc_server));
	malloc_strcpy(&tmp->name, server1);
	tmp->hopcount = dist;
	if (!map)
	{
		map = tmp;
		return;
	}
	for (insert = map, prev = map; insert && insert->hopcount < dist; )
	{
		prev = insert;
		insert = insert->next;
	}	
	if (insert && insert->hopcount >= dist)
	{
		tmp->next = insert;
		if (insert == map)
			map = tmp;
		else
			prev->next = tmp;
	} else
		prev->next = tmp;
}

void show_server_map (void)
{
	int prevdist = 0;
	irc_server *tmp;
	char tmp1[80];
	char tmp2[BIG_BUFFER_SIZE+1];
#ifdef ONLY_STD_CHARS
	char *ascii="-> ";
#else
	char *ascii = "ÀÄ> ";
#endif			    
	if (map) prevdist = map->hopcount;

	for (tmp = map; tmp; tmp = map)
	{
		map = tmp->next;
		if (!tmp->hopcount || tmp->hopcount != prevdist)
			strmcpy(tmp1, convert_output_format("%K[%G$0%K]", "%d", tmp->hopcount), 79);
		else
			*tmp1 = 0;
		snprintf(tmp2, BIG_BUFFER_SIZE, "$G %%W$[-%d]1%%c $0 %s", tmp->hopcount*3, tmp1);
		put_it("%s", convert_output_format(tmp2, "%s %s", tmp->name, prevdist!=tmp->hopcount?ascii:empty_string));
		prevdist = tmp->hopcount;
		new_free(&tmp->name);
		new_free((char **)&tmp);
	}	
}

extern int timed_server (void *);
extern int in_timed_server;
void check_server_connect(int server)
{
	if (!get_int_var(AUTO_RECONNECT_VAR))
		return;
	if ((from_server == -1) || (!in_timed_server && server_list[from_server].last_msg + 50 < now))
	{
		add_timer(0, "", 10, 1, timed_server, m_strdup(zero), NULL, current_window);
		in_timed_server++;
	}
}

char *country(char *hostname)
{
typedef struct _domain {
char *code;
char *country;
} Domain;
Domain domain[] = {
	{"AD", "Andorra" },
	{"AE", "United Arab Emirates" },
	{"AF", "Afghanistan" },
	{"AG", "Antigua and Barbuda" },
	{"AI", "Anguilla" },
	{"AL", "Albania" },
	{"AM", "Armenia" },
	{"AN", "Netherlands Antilles" },
	{"AO", "Angola" },
	{"AQ", "Antarctica (pHEAR)" },
	{"AR", "Argentina" },
	{"AS", "American Samoa" },
	{"AT", "Austria" },
	{"AU", "Australia" },
	{"AW", "Aruba" },
	{"AZ", "Azerbaijan" },
	{"BA", "Bosnia and Herzegovina" },
	{"BB", "Barbados" },
	{"BD", "Bangladesh" },
	{"BE", "Belgium" },
	{"BF", "Burkina Faso" },
	{"BG", "Bulgaria" },
	{"BH", "Bahrain" },
	{"BI", "Burundi" },
	{"BJ", "Benin" },
	{"BM", "Bermuda" },
	{"BN", "Brunei Darussalam" },
	{"BO", "Bolivia" },
	{"BR", "Brazil" },
	{"BS", "Bahamas" },
	{"BT", "Bhutan" },
	{"BV", "Bouvet Island" },
	{"BW", "Botswana" },
	{"BY", "Belarus" },
	{"BZ", "Belize" },
	{"CA", "Canada (pHEAR)" },
	{"CC", "Cocos Islands" },
	{"CF", "Central African Republic" },
	{"CG", "Congo" },
	{"CH", "Switzerland" },
	{"CI", "Cote D'ivoire" },
	{"CK", "Cook Islands" },
	{"CL", "Chile" },
	{"CM", "Cameroon" },
	{"CN", "China" },
	{"CO", "Colombia" },
	{"CR", "Costa Rica" },
	{"CS", "Former Czechoslovakia" },
	{"CU", "Cuba" },
	{"CV", "Cape Verde" },
	{"CX", "Christmas Island" },
	{"CY", "Cyprus" },
	{"CZ", "Czech Republic" },
	{"DE", "Germany" },
	{"DJ", "Djibouti" },
	{"DK", "Denmark" },
	{"DM", "Dominica" },
	{"DO", "Dominican Republic" },
	{"DZ", "Algeria" },
	{"EC", "Ecuador" },
	{"EE", "Estonia" },
	{"EG", "Egypt" },
	{"EH", "Western Sahara" },
	{"ER", "Eritrea" },
	{"ES", "Spain" },
	{"ET", "Ethiopia" },
	{"FI", "Finland" },
	{"FJ", "Fiji" },
	{"FK", "Falkland Islands" },
	{"FM", "Micronesia" },
	{"FO", "Faroe Islands" },
	{"FR", "France" },
	{"FX", "France, Metropolitan" },
	{"GA", "Gabon" },
	{"GB", "Great Britain" },
	{"GD", "Grenada" },
	{"GE", "Georgia" },
	{"GF", "French Guiana" },
	{"GH", "Ghana" },
	{"GI", "Gibraltar" },
	{"GL", "Greenland" },
	{"GM", "Gambia" },
	{"GN", "Guinea" },
	{"GP", "Guadeloupe" },
	{"GQ", "Equatorial Guinea" },
	{"GR", "Greece" },
	{"GS", "S. Georgia and S. Sandwich Isles." },
	{"GT", "Guatemala" },
	{"GU", "Guam" },
	{"GW", "Guinea-Bissau" },
	{"GY", "Guyana" },
	{"HK", "Hong Kong" },
	{"HM", "Heard and McDonald Islands" },
	{"HN", "Honduras" },
	{"HR", "Croatia" },
	{"HT", "Haiti" },
	{"HU", "Hungary" },
	{"ID", "Indonesia" },
	{"IE", "Ireland" },
	{"IL", "Israel" },
	{"IN", "India" },
	{"IO", "British Indian Ocean Territory" },
	{"IQ", "Iraq" },
	{"IR", "Iran" },
	{"IS", "Iceland" },
	{"IT", "Italy" },
	{"JM", "Jamaica" },
	{"JO", "Jordan" },
	{"JP", "Japan" },
	{"KE", "Kenya" },
	{"KG", "Kyrgyzstan" },
	{"KH", "Cambodia" },
	{"KI", "Kiribati" },
	{"KM", "Comoros" },
	{"KN", "St. Kitts and Nevis" },
	{"KP", "North Korea" },
	{"KR", "South Korea" },
	{"KW", "Kuwait" },
	{"KY", "Cayman Islands" },
	{"KZ", "Kazakhstan" },
	{"LA", "Laos" },
	{"LB", "Lebanon" },
	{"LC", "Saint Lucia" },
	{"LI", "Liechtenstein" },
	{"LK", "Sri Lanka" },
	{"LR", "Liberia" },
	{"LS", "Lesotho" },
	{"LT", "Lithuania" },
	{"LU", "Luxembourg" },
	{"LV", "Latvia" },
	{"LY", "Libya" },
	{"MA", "Morocco" },
	{"MC", "Monaco" },
	{"MD", "Moldova" },
	{"MG", "Madagascar" },
	{"MH", "Marshall Islands" },
	{"MK", "Macedonia" },
	{"ML", "Mali" },
	{"MM", "Myanmar" },
	{"MN", "Mongolia" },
	{"MO", "Macau" },
	{"MP", "Northern Mariana Islands" },
	{"MQ", "Martinique" },
	{"MR", "Mauritania" },
	{"MS", "Montserrat" },
	{"MT", "Malta" },
	{"MU", "Mauritius" },
	{"MV", "Maldives" },
	{"MW", "Malawi" },
	{"MX", "Mexico" },
	{"MY", "Malaysia" },
	{"MZ", "Mozambique" },
	{"NA", "Namibia" },
	{"NC", "New Caledonia" },
	{"NE", "Niger" },
	{"NF", "Norfolk Island" },
	{"NG", "Nigeria" },
	{"NI", "Nicaragua" },
	{"NL", "Netherlands" },
	{"NO", "Norway" },
	{"NP", "Nepal" },
	{"NR", "Nauru" },
	{"NT", "Neutral Zone (pHEAR)" },
	{"NU", "Niue" },
	{"NZ", "New Zealand" },
	{"OM", "Oman" },
	{"PA", "Panama" },
	{"PE", "Peru" },
	{"PF", "French Polynesia" },
	{"PG", "Papua New Guinea" },
	{"PH", "Philippines" },
	{"PK", "Pakistan" },
	{"PL", "Poland" },
	{"PM", "St. Pierre and Miquelon" },
	{"PN", "Pitcairn" },
	{"PR", "Puerto Rico" },
	{"PT", "Portugal" },
	{"PW", "Palau" },
	{"PY", "Paraguay" },
	{"QA", "Qatar" },
	{"RE", "Reunion" },
	{"RO", "Romania" },
	{"RU", "Russian Federation (pHEAR)" },
	{"RW", "Rwanda" },
	{"SA", "Saudi Arabia" },
	{"Sb", "Solomon Islands" },
	{"SC", "Seychelles" },
	{"SD", "Sudan" },
	{"SE", "Sweden" },
	{"SG", "Singapore" },
	{"SH", "St. Helena" },
	{"SI", "Slovenia" },
	{"SJ", "Svalbard and Jan Mayen Islands" },
	{"SK", "Slovak Republic" },
	{"SL", "Sierra Leone" },
	{"SM", "San Marino" },
	{"SN", "Senegal" },
	{"SO", "Somalia" },
	{"SR", "Suriname" },
	{"ST", "Sao Tome and Principe" },
	{"SU", "Former USSR (pHEAR)" },
	{"SV", "El Salvador" },
	{"SY", "Syria" },
	{"SZ", "Swaziland" },
	{"TC", "Turks and Caicos Islands" },
	{"TD", "Chad" },
	{"TF", "French Southern Territories" },
	{"TG", "Togo" },
	{"TH", "Thailand" },
	{"TJ", "Tajikistan" },
	{"TK", "Tokelau" },
	{"TM", "Turkmenistan" },
	{"TN", "Tunisia" },
	{"TO", "Tonga" },
	{"TP", "East Timor" },
	{"TR", "Turkey" },
	{"TT", "Trinidad and Tobago" },
	{"TV", "Tuvalu" },
	{"TW", "Taiwan" },
	{"TZ", "Tanzania" },
	{"UA", "Ukraine" },
	{"UG", "Uganda" },
	{"UK", "United Kingdom" },
	{"UM", "US Minor Outlying Islands" },
	{"US", "United States of America" },
	{"UY", "Uruguay" },
	{"UZ", "Uzbekistan" },
	{"VA", "Vatican City State" },
	{"VC", "St. Vincent and the grenadines" },
	{"VE", "Venezuela" },
	{"VG", "British Virgin Islands" },
	{"VI", "US Virgin Islands" },
	{"VN", "Vietnam" },
	{"VU", "Vanuatu" },
	{"WF", "Wallis and Futuna Islands" },
	{"WS", "Samoa" },
	{"YE", "Yemen" },
	{"YT", "Mayotte" },
	{"YU", "Yugoslavia" },
	{"ZA", "South Africa" },
	{"ZM", "Zambia" },
	{"ZR", "Zaire" },
	{"ZW", "Zimbabwe" },
	{"COM", "Internic Commercial" },
	{"EDU", "Educational Institution" },
	{"GOV", "Government" },
	{"INT", "International" },
	{"MIL", "Military" },
	{"NET", "Internic Network" },
	{"ORG", "Internic Non-Profit Organization" },
	{"RPA", "Old School ARPAnet" },
	{"ATO", "Nato Fiel" },
	{"MED", "United States Medical" },
	{"ARPA", "Reverse DNS" },
	{NULL, NULL}
};
char *p;
int i = 0;
	if (!hostname || !*hostname || isdigit(hostname[strlen(hostname)-1]))
		return "unknown";
	if ((p = strrchr(hostname, '.')))
		p++;
	else
		p = hostname;
	for (i = 0; domain[i].code; i++)
		if (!my_stricmp(p, domain[i].code))
			return domain[i].country;
	return "unknown";
}

