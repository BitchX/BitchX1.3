#ifndef _TOC_H
#define _TOC_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "ll.h"

/* TOC DEFS */
#define FLAPON "FLAPON\r\n\r\n"
#define MSG_LEN 2048
#define BUF_LEN MSG_LEN
#define MAX_OUTPUT_MSG_LEN 4096
#define BUF_LONG BUF_LEN * 2
#define LANGUAGE "english"
#define REVISION "gaim-libtoc:$Revision: 40 $"
#define ROAST "Tic/Toc"
#define TOC_HOST "toc.oscar.aol.com"
#define TOC_PORT 9898
#define AUTH_HOST "login.oscar.aol.com"
#define AUTH_PORT 5190
#define LAGOMETER_STR "123CHECKLAG456"

/* connection states */
#define STATE_OFFLINE 0
#define STATE_FLAPON 1
#define STATE_SIGNON_REQUEST 2
#define STATE_SIGNON_ACK 3
#define STATE_CONFIG 4
#define STATE_ONLINE 5

/* communication types */
#define TYPE_SIGNON    1  
#define TYPE_DATA      2
#define TYPE_ERROR     3
#define TYPE_SIGNOFF   4
#define TYPE_KEEPALIVE 5

/* permit modes */
#define PERMIT_PERMITALL  1
#define PERMIT_DENYALL    2
#define PERMIT_PERMITSOME 3
#define PERMIT_DENYSOME   4

/* User Types */
#define UC_AOL		1
#define UC_ADMIN 	2
#define UC_UNCONFIRMED	4
#define UC_NORMAL	8
#define UC_UNAVAILABLE  16


/* INTERFACE */

#define TOC_HANDLE 			1
#define TOC_RAW_HANDLE  		2

/* The following can be handlers in either normal or raw mode */

#define TOC_SIGN_ON			0
#define TOC_CONFIG			1
#define TOC_NICK 			2
#define TOC_IM_IN			3
#define TOC_UPDATE_BUDDY		4
#define TOC_ERROR			5
#define TOC_EVILED			6
#define TOC_CHAT_JOIN			7
#define TOC_CHAT_IN			8
#define TOC_CHAT_UPDATE_BUDDY		9
#define TOC_CHAT_INVITE			10
#define TOC_CHAT_LEFT			11
#define TOC_GOTO_URL 			12
#define TOC_DIR_STATUS			13

/* TEMP */
#define TOC_REINSTALL_TIMER		19

/* Special HANDLES -- can only be used in Normal mode */
		
#define TOC_SOCKFD			20
#define TOC_RM_SOCKFD			21
#define TOC_RECIEVED_IM 		TOC_IM_IN
#define TOC_BUDDY_LOGGED_ON		22
#define TOC_BUDDY_LOGGED_OFF		23
#define TOC_CONNECT_MSGS 		24
#define TOC_TRANSLATED_ERROR		25
#define TOC_BUDDY_LEFT_CHAT		26
#define TOC_BUDDY_JOIN_CHAT		27
#define TOC_LAG_UPDATE			28
#define TOC_WENT_IDLE			29


#define TOC_DEBUG_LOG "/tmp/aim-bx.log"

/* structs */
struct sflap_hdr {
	unsigned char ast;
	unsigned char type;
	unsigned short seqno;
	unsigned short len;
};

struct signon {
	unsigned int ver;
	unsigned short tag;
	unsigned short namelen;
	char username[80];
};


struct buddy {
	char name[80];
        int present;
        int log_timer;
	int evil;
	time_t signon;
	time_t idle;
        int uc;
};


struct group {
	char name[80];
	LL members;
};

struct buddy_chat {
        LL in_room;
        LL ignored;
	int makesound;
        int id;
	int init_chat;
	char name[80];
};



/* toc.c */
int toc_login(char *username, char *password);
int toc_signon(char *username, char *password);
int wait_reply(char *buffer, int buflen);
unsigned char *roast_password(char *pass);
char *print_header(void *hdr_v);
int toc_wait_signon();
char *toc_wait_config();
int sflap_send(char *buf, int olen, int type);
int toc_signoff();
void toc_close();
void toc_build_config(char *s, int len);
void parse_toc_buddy_list(char *);
void translate_toc_error_code(char *c);

