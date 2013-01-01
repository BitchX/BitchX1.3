/*
 * Copyright Colten Edwards. Oct 96
 */
 
#include "irc.h"
static char cvsrevision[] = "$Id: bot_link.c 62 2009-09-02 14:14:21Z keaston $";
CVS_REVISION(botlink_c)
#include <stdarg.h>

#include "ircaux.h"
#include "struct.h"
#include "commands.h"
#include "dcc.h"
#include "server.h"
#include "output.h"
#include "list.h"
#include "who.h"
#include "ctcp.h"
#include "tcl_bx.h"
#include "status.h"
#include "vars.h"
#include "misc.h"
#include "userlist.h"
#include "hash2.h"
#define MAIN_SOURCE
#include "modval.h"

#ifndef BITCHX_LITE
static int clink_active = 0;
static	int xlink_commands = 0;

#ifndef TCL_OK
#define TCL_OK 0
#endif

#ifndef TCL_ERROR
#define TCL_ERROR 1
#endif

extern  int in_add_to_tcl;
extern	char *FromUserHost;

int BX_get_max_fd();

extern cmd_t C_dcc[];

cmd_t C_tandbot[] =
{
	{ "zapf",	tand_zapf,	 0,	NULL },
	{ "zapf-broad", tand_zapfbroad,  0,	NULL },
	{ "chan",	tand_chan,	 0,	NULL },
	{ "part",	tand_part,	 0,	NULL },
	{ "join",	tand_join,	 0,	NULL },
	{ "who",	tand_who,	 0,	NULL },
	{ "whom",	tand_whom,	 0,	NULL },
	{ "priv",	tand_priv,	 0,	NULL },
	{ "boot",	tand_boot,	 0,	NULL },
	{ "privmsg",	tand_privmsg,	 0,	NULL },
	{ "clink_com",	tand_clink,	 0,	NULL },
	{ "command",	tand_command,	 0,	NULL },
	{ NULL,		NULL,		-1,	NULL }
};
#endif

int BX_dcc_printf(int idx, char *format, ...)
{
va_list args;
char putbuf[BIG_BUFFER_SIZE+1];
int len = 0;
	va_start(args, format);
	vsnprintf(putbuf, BIG_BUFFER_SIZE, format, args);
	va_end(args);
	if (strlen(putbuf) > 510)
		putbuf[510] = '\n';
	putbuf[511] = '\0';
	if (idx == -1)
		put_it("%s", chop(putbuf, 1));
	else
	{
		len = strlen(putbuf);
		send(idx, putbuf, len, 0);
	}
	return len;
}

#ifndef BITCHX_LITE
void tandout_but(int idx, char *format, ...)
{
va_list args;
char putbuf[BIG_BUFFER_SIZE+1];
SocketList *s;
int i;
	va_start(args, format);
	vsnprintf(putbuf, BIG_BUFFER_SIZE, format, args);
	va_end(args);

	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i) || (idx == i)) 
			continue;
		s = get_socket(i);
		if (((s->flags & DCC_TYPES) == DCC_BOTMODE) && s->flags & DCC_ACTIVE)
			send(i, putbuf, strlen(putbuf), 0);
	}
}

void chanout_but(int idx, char *format, ...)
{
va_list args;
char putbuf[BIG_BUFFER_SIZE+1];
SocketList *s;
int i;
	va_start(args, format);
	vsnprintf(putbuf, BIG_BUFFER_SIZE, format, args);
	va_end(args);

	for (i = 0; i < get_max_fd()+1; i++)
	{
		DCC_int *n;
		if (!check_dcc_socket(i) || (idx == i)) continue;
		s = get_socket(i);
		if (!(s->flags & DCC_ACTIVE)) continue;
		n = get_socketinfo(i);
		
		if (idx != i && (s->flags & DCC_BOTCHAT))
			send(i, putbuf, strlen(putbuf), 0);
		else if (idx == i && (s->flags & DCC_ECHO))
			send(i, putbuf, strlen(putbuf), 0);
	}
}

