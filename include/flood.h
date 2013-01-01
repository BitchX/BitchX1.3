/*
 * flood.h: header file for flood.c
 *
 * @(#)$Id: flood.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __flood_h_
#define __flood_h_

	int	BX_check_flooding (char *, int, char *, char *);
	int	BX_is_other_flood (ChannelList *, NickList *, int, int *);
	int	BX_flood_prot (char *, char *, char *, int, int, char *);
	void	clean_flood_list (void);		

#define MSG_FLOOD 	0x0001
#define PUBLIC_FLOOD 	0x0002
#define NOTICE_FLOOD	0x0004
#define WALL_FLOOD	0x0008
#define WALLOP_FLOOD	0x0010
#define CTCP_FLOOD	0x0020
#define INVITE_FLOOD	0x0040
#define CDCC_FLOOD	0x0080
#define CTCP_ACTION_FLOOD	0x0100
#define NICK_FLOOD	0x0200
#define DEOP_FLOOD	0x0400
#define KICK_FLOOD	0x0800
#define JOIN_FLOOD	0x1000

#include "hash.h"
#define FLOOD_HASHSIZE 31
extern HashEntry no_flood_list[FLOOD_HASHSIZE];

#endif /* __flood_h_ */
