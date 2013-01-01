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

#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "toc.h"

/* descriptor for talking to TOC */
static int toc_fd;
static int seqno;
static unsigned int peer_ver=0;
int state;
/* static int inpa=-1; */
int permdeny = PERMIT_PERMITALL;

int toc_login(char *username, char *password)
{
	char *config;
	struct in_addr *sin;
	char buf[80];
	char buf2[2048];
	
	toc_debug_printf("looking up host! %s", aim_host);

	sin = (struct in_addr *)get_address(aim_host);
	if (!sin) {  
		set_state(STATE_OFFLINE); 
		toc_msg_printf(TOC_CONNECT_MSGS,"Unable to lookup %s", aim_host);
		return -1;
	}
	
	snprintf(toc_addy, sizeof(toc_addy), "%s", inet_ntoa(*sin));
	snprintf(buf, sizeof(buf), "Connecting to %s", inet_ntoa(*sin));
	
	toc_msg_printf(TOC_CONNECT_MSGS,"%s",buf); 
	
	toc_fd = connect_address(sin->s_addr, aim_port);

        if (toc_fd < 0) {
                set_state(STATE_OFFLINE); 
		toc_msg_printf(TOC_CONNECT_MSGS,"Connect to %s failed", inet_ntoa(*sin));
		return -1;
        }

        free(sin);
	
	toc_msg_printf(TOC_CONNECT_MSGS,"Signon: %s",username);
	
	if (toc_signon(username, password) < 0) {
                set_state(STATE_OFFLINE);
		toc_msg_printf(TOC_CONNECT_MSGS,"Disconnected.");
		return -1;
	}

	toc_msg_printf(TOC_CONNECT_MSGS,"Waiting for reply...");
	if (toc_wait_signon() < 0) {
                set_state(STATE_OFFLINE);
		toc_msg_printf(TOC_CONNECT_MSGS,"Authentication Failed");
		return -1;
	}


	snprintf(aim_username, sizeof(aim_username), "%s", username);
	snprintf(aim_password, sizeof(aim_password), "%s", password);

	save_prefs();

	toc_msg_printf(TOC_CONNECT_MSGS,"Retrieving config...");
        if ((config=toc_wait_config()) == NULL) {
		toc_msg_printf(TOC_CONNECT_MSGS,"No Configuration\n");
		set_state(STATE_OFFLINE);
		return -1;

	}
	
	init_lists();
/*        gtk_widget_hide(mainwindow);
	show_buddy_list();  */
        parse_toc_buddy_list(config);
/*        refresh_buddy_window(); */
   
	snprintf(buf2, sizeof(buf2), "toc_init_done");
	sflap_send(buf2, -1, TYPE_DATA);
        
        serv_finish_login();
	return 0;
}

void toc_close()
{
        seqno = 0;
        state = STATE_OFFLINE;
	toc_remove_input_stream(toc_fd);
	close(toc_fd);
	toc_fd=-1;
}

int toc_signon(char *username, char *password)
{
	char buf[BUF_LONG];
	int res;
	struct signon so;

    	toc_debug_printf("State = %d\n", state);

	strncpy(aim_username, username, sizeof(aim_username));
	
	if ((res = write(toc_fd, FLAPON, strlen(FLAPON))) < 0)
		return res;
	/* Wait for signon packet */

	state = STATE_FLAPON;

	if ((res = wait_reply(buf, sizeof(buf)) < 0))
		return res;
	
	if (state != STATE_SIGNON_REQUEST) {
			toc_debug_printf( "State should be %d, but is %d instead\n", STATE_SIGNON_REQUEST, state);
			return -1;
	}
	
	/* Compose a response */
	
	snprintf(so.username, sizeof(so.username), "%s", username);
	so.ver = ntohl(1);
	so.tag = ntohs(1);
	so.namelen = htons(strlen(so.username));	
	
	sflap_send((char *)&so, ntohs(so.namelen) + 8, TYPE_SIGNON);
	
	snprintf(buf, sizeof(buf), 
		"toc_signon %s %d %s %s %s \"%s\"",
		login_host, login_port, normalize(username), roast_password(password), LANGUAGE, REVISION);

        toc_debug_printf("Send: %s\n", buf);

	return sflap_send(buf, -1, TYPE_DATA);
}

