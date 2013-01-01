/* CDCC v2 1.00 (c) Copyright 1996 William Glozer  */
/* ----------------------------------------------- */
/* Last revision 04.19.96 by Ananda                */

/* New CDCC for BitchX and any other clients that deserve to run it :) */
/* Note that I did use a lot of code/ideas from a copy of CDCC written */
/* for BitchX by panasync, so thanks to him for alot of the code and   */
/* ideas :)  I would appreciate any bugs reported to me as soon as is  */
/* possible, and cdcc.c + cdcc.h for any mods you do... if you modify  */
/* it for your client, I can add those mods as #ifdefs so the next ver */
/* will work with your client and have all the new stuff.  -Ananda '96 */
/* Modifed even more by panasync (edwards@bitchx.dimension6.com) to    */
/* interface cleanly and nicely with BitchX. Blame all bugs on me      */
/* instead of Ananda                                                   */

#define CDCC_FLUD

#include "irc.h"
static char cvsrevision[] = "$Id: cdcc.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(cdcc_c)
#include "ircaux.h"
#include "struct.h"
#include "commands.h"
#include "ignore.h"
#include "ctcp.h"
#include "hook.h"
#include "dcc.h"
#include "flood.h"
#include "screen.h"
#include "parse.h"
#include "output.h"
#include "input.h"
#include "server.h"
#include "vars.h"
#include "list.h"
#include "userlist.h"
#include "misc.h"
#include "who.h"

#include "cdcc.h"
#include "misc.h"

#define MAIN_SOURCE
#include "modval.h"

#ifdef WANT_CDCC

/* external ircII stuff */
static int l_timer (char *, char *);
static int r_info (char *, char *);
static int r_rmsend (char *, char *);
static int r_queue (char *, char *);
static int l_tsend (char *, char *);
static int l_tresend (char *, char *);
static int l_resume (char *, char *);
static int l_describe (char *, char *);
static int l_help(char *, char *);

/* add a pack to the offer list */
static int l_offer(char *, char *);

/* send a pack to someone */
static int l_send(char *, char *);

/* re-send a pack to someone */
static int l_resend(char *, char *);

/* remove a pack or all packs from the offer list */
static int l_doffer(char *, char *);

/* localy list offered packs */
static int l_list(char *, char *);

/* notify the channel that packs are offered */
static int l_notice(char *, char *);

/* view your queue */
static int l_queue(char *, char *);

/* save all offered packs to cdcc.save */
static int l_save(char *, char *);

/* load packs from cdcc.save */
static int l_load(char *, char *);

static int l_minspeed(char *, char *);

static int l_secure(char *, char *);

/* remote CDCC commands */
/* -------------------- */

/* show help to a remote user */
static int r_help(char *, char *);

/* list packs offered to remote user */
static int r_list(char *, char *);

/* send pack to remote user */
static int r_send(char *, char *);

/* re-send pack to remote user */
static int r_rsend(char *, char *);

#if 0
/* send pack to remote user */
static int r_tsend(char *, char *);

/* re-send pack to remote user */
static int r_trsend(char *, char *);
#endif

/* add files */
static void add_files(char *, char *);
/* add description */
static void add_desc(char *, char *);
/* remove pack/all packs */
static void del_pack(char *, char *);
/* add/remove public channel */
static int l_channel(char *, char *);
/* add note to a pack */
static int l_note(char *, char *);
static int l_echo(char *, char *);
static int l_stats(char *, char *);
static int l_type(char *, char *);
/* add a person to the dcc queue */
int BX_add_to_queue(char *, char *, pack *);

void dcc_getfile_resume (char *, char *);

/* local commands */
local_cmd local[] = {
	{ "CHANNEL",	l_channel,	"public timer channel" },
	{ "DESCRIBE",	l_describe,	"change description of pack" },
	{ "DOFFER",	l_doffer,	"remove pack from the offer list" },
	{ "LIST",	l_list,		"list the packs you have offered" },
	{ "LOAD",	l_load,		"load packs saved to .cdcc.save or specified name" },
	{ "HELP",	l_help,		"cdcc help" },
	{ "MINSPEED",	l_minspeed,	"minspeed for cdcc ( #.##) [mintime in seconds]" },
	{ "NOTICE",	l_notice,	"notify the channel of offered packs" },
	{ "OFFER",	l_offer,	"add a pack to the offer list" },
	{ "PLIST",	l_plist,	"publicly list your offered packs" },
	{ "QUEUE",	l_queue,	"view entries in the send queue" },
	{ "SAVE",	l_save,		"save your offerlist to .cdcc.save or specified name" },
	{ "SEND",	l_send,		"send a pack to user" },
	{ "RESEND",	l_resend,	"re-send a pack to user" },
	{ "TSEND",	l_tsend,	"tdcc send a pack to user" },
	{ "TRESEND",	l_tresend,	"tdcc resend a pack to user" },
#ifdef MIRC_BROKEN_DCC_RESUME
	{ "RESUME",	l_resume,	"mirc resume" },
#endif
	{ "TIMER",	l_timer,	"public list timer in minutes" },
	{ "NOTE",	l_note,		"add note to pack number"},
	{ "TYPE",	l_type,		"toggle between public and notice" },
	{ "ECHO",	l_echo,		"toggle echo on/off" },
	{ "STATS",	l_stats,	"display cdcc statistics" },
	{ "SECURE",	l_secure,	"adds a password to a pack"},
	{ "ON",		NULL,		"cdcc offers on" },
	{ "OFF",	NULL,		"cdcc offers off" },
	{ empty_string,		NULL,		empty_string } 
};
#define NUM_LOCAL (sizeof(local) / sizeof(local_cmd))

/* remote commands */
remote_cmd remote[] = {
	{ "HELP",	r_help,		"help on CDCC commands" },
	{ "RESEND",	r_rsend,	"have CDCC resend pack #N"},
#ifdef MIRC_BROKEN_DCC_RESUME
	{ "RESUME",	r_rmsend,	"have CDCC resume pack #N"},
#endif
	{ "SEND",	r_send,		"have CDCC send pack #N" },
#if 0
	{ "TRESEND",	r_trsend,	"have CDCC tresend pack #N"},
	{ "TSEND",	r_tsend,	"have CDCC tsend pack #N" },
#endif
	{ "LIST",	r_list,		"list of offered packs" },
	{ "INFO",	r_info,		"info on pack #N" },
	{ "QUEUE",	r_queue,	"queue status for us" },
	{ empty_string,		NULL,		empty_string }, 
};
#define NUM_REMOTE (sizeof(remote) / sizeof(remote_cmd))

/* ahh yes, global variables :( well, interfacing with ircII is a pain in  */
/* the ass, and I couldn't figure out a better way... more than one of my  */
/* routines need these... the static will keep them from being used by any */
/* functions outside of cdcc.c                                             */

unsigned int cdcc_numpacks = 0;
unsigned int send_numpacks = 0;

pack *offerlist = NULL;
static pack *newpack;

static queue *queuelist = NULL;
static unsigned long total_size_of_packs = 0;
static int numqueue = 0;
static int ptimer = 0;
static int do_notice_list = 0;
static int do_cdcc_echo = 1;

double cdcc_minspeed = 0.0;
static char *public_channel = NULL;


extern double dcc_max_rate_out, dcc_bytes_out, dcc_max_rate_in, dcc_bytes_in;

#define cparse(s) convert_output_format(s, NULL, NULL)

/* parse a users CDCC command */
BUILT_IN_COMMAND(cdcc)
{
	int i;
	char *cmd, *rest;
	
	cmd = next_arg(args, &args);
	rest = next_arg(args, &args);
	if (!cmd) 
	{
		l_list(NULL, NULL);
		put_it("%s: CDCC is [\002%s\002]. Use \002/cdcc help\002 to get help with cdcc", cparse(get_string_var(CDCC_PROMPT_VAR)), on_off(get_int_var(CDCC_VAR)));
		return;
	}
	if (!my_stricmp(cmd, "ON") || !my_stricmp(cmd, "OFF"))	
	{
		set_int_var(CDCC_VAR, !my_stricmp(cmd, "ON") ? 1 : 0);
		put_it("%s: offers \002%s\002", cparse(get_string_var(CDCC_PROMPT_VAR)), cmd);
		return;
	}
	
	for (i = 0; *local[i].name; i++) 
	{
		if (!my_stricmp(local[i].name, cmd) && local[i].function) 
		{  
			local[i].function(rest, args);
			return;
		}
	}
	put_it("%s: unknown command \002%s\002", cparse(get_string_var(CDCC_PROMPT_VAR)), cmd);
	return;
}
			
