/************
 *  hash.c  *
 ************
 *
 * My hash routines for hashing NickLists, and eventually ChannelList's
 * and WhowasList's
 *
 * These are not very robust, as the add/remove functions will have
 * to be written differently for each type of struct
 * (To DO: use C++, and create a hash "class" so I don't need to
 * have the functions different.)
 *
 *
 * Written by Scott H Kilau
 *
 * Copyright(c) 1997
 *
 * Modified by Colten Edwards for use in BitchX.
 * Added Whowas buffer hashing.
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 */

#include "irc.h"
static char cvsrevision[] = "$Id: hash.c 52 2008-06-14 06:45:05Z keaston $";
CVS_REVISION(hash_c)
#include "struct.h"
#include "ircaux.h"
#include "hook.h"
#include "vars.h"
#include "output.h"
#include "misc.h"
#include "server.h"
#include "list.h"
#include "window.h"

#include "hash.h"
#include "hash2.h"
#define MAIN_SOURCE
#include "modval.h"

/*
 * hash_nickname: for now, does a simple hash of the 
 * nick by counting up the ascii values of the lower case, and 
 * then %'ing it by NICKLIST_HASHSIZE (always a prime!)
 */
unsigned long hash_nickname(char *nick, unsigned int size)
{
        register u_char  *p = (u_char *) nick;
	unsigned long hash = 0, g;
	if (!nick) return -1;
	while (*p)
	{
		hash = (hash << 4) + ((*p >= 'A' && *p <= 'Z') ? (*p+32) : *p);
		if ((g = hash & 0xF0000000))
			hash ^= g >> 24;
		hash &= ~g;
		p++;
	}
	return (hash %= size);
}

/*
 * move_link_to_top: used by find routine, brings link
 * to the top of the list in the specific array location
 */
static inline void move_link_to_top(NickList *tmp, NickList *prev, HashEntry *location)
{
	if (prev) 
	{
		NickList *old_list;
		old_list = (NickList *) location->list;
		location->list = (void *) tmp;
		prev->next = tmp->next;
		tmp->next = old_list;
	}
}

/*
 * remove_link_from_list: used by find routine, removes link
 * from our chain of hashed entries.
 */
static inline void remove_link_from_list(NickList *tmp, NickList *prev, HashEntry *location)
{
	if (prev) 
	{
		/* remove the link from the middle of the list */
		prev->next = tmp->next;
	}
	else {
		/* unlink the first link, and connect next one up */
		location->list = (void *) tmp->next;
	}
	/* set tmp's next to NULL, as its unlinked now */
	tmp->next = NULL;
}

void BX_add_name_to_genericlist(char *name, HashEntry *list, unsigned int size)
{
	List *nptr;
	unsigned long hvalue = hash_nickname(name, size);

	nptr = (List *) new_malloc(sizeof(List));
	nptr->next = (List *) list[hvalue].list;
	nptr->name = m_strdup(name);

	/* assign our new linked list into array spot */
	list[hvalue].list = (void *) nptr;
	/* quick tally of nicks in chain in this array spot */
	list[hvalue].links++;
	/* keep stats on hits to this array spot */
	list[hvalue].hits++;
}

/*
 * move_link_to_top: used by find routine, brings link
 * to the top of the list in the specific array location
 */
static inline void move_gen_link_to_top(List *tmp, List *prev, HashEntry *location)
{
	if (prev) 
	{
		List *old_list;
		old_list = (List *) location->list;
		location->list = (void *) tmp;
		prev->next = tmp->next;
		tmp->next = old_list;
	}
}

/*
 * remove_link_from_list: used by find routine, removes link
 * from our chain of hashed entries.
 */
static inline void remove_gen_link_from_list(List *tmp, List *prev, HashEntry *location)
{
	if (prev) 
	{
		/* remove the link from the middle of the list */
		prev->next = tmp->next;
	}
	else {
		/* unlink the first link, and connect next one up */
		location->list = (void *) tmp->next;
	}
	/* set tmp's next to NULL, as its unlinked now */
	tmp->next = NULL;
}