int toc_wait_signon()
{
	/* Wait for the SIGNON to be approved */
	char buf[BUF_LEN];
	int res;
	res = wait_reply(buf, sizeof(buf));
	if (res < 0)
		return res;
	if (state != STATE_SIGNON_ACK) {
			toc_debug_printf("State should be %d, but is %d instead\n",STATE_SIGNON_ACK, state);
		return -1;
	}
	return 0;
}


int wait_reply(char *buffer, int buflen)
{
        int res=6;
	struct sflap_hdr *hdr=(struct sflap_hdr *)buffer;
        char *c;

        while((res = read(toc_fd, buffer, 1))) {
		if (res < 0)
			return res;
		if (buffer[0] == '*')
                        break;

	}

	res = read(toc_fd, buffer+1, sizeof(struct sflap_hdr) - 1);

        if (res < 0)
		return res;

	res += 1;
	
        
	toc_debug_printf( "Rcv: %s %s\n",print_header(buffer), "");
	
        while (res < (sizeof(struct sflap_hdr) + ntohs(hdr->len))) {
		res += read(toc_fd, buffer + res, (ntohs(hdr->len) + sizeof(struct sflap_hdr)) - res);
		/* while(gtk_events_pending())
			gtk_main_iteration(); */
	}
        
        if (res >= sizeof(struct sflap_hdr)) 
		buffer[res]='\0';
	else
		return res - sizeof(struct sflap_hdr);
		
	switch(hdr->type) {
	case TYPE_SIGNON:
		memcpy(&peer_ver, buffer + sizeof(struct sflap_hdr), 4);
		peer_ver = ntohl(peer_ver);
		seqno = ntohs(hdr->seqno);
		state = STATE_SIGNON_REQUEST;
		break;
	case TYPE_DATA:
		if (!strncasecmp(buffer + sizeof(struct sflap_hdr), "SIGN_ON:", strlen("SIGN_ON:")))
			state = STATE_SIGNON_ACK;
		else if (!strncasecmp(buffer + sizeof(struct sflap_hdr), "CONFIG:", strlen("CONFIG:"))) {
			state = STATE_CONFIG;
		} else if (state != STATE_ONLINE && !strncasecmp(buffer + sizeof(struct sflap_hdr), "ERROR:", strlen("ERROR:"))) {
			c = strtok(buffer + sizeof(struct sflap_hdr) + strlen("ERROR:"), ":");
			translate_toc_error_code(c);
			toc_debug_printf("ERROR CODE: %s\n",c);
		}

		toc_debug_printf("Data: %s\n",buffer + sizeof(struct sflap_hdr));

		break;
	default:
			toc_debug_printf("Unknown/unimplemented packet type %d\n",hdr->type);
	}
        return res;
}

int sflap_send(char *buf, int olen, int type)
{
	int len;
	int slen=0;
	struct sflap_hdr hdr;
	char obuf[MSG_LEN];

        /* One _last_ 2048 check here!  This shouldn't ever
         * get hit though, hopefully.  If it gets hit on an IM
         * It'll lose the last " and the message won't go through,
         * but this'll stop a segfault. */
        if (strlen(buf) > (MSG_LEN - sizeof(hdr))) {
                buf[MSG_LEN - sizeof(hdr) - 3] = '"';
                buf[MSG_LEN - sizeof(hdr) - 2] = '\0';
        }

        toc_debug_printf("%s [Len %d]\n", buf, strlen(buf));

	
	if (olen < 0)
		len = escape_message(buf);
	else
		len = olen;
	hdr.ast = '*';
	hdr.type = type;
	hdr.seqno = htons(seqno++ & 0xffff);
        hdr.len = htons(len + (type == TYPE_SIGNON ? 0 : 1));

	toc_debug_printf("Escaped message is '%s'\n",buf);

	memcpy(obuf, &hdr, sizeof(hdr));
	slen += sizeof(hdr);
	memcpy(&obuf[slen], buf, len);
	slen += len;
	if (type != TYPE_SIGNON) {
		obuf[slen]='\0';
		slen += 1;
	}
	/* print_buffer(obuf, slen); */

	return write(toc_fd, obuf, slen);
}