static int l_help(char *cmd, char *args)
{
int i;
char buffer[BIG_BUFFER_SIZE+1];
	if (!cmd)
	{
		int c = 0;
		*buffer = 0;
		for (i = 0; *local[i].name; i++)
		{
			strmcat(buffer, local[i].name, BIG_BUFFER_SIZE);
			strmcat(buffer, space, BIG_BUFFER_SIZE);
			if (++c == 5)
			{
				put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
				*buffer = 0;
				c = 0;
			}
		}
		if (c)
			put_it("%s", convert_output_format("$G $[13]0 $[13]1 $[13]2 $[13]3 $[13]4", "%s", buffer));
		userage("CDCC help", "%R[%ncommand%R]%n to get help on specific commands");
	}	
	else
	{
		int done = 0;
		for (i = 0; *local[i].name; i++)
		{
			if (my_stricmp(local[i].name, cmd))
				continue;
			sprintf(buffer, "CDCC %s", cmd);
			userage(buffer, local[i].help?local[i].help:" - No help available");
			done++;
		}
		if (!done)
			put_it("%s", convert_output_format("$G CDCC - No such command", NULL, NULL));
	}
	return 0;
}

/* parse a remote message CDCC command */
char *msgcdcc(char *from, char *to, char *args)
{
	int i;
	char *secure = NULL;
	char *cdcc, *rest, *cmd, *temp = NULL;

	temp = LOCAL_COPY(args);	
	cdcc = next_arg(temp, &temp);
	if (!cdcc || (my_strnicmp(cdcc, "XDCC", 4) && my_strnicmp(cdcc, "CDCC", 4)))
	 	return args;
	if (!get_int_var(CDCC_VAR))
		return args;
	if ((check_ignore(from, FromUserHost, to, IGNORE_CDCC, NULL) == IGNORED))
		return args;
			
	if (!check_flooding(from, CDCC_FLOOD, args, NULL) || !offerlist)
		return NULL;


	if ((secure = get_string_var(CDCC_SECURITY_VAR)))
	{
		UserList *tmp;
		char *pass = NULL;
		if (*secure == '0' && strlen(secure) == 1)
			goto got_good_pass;
		if (temp && *temp)
			pass = strrchr(temp, ' ');
		if (pass && *pass)
		{ 
			pass++;
			if (*pass && !my_stricmp(pass, secure))
			{
				*pass-- = 0;
				goto got_good_pass;
			}
		}
#ifdef WANT_USERLIST
		if (!(tmp = lookup_userlevelc("*", FromUserHost, "*", NULL)) || !(tmp->flags & ADD_DCC))
			return args;
#else
		return args;
#endif
	}
got_good_pass:
	cmd = next_arg(temp, &temp);
	if (!cmd) 
		return args;

	rest = temp;

	for (i = 0; *remote[i].name; i++) 
	{
		if (!my_stricmp(cmd, remote[i].name)) 
		{
			remote[i].function(from, rest);
			return NULL;
		}
	}
	queue_send_to_server(from_server, "NOTICE %s :try /ctcp %s cdcc help",from, get_server_nickname(from_server));
	return args;
}

static int r_info(char *args, char *rest)
{
	if (rest && *rest)
	{
		char *q;
		pack *ptr = NULL;
		
		q = next_arg(rest, &rest);
		for (ptr = offerlist; ptr; ptr = ptr->next)
			if (matchmcommand(q, ptr->num))
				break;
		if (ptr)
			queue_send_to_server(from_server, "NOTICE %s :%d file%s %d gets %ld size %2.4f minspeed %ld time added",args, ptr->numfiles, plural(ptr->numfiles), ptr->gets, ptr->size, ptr->minspeed, ptr->timeadded); 
		else
			queue_send_to_server(from_server, "NOTICE %s :Invalid info request", args);
	}
	return 0;
}
 
static int r_queue(char *args, char *rest)
{
queue *new = NULL;
int count;
int num = 0;
char buffer[BIG_BUFFER_SIZE+1];
	if (queuelist && args)
	{
		*buffer = 0;
		for (new = queuelist, count = 1; new; new = new->next)
		{
			if (!my_stricmp(new->nick, args))
			{
				num++;
				strmopencat(buffer, BIG_BUFFER_SIZE, ltoa(count), ",", NULL);
				count++;
			}
		}
		if (num)
		{
			chop(buffer, 1);
			queue_send_to_server(from_server, "NOTICE %s :You have %d packs queued at %s", args, num, buffer);
		}
		else
			queue_send_to_server(from_server, "NOTICE %s :You have no packs queued.", args);
	}
	return 0;
}

static int do_local_send(char *command, char *args, char *rest)
{
	pack *ptr = NULL;
	char *temp = NULL, *file = NULL, *dccinfo = NULL, *q = NULL, *p;
	int maxdcc, maxqueue;
	int tdcc = 0;
	int queued_files =  0;
	int count = 0;
				
	if (*command == 'T')
		tdcc = 1;	
	if (!args || !*args)
		return 0;

	maxdcc = get_int_var(DCC_SEND_LIMIT_VAR);
	maxqueue = get_int_var(DCC_QUEUE_LIMIT_VAR);
		
	while (1)
	{
		if (!(temp = next_arg(rest, &rest)))
			break;

		if (isdigit((unsigned char)*temp) || (*(temp+1) && isdigit((unsigned char)*(temp+1))))
		{
			if (*temp == '#')
				temp++;
			for (ptr = offerlist; ptr; ptr = ptr->next)
				if (matchmcommand(temp, ptr->num))
					break;
		}		
		if (ptr)
		{
			if (maxdcc && get_active_count() >= maxdcc) 
			{
				if (maxqueue && (numqueue >= maxqueue)) 
				{
					put_it("%s: all dcc and queue slots full", cparse(get_string_var(CDCC_PROMPT_VAR)));
					if (queued_files)
						put_it("%s: Queued %d files", cparse(get_string_var(CDCC_PROMPT_VAR)), queued_files );
					return count + queued_files;
				}
				queued_files += add_to_queue(args, command, ptr);
				do_hook(CDCC_SEND_NICK_LIST, "%s %s %s %d %d %d %s %s", args, "unknown", command, ptr->num, ptr->numfiles, ptr->gets, ptr->file, ptr->desc);

				continue;
			}
	
			put_it("%s: %s %s%s%s pack #\002%d\002 (\002%d\002 file%s)", cparse(get_string_var(CDCC_PROMPT_VAR)),
				!my_stricmp(command, "SEND")||!my_stricmp(command,"TSEND")?"sending":"resending",
				UND_TOG_STR, args, UND_TOG_STR, ptr->num, ptr->numfiles,
				plural(ptr->numfiles));
			malloc_strcpy(&file, ptr->file);
			q = file;

			for (p = new_next_arg(file, &file); p && *p; p = new_next_arg(file, &file)) 
			{
				malloc_sprintf(&dccinfo, "%s \"%s\"", args, p);
				if (!my_stricmp(command, "SEND") || !my_stricmp(command, "TSEND"))
					dcc_filesend(command, dccinfo);
				else
					dcc_resend(command, dccinfo);
			}
			send_numpacks++;
			count++;
			ptr->gets++;
			do_hook(CDCC_SEND_NICK_LIST, "%s %s %s %d %d %d %s %s", args, "unknown", command, ptr->num, ptr->numfiles, ptr->gets, ptr->file, ptr->desc);
		}
		else
		{
			if (offerlist && (isdigit((unsigned char)*temp) || (*(temp+1) && (isdigit((unsigned char)*(temp+1))))))
				put_it("%s: No such pack number", cparse(get_string_var(CDCC_PROMPT_VAR)));
			else
			{
				malloc_sprintf(&dccinfo, "%s \"%s\"", args, temp);
				if (!my_stricmp(command, "SEND") || !my_stricmp(command, "TSEND"))
					dcc_filesend(command, dccinfo);
				else
					dcc_resend(command, dccinfo);
				count++;
			}
		}  
		new_free(&q);
		file = NULL;
	}
	if (queued_files)
		put_it("%s: Queued %d files", cparse(get_string_var(CDCC_PROMPT_VAR)), queued_files);
	new_free(&dccinfo);
	new_free(&q);
	return count + queued_files;
}
 
static int l_send(char *args, char *rest)
{
	do_local_send("SEND", args, rest);
	return 0;
}

static int l_resend(char *args, char *rest)
{
	do_local_send("RESEND", args, rest);
	return 0;
}

#ifdef MIRC_BROKEN_DCC_RESUME
static int l_resume(char *args, char *rest)
{
	do_local_send("RESUME", args, rest);
	return 0;
}
#endif

static int l_tsend(char *args, char *rest)
{
	do_local_send("TSEND", args, rest);
	return 0;
}

static int l_tresend(char *args, char *rest)
{
	do_local_send("TRESEND", args, rest);
	return 0;
}


/*resends a pack to the requestee*/
/*Added by Wicked Angel: wangel@wgrobez1.remote.louisville.edu*/
/*It wasn't hard ... but hey ... it seemed like a good idea :>*/
/* routine modified highly by Colten Edwards. */

