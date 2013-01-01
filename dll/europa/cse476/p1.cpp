#include <iostream.h>
#include <fstream.h>
#include <string.h>

#include "parse.h"
#include "grammar.h"
#include "lexicon.h"
#include "list.h"
#include "gnode.h"

Grammar grammar("grammar.txt");
Lexicon lexicon("lexicon.txt");

int main(int argc, char *argv[]) {
  char *sentence = "Jack was happy to see a dog.";

//  cout << "[nlp]$ " << sentence << endl;
//  parse(sentence);

  sentence = new char[1000];
  cout << "[nlp]$ " << flush;
  cin.getline(sentence, 999);
//  while(cin) {
    if(strlen(sentence))
      parse(sentence);

//    cout << "[nlp]$ " << flush;
//    cin.getline(sentence, 999);
//  }

  cout << "Bye bye..." << endl;
}