List *BX_find_name_in_genericlist(char *name, HashEntry *list, unsigned int size, int remove)
{
	HashEntry *location;
	register List *tmp, *prev = NULL;
	unsigned long hvalue = hash_nickname(name, size);

	location = &(list[hvalue]);

	/* at this point, we found the array spot, now search
	 * as regular linked list, or as ircd likes to say...
	 * "We found the bucket, now search the chain"
	 */
	for (tmp = (List *) location->list; tmp; prev = tmp, tmp = tmp->next) 
	{
		if (!my_stricmp(name, tmp->name)) 
		{
			if (remove != REMOVE_FROM_LIST)
				move_gen_link_to_top(tmp, prev, location);
			else 
			{
				location->links--;
				remove_gen_link_from_list(tmp, prev, location);
			}
			return tmp;
		}
	}
	return NULL;
}

/*
 * add_nicklist_to_channellist: This function will add the nicklist
 * into the channellist, ensuring that we hash the nicklist, and
 * insert the struct correctly into the channelist's Nicklist hash
 * array
 */
void BX_add_nicklist_to_channellist(NickList *nptr, ChannelList *cptr)
{
	unsigned long hvalue = hash_nickname(nptr->nick, NICKLIST_HASHSIZE);

	/* take this nicklist, and attach it as the HEAD pointer
	 * in our chain at the hashed location in our array...
	 * Note, by doing this, this ensures that the "most active"
	 * users always remain at the top of the chain... ie, faster
	 * lookups for active users, (and as a side note, makes
	 * doing the add quite simple!)
	 */
	nptr->next = (NickList *) cptr->NickListTable[hvalue].list;

	/* assign our new linked list into array spot */
	cptr->NickListTable[hvalue].list = (void *) nptr;
	/* quick tally of nicks in chain in this array spot */
	cptr->NickListTable[hvalue].links++;
	/* keep stats on hits to this array spot */
	cptr->NickListTable[hvalue].hits++;
}

NickList *BX_find_nicklist_in_channellist(char *nick, ChannelList *cptr, int remove)
{
	HashEntry *location;
	register NickList *tmp, *prev = NULL;
	unsigned long hvalue = hash_nickname(nick, NICKLIST_HASHSIZE);

	if (!cptr)
		return NULL;
	location = &(cptr->NickListTable[hvalue]);

	/* at this point, we found the array spot, now search
	 * as regular linked list, or as ircd likes to say...
	 * "We found the bucket, now search the chain"
	 */
	for (tmp = (NickList *) location->list; tmp; prev = tmp, tmp = tmp->next) 
	{
		if (!my_stricmp(nick, tmp->nick)) 
		{
			if (remove != REMOVE_FROM_LIST)
				move_link_to_top(tmp, prev, location);
			else 
			{
				location->links--;
				remove_link_from_list(tmp, prev, location);
			}
			return tmp;
		}
	}
	return NULL;
}

/*
 * Basically this makes the hash table "look" like a straight linked list
 * This should be used for things that require you to cycle through the
 * full list, ex. for finding ALL matching stuff.
 * : usage should be like :
 *
 *	for (nptr = next_nicklist(cptr, NULL); nptr; nptr = 
 *	     next_nicklist(cptr, nptr))
 *		YourCodeOnTheNickListStruct
 */
NickList *BX_next_nicklist(ChannelList *cptr, NickList *nptr)
{
	unsigned long hvalue = 0;
	if (!cptr) 
		/* No channel! */
		return NULL;
	else if (!nptr) 
	{
		/* wants to start the walk! */
		while ((NickList *) cptr->NickListTable[hvalue].list == NULL) 
		{
			hvalue++;
			if (hvalue >= NICKLIST_HASHSIZE)
				return NULL;
		}
		return (NickList *) cptr->NickListTable[hvalue].list;
	}
	else if (nptr->next) 
	{
		/* still returning a chain! */
		return nptr->next;
	}
	else if (!nptr->next) 
	{
		int hvalue;
		/* hit end of chain, go to next bucket */
		hvalue = hash_nickname(nptr->nick, NICKLIST_HASHSIZE) + 1;
		if (hvalue >= NICKLIST_HASHSIZE) 
		{
			/* end of list */
			return NULL;
		}
		else 
		{
			while ((NickList *) cptr->NickListTable[hvalue].list == NULL) 
			{
				hvalue++;
				if (hvalue >= NICKLIST_HASHSIZE)
					return NULL;
			}
			/* return head of next filled bucket */
                        return (NickList *) cptr->NickListTable[hvalue].list;
		}
	}
	else 
		/* shouldn't ever be here */
		say ("HASH_ERROR: next_nicklist");
	return NULL;
}

