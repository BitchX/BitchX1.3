/*
 * who.c -- The WHO queue.  The ISON queue.  The USERHOST queue.
 *
 * Written by Jeremy Nelson
 * Copyright 1996, 1997 EPIC Software Labs
 */

#include "irc.h"
static char cvsrevision[] = "$Id: who.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(who_c)
#include "struct.h"

#include "commands.h"
#include "ircaux.h"
#include "who.h"
#include "server.h"
#include "window.h"
#include "vars.h"
#include "hook.h"
#include "output.h"
#include "numbers.h"
#include "parse.h"
#include "if.h"
#include "names.h"
#include "misc.h"
#define MAIN_SOURCE
#include "modval.h"

/*
 *
 *
 *
 * 				WHO QUEUE
 *
 *
 *
 */

/* flags used by who queue */
#define WHO_OPS		0x0001
#define WHO_NAME	0x0002
#define WHO_ZERO	0x0004
#define WHO_CHOPS	0x0008
#define WHO_FILE	0x0010
#define WHO_HOST	0x0020
#define WHO_SERVER	0x0040
#define	WHO_HERE	0x0080
#define	WHO_AWAY	0x0100
#define	WHO_NICK	0x0200
#define	WHO_LUSERS	0x0400
#define	WHO_REAL	0x0800
#define WHO_NOCHOPS	0x1000

#define WHO_INVISIBLE	0x2000
/*
 * This is tricky -- this doesnt get the LAST one, it gets the
 * next to the last one.  Why?  Because the LAST one is the one
 * asking, and they want to know who is LAST (before them)
 * So it sucks.  Sue me.
 */
static WhoEntry *who_previous_query (WhoEntry *me)
{
	WhoEntry *what = who_queue_top(from_server);

	while (what && what->next != me)
		what = what->next;

	return what;
}

static void who_queue_add (WhoEntry *item)
{
	WhoEntry *bottom = who_queue_top(from_server);
	while (bottom && bottom->next)
		bottom = bottom->next;

	if (!bottom)
		set_who_queue_top(from_server, item);
	else
		bottom->next = item;

	return;
}

static void delete_who_item (WhoEntry *save)
{
	new_free(&save->who_target);
	new_free(&save->who_name);
	new_free(&save->who_host);
	new_free(&save->who_server);
	new_free(&save->who_nick);
	new_free(&save->who_real);
	new_free(&save->who_stuff);
	new_free(&save->who_end);
	new_free(&save->who_buff);
	new_free(&save->who_args);
	new_free((char **)&save);
}

static void who_queue_pop (void)
{
	WhoEntry *save;
	int	piggyback;

	do
	{
		if (!(save = who_queue_top(from_server)))
			break;

		piggyback = save->piggyback;
		set_who_queue_top(from_server, save->next);
		delete_who_item(save);
	}
	while (piggyback);

	return;
}


static WhoEntry *get_new_who_entry (void)
{
	WhoEntry *new_w = (WhoEntry *)new_malloc(sizeof(WhoEntry));
#if 0
no need to do this
	new_w->dirty = 0;
	new_w->piggyback = 0;
	new_w->who_mask = 0;
	new_w->who_target = NULL;
	new_w->who_host = NULL;
	new_w->who_name = NULL;
	new_w->who_server = NULL;
	new_w->who_nick = NULL;
	new_w->who_real = NULL;
	new_w->who_stuff = NULL;
	new_w->who_end = NULL;
	new_w->next = NULL;
#endif
	return new_w;
}

static void who_queue_list (void)
{
	WhoEntry *item = who_queue_top(from_server);
	int count = 0;

	while (item)
	{
		yell("[%d] [%d] [%s] [%s] [%s]", count,
			item->who_mask,
			item->who_nick ? item->who_nick : empty_string,
			item->who_stuff ? item->who_stuff : empty_string,
			item->who_end ? item->who_end : empty_string);
		count++;
		item = item->next;
	}
}

static void who_queue_flush (void)
{
	WhoEntry *item;

	while ((item = who_queue_top(from_server)))
		who_queue_pop();

	yell("Who queue for server [%d] purged", from_server);
}


/*
 * who: the /WHO command. Parses the who switches and sets the who_mask and
 * whoo_stuff accordingly.  Who_mask and who_stuff are used in whoreply() in
 * parse.c 
 */