static int do_dcc_sends(char *command, char *from, char *args)
{
	pack *ptr;
	char *temp = NULL, *file = NULL, *dccinfo = NULL, *q = NULL, *p;
	char *password = NULL;
	int maxdcc, maxqueue;
	int count = 0;
	int queued_files = 0;
		
	maxdcc = get_int_var(DCC_SEND_LIMIT_VAR);
	maxqueue = get_int_var(DCC_QUEUE_LIMIT_VAR);

	while (1)
	{
		if (!(temp = next_arg(args, &args)))
			break;
		if (args && *args && (!my_isdigit(args)))
			password = next_arg(args, &args);
		
		for (ptr = offerlist; ptr; ptr = ptr->next)
			if (matchmcommand(temp, ptr->num))
				break;
		if (ptr)
		{
			if (ptr->password && (!password || (password && strcmp(ptr->password, password))))
			{
				put_it("%s: Attempted get of secure pack %d from %s failed. [%s]", cparse(get_string_var(CDCC_PROMPT_VAR)), ptr->num, from, !password? "No Password": "Invalid Password");
				queue_send_to_server(from_server, "NOTICE %s :\002CDCC\002: Failed attempt to get secure pack %d", from, ptr->num);
				continue;
			}
			if (maxdcc && ((maxdcc - get_active_count()) < ptr->numfiles || get_active_count() >= maxdcc)) 
			{
				if (maxqueue && (numqueue >= maxqueue)) 
				{
					if (queued_files)
					{
						put_it("%s: Queued %d files for %s", cparse(get_string_var(CDCC_PROMPT_VAR)), queued_files, from);
						queue_send_to_server(from_server, "NOTICE %s :\002CDCC\002: all slots full... Some requests ignored. Added to queue for %d requests", from, queued_files);
					}
					else
						queue_send_to_server(from_server, "NOTICE %s :\002CDCC\002: all dcc and queue slots full... Try again later", from);

					return 0;
				}
				queued_files += add_to_queue(from, command, ptr);
				count++;
				do_hook(CDCC_SEND_NICK_LIST, "%s %s %s %d %d %d %s %s", from, FromUserHost, command, ptr->num, ptr->numfiles, ptr->gets, ptr->file, ptr->desc);
				continue;
			}
	
			put_it("%s: %s %s%s%s pack #\002%d\002 (\002%d\002 file%s)", cparse(get_string_var(CDCC_PROMPT_VAR)),
				!my_stricmp(command, "SEND")||!my_stricmp(command, "TSEND")?"sending":!my_stricmp(command,"RESUME")?"resuming":"resending", UND_TOG_STR, from, UND_TOG_STR, 
				ptr->num, ptr->numfiles, plural(ptr->numfiles));
			malloc_strcpy(&file, ptr->file);
			q = file;

			for (p = next_arg(file, &file); p && *p; p = next_arg(file, &file)) 
			{
				malloc_sprintf(&dccinfo, "%s %s", from, p);
				if (!my_stricmp(command, "RESEND") || !my_stricmp(command, "TRESEND"))
					dcc_resend(command, dccinfo);
#ifdef MIRC_BROKEN_DCC_RESUME
				else if (!my_stricmp(command, "RESUME"))
					dcc_resume(command, dccinfo);
#endif
				else
					dcc_filesend(command, dccinfo);
			}
			send_numpacks++;
			ptr->gets++;
			do_hook(CDCC_SEND_NICK_LIST, "%s %s %s %d %d %d %s %s", from, FromUserHost, command, ptr->num, ptr->numfiles, ptr->gets, ptr->file, ptr->desc);
		}
		else if (!count)
			queue_send_to_server(from_server, "NOTICE %s :\002CDCC\002: invalid pack number", from);
		count++;
		new_free(&q);
		file = NULL;
	}
	if (queued_files)
	{
		put_it("%s: Queued %d files for %s", cparse(get_string_var(CDCC_PROMPT_VAR)), queued_files, from);
		queue_send_to_server(from_server, "NOTICE %s :\002CDCC\002: all slots full... Added to the queue for %d requests", from, queued_files);
	}
	new_free(&dccinfo);
	new_free(&q);
	return 0;
}


static int r_rsend(char *from, char *args)
{
	do_dcc_sends("RESEND", from, args);
	return 0;
}

#ifdef MIRC_BROKEN_DCC_RESUME
static int r_rmsend(char *from, char *args)
{
	do_dcc_sends("RESUME", from, args);
	return 0;
}
#endif

#if 0
static int r_trsend(char *from, char *args)
{
	do_dcc_sends("TRESEND", from, args);
	return 0;
}

static int r_tsend(char *from, char *args)
{
	do_dcc_sends("TSEND", from, args);
	return 0;
}
#endif

/* senD a pack to the remote user */
static int r_send(char *from, char *args)
{
	do_dcc_sends("SEND", from, args);
	return 0;
}
			

/* remote pack list */
static int r_list(char *from, char *args)
{
	pack *ptr;
	char size[30];
	char mrate_out[30];
	char mrate_in[30];
	char bytes_out[30];
	char bytes_in[30];
	char speed_out[30];
	char *type_msg;
	int once = 0;

	sprintf(mrate_out, "%1.3g", dcc_max_rate_out);
	sprintf(mrate_in, "%1.3g", dcc_max_rate_in);
	sprintf(bytes_out, "%1.3g", dcc_bytes_out);
	sprintf(bytes_in, "%1.3g", dcc_bytes_in);
	sprintf(speed_out, "%1.3g", cdcc_minspeed);

	type_msg = (do_notice_list)? "NOTICE":"PRIVMSG";

	for (ptr = offerlist; ptr; ptr = ptr->next) 
	{
		if (!once && do_hook(CDCC_PREPACK_LIST, "%s %s %s %d %d %d %d %d %s %s %s %s %lu %s", 
			"NOTICE", from, get_server_nickname(from_server), 
			cdcc_numpacks, 
			get_int_var(DCC_SEND_LIMIT_VAR)-get_active_count(), 
			get_int_var(DCC_SEND_LIMIT_VAR), numqueue, 
			get_int_var(DCC_QUEUE_LIMIT_VAR), 
			mrate_out, bytes_out, mrate_in, bytes_in, 
			total_size_of_packs, speed_out))
		{
			queue_send_to_server(from_server,"NOTICE %s :Files Offered: /ctcp %s CDCC send #N for pack N", from, get_server_nickname(from_server));
			if (get_int_var(DCC_SEND_LIMIT_VAR))
				queue_send_to_server(from_server, "NOTICE %s :    [%d pack%s %d/%d slots open]", from, 
					cdcc_numpacks, plural(cdcc_numpacks), get_int_var(DCC_SEND_LIMIT_VAR)-get_active_count(), get_int_var(DCC_SEND_LIMIT_VAR));
			else
				queue_send_to_server(from_server, "NOTICE %s :    [%d pack%s]", from, cdcc_numpacks,plural(cdcc_numpacks));
		}
	 	if (ptr->size / 1024 > 999)
			sprintf(size, "\002%4.1f\002mb",
				(((double)ptr->size) / 1024) / 1024);
		else
			sprintf(size, "\002%4.1f\002kb", (((double)ptr->size) / 1024)); 
				
		if (do_hook(CDCC_PACK_LIST, "%s %s %d %d %lu %d %s", 
			"NOTICE", from, ptr->num, ptr->numfiles, ptr->size, ptr->gets, ptr->desc))
		{
			queue_send_to_server(from_server, "NOTICE %s :#%d \037(\037%10s\037:\037\002%4d\002 get%s\037)\037 %s",
				from, ptr->num, size, ptr->gets, plural(ptr->gets), 
				ptr->desc ? ptr->desc : "no description");
		}
		if (ptr->notes && do_hook(CDCC_NOTE_LIST, "%s %s %s", "NOTICE", from, ptr->notes))
			queue_send_to_server(from_server, "NOTICE %s :\t%s", from, ptr->notes);
		once++;
	}
	if (once)
		do_hook(CDCC_POSTPACK_LIST, "%s %s %s %d %d %d %d %d %s %s %s %s %lu %s", 
			"NOTICE", from, get_server_nickname(from_server), 
			cdcc_numpacks, get_int_var(DCC_SEND_LIMIT_VAR)-get_active_count(), 
			get_int_var(DCC_SEND_LIMIT_VAR), numqueue, get_int_var(DCC_QUEUE_LIMIT_VAR), 
			mrate_out, bytes_out, mrate_in, bytes_in, total_size_of_packs, speed_out);
	return 0;
}		

/* remote help display  */
static int r_help(char *from, char *args)
{
	int i;
#ifdef CDCC_FLUD
	if (args && *args)
	{
		char *q;
		q = next_arg(args, &args);
		for (i = 0; *remote[i].name; i++) 
		{
			if (!my_stricmp(remote[i].name, q))
			{
				queue_send_to_server(from_server, "NOTICE %s :\002CDCC\002: %-6s - %s",
					from, remote[i].name, remote[i].help);
				break;
			}
		}
	} 
	else
	{
		char buffer[BIG_BUFFER_SIZE+1];
		char *q = buffer;
		for (i = 0; *remote[i].name; i++) 
		{
			snprintf(q, BIG_BUFFER_SIZE, "%s ", remote[i].name);
			q = &buffer[strlen(buffer)];
		}
		queue_send_to_server(from_server, "NOTICE %s :\002CDCC\002: %s", from, buffer);
	}
#endif
	return 0;
}