List *BX_next_namelist(HashEntry *cptr, List *nptr, unsigned int size)
{
	unsigned long hvalue = 0;
	if (!cptr) 
		/* No channel! */
		return NULL;
	else if (!nptr) 
	{
		/* wants to start the walk! */
		while ((List *) cptr[hvalue].list == NULL) 
		{
			hvalue++;
			if (hvalue >= size)
				return NULL;
		}
		return (List *) cptr[hvalue].list;
	}
	else if (nptr->next) 
	{
		/* still returning a chain! */
		return nptr->next;
	}
	else if (!nptr->next) 
	{
		int hvalue;
		/* hit end of chain, go to next bucket */
		hvalue = hash_nickname(nptr->name, size) + 1;
		if (hvalue >= size) 
		{
			/* end of list */
			return NULL;
		}
		else 
		{
			while ((List *) cptr[hvalue].list == NULL) 
			{
				hvalue++;
				if (hvalue >= size)
					return NULL;
			}
			/* return head of next filled bucket */
                        return (List *) cptr[hvalue].list;
		}
	}
	else 
		/* shouldn't ever be here */
		say ("HASH_ERROR: next_namelist");
	return NULL;
}

void clear_nicklist_hashtable(ChannelList *cptr)
{
	if (cptr) 
	{
		memset((char *) cptr->NickListTable, 0, 
		   sizeof(HashEntry) * NICKLIST_HASHSIZE);
	}
}


void show_nicklist_hashtable(ChannelList *cptr)
{
        int count, count2;
        NickList *ptr;

        for (count = 0; count < NICKLIST_HASHSIZE; count++) 
        {
		if (cptr->NickListTable[count].links == 0)
			continue;
                say("HASH DEBUG: %d   links %d   hits %d",
                    count,
                    cptr->NickListTable[count].links,
                    cptr->NickListTable[count].hits);

                for (ptr = (NickList *) cptr->NickListTable[count].list,
                    count2 = 0; ptr; count2++, ptr = ptr->next) 
                {
                        say("HASH_DEBUG: %d:%d  %s!%s", count, count2,
                            ptr->nick, ptr->host);
                }
        }
}

void show_whowas_debug_hashtable(WhowasWrapList *cptr)
{
        int count, count2;
        WhowasList *ptr;

        for (count = 0; count < WHOWASLIST_HASHSIZE; count++) 
        {
		if (cptr->NickListTable[count].links == 0)
			continue;
                say("HASH DEBUG: %d   links %d   hits %d",
                    count,
                    cptr->NickListTable[count].links,
                    cptr->NickListTable[count].hits);

                for (ptr = (WhowasList *) cptr->NickListTable[count].list,
                    count2 = 0; ptr; count2++, ptr = ptr->next) 
                {
                        say("HASH_DEBUG: %d:%d  %10s %s!%s", count, count2,
                            ptr->channel, ptr->nicklist->nick, ptr->nicklist->host);
                }
        }
}

