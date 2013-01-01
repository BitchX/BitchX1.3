/*
 * This is panasync's tcl code for BitchX.
 *  (c) 1996 Colten Edwards
 *
 * Released under the 3-clause BSD license in 2012.  See the COPYRIGHT file.
 */

#include "irc.h"
#include "struct.h"

#include "dcc.h"
#include "parse.h"
#include "ircterm.h"
#include "server.h"
#include "chelp.h"
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
#include "if.h"
#include "help.h"
#include "stack.h"
#include "queue.h"
#include "timer.h"
#include "list.h"
#include "userlist.h"
#include "misc.h"
#include "who.h"
#include "whowas.h"
#include "hash2.h"
#define MAIN_SOURCE
#include "modval.h"


char tcl_versionstr[] = "Tcl 2.1";

#ifdef WANT_TCL
/*#define TCL_PLUS*/
#include "tcl_bx.h"

Tcl_HashTable *gethashtable (int, int *, char *);

TimerList *tcl_Pending_timers = NULL;
TimerList *tcl_Pending_utimers = NULL;

Tcl_HashTable	H_msg, H_dcc, H_fil, H_pub, H_msgm, H_pubm, H_join, H_part,
		H_sign, H_kick, H_topc, H_mode, H_ctcp, H_ctcr, H_nick, H_raw, H_bot,
		H_chon, H_chof, H_sent, H_rcvd, H_chat, H_link, H_disc, H_splt, H_rejn,
		H_filt, H_cmd, H_input, H_notice, H_alias, H_help, H_functions, H_hook;

#define CMD_MSG   0
#define CMD_DCC   1
#define CMD_FIL   2
#define CMD_PUB   3
#define CMD_MSGM  4
#define CMD_PUBM  5
#define CMD_JOIN  6
#define CMD_PART  7
#define CMD_SIGN  8
#define CMD_KICK  9
#define CMD_TOPC  10
#define CMD_MODE  11
#define CMD_CTCP  12
#define CMD_CTCR  13
#define CMD_NICK  14
#define CMD_RAW   15
#define CMD_BOT   16
#define CMD_CHON  17
#define CMD_CHOF  18
#define CMD_SENT  19
#define CMD_RCVD  20
#define CMD_CHAT  21
#define CMD_LINK  22
#define CMD_DISC  23
#define CMD_SPLT  24
#define CMD_REJN  25
#define CMD_FILT  26
#define CMD_CMD   27
#define CMD_INPUT 28
#define CMD_ALIAS 29
#define CMD_HELP 30
#define CMD_HOOK 31
#define CMD_FUNCTION 32
#define BINDS 33

/* extra commands are stored in Tcl hash tables (one hash table for each type
   of command: msg, dcc, etc) */

typedef struct tct {
	int flags_needed;
	char *func_name;
	char *(*func) (char *, char *);
	struct tct *next;
} tcl_cmd_t;


typedef struct
{
	char	name;
	char	*(*func) (void);
}	BuiltIns;
                


BuiltInFunctions 	*find_func_alias (char *);
BuiltIns 		*find_func_aliasvar(char *);
IrcVariable 		*get_var_address(char *);
IrcVariable 		*fget_var_address(char *);
void 			add_tcl_fset(Tcl_Interp *);

extern			cmd_t C_msg[], C_ctcp[], C_notice[], C_dcc[];


int in_add_to_tcl = 0;

int tcl_bots STDVAR
{
char buff[BIG_BUFFER_SIZE];
SocketList *s;
int count = 0;
int i;
	BADARGS(1, 1, "");
	
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i))
			continue;
		s = get_socketinfo(i);
		if ((s->flags & DCC_TYPES) == DCC_BOTMODE)
		{
			DCC_int *n;
			n = (DCC_int *)s->info;
			sprintf(buff, "%d %s", n->dccnum, s->server);
			Tcl_AppendElement(irp, buff);
			count++;
		}
	}
	if (count)
		return TCL_OK;
	Tcl_AppendResult(irp, "No bots linked", NULL);
	return TCL_ERROR;
}

int tcl_clients STDVAR
{
SocketList *s;
DCC_int *n;
int i;
char buff[BIG_BUFFER_SIZE];
int count = 0;

	BADARGS(1, 1, "");
	
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i))
			continue;
		s = get_socket(i);
		if ((s->flags & DCC_TYPES) == DCC_CHAT)
		{
			n = (DCC_int *) s->info;
			sprintf(buff, "%d %s", n->dccnum, s->server);
			Tcl_AppendElement(irp, buff);
			count++;
		}
	}
	if (count)
		return TCL_OK;

	Tcl_AppendResult(irp, "No dcc chats", NULL);
	return TCL_ERROR;
}

void add_to_tcl(Window *tmp, char *buf)
{
	Tcl_AppendElement(tcl_interp, stripansicodes(buf));
}

int tcl_alias STDVAR
{
BuiltInFunctions *tmp;
char *buffer = NULL;
char *ret = NULL;
int i;
char *p;
	if (argc == 1)
	{
		Tcl_AppendResult(irp, " ?args?", NULL);
		return TCL_ERROR;
	}
	for (i = 1; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);

	p = argv[0];
	p++;
	if (p && (tmp = find_func_alias(p)))
	{
		ret = (char *)(*tmp->func)(tmp->name, buffer);
		Tcl_AppendResult(irp, ret, NULL);
		new_free(&ret);
	}
	new_free(&buffer);
	return TCL_OK;
}

int tcl_aliasvar STDVAR
{
BuiltIns *tmp;
char *ret = NULL;
char *p;
	p = argv[0];
	p++;
	if (*p && (tmp = find_func_aliasvar(p)))
	{
		ret = (char *)(*tmp->func)();
		Tcl_AppendResult(irp, ret, NULL);
		new_free(&ret);
	}
	return TCL_OK;
}


int tcl_get_var STDVAR
{
char *s = NULL;
	BADARGS(2,2," ?variable?");
	if (argv[1] && (s = make_string_var(argv[1])))
	{
		Tcl_AppendResult(irp, s, NULL);
		new_free(&s);
		return TCL_OK;
	}
	Tcl_AppendResult(irp, " No such variable", NULL);
	return TCL_ERROR;
}

int tcl_set_var STDVAR
{
int ret = TCL_ERROR, i;
char *buffer = NULL;
IrcVariable *tmp = NULL;
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " ?args?", NULL);
		return TCL_ERROR;
	}
	for (i = 2; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);
	if ((tmp = get_var_address(argv[1])))
	{
		switch (tmp->type)
		{
			case STR_TYPE_VAR:
			{
				ret = TCL_OK;
				if (buffer)
					malloc_strcpy(&tmp->string, buffer);
				else
					new_free(&tmp->string);
				break;
			}
			case BOOL_TYPE_VAR:
			case INT_TYPE_VAR:
			{
				ret = TCL_OK;
				if (buffer)
				{
					int j;
					j = my_atol(buffer);
					if (j && (tmp->type == BOOL_TYPE_VAR))
						tmp->integer = 1;
					else
						tmp->integer = j;
				} else
					tmp->integer = 0;
				break;
			}
		}
	}
	new_free(&buffer);
	return ret;
}

int tcl_fget_var STDVAR
{
char *s = NULL;
	BADARGS(2,2," ?variable?");
	
	if (argv[1] && (s = make_fstring_var(argv[1])))
	{
		Tcl_AppendResult(irp, s, NULL);
		new_free(&s);
		return TCL_OK;
	}
	Tcl_AppendResult(irp, " No such variable", NULL);
	return TCL_ERROR;
}

int tcl_fset_var STDVAR
{
int ret = TCL_ERROR, i;
char *buffer = NULL;
IrcVariable *tmp = NULL;
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " ?args?", NULL);
		return TCL_ERROR;
	}
	for (i = 2; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);
	if ((tmp = fget_var_address(argv[1])))
	{
		switch (tmp->type)
		{
			case STR_TYPE_VAR:
			{
				ret = TCL_OK;
				if (buffer)
					malloc_strcpy(&tmp->string, buffer);
				else
					new_free(&tmp->string);
				break;
			}
			case BOOL_TYPE_VAR:
			case INT_TYPE_VAR:
			{
				ret = TCL_OK;
				if (buffer)
				{
					int j;
					j = my_atol(buffer);
					if (j && (tmp->type == BOOL_TYPE_VAR))
						tmp->integer = 1;
					else
						tmp->integer = j;
				} else
					tmp->integer = 0;
				break;
			}
		}
	}
	new_free(&buffer);
	return ret;
}



int tcl_cparse STDVAR
{
char *buf;
char *buffer = NULL;
int i;
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " ?args?", NULL);
		return TCL_ERROR;
	}
	for (i = 1; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);
	buf = convert_output_format(buffer, NULL, NULL);
	Tcl_AppendResult(irp, buf, NULL);
	new_free(&buffer);
	return TCL_OK;
}

