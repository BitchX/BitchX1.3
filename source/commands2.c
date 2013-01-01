/*
 * Routines within this files are Copyright Colten Edwards 1996
 * Aka panasync on irc.
 * Thanks to Shiek and Flier for helpful hints and suggestions. As well 
 * as code in some cases.
 */
 
#include "irc.h"
static char cvsrevision[] = "$Id: commands2.c 163 2012-04-30 08:05:04Z keaston $";
CVS_REVISION(commands2_c)
#include "struct.h"
#include <sys/stat.h>

#if defined(_ALL_SOURCE) || defined(__EMX__) || defined(__QNX__) || defined(__FreeBSD__)
#include <termios.h>
#else
#include <sys/termios.h>
#endif
#include <sys/ioctl.h>


#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

#include <unistd.h>
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#include <sys/wait.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif


#include "parse.h"
#include "server.h"
#include "chelp.h"
#include "commands.h"
#include "encrypt.h"
#include "vars.h"
#include "ircaux.h"
#include "lastlog.h"
#include "log.h"
#include "window.h"
#include "screen.h"
#include "ircterm.h"
#include "who.h"
#include "hook.h"
#include "input.h"
#include "ignore.h"
#include "keys.h"
#include "names.h"
#include "alias.h"
#include "array.h"
#include "history.h"
#include "funny.h"
#include "ctcp.h"
#include "dcc.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "if.h"
#include "help.h"
#include "stack.h"
#include "queue.h"
#include "timer.h"
#include "list.h"
#include "misc.h"
#include "userlist.h"
#include "whowas.h"
#include "window.h"
#include "flood.h"
#include "hash2.h"
#include "notice.h"
#include "who.h"
#include "gui.h"
#define MAIN_SOURCE
#include "modval.h"

void    toggle_reverse_status(Window *, char *, int);

#ifdef GUI
char *msgtext = NULL;

#ifndef GLOBAL_MENU_ID
#define GLOBAL_MENU_ID 5000
#endif

MenuStruct *morigin = NULL;
int globalmenuid=GLOBAL_MENU_ID;
#endif

extern char attach_ttyname[];

char *org_nick = NULL;

static int delay_gain_nick (void *, char *);

BUILT_IN_COMMAND(nwhois)
{
ChannelList *tmp = NULL;
NickList *user = NULL;
int once = 0;
char *nick = NULL;
int mem_use = 0, max_mem = 0;
int server = from_server;
char *arg = NULL, *t = NULL;
int all = 0;
int to_chan = 0;

	if (!(tmp = get_server_channels(server)))
	{
		bitchsay("No Channel or no server");
		return;
	}

	if (args && *args)
		t = arg = m_strdup(args);
	else
	{
		all = 1;
		nick = get_current_channel_by_refnum(0);
	}		
	
	while (all || (nick = next_arg(arg, &arg)))
	{
		while (tmp)
		{
			if ((all || (nick && (to_chan = is_channel(nick)))) && my_stricmp(tmp->channel, nick))
			{
				tmp = tmp->next;
				continue;
			} 
			for (user = next_nicklist(tmp, NULL); user; user = next_nicklist(tmp, user))
			{
				if (all || to_chan || wild_match(nick, user->nick))
				{
					if (!once)
					{
						put_it("%s", convert_output_format("%G$[7]0    @ v  UL:               SL:    +o   -o   +b   -b  kicks  nicks   pub", "%s", (all || to_chan)?/*"Channel"*/tmp->channel:"Nick"));
#ifdef ONLY_STD_CHARS
						put_it("%s", convert_output_format("%B------------------------------------------------------------------------------", NULL, NULL));
#else
						put_it("%s", convert_output_format("%BÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ", NULL, NULL));
#endif
						once++;
					}
					put_it("%s", convert_output_format("$[10]0 %C$1 $2%w  %B$[16]3  %R$[6]4  %M$[4]5 $[4]6 $[4]7 $[4]8$[-4]9  $[-4]10 $[-6]11",
						"%s %c %c %s %u %u %u %u %u %u %u %u",
						to_chan ? tmp->channel: user->nick, 
						nick_isop(user)? '@':'ÿ',nick_isvoice(user)?'v':'ÿ',
#ifdef WANT_USERLIST
						user->userlist?convert_flags(user->userlist->flags):"none", 
						user->shitlist?user->shitlist->level: 0,
#else
						"none", 0,
#endif
						user->stat_ops,user->stat_dops, user->stat_bans, user->stat_unbans, 
						user->stat_kicks, user->stat_nicks, user->stat_pub));
					mem_use += sizeof(NickList) + (user->userlist ? sizeof(UserList) : 0) + (user->shitlist ? sizeof(ShitList):0);
					if (max_mem < mem_use)
						max_mem = mem_use;
					if (!all)
						break;
				}
			}				
			tmp = tmp->next;
		}
		if (once && nick)
			put_it("%s", convert_output_format("$G End /NWhoIs list for $1- [Mem Usage: $0 bytes per chan]", "%d %s", max_mem, (all||to_chan) ? "All":nick));
		if (all)
			break;
	}
	if (!all && !once)
		put_it("%s", convert_output_format("$G No such nick or channel", NULL, NULL));
	new_free(&t);
}

BUILT_IN_COMMAND(whowas)
{
	
	if (!command)
		show_whowas();
	else
		show_wholeft(NULL);
}

BUILT_IN_COMMAND(add_ban_word)
{
char *word = NULL;
char *chan = NULL;
WordKickList *new;

	
	if (args && *args)
	{
		chan = next_arg(args, &args);
		if ((!is_channel(chan) && (chan && *chan != '*')) || !args || !*args)
			return;
		if (!command)
		{
			new = (WordKickList *)find_in_list((List **)&ban_words, args, 0);
			if (!ban_words || !new || (new && !wild_match(new->channel, chan)))
			{
				new = (WordKickList *) new_malloc(sizeof(WordKickList));
				malloc_strcpy(&new->channel, chan);
				malloc_strcpy(&new->string, args);
				add_to_list((List **)&ban_words, (List *)new);
				bitchsay("Added %s to %s Banned Word List", new->string, new->channel);
			} else bitchsay("[%s] is already in the list for channel %s", args, chan);
		}
		else
		{
			int count = 0;
			malloc_strcpy(&word, args);
			while ((new = (WordKickList *) removewild_from_list((List **)&ban_words, word)))
			{
				bitchsay("Removed %s Banned Word [%s]", new->channel, new->string);
				new_free(&new->channel);
				new_free(&new->string);
				new_free((char **)&new);
				count++;
			}
			if (!count)
				bitchsay("Banned Word %s not found.", word);
			new_free(&word);
		}
	}
}

BUILT_IN_COMMAND(show_word_kick)
{
WordKickList *new;

	
	if (ban_words) 
	{
		put_it("%14s %40s", "Channel", "Banned Word(s)");
		for (new = ban_words; new; new = new->next)
			put_it("%-14s %40s", new->channel, new->string);
	} else
		bitchsay("No Banned Words on list");
}

void save_banwords(FILE *outfile)
{
	int count = 0;
	WordKickList *new;

	
	if (ban_words)
	{
		fprintf(outfile, "# %s Banned Words\n", version);
		for (new = ban_words; new; new = new->next)
		{
			fprintf(outfile, "BANWORD %s %s\n", new->channel, new->string);
			count++;
		}
	}
	if (count && do_hook(SAVEFILE_LIST,"BanWords %d", count))
		bitchsay("Saved %d Banned Words List", count);
}

#if 0
static char *recreate_saymode(ChannelList *chan)
{
static char mode_str[] = "iklmnpsta";
static char buffer[BIG_BUFFER_SIZE/4+1];
int mode_pos = 0;
int mode;
char *s;

	
        buffer[0] = '\0';
        s = buffer;
        mode = chan->modelock_val;
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
		*s = '\0';
	return buffer;
}
#endif

int check_mode_lock(char *channel, char *mode_list, int server)
{
ChannelList *chan;
char buffer[BIG_BUFFER_SIZE+1];
	
	if ((chan = lookup_channel(channel, server, 0)) && chan->modelock_key)
	{
		char *newmode;
		char *modelock = NULL;
		char *new_mode_list = NULL;
		char *save, *save1;
		char *args = NULL, *args1 = NULL;
		int add = 0;
		memset(buffer, 0, sizeof(buffer));
		
		
		malloc_strcpy(&modelock, chan->modelock_key);
		malloc_strcpy(&new_mode_list, mode_list);
		save1 = new_mode_list;
		save = modelock;
		new_mode_list = next_arg(new_mode_list, &args1);
		modelock = next_arg(modelock, &args);		

		while (*modelock)
		{
			newmode = strchr(mode_list, *modelock);
			switch(*modelock)
			{
				case '+':
					add = 1;
					break;
				case '-':
					add = 0;
					break;
				case 'k':
					if (newmode)
					{
						if (add)
						{
							char *key;
							key = next_arg(args1, &args1);
							if (chan->key)
							{
								strcat(buffer, "-k " );
								strcat(buffer, chan->key);
							}
							key = next_arg(args, &args);
							if (key)
							{
								strcat(buffer, " +k ");
								strcat(buffer, key);
								strcat(buffer, space);
							}
						}
						else
						{
							if (!chan->key)
								break;
							strcat(buffer, "-k ");
							strcat(buffer, chan->key);
						}
						strcat(buffer, space);
					}
					break;
				case 'l':
					if (newmode)
					{
						if (add)
						{
							int limit = 0;
							if (args && *args)
								limit = strtoul(args, &args, 10);
							if (limit > 0)
							{
								strcat(buffer, "+l ");
								strcat(buffer, ltoa(limit));
								strcat(buffer, space);
							}
						}
						else
						{
							chan->limit = 0;
							strcat(buffer, "-l");
						}
					}
					break;
				default:
					if (newmode)
					{
						if (add)
						{
							strcat(buffer, "+");
						}
						else
						{
							strcat(buffer, "-");
						}
						buffer[strlen(buffer)] = *modelock;
						strcat(buffer, space);
					}
					break;
			}
			modelock++;
		}
		if (chan && chan->have_op && buffer)
			send_to_server("MODE %s %s", chan->channel, buffer);
		new_free(&save);
		new_free(&save1);
		return 1;
	}
	return 0;
}

