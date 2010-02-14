///////////////////////////////////////////////////////////////
// iwuvbr
// an interworking unit for IP to UBR
// it muxes several IP connections and marks the packets
// according to their bw share
//
// iwuubr <name>
//    NINP=<int>,   	   // number of inputs
//    MAXFLOW=<int>, 	   // max. number of flows [NINP]
//    SLOTLENGTH=<double>, // length of 1 slot (s)
//    PCR=<double>,   	   // net PCR of the ATM conn. (bit/s) [149.76e6]
//    BUFF=<int>, 	   // buffer size (packets)
//    STATTIMER=<double>,  // timer interval for fairness stat (s)
//    WQ=<double>          // weight of new value for moving exp. aver. of
//    	             	   // flow_ratio for fairness (0<WQ<1)
//    OUT=<suc>;     	   // next module
//      
///////////////////////////////////////////////////////////////
#include "iwuubr.h"

CONSTRUCTOR(Iwuubr,iwuubr);
USERCLASS("IWUUBR",Iwuubr);

//////////////////////////////////////////////////////////////
// void iwuubr::init()
// read in the command line arguments
//////////////////////////////////////////////////////////////
void iwuubr::init(void)
{	
   int i;
   double TI;
   
   skip(CLASS);
   name = read_id(NULL);
   skip(':');
   ninp = read_int("NINP");
   if (ninp < 1)
      syntax0("invalid NINP");
   skip(',');
 
   if (test_word("MAXFLOW"))
   {
      max_flow = read_int("MAXFLOW");
      if (max_flow < 1)
         syntax0("invalid MAXFLOW");
      skip(',');
   }
   else
      max_flow = ninp;

   nactive = max_flow;
 
   if (test_word("SLOTLENGTH"))
   {
      SlotLength = read_double("SLOTLENGTH");
      if(SlotLength <= 0)
         syntax0("SLOTLENGTH must be > 0");
      skip(',');
   }
   else
      SlotLength = 53.0*8.0 / 149.76 / 1e6;	// STM1
   
   PCR = read_double("PCR");
   if (PCR <= 0)
      syntax0("invalid PCR: must be >0");
   skip(',');

   maxbuff = read_int("BUFF");
   if (maxbuff <= 0)
      syntax0("invalid BUFF: must be >0");
   skip(',');

   TI = read_double("STATTIMER");
   if( TI <= 0)
         syntax0("STATTIMER must be > 0");
   skip(',');

   StatTimerInterval = TimeToSlot(TI);
   if(StatTimerInterval < 1)
      StatTimerInterval = 1;
   
   WQ = read_double("WQ");
   if(WQ < 0 || WQ > 1)
      syntax0("WQ must be 0<=WQ<=1");
   skip(',');

   output("OUT",SucData);
   
   // generate the inputs
   inputs("I",ninp,-1);

   CHECK(inp_buff = new inpstruct[ninp]);
      inp_ptr = inp_buff;

   // the connection parameter
   CHECK(connpar = new IWUUBRConnParam* [max_flow+1]);
   for (i = 0; i <= max_flow; i++)
      CHECK(connpar[i] = new IWUUBRConnParam(i));

   eache( &evtPCR); // alarm in each TimeSlot (this is very easy to implement)
   alarml(&evtStatTimer, (time_t) uniform()*StatTimerInterval);

} // iwuubr::init(void)


///////////////////////////////////////////////////////////////
// void iwuubr::REC()
// cells are received
//////////////////////////////////////////////////////////////
rec_typ iwuubr::REC(data *pd,int i)
{
   inp_ptr->inp = i;
   (inp_ptr++)->pdata = pd;
   if (!alarmed_late)
   {
      alarmed_late = TRUE;
      alarml( &std_evt, 0);
   }
   
   return ContSend;
   
} // iwuubr::REC()


