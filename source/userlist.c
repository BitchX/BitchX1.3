/*
 *   CDE created userlist functions.
 *   Copyright Colten Edwards 04/10/96
 *
 */
 
#include "irc.h"
static char cvsrevision[] = "$Id: userlist.c 137 2011-09-06 06:48:57Z keaston $";
CVS_REVISION(userlist_c)
#include "struct.h"

#include "server.h"
#include "commands.h"
#include "encrypt.h"
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
#include "dcc.h"
#include "output.h"
#include "exec.h"
#include "notify.h"
#include "numbers.h"
#include "status.h"
#include "list.h"
#include "struct.h"
#include "timer.h"
#include "whowas.h"
#include "misc.h"
#include "userlist.h"
#include "parse.h"
#include "hash2.h"
#include "cset.h"
#define MAIN_SOURCE
#include "modval.h"

#include <sys/types.h>
#include <sys/stat.h>


/*
 *  Shit levels
 *  1 no opz
 *  2 Auto Kick
 *  3 Auto Ban Kick
 *  4 perm ban. all times.
 *  5 perm ignore all
 *
 *  User levels
 *  25 ctcp invite and whoami
 *  50 ops, chops and unban
 *  90 no Kick/Dop flood checking
 *
 *  Auto-op levels
 *  0 auto-op off
 *  1 timed autoop
 *  2 instant op
 *  3 delay +v
 *  4 instant +v
 *
 *  Protection levels
 *  1 Reop if de-oped
 *  2 De-op offender
 *  3 Kick  offender
 *  4 KickBan offender
 */

int user_count = 0;
int shit_count = 0;

extern char *FromUserHost;
#define PERM_IGNORE 5
/* CDE this shouldn't be here at all */
#define IGNORE_REMOVE 1


extern UserList *user_list;
ShitList *shitlist_list = NULL;
LameList *lame_list = NULL;
WordKickList *ban_words = NULL;
extern AJoinList *ajoin_list;

extern void save_idle (FILE *);
extern void save_banwords (FILE *);
extern int  save_formats (FILE *);
extern void sync_nicklist (UserList *, int);
extern void sync_shitlist (ShitList *, int);

#ifdef WANT_USERLIST

#define COMPATIBILITY 1

char *strflags[] = {"VOICE", "OPS", "BAN", "UNBAN", "INVITE", "DCC", "TCL", "I_OPS", "FLOOD", "BOT", NULL};
#define NUMBER_OF_FLAGS (sizeof(strflags) / sizeof(char *))

char *protflags[] = {"REOP", "DEOP", "KICK", "PBAN", "PINVITE", "UOPS", "UPROT", "CTCP", NULL};
#define NUMBER_OF_PROT (sizeof(protflags) / sizeof(char *))

unsigned long convert_str_to_flags(char *str)
{
char *p;
register int i;
register unsigned long j;
unsigned long flags = 0;
int done;
	if (!str || !*str)
		return 0;
	if (!my_strnicmp("FRIEND", str, 6))
		flags = ADD_FRIEND;
	else if (!my_strnicmp("MASTER", str, 6))
		flags = ADD_MASTER;
	else if (!my_strnicmp("OWNER", str, 5))
		flags = ADD_OWNER;

	while ((p = next_in_comma_list(str, &str)))
	{
		if (!*p)
			break;
		upper(p);
		done = 0;
		for (i = 0, j = 1; strflags[i]; i++, j <<= 1 )
		{
			if (!strcmp(p, strflags[i]))
			{
				flags |= j;	
				done = 1;
				break;
			}
		}
		if (!done)
		{
			for (i = 0, j = PROT_REOP; protflags[i]; i++, j <<= 1)
			{
				if (!strcmp(p, protflags[i]))
				{
					flags |= j;	
					break;
				}
			}
		}
	}
	return flags;
}

char * convert_flags_to_str(unsigned long flags)
{
unsigned int i;
unsigned long p;
static char buffer[290];
	*buffer = 0;
	for (i = 0, p = 1; strflags[i]; i++, p <<= 1)
	{
		if (flags & p)
		{
			strmcat(buffer, strflags[i], 280);
			strmcat(buffer, ",", 280);
		}
	}
	for (i = 0, p = PROT_REOP; protflags[i]; i++, p <<= 1)
	{
		if (flags & p)
		{
			strmcat(buffer, protflags[i], 280);
			strmcat(buffer, ",", 280);
		}
	}
	if (*buffer)
		chop(buffer, 1);
	return buffer;
}

char * convert_flags(unsigned long flags)
{
unsigned int i;
unsigned long p;
static char buffer[40];
char *q;
	*buffer = 0;
	q = buffer;
	for (i = 0, p = 1; strflags[i]; i++, p <<= 1)
	{
		if (flags & p)
			*q = '1';
		else
			*q = '0';
		q++;
	}
	for (i = 0, p = PROT_REOP; protflags[i]; i++, p <<= 1)
	{
		if (flags & p)
			*q = '1';
		else
			*q = '0';
		q++;
	}
	*q = 0;
	return buffer;
}

void prepare_addshit(UserhostItem *stuff, char *nick, char *args)
{
	char *uh;
	char *channels, *reason;
	char listbuf[BIG_BUFFER_SIZE+1];
	int thetype = 0, shit = 0;
	

        if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick))
        {
        	bitchsay("No such nick [%s] found", nick);
                return;
        }

	thetype = my_atol(args);
	next_arg(args, &args);
	shit = my_atol(args);
	next_arg(args, &args);
	channels = next_arg(args, &args);
	reason = args;
	
	uh = clear_server_flags(stuff->user);
	while (strlen(uh) > 7) uh++;
	sprintf(listbuf, "*%s@%s", uh, stuff->host);
	add_to_a_list(listbuf, thetype, "*", channels, reason, shit);
}

void add_userhost_to_userlist(char *nick, char *uhost, char *channels, char *passwd, unsigned int flags) 
{
UserList *uptr;
	uptr = new_malloc(sizeof(UserList));
	uptr->nick = m_strdup(nick);
	uptr->host = m_strdup(uhost); 
	uptr->channels = m_strdup(channels);
	if (passwd)
		malloc_strcpy(&uptr->password, passwd);
	uptr->flags = flags;
	uptr->time = now;
	add_userlist(uptr);
	sync_nicklist(uptr, 1);
	user_count++;
}

int remove_userhost_from_userlist(char *host, char *channel)
{
UserList *uptr = NULL;
int count = 0;
	if ((uptr = find_userlist(host, channel, 1)))
	{
		sync_nicklist(uptr, 0);
		new_free(&uptr->nick);
		new_free(&uptr->host);
		new_free(&uptr->channels);
		new_free(&uptr->password);
		new_free(&uptr->comment);
		new_free(&uptr);
		user_count--;
		count++;
	}
	return count;
}