BUILT_IN_COMMAND(show_hash)
{
char *c;
ChannelList *chan = NULL, *chan2;
extern int from_server;
extern WhowasWrapList whowas_userlist_list;
extern WhowasWrapList whowas_reg_list;
extern WhowasWrapList whowas_splitin_list;
	if (args && *args)
		c = next_arg(args, &args);
	else
		c = get_current_channel_by_refnum(0);
	if (c && from_server > -1)
	{
		chan2 = get_server_channels(from_server);
		chan = (ChannelList *)find_in_list((List **)&chan2, c, 0);
	}
	if (chan)
		show_nicklist_hashtable(chan);
	show_whowas_debug_hashtable(&whowas_userlist_list);
	show_whowas_debug_hashtable(&whowas_reg_list);
	show_whowas_debug_hashtable(&whowas_splitin_list);
}

/*
 * the following routines are written by Colten Edwards (panasync)
 * to hash the whowas lists that the client keeps.
 */

static unsigned long hash_userhost_channel(char *userhost, char *channel, unsigned int size)
{
	register const unsigned char *p = (const unsigned char *)userhost;
	unsigned long g, hash = 0;
	if (!userhost) return -1;
	while (*p)
	{
		hash = (hash << 4) + ((*p >= 'A' && *p <= 'Z') ? (*p+32) : *p);
		if ((g = hash & 0xF0000000))
			hash ^= g >> 24;
		hash &= ~g;
		p++;
	}
	p = (const unsigned char *)channel;
	if (p)
	{
		while (*p)
		{
			if (*p == ',')
				return -1;
			hash = (hash << 4) + ((*p >= 'A' && *p <= 'Z') ? (*p+32) : *p);
			if ((g = hash & 0xF0000000))
				hash ^= g >> 24;
			hash &= ~g;
			p++;
		}
	}
	return (hash % size);
}

/*
 * move_link_to_top: used by find routine, brings link
 * to the top of the list in the specific array location
 */
static inline void move_link_to_top_whowas(WhowasList *tmp, WhowasList *prev, HashEntry *location)
{
	if (prev) 
	{
		WhowasList *old_list;
		old_list = (WhowasList *) location->list;
		location->list = (void *) tmp;
		prev->next = tmp->next;
		tmp->next = old_list;
	}
}

/*
 * remove_link_from_list: used by find routine, removes link
 * from our chain of hashed entries.
 */
static inline void remove_link_from_whowaslist(WhowasList *tmp, WhowasList *prev, HashEntry *location)
{
	if (prev) 
	{
		/* remove the link from the middle of the list */
		prev->next = tmp->next;
	}
	else {
		/* unlink the first link, and connect next one up */
		location->list = (void *) tmp->next;
	}
	/* set tmp's next to NULL, as its unlinked now */
	tmp->next = NULL;
}

/*
 * add_nicklist_to_channellist: This function will add the nicklist
 * into the channellist, ensuring that we hash the nicklist, and
 * insert the struct correctly into the channelist's Nicklist hash
 * array
 */
void BX_add_whowas_userhost_channel(WhowasList *wptr, WhowasWrapList *list)
{
	unsigned long hvalue = hash_userhost_channel(wptr->nicklist->host, wptr->channel, WHOWASLIST_HASHSIZE);

	/* take this nicklist, and attach it as the HEAD pointer
	 * in our chain at the hashed location in our array...
	 * Note, by doing this, this ensures that the "most active"
	 * users always remain at the top of the chain... ie, faster
	 * lookups for active users, (and as a side note, makes
	 * doing the add quite simple!)
	 */
	wptr->next = (WhowasList *) list->NickListTable[hvalue].list;

	/* assign our new linked list into array spot */
	list->NickListTable[hvalue].list = (void *) wptr;
	/* quick tally of nicks in chain in this array spot */
	list->NickListTable[hvalue].links++;
	/* keep stats on hits to this array spot */
	list->NickListTable[hvalue].hits++;
	list->total_links++;
}