BUILT_IN_COMMAND(csay)
{
int found = 0;
SocketList *s;
char *t = NULL;
int i;
	if (!args || !*args)
		return;
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i)) continue;
		s = get_socket(i);
		if (s->flags & DCC_ACTIVE)
		{
			DCC_int *n;
			n = get_socketinfo(i);
			if ((s->flags & DCC_TYPES) == DCC_BOTMODE)
			{
				n->bytes_sent += dcc_printf(i, "chan %s %d %s@%s %s\n", get_server_nickname(from_server), 0, get_server_nickname(from_server), get_server_nickname(from_server), args);
				found++;
			}
			else if (((s->flags & DCC_TYPES) == DCC_CHAT) && (s->flags & DCC_BOTCHAT))
			{
				n->bytes_sent += dcc_printf(i, "[%s@%s] %s\n", get_server_nickname(from_server), get_server_nickname(from_server), args);
				found++;
			}
		}
	}
	new_free(&t);

	if (!found)
		bitchsay("No Active bot link");
	else
		put_it("%s", convert_output_format("%K(%R$1%K(%rxlink%K))%n $2-", "%s %s %s", update_clock(GET_TIME), get_server_nickname(from_server), args));
}

BUILT_IN_COMMAND(cwho)
{
int whom = 0;
SocketList *s;
int i;
	whom = !my_stricmp(command, "cwhom");
	whom ? tell_whom(-1, NULL) : tell_who(-1, NULL);
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i)) continue;
		s = get_socket(i);
		if (s->flags & DCC_ACTIVE)
		{
			if ((s->flags & DCC_TYPES) == DCC_BOTMODE)
			{
				if (whom)
					dcc_printf(i, "whom %d:%s@%s %s %d\n",
						-1, get_server_nickname(from_server), get_server_nickname(from_server), args?args:empty_string, 0);
				else			
					send_who(-1, empty_string);
			}
		}
	}
}

BUILT_IN_COMMAND(cboot)
{
char *nick, *reason;
int i;
SocketList *s;
	nick = next_arg(args, &args);
	reason = args ? args: "We don't like you";
	if (!nick)
		return;
	for (i = 0; i < get_max_fd(); i++)
	{
		if (!check_dcc_socket(i)) continue;
		s = get_socket(i);
		if (s->flags & DCC_ACTIVE)
		{
			if ((s->flags & DCC_TYPES) == DCC_CHAT && (s->flags & DCC_BOTCHAT))
				dcc_printf(i, ".boot %s %s %s\n", get_server_nickname(from_server), nick, reason);
		}
	}
}

BUILT_IN_COMMAND(toggle_xlink)
{
	xlink_commands ^= 1;
	put_it("X-link commands toggled %s", on_off(xlink_commands));
}

BUILT_IN_COMMAND(cmsg)
{
char *nick, *reason;
SocketList *s;
int i;
	nick = next_arg(args, &args);
	reason = args;
	if (!nick)
		return;
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i)) continue;
		s = get_socket(i);
		if ((s->flags & DCC_ACTIVE) && ((s->flags & DCC_TYPES) == DCC_CHAT) && (s->flags & DCC_BOTCHAT))
			dcc_printf(i, ".cmsg %s %s\n", nick, reason);
	}
}

void userhost_clink(UserhostItem *stuff, char *nick, char *args)
{
	if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick))
	{
		bitchsay("No information for %s found", nick);
		return;
	}
	bitchsay("Attempting Clink to %s!%s@%s", nick, stuff->user, stuff->host);
	send_ctcp(CTCP_PRIVMSG, nick, CTCP_BOTLINK, "%s", args);
	clink_active++;
}

BUILT_IN_COMMAND(clink)
{
char *nick;
NickList *Nick = NULL;
ChannelList *chan;
int i = 0;
	nick = next_arg(args, &args);
	if (nick)
	{
		for (i = 0; i < server_list_size(); i++)
			for (chan = get_server_channels(i); chan; chan = chan->next)
				if ((Nick = find_nicklist_in_channellist(nick, chan, 0)))
					break;
		if (Nick)
		{
			if (Nick->userlist && (Nick->userlist->flags & ADD_BOT))
			bitchsay("Attempting clink with %s!%s", Nick->nick, Nick->host);
			send_ctcp(CTCP_PRIVMSG, nick, CTCP_BOTLINK, "%s", args);
			clink_active++;
		}
		else	
			userhostbase(nick, userhost_clink, 1, "%s", args);
	}
}

