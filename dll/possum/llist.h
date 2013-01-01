#ifndef LLIST_H
#define LLIST_H 1

struct lnode_struct {
  void *object;
  struct lnode_struct *prev, *next;
};

struct llist_struct {
  struct lnode_struct *first, *last, *current;
  size_t length, size;
};

typedef struct lnode_struct lnode;
typedef struct llist_struct llist;

llist *lmake(size_t size);
int ldelete(llist *l);
int lpush(llist *l, void *object);
void *lindex(llist *l, size_t x);


#endif

