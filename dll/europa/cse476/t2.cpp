#include <iostream.h>
#include "gnode.h"

int main(int argc, char *argv[]) {
  genericNode a("((CAT (V)) (SUBCAT (_vp:inf)) (AGR (?a)) (VFORM (?v)))");
  genericNode b("want ((CAT (V)) (ROOT (WANT1)) (VFORM (base)) (SUBCAT (_np _vp:inf _np_vp:inf)))");

  cout << "A: " << a << endl;
  cout << "B: " << b << endl;

  cout << "unify(b, a): " << unify(b, a) << endl;
}