BUILT_IN_COMMAND(mode_lock)
{
ChannelList *chan;
int i = 0;
char *channel = NULL;
u_long mode = 0;
int value = 0;

char *t = NULL;
char *key = NULL;

	
	if (command && *command && !strcmp(command, "ModeLock"))
	{
		if (!args || !*args)
			return;
		t = next_arg(args, &args);
		if (is_channel(t))
		{
			channel = t;		
			t = next_arg(args, &args);
		} else
			channel = get_current_channel_by_refnum(0);
		if (!t || !*t)
		{
			bitchsay("No Mode Specified");
			return;
		}
		if ((chan = lookup_channel(channel, from_server, 0)))
		{
			char valid_mode[BIG_BUFFER_SIZE + 1];
			char *buffer = NULL;
			int limit = -1;
			int add = 0;
			memset(valid_mode, 0, sizeof(valid_mode));
			while (*t && i < BIG_BUFFER_SIZE)
			{
				switch(*t)
				{
					case '+':
						add = 1;
						valid_mode[i++] = '+';
						break;
					case '-':
						add = 0;
						valid_mode[i++] = '-';
						break;
					case 'm':
						value = MODE_MODERATED;
						valid_mode[i++] = *t;
						break;
					case 'a':
						value = MODE_ANONYMOUS;
						valid_mode[i++] = *t;
						break;
					case 'i':
						value = MODE_INVITE;
						valid_mode[i++] = *t;
						break;
					case 'n':
						value = MODE_MSGS;
						valid_mode[i++] = *t;
						break;
					case 's':
						value = MODE_SECRET;
						valid_mode[i++] = *t;
						break;
					case 't':
						value = MODE_TOPIC;
						valid_mode[i++] = *t;
						break;
					case 'p':
						value = MODE_PRIVATE;
						valid_mode[i++] = *t;
						break;
					case 'k':
						value = MODE_KEY;
						valid_mode[i++] = *t;
						if (add)
							key = next_arg(args, &args);
						else
							key = NULL;
						break;
						
					case 'l':
						valid_mode[i++] = *t;
						value = MODE_LIMIT;
						if (add)
						{
							if (args && *args)
								limit = strtoul(args, &args, 10);
						}
						else
							limit = 0;
						break;
					default:
						break;
				}
				if (add)
					mode |= value;
				else
					mode &= ~value;
		
				t++;
			}
			chan->modelock_val = mode;
			malloc_strcpy(&buffer, valid_mode);
			if (key && *key)
			{
				malloc_strcat(&buffer, space);
				malloc_strcat(&buffer, key);
			}
			if (limit > 0)
			{
				malloc_strcat(&buffer, space);
				malloc_strcat(&buffer, ltoa(limit));
			}
			if (*buffer)
			{
				malloc_strcpy(&chan->modelock_key, buffer);
				bitchsay("%s Mode Locked at: [%s] %s", chan->channel, buffer, (chan->limit > 0) ? "Users":empty_string);
			} else
				bitchsay("Invalid Mode for [%s]", chan->channel);
		} else
			bitchsay("No Such Channel");
	}
	else if (command && *command && !my_stricmp(command, "ClearLock"))
	{

		if (!args || !*args)
			return;
		t = next_arg(args, &args);
		if (t && *t && *t != '*')
		{
			if ((chan = lookup_channel(t, from_server, 0)))
			{
				new_free(&chan->modelock_key);
				chan->modelock_val = 0;
				bitchsay("Cleared %s Mode Lock", chan->channel);
			} else
				bitchsay("No such Channel [%s]", t);
		} 
		else if (t && *t && *t == '*')
		{
			for (chan = get_server_channels(from_server); chan; chan = chan->next)
			{
				new_free(&chan->modelock_key);
				chan->modelock_val = 0;
			}
			bitchsay("Cleared All Channel Mode Locks");
		}
	}
	else
	{
		for (i = 0; i < server_list_size(); i++ )
		{
			for (chan = get_server_channels(from_server); chan; chan = chan->next)
				bitchsay("Lock on %s: %s", chan->channel, chan->modelock_key ? chan->modelock_key : "none");
		}
	}
}

BUILT_IN_COMMAND(randomnick)
{
char *prefix = NULL, *p;
int count = 1;
int len = 0;

	
	while ((p = next_arg(args, &args)))
	{
		if (isdigit((unsigned char)*p))
			count = atol(p);
		else
			prefix = p;
	}
	if (prefix && (len = strlen(prefix)))
	{
		if (len > 5)
			*(prefix + 6) = 0;
	}
	while (count > 0)
	{
		send_to_server("NICK %s%s", prefix?prefix:empty_string, random_str(3,9-len));
		count--;
	}
}

BUILT_IN_COMMAND(topic_lock)
{
ChannelList *chan;
char *t, *channel;

	
	if (!args || !*args)
		return;
	t = next_arg(args, &args);
	if (args && *args)
	{
		channel = make_channel(t);
		if (!channel || !is_channel(channel))
			return;
		t = next_arg(args, &args);
	} else 
		channel = get_current_channel_by_refnum(0);
		
	if ((chan = lookup_channel(channel, from_server, 0)))
	{
		if (t && *t && !my_stricmp(t, "ON"))
		{
			chan->topic_lock = 1;
		}
		else if (t && *t && !my_strnicmp(t, "OF", 2))
			chan->topic_lock = 0;
		bitchsay("Topic lock for [%s] - %s", chan->channel, on_off(chan->topic_lock));
	}
}

BUILT_IN_COMMAND(sping)
{
	char *sname;
	
	if (!args || !*args)
	{
		Sping *tmp;
		int count = 0;
		tmp = get_server_sping(from_server, NULL);
		for ( ;tmp; tmp = tmp->next)
		{
			if (!count)
				say("Waiting Sping");
			say("%d\t%s", ++count, tmp->sname);
		}
		return;
	}
	else if (!my_strnicmp(args, "-CLEAR", 3))
		clear_server_sping(from_server, NULL);
	else
	{
		Sping *tmp;
		reset_display_target();
		bitchsay("Sent server ping to [\002%s\002]", args);
		while ((sname = next_arg(args, &args)))
		{
			if (*sname == '.')
				if (!(sname = get_server_itsname(from_server)))
					sname = get_server_name(from_server);
			if (!wild_match("*.*", sname))
			{
				bitchsay("%s is not a server", sname);
				continue;
			}
			tmp = new_malloc(sizeof(Sping));
			tmp->sname = m_strdup(sname);
#ifdef HAVE_GETTIMEOFDAY
			gettimeofday(&tmp->in_sping, NULL);
			set_server_sping(from_server, tmp);
#else
			tmp->in_sping = now;
			set_server_sping(from_server, tmp);
#endif
			if (!my_stricmp(sname, get_server_name(from_server)) || !my_stricmp(sname, get_server_itsname(from_server)))
#ifdef HAVE_GETTIMEOFDAY
				send_to_server("PING LAG%ld.%ld :%s", tmp->in_sping.tv_sec, tmp->in_sping.tv_usec, sname);
#else
				send_to_server("PING LAG%ld :%s", now, sname);
#endif
			else
				send_to_server("PING %s :%s", 
					get_server_itsname(from_server) ? 
					get_server_itsname(from_server) : 
					get_server_name(from_server), sname);
		}
		
	}
#if 0
#ifdef HAVE_GETTIMEOFDAY
	struct timeval in_sping = {0};
#endif
	if (!servern || !*servern)
		if (!(servern = get_server_itsname(from_server)))
			servern = get_server_name(from_server);
	
	if (servern && *servern && wild_match("*.*", servern))
	{
#ifdef HAVE_GETTIMEOFDAY
			gettimeofday(&in_sping, NULL);
			send_to_server("PING LAG%ld.%ld :%s", in_sping.tv_sec, in_sping.tv_usec, servern);
#else
			send_to_server("PING LAG%ld :%s", now, servern);
#endif
		}
		else
		{
#ifdef HAVE_GETTIMEOFDAY
			gettimeofday(&in_sping, NULL);
			set_server_sping(from_server, in_sping);
#else
			set_server_sping(from_server, now);
#endif
			send_to_server("PING %s :%s", get_server_itsname(from_server), servern);
		}
	}
#endif
}

BUILT_IN_COMMAND(tog_fprot)
{
static int here = 0;

	
	if (args && *args)
	{
		if (!my_stricmp(args, "ON"))
			here = 0;
		else if (!my_stricmp(args, "OFF"))
			here = 1;
		else
			return;
	}		

	if (here)
	{
		set_int_var(CTCP_FLOOD_PROTECTION_VAR, 0);
		set_int_var(FLOOD_PROTECTION_VAR, 0);
		here = 0;
	} else
	{
		set_int_var(CTCP_FLOOD_PROTECTION_VAR, 1);
		set_int_var(FLOOD_PROTECTION_VAR, 1);
		here = 1;
	}
	bitchsay("Toggled flood protection - [%s]", on_off(here));
}