extern void got_tcluserhost (UserhostItem *stuff, char *nick, char *userhost)
{
char *buffer = NULL;
	if (stuff && strcmp(stuff->host, "<UNKNOWN>") && strcmp(stuff->user, "<UNKNOWN>"))
	{
		malloc_sprintf(&buffer, "%s %s %s", nick, stuff->user, stuff->host);
		Tcl_SetVar(tcl_interp, "userhost", buffer, TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
		new_free(&buffer);
		return;
	}	
	Tcl_SetVar(tcl_interp, "un-userhost", nick, TCL_APPEND_VALUE|TCL_LIST_ELEMENT);
	return;
}

int tcl_userhost STDVAR
{
char *buffer = NULL;
int i;
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " ?args?", NULL);
		return TCL_ERROR;
	}
	Tcl_UnsetVar(irp, "userhost", TCL_GLOBAL_ONLY|TCL_LIST_ELEMENT);
	Tcl_UnsetVar(irp, "un-userhost", TCL_GLOBAL_ONLY|TCL_LIST_ELEMENT);
	for (i = 1; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);
	userhostbase(buffer, got_tcluserhost, 1, "%s", buffer);
	new_free(&buffer);
	return TCL_OK;
}

int tcl_msg STDVAR
{
char *buffer = NULL;
int i;
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " channel|nick ?args?", NULL);
		return TCL_ERROR;
	}
	for (i = 2; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);
	send_text(argv[1], buffer, !strcmp(argv[0], "notice")?"NOTICE":"PRIVMSG", 1, 1);
	new_free(&buffer);
	return TCL_OK;
}

int tcl_say STDVAR
{
char *buffer = NULL;
int i;
	BADARGS(1,999," ?args?");
	if (argc == 1)
	{
		Tcl_AppendResult(irp, " ?args?", NULL);
		return TCL_ERROR;
	}
	for (i = 1; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);
	send_text(get_current_channel_by_refnum(0), buffer, "PRIVMSG", 1, 1);
	new_free(&buffer);
	return TCL_OK;
}


int tcl_desc STDVAR
{
char *buffer = NULL;
int i;
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " target ?args?", NULL);
		return TCL_ERROR;
	}
	for (i = 2; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);
	send_ctcp(CTCP_PRIVMSG, argv[1], CTCP_ACTION, "%s", buffer);
	new_free(&buffer);
	return TCL_OK;
}


int tcl_getchanmode STDVAR
{
ChannelList *chan;
	BADARGS(2,2, " channel");
	chan = lookup_channel(argv[1], from_server, 0);
	if (chan)
	{
		Tcl_AppendResult(irp, recreate_mode(chan), NULL);
		return TCL_OK;
	}
	Tcl_AppendResult(irp, " not on that channel", NULL);
	return TCL_ERROR;
}

int tcl_setcomment STDVAR
{
UserList *tmp;
void *tmp2 = NULL;
int i = 0;
int size = -1;

	BADARGS(2,999, " nick");
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " nick args", NULL);
		return TCL_ERROR;
	}
	for (tmp = next_userlist(NULL, &size, &tmp2); tmp; tmp = next_userlist(tmp, &size, &tmp2))
	{
		if (!my_stricmp(tmp->nick, argv[1]))
		{
			for (i = 2; i < argc; i++)
				m_s3cat(&tmp->comment, space, argv[i]);
		}
	}
	return TCL_OK;
}

int tcl_getcomment STDVAR
{
UserList *tmp;
void *tmp2 = NULL;
int size = -1;
	BADARGS(2,2, " nick");
	Tcl_AppendResult(irp, "", NULL);
	for (tmp = next_userlist(NULL, &size, &tmp2); tmp; tmp = next_userlist(tmp, &size, &tmp2))
	{
		if (tmp->comment)
		{
			Tcl_AppendResult(irp, tmp->comment, NULL);
			break;
		}
	}
	return TCL_OK;
}

int tcl_strftime STDVAR
{
	char buf[BIG_BUFFER_SIZE];
	struct tm *tm1;
	time_t t;	
	BADARGS(3,3, " format time");
	t = atol(argv[2]);
	tm1 = localtime(&t);
	if (strftime(buf, sizeof(buf)-1, argv[1], tm1))
	{
		Tcl_AppendResult(irp, buf, NULL);
		return TCL_OK;
	}
	Tcl_AppendResult(irp, " error with strftime" , NULL);
	return TCL_ERROR;
}


int tcl_ircii STDVAR
{
char *buffer = NULL;
int i;
	
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " command args", NULL);
		return TCL_ERROR;
	}
	for (i = 1; i < argc; i++)
		m_s3cat(&buffer, space, argv[i]);

	in_add_to_tcl++;
	parse_line(NULL, buffer, NULL, 0, 0, 1);
	in_add_to_tcl--;
	new_free(&buffer);
	if (get_string_var(BOT_RETURN_VAR))
	{
		char *arg = NULL;
		malloc_sprintf(&arg, "set ircii %s", get_string_var(BOT_RETURN_VAR));
		Tcl_Eval(tcl_interp, arg);
		new_free(&arg);
	}
	return TCL_OK;
}

int tcl_pushmode STDVAR
{
ChannelList *chan = NULL;
char *mode;
char plus;
int num_modes = 3;
	
	BADARGS(3,4," channel mode ?arg?");
	if (!(chan = lookup_channel(argv[1], from_server, CHAN_NOUNLINK)))
	{
		Tcl_AppendResult(irp,"invalid channel: ",argv[1],NULL);
		return TCL_ERROR;
	}
	plus=argv[2][0]; 
	mode= (char *)&argv[2][1];
	
	if ((plus!='+') && (plus!='-')) 
	{ 
		mode=argv[2]; 
		plus='+'; 
	}
	if ((*mode < 'a') || (*mode > 'z')) 
	{
		Tcl_AppendResult(irp,"invalid mode: ",argv[2],NULL);
		return TCL_ERROR;
	}
	if ((argc<4) && (!strchr(mode, 'o') || !strchr(mode, 'v') || !strchr(mode,'b')))
	{
		Tcl_AppendResult(irp,"modes b/v/o require an argument",NULL);
		return TCL_ERROR;
	}
	num_modes = (*mode == 'b') ? get_int_var(NUM_BANMODES_VAR) : get_int_var(NUM_OPMODES_VAR);
	if (argc==4) 
		add_mode(chan,(char *)mode,plus == '+'?1:0, argv[3], NULL, num_modes);
	else 
		add_mode(chan,(char *)mode,plus == '+'?1:0, NULL, NULL, num_modes);
	
	return TCL_OK;
}

int tcl_flushmode STDVAR
{
ChannelList *chan;
	
	BADARGS(2,2," channel");
	if (!(chan = lookup_channel(argv[1], from_server, CHAN_NOUNLINK)))
	{
		Tcl_AppendResult(irp,"invalid channel: ",argv[1],NULL);
		return TCL_ERROR;
	}
	flush_mode_all(chan);
	return TCL_OK;              
}

int tcl_maskhost STDVAR
{
char buffer[BIG_BUFFER_SIZE+1];
char *uh, *h;
	
	BADARGS(2, 2, " nick!user@host");
	strcpy(buffer, argv[1]);
	if (!(uh = strchr(buffer, '!')) || !(h = strchr(buffer, '@')))
		return TCL_ERROR;
	*uh++ = '\0';
	Tcl_AppendResult(irp, cluster(uh), NULL);
	return TCL_OK;
}

int tcl_onchansplit STDVAR
{
WhowasList *tmp;
int found = 0;
	
	BADARGS(3, 3, " nickname channel");
	for (tmp = next_userhost(&whowas_splitin_list, NULL); tmp; tmp = next_userhost(&whowas_splitin_list, tmp))
	{
		if (!my_stricmp(tmp->nicklist->nick, argv[1]) && !my_stricmp(tmp->channel, argv[2]))
		{
			found = 1;
			break;
		}	
	}
	Tcl_AppendResult(irp, found?one:zero, NULL);
	return TCL_OK;
}

int tcl_timers STDVAR
{
	
	BADARGS(1, 1, "");
	tcl_list_timer(irp, tcl_Pending_timers);
	return TCL_OK;
}

int tcl_utimers STDVAR
{
	
	BADARGS(1, 1, "");
	tcl_list_timer(irp, tcl_Pending_utimers);
	return TCL_OK;
}

int tcl_servers STDVAR
{
char *s = NULL;
int i;
	
	for (i = 0; i < server_list_size(); i++)
	{
		malloc_sprintf(&s, "%d %s %d %d", i, get_server_name(i), get_server_port(i), is_server_connected(i));
		Tcl_AppendElement(irp, s);
	}
	new_free(&s);
	return TCL_OK;
}

extern char *get_dcc_info(SocketList *, DCC_int *, int);

int tcl_dcc_stat STDVAR
{
int i;
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (check_dcc_socket(i))
		{
			SocketList *s;
			char *str;
			s = get_socket(i);
			str = get_dcc_info(s, s->info, i);
			Tcl_AppendElement(irp, str);
			new_free(&str);
		}
	}
	return TCL_OK;
}

int tcl_dcc_close STDVAR
{
int i;
	BADARGS(2, 2, " number");
	i = atol(argv[1]);
	if (i)
		close_dcc_number(i);	
	return TCL_OK;
}