int handle_tcl_chan(int idx, char *user, char *host, char *text)
{
	tandout_but(idx, "chan %s %d %s@%s %s\n", get_server_nickname(from_server), 0, user, get_server_nickname(from_server), text);
	chanout_but(idx, "[%s@%s] %s\n", user, get_server_nickname(from_server), text);
	put_it("%s", convert_output_format("%K(%R$1%K(%rxlink%K))%n $2-", "%s %s %s", update_clock(GET_TIME), user, text));
	return 0;
}

int tand_chan(int idx, char *par)
{
char *bot;
char *chan;
char *who;
char *uhost;
/*
	 tand_chan (idx=7, par=0x8140009 "panasync 0 sabina@botnick testing\r")
*/
	bot = next_arg(par, &par);
	chan = next_arg(par, &par);
	who = next_arg(par, &par);
	if (!who || !chan || !bot)
		return 0;
	tandout_but(idx, "chan %s %d %s %s\n", get_server_nickname(from_server), 0, who, par);
	chanout_but(idx, "%s %s\n", who, par);
	if ((uhost = strchr(who, '@')))
		*uhost++ = '\0';
	put_it("%s", convert_output_format("%K[%RxLink%Y$1%K(%r$2%K)] %n$3-", "%s %s %s %s", update_clock(GET_TIME), who, uhost?uhost:"u@h", par));
	return 0;
}

int tand_zapf(int idx, char *par)
{
char *from, *to; 
SocketList *s = NULL, *s1 = NULL;
#ifdef WANT_TCL
char *opcode;
#endif
	from = next_arg(par, &par);
	to = next_arg(par, &par);
	if (!(s = find_dcc(from, "chat", NULL, DCC_BOTMODE, 0, 1, -1)) || s->is_read != idx)
		return 0;
#ifdef WANT_TCL
	if (!my_stricmp(to, get_server_nickname(from_server)))
	{
		opcode = next_arg(par, &par);
		check_tcl_tand(from, opcode, par);
		return 0;
	}
#endif
	s1 = find_dcc(to, "chat", NULL, DCC_BOTMODE, 0, 1, -1);
	if (!s1)
		tandout_but(idx, "zapf %s %s %s\n", from, to, par);
	else 
		dcc_printf(s1->is_read, "zapf %s %s %s\n", from, to, par);
	return 0;
}

/* used to send a global msg from Tcl on one bot to every other bot */
/* zapf-broad <frombot> <code [param]> */
int tand_zapfbroad(int idx, char *par)
{
	char *from, *opcode;

	
	from = next_arg(par, &par);
	opcode = next_arg(par, &par);

#ifdef WANT_TCL
	check_tcl_tand(from, opcode, par);
#endif
	tandout_but(idx, "zapf-broad %s %s %s\n", from, opcode, par);
	return 0;
}


int handle_dcc_bot(int idx, char *param)
{
char *code;
int i = 0;
int found = 0;
	code = next_arg(param, &param);
	if (!code)
		return 0;
	while (C_tandbot[i].access != -1)
	{
		if (!my_stricmp(code, C_tandbot[i].name))
		{
			(C_tandbot[i].func)(idx, param);
			found = 1;
			break;
		}
		i++;
	}
	return found;
}


int tand_join (int idx, char *args)
{
char *bot, *nick;
	bot = next_arg(args, &args);
	nick = next_arg(args, &args);
	tandout_but(idx, "join %s %s %d\n", bot, nick, 0);
	chanout_but(idx, "[%s@%s] has entered the TwilightZone\n", nick, bot);
	put_it("%s", convert_output_format("%K[%RxLink%Y$1%K(%r$2%K)] %n$3-", "%s %s %s %s", update_clock(GET_TIME), nick, bot, "entered the TwilightZone"));
	return 0;
}

int tand_part (int idx, char *args)
{
char *bot, *nick;
	bot = next_arg(args, &args);
	nick = next_arg(args, &args);
	tandout_but(idx, "part %s %s\n", bot, nick);
	chanout_but(idx, "[%s@%s] has left the TwilightZone\n", nick, bot);
	put_it("%s", convert_output_format("%K[%RxLink%Y$1%K(%r$2%K)] %n$3-", "%s %s %s %s", update_clock(GET_TIME), nick, bot, "left the TwilightZone"));
	return 0;
}