void prepare_adduser(UserhostItem *stuff, char *nick, char *args)
{
	int 	thetype = 0;
unsigned long	flags = 0;
	int	ppp = 0;
	UserList *uptr = NULL;
			
	char	*channels = NULL, 
		*passwd = NULL,
		*p = NULL,
		*uh,
		*e_host,
		*host;
	
	
        if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick))
        {
		bitchsay("No such nick [%s] found", nick);
		return;
        }

	thetype = my_atol(args);
	next_arg(args, &args);
	ppp = my_atol(args);
	next_arg(args, &args);

	channels = next_arg(args, &args);

	p = next_arg(args, &args);	
	flags = convert_str_to_flags(p);

	passwd = next_arg(args, &args);
	
	uh = clear_server_flags(stuff->user);
	if (*stuff->user == '~' || *stuff->user == '^')
	{
		while ((strlen(uh) > 8))
			uh++;
		host = m_sprintf("*%s@%s", uh, ppp ? cluster(stuff->host) : stuff->host);
	}
	else
		host = m_sprintf("%s@%s", uh, ppp ? cluster(stuff->host) : stuff->host);

	e_host = m_sprintf("%s@%s", stuff->user, stuff->host);

	if (thetype == USERLIST_ADD)
	{
		char *e_pass = NULL;
		if (passwd)
			e_pass = cryptit(passwd);
		if (!(uptr = lookup_userlevelc(stuff->nick, e_host, channels, NULL)))
		{
			add_userhost_to_userlist(stuff->nick, host, channels, e_pass, flags);
			send_to_server("NOTICE %s :You have been added to my Userlist as *!%s with [%s]", nick, host, convert_flags_to_str(flags));
			send_to_server("NOTICE %s :You are allowed on channels [%s]", nick, channels);
			if (passwd)
				send_to_server("NOTICE %s :Your password is [%s]", nick, passwd);
			put_it("%s", convert_output_format("$G Added [*!$0] on $1. Password is [$2] and flags are [$3-]", "%s %s %s %s", host, channels, passwd?passwd:"None", convert_flags_to_str(flags)));
		}
		else if (passwd)
		{
			send_to_server("NOTICE %s :Your password is %s[%s]", nick, uptr->password ? "changed to " : "", passwd);
			malloc_strcpy(&uptr->password, e_pass);
		}
	}
	else
	{
		if (!(remove_userhost_from_userlist(host,  channels)))
			bitchsay("User not found on the userlist");
		else
			put_it("%s", convert_output_format("$G Removed [$0] on $1 from userlist", "%s %s", host, channels));
	}
	new_free(&e_host);
	new_free(&host);
}

void remove_all(int type)
{
UserList *tmp;
ShitList *tmp_s, *next_s;
ChannelList *chan;
NickList *nick;
void *location = NULL;
int i = 0;
	
	if (type == -1 || type == USERLIST_REMOVE)
	{
		int size = -1;
		while ((tmp = next_userlist(NULL, &size, &location)))
		{
			if ((tmp = find_userlist(tmp->host, tmp->channels, 1)))
			{
				new_free(&tmp->nick);
				new_free(&tmp->host);
				new_free(&tmp->channels);
				new_free(&tmp->password);
				new_free(&tmp->comment);
				new_free((char **)&tmp);
			}
		}

		user_count  = 0;
		user_list = NULL;
		for (i = 0; i < server_list_size(); i ++)
		{
			for (chan = get_server_channels(i); chan; chan = chan->next)
			{
				for(nick = next_nicklist(chan,NULL); nick; nick = next_nicklist(chan, nick))
					nick->userlist = NULL;	
			}
		}
	}
	if (type == -1 || type == SHITLIST_REMOVE)
	{
		for (tmp_s = shitlist_list; tmp_s; tmp_s = next_s)
		{
			next_s = tmp_s->next;
			sync_shitlist(tmp_s, 0);
			new_free(&tmp_s->filter);
			new_free(&tmp_s->channels);
			new_free(&tmp_s->reason);
			new_free((char **)&tmp_s);
		}
		for (i = 0; i < server_list_size(); i ++)
		{
			for (chan = get_server_channels(i); chan; chan = chan->next)
			{
				for(nick = next_nicklist(chan,NULL); nick; nick = next_nicklist(chan, nick))
						nick->shitlist = NULL;	
			}
		}
		shit_count = 0;
		shitlist_list = NULL;
	}
}

BUILT_IN_COMMAND(add_user)
{
	char *nick, *capabilities = NULL;
	char *passwd = NULL;
	char *channels = NULL;
	char *bang, *at;
	int type = USERLIST_ADD;
	int ppp = 0;
#ifdef COMPATIBILITY
	char buffer[BIG_BUFFER_SIZE+1];
	*buffer = 0;
#endif
	
	if (command && !my_stricmp(command,"UNUSER"))
		type = USERLIST_REMOVE; 

	if ((nick = next_arg(args, &args)) && *nick)
	{
		while (nick && *nick == '-')
		{
			if ((type == USERLIST_REMOVE) && !my_stricmp(nick, "-ALL"))
			{
				bitchsay("Removing all Users on Userlist");
				remove_all(type);
				return;
			}
			if (!my_stricmp(nick, "-PPP"))
			{
				ppp = 1;
				nick = next_arg(args, &args);
			} else
				nick = next_arg(args, &args);
		}
		if (!nick || !*nick)
			return;
		if (!(channels = next_arg(args, &args)) || !*channels)
		{
			bitchsay("Need a channel for %s", command ? "UnUser" : "AddUser");
			return;
		}
#ifdef COMPATIBILITY
		capabilities = next_arg(args, &args);
		passwd = next_arg(args, &args);
		if (passwd && is_number(passwd) && args && *args)
		{
			int aop = my_atol(passwd);
			int level = my_atol(capabilities);
			int prot = my_atol(next_arg(args, &args));
			if (aop == 1 || aop == 2)
				strcpy(buffer, "OPS,");
			else if (aop == 3 || aop == 4)
				strcpy(buffer, "VOICE,");
			passwd = next_arg(args, &args);
			if (level >= 25)
				strcat(buffer, "INVITE,");
			if (level >= 49)
				strcat(buffer, "UNBAN,");
			if (level >= 90)
				strcat(buffer, "FLOOD,");
			switch(prot)
			{
				case 1:
					strcat(buffer, "REOP,");
					break;
				case 2:
					strcat(buffer, "DEOP,");
					break;
				default:
					break;
			}
			capabilities = buffer;
			if (*buffer)
				chop(buffer, 1);

		}
#else
		capabilities = next_arg(args, &args);
		passwd = next_arg(args, &args);
#endif
		if ((bang = strchr(nick, '!')) && (at = strchr(nick, '@')))
		{
			char *p = nick;
			char *newuh = NULL;
			char *e_pass = passwd;
			*bang++ = 0;
			if (passwd)
			{
				extern int loading_savefile;
				if (!loading_savefile)
					e_pass = cryptit(passwd);
				else
					e_pass = passwd;
			}
			if (ppp)
			{
				*at++ = 0;
				newuh = m_sprintf("%s@%s", bang, cluster(at));
			}			
			if (type == USERLIST_ADD)
				add_userhost_to_userlist(p, newuh? newuh : bang, channels, e_pass, convert_str_to_flags(capabilities));
			else
				remove_userhost_from_userlist(newuh ? newuh : bang, channels);
			bitchsay("%s %s!%s %s Userlist", type == USERLIST_ADD ? "Adding":"Deleting", p, newuh ? newuh : bang,
					 type == USERLIST_ADD ? "to":"from");
			new_free(&newuh);
		}
		else
		{
			char format[80];
			
			strcpy(format, "%d %d %s");
			if (capabilities)
				strcat(format, " %s");
			if (passwd)
				strcat(format, " %s");
			if (passwd)
				userhostbase(nick, prepare_adduser, 1, format, type, ppp, 
					channels, capabilities, passwd);
			else if (capabilities)
				userhostbase(nick, prepare_adduser, 1, format, type, ppp, 
					channels, capabilities);
			else
				userhostbase(nick, prepare_adduser, 1, format, type, ppp, channels);
		}
	} 
}