BUILT_IN_COMMAND(whocmd)
{
	whobase(args, NULL, NULL, NULL);
}


/*
 * whobase: What does all the work.
 */
void BX_whobase(char *args, void (*line) (WhoEntry *, char *, char **), void (*end) (WhoEntry *, char *, char **), char *format, ...)
{
	char	*arg,
		*channel = NULL;
	int	len;
	WhoEntry *new_w, *old;
	int done = 0;

	/* Maybe should output a warning? */
	if (from_server <= -1 || !is_server_connected(from_server))
		return;

	new_w = get_new_who_entry();
	new_w->line = line;
	new_w->end = end;

	while ((arg = next_arg(args, &args)) != NULL && !done)
	{
		lower(arg);

		if (*arg == '-' || *arg == '/')
		{
			arg++;
			if ((len = strlen(arg)) == 0)
			{
				say("Unknown or missing flag");
				return;
			}

			if (!strncmp(arg, "line", 4))		/* LINE */
			{
				char *line;

				if ((line = next_expr(&args, '{')))
					malloc_strcpy(&new_w->who_stuff, line);
				else
					say("Need {...} argument for -LINE argument.");
			}
			else if (!strncmp(arg, "end", 3))	/* END */
			{
				char *line;

				if ((line = next_expr(&args, '{')))
					malloc_strcpy(&new_w->who_end, line);
				else
					say("Need {...} argument for -END argument.");
			}
			else if (!strncmp(arg, "raw", 3))	/* RAW */
			{
				m_s3cat(&new_w->who_args, " ", args);
				done = 1;
			}
			else if (!strncmp(arg, "o", 1))		/* OPS */
				new_w->who_mask |= WHO_OPS;
			else if (!strncmp(arg, "lu", 2))	/* LUSERS */
				new_w->who_mask |= WHO_LUSERS;
			else if (!strncmp(arg, "ch", 2))	/* CHOPS */
				new_w->who_mask |= WHO_CHOPS;
			else if (!strncmp(arg, "no", 2))	/* NOCHOPS */
				new_w->who_mask |= WHO_NOCHOPS;
			else if (!strncmp(arg, "u-i", 3))	/* INVISIBLE */
				new_w->who_mask |= WHO_INVISIBLE;
			else if (!strncmp(arg, "ho", 2))	/* HOSTS */
			{
				if ((arg = next_arg(args, &args)) == NULL)
				{
					say("WHO -HOST: missing argument");
					return;
				}

				new_w->who_mask |= WHO_HOST;
				malloc_strcpy(&new_w->who_host, arg);
				channel = new_w->who_host;
			}
		 	else if (!strncmp(arg, "he", 2))	/* here */
				new_w->who_mask |= WHO_HERE;
			else if (!strncmp(arg, "a", 1))		/* away */
				new_w->who_mask |= WHO_AWAY;
			else if (!strncmp(arg, "s", 1)) 	/* servers */
			{
				if ((arg = next_arg(args, &args)) == NULL)
				{
					say("WHO -SERVER: missing arguement");
					return;
				}

				new_w->who_mask |= WHO_SERVER;
				malloc_strcpy(&new_w->who_server, arg);
				channel = new_w->who_server;
			}
			else if (!strncmp(arg, "na", 2))
			{
				if ((arg = next_arg(args, &args)) == NULL)
				{
					say("WHO -NAME: missing arguement");
					return;
				}

				new_w->who_mask |= WHO_NAME;
				malloc_strcpy(&new_w->who_name, arg);
				channel = new_w->who_name;
			}
			else if (!strncmp(arg, "re", 2))
			{
				if ((arg = next_arg(args, &args)) == NULL)
				{
					say("WHO -REALNAME: missing arguement");
					return;
				}

				new_w->who_mask |= WHO_REAL;
				malloc_strcpy(&new_w->who_real, arg);
				channel = new_w->who_real;
			}
			else if (!strncmp(arg, "ni", 2))
			{
				if ((arg = next_arg(args, &args)) == NULL)
				{
					say("WHO -NICK: missing arguement");
					return;
				}

				new_w->who_mask |= WHO_NICK;
				malloc_strcpy(&new_w->who_nick, arg);
				channel = new_w->who_nick;
			}
			else if (!strncmp(arg, "d", 1))
			{
				who_queue_list();
				delete_who_item(new_w);
				return;
			}
			else if (!strncmp(arg, "f", 1))
			{
				who_queue_flush();
				delete_who_item(new_w);
				return;
			}
			else
				m_s3cat(&new_w->who_args, " ", arg);
		}
		else if (strcmp(arg, "*") == 0)
		{
			channel = get_current_channel_by_refnum(0);
			if (!channel || !*channel)
			{
				say("You are not on a channel.  Use /WHO ** to see everybody.");
				delete_who_item(new_w);
				return;
			}
		}
		else
			channel = arg;
	}

	if (!channel && (new_w->who_mask & WHO_OPS))
		channel = "*.*";

	if (!channel || !*channel)
	{
		channel = get_current_channel_by_refnum(0);
		if (!channel || !*channel)
		{
			say("You are not on a channel.  Use /WHO ** to see everybody.");
			delete_who_item(new_w);
			return;
		}
	}

	who_queue_add(new_w);
	new_w->who_target = m_strdup(channel);
	if (format)
	{
		va_list arg;
		char buffer[BIG_BUFFER_SIZE+1];
		*buffer = 0;
		va_start(arg, format);
		vsnprintf(buffer, BIG_BUFFER_SIZE, format, arg);
		va_end(arg);
		new_w->who_buff = m_strdup(buffer);
	}

	/*
	 * Check to see if we can piggyback
	 */
	old = who_previous_query(new_w);
	if (old && !old->dirty && old->who_target && channel && 
		!strcmp(old->who_target, channel))
	{
		old->piggyback = 1;
		if (x_debug & DEBUG_OUTBOUND)
			yell("Piggybacking this WHO onto last one.");
	}
	else
		send_to_server("WHO %s %s%s%s", new_w->who_target,
			(new_w->who_mask & WHO_OPS) ?  "o" : empty_string,
			(new_w->who_mask & WHO_INVISIBLE) ? "x": empty_string,
			new_w->who_args ? new_w->who_args : empty_string);
}