WhowasList *BX_find_userhost_channel(char *host, char *channel, int remove, WhowasWrapList *wptr)
{
	HashEntry *location;
	register WhowasList *tmp, *prev = NULL;
	unsigned long hvalue;

	hvalue = hash_userhost_channel(host, channel, WHOWASLIST_HASHSIZE);
	location = &(wptr->NickListTable[hvalue]);

	/* at this point, we found the array spot, now search
	 * as regular linked list, or as ircd likes to say...
	 * "We found the bucket, now search the chain"
	 */
	for (tmp = (WhowasList *) (&(wptr->NickListTable[hvalue]))->list; tmp; prev = tmp, tmp = tmp->next) 
	{
		if (!tmp->nicklist->host || !tmp->channel || !host || !channel)
			continue;
		if (!my_stricmp(host, tmp->nicklist->host) && !my_stricmp(channel, tmp->channel))
		{
			if (remove != REMOVE_FROM_LIST)
				move_link_to_top_whowas(tmp, prev, location);
			else 
			{
				location->links--;
				remove_link_from_whowaslist(tmp, prev, location);
				wptr->total_unlinks++;
			}
			wptr->total_hits++;
			return tmp;
		}
	}
	return NULL;
}

/*
 * Basically this makes the hash table "look" like a straight linked list
 * This should be used for things that require you to cycle through the
 * full list, ex. for finding ALL matching stuff.
 * : usage should be like :
 *
 *	for (nptr = next_userhost(cptr, NULL); nptr; nptr = 
 *	     next_userhost(cptr, nptr))
 *		YourCodeOnTheWhowasListStruct
 */
WhowasList *BX_next_userhost(WhowasWrapList *cptr, WhowasList *nptr)
{
	unsigned long hvalue = 0;
	if (!cptr) 
		/* No channel! */
		return NULL;
	else if (!nptr) 
	{
		/* wants to start the walk! */
		while ((WhowasList *) cptr->NickListTable[hvalue].list == NULL) 
		{
			hvalue++;
			if (hvalue >= WHOWASLIST_HASHSIZE)
				return NULL;
		}
		return (WhowasList *) cptr->NickListTable[hvalue].list;
	}
	else if (nptr->next) 
	{
		/* still returning a chain! */
		return nptr->next;
	}
	else if (!nptr->next) 
	{
		int hvalue;
		/* hit end of chain, go to next bucket */
		hvalue = hash_userhost_channel(nptr->nicklist->host, nptr->channel, WHOWASLIST_HASHSIZE) + 1;
		if (hvalue >= WHOWASLIST_HASHSIZE) 
		{
			/* end of list */
			return NULL;
		}
		else 
		{
			while ((WhowasList *) cptr->NickListTable[hvalue].list == NULL) 
			{
				hvalue++;
				if (hvalue >= WHOWASLIST_HASHSIZE)
					return NULL;
			}
			/* return head of next filled bucket */
                        return (WhowasList *) cptr->NickListTable[hvalue].list;
		}
	}
	else 
		/* shouldn't ever be here */
		say ("WHOWAS_HASH_ERROR: next_userhost");
	return NULL;
}

void show_whowas_hashtable(WhowasWrapList *cptr, char *list)
{
        int count, count2 = 1;
        WhowasList *ptr;
	
	say("WhoWas %s Cache Stats: %lu hits  %lu  links  %lu unlinks", list, cptr->total_hits, cptr->total_links, cptr->total_unlinks);
        for (count = 0; count < WHOWASLIST_HASHSIZE; count++) 
        {
		
		if (cptr->NickListTable[count].links == 0)
			continue;
                for (ptr = (WhowasList *) cptr->NickListTable[count].list; ptr; count2++, ptr = ptr->next) 
			put_it("%s", convert_output_format("%K[%W$[3]0%K] %Y$[10]1 %W$2%G!%c$3", "%d %s %s %s", count2, ptr->channel, ptr->nicklist->nick, ptr->nicklist->host));
        }
}

