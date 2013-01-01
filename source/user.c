
#include "irc.h"
static char cvsrevision[] = "$Id: user.c 3 2008-02-25 09:49:14Z keaston $";
CVS_REVISION(user_c)
#include "struct.h"

#include "ircaux.h"
#include "list.h"
#include "hash.h"
#include "struct.h"
#include "userlist.h"
#include "misc.h"
#include "output.h"
#define MAIN_SOURCE
#include "modval.h"

#define USERHOST_HASHSIZE 501
#define USERCHAN_HASHSIZE 20

#ifdef WANT_USERLIST
HashEntry UserListByHost_Table[USERHOST_HASHSIZE];
HashEntry UserListByChannel_Table[USERCHAN_HASHSIZE];

UserList *user_list = NULL;

static long hash_name(char *str, unsigned int size)
{
	const unsigned char *p = (const unsigned char *)str;
	unsigned long g, hash = 0;
	if (!p) return -1;
	while (*p)
	{
		if (*p == '*' || *p =='?' || *p == ',')
			return -1;
		hash = (hash << 4) + ((*p >= 'A' && *p <= 'Z') ? (*p+32) : *p);
		if ((g = hash & 0xF0000000))
			hash ^= g >> 24;
		hash &= ~g;
		p++;
	}
	return (hash % size);
}

  /* userlist entry can be put into our tier 1 */
static inline void add_userlist_to_tier_1(UserList *ptr, unsigned long hvalue)
{
        ptr->next = (UserList *) UserListByHost_Table[hvalue].list;
        /* assign our new linked list into array spot */
        UserListByHost_Table[hvalue].list = (void *) ptr;
        /* quick tally of userlists in chain in this array spot */
        UserListByHost_Table[hvalue].links++;
}

static inline void add_userlist_to_tier_2(UserList *ptr, unsigned long hvalue)
{
	ptr->next = (UserList *) UserListByChannel_Table[hvalue].list;
	/* assign our new linked list into array spot */
	UserListByChannel_Table[hvalue].list = (void *) ptr;
	/* quick tally of userlists in chain in this array spot */
	UserListByChannel_Table[hvalue].links++;
}

static inline void add_userlist_to_tier_3(UserList *uptr)
{
	add_to_list((List **)&user_list, (List *)uptr);
}

void add_userlist(UserList *uptr)
{
char *ptr;
long hvalue;
	if (!(ptr = strrchr(uptr->host, '@')))
	{
		add_userlist_to_tier_3(uptr);
		return;
	}
	if ((hvalue = hash_name(ptr, USERHOST_HASHSIZE)) != -1)
	{
		add_userlist_to_tier_1(uptr, hvalue);
		return;
	}
#if 0
	if (!strchr(uptr->channels, '*'))
	{
		char *channel, *ptr;
		channel = alloca(strlen(uptr->channels)+1);
		strcpy(channel, uptr->channels);
		while ((ptr = next_in_comma_list(channel, &channel)))
		{
			if (!*ptr)
				return;
			hvalue = hash_name(ptr, USERCHAN_HASHSIZE);
			add_userlist_to_tier_2(uptr, hvalue);
			if (channel && *channel)
			{
				UserList *newptr;
				newptr = new_malloc(sizeof(UserList));
				memcpy(newptr, uptr, sizeof(UserList));
				uptr = newptr;
			}
		}
	}
#else
	if ((hvalue = hash_name(uptr->channels, USERCHAN_HASHSIZE)) != -1)
	{
		add_userlist_to_tier_2(uptr, hvalue);
		return;
	}
#endif
	add_userlist_to_tier_3(uptr);
}

/*
 * move_link_to_top: used by find routine, brings link
 * to the top of the list in the specific array location
 */
static inline void move_link_to_top(UserList *tmp, UserList *prev, HashEntry *location)
{
	if (prev) 
	{
		UserList *old_list;
		old_list = (UserList *) location->list;
		location->list = (void *) tmp;
		prev->next = tmp->next;
		tmp->next = old_list;
	}
}

/*
 * remove_link_from_list: used by find routine, removes link
 * from our chain of hashed entries.
 */
static inline void remove_link_from_list(UserList *tmp, UserList *prev, HashEntry *location)
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



static inline UserList *find_userlist_tier3(char *host, char *channel, int remove)
{
	register UserList *tmp;
	for (tmp = user_list; tmp; tmp = tmp->next)
	{
		if (wild_match(tmp->host, host) || !my_stricmp(tmp->host, host))
		{
			if (check_channel_match(tmp->channels, channel) || !strcmp(tmp->channels, channel))
			{
				if (remove)
					tmp = (UserList *)remove_from_list((List **)&user_list, tmp->nick);
				return tmp;
			}
		}
	}
	return NULL;
}