int send_who_to(int idx, char *from, int arg)
{
/*	dcc_printf(idx, "priv %s %s testing\n", get_server_nickname(from_server), from);*/
SocketList *s;
int found = 0;
int i;
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i)) continue;
		s = get_socket(i);
		if (((s->flags & DCC_TYPES) == DCC_CHAT) && (s->flags & DCC_BOTCHAT))
		{
			DCC_int *n = (DCC_int *)s->info;
			if (!found++)
				dcc_printf(idx, "priv %s %s %-10s  %-10s  %-25s\n", get_server_nickname(from_server), from, "Who", "Bot", "Host");
			dcc_printf(idx, "priv %s %s %-10s  %-10s  %-25s\n", get_server_nickname(from_server), from, s->server, get_server_nickname(from_server), n->userhost);
		}
	} 
	return 0;
}

int tand_who (int idx, char *args)
{
char *from, *to, *p;
char buffer[IRCD_BUFFER_SIZE+1];
	from = LOCAL_COPY(next_arg(args, &args));
	if (!(p = strchr(from, '@')))
	{
		strmopencat(buffer, IRCD_BUFFER_SIZE, from, "@", get_server_nickname(from_server), NULL);
		from = buffer;
	}
	if ((to = next_arg(args, &args)))
	{
		if (!my_stricmp(to, get_server_nickname(from_server)))
			send_who_to(idx, from, atoi(args));
		else
			tandout_but(idx, "who %s %s %s\n", from, to, args);
	}
	return 0;   
}

void do_command(int idx, char *from, char *par)
{
	logmsg(LOG_TCL, from, 0, "[command] %s", par);
	parse_line("BOT", par, NULL, 0, 0, 1);
}

int tand_command (int idx, char *args)
{
char *from, *to, *p;
char buffer[IRCD_BUFFER_SIZE+1];
	from = LOCAL_COPY(next_arg(args, &args));
	if (!(p = strchr(from, '@')))
	{
		strmopencat(buffer, IRCD_BUFFER_SIZE, from, "@", get_server_nickname(from_server), NULL);
		from = buffer;
	}
	if ((to = next_arg(args, &args)))
	{
		if (!args) return 0;
		if (!my_stricmp(to, get_server_nickname(from_server)))
			do_command(idx, from, args);
		else
			tandout_but(idx, "command %s %s %s\n", from, to, args);
	}
	return 0;   
}

int tand_whom (int idx, char *args)
{
char *bot, *nick, *from, *p;
char buffer[IRCD_BUFFER_SIZE+1];
	from = LOCAL_COPY(next_arg(args, &args));
	if (!(p = strchr(from, '@')))
	{
		strmopencat(buffer, IRCD_BUFFER_SIZE, from, "@", get_server_nickname(from_server), NULL);
		from = buffer;
	}
	nick = next_arg(args, &args);
	bot = next_arg(args, &args);
	send_who_to(idx, from, 0);
	tandout_but(idx, "whom %s %s %s %s\n", from, nick, bot, args);
	return 0;
}

int tell_who(int idx, char *arg)
{
SocketList *s;
DCC_int *n;
int i;
int found = 0;
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i)) continue;
		s = get_socket(i);
		n = get_socketinfo(i);
		if ((s->flags & DCC_TYPES) == DCC_CHAT && (s->flags & DCC_BOTCHAT))
		{
			if (!found++)
				dcc_printf(idx, "%-10s  %-10s\n", "Who", "Bot");
			dcc_printf(idx, "%-10s  %-10s\n", s->server, get_server_nickname(from_server));
		}
	} 
	if (!found)
		dcc_printf(idx, "No Clients connected\n");
	found = 0;
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i)) continue;
		s = get_socket(i);
		n = get_socketinfo(i);
		if ((s->flags & DCC_TYPES) == DCC_BOTMODE)
		{
			if (!found++)
				dcc_printf(idx, "%-10s  %-15s\n", "Bots", "Connected");
			dcc_printf(idx, "%-10s  %10s\n", s->server, get_server_nickname(from_server));
		}
	} 
	if (!found)
		dcc_printf(idx, "No Bots connected\n");
	return 0;
}