int show_wholeft_hashtable(WhowasWrapList *cptr, time_t ltime, int *total, int *hook, char *list)
{
        int count, count2;
        WhowasList *ptr;

        for (count = 0; count < WHOWASLIST_HASHSIZE; count++) 
        {
		
		if (cptr->NickListTable[count].links == 0)
			continue;
                for (ptr = (WhowasList *) cptr->NickListTable[count].list, count2 = 1; ptr; count2++, ptr = ptr->next) 
		{
			if (ptr->server1/* && ptr->server2*/)
			{
				if (!(*total)++ && (*hook = do_hook(WHOLEFT_HEADER_LIST, "%s %s %s %s %s %s", "Nick", "Host", "Channel", "Time", "Server", "Server")))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOLEFT_HEADER_FSET), NULL));
				if (do_hook(WHOLEFT_LIST, "%s %s %s %ld %s %s", ptr->nicklist->nick, ptr->nicklist->host, ptr->channel, ltime-ptr->time, ptr->server1?ptr->server1:"Unknown", ptr->server2?ptr->server2:"Unknown"))
					put_it("%s", convert_output_format(fget_string_var(FORMAT_WHOLEFT_USER_FSET), "%s %s %s %l %s", ptr->nicklist->nick, ptr->nicklist->host, ptr->channel, (long)ltime-ptr->time, ptr->server1?ptr->server1:empty_string));
			}
		}
        }
	if (*total)
		do_hook(WHOLEFT_FOOTER_LIST, "%s", "End of WhoLeft");
	return *hook;
}

int BX_remove_oldest_whowas_hashlist(WhowasWrapList *list, time_t timet, int count)
{
WhowasList *ptr;
int total = 0;
register unsigned long x;
	if (!count)
	{
		for (x = 0; x < WHOWASLIST_HASHSIZE; x++)
		{
			ptr = (WhowasList *) (&(list->NickListTable[x]))->list;
			if (!ptr || !ptr->nicklist)
				continue;
			while (ptr)
			{
				if ((ptr->time + timet) <= now)
				{
					if (!(ptr = find_userhost_channel(ptr->nicklist->host, ptr->channel, 1, list)))
						break;
					new_free(&(ptr->nicklist->ip));
					new_free(&(ptr->nicklist->nick));
					new_free(&(ptr->nicklist->host));
					new_free(&(ptr->nicklist->server));
					new_free((char **)&(ptr->nicklist));
					new_free(&(ptr->channel));
					new_free(&(ptr->server1));
					new_free(&(ptr->server2));
					new_free((char **)&ptr);
					total++;
					ptr = (WhowasList *) (&(list->NickListTable[x]))->list;
				} else ptr = ptr->next;
			}
		}
	}
	else
	{
		while((ptr = next_userhost(list, NULL)) && count)
		{
			x = hash_userhost_channel(ptr->nicklist->host, ptr->channel, WHOWASLIST_HASHSIZE);
			if (!(ptr = find_userhost_channel(ptr->nicklist->host, ptr->channel, 1, list)))
				break;
			if (ptr->nicklist)
			{
				new_free(&(ptr->nicklist->ip));
				new_free(&(ptr->nicklist->nick));
				new_free(&(ptr->nicklist->host));
				new_free(&(ptr->nicklist->server));
				new_free((char **)&(ptr->nicklist));
			}
			new_free(&(ptr->channel));
			new_free(&(ptr->server1));
			new_free(&(ptr->server2));
			new_free((char **)&ptr);
			total++; count--;			
		}
	}
	return total;
}

int cmp_host (List *a, List *b)
{
NickList *a1 = (NickList *)a, *b1 = (NickList *)b;
	return strcmp(a1->host, b1->host);
}

int cmp_time (List *a, List *b)
{
NickList *a1 = (NickList *)a, *b1 = (NickList *)b;
	if (a1->idle_time > b1->idle_time)
		return -1;
	if (a1->idle_time < b1->idle_time)
		return 1;
	return strcmp(a1->nick, b1->nick);
}


int cmp_ip (List *a, List *b)
{
NickList *a1 = (NickList *)a, *b1 = (NickList *)b;
unsigned long at, bt;
	if (!a1->ip && !b1->ip)
		return -1;
/*		return strcmp(a1->nick, b1->nick);*/
	if (!a1->ip)
		return -1;
	if (!b1->ip)
		return 1;
	at = inet_addr(a1->ip); bt = inet_addr(b1->ip);
	if (at < bt)
		return 1;
	if (at > bt)
		return -1;
	return strcmp(a1->nick, b1->nick);
}