void quote_whine(char *type)
{
	yell("### Please dont do /QUOTE %s. Use /%s instead", type, type);
	return;
}

static int who_whine = 0;

void whoreply (char *from, char **ArgList)
{
	int	ok = 1;
	int	voice = 0, opped = 0;
	char	*channel,
		*user,
		*host,
		*server,
		*nick,
		*stat,
		*name;
	ChannelList *chan = NULL;
	char buf_data[BIG_BUFFER_SIZE+1];
	WhoEntry *new_w = who_queue_top(from_server);

	if (!ArgList[5])
		return;		/* Fake! */

	if (!new_w && !who_whine)
	{
		who_whine = 1;
		channel = ArgList[0];
		user    = ArgList[1];
		host    = ArgList[2];
		server  = ArgList[3];
		nick    = ArgList[4];
		stat    = ArgList[5];
		PasteArgs(ArgList, 6);
		name    = ArgList[6];
		voice = (strchr(stat, '+') != NULL);
		opped = (strchr(stat, '@') != NULL);
		set_display_target(channel, LOG_CRAP);
		if (do_hook(WHO_LIST, "%s %s %s %s %s %s %s", channel, nick, stat, user, host, server, name))
			put_it("%s",convert_output_format(fget_string_var(FORMAT_WHO_FSET), "%s %s %s %s %s %s %s", channel, nick, stat, user, host, server, name));
		reset_display_target();
		return;
	}

do
{
	/* 
	 * this can happen in a very likely situation. The timing is critical.
	 * a user joins a channel. We send a userhost, but before the server 
	 * responds, we are kicked and rejoin the channel. A reply is still
	 * coming from the server but the channel sync hasn't finished, 
	 * which means we are in limbo. This should also fix the core when
	 * /quote who is done.
	 */
	if (!new_w)
		break;
	/*
	 * We have recieved a reply to this query -- its too late to
	 * piggyback it now!
	 */
	new_w->dirty = 1;
	/*
	 * We dont always want to use this function.
	 * If another function is supposed to do the work for us,
	 * we yield to them.
	 */
	if (new_w->line)
	{
		new_w->line(new_w, from, ArgList);
		continue;
	}

	channel = ArgList[0];
	user    = ArgList[1];
	host    = ArgList[2];
	server  = ArgList[3];
	nick    = ArgList[4];
	stat    = ArgList[5];
	PasteArgs(ArgList, 6);
	name    = ArgList[6];
	voice = (strchr(stat, '+') != NULL);
	opped = (strchr(stat, '@') != NULL);
	*buf_data = 0;
	strmopencat(buf_data, BIG_BUFFER_SIZE, user, "@", host, NULL);
	
                
	if (*stat == 'S')	/* this only true for the header WHOREPLY */
	{
		char buffer[1024];

		channel = "Channel";
		snprintf(buffer, 1024, "%s %s %s %s %s %s %s", channel,
				nick, stat, user, host, server, name);
		set_display_target(channel, LOG_CRAP);
		if (new_w->who_stuff)
			;			/* munch it */

		else if (do_hook(WHO_LIST, "%s", buffer))
			put_it("%s",convert_output_format(fget_string_var(FORMAT_WHO_FSET), "%s %s %s %s %s %s %s", channel, nick, stat, user, host, server, name));
		reset_display_target();
		return;
	}

	if (new_w && new_w->who_mask)
	{
		if (new_w->who_mask & WHO_HERE)
			ok = ok && (*stat == 'H');
		if (new_w->who_mask & WHO_AWAY)
			ok = ok && (*stat == 'G');
		if (new_w->who_mask & WHO_OPS)
			ok = ok && (*(stat + 1) == '*');
		if (new_w->who_mask & WHO_LUSERS)
			ok = ok && (*(stat + 1) != '*');
		if (new_w->who_mask & WHO_CHOPS)
			ok = ok && ((*(stat + 1) == '@') ||
				    (*(stat + 2) == '@'));
		if (new_w->who_mask & WHO_NOCHOPS)
			ok = ok && ((*(stat + 1) != '@') &&
				    (*(stat + 2) != '@') &&
				    (*(stat + 3) != '@'));
		if (new_w->who_mask & WHO_NAME)
			ok = ok && wild_match(new_w->who_name, user);
		if (new_w->who_mask & WHO_NICK)
			ok = ok && wild_match(new_w->who_nick, nick);
		if (new_w->who_mask & WHO_HOST)
			ok = ok && wild_match(new_w->who_host, host);
		if (new_w->who_mask & WHO_REAL)
			ok = ok && wild_match(new_w->who_real, name);
		if (new_w->who_mask & WHO_SERVER)
			ok = ok && wild_match(new_w->who_server, server);
	}

	if (ok)
	{
		char buffer[1024];

		snprintf(buffer, 1023, "%s %s %s %s %s %s %s", channel,
				nick, stat, user, host, server, name);

		set_display_target(channel, LOG_CRAP);
		chan = add_to_channel(channel, nick, from_server, opped, voice, buf_data, server, stat, 0, my_atol(name));
		if (new_w->who_stuff)
			parse_line(NULL, new_w->who_stuff, buffer, 0, 0, 1);
		else if (!in_join_list(channel, from_server) && do_hook(WHO_LIST, "%s", buffer))
		{
			if (do_hook(current_numeric, "%s", buffer))
			{
				if (!get_int_var(SHOW_WHO_HOPCOUNT_VAR))
					next_arg(name, &name);
				put_it("%s",convert_output_format(fget_string_var(FORMAT_WHO_FSET), "%s %s %s %s %s %s %s", channel, nick, stat, user, host, server, name));
			}
		}
#if WANT_NSLOOKUP		
		if (get_int_var(AUTO_NSLOOKUP_VAR))
			do_nslookup(host, nick, user, channel, from_server, auto_nslookup, NULL);
#endif
		reset_display_target();
	}
}
while (new_w->piggyback && (new_w = new_w->next));
}


