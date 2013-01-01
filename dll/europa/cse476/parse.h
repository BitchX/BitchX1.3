#include "gnode.h"

void parse(char *text);
void processAgenda(void);

typedef struct activeArc {
  int begLoc;
  int endLoc;
  int numFound;
  List *ruleLine;
};

class ActiveArcList {
 public:
  ActiveArcList();
  void add(activeArc *newArc);
  List *findMatch(int begLoc, int endLoc, genericNode *obj);
  bool goTop(void);
  bool goNext(void);
  activeArc *currArc(void);
  void print(void);


 private:
  struct arcNode;
  typedef struct arcNode {
    activeArc *arc;
    arcNode *nextPtr;
  };

  arcNode *rootPtr;
  arcNode *currPtr;
};

class Chart {
 public:
  Chart();

  void add(int begLoc, int endLoc, genericNode *obj);
  void getKey(int &begLoc, int &endLoc, genericNode *&obj);
  
  // returns true if there are more keys on the agenda that need processing
  bool process(void); // GOOD

  bool findMatch(genericNode *obj, List *&assign, int begLoc, int &endLoc);
  List *findNext(void);

  void print(void);

 private:
  struct chartNode;
  typedef struct chartNode {
    int begLoc;
    int endLoc;
    genericNode *obj;
    bool processFlag;
    
    chartNode *nextPtr;
  };

  chartNode *rootPtr;
  chartNode *currPtr;
  genericNode *searchObj;
};
