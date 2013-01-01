/*
napster code base by Drago (drago@0x00.org)
released: 11-30-99
*/

#include <stdio.h>
#include <errno.h>
#include "commands.h"

#ifndef NAPI_H
#define NAPI_H

int n_serverfd;
int n_connectionspeed;
int n_dataport;

#define N_DEBUG
#define N_DISTRO_SERVER "server.napster.com"
#define N_DISTRO_SERVER_PORT 8875

/* Error faculty */
#define n_Error(fmt, args...) \
	do { \
		fprintf(stderr, "E:%s:%s():%d:%s:", __FILE__, __FUNCTION__, __LINE__, strerror(errno)); \
		fprintf(stderr, fmt, ## args); \
		fprintf(stderr, "\n"); \
	} while (0)

/* Debug faculty.....duh? */
#ifdef N_DEBUG
# define n_Debug(fmt, args...) \
	do { \
		fprintf(stderr, "D:%s:%s():%d:", __FILE__, __FUNCTION__, __LINE__); \
		fprintf(stderr, fmt, ## args); \
		fprintf(stderr, "\n"); \
	} while (0)
#else
# define n_Debug(fmt, args...) (void)0
#endif

typedef struct {
	char address[100];
	int port;
} _N_SERVER;

typedef struct {
	char username[100];
	char password[100];
} _N_AUTH;

typedef unsigned char _N_CMD;

typedef struct {
	_N_CMD cmd[4];
	char *data;
} _N_COMMAND;

typedef struct {
	int libraries;
	int gigs;
	int songs;
} _N_STATS;

/* Get a napster server */
_N_SERVER *n_GetServer(void);

/* Connect to a napster server */
int n_Connect(_N_SERVER *, _N_AUTH *);

/* nslookup helper function */
char *n_nslookup(char *addr);

/* Send a napster command */
void n_SendCommand(_N_CMD, char *, ...);

/* Send raw napster data */
int n_Send(char *, int);

/* This is the main napster loop */
int n_Loop(void);

/* Command handler */
_N_COMMAND *n_GetCommand(void);

/* Force a read of X bytes */
int n_ReadCount(char *, int);

/* MOTD hook handling */
void (*n_HookMotd)(char *);
void n_SetMotdHook(void (*)(char *));

/* Stats hook handling */
void (*n_HookStats)(_N_STATS *);
void n_SetStatsHook(void (*)(_N_STATS *));

void n_DoCommand(void);
void n_HandleCommand(_N_COMMAND *);

#endif
