
#ifndef _WhoWas_h
#define _WhoWas_h

#define WHOWAS_USERLIST_MAX 300
#define WHOWAS_REG_MAX 500
#define WHOWAS_CHAN_MAX 20
#include "hash.h"

typedef struct _whowaschan_str {
	struct _whowaschan_str *next;
	char *channel;
	int refnum;
	ChannelList *channellist;
	time_t time;
} WhowasChanList;

typedef struct _whowaswrapchan_str {
	HashEntry NickListTable[WHOWASLIST_HASHSIZE];
} WhowasWrapChanList;

typedef struct _whowas_str {
	struct _whowas_str *next;
	int	has_ops; 	/* true is user split away with opz */
	char 	*channel;	/* name of channel */
	time_t 	time;		/* time of split/leave */
	char	*server1;
	char	*server2;
	NickList *nicklist;	/* pointer to nicklist */
	ShitList *shitlist;	/* pointer to shitlist */
	ChannelList *channellist;		
} WhowasList;

typedef struct _whowas_wrap_str {
	unsigned long total_hits;
	unsigned long total_links;
	unsigned long total_unlinks;
	HashEntry NickListTable[WHOWASLIST_HASHSIZE];
} WhowasWrapList;

WhowasList *check_whowas_buffer (char *, char *, char *);
WhowasList *check_whowas_nick_buffer (char *, char *, int);
WhowasList *check_whosplitin_buffer (char *, char *, char *, int);

void add_to_whowas_buffer (NickList *, char *, char *, char *);
void add_to_whosplitin_buffer (NickList *, char *, char *, char *);

int remove_oldest_whowas (WhowasWrapList *, time_t, int);
void clean_whowas_list (void);
void sync_whowas_adduser (UserList *);
void sync_whowas_unuser (UserList *);
void sync_whowas_addshit (ShitList *);
void sync_whowas_unshit (ShitList *);

WhowasChanList *check_whowas_chan_buffer (char *, int, int);
void add_to_whowas_chan_buffer (ChannelList *);
int remove_oldest_chan_whowas (WhowasChanList **, time_t, int);
void clean_whowas_chan_list (void);
void show_whowas (void);
void show_wholeft (char *);

extern WhowasWrapList whowas_splitin_list;
#endif
