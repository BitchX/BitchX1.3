#include <stdlib.h>
#include "llist.h"

llist *lmake(size_t size) {
  llist *l;
  l = (llist *) malloc(sizeof(llist));
  if (l) {
    l->first = l->last = l->current = NULL;
    l->size = size;
    l->length = 0;
  } else l = NULL; /* just making sure malloc returns NULL on a fail */
  return (l);
}

int ldelete(llist *l) {
  int retval;
  if (l) {
    l->current = l->first; /* make sure we are at the start */
    while (l->first) { 
      l->current = l->first; /* save the position */
      l->first = l->first->next; /* save the next position */

      free(l->current->object);
      free(l->current);
    }
    free(l);
    retval = 0; /* all done deleting */
  } else 
    retval = 0; /* yeah we deleted nothing */

  return(retval);
}

int lpush(llist *l, void *object) {
  lnode *new_node;
  int retval;

  new_node = (lnode *) malloc(sizeof(lnode));
  if (new_node) {
    new_node->object = (void *) malloc(l->size);
    if (new_node->object) {
      new_node->next = new_node->prev = NULL;
      memcpy(new_node->object, object, l->size);
      l->length++;

      if (l->first == NULL) l->first = l->last = l->current = new_node;
      else if (l->last) {
        l->last->next = new_node;
        new_node->prev = l->last;
        l->last = l->last->next;
      }
      retval = 0;
    } else {
      free(new_node);
      retval = 1;
    }
  } else 
    retval = 1;

  return(retval);
}

void *lindex(llist *l, size_t x) {
  int z;
  lnode *tmp = NULL;
  void *retval;

/*  while (x > l->length) 
    x = x - l->length; *//* we are allowed to wrap the list */
  if (x > l->length) return NULL;

  l->current = l->first;

  for (z = 0; z <= x; z++) {
    if (!l->current) break;
    tmp = l->current;
    l->current = l->current->next;
  }

  if (tmp)
    retval = tmp->object;
  else
    retval = NULL;
  
  return(retval);
}