static inline UserList *find_userlist_tier1(char *userhost, char *channel, unsigned long hvalue, int remove)
{
HashEntry *location;
register UserList *tmp, *prev = NULL;
	location = &UserListByHost_Table[hvalue];
	for (tmp = (UserList *)location->list; tmp; prev = tmp, tmp = tmp->next)
	{
		if ((wild_match(tmp->host, userhost) ||!my_stricmp(tmp->host, userhost)) && check_channel_match(tmp->channels, channel))
		{
			if (remove)
			{
				location->links--;
				remove_link_from_list(tmp, prev, location);
			} else
				move_link_to_top(tmp, prev, location);
			location->hits++;
			return tmp;
		}
	}
	return NULL;
}

static inline UserList *find_userlist_tier2(char *userhost,unsigned long hvalue, int remove)
{
HashEntry *location;
register UserList *tmp, *prev = NULL;
	location = &UserListByChannel_Table[hvalue];
	for (tmp = (UserList *)location->list; tmp; prev = tmp, tmp = tmp->next)
	{
		if (wild_match(tmp->host, userhost) || !my_stricmp(tmp->host, userhost))
		{
			if (remove)
			{
				location->links--;
				remove_link_from_list(tmp, prev, location);
			} else
				move_link_to_top(tmp, prev, location);
			location->hits++;
			return tmp;
		}
	}
	return NULL;
}

UserList *find_userlist(char *userhost, char *channel, int remove)
{
char *ptr;
long hvalue;
/*char *host = clear_server_flags(userhost);*/
char *host = userhost;
	if (!(ptr = strrchr(userhost, '@')))
		return find_userlist_tier3(host, channel, remove);
	if ((hvalue = hash_name(ptr, USERHOST_HASHSIZE)) != -1)
	{
		UserList *tmp;
		if ((tmp = find_userlist_tier1(host, channel, hvalue, remove)))		
			return tmp;
	}	
	if ((hvalue = hash_name(channel, USERCHAN_HASHSIZE)) != -1)
	{
		UserList *tmp;
		if ((tmp = find_userlist_tier2(host, hvalue, remove)))
			return tmp;
	}	
	/* if we got here this is shitty */
	return find_userlist_tier3(host, channel, remove);
}

UserList *create_sorted_userlist(void)
{

	UserList *tmp = NULL, *t, *new;
	int i;
	for (i = 0; i < USERHOST_HASHSIZE; i++)
	{
		t = (UserList *)UserListByHost_Table[i].list;
		while (t)
		{
			new = new_malloc(sizeof(UserList));
			memcpy(new, t, sizeof(UserList));
			add_to_list((List **)&tmp, (List *)new);
			t = t->next;
		}
	}
	for (i = 0; i < USERCHAN_HASHSIZE; i++)
	{
		t = (UserList *)UserListByChannel_Table[i].list;
		while (t)
		{
			new = new_malloc(sizeof(UserList));
			memcpy(new, t, sizeof(UserList));
			add_to_list((List **)&tmp, (List *)new);
			t = t->next;
		}
	}
	for (t = user_list; t; t = t->next)
	{
		new = new_malloc(sizeof(UserList));
		memcpy(new, t, sizeof(UserList));
		add_to_list((List **)&tmp, (List *)new);
	}
	return tmp;
}

void destroy_sorted_userlist(UserList **uptr)
{
UserList *t;
	if (!uptr || !*uptr)
		return;
	while (*uptr)
	{
		t = (*uptr)->next;
		new_free(uptr);
		*uptr = t;
	}
}

