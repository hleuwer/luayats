///////////////////////////////////////////////////////////////
// iwumark_lb
// muxes several IP connections and marks the packets IN (CLP=0)
//    or OUT (CLP=1)
//
// iwumark_lb <name>
//    NINP=<int>,   	   // number of inputs
//    MAXFLOW=<int>, 	   // max. number of flows [NINP]
//    SLOTLENGTH=<double>, // length of 1 slot (s)
//    SCR=<double>,   	   // net SCR of the ATM conn. (bit/s)
//    PCR=<double>,   	   // net PCR of the ATM conn. (bit/s) [149.76e6]
//    BUFF=<int>, 	   // buffer size (packets)
//    STATTIMER=<double>,  // timer interval for fairness stat (s)
//    WQ=<double>,         // weight of new value for moving exp. aver. of
//    	             	   // flow_ratio for fairness (0<WQ<1)
//    MARKMETHOD=<int>,    // Marking Method (for explaination see [YR99])
//    	             	   // 0 - no marking
//    	             	   // 1 - bw protortional marking (no flow awareness)
//    	             	   // 2 - bw fair marking
//    	             	   // 3 - IN fair marking
//    RESLEVEL=<double>,   // the reservation level
//    {NACTIVE_ON=<int>,}  // dynamic number of connection on? std=0
//    OUT=<suc>;     	   // next module
//      
///////////////////////////////////////////////////////////////
#include "iwumark_lb.h"

CONSTRUCTOR(Iwumark_lb,iwumark_lb);
USERCLASS("IWUMARK_LB",Iwumark_lb);

//////////////////////////////////////////////////////////////
// void iwumark_lb::init()
// read in the command line arguments
//////////////////////////////////////////////////////////////
void iwumark_lb::init(void)
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
   

   SCR = read_double("SCR");
   if (SCR <= 0)
      syntax0("invalid SCR: must be >0");
   skip(',');

   PCR = read_double("PCR");
   if (PCR <= 0)
      syntax0("invalid PCR: must be >0");
   skip(',');

   if(SCR > PCR)
      syntax0("SCR must be <= PCR");


   maxbuff = read_int("BUFF");
   if (maxbuff <= 0)
      syntax0("invalid BUFF: must be >0");
   skip(',');

   TI = read_double("STATTIMER");
   if( TI <= 0)
         syntax0("STATTIMER must be > 0");
   skip(',');

   StatTimerInterval = TimeToSlot(TI);
   TimerIntervalSeconds = TI;
   if(StatTimerInterval < 1)
      StatTimerInterval = 1;
   
   WQ = read_double("WQ");
   if(WQ < 0 || WQ > 1)
      syntax0("WQ must be 0<=WQ<=1");
   skip(',');

   MarkMethod = read_int("MARKMETHOD");
   if(MarkMethod < 0)
      syntax0("MARKMETHOD must be >= 0");
   skip(',');

   ReservLevel = read_double("RESERVLEVEL");
   if(ReservLevel < 0)
      syntax0("RESERVLEVEL must be >= 0");
   skip(',');

   if (test_word("NACTIVE_ON"))
   {
      nactive_on = read_int("NACTIVE_ON");
      if(!(nactive_on == 0 || nactive_on == 1))
         syntax0("NACTIVE_ON must be 0 or 1");
      skip(',');
   }
   else
      nactive_on = 0;


   output("OUT",SucData);
   
   // generate the inputs
   inputs("I",ninp,-1);

   CHECK(inp_buff = new inpstruct[ninp]);
      inp_ptr = inp_buff;

   // the connection parameter
   CHECK(connpar = new IWUMARKLBConnParam* [max_flow+1]);
   for (i = 0; i <= max_flow; i++)
      CHECK(connpar[i] = new IWUMARKLBConnParam(i));

   eache( &evtPCR); // alarm in each TimeSlot (this is very easy to implement)
   alarml(&evtStatTimer, (time_t) uniform()*StatTimerInterval);
   
   actual_rate = 0.99*SCR; //PCR/2.0;  // not worse than setting to 0

} // iwumark_lb::init(void)


///////////////////////////////////////////////////////////////
// void iwumark_lb::REC()
// cells are received
//////////////////////////////////////////////////////////////
rec_typ iwumark_lb::REC(data *pd,int i)
{
   inp_ptr->inp = i;
   (inp_ptr++)->pdata = pd;
   if (!alarmed_late)
   {
      alarmed_late = TRUE;
      alarml( &std_evt, 0);
   }
   
   return ContSend;
   
} // iwumark_lb::REC()


