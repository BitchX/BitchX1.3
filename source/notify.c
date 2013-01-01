/*
 * notify.c: a few handy routines to notify you when people enter and leave irc 
 *
 * Written By Michael Sandrof
 * Copyright(c) 1990 
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * Modified Colten Edwards 96
 */


#include "irc.h"
static char cvsrevision[] = "$Id: notify.c 26 2008-04-30 13:57:56Z keaston $";
CVS_REVISION(notify_c)
#include "struct.h"

#include "list.h"
#include "notify.h"
#include "ircaux.h"
#include "who.h"
#include "hook.h"
#include "server.h"
#include "output.h"
#include "vars.h"
#include "timer.h"
#include "misc.h"
#include "status.h"
#include "userlist.h"
#include "hash2.h"
#include "cset.h"
#include "server.h"
#define MAIN_SOURCE
#include "modval.h"

extern Server *server_list;

#define NOTIFY_LIST(s)          (&(server_list[s].notify_list))
#define NOTIFY_MAX(s)           (NOTIFY_LIST(s)->max)
#define NOTIFY_ITEM(s, i)       (NOTIFY_LIST(s)->list[i])

#define WATCH_LIST(s)          (&(server_list[s].watch_list))
#define WATCH_MAX(s)           (WATCH_LIST(s)->max)
#define WATCH_ITEM(s, i)       (WATCH_LIST(s)->list[i])

void batch_notify_userhost (char *);
void dispatch_notify_userhosts (void);
void notify_userhost_dispatch (UserhostItem *, char *, char *);
void notify_userhost_reply (char *, char *);

void	rebuild_notify_ison (int server)
{
	char *stuff;
	int i;
	if (from_server == -1)
		return;
	stuff = NOTIFY_LIST(from_server)->ison;

	if (NOTIFY_LIST(from_server)->ison)
		NOTIFY_LIST(from_server)->ison[0] = 0;

	for (i = 0; i < NOTIFY_MAX(from_server); i++)
	{
		m_s3cat(&(NOTIFY_LIST(from_server)->ison),
			space, NOTIFY_ITEM(from_server, i)->nick);
	}
}

void	rebuild_all_ison (void)
{
	int i;
	int ofs = from_server;
	for (i = 0; i < server_list_size(); i++)
	{
		from_server = i;
		rebuild_notify_ison(i);
	}
	from_server = ofs;
}



void ison_notify(char *AskedFor, char *AreOn)
{
	char	*NextAsked;
	char	*NextGot;

	NextGot = next_arg(AreOn, &AreOn);
	while ((NextAsked = next_arg(AskedFor, &AskedFor)) != NULL)
	{
		if (NextGot && !my_stricmp(NextAsked, NextGot))
		{
			notify_mark(NextAsked, NULL, 1, 1);
			NextGot = next_arg(AreOn, &AreOn);
		}
		else
			notify_mark(NextAsked, NULL, 0, 1);
	}
        dispatch_notify_userhosts();
}


void add_to_notify_queue(char *buffer)
{
	char *lame = LOCAL_COPY(buffer);
	isonbase(lame, ison_notify);
}

void notify_count(int server, int *on, int *off)
{
	NotifyItem *tmp;
	int i;
	if (server <= -1)
		return;	

	if (on) *on = 0;
	if (off) *off = 0;
	for (i = 0; i < NOTIFY_MAX(server); i++)
	{
		tmp = NOTIFY_ITEM(server, i);
		if (tmp->flag)
		{
			if (on) (*on)++;
		}
		else
		{
			if (off) (*off)++;
		}			
	}
}