/* remove a pack or all packs from the offerlist */
static int l_doffer(char *args, char *rest)
{
	if (!cdcc_numpacks) {
		put_it("%s: you have no packs offered", cparse(get_string_var(CDCC_PROMPT_VAR)));
		return 0;
	}
	if (args && *args)
	{
		char * temp = NULL;
		malloc_sprintf(&temp, "%s %s", args, rest ? rest : empty_string);
		del_pack(NULL, temp);
		new_free(&temp);
		l_list(NULL, NULL);
	}
	else 
	{
		l_list(NULL, NULL);
		add_wait_prompt("Remove pack [* for all packs]: ", del_pack, empty_string, WAIT_PROMPT_LINE, 1);
	}
	return 0;
}

/* localy list the packs you have offered */
static int l_list(char *args, char *rest)
{
	pack *ptr;
	char temp[30];
	
	if (!cdcc_numpacks) 
	{
		put_it("%s: you have no packs offered", cparse(get_string_var(CDCC_PROMPT_VAR)));
		return 0;
	}
	if (args)
	{
		char *num = next_arg(args, &args);
		int it;

		it = my_atol(num);
		if (it <= 0)
			return 0;
		for (ptr = offerlist; ptr; ptr= ptr->next)
		{
			if (it == ptr->num)
			{
				char *temp = NULL;
				char *p, *q;
				int i = 1;
				malloc_strcpy(&temp, ptr->file);
				q = temp;				
				while (temp && *temp)
				{
					p = next_arg(temp, &temp);
					put_it("#%-2d %-2d  %-2d   %s",
						ptr->num, ptr->gets, i++, p);
				}
				
				new_free(&q);
				break;
			}
		}
	}
	else 
	{
		
		put_it("%s", convert_output_format("#   files    size     gets minspeed  description", NULL, NULL));
		for (ptr = offerlist; ptr; ptr = ptr->next) 
		{
			sprintf(temp, "%4.1f", _GMKv(ptr->size));
/* buggy SUNOS doesn't like this next line at all. Why?
			sprintf(temp2, "%4.1f", (double)(ptr->minspeed));*/
			put_it("%-2d    \002%3d\002  %6s%-4s \002%4d\002     0.0  %s",
				ptr->num, ptr->numfiles,
				temp, _GMKs(ptr->size), /*temp2*/ptr->gets, ptr->desc);
		}
	}
	return 0;
}

/* add a pack to the offer list */
static int l_offer(char *args, char *rest)
{
char *tmp = NULL;
	if (args && *args)
	{
		tmp = m_sprintf("%s %s", args, rest?rest:empty_string);
		add_files(NULL, tmp);
	}
	else
	{
		malloc_sprintf(&tmp, "Add file(s) to pack%s #%d : ", plural(cdcc_numpacks), cdcc_numpacks+1);
		add_wait_prompt(tmp, add_files, empty_string, WAIT_PROMPT_LINE, 1);
	}
	new_free(&tmp);
	return 0;
}

/* display the offerlist to current channel */
int l_plist(char *args, char *rest)
{
	pack *ptr;
	char *chan = NULL, *string = NULL;
	char size[20];
	char mrate_out[30];
	char mrate_in[30];
	char bytes_out[30];
	char bytes_in[30];
	char speed_out[30];
	char *type_msg;
	int maxdccs, blocksize, maxqueue;
	
	if (!get_current_channel_by_refnum(0) || !cdcc_numpacks || (args && *args && !is_channel(args))) {
		put_it("%s: you %s",cparse(get_string_var(CDCC_PROMPT_VAR)),
			cdcc_numpacks ? "are not on a channel!" :
				   "have no packs offered!");
		return 0;
	}
	type_msg = (do_notice_list)? "NOTICE":"PRIVMSG";
	
	if (args && *args)
		chan = LOCAL_COPY(args);
	else	
		chan = LOCAL_COPY(get_current_channel_by_refnum(0));

	maxdccs = get_int_var(DCC_SEND_LIMIT_VAR);
	blocksize = get_int_var(DCC_BLOCK_SIZE_VAR);
	maxqueue = get_int_var(DCC_QUEUE_LIMIT_VAR);
	set_display_target(chan, LOG_CRAP);
	sprintf(mrate_out, "%1.3g", dcc_max_rate_out);
	sprintf(mrate_in, "%1.3g", dcc_max_rate_in);
	sprintf(bytes_out, "%1.3g", dcc_bytes_out);
	sprintf(bytes_in, "%1.3g", dcc_bytes_in);
	sprintf(speed_out, "%1.3g", cdcc_minspeed);
	if (do_hook(CDCC_PREPACK_LIST, "%s %s %s %d %d %d %d %d %s %s %s %s %lu %s", type_msg, chan, get_server_nickname(from_server), cdcc_numpacks, get_int_var(DCC_SEND_LIMIT_VAR)-get_active_count(), get_int_var(DCC_SEND_LIMIT_VAR), numqueue, get_int_var(DCC_QUEUE_LIMIT_VAR), mrate_out, bytes_out, mrate_in, bytes_in, total_size_of_packs, speed_out))
	{
		if (get_int_var(QUEUE_SENDS_VAR))
		{
			malloc_sprintf(&string, "\037[\037cdcc\037]\037 \002%d\002 file%s offered\037-\037 /ctcp \002%s\002 cdcc send #x for pack #x",
				cdcc_numpacks, plural(cdcc_numpacks), get_server_nickname(from_server));
			queue_send_to_server(from_server, "%s %s :%s", 
				do_notice_list?"NOTICE":"PRIVMSG", chan, string);
			malloc_sprintf(&string, "\037[\037cdcc\037]\037 dcc block size\037:\037 \002%d\002, slots open\037:\037 \002%2d\002/\002%2d\002, dcc queue\037:\037 \002%2d\002/\002%2d\002",
				(blocksize) ? blocksize : 1024, maxdccs - get_active_count(),
				maxdccs, maxqueue - numqueue, maxqueue);
			queue_send_to_server(from_server, "%s %s :%s", 
				do_notice_list?"NOTICE":"PRIVMSG", chan, string);
		}
		else
		{
			malloc_sprintf(&string, "\037[\037cdcc\037]\037 \002%d\002 file%s offered\037-\037 /ctcp \002%s\002 cdcc send #x for pack #x",
				cdcc_numpacks, plural(cdcc_numpacks), get_server_nickname(from_server));
			send_text(chan, string, do_notice_list?"NOTICE":NULL, do_cdcc_echo, 0);
			malloc_sprintf(&string, "\037[\037cdcc\037]\037 dcc block size\037:\037 \002%d\002, slots open\037:\037 \002%2d\002/\002%2d\002, dcc queue\037:\037 \002%2d\002/\002%2d\002",
				(blocksize) ? blocksize : 1024, maxdccs - get_active_count(),
				maxdccs, maxqueue - numqueue, maxqueue);
			send_text(chan, string, do_notice_list?"NOTICE":NULL, do_cdcc_echo, 0);
		}
	}
	for (ptr = offerlist; ptr; ptr = ptr->next) 
	{
	 	if (ptr->size / 1024 > 999)
			sprintf(size, "\002%3.2f\002mb",
				(float) (ptr->size / 1024) / 1024);
		else
			sprintf(size, "\002%3.2f\002kb", (float) ptr->size / 1024); 
				
		if (do_hook(CDCC_PACK_LIST, "%s %s %d %d %lu %d %s", 
			type_msg, chan, ptr->num, ptr->numfiles, ptr->size, ptr->gets, ptr->desc))
		{
			if (get_int_var(QUEUE_SENDS_VAR))
			{
				malloc_sprintf(&string, "\037%%\037 #%-2d \037(\037%10s\037:\037\002%4d\002 get%s\037)\037 %s",
					ptr->num, size, ptr->gets, plural(ptr->gets), 
					ptr->desc ? ptr->desc : "no description");
				queue_send_to_server(from_server, "%s %s :%s", 
					do_notice_list?"NOTICE":"PRIVMSG", chan, string);
			}
			else
			{
				malloc_sprintf(&string, "\037%%\037 #%-2d \037(\037%10s\037:\037\002%4d\002 get%s\037)\037 %s",
					ptr->num, size, ptr->gets, plural(ptr->gets), 
					ptr->desc ? ptr->desc : "no description");
				send_text(chan, string, do_notice_list?"NOTICE":NULL, do_cdcc_echo, 0);
			}
		}
		if (ptr->notes && do_hook(CDCC_NOTE_LIST, "%s %s %s", type_msg, chan, ptr->notes))
		{
			malloc_sprintf(&string, "\t%s", ptr->notes);
			if (get_int_var(QUEUE_SENDS_VAR))
				queue_send_to_server(from_server, "%s %s :%s", 
					do_notice_list?"NOTICE":"PRIVMSG", chan, string);
			else
				send_text(chan, string, do_notice_list?"NOTICE":NULL, do_cdcc_echo, 0);
		}
	}
	do_hook(CDCC_POSTPACK_LIST, "%s %s %s %d %d %d %d %d %s %s %s %s %lu %s", 
		type_msg, chan, get_server_nickname(from_server), cdcc_numpacks, 
		get_int_var(DCC_SEND_LIMIT_VAR)-get_active_count(), 
		get_int_var(DCC_SEND_LIMIT_VAR), numqueue, 
		get_int_var(DCC_QUEUE_LIMIT_VAR), mrate_out, bytes_out, 
		mrate_in, bytes_in, total_size_of_packs, speed_out);
	reset_display_target();
	return 0;
}
		
