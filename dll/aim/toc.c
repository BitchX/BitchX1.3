/*
 * AOL Instant Messanger Module for BitchX 
 *
 * By Nadeem Riaz (nads@bleh.org)
 *
 * toc.c
 *
 * Interface to libtoc (libtoc -> client)
 */


#include <irc.h>
#include <struct.h>
#include <hook.h>
#include <ircaux.h>
#include <output.h>
#include <lastlog.h>
#include <status.h>
#include <vars.h>
#include <window.h>
#include <sys/stat.h>
#include <module.h>
#include <modval.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "toc.h"
#include "aim.h"

int sock_read_id;
void (*chatprintf)(char *, ...) = statusprintf;

/* LIBToc Call back functions */

int toc_add_input_stream(int fd,int (*func)(int)) {
	sock_read_id = add_socketread(fd, 0, 0, "bleh", func, func);	
        return 1;
}

int toc_remove_input_stream(int fd) {
	close_socketread(sock_read_id);
        return 1;
}

int toc_main_interface(int type, char **args) {
	
	switch (type) {
		case TOC_IM_IN: {
			char *msg, *nick;		
			nick = rm_space(args[0]);
			msg = strip_html(args[1]);
			RemoveFromLLByKey(msgdus,nick);
			AddToLL(msgdus,nick,NULL);	
			msgprintf("%s", cparse(fget_string_var(FORMAT_MSG_FSET), 
				"%s %s %s %s",update_clock(GET_TIME), 
				nick, "AIM", msg));			
			if ( is_away ) 
				serv_send_im(args[0],away_message);
			free(nick);
			break;
		}
		case TOC_TRANSLATED_ERROR:
		case TOC_CONNECT_MSGS:
			statusprintf(args[0]);
			break;
		case TOC_BUDDY_LOGGED_OFF:
			statusprintf("%s logged off",args[0]);
			if ( get_dllint_var("aim_window") )
				build_aim_status(get_window_by_name("AIM"));
			break;
		case TOC_BUDDY_LOGGED_ON:
			statusprintf("%s logged on", args[0]);
			if ( get_dllint_var("aim_window") )
				build_aim_status(get_window_by_name("AIM"));
			break;
		case TOC_EVILED:
			statusprintf("You have been warned by %s.", ((args[0] == NULL) ? "an anonymous person" : args[0])); 
			statusprintf("Your new warning level is %s%%" , args[1]);
			if ( get_dllint_var("aim_window") )
				build_aim_status(get_window_by_name("AIM"));
			break;
		case TOC_CHAT_JOIN:
			chatprintf("Joined buddy chat %s",args[1]);
			strncpy(current_chat,args[1],511);
			break;
		case TOC_BUDDY_LEFT_CHAT:
			chatprintf("%s left %s",args[1],args[0]);
			break;
		case TOC_BUDDY_JOIN_CHAT:
			chatprintf("%s joined %s",args[1],args[0]);
			break;
		case TOC_CHAT_LEFT:
			chatprintf("Left chat id: %s",args[0]);	
			break;
		case TOC_CHAT_IN: {
			char *e,*name,*chat;
			/* chatprintf("got msg from chat: <%s@AIM> %s",args[1],args[3]); */
			/* Need to take better action here */
			e = strip_html(args[3]);
			name = rm_space(args[1]);
			chat = rm_space(args[4]);
			msgprintf("%s",cparse(fget_string_var(FORMAT_PUBLIC_OTHER_FSET), "%s %s %s %s", update_clock(GET_TIME), name, chat, e));			
			free(name); free(chat);
			break;
		}
		case TOC_GOTO_URL: 
			statusprintf("GOTO_URL: %s",args[0]);
			break;
		case TOC_CHAT_INVITE:
			statusprintf("Invited to %s by %s '%s'",args[0],args[2],args[3]);
			break;
		case TOC_LAG_UPDATE:
		case TOC_WENT_IDLE:
			if ( get_dllint_var("aim_window") )
				build_aim_status(get_window_by_name("AIM"));
			break;					
		case TOC_DIR_STATUS:
			if ( atoi(args[0]) == 1 )
				statusprintf("Directory information successfully changed.");
			else
				statusprintf("Error altering directory information, error code: %s",args[0]);
			break;
		default:
			statusprintf("INTERNAL ERROR: Unknown toc type: %d",type);
	}
	return 1;
}

int toc_timer(int type, char **args) {
	timer_id = add_timer(0,"aimtime",20000,0,&check_idle,NULL,NULL,0,"aimtime");
	return 1;
}

/* int toc_buddy_logged_on( */

void bx_init_toc() {
	init_toc();
	strcpy(current_chat,"");
	/* Setup Hanlders */
	install_handler(TOC_IM_IN,&toc_main_interface);
	install_handler(TOC_TRANSLATED_ERROR,&toc_main_interface);
	install_handler(TOC_CONNECT_MSGS,&toc_main_interface);
	install_handler(TOC_BUDDY_LOGGED_ON,&toc_main_interface);
	install_handler(TOC_BUDDY_LOGGED_OFF,&toc_main_interface);
	install_handler(TOC_EVILED,&toc_main_interface);	
	install_handler(TOC_CHAT_JOIN,&toc_main_interface);
	install_handler(TOC_BUDDY_LEFT_CHAT,&toc_main_interface);
	install_handler(TOC_BUDDY_JOIN_CHAT,&toc_main_interface);
	install_handler(TOC_CHAT_LEFT,&toc_main_interface);
	install_handler(TOC_CHAT_IN,&toc_main_interface);
	install_handler(TOC_CHAT_INVITE,&toc_main_interface);
	install_handler(TOC_GOTO_URL,&toc_main_interface);
	install_handler(TOC_LAG_UPDATE,&toc_main_interface);
	install_handler(TOC_WENT_IDLE,&toc_main_interface);
	install_handler(TOC_DIR_STATUS,&toc_main_interface);
	install_handler(TOC_REINSTALL_TIMER,&toc_timer);	
}