/* Compare two Nicks by channel status, chanop > halfop > voice */
int cmp_stat (List *a, List *b)
{
	NickList *a1 = (NickList *)a, *b1 = (NickList *)b;
	int a_status = 
		nick_isop(a1) ? 0 : nick_ishalfop(a1) ? 1 : nick_isvoice(a1) ? 2 : 3;
	int b_status = 
		nick_isop(b1) ? 0 : nick_ishalfop(b1) ? 1 : nick_isvoice(b1) ? 2 : 3;
	int cmp;

	cmp = a_status - b_status;

	/* Equal status */
	if (cmp == 0)
		cmp = strcmp(a1->nick, b1->nick);

	return cmp;
}

/* Determines if the Nick matches the nick!user@host mask given. */
int nick_match(NickList *nick, char *mask)
{
    int match = 0;
    char *nuh = m_3dup(nick->nick, "!", nick->host);

    match = wild_match(mask, nuh);
    new_free(&nuh);

    return match;
}
 
NickList *BX_sorted_nicklist(ChannelList *chan, int sort)
{
	NickList *tmp, *l = NULL, *list = NULL, *last = NULL;
	for (tmp = next_nicklist(chan, NULL); tmp; tmp = next_nicklist(chan, tmp))
	{
		l = (NickList *)new_malloc(sizeof(NickList));
		memcpy(l, tmp, sizeof(NickList));
		l->next = NULL;
		switch(sort)
		{
			case NICKSORT_HOST:
				add_to_list_ext((List **)&list, (List *)l, cmp_host);
				break;
			case NICKSORT_STAT:
				add_to_list_ext((List **)&list, (List *)l, cmp_stat);
				break;
			case NICKSORT_TIME:
				add_to_list_ext((List **)&list, (List *)l, cmp_time);
				break;
			case NICKSORT_IP:
				add_to_list_ext((List **)&list, (List *)l, cmp_ip);
				break;
			case NICKSORT_NONE:
				if (last)
					last->next = l;
				else
					list = l;
				break;
			default:
			case NICKSORT_NICK:
			case NICKSORT_NORMAL:
				add_to_list((List **)&list, (List *)l);
				break;
		}
		last = l;
	}
	return list;
}

void BX_clear_sorted_nicklist(NickList **list)
{
	register NickList *t;
	while(*list)
	{
		t = (*list)->next;
		new_free((char **)&(*list));
		*list = t;
	}
}

Flooding *BX_add_name_to_floodlist(char *name, char *host, char *channel, HashEntry *list, unsigned int size)
{
	Flooding *nptr;
	unsigned long hvalue = hash_nickname(name, size);
	nptr = (Flooding *)new_malloc(sizeof(Flooding));
	nptr->next = (Flooding *) list[hvalue].list;
	nptr->name = m_strdup(name);
	nptr->host = m_strdup(host);
	list[hvalue].list = (void *) nptr;
	/* quick tally of nicks in chain in this array spot */
	list[hvalue].links++;
	/* keep stats on hits to this array spot */
	list[hvalue].hits++;
	return nptr;
}

Flooding *BX_find_name_in_floodlist(char *name, char *host, HashEntry *list, unsigned int size, int remove)
{
	HashEntry *location;
	register Flooding *tmp, *prev = NULL;
	unsigned long hvalue = hash_nickname(name, size);

	location = &(list[hvalue]);

	/* at this point, we found the array spot, now search
	 * as regular linked list, or as ircd likes to say...
	 * "We found the bucket, now search the chain"
	 */
	for (tmp = (Flooding *) location->list; tmp; prev = tmp, tmp = tmp->next) 
	{
		if (!my_stricmp(name, tmp->name)) 
		{
			if (remove != REMOVE_FROM_LIST)
				move_gen_link_to_top((List *)tmp, (List *)prev, location);
			else 
			{
				location->links--;
				remove_gen_link_from_list((List *)tmp, (List *)prev, location);
			}
			return tmp;
		}
	}
	return NULL;
}

