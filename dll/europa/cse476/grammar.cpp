// Copyright (c) 1998-99, Ed Schlunder

#include <fstream.h>
#include <iostream.h>
#include <string.h>
#include "grammar.h"
#include "gnode.h"

Grammar::Grammar(char *fileName) {
  char buffer[1000];
  ifstream inFile;

  // specify that this is currently an empty grammar rule list
  rootPtr = currPtr = NULL;

  // try to open the grammar rules file
  inFile.open(fileName);
  if(!inFile) {
    cout << "Grammar rules file can not be opened." << endl;
    return;
  }

  while(inFile) {
    inFile.getline(buffer, 999);

    // ignore blank lines
    if(strlen(buffer) < 2) continue;
    trim(buffer);
    insert(buffer);
  }
}

void Grammar::insert(char *text) {
  List *ruleLine = new List;
  genericNode *obj;
  char *tmp;
  int length, level, beg, end;

  length = strlen(text);
  end = -1;
  while(end < length) {
    beg = end + 1;
    
    // locate opening parenthesis
    while(beg < length)
      if(text[beg] == '(') break;
      else beg++;

    for(level = 0, end = beg + 1; end < length; end++) {
      if((text[end] == ')') && (level == 0)) break;
      
      // make sure that items encased within () are kept intact
      if(text[end] == '(') level++;
      if(text[end] == ')') level--;
    }

    end++;
    tmp = new char[end - beg + 1];
    strncpy(tmp, text + beg, end - beg);
    tmp[end - beg] = 0;

    if(strlen(tmp) == 0)
      break;

    // make a genericNode out of this phrasal object and add it to the ruleLine
    //    cout << '[' << tmp << ']' << endl;
    obj = new genericNode(tmp);
    ruleLine->InsertAfter(obj);
  }

  insert(ruleLine);
}

void Grammar::insert(List *ruleLine) {
  grammarLine *tmp;

  //  cout << "grammarInsert: " << *ruleLine << endl;
  if(rootPtr == NULL) {
    rootPtr = new grammarLine;
    rootPtr->line = ruleLine;
    rootPtr->nextPtr = NULL;
  }
  else {
    // find end of list
    tmp = rootPtr;
    while(tmp->nextPtr != NULL)
      tmp = tmp->nextPtr;

    // tack a new rule on the end of the list
    tmp->nextPtr = new grammarLine;
    tmp = tmp->nextPtr;
    tmp->line = ruleLine;
    tmp->nextPtr = NULL;
  }
}

Grammar::~Grammar() {
  
}

List *Grammar::currLine(void) {
  return currPtr->line;
}

void Grammar::goTop(void) {
  currPtr = rootPtr;
}

bool Grammar::goNext(void) {
  if(currPtr)
    if(currPtr->nextPtr) {
      currPtr = currPtr->nextPtr;
      return true;
    }
  
  return false;
}

List *Grammar::findMatch(genericNode *obj) {
  searchObj = obj;
  currPtr = rootPtr;

  return findNext();
}

List *Grammar::findNext(void) {
  List *assign, *currLine;

  // go through each rule in the grammar and try to match it with the given obj
  while(currPtr) {
    currLine = currPtr->line;
    currLine->GoTop();
    currLine->GoNext();
    assign = unify(*currLine->currItemNode(), *searchObj);

    currPtr = currPtr->nextPtr;

    if(assign)
      return substitute(currLine, assign);
  }

  return NULL;
}
