/*
 * gaim
 *
 * Copyright (C) 1998-1999, Mark Spencer <markster@marko.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA 
 * 
 */


/* 
 * Heavily modified by Nadeem Riaz (nads@bleh.org)
 * for use in libtoc
 */


#include <string.h>
#include "ll.h"
#include "toc.h"

LL groups;
LL permit;
LL deny;  
LL buddy_chats;
LL invited_chats;

struct buddy *add_buddy(char *group, char *buddy)
{
	struct buddy *b;
	struct group *g;
	
	toc_debug_printf("adding '%s' to '%s'\n",buddy,group);

	if ((b = find_buddy(buddy)) != NULL)
                return b;

	g = find_group(group);

	if (g == NULL)
		g = add_group(group);
	
        b = (struct buddy *)  malloc(sizeof(struct buddy));
        
	if (!b)
		return NULL;

	b->present = 0;

	snprintf(b->name, sizeof(b->name), "%s", buddy);
	AddToLL(g->members,b->name,b);
       
        b->idle = 0;
	
	return b;
}

struct group *add_group(char *group)
{
	struct group *g;
	g = (struct group *) malloc(sizeof(struct group));
	if (!g)
		return NULL; 

	strncpy(g->name, group, sizeof(g->name));
        AddToLL(groups, g->name, g);
	
	g->members = CreateLL();
	
	return g;	
}

struct group *find_group(char *group)
{
	struct group *g;
	LLE e;
	char *grpname = malloc(strlen(group) + 1);
	strcpy(grpname, normalize(group));
	
	for ( TLL(groups,e) ) {
		g = (struct group *)e->data;
		if (!strcasecmp(normalize(g->name), grpname)) {
			free(grpname);
			return g;
		}
	}

	free(grpname);
	return NULL;	
	
}

struct buddy *find_buddy(char *who)
{
	struct group *g;
	struct buddy *b;
	LLE tg,tb;
	LL mems;
        char *whoname = malloc(strlen(who) + 1);

	strcpy(whoname, normalize(who));
	
	for ( TLL(groups,tg) ) {
		g = (struct group *) tg->data;
		mems =  g->members;
		
		for ( TLL(mems,tb) )  {
			b = (struct buddy *)tb->data;
			if (!strcasecmp(normalize(b->name), whoname)) {
				free(whoname);
				return b;
			}
		}
		
	}
	
	free(whoname);
	return NULL;
}

int user_remove_buddy(char *buddy) {
	struct group *g;
	struct buddy *b;
	LLE e,m;
	char *budname = malloc(strlen(buddy) + 1);
	strcpy(budname, normalize(buddy));
	
	for ( TLL(groups,e) ) {
		g = (struct group *)e->data;
		
		for ( TLL(g->members,m) ) {
			b = (struct buddy *)m->data;
			if ( ! strcasecmp(normalize(b->name),budname) ) {
				RemoveFromLLByKey(g->members,buddy);
				serv_remove_buddy(buddy);
				serv_save_config();
				free(budname);				
				return 1;
			}
		}
	}

	free(budname);
	return -1;	
}

int user_add_buddy(char *group, char *buddy) {
	struct buddy *b;
	b = find_buddy(buddy);
	if ( b != NULL )
		return -1;
	add_buddy(group,buddy);
	serv_add_buddy(buddy);
	serv_save_config();
	return 1; 
}

/* 
 * mode 1 = move current group members to a enw group
 * mode 2 = delete current group members from buddy list
 */
int remove_group(char *group, char *newgroup, int mode)
{
	LL mem;
	LLE t;
	
	struct group *delg = find_group(group);
	struct group *newg = NULL;
	struct buddy *delb;

	if ( ! delg ) {		
		return -1;
	}
	
	if ( mode == 1 ) {
		newg = find_group(newgroup);
		if ( ! newg ) {
			newg = add_group(newgroup);
		}
	}

        mem = delg->members; 
	for ( TLL(mem,t) ) {
		delb = (struct buddy *)t->data;
		if ( mode == 1 ) { 
			AddToLL(newg->members,delb->name,delb);
		} else {
        	        serv_remove_buddy(delb->name);
                	/* free(delb); */
		}
	}

	RemoveFromLLByKey(groups,delg->name);
        serv_save_config();
	return 1;
}