void chan_struct(Tcl_Interp *irp, ChannelList *c)
{
char *s = NULL;
	
	malloc_sprintf(&s, "%s %d %lu %s %d %d %d %lu %lu %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %lu %d %lu %d %lu %d %lu %d %lu %d %d %d %d %d %d %d %lu %d %d %lu %d %d", 
		c->channel, c->server, c->mode, c->s_mode, c->limit, 
		(int)c->have_op, (int)c->voice, c->channel_create.tv_sec, c->join_time.tv_sec, c->stats_ops, 
		c->stats_dops, c->stats_bans, c->stats_unbans, c->stats_sops, 
		c->stats_sdops, c->stats_sbans, c->stats_sunbans, c->stats_topics, 
		c->stats_kicks, c->stats_pubs, c->stats_parts, c->stats_signoffs, 
		c->stats_joins, c->csets->set_ainv, c->csets->set_auto_rejoin, 
		c->csets->set_deop_on_deopflood, c->csets->set_deop_on_kickflood, 
		c->csets->set_deopflood, c->csets->set_deopflood_time, c->csets->set_hacking, 
		c->csets->set_kick_on_deopflood, c->csets->set_kick_on_joinflood, 
		c->csets->set_kick_on_kickflood, c->csets->set_kick_on_nickflood, 
		c->csets->set_kick_on_pubflood, c->csets->set_kickflood, c->csets->set_kickflood_time, 
		c->csets->set_nickflood, c->csets->set_nickflood_time, c->csets->set_joinflood, 
		c->csets->set_joinflood_time, c->csets->set_pubflood, c->csets->set_pubflood_time, 
		c->csets->set_pubflood_ignore, c->csets->set_userlist, c->csets->set_shitlist, c->csets->set_lamelist, 
		c->csets->set_kick_if_banned, c->totalnicks, c->maxnicks, c->maxnickstime, 
		c->totalbans, c->maxbans, c->maxbanstime, c->csets->set_aop, c->csets->bitch_mode);
	Tcl_AppendElement(irp, s);
	new_free(&s);
}

int tcl_chanstruct STDVAR
{
ChannelList *c;
	
	BADARGS(1, 1, "");
	for (c = get_server_channels(from_server); c; c = c->next)
		chan_struct(irp, c);
	return TCL_OK;
}

int tcl_channels STDVAR
{
ChannelList *c;
	
	BADARGS(1, 1, "");
	for (c = get_server_channels(from_server); c; c = c->next)
		Tcl_AppendElement(irp, c->channel);
	return TCL_OK;
}

int tcl_channel_modify(Tcl_Interp *irp, char *channel, ChannelList *chan, int argc, char **argv)
{
	
	return TCL_OK;
}

ChannelList *tcl_channel_add(Tcl_Interp *irp, char *chan, char *options)
{
ChannelList *tmp = NULL;
int argc = 0; char **argv = NULL;

	
	if (Tcl_SplitList(irp, options, &argc, &argv) != TCL_OK)
		return NULL;
	
	if (!(tmp = lookup_channel(chan, from_server, CHAN_NOUNLINK)))
		tmp = add_channel(chan, from_server, 0);
	if (tmp)
		tcl_channel_modify(irp, chan, tmp, argc, argv);
	return tmp ? tmp : NULL;
}

int tcl_channel STDVAR
{
ChannelList *chan = NULL; 
int server = from_server;
	
	if (argc <= 2)
	{
		Tcl_AppendResult(irp, " command ?options?", NULL);
		return TCL_ERROR;
	}
	if (!my_stricmp(argv[1], "add"))
	{
		BADARGS(3, 999, " add channel-name ?options?");
		if (!(chan = tcl_channel_add(irp, argv[2], argc==3?"":argv[3])))
		{
			Tcl_AppendResult(irp, " channel error in [channel]", NULL);
			return TCL_ERROR;
		}
		send_to_server("JOIN %s %s", chan->channel, chan->key?chan->key:"");
		return TCL_OK;
	}
	if (!my_stricmp(argv[1], "set"))
	{
		BADARGS(3, 3, " set channel-name ?options?");
		if (!(chan = prepare_command(&server, argv[2], 3)))
		{
	        	WhowasChanList *whowaschan;
			if ((whowaschan = check_whowas_chan_buffer(argv[2], 0, 0)) == NULL)
			{
				Tcl_AppendResult(irp, " no such channel", NULL);
				return TCL_ERROR;
			} else
				chan = whowaschan->channellist;
		}
		return tcl_channel_modify(irp, chan->channel, chan, argc-3, &argv[3]);
	}
	if (!my_stricmp(argv[1], "info"))
	{
		BADARGS(3, 3, " info channel-name");
		if (!(chan = prepare_command(&server, argv[2], 3)))
		{
			Tcl_AppendResult(irp, " no such channel", NULL);
			return TCL_ERROR;
		}
		chan_struct(irp, chan);
		return TCL_OK;
	}
	if (!my_stricmp(argv[1], "remove"))
	{
		BADARGS(3, 3, " remove channel-name");
		if (!(chan = prepare_command(&server, argv[2], 3)))
		{
			Tcl_AppendResult(irp, " no such channel", NULL);
			return TCL_ERROR;
		}
		send_to_server("PART %s", chan->channel);
		return TCL_OK;
	}
	Tcl_AppendResult(irp,"unknown channel command: should be one of: ", "add, set, info, remove",NULL);
	return TCL_ERROR;
}

int tcl_getchanhost STDVAR
{
ChannelList *chan;
NickList *nick;
	
	BADARGS(3,3," nickname channel");
	if ((chan = lookup_channel(argv[2], from_server, 0)))
	{
		if ((nick = find_nicklist_in_channellist(argv[1], chan, 0)))
		{
			Tcl_AppendResult(irp, nick->host, NULL);
			return TCL_OK;
		}
		Tcl_AppendResult(irp, "no such nick: ", NULL);
	} else
		Tcl_AppendResult(irp, "illegal channel: ", NULL);
	return TCL_ERROR;
}

int is_user(char *nick)
{
UserList *tmp = NULL;
void *tmp2 = NULL;
int size = -1;
	for (tmp = next_userlist(NULL, &size, &tmp2); tmp; tmp = next_userlist(tmp, &size, &tmp2))
		if (!my_stricmp(nick, tmp->nick))
			break;
	return tmp?1:0;
}

int tcl_validuser STDVAR
{
	BADARGS(2,2," handle");
	if (is_user(argv[1])) 
		Tcl_AppendResult(irp,one,NULL);
	else 
		Tcl_AppendResult(irp,zero,NULL);
	return TCL_OK;
}
        
int tcl_finduser STDVAR
{
char *n, *host, *s = NULL;
UserList *nick;

	
	BADARGS(2, 2, " nick!user@host");
	n = argv[1];
	if ((host = strchr(argv[1], '!')))
		*host++ = '\0';
	if ((nick = lookup_userlevelc("*", host, "*", NULL)))
		malloc_sprintf(&s, "%d %s%s%s", nick->flags, nick->channels,nick->password?" ":"",nick->password?nick->password:"");
	else
		malloc_strcat(&s, "*");
	Tcl_AppendResult(irp, s, NULL);
	new_free(&s);
	return TCL_OK;
}

int tcl_findshit STDVAR
{
char *n, *host, *s = NULL;
ShitList *nick;

	
	BADARGS(2, 2, " nick!user@host");
	n = argv[1];
	if ((host = strchr(argv[1], '!')))
		*host++ = '\0';
	nick = nickinshit(n, host);
	if (nick)
		malloc_sprintf(&s, "%d %s %s", nick->level, nick->channels, nick->reason);
	else
		malloc_strcat(&s, "*");
	Tcl_AppendResult(irp, s, NULL);
	new_free(&s);
	return TCL_OK;
}

int matchattr STDVAR
{
int is_match = 0;
unsigned int flags = 0;
NickList *nick = NULL;
ChannelList *chan;

	
	BADARGS(3, 3, " nickname level");
	for (chan = get_server_channels(from_server); chan; chan = chan->next)
	{
		for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
			if (wild_match(argv[1], nick->nick))
				goto found;
	}
found:
	if (!nick)
	{
		Tcl_AppendResult(irp, "no such nick: ", NULL);
		return TCL_ERROR;
	}

	flags = convert_str_to_flags(argv[2]);
	if (nick->userlist && (nick->userlist->flags & flags))
		is_match = 1;
	Tcl_AppendResult(irp, is_match ? one:zero, NULL);
	return TCL_OK;
}

int tcl_isop STDVAR
{
ChannelList *chan;
NickList *nick;

	
	BADARGS(3, 3, " nick channel");
	if ((chan = lookup_channel(argv[2], from_server, 0)))
	{
		if ((nick = find_nicklist_in_channellist(argv[1], chan, 0)))
		{
			if (nick_isop(nick))
				Tcl_AppendResult(irp, one, NULL);
			else
				Tcl_AppendResult(irp, zero, NULL);
			return TCL_OK;
		}
		Tcl_AppendResult(irp, "no such nick: ", NULL);
		return TCL_ERROR;
	} 
	Tcl_AppendResult(irp, "illegal channel: ", NULL);
	return TCL_ERROR;
}

int tcl_date STDVAR
{
char s[81]; time_t t;

	
	BADARGS(1,1,"");
	t = now; 
	strcpy(s,my_ctime(t));
	s[10]=s[24]=0; 
	strcpy(s,&s[8]); 
	strcpy(&s[8],&s[20]);
	strcpy(&s[2],&s[3]);
	Tcl_AppendResult(irp,s,NULL);
	return TCL_OK;
}

int tcl_time STDVAR
{
char s[81]; time_t t;

	
	BADARGS(1,1,"");
	t = now; 
	strcpy(s,my_ctime(t));
	strcpy(s,&s[11]); 
	s[5]=0;
	Tcl_AppendResult(irp,s,NULL);
	return TCL_OK;
}

int tcl_ctime STDVAR
{

	
	BADARGS(2, 2, " unixtime");
	Tcl_AppendResult(irp, my_ctime((time_t)atol(argv[1])), NULL);
	return TCL_OK;
}

