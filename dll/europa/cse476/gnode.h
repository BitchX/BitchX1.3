#ifndef _GNODE_H_
#define _GNODE_H_

#include <iostream.h>

class List;

void trim(char *text); // GOOD

// genericNode -- holds any lexical or phrase object
class genericNode {
 public:
  // constructor: pass it a string containing a lexical or phrase
  // object in the format used in our lexicon.txt and grammar.txt
  genericNode(char *obj); // GOOD
  genericNode(char *word, List *features); // GOOD except feature doesn't copy
  ~genericNode();

  friend ostream &operator<<(ostream &out_file, const genericNode &n); // GOOD

  // GOOD: sort of.. this routine leaks memory because the lists for
  // assignment it makes aren't properly deallocated upon unification failure
  friend List *unify(genericNode &s, genericNode &g);
  friend List *cmpFeatures(List &a, List &b);
  List *lookupFeature(const char *name) const;
  friend genericNode *substitute(genericNode *old, List *assign);

  char *word(void); // GOOD
  List *features(void);

 private:
  char *name;
  List *featureList;
};

#endif