void who_end (char *from, char **ArgList)
{
	WhoEntry 	*new_w = who_queue_top(from_server);
	char 		buffer[1025];

	if (who_whine)
		who_whine = 0;
	if (!new_w)
		return;	

	do
	{
		/*
		 * Defer to another function, if neccesary.
		 */
		if (new_w->end)
			new_w->end(new_w, from, ArgList);
		else
		{
			snprintf(buffer, 1024, "%s %s %s", from, ArgList[0], ArgList[1]);
			if (new_w->who_end)
			    parse_line(NULL, new_w->who_end, buffer, 0, 0, 1);

			else if (get_int_var(SHOW_END_OF_MSGS_VAR))
			    if (do_hook(current_numeric, "%s", buffer))
				put_it("%s %s", numeric_banner(), buffer);
		}
	} 
	while (new_w->piggyback && (new_w = new_w->next));

	who_queue_pop();
}


/*
 *
 *
 *
 * 				ISON QUEUE
 *
 *
 *
 */

static void ison_queue_add (IsonEntry *item)
{
	IsonEntry *bottom = ison_queue_top(from_server);

	while (bottom && bottom->next)
		bottom = bottom->next;

	if (!bottom)
		set_ison_queue_top(from_server, item);
	else
		bottom->next = item;

	return;
}

