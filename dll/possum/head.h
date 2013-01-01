#ifndef HEAD_H
#define HEAD_H 1

#include <ctype.h>
#include <sys/param.h>
#ifdef __EMX__
#include <sys/types.h>
#endif
#include <sys/stat.h>
#include <sys/time.h>

#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define   NOSTR           ((char *) 0)    /* Null string pointer */
#define   LINESIZE        BUFSIZ          /* max readable line width */


struct headline {
char    *l_from;     
char    *l_tty;
char    *l_date; 
};
                        

int ishead(char *);
void fail(char *, char *);
void parse(char *, struct headline *, char *);
char *copyin(char *, char **);
int isdate(char *);
int cmatch(register char *, register char *);
char *nextword(register char *, register char *);
                
#endif

