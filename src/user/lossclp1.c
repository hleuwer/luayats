#include "in1out.h"
/////////////////////////////////////////////////////////////////////////////
// LossCLP1
// Module, which discards CLP=1 data items with prob PLOSS
// it does not support Start/Stop protocol
// LossCLP1 <name>:
//    PLOSS=<double>, 	// loss probability
//    LOOSECLP0=0|1, 	// loose also CLP=0 cells
//    BURSTLEN=<int>,	// loose bursts of this size
//    OUT=<suc>;
/////////////////////////////////////////////////////////////////////////////

class lossclp1: public in1out
{
   typedef in1out baseclass;
   public:
   
      double ploss;  	// loss probability
      int LooseClp0; 	// loose also CLP0 cells
      int BurstLen; 	// loose Bursts of this length
      int Loosing;   	// am I loosing now
      int CurrentLen;	// data items currently lost or served
      int StartLoosing; // when to start with Loosing
      int received;  	// received data items
   
      double uniform(){return my_rand() / (32767.0+1.0);} // interval = [0,1) !
      void init();
      rec_typ REC(data *, int);
      
};

CONSTRUCTOR(Lossclp1, lossclp1);
USERCLASS("LossCLP1",Lossclp1);

/////////////////////////////////////////////////////////////////////////////
// init()
/////////////////////////////////////////////////////////////////////////////
void lossclp1::init(void)
{
   skip(CLASS);
   name = read_id(NULL);
   skip(':');
   
   ploss = read_double("PLOSS");
   if(ploss < 0 || ploss > 1)
      syntax0("PLOSS must be >=0 and <=1");
   skip(',');
   
   LooseClp0 = read_int("LOOSECLP0");
   if(LooseClp0 != 0 && LooseClp0 != 1)
      syntax0("LOOSECLP0 must be 0 or 1");
   skip(',');

   BurstLen = read_int("BURSTLEN");
   if(BurstLen < 1)
      syntax0("BURSTLEN must be >=1");
   skip(',');

   if(test_word("STARTLOOSING"))
   {
      StartLoosing = read_int("STARTLOOSING");
      if(StartLoosing < 1)
	 syntax0("STARTLOOSING must be >=1");
      skip(',');
   }
   else
      StartLoosing = 1;

   output("OUT");
   stdinp();
   
   CurrentLen = 0;
   Loosing = 0;
   if(uniform() < ploss)
      Loosing = 1;
   
   received = 0;

} // lossclp1:init()
	
/////////////////////////////////////////////////////////////////////////////
// rec()
/////////////////////////////////////////////////////////////////////////////
rec_typ lossclp1::REC(data *pd,int)
{

   received++;

   // serve CLP=0 items (if not LosseCLP0 is set)
   if(received < StartLoosing || (pd->clp == 0 && !LooseClp0))
   {
      suc->rec(pd, shand);
      return ContSend;
   }

   // now, we have a low priority cell
   // if the Burstlen for CLP=1 is over, decide, if to drop
   CurrentLen++;
   if(CurrentLen >= BurstLen)
   {
      CurrentLen = 0;
      Loosing = 0;

      if(uniform() < ploss)
         Loosing = 1;
   } 

   if( Loosing)
      delete pd;
   else
      suc->rec(pd, shand);

   return ContSend;

} // lossclp1::REC()
