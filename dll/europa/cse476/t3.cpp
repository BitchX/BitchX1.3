#include "lexicon.h"

int main(int argc, char *argv[]) {
  Lexicon lex("lexicon.txt");
  bool res;

  res = lex.lookupWord("want");
  cout << "lookupWord(\"want\"): " << res << endl;
  if(res)
    cout << "  gnode: " << *lex.currNode() << endl;  
}