int tcl_chanlist STDVAR
{
ChannelList *chan;
NickList *nick;
int level = 0;

	
	BADARGS(2, 3, " channel <flags>");
	if (!(chan = lookup_channel(argv[1], from_server, 0)))
	{
		Tcl_AppendResult(irp, "invalid channel:", argv[1], NULL);
		return TCL_ERROR;
	}
	if (argc == 2)
	{
		for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
			Tcl_AppendElement(irp, nick->nick);
		return TCL_OK;
	}
	if (isdigit(*argv[2]))
	{
		level = atoi(argv[2]);
		for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
		{
			if ((level <= 0) || (level && nick->userlist && nick->userlist->flags >= level))
				Tcl_AppendElement(irp, nick->nick);
		}
	} 
	else if (tolower(*argv[2]) == 'o' || tolower(*argv[2]) =='v')
	{
		for (nick = next_nicklist(chan, NULL); nick; nick = next_nicklist(chan, nick))
		{
			if (!nick_isop(nick) && !nick_isvoice(nick))
				continue;
			switch(tolower(*argv[2]))
			{
				case 'o':
					if (nick_isop(nick))
						Tcl_AppendElement(irp, nick->nick);
					break;
				case 'v':
					if (nick_isvoice(nick))
						Tcl_AppendElement(irp, nick->nick);
					break;
			}
		}
	}
	return TCL_OK;
}

int tcl_onchan STDVAR
{
ChannelList *chan;
NickList *nick;

	
	BADARGS(3, 3, " nickname channel");
	if (!(chan = lookup_channel(argv[2], from_server, 0)))
	{
		Tcl_AppendResult(irp, "invalid channel:", argv[2], NULL);
		return TCL_ERROR;
	}
	if ((nick = find_nicklist_in_channellist(argv[1], chan, 0)))
		Tcl_AppendResult(irp, one, NULL);
	else
		Tcl_AppendResult(irp, zero, NULL);
	return TCL_OK; 
}

int tcl_putlog STDVAR
{
char logtext[BIG_BUFFER_SIZE+1];

	
	BADARGS(2,2," text");
	strmcpy(logtext,argv[1],BIG_BUFFER_SIZE); 
	putlog(LOG_CRAP,"*","%s",logtext);
	return TCL_OK;
}

int tcl_putcmdlog STDVAR
{
char logtext[BIG_BUFFER_SIZE+1];

	
	BADARGS(2,2," text");
	strmcpy(logtext,argv[1],BIG_BUFFER_SIZE); 
	putlog(LOG_ALL,"*","%s",logtext);
	return TCL_OK;
}

int tcl_putloglev STDVAR
{
int i,lev=0; 
char logtext[BIG_BUFFER_SIZE + 1];

	
	BADARGS(4,4," level channel text");
	i=atoi(argv[1]); 
	if (i < 1 || i > 5) 
	{
		Tcl_AppendResult(irp,"level out of range (must be 1-5)",NULL);
		return TCL_ERROR;
	}
	switch(i) 
	{
		case 1: 
			lev=LOG_USER1; 
			break;
		case 2: 
			lev=LOG_USER2; 
			break;
		case 3: 
			lev=LOG_USER3; 
			break;
		case 4: 
			lev=LOG_USER4; 
			break;
		case 5: 
			lev=LOG_USER5; 
			break;
	}
	strmcpy(logtext,argv[3],BIG_BUFFER_SIZE); 
	putlog(lev,argv[2],"%s",logtext);
	return TCL_OK;
}

int tcl_unixtime STDVAR
{
	char s[60]; 

	
	BADARGS(1,1,"");
	sprintf(s,"%lu", now);
	Tcl_AppendResult(irp,s,NULL);
	return TCL_OK;
}

int tcl_putserv STDVAR
{
char s[BIG_BUFFER_SIZE+1],*p;

	
	BADARGS(2,2," \"text\"");
	strmcpy(s,argv[1],BIG_BUFFER_SIZE-1); 
	s[BIG_BUFFER_SIZE-1]=0;
	if ((p=strchr(s,'\n')))
		*p = 0;
	if ((p=strchr(s,'\r')))
		*p=0;
	send_to_server("%s",s); 
	return TCL_OK;
}

int tcl_putscr STDVAR
{
char s[BIG_BUFFER_SIZE+1],*p;

	
	BADARGS(2,2," \"text\"");
	strmcpy(s,argv[1],BIG_BUFFER_SIZE-1); 
	s[BIG_BUFFER_SIZE-1]=0;
	if ((p=strchr(s,'\n')))
		*p = 0;
	if ((p=strchr(s,'\r')))
		*p=0;
	put_it("%s",s); 
	return TCL_OK;
}

int tcl_putdcc STDVAR
{
char s[BIG_BUFFER_SIZE+1];
int idx = 0;
	BADARGS(3,3 ," hand idx text");

	strmcpy(s,argv[2],BIG_BUFFER_SIZE-2); 
	strncat(s,"\n",BIG_BUFFER_SIZE-1);
	s[BIG_BUFFER_SIZE]=0;
	idx = atoi(argv[1]);
	send(idx, s, strlen(s), 0);
	return TCL_OK;
}

int tcl_putbot STDVAR
{
char *msg = NULL;
SocketList *s = NULL;	
	BADARGS(3,3," botnick message");
	s = find_dcc(argv[1], "chat", NULL, DCC_BOTMODE, 0, 1, -1);
	if (!s) 
	{
		Tcl_AppendResult(irp,"user is not connected",NULL);
		return TCL_ERROR;
	}
	malloc_sprintf(&msg, "zapf %s %s %s\n", get_server_nickname(from_server), argv[1], argv[2]);
	send(s->is_read, msg, strlen(msg), 0);
	new_free(&msg);
	return TCL_OK;
}

int tcl_putallbots STDVAR
{
char *msg = NULL;
int i;
SocketList *s;
	BADARGS(2,2," message");
	malloc_sprintf(&msg, "zapf-broad %s %s\n", get_server_nickname(from_server), argv[1]); 
	for (i = 0; i < get_max_fd()+1; i++)
	{
		if (!check_dcc_socket(i))
			continue;
		s = get_socketinfo(i);
		if (((s->flags & DCC_TYPES) == DCC_BOTMODE) && (s->is_read != -1))
			send(s->is_read, msg, strlen(msg), 0);
	}
	new_free(&msg);
	return TCL_OK;
}

/* initialize hash tables */
void init_hash()
{

	
	Tcl_InitHashTable(&H_msg,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_dcc,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_fil,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_pub,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_msgm,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_pubm,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_join,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_part,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_sign,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_kick,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_topc,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_mode,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_ctcp,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_ctcr,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_nick,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_raw,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_bot,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_chon,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_chof,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_sent,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_rcvd,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_chat,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_link,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_disc,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_splt,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_rejn,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_filt,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_cmd,TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_input, TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_notice, TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_alias, TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_help, TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_functions, TCL_STRING_KEYS);
	Tcl_InitHashTable(&H_hook, TCL_STRING_KEYS);
}


/* remove command */
int cmd_unbind(int typ,int flags,char *cmd, char *proc)
{
tcl_cmd_t *tt,*last; 
Tcl_HashEntry *he; Tcl_HashTable *ht;


	
	ht=gethashtable(typ,NULL,NULL);
	he=Tcl_FindHashEntry(ht,cmd);

	if (he==NULL) 
		return 0;   /* no such binding */
	tt=(tcl_cmd_t *)Tcl_GetHashValue(he); 
	last=NULL;

	while (tt!=NULL) 
	{
		/* if procs are same, erase regardless of flags */
		if (!my_stricmp(tt->func_name,proc)) {
			/* erase it */
			if (last!=NULL) 
				last->next=tt->next;
			else 
			{
				if (tt->next==NULL) 
					Tcl_DeleteHashEntry(he);
				else 
					Tcl_SetHashValue(he,tt->next);
			}
			new_free((char **)&tt->func_name);
			new_free((char **)&tt); 
			return 1;
		}
		last=tt; 
		tt=tt->next;
	}
	return 0;   /* no match */
}

/* add command (remove old one if necessary) */
int cmd_bind(int typ,int flags,char *cmd, char *proc, char *(*func)(char *, char *))
{
tcl_cmd_t *tt; 
int new; 
Tcl_HashEntry *he; 
Tcl_HashTable *ht; 
int stk;

	
	if (proc[0]=='#') 
		return 0;

	cmd_unbind(typ,flags,cmd,proc);    /* make sure we don't dup */
	tt=(tcl_cmd_t *)new_malloc(sizeof(tcl_cmd_t)); 
	
	tt->flags_needed=flags; 
	tt->next=NULL;
	malloc_strcpy(&(tt->func_name), proc);
	tt->func = func;
	ht=gethashtable(typ,&stk,NULL);
	he=Tcl_CreateHashEntry(ht,cmd,&new);
	
	if (!new) 
	{
		tt->next=(tcl_cmd_t *)Tcl_GetHashValue(he);
		if (!stk) 
		{
			/* remove old one -- these are not stackable */
			new_free(&tt->next->func_name); 
			new_free((char **)&tt->next); 
			tt->next=NULL;
		}
	}
	Tcl_SetHashValue(he,tt);
	return 1;
}

/* match types for check_tcl_bind */
#define MATCH_PARTIAL       0
#define MATCH_EXACT         1
#define MATCH_MASK          2
/* bitwise 'or' these: */
#define BIND_USE_ATTR       4
#define BIND_STACKABLE      8
#define BIND_HAS_BUILTINS   16
#define BIND_WANTRET        32

