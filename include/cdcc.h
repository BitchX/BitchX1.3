#ifndef _CDCC_H_
#define _CDCC_H_

#include <sys/types.h>
#include <sys/stat.h>

/* local commands */
extern double cdcc_minspeed;

typedef struct {
	char *name;
	int (*function)(char *args, char *rest);
	char *help;
} local_cmd;

/* remote commands */
typedef struct {
	char *name;
	int (*function)(char *from, char *args);
	char *help;
} remote_cmd;

/* offer pack type */
typedef struct packtype {
	struct packtype *next;
	int num;
	char *file;
	char *desc;
	char *notes;
	int numfiles;
	int gets;
	int server;
	time_t timeadded;
	unsigned long size;
	double minspeed;
	char	*password;
} pack;

/* cdcc queue struct */
typedef struct queuetype {
	struct queuetype *next;
	char *nick;
	char *file;
	int numfiles;
	time_t time;
	char *desc;
	char *command;
	int num;
	int server;
} queue;
	
/* local command parser */
void cdcc(char *, char *, char *, char *);

/* remote message command parser */
char *msgcdcc(char *, char *, char *);

/* send a file from the queue */
void dcc_sendfrom_queue (void);

void cdcc_timer_offer (void);

/* publicly list offered packs */
int l_plist(char *, char *);

int BX_get_num_queue(void);
int BX_add_to_queue(char *, char *, pack *);

#endif