BUILT_IN_COMMAND(add_shit)
{
	char *nick, *ptr;
	int level = 1;
	char *reason = NULL;
	char *channels = NULL;
	int type = SHITLIST_ADD;
	

	
	if (command && *command && !my_stricmp(command, "UNSHIT"))
		type = SHITLIST_REMOVE;
	if (args && *args)
	{
		char *bang = NULL;
		char *atsign = NULL;
		char *cn;
		nick = next_arg(args, &args);
		if (!nick || !*nick)
			return;

		if (type == SHITLIST_REMOVE && !my_stricmp(nick, "-ALL"))
		{
			bitchsay("Removing all Users on shitlist");
			remove_all(SHITLIST_REMOVE);
			return;
		}
		channels = next_arg(args, &args);
		if (!channels || !*channels) {
			bitchsay("Need a channel for %s", 
				type == SHITLIST_REMOVE ? "UnShit": "AddShit");
			return;
		}
		ptr = next_arg(args, &args);
		if (ptr && *ptr)
			level = my_atol(ptr);

		if (args && *args)
			malloc_strcpy(&reason, args);
		else
			malloc_strcpy(&reason, "ShitListz");

		bang = strchr(nick, '!');
		if (bang)
			atsign = strchr(bang, '@');
		else
			atsign = strchr(nick, '@');
		
		if (!bang && !atsign && (cn = check_nickname(m_strdup(nick)))) {
			userhostbase(cn, prepare_addshit, 1, "%d %d %s %s", type, level, channels, reason);
			new_free(&cn);
		}
		else
		{
			/* 
			 *  *!*@*test*
			 *  *!*test@*
			 *  *!*  bang !atsign
			 *  *@*  !bang atsign
			 *  *test* !bang !atsign
			 */
			char *uh;

			if (bang) {
				*bang++ = '\0';
				uh = bang;
			} else {
				uh = nick;
				nick = "*";
			}
			
			add_to_a_list(uh, type, nick, channels, reason, level);
		}
		new_free(&reason);
	} 
}

void sync_nicklist(UserList *added, int type)
{
ChannelList *chan;
NickList *nick;
int i;
char *check;
	check = clear_server_flags(added->host);	
	for (i = 0; i < server_list_size(); i ++)
	{
		for (chan = get_server_channels(i); chan; chan = chan->next)
		{
			for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
			{
				if (wild_match(check, nick->host))
				{
					if (type) 
					{
						nick->userlist = added;
						check_auto(chan->channel, nick, chan);
					}
					else 
						nick->userlist = NULL;	
				}
			}
		}
	}
}

void sync_shitlist(ShitList *added, int type)
{
ChannelList *chan;
NickList *nick;
int i;
char tmp[BIG_BUFFER_SIZE+1];
char *check;
	
	for (i = 0; i < server_list_size(); i ++)
	{
		for (chan = get_server_channels(i); chan; chan = chan->next)
		{
			for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
			{
				check = clear_server_flags(nick->host);
				sprintf(tmp, "%s!%s", nick->nick, check);
				if (wild_match(added->filter, tmp))
				{
					if (type) 
					{
						nick->shitlist = added;
						check_auto(chan->channel, nick, chan);
					}
					else
					{
						BanList *b = NULL;
						nick->shitlist = NULL;
						if ((b = ban_is_on_channel(tmp, chan)) && !eban_is_on_channel(tmp, chan))
							add_mode(chan, "b", 0, b->ban, NULL, get_int_var(NUM_BANMODES_VAR));
					}
				}
			}
			flush_mode_all(chan);
		}
	}
}

void add_to_a_list(char *thestring, int thetype, char *nick, char *channels, char *reason, int shitlevel)
{
	ShitList *sremove = NULL;
	int scount = 0;
	switch(thetype)
	{
		case SHITLIST_ADD:
		{
			if (!(sremove = nickinshit(nick, thestring)))
			{
				shit_count++;
				sremove = (ShitList *) new_malloc(sizeof(ShitList));
				sremove->level = shitlevel;
				sremove->reason = m_strdup(reason);
				sremove->channels = m_strdup(channels);
				sremove->filter = m_sprintf("%s!%s", nick, thestring);
				add_to_list((List **)&shitlist_list, (List *)sremove);
				sync_whowas_addshit(sremove);
				sync_shitlist(sremove, 1);
				if (shitlevel == PERM_IGNORE)
					ignore_nickname(sremove->filter, IGNORE_ALL, 0);
				bitchsay("Adding %s!%s to Shitlist", nick, thestring);
			}
			else
				bitchsay ("%s!%s already on my Shitlist", nick, thestring);
			break;
		}
		case SHITLIST_REMOVE:
		{
			char *s_str;
			s_str = m_sprintf("%s!%s", nick, thestring);
			while ((sremove = (ShitList *)removewild_from_list((List **)&shitlist_list, s_str)))
			{
				shit_count--;
				scount++;
				if (sremove->level == PERM_IGNORE)
					ignore_nickname(sremove->filter, IGNORE_ALL, IGNORE_REMOVE); 
				sync_whowas_unshit(sremove);
				sync_shitlist(sremove, 0);
				new_free(&sremove->filter);
				new_free(&sremove->reason);
				new_free(&sremove->channels);
				new_free((char **)&sremove);
				bitchsay("Deleting %s!%s from Shitlist", nick, thestring);
			}
			if (!scount)
				bitchsay("Didnt find %s!%s on the Shitlist", nick, thestring);
			new_free(&s_str);
			break;
		}
	}	
}

