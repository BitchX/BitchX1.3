/*
 * debug.h -- the runtime debug settings.  Can also be done on command line.
 */

#ifndef __X_DEBUG_H__
#define __X_DEBUG_H__

extern 	unsigned long x_debug;
extern	unsigned long internal_debug;
extern	unsigned long alias_debug;
extern	unsigned int debug_count;
extern	int in_debug_yell;

BUILT_IN_COMMAND(xdebugcmd);
void	debugyell	(const char *, ...);
void	debug_alias	(char *, int);
void	debug_hook	(char *, int);

#define DEBUG_LOCAL_VARS	1 << 0
#define DEBUG_ALIAS		1 << 1
#define DEBUG_CTCPS		1 << 2
#define DEBUG_DCC_SEARCH	1 << 3
#define DEBUG_OUTBOUND		1 << 4
#define DEBUG_INBOUND		1 << 5
#define DEBUG_DCC_XMIT		1 << 6
#define DEBUG_WAITS		1 << 7
#define DEBUG_MEMORY		1 << 8
#define DEBUG_SERVER_CONNECT	1 << 9
#define DEBUG_CRASH		1 << 10
#define DEBUG_COLOR		1 << 11
#define DEBUG_NOTIFY		1 << 12
#define DEBUG_REGEX		1 << 13
#define DEBUG_REGEX_DEBUG	1 << 14
#define DEBUG_BROKEN_CLOCK	1 << 15
#define DEBUG_UNKNOWN		1 << 16
#define DEBUG_DEBUGGER		1 << 17
#define DEBUG_NEW_MATH		1 << 18
#define DEBUG_NEW_MATH_DEBUG    1 << 19
#define DEBUG_AUTOKEY		1 << 20
#define DEBUG_STRUCTURES	1 << 21

#define DEBUG_ALL		(unsigned long)0xffffffff
#endif
