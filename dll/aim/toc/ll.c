/* 
 * Simple linked list library (replaces GList stuff)
 * Yea, it isnt efficient, but its only meant to be used for buddy lists (n < 100) THAT AINT LARGE :)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ll.h"

/* Creation */

LL CreateLL() {
	LL newlist;
	LLE head;
	newlist = (LL) malloc(sizeof(struct _ll));
	head = (LLE) CreateLLE("head element",NULL,NULL);
	if ( ! head )
		return NULL;
	newlist->head = head;
	newlist->items = 0;
	newlist->curr = head;
	newlist->free_e = NULL;
	return newlist;
}

LLE CreateLLE (char *key, void *data, LLE next) {
	LLE newe;
	newe = (LLE) malloc(sizeof(struct _lle)); 
	if ( ! newe ) {
		perror("MEM allocation errory!");
		return NULL;
	}
	newe->key = (char *) malloc(strlen(key)+1);
	strcpy(newe->key,key);
	newe->data = data;
	newe->next = next;
	return newe;
}

void SetFreeLLE(LL List, void (*free_e)(void *)) {
	List->free_e = free_e;
}

int AddToLL(LL List, char *key, void *data) {
	LLE p = List->head;
	LLE e;
	while ( p->next != NULL ) {
		p = p->next;
	}
	e = CreateLLE(key,data,NULL);
	p->next = e;
	List->items++;
	return 1;
}

LLE FindInLL(LL List, char *key) {
	LLE p = List->head->next;	
	while ( p != NULL ) {
	/*	debug_printf("p != null, key = '%s'",p->key); */
		if ( ! strcasecmp(p->key,key) ) 
			break;
		p = p->next;
	}	
	return p;	
}

void *GetDataFromLLE(LLE e) {
	if ( e == NULL ) 
		return NULL;
	else 
		return e->data;
}

/* Removing Items from List */

int RemoveFromLL(LL List, LLE e) {
	LLE p = List->head;
	LLE b = NULL;
	while ( p != NULL && p != e ) {
		b = p;
		p = p->next;
	}	
	if ( p == NULL ) 
		return -1;
	b->next = p->next;
	FreeLLE(p, List->free_e);	
	List->items--;
	return 1;
}

int RemoveFromLLByKey(LL List, char *key) {
	LLE b = List->head;
	LLE p = b->next;	
	while ( p != NULL ) {
		if ( ! strcasecmp(p->key,key) ) 
			break;
		b = p;	
		p = p->next;
	}	
	if ( p == NULL ) 
		return -1;
	b->next = p->next;
	FreeLLE(p, List->free_e);	
	List->items--;
	return 1;
}

/* For easy loop traversals */

LLE GetNextLLE(LL List) {
	if ( List->curr != NULL ) 
		List->curr = List->curr->next;
	return List->curr;
}

void ResetLLPosition(LL List) {
	List->curr = List->head;
}

/* Only Free the keys at the moment */

void FreeLLE(LLE e, void (*free_e)(void *)) {
	if ( e->key != NULL )
		free(e->key);
	if ( free_e != NULL && e->data != NULL) 
		free_e(e->data); 
	free(e);
	return;
}

void FreeLL(LL List) {
	LLE e; 
	LLE n;
	if ( List == NULL ) {
		perror("SERIOUS ERROR: tried to free null list!");
		return;
	}	
	e = List->head->next;
	free(List->head);
	while ( e != NULL ) {
		n = e->next;
		FreeLLE(e, List->free_e);
		e = n;
	}
	free(List);  
	return;
}