BUILT_IN_COMMAND(showuserlist)
{
	UserList *tmp;
	void *location = NULL;
	int first = 0;
	int hook = 0;
	int size = -1;
	char *p, *channel = NULL, *hostname = NULL;

	
	if (args && *args)
	{
		while ((p = next_arg(args, &args)))
		{
			if (is_channel(p))
				channel = p;
			else
				hostname = p;
		}
	}

	for (tmp = next_userlist(NULL, &size, &location); tmp; tmp = next_userlist(tmp, &size, &location))
	{
		if (channel)
			if (!wild_match(tmp->channels, channel))
				continue;
		if (hostname)
			if (!wild_match(hostname, tmp->host))
				continue;
				
		if (!first++ && (hook = do_hook(USERLIST_HEADER_LIST, "%s %s %s %s %s", "Level","Nick","Password","Host","Channels")))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_USERLIST_HEADER_FSET), NULL)); 
		if (do_hook(USERLIST_LIST, "%lu %s %s %s %s", tmp->flags, tmp->nick,tmp->host,tmp->channels,tmp->password?tmp->password:empty_string))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_USERLIST_FSET), 
				"%s %s %s %s %s", convert_flags(tmp->flags), tmp->nick, tmp->password?tmp->password:"<none>", tmp->host, tmp->channels));
	}
	if (first && hook && do_hook(USERLIST_FOOTER_LIST, "%s", "End of userlist"))
		put_it("%s", convert_output_format(fget_string_var(FORMAT_USERLIST_FOOTER_FSET), "%s %d", update_clock(GET_TIME), first)); 
}

BUILT_IN_COMMAND(showshitlist)
{
	ShitList *tmp = shitlist_list;
	int first = 0;
	int hook = 0;

	
	if (!tmp)
	{
		bitchsay("No entries in Shit list");
		return;
	}
	while (tmp)
	{
		if (!first++ && (hook = do_hook(SHITLIST_HEADER_LIST,"%s %s %s %s %s %s", "Lvl","Nick","Channels","Reason", empty_string, empty_string)))
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SHITLIST_HEADER_FSET), NULL)); 
		if (do_hook(SHITLIST_LIST, "%d %s %s %s %s %s", tmp->level, tmp->filter,tmp->channels, tmp->reason?tmp->reason:"<none>", empty_string, empty_string))
		{
			put_it("%s", convert_output_format(fget_string_var(FORMAT_SHITLIST_FSET), 
				"%d %s %s %s", tmp->level, tmp->filter, tmp->channels, tmp->reason?tmp->reason:"<No Reason"));
		}
		tmp = tmp->next;
	}
	if (first && hook && do_hook(SHITLIST_FOOTER_LIST, "%s", "End of shitlist"))
		put_it("%s", convert_output_format(fget_string_var(FORMAT_SHITLIST_FOOTER_FSET), "%s %d", update_clock(GET_TIME), shit_count)); 
}

BUILT_IN_COMMAND(set_user_info)
{
int size = -1;
	
	if (command)
	{
		UserList *tmp;
		void *location = NULL;
		int count = 0;
		for (tmp = next_userlist(NULL, &size, &location); tmp; tmp = next_userlist(tmp, &size, &location))
		{
			if (tmp->comment)
			{
				put_it("%s", convert_output_format("$[10]0 $[25]1 %K[%C$2-%K]", "%s %s %s", tmp->nick, tmp->host, tmp->comment));
				count++;
			}
			
		}
		if (count)
			bitchsay("%d entries with a comment", count);
		else
			bitchsay("There are no comments");
		return;
	}
	if (args && *args)
	{
		char *uh;
		UserList *tmp = NULL;
		void *location = NULL;
		size = -1;
		uh = next_arg(args, &args);
		for (tmp = next_userlist(NULL, &size, &location); tmp; tmp = next_userlist(tmp, &size, &location))
		{
			if (wild_match(uh, tmp->nick) || wild_match(uh, tmp->host))
			{
				if (args && *args)
					malloc_strcpy(&tmp->comment, args);
				else
					new_free(&tmp->comment);
				bitchsay("%s info for %s", (args && *args)?"Added":"Removed", uh);
			}
		}
	}
}

/*
 * Function courtesy of Sheik. From his CtoolZ client.
 * but modified a little by panasync
 */
UserList *lookup_userlevelc(char *nick, char *userhost, char *channel, char *passwd)
{
	
	if (!nick || !userhost || !*userhost || !channel)
		return NULL;
	return find_bestmatch(nick, userhost, channel, passwd);
}

ShitList *nickinshit(char *niq, char *uh)
{
	char theuh[BIG_BUFFER_SIZE+1];
register ShitList *thisptr = shitlist_list;
	char *u;
	

	
	if (!uh || !niq)
		return NULL;
	u = clear_server_flags(uh);
	sprintf(theuh, "%s!%s", niq, u);
	while (thisptr)
	{
		if (!strcmp(thisptr->filter, theuh) || (/*wild_match(theuh, thisptr->filter) || */wild_match(thisptr->filter, theuh))) 
			return(thisptr);
		thisptr = thisptr->next;
	}
	return NULL;
}

int find_user_level(char *from, char *host, char *channel)
{
UserList *tmp;
	if ((tmp = (UserList *) lookup_userlevelc(from, host, channel, NULL)))
		return tmp->flags;
	return 0;
}

int find_shit_level(char *from, char *host, char *channel)
{
register ShitList * tmp = shitlist_list;

	
	if (!shitlist_list)
		return 0;
	if (!(tmp = (ShitList *) nickinshit(from, host)))
		return 0;
	if (check_channel_match(tmp->channels, channel))
		return tmp->level;
	return 0;
}


int real_check_auto (void *arg, char *sub)
{
	char *nick, *host, *channel;
	char *p = (char *)arg;
	char *args = (char *)arg;
	char *serv_num = NULL;
	int  this_server = from_server;	

	
	
	channel = next_arg(args, &args);
	nick = next_arg(args, &args);
	host = next_arg(args, &args);
	if ((serv_num = next_arg(args, &args)))
		from_server = my_atol(serv_num);
	
	if (channel && *channel && nick && *nick && host && *host)
	{
		ChannelList *chan;
		if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)))
			check_auto(channel, find_nicklist_in_channellist(nick, chan, 0), NULL);
	}
	this_server = from_server;
	new_free(&p);
	return 0;
}

int delay_check_auto (char *channel)
{
	ChannelList *chan = NULL;
	char *buffer = NULL;
	NickList *possible;	

	
	if (!channel || !*channel)
		return -1;
		
	if ((chan = lookup_channel(channel, from_server, CHAN_NOUNLINK)) == NULL)	
		return -1;

	for (possible = next_nicklist(chan, NULL); possible; possible = next_nicklist(chan, possible))
	{
		if ((possible->shitlist || possible->userlist) && (!(possible->sent_reop < 4) || !(possible->sent_deop < 4)))
		{
			malloc_sprintf(&buffer, "%s %s %s %d", channel, possible->nick, possible->host, from_server);
			add_timer(0, empty_string, 3 * 1000, 1, real_check_auto, buffer, NULL, -1, "check-auto");
		}
	}
	add_timer(0, empty_string, 5 * 1000, 1, delay_flush_all, m_sprintf("%s %d", channel, from_server), NULL, -1, "check-auto");
	return 0;
}

