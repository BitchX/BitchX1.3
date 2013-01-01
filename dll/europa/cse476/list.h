#ifndef _LIST_H_
#define _LIST_H_

#include "gnode.h"

struct listNode;
typedef struct listNode {
  char *item;
  List *itemList;
  genericNode *itemNode;
  
  listNode *nextPtr;            // pointer to next option
  listNode *prevPtr;
};

class List {
public:
  List();
  ~List();
  void InsertBefore(char *item);
  void InsertBefore(List *item);
  void InsertBefore(genericNode *item);
  void InsertAfter(char *item);
  void InsertAfter(List *item);
  void InsertAfter(genericNode *item);

  // returns true if everything is good, false if operation failed
  bool GoTop(void); // GOOD
  bool GoNext(void); // GOOD
  bool GoPrev(void);
  bool GoItem(int n);
  bool empty(void); // GOOD
  int length(void); // GOOD

  char *currItem(void);
  List *currItemList(void);
  genericNode *currItemNode(void);
  void Print(void);

  friend List *MakeFeatList(char *text);

  friend List *makeFeatureList(char *text); // GOOD
  friend ostream &operator<<(ostream &out_file, const List &l); // GOOD
  friend List *substitute(List *old, List *assign);

 private:
  listNode *rootPtr;
  listNode *currPtr;
};

List *MakeList(char *text);

#endif