/* Rewritten, -lynx */
void show_notify_list(int all)
{
	int count = 0;
	int i;
	char lastseen[20];
	char period[20];
	char timeson[20];
	NotifyItem *tmp;
	
	if (from_server == -1)
		return;	

	for (i = 0; i < NOTIFY_MAX(from_server); i++)
	{
		tmp = NOTIFY_ITEM(from_server, i);
		if (tmp->flag)
		{
			if (count == 0)
			{
				if (do_hook(NOTIFY_HEADER_LIST, "%s", "Online users"))
				{
					put_it("%s", convert_output_format("$G Online Users", NULL, NULL));
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_ON_FSET), "%s %s %s %s %s", "Nick", "UserHost", "Times", "Period", "Last seen"));
				}
			}
			strcpy(period, ltoa(tmp->period));
			strcpy(timeson, ltoa(tmp->times));
			strcpy(lastseen, ltoa(now - tmp->added));
			if (do_hook(NOTIFY_LIST, "%s %s %d %s %s %s", tmp->nick, tmp->host?tmp->host:"unknown@unknown", tmp->flag, timeson, period, lastseen))
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_ON_FSET), "%s %s %s %s %s", tmp->nick, tmp->host?tmp->host:tmp->looking, timeson, lastseen, "now" ));
			count++;
		}
	}
	count = 0;
	for (i = 0; i < NOTIFY_MAX(from_server); i++)
	{
		tmp = NOTIFY_ITEM(from_server, i);
		if ((all && !tmp->flag) || (tmp->times && !tmp->flag))
		{
			if (count == 0)
			{
				if (do_hook(NOTIFY_HEADER_LIST, "%s", "Offline users"))
				{
					put_it("%s", convert_output_format("$G Offline Users", NULL, NULL));
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_OFF_FSET), "%s %s %s %s %s", "Nick", "UserHost", "Times", "Period", "Last seen"));
				}
			}
			strcpy(period, ltoa(tmp->period));
			strcpy(timeson, ltoa(tmp->times));
			strcpy(lastseen, ltoa(now - tmp->lastseen));
			if (do_hook(NOTIFY_LIST, "%s %s %d %s %s %s", tmp->nick, tmp->host?tmp->host:"unknown@unknown", tmp->flag, timeson, period, lastseen))
			{
				if (!tmp->times)
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_OFF_FSET), "%s %s %s %s %s", tmp->nick, tmp->host?tmp->host:tmp->looking, "never", "none", "n/a"));
				else
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_OFF_FSET), "%s %s %s %s %s", tmp->nick, tmp->host?tmp->host:tmp->looking, timeson, period, lastseen));
			}
			count++;
		}
	}
}