static void ison_queue_pop (void)
{
	IsonEntry *save = ison_queue_top(from_server);
	if (save)
	{
		set_ison_queue_top(from_server, save->next);
		new_free(&save->ison_asked);
		new_free(&save->ison_got);
		new_free((char **)&save);
	}
	return;
}

static IsonEntry *get_new_ison_entry (void)
{
	IsonEntry *new_w = (IsonEntry *)new_malloc(sizeof(IsonEntry));
#if 0
	new_w->ison_asked = NULL;
	new_w->ison_got = NULL;
	new_w->next = NULL;
	new_w->line = NULL;
#endif
	ison_queue_add(new_w);
	return new_w;
}

#if 0
static void ison_queue_list (void)
{
	IsonEntry *item = ison_queue_top(from_server);
	int count = 0;

	while (item)
	{
		yell("[%d] [%s] [%#x]", count, ison_asked, line);
		count++;
		item = item->next;
	}
}
#endif

BUILT_IN_COMMAND(isoncmd)
{
	if (!args || !*args)
		args = LOCAL_COPY(get_server_nickname(from_server));

	isonbase(args, NULL);
}

void BX_isonbase (char *args, void (*line) (char *, char *))
{
	IsonEntry 	*new_i;
	char 		*next = args;

	/* Maybe should output a warning? */
	if (from_server <= -1 || !is_server_connected(from_server))
		return;

	while ((args = next))
	{
		new_i = get_new_ison_entry();
		new_i->line = line;
		if (strlen(args) > 500)
		{
			next = args + 500;
			while (!isspace((unsigned char)*next))
				next--;
			*next++ = 0;
		}
		else
			next = NULL;

		malloc_strcpy(&new_i->ison_asked, args);
		send_to_server("ISON %s", new_i->ison_asked);
	}
}

/* 
 * ison_returned: this is called when numeric 303 is received in
 * numbers.c. ISON must always be the property of the WHOIS queue.
 * Although we will check first that the top element expected is
 * actually an ISON.
 */
void	ison_returned (char *from, char **ArgList)
{
	IsonEntry *new_i = ison_queue_top(from_server);

	if (!new_i)
	{
		quote_whine("ISON");
		return;
	}

	PasteArgs(ArgList, 0);
	if (new_i->line)
		new_i->line(new_i->ison_asked, ArgList[0]);
	else
	{
		if (do_hook(current_numeric, "%s", ArgList[0]))
			put_it("%s Currently online: %s", numeric_banner(), ArgList[0]);
	}

	ison_queue_pop();
	return;
}


/*
 *
 *
 *
 *
 *				USERHOST QUEUE
 *
 *
 *
 *
 */


static void userhost_queue_add (UserhostEntry *item)
{
	UserhostEntry *bottom = userhost_queue_top(from_server);

	while (bottom && bottom->next)
		bottom = bottom->next;

	if (!bottom)
		set_userhost_queue_top(from_server, item);
	else
		bottom->next = item;

	return;
}

static void userhost_queue_pop (void)
{
	UserhostEntry *save = userhost_queue_top(from_server);

	set_userhost_queue_top(from_server, save->next);
	new_free(&save->userhost_asked);
	new_free(&save->text);
	new_free((char **)&save);
	return;
}

static UserhostEntry *get_new_userhost_entry (void)
{
	UserhostEntry *new_u = (UserhostEntry *)new_malloc(sizeof(UserhostEntry));

	userhost_queue_add(new_u);
	return new_u;
}