unsigned char *roast_password(char *pass)
{
	/* Trivial "encryption" */
	static char rp[256];
	static char *roast = ROAST;
	int pos=2;
	int x;
	strcpy(rp, "0x");
	for (x=0;(x<150) && pass[x]; x++) 
		pos+=sprintf(&rp[pos],"%02x", pass[x] ^ roast[x % strlen(roast)]);
	rp[pos]='\0';
        return rp;
}

char *print_header(void *hdr_v)
{
	static char s[80];
	struct sflap_hdr *hdr = (struct sflap_hdr *)hdr_v;
	snprintf(s,sizeof(s), "[ ast: %c, type: %d, seqno: %d, len: %d ]",
		hdr->ast, hdr->type, ntohs(hdr->seqno), ntohs(hdr->len));
	return s;
}

int toc_callback(int fd)
{
        char *buf;
	char *c;
	char **args = NULL;
	char *dup,*raw;
        char *l;
	int numargs =0;

        buf = malloc(BUF_LONG);

        if (wait_reply(buf, BUF_LONG) < 0) {
                toc_signoff();
		toc_debug_printf("need to do proper sign off on this\n");
		toc_msg_printf(TOC_CONNECT_MSGS,"Connection Closed");
		return -1;
        }
                         
        dup = strdup(buf+sizeof(struct sflap_hdr));
	raw = rindex(dup,':'); 
	c=strtok(buf+sizeof(struct sflap_hdr),":");	/* Ditch the first part */
	if (!strcasecmp(c,"UPDATE_BUDDY")) {
		char *uc, *t;
		int logged, evil, idle, type = 0;
                time_t signon;
                time_t time_idle;

		numargs = 7;
		args = (char **) malloc(sizeof(char *)*numargs);
		use_handler(TOC_RAW_HANDLE,TOC_UPDATE_BUDDY,raw);
		c = strtok(NULL,":"); /* c is name */
		args[0] = strdup(c);
		
		l = strtok(NULL,":"); /* l is T/F logged status */
       		args[1] = strdup(l);
	
		t = strtok(NULL, ":");
		args[2] = strdup(t);
		sscanf(t, "%d", &evil);
		
		t = strtok(NULL, ":");
		args[3] = strdup(t);		
		sscanf(t, "%ld", &signon);

		t = strtok(NULL, ":");
		args[4] = strdup(t);		
		sscanf(t, "%d", &idle);
		
                uc = strtok(NULL, ":");
		args[5] = strdup(uc);

		if (!strncasecmp(l,"T",1))
			logged = 1;
		else
			logged = 0;


		if (uc[0] == 'A')
			type |= UC_AOL;
		
		switch(uc[1]) {
		case 'A':
			type |= UC_ADMIN;
			break;
		case 'U':
			type |= UC_UNCONFIRMED;
			break;
		case 'O':
			type |= UC_NORMAL;
			break;
		default:
			break;
		}

                switch(uc[2]) {
		case 'U':
			type |= UC_UNAVAILABLE;
			break;
		default:
			break;
		}

                if (idle) {
                        time(&time_idle);
                        time_idle -= idle*60;
                } else
                        time_idle = 0;
		
		serv_got_update(c, logged, evil, signon, time_idle, type); 
		args[6] = NULL;
		use_handler(TOC_HANDLE,TOC_UPDATE_BUDDY,args);
	} else if (!strcasecmp(c, "ERROR")) {
		use_handler(TOC_RAW_HANDLE,TOC_ERROR,raw);
		c = strtok(NULL,":");
		translate_toc_error_code(c);
		args = (char **) malloc(sizeof(char *)*1 + 1);
		numargs = 1;
		args[0] = strdup(c);
		use_handler(TOC_HANDLE,TOC_ERROR,args);
		toc_debug_printf("ERROR: %s",c);
	} else if (!strcasecmp(c, "NICK")) {
		use_handler(TOC_RAW_HANDLE,TOC_NICK,raw);
		c = strtok(NULL,":");
		snprintf(aim_username, sizeof(aim_username), "%s", c);
		numargs = 2;
		args = (char **) malloc(sizeof(char *)*numargs);
		args[0] = strdup(c);
		args[1] = NULL;
		use_handler(TOC_HANDLE,TOC_NICK,args);	
	} else if (!strcasecmp(c, "IM_IN")) {
		char *away, *message;
                int a = 0;
		use_handler(TOC_RAW_HANDLE,TOC_IM_IN,raw);
		c = strtok(NULL,":");
		away = strtok(NULL,":");

		message = away;

                while(*message && (*message != ':'))
                        message++;

                message++;

		if (!strncasecmp(away, "T", 1))
			a = 1;

		if ( serv_got_im(c, message,a) > 0 ) {
			numargs = 3;
			args = (char **) malloc(sizeof(char *)*numargs);		
			args[0] = strdup(c);
			args[1] = strdup(message);
			args[2] = NULL;
			use_handler(TOC_HANDLE,TOC_IM_IN,args);
		}
	} else if (!strcasecmp(c, "GOTO_URL")) {
		char *name;
		char *url;

		char tmp[256];
		
		use_handler(TOC_RAW_HANDLE,TOC_GOTO_URL,raw);
		name = strtok(NULL, ":");
		url = strtok(NULL, ":");


		snprintf(tmp, sizeof(tmp), "http://%s:%d/%s", toc_addy, aim_port, url);
/*		fprintf(stdout, "Name: %s\n%s\n", name, url);
		printf("%s", grab_url(tmp));*/

		numargs = 2;
		args = (char **) malloc(sizeof(char *)*numargs);
		args[0] = strdup(tmp);
		args[1] = NULL;
		use_handler(TOC_HANDLE,TOC_GOTO_URL,args);
		/* statusprintf("GOTO_URL: %s","tmp"); */
        } else if (!strcasecmp(c, "EVILED")) {
                int lev;
		char *name = NULL;
		char *levc;

		use_handler(TOC_RAW_HANDLE,TOC_EVILED,raw);
		levc = strtok(NULL, ":");
                sscanf(levc, "%d", &lev);
                name = strtok(NULL, ":");

                toc_debug_printf("evil: %s | %d\n", name, lev);

		numargs = 3;
		my_evil = lev;
		args = (char **) malloc(sizeof(char *)*numargs);		
		if ( name != NULL )
			args[0] = strdup(name);
		else 
			args[0] = NULL;
		args[1] = strdup(levc);
		args[2] = NULL;
		use_handler(TOC_HANDLE,TOC_EVILED,args);
		
        } else if (!strcasecmp(c, "CHAT_JOIN")) {
                char *name,*idc;
                int id;
		
                use_handler(TOC_RAW_HANDLE,TOC_CHAT_JOIN,raw);
		idc = strtok(NULL, ":");
		sscanf(idc, "%d", &id);
                name = strtok(NULL, ":");
                serv_got_joined_chat(id, name); 
		numargs = 3;
		args = (char **) malloc(sizeof(char *)*numargs);		
		args[0] = strdup(idc);
		args[1] = strdup(name);
		args[2] = NULL;
		use_handler(TOC_HANDLE,TOC_CHAT_JOIN,args);		
	} else if (!strcasecmp(c, "DIR_STATUS")) {
		char *status;
		use_handler(TOC_RAW_HANDLE,TOC_DIR_STATUS,raw);
		status = strtok(NULL,":");
		numargs = 2;
		args = (char **) malloc(sizeof(char *)*numargs);		
		args[0] = strdup(status);
		args[1] = NULL;		
		use_handler(TOC_HANDLE,TOC_DIR_STATUS,args);		
	} else if (!strcasecmp(c, "CHAT_UPDATE_BUDDY")) {
		int id;
		char *in,*idc;
		char *buddy;
                LLE t;
		struct buddy_chat *b = NULL;
	
		use_handler(TOC_RAW_HANDLE,TOC_CHAT_UPDATE_BUDDY,raw);		
		idc = strtok(NULL, ":");
		sscanf(idc, "%d", &id);

		in = strtok(NULL, ":");

		for ( TLL(buddy_chats,t) ) {
			b = (struct buddy_chat *)t->data;
			if (id == b->id)
				break;	
                        b = NULL;
		}
		
		if (!b)
			return -2;

		
		if (!strcasecmp(in, "T")) {
			while((buddy = strtok(NULL, ":")) != NULL) {
				/* 
				 * Fuxin aim causes a problem here
				 */
				AddToLL(b->in_room, buddy,NULL);
				if ( b->init_chat ) {
					args = (char **) malloc(sizeof(char *)*3);
					args[0] = strdup(b->name);
					args[1] = strdup(buddy);		
					args[2] = NULL;					
					use_handler(TOC_HANDLE,TOC_BUDDY_JOIN_CHAT,args);
					free(args[0]); free(args[1]); free(args); args = NULL;
				}
			}
			/*
			 * init_chat is so that the user doenst get flooded 
			 * with user joined chat when he first joins a chat
			 */
			b->init_chat = 1;				
		} else {
			while((buddy = strtok(NULL, ":")) != NULL) {
				RemoveFromLLByKey(b->in_room, buddy);
				/* 
				 * Since we might get multiple leave/joins at once 
				 * we allocate & deallocate here 
				 */
				args = (char **) malloc(sizeof(char *)*3);
				args[0] = strdup(b->name);
				args[1] = strdup(buddy);		
				args[2] = NULL;					
				use_handler(TOC_HANDLE,TOC_BUDDY_LEFT_CHAT,args);
				free(args[0]); free(args[1]); free(args); args = NULL;
			}
		}
	} else if (!strcasecmp(c, "CHAT_LEFT")) {
		char *idc;
		int id;

		use_handler(TOC_RAW_HANDLE,TOC_CHAT_LEFT,raw);
		idc = strtok(NULL, ":");
                sscanf(idc, "%d", &id);

                serv_got_chat_left(id); 

		numargs = 2;
		args = (char **) malloc(sizeof(char *)*numargs);
		args[0] = strdup(idc);	
		args[1] = NULL;
		use_handler(TOC_HANDLE,TOC_CHAT_LEFT,args);		
	} else if (!strcasecmp(c, "CHAT_IN")) {

		int id, w;
		char *m,*idc;
		char *who, *whisper, *chan;
		struct buddy_chat *b;		

		use_handler(TOC_RAW_HANDLE,TOC_CHAT_IN,raw);
		idc = strtok(NULL, ":");
		sscanf(idc, "%d", &id);
		who = strtok(NULL, ":");
		whisper = strtok(NULL, ":");
		m = whisper;
                while(*m && (*m != ':')) m++;
                m++;

                if (!strcasecmp(whisper, "T"))
			w = 1;
		else
			w = 0;

		/* serv_got_chat_in(id, who, w, m); */
		b = buddy_chat_getbyid(id);
		if ( ! b ) {
			chan = (char *) malloc(50);
			strcpy(chan,"ERROR Couldn't lookup chan!");
		} else {
			chan = (char *) malloc(strlen(b->name)+1);
			strcpy(chan,b->name);
		}
		numargs = 6;
		args = (char **) malloc(sizeof(char *)*numargs);
		args[0] = strdup(idc);
		args[1] = strdup(who);
		args[2] = strdup(whisper);
		args[3] = strdup(m);
		/* Added arg to make things simple */
		args[4] = chan;
		args[5] = NULL;
		use_handler(TOC_HANDLE,TOC_CHAT_IN,args);
	} else if (!strcasecmp(c, "CHAT_INVITE")) {
		char *name;
		char *who;
		char *message,*idc;
                int id, *pid;

                use_handler(TOC_RAW_HANDLE,TOC_CHAT_INVITE,raw);
		name = strtok(NULL, ":");
		idc = strtok(NULL, ":");
		sscanf(idc, "%d", &id);
		who = strtok(NULL, ":");
                message = strtok(NULL, ":");	
               /* serv_got_chat_invite(name, id, who, message); */
	        pid = (int *) malloc(sizeof(int));
		*pid = id;
	        AddToLL(invited_chats,name,pid);
		numargs = 5;
		args = (char **) malloc(sizeof(char *)*numargs);
		args[0] = strdup(name);
		args[1] = strdup(idc);
		args[2] = strdup(who);
		args[3] = strdup(message);
		args[4] = NULL;
		use_handler(TOC_HANDLE,TOC_CHAT_INVITE,args);	       
	} else {
		toc_debug_printf("don't know what to do with %s\n", c);
	}
	free(dup);
        free(buf);
	if ( args != NULL ) {
		int x;
		/* toc_debug_printf("\nGOING TO FREE!: numargs = %d",numargs); */
		for (x=0;x< numargs; x++) 
			if ( args[x] != NULL ) {
				/* toc_debug_printf("freeing %d",x); */
				free(args[x]);
			}
		/* toc_debug_printf(""); */
		free(args);	
	}
	return 1;
}

