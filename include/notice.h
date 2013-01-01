
#ifndef _NOTICE_H
#define _NOTICE_H

#ifdef WANT_OPERVIEW
#undef NEWNET_IRCOP

#define NICK_COLLIDE	0x000001
#define NICK_KILL	0x000002
#define IP_MISMATCH	0x000004
#define HACK_OPS	0x000008
#define IDENTD		0x000010
#define FAKE_MODE	0x000020
#define UNAUTHS		0x000040
#define TOO_MANY	0x000080
#define TRAFFIC		0x000100
#define REHASH		0x000200
#define KLINE		0x000400
#define POSSIBLE_BOT	0x000800
#define OPER_MODE	0x001000
#define SQUIT		0x002000
#define SERVER_CONNECT	0x004000
#define CLIENT_CONNECT	0x008000
#define TERM_FLOOD	0x010000
#define INVALID_USER	0x020000
#define STATS_REQUEST	0x040000
#define NICK_FLOODING	0x080000
#define KILL_ACTIVE	0x100000
#define SERVER_CRAP	0x200000

void	s_watch (char *, char *, char *, char *);
void	ov_window (char *, char *, char *, char *);
void	setup_ov_mode (int, int, int);
#endif


#endif
