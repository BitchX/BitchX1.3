/*
 * whowas.c   a linked list buffer of people who have left your channel 
 * mainly used for ban prot and stats stuff.
 * Should even speed stuff up a bit too.
 *
 * Written by Scott H Kilau
 *
 * Copyright(c) 1995
 * Modified Colten Edwards 1996
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 */

#include "irc.h"
static char cvsrevision[] = "$Id: whowas.c 77 2009-10-16 12:35:56Z keaston $";
CVS_REVISION(whowas_c)
#include "struct.h"

#include "vars.h"
#include "ircaux.h"
#include "window.h"
#include "who.h"
#include "hook.h"
#include "input.h"
#include "names.h"
#include "alias.h"
#include "output.h"
#include "numbers.h"
#include "status.h"
#include "screen.h"
#include "commands.h"
#include "config.h"
#include "list.h"
#include "userlist.h"
#include "misc.h"
#include "hash.h"
#include "hash2.h"
#define MAIN_SOURCE
#include "modval.h"

/*
#include "ctimers.h"
#include "edit2.h"
#include "edit3.h"
#include "edit4.h"
*/

#include "userlist.h"
#include "whowas.h"

WhowasWrapList whowas_userlist_list = { 0 };
WhowasWrapList whowas_reg_list = { 0 };
WhowasWrapList whowas_splitin_list = { 0 };

WhowasChanList *whowas_chan_list = NULL;

static int whowas_userlist_count = 0;
static int whowas_reg_count = 0;
static int whowas_chan_count = 0;

extern	WhowasList *check_whowas_buffer(char *nick, char *userhost, char *channel)
{
	WhowasList *tmp;

	tmp = find_userhost_channel(userhost, channel, 1, &whowas_userlist_list);
	if (tmp)
	{
		whowas_userlist_count--;
		return tmp;
	}

	tmp = find_userhost_channel(userhost, channel, 1, &whowas_reg_list);
	if (tmp)
	{
		whowas_reg_count--;
	}
	
	return tmp;
}


extern	WhowasList * check_whowas_nick_buffer(char *nick, char *channel, int unlink)
{
	WhowasList *tmp = NULL, *last = NULL;
	for (tmp = next_userhost(&whowas_userlist_list, NULL); tmp; tmp = next_userhost(&whowas_userlist_list, tmp))
	{
		if (!my_stricmp(tmp->nicklist->nick, nick) && !my_stricmp(tmp->channel, channel))
		{
			if (unlink)
			{
				last = find_userhost_channel(tmp->nicklist->host, tmp->channel, 1, &whowas_userlist_list);
				tmp = NULL;
			}
			return last?last:tmp;
		}
	}
	for (tmp = next_userhost(&whowas_reg_list, NULL); tmp; tmp = next_userhost(&whowas_reg_list, tmp))
	{
		if (!my_stricmp(tmp->nicklist->nick, nick) && !my_stricmp(tmp->channel, channel))
		{
			if (unlink)
			{
				last = find_userhost_channel(tmp->nicklist->host, tmp->channel, 1, &whowas_reg_list);
				tmp = NULL;
			}
			return last?last:tmp;
		}
	}
	return( NULL );
}

extern	WhowasList * check_whosplitin_buffer(char *nick, char *userhost, char *channel, int unlink)
{
	WhowasList *tmp = NULL;
	tmp = find_userhost_channel(userhost, channel, unlink, &whowas_splitin_list);
	return tmp;
}

void add_to_whowas_buffer(NickList *nicklist, char *channel, char *server1, char *server2)
{
	WhowasList *new;
	if (!nicklist || !nicklist->nick)
		return;

	if (nicklist->userlist) 
	{
		if (whowas_userlist_count >= WHOWAS_USERLIST_MAX) 
		{
			whowas_userlist_count -=
			   remove_oldest_whowas(&whowas_userlist_list, 0,
			   (whowas_userlist_count + 1) - WHOWAS_USERLIST_MAX); 
		}
		new = (WhowasList *) new_malloc(sizeof(WhowasList));
		new->has_ops = nick_isop(nicklist);
		new->nicklist = nicklist;
		malloc_strcpy(&(new->channel), channel);
		malloc_strcpy(&(new->server1), server1);
		if (server2)
			malloc_strcpy(&(new->server2), server2);
		new->time = now;
		add_whowas_userhost_channel(new, &whowas_userlist_list);
		whowas_userlist_count++;
	}
	else 
	{
		if (whowas_reg_count >= WHOWAS_REG_MAX) 
		{
			whowas_reg_count -=
			   remove_oldest_whowas(&whowas_reg_list, 0,
			   (whowas_reg_count + 1) - WHOWAS_REG_MAX); 
		}
		new = (WhowasList *) new_malloc(sizeof(WhowasList));
		new->has_ops = nick_isop(nicklist);
		new->nicklist = (NickList *) nicklist;
		malloc_strcpy(&(new->channel), channel);
		malloc_strcpy(&(new->server1), server1);
		malloc_strcpy(&(new->server2), server2);
		new->time = now;
		add_whowas_userhost_channel(new, &whowas_reg_list);
		whowas_reg_count++;
	}
}