int send_who(int idx, char *arg)
{
int found = 0;
	if (arg && *arg)
	{
		if (!my_stricmp(arg, get_server_nickname(from_server)))
			tell_who(idx, NULL);
		else
		{
			int i;
			SocketList *s, *C = NULL;
			C = get_socket(idx);
			for (i = 0; i < get_max_fd()+1; i++)
			{
				if (!check_dcc_socket(i)) continue;
				s = get_socket(i);
				if (!(s->flags & DCC_ACTIVE) || !((s->flags & DCC_TYPES) == DCC_BOTMODE))
					continue;
				if (C->info && !my_stricmp(s->server, arg))
				{
					dcc_printf(i, "who %d:%s@%s %s %d\n",
						idx, C->server, get_server_nickname(from_server), arg, 0);
					found = 1;
				}
				
			}
			if (!found)
				dcc_printf(idx, "Not found %s\n", arg);
		}
	} else
		tell_who(idx, NULL);
	return 0;
}

int tell_whom(int idx, char *arg)
{
DCC_int *n;
int found = 0;
int i;
SocketList *s;
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i)) continue;
		s = get_socket(i);
		n = get_socketinfo(i);
		if ((s->flags & DCC_TYPES) == DCC_CHAT && (s->flags & DCC_BOTCHAT))
		{
			if (!found++)
				dcc_printf(idx, "%-10s  %-10s  %-25s\n", "Who", "Bot", "Host");
			dcc_printf(idx, "%-10s  %-10s  %-25s\n", s->server, get_server_nickname(from_server), n->userhost?n->userhost:"Unknown");
		}
	} 
	return 0;
}

int send_whom(int idx, char *arg)
{
int found = 0;
int i;
int j;
SocketList *s, *s1 = NULL;
	tell_whom(idx, NULL);
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i)) continue;
		if (idx == i) { s1 = get_socketinfo(i); break; }
	}
	if (!s1) return 0;
	for (j = 0; j < get_max_fd()+1; j++)
	{
		if (!check_dcc_socket(j)) continue;
		s = get_socketinfo(j);
		if ((s->flags & DCC_TYPES) == DCC_BOTMODE)
		{
			dcc_printf(s->is_read, "whom %d:%s@%s %s %d\n",
				idx, s1->server, get_server_nickname(from_server), arg, 0);
			found = 1;
		}
	}
	return 0;
}

int tand_priv (int idx, char *args)
{
char *to, *from, *p, *i_dx;
	from = next_arg(args, &args);
	to = next_arg(args, &args);
	p = strchr(to, '@');
	if (p && !my_stricmp(p+1, get_server_nickname(from_server)))
	{
		char *t = strchr(to, ':');
		i_dx = to;
		if (t)
			*t = 0;
		/* this ones for me */
		dcc_printf(atoi(i_dx), "%s\n", args);		
	} 
	else
	{
		tandout_but(idx, "%s\n", args);
	}
	return 0;
}

int tand_boot (int idx, char *args)
{
char *nick;
char *from;
char *reason;
SocketList *s;
	from = next_arg(args, &args);
	nick = next_arg(args, &args);
	reason = args;
	if ((s = find_dcc(nick, "chat", NULL, DCC_CHAT, 0, 1, -1)))
	{
		chanout_but(s->is_read, "%s has booted %s out of the TwilightZone for (%s)\n", from, s->server, reason);
		dcc_printf(s->is_read, "You have been booted by %s for %s\n", from, reason);
		erase_dcc_info(s->is_read, 0, NULL);
	}
	else
		tandout_but(idx, "boot %s %s %s\n", from, nick, args);	
	return 0;
}

int tand_privmsg(int idx, char *par)
{
char *to, *from;
SocketList *s;
	to = next_arg(par, &par);
	from = next_arg(par, &par);
	s = find_dcc(to, "chat", NULL, DCC_CHAT, 0, 1, -1);
	if (s && s->flags & DCC_BOTCHAT)
		dcc_printf(s->is_read, "[%s] %s\n", from, par?par:empty_string);
	else
		tandout_but(idx, "privmsg %s %s %s\n", to, from, par);
	return 0;
}

