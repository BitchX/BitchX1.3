// Copyright (c) 1998-99, Ed Schlunder

#include <string.h>
#include "list.h"

// ---------------------------------------------------------------------------
// Class Implementation: List
// ---------------------------------------------------------------------------
List::List() {
  rootPtr = NULL;
  currPtr = NULL;
}

List::~List() {
  listNode *tmpPtr = rootPtr, *nextPtr;
  
  // go through the list and delete each node
  while(tmpPtr) {
    nextPtr = tmpPtr->nextPtr;
    if(tmpPtr->item)
      delete tmpPtr->item;
    if(tmpPtr->itemList)
      delete tmpPtr->itemList;
    if(tmpPtr->itemNode)
      delete tmpPtr->itemNode;
    
    delete tmpPtr;
    tmpPtr = nextPtr;
  }
};

void List::InsertBefore(char *item) {
  // this version does char strings

  if(rootPtr == NULL) {
    // creating the first node in a previously empty list.
    rootPtr = new listNode;
    
    rootPtr->item = new char[strlen(item) + 1];
    strcpy(rootPtr->item, item);
    rootPtr->itemList = NULL;
    rootPtr->itemNode = NULL;
    
    rootPtr->nextPtr = NULL;
    rootPtr->prevPtr = NULL;
    
    currPtr = rootPtr;
  }
  else {
    // create a new node and insert it before currPtr's node
    listNode *newPtr = new listNode;
    
    newPtr->nextPtr = currPtr;
    newPtr->prevPtr = currPtr->prevPtr;
    newPtr->item = new char[strlen(item) + 1];
    strcpy(newPtr->item, item);
    newPtr->itemList = NULL;
    newPtr->itemNode = NULL;
    
    currPtr->prevPtr = newPtr;
    if(newPtr->prevPtr)
      newPtr->prevPtr->nextPtr = newPtr;
    
    currPtr = newPtr;
  }
}

void List::InsertBefore(List *item) {
  // this version does Lists

  if(rootPtr == NULL) {
    rootPtr = new listNode;
    
    rootPtr->item = NULL;
    rootPtr->itemList = item;
    rootPtr->itemNode = NULL;
    
    rootPtr->nextPtr = NULL;
    rootPtr->prevPtr = NULL;
    
    currPtr = rootPtr;
  }
  else {
    listNode *newPtr = new listNode;
    
    newPtr->item = NULL;
    newPtr->itemList = item;
    newPtr->itemNode = NULL;
    newPtr->nextPtr = currPtr;
    newPtr->prevPtr = currPtr->prevPtr;
    
    currPtr->prevPtr = newPtr;
    if(newPtr->prevPtr)
      newPtr->prevPtr->nextPtr = newPtr;
    
    currPtr = newPtr;
  }
}

void List::InsertBefore(genericNode *item) {
  if(rootPtr == NULL) {
    rootPtr = new listNode;
    
    rootPtr->item = NULL;
    rootPtr->itemList = NULL;
    rootPtr->itemNode = item;
    
    rootPtr->nextPtr = NULL;
    rootPtr->prevPtr = NULL;
    
    currPtr = rootPtr;
  }
  else {
    listNode *newPtr = new listNode;
    
    newPtr->item = NULL;
    newPtr->itemList = NULL;
    newPtr->itemNode = item;
    newPtr->nextPtr = currPtr;
    newPtr->prevPtr = currPtr->prevPtr;
    
    currPtr->prevPtr = newPtr;
    if(newPtr->prevPtr)
      newPtr->prevPtr->nextPtr = newPtr;
    
    currPtr = newPtr;
  }
}

void List::InsertAfter(char *item) {
  if(rootPtr == NULL) {
    rootPtr = new listNode;
    rootPtr->prevPtr = NULL;
    rootPtr->nextPtr = NULL;
    rootPtr->itemList = NULL;
    rootPtr->itemNode = NULL;
    rootPtr->item = new char[strlen(item) + 1];
    strcpy(rootPtr->item, item);
    
    currPtr = rootPtr;
  }
  else {
    listNode *newPtr = new listNode;
    
    newPtr->nextPtr = currPtr->nextPtr;
    newPtr->prevPtr = currPtr;
    newPtr->itemList = NULL;
    newPtr->itemNode = NULL;
    newPtr->item = new char[strlen(item) + 1];
    strcpy(newPtr->item, item);
    
    if(newPtr->nextPtr)
      newPtr->nextPtr->prevPtr = newPtr;
    newPtr->prevPtr->nextPtr = newPtr;
    
    currPtr = newPtr;
  }
}

