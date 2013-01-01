// Copyright (c) 1998-99, Ed Schlunder

#include <string.h>
#include "gnode.h"
#include "list.h"

// takes a string and hacks off leading/trailing spaces and *'s.
void trim(char *text) {
  int i, j, len;

  len = strlen(text);

  // empty strings don't need trimming...
  if(len == 0) return;

  // find beginning of actual text
  for(i = 0; i < len; i++)
    if(text[i] != ' ') break;

  // was there any preceeding white space to clear out?
  if(i > 0)
    for(j = 0; text[i] != 0; j++, i++)
      text[j] = text[i];

  // step from the end backwards to see find where we need to chop
  for(j = strlen(text); j > 1; j--)
    if(text[j-1] != ' ') {
      text[j] = 0;
      break;
    }
}


// ---------------------------------------------------------------------------
// Class Implementation: genericNode
// ---------------------------------------------------------------------------

genericNode::genericNode(char *word, List *features) {
  if(word) {
    name = new char[strlen(word)];
    strcpy(name, word);
  }
  else
    word = NULL;

  featureList = features;
}

// Example Phrasal: ((CAT N) (AGR ?a))
// Example Lexical: saw ((CAT N) (ROOT SAW1) (AGR 3s))
genericNode::genericNode(char *obj) {
  unsigned int i, j;

  // first try to read in the name field (in lexical objects)
  for(i = 0; i < strlen(obj); i++)
    if(obj[i] != ' ') break;
  
  // maybe this is a phrasal object -- doesn't have a "name" field
  if(obj[i] == '(') {
    name = NULL;
    j = i;
  }
  else {
    // this is a lexical object, pick up the name:
    for(j = i; j < strlen(obj); j++)
      if(obj[j] == ' ') break;
    
    name = new char[j - i + 1];
    strncpy(name, obj, j - i);
    name[j - i] = 0;
    j++;
  }

  // pick up the object's feature list
  char *tmp = obj + j;
  //  cout << "[" << tmp << "]" << endl;
  featureList = makeFeatureList(tmp);
}

ostream& operator<<(ostream &out_file, genericNode const &n) {
  if(n.name == NULL)
    out_file << "(nil";
  else
    out_file << '(' << n.name;

  if(n.featureList)
    out_file << ' ' << *n.featureList;

  out_file << ')';

  return out_file;
}

List *unify(genericNode &s, genericNode &g) {
  List &sFeatures = *s.featureList, &gFeatures = *g.featureList, *currFeature;
  List *assignList = new List;
  char *featureName;
  List *featureValueA, *featureValueB;

  // if a node has no feature list, then we automatically know it can unify
  if(!sFeatures.GoTop()) return new List;
  if(!gFeatures.GoTop()) return new List;
  
  do {
    // get the current feature
    currFeature = sFeatures.currItemList();
    currFeature->GoTop();

    // pull out the feature name and value
    featureName = currFeature->currItem();
    currFeature->GoNext();
    featureValueA = currFeature->currItemList();
    featureValueB = g.lookupFeature(featureName);

    /*
    cout << "unify: [" << featureName << "] = " << *featureValueA << "";
    cout << " with " << *featureValueB << " --> ";
    */

    /*
    if(featureValueB == NULL)
      // if feature lookup failed, no corresponding feature exists -- 
      // automatically agrees...
      //      cout << "yes" << endl;
    else {
    */
    // if featureB is nothing, no corresponding featuture exists -- agrees
    if(featureValueB != NULL) {
      List *assignment = cmpFeatures(*featureValueA, *featureValueB);
      if(assignment == NULL) {
	//	cout << "no" << endl;
	delete assignList;
	return NULL;
      }
      //      cout << "yes: " << *assignment << endl;
      if(!assignment->empty())
	assignList->InsertAfter(assignment);
    }
  } while(sFeatures.GoNext());
  //  cout << "assignment list: " << *assignList << endl;
  return assignList;
}

List *cmpFeatures(List &a, List &b) {
  char *valueA, *valueB;
  a.GoTop();

  do {
    valueA = a.currItem();

    b.GoTop();
    do {
      valueB = b.currItem();

      // do we have a match?
      if(strcmp(valueA, valueB) == 0)
	return new List;

      // if one of the values is a variable, then it can match anything
      if(valueA[0] == '?') {
	List *assignList = new List;
	assignList->InsertAfter(valueA);
	assignList->InsertAfter(&b);

	return assignList;
      }

      if(valueB[0] == '?') {
	List *assignList = new List;
	assignList->InsertAfter(valueB);
	assignList->InsertAfter(&a);

	return assignList;
      }
    } while(b.GoNext());
  } while(a.GoNext());

  return NULL;
}

List *genericNode::lookupFeature(const char *name) const {
  char *tmp;
  List *featureSeek;
  
  // if this node has no features, abort search
  if(featureList == NULL) return NULL;

  featureList->GoTop();
  do {
    // pick up current feature
    featureSeek = featureList->currItemList();
    featureSeek->GoTop();

    // pick up current feature's name
    tmp = featureSeek->currItem();

    // is this the feature we were trying to find?
    if(strcmp(tmp, name) == 0) {
      // found our feature, pick up the value
      featureSeek->GoNext();
      return featureSeek->currItemList();
    }
  } while(featureList->GoNext());

  // exhausted the feature list, this feature doesn't exist in this genericNode
  return NULL;
}

char *genericNode::word(void) {
  return name;
}

genericNode::~genericNode() {
  // -----------------------
  // THIS NEEDS IMPLEMENTING
  // -----------------------
  delete name;
  delete featureList;
}

List *genericNode::features(void) {
  return featureList;
}
// ---------------------------------------------------------------------------
genericNode *substitute(genericNode *old, List *assign) {
  genericNode *subst;

  subst = new genericNode(old->name, substitute(old->featureList, assign));
  return subst;
}
