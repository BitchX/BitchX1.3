/*
 * who.h -- header info for the WHO, ISON, and USERHOST queues.
 * Copyright 1996 EPIC Software Labs
 */

#ifndef __who_h__
#define __who_h__

void clean_server_queues (int);

/* WHO queue */

typedef struct WhoEntryT
{
	int  dirty;
	int  piggyback;
        int  who_mask;
	char *who_target;
        char *who_name;
        char *who_host;
        char *who_server;
        char *who_nick;
        char *who_real;
	char *who_stuff;
	char *who_end;
	char *who_buff;
	char *who_args;
        struct WhoEntryT *next;
	void (*line) (struct WhoEntryT *, char *, char **);
	void (*end) (struct WhoEntryT *, char *, char **);

} WhoEntry;

BUILT_IN_COMMAND(whocmd);
void BX_whobase (char *, void (*)(WhoEntry *, char *, char **), void (*)(WhoEntry *, char *, char **), char *, ...);
void whoreply (char *, char **);
void who_end (char *, char **);



/* ISON queue */

typedef struct IsonEntryT
{
	char *ison_asked;
	char *ison_got;
	struct IsonEntryT *next;
	void (*line) (char *, char *);
} IsonEntry;

BUILT_IN_COMMAND(isoncmd);
void BX_isonbase (char *args, void (*line) (char *, char *));
void ison_returned (char *, char **);



/* USERHOST queue */

typedef struct UserhostItemT
{
	char *	nick;
	int   	oper;
	int	connected;
	int   	away;
	char *	user;
	char *	host;
} UserhostItem;

typedef struct UserhostEntryT
{
	char *userhost_asked;
	char *text;
	struct UserhostEntryT *next;
	void (*func) (UserhostItem *, char *, char *);
} UserhostEntry;

BUILT_IN_COMMAND(userhostcmd);
BUILT_IN_COMMAND(useripcmd);
BUILT_IN_COMMAND(usripcmd);
void BX_userhostbase (char *arg, void (*line) (UserhostItem *, char *, char *), int, char *, ...);
void userhost_returned (char *, char **);
void userhost_cmd_returned (UserhostItem *, char *, char *);

#endif 