void List::InsertAfter(List *item) {
  if(rootPtr == NULL) {
    rootPtr = new listNode;
    rootPtr->prevPtr = NULL;
    rootPtr->nextPtr = NULL;
    rootPtr->item = NULL;
    rootPtr->itemList = item;
    rootPtr->itemNode = NULL;
    
    currPtr = rootPtr;
  }
  else {
    listNode *newPtr = new listNode;
    
    newPtr->nextPtr = currPtr->nextPtr;
    newPtr->prevPtr = currPtr;
    newPtr->item = NULL;
    newPtr->itemList = item;
    newPtr->itemNode = NULL;
    
    if(newPtr->nextPtr)
      newPtr->nextPtr->prevPtr = newPtr;
    newPtr->prevPtr->nextPtr = newPtr;
    
    currPtr = newPtr;
  }
}

void List::InsertAfter(genericNode *item) {
  if(rootPtr == NULL) {
    rootPtr = new listNode;
    rootPtr->prevPtr = NULL;
    rootPtr->nextPtr = NULL;
    rootPtr->item = NULL;
    rootPtr->itemList = NULL;
    rootPtr->itemNode = item;
    
    currPtr = rootPtr;
  }
  else {
    listNode *newPtr = new listNode;
    
    newPtr->nextPtr = currPtr->nextPtr;
    newPtr->prevPtr = currPtr;
    newPtr->item = NULL;
    newPtr->itemList = NULL;
    newPtr->itemNode = item;
    
    if(newPtr->nextPtr)
      newPtr->nextPtr->prevPtr = newPtr;
    newPtr->prevPtr->nextPtr = newPtr;
    
    currPtr = newPtr;
  }
}

bool List::GoTop(void) {
    currPtr = rootPtr;

    if(currPtr == NULL)
      return false;
    else
      return true;
}

bool List::GoNext(void) {
  if(!currPtr)
    return false;
  
  currPtr = currPtr->nextPtr;
  if(currPtr == NULL)
    return false;
  else
    return true;
}

bool List::GoPrev(void) {
  if(!currPtr)
    return false;
  
  currPtr = currPtr->prevPtr;
  return true;
}

bool List::GoItem(int n) {
  if(!rootPtr) // if empty list, abort
    return false;
  
  currPtr = rootPtr;
  
  while(n--) { // visit each node from the root up to n nodes deep
    if(!currPtr->nextPtr)
      break;
    
    currPtr = currPtr->nextPtr;
  }
  
  if(currPtr == NULL)
    return false;
  else
    return true;
}

char *List::currItem(void) {
  if(currPtr)
    return currPtr->item;
  else
    return NULL;
}

List *List::currItemList(void) {
  if(currPtr)
    return currPtr->itemList;
  else
    return NULL;
}

genericNode *List::currItemNode(void) {
  if(currPtr)
    return currPtr->itemNode;
  else
    return NULL;
}

void List::Print(void) {
  listNode *tmpPtr = rootPtr;
  
  cout << "(";
    while(tmpPtr) {
      if(tmpPtr->item)
	cout << tmpPtr->item << " ";
      
      if(tmpPtr->itemList)
	tmpPtr->itemList->Print();
      
      if(tmpPtr->itemNode)
	cout << tmpPtr->itemNode;
      
      tmpPtr = tmpPtr->nextPtr;
    }
    cout << "\b) ";
}


List *MakeList(char *text) {
  int length, i, j;
  char *tmp = text;
  List *l = new List;

  length = strlen(text);
  for(i = 0; i < length; i++) {
    if(text[i] == '{' || text[i] == '(') {
      // sub-list found
      for(j = i + 1; j < length; j++)
	if(text[j] == '}' || text[j] == ')')
	  break;

      tmp = new char[j - i];
      strncpy(tmp, text + i + 1, j - i - 1);
      tmp[j - i - 1] = 0;

      l->InsertAfter(MakeList(tmp));
      delete tmp;
      i = j + 1;
    }
    else if(text[i] != ' ' && text[i] != '\t' && text[i] != '.') {
      // item found
      for(j = i + 1; j < length; j++)
	if(text[j] == ' ' || text[j] == '\t' || text[j] == '.')
	  break;
      tmp = new char[j - i + 1];
      strncpy(tmp, text + i, j - i);
      tmp[j-i] = 0;

      l->InsertAfter(tmp);
      if(text[j] == '.')
	l->InsertAfter(".");
      delete tmp;
      i = j;
    }
  }

  return l;
}

List *MakeFeatList(char *text) {
  int length, i, j;
  char *tmp;
  List *l = new List;

  length = strlen(text);
  for(i = 0; i < length; i++) {
    for(j = i; j < length; j++)
      if(text[j] == ' ')
	break;

    for(j++; j < length; j++)
      if(text[j] == ' ' || text[j] == '}')
	break;

    if(text[j+1] == '{')
      for(j++; j < length; j++)
	if(text[j] == '}') {
	  j++;
	  break;
	}

    tmp = new char[j - i + 1];
    strncpy(tmp, text + i, j - i);
    tmp[j - i] = 0;
    //    cout << "Mklist: [" << tmp << "]" << endl;
    l->InsertAfter(MakeList(tmp));
    delete tmp;

    i = j;
  }

  return l;
}
// ---------------------------------------------------------------------------

