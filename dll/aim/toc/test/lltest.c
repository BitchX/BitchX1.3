#include <stdio.h>
#include <stdlib.h>
#include "ll.h"

void myfunc(void *data) {
	char *t;
	t = (char *) data;
	printf("GOT data = %s\n",t);
	free(t);
}


int main() {
	LL l = CreateLL(); 
	LL z = CreateLL();
	LLE e;
	char *b;
	FreeLL(z);
	SetFreeLLE(l,&myfunc);
	b = (char *) malloc(1000);	
	strcpy(b,"I like you, you like me");
	AddToLL(l,"1",b);
	b = (char *) malloc(1000);
	strcpy(b,"or maybe not?");
	AddToLL(l,"2",b);
	b = (char *) malloc(1000);
	strcpy(b,"I hope you do at least!@$");
	AddToLL(l,"3",b);	
	b = (char *) malloc(1000);
	strcpy(b,"8if you dont, why the fuxor not1@$");
	AddToLL(l,"4",b);
	ResetLLPosition(l);
	while ( (e = GetNextLLE(l)) ) {
		printf("key = %s, data = %s\n",e->key,(char *)e->data);
	}
	printf("Going to TLL Traversal\n");
	for ( TLL(l,e) ) {
		printf("key = %s, data = %s\n",e->key,(char *)e->data);
	}
	ResetLLPosition(l);
	e = FindInLL(l,"3");
	printf("result of find = %s\n",(char *)e->data);
	RemoveFromLLByKey(l,"2");
        while ( (e = GetNextLLE(l)) ) {
                printf("key = %s, data = %s\n",e->key,(char *)e->data);
        }
	e = FindInLL(l,"9");
	if ( e ) {
		printf("Found 9\n");
	} else {
		printf("didnt find key 9\n");
	}
	FreeLL(l);
	printf("l is freed, all good!\n");
	l = CreateLL();
	printf("back here\n");
	printf("%d\n",sizeof(struct _lle));
	printf("%d\n",sizeof(LLE)); 
	return 1;
}
