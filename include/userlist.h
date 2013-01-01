/*
 * Userlist functions and definitions.
 * Copyright Colten Edwards 1996
 * 
 */
 
#ifndef _user_list_h
#define _user_list_h

void add_shit (char *, char *, char *, char *);
void add_user (char *, char *, char *, char *);
void showuserlist (char *, char *, char *, char *);
void change_user (char *, char *, char *, char *);
void savelists (char *, char *, char *, char *);

void add_to_a_list (char *, int, char *, char *, char *, int);
void showlist (NickList *, char *);
UserList *lookup_userlevelc (char *, char *, char *, char *);

UserList *nickinuser (char *, char *);
ShitList *nickinshit (char *, char *);

int	find_user_level (char *, char *, char *);
int	find_shit_level (char *, char *, char *);

NickList *check_auto (char *, NickList *, ChannelList *);
int check_prot (char *, char *, ChannelList *, BanList *, NickList *);
void check_shit (ChannelList *);
void check_hack (char *, ChannelList *, NickList *, char *);
int check_channel_match (char *, char *);
int delay_check_auto (char *);


extern ShitList *shitlist_list;
extern LameList *lame_list;
extern WordKickList *ban_words;

#define USERLIST_REMOVE 0
#define USERLIST_ADD 1
#define SHITLIST_ADD 11
#define SHITLIST_REMOVE 12

#define ADD_VOICE	0x00000001
#define ADD_OPS		0x00000002
#define ADD_BAN		0x00000004
#define ADD_UNBAN	0x00000008
#define ADD_INVITE	0x00000010
#define ADD_DCC		0x00000020
#define ADD_TCL		0x00000040
#define ADD_IOPS	0x00000080
#define ADD_FLOOD	0x00000100
#define ADD_BOT		0x00000200

#define PROT_REOP	0x00000400
#define PROT_DEOP	0x00000800
#define PROT_KICK	0x00001000
#define PROT_BAN	0x00002000
#define PROT_INVITE	0x00004000
#define USER_FLAG_OPS	0x00008000
#define USER_FLAG_PROT	0x00010000
#define ADD_CTCP	0x00100000

#define ADD_FRIEND	(ADD_VOICE|ADD_OPS|ADD_UNBAN|ADD_INVITE)
#define ADD_MASTER	(ADD_VOICE|ADD_OPS|ADD_BAN|ADD_UNBAN|ADD_INVITE|ADD_DCC|ADD_FLOOD)
#define ADD_OWNER	(ADD_MASTER|ADD_BOT|ADD_CTCP)
#define PROT_ALL	(PROT_REOP|PROT_DEOP|PROT_KICK|PROT_BAN|PROT_INVITE)

#define SHIT_NOOP	0x0001
#define SHIT_KICK	0x0002
#define SHIT_KICKBAN	0x0004
#define SHIT_PERMBAN	0x0008
#define SHIT_IGNORE	0x0010

/* user.c functions for dealing with hashed userlist */
UserList *find_bestmatch(char *, char *, char *, char *);
char * convert_flags(unsigned long flags);
UserList *find_userlist(char *, char *, int);
void add_userlist(UserList *);
UserList *next_userlist(UserList *, int *, void **);
void destroy_sorted_userlist(UserList **);
UserList *create_sorted_userlist(void);
char *convert_flags_to_str(unsigned long);
unsigned long convert_str_to_flags(char *);
int change_pass(char *, char *);

#endif