int cmd_cmsg(int idx, char *par)
{
	char *nick, *user;
	SocketList *s, *s1 = NULL;

	nick = next_arg(par, &par);

	if (nick)
	{
		int i;

		if ((user = strchr(nick, '@')))
			*user++ = '\0';

		for (i = 0; i < get_max_fd()+1; i++)
		{
			if (!check_dcc_socket(i)) continue;
			if (idx == i) { s1 = get_socket(i); break; }
		}
		if (!s1) return TCL_OK;
		s = find_dcc(nick, "chat", NULL, DCC_CHAT, 0, 1, -1);
		if (s) 
			dcc_printf(s->is_read, "[%s@%s] %s\n", s1->server, user?user:get_server_nickname(from_server), par);
		else
			tandout_but(idx, "privmsg %s %s@%s %s\n", nick, s1->server, user?user:get_server_nickname(from_server), par);
	}
	return TCL_OK;
}

int cmd_boot(int idx, char *par)
{
	char *nick = next_arg(par, &par);
	char *reason;
	SocketList *s, *s1 = NULL;

	s = find_dcc(nick, "chat", NULL, DCC_CHAT, 0, 1, -1);
	if ((check_dcc_socket(idx)))
		s1 = get_socketinfo(idx);

	if (my_stricmp(nick, get_server_nickname(from_server)))
	{
		if (!par || !*par)
			reason = "We don't like you";
		else
			reason = par;
		if (!s)
			tandout_but(idx, "boot %s %s %s\n", get_server_nickname(from_server), nick, reason);
		else
		{
			dcc_printf(s->is_read, "You have been booted by %s for (%s)\n", s1? s1->server:get_server_nickname(from_server), reason);
			chanout_but(s->is_read, "%s booting %s out of the TwilightZone (%s)\n", s1? s1->server:get_server_nickname(from_server), s->server, reason);
			erase_dcc_info(s->is_read, 0, NULL);
		}
	} else
		dcc_printf(idx, "Can't boot a bot\n");
	return TCL_OK;
}

int cmd_act(int idx, char *par)
{
char *nick;
SocketList *s;
	if (!check_dcc_socket(idx))
		return TCL_OK;
	s = get_socket(idx);	
	if (idx > -1)
		nick = s->server;
	else
		nick = empty_string;
	tandout_but(idx, "  * %s %s\n", nick, par);
	chanout_but(idx, "  * %s %s\n", nick, par);	
	return TCL_OK;
}

int cmd_msg(int idx, char *par)
{
	char *nick = next_arg(par, &par);
	send_to_server("PRIVMSG %s :%s\n",nick, par);
	return TCL_OK;
}

int cmd_say(int idx, char *par)
{
	char *nick = next_arg(par, &par);
	send_to_server("PRIVMSG %s :%s\n",nick, par);
	return TCL_OK;
}

int cmd_tcl(int idx, char *par)
{
#ifdef WANT_TCL
	if (!get_int_var(BOT_TCL_VAR))
		return TCL_ERROR;
	if ((Tcl_Eval(tcl_interp, par)) == TCL_OK)
	{
		dcc_printf(idx, "Tcl: %s\n", tcl_interp->result);
	} else 
		dcc_printf(idx, "Tcl Error: %s\n", tcl_interp->result);
#else
		dcc_printf(idx, "Not implemented in this client\n");
#endif
	return TCL_OK;
}

int cmd_ircii(int idx, char *par)
{

	parse_line("BOT", par, NULL, 0, 0, 1);
	return TCL_OK;
}

int cmd_ops(int idx, char *par)
{
DCC_int *n;
	n = get_socketinfo(idx);
	if (n && n->ul)
	{
		ChannelList *ch;
		NickList *nl;
		char *nick, *chan, *loc, *n1;
		loc = LOCAL_COPY(par);
		nick = next_arg(loc, &loc);
		chan = next_arg(loc, &loc);
		if (chan)
			ch = lookup_channel(chan, from_server, 0);
		else
			ch = lookup_channel(get_current_channel_by_refnum(0), from_server, 0);	
		if (!nick || !ch || !ch->have_op) { dcc_printf(idx, "No Nick specified, not on that channel or not channel op\n"); return TCL_ERROR; }
		while ((n1 = next_in_comma_list(nick, &nick)))
		{
			if (!n1 || !*n1) break;
			if ((nl = find_nicklist_in_channellist(n1, ch, 0)))
			{
				if (nl->userlist && (nl->userlist->flags & ADD_OPS))
					my_send_to_server(from_server, "MODE %s +o %s", ch->channel, n1);
			} else
				dcc_printf(idx, "Nick %s is not on channel %s\n", n1, ch->channel);
		}
		return TCL_OK;
	}
	return TCL_ERROR;
}