/* notify: the NOTIFY command.  Does the whole ball-o-wax */
BUILT_IN_COMMAND(notify)
{
	char	*nick,
		*list = NULL,
		*ptr;
	int	no_nicks = 1;
	int	do_ison = 0;
	int	servnum = from_server;
	int	shown = 0;
	NotifyItem	*new_n;

	malloc_strcpy(&list, empty_string);
	while ((nick = next_arg(args, &args)))
	{
		for (no_nicks = 0; nick; nick = ptr)
		{
			char *host = NULL;
			shown = 0;
			if ((ptr = strchr(nick, ',')) != NULL)
				*ptr++ = 0;
			if ((host = strchr(nick, '!')))
				*host++ = 0;			
			else
				host = "*@*";
				
			if (*nick == '-')
			{
				nick++;
				
				if (*nick)
				{
					for (servnum = 0; servnum < server_list_size(); servnum++)
					{
						if ((new_n = (NotifyItem *)remove_from_array(
							(Array *)NOTIFY_LIST(servnum), nick)))
						{
							new_free(&new_n->nick);
							new_free(&new_n->host);
							new_free(&new_n->looking);
							new_free((char **)&new_n);

							if (!shown)
							{
								bitchsay("%s!%s removed from notification list", nick, host);
								shown = 1;
							}
						}
						else
						{
							if (!shown)
							{
								bitchsay("%s!%s is not on the notification list", nick, host);
								shown = 1;
							}
						}
					}
				}
				else
				{
					for (servnum = 0; servnum < server_list_size(); servnum++)
					{
						while ((new_n = (NotifyItem *)array_pop(
							(Array *)NOTIFY_LIST(servnum), 0)))
						{
							new_free(&new_n->nick);
							new_free(&new_n->host);
							new_free(&new_n->looking);
							new_free((char **)&new_n);
						}
					}
					bitchsay("Notify list cleared");
				}
			}
			else
			{
				/* compatibility */
				if (*nick == '+')
					nick++;

				if (*nick)
				{
					int added = 0;
					if (strchr(nick, '*'))
						bitchsay("Wildcards not allowed in NOTIFY nicknames!");
					else
					{
						for (servnum = 0; servnum < server_list_size(); servnum++)
						{

							if ((new_n = (NotifyItem *)array_lookup(
								(Array *)NOTIFY_LIST(servnum), nick, 0, 0)))
							{
								malloc_strcpy(&new_n->looking, host);
								new_n->flag = 0;
								continue;	/* Already there! */
							}

							new_n = (NotifyItem *)new_malloc(sizeof(NotifyItem));
							new_n->nick = m_strdup(nick);
							new_n->looking = m_strdup(host);
							new_n->host = NULL;
							new_n->flag = 0;
							add_to_array((Array *)NOTIFY_LIST(servnum), 
							     (Array_item *)new_n);
							added = 1;
						}
						if (added)
						{
							m_s3cat(&list, space, new_n->nick);
							do_ison = 1;
						}
						bitchsay("%s!%s added to the notification list", nick, host);
					}
				} else
					show_notify_list(1);
			}
		}
	}

	if (do_ison && get_int_var(NOTIFY_VAR))
	{
		int ofs = from_server;
		for (servnum = 0; servnum < server_list_size(); servnum++)
		{
			from_server = servnum;
			if (is_server_connected(from_server) && list && *list)
				add_to_notify_queue(list);
		}
		from_server = ofs;
	}
	
	new_free(&list);	
	rebuild_all_ison();
	if (no_nicks)
		show_notify_list(0);
}

/*
 * do_notify: This simply goes through the notify list, sending out a WHOIS
 * for each person on it.  This uses the fancy whois stuff in whois.c to
 * figure things out.
 */
void do_notify(void)
{
	int	old_from_server = from_server;
	int	servnum;
	static	time_t		last_notify = 0;
	int		interval = get_int_var(NOTIFY_INTERVAL_VAR);
	time_t current_time = time(NULL);

	if (current_time < last_notify)
		last_notify = current_time;
	else if (!interval || interval > (current_time - last_notify))
		return;		/* Not yet */

	last_notify = current_time;

	if (!server_list_size() || !get_int_var(NOTIFY_VAR))
		return;
	for (servnum = 0; servnum < server_list_size(); servnum++)
	{
		if (is_server_connected(servnum) && !get_server_watch(servnum))
		{
			from_server = servnum;
			if (NOTIFY_LIST(servnum)->ison && *NOTIFY_LIST(servnum)->ison)
			{
				char *lame = LOCAL_COPY(NOTIFY_LIST(servnum)->ison);
				isonbase(lame, ison_notify);
			}
		}
	}
	from_server = old_from_server;
	return;
}

void check_auto_invite(char *nick, char *userhost)
{
#ifdef WANT_USERLIST
ChannelList *chan = NULL;
UserList *tmp = NULL;
	for (chan = get_server_channels(from_server); chan; chan = chan->next)
	{
		if ((tmp = lookup_userlevelc("*", userhost, chan->channel, NULL)))
		{
			NickList *n = NULL;
			n = find_nicklist_in_channellist(nick, chan, 0);
			if (!n && chan->have_op && get_cset_int_var(chan->csets, AINV_CSET) && (tmp->flags & ADD_INVITE) && get_cset_int_var(chan->csets, AINV_CSET))
			{
				bitchsay("Auto-inviting %s to %s", nick, chan->channel);
				send_to_server("NOTICE %s :Auto-invite from %s", nick, get_server_nickname(from_server));
				send_to_server("INVITE %s %s%s%s", nick, chan->channel, chan->key?space:empty_string, chan->key?chan->key:empty_string);
			}
		}
		tmp = NULL;
	}
#endif
}