char *toc_wait_config()
{
	/* Waits for configuration packet, returning the contents of the packet */
	static char buf[BUF_LEN];
	int res;
        res = wait_reply(buf, sizeof(buf));
	if (res < 0)
		return NULL;
	if (state != STATE_CONFIG) {
     		toc_debug_printf("State should be %d, but is %d instead\n",STATE_CONFIG, state);
		return NULL;
	}
	/* At this point, it's time to setup automatic handling of incoming packets */
        state = STATE_ONLINE;
	
        // inpa = gdk_input_add(toc_fd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, toc_callback, NULL);
	toc_add_input_stream(toc_fd,&toc_callback);
	
	return buf;
}

void parse_toc_buddy_list(char *config)
{
	char *c;
        char current[256];
	char *name;
	LL bud = CreateLL();		
        
        /* skip "CONFIG:" (if it exists)*/

        c = strncmp(config + sizeof(struct sflap_hdr),"CONFIG:",strlen("CONFIG:"))?
			strtok(config, "\n"):
			strtok(config + sizeof(struct sflap_hdr)+strlen("CONFIG:"), "\n");
	
	do {
		if (c == NULL) 
			break;
		if (*c == 'g') {
			strncpy(current,c+2, sizeof(current));
			add_group(current);
		} else if (*c == 'b') {
			add_buddy(current, c+2);
			AddToLL(bud, c+2, NULL);
        	} else if (*c == 'p') {
           		name = malloc(strlen(c+2) + 2);
			snprintf(name, strlen(c+2) + 1, "%s", c+2);
			AddToLL(permit, name, NULL);
		} else if (*c == 'd') {
			name = malloc(strlen(c+2) + 2);
			snprintf(name, strlen(c+2) + 1, "%s", c+2);
			AddToLL(deny, name,NULL);
		} else if (*c == 'm') {
 			sscanf(c + strlen(c) - 1, "%d", &permdeny);
			if (permdeny == 0)
				permdeny = PERMIT_PERMITALL;
		}
	} while ((c=strtok(NULL,"\n"))); 
       
	serv_add_buddies(bud);
	FreeLL(bud);
        serv_set_permit_deny();
}