int delay_opz (void *arg, char *sub)
{
char * args = (char *)arg;
char * from = NULL;
char * host = NULL;
char * channel = NULL;	
char * mode = NULL;
char * serv_num = NULL;
ChannelList *chan = NULL;
int	this_server = from_server;
char *p = (char *) arg; /* original args unmodified  so we can free them */
	

	channel = next_arg(args, &args);
	from = next_arg(args, &args);
	host = next_arg(args, &args);
	mode = next_arg(args, &args);

	if ((serv_num = next_arg(args, &args)))
		this_server = my_atol(serv_num);
	chan = lookup_channel(channel, this_server, 0);
	if (chan && is_on_channel(channel, this_server, from) && chan->have_op) 
	{
		NickList *nick;
		for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
		{
			if (!my_stricmp(nick->nick, from))
			{
				if (!my_stricmp(host, nick->host))
					break;
				else
				{
					new_free(&p); new_free(&sub);
					return 0;
				}
			}
		}
		if (nick && ((!nick_isop(nick) && *mode == 'o') || (!nick_isvoice(nick) && *mode == 'v')))
		{
			my_send_to_server(this_server, "MODE %s +%s %s", channel, mode, from); 
			if (get_int_var(SEND_OP_MSG_VAR))
				my_send_to_server(this_server, "NOTICE %s :You have been delay Auto-%s'd", from, *mode == 'o'? "op":"voice");
		}
	}
	new_free(&p); new_free(&sub);
	return 0;
}


static char *protected = NULL;

int delay_kick (void *arg, char *sub)
{
char * args = (char *)arg;
char * from = NULL;
char * channel = NULL;	
char * serv_num = NULL;
int this_server = from_server;
int server;
ChannelList *chan;
char *p = (char *) arg; /* original args unmodified  so we can free them */

	if (protected)
	{
		from = next_arg(args, &args);
		channel = next_arg(args, &args);
		if ((serv_num = next_arg(args, &args)))
			this_server = my_atol(serv_num);
		if ((chan = prepare_command(&server, channel, PC_SILENT)))
			my_send_to_server(this_server, "KICK %s %s :\002%s\002 Kick/ban me will ya", channel, from, _VERSION_);
		new_free(&protected);
	}
	new_free(&p); new_free(&sub);
	return 0;
}

void run_user_flag(char *name, char *what, NickList *user, NickList *kicker)
{
	char *tmp = NULL;
	char *name_copy, *stuff_copy;
	int sa = 0;
	char buffer[BIG_BUFFER_SIZE*5+1];
	*buffer = 0;
	strmopencat(buffer, BIG_BUFFER_SIZE*5, user->nick, space, user->host, space, user->ip, space, user->userlist?one:zero, space, NULL);
	if (user->userlist)
		strmopencat(buffer, BIG_BUFFER_SIZE*5, user->userlist->nick, space, user->userlist->host, space, user->userlist->channels, space, ltoa(user->userlist->flags), space, NULL);
	if (kicker)
	{
		strmopencat(buffer, BIG_BUFFER_SIZE*5, kicker->nick, space, kicker->host, space, kicker->ip, space, kicker->userlist?one:zero, NULL);
		if (kicker->userlist)
			strmopencat(buffer, BIG_BUFFER_SIZE*5, space, kicker->userlist->nick, space, kicker->userlist->host, space, kicker->userlist->channels, space, ltoa(kicker->userlist->flags), NULL);
	}
	if ((tmp = expand_alias(what, empty_string, &sa, NULL)))
	{
		stuff_copy = alloca(strlen(tmp) + 1);
		name_copy = LOCAL_COPY(name);
		stuff_copy = LOCAL_COPY(tmp);
		will_catch_return_exceptions++;
		parse_line(name_copy, stuff_copy, buffer, 0, 0, 1);
		will_catch_return_exceptions--;
		return_exception = 0;
	}
	new_free(&tmp);
}