void add_to_whosplitin_buffer(NickList *nicklist, char *channel, char *server1, char *server2)
{
	WhowasList *new;

	new = (WhowasList *) new_malloc(sizeof(WhowasList));
	new->has_ops = nick_isop(nicklist);

	new->nicklist = (NickList *)new_malloc(sizeof(NickList)); /*nicklist;*/
	new->nicklist->nick = m_strdup(nicklist->nick);
	new->nicklist->host = m_strdup(nicklist->host);

	malloc_strcpy(&(new->channel), channel);
	malloc_strcpy(&(new->server1), server1);
	malloc_strcpy(&(new->server2), server2);
	new->time = now;
	add_whowas_userhost_channel(new, &whowas_splitin_list);

}

int remove_oldest_whowas(WhowasWrapList *list, time_t timet, int count)
{
	int total = 0;
	/* if no ..count.. then remove ..time.. links */
	total = remove_oldest_whowas_hashlist(list, timet, count);
	return total;
}

void clean_whowas_list(void)
{
	if (whowas_userlist_count)
		whowas_userlist_count -= remove_oldest_whowas_hashlist(&whowas_userlist_list, 20 * 60, 0);
	if (whowas_reg_count)
		whowas_reg_count -= remove_oldest_whowas_hashlist(&whowas_reg_list, 10 * 60, 0);
	remove_oldest_whowas_hashlist(&whowas_splitin_list, 25 * 60 , 0);
}

/* Used to rehash whowas listings for new users */
void sync_whowas_adduser(UserList *added)
{
WhowasList *tmp;
	for (tmp = next_userhost(&whowas_userlist_list, NULL); tmp; tmp = next_userhost(&whowas_userlist_list, tmp))
	{
		if (check_channel_match(added->channels, tmp->channel))
			if (wild_match(added->host, tmp->nicklist->host) && wild_match(added->nick, tmp->nicklist->nick))
				tmp->nicklist->userlist = added;
	}
	for (tmp = next_userhost(&whowas_reg_list, NULL); tmp; tmp = next_userhost(&whowas_reg_list, tmp))
	{
		if (check_channel_match(added->channels, tmp->channel))
			if (wild_match(added->host, tmp->nicklist->host) && wild_match(added->nick, tmp->nicklist->nick))
				tmp->nicklist->userlist = added;
	}
}

/* Used to rehash whowas listings for removed userlist entries */
void sync_whowas_unuser(UserList *entry)
{
WhowasList *tmp;
	for (tmp = next_userhost(&whowas_userlist_list, NULL); tmp; tmp = next_userhost(&whowas_userlist_list, tmp))
	{
		if (!tmp->nicklist->userlist)  continue;
		if (!my_stricmp(tmp->nicklist->userlist->host, entry->host) && !my_stricmp(tmp->nicklist->userlist->nick, entry->nick) && 
			check_channel_match(tmp->nicklist->userlist->channels, entry->channels))
			tmp->nicklist->userlist = NULL;
	}
	for (tmp = next_userhost(&whowas_reg_list, NULL); tmp; tmp = next_userhost(&whowas_reg_list, tmp))
	{
		if (!tmp->nicklist->userlist)  continue;
		if (!my_stricmp(tmp->nicklist->userlist->host, entry->host) && !my_stricmp(tmp->nicklist->userlist->nick, entry->nick) && 
			check_channel_match(tmp->nicklist->userlist->channels, entry->channels))
			tmp->nicklist->userlist = NULL;
	}
}

/* Used to rehash whowas listings for new shitlist entries */
void sync_whowas_addshit(ShitList *added)
{
WhowasList *tmp;
char user[BIG_BUFFER_SIZE];
	for (tmp = next_userhost(&whowas_userlist_list, NULL); tmp; tmp = next_userhost(&whowas_userlist_list, tmp))
	{
		if (check_channel_match(added->channels, tmp->channel)) 
		{
			sprintf(user, "%s!%s", tmp->nicklist->nick, tmp->nicklist->host);
			if (wild_match(added->filter, user))
				tmp->nicklist->shitlist = added;
		}
	}
	for (tmp = next_userhost(&whowas_reg_list, NULL); tmp; tmp = next_userhost(&whowas_reg_list, tmp))
	{
		if (check_channel_match(added->channels, tmp->channel))
		{
			sprintf(user, "%s!%s", tmp->nicklist->nick, tmp->nicklist->host);
			if (wild_match(added->filter, user))
				tmp->nicklist->shitlist = added;
		}
	}
}

/* Used to rehash whowas listings for removed shitlist entries */
void sync_whowas_unshit(ShitList *entry)
{
WhowasList *tmp;
	for (tmp = next_userhost(&whowas_userlist_list, NULL); tmp; tmp = next_userhost(&whowas_userlist_list, tmp))
	{
		if (!tmp->nicklist->shitlist)  continue;
		if (!my_stricmp(tmp->nicklist->shitlist->filter, entry->filter) &&
			check_channel_match(tmp->nicklist->shitlist->channels, entry->channels))
			tmp->nicklist->shitlist = NULL;
	}
	for (tmp = next_userhost(&whowas_reg_list, NULL); tmp; tmp = next_userhost(&whowas_reg_list, tmp))
	{
		if (!tmp->nicklist->shitlist)  continue;
		if (!my_stricmp(tmp->nicklist->shitlist->filter, entry->filter) &&
			check_channel_match(tmp->nicklist->shitlist->channels, entry->channels))
			tmp->nicklist->shitlist = NULL;
	}
}


