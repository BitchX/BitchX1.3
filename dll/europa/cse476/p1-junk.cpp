// CSE476 Bottom Up Chart Parser
// Ed Schlunder (zilym@asu.edu)

#include <iostream.h>
#include <fstream.h>
#include <stdio.h>
#include <string.h>

#include "grammar.h"
#include "lexicon.h"

Lexicon lex("lexicon.txt");
Grammar gra("grammar.txt");

void procAgenda(genericNode *a) {
  List *currLine;
  genericNode *pred;
  char *category;

  category = a->Feature("CAT");
  cout << "  Unify searching for: " << category << endl;
  gra.GoTop();
  do {
    currLine = gra.currLine();
    currLine->GoTop();
    currLine->GoNext();
    pred = currLine->currItemNode();
    
    if(unify(a, pred)) {
      gra.Print();
      cout << endl;
    }

  } while(gra.GoNext());
  cout << endl;
}

// Jack was happy to see a dog
void Parse(char *sentence) {
  List *s = MakeList(sentence);

  cout << "Sentence: ";
  s->Print();
  cout << endl;

  s->GoTop();
  do {
    if(s->currItem() == NULL)
      break;

    cout << "Word: " << s->currItem() << endl;
    if(lex.Lookup(s->currItem())) {
      cout << "  CAT: " << lex.Feature("CAT") << endl;
      procAgenda(lex.currNode());
    }
    else
      cout << "  No lexical entry!" << endl;
  } while(s->GoNext());
}

int main(int argc, char *argv[]) {
  char sentence[1000];
  
  cout << "[nlp]$ Jack was happy to see a dog." << endl;
  Parse("Jack was happy to see a dog.");

  cout << "[nlp]$ " << flush;
  cin.getline(sentence, 999);
  while(cin) {
    Parse(sentence);

    cout << "[nlp]$ " << flush;
    cin.getline(sentence, 999);
  }

  cout << endl;

  /*
  char tmp[100];
  cin.getline(tmp, 99);
  while(cin) {
    cout << "Lexical Lookup: \"" << tmp << "\" ";
    if(lex.Lookup(tmp)) {
      cout << " - cat: " << lex.Feature("CAT");
      cout << " subcat: " << lex.Feature("SUBCAT");
      while(char *subcat = lex.Feature())
	cout << ", " << subcat;
      cout << endl;
    }
    else
      cout << "does not exist" << endl;

    cin.getline(tmp, 99);
  }
  */
}

