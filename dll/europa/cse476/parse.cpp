#include "parse.h"
#include "list.h"
#include "grammar.h"
#include "lexicon.h"

extern Grammar grammar;
extern Lexicon lexicon;
Chart chart;
ActiveArcList arcs;

void parse(char *text) {
  List *s = MakeList(text);
  int i;

  cout << *s << endl;

  // go through each word and process
  s->GoTop();
  i = 1;
  do {
    if(lexicon.lookupWord(s->currItem()) == false) {
      cout << "Can't find [" << s->currItem() << "] in the lexicon..." << endl;
      return;
    }

    do {
      chart.add(i, i + 1, lexicon.currNode());
    } while(lexicon.lookupNext());

    processAgenda();

    i++;
  } while(s->GoNext());
}

void processAgenda(void) {
  int begLoc, endLoc;
  genericNode *obj;

  while(chart.process()) {
    List *newMatch;

    chart.getKey(begLoc, endLoc, obj);
    
    cout << "find: " << *obj << endl;
    newMatch = grammar.findMatch(obj);

    if(newMatch) {
      do {
	cout << "Grammar search success:\n" << *newMatch << endl;
	if(newMatch->length() <= 2) {
	  newMatch->GoTop();
	  chart.add(begLoc, endLoc, newMatch->currItemNode());
	}
	else {
	  activeArc *newArc;

	  newArc = new activeArc;
	  newArc->begLoc = begLoc;
	  newArc->endLoc = endLoc;
	  newArc->numFound = 1;
	  newArc->ruleLine = newMatch;
	  arcs.add(newArc);
	}
      } while((newMatch = grammar.findNext()));
    }
    else
      cout << "Grammar search fail" << endl;

    arcs.print();

    // if there is nothing on the active arc list, we can skip checking arcs
    if(!arcs.goTop())
      continue;
    
    do {
      activeArc *checkArc;
      
      checkArc = arcs.currArc();
      if(checkArc->endLoc == begLoc) {
	// find the current waiting component of this arc
	checkArc->ruleLine->GoTop();
	for(int i = 0; i <= checkArc->numFound; i++)
	  checkArc->ruleLine->GoNext();

	cout << "Arc search: " << *checkArc->ruleLine->currItemNode() << ", "
	     << *obj;
	List *assign = unify(*checkArc->ruleLine->currItemNode(), *obj);
       
	// did this arc unify with the current obj?
	if(!assign) {
	  cout << " = FAIL" << endl;
	}
	else {
	  cout << " = " << *assign << endl;
	  // do the variable substitutions
	  assign = substitute(checkArc->ruleLine, assign);
	  
	  if(assign->length() > (checkArc->numFound + 2)) {
	    // this arc has been extended, but not completed
	    cout << "Extended arc:\n" << *assign << "\n" << endl;
	    activeArc *newArc = new activeArc;
	    newArc->begLoc = checkArc->begLoc;
	    newArc->endLoc = endLoc;
	    newArc->numFound = checkArc->numFound + 1;
	    newArc->ruleLine = assign;
	    arcs.add(newArc);
	  }
	  else {
	    // this arc has been completed, we can add it to the chart
	    cout << "Completed arc:\n" << *assign << "\n" << endl;
	    assign->GoTop();
	    chart.add(checkArc->begLoc, endLoc, assign->currItemNode());	  
	  }
	}
      }
    } while(arcs.goNext());
  }

  chart.print();
}

Chart::Chart() {
  // mark this chart as being empty
  rootPtr = currPtr = NULL;
}

void Chart::add(int begLoc, int endLoc, genericNode *obj) {
  cout << '[' << begLoc << ',' << endLoc << ',' << *obj << ']' << endl;

  if(rootPtr == NULL) {
    rootPtr = new chartNode;
    rootPtr->begLoc = begLoc;
    rootPtr->endLoc = endLoc;
    rootPtr->obj = obj;
    rootPtr->processFlag = true;
    rootPtr->nextPtr = NULL;
  }
  else {
    chartNode *tmp = rootPtr;

    while(tmp->nextPtr != NULL)
      tmp = tmp->nextPtr;

    tmp->nextPtr = new chartNode;
    tmp = tmp->nextPtr;
    tmp->begLoc = begLoc;
    tmp->endLoc = endLoc;
    tmp->obj = obj;
    tmp->processFlag = true;
    tmp->nextPtr = NULL;
  }
}

bool Chart::process(void) {
  chartNode *tmp = rootPtr;

  while(tmp != NULL) {
    if(tmp->processFlag)
      return true;

    tmp = tmp->nextPtr;
  }

  return false;
}