NickList *check_auto(char *channel, NickList *nicklist, ChannelList *chan)
{

ShitList *shitptr = NULL;
UserList *userptr = NULL;
ChannelList *chan_ptr =  NULL;
char *ban;

	
	if (!channel || !*channel || !nicklist)
		return NULL;
	
	if (!chan)
	{
		ChannelList *chan2;
		chan2 = get_server_channels(from_server);
		chan_ptr = (ChannelList *) find_in_list((List **)&chan2, channel, 0);
	}
	else
		chan_ptr = chan;
	if (!chan_ptr)
		return NULL;		

	if (!chan_ptr->have_op)
		return nicklist;
	userptr = nicklist->userlist;
	shitptr = nicklist->shitlist;


	if (userptr && !check_channel_match(userptr->channels, channel))
		userptr = NULL;
	if (shitptr && (!check_channel_match(shitptr->channels, channel) || isme(nicklist->nick)))
		shitptr = NULL;
		
	if (get_cset_int_var(chan_ptr->csets, SHITLIST_CSET) && shitptr != NULL && userptr == NULL)
	{
		char *theshit;
		time_t current = now;
		theshit = get_string_var(SHITLIST_REASON_VAR);
		switch(shitptr->level)
		{
		
			case 0:
				return nicklist;
				break;
			case 1:/* never give opz */
				if (nicklist->sent_deop < 4 && nick_isop(nicklist))
				{
					add_mode(chan_ptr, "o", 0, nicklist->nick, NULL, get_int_var(NUM_OPMODES_VAR));
					nicklist->sent_deop++;
					nicklist->sent_deop_time = current;
				}
				break;				
			case 2: /* Auto Kick  offender */
				add_mode(chan_ptr, NULL, 0, nicklist->nick, shitptr->reason?shitptr->reason:theshit, 0);
				break;
			case 3: /* kick ban the offender */
			case 4:	/* perm ban on offender */
				if (nicklist->sent_deop  < 4 || (nicklist->sent_deop < 4 && shitptr->level == 4))
				{
					send_to_server("MODE %s -o+b %s %s", channel, nicklist->nick, shitptr->filter);
					nicklist->sent_deop++;
					nicklist->sent_deop_time = current;
					if (get_int_var(AUTO_UNBAN_VAR) && shitptr->level != 4)
						add_timer(0, empty_string, get_int_var(AUTO_UNBAN_VAR) * 1000, 1, timer_unban, m_sprintf("%d %s %s", from_server, channel, shitptr->filter), NULL, -1, "auto-unban");
				}
				if (get_cset_int_var(chan_ptr->csets, KICK_IF_BANNED_CSET))
					send_to_server("KICK %s %s :%s", channel,
						nicklist->nick, (shitptr->reason && *shitptr->reason) ? shitptr->reason : theshit);
			default:
				break;
		}
		return nicklist;
	} 

	if (userptr && get_cset_int_var(chan_ptr->csets, USERLIST_CSET))
	{	
		char *buffer = NULL;
		time_t current = now;
		int done = 0;
		char *p = NULL;
		if (get_cset_int_var(chan_ptr->csets, AOP_CSET))
		{
			if ((userptr->flags & ADD_OPS))
			{
				done++;
				if (!userptr->password && !nicklist->sent_reop && !nick_isop(nicklist))
				{
					nicklist->sent_reop++;
					nicklist->sent_reop_time = current;
					if (!(userptr->flags & ADD_IOPS))
					{
						malloc_sprintf(&buffer, "%s %s %s %s %d", channel, nicklist->nick, nicklist->host, "o", from_server);
						add_timer(0, empty_string, 10 * 1000, 1, delay_opz, buffer, NULL, -1, "delay-ops");
					} 
					else
						send_to_server("MODE %s +o %s", chan_ptr->channel, nicklist->nick);
				}
			}
			else if ((userptr->flags & ADD_VOICE))
			{
				done++;
				if (!nicklist->sent_voice && !nick_isvoice(nicklist))
				{
					nicklist->sent_voice++;
					nicklist->sent_voice_time = current;
					if (!(userptr->flags & ADD_IOPS))
					{
						malloc_sprintf(&buffer, "%s %s %s %s %d", channel, nicklist->nick, nicklist->host, "v", from_server);
						add_timer(0, empty_string, 10 * 1000, 1, delay_opz, buffer, NULL, -1, "delay-ops");
					}
					else
						send_to_server("MODE %s +v %s", chan_ptr->channel, nicklist->nick);
				}
			}
		}
		if ((userptr->flags & USER_FLAG_OPS) && (p = get_string_var(USER_FLAG_OPS_VAR)))
		{
			done++;
			run_user_flag("USER_FLAG_OPS", p, nicklist, NULL);
		}
		if (done)
			return nicklist;
	}
	if (get_cset_int_var(chan_ptr->csets, KICK_IF_BANNED_CSET) && check_channel_match(get_string_var(PROTECT_CHANNELS_VAR), chan_ptr->channel))
	{
		char *ipban = NULL;
		char *u = NULL;
		if (!nicklist->host)
			return nicklist;
		ban = m_3dup(nicklist->nick, "!", nicklist->host);
		if (nicklist->ip && nicklist->host)
		{
			char *user = alloca(strlen(nicklist->host)+1);
			strcpy(user, nicklist->host);
			if ((u = strchr(user, '@')))
				*u = 0;
			ipban = m_opendup(nicklist->nick, "!", user, "@", nicklist->ip, NULL);
		}
		if (!isme(nicklist->nick) && (!eban_is_on_channel(ban, chan_ptr) && (ban_is_on_channel(ban, chan_ptr) || (ipban && ban_is_on_channel(ipban, chan_ptr)))) )
		{
			new_free(&ban);
			new_free(&ipban);
			if (nick_isop(nicklist) && !get_cset_int_var(chan_ptr->csets, KICK_OPS_CSET))
				return nicklist;
			my_send_to_server(from_server, "KICK %s %s :Banned", chan_ptr->channel, nicklist->nick);
		}
		new_free(&ban);
		new_free(&ipban);
	}
	return nicklist;
}




/*
 *  Protection levels
 *  1 Reop if de-oped
 *  2 De-op offender
 *  3 Kick  offender
 *  4 KickBan offender
 */
int check_prot(char *from, char *person, ChannelList *chan, BanList *thisban, NickList *n)
{
NickList *tmp = NULL;
NickList *kicker;
char *tmp_ban = NULL;
char *nick = NULL, *userhost = NULL, *p;

	
	if (!from || !*from || !person || !*person || !chan)
		return 0;

	if (!my_stricmp(person, from))
		return 0;

	tmp_ban = LOCAL_COPY(person);
	if ((p = strchr(tmp_ban, '!')))
	{
		nick = tmp_ban;
		*p++ = 0;
		userhost  = p;
	} else nick = person;

	if (!n)
	{
		if (!(tmp = find_nicklist_in_channellist(person, chan, 0)))
		{
			for (tmp = next_nicklist(chan, NULL); tmp; tmp = next_nicklist(chan, tmp))
			{
				if (wild_match(nick, tmp->nick) && wild_match(userhost, tmp->host) && tmp->userlist)
				{
					n = tmp;
					break;
				}
			}
		}
	}

	if (get_cset_int_var(chan->csets, USERLIST_CSET) && chan->have_op && ((n && n->userlist) || 
		(tmp && tmp->userlist)))
	{
		UserList *user = NULL;		
		time_t current = now;
		if (n)
			user = n->userlist;
		else
			user = tmp->userlist;
		if (!user || (user && !check_channel_match(user->channels, chan->channel)))
			return 0;
		if (!(kicker = find_nicklist_in_channellist(from, chan, 0)))
			return 0;
		if ((user->flags & (PROT_DEOP | PROT_INVITE | PROT_BAN | PROT_KICK | PROT_REOP)))
		{
			int do_reop = 0;
			if (user->flags & PROT_DEOP)
			{
				if (!kicker->sent_deop)
				{
					if (!kicker->userlist || 
						(kicker->userlist && !(kicker->userlist->flags & PROT_DEOP)))
						send_to_server("MODE %s -o %s", chan->channel, from);
					kicker->sent_deop++;
				}
				do_reop = 1;
			}

			if (user->flags & PROT_INVITE)
			{
				
				if (thisban && !thisban->sent_unban)
				{
					thisban->sent_unban++;
					thisban->sent_unban_time = current;
					send_to_server("MODE %s -b %s", chan->channel, thisban->ban);
					send_to_server("INVITE %s %s", n?n->nick:tmp->nick, chan->channel);  
				}
			}
			if (user->flags & PROT_BAN)
			{
/*				do_reop = 1; */
				if (kicker->userlist)
					send_to_server("MODE %s -o %s", chan->channel, from);
				else 
				{
					char *h, *u;
					u = LOCAL_COPY(kicker->host);
					h = strchr(u, '@');
					*h++ = 0;
					send_to_server("MODE %s -o+b %s %s", chan->channel, kicker->nick, ban_it(kicker->nick, u, h, kicker->ip));
					if (get_int_var(AUTO_UNBAN_VAR))
						add_timer(0, empty_string, get_int_var(AUTO_UNBAN_VAR) * 1000, 1, timer_unban, m_sprintf("%d %s %s", from_server, chan->channel, ban_it(kicker->nick, u, h, kicker->ip)), NULL, -1, "auto-unban");
				}
			}
			if (user->flags & PROT_KICK)
			{
				if (kicker && (!kicker->userlist || (kicker->userlist && !(kicker->userlist->flags & PROT_KICK))))
					send_to_server("KICK %s %s :\002BitchX\002 Protected User", chan->channel, kicker->nick);
			}
			if ((user->flags & PROT_REOP) || do_reop)
			{
				/* added by Sergs serg@gamma.niimm.spb.su */
			        if (thisban)
				{
					if (thisban->sent_unban_time - current > 30)
					{
						thisban->sent_unban++;
						thisban->sent_unban_time = current;
						send_to_server("MODE %s -b %s", chan->channel, thisban->ban);
					} 
					else if (thisban->sent_unban < 3)
					{
						thisban->sent_unban++;
						thisban->sent_unban_time = current;
						send_to_server("MODE %s -b %s", chan->channel, thisban->ban);
					}
				}
				if (n && (!n->sent_reop || (n->sent_reop_time && (current - n->sent_reop_time > 60))))
				{
					char *u = NULL;
					malloc_sprintf(&u, "%s %s %s %s %d", chan->channel, n->nick, n->host, "o", from_server);
					add_timer(0, empty_string, 10 * 1000, 1, delay_opz, u, NULL, -1, "delay-ops");
					n->sent_reop++;
					n->sent_reop_time = current;
				}
			}
		}
		if ((user->flags & USER_FLAG_PROT) && (p = get_string_var(USER_FLAG_PROT_VAR)))
			run_user_flag("USER_FLAG_PROT", p, n, kicker);
		return 1;
	}
	return 0;
}

