#include "tcpmark.h"
#include <math.h>

CONSTRUCTOR(Tcpmark, tcpmark);
USERCLASS("TCPMark", Tcpmark);

//////////////////////////////////////////////////////////////////////////
//  initialization
//////////////////////////////////////////////////////////////////////////
void tcpmark::init(void)
{
   // read parameters
   skip(CLASS);
   name = read_id(NULL);
   skip(':');

   if (test_word("MARK"))
   {
      mark = read_int("MARK");
      if(mark != 0 && mark != 1)
         syntax0("MARK must be 0 or 1");
      skip(',');
   }
   else
      mark = 1;

   vc = read_int("VC");
   if(vc <= 0)
      syntax0("VC must be > 0");
   skip(',');

   nflows = read_int("NFLOWS");
   if(nflows <= 0)
      syntax0("NFLOWS must be > 0");
   skip(',');

   NextMuxString = read_suc("MUXCLP");
   skip(',');

   output("OUTBACK", OutBack);         // output CORE-DATA
   skip(',');
   output("OUTDATA", OutData);         // output DATA
   
   input (NULL, InpData);            // DATA input
   input ("Back", InpBack);            // Backward input
  
   //nbytes0 = 0;
   //nbytes1 = 0;
   counter_bytes = 0;

} // init()

//////////////////////////////////////////////////////////////////////////
//  connect()
//  find the mux object (necessary for CLR)
//////////////////////////////////////////////////////////////////////////
void tcpmark::connect(void)
{
   root *obj;

   inxout::connect();

   if ((obj = find_obj(NextMuxString)) == NULL)
      syntax2s("%s: could not find object `%s'", name, NextMuxString);
   NextMux = (muxCLP*) obj;

   delete NextMuxString;

}


//////////////////////////////////////////////////////////////////////////
//  receiving a frame
//////////////////////////////////////////////////////////////////////////
rec_typ tcpmark::REC(data *pd, int key)
{
   double markprob;

   switch (key)
   {
      case InpBack:
      	 return sucs[OutBack]->rec(pd, shands[OutBack]);

//       case InpData:
//       
//       	 int bytes_vc;
// 	 double flow_ratio, clr0, clr1;
//       	 
// 	 if (!typequery(pd, FrameType))
// 	    errm1s("%s: arrived data item not of type Frame",name);
// 
//       	 counter++;
// 	 pd->clp = 0;
// 
// 	 clr0 = NextMux->getCLR0(vc);
// 	 clr1 = NextMux->getCLR1(vc);
// 	 bytes_vc = NextMux->getServedBytes(vc);
// 
// 	 if(bytes_vc > 0)
// 	    flow_ratio = (nbytes0 *(1.0-clr0) + nbytes1 *(1.0-clr1)) / (double) bytes_vc;
// 	 else
// 	    flow_ratio = 0.0;
// 	 
// 	 // now calculate the marking probability for this vc
// 	 markprob = clr0;
// 	 
// //	 double sr = 2.0;
// //       if(flow_ratio < 1.0/nflows/sr)
// // 	    markprob = 0.0;
// // 	 else if(flow_ratio > 1.0/nflows*sr)
// // 	    markprob = 1.0;
// // 	 else
// // 	    markprob *= (flow_ratio *nflows);
// 
//       	 if(flow_ratio < 1.0/nflows/2.0)
// 	    markprob = 0.0;
// 	 else if(flow_ratio > 1.5/nflows && clr0 > 0.01)
// 	    markprob = 1.0;
// 	 else
// 	    markprob *= (flow_ratio *nflows);
// 
// 	 if(uniform() <= markprob && mark)
// 	    pd->clp = 1;
// 
//       	 if(pd->clp == 0)
// 	    nbytes0 += (int)(( frame*) pd)->frameLen; // * (1-clr0);
// 	 else
// 	    nbytes1 += (int)(( frame*) pd)->frameLen; // * (1-clr1);
// 	 
//       	 return sucs[OutData]->rec(pd, shands[OutData]);
	 
      case InpData:
      
	 if (!typequery(pd, FrameType))
	    errm1s("%s: arrived data item not of type Frame",name);

	 int id = (( frame*) pd)->connID;

      	 counter++;
	 pd->clp = 0;
	 
	 int bytes_vc = NextMux->getBytes(vc);
	 double flow_ratio;
	 if(bytes_vc > 0 && counter_bytes > 0)
	    flow_ratio = counter_bytes / (double) bytes_vc;
	 else
	    flow_ratio = 1.0/nflows; // 0.0; // 1.0;

	 double clr = NextMux->getCLR(vc);

	 // now calculate the marking probability for this vc
	 markprob = clr; //1.0 - pow(1.0-clr, 32);

      	 if(flow_ratio < 1.0/nflows/2.0)
	    markprob = 0.0;
	 else if(flow_ratio > 1.0/nflows*2.0)
	    markprob = 1.0;
	 else
	    markprob *= flow_ratio;

	 if(uniform() <= markprob && mark)
	    pd->clp = 1;

      	 counter_bytes += (( frame*) pd)->frameLen;
	 
      	 return sucs[OutData]->rec(pd, shands[OutData]);
         
      default:
         errm1s("%s: Internal error: Receive data on input with no receive method.", name);
         return ContSend;

   } // switch()

} // REC()