////////////////////////////////////////////////////////////////////
// void iwuubr::late()
// - perform Timers and
// - read packets from input buffers and write them to the queue
//    mark packets with priorities according to their share of bw.
////////////////////////////////////////////////////////////////////
void   iwuubr::late(event *evt)
{
   /////////////////////////////////////////////////////////////////
   // Statistics Timer
   // calculates the exp. mov. average of flow_ratio for each flow,
   // resets the counters,
   /////////////////////////////////////////////////////////////////
   if(evt->key == keyStatTimer)
   {
      int i;
      double fr;

      nactive = 0;
      
      for(i=0; i<= max_flow; i++)
      {
	 if(bytes_sent > 0)
	 {
	    fr = connpar[i]->bytes_sent /(double) (bytes_sent);

	    if(connpar[i]->bytes_sent > 0)	// flow wants to send something
	       nactive++;
	 }
	 else
	    fr = 1.0/max_flow;	 // at the beginning all are treated equal
	 
	 connpar[i]->flow_ratio = connpar[i]->flow_ratio * (1.0-WQ) + fr*WQ;
         connpar[i]->bytes_sent = 0;
	 connpar[i]->bytes_sent0 = 0;
      }

      bytes_sent0 = 0;
      bytes_sent = 0;
      
      alarml(&evtStatTimer, StatTimerInterval); // alarm again
      return;

   } // Statistics Timer

   ///////////////////////
   // normal data
   ///////////////////////
   frame *pf;
   int n, cid, len;
   IWUUBRConnParam *cpar;
   inpstruct   *p;
   int todrop;
   double markprob;

   alarmed_late = FALSE;	// I am not alarmed anymore
   n = inp_ptr - inp_buff;	// number of cells to serve
   while (n > 0)
   {	
      if (n > 1)
         p = inp_buff + my_rand() % n;
      else
         p = inp_buff;
         
      if (!typequery(p->pdata, FrameType))
      	 errm1s("%s: arrived data item not of type Frame",name);

      pf = (frame *) p->pdata;
      cid = pf->connID;
      if (cid < 0 || cid > max_flow)
	 errm1s1d("%s: frame with invalid connID = %d received", name, cid);
      
      len = pf->frameLen;
      cpar = connpar[cid];
      
      //////////////////////////////////////////////////
      // check, if cells have to be dropped
      //////////////////////////////////////////////////
      if(buff < maxbuff)
         todrop = 0;
      else
         todrop = 1;

      if(todrop)
         dropItem(p);
      else
      {
	 // now calculate the marking probability for this flow
	 markprob = 0.0;
	 if(cpar->flow_ratio > 1.0/max_flow)
	 {
	    //markprob = 1.0-(1.0-cpar->flow_ratio)/max_flow;
	    markprob = cpar->flow_ratio;
	 }
	 
	 if(nactive == 1)
	 {
	    markprob = 0.0;
	 }

	 if(uniform() <= markprob)
	    pf->clp = 1;
	 else
	    pf->clp = 0;

	 q.enqueue(pf);       	       // enqueue the frame
	 buff++;
 
      } // else - not to drop

      *p = inp_buff[ --n];
      
   } // while(n>0) - as long as there are frames

   inp_ptr = inp_buff;

} // iwuubr::late(event *)

///////////////////////////////////////////////////////////////
// void iwuubr::early()
// send cells
//////////////////////////////////////////////////////////////
void iwuubr::early(event *evt)
{
   frame *pf;
   int len;
   int cid;

   if(SimTime < TimeSendNextPCR) // not the time to send because of PCR
      return;

   pf = (frame*) q.dequeue();
   if(pf != NULL)
   {
      len = pf->frameLen;
      cid = pf->connID;
      
      if(pf->clp == 0)
      {
         bytes_sent0 += len;
	 connpar[cid]->bytes_sent0 += len;
      }

      bytes_sent += len;
      connpar[cid]->bytes_sent += len;

      sucs[SucData]->rec(pf, shands[SucData]);  // send it
      buff--;

      // calculate next sending time
      if(TimeSendNextPCR < SimTime)
         TimeSendNextPCR = SimTime;
      TimeSendNextPCR = TimeSendNextPCR + len * 8.0 / PCR / SlotLength;

   } // if - frame available

   return;

} // iwuubr::early()


//////////////////////////////////////////////////////////////
// int iwuubr::export()
//////////////////////////////////////////////////////////////
int iwuubr::export(exp_typ   *msg)
{
   return baseclass::export(msg);
      
} // iwuubr::export()


//////////////////////////////////////////////////////////////
// int iwuubr::command()
/////////////////////////////////////////////////////////////
int iwuubr::command(char *s,tok_typ *pv)
{
   if (baseclass::command(s, pv))
      return TRUE;
   
   pv->tok = NILVAR;
   return TRUE;
   
} // iwuubr::command()

//////////////////////////////////////////////////////////////
// iwuubr::connect(void)
//////////////////////////////////////////////////////////////
void iwuubr::connect(void)
{
   baseclass::connect();

} // iwuubr::connect()

///////////////////////////////////////////////////////////////
// iwuubr::dropItem(inpstruct *p)
// register a loss and delete the data item
//////////////////////////////////////////////////////////////
inline void iwuubr::dropItem(inpstruct *p)
{
   int cid;
   cid = ((frame *) p->pdata)->connID;
   connpar[cid]->lost++;

   delete p->pdata;

} // void iwuubr::dropItem()
