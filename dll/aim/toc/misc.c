/*
 * misc.c
 *
 * by Nadeem Riaz (nads@bleh.org)
 */

#include "toc.h"

char aim_host[512];
int aim_port;
char login_host[512];
int login_port;
char toc_addy[16];
char aim_username[80];
char aim_password[16];
char *quad_addr;
char debug_buff[1024];
char user_info[2048];
int registered;

char *USER_CLASSES[5] = {
	"AOL User",
	"AIM Admin",
	"Trial Aim User",
	"Normal Aim User",
	"Unavailable"
};

char *PERMIT_MODES[4] = {
	"Permit All",
	"Deny All",
	"Permit Some",
	"Deny Some"
};	

void save_prefs()
{ 
}

void misc_free_group(void *data) {
	struct group *g;
	g = (struct group *) data;
	FreeLL(g->members);
	free(g);
}

void misc_free_buddy_chat(void *data) {
	struct buddy_chat *b;
	b = (struct buddy_chat *) data;
	FreeLL(b->in_room);
	FreeLL(b->ignored);
	free(b);
}

void misc_free_invited_chats(void *data) {
	int *t;
	t = (int *) data;
	free(t);
}