/* notify the current channel that packs are offered */
static int l_notice(char *args, char *rest)
{
	char *string = NULL, *chan = NULL;
	char mrate_out[30];
	char mrate_in[30];
	char bytes_out[30];
	char bytes_in[30];
	char speed_out[30];	
	
	if (!get_current_channel_by_refnum(0) || !cdcc_numpacks || (args && *args && !is_channel(args))) {
		put_it("%s: you %s",cparse(get_string_var(CDCC_PROMPT_VAR)),
			cdcc_numpacks ? "are not on a channel!" :
				   "have no packs offered!");
		return 0;
	}
	
	if (args && *args)
		malloc_strcpy(&chan, args);
	else
		malloc_strcpy(&chan, get_current_channel_by_refnum(0));

	set_display_target(chan, LOG_CRAP);	
	sprintf(mrate_out, "%1.3g", dcc_max_rate_out);
	sprintf(mrate_in, "%1.3g", dcc_max_rate_in);
	sprintf(bytes_out, "%1.3g", dcc_bytes_out);
	sprintf(bytes_in, "%1.3g", dcc_bytes_in);
	sprintf(speed_out, "%1.3g", cdcc_minspeed);
	if (do_hook(CDCC_PREPACK_LIST, "%s %s %s %d %d %d %d %d %s %s %s %s %lu %s", "NOTICE", chan, get_server_nickname(from_server), cdcc_numpacks, get_int_var(DCC_SEND_LIMIT_VAR)-get_active_count(), get_int_var(DCC_SEND_LIMIT_VAR), numqueue, get_int_var(DCC_QUEUE_LIMIT_VAR), mrate_out, bytes_out, mrate_in, bytes_in, total_size_of_packs, speed_out))
	{
		malloc_sprintf(&string, "\037[\037cdcc\037]\037 \002%d\002 file%s offered\037-\037 \037\"\037/ctcp \002%s\002 cdcc list\037\"\037 for pack list",   
			cdcc_numpacks, plural(cdcc_numpacks), get_server_nickname(from_server));
		if (get_int_var(QUEUE_SENDS_VAR))
			queue_send_to_server(from_server, "NOTICE %s :%s", chan, string);
		else
			send_text(chan, string, "NOTICE", do_cdcc_echo, 0);
	
	}

	do_hook(CDCC_POSTPACK_LIST, "%s %s %s %d %d %d %d %d %s %s %s %s %lu %s", 
		"NOTICE", chan, get_server_nickname(from_server), cdcc_numpacks, 
		get_int_var(DCC_SEND_LIMIT_VAR)-get_active_count(), 
		get_int_var(DCC_SEND_LIMIT_VAR), numqueue, 
		get_int_var(DCC_QUEUE_LIMIT_VAR), mrate_out, bytes_out, mrate_in, 
		bytes_in, total_size_of_packs, speed_out);
	reset_display_target();
	new_free(&chan);
	new_free(&string);
	return 0;		
}
		
/* display entries in your send queue */
static int l_queue(char *args, char *rest)
{
	queue *ptr, *del, *prev = NULL;
	int num = 1;
	char *command = NULL;	
	int count = 0;

	if (!queuelist) 
	{
		put_it("%s: there are no queue entries",cparse(get_string_var(CDCC_PROMPT_VAR)));
		return 0;
	}

	if (args && *args)
		command = next_arg(args, &args);
	if ((command == NULL) || !my_stricmp(command, "LIST"))
	{
		for (ptr = queuelist; ptr; ptr = ptr->next, num++) 
		{
			if (command && rest && *rest && !wild_match(rest, ptr->nick))
				continue;
			if (command)
			{
				if (do_hook(CDCC_QUEUE_LIST, "%s %s %d %d %s", ptr->nick, my_ctime(ptr->time), ptr->num, ptr->numfiles, ptr->desc))
					put_it("#\002%2d\002 nick: \037%9s\037 (\002%d\002 file%s)",
						num, ptr->nick, ptr->numfiles,
						plural(ptr->numfiles));
			} else
				count++;
		}
		if (count && !command)
			put_it("%s: %d entries in the queue", cparse(get_string_var(CDCC_PROMPT_VAR)), count);
		return 0;
	} 
	else if (!my_stricmp(command, "REMOVE"))
	{	
		char *n;
		int success = 0;
		n = rest;
		if (!n || !*n)
			return 0;
		for (ptr = queuelist; ptr;)
		{
			del = ptr;
			ptr = ptr->next;
			count++;
			if (matchmcommand(n, count) || wild_match(n, del->nick))
			{
				if (prev) 
					prev->next=del->next;
				else 
					queuelist=del->next;
				new_free(&del->file);
				new_free(&del->nick);
				new_free(&del->desc);
				new_free((char **)&del);                                        
				success++;
			} else
				prev = del;
		}
		put_it("%s: deleted %d of %d entries from the queue", cparse(get_string_var(CDCC_PROMPT_VAR)), success, count);
		
	}
	else
		put_it("%s: /Cdcc queue remove #|nick /Cdcc queue list [nick]", cparse(get_string_var(CDCC_PROMPT_VAR)));
	return 0;
} 

/* save all of your offered packs */
static int l_save(char *args, char *rest)
{
#ifdef PUBLIC_ACCESS
	bitchsay("This command has been disabled on a public access system");
	return;
#else
	FILE *file;
	char *name = NULL, *expand = NULL;
	pack *ptr;
	int count = 0;
	char *fn;
		
	if (!offerlist) 
	{
		put_it("%s: you have no packs offered", cparse(get_string_var(CDCC_PROMPT_VAR)));
		return 0;
	}
	if (args)
		fn = args;
	else
#if defined(WINNT) || defined(__EMX__)
		fn = "cdcc.save";
#else
		fn = ".cdcc.save";
#endif		
	malloc_sprintf(&expand, "~/%s", fn);
	name = expand_twiddle(expand);
	new_free(&expand);

#if defined(WINNT) || defined(__EMX__)
	if (!name || !(file = fopen(name, "wt"))) 
#else
	if (!name || !(file = fopen(name, "w"))) 
#endif
	{
		put_it("%s: couldn't open \"%s\"", cparse(get_string_var(CDCC_PROMPT_VAR)),name?name:empty_string);
		new_free(&name);
		return 0;
	}
	fprintf(file, "#cdcc save file 1.0\n");	
	for (ptr = offerlist; ptr; ptr = ptr->next) 
	{
		fprintf(file, "%s\n", ptr->file);
		fprintf(file, "%s %d\n", ptr->desc, ptr->numfiles);
		fprintf(file, "%s\n", ptr->notes?ptr->notes:empty_string);
		fprintf(file, "%d %lu 0.00 %d %lu %s\n", ptr->gets, ptr->size, ptr->server, (unsigned long)ptr->timeadded, ptr->password?ptr->password:empty_string);
		count++;
	}
		
	fclose(file);
	put_it("%s: \002%d\002 pack%s saved to %s", cparse(get_string_var(CDCC_PROMPT_VAR)),
		count, plural(count), name);
	new_free(&name);

	return 0;
#endif
} 