#if 0
UserHist *next_userlist(UserList *uptr, int *size)
{
	long hvalue = -1;
	static HashEntry *location = NULL;
	
	if (!uptr)
	{
		location = &UserListByHost_Table[0];
		*size = USERHOST_HASHSIZE;
		hvalue = 0;
		while(((UserList *)location[hvalue].list) == NULL)
		{
			hvalue++;
			if (hvalue == USERHOST_HASHSIZE && *size == USERHOST_HASHSIZE)
			{
				location = &UserListByChannel_Table[0];
				hvalue = 0;
				*size = USERCHAN_HASHSIZE;
			}
			else if (hvalue == USERCHAN_HASHSIZE && *size == USERCHAN_HASHSIZE)
			{
				hvalue = -1;
				break;
			}
		}		
		if (hvalue != -1 && location[hvalue].list != NULL)
			return ((UserHist *)location[hvalue].list);
		location = NULL;		
		*size = -1;
		return (user_list);
	}
	if (uptr->next)
		return uptr->next;
	else if (location && !uptr->next)
	{
		if (*size > -1 && location)
		{
			if (*size == USERHOST_HASHSIZE)
				hvalue = hash_name(strrchr(uptr->host, '@'), *size);
			else if (*size == USERCHAN_HASHSIZE)
				hvalue = hash_name(uptr->channels, *size);
			if (hvalue == -1)
			{
				location = NULL;
				*size = -1;
				return user_list;
			}
			hvalue++;
			if (*size == USERHOST_HASHSIZE && hvalue >= USERHOST_HASHSIZE)
			{
				/* end of current list */
				location = &UserListByChannel_Table[0];
				*size = USERCHAN_HASHSIZE;
			}
			while (((UserList *)location[hvalue].list) == NULL)
			{
				hvalue++;
				if (hvalue >= USERHOST_HASHSIZE)
				{
					location = &UserListByChannel_Table[0];
					hvalue = 0;
					*size = USERCHAN_HASHSIZE;
				}
				else if (*size == USERCHAN_HASHSIZE && hvalue == USERCHAN_HASHSIZE)
					break;
			}
			if (location[hvalue].list != NULL)
				return ((UserList *)location[hvalue].list);
			location = NULL;		
			*size = -1;
			return user_list;
		}
	}	
	return NULL;
}
#endif

static inline int check_best_passwd(char *passwd, char *test)
{
	if (passwd && test)
		return !checkpass(test, passwd) ? 1 : 0;
/*		return !strcmp(passwd, test) ? 1 : 0;*/
	return 0;
}

UserList *find_bestmatch(char *nick, char *userhost, char *channel, char *passwd)
{
UserList *best = NULL;
UserList *best_passwd = NULL;
register UserList *tmp;
long hvalue = 0;
int	best_user_match = 0,
	best_chan_match = 0;
int	chan_match,
	user_match,
	passwd_match = 0;
char	*check;

	/*check = clear_server_flags(userhost);*/
	check = userhost;
	if ((hvalue = hash_name(strrchr(check, '@'), USERHOST_HASHSIZE)) != -1)
	{
		for (tmp = (UserList *)UserListByHost_Table[hvalue].list; tmp; tmp = tmp->next)
		{
			user_match = wild_match(tmp->host, check);
			chan_match = check_channel_match(tmp->channels, channel);
			if (user_match > best_user_match && chan_match > best_chan_match)
			{
				best_chan_match = chan_match;
				best_user_match = user_match;
				best = tmp;
				if ((passwd_match = check_best_passwd(tmp->password, passwd)))
					best_passwd = tmp;
			} 
			else if (best_user_match && user_match == best_user_match)
			{
				if (chan_match > best_chan_match)
				{
					best_chan_match = chan_match;
					best_user_match = user_match;
					best = tmp;
					if ((passwd_match = check_best_passwd(tmp->password, passwd)))
						best_passwd = tmp;
				}
			}
		}
	}
	if ((hvalue = hash_name(channel, USERCHAN_HASHSIZE)) != -1)
	{
		for (tmp = (UserList *)UserListByChannel_Table[hvalue].list; tmp; tmp = tmp->next)
		{
			user_match = wild_match(tmp->host, check);
			if (user_match > best_user_match)
			{
				best_user_match = user_match;
				best = tmp;
				if ((passwd_match = check_best_passwd(tmp->password, passwd)))
					best_passwd = tmp;
			}
		}
	}
	else if (hvalue == -1)
	{
		for (hvalue = 0; hvalue < USERCHAN_HASHSIZE; hvalue++)
		{
			for (tmp = (UserList *)UserListByChannel_Table[hvalue].list; tmp; tmp = tmp->next)
			{
				user_match = wild_match(tmp->host, check);
				chan_match = check_channel_match(tmp->channels, channel);
				if (user_match > best_user_match && chan_match > best_chan_match)
				{
					best_user_match = user_match;
					best_chan_match = chan_match;
					best = tmp;
					if ((passwd_match = check_best_passwd(tmp->password, passwd)))
						best_passwd = tmp;
				}
				else if (best_user_match && user_match == best_user_match)
				{
					if (chan_match > best_chan_match) 
					{
						best_chan_match = chan_match;
						best_user_match = user_match;
						best = tmp;
						if ((passwd_match = check_best_passwd(tmp->password, passwd)))
							best_passwd = tmp;
					}
				}
			}
		}
	}
	for (tmp = user_list; tmp; tmp = tmp->next) 
	{
		if ((chan_match = check_channel_match(tmp->channels, channel)) > 0) 
		{
			user_match = wild_match(tmp->host, check);
			if (user_match > best_user_match && chan_match > best_chan_match)
			{
				best_chan_match = chan_match;
				best_user_match = user_match;
				best = tmp;
				if ((passwd_match = check_best_passwd(tmp->password, passwd)))
					best_passwd = tmp;
			}
			else if (best_user_match && user_match == best_user_match)
			{
				if (chan_match > best_chan_match) 
				{
					best_chan_match = chan_match;
					best_user_match = user_match;
					best = tmp;
					if ((passwd_match = check_best_passwd(tmp->password, passwd)))
						best_passwd = tmp;
				}
			}
		}
	}
	if (passwd)
	{
		if (!best_passwd)
			return NULL;
		else if (best_passwd)
			return best_passwd;
	}
	return best;
}