int cmd_chat(int idx, char *par)
{
SocketList *s;
char buffer[IRCD_BUFFER_SIZE+1];
char *pass = NULL;
	s = get_socket(idx);
	if (check_dcc_socket(idx) && !(s->flags & DCC_BOTCHAT))
	{
		DCC_int *n;
		n = (DCC_int *)s->info;
		pass = next_arg(par, &par);
		if (get_int_var(BOT_MODE_VAR))
		{
			char *main_pass;
			int got_it = 0;
			
			main_pass = get_string_var(BOT_PASSWD_VAR);
			if (pass && n->ul && n->ul->password && !checkpass(pass, n->ul->password))
				got_it++;
			else if (pass && main_pass && !strcmp(pass, main_pass))
				got_it++;
			if (!got_it)
			{
				dcc_printf(idx, "Wrong Password\n");
				erase_dcc_info(s->is_read, 1, NULL);
				close_socketread(s->is_read);		
				return TCL_ERROR;		
			}
		}
		s->flags |= DCC_BOTCHAT;
		snprintf(buffer, IRCD_BUFFER_SIZE, "%s %s %s", get_server_nickname(from_server), s->server, n->userhost);
		tand_join(idx, buffer);
		dcc_printf(idx, "Welcome to the TwilightZone\n");
	}
	return TCL_OK;
}

int cmd_password(int idx, char *par)
{
SocketList *s;
char buffer[IRCD_BUFFER_SIZE+1];
char *pass = NULL;
	s = get_socket(idx);
	if (check_dcc_socket(idx))
	{
		DCC_int *n;
		char *main_pass = NULL;
		char *user_pass = NULL;
		int got_it = 0;
		
		n = (DCC_int *)s->info;
		pass = next_arg(par, &par);
		if (!pass) 
			return TCL_ERROR;
		main_pass = get_string_var(BOT_PASSWD_VAR);
		if (n->ul && n->ul->password)
			user_pass = n->ul->password;
		if (pass)
		{
			if (main_pass && !strcmp(pass, main_pass))
				got_it++;
			if (user_pass && !checkpass(pass, user_pass))
				got_it++;
		}
		if (!got_it)
			return TCL_ERROR;			

		s->flags |= DCC_BOTCHAT;
		snprintf(buffer, IRCD_BUFFER_SIZE, "%s %s %s", get_server_nickname(from_server), s->server, n->userhost);
		tand_join(idx, buffer);
		dcc_printf(idx, "Welcome to the TwilightZone\n");
	}
	return TCL_OK;
}

int cmd_quit(int idx, char *par)
{
SocketList *s;
char buffer[IRCD_BUFFER_SIZE+1];
	s = get_socket(idx);
	if (check_dcc_socket(idx) && (s->flags & DCC_BOTCHAT))
	{
		DCC_int *n = (DCC_int *)s->info;
		s->flags |= DCC_BOTCHAT;
		snprintf(buffer, IRCD_BUFFER_SIZE, "%s %s %s", get_server_nickname(from_server), s->server, n->userhost);
		tand_part(idx, buffer);
		dcc_printf(idx, "\002GoodBye %s.\002\n", s->server);
		erase_dcc_info(s->is_read, 1, NULL);
		close_socketread(s->is_read);		
	}
	return TCL_OK;
}

void invite_dcc_chat(UserhostItem *stuff, char *nick, char *args)
{
char *id;
int idx = -1;
	id = next_arg(args, &args);
	idx = atol(id);
        if (!stuff || !stuff->nick || !nick || !strcmp(stuff->user, "<UNKNOWN>") || my_stricmp(stuff->nick, nick))
        {
		if (check_dcc_socket(idx))
	                dcc_printf(idx, "No such for user %s\n", nick);
                return;
        }
	dcc_chat(NULL, nick);
	send_to_server("NOTICE %s :You've been invited to a TwilightZone", nick);
	send_to_server("NOTICE %s :Please type .chat in dcc chat to start", nick);
	chanout_but(idx, "Inviting %s!%s@%s to TwilightZone\n", nick, stuff->user, stuff->host);
	return;
}