int toc_signoff() {	
	/* Leaking memory like a MOFO */
	FreeLL(groups);
	FreeLL(buddy_chats);
	FreeLL(invited_chats);
	FreeLL(permit);
	FreeLL(deny);
	deny = groups = permit = buddy_chats = invited_chats = NULL;
	toc_debug_printf("LEAKING MEMORY LIKE A BITCH in toc_signoff!");

	serv_close();
	toc_msg_printf(TOC_CONNECT_MSGS,"%s signed off",aim_username);
	return 1;
}

void toc_build_config(char *s, int len)
{
	struct group *g;
	struct buddy *b;
	LLE t,t1;
	LL mem;

	int pos=0;
	toc_debug_printf("FIX this permdeny hack shit!");

	if (!permdeny)
		    permdeny = PERMIT_PERMITALL;
	pos += snprintf(&s[pos], len - pos, "m %d\n", permdeny);
	
	/* Create Buddy List */
	for ( TLL(groups,t) ) {
		g = (struct group *)t->data;
		pos += snprintf(&s[pos], len - pos, "g %s\n", g->name);
		mem = g->members;
		for ( TLL(mem,t1) ) {
			b = (struct buddy *)t1->data;
			pos += snprintf(&s[pos], len - pos, "b %s\n", b->name);
		}
	}
	
	/* Create Permit and Deny Lists */;
	for ( TLL(permit,t) ) {
		toc_debug_printf("permit: added %s\n",(char *)t->key);
    		pos += snprintf(&s[pos], len - pos, "p %s\n", (char *)t->key);
	}
	for ( TLL(deny,t) ) {
		toc_debug_printf("deny: added %s\n",(char *)t->key);
    		pos += snprintf(&s[pos], len - pos, "d %s\n", (char *)t->key);
	}	
}