static char *batched_notify_userhosts = NULL;
static int batched_notifies = 0;

void batch_notify_userhost (char *nick)
{
	m_s3cat(&batched_notify_userhosts, space, nick);
	batched_notifies++;
}

void dispatch_notify_userhosts (void)
{
	if (batched_notify_userhosts)
	{
		if (x_debug & DEBUG_NOTIFY)
			yell("Dispatching notifies to server [%d], [%s]", from_server, batched_notify_userhosts);
		userhostbase(batched_notify_userhosts, notify_userhost_dispatch, 1, NULL);
		new_free(&batched_notify_userhosts);
		batched_notifies = 0;
	}
}

void notify_userhost_dispatch (UserhostItem *stuff, char *nick, char *text)
{
	char userhost[BIG_BUFFER_SIZE + 1];

	snprintf(userhost, BIG_BUFFER_SIZE, "%s@%s", stuff->user, stuff->host);
	notify_userhost_reply(stuff->nick, userhost);
}

#if 0
<MadHack> #0  0x809a571 in n_malloc_strcpy ()
<MadHack> #1  0x80b400e in notify_userhost_reply ()
<MadHack> #2  0x80b3f65 in notify_userhost_dispatch ()
<MadHack> #3  0x80d2ef0 in userhost_returned ()
<MadHack> #4  0x80b61bd in numbered_command ()
hard_uh_notify is on
#endif

void notify_userhost_reply (char *nick, char *userhost)
{
	NotifyItem *tmp;

	if ((tmp = (NotifyItem *)array_lookup(
				(Array *)NOTIFY_LIST(from_server), nick, 0, 0)))
	{
		set_display_target(nick, LOG_CRAP);
		malloc_strcpy(&tmp->nick, nick);
		if (userhost)
		{
			malloc_strcpy(&tmp->host, userhost);
			if (!wild_match(tmp->looking, userhost))
			{
				reset_display_target();
				return;
			}
			check_auto_invite(nick, userhost);
		}
		if ((do_hook(NOTIFY_SIGNON_LIST, "%s %s", tmp->nick, tmp->host?tmp->host:empty_string)))
		{
			put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_SIGNON_FSET), "%s %s %s",update_clock(GET_TIME),tmp->nick,tmp->host?tmp->host:empty_string));
                        logmsg(LOG_NOTIFY, tmp->nick,  0, "%s(%s) has logged on", tmp->host ? tmp->host : empty_string, tmp->looking);
		}
		malloc_strcpy(&last_notify_nick, nick);
		reset_display_target();
	}
}


/*
 * notify_mark: This marks a given person on the notify list as either on irc
 * (if flag is 1), or not on irc (if flag is 0).  If the person's status has
 * changed since the last check, a message is displayed to that effect.  If
 * the person is not on the noTify list, this call is ignored 
 * doit if passed as 0 means it comes from a join, or a msg, etc, not from
 * an ison reply.  1 is the other..
 */
void notify_mark(char *nick, char *userhost, int flag, int doit)
{
	NotifyItem 	*tmp;
	int		count = 0;
	int		loc = 0;
	
	if ((tmp = (NotifyItem *)find_array_item(
				(Array *)NOTIFY_LIST(from_server), nick, 
				&count, &loc)) && count < 0)
	{
		if (flag)
		{
			if (tmp->host && !wild_match(tmp->looking, tmp->host))
			{
				tmp->flag = 0;
				return;
			}
			if (tmp->flag != 1)
			{
			        batch_notify_userhost(nick);
				tmp->lastseen = 0;
				if (!tmp->added)
					tmp->added = now;
				tmp->times++;
			}
			tmp->flag = 1;
			if (!doit)
				dispatch_notify_userhosts();
		}
		else
		{
			if (tmp->flag == 1)
			{
				const char *who = NULL;
				unsigned long level = 0;
				save_display_target(&who, &level);
				set_display_target(nick, LOG_NOTIFY);
				check_orig_nick(nick);
				if ((do_hook(NOTIFY_SIGNOFF_LIST, "%s %s", tmp->nick, tmp->host?tmp->host:empty_string)) && doit)
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_SIGNOFF_FSET), "%s %s %s",update_clock(GET_TIME),tmp->nick,tmp->host?tmp->host:empty_string));
				tmp->period += now - tmp->added;
				tmp->lastseen = now;
	                        logmsg(LOG_NOTIFY, tmp->nick,  0, "%s(%s) has logged off", tmp->host ? tmp->host : empty_string, tmp->looking);
				restore_display_target(who, level);
			}
			tmp->flag = 0;
		}
	}
}

