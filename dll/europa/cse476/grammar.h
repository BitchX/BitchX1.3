#ifndef _GRAMMAR_H_
#define _GRAMMAR_H_

#include "list.h"
#include "gnode.h"

class Grammar {
 public:
  Grammar(char *fileName); // GOOD
  ~Grammar();
  List *currLine(void);

  List *findMatch(genericNode *obj);
  List *findNext(void);

  void goTop(void);
  bool goNext(void);

 private:
  struct grammarLine;
  typedef struct grammarLine {
    List *line;
    grammarLine *nextPtr;
  };

  void insert(char *text); // GOOD
  void insert(List *ruleLine); // GOOD

  grammarLine *rootPtr;
  grammarLine *currPtr;
  genericNode *searchObj;
};

#endif
