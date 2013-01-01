// Copyright (c) 1998-99, Ed Schlunder

#include <iostream.h>
#include <fstream.h>
#include "lexicon.h"

// ---------------------------------------------------------------------------
// Class Implementation: Lexicon
// ---------------------------------------------------------------------------
Lexicon::Lexicon(char *fileName) {
  char buffer[1000];
  ifstream inFile;
  
  // specify that this is an empty lexicon currently
  rootPtr = currPtr = NULL;
  
  // try to open the lexicon file
  inFile.open(fileName);
  if(!inFile) {
    cout << "Lexicon file can not be opened." << endl;
    return;
  }

  // read in each lexicon entry and insert it into our BST
  while(inFile) {
    inFile.getline(buffer, 999);

    // ignore blank lines
    if(strlen(buffer) < 2) continue;

    insert(new genericNode(buffer));
  }
}

void Lexicon::insert(genericNode *newWord) {
#ifdef DEBUG
  cout << "insertLexicon: [" << *newWord << "]" << endl;
#endif

  // if this is an empty lexicon, insert at the top of the BST
  if(rootPtr == NULL) {
    rootPtr = new lexicalNode;
    rootPtr->node = newWord;
    rootPtr->leftPtr = NULL;
    rootPtr->rightPtr = NULL;
  }
  else
    insert(newWord, rootPtr);
}

void Lexicon::insert(genericNode *newWord, lexicalNode *root) {
  lexicalNode *tmp;
  
  // does newWord belong to the right of this node?
  if(strcmp(root->node->word(), newWord->word()) > 0)
    // ASSERT: newWord belongs to the right of this word
    if(root->rightPtr) {
      // nodes already exist to the right, follow link down and try again
      insert(newWord, root->rightPtr);
      return;
    }
    else
      // make a new node to the right
      tmp = root->rightPtr = new lexicalNode;
  else
    if(root->leftPtr) {
      insert(newWord, root->leftPtr);
      return;
    }
    else
      tmp = root->leftPtr = new lexicalNode;
  
  tmp->node = newWord;
  tmp->leftPtr = NULL;
  tmp->rightPtr = NULL;
}

Lexicon::~Lexicon() {
  // -----------------------
  // THIS NEEDS IMPLEMENTING
  // -----------------------
}

genericNode *Lexicon::currNode(void) {
  if(currPtr)
    return currPtr->node;
  
  return NULL;
}

// ---------------------------------------------------------------------------

bool Lexicon::lookupNext(void) {
  if(currPtr->leftPtr == NULL)
    return false;
  else
    return lookupWord(currPtr->leftPtr, currPtr->node->word());
}

bool Lexicon::lookupWord(char *word) {
  return lookupWord(rootPtr, word);
}

bool Lexicon::lookupWord(lexicalNode *root, char *word) {
  int cmp;
  
  // if we've hit a dead end, we know this word doesn't exist in the tree
  if(root == NULL) return false;

  // is this word the one we were looking for?
  cmp = strcmp(root->node->word(), word);
  if(cmp == 0) {
    currPtr = root;
    return true;
  }
  
  if(cmp > 0)
    return lookupWord(root->rightPtr, word);
  else
    return lookupWord(root->leftPtr, word);    
}