void save_notify(FILE *fp)
{
	int i = 0;
	if (server_list_size() && NOTIFY_MAX(0))
	{
		fprintf(fp, "NOTIFY");
		for (i = 0; i < NOTIFY_MAX(0); i++)
			fprintf(fp, " %s!%s", NOTIFY_ITEM(0, i)->nick, NOTIFY_ITEM(0, i)->looking);
		fprintf(fp, "\n");
	}
	if (NOTIFY_MAX(0) && do_hook(SAVEFILE_LIST, "Notify %d", NOTIFY_MAX(0)))
		bitchsay("Saved %d Notify entries", NOTIFY_MAX(0));
}

/* I hate broken compilers -mrg */
static	char	*vals[] = { "NOISY", "QUIET", "OLD", NULL };

void set_notify_handler(Window *win, char *value, int unused)
{
	int	len;
	int	i;
	char	*s;

	if (!value)
		value = empty_string;
	for (i = 0, len = strlen(value); (s = vals[i]); i++)
		if (0 == my_strnicmp(value, s, len))
			break;
	set_string_var(NOTIFY_HANDLER_VAR, s);
	return;
}

void make_notify_list (int servnum)
{
	NotifyItem *tmp;
	char *list = NULL;
	int i;

	server_list[servnum].notify_list.func = (alist_func)global[MY_STRICMP];
	server_list[servnum].notify_list.hash = HASH_INSENSITIVE;

	if (!get_int_var(NOTIFY_VAR))
		return;
	for (i = 0; i < NOTIFY_MAX(0); i++)
	{
		tmp = (NotifyItem *)new_malloc(sizeof(NotifyItem));
		tmp->nick = m_strdup(NOTIFY_ITEM(0, i)->nick);
		tmp->looking = m_strdup(NOTIFY_ITEM(0, i)->looking);
		tmp->flag = 0;

		add_to_array ((Array *)NOTIFY_LIST(servnum),
			      (Array_item *)tmp);
		m_s3cat(&list, space, tmp->nick);
	}
	if (list)
		isonbase(list, ison_notify);
	new_free(&list);
}

char *get_notify_nicks (int showserver, int showon, char *nick, int extra)
{
	char *list = NULL;
	int i = 0;
	
	if (showserver < 0 || showserver >= server_list_size())
		return m_strdup(empty_string);
 
	if (nick && *nick)
	{
		NotifyItem *tmp;
		for (i = 0; i < NOTIFY_MAX(showserver); i++)
		{
			if (my_stricmp(nick, NOTIFY_ITEM(showserver, i)->nick))
				continue;
			tmp = NOTIFY_ITEM(from_server, i);
			m_s3cat(&list, space, tmp->nick);
			m_s3cat(&list, space, tmp->host);
			m_s3cat(&list, space, ltoa(tmp->flag));
			m_s3cat(&list, space, ltoa(tmp->period));
			m_s3cat(&list, space, ltoa(tmp->times));
			m_s3cat(&list, space, ltoa(tmp->lastseen));
			break;
		}
	}
	else
	{
		for (i = 0; i < NOTIFY_MAX(showserver); i++)
		{
			if (showon == -1 || showon == NOTIFY_ITEM(showserver, i)->flag)
				m_s3cat(&list, space, NOTIFY_ITEM(showserver, i)->nick);
		}
	}
	return list ? list : m_strdup(empty_string);
}


