#ifndef SIM_INCL
#define SIM_INCL

#include "defs.h"

extern event	*eventse[TIME_LEN];
extern event	*eventsl[TIME_LEN];

int flushevents(int);

class sim: public root { 
   typedef root baseclass;
public:
   sim(void);
   void connect(void){}
   int run(int, int);
   int run(int);
   void stop(void);
   void reset(int);
   void SetRand(int n){my_srand(n);}
   int GetRand(void){return my_rand();}
   void ResetTime_(void);
   void SetSlotLength(double n){SlotLength=n;}
   virtual ~sim(void){}
};

extern sim _sim;

#endif