BUILT_IN_COMMAND(do_toggle)
{
#ifndef BITCHX_LITE
if (!args || !*args)
{
#ifdef ONLY_STD_CHARS

put_it("%s", convert_output_format("%G-----------%K[ %WBitchX %wToggles %K]%G----------------------------------------------", NULL));
put_it("%s", convert_output_format("%G|   %Cauto_ns%clookup %K[%W$[-3]0%K]    %Cctcp_f%clood_protection %K[%W$[-3]1%K]    %Cbeep%c        %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(AUTO_NSLOOKUP_VAR)), on_off(get_int_var(CTCP_FLOOD_PROTECTION_VAR)), on_off(get_int_var(BEEP_VAR))));
put_it("%s", convert_output_format("%G|   %Cpub%cflood      %K[%W$[-3]0%K]    %Cflood_p%crotection      %K[%W$[-3]1%K]    %Ckickf%clood   %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(PUBFLOOD_VAR)), on_off(get_int_var(FLOOD_PROTECTION_VAR)), on_off(get_int_var(KICKFLOOD_VAR))));
put_it("%s", convert_output_format("%G|   %Cdcc_a%cutoget   %K[%W$[-3]0%K]    %Cflood_k%cick            %K[%W$[-3]1%K]    %Cmsg%clog      %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(DCC_AUTOGET_VAR)), on_off(get_int_var(FLOOD_KICK_VAR)), on_off(get_int_var(MSGLOG_VAR))));
put_it("%s", convert_output_format("%G|   %Cll%cook         %K[%W$[-3]0%K]    %Cdeop%cflood             %K[%W$[-3]1%K]    %Cjoin%cflood   %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(LLOOK_VAR)), on_off(get_int_var(DEOPFLOOD_VAR)), on_off(get_int_var(JOINFLOOD_VAR))));
put_it("%s", convert_output_format("%G|   %Cauto_w%chowas   %K[%W$[-3]0%K]    %Cverb%cose_ctcp          %K[%W$[-3]1%K]    %Cnickfl%cood   %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(AUTO_WHOWAS_VAR)), on_off(get_int_var(CTCP_VERBOSE_VAR)), on_off(get_int_var(NICKFLOOD_VAR))));
put_it("%s", convert_output_format("%G|   %Ccl%coak         %K[%W$[-3]0%K]    %Coper%cview              %K[%W$[-3]1%K]    %Cshit%clist    %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(CLOAK_VAR)), on_off(get_int_var(OV_VAR)), on_off(get_int_var(SHITLIST_VAR))));
put_it("%s", convert_output_format("%G|   %Ckick_o%cps      %K[%W$[-3]0%K]    %Cannoy%c_kick            %K[%W$[-3]1%K]    %Cuser%clist    %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(KICK_OPS_VAR)), on_off(get_int_var(ANNOY_KICK_VAR)), on_off(get_int_var(USERLIST_VAR))));
put_it("%s", convert_output_format("%G|   %Chack%cing       %K[%W$[-3]0%K]    %Cnick_c%completion       %K[%W$[-3]1%K]    %Cauto_r%cejoin %K[%W$[-3]2%K]","%s %s %s", on_off(get_int_var(HACKING_VAR)),on_off(get_int_var(NICK_COMPLETION_VAR)), on_off((get_int_var(AUTO_REJOIN_VAR)?1:0)) ));
put_it("%s", convert_output_format("%K|   %Caop           %K[%W$[-3]0%K]    %Cauto_aw%cay             %K[%W$[-3]1%K]    %Cauto_rec%conn %K[%W$[-3]2%K]", "%s %s %s", on_off((get_int_var(AOP_VAR))),on_off((get_int_var(AUTO_AWAY_VAR))), on_off(get_int_var(AUTO_RECONNECT_VAR))));
put_it("%s", convert_output_format("%K|   %Cbitch         %K[%W$[-3]0%K]    %Cdcc_f%cast              %K[%W$[-3]1%K]    %Ckick_if_ban %K[%W$[-3]2%K]", "%s %s %s", on_off((get_int_var(BITCH_VAR))),on_off((get_int_var(DCC_FAST_VAR))), on_off(get_int_var(KICK_IF_BANNED_VAR))));
put_it("%s", convert_output_format("%g|   %Cftp_g%crab      %K[%W$[-3]0%K]    %Cmircs                 %K[%W$[-3]1%K]    %Chttp%c_grab   %K[%W$[-3]2%K]", "%s %s", on_off((get_int_var(FTP_GRAB_VAR))),on_off((get_int_var(MIRCS_VAR))),on_off((get_int_var(HTTP_GRAB_VAR))) ));
put_it("%s", convert_output_format("%G|   %Cdisp%clay_ansi  %K[%W$[-3]0%K]    %Wtype /toggle <setting>         %Clog         %K[%W$[-3]1%K]", "%s %s", on_off((get_int_var(DISPLAY_ANSI_VAR))),on_off((get_int_var(LOG_VAR))) ));

#else

put_it("%s", convert_output_format("%GÚÄÄÄÄÄ---%gÄ%G-%K[ %WBitchX %wToggles %K]-%gÄÄ%G-%gÄÄÄÄÄÄ---%KÄ%g--%KÄÄ%g-%KÄÄÄÄÄÄÄÄÄ--- --  - --- -- -", NULL));
put_it("%s", convert_output_format("%G³   %Cauto_ns%clookup %K[%W$[-3]0%K]    %Cctcp_f%clood_protection %K[%W$[-3]1%K]    %Cbeep%c        %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(AUTO_NSLOOKUP_VAR)), on_off(get_int_var(CTCP_FLOOD_PROTECTION_VAR)), on_off(get_int_var(BEEP_VAR))));
put_it("%s", convert_output_format("%G³   %Cpub%cflood      %K[%W$[-3]0%K]    %Cflood_p%crotection      %K[%W$[-3]1%K]    %Ckickf%clood   %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(PUBFLOOD_VAR)), on_off(get_int_var(FLOOD_PROTECTION_VAR)), on_off(get_int_var(KICKFLOOD_VAR))));
put_it("%s", convert_output_format("%g³   %Cdcc_a%cutoget   %K[%W$[-3]0%K]    %Cflood_k%cick            %K[%W$[-3]1%K]    %Cmsg%clog      %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(DCC_AUTOGET_VAR)), on_off(get_int_var(FLOOD_KICK_VAR)), on_off(get_int_var(MSGLOG_VAR))));
put_it("%s", convert_output_format("%G³   %Cll%cook         %K[%W$[-3]0%K]    %Cdeop%cflood             %K[%W$[-3]1%K]    %Cjoin%cflood   %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(LLOOK_VAR)), on_off(get_int_var(DEOPFLOOD_VAR)), on_off(get_int_var(JOINFLOOD_VAR))));
put_it("%s", convert_output_format("%g|   %Cauto_w%chowas   %K[%W$[-3]0%K]    %Cverb%cose_ctcp          %K[%W$[-3]1%K]    %Cnickfl%cood   %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(AUTO_WHOWAS_VAR)), on_off(get_int_var(CTCP_VERBOSE_VAR)), on_off(get_int_var(NICKFLOOD_VAR))));
put_it("%s", convert_output_format("%G:   %Ccl%coak         %K[%W$[-3]0%K]    %Coper%cview              %K[%W$[-3]1%K]    %Cshit%clist    %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(CLOAK_VAR)), on_off(get_int_var(OV_VAR)), on_off(get_int_var(SHITLIST_VAR))));
put_it("%s", convert_output_format("%G:   %Ckick_o%cps      %K[%W$[-3]0%K]    %Cannoy%c_kick            %K[%W$[-3]1%K]    %Cuser%clist    %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(KICK_OPS_VAR)), on_off(get_int_var(ANNOY_KICK_VAR)), on_off(get_int_var(USERLIST_VAR))));
put_it("%s", convert_output_format("%K|   %Chack%cing       %K[%W$[-3]0%K]    %Cnick_c%completion       %K[%W$[-3]1%K]    %Cauto_rej%coin %K[%W$[-3]2%K]", "%s %s %s", on_off(get_int_var(HACKING_VAR)),on_off(get_int_var(NICK_COMPLETION_VAR)), on_off((get_int_var(AUTO_REJOIN_VAR)?1:0)) ));
put_it("%s", convert_output_format("%K:   %Caop           %K[%W$[-3]0%K]    %Cauto_aw%cay             %K[%W$[-3]1%K]    %Cauto_rec%conn %K[%W$[-3]2%K]", "%s %s %s", on_off((get_int_var(AOP_VAR)?1:0)),on_off((get_int_var(AUTO_AWAY_VAR))), on_off(get_int_var(AUTO_RECONNECT_VAR))));
put_it("%s", convert_output_format("%K:   %Cbitch         %K[%W$[-3]0%K]    %Cdcc_f%cast              %K[%W$[-3]1%K]    %Ckick_if_ban %K[%W$[-3]2%K]", "%s %s %s", on_off((get_int_var(BITCH_VAR))),on_off((get_int_var(DCC_FAST_VAR))), on_off(get_int_var(KICK_IF_BANNED_VAR))));
put_it("%s", convert_output_format("%g:   %Cftp_g%crab      %K[%W$[-3]0%K]    %Cmircs                 %K[%W$[-3]1%K]    %Chttp%c_grab   %K[%W$[-3]2%K]", "%s %s %s", on_off((get_int_var(FTP_GRAB_VAR))),on_off((get_int_var(MIRCS_VAR))),on_off((get_int_var(HTTP_GRAB_VAR))) ));
put_it("%s", convert_output_format("%K:   %Cdisp%clay_ansi  %K[%W$[-3]0%K]    %Ctimestamp             %K[%W$[-3]1%K]    %Clog         %K[%W$[-3]2%K]", "%s %s %s", on_off((get_int_var(DISPLAY_ANSI_VAR))),on_off((get_int_var(TIMESTAMP_VAR))), on_off((get_int_var(LOG_VAR))) ));
put_it("%s", convert_output_format("%K:                             %Wtype /toggle <setting> ", NULL));

#endif
} 
else
{
	char *arg;
	while ((arg = next_arg(args, &args)))
	{
		int var = -1;
		char *str = NULL;
		upper(arg);
		
		if (!my_strnicmp(arg, "auto_nslookup", 7))
		{
			var = AUTO_NSLOOKUP_VAR;
			str = "$G %cToggled %GAuto-NSlookup %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "pubflood", 3))
		{
			var = PUBFLOOD_VAR;
			str = "$G %cToggled %GPub Flood %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "dcc_autoget", 5))
		{
			var = DCC_AUTOGET_VAR;
			str = "$G %cToggled %GDCC Auto Get %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "llook", 2))
		{
			var = LLOOK_VAR;
			str = "$G %cToggled %GLink Look %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "auto_whowas", 6))
		{
			var = AUTO_WHOWAS_VAR;
			str = "$G %cToggled %GAuto-WhoWas %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "cloak", 2))
		{
			var = CLOAK_VAR;
			str = "$G %cToggled %GCloaking %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "kick_ops", 6))
		{
			var = KICK_OPS_VAR;
			str = "$G %cToggled %GKick Ops %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "ctcp_flood_protection", 6))
		{
			var = CTCP_FLOOD_PROTECTION_VAR;
			str = "$G %cToggled %GCtcp Flood Protection %K[%W$[-3]0%K]","%s";
		} else if (!my_strnicmp(arg, "flood_protection",7))
		{
			var = FLOOD_PROTECTION_VAR;
			str = "$G %cToggled %GFlood Protection %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "flood_kick", 7))
		{
			var = FLOOD_KICK_VAR;
			str = "$G %cToggled %GFlood Kicks %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "deopflood",4))
		{
			var = DEOPFLOOD_VAR;
			str = "$G %cToggled %GDeOp Flood %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "verbose_ctcp",4))
		{
			var = CTCP_VERBOSE_VAR;
			str = "$G %cToggled %GVerbose CTCP %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "operview", 4))
		{
			int ov_mode = get_int_var(OV_VAR);
			var = OV_VAR;
			setup_ov_mode(ov_mode, 0, current_window->log);
			str = "$G %cToggled %GOper View %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "annoy_kick",4))
		{
			var = ANNOY_KICK_VAR;
			str = "$G %cToggled %GAnnoy Kicks %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "beep",4))
		{
			var = BEEP_VAR;
			str = "$G %cToggled %GBeep %K[%W$[-3]0%K]";

		} else if (!my_strnicmp(arg, "kickflood",5))
		{
			var = KICKFLOOD_VAR;
			str = "$G %cToggled %GKick Flood %K[%W$[-3]0%K]","%s";
		} else if (!my_strnicmp(arg, "msglog", 3))
		{
			var = MSGLOG_VAR;
			str = "$G %cToggled %GMSG log %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "joinflood", 4))
		{
			var = JOINFLOOD_VAR;
			str = "$G %cToggled %GJoin Flood %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "nickflood", 6))
		{
			var = NICKFLOOD_VAR;
			str = "$G %cToggled %GNick Flood %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "shitlist", 4))
		{
			var = SHITLIST_VAR;
			str = "$G %cToggled %GShitList %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "userlist", 4))
		{
			var = USERLIST_VAR;
			str = "$G %cToggled %GUserList %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "hacking", 4))
		{
			var = HACKING_VAR;
			str = "$G %cToggled %GHacking %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "auto_rejoin", 8))
		{
			var = AUTO_REJOIN_VAR;
			str = "$G %cToggled %GAuto_Rejoin %K[%W$[-3]0%K]","%s";
		} else if (!my_strnicmp(arg, "nick_completion", 6))
		{
			var = NICK_COMPLETION_VAR;
			str = "$G %cToggled %GNick Completion %K[%W$[-3]0%K]","%s";
		} else if (!my_strnicmp(arg, "aop", 3))
		{
			var = AOP_VAR;
			str = "$G %cToggled %GAOP %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "auto_away", 7))
		{
			var = AUTO_AWAY_VAR;
			str = "$G %cToggled %GAuto Away %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "auto_reconnect", 8))
		{
			var = AUTO_RECONNECT_VAR;
			str = "$G %cToggled %GAuto Reconnect %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "mircs", 5))
		{
			var = MIRCS_VAR;
			str = "$G %cToggled %GmIRC Color %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "bitch", 5))
		{
			var = BITCH_VAR;
			str = "$G %cToggled %GBitch %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "dcc_fast", 5))
		{
			var = DCC_FAST_VAR;
			str = "$G %cToggled %GDCC fast %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "kick_if_banned", 5))
		{
			var = KICK_IF_BANNED_VAR;
			str = "$G %cToggled %GKick banned Users %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "ftp_grab", 5))
		{
			var = FTP_GRAB_VAR;
			str = "$G %cToggled %GFTP grab %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "timestamp", 3))
		{
			var = TIMESTAMP_VAR;
			str = "$G %cToggled %GTimestamps %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "http_grab", 4))
		{
			var = HTTP_GRAB_VAR;
			str = "$G %cToggled %GHTTP grab %K[%W$[-3]0%K]";
		} else if (!my_strnicmp(arg, "display_ansi", 4))
		{
			set_int_var(DISPLAY_ANSI_VAR, get_int_var(DISPLAY_ANSI_VAR)?0:1);
			toggle_reverse_status(current_window, NULL, get_int_var(DISPLAY_ANSI_VAR));
			if (do_hook(SET_LIST, "%s %s", arg, on_off(get_int_var(DISPLAY_ANSI_VAR))))
				put_it("%s", convert_output_format("$G %cToggled %GAnsi Display %K[%W$[-3]0%K]", "%s", on_off(get_int_var(DISPLAY_ANSI_VAR)) ));
			set_input_prompt(current_window, empty_string, 0);
			set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
			status_update(1);
			continue;
		} else if (!my_strnicmp(arg, "log", 3))
		{
			int old_window_display = window_display;
			var = LOG_VAR;
			str = "$G %cToggled %GLogging %K[%W$[-3]0%K]";
			window_display = 0;
			logger(current_window, NULL, get_int_var(LOG_VAR)?0:1);
			window_display = old_window_display;
		}
		if (var != -1)
			set_int_var(var, get_int_var(var)?0:1);
		if (str)
		{
			if (do_hook(SET_LIST, "%s %s", arg, on_off(get_int_var(var))))
				put_it("%s", convert_output_format(str, "%s", on_off(get_int_var(var)) ));
		}
		if (var == -1)
			bitchsay("Unknown /toggle [%s]", arg);
	}
}
#endif
}


BUILT_IN_COMMAND(show_version)
{
char *nick;
char *version_buf = NULL;
extern char tcl_versionstr[];

#ifdef HAVE_UNAME
struct utsname buf;
	
	uname(&buf);
	malloc_strcpy(&version_buf, stripansicodes(convert_output_format(fget_string_var(FORMAT_VERSION_FSET), "%s %s %s %s %s", irc_version, internal_version, buf.sysname, buf.release?buf.release:empty_string, tcl_versionstr)));
#else
	malloc_strcpy(&version_buf, stripansicodes(convert_output_format(fget_string_var(FORMAT_VERSION_FSET), "%s %s %s %s %s", irc_version, internal_version, "unknown", tcl_versionstr, empty_string)));
#endif
	if (args && *args)
		nick = next_arg(args, &args);
	else
		nick = get_current_channel_by_refnum(0);
	send_text(nick, version_buf, "PRIVMSG", 1, 0);
	new_free(&version_buf);
}



void who_user_kill(WhoEntry *w, char *from, char **ArgList)
{
char *match_arg;
char *arg = NULL;
int server = 0;
char *nick_arg = NULL;
char *reason = NULL;

	if (isme(ArgList[4]))
		return;
	match_arg = alloca(2000);
	*match_arg = 0;
	
	arg = LOCAL_COPY(w->who_buff);

	strmopencat(match_arg, 1999, "*!", ArgList[1], "@", ArgList[2], NULL);

	server = atol(next_arg(arg, &arg));
	nick_arg = next_arg(arg, &arg);

	if (wild_match(nick_arg, match_arg))
	{
		char *t_arg, *t_nick;
		t_arg = alloca(2000);
		*t_arg = 0;
		if ((reason = strchr(arg, ':')))
			*reason++ = 0;
		t_nick = next_arg(arg, &arg);
		if (t_nick && *t_nick)
			strmopencat(t_arg, 1999, t_nick, ",", ArgList[4],"!", ArgList[1],"@", ArgList[2], NULL); 
		else
			strmopencat(t_arg, 1999, ArgList[4], "!", ArgList[1],"@", ArgList[2], NULL);
		malloc_sprintf(&w->who_buff, "%d %s %s :%s", server, nick_arg, t_arg, reason);
	}
}

void who_user_killend(WhoEntry *w, char *unused, char **unused1)
{
char *pattern, *match, *who_buff, *reason;
int server = -1;

	if (w->who_buff)
	{
		who_buff = LOCAL_COPY(w->who_buff);
		reason = strchr(who_buff, ':');
		*reason++ = 0;
		server = atol(next_arg(who_buff, &who_buff));
		pattern = next_arg(who_buff, &who_buff);
		match = next_arg(who_buff, &who_buff);
		if (match && *match && server != -1)
		{
			char *m_buff, *nick, *uh;
			char *buffer = NULL;
			char *save_buffer = NULL;
			int num = 0, count = 0;
			m_buff = alloca(strlen(match)+1);
			strcpy(m_buff, match);
			bitchsay("Killing all matching %s.", pattern);
			while ((nick = next_in_comma_list(m_buff, &m_buff)))
			{
				if (!nick || !*nick)
					break;
				m_s3cat(&save_buffer, space, nick);
				uh = strchr(nick, '!');
				*uh++ = 0;
				num++;
				count++;
				m_s3cat(&buffer, ",", nick);
				if (num >= get_int_var(NUM_KILLS_VAR))
				{
					bitchsay("Killing %s :%s[%d]", save_buffer, reason, count);
					my_send_to_server(server, "KILL %s :%s(%d)", buffer, reason, count);
					new_free(&buffer);
					num = 0;
				}
			}
			if (buffer)
			{
				bitchsay("Killing %s %s[%d]", save_buffer, reason, count);
				my_send_to_server(server, "KILL %s :%s(%d)", buffer, reason, count);
			}
			new_free(&save_buffer);
			new_free(&buffer);
		}
		else
			bitchsay("None Matching for /whokill or no longer connected");
	}
}

BUILT_IN_COMMAND(whokill)
{
	char *pattern;
	char *p = NULL;	
	char *reason = NULL;
	char *nick_arg = NULL;

        if (!args || !*args)
		return;
	if ((reason = strchr(args, ':')))
		*reason++ = 0;	
	else
		reason = get_reason(empty_string, NULL);
		
	while ((pattern = next_arg(args, &args))) 
	{
		malloc_sprintf(&nick_arg, "%s%s%s", *pattern != '*'?"*":empty_string, pattern, *(pattern+strlen(pattern)-1) != '*'?"*":empty_string);
		if ((p = strchr(nick_arg, '@')))
			p++;
		whobase(p ? p : nick_arg, who_user_kill, who_user_killend, "%d %s :%s", from_server, nick_arg, (reason && *reason) ? reason : empty_string);
		new_free(&nick_arg);
	} 
}

static char *tnick_arg = NULL;
static char *treason = NULL;

void trace_handlekill(int comm, char *nick)
{
static int count = 0;

	
	if (!nick || !*nick)
	{
		if (comm == 262)
		{
			set_server_trace_kill(from_server, 0);
			return;	
		}
		if (!count && get_server_trace_kill(from_server) != 2 && tnick_arg)
			bitchsay("No Match for trace kill of [%s]", tnick_arg);
		new_free(&treason);
		new_free(&tnick_arg);
		count = 0;
		return;
        }
	if (!my_stricmp(nick, get_server_nickname(from_server)))
		return;
	bitchsay("Killing %s[%s] %d", nick, tnick_arg, ++count);
	if (!treason)
		malloc_strcpy(&treason, get_reason(nick, NULL));
	send_to_server("KILL %s :%s (%d)", nick, treason, count);
}

void handle_tracekill(int comm, char *nick, char *user, char *host)
{
	char *q, *n;
	if (!nick || !*nick)
	{
		trace_handlekill(comm, NULL);
		return;
	}
	n = LOCAL_COPY(nick);
	if ((q = strchr(n, '[')))
		*q++ = 0;
	else
		return;
	chop(q, 1);
	if (wild_match(tnick_arg, q))
	{
		if (get_server_trace_kill(from_server) == 2)
			bitchsay("User: %s", nick);
		else
		{
			if (!my_stricmp(n, get_server_nickname(from_server)))
				return;
			trace_handlekill(comm, n);
		}
	}
}

BUILT_IN_COMMAND(tracekill)
{
	char *pattern;

	
	if (get_server_trace_kill(from_server))
	{
		bitchsay("Already in %s", command);
		return;
	}
	if (!(pattern = next_arg(args, &args)))
		return;
	if (args && *args)
		malloc_strcpy(&treason, args);
	else
		new_free(&treason);

	malloc_sprintf(&tnick_arg, "%s%s%s", *pattern != '*'?"*":empty_string, pattern, *(pattern+strlen(pattern)-1) != '*'?"*":empty_string);

	if (treason && *treason == '-')
		set_server_trace_kill(from_server, 2);
	else
		set_server_trace_kill(from_server, 1);

	send_to_server("TRACE");
}

BUILT_IN_COMMAND(traceserv)
{
	char *server, *pattern;

	
	if (get_server_trace_kill(from_server))
	{
		bitchsay("Already in %s", command);
		return;
	}
	if (!(server = next_arg(args, &args)) ||
		!(pattern = next_arg(args, &args)))
		return;
	if (args && *args)
	        malloc_strcpy(&treason, args);
	else
		new_free(&treason);

	malloc_sprintf(&tnick_arg, "%s%s%s", *pattern != '*'?"*":empty_string, pattern, *(pattern+strlen(pattern)-1) != '*'?"*":empty_string);

	if (treason && *treason == '-') 
		set_server_trace_kill(from_server, 2);
	else
		set_server_trace_kill(from_server, 1);
        send_to_server("TRACE %s", server);
}

#if 0
BUILT_IN_COMMAND(ftp)
{
Window *window, *tmp;
char name[40];
char *pgm = NULL;
int direct = 0;

	
	sprintf(name, "%%%s", command);
	if (command && !my_stricmp(command, "shell"))
	{
		pgm = get_string_var(SHELL_VAR);
		direct = 1;
	}
	else
		pgm = command;
		
	if (!args || !*args)
	{
		if (!is_window_name_unique(name+1))
		{
			int logic = -1;
			if ((tmp = get_window_by_name(name+1)))
			{
				delete_window(tmp);
				if ((logic = logical_to_index(name+1)) > -1)
					kill_process(logic, 15);
				else bitchsay("No such process [%s]", name+1);
			}
			recalculate_windows(tmp->screen);
			update_all_windows();
			return;
		}
	}
	if ((tmp = new_window(current_window->screen)))
	{
		int refnum;
		char *p = NULL;
		window = tmp;
		if (is_window_name_unique(name+1))
		{
			malloc_strcpy(&window->name, name+1);
			window->update |= UPDATE_STATUS;
		}

		window->screen = main_screen;
		set_screens_current_window(window->screen, window);
		recalculate_windows(tmp->screen);
		hide_window(window);
		refnum = window->refnum;
		
/*		update_all_windows();*/
		malloc_sprintf(&p, "-NAME %s %s %s", name, pgm, args);
/*		start_process(p, name+1, NULL, NULL, refnum, direct);*/
		execcmd(NULL, p, NULL, NULL);
		if (is_valid_process(name))
		{
			NickList *tmp_nick = NULL;
			malloc_strcpy(&window->query_nick, name);
			tmp_nick = (NickList *)new_malloc(sizeof(NickList));
			malloc_strcpy(&tmp_nick->nick, name);
			add_to_list((List **)&window->nicks, (List *)tmp_nick);
		}
		new_free(&p);
	} else bitchsay("Unable to create a new window");	
}
#endif

BUILT_IN_COMMAND(lkw)
{
	delete_window(current_window);
	update_all_windows();
}

BUILT_IN_COMMAND(jnw)
{
char *channel;
int hidden = 0;
	
	if ((channel = next_arg(args, &args)))
	{
		Window *tmp;
		if (*channel == '-' && !my_stricmp(channel, "-hide"))
		{
			if (!(channel = next_arg(args, &args)))
				return;
			hidden = 1;
		}
		if ((tmp = new_window(current_window->screen)))
		{
			int     server;
			server = from_server;
			from_server = tmp->server;
			if (channel && (channel = make_channel(channel)))
			{
				send_to_server("JOIN %s%s%s", channel, args?space:empty_string, args?args:empty_string);
				malloc_strcpy(&tmp->waiting_channel, channel);
			}
			from_server = server;
			build_status(tmp, NULL, 0);
			if (hidden)
				hide_window(tmp);
			update_all_windows();
		}
	}
}

int change_orig_nick(int server)
{
char *nick = get_server_orignick(server);
char *curnick = get_server_nickname(server);

	if (!nick || !curnick || !is_server_connected(server) ||
        is_orignick_pending(server) || !strcmp(nick, curnick))
		return 0;
	orignick_is_pending(server, 1);
	change_server_nickname(server, nick);
	bitchsay("Attempting to regain nick [%s]", nick);
	update_all_status(current_window, NULL, 0);
	update_input(UPDATE_ALL);
	return 1;
}

static void gain_nick (UserhostItem *stuff, char *nick, char *args)
{
int gotit = 0;
int server = -1;
int old_serv = from_server;
	if (!get_server_orignick(from_server) || !is_server_connected(from_server))
		return;
	server = my_atol(args);
	old_serv = server;
	if (stuff && stuff->nick && (!strcmp(stuff->user, "<UNKNOWN>") || !strcmp(stuff->host, "<UNKNOWN>")))
		gotit = change_orig_nick(from_server);
	if (!gotit && get_int_var(ORIGNICK_TIME_VAR) > 0 && !timer_callback_exists(delay_gain_nick))
		add_timer(0, empty_string, get_int_var(ORIGNICK_TIME_VAR) * 1000, 1, delay_gain_nick, NULL, NULL, -1, "orignick");
	from_server = old_serv;
}

static int delay_gain_nick(void *arg, char *sub)
{
char *nick = get_server_orignick(from_server);	
	if (from_server == -1)
		nick = get_server_orignick(0);
	if (nick)
		userhostbase(nick, gain_nick, 1, "%d", from_server);
	arg = NULL;
	sub = NULL;
	return 0;
}

void check_orig_nick(char *nick)
{
char *n = get_server_orignick(from_server);
	if (!nick && n)
	{
		userhostbase(n, gain_nick, 1, "%d", from_server);
		return;
	}
	if (n && !my_stricmp(nick, n))
		change_orig_nick(from_server);
}

BUILT_IN_COMMAND(orig_nick)
{
char *nick;
char *org_nick = get_server_orignick(from_server);
	if (!args || !*args || !(nick = next_arg(args, &args)))
	{
		if (org_nick)
			bitchsay("Attempting to gain \"%s\"", org_nick);
		return;
	}
	if (*nick == '-')
	{
		if (!org_nick)
			bitchsay("Not trying to gain a nick");
		else
		{
			bitchsay("Removing gain nick [%s]. Timer will expire.", org_nick);
			set_server_orignick(from_server, NULL);
		}
	}
	else
	{
		if (org_nick && !my_stricmp(nick, org_nick) && timer_callback_exists(delay_gain_nick))
			bitchsay("Already attempting that nick %s", nick);
		else if ((nick = check_nickname(nick)))
		{
			set_server_orignick(from_server, nick);
			userhostbase(get_server_orignick(from_server), gain_nick, 1, "%d", from_server);
			bitchsay("Trying to regain nick [%s] every %d seconds", get_server_orignick(from_server), get_int_var(ORIGNICK_TIME_VAR));
		}
		else
			bitchsay("Nickname was bad.");
	}
}

BUILT_IN_COMMAND(add_bad_nick)
{
char *buffer = NULL;
LameList *lame_nick = NULL;
char *nick = NULL;
extern LameList *lame_list;
int add = 0;
	if (!args || !*args || !command)
	{
		if (!command)
		{
			int i = 0;
			if (!lame_list)
			{
				bitchsay("There are no nicks on your lame nick list");
				return;
			}
			bitchsay("Lame Nick list:");
			for (lame_nick = lame_list; lame_nick; lame_nick = lame_nick->next)
			{
				if (buffer)
					m_s3cat(&buffer, lame_nick->nick, "\t");
				else
					buffer = m_sprintf("%s\t", lame_nick->nick);
				i++;
				if (i == 6)
				{
					i = 0;
					put_it("%s", buffer);
					new_free(&buffer);
				}	
			}
			if (buffer)
				put_it("%s", buffer);
			new_free(&buffer);
		}
		return;
	}
	add = !my_stricmp(command, "addlamenick") ? 1 : 0;
	bitchsay("%s %s LameNick list", add ? "Added":"Removed", add?"to":"from");
	while (args && *args)
	{
		nick = next_arg(args, &args);
		if (add && nick)
		{
			
			lame_nick = (LameList *)new_malloc(sizeof(LameList));
			malloc_strcpy(&lame_nick->nick, nick);
			add_to_list((List **)&lame_list, (List *)lame_nick);
		}
		else if (!add && nick)
		{
			lame_nick = (LameList *)remove_from_list((List **)&lame_list, nick);
			if (lame_nick)
			{
				new_free(&lame_nick->nick);
				new_free((char **)&lame_nick);
			}
			else
				nick = NULL;
		}
		if (nick && *nick)
		{
			if (buffer)
				m_s3cat(&buffer, nick, "\t");
			else
				buffer = m_sprintf("%s\t", nick);
		}
	}
	if (buffer)
		put_it("%s", buffer);
	new_free(&buffer);
}

static UrlList *url_list = NULL;

int grab_http(char *from, char *to, char *text) 
{
#ifdef PUBLIC_ACCESS
	return 0;
#else
	if ((get_int_var(HTTP_GRAB_VAR) && stristr(text, "HTTP:")) || (get_int_var(FTP_GRAB_VAR) && (stristr(text, "FTP:") || stristr(text, "FTP."))))
	{
		static int count = 0;
		int url_count = 0;
		char *q = NULL;
		UrlList *cur_url, *prev_url, *new_url;

		malloc_sprintf(&q, "%s %s -- %s", from, to, text);

		/* Look for end of the list, counting as we go */
		for (cur_url = url_list, prev_url = NULL; cur_url; prev_url = cur_url, cur_url = cur_url->next)
		{
			url_count++;
			/* If we find that the URL is already in the list, bail out. */
			if (cur_url->name && !my_stricmp(cur_url->name, q))
			{
				new_free(&q);
				return 0;
			}
		}

		/* Add the new URL at the end of the list */
		new_url = (UrlList *) new_malloc(sizeof(UrlList));
		new_url->name = q;
		new_url->next = NULL;
		if (!prev_url)
			url_list = new_url;
		else
			prev_url->next = new_url;
		url_count++;
		count++;

		/* Prune off any excess entries */
		while (url_count > get_int_var(MAX_URLS_VAR) && url_list)
		{
			UrlList *tmp = url_list;
			url_list = url_list->next;
			new_free(&tmp->name);
			new_free(&tmp);
			url_count--;	
		}

		if (do_hook(URLGRAB_LIST, "%d %d %s %s %s %s", url_count, count, from, FromUserHost, to, text))
			bitchsay("Added HTTP/FTP grab [%d/%d]", url_count, count);
		return 1;
	}
	return 0;
#endif
}

BUILT_IN_COMMAND(url_grabber)
{
#ifdef PUBLIC_ACCESS
	bitchsay("This command has been disabled on a public access system");
	return 0;
#else
	int do_display = 1;

	if (args && *args)
	{
		int i, q;
		char *p;
		UrlList *cur_url, *prev_url;

		while ((p = next_arg(args, &args)))
		{
			if (!my_stricmp(p, "SAVE"))
			{
				char *filename = NULL;
				FILE *file;
				char buffer[BIG_BUFFER_SIZE+1];
				char *number = "*";

				snprintf(buffer, BIG_BUFFER_SIZE, "%s/BitchX.url", get_string_var(CTOOLZ_DIR_VAR));
				filename = expand_twiddle(buffer);
				if (!filename)
					return;

				file = fopen(filename, "a");
				if (!file)
				{
					new_free(&filename);
					return;
				}

				if (args && *args && (isdigit((unsigned char)*args) || *args == '-'))
					number = next_arg(args, &args);
				for (i = 0, cur_url = url_list; cur_url; cur_url = cur_url->next, i++)
				{
					if (matchmcommand(number, i) && cur_url->name)
						fprintf(file, "%s\n", cur_url->name);
				}
				new_free(&filename);
				fclose(file);

				/* Clear URL list */
				while (url_list)
				{
					UrlList *tmp = url_list;
					url_list = url_list->next;
					new_free(&tmp->name);
					new_free(&tmp);
				}

				bitchsay("Url list saved");
				do_display = 0;
			}
			else if (!my_stricmp("HTTP", p))
			{
				int on = get_int_var(HTTP_GRAB_VAR);
				char *ans = next_arg(args, &args);
				if (ans && !my_stricmp(ans, "ON"))
					on = 1;
				else
					on = 0;
				set_int_var(HTTP_GRAB_VAR, on);
			}
			else if (!my_stricmp("FTP", p))
			{
				int on = get_int_var(FTP_GRAB_VAR);
				char *ans = next_arg(args, &args);
				if (ans && !my_stricmp(ans, "ON"))
					on = 1;
				else
					on = 0;
				set_int_var(FTP_GRAB_VAR, on);
			}
			else if (*p == '-')
  			{
				if (!*++p)
				{
					while (url_list)
					{
						UrlList *tmp = url_list;
						url_list = url_list->next;
						new_free(&tmp->name);
						new_free(&tmp);
					}
					bitchsay("Url list cleared");
				}
				else
				{       
					q = my_atol(p);
					for (i = 1, cur_url = url_list, prev_url = NULL; cur_url && i < q; prev_url = cur_url, cur_url = cur_url->next, i++);
					if (cur_url && i == q)
					{
						if (!prev_url)
							url_list = url_list->next;
						else
							prev_url->next = cur_url->next;
						new_free(&cur_url->name);
						new_free(&cur_url);
						bitchsay("Cleared Url [%d]", q);
					}
					else
						bitchsay("Url [%d] not found", q);
				}
				do_display = 0;
			}
			else if (!my_stricmp("LIST", p) || !my_stricmp("+", p))
			{
				if (!url_list)
				{
					bitchsay("No Urls in Url list");
					return;
				}
				bitchsay("Url list:");
				for (i = 1, cur_url = url_list; cur_url; cur_url = cur_url->next, i++)
				{
					if (!cur_url->name)
						break;
					put_it("%s", convert_output_format("$G HTTP[$0] $1-", "%d %s", i, cur_url->name));
				}
				do_display = 0;
			}
		}
	}
	if (do_display)
		put_it("%s", convert_output_format("$G HTTP grab %K[%W$0%K]%n FTP grab %K[%W$1%K]", "%s %s", on_off(get_int_var(HTTP_GRAB_VAR)), on_off(get_int_var(FTP_GRAB_VAR))));
#endif
}

BUILT_IN_COMMAND(serv_stat)
{
extern long nick_collisions, oper_kills, serv_fakes, serv_unauth, serv_split;
extern long serv_rejoin, client_connects, serv_rehash, client_exits,serv_klines;
extern long client_floods, client_invalid, stats_req, client_bot, client_bot_alarm;
extern long oper_requests, serv_squits, serv_connects;
#ifdef ONLY_STD_CHARS

put_it("%s", convert_output_format("%G-----------%K[ %WServer %wStats %K]%G----------------------------------------------", NULL));
put_it("%s", convert_output_format("%G| %CN%cick Collisions %K[%W$[-4]0%K]    %CO%cper Kills   %K[%W$[-4]1%K]", "%d %d", nick_collisions, oper_kills));
put_it("%s", convert_output_format("%G| %CF%cake Modes      %K[%W$[-4]0%K]    %CU%cnauth       %K[%W$[-4]1%K]", "%d %d",serv_fakes, serv_unauth));
put_it("%s", convert_output_format("%G| %CH%cigh Traffic    %K[%W$[-4]0%K]    %CN%corm Traffic %K[%W$[-4]1%K]", "%d %d",serv_split, serv_rejoin));
put_it("%s", convert_output_format("%G| %CT%cotal Clients   %K[%W$[-4]0%K]    %CS%cerv rehash  %K[%W$[-4]1%K]", "%d %d",client_connects, serv_rehash));
put_it("%s", convert_output_format("%G| %CC%client exits    %K[%W$[-4]0%K]    %CK%c-lines adds %K[%W$[-4]1%K]", "%d %d",client_exits, serv_klines));
put_it("%s", convert_output_format("%G| %CC%client Floods   %K[%W$[-4]0%K]    %CS%ctats reqs   %K[%W$[-4]1%K]", "%d %d",client_floods, stats_req));
put_it("%s", convert_output_format("%G| %CI%cnvalid User    %K[%W$[-4]0%K]    %CO%cper Reqs    %K[%W$[-4]1%K]", "%d %d",client_invalid, oper_requests));
put_it("%s", convert_output_format("%G| %CP%cossible Bots   %K[%W$[-4]0%K]    %CB%cot Alarms   %K[%W$[-4]1%K]", "%d %d",client_bot, client_bot_alarm));
put_it("%s", convert_output_format("%G| %CS%cerv Squits     %K[%W$[-4]0%K]    %CS%cerv Connect %K[%W$[-4]1%K]", "%d %d",serv_squits, serv_connects));

#else

put_it("%s", convert_output_format("%GÚÄÄÄÄÄ---%gÄ%G-%K[ %WServer %wStats %K]-%gÄÄ%G-%gÄÄÄÄÄÄ---%KÄ%g--%KÄÄ%g-%KÄÄÄÄÄÄÄÄÄ--- --  - --- -- -", NULL));
put_it("%s", convert_output_format("%G³ %CN%cick Collisions %K[%W$[-4]0%K]    %CO%cper Kills   %K[%W$[-4]1%K]", "%l %l", nick_collisions, oper_kills));
put_it("%s", convert_output_format("%G³ %CF%cake Modes      %K[%W$[-4]0%K]    %CU%cnauth       %K[%W$[-4]1%K]", "%l %l",serv_fakes, serv_unauth));
put_it("%s", convert_output_format("%g³ %CH%cigh Traffic    %K[%W$[-4]0%K]    %CN%corm Traffic %K[%W$[-4]1%K]", "%l %l",serv_split, serv_rejoin));
put_it("%s", convert_output_format("%G³ %CT%cotal Clients   %K[%W$[-4]0%K]    %CS%cerv rehash  %K[%W$[-4]1%K]", "%l %l",client_connects, serv_rehash));
put_it("%s", convert_output_format("%g| %CC%client exits    %K[%W$[-4]0%K]    %CK%c-lines adds %K[%W$[-4]1%K]", "%l %l",client_exits, serv_klines));
put_it("%s", convert_output_format("%G: %CC%client Floods   %K[%W$[-4]0%K]    %CS%ctats reqs   %K[%W$[-4]1%K]", "%l %l",client_floods, stats_req));
put_it("%s", convert_output_format("%G: %CI%cnvalid User    %K[%W$[-4]0%K]    %CO%cper Reqs    %K[%W$[-4]1%K]", "%l %l",client_invalid, oper_requests));
put_it("%s", convert_output_format("%K| %CP%cossible Bots   %K[%W$[-4]0%K]    %CB%cot Alarms   %K[%W$[-4]1%K]", "%l %l",client_bot, client_bot_alarm));
put_it("%s", convert_output_format("%g: %CS%cerv Squits     %K[%W$[-4]0%K]    %CS%cerv Connect %K[%W$[-4]1%K]", "%l %l",serv_squits, serv_connects));

#endif

}

char *strip(char *str, char *unwanted)
{
static char buffer[NICKNAME_LEN*4];
register char *cp, *dp;
	if (!str)
		return empty_string;
	for (cp = str, dp = buffer; *cp; cp++)
	{
		if (!strchr(unwanted, *cp))
			*dp++ = *cp;
	}	
	*dp = 0;
	return buffer;
}


BUILT_IN_COMMAND(set_autoreply)
{
char new_nick[NICKNAME_LEN+10];
char not_wanted1[] = "_^\\{}[]|-";
char not_wanted2[] = "_^\\{}[]|-0123456789";
extern char *auto_str;
	if (!args || !*args)
		return;
	if (*args == '-')
	{
		set_string_var(AUTO_RESPONSE_STR_VAR, NULL);
		bitchsay("Auto-reply pats are deleted");
		new_free(&auto_str);
	}
	else
	{
		char *p, *new_args = NULL;
		p = next_arg(args, &args);
		if (*p == 'd' && strlen(p) == 1)
		{
			int len = strlen(nickname);
			m_e3cat(&new_args, nickname, space);
			m_e3cat(&new_args, strip(nickname, not_wanted1), (len > 4 ? space: empty_string));
			if (len > 4)
			{
				memset(new_nick, 0, sizeof(new_nick));
				new_nick[0] = '*';
				strncpy(new_nick+1, strip(nickname, not_wanted1), 4);
				new_nick[5] = '*';
				if (!strstr(new_args, new_nick))
					m_e3cat(&new_args, new_nick, space);
			}
			if (!strstr(new_args, strip(nickname, not_wanted2)))
				m_e3cat(&new_args, strip(nickname, not_wanted2), (len > 4 ? space:empty_string));
			if (len > 4)
			{
				memset(new_nick, 0, sizeof(new_nick));
				new_nick[0] = '*';
				strncpy(new_nick+1, strip(nickname, not_wanted2), 4);
				new_nick[5] = '*';
				if (!strstr(new_args, new_nick))
					malloc_strcat(&new_args, new_nick);
			}
			set_string_var(AUTO_RESPONSE_STR_VAR, new_args);
			reinit_autoresponse(current_window, new_args, 0);
		}
		else
		{
			do {
				m_3cat(&new_args, p, args?space:empty_string);
			} while ((p = next_arg(args, &args)));
			set_string_var(AUTO_RESPONSE_STR_VAR, new_args);
			reinit_autoresponse(current_window, new_args, 0);
		}
		new_free(&new_args);
		bitchsay("Auto-Response now set to [%s]", get_string_var(AUTO_RESPONSE_STR_VAR));
	}
}



BUILT_IN_COMMAND(unload)
{
	remove_bindings();
	init_keys();
	init_keys2();
	delete_all_timers();
	destroy_aliases(VAR_ALIAS);
	destroy_aliases(COMMAND_ALIAS);
	destroy_aliases(VAR_ALIAS_LOCAL);
	delete_all_arrays();
	delete_all_ext_fset();
	flush_on_hooks();
	init_variables();
	init_window_vars(NULL, NULL, NULL, NULL);
}


BUILT_IN_COMMAND(add_no_flood)
{
char *nick;
List *nptr;
	if (!args || !*args)
	{
		int count = 0;
		for (nptr = next_namelist(no_flood_list, NULL, FLOOD_HASHSIZE); nptr; nptr = next_namelist(no_flood_list, nptr, FLOOD_HASHSIZE))
		{
			if (count == 0)
				put_it("%s", convert_output_format("No Flood list", NULL, NULL));
			put_it("%s", nptr->name);
			count++;
		}
		return;
	}
	say("Adding \"%s\" to No Flood list.", args);
	nick = next_arg(args, &args);
	while (nick && *nick)
	{
		if (!(nptr = find_name_in_genericlist(nick, no_flood_list, FLOOD_HASHSIZE, 0)))
			add_name_to_genericlist(nick, no_flood_list, FLOOD_HASHSIZE);
		nick = next_arg(args, &args);
	}
}

BUILT_IN_COMMAND(check_clones)
{
char *channel= NULL;
ChannelList *chan = NULL;
NickList *nptr = NULL, *nptr1 = NULL;
int server = from_server;
int clones = 0;
	if (args && *args)
	{
		channel = next_arg(args, &args);
		channel = make_channel(channel);
		chan = prepare_command(&server, channel, NO_OP);
	}
	else
	{
		channel = get_current_channel_by_refnum(0);
		chan = prepare_command(&server, channel, NO_OP);
	}
	if (chan == NULL)
		return;
	for (nptr = next_nicklist(chan, NULL); nptr; nptr = next_nicklist(chan, nptr))
		nptr->check_clone = 0;
	for (nptr = next_nicklist(chan, NULL); nptr; nptr = next_nicklist(chan, nptr))
	{
		char *q;

		if (nptr->check_clone)
			continue;

		q = strchr(nptr->host, '@'); q++;

		/* Scan forward from this point, looking for clones */
		for (nptr1 = next_nicklist(chan, nptr); nptr1; nptr1 = next_nicklist(chan, nptr1))
		{
			char *p;

			p = strchr(nptr1->host, '@'); p++;

			if (!strcmp(p, q))
			{
				/* clone detected */
				if (!clones && do_hook(CLONE_LIST, "Clones detected on %s", chan->channel))
					bitchsay("Clones detected on %s", chan->channel);
				if (!nptr->check_clone++)
				{
					if (do_hook(CLONE_LIST, "%s %s %s", nptr->nick, nptr->host, nick_isop(nptr)? "@":empty_string))
						put_user(nptr, chan->channel);
					clones++;
				}
				if (do_hook(CLONE_LIST, "%s %s %s", nptr1->nick, nptr1->host, nick_isop(nptr1)? "@":empty_string))
					put_user(nptr1, chan->channel);
				nptr1->check_clone++;
				clones++;
			}
		}
	}
	if (!clones)
	{
		if (do_hook(CLONE_LIST, "No clones detected on %s", channel))
			bitchsay("No clones detected on %s", channel);
	}
	else if (do_hook(CLONE_LIST, "Found %d clones", clones))
		bitchsay("Found %d clones", clones);
}

void get_range(char *line, int *start, int *end)
{
char *q = line, *p = line;
	while (*p && isdigit((unsigned char)*p))
		p++;
	if (*p)
		*p++ = 0;
	*start = my_atol(q);
	*end = *p? my_atol(p): *start;
	if (*end < *start)
		*end = *start;
}

BUILT_IN_COMMAND(pastecmd)
{
	char *lines;
	char *channel = NULL;
	Window *win;
	int	winref = 0;
	int line = 1;
	int topic = 0;

	int start_line = 1, end_line = 1, count;
#if 0
	Lastlog *start_pos;
#else
	Display *start_pos;
#endif        
	if ((lines = next_arg(args, &args)))
		get_range(lines, &start_line, &end_line);
	if (!args || !*args)
		channel = get_current_channel_by_refnum(0);
	else
	{
		char *t;
		while (args && *args)
		{
			t = next_arg(args, &args);
			if (*t == '-')
			{
				if (!my_strnicmp(t, "-win", strlen(t)))
				{
					if (*args && isdigit((unsigned char)*args))
					{
						t = next_arg(args, &args);
						winref = my_atol(t);
					}
				}
				else if (!my_strnicmp(t, "-topic", strlen(t)))
				{
					topic = 1;
				}
			}
			else 
				channel = t;
		}
	}
	if (!channel && !(channel = get_current_channel_by_refnum(0)))
		return;
	win = get_window_by_refnum(winref);

	if (!win) {
		bitchsay("That window doesn't exist");
		return;
	}

#if 0
	if (!(end_line - start_line))
		line = end_line;
	else
		line = end_line - start_line + 1;
#endif
	line = end_line;
	
#if 0
	if (line < 1 || line > win->lastlog_size)
	{
		bitchsay("Try a realistic number within your scrollbuffer");
		return;
	}
	for (start_pos = get_input_hold(win) ? get_input_hold(win):win->lastlog_head; line; start_pos = start_pos->next)
		line--;

	if (!start_pos)
		start_pos = win->lastlog_tail;
	else
		start_pos = start_pos->prev;
	count = end_line - start_line + 1;
	if (count == 1 && topic && start_pos)
	{
		send_to_server("TOPIC %s :%s", channel, start_pos->msg);
		return;
	}
	while (count)
	{
		line++;
		if (start_pos && start_pos->msg)
		{
			if (do_hook(PASTE_LIST, "%s %s", channel, start_pos->msg))
				send_text(channel, convert_output_format(fget_string_var(FORMAT_PASTE_FSET),"%s %d %s", channel, line, start_pos->msg), NULL, 1, 0);
			start_pos = start_pos->prev;
		}
		count--;
	}
#else
	if (line < 1 || line > win->display_buffer_size)
	{
		bitchsay("Try a realistic number within your scrollbuffer");
		return;
	}
	start_pos = get_screen_hold(win);
	for (start_pos = start_pos ? start_pos->prev : win->display_ip->prev; line > 1; start_pos = start_pos->prev)
		line--;

	if (!start_pos)
		start_pos = win->display_ip->prev;

	count = end_line - start_line + 1;
	if (count == 1 && topic && start_pos)
	{
		send_to_server("TOPIC %s :%s", channel, start_pos->line);
		return;
	}
	line = 0;
	while (count)
	{
		line++;
		if (start_pos && start_pos->line)
		{
			if (do_hook(PASTE_LIST, "%s %s", channel, start_pos->line))
				send_text(channel, convert_output_format(fget_string_var(FORMAT_PASTE_FSET),"%s %d %s", channel, line, start_pos->line), NULL, 1, 0);
			start_pos = start_pos->next;
		}
		count--;
	}
#endif
}

#if defined(GUI)
void parsemenuitem(char *pargs, char **itemtext, char **options, char **itemalias, int *refnum)
{
	char *text;
	*refnum=0;

	text = pargs;
	while(*pargs == '-')
	{
		while(*pargs && *pargs != ' ')
			pargs++;
		while(*pargs==' ')
			pargs++;
	}
	if(text == pargs)
		*options = empty_string;
	else
	{
		*(pargs-1)=0;
		*options = text;
	}

	if(pargs && (*pargs=='\"' || *pargs == 's') )
		*itemtext=new_next_arg(pargs, &pargs);
	else
	{
		*refnum=my_atol(new_next_arg(pargs, &pargs));
		*itemtext=new_next_arg(pargs, &pargs);
	}
	if (pargs && *pargs == '{')
		*itemalias = next_expr(&pargs, '{');
	else
		*itemalias = pargs;
}

void parsesubmenu(char *pargs, char **submenutext, char **options)
{
	if (*pargs == '-')
	{
		*options = new_next_arg(pargs, &pargs);
	}
	*submenutext = new_next_arg(pargs, &pargs);
}

/*
 * Returns a pointer to the menu you are looking for, or returns NULL if it does not exist
 */

MenuStruct *findmenu(char *menuname)
{
MenuStruct *thismenu;
	if(morigin!=NULL)
	{
		thismenu=morigin;
		while(thismenu)
		{
			if(!strcmp(menuname, thismenu->name))
				return thismenu;
			thismenu = thismenu->next;
		}
	}
	return NULL;
}


/*
 * This adds an empty menu entry
 */

static int null_cmp_func(List *s, List *s1)
{
	return 0;
}

MenuStruct *addmenu(char *menuname)
{
	MenuStruct *new = NULL;
	new = new_malloc(sizeof(MenuStruct));
	memset(new, 0, sizeof(MenuStruct));
	malloc_strcpy(&new->name, menuname);

	new->menuorigin=NULL;
	new->next=NULL;
	new->root=NULL;
#ifdef __EMXPM__
	new->sharedhandle = NULLHANDLE;
#elif defined(GTK)
	new->sharedhandle = (GtkWidget *)NULL;
#endif
	add_to_list_ext((List **)&morigin, (List *)new, null_cmp_func);
	return new;
}

/*
 * This function recursively deletes all menuitems 
 */

void removemenu(char *);

void recdel(MenuList *delitem)  
{
MenuList *last;
	while (delitem)
	{
		last = delitem->next;
		new_free(&delitem->name);
		if (delitem->menutype & GUISUBMENU)
		{
			new_free(&delitem->alias);
			if (!(delitem->menutype & GUISHARED))
				removemenu(delitem->submenu);
		}
		new_free(&delitem->alias);
		new_free(&delitem);
		delitem = last;
	}
}

/*
 * This removes a menu entry and all menuitems on it
 */

void removemenu(char *menuname)
{
MenuStruct *thismenu;
	if ((thismenu = (MenuStruct *)remove_from_list((List **)&morigin, menuname)))
	{
		if (thismenu->menuorigin)
			recdel(thismenu->menuorigin);
		new_free(&thismenu->name);
		new_free(&thismenu);
	}
}

BUILT_IN_COMMAND(os2menu)
{
	char *mpar;
	MenuStruct *thismenu;

	if ((mpar = new_next_arg(args, &args)))
	{
		if(*mpar =='-')
		{
			gui_remove_menurefs(mpar + 1);
			removemenu(mpar + 1);
		}
		else
			addmenu(mpar);
	}
	else
	{
		if (morigin)
		{
			thismenu=morigin;
			while (thismenu!=NULL)
			{
				say("Menu \'%s\'",thismenu->name);
				thismenu=thismenu->next;
			}
		}
		else
			say("No menus defined.");
	}
}

BUILT_IN_COMMAND(os2menuitem)
{
	char	*menuname = NULL,
	*itemtext = NULL,
	*options = NULL,
	*itemalias = NULL;
	MenuStruct *thismenu;
	MenuList *lastitem = NULL, *thisitem = NULL;
	int refnum = -1, z;

	if ((menuname = next_arg(args, &args)) && (thismenu=findmenu(menuname)))
	{
		parsemenuitem(args, &itemtext, &options, &itemalias, &refnum);
		if (thismenu->menuorigin!=NULL)
		{
			thisitem=thismenu->menuorigin;
			while (thisitem)
			{
				lastitem=thisitem;
				thisitem=lastitem->next;
			}
			lastitem->next=(MenuList *)new_malloc(sizeof(MenuList));
			thisitem=lastitem->next;
		}
		else
		{
			thismenu->menuorigin=(MenuList *)new_malloc(sizeof(MenuList));
			thisitem=thismenu->menuorigin;
		}
		thisitem->menuid=globalmenuid;
		thisitem->refnum=refnum;
		globalmenuid++;
        /* Add checks for inactive items, etc in options */
		if (!my_stricmp(itemtext, "separator"))
			thisitem->menutype = GUISEPARATOR;
		else
		{
			thisitem->menutype = GUIMENUITEM;
			if(options && *options)
			{
				for(z=0;z<strlen(options);z++)
				{
					if(options[z] == 'd')
						thisitem->menutype |= GUIDEFMENUITEM;
					else if(options[z] == 'b')
						thisitem->menutype |= GUIBRKMENUITEM;
					else if(options[z] == 'c')
						thisitem->menutype |= GUICHECKEDMENUITEM | GUICHECKMENUITEM;
					else if(options[z] == 'n')
						thisitem->menutype |= GUINDMENUITEM;
				}
			}
			malloc_strcpy(&thisitem->name,itemtext);
			thisitem->alias = convert_output_format2(itemalias);
			thisitem->submenu=NULL;
			thisitem->next=NULL;
		}

	}
}

BUILT_IN_COMMAND(os2submenu)
{
char	*menuname = NULL, 
   	*submenuname = NULL, 
   	*submenutext = NULL, 
   	*options = NULL; 
MenuStruct *thismenu = NULL, *submenu = NULL;
MenuList *lastitem = NULL, *thisitem = NULL;
int z;


	if ((menuname = next_arg(args, &args)))
	{
		submenuname = next_arg(args, &args);
		if(!submenuname)
			return;
		if (!(thismenu = findmenu(menuname)))
		{
			say("Menu not found.");
			return;
		}
		if (thismenu->menuorigin)
			submenu = findmenu(submenuname);
		if (!submenu)
			submenu = addmenu(submenuname);
	}
	if (menuname && thismenu)
	{
		parsesubmenu(args, &submenutext, &options);
		if(thismenu->menuorigin)
		{
			thisitem=thismenu->menuorigin;
			while(thisitem)
			{
				lastitem=thisitem;
				thisitem=lastitem->next;
			}
            /* I need to add error checking here later */
			lastitem->next=(MenuList *)new_malloc(sizeof(MenuList)); 
			thisitem=lastitem->next;
		}
		else
		{
            /* I need to add error checking here later */
			thismenu->menuorigin=(MenuList *)new_malloc(sizeof(MenuList)); 
			thisitem=thismenu->menuorigin;
		}
		memset(thisitem, 0, sizeof(MenuList));
		thisitem->menuid=globalmenuid;
		thisitem->refnum=-1;
		globalmenuid++;
		/* Add checks for inactive items, etc in options */
		thisitem->menutype = GUISUBMENU;
		if(options && *options)
		{
			for(z=0;z<strlen(options);z++)
			{
				if(options[z] == 'b')
					thisitem->menutype |= GUIBRKSUBMENU;
				else if(options[z] == 's')
					thisitem->menutype |= GUISHARED;
			}
		}
		malloc_strcpy(&thisitem->name,submenutext);
		malloc_strcpy(&thisitem->submenu, submenuname);
		thisitem->next=NULL;
	}
}

BUILT_IN_COMMAND(fontdialog)
{
	gui_font_dialog(last_input_screen);
}

BUILT_IN_COMMAND(filedialog)
{
char	*type = NULL,
	*path = NULL,
	*title = NULL,
	*ok = NULL,
	*apply = NULL,
	*code = NULL,
	szButton[10] = "OK";

	if (!(type = next_arg(args, &args)) || !(path = next_arg(args, &args)))
		return;
	if (!(title = new_next_arg(args, &args)))
		return;
	if (*args=='\"')
	{
		ok = new_next_arg(args, &args);
		strcpy(szButton, ok);
	}
	if (*args=='\"')
		apply = new_next_arg(args, &args);

	if (args && *args == '{')
		code = next_expr(&args, '{');
	else
		code = args;

	gui_file_dialog(type, path, title, ok, apply, code, szButton);
}

BUILT_IN_COMMAND(pmpaste)
{
	gui_paste(args);
}

BUILT_IN_COMMAND(pmprop)
{
	gui_properties_notebook();
}

BUILT_IN_COMMAND(os2popupmenu)
{
char *menuname;

	if(args && *args)
	{
		menuname = next_arg(args, &args);
		gui_popupmenu(menuname);
	}
}

BUILT_IN_COMMAND(os2popupmsg)
{
char *tmpmsg;

	if (args && *args)
	{
		tmpmsg = new_next_arg(args, &args);
		if (tmpmsg && *tmpmsg)
		{
			msgtext = m_strdup(tmpmsg);
			gui_msgbox();
		}
	 }
}
#endif

#ifdef __EMXPM__
BUILT_IN_COMMAND(pmcodepage)
{
int codepage=0;
extern HMQ hmq;

	if (args && *args)
	{
		codepage = my_atol(args);
		if (codepage)
		{
			current_window->screen->codepage = codepage;
			WinSetCp(hmq, codepage);
			VioSetCp(0, codepage, current_window->screen->hvps);
			VioSetCp(0, codepage, current_window->screen->hvpsnick);
		}
	}
	else
		say("Usage: /codepage <codepage number>");
}
#endif

#if !defined(__EMX__) && !defined(WINNT) && !defined(GUI) && defined(WANT_DETACH)
#ifndef PUBLIC_ACCESS
extern char socket_path[];
extern char *old_pass;

int displays = 0;
int reattach_socket = -1;
int save_ipc = -1;
int save_pid = -1;
int already_detached = 0;

struct param {
	pid_t	pgrp,
		pid;
	uid_t	uid;
	int	cols;
	int	rows;
	char	tty[80];
	char	cookie[30];
	char	password[80];
	char	termid[81];
};

static char connect_cookie[30];

#define SOCKMODE (S_IWRITE | S_IREAD | (displays ? S_IEXEC : 0))

struct param parm;                                                

void handle_reconnect(int s)
{
int len;
struct sockaddr_in addr;
int n;
	memset(&addr, 0, sizeof(addr));
	memset(&parm, 0, sizeof(parm));
	len = sizeof(addr);
	alarm(10);
	n = my_accept(s, (struct sockaddr *)&addr, &len);
	alarm(0);
	if (n < 0 || getpeername(n, (struct sockaddr *)&addr, &len) < 0)
	{
		put_it("%d %s", errno, strerror(errno));
		if ( n > -1) close(n);
		return;
	}
	if (strcmp(inet_ntoa(addr.sin_addr), "127.0.0.1"))
	{
		put_it("Invalid hostname connect from %s", inet_ntoa(addr.sin_addr));
		close(n);
		return;
	}
	set_blocking(n);
	alarm(5);
	if ((len = read(n, &parm, sizeof(struct param))) > 0)
	{
		alarm(0);
		if ( ((parm.password[0] != '\0') && !old_pass) ||
			((parm.password[0] == '\0') && old_pass) ||
			((parm.password[0] != '\0') && old_pass && checkpass(parm.password, old_pass) ))
		{
			bitchsay("Invalid password attempt on reconnect");
			close(n);
			return;
		}
		if ((parm.uid != getuid()) || !*parm.cookie || strcmp(parm.cookie, connect_cookie))
		{
			bitchsay("Invalid uid attempting connect [%d] or bad cookie", parm.uid);
			close(n);
			return;
		}

		save_pid = parm.pid;

		new_close(0);
		close(1);
		close(2);
		fclose(main_screen->fpout);
		fclose(main_screen->fpin);
		close(main_screen->fdout);
		close(main_screen->fdin);

		dup2(n, 0);
		dup2(n, 1);
		dup2(n, 2);

		parm.pid = getpid();
		parm.pgrp = getpgrp();
		strcpy(attach_ttyname, ttyname(0)? ttyname(0): parm.tty);
		write(n, &parm, sizeof(struct param));

		main_screen->fpin = fdopen(0, "r");
		main_screen->fpout = fdopen(1, "w");
		main_screen->fdin = fileno(main_screen->fpin);
		main_screen->fdout = fileno(main_screen->fpout);

		new_open(n);


		term_init((parm.termid && *parm.termid) ? parm.termid : NULL);
		reset_cols(parm.cols);
		reset_lines(parm.rows);
		reinit_term(main_screen->fdin);
		set_term_eight_bit(1);
		charset_ibmpc();
		update_all_windows();
		refresh_screen(0, NULL);
		reattach_socket = n;
		close_socketread(s);
		use_input = 1;
		already_detached = 0;
		save_ipc = -1;
		return;
	}
	alarm(0);
	close(n);
}

extern SIGNAL_HANDLER(sigpipe);

void kill_attached_if_needed(int type)
{
	if (reattach_socket != -1)
	{
		my_signal(SIGPIPE, SIG_IGN, 0);
		new_close(0); 
		kill(save_pid, type? SIGPIPE : SIGHUP);
		new_close(reattach_socket);
		dup2(main_screen->fdin, 0);
		new_open(0);
		dup2(main_screen->fdout, 1);
		dup2(main_screen->fdout, 2);
		reattach_socket = -1;
		save_pid = -1;
		my_signal(SIGPIPE, sigpipe, 0);
	}
}

void make_cookie(void)
{
int i, j;
	memset(connect_cookie, 0, sizeof(connect_cookie));
	for (i = 0; i < (int) (20.0 * rand()/RAND_MAX) + 5; i++)
	{
		j = (int)(95.0 * rand()/RAND_MAX);
		connect_cookie[i] = j + 32;
	}
}

static int create_ipc_socket(void)
{
	int	s = -1, u = -1;
	unsigned short port = 0;
	char buf[BIG_BUFFER_SIZE+1];

	init_socketpath();

	if ((s = connect_by_number(NULL, &port, SERVICE_SERVER, PROTOCOL_TCP, 0)) < 0)
	{
		bitchsay("Error creating IPC socket: [%d] %s", s, strerror(errno));
		return 1;
	}

	sprintf(buf, socket_path, port);
	if ((u = open(buf, O_CREAT|O_WRONLY, 0600)) != -1)
	{
		chmod(buf, SOCKMODE);
		chown(buf, getuid(), getgid());
		make_cookie();
		write(u, connect_cookie, strlen(connect_cookie));
		write(u, "\n", 1);
		close(u);
	}

	set_non_blocking(s);
	add_socketread(s, port, 0, socket_path, handle_reconnect, NULL);
	save_ipc = s;

	return 0;
}


void do_detach(int pid, char *args)
{

	cursor_to_input(); 
	term_cr(); 
	term_clear_to_eol();
	term_reset2();
	fprintf(stdout, "\r");
	if (args && *args)
		old_pass = m_strdup(next_arg(args, &args));
	if (pid !=  -1)
		fprintf(stdout, "detached from %s. To re-attach type scr-bx %s\n\r", attach_ttyname, old_pass?"-p":empty_string);
	fflush(stdout);
	fflush(stderr);
	exit(pid);
}
#endif
#endif /* EMX WINNT WANT_DETACH */

void close_detach_fd(void)
{
Screen *screen = NULL;
	if (main_screen)
	{
		main_screen->fdout = main_screen->fdin = open("/dev/null", O_RDWR|O_NOCTTY);
		main_screen->fpin = fdopen(main_screen->fdin, "r");
		main_screen->fpout = fdopen(main_screen->fdout, "w");
	}
	else
	{
		close(0); 
		close(1); 
		close(2);
	}
	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
	for (screen = screen_list; screen; screen = screen->next)
	{
		screen->fpin = main_screen->fpin;
		screen->fpout = main_screen->fpout;
		screen->fdin = main_screen->fdin;
		screen->fdout = main_screen->fdout;
	}

}

extern int do_check_pid;

BUILT_IN_COMMAND(detachcmd)
{
#if !defined(__EMX__) && !defined(WINNT) && !defined (GUI) && defined(WANT_DETACH)
#ifndef PUBLIC_ACCESS
pid_t pid;
	/* 
	 * this is written so that a running bx client will attempt to
	 * detach and then later a screen type program can be used to re-attach 
	 * to us. 
	 */	
	if (already_detached)
		return;
	if (args && *args)
	{
		char *p;
		if ((p = next_arg(args, &args)))
			malloc_strcpy(&old_pass, cryptit(p));
	}
	/*
	 * create a server socket with the above name.
	 */
	if (create_ipc_socket())
		return;

	switch(pid = fork())
	{
		case -1:
			bitchsay("fork() failed: %s", strerror(errno));
			return;
		default:
			do_detach(pid, args);
			/* Not reached */
			return;
		case 0:
			break;
	}

	setpgid(0, 0);
	already_detached = 1;
	use_input = 0;

	close_detach_fd();
	kill_attached_if_needed(1);
	disable_stop();
	do_check_pid = 1;
	setup_pid();
#endif
#endif
}

BUILT_IN_COMMAND(spam)
{
char *c;
ChannelList *chan = NULL;
	if (!args || !*args || (from_server == -1))
		return;
	c = next_arg(args, &args);
	c = make_channel(c);
	if (!c || !args || !*args)
	{
		bitchsay("Please specify channel(s) to compare");
		return;
	}
	if ((chan = lookup_channel(c, from_server, 0)))
	{
		NickList *u, *u1 = NULL;		
		for (u = next_nicklist(chan, NULL); u; u = next_nicklist(chan, u))
		{
			ChannelList *chan1;
			char *tmpc, *t;
			tmpc = LOCAL_COPY(args);

			while ((t = next_arg(tmpc, &tmpc)))
			{
				if (!(t = make_channel(t)))
					continue;
				chan1 = lookup_channel(t, from_server, 0);
				if (!chan1 || (chan1 == chan))
					continue;
				if (!(u1 = find_nicklist_in_channellist(u->nick, chan1, 0)))
					continue;
				bitchsay("Found %s!%s on %s", u1->nick, u1->host, chan1->channel);
			}
		}
	}
}

BUILT_IN_COMMAND(send_kline)
{
	char 	*dur,
		*target = NULL,
		*t;
	time_t	t_time = DEFAULT_TKLINE_TIME;
	int	tkline = 0;

	if (*command == 'U')
	{
		while ((target = next_arg(args, &args)))
		{
			while ((t = next_in_comma_list(target, &target)))
			{
				if (!t || !*t) break;
				send_to_server("%s %s", command, t);
			}
		}
		return;
	}
	dur = next_arg(args, &args);

	if (*command == 'D')
		target = dur;
	else if (*command == 'K' || *command == 'T')
	{
		if (*command == 'T') /* timed kline */
			command++, tkline++;
		if (dur && is_number(dur))
		{
			int l;
			if ((l = my_atol(dur)))
				t_time = l;
			target = next_arg(args, &args);
		}
		else
			target = dur;
	}
	while (args && *args == ':') 
		args++;
	if (!args || !*args)
	{
		bitchsay("Specify a reason for your %s", command);
		return;
	}
	bitchsay("Sending %s for %s : %s", command, target, args);
	while ((t = next_in_comma_list(target, &target)))
	{
		if (!t || !*t) break;
		if (tkline)
			send_to_server("%s %u %s :%s", command, t_time, t, args);
		else
			send_to_server("%s %s :%s", command, t, args);
	}
}

typedef void (*cvsfunc)(char *);

cvsfunc cvsfuncs[] = {
	alias_c,
	alist_c,
	array_c,
	banlist_c,
	botlink_c,
	cdcc_c,
	chelp_c,
	commands_c,
	commands2_c,
	cset_c,
	ctcp_c,
	dcc_c,
	debug_c,
	encrypt_c,
	exec_c,
	files_c,
	flood_c,
	fset_c,
	functions_c,
	funny_c,
	hash_c,
	help_c,
	history_c,
	hook_c,
	if_c,
	ignore_c,
	input_c,
	irc_c,
	ircaux_c,
	keys_c,
	lastlog_c,
	list_c,
	log_c,
	mail_c,
	misc_c,
	modules_c,
	names_c,
	network_c,
	newio_c,
	notice_c,
	notify_c,
	numbers_c,
	output_c,
	parse_c,
	queue_c,
	readlog_c,
	reg_c,
	screen_c,
	server_c,
	stack_c,
	status_c,
	struct_c,
	tcl_public_c,
	term_c,
	timer_c,
	translat_c,
	user_c,
	userlist_c,
	vars_c,
	who_c,
	whowas_c,
	window_c,
	words_c,
	0
};

BUILT_IN_COMMAND(show_revisions)
{
	int z = 0, found = 0;
	char revisionbuf[100];

	while(cvsfuncs[z])
	{
		cvsfuncs[z](revisionbuf);
		/* If there are args... we look for a matching filename or start string */
		if(args && *args)
		{
			if(my_strnicmp(&revisionbuf[5], args, strlen(args)) == 0)
			{
				say(revisionbuf);
				found = 1;
			}
		}
		else
		{
			say(revisionbuf);
			found = 1;
		}
		z++;
	}
	if(!found)
		say("Could not find any revisions.");
}