void Chart::getKey(int &begLoc, int &endLoc, genericNode *&obj) {
  chartNode *tmp = rootPtr;

  while(tmp != NULL) {
    if(tmp->processFlag) {
      // this key hasn't been processed yet, return it and mark as processed
      begLoc = tmp->begLoc;
      endLoc = tmp->endLoc;
      obj = tmp->obj;
      tmp->processFlag = false;

      return;
    }

    tmp = tmp->nextPtr;
  }

  return;
}

void Chart::print(void) {
  cout << "Chart ";
  for(int i = 0; i < 160 - 6; i++)
    cout << '-';
  cout << endl;

  currPtr = rootPtr;
  while(currPtr) {
    cout << '[' << currPtr->begLoc << ',' << currPtr->endLoc << ','
	 << currPtr->processFlag << ',' << *currPtr->obj << ']' << endl;

    currPtr = currPtr->nextPtr;
  }
  cout << '\n' << endl;
}

bool Chart::findMatch(genericNode *obj, List *&assign, int begLoc, int &endLoc) {
  searchObj = obj;
  currPtr = rootPtr;

  // empty chart has nothing to match -- abort
  if(currPtr == NULL)
    return false;

  // go through each rule in the grammar and try to match it with the given obj
  while(currPtr) {
    if(currPtr->begLoc == begLoc) {
      assign = unify(*currPtr->obj, *searchObj);

      if(assign) {
	endLoc = currPtr->endLoc;
	return true;
      }
    }

    currPtr = currPtr->nextPtr;
  }

  return false;
}

ActiveArcList::ActiveArcList() {
  rootPtr = currPtr = NULL;
}

void ActiveArcList::add(activeArc *newArc) {
  List *assign;
  int endLoc;

  if(rootPtr) {
    arcNode *tmp = rootPtr;

    // skip down to the last node
    while(tmp->nextPtr)
      tmp = tmp->nextPtr;

    tmp->nextPtr = new arcNode;
    tmp = tmp->nextPtr;
    
    tmp->arc = newArc;
    tmp->nextPtr = NULL;
  }
  else {
    rootPtr = new arcNode;
    rootPtr->arc = newArc;
    rootPtr->nextPtr = NULL;
  }

  // whenever a new arc is added, we need to try to unify it with
  // any of the completed objects on the chart to see if it can be extended
  newArc->ruleLine->GoTop();
  for(int i = 0; i <= newArc->numFound; i++)
    newArc->ruleLine->GoNext();

  if(!chart.findMatch(newArc->ruleLine->currItemNode(), assign, newArc->endLoc, endLoc))
    return;

  assign = substitute(newArc->ruleLine, assign);
  if(assign->length() > (newArc->numFound + 2)) {
    cout << "Extended arc: " << newArc->numFound + 1 << "\n" << *assign << "\n" << endl;
    activeArc *newArc2 = new activeArc;
    newArc2->begLoc = newArc->begLoc;
    newArc2->endLoc = endLoc;
    newArc2->numFound = newArc->numFound + 1;
    arcs.add(newArc2);
  }
  else {
    // this arc has been completed, we can add it to the chart
    cout << "Completed arc:\n" << *assign << "\n" << endl;

    chart.print();

    assign->GoTop();
    chart.add(newArc->begLoc, endLoc, assign->currItemNode());
  }
}

bool ActiveArcList::goTop(void) {
  if(rootPtr) {
    currPtr = rootPtr;
    return true;
  }
  else {
    currPtr = NULL;
    return false;
  }
}

bool ActiveArcList::goNext(void) {
  if(currPtr && currPtr->nextPtr) {
    currPtr = currPtr->nextPtr;
    return true;
  }
  else {
    currPtr = NULL;
    return false;
  }
}

activeArc *ActiveArcList::currArc(void) {
  if(currPtr)
    return currPtr->arc;
  else
    return NULL;
}

void ActiveArcList::print(void) {
  cout << "Active Arcs ";
  for(int i = 0; i < 160-12; i++)
    cout << '-';
  cout << endl;

  currPtr = rootPtr;
  while(currPtr) {
    cout << '[' << currPtr->arc->begLoc << ',' << currPtr->arc->endLoc << ',';
    currPtr->arc->ruleLine->GoTop();
    cout << *currPtr->arc->ruleLine->currItemNode();
    for(int i = 0; i < currPtr->arc->numFound; i++) {
      currPtr->arc->ruleLine->GoNext();
      cout << ' ' << *currPtr->arc->ruleLine->currItemNode();
    }
    cout << " o";
    while(currPtr->arc->ruleLine->GoNext())
      cout << ' ' << *currPtr->arc->ruleLine->currItemNode();
    cout << ']' << endl;

    currPtr = currPtr->nextPtr;
  }

  cout << '\n' << endl;
}
