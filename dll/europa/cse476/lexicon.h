#ifndef _LEXICON_H_
#define _LEXICON_H_

#include "gnode.h"
#include "list.h"

class Lexicon {
 public:
  Lexicon(char *fileName); // GOOD
  ~Lexicon();
  bool lookupWord(char *word);
  bool lookupNext(void);
  genericNode *currNode(void); // GOOD

 private:
  struct lexicalNode;
  typedef struct lexicalNode {
    genericNode *node;

    lexicalNode *leftPtr;
    lexicalNode *rightPtr;
  };

  void insert(genericNode *newWord); // GOOD 
  void insert(genericNode *newWord, lexicalNode *root); // GOOD

  bool lookupWord(lexicalNode *root, char *word);

  lexicalNode *rootPtr;
  lexicalNode *currPtr;
  List *featureSeek;
};

#endif