/* BELOW THIS MARK IS THE CHANNEL WHOWAS STUFF */

extern	WhowasChanList *check_whowas_chan_buffer(char *channel, int refnum, int unlink)
{
	WhowasChanList *tmp, *last = NULL;

	for (tmp = whowas_chan_list; tmp; tmp = tmp->next) {
		if (!my_stricmp(tmp->channellist->channel, channel))
		{
			if ((refnum == -1) || (refnum == tmp->channellist->refnum)) 
			{
				if (unlink) {
					if (last)
						last->next = tmp->next;
					else
						whowas_chan_list = tmp->next;
					whowas_chan_count--;
				}
				return(tmp);
			}
		}
		last = tmp;
	}
	return( NULL );
}

void add_to_whowas_chan_buffer(ChannelList *channel)
{
	WhowasChanList *new;
	WhowasChanList **slot;

	if (whowas_chan_count >= WHOWAS_CHAN_MAX) 
	{
		whowas_chan_count -=
		   remove_oldest_chan_whowas(&whowas_chan_list, 0,
		   (whowas_chan_count + 1) - WHOWAS_CHAN_MAX); 
	}
	new = (WhowasChanList *) new_malloc(sizeof(WhowasChanList));

	new->channellist = channel;
	new->time = now;
	clear_nicklist_hashtable(channel);
	/* we've created it, now put it in order */
	for (slot = &whowas_chan_list; *slot; slot = &(*slot)->next)
	{
		if ((*slot)->time > new->time)
			break;
	}
	new->next = *slot;
	*slot = new;
	whowas_chan_count++;
}

int remove_oldest_chan_whowas(WhowasChanList **list, time_t timet, int count)
{
	WhowasChanList *tmp = NULL;
	time_t t;
	int total = 0;

	/* if no ..count.. then remove ..time.. links */
	if (!count) 
	{
		t = now;
		while (*list && ((*list)->time + timet) <= t) 
		{
			tmp = *list;
			new_free(&(tmp->channellist->channel));
			new_free(&(tmp->channellist->topic));
			new_free(&(tmp->channellist->modelock_key));
			clear_bans(tmp->channellist);
			if (tmp->channellist->msglog_fp)
				fclose(tmp->channellist->msglog_fp);
			remove_csets_for_channel(tmp->channellist->csets);
			new_free((char **)&(tmp->channellist));
			*list = tmp->next;
			new_free((char **)&tmp);
			total++;
		}
	}
	else 
	{
		while (*list && count) 
		{
			tmp = *list;
			new_free(&(tmp->channellist->channel));
			new_free(&(tmp->channellist->topic));
			new_free(&(tmp->channellist->modelock_key));
			clear_bans(tmp->channellist);
			if (tmp->channellist->msglog_fp)
				fclose(tmp->channellist->msglog_fp);
			remove_csets_for_channel(tmp->channellist->csets);
			new_free((char **)&(tmp->channellist));
			*list = tmp->next;
			new_free((char **)&tmp);
			total++;
			count--;
		}
	}
	return total;
}

void clean_whowas_chan_list(void)
{
   whowas_chan_count -= remove_oldest_chan_whowas(&whowas_chan_list, 24 * 60 * 60, 0);
}

void clear_whowas(void)
{
	whowas_chan_count -= remove_oldest_chan_whowas(&whowas_chan_list, 0, 0);
	if (whowas_userlist_count)
		whowas_userlist_count -= remove_oldest_whowas_hashlist(&whowas_userlist_list, 0, 0);
	if (whowas_reg_count)
		whowas_reg_count -= remove_oldest_whowas_hashlist(&whowas_reg_list, 0, 0);
	remove_oldest_whowas_hashlist(&whowas_splitin_list, 0 , 0);
}

void show_whowas(void)
{
	reset_display_target();
	show_whowas_hashtable(&whowas_userlist_list, "Userlist");
	show_whowas_hashtable(&whowas_reg_list, "Reglist");
}

void show_wholeft(char *channel)
{
int count = 0;
int hook = 0;
time_t ltime = now;
	reset_display_target();
#if 0
	hook = show_wholeft_hashtable(&whowas_splitin_list, ltime, &count, &hook, "SplitList");
#endif
	hook = show_wholeft_hashtable(&whowas_userlist_list, ltime, &count, &hook, "Splitin");
	hook = show_wholeft_hashtable(&whowas_reg_list, ltime, &count, &hook, "Splitin");
	if (count && hook && fget_string_var(FORMAT_WHOLEFT_FOOTER_FSET))
		put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOLEFT_FOOTER_FSET), NULL));
}
