/*
 * ignore.h: header for ignore.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef __ignore_h_
#define __ignore_h_

/* Type of ignored nicks */
#define IGNORE_MSGS	0x0001
#define IGNORE_PUBLIC	0x0002
#define IGNORE_WALLS	0x0004
#define IGNORE_WALLOPS	0x0008
#define IGNORE_INVITES	0x0010
#define IGNORE_NOTICES	0x0020
#define IGNORE_NOTES	0x0040
#define IGNORE_CTCPS	0x0080

#define IGNORE_CDCC	0x0100
#define IGNORE_KICKS	0x0200
#define IGNORE_MODES	0x0400
#define IGNORE_SMODES	0x0800
#define IGNORE_JOINS	0x1000
#define IGNORE_TOPICS	0x2000
#define IGNORE_QUITS	0x4000
#define IGNORE_PARTS	0x8000
#define IGNORE_NICKS	0x10000
#define IGNORE_PONGS	0x20000
#define IGNORE_SPLITS	0x40000
#define IGNORE_CRAP	0x80000


#define IGNORE_ALL	(IGNORE_MSGS | IGNORE_PUBLIC | IGNORE_WALLS | \
			IGNORE_WALLOPS | IGNORE_INVITES | IGNORE_NOTICES | \
			IGNORE_NOTES | IGNORE_CTCPS | IGNORE_CRAP | \
			IGNORE_CDCC | IGNORE_KICKS | IGNORE_MODES | \
			IGNORE_SMODES | IGNORE_JOINS | IGNORE_TOPICS | \
			IGNORE_QUITS | IGNORE_PARTS | IGNORE_NICKS | \
			IGNORE_PONGS | IGNORE_SPLITS)

#define IGNORED		1
#define DONT_IGNORE	2
#define HIGHLIGHTED	-1
#define CHANNEL_GREP	-2

extern	int	ignore_usernames;
extern	char	*highlight_char;
extern Ignore	*ignored_nicks;

int	is_ignored (char *, long);
int	check_ignore (char *, char *, char *, long, char *);
void	ignore (char *, char *, char *, char *);
void	tignore (char *, char *, char *, char *);
void	ignore_nickname (char *, long, int);
long	ignore_type (char *, int);
int	check_is_ignored(char *);
char 	*get_ignores_by_pattern (char *patterns, int covered);
int	get_type_by_desc (char *type, int *do_mask, int *dont_mask);
char	*get_ignore_types (Ignore *tmp);
char	*get_ignore_types_by_pattern (char *pattern);
char	*get_ignore_patterns_by_type (char *ctype);

#endif /* __ignore_h_ */