/*
 * userhost: Does the USERHOST command.  Need to split up the queries,
 * since the server will only reply to 5 at a time.
 */
BUILT_IN_COMMAND(userhostcmd)
{
	userhostbase(args, NULL, 1, NULL);
}

BUILT_IN_COMMAND(useripcmd)
{
	userhostbase(args, NULL, 0, NULL);
}

BUILT_IN_COMMAND(usripcmd)
{
	userhostbase(args, NULL, 2, NULL);
}


void BX_userhostbase(char *args, void (*line) (UserhostItem *, char *, char *), int userhost, char *format, ...)
{
	int	total = 0,
		userhost_cmd = 0;
	int	server_query_reqd = 0;
	char	*nick;
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	buf_data[BIG_BUFFER_SIZE + 1];
	char 	*ptr, 
		*next_ptr,
		*body = NULL;

	if (from_server <= -1 || !is_server_connected(from_server))
		return;

	if (format)
	{
		va_list args;
		va_start(args, format);
		vsnprintf(buf_data, BIG_BUFFER_SIZE, format, args);
		va_end(args);
	}
	else 
		*buf_data = 0;
		
	*buffer = 0;
	while ((nick = next_arg(args, &args)) != NULL)
	{
		if (check_nickname(nick))
		{
			total++;
			if (!fetch_userhost(from_server, nick))
				server_query_reqd++;

			if (*buffer)
				strmcat(buffer, space, BIG_BUFFER_SIZE);
			strmcat(buffer, nick, BIG_BUFFER_SIZE);
		}

		else if (!my_strnicmp(nick, "-cmd", 2))
		{
			if (!total)
			{
				say("%s -cmd with no nicks specified", (userhost == 1) ? "USERHOST":(userhost == 0)?"USERIP":"USRIP");
				return;
			}

			while (my_isspace(*args))
				args++;

			if (!(body = next_expr_failok(&args, '{')))
				body = args;

			userhost_cmd = 1;
			break;
		}

		else if (!my_strnicmp(nick, "-direct", 2))
			server_query_reqd++;
	}

	if (!userhost_cmd && !total)
	{
		server_query_reqd++;
		strlcpy(buffer, get_server_nickname(from_server), BIG_BUFFER_SIZE);
	}
		
	ptr = buffer;

	if (server_query_reqd || (!line && !userhost_cmd))
	{
		ptr = buffer;
		while (ptr && *ptr)
		{
			UserhostEntry *new_u = get_new_userhost_entry();

			move_to_abs_word(ptr, &next_ptr, 5);

			if (next_ptr && *next_ptr && next_ptr > ptr)
				next_ptr[-1] = 0;

			new_u->userhost_asked = m_strdup(ptr);
			send_to_server("%s %s", (userhost == 1) ? "USERHOST" : (!userhost)?"USERIP":"USRIP", new_u->userhost_asked);

			if (userhost_cmd)
				new_u->text = m_strdup(body);
			else if (*buf_data)
				new_u->text = m_strdup(buf_data);
				
			if (line)
				new_u->func = line;
			else if (userhost_cmd)
				new_u->func = userhost_cmd_returned;
			else
				new_u->func = NULL;

			ptr = next_ptr;
		}
	}
	else
	{
		while (ptr && *ptr)
		{
			char *nick = next_arg(ptr, &ptr);
			const char *ouh = fetch_userhost(from_server, nick);
			char *uh;
			UserhostItem item = {0};

			uh = LOCAL_COPY(ouh);
			item.nick = nick;
			item.oper = 0;
			item.connected = 1;
			item.away = 0;
			item.user = uh;
			item.host = strchr(uh, '@');
			if (item.host)
				*item.host++ = 0;
			else
				item.host = "<UNKNOWN>";

			if (line)
				line(&item, nick, body ?  body : *buf_data ? buf_data : NULL);
			else if (userhost_cmd)
				userhost_cmd_returned(&item, nick, body);
			else
				yell("Yowza!  I dont know what to do here!");
		}
	}
}

/* 
 * userhost_returned: this is called when numeric 302 is received in
 * numbers.c. USERHOST must always remain the property of the userhost
 * queue.  Sending out USERHOST requests to the server without going
 * through this queue will cause it to be corrupted and the client will
 * go higgledy-piggledy.
 */