// Creates a list of features from a string. Example string:
// ((CAT NP) (AGR ?a))
List *makeFeatureList(char *text) {
  int length, i, j, level, beg, end;
  char *tmp, *thisList;
  List *l = new List;

  length = strlen(text);

  // locate opening parenthesis
  for(i = 0; i < length; i++)
    if(text[i] == '(') break;

  // no list given if no opening parenthesis found -- abort
  if(i == length) return NULL;

  // get -past- the (
  i++;

  // find end of this list
  for(level = 1, j = i+1; j < length; j++) {
    if(text[j] == ')') level--;
    if(text[j] == '(') level++;
  }

  j--;

  // make a new string containing a copy of the list text we want to work
  // with only...
  thisList = new char[j - i + 1];
  strncpy(thisList, text + i, j - i);
  thisList[j - i] = 0;
  trim(thisList);

  //  cout << "thisList: [" << thisList << "]" << endl;

  // step through the list text and take out each item (*tmp)
  length = strlen(thisList);
  end = -1;
  while(end < length) {
    beg = end + 1;

    // find the end of this item
    for(level = 0, end = beg; end < length; end++) {
      if((thisList[end] == ' ') && (level == 0))
	break;

      // make sure that items encased within ( ) are kept as one piece
      if(thisList[end] == '(') level++;
      if(thisList[end] == ')') level--;
    }

    // make a copy of the text just for this item
    tmp = new char[end - beg + 1];
    strncpy(tmp, thisList + beg, end - beg);
    tmp[end - beg] = 0;

    // is this item a sub list?
    if(tmp[0] == '(')
      l->InsertAfter(makeFeatureList(tmp));
    else
      l->InsertAfter(tmp);

    delete tmp;
  }
  
  // free up memory for temporary list string
  delete thisList;
  return l;
}

ostream &operator<<(ostream &out_file, const List &l) {
  if(&l == NULL) {
    out_file << "nil";
    return out_file;
  }

  listNode *tmpPtr = l.rootPtr;
  
  // is this an empty list?
  if(tmpPtr == NULL)
    out_file << "()";
  else {
    out_file << "(";

    // print out first item without a separating space for it
    if(tmpPtr->item) out_file << tmpPtr->item;
    if(tmpPtr->itemList) out_file << *tmpPtr->itemList;
    if(tmpPtr->itemNode) out_file << *tmpPtr->itemNode;

    // step through the linked list of List items and print each one...
    while((tmpPtr = tmpPtr->nextPtr)) {
      if(tmpPtr->item) out_file << ' ' << tmpPtr->item;
      if(tmpPtr->itemList) out_file << ' ' << *tmpPtr->itemList;
      if(tmpPtr->itemNode) out_file << ' ' << *tmpPtr->itemNode;
    }
    out_file << ')';
  }

  return out_file;
}

bool List::empty(void) {
  if(rootPtr == NULL)
    return true;
  else
    return false;
}

int List::length(void) {
  listNode *tmp;
  int num;

  // count each node in the list and return the count
  tmp = rootPtr;
  num = 0;
  while(tmp) {
    num++;
    tmp = tmp->nextPtr;
  }

  return num;
}

List *findSubst(char *var, List *assign) {
  if(assign->GoTop() == false)
    return NULL;

  do {
    List *varSubst = assign->currItemList();
    varSubst->GoTop();

    // is this the variable that we are trying to find?
    if(strcmp(varSubst->currItem(), var) == 0) {
      varSubst->GoNext();
      return varSubst->currItemList();
    }
  } while(assign->GoNext());

  // couldn't find var in the variable substitution list
  return NULL;
}

List *substitute(List *old, List *assign) {
  List *subst = new List;
  listNode *tmp;

  tmp = old->rootPtr;
  while(tmp) {
    if(tmp->item) {
      //      cout << "recreating item: " << tmp->item << endl;
      if(tmp->item[0] == '?') {
	// variable may need to be substituted
	List *newItem = findSubst(tmp->item, assign);
	if(newItem == NULL)
	  subst->InsertAfter(tmp->item);
	else {
	  newItem->GoTop();
	  do {
	    if(newItem->currItem())
	      subst->InsertAfter(newItem->currItem());
	    if(newItem->currItemList())
	      subst->InsertAfter(newItem->currItemList());
	    if(newItem->currItemNode())
	      subst->InsertAfter(newItem->currItemNode());
	  } while(newItem->GoNext());
	}
      }
      else
	// constants just stay
	subst->InsertAfter(tmp->item);
    }

    // just call ourselves recursively for sublists
    if(tmp->itemList) {
      //      cout << "recreating list: " << *tmp->itemList << endl;
      subst->InsertAfter(substitute(tmp->itemList, assign));
    }

    if(tmp->itemNode) {
      //      cout << "recreating genericNode: " << *tmp->itemNode << endl;
      subst->InsertAfter(new genericNode(NULL, substitute(tmp->itemNode->features(), assign)));
    }

    tmp = tmp->nextPtr;
  }

  return subst;
}