/* return values */
#define BIND_NOMATCH    0
#define BIND_AMBIGUOUS  1
#define BIND_MATCHED    2    /* but the proc couldn't be found */
#define BIND_EXECUTED   3
#define BIND_EXEC_LOG   4    /* proc returned 1 -> wants to be logged */

/* 10+  -->  matches builtin # (retval-10)  */

/* trigger (execute) a proc */

int trigger_bind(char *proc, char *param, char *(*func)(char *, char *))
{
char *result = NULL;
	
	if (internal_debug & DEBUG_TCL)
		debugyell("Tcl exec [%s] with [%s]", proc, param);
	if (func)
	{
		result = (*func)(proc, param);
		Tcl_AppendResult(tcl_interp,result);
		new_free(&result);
		if (internal_debug & DEBUG_TCL)
			debugyell("Tcl return from [%s] with [%s]", proc, result);
		return BIND_EXECUTED;
	}
	if (Tcl_VarEval(tcl_interp,proc,param,NULL)==TCL_ERROR) 
	{
		if (internal_debug & DEBUG_TCL)
			debugyell("Tcl return from [%s] with [%s]", proc, tcl_interp->result);
		putlog(LOG_ALL,"*","Tcl error [%s]: %s",proc,tcl_interp->result);
		return BIND_EXECUTED;
	}
	if (internal_debug & DEBUG_TCL)
		debugyell("Tcl return from [%s] with [%s]", proc, tcl_interp->result);
	return (atoi(tcl_interp->result)>0)?BIND_EXEC_LOG:BIND_EXECUTED;
}

void init_builtins()
{
int i, j, new; 
Tcl_HashTable *ht=NULL; 
Tcl_HashEntry *he;
tcl_cmd_t *tt; 
cmd_t *cc=NULL;


	
	for (j = 0; j < 4; j++ )
	{	
		switch(j)
		{
			case 0:
				ht = &H_msg; cc = C_msg; break;
			case 1:
				ht = &H_dcc; cc = C_dcc; break;
			case 2:
				ht = &H_ctcp; cc = C_ctcp; break;
			case 3:
				ht = &H_notice; cc = C_notice; break;
		}
		i = 0;
		while (cc[i].access != -1) 
		{
			tt = (tcl_cmd_t *) new_malloc(sizeof(tcl_cmd_t));
			tt->func_name = m_sprintf("*%s", cc[i].name);  
			tt->flags_needed=cc[i].access; 
			tt->next=NULL;
			he=Tcl_CreateHashEntry(ht,cc[i].name,&new);
			if (!new) 
			{
				/* append old entry */
				tcl_cmd_t *ttx=(tcl_cmd_t *)Tcl_GetHashValue(he);
				Tcl_DeleteHashEntry(he);
				tt->next=ttx;
			}
			Tcl_SetHashValue(he,tt);
			i++;
		}
	}	
}


int flags_needed(unsigned long atr, unsigned long needed)
{
#if 0
	if (atr & ADD_OWNER) return 1;
	if (atr & ADD_MASTER) return 1;
#endif
	if ((atr & needed) == needed) return 1;
	return 0;
}

/* check a tcl binding and execute the procs necessary */
int check_tcl_bind(Tcl_HashTable *hash, char *mat, int atr, char *param, 
		int match_type, cmd_t *builtin)
{
Tcl_HashSearch srch; 
Tcl_HashEntry *he; 
int cnt=0; 
char *proc=NULL;
tcl_cmd_t *tt; 
int f=0,atrok,x;


	
	for (he=Tcl_FirstHashEntry(hash,&srch); (he!=NULL) && (!f); he=Tcl_NextHashEntry(&srch)) 
	{
		int ok=0;
		switch (match_type&0x03) {
			case MATCH_PARTIAL:
				ok=(my_strnicmp(mat,Tcl_GetHashKey(hash,he),strlen(mat))==0); 
				break;
			case MATCH_EXACT:
				ok=(my_stricmp(mat,Tcl_GetHashKey(hash,he))==0);
				break;
			case MATCH_MASK:
				ok=wild_match(Tcl_GetHashKey(hash,he), mat);
				break;
		}
		if (ok) {
			tt=(tcl_cmd_t *)Tcl_GetHashValue(he);
			switch (match_type&0x03) 
			{
				case MATCH_MASK:
					/* could be multiple triggers */
					while (tt!=NULL) 
					{
						if (match_type & BIND_HAS_BUILTINS)
							atrok = flags_needed(atr, tt->flags_needed);
						else
							atrok = ((atr & tt->flags_needed) == tt->flags_needed);
#if 0
						if (match_type & BIND_HAS_BUILTINS)
							atrok= (atr > tt->flags_needed);
						else 
							atrok= (atr >= tt->flags_needed);
#endif
						if ((!(match_type&BIND_USE_ATTR)) || atrok)
						{
							cnt++; 
							x=trigger_bind(tt->func_name,param, tt->func);
							if ((match_type & BIND_WANTRET) && (x==BIND_EXEC_LOG))
								return x;
						}
						tt=tt->next;
					}
					break;
				default:
					if (match_type & BIND_HAS_BUILTINS)   
						atrok = flags_needed(atr, tt->flags_needed);
					else
						atrok = ((atr & tt->flags_needed) == tt->flags_needed);
#if 0
					else 
						atrok=(atr >= tt->flags_needed);
#endif
					if ((!(match_type&BIND_USE_ATTR)) || atrok) 
					{
						cnt++; 
						proc=tt->func_name;
						if (my_stricmp(mat,Tcl_GetHashKey(hash,he))==0) 
						{
							cnt=1; 
							f=1;  /* perfect match */
						}
					}
					break;
			}
		}
	}
	if (cnt==0) return BIND_NOMATCH;
	if ((match_type&0x03)==MATCH_MASK) return BIND_EXECUTED;
	if (cnt>1) return BIND_AMBIGUOUS;

	if ((match_type&BIND_HAS_BUILTINS) && (proc[0]=='*')) 
	{
		/* calling builtin function */
		int top=0,bot=0,try=0,xx; 
		char pr[81];
		
		strcpy(pr,&proc[1]); 
		f=0;
		while (builtin[bot].access!=(-1)) bot++;  /* find bottom */
		
		/* binary search: */
		while (!f) 
		{
			try=(top+bot)/2; 
			xx=my_stricmp(builtin[try].name,pr);
			if (xx==0) f=1;
			if (xx<0) top=try+1;
			if (xx>0) bot=try;
			if (top==bot)
				return BIND_MATCHED;
		}
		return try+10;
	}
	return trigger_bind(proc,param, NULL);
}

int tcl_timer STDVAR
{
char *x; char s[80];
	BADARGS(3,3," minutes command");
	if (atoi(argv[1]) <= 0) 
	{
		Tcl_AppendResult(irp,"time value must be positive, nonzero",NULL);
		return TCL_ERROR;
	}
	if (argv[2][0]!='#') 
	{
		x = tcl_add_timer(&tcl_Pending_timers, atol(argv[1])*60, argv[2], 0L);
		sprintf(s,"timer%s",x); 
		Tcl_AppendResult(irp,s,NULL);
	}
	return TCL_OK;
}

int tcl_rand STDVAR
{
unsigned long x; 
char s[80];

	
	BADARGS(2,2," limit");
	if (strtoul(argv[1], NULL, 10) <= 0)
	{
		Tcl_AppendResult(irp,"limit must be greater than 0",NULL);
		return TCL_ERROR;
	}
 	x = rand() % (strtoul(argv[1], NULL, 10));
	sprintf(s,"%lu",x);
	Tcl_AppendResult(irp,s,NULL);
	return TCL_OK;
}

int tcl_utimer STDVAR
{
char *x; 
char s[80];
	BADARGS(3,3," seconds command");
	if (atoi(argv[1]) <= 0) 
	{
		Tcl_AppendResult(irp,"time value must be positive, nonzero",NULL);
		return TCL_ERROR;
	}
	if (argv[2][0]!='#') 
	{
		x=tcl_add_timer(&tcl_Pending_utimers, atol(argv[1]), argv[2], 0L);
		sprintf(s,"timer%s",x); 
		Tcl_AppendResult(irp,s,NULL);
	}
	return TCL_OK;
}

int tcl_killtimer STDVAR
{
	BADARGS(2,2," timerID");
	if (strncmp(argv[1],"timer",5)!=0) 
	{
		Tcl_AppendResult(irp,"argument is not a timerID",NULL);
		return TCL_ERROR;
	}
	if (tcl_remove_timer(&tcl_Pending_timers, atol(&argv[1][5]))) 
		return TCL_OK;
	Tcl_AppendResult(irp,"invalid timerID",NULL);
	return TCL_ERROR;
}

int tcl_killutimer STDVAR
{
	BADARGS(2,2," timerID");
	if (strncmp(argv[1],"timer",5)!=0) 
	{
		Tcl_AppendResult(irp,"argument is not a timerID",NULL);
		return TCL_ERROR;
	}
	if (tcl_remove_timer(&tcl_Pending_utimers, atol(&argv[1][5]))) 
		return TCL_OK;
	Tcl_AppendResult(irp,"invalid timerID",NULL);
	return TCL_ERROR;
}

