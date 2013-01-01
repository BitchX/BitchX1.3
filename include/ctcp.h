/*
 * ctcp.h: header file for ctcp.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: ctcp.h 40 2008-05-07 11:07:31Z keaston $
 */

#ifndef _CTCP_H_
#define _CTCP_H_

#include "irc.h"
#include "irc_std.h"

#define CTCP_PRIVMSG 	0
#define CTCP_NOTICE 	1

#define CTCP_SED	0
#define CTCP_UTC	1
#define CTCP_ACTION	2
#define CTCP_DCC	3
#define CTCP_CDCC	4
#define CTCP_CDCC1	5
#define CTCP_CDCC2	6

#define CTCP_VERSION	7
#define CTCP_CLIENTINFO	8
#define CTCP_USERINFO	9
#define CTCP_ERRMSG	10
#define CTCP_FINGER	11
#define CTCP_TIME	12
#define CTCP_PING	13
#define	CTCP_ECHO	14
#define CTCP_INVITE	15
#define CTCP_WHOAMI	16
#define CTCP_OP		17
#define CTCP_OPS	18
#define CTCP_IDENT	19
#define CTCP_UNBAN	20
#define CTCP_BOTLINK	21
#define CTCP_UPTIME	22
#define CTCP_CUSTOM	23

#define NUMBER_OF_CTCPS	CTCP_CUSTOM

#define CTCP_SPECIAL    0       /* Limited, quiet, noreply, not expanded */
#define CTCP_REPLY      1       /* Sends a reply (not significant) */
#define CTCP_INLINE     2       /* Expands to an inline value */
#define CTCP_NOLIMIT    4       /* Limit of one per privmsg. */
#define CTCP_TELLUSER   8       /* Tell the user about it. */

extern CtcpEntryDll *dll_ctcp;


extern		int	sed;
extern		int	in_ctcp_flag;

#define CTCP_DELIM_CHAR '\001'
#define CTCP_DELIM_STR 	"\001"
#define CTCP_QUOTE_CHAR '\\'
#define CTCP_QUOTE_STR 	"\\"
#define CTCP_QUOTE_EM 	"\r\n\001\\"


extern	char *	ctcp_quote_it (char *, int);
extern	char *	ctcp_unquote_it (char *, int *);
extern	char *	do_ctcp (char *, char *, char *);
extern	char *	do_notice_ctcp (char *, char *, char *);
extern	int	in_ctcp (void);
extern	void	send_ctcp (int, char *, int, char *, ...);
extern	int	get_ctcp_val (char *);

void 	BX_split_CTCP (char *, char *, char *);

#endif /* _CTCP_H_ */