int add_permit(char *sn) {
	LLE t;
	t = FindInLL(permit,sn);
	if ( t ) 
		return -1;
	AddToLL(permit,sn,NULL);
	if ( permdeny == PERMIT_PERMITSOME )
		serv_add_permit(sn);
	serv_save_config();
	return 1;
}

int remove_permit(char *sn) {
	LLE t;
	t = FindInLL(permit,sn);
	if ( ! t ) 
		return -1;
	RemoveFromLLByKey(permit,sn);
	serv_save_config();	
	if (permdeny == PERMIT_PERMITSOME ) 
		serv_set_permit_deny();	
	return 1;
}

int add_deny(char *sn) {
	LLE t;
	t = FindInLL(deny,sn);
	if ( t ) 
		return -1;
	AddToLL(deny,sn,NULL);
	if ( permdeny == PERMIT_DENYSOME )
		serv_add_deny(sn);
	serv_save_config();
	return 1;
}

int remove_deny(char *sn) {
	LLE t;
	t = FindInLL(deny,sn);
	if ( ! t ) 
		return -1;
	RemoveFromLLByKey(deny,sn);
	if ( permdeny == PERMIT_DENYSOME ) {
		/* 
		 * DAMN AOL HOEBAGS to lazzy toinclude a delete from deny list
		 * Thus we need to first go into permit mode */
		serv_set_permit_deny();
	}
	serv_save_config();	
	return 1;	
}

int buddy_invite(char *chat, char *buddy, char *msg) {
	LLE t;
	struct buddy_chat *b;
	t = FindInLL(buddy_chats,chat);
	if ( ! t ) 
		return -1;
	b = (struct buddy_chat *)t->data;
	serv_chat_invite(b->id, msg, buddy);
	return 1;
}

/*
 * Checks invite list first 
 * then tries to create chat
 */
 
void buddy_chat_join(char *chan) {
	LLE t = FindInLL(invited_chats,chan);
	if ( ! t ) {
		/* Standard exchange is 4 */
		toc_debug_printf("Creating chan %s",chan);
		serv_join_chat(4,chan);
	} else {
		/* The chat is in our invite list */
		int *d;
		d = (int *) t->data;
		serv_accept_chat(*d);
		toc_debug_printf("Trying to join invited to %s %d",t->key, *d);
		RemoveFromLLByKey(invited_chats,chan);		
	}
}

struct buddy_chat *find_buddy_chat(char *chat) {
	LLE t;
	t = FindInLL(buddy_chats,chat);
	if ( ! t ) 
		return NULL;
	else
		return (struct buddy_chat *) t->data;
}

int buddy_chat_leave(char *chan) {
	LLE t;
	struct buddy_chat *b;
	t = FindInLL(buddy_chats,chan);	
	if ( ! t ) 
		return -1;
	b = (struct buddy_chat *) t->data;
	serv_chat_leave(b->id);
	/* Removed from buddy_chats in toc_callback */	
	return 1;
}

struct buddy_chat *buddy_chat_getbyid(int id) {
	LLE t;
	struct buddy_chat *b;
	for ( TLL(buddy_chats,t) ) {
		b = (struct buddy_chat *) t->data;
		if ( id == b->id ) 
			return b;
	}	
	return NULL;		
}

int buddy_chat_invite(char *chat, char *buddy, char *msg) {
	LLE t;
	struct buddy_chat *b;
	t = FindInLL(buddy_chats,chat);	
	if ( ! t ) 
		return -1;
	b = (struct buddy_chat *) t->data;
	serv_chat_invite(b->id, msg, buddy);
	return 1;
}

int buddy_chat_warn(char *chat, char *user, int anon) {
	LLE t;
	struct buddy_chat *b;
	t = FindInLL(buddy_chats,chat);	
	if ( ! t ) 
		return -1;
	b = (struct buddy_chat *) t->data;	
	serv_chat_warn(b->id, user, anon);
	return 1;
}