/* load packs from cdcc.save */
static int l_load(char *args, char *rest)
{
	FILE *file;
	char *buffer = NULL, *expand = NULL, *temp;
	pack *ptr, *last = NULL;
	char *p, *q;
	int count = 0;
	int got_header = 0;
		
#if defined(WINNT) || defined(__EMX__)
	malloc_sprintf(&expand, "~/%s", args ? args: "cdcc.save");
#else
	malloc_sprintf(&expand, "~/%s", args ? args: ".cdcc.save");
#endif
	buffer = expand_twiddle(expand);
	new_free(&expand);

	if (!buffer || !(file = fopen(buffer, "rt"))) 
	{
		put_it("%s: couldn't open \"%s\"",  cparse(get_string_var(CDCC_PROMPT_VAR)), buffer ? buffer : expand);
		new_free(&buffer);
		return 0;
	}

	for (ptr = offerlist; ptr; ptr = ptr->next)
		last = ptr;
	
	new_free(&buffer);
	buffer = new_malloc(BIG_BUFFER_SIZE+1);
		
	while (fgets(buffer, BIG_BUFFER_SIZE, file)) {
		p = buffer;
		buffer[BIG_BUFFER_SIZE-1] = 0;
		chop(buffer, 1);
		if (!got_header)
		{
			if (my_strnicmp(p, "#cdcc save file 1.0", 16))
			{
				put_it("File is not a cdcc save file.");
				break;
			}
			got_header++;
			continue;
		}
		ptr = (pack *) new_malloc(sizeof(pack));
		ptr->num = ++cdcc_numpacks;
		malloc_strcpy(&ptr->file, buffer);

		fgets(buffer, BIG_BUFFER_SIZE, file);
		chop(buffer, 1);

		temp = strrchr(buffer, ' ');
		*temp = '\0';
		temp++;
		malloc_strcpy(&ptr->desc, buffer);

		if (*temp && !isdigit((unsigned char)*temp))
		{
			put_it("%s: not a cdcc pack aborting",  cparse(get_string_var(CDCC_PROMPT_VAR)));
			new_free(&ptr->file);
			new_free((char **)&ptr);
			fclose(file);
			return 0;
		}
		ptr->numfiles = atoi(temp);

		fgets(buffer, BIG_BUFFER_SIZE, file);
		chop(buffer, 1);
		if (*buffer)
			malloc_strcpy(&ptr->notes, buffer);
	

		fgets(buffer, BIG_BUFFER_SIZE, file);
		chop(buffer, 1);

		if ((q = next_arg(p, &p)))
			ptr->gets = my_atol(q);
		if ((q = next_arg(p, &p)))
			ptr->size = my_atol(q);
		next_arg(p, &p); /* skip over min speed for now */
		if ((q = next_arg(p, &p)))
			ptr->server = my_atol(q);

		if ((q = next_arg(p, &p)))
			ptr->timeadded = my_atol(q);

		if (p && *p)
			ptr->password = m_strdup(q);

/*		ptr->gets = atoi(strtok(buffer, space));*/
/*		ptr->size = atol(strtok(NULL, space));*/
/*		ptr->minspeed = atof(strtok(NULL,"\r")); */

		total_size_of_packs += ptr->size;	
		ptr->next = NULL;
		
		if (last) {
			last->next = ptr;
			last = ptr;
		} else {
			offerlist = ptr;
			last = offerlist;
		}
		count++;
	}
		
	fclose(file);
	put_it("%s: \002%d\002 pack%s loaded",  cparse(get_string_var(CDCC_PROMPT_VAR)), count, plural(count));
	set_int_var(_CDCC_PACKS_OFFERED_VAR, cdcc_numpacks);
	new_free(&buffer);

	return 0;
} 
	
	
 
/* --- Misc functions --- */

/* add file/files to a pack */
static void add_files(char *args, char *rest)
{
	char *thefile = NULL, *expand = NULL, *path = NULL;
	char *temp = NULL, *filebuf = NULL;
	char *fptr = NULL, *f_path = NULL;

	DIR *dptr = NULL;
	struct dirent *dir;
	struct stat statbuf;

	path = alloca(strlen(rest)+1);
	strcpy(path, rest);
	
	temp = alloca(BIG_BUFFER_SIZE + 1);
	*temp = 0;

	f_path = alloca(BIG_BUFFER_SIZE + 1);
	*f_path = 0;
			
	newpack = (pack *) new_malloc(sizeof(pack));

	while((thefile = new_next_arg(path, &path)))
	{
		if (!thefile || !*thefile)
			break;
		if ((fptr = strrchr(thefile, '/')))
		{
			*fptr++ = 0;
			strcpy(f_path, thefile);
		}
		else 
		{
			fptr = thefile;
			strcpy(f_path, "~");
		}
		if ((expand = expand_twiddle(f_path)))
			dptr = opendir(expand);
		if (!dptr) 
		{
			put_it("%s: you cannot access dir %s. Attempting to continue",  cparse(get_string_var(CDCC_PROMPT_VAR)),expand ? expand : thefile);
			new_free(&expand);
			new_free(&filebuf);
			continue;
		}
		while ((dir = readdir(dptr))) 
		{
			if (!dir->d_ino || !wild_match(fptr, dir->d_name))
				continue;
			sprintf(temp, "%s/%s", expand, dir->d_name);
			stat(temp, &statbuf);
			sprintf(temp, "\"%s/%s\"", expand, dir->d_name);
			if (filebuf)
				malloc_strcat(&filebuf, space);
			malloc_strcat(&filebuf, temp);
			
			if (S_ISDIR(statbuf.st_mode))
				continue;
			newpack->size += statbuf.st_size;
			newpack->numfiles++;
			total_size_of_packs += statbuf.st_size;
		}
		closedir(dptr);
	}
	if (!newpack->numfiles) 
	{
		put_it("%s: no files found, aborting...",  cparse(get_string_var(CDCC_PROMPT_VAR)));
		new_free(&expand);
		new_free(&filebuf);
		new_free((char **) &newpack);
		return;
	}
	
	newpack->timeadded = now;
	newpack->num = ++cdcc_numpacks;
	newpack->server = from_server;
	set_int_var(_CDCC_PACKS_OFFERED_VAR, cdcc_numpacks);
	malloc_strcpy(&newpack->file, filebuf);

	sprintf(temp, "Description of pack #%d : ", cdcc_numpacks);
	add_wait_prompt(temp, add_desc, empty_string, WAIT_PROMPT_LINE, 1);

	new_free(&expand);
	new_free(&filebuf);
	
	return;
}

/* add a notes type description to the pack */
static void add_note(char *args, char *rest)
{
	if (rest && *rest)
	{
		malloc_strcpy(&newpack->notes, rest);
		put_it("%s: added note to pack #\002%d\002 %s",  cparse(get_string_var(CDCC_PROMPT_VAR)), newpack->num, newpack->notes);
	}
}

/* add a description to the new pack, and add to list */
static void add_desc(char *args, char *rest)
{
	pack *ptr, *last = NULL; 
	char size[20];
	char *temp = NULL;
		
	malloc_strcpy(&newpack->desc, rest);

	for (ptr = offerlist; ptr; ptr = ptr->next)
		last = ptr;

	if (last)
		last->next = newpack;
	else
		offerlist = newpack;
	newpack->next = NULL;
			
	if (newpack->size / 1024 > 999)
		sprintf(size, "\002%3.2f\002mb", (double) (newpack->size / 1024) / 1024);
	else
		sprintf(size, "\002%3.2f\002kb", (double) newpack->size / 1024); 
	put_it("%s: added pack #\002%d\002, \002%d\002 file%s (%s)", cparse(get_string_var(CDCC_PROMPT_VAR)),
		newpack->num, newpack->numfiles,
		plural(newpack->numfiles == 1), size);
	malloc_sprintf(&temp, "Notes for pack #%d : ", cdcc_numpacks);
	add_wait_prompt(temp, add_note, empty_string, WAIT_PROMPT_LINE, 1);
	new_free(&temp);
	return;
}

/* handle the actual removing of packs / all packs */
static void del_pack(char *args, char *rest)
{
	pack *ptr, *last = offerlist;
	int packnum;
	int num = 0;
			
	if (!rest || !*rest)
	{
		put_it("%s: No pack specified for removal", cparse(get_string_var(CDCC_PROMPT_VAR)));
		return;
	}
	if (*rest == '*') {
		cdcc_numpacks = 0;
		for (ptr = last = offerlist; last;) 
		{
			ptr = last->next;
			new_free(&last->desc);
			new_free(&last->notes);
			new_free(&last->file);
			new_free(&last->password);
			new_free((char **) &last);
			last = ptr;
		}
		offerlist = NULL;
		total_size_of_packs = 0;
		set_int_var(_CDCC_PACKS_OFFERED_VAR, 0);
		put_it("%s: removed all packs from offer list", cparse(get_string_var(CDCC_PROMPT_VAR)));
		return;
	}
	
	while (rest && *rest)
	{
		packnum = atoi(next_arg(rest, &rest));
		if (packnum <= 0) 
		{
			put_it("%s: invalid pack specification", cparse(get_string_var(CDCC_PROMPT_VAR)));
			continue;
		}

		for (ptr = offerlist; ptr; ptr = ptr->next) 
		{
			if (ptr->num == packnum) 
			{
				if (ptr != offerlist)
					last->next = ptr->next;
				else
					offerlist = ptr->next;

				new_free(&ptr->desc);
				new_free(&ptr->file);
				new_free(&ptr->notes);
				new_free(&ptr->password);
				total_size_of_packs -= ptr->size;
				new_free((char **) &ptr);
				cdcc_numpacks--;
				set_int_var(_CDCC_PACKS_OFFERED_VAR, cdcc_numpacks);
				put_it("%s: removed pack \002%d\002 from offer list",cparse(get_string_var(CDCC_PROMPT_VAR)), packnum);	
				num++;
				break;
			}
			last = ptr;
		}
	}
	if (num)
	{
		for (ptr = offerlist,num = 1; ptr; ptr = ptr->next)
			ptr->num = num++;
	}
	else
		put_it("%s: pack \002%s\002 does not exist", cparse(get_string_var(CDCC_PROMPT_VAR)),rest); 
	return;
}