////////////////////////////////////////////////////////////////////
// void iwumark_lb::late()
// - perform Timers and
// - read packets from input buffers and write them to the queue
//    mark packets with priorities according to their share of bw.
////////////////////////////////////////////////////////////////////
void   iwumark_lb::late(event *evt)
{
   /////////////////////////////////////////////////////////////////
   // Statistics Timer
   // calculates the exp. mov. average of flow_ratio for each flow,
   // resets the counters,
   /////////////////////////////////////////////////////////////////
   if(evt->key == keyStatTimer)
   {
      int i;
      double fr, fr0;

      for(i=1; i<= max_flow; i++)
      {
	 if(bytes_sent > 0)
	 {
	    fr  = connpar[i]->bytes_sent /(double) (bytes_sent);
	    if(bytes_sent0 > 0)
	       fr0 = connpar[i]->bytes_sent0 /(double) (bytes_sent0);
	    else
	       fr0 = 1.0/max_flow;

	    if(connpar[i]->bytes_sent > 0)	// flow wants to send something
	       nactive++;
	 }
	 else
	 {
	    fr = 1.0/max_flow;	 // at the beginning all are treated equal
	    fr0 = fr;
	 }

	 if(connpar[i]->bytes_sent > 0)
	 {
	    connpar[i]->p = 1.0 - bytes_sent0 /
	       (double)(max_flow *connpar[i]->bytes_sent);

      	    if(connpar[i]->p < 0)
	       connpar[i]->p = 0;
	 }
	 else
	    connpar[i]->p = 0.0;


	 if(MarkMethod == 5)
	 {
	    connpar[i]->actual_rate = connpar[i]->actual_rate *(1.0-WQ)
	       + connpar[i]->bytes_received *8.0 / TimerIntervalSeconds * WQ;
      	    if(nactive_on && nactive > 0)
	       connpar[i]->reserved_rate = SCR / (double) nactive;
	    else
	       connpar[i]->reserved_rate = SCR / (double) max_flow;

            connpar[i]->p = (connpar[i]->actual_rate - ReservLevel*connpar[i]->reserved_rate)
	       / connpar[i]->actual_rate;
	    if(connpar[i]->p < 0.0)
	       connpar[i]->p = 0.0;
	    else if(connpar[i]->p > 1.0)
	       connpar[i]->p = 1.0;

	    //printf("%s: flow %d, rate=%f, reserved_rate=%f\n",
	    //   name,i,connpar[i]->actual_rate/1e6, connpar[i]->reserved_rate/1e6);
	 }
// 	 if(MarkMethod == 5)
// 	 {
// 	    connpar[i]->actual_rate = connpar[i]->actual_rate *(1.0-WQ)
// 	       + connpar[i]->bytes_sent *8.0 / TimerIntervalSeconds * WQ;
// 	    connpar[i]->reserved_rate = SCR / (double) max_flow;
// 
//             connpar[i]->p = (connpar[i]->actual_rate - ReservLevel*connpar[i]->reserved_rate)
// 	       / connpar[i]->actual_rate;
// 	    if(connpar[i]->p < 0.0)
// 	       connpar[i]->p = 0.0;
// 	    else if(connpar[i]->p > 1.0)
// 	       connpar[i]->p = 1.0;
// 
// 	    //printf("%s: flow %d, rate=%f, reserved_rate=%f\n",
// 	    //   name,i,connpar[i]->actual_rate/1e6, connpar[i]->reserved_rate/1e6);
// 	 }
	 else if(MarkMethod == 6)
	 {
	    connpar[i]->actual_rate = connpar[i]->actual_rate *(1.0-WQ)
	       + connpar[i]->bytes_sent *8.0 / TimerIntervalSeconds * WQ;
	    connpar[i]->reserved_rate = connpar[i]->reserved_rate * (1.0-WQ) +
	       bytes_sent0 * 8.0 / TimerIntervalSeconds / (double) max_flow * WQ;
	 }

	 connpar[i]->flow_ratio = connpar[i]->flow_ratio * (1.0-WQ) + fr*WQ;
	 connpar[i]->flow_ratio0 = connpar[i]->flow_ratio0 * (1.0-WQ) + fr0*WQ;
         connpar[i]->bytes_sent = 0;
	 connpar[i]->bytes_sent0 = 0;
	 connpar[i]->bytes_received = 0;
	 
      }


      double br;
      br = bytes_sent * 8.0 / TimerIntervalSeconds;   // calculate bitrate
      actual_rate = actual_rate * (1.0-WQ) + br * WQ; // EWMA
      
      //printf("%s: br=%f, actrate=%f, SCR=%f\n",name, br/1e6, actual_rate/1e6, SCR/1e6);
      //printf("%s: gesamtrate=%f\n", name, actual_rate/1e6);

      bytes_sent0 = 0;
      bytes_sent = 0;
      bytes_received = 0;
      TimeLastUpdate = SimTime;
      
      nactive = 0;      
      
      alarml(&evtStatTimer, StatTimerInterval); // alarm again
      return;

   } // Statistics Timer

   ///////////////////////
   // normal data
   ///////////////////////
   frame *pf;
   int n, cid, len;
   IWUMARKLBConnParam *cpar;
   inpstruct   *p;
   int todrop;
   double markprob = 0.0;

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
      
      bytes_received += len;
      cpar->bytes_received += len;
      
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
	 // depending on the Marking Method

      	 if(MarkMethod == 0)
	 {
	    markprob = 0.0;
	 
	 }
	 else if(MarkMethod == 1)
	 {
	    markprob = 0.0;

      	    // calculate aggregated bitrate
	    double br;
	    double iv;
	    iv = (SimTime - TimeLastUpdate)*SlotLength;
	    br = bytes_sent *8.0 / iv;   // bitrate in bit/s
	    if(br > SCR)
	       markprob = (br - SCR)/br;
	 }
	 else if(MarkMethod == 5)
	 {
	    markprob = cpar->p;
	 }
	 else if(MarkMethod == 2 || MarkMethod == 3 || MarkMethod == 5 || MarkMethod == 6)
	 {
	    markprob = 0.0;

      	    // calculate aggregated bitrate
	    int i;
	    double br;
	    double iv;
	    // calculate total bitrate
	    iv = (SimTime - TimeLastUpdate)*SlotLength;  // the interval in s
	    if(MarkMethod == 5 || MarkMethod == 6)
	    {
	       br = actual_rate;
	    }
	    else
	       br = bytes_sent *8.0 / iv;       	      	 // bitrate in bit/s

      	    // calculate actual bitrate of this flow
	    for(i=1; i<=max_flow; i++)
	    {
	       if(MarkMethod == 2)
	       {
	          connpar[i]->actual_rate = connpar[i]->bytes_sent *8.0 / iv;
	          connpar[i]->reserved_rate = SCR / (double) max_flow;
	       }
	       else if(MarkMethod == 3)
	       {
	          connpar[i]->actual_rate = connpar[i]->bytes_sent *8.0 / iv;
	          connpar[i]->reserved_rate = bytes_sent0 * 8.0 / iv / (double) max_flow;
	       }
	       else if(MarkMethod == 5)
	       {
	          // see above keyStatTimer
	       }
	       else if(MarkMethod == 6)
	       {
	          // see above keyStatTimer
	       }
	       else
	          errm1s("%s: internal error: MarkMethod 2 or 3 expected",name);

	    }

	    if(br >= SCR)
	    {
	       if(cpar->actual_rate > cpar->reserved_rate)
	       {
	          double E = 0.0;
		  for(i=1; i<=max_flow; i++)
		  {
		     int diff = (int) (connpar[i]->actual_rate - connpar[i]->reserved_rate);
		     if(diff > 0)
		     	E += diff;
		  }
		  if(E>0)
		  {
		     markprob = (br - SCR)/E;
		  }
	       } // if - actual rate > reserved rate

	    }  // if (br>=SCR)

	 } // Mathod 2 or 3
	 else if(MarkMethod == 9)   // modified FSM
	 {
	    double alpha = 2.0;

	    // by standard 1 would like to transmit everything with
	    // CLP=0
	    markprob = 0.0;

      	    // if the flow takes too much (more than alpha times its fair
	    // share (of the mean of all), then we mark it as CLP=1
	    // with a precalculated probability
	    if(cpar->flow_ratio > alpha*1.0/nactive)
	       markprob = cpar->p;

	 }
	 else
	 {
	    errm1s1d("%s: wrong MarkMethod %d not implemented",
	       name, MarkMethod);
      	 }


      	 // mark the packet now
	 if(uniform() <= markprob)
	 {
	    pf->clp = 1;
	 }
	 else
	 {
	    pf->clp = 0;
	 }

         // Buffer Management
	 int todrop = 0;
         if(connpar[cid]->buffered_packets >= maxbuff / (double)max_flow)
	 {
	    todrop = 1;
	    //printf("%s: packet of cid=%d dropped, packets=%d maxbuff=%d\n",
	    //   name,cid,connpar[cid]->buffered_packets,maxbuff);
	 }

	    
         if(todrop)
	 {
	    delete pf;
	 }
	 else
	 {
	    q.enqueue(pf);       	       // enqueue the frame
	    connpar[cid]->buffered_packets++;
	    buff++;
	 }
 
      } // else - not to drop

      *p = inp_buff[ --n];
      
   } // while(n>0) - as long as there are frames

   inp_ptr = inp_buff;

} // iwumark_lb::late(event *)