int cmd_invite(int idx, char *par)
{
int old_server = from_server;
	from_server = get_window_server(0);
	if (par && *par)
	{
		char *nick = next_arg(par, &par);
		userhostbase(nick, invite_dcc_chat, 1, "%d %s", idx, nick);
	}
	from_server = old_server;
	return TCL_OK;
}

int cmd_echo(int idx, char *par)
{
SocketList *s;
	if ((idx == -1) || !check_dcc_socket(idx))
		return TCL_ERROR;

	s = get_socket(idx);	
	if (par && *par)
	{
		if (!my_stricmp(par, "off"))
			s->flags &= ~DCC_ECHO;
		else
			s->flags &= DCC_ECHO;
	} 
	else
		s->flags &= (s->flags & DCC_ECHO) ? ~DCC_ECHO : DCC_ECHO;		
	dcc_printf(idx, " echo is now %s\n", on_off((s->flags & DCC_ECHO)));
	return TCL_OK;
}

int send_command (int idx, char *par)
{
	if (par && *par && xlink_commands)
	{
		tandout_but(idx, "clink_com %s\n", par);
		if (xlink_commands)
			parse_command(par, 0, empty_string);
		return TCL_OK;
	}
	return TCL_ERROR;
}

int tand_clink(int idx, char *par)
{
	chanout_but(idx, ".xlink %s\n", par);
	tandout_but(idx, "clink_com %s\n", par);
	if (xlink_commands)
		parse_command(par, 0, empty_string);
	return TCL_OK;
}

int cmd_adduser(int idx, char *par)
{
/*	add_user();*/
	return TCL_OK;
}

int cmd_whoami(int idx, char *par)
{
char *host;
unsigned long atr = 0;
UserList *n;
	host = FromUserHost;
#ifdef WANT_USERLIST
	if ((n = lookup_userlevelc("*", host, "*", NULL)))
		atr = n->flags;
	if  (n)
		dcc_printf(idx, "You are %s!%s with %s flags.\n", n->nick, n->host, convert_flags_to_str(atr));
	else
#endif
		dcc_printf(idx, "Nobody special\n");
	return TCL_OK;
}

int tand_ircii(int idx, char *par)
{
char *from, *to, *p = NULL;
char buffer[IRCD_BUFFER_SIZE+1];

	from = LOCAL_COPY(next_arg(par, &par));
	if (!(p = strchr(from, '@')))
	{
		strmopencat(buffer, IRCD_BUFFER_SIZE, from, "@", get_server_nickname(from_server), NULL);
		from = buffer;
	}
	to = next_arg(par, &par);
	if (!my_stricmp(to, get_server_nickname(from_server)))
		send_who_to(idx, from, atoi(par));
	else
		tandout_but(idx, "ircii %s %s %s\n", from, to, par);
	return 0;   
}

int cmd_help(int idx, char *par)
{
	char *command;
	int i, j;
	
	command = next_arg(par, &par);
	if (!command)
	{
		DCC_int *n;
		n = get_socketinfo(idx);
		dcc_printf(idx, "DCC commands :\n");
		for (i = 1, j = 1; C_dcc[i-1].name; i++)
		{	
			if (C_dcc[i-1].access != 0)
				if (!n->ul || !(n->ul->flags & C_dcc[i-1].access))
					continue;
#ifdef WANT_USERLIST
			dcc_printf(idx, "%6s %-10s", convert_flags_to_str(C_dcc[i-1].access), C_dcc[i-1].name);
#else
			dcc_printf(idx, "%-10s", C_dcc[i-1].name);
#endif
			if (!(j % 3))
				dcc_printf(idx, "\n");
			j++;
		}
		dcc_printf(idx, "\n");
	}
	else
	{
		for (i = 0; C_dcc[i].name; i++)
			if (!my_stricmp(C_dcc[i].name, command))
				break;
		if (C_dcc[i].name)
#ifdef WANT_USERLIST
			dcc_printf(idx, "%6s %-10s %s\n", convert_flags_to_str(C_dcc[i].access), C_dcc[i].name, C_dcc[i].help);
#else
			dcc_printf(idx, "%-10s %s\n", C_dcc[i].name, C_dcc[i].help);
#endif
		else
			dcc_printf(idx, "No such command [%s]\n", command);
	}
	return TCL_OK;
}


#endif