char *get_watch_nicks (int showserver, int showon, char *nick, int extra)
{
	char *list = NULL;
	int i = 0;
	
	if (showserver < 0 || showserver >= server_list_size())
		return m_strdup(empty_string);
 
	if (nick && *nick)
	{
		NotifyItem *tmp;
		for (i = 0; i < WATCH_MAX(showserver); i++)
		{
			if (my_stricmp(nick, WATCH_ITEM(showserver, i)->nick))
				continue;
			tmp = WATCH_ITEM(from_server, i);
			m_s3cat(&list, space, tmp->nick);
			m_s3cat(&list, space, tmp->host);
			m_s3cat(&list, space, ltoa(tmp->flag));
			m_s3cat(&list, space, ltoa(tmp->period));
			m_s3cat(&list, space, ltoa(tmp->times));
			m_s3cat(&list, space, ltoa(tmp->lastseen));
			break;
		}
	}
	else
	{
		for (i = 0; i < WATCH_MAX(showserver); i++)
		{
			if (showon == -1 || showon == WATCH_ITEM(showserver, i)->flag)
				m_s3cat(&list, space, WATCH_ITEM(showserver, i)->nick);
		}
	}
	return list ? list : m_strdup(empty_string);
}


int iswatch(int serv, char *list, int all)
{
	if (all)
	{
		int servnum;
		for (servnum = 0; servnum < server_list_size(); servnum++)
		{
			if (get_server_watch(servnum) && is_server_connected(servnum))
				my_send_to_server(servnum, "WATCH %s", list);
		}
		return 0;
	}
	if (get_server_watch(serv) && is_server_connected(serv))
		my_send_to_server(serv, "WATCH %s", list);
	return 0;
}

void make_watch_list (int servnum)
{
	NotifyItem *tmp;
	char *list = NULL;
	int i;

	server_list[servnum].watch_list.func = (alist_func)global[MY_STRICMP];
	server_list[servnum].watch_list.hash = HASH_INSENSITIVE;

	for (i = 0; i < WATCH_MAX(0); i++)
	{
		if (!(tmp = (NotifyItem *)array_lookup((Array *)WATCH_LIST(servnum), WATCH_ITEM(0, i)->nick, 0, 0)))
		{
			tmp = (NotifyItem *)new_malloc(sizeof(NotifyItem));
			malloc_strcpy(&tmp->nick, WATCH_ITEM(0, i)->nick);
			add_to_array ((Array *)WATCH_LIST(servnum), (Array_item *)tmp);
		}
		tmp->flag = 0;
		if (!list)
			list = m_opendup(space_plus, tmp->nick, NULL);
		else
			m_s3cat(&list, space_plus, tmp->nick);
	}
	if (list)
		iswatch(servnum, list, 0);
	new_free(&list);
}