void check_shit(ChannelList *chan)
{
ShitList *Bans;
	if (!chan || !chan->have_op || !shitlist_list || chan->server <= -1 || !get_cset_int_var(chan->csets, SHITLIST_CSET))
		return;
	if (!check_channel_match(get_string_var(PROTECT_CHANNELS_VAR), chan->channel))
		return;
	for (Bans = shitlist_list; Bans; Bans = Bans->next)
	{
		/* this is a permanent ban */
		if (!check_channel_match(Bans->channels, chan->channel))
			continue;
		if ((Bans->level == 4) && !ban_is_on_channel(Bans->filter, chan) && !eban_is_on_channel(Bans->filter, chan))
			add_mode(chan, "b", 1, Bans->filter, NULL, get_int_var(NUM_BANMODES_VAR));
	}
	flush_mode_all(chan);
}

BUILT_IN_COMMAND(change_user)
{
#if 0
int comm = 0;
char *user, *what;

char err_msg[7][50] = { empty_string, "No Level Specified", "No Protection level Specified", "No Channel(s) Specified", "No Auto-op Specified", "No password Specified", "No Userhost Specified"};

	
	if (!my_stricmp(command, "CHGFLAGS"))
		comm = 1;
	else if (!my_stricmp(command, "CHGCHAN"))
		comm = 3;
	else if (!my_stricmp(command, "CHGPASS"))
		comm = 5;
	else if (!my_stricmp(command, "CHGUH"))
		comm = 6;

	if (comm && (user = next_arg(args, &args)))
	{
		UserList *ThisNick;
		int prot, level, autoop;
		char *channels = NULL;
		char *password = NULL;
		char *format = NULL;
		char *userhost = NULL;
		if (!(what = next_arg(args, &args)))
		{
			bitchsay("%s %s", command, err_msg[comm]);
			return;
		}
		if (!(ThisNick = (UserList *)find_bestmatch(user, 0)))
		{
			bitchsay("Nick [%s] was not found on userlist", user);
			return;
		}	
		flags = ThisNick->flags;
		channels = ThisNick->channels;
		password = ThisNick->password;					
		userhost = ThisNick->host;
		
		switch(comm)
		{
			case 1:
				ThisNick->level = my_atol(what);
				format = "Changed level for [$0] $2 to $1";
				put_it("%s", convert_output_format(format,"%s %d %d", ThisNick->nick, ThisNick->level, level));
				break;
			case 2:
				ThisNick->channels = m_strdup(what);
				format = "Changed allowed channels for [$0] $2 to $1";
				put_it("%s", convert_output_format(format,"%s %s %s", ThisNick->nick,ThisNick->channels?ThisNick->channels:"null", channels?channels:"null"));
				new_free(&channels);
				break;
			case 3:
				ThisNick->password = m_Strdup(what);
				format = "Changed passwd for [$0] $2 to $1";
				put_it("%s", convert_output_format(format,"%s %s %s", ThisNick->nick, ThisNick->password?ThisNick->password:"null", password?password:"null"));
				new_free(&password);
			case 4:
				ThisNick->host = m_strdup(what);
				format = "Changed userhost for [$0] $2 to $1";
				put_it("%s", convert_output_format(format,"%s %s %s", ThisNick->nick, ThisNick->host, userhost));
				new_free(&userhost);
				break;
			default:
				break;
		}
	}		
#endif
	return;
}
#endif

int check_channel_match(char *tmp, char *channel)
{
	char *p, *q, *chan = NULL;
	int wmatch = 0;
	if (!tmp || !channel)
		return 0;
	if (*channel == '*' && (strlen(channel)==1))
		return 1;
	q = chan = alloca(strlen(tmp)+1);
	strcpy(chan, tmp);

	while ((p = next_in_comma_list(chan, &chan)))
	{
		if (!p || !*p)
			break;
		if (*p == '!'  && wild_match(p+1, channel))
		{
			wmatch = 0;
			break;
		}
		if (!my_stricmp(p, channel))
		{
			wmatch = strlen(channel);
			break;
		}
		else if ((wmatch = wild_match(p, channel)))
			break;
	}
	return wmatch;
}

void check_hack(char *nick, ChannelList *channel, NickList *ThisNick, char *from)
{
#ifdef WANT_USERLIST
UserList *userptr = NULL;
ShitList *shitptr = NULL;
#endif
WhowasList *check_op = NULL;

int	flag;

	if (from && *from && !strchr(from, '.'))
		return;

	if (!channel || !are_you_opped(channel->channel) || !nick || !ThisNick || wild_match(nick, get_server_nickname(from_server)))
		return;
	if (!(flag = get_cset_int_var(channel->csets, HACKING_CSET)))
		return;
	if ((ThisNick->sent_deop) && (now - ThisNick->sent_deop_time < 10))
		return;

#ifdef WANT_USERLIST
	userptr = ThisNick->userlist;
	shitptr = ThisNick->shitlist;
#endif
	check_op = check_whosplitin_buffer(nick, ThisNick->host, channel->channel, 0);
	if (check_op && check_op->has_ops)
		return;
		
#ifdef WANT_USERLIST
	if ( !userptr || (userptr && !check_channel_match(userptr->channels, channel->channel)) ||
		(shitptr && check_channel_match(shitptr->channels, channel->channel)))
#endif
	{
		switch(flag)
		{
			case 1:
			case 3:
				if (is_chanop(channel->channel, nick))
					add_mode(channel, "o", 0, nick, NULL, get_int_var(NUM_OPMODES_VAR));
				ThisNick->sent_deop++;
				ThisNick->sent_deop_time = now;
			case 2:
				if (flag != 1)
					bitchsay("NetHack detected on %s by %s!%s", channel->channel, nick, ThisNick->host); 
			case 0:
			default:
				break;
		}
	}
}