///////////////////////////////////////////////////////////////
// void iwumark_lb::early()
// send cells
//////////////////////////////////////////////////////////////
void iwumark_lb::early(event *evt)
{
   frame *pf;
   int len;
   int cid;
   int sendclp;

   if(SimTime < TimeSendNextPCR) // not the time to send because of PCR
      return;

   // first, I have to decide, if to send CLP=0 or CLP=1
   if(SimTime >= TimeSendNextSCR)
   {
      sendclp = 0;
      //printf("%s: 0\n",name);
   }
   else
   {
      sendclp = 1;
      //printf("%s: 1\n",name);      
   }


   pf = (frame*) q.first();
   if(pf != NULL)
   {
      if(pf->clp == 0 && sendclp == 1)
      	 return;  // we have to wait
   }
   else
      return;  // no frame in queue

   // Test
   if(pf->clp == 1)
      sendclp = 1;


   pf = (frame*) q.dequeue();   
   if(pf != NULL)
   {
      len = pf->frameLen;
      cid = pf->connID;

      //if(sendclp == 1)
      //   printf("%s: clp=%d, cid=%d\n",name,sendclp,cid);

      if(pf->clp == 0)
      {
         bytes_sent0 += len;
	 connpar[cid]->bytes_sent0 += len;
      }

      bytes_sent += len;
      connpar[cid]->bytes_sent += len;

      sucs[SucData]->rec(pf, shands[SucData]);  // send it
      buff--;
      connpar[cid]->buffered_packets--;

      // calculate next sending time
      if(TimeSendNextPCR < SimTime)
         TimeSendNextPCR = SimTime;
      TimeSendNextPCR = TimeSendNextPCR + len * 8.0 / PCR / SlotLength;

      if(sendclp == 0)
      {
	 // we cannot accumulate SCR sendings
	 if(TimeSendNextSCR < SimTime)
	    TimeSendNextSCR = SimTime;

         TimeSendNextSCR = TimeSendNextSCR + len * 8.0 / SCR / SlotLength;
      }

/*
      if(cid == 2)
      printf("%s, clp=%d, now=%d, nexttime=%f, diff=%f\n", name, sendclp, 
         SimTime, TimeSendNextSCR, len * 8.0 / SCR / SlotLength);
*/

   } // if - frame available

   return;

} // iwumark_lb::early()


//////////////////////////////////////////////////////////////
// int iwumark_lb::export()
//////////////////////////////////////////////////////////////
int iwumark_lb::export(exp_typ   *msg)
{
   return baseclass::export(msg);
      
} // iwumark_lb::export()


//////////////////////////////////////////////////////////////
// int iwumark_lb::command()
/////////////////////////////////////////////////////////////
int iwumark_lb::command(char *s,tok_typ *pv)
{
   if (baseclass::command(s, pv))
      return TRUE;
   
   pv->tok = NILVAR;
   return TRUE;
   
} // iwumark_lb::command()

//////////////////////////////////////////////////////////////
// iwumark_lb::connect(void)
//////////////////////////////////////////////////////////////
void iwumark_lb::connect(void)
{
   baseclass::connect();

} // iwumark_lb::connect()

///////////////////////////////////////////////////////////////
// iwumark_lb::dropItem(inpstruct *p)
// register a loss and delete the data item
//////////////////////////////////////////////////////////////
inline void iwumark_lb::dropItem(inpstruct *p)
{
   int cid;
   cid = ((frame *) p->pdata)->connID;
   connpar[cid]->lost++;

   delete p->pdata;

} // void iwumark_lb::dropItem()