void show_watch_list(int all)
{
#if 0
int i;
char *list = NULL;
	if ((server != -1) && server < server_list_size() && WATCH_MAX(server))
	{
		put_it("Watch List");
		for (i = 0; i < WATCH_MAX(server); i++)
			m_s3cat(&list, space, WATCH_ITEM(server, i)->nick);
		put_it("%s", list);
	}
	new_free(&list);
#endif
int i, count = 0;
char period[80], timeson[80], lastseen[80];
NotifyItem *tmp;

	if (from_server == -1)
		return;
	for (i = 0; i < WATCH_MAX(from_server); i++)
	{
		tmp = WATCH_ITEM(from_server, i);
		if (tmp->flag)
		{
			strcpy(period, ltoa(tmp->period));
			strcpy(timeson, ltoa(tmp->times));
			strcpy(lastseen, ltoa(now - tmp->added));
			if (do_hook(WATCH_LIST, "%s %s %d %s %s %s", tmp->nick, tmp->host?tmp->host:"unknown@unknown", tmp->flag, timeson, period, lastseen))
			{
				if (!count)
				{
					put_it("%s", convert_output_format("$G Online Users", NULL, NULL));
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_ON_FSET), "%s %s %s %s %s", "Nick", "UserHost", "Times", "Period", "Last seen"));
				}
				put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_ON_FSET), "%s %s %s %s %s", tmp->nick, tmp->host?tmp->host:tmp->looking, timeson, lastseen, "now" ));
			}
			count++;
		}
	}
	count = 0;
	for (i = 0; i < WATCH_MAX(from_server); i++)
	{
		tmp = WATCH_ITEM(from_server, i);
		if ((all && !tmp->flag) || (tmp->times && !tmp->flag))
		{
			strcpy(period, ltoa(tmp->period));
			strcpy(timeson, ltoa(tmp->times));
			strcpy(lastseen, ltoa(now - tmp->lastseen));
			if (do_hook(WATCH_LIST, "%s %s %d %s %s %s", tmp->nick, tmp->host?tmp->host:"unknown@unknown", tmp->flag, timeson, period, lastseen))
			{
				if (!count)
				{
					put_it("%s", convert_output_format("$G Offline Users", NULL, NULL));
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_OFF_FSET), "%s %s %s %s %s", "Nick", "UserHost", "Times", "Period", "Last seen"));
				}

				if (!tmp->times)
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_OFF_FSET), "%s %s %s %s %s", tmp->nick, tmp->host?tmp->host:tmp->looking, "never", "none", "n/a"));
				else
					put_it("%s", convert_output_format(fget_string_var(FORMAT_NOTIFY_OFF_FSET), "%s %s %s %s %s", tmp->nick, tmp->host?tmp->host:tmp->looking, timeson, period, lastseen));
			}
			count++;
		}
	}

}

BUILT_IN_COMMAND(watchcmd)
{
char *nick;
char *list = NULL;
int servnum = 0;
	if (args && *args)
	{
		while ((nick = next_arg(args, &args)))
		{
			NotifyItem *new_n;
			int shown = 0;
			if (*nick == '+')
			{
				int added = 0;
				nick++;
				if (!*nick)
				{
					show_watch_list(0);
					continue;
				}
				if (strchr(nick, '*'))
					bitchsay("Wildcards not allowed in WATCH nicknames!");
				else
				{
					for (servnum = 0; servnum < server_list_size(); servnum++)
					{
						if ((new_n = (NotifyItem *)array_lookup(
							(Array *)WATCH_LIST(servnum), nick, 0, 0)))
						{
							new_n->flag = 0;
							continue;	/* Already there! */
						}
						if (WATCH_MAX(servnum) >= (is_server_connected(servnum) ? get_server_watch(servnum) : 128))
						{
							bitchsay("Too many WATCH entries [%d]", get_server_watch(servnum));
							continue;
						}
						new_n = (NotifyItem *)new_malloc(sizeof(NotifyItem));
						new_n->nick = m_strdup(nick);
						new_n->flag = 0;
						add_to_array((Array *)WATCH_LIST(servnum), 
						     (Array_item *)new_n);
						added = 1;
					}
					if (added)
					{
						if (!list)
							list = m_opendup(space_plus, new_n->nick, NULL);
						else
							m_s3cat(&list, space_plus, new_n->nick);
					}
					bitchsay("%s added to the watch list", nick);
				}
			}
			else if (*nick == '-')
			{
				nick++;
				if (!*nick)
					continue;
				for (servnum = 0; servnum < server_list_size(); servnum++)
				{
					if ((new_n = (NotifyItem *)remove_from_array(
						(Array *)WATCH_LIST(servnum), nick)))
					{
						if (!list)
							list = m_opendup(space_minus, nick, NULL);
						else
							m_s3cat(&list, space_minus, nick);
						new_free(&new_n->nick);
						new_free(&new_n->host);
						new_free(&new_n->looking);
						new_free((char **)&new_n);
						if (!shown)
						{
							bitchsay("%s removed from watch list", nick);
							shown = 1;
						}
					}
					else
					{
						if (!shown)
						{
							bitchsay("%s is not on the watch list", nick);
							shown = 1;
						}
					}
				}
			}
			else if (*nick == 'S')
				my_send_to_server(from_server, "%s S", command);
			else if (*nick == 'L')
				my_send_to_server(from_server, "%s L", command);
			else if (*nick == 'l')
				my_send_to_server(from_server, "%s l", command);
			else if (*nick == 'C')
			{
				for (servnum = 0; servnum < server_list_size(); servnum++)
				{
					if (!get_server_watch(servnum))
						continue;
					while ((new_n = (NotifyItem *)array_pop(
						(Array *)WATCH_LIST(servnum), 0)))
					{
						new_free(&new_n->nick);
						new_free(&new_n->looking);
						new_free(&new_n->host);
						new_free((char **)&new_n);
					}
					if (get_server_watch(servnum) && is_server_connected(servnum))
						my_send_to_server(servnum, "%s S", command);
				}
				bitchsay("Watch list cleared");
			}
		}
		if (list)
			iswatch(servnum, list, 1);
		new_free(&list);
	} else	
		show_watch_list(1);
}

