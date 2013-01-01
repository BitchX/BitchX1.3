#ifndef _LL_H
#define _LL_H

/*
 * Really bad list implementation
 */

#define TLL(list,e) e = list->head->next; e; e = e->next

struct _lle {
	char *key;
	void *data;	
	struct _lle *next;
};

typedef struct _lle * LLE;

struct _ll {
	LLE head;
	LLE curr;
	void (*free_e)(void *);
	int items;
	
};

typedef struct _ll * LL;

LL CreateLL();
void SetFreeLLE(LL List, void (*free_e)(void *));
LLE CreateLLE (char *key, void *data, LLE next);
int AddToLL(LL List, char *key, void *data);
LLE FindInLL(LL List, char *key);
void *GetDataFromLLE(LLE e);
int RemoveFromLL(LL List, LLE e);
int RemoveFromLLByKey(LL List, char *key);
LLE GetNextLLE(LL List);
void ResetLLPosition(LL List);
void FreeLLE(LLE e, void (*free_e)(void *));
void FreeLL(LL List);


/* Internal */

#endif // _LL_H