BUILT_IN_COMMAND(reload_save)
{
char *p = NULL;
char *buffer = NULL;

	
	if (get_string_var(CTOOLZ_DIR_VAR))
	{
		malloc_sprintf(&buffer, "%s", get_string_var(CTOOLZ_DIR_VAR));
		p = expand_twiddle(buffer);
		if (p && access(p, F_OK) != 0)
		{
			bitchsay("Created directory %s", p);
			mkdir(p, S_IWUSR|S_IRUSR|S_IXUSR);
		}
		new_free(&p);
		malloc_sprintf(&buffer, "%s/%s.sav", get_string_var(CTOOLZ_DIR_VAR), version);
	}
	else
		malloc_sprintf(&buffer, "~/%s.sav", version);
#ifdef WANT_USERLIST
	remove_all(-1);
#endif
	p = expand_twiddle(buffer);
	load(empty_string, p, empty_string, NULL); /*CDE XXX p */
	new_free(&buffer);
	new_free(&p);
}


BUILT_IN_COMMAND(savelists)
{
#ifdef PUBLIC_ACCESS
	bitchsay("This command is disabled on a public access system");
	return;
#else
	char thefile[BIG_BUFFER_SIZE+1];
	char *p = NULL;
	FILE *outfile = NULL;
	extern int defban;
	ShitList *slist = shitlist_list;
	UserList *ulist = NULL;
	void *location = NULL;
	LameList *lame_nick = lame_list;
	AJoinList *ajoin = ajoin_list;
	int count = 0;
	int size = -1;
	
	
	sprintf(thefile, "%s/%s.sav", get_string_var(CTOOLZ_DIR_VAR), version);
	if ((p = expand_twiddle(thefile)))
		outfile = fopen(p, "w");

	if (!outfile)
	{
		bitchsay("Cannot open file %s for saving!", thefile);
		new_free(&p);
		return;
	}
	if (do_hook(SAVEFILEPRE_LIST, "%s %s", thefile, p)) 
		bitchsay("Saving All Your Settings to %s", thefile);

#ifdef WANT_USERLIST
	if (ulist)
		fprintf(outfile, "# %s UserList\n", version);


	count = 0;
	for (ulist = next_userlist(NULL, &size, &location); ulist; ulist = next_userlist(ulist, &size, &location))
	{
		fprintf(outfile, "ADDUSER %s!%s %s %s%s%s\n", ulist->nick, ulist->host, ulist->channels, convert_flags_to_str(ulist->flags), ulist->password?space:empty_string, ulist->password? ulist->password : empty_string);
		count ++;
	}
	if (count && do_hook(SAVEFILE_LIST, "UserList %d", count))
		bitchsay("Saved %d UserList entries", count);

	count = 0;
	if (slist)
		fprintf(outfile, "# %s ShitList\n", version);
	while(slist)
	{
		fprintf(outfile, "ADDSHIT %s %s %d %s\n", slist->filter, slist->channels, slist->level, slist->reason? slist->reason : empty_string);
		slist = slist->next;
		count ++;
	}
	if (count && do_hook(SAVEFILE_LIST, "ShitList %d", count))
		bitchsay("Saved %d ShitList entries", count);
#endif

	count = 0;
	if (lame_list)
		fprintf(outfile, "# %s LameNick List\n", version);
	while(lame_nick)
	{
		fprintf(outfile, "ADDLAMENICK %s\n", lame_nick->nick);
		lame_nick = lame_nick->next;
		count ++;
	}
	if (count && do_hook(SAVEFILE_LIST, "LameNickList %d", count))
		bitchsay("Saved %d LameNick entries", count);

	if (ajoin)
		fprintf(outfile, "# %s Auto-Join List\n", version);
	count = 0;
	while (ajoin)
	{
		if (ajoin->ajoin_list == 1) /* actual auto-join entry */
		{
			fprintf(outfile, "AJOIN %s%s%s%s%s\n", ajoin->name, ajoin->group?" -g ":empty_string,
					ajoin->group?ajoin->group:empty_string, ajoin->key?space:empty_string, ajoin->key?ajoin->key:empty_string);
			count ++;
		}
		ajoin = ajoin->next;
	}
	if (count && do_hook(SAVEFILE_LIST, "AutoJoin %d", count))
		bitchsay("Saved %d AutoJoin entries", count);

#ifdef WANT_USERLIST
	ulist = user_list;
	count = 0;
	while(ulist)
	{
		if (ulist->comment)
		{
			if (count == 0)
				fprintf(outfile, "# %s UserInfo\n", version);
			fprintf(outfile, "USERINFO %s %s\n", ulist->nick, ulist->comment);
			count++;
		}
		ulist = ulist->next;
	}
	if (count && do_hook(SAVEFILE_LIST, "UserInfo %d", count))
		bitchsay("Saved %d UserInfo entries", count);
#endif

	save_notify(outfile);
	save_watch(outfile);
	save_idle(outfile);
	save_banwords(outfile);
	dcc_exempt_save(outfile);
	savebitchx_variables(outfile);

	fprintf(outfile, "BANTYPE %c\n", defban == 1? 'N':defban == 2? 'B':defban==3?'H':defban==4?'D':defban==5?'U':defban==6?'S':'I');
	if (do_hook(SAVEFILEPOST_LIST, "%s %s", thefile, p))
		bitchsay("Done Saving to file %s", thefile);

	fclose(outfile);
	new_free(&p);
	/* this looks like a bug but it isn't. formats are saved in a
	 * differant file altogether.
	 */
	save_formats(outfile);
#endif
}

int change_pass(char *from, char *msg)
{
UserList *tmp;
char *old_pass, *new_pass, *args;
	args = LOCAL_COPY(msg);
	next_arg(args, &args);
	old_pass = next_arg(args, &args);
	new_pass = next_arg(args, &args);
	if ((tmp = (UserList *) lookup_userlevelc(from, FromUserHost, "*", NULL)))
	{
		if (tmp->password)
		{
			if (!old_pass || !new_pass)
				return 0;
			if (!(checkpass(old_pass, tmp->password)))
			{
				int old_display_win = window_display;
				window_display = 0;
				malloc_strcpy(&tmp->password, cryptit(new_pass));
				savelists(NULL, NULL, NULL, NULL);
				window_display = old_display_win;
				return 1;
			}
		}
		/* user doesn't have a pass, so set it up */
		if (old_pass)
		{
			int old_display_win = window_display;
			window_display = 0;
			tmp->password = m_strdup(cryptit(old_pass));
			savelists(NULL, NULL, NULL, NULL);
			window_display = old_display_win;
			return 1;
		}
	}
	return 0;
}