void translate_toc_error_code(char *c) {

	int no = atoi(c);
	char *w = strtok(NULL, ":");
	char buf[256];

        switch ( no ) {
        	case 901:
                	snprintf(buf, sizeof(buf), "%s not currently logged in.", w);
	                break;
        	case 902:
	                snprintf(buf, sizeof(buf), "Warning of %s not allowed.", w);
        	        break;
	        case 903:
        	        snprintf(buf, sizeof(buf), "A message has been dropped, you are exceeding the server speed limit.");
                	break;
	        case 950:
        	        snprintf(buf, sizeof(buf), "Chat in %s is not available.", w);
                	break;
	        case 960:
        	        snprintf(buf, sizeof(buf), "You are sending messages too fast to %s.", w);
                	break;
	        case 961:
        	        snprintf(buf, sizeof(buf), "You missed an IM from %s because it was too big.", w);
                	break;
	        case 962:
	                snprintf(buf, sizeof(buf), "You missed an IM from %s because it was sent too fast.", w);
                	break;
	        case 970:
        	        snprintf(buf, sizeof(buf), "Failure.");
                	break;
	        case 971:
        	        snprintf(buf, sizeof(buf), "Too many matches.");
                	break;
	        case 972:
        	        snprintf(buf, sizeof(buf), "Need more qualifiers.");
                	break;
        	case 973:
	                snprintf(buf, sizeof(buf), "Dir service temporarily unavailable.");
        	        break;
	        case 974:
	                snprintf(buf, sizeof(buf), "Email lookup restricted.");
        	        break;
	        case 975:
                	snprintf(buf, sizeof(buf), "Keyword ignored.");
        	        break;
	        case 976:
         	       snprintf(buf, sizeof(buf), "No keywords.");
                	break;
	        case 977:
                	snprintf(buf, sizeof(buf), "User has no directory information.");
        	        /* snprintf(buf, sizeof(buf), "Language not supported."); */
	                break;
        	case 978:
                	snprintf(buf, sizeof(buf), "Country not supported.");
	                break;
        	case 979:
                	snprintf(buf, sizeof(buf), "Failure unknown: %s.", w);
	                break;
        	case 980:
                	snprintf(buf, sizeof(buf), "Incorrect nickname or password.");
	                break;
	        case 981:
        	        snprintf(buf, sizeof(buf), "The service is temporarily unavailable.");
                	break;
	        case 982:
        	        snprintf(buf, sizeof(buf), "Your warning level is currently too high to log in.");
                	break;
	        case 983:
        	        snprintf(buf, sizeof(buf), "You have been connecting and disconnecting too frequently.  Wait ten minutes and try again.  If you continue to try, you will need to wait even longer.");
                	break;
	        case 989:
        	        snprintf(buf, sizeof(buf), "An unknown signon error has occurred: %s.", w);
                	break;
	        default:
        	        snprintf(buf, sizeof(buf), "An unknown error, %d, has occured.  Info: %s", no, w);
	}
	toc_msg_printf(TOC_TRANSLATED_ERROR,buf);
        return;
}