/* check for tcl-bound msg command, return 1 if found */
/* msg: proc-name <nick> <user@host> <handle> <args...> */
int check_tcl_msg(char *cmd, char *nick, char *uhost, char *hand, char *args)
{
int x, atr = 0;
UserList *n;
	
	if (!cmd)
		return 0;
	if ((n = lookup_userlevelc("*", uhost, "*", NULL)))
		atr = n->flags;
		
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",!n?hand:n->nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",args,TCL_GLOBAL_ONLY);
	x=check_tcl_bind(&H_msg,cmd,atr," $_n $_uh $_h $_a",
		MATCH_PARTIAL|BIND_HAS_BUILTINS|BIND_USE_ATTR,C_msg);
	if (x == BIND_EXEC_LOG)
		putlog(LOG_TCL, "*", "(%s!%s) !%s! %s %s",nick,uhost,hand,cmd,args);
	if (x>=10) 
	{
		if (internal_debug & DEBUG_TCL)
			debugyell("Tcl func [%s] with [%s %s %s %s]", C_msg[x-10].name , hand, nick, uhost, args);
		return (C_msg[x-10].func)(hand,nick,uhost,args);
	}
	return ((x==BIND_MATCHED)||(x==BIND_EXECUTED)||(x==BIND_EXEC_LOG));
}

/* check for tcl-bound dcc msg command, return 1 if found */
/* msg: proc-name <nick> <user@host> <idx>*/
int check_tcl_dcc(char *cmd, char *nick, char *host, int idx)
{
	int x, atr = 1;
	char *c, *args;
	UserList *n;
	DCC_int *info;
	
	info = get_socketinfo(idx);
	if (info->ul)
		atr = info->ul->flags;
#if 0		
	if ((n = lookup_userlevelc("*", host, "*", NULL)))
	{
		DCC_int *info;
		info = get_socketinfo(idx);
		atr = n->flags;
	}
#endif
	if (!cmd || !*cmd)
		return 0;
	c = next_arg(cmd, &cmd);
	if (!c || !*c)
		return 0;

	args = cmd;
	Tcl_SetVar(tcl_interp,"_i",ltoa(idx), TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",host,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",args,TCL_GLOBAL_ONLY);
	x=check_tcl_bind(&H_dcc,c,atr," $_n $_i $_a", MATCH_PARTIAL|BIND_HAS_BUILTINS|BIND_USE_ATTR, C_dcc);
	if (x==BIND_AMBIGUOUS || x==BIND_NOMATCH)
		return 0;
	if (x>=10) 
	{
		if (C_dcc[x-10].func == NULL)
			return 1;
		if (internal_debug & DEBUG_TCL)
			debugyell("Tcl func [%s] with [%d %s]", C_dcc[x-10].name , idx, args);
		(C_dcc[x-10].func)(idx,args);
		return 1;
	}
	return 1;
}

int check_tcl_ctcp(char *nick, char *uhost, char *hand, char *dest, char *keyword, char *args)
{
	int atr = 0,x;
	UserList *n;
	
	if ((n = lookup_userlevelc("*", uhost, "*", NULL)))
		atr = n->flags;
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",dest,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",keyword,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aaa",args,TCL_GLOBAL_ONLY);
	x=check_tcl_bind(&H_ctcp,keyword,atr," $_n $_uh $_h $_a $_aa $_aaa",
		MATCH_MASK|BIND_HAS_BUILTINS|BIND_USE_ATTR,NULL);
	return ((x==BIND_MATCHED)||(x==BIND_EXECUTED)||(x==BIND_EXEC_LOG));
}

int check_tcl_pub(char *nick, char *from, char *chname, char *msg)
{
int x,atr = 0; 
char *args,*cmd, *free_me;
UserList *n;
	
	free_me = args = m_strdup(msg); 

	cmd = next_arg(args, &args);
	if (!cmd || !*cmd)
	{
		new_free(&free_me);
		return 0;
	}
	if ((n = lookup_userlevelc("*",from, chname, NULL)))
		atr = n->flags;
	
	Tcl_SetVar(tcl_interp,"_n",		nick,		TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",	from,		TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",		n?n->nick:nick,	TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",		chname,		TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",	args,		TCL_GLOBAL_ONLY);
	
	x=check_tcl_bind(&H_pub,cmd,atr," $_n $_uh $_h $_a $_aa",
		MATCH_EXACT|BIND_USE_ATTR,NULL);
	new_free(&free_me);
	if (x==BIND_NOMATCH) return 0;

	return 1;
}

void check_tcl_pubm(char *nick, char *uhost, char *chname, char *msg)
{
char *args = NULL; 
int atr = 0;
UserList *n;	

	
	malloc_sprintf(&args, "%s %s", chname, msg);
	if ((n = lookup_userlevelc("*",uhost, chname, NULL)))
		atr = n->flags;
	
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",msg,TCL_GLOBAL_ONLY);
	
	check_tcl_bind(&H_pubm,args,atr," $_n $_uh $_h $_a $_aa",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
	new_free(&args);
}

void check_tcl_msgm(char *cmd, char *nick, char *uhost, char *hand, char *arg)
{
int atr = 0; 
char *args = NULL;
UserList *n;
	
	if (!cmd)
		return;
	malloc_sprintf(&args, "%s%s%s", cmd, arg ?" ":"", arg?arg:"");
	if ((n = lookup_userlevelc("*",uhost, "*", NULL)))
		atr = n->flags;

	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",args,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_msgm,args,atr," $_n $_uh $_h $_a",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
	new_free(&args);
}

int check_on_hook(int which, char *buffer)
{
int ret = 0;
char name[BIG_BUFFER_SIZE+1];

	*name = 0;
	if (!buffer || !*buffer)
		return ret;
		
	if (which > -1 && which < NUMBER_OF_LISTS)
	{
		Tcl_SetVar(tcl_interp, "_a", hook_functions[which].name, TCL_GLOBAL_ONLY);
		strcpy(name, hook_functions[which].name);
	}
	else 
	{
		strcpy(name, ltoa(which));
		Tcl_SetVar(tcl_interp, "_a", name, TCL_GLOBAL_ONLY);
	}
	strcat(name, " ");
	Tcl_SetVar(tcl_interp, "_aa", buffer, TCL_GLOBAL_ONLY);
	strmcat(name, buffer, BIG_BUFFER_SIZE);
	ret = check_tcl_bind(&H_hook, name, -1, " $_a $_aa", MATCH_MASK|BIND_STACKABLE|BIND_WANTRET,NULL);
	return ret;
}

void check_tcl_join(char *nick,char *uhost, char *hand, char *channel)
{
int atr = 0; 
char args[BIG_BUFFER_SIZE+1];
UserList *n;

	
	if ((n = lookup_userlevelc("*", uhost, channel, NULL)))
		atr=n->flags;
	snprintf(args, BIG_BUFFER_SIZE, "%s %s!%s", channel, nick, uhost);
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",channel,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_join,args,atr," $_n $_uh $_h $_a",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
}

void check_tcl_part(char *nick, char *uhost, char *hand, char *chname)
{
int atr = 0; 
char args[BIG_BUFFER_SIZE+1];
UserList *n;
int x;

	snprintf(args,BIG_BUFFER_SIZE, "%s %s!%s",chname,nick,uhost);
	if ((n = lookup_userlevelc("*",uhost, chname, NULL)))
		atr = n->flags;
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	x = check_tcl_bind(&H_part,args,atr," $_n $_uh $_h $_a",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
}

void check_tcl_sign(char *nick, char *uhost,char *hand, char *chname,char *reason)
{
int atr = 0; 
char args[BIG_BUFFER_SIZE+1];
UserList *n;
	
	if ((n = lookup_userlevelc("*",uhost, chname, NULL)))
		atr = n->flags;
	snprintf(args,BIG_BUFFER_SIZE, "%s %s!%s",chname,nick,uhost);
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",reason,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_sign,args,atr," $_n $_uh $_h $_a $_aa",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
}

void check_tcl_topc(char *nick,char *uhost,char *hand,char *chname,char *topic)
{
int atr = 0;
char args[BIG_BUFFER_SIZE+1];
UserList *n;

	snprintf(args,BIG_BUFFER_SIZE, "%s %s",chname,topic);
	if ((n = lookup_userlevelc("*",uhost, chname, NULL)))
		atr = n->flags;
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",topic,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_topc,args,atr," $_n $_uh $_h $_a $_aa",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
}

void check_tcl_nick(char *nick,char *uhost,char *hand,char *chname, char *newnick)
{
int atr = 0; 
char args[BIG_BUFFER_SIZE];
UserList *n;
	
	if ((n = lookup_userlevelc("*",uhost, chname, NULL)))
		atr = n->flags;
	snprintf(args, BIG_BUFFER_SIZE, "%s %s",chname,newnick);
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",newnick,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_nick,args,atr," $_n $_uh $_h $_a $_aa",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
}

void check_tcl_kick(char *nick,char *uhost,char *hand,char *chname,char *dest,char *reason)
{
char args[BIG_BUFFER_SIZE];
int atr = 0;
UserList *n;
	
	if ((n = lookup_userlevelc("*",uhost, chname, NULL)))
		atr = n->flags;
	sprintf(args,"%s %s",chname,dest);
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",dest,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aaa",reason,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_kick,args,atr," $_n $_uh $_h $_a $_aa $_aaa",
		MATCH_MASK|BIND_STACKABLE,NULL);
}

int check_tcl_raw(char *raw, char *comm)
{
	int ret = 0;
	if (raw && *raw)
	{
		Tcl_SetVar(tcl_interp,"_a",raw,TCL_GLOBAL_ONLY);
		ret = check_tcl_bind(&H_raw,comm, -1," $_a",MATCH_MASK|BIND_STACKABLE|BIND_WANTRET,NULL);
	}
	return (ret == BIND_EXEC_LOG)? 1 : 0;
}

char *check_tcl_alias(char *command, char *args)
{
	
	Tcl_SetVar(tcl_interp, "_a", args?args:empty_string, TCL_GLOBAL_ONLY);
	if (check_tcl_bind(&H_functions, command, -1, " $_a", MATCH_MASK|BIND_STACKABLE, NULL))
		return m_strdup(tcl_interp->result?tcl_interp->result:empty_string);
	return NULL;
}

int check_tcl_input(char *raw)
{
	int ret = 0;
	
	if (raw && *raw)
	{
		Tcl_SetVar(tcl_interp,"_a",raw,TCL_GLOBAL_ONLY);
		ret = check_tcl_bind(&H_input,raw, -1," $_a",MATCH_MASK|BIND_STACKABLE,NULL);
	}
	return (ret == BIND_EXEC_LOG)? 1 : 0;
}

int check_help_bind(char *command)
{
int x = 0;
	
	if (command && *command)
	{
		char *comm;
		comm = m_sprintf("%s_help", lower(command));
		x=check_tcl_bind(&H_help,comm,-1,NULL, MATCH_EXACT|BIND_HAS_BUILTINS,NULL);
		new_free(&comm);
	}
	return (x ? 0 : 1);
}

void check_tcl_mode(char *nick,char *uhost,char *hand,char *chname,char *mode)
{
char args[BIG_BUFFER_SIZE+1];
UserList *n;

	n = lookup_userlevelc("*",uhost, chname, NULL);
	snprintf(args,BIG_BUFFER_SIZE, "%s %s",chname,mode);
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",mode,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_mode,args,0," $_n $_uh $_h $_a $_aa",
		MATCH_MASK|BIND_STACKABLE,NULL);
}