BUILT_IN_COMMAND(debug_user)
{
HashEntry *location;
UserList *user;
int i;
int tier1 = 0, tier2 = 0, tier3 = 0;
	for (i = 0; i < USERHOST_HASHSIZE; i++)
	{
		user = (UserList *)UserListByHost_Table[i].list;
		location = &UserListByHost_Table[i];
		while (user)
		{
			tier1++;
			put_it("Tier1[%d] %d %d %s %s", i, location->links, location->hits, user->host, user->channels);
			user = user->next;
		}
	}
	for (i = 0; i < USERCHAN_HASHSIZE; i++)
	{
		user = (UserList *)UserListByChannel_Table[i].list;
		location = &UserListByChannel_Table[i];
		while (user)
		{
			tier2++;
			put_it("Tier2[%d] %d %d %s %s", i, location->links, location->hits, user->host, user->channels);
			user = user->next;
		}
	}
	for (tier3 = 0, user = user_list; user; user = user->next, tier3++)
		put_it("Tier3[%d] %s %s", tier3, user->host, user->channels);
	put_it("Tier1 = [%d] Tier2 = [%d] Tier3 = [%d]", tier1, tier2, tier3); 

}

UserList *next_userlist(UserList *uptr, int *size, void **location)
{
unsigned long hvalue = -1;
	/* we start iterating the list will a null pointer */
	if (!uptr)
	{
		*location = &UserListByHost_Table[0];
		*size = USERHOST_HASHSIZE;
		hvalue = 0;
		while(((UserList *)((HashEntry*)*location)[hvalue].list) == NULL)
		{
			hvalue++;
			if (hvalue == USERHOST_HASHSIZE && *size == USERHOST_HASHSIZE)
			{
				*location = &UserListByChannel_Table[0];
				hvalue = 0;
				*size = USERCHAN_HASHSIZE;
			}
			else if (hvalue == USERCHAN_HASHSIZE && *size == USERCHAN_HASHSIZE)
			{
				hvalue = -1;
				break;
			}
		}		
		if (hvalue != -1 && ((HashEntry *)*location)[hvalue].list != NULL)
			return ((UserList *)((HashEntry *)*location)[hvalue].list);
		*location = NULL;		
		*size = -1;
		return (user_list);
	}
	else if (uptr->next)
		return uptr->next;
	else if (*location)
	{
		if (*size > -1)
		{
			if (*size == USERHOST_HASHSIZE)
				hvalue = hash_name(strrchr(uptr->host, '@'), *size);
			else if (*size == USERCHAN_HASHSIZE)
				hvalue = hash_name(uptr->channels, *size);
			if (hvalue == -1)
			{
				*location = NULL;
				*size = -1;
				return user_list;
			}
			hvalue++;
			if (*size == USERHOST_HASHSIZE && hvalue >= USERHOST_HASHSIZE)
			{
				/* end of current list */
				*location = &UserListByChannel_Table[0];
				*size = USERCHAN_HASHSIZE;
			}
			while (((UserList *)((HashEntry *)*location)[hvalue].list) == NULL)
			{
				hvalue++;
				if (hvalue >= USERHOST_HASHSIZE)
				{
					*location = &UserListByChannel_Table[0];
					hvalue = 0;
					*size = USERCHAN_HASHSIZE;
				}
				else if (*size == USERCHAN_HASHSIZE && hvalue >= USERCHAN_HASHSIZE)
				{
					hvalue = -1;
					break;
				}
			}
			if (hvalue != -1 && ((HashEntry *)*location)[hvalue].list != NULL)
				return ((UserList *)((HashEntry *)*location)[hvalue].list);
			*location = NULL;		
			*size = -1;
			return user_list;
		}
	}
	return NULL;
}
#endif