/* add a person to the dcc send queue */
int BX_add_to_queue(char *nick, char *command, pack *sendpack)
{
	queue *ptr = NULL, *last = NULL, *new = NULL;
	static char *lastnick = NULL;
	int count;
		
	if (!sendpack || !sendpack->file)
	{
		put_it("%s: ERROR occured in cdcc add to queue", cparse(get_string_var(CDCC_PROMPT_VAR)));
		return 0;
	}
	if (queuelist) 
	{
		for (new = queuelist, count = 1; new; new = new->next)
		{
			if (!my_stricmp(nick, new->nick) && !my_stricmp(sendpack->file, new->file))
			{
				if (!lastnick || my_stricmp(lastnick, nick))
					queue_send_to_server(from_server, "NOTICE %s :\002CDCC\002: You're already Queued for %s at position %d", nick, new->desc, count);
				put_it("%s: Already queued %d files for %s at %d", cparse(get_string_var(CDCC_PROMPT_VAR)), sendpack->numfiles, nick, count);
				return 0;
			}
		}
	}
	new = (queue *) new_malloc(sizeof(queue));
	malloc_strcpy(&new->nick, nick);
	malloc_strcpy(&new->file, sendpack->file);
	malloc_strcpy(&new->desc, sendpack->desc);
	new->time = now;
	new->numfiles = sendpack->numfiles;
	new->server = from_server;
	new->command = m_strdup(command);
	new->next = NULL;
	sendpack->gets++;
		
	for (ptr = queuelist, count = 1; ptr; ptr = ptr->next, count++)
		last = ptr;

	if (last)
		last->next = new;
	else
		queuelist = new;
	numqueue++;
	put_it("%s: Queue position %d queuing %d files for %s", cparse(get_string_var(CDCC_PROMPT_VAR)), numqueue, sendpack->numfiles, nick);
	if (!lastnick || (lastnick && my_stricmp(lastnick, nick)))
		malloc_strcpy(&lastnick, nick);
	return 1;
}
	
/* check queue & send files... called by irc.c io() */
void dcc_sendfrom_queue(void) 
{
	queue *ptr = queuelist;
	char *dccinfo = NULL, *file = NULL, *temp = NULL;
	int old_server = from_server;
	int active =  0;	

	if (!ptr)
		return;	
	active = get_active_count();
	if (active && active >= get_int_var(DCC_SEND_LIMIT_VAR))
		return;	

	put_it("%s: sending \037%s\037 \002%d\002 file%s from queue", cparse(get_string_var(CDCC_PROMPT_VAR)),
		ptr->nick, ptr->numfiles,
		plural(ptr->numfiles));

	file = LOCAL_COPY(ptr->file);

	if (ptr->server < server_list_size() && is_server_connected(ptr->server))
		from_server = ptr->server;

	while ((temp = new_next_arg(file, &file)))
	{
		if (!temp || !*temp)
			break;
		malloc_sprintf(&dccinfo, "%s %s", ptr->nick, temp);
		if (ptr->command)
		{
			if (!my_stricmp(ptr->command, "RESEND") || !my_stricmp(ptr->command, "TRESEND"))
				dcc_resend(ptr->command, dccinfo);
#ifdef MIRC_BROKEN_DCC_RESUME
			else if (!my_stricmp(ptr->command, "RESUME"))
				dcc_resume(ptr->command, dccinfo);
#endif
			else
				dcc_filesend(NULL, dccinfo);
		}
		else
			dcc_filesend(ptr->command, dccinfo);
	}
	new_free(&dccinfo);

	queuelist = ptr->next;
	new_free(&ptr->nick);
	new_free(&ptr->file);
	new_free(&ptr->command);
	new_free((char **) &ptr);
	numqueue--;
	from_server = old_server;	
	return;
}

static time_t plist_last_time = 0;
static void get_minspeed(char *args, char *rest)
{
char *last = NULL;
unsigned long cdcc_mintime = 0;
	if (rest && *rest)
		cdcc_minspeed = strtod(rest, &last);
	if (cdcc_minspeed)
	{
		put_it("%s: Minspeed value to %1.4g KB/s", cparse(get_string_var(CDCC_PROMPT_VAR)),cdcc_minspeed);
		if (last && *last && isdigit((unsigned char)*last))
		{
			cdcc_mintime = strtod(last, NULL);
			set_int_var(_CDCC_MINSPEED_TIME_VAR, cdcc_mintime);
			put_it("%s: Minspeed time to %5d seconds", cparse(get_string_var(CDCC_PROMPT_VAR)), cdcc_mintime);
		}
		else if (!get_int_var(_CDCC_MINSPEED_TIME_VAR))
			put_it("%s: Make sure and /set _CDCC_MINSPEED_TIME as well", cparse(get_string_var(CDCC_PROMPT_VAR)));
	}
	else
		cdcc_minspeed = 0.0;
	
}

static int l_minspeed(char *args, char *rest)
{
char *temp = NULL;
	malloc_sprintf(&temp, "%s min-speed (0 to disable): ", cparse(get_string_var(CDCC_PROMPT_VAR)));
	if (args && *args)
		get_minspeed(NULL, args);
	else
		add_wait_prompt(temp, get_minspeed, empty_string, WAIT_PROMPT_LINE, 1);
	new_free(&temp);
	return 0;
}

static void get_ptimer(char *args, char *rest)
{
	if (rest && *rest)
		ptimer = strtoul(rest, NULL, 10);
	ptimer *= 60;
	if (ptimer)
		put_it("%s: Ptimer interval %d minutes", cparse(get_string_var(CDCC_PROMPT_VAR)),ptimer/60);
	else
		plist_last_time = 0;
}

static int l_timer(char *args, char *rest)
{
char *temp = NULL;
	malloc_sprintf(&temp, "%s p-timer interval(s) (0 to disable): ", cparse(get_string_var(CDCC_PROMPT_VAR)));
	if (args && *args)
		get_ptimer(NULL, args);
	else
		add_wait_prompt(temp, get_ptimer, empty_string, WAIT_PROMPT_LINE, 1);
	new_free(&temp);
	return 0;
}

static void get_pchannel(char *args, char *rest)
{
	if (rest && *rest && is_channel(rest))
		malloc_strcpy(&public_channel, rest);
	else if (rest && *rest && *rest == '*')
	{
		int i = get_window_server(0);
		ChannelList *chan;
		if (i != -1)
		{
			new_free(&public_channel);
			for (chan = get_server_channels(i); chan; chan = chan->next)
				m_s3cat(&public_channel, ",", chan->channel);
		} else
			new_free(&public_channel);
	}
	else
		new_free(&public_channel);
	if (public_channel)
		put_it("%s: Public timer channel(s) are [%s]", cparse(get_string_var(CDCC_PROMPT_VAR)),public_channel);
	else
		put_it("%s: Disabled %s public timer channel(s)", cparse(get_string_var(CDCC_PROMPT_VAR)), cparse(get_string_var(CDCC_PROMPT_VAR)));
}

static int l_channel(char *args, char *rest)
{
	if (args && *args)
		get_pchannel(NULL, args);
	else
		if (public_channel)
			put_it("%s: Public timer channel is [%s]", cparse(get_string_var(CDCC_PROMPT_VAR)),public_channel);
		else
			put_it("%s: Disabled %s public timer channel", cparse(get_string_var(CDCC_PROMPT_VAR)), cparse(get_string_var(CDCC_PROMPT_VAR)));
	return 0;
}

void cdcc_timer_offer(void)
{
	if (!offerlist || !ptimer)
		return;
	if (now - plist_last_time > ptimer)
	{
		plist_last_time = now;
		if (do_notice_list)
			l_notice(public_channel, NULL);
		else
			l_plist(public_channel, NULL);
	}
}

static void add_note1(unsigned long pnum, char *note)
{
pack *this_pack = NULL;
int i;
	if (pnum && note)
	{
		for (i = 1, this_pack = offerlist; this_pack; this_pack = this_pack->next, i++)
			if (i == pnum)
				break;
		if (this_pack)
		{
			if (note && *note)
				malloc_strcpy(&this_pack->notes, note);
			else
				new_free(&this_pack->notes);
		}
		else
			put_it("%s: Invalid pack number %ld", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);

	}
	else
		put_it("%s: Invalid pack number %ld", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);
}

static void add_describe(unsigned long pnum, char *describe)
{
pack *this_pack = NULL;
int i;
	if (pnum && describe)
	{
		for (i = 1, this_pack = offerlist; this_pack; this_pack = this_pack->next, i++)
			if (i == pnum)
				break;
		if (this_pack)
			malloc_strcpy(&this_pack->desc, describe);
		else
			put_it("%s: Invalid pack number %ld", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);

	}
	else
		put_it("%s: Invalid pack number %ld", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);
}

static unsigned long got_pnum = 0;
static unsigned long got_dnum = 0;

static void get_pnote1(char *args, char *rest)
{
	if (got_pnum && rest)
		add_note1(got_pnum, rest);
	got_pnum = 0;
}

static void get_desc(char *args, char *rest)
{
	if (got_dnum && rest)
		add_describe(got_dnum, rest);
	got_dnum = 0;
}

