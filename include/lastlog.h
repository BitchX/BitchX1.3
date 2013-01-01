/*
 * lastlog.h: header for lastlog.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: lastlog.h 3 2008-02-25 09:49:14Z keaston $
 */

#ifndef __lastlog_h_
#define __lastlog_h_

#define LOG_NONE	0x00000000
#define LOG_CURRENT	0x00000000
#define LOG_CRAP	0x00000001
#define LOG_PUBLIC	0x00000002
#define LOG_MSG		0x00000004
#define LOG_NOTICE	0x00000008
#define LOG_WALL	0x00000010
#define LOG_WALLOP	0x00000020
#define LOG_NOTES	0x00000040
#define LOG_OPNOTE	0x00000080
#define	LOG_SNOTE	0x00000100
#define	LOG_ACTION	0x00000200
#define	LOG_DCC		0x00000400
#define LOG_CTCP	0x00000800
#define	LOG_USER1	0x00001000
#define LOG_USER2	0x00002000
#define LOG_USER3	0x00004000
#define LOG_USER4	0x00008000
#define LOG_USER5	0x00010000
#define LOG_BEEP	0x00020000
#define LOG_TCL		0x00040000
#define LOG_SEND_MSG	0x00080000
#define LOG_KILL	0x00100000
#define LOG_MODE_USER	0x00200000
#define LOG_MODE_CHAN	0x00400000
#define LOG_KICK	0x00800000
#define LOG_KICK_USER	0x01000000
#define LOG_PART	0x02000000
#define LOG_INVITE	0x04000000
#define LOG_JOIN	0x08000000
#define LOG_TOPIC	0x10000000
#define LOG_HELP	0x20000000
#define LOG_NOTIFY	0x40000000
#define LOG_DEBUG	0x80000000

#define LOG_ALL (LOG_CRAP | LOG_PUBLIC | LOG_MSG | LOG_NOTICE | LOG_WALL | \
		LOG_WALLOP | LOG_NOTES | LOG_OPNOTE | LOG_SNOTE | LOG_ACTION | \
		LOG_CTCP | LOG_DCC | LOG_USER1 | LOG_USER2 | LOG_USER3 | \
		LOG_USER4 | LOG_USER5 | LOG_BEEP | LOG_TCL | LOG_SEND_MSG | \
		LOG_MODE_USER | LOG_MODE_CHAN | LOG_KICK_USER | LOG_KICK | \
		LOG_PART | LOG_INVITE | LOG_JOIN | LOG_TOPIC | LOG_HELP | \
		LOG_KILL | LOG_NOTIFY)

# define LOG_DEFAULT	LOG_NONE

	void	set_lastlog_level (Window *, char *, int);
unsigned long	BX_set_lastlog_msg_level (unsigned long);
	void	set_lastlog_size (Window *, char *, int);
	void	set_notify_level (Window *, char *, int);
	void	set_msglog_level (Window *, char *, int);
	void	set_new_server_lastlog_level(Window *, char *, int);
	
	BUILT_IN_COMMAND(lastlog);

	void	add_to_lastlog (Window *, const char *);
	char	*bits_to_lastlog_level (unsigned long);
unsigned long	real_lastlog_level (void);
unsigned long	real_notify_level (void);
unsigned long	parse_lastlog_level (char *, int);
	int	islogged (Window *);
extern	void	remove_from_lastlog (Window *);
extern	int	grab_http (char *, char *, char *);

BUILT_IN_FUNCTION(function_line);
BUILT_IN_FUNCTION(function_lastlog);

extern unsigned long beep_on_level;
extern unsigned long new_server_lastlog_level;

	void	set_beep_on_msg(Window *, char *, int);
	Lastlog *get_lastlog_current_head(Window *);
	void	free_lastlog(Window *);
	int	logmsg(unsigned long, char *, int, char *, ...);
	void	reset_hold_mode(Window *);
		  				
#endif /* __lastlog_h_ */
