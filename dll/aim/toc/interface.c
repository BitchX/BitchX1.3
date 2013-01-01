/*
 * interface.c
 *
 * by Nadeem Riaz (nads@bleh.org)
 *
 * Probably should be renamed misc.c (oh well)
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "toc.h"

int (*TOC_RAW_HANDLERS[30])(int, char *); 
int (*TOC_HANDLERS[30])(int, char **);

void init_toc() {
	int x;
	groups = NULL;
	permit = NULL;
	deny = NULL;
	buddy_chats = NULL;
	invited_chats = NULL;
	strcpy(aim_host,TOC_HOST);
	aim_port = TOC_PORT;
	strcpy(login_host,AUTH_HOST);
	login_port = AUTH_PORT;

	/* Init Handlers */
	for (x=0;x<30;x++) 		
		TOC_HANDLERS[x] = NULL;
	for (x=0;x<30;x++) 
		TOC_RAW_HANDLERS[x] = NULL;
	
}

void init_lists() {
	if ( groups == NULL ) {
		groups = (LL) CreateLL();
		SetFreeLLE(groups,&misc_free_group);
	}
	if ( permit == NULL )
		permit = (LL) CreateLL();
	if ( deny == NULL )
		deny = (LL) CreateLL();
	if ( buddy_chats == NULL ) {
		buddy_chats = CreateLL();
		SetFreeLLE(buddy_chats,&misc_free_buddy_chat);
	}
	if ( invited_chats == NULL ) {
		invited_chats = CreateLL();
		SetFreeLLE(invited_chats,&misc_free_invited_chats);
	}
}


int install_handler(int type, int (*func)(int, char **)) {
	TOC_HANDLERS[type] = func;
	return 1;
}

int install_raw_handler(int type, int (*func)(int, char *)) {
	TOC_RAW_HANDLERS[type] = func; 
	return 1;
}


int use_handler(int mode,int type, void *args) {
	int ret = 0;
	toc_debug_printf("use_handler: mode = %d type = %d",mode,type);
	if ( mode == TOC_HANDLE ) {
		if ( TOC_HANDLERS[type] == NULL ) 
			toc_debug_printf("Error, no handler installed for %d type",type);
		else 
			ret = TOC_HANDLERS[type](type, (char **) args);
	} else if ( mode == TOC_RAW_HANDLE ) {
		if ( TOC_RAW_HANDLERS[type] == NULL ) 
			toc_debug_printf("Error, no raw handler installed for %d type",type);
		else 
			ret = TOC_RAW_HANDLERS[type](type, (char *) args);	
	} else {
		toc_debug_printf("Error: %d : unkown handle mode!",mode);
		ret = -1;
	} 
	return ret;
}