static void get_pnote(char *args, char *rest)
{
unsigned long pnum = 0;
char *temp = NULL;
char *p;
	if ((p = next_arg(rest, &rest)))
		pnum = strtoul(p, NULL, 10);
	if (rest && *rest)
		add_note1(pnum, rest);
	else
	{
		got_pnum = pnum;
		malloc_sprintf(&temp, "%s note to add to pack %ld ", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);
		add_wait_prompt(temp, get_pnote1, empty_string, WAIT_PROMPT_LINE, 1);	
		new_free(&temp);
	}
}

static void get_describe(char *args, char *rest)
{
unsigned long pnum = 0;
char *temp = NULL;
char *p;
	if ((p = next_arg(rest, &rest)))
		pnum = strtoul(p, NULL, 10);
	if (rest && *rest)
		add_describe(pnum, rest);
	else
	{
		got_dnum = pnum;
		malloc_sprintf(&temp, "%s description to add to pack %ld ", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);
		add_wait_prompt(temp, get_desc, empty_string, WAIT_PROMPT_LINE, 1);
		new_free(&temp);
	}
}

static int l_note(char *args, char *rest)
{
char *temp = NULL;
	if (!offerlist)
		return 0;
	malloc_sprintf(&temp, "%s add note to pack #: ", cparse(get_string_var(CDCC_PROMPT_VAR)));
	if (args && *args)
		get_pnote(NULL, args);
	else
		add_wait_prompt(temp, get_pnote, empty_string, WAIT_PROMPT_LINE, 1);
	new_free(&temp);
	return 1;
}

static int l_describe(char *args, char *rest)
{
char *temp = NULL;
	if (!offerlist)
		return 0;
	malloc_sprintf(&temp, "%s change description of pack #: ", cparse(get_string_var(CDCC_PROMPT_VAR)));
	if (args && *args)
		get_describe(NULL, args);
	else
		add_wait_prompt(temp, get_describe, empty_string, WAIT_PROMPT_LINE, 1);
	new_free(&temp);
	return 1;
}

static int l_type(char *args, char *rest)
{
	if (args && *args)
		do_notice_list = do_notice_list ? 0 : 1;
	put_it("%s: type of output for offer set to [%s]", cparse(get_string_var(CDCC_PROMPT_VAR)), do_notice_list? "Notice":"privmsg");
	return 0;
}

static int l_echo(char *args, char *rest)
{
	if (args && *args)
		do_cdcc_echo = do_cdcc_echo ? 0 : 1;
	put_it("%s: local echo set to [%s]", cparse(get_string_var(CDCC_PROMPT_VAR)), on_off(do_cdcc_echo));
	return 0;
}

static int l_stats(char *args, char *rest)
{
	char cdcc_minspeed_s[80];
	sprintf(cdcc_minspeed_s, "%1.3f", cdcc_minspeed);
	put_it("%s",convert_output_format("       %GÕÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ%K[%C    cdcc stat     %K]%GÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¸", NULL));
	put_it("%s",convert_output_format("       %G³                                                                 ³", NULL));
	put_it("%s",convert_output_format("       %G³%gÖÄ%K[%Cp%ctimer  %K]%gÄÖ-%K[%Ct%cype     %K]%gÄ·Ä%K[%Ct%cotal %Cp%cacks%K]%gÄÖÄ%K[%Cs%cent  %K]%gÄ·Ä[%Cq%cueue%K]%gÄ·%G³", NULL));
	put_it("%s",convert_output_format("       %G³%gº %W$[-10]0 %gº  %W$[-10]1 %gº    %W$[-10]2 %gº %W$[-8]3 %gº %W$[-7]4 %gº%G³", "%d %s %d %d %d", ptimer, do_notice_list ?"notice":"privmsg", cdcc_numpacks, send_numpacks, numqueue));
	put_it("%s",convert_output_format("       %G³%gÓÄÄÄÄÄÄÄÄÄÄÄÄ½ÄÄÄÄÄÄÄÄÄÄÄÄÄÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½ÄÄÄÄÄÄÄÄÄÄÓÄÄÄÄÄÄÄÄÄ½%G³", NULL));
	put_it("%s",convert_output_format("       %G³ CDCC channel                                                    ³", NULL));
	put_it("%s",convert_output_format("       %G³ %W$[63]0-%G ³", "%s", !public_channel ? "current channel": public_channel));
	put_it("%s",convert_output_format("       %gÖÄÄÄÄ%K[%C %c  %C %c    %K]%gÄÄÄÖÄÄÄ%K[%C %c   %C %c    %K]%gÄÄÄ·ÄÄÄÄÄÄÄÄÄÄ%K[%Ct%coggles%K]%gÄÄÄÄÄÄÄÄÄÄ·", NULL));
	put_it("%s",convert_output_format("       %gº %C %n    %W$[-6]0%n%R     %gº %C %n    %W$[-6]1%n%R     %gº   %Ct%nimer:   %W$[-3]2%n   %Ce%ncho:  %W$[-3]3 %gº", "1 1 %s %s", on_off(ptimer), on_off(do_cdcc_echo)));
	put_it("%s",convert_output_format("       %gº %C %n    %W$[-6]0%n%R     %gº %C %n    %W$[-6]1%n%R     %gº %Cm%ninspeed:  %W$[-3]2%n   %Cs%necure:%W$[-3]3 %gº", "1 1 %s %s", cdcc_minspeed == 0.0 ? "off":cdcc_minspeed_s, on_off(get_string_var(CDCC_SECURITY_VAR) ? 1 : 0)));
	put_it("%s",convert_output_format("       %gÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÓÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ½", NULL));
	return 0;
}

/* 
 * TimeCawp needed this particular function so I coded it for him.
 * personally I don't think it's needed, but then who am I to say.
 */
 
BUILT_IN_FUNCTION(function_cdcc)
{
pack *ptr;
char *p;
int it = 0;
	if (!cdcc_numpacks)
		return m_strdup(empty_string);

	if (!(p = next_arg(input, &input)))
		return m_strdup(empty_string);
	if (!(it = my_atol(p)))
		return m_sprintf("%d", cdcc_numpacks);

	for (ptr = offerlist; ptr; ptr= ptr->next)
	{
		if (it == ptr->num)
			return m_sprintf("%d %d %lu %d %s", ptr->num, ptr->numfiles, ptr->size, ptr->gets, ptr->desc);
	}
	return m_strdup(empty_string);
}

BUILT_IN_FUNCTION(function_sendcdcc)
{
char *nick = NULL;
int count = 0;
int old_window_display = window_display;

	window_display = 0;
	if ((nick = next_arg(input, &input)))
		count = do_local_send("SEND", nick, input);
	window_display = old_window_display;
	return m_sprintf("%d", count);
}

static void add_password(unsigned long pnum, char *password)
{
pack *this_pack = NULL;
int i;
	if (pnum && password)
	{
		for (i = 1, this_pack = offerlist; this_pack; this_pack = this_pack->next, i++)
			if (i == pnum)
				break;
		if (this_pack)
		{
			if (password && *password)
				malloc_strcpy(&this_pack->password, password);
			else
			{
				new_free(&this_pack->password);
				put_it("%s: Removed passwd from %ld", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);
			}
		}
		else
			put_it("%s: Invalid pack number %ld", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);

	}
	else
		put_it("%s: Invalid pack number %ld", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);
}


static void get_passwd(char *args, char *rest)
{
	if (got_dnum && rest)
		add_password(got_dnum, rest);
	got_dnum = 0;
}

static void get_password(char *args, char *rest)
{
unsigned long pnum = 0;
char *temp = NULL;
char *p;
	if ((p = next_arg(rest, &rest)))
		pnum = strtoul(p, NULL, 10);
	if (rest && *rest)
		add_password(pnum, rest);
	else
	{
		got_dnum = pnum;
		malloc_sprintf(&temp, "%s password to add to pack %ld: ", cparse(get_string_var(CDCC_PROMPT_VAR)), pnum);
		add_wait_prompt(temp, get_passwd, empty_string, WAIT_PROMPT_LINE, 1);
		new_free(&temp);
	}
}

static int l_secure(char *args, char *rest)
{
char *temp = NULL;
	if (!offerlist)
		return 0;
	malloc_sprintf(&temp, "%s change password of pack #: ", cparse(get_string_var(CDCC_PROMPT_VAR)));
	if (args && *args)
		get_password(NULL, args);
	else
		add_wait_prompt(temp, get_password, empty_string, WAIT_PROMPT_LINE, 1);
	new_free(&temp);
	return 0;
}

int BX_get_num_queue(void)
{
	return numqueue;
}

#else /* WANT_CDCC */

/* this is required for functions.c to compile properly */

BUILD_IN_FUNCTION(function_cdcc)
{
	return m_strdup(empty_string);
}

BUILT_IN_FUNCTION(function_sendcdcc)
{
	return m_strdup(empty_string);
}

int get_num_queue(void)
{
	return 0;
}

#endif
