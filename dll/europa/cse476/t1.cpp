#include <iostream.h>
#include "gnode.h"

int main(int argc, char *argv[]) {
  genericNode p("((CAT (VP)) (VFORM (inf)))");
  genericNode l("Jack ((CAT NAME) (AGR 3s))");

  cout << "P: " << p << endl;
  cout << "L: " << l << endl;
}