int check_tcl_ctcr(char *nick,char *uhost,char *hand,char *dest,char *keyword,char *args)
{
int atr = 0;
UserList *n;
	
	if ((n = lookup_userlevelc("*",uhost, "*", NULL)))
		atr = n->flags;
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",dest,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aa",keyword,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_aaa",args,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_ctcr,keyword,atr," $_n $_uh $_h $_a $_aa $_aaa",
		MATCH_MASK|BIND_USE_ATTR,NULL);
	return 1;
}

void check_tcl_chat(char *from,int chan,char *text)
{

	
	Tcl_SetVar(tcl_interp,"_n",from,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",zero,TCL_GLOBAL_ONLY); /* chan 0 always */
	Tcl_SetVar(tcl_interp,"_aa",text,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_chat,text,0," $_n $_a $_aa",MATCH_MASK|BIND_STACKABLE, NULL);
}

#if 0
void check_tcl_link(bot,via)
char *bot,*via;
{

	
	Tcl_SetVar(tcl_interp,"_n",bot,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",via,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_link,bot,0," $_n $_a",MATCH_MASK|BIND_STACKABLE,NULL);
}

void check_tcl_disc(bot)
char *bot;
{

	
	Tcl_SetVar(tcl_interp,"_n",bot,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_disc,bot,0," $_n",MATCH_MASK|BIND_STACKABLE,NULL);
}
#endif

void check_tcl_split(char *nick,char *uhost,char *hand,char *chname)
{
int atr = 0; 
char args[BIG_BUFFER_SIZE+1];
UserList *n;

	
	if ((n = lookup_userlevelc("*",uhost, chname, NULL)))
		atr = n->flags;
	snprintf(args,BIG_BUFFER_SIZE, "%s %s!%s",chname,nick,uhost);
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_splt,args,atr," $_n $_uh $_h $_a",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
}

void check_tcl_rejoin(char *nick,char *uhost,char *hand,char *chname)
{
int atr = 0; 
char args[BIG_BUFFER_SIZE+1];
UserList *n;

	
	if ((n = lookup_userlevelc("*",uhost, chname, NULL)))
		atr = n->flags;
	sprintf(args,"%s %s!%s",chname,nick,uhost);
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_uh",uhost,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",n?n->nick:hand,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",chname,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_rejn,args,atr," $_n $_uh $_h $_a",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE,NULL);
}

#if 0
int check_tcl_filt(idx,text)
int idx; char *text;
{
  char s[10]; int x,atr;

	
	atr=get_attr_handle(dcc[idx].nick); sprintf(s,"%d",dcc[idx].sock);
	Tcl_SetVar(tcl_interp,"_n",s,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",text,TCL_GLOBAL_ONLY);
	x=check_tcl_bind(&H_filt,text,atr," $_n $_a",
		MATCH_MASK|BIND_USE_ATTR|BIND_STACKABLE|BIND_WANTRET,NULL);
	return (x==BIND_EXEC_LOG);
}
#endif

void check_tcl_tand(char *nick, char *opcode, char *param)
{
	
	Tcl_SetVar(tcl_interp,"_n",nick,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_h",opcode,TCL_GLOBAL_ONLY);
	Tcl_SetVar(tcl_interp,"_a",param,TCL_GLOBAL_ONLY);
	check_tcl_bind(&H_bot,opcode,0," $_n $_h $_a",MATCH_EXACT,NULL);
}

Tcl_HashTable *gethashtable(typ,stk,name)
int typ,*stk; char *name;
{
char *nam=NULL; 
int st=0; 
Tcl_HashTable *ht=NULL;

	
	switch(typ) 
	{
		case CMD_MSG: ht=&H_msg; nam="msg"; break;
		case CMD_DCC: ht=&H_dcc; nam="dcc"; break;
		case CMD_FIL: ht=&H_fil; nam="fil"; break;
		case CMD_PUB: ht=&H_pub; nam="pub"; break;
		case CMD_MSGM: ht=&H_msgm; nam="msgm"; st=1; break;
		case CMD_PUBM: ht=&H_pubm; nam="pubm"; st=1; break;
		case CMD_JOIN: ht=&H_join; nam="join"; st=1; break;
		case CMD_PART: ht=&H_part; nam="part"; st=1; break;
		case CMD_SIGN: ht=&H_sign; nam="sign"; st=1; break;
		case CMD_KICK: ht=&H_kick; nam="kick"; st=1; break;
		case CMD_TOPC: ht=&H_topc; nam="topc"; st=1; break;
		case CMD_MODE: ht=&H_mode; nam="mode"; st=1; break;
		case CMD_CTCP: ht=&H_ctcp; nam="ctcp"; break;
		case CMD_CTCR: ht=&H_ctcr; nam="ctcr"; break;
		case CMD_NICK: ht=&H_nick; nam="nick"; st=1; break;
		case CMD_RAW: ht=&H_raw; nam="raw"; st=1; break;
		case CMD_BOT: ht=&H_bot; nam="bot"; break;
		case CMD_CHON: ht=&H_chon; nam="chon"; st=1; break;
		case CMD_CHOF: ht=&H_chof; nam="chof"; st=1; break;
		case CMD_SENT: ht=&H_sent; nam="sent"; st=1; break;
		case CMD_RCVD: ht=&H_rcvd; nam="rcvd"; st=1; break;
		case CMD_CHAT: ht=&H_chat; nam="chat"; st=1; break;
		case CMD_LINK: ht=&H_link; nam="link"; st=1; break;
		case CMD_DISC: ht=&H_disc; nam="disc"; st=1; break;
		case CMD_SPLT: ht=&H_splt; nam="splt"; st=1; break;
		case CMD_REJN: ht=&H_rejn; nam="rejn"; st=1; break;
		case CMD_FILT: ht=&H_filt; nam="filt"; st=1; break;
		case CMD_CMD: ht = &H_cmd; nam = "cmd"; st = 1; break;
		case CMD_INPUT: ht=&H_input; nam = "input"; st = 1; break;
		case CMD_ALIAS: ht=&H_alias; nam = "alias"; st = 1; break;
		case CMD_HELP: ht=&H_help; nam = "help"; st = 1; break;
		case CMD_FUNCTION: ht=&H_functions; nam = "func"; st = 1; break;
		case CMD_HOOK: ht=&H_hook; nam = "hook"; st = 1; break;
	}
	if (name!=NULL) 
		strcpy(name,nam);
	if (stk!=NULL) 
		*stk=st;
	return ht;
}

int get_bind_type(char *name)
{
int tp = -1;

	
	if (!my_stricmp(name,"dcc")) tp=CMD_DCC;
	else if (!my_stricmp(name,"msg"))  tp=CMD_MSG;
	else if (!my_stricmp(name,"fil"))  tp=CMD_FIL;
	else if (!my_stricmp(name,"pub"))  tp=CMD_PUB;
	else if (!my_stricmp(name,"msgm")) tp=CMD_MSGM;
	else if (!my_stricmp(name,"pubm")) tp=CMD_PUBM;
	else if (!my_stricmp(name,"join")) tp=CMD_JOIN;
	else if (!my_stricmp(name,"part")) tp=CMD_PART;
	else if (!my_stricmp(name,"sign")) tp=CMD_SIGN;
	else if (!my_stricmp(name,"kick")) tp=CMD_KICK;
	else if (!my_stricmp(name,"topc")) tp=CMD_TOPC;
	else if (!my_stricmp(name,"mode")) tp=CMD_MODE;
	else if (!my_stricmp(name,"ctcp")) tp=CMD_CTCP;
	else if (!my_stricmp(name,"ctcr")) tp=CMD_CTCR;
	else if (!my_stricmp(name,"nick")) tp=CMD_NICK;
	else if (!my_stricmp(name,"bot"))  tp=CMD_BOT;
	else if (!my_stricmp(name,"chon")) tp=CMD_CHON;
	else if (!my_stricmp(name,"chof")) tp=CMD_CHOF;
	else if (!my_stricmp(name,"sent")) tp=CMD_SENT;
	else if (!my_stricmp(name,"rcvd")) tp=CMD_RCVD;
	else if (!my_stricmp(name,"chat")) tp=CMD_CHAT;
	else if (!my_stricmp(name,"link")) tp=CMD_LINK;
	else if (!my_stricmp(name,"disc")) tp=CMD_DISC;
	else if (!my_stricmp(name,"rejn")) tp=CMD_REJN;
	else if (!my_stricmp(name,"splt")) tp=CMD_SPLT;
	else if (!my_stricmp(name,"filt")) tp=CMD_FILT;
	else if (!my_stricmp(name, "raw")) tp =CMD_RAW;
	else if (!my_stricmp(name,"cmd"))  tp = CMD_CMD;
	else if (!my_stricmp(name,"input")) tp = CMD_INPUT;
	else if (!my_stricmp(name,"alias")) tp = CMD_ALIAS;
	else if (!my_stricmp(name,"help")) tp = CMD_HELP;
	else if (!my_stricmp(name,"func")) tp = CMD_FUNCTION;
	else if (!my_stricmp(name, "hook")) tp = CMD_HOOK;
	return tp;
}

int tcl_bind STDVAR
{
	int fl = 0,tp;

	
	BADARGS(5,5," type flags cmdName procName");
	fl=convert_str_to_flags(argv[2]);
	tp=get_bind_type(argv[1]);
	if (tp<0) {
		Tcl_AppendResult(irp,"bad type, should be one of: dcc, msg, fil, pub, ",
					"msgm, pubm, join, part, sign, kick, topc, mode, ctcp, ",
					"nick, bot, chon, chof, sent, rcvd, chat, link, disc, ",
					"splt, rejn, filt, cmd, raw, input, alias, help, func, hook",NULL);
		return TCL_ERROR;
	}
	if ((long int)cd == 1) {
		if (!cmd_unbind(tp,fl,argv[3],argv[4])) 
		{
			/* don't error if trying to re-unbind a builtin */
			if (strcmp(argv[3],&argv[4][1])!=0) 
			{
				Tcl_AppendResult(irp,"no such binding",NULL);
				return TCL_ERROR;
			}
		}
	}
	else 
	{
		if (tp != CMD_ALIAS)
				cmd_bind(tp,fl,argv[3],argv[4], NULL);
		else
		{
			BuiltInFunctions *tmp;
			if ((tmp = find_func_alias(argv[4])))
				cmd_bind(tp, fl, argv[3], argv[4], tmp->func);
		}
	}
	Tcl_AppendResult(irp,argv[3],NULL);
	return TCL_OK;
}

int tcl_tellbinds STDVAR
{
Tcl_HashEntry *he;
Tcl_HashSearch srch;
Tcl_HashTable *ht;
int i,fnd=0;
tcl_cmd_t *tt;
char typ[5],*s,*proc;
int kind = -1;

	
	if (argc > 1)
		kind = get_bind_type(argv[1]);
	for (i=0; i<BINDS; i++) 
	{
		if ((kind==(-1)) || (kind==i)) 
		{
			ht=gethashtable(i,NULL,typ);
			for (he=Tcl_FirstHashEntry(ht,&srch); (he!=NULL); he=Tcl_NextHashEntry(&srch)) 
			{
				if (!fnd) 
				{
					bitchsay("TCL Command bindings:"); 
					fnd=1;
					put_it("  TYPE FLGS                 COMMAND              BINDING (TCL)");
				}
				tt=(tcl_cmd_t *)Tcl_GetHashValue(he);
				s=Tcl_GetHashKey(ht,he);
				while (tt!=NULL) {
					proc=tt->func_name; 
					if ((proc[0]!='*') || (strcmp(s,proc+1)!=0))
						put_it("  %-4s %-16s %-20s %s",typ,convert_flags_to_str(tt->flags_needed),s,tt->func_name);
					tt=tt->next;
				}	
			}
			if (!fnd) 
			{
				if (kind==(-1)) 
					put_it("No TCL command bindings.");
				else 
					put_it("No bindings for %s.",argv[1]);
			}
		}	
	}
	return 0;
}


void tcl_init(void)
{
static int done = 0;
	
	if (tcl_interp != NULL && !done)
	{
		done++;
#ifdef TCL_PLUS
		Tcl_InitStandAlone(tcl_interp);
#else
		Tcl_Init(tcl_interp);
#endif
		init_hash();
		init_builtins();

		init_public_tcl(tcl_interp);
		add_tcl_fset(tcl_interp);
	        Tcl_SetVar(tcl_interp,"botnick",nickname,TCL_GLOBAL_ONLY);
               	Tcl_SetVar(tcl_interp,"nick",nickname,TCL_GLOBAL_ONLY);
	} 
	if (tcl_interp)
		init_public_var(tcl_interp);
}

static int tcl_echo = 1;


BUILT_IN_COMMAND(tcl_version)
{
	put_it("%s", convert_output_format("BitchX Tcl Interpreter %GVersion $1%n (c)1997 Colten Edwards", "%s", tcl_versionstr));
}

BUILT_IN_COMMAND(tcl_command)
{
int result = 0;
	
	tcl_init();
	if (args && *args)
	{
		if (*args == '-' && *(args + 1))
		{
			char *bla;
			if (!my_strnicmp(args+1, "file", 4))
			{
				bla = next_arg(args, &args);
				if (get_string_var(LOAD_PATH_VAR))
					bla = path_search(args, get_string_var(LOAD_PATH_VAR));
				if ((result = Tcl_EvalFile(tcl_interp, bla ? bla : args)) != TCL_OK)
					put_it("Tcl:  [%s]",tcl_interp->result);
				else if (tcl_echo && *tcl_interp->result)
					put_it("Tcl:  [%s]", tcl_interp->result);
			} 
			else if (!my_strnicmp(args+1, "xecho", 4)) 
			{
				tcl_echo = (tcl_echo ? 0:1);
				put_it("Tcl:  xecho %s", tcl_echo?"on": "off");
			}	 
			else if (!my_strnicmp(args+1, "version", 4)) 
				tcl_version(NULL, NULL, NULL, NULL);
			else
				put_it("Tcl: unknown cmd [%s]", args);
			return;					
		}
		if ((result = Tcl_Eval(tcl_interp, args)) != TCL_OK)
			put_it("Tcl: %s %s", args, tcl_interp->result);
		else if (tcl_echo && *tcl_interp->result)
			put_it("Tcl: [%s] %s", args, tcl_interp->result);
	} 
	else
	{
		userage("tcl", "<-version | -file filename> <command>");
		tcl_version(NULL, NULL, NULL, NULL);
	}
}

int Tcl_Invoke(Tcl_Interp *irp, char *cmdname, char *rest)
{
int result = TCL_ERROR, argc = 0;
char **ArgList;
char *TrueArgs[41] = {NULL};
Tcl_CmdInfo info = {0};
char *p;
char *copy;
char *com;
	copy = LOCAL_COPY(rest);
	com = LOCAL_COPY(cmdname);
	
	ArgList = TrueArgs;
	ArgList[argc] = com;
	for (argc = 1; argc < 39; argc++) 
	{
		p = new_next_arg(copy, &copy);
		if (!p || !*p)
			break;
		ArgList[argc] = p;
	}
	if (copy && *copy)
		ArgList[argc] = copy;
	Tcl_ResetResult(irp);
	lower(com);
	if (internal_debug & DEBUG_TCL)
		debugyell("Invoking tcl [%s] with [%s]", com, rest);
	if (Tcl_GetCommandInfo(irp, com, &info) && info.proc)
	{
		result = (*info.proc)(info.clientData, irp, argc, ArgList);
		if (internal_debug & DEBUG_TCL)
			debugyell("Tcl returning with [%d]", result);
	}
	else
		Tcl_AppendResult(irp, "Unknown command \"", com, "\"", NULL);
	return result;
}

int tcl_cset STDVAR
{
extern char *set_cset(char *, ChannelList *, char *);
ChannelList *chan = NULL;
char *s = NULL;
	BADARGS(2, 4, " channel variable [value]");
	if (!(chan = lookup_channel(argv[1], from_server, 0)))
	{
		Tcl_AppendResult(irp, "No such channel \"", argv[1], "\"", NULL);
		return TCL_ERROR;
	}
	if (argv[2]) 
	{
		if (!(s = get_cset(argv[2], chan, NULL)))
		{
			Tcl_AppendResult(irp, "No such channel variable \"", argv[2], "\"", NULL);
			return TCL_ERROR;
		}
	}
	switch(argc)
	{
		case 2: /* no variable, do em all*/
			break;
		case 3: /* variable name only */
			break;
		case 4: /* set variable for channel */
			if (set_cset(argv[2], chan, argv[3]))
				malloc_strcpy(&s, argv[3]);
			else
			{
				Tcl_AppendResult(irp, "Error setting variable \"", argv[2], "\"", NULL);
				new_free(&s);
				return TCL_ERROR;
			}
			break;
	}
	if (s)
		Tcl_AppendResult(irp, s, NULL);
	new_free(&s);
	return TCL_OK;
}


#endif

#ifdef WANT_TCLMODULE
int Tcl_Init()
{

}
#endif