void	userhost_returned (char *from, char **ArgList)
{
	UserhostEntry *top = userhost_queue_top(from_server);
	char *ptr;

	if (!top)
	{
		quote_whine("USERHOST");
		return;
	}

	ptr = top->userhost_asked;

	/*
	 * Go through the nicknames that were requested...
	 */
	while (ptr && *ptr)
	{
		/*
		 * Grab the next nickname
		 */
		char *cnick = next_arg(ptr, &ptr);
		int len = strlen(cnick);

		/*
		 * Now either it is present at the next argument
		 * or its not.  If it is, it will match the first
		 * part of ArgList, and the following char will
		 * either be a * or an = (eg, nick*= or nick=)
		 */
		if (ArgList)
		{
			while (*(*ArgList) == ' ')
				(*ArgList)++;
		}
		if (ArgList && *ArgList && (!my_strnicmp(cnick, *ArgList, len)
	            && ((*ArgList)[len] == '*' || (*ArgList)[len] == '=')))
		{
			UserhostItem item;

			/* Extract all the interesting info */
			item.connected = 1;
			item.nick = next_arg(*ArgList, ArgList);
			item.user = strchr(item.nick, '=');

			if (item.user[-1] == '*')
			{
				item.user[-1] = 0;
				item.oper = 1;
			}
			else
				item.oper = 0;

			if (item.user[1] == '+')
				item.away = 0;
			else
				item.away = 1;

			*item.user++ = 0;
			item.user++;

			item.host = strchr(item.user, '@');
			*item.host++ = 0;


			/*
			 * If the user wanted a callback, then
			 * feed the callback with the info.
			 */
			if (top->func)
				top->func(&item, cnick, top->text);

			/*
			 * Otherwise, the user just did /userhost,
			 * so we offer the numeric, and if the user
			 * doesnt bite, we output to the screen.
			 */
			else if (do_hook(current_numeric, "%s %s %s %s %s", 
						item.nick,
						item.oper ? "+" : "-", 
						item.away ? "-" : "+", 
						item.user, item.host))
				put_it("%s %s is %s@%s%s%s", numeric_banner(),
						item.nick, item.user, item.host, 
						item.oper ?  " (Is an IRC operator)" : empty_string,
						item.away ? " (away)" : empty_string);
		}

		/*
		 * If ArgList isnt the current nick, then the current nick
		 * must not be on irc.  So we whip up a dummy UserhostItem
		 * and send it on its way.  We DO NOT HOOK the 302 numeric
		 * with this bogus entry, because thats the historical
		 * behavior.  This can cause a problem if you do a USERHOST
		 * and wait on the 302 numeric.  I think waiting on the 302
		 * numeric is stupid, anyhow.
		 */
		else
		{
			/*
			 * Of course, only if the user asked for a callback
			 * via /userhost -cmd or a direct call to userhostbase.
			 * If the user just did /userhost, and the nicks arent
			 * on, then we just dont display anything.
			 */
			if (top->func)
			{
				UserhostItem item;

				item.nick = cnick;
				item.user = item.host = "<UNKNOWN>";
				item.oper = item.away = 0;
				item.connected = 1;

				top->func(&item, cnick, top->text);
			}
		}
	}

	userhost_queue_pop();
}

void	userhost_cmd_returned (UserhostItem *stuff, char *nick, char *text)
{
	char	args[BIG_BUFFER_SIZE + 1];

	strcpy(args, stuff->nick ? stuff->nick : empty_string);
	strcat(args, stuff->oper ? " + " : " - ");
	strcat(args, stuff->away ? "+ " : "- ");
	strcat(args, stuff->user ? stuff->user : empty_string);
	strcat(args, space);
	strcat(args, stuff->host ? stuff->host : empty_string);
	parse_line(NULL, text, args, 0, 0, 1);
}

void clean_server_queues (int i)
{
	int old_from_server = from_server;

	if (i == -1 || !get_server_list() || !is_server_connected(i))
		return;		/* Whatever */

	from_server = i;

	while (who_queue_top(i))
		who_queue_pop();

	while (ison_queue_top(i))
		ison_queue_pop();

	while (userhost_queue_top(i))
		userhost_queue_pop();

	from_server = old_from_server;
}


