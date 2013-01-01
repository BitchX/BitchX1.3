#include "struct.h"
#include "ircaux.h"
#include "server.h"
#include "output.h"
#include "module.h"
#include "parse.h"
#include "timer.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>

#define INIT_MODULE
#include "modval.h"

/*
 * I put this here because pana said I should use it, but its commented out
 * in ircaux.h :)
 */
#ifndef new_realloc
#define new_realloc(x,y) n_realloc((x),(y),__FILE__,__LINE__)
#endif

#define MINPLAYERS 2            /* Minimum number of players per game */
#define MAXPLAYERS 10           /* Max number of players per game */
#define MINLENGTH 3             /* Minimum number of letters per acronym */
#define MAXLENGTH 5             /* Max number of letters per acronym */
#define ROUNDS 10               /* Number of rounds to play */
#define EXTENSIONS 3            /* Max number of 30 second extensions */
#define TOP 10                  /* Show the top XX names for the scores */
#define MAXLEN 15               /* Max number of characters per acro word */

/*
 * These must be "proper" paths -- ie the ENTIRE path must exist and there
 * are no expansion characters in there (IE ~ or $(HOME))
 */

#define SCOREFILE ".BitchX/acro.score"
#define WEBSCORE "acro.html"

/* prec -- a linked list containing all the player info */

typedef struct _prec {
	char *nick;
	char *host;
	char *acro;
	char *last;
	struct _prec *next;
} prec;

/* vrec -- linked list storing voters, and who they voted for */

typedef struct _vrec {
	char *nick;
	char *host;
	int vote;
	struct _vrec *next;
} vrec;

/* grec -- struct containing the info about the current game */

typedef struct {
	int progress;
	int round;
	int rounds;
	int players;
	int extended;
	int top;
	int maxlen;
	char *nym;
} grec; 

/* srec -- linked list of scores */

typedef struct _srec {
	char *nick;
	unsigned long score;
	struct _srec *next;
} srec;

struct settings {
	int minplayers;
	int maxplayers;
	int minlength;
	int maxlength;
	int rounds;
	int extensions;
	int top;
	int maxlen;
	char *scorefile;
	char *webscore;
};

static char letters[] = "ABCDEFGHIJKLMNOPRSTUVWY";

static int acro_main (char *, char *, char *, char **);
int Acro_Init(IrcCommandDll **, Function_ptr *);
BUILT_IN_DLL(put_scores);
grec *init_acro(grec *);
void make_acro(grec *);
int valid_acro(grec *, char *);
srec *read_scores(void);
int write_scores(srec *);
prec *take_acro(grec *, prec *, char *, char *, char *);
vrec *take_vote(grec *, vrec *, prec *, char *, char *, char *);
srec *end_vote(vrec *, prec *, srec *);
srec *sort_scores(srec *);
int comp_score(srec **one, srec **two);
void show_scores(grec *, srec *, srec *, char *);
void warn_acro(char *);
void start_vote(char *);
void warn_vote(char *);
void end_voting(char *);
void show_acros(prec *, char *);
void free_round(prec **, vrec **);
void free_score(srec **);
/* static void game_setup _(( IrcCommandDll *, char *, char *, char *)); */
