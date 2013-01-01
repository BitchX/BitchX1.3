#define AUTO_VERSION "1.00"

/*
 *
 * Written by Colten Edwards. (C) August 97, March 98
 * Based on script by suicide for evolver script.
 */
#include "irc.h"
#include "struct.h"
#include "dcc.h"
#include "ircaux.h"
#include "ctcp.h"
#include "status.h"
#include "lastlog.h"
#include "screen.h"
#include "vars.h" 
#include "misc.h"
#include "output.h"
#include "module.h"
#include "hook.h"
#include "hash2.h"
#define INIT_MODULE
#include "modval.h"

#include <sys/time.h>

#define cparse convert_output_format
char auto_bot_version[] = "Auto-Bot 1.000";
UserList *auto_bot = NULL;
char *auto_filename = NULL;

BUILT_IN_DLL(add_abot);
BUILT_IN_DLL(remove_abot);

void write_abot (char *, int);
void read_abot (char *);
int join_proc (char *, char *, char **);

int check_userop(UserList *u, char *channel, int server)
{
NickList *n;
ChannelList *cptr;
	cptr = get_server_channels(server);
	cptr = (ChannelList *)find_in_list((List **)&cptr, channel, 0);
	if (cptr && (n = find_nicklist_in_channellist(u->nick, cptr, 0)))
	{
		if (n->host && u->host && !my_stricmp(u->host, n->host) && wild_match(u->channels, channel))
		{
			if (!nick_isop(n))
			{
				put_it("%s", cparse("$G %G$0 is not a channel op on $1", "%s %s", u->nick, channel));
				return 0;
			}		
			put_it("%s", cparse("$G %GRequesting OPS from $0 on $1", "%s %s", u->nick, channel));
			switch(get_dllint_var("autobot-type"))
			{
				case 0:
					send_to_server("PRIVMSG %s :OP %s", u->nick, u->password);
					break;
				case 1:
					send_to_server("PRIVMSG %s :OP %s %s", u->nick, channel, u->password);
					break;
				case 2:
					send_to_server("PRIVMSG %s :+OP %s", u->nick, get_server_nickname(server));
					break;
			}
			return 1;
		} 
	}
	return 0;
}

int join_proc (char *which, char *str, char **unused)
{
char channel[BIG_BUFFER_SIZE];
char *p;
	strncpy(channel, str, BIG_BUFFER_SIZE-10);
	if (!(p = strchr(channel, ' ')))
		return 1;
	*p = 0;
	if (!is_chanop(channel, get_server_nickname(from_server)))
	{
		UserList *u;
		for (u = auto_bot; u; u = u->next)
			check_userop(u, channel, from_server);
	}
	return 1;
}



BUILT_IN_DLL(remove_abot)
{
UserList *n;
char *nick;
int count = 0;
	nick = next_arg(args, &args);
	if (nick)
	{
		while ((n = (UserList *)remove_from_list((List **)&auto_bot, nick)))
		{
			put_it("%s", cparse("$G Removing Abot entry $0", "%s", n->nick));
			new_free(&n->nick);
			new_free(&n->host);
			new_free(&n->channels);
			new_free(&n->password);
			new_free((char **)&n);
			write_abot(auto_filename, 0);
			count++;
		} 
	}
	if (!count)
		put_it("%s", cparse("$G Couldn't find Abot entry $0", "%s", nick?nick:empty_string));
	return;
}

BUILT_IN_DLL(add_abot)
{
ChannelList *chan;
NickList *n = NULL;
char *nick, *password, *channels;
	nick = next_arg(args, &args);
	password = next_arg(args, &args);
	channels = next_arg(args, &args);
	if (from_server == -1)
		return;
	if (!nick || !password)
	{
		if (auto_bot)
		{
			int count = 0;
			UserList *u;
			for (u = auto_bot; u; u = u->next, count++)
				put_it("%s", cparse("$0 $1!$2 $4", "%d %s %s %s", count, u->nick, u->host, u->channels));
		}
		userage("abot", helparg);
		return;
	}
	if (!channels || !*channels)
		channels = "*";
	chan = get_server_channels(from_server);
	for ( ; chan; chan = chan->next)
	{
		for (n = next_nicklist(chan, NULL); n; n = next_nicklist(chan, n))
			if (!my_stricmp(nick, n->nick))
				goto done;
	}
done:
	if (n)
	{
		UserList *new;
		new = (UserList *) new_malloc(sizeof(UserList));
		new->nick = m_strdup(n->nick);
		new->host = m_strdup(n->host);
		new->password = m_strdup(password);
		new->channels = m_strdup(channels);
		add_to_list((List **)&auto_bot, (List *)new);
		write_abot(auto_filename, 1);
	}
}

char *auto_Version(IrcCommandDll **intp)
{
	return auto_bot_version;
}

void write_abot(char *filename, int out)
{
FILE *f;
UserList *u;
	if (filename && (f = fopen(filename, "w")))
	{
		if (out)
			put_it("%s", cparse("$G Auto-Saving $0", "%s", filename));
		for (u = auto_bot; u; u = u->next)
			fprintf(f, "%s!%s,%s,%s\n", u->nick,u->host,u->password,u->channels);
		fclose(f);
	} else if (filename)
		put_it("%s", cparse("$G Could not open $0 for write", "%s", filename));
}

void read_abot(char *filename)
{
FILE *f;
char buffer[BIG_BUFFER_SIZE];
char *u, *h, *p, *c;
UserList *new;

	if (!(f = fopen(filename, "r")))
	{
		put_it("Could not open %s for reading", filename); 
		return;
	}
	while (!feof(f))
	{
		if ((fgets(buffer, sizeof(buffer), f)))
		{
			chop(buffer, 1);
			u = buffer;
			h = strchr(u, '!');
			*h++ = 0;
			p = strchr(h, ',');
			*p++ = 0;
			if ((c = strchr(p, ',')))
				*c++ = 0;
			else
				c = "*";
			
			new = (UserList *) new_malloc(sizeof(UserList));
			new->nick = m_strdup(u);
			new->host = m_strdup(h);
			new->password = m_strdup(p);
			new->channels = m_strdup(c);
			add_to_list((List **)&auto_bot, (List *)new);
		}
	}
	fclose(f);
}
char *Autobot_Version(IrcCommandDll **intp)
{
	return AUTO_VERSION;
}


int Autobot_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
char buffer[BIG_BUFFER_SIZE+1];
	initialize_module("Autobot");

	add_module_proc(VAR_PROC, "Autobot", "autobot-type", NULL, INT_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(COMMAND_PROC, "Autobot", "abot", NULL, 0, 0, add_abot, "Add bot to msg for auto-ops");
	add_module_proc(COMMAND_PROC, "Autobot", "rbot", NULL, 0, 0, remove_abot, "Remove bot from autoop list");
	add_module_proc(HOOK_PROC, "Autobot", NULL, "*", CHANNEL_SYNCH_LIST, 1, NULL, join_proc);

	put_it("%s", convert_output_format("$G $0 v$1 by panasync. Based on suicide's Abot script.", "%s %s", auto_bot_version, AUTO_VERSION));
	sprintf(buffer, "%s/abots.sav", get_string_var(CTOOLZ_DIR_VAR));
	auto_filename  = expand_twiddle(buffer);
	/* read abots.sav */
	read_abot(auto_filename);
	return 0;
}