extern int state;
/* extern int inpa; */


/* util.c */
void set_state(int i);
int escape_message(char *msg);
char *normalize(char *s);
void strdown(char *s);
int escape_text(char *msg);
void toc_debug_printf(char *fmt, ...);
void toc_msg_printf(int type, char *fmt, ...);
char *strip_html(char *text);


/* network.c */
unsigned int *get_address(char *hostname);
int connect_address(unsigned int addy, unsigned short port);


/* server.c */
void serv_finish_login();
void serv_add_buddy(char *name);
void serv_remove_buddy(char *name);
void serv_set_info(char *info);
void serv_get_info(char *name);
int serv_got_im(char *name, char *message, int away);
void serv_add_buddies(LL buddies);
void serv_send_im(char *name, char *message);
void serv_got_update(char *name, int loggedin, int evil, time_t signon, time_t idle, int type);
void serv_close();
void serv_save_config();
void serv_warn(char *name, int anon);
void serv_add_permit(char *);
void serv_add_deny(char *);
void serv_set_permit_deny();
void serv_got_joined_chat(int id, char *name);
void serv_got_chat_left(int id);
void serv_accept_chat(int);
void serv_join_chat(int, char *);
void serv_chat_invite(int, char *, char *);
void serv_chat_leave(int);
void serv_chat_whisper(int, char *, char *);
void serv_chat_send(int, char *);
void serv_chat_warn(int  id, char *user, int anon);
void serv_get_dir(char *name);
void serv_set_dir(char *first, char *middle, char *last, char *maiden, char *city, char *state, char *country, char *email, int web);
void serv_dir_search(char *first, char *middle, char *last, char *maiden, char *city, char *state, char *country, char *email);
void serv_touch_idle();
void serv_set_idle(int time);
int check_idle();
void serv_set_away(char *message);

extern int idle_timer;
extern time_t login_time;
extern int is_idle;
extern int lag_ms;
extern int permdeny;
extern int my_evil;
extern int is_away;
extern int time_to_idle;


/* misc.c */
void save_prefs();
void misc_free_group(void *);
void misc_free_buddy_chat(void *);
void misc_free_invited_chats(void *);

extern char aim_host[512];
extern int aim_port;
extern char login_host[512];
extern int login_port;
extern char toc_addy[16];
extern char aim_username[80];
extern char aim_password[16];
extern char *quad_addr;
extern char debug_buff[1024];
extern char user_info[2048];
extern int registered;
extern char *USER_CLASSES[5];
extern char *PERMIT_MODES[4];


/* buddy.c */
struct buddy *add_buddy(char *group, char *buddy);
struct buddy *find_buddy(char *who);
struct group *add_group(char *group);
struct group *find_group(char *group);
int user_add_buddy(char *group,char *buddy);
int user_remove_buddy(char *buddy);
int remove_group(char *group, char *newgroup, int mode);
int add_permit(char *sn);
int remove_permit(char *sn);
int add_deny(char *sn);
int remove_deny(char *sn);
int buddy_chat_invite(char *chat, char *buddy, char *msg);
void buddy_chat_join(char *chan);
int buddy_chat_leave(char *chan);
struct buddy_chat *find_buddy_chat(char *chat);
struct buddy_chat *buddy_chat_getbyid(int id);
int buddy_chat_warn(char *chat, char *user, int anon);

extern LL groups;
extern LL permit;  /* The list of people permitted */
extern LL deny;    /* The list of people denied */
extern LL buddy_chats;
extern LL invited_chats;


/* inteface.c */
void init_toc();
void init_lists();
int install_handler(int type, int (*func)(int, char **));
int install_raw_handler(int type, int (*func)(int, char *));
int use_handler(int mode,int type, void *args);

extern int (*TOC_RAW_HANDLERS[30])(int, char *); 
extern int (*TOC_HANDLERS[30])(int, char **);


/* EXTERNAL FUNCTIONS */

extern int toc_add_input_stream(int,int (*)(int));
extern int toc_remove_input_stream(int);	

#endif // _TOC_H