void save_watch(FILE *fp)
{
	int i = 0;
	if (server_list_size() && WATCH_MAX(0))
	{
		fprintf(fp, "WATCH");
		for (i = 0; i < WATCH_MAX(0); i++)
			fprintf(fp, " +%s", WATCH_ITEM(0, i)->nick);
		fprintf(fp, "\n");
	}
	if (WATCH_MAX(0) && do_hook(SAVEFILE_LIST, "Watch %d", WATCH_MAX(0)))
		bitchsay("Saved %d Watch entries", WATCH_MAX(0));
}

void show_watch_notify(char *from, int online, char **args)
{
NotifyItem *new_n;
	if ((new_n = (NotifyItem *)array_lookup((Array *)WATCH_LIST(from_server),
		args[0], 0, 0)))
	{
		if (online)
		{
			new_free(&new_n->host);
			new_n->host = m_opendup(args[1], "@", args[2], NULL);
			if (!new_n->flag)
			{
				new_n->lastseen = 0;
				if (!new_n->added)
					new_n->added = now;
				new_n->times++;
	                        logmsg(LOG_NOTIFY, new_n->nick,  0, "%s has logged on", new_n->host ? new_n->host : empty_string);
			}
			new_n->period = my_atol(args[3]);
		}
		else
		{
			if (new_n->flag)
			{
				new_n->period = now - new_n->added;
				new_n->lastseen = now;
	                        logmsg(LOG_NOTIFY, new_n->nick,  0, "%s has logged off", new_n->host ? new_n->host : empty_string);
			}
		}
		new_n->flag = online;
		put_it("%s", convert_output_format(fget_string_var(online?FORMAT_WATCH_SIGNON_FSET:FORMAT_WATCH_SIGNOFF_FSET), "%s %s %s %s", args[0], args[1], args[2], args[3]));
	}
}

void send_watch(int server)
{
int i;
char *list = NULL;
NotifyItem *tmp;
	for (i = 0; i < WATCH_MAX(server); i++)
	{
		if (!(tmp = (NotifyItem *)array_lookup((Array *)WATCH_LIST(server), WATCH_ITEM(server, i)->nick, 0, 0)))
		{
			tmp = (NotifyItem *)new_malloc(sizeof(NotifyItem));
			malloc_strcpy(&tmp->nick, WATCH_ITEM(server, i)->nick);
			add_to_array ((Array *)WATCH_LIST(server), (Array_item *)tmp);
		}
		tmp->flag = 0;
		if (!list)
			list = m_opendup(space_plus, tmp->nick, NULL);
		else
		{
			if ((strlen(list) + strlen(tmp->nick)) > 490)
			{
				iswatch(server, list, 0);
				new_free(&list);
				list = m_opendup(space_plus, tmp->nick, NULL);
			}
			else
				m_s3cat(&list, space_plus, tmp->nick);
		}
	}
	if (list)
		iswatch(server, list, 0);
	new_free(&list);
}
