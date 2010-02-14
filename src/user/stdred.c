///////////////////////////////////////////////////////////////
// stdred
// muxes several IP connections and marks the packets IN (CLP=0)
//    or OUT (CLP=1)
//
// stdred <name>
//    NINP=<int>,   	   // number of inputs
//    MAXFLOW=<int>, 	   // max. number of flows [NINP]
//    SLOTLENGTH=<double>, // length of 1 slot (s)
//    PIR=<double>,   	   // net PIR of the ATM conn. (bit/s)
//    BUFF_PACKET=<int>, 	   // buffer size (packets)
//    ... RED
//    STATTIMER=<double>,  // timer interval for fairness stat (s)
//    OUT=<suc>;     	   // next module
//      
///////////////////////////////////////////////////////////////
#include "stdred.h"

CONSTRUCTOR(Stdred,stdred);
USERCLASS("STDRED",Stdred);

//////////////////////////////////////////////////////////////
// void stdred::init()
// read in the command line arguments
//////////////////////////////////////////////////////////////
void stdred::init(void)
{	
   skip(CLASS);
   name = read_id(NULL);
   skip(':');
   ninp = read_int("NINP");
   if (ninp < 1)
      syntax0("invalid NINP");
   skip(',');
 
 
   if (test_word("SLOTLENGTH"))
   {
      SlotLength = read_double("SLOTLENGTH");
      if(SlotLength <= 0)
         syntax0("SLOTLENGTH must be > 0");
      skip(',');
   }
   else
      SlotLength = 53.0*8.0 / 149.76 / 1e6;	// STM1


   PIR = read_double("PIR");
   if (PIR <= 0)
      syntax0("invalid PIR: must be >0");
   skip(',');


   // the slots for a packet of minimum size
   double time_per_minpacket = 1500.0 * 8.0 / PIR;
   slot_per_minpacket = time_per_minpacket / SlotLength;
   
   maxbuff_packet = read_int("MAXBUFF_PACKET");
   if (maxbuff_packet <= 0)
      syntax0("invalid MAXBUFF_PACKET: must be >0");
   skip(',');

   maxbuff_byte = read_int("MAXBUFF_BYTE");
   if (maxbuff_byte <= 0)
      syntax0("invalid MAXBUFF_BYTE: must be >0");
   skip(',');


   ///////////////////////////////////
   // read the RED parameters
   //////////////////////////////////
   if (test_word("RED_MIN_TH"))
   {   
      red_min_th = read_int("RED_MIN_TH");	// lower RED threshold
      skip(',');

      red_max_th = read_int("RED_MAX_TH");	// upper RED threshold
      skip(',');

      red_pmax = read_double("RED_PMAX");	// RED pmax
      if(red_pmax < 0 || red_pmax > 1)
	 syntax0("RED_PMAX must be in [0,1] (probability)");
      skip(',');

      red_w_q = read_double("RED_W_Q");
      if(red_w_q < 0 || red_w_q > 1)
	 syntax0("RED_W_Q must be 0<=W_Q<=1");
      skip(',');
   }
   else
   {
      red_min_th = 5;
      red_max_th = 15;
      red_pmax = 0.01;
      red_w_q = 0.002;
   }

   if (test_word("USE_RED"))  	       // use RED or not?
   {
      use_red = read_int("USE_RED");
      skip(',');
   }
   else
      use_red = 1;

   double TI;
   TI = read_double("STATTIMER");   // a statistics timer in seconds
   if( TI <= 0)
         syntax0("STATTIMER must be > 0 (unit: s)");
   skip(',');

   StatTimerInterval = TimeToSlot(TI);
   TimerIntervalSeconds = TI;
   if(StatTimerInterval < 1)
      StatTimerInterval = 1;
   

   output("OUT",SucData);
   
   // generate the inputs
   inputs("I",ninp,-1);

   CHECK(inp_buff = new inpstruct[ninp]);
      inp_ptr = inp_buff;

   eache( &evtPIR); // polling: alarm in each TimeSlot (simple)
   alarml(&evtStatTimer, (time_t) uniform()*StatTimerInterval);


   // RED initialization
   red_avg = 0;
   red_count = -1;
   red_q_time = SimTime;  // queue is empty now
   pa = pb = 0.0;
   
   // buffer initialisation
   buff_packet=0;
   buff_byte=0;
   dropped_packets = 0;
   forced_drops = 0;
   unforced_drops = 0;
   sent_packets = 0;
   sent_bytes = 0;
   arrived_packets = 0;
   throughput = 0.0;
   utilisation = 0.0;


} // stdred::init(void)


///////////////////////////////////////////////////////////////
// void stdred::REC()
// packets are received, late phase alarmed
//////////////////////////////////////////////////////////////
rec_typ stdred::REC(data *pd,int i)
{
   inp_ptr->inp = i;
   (inp_ptr++)->pdata = pd;
   if (!alarmed_late)
   {
      alarmed_late = TRUE;
      alarml( &std_evt, 0);
   }
   
   return ContSend;
   
} // stdred::REC()


////////////////////////////////////////////////////////////////////
// void stdred::late()
// - perform Timers and
// - read packets from input buffers and write them to the queue
//    mark packets with priorities according to their share of bw.
////////////////////////////////////////////////////////////////////
void   stdred::late(event *evt)
{
   /////////////////////////////////////////////////////////////////
   // Statistics Timer has expired
   /////////////////////////////////////////////////////////////////
   if(evt->key == keyStatTimer)
   {
      // nothing defined yet
      alarml(&evtStatTimer, StatTimerInterval); // alarm again
      return;

   } // Statistics Timer

   ///////////////////////
   // normal data
   ///////////////////////
   frame *pf;
   int n;
   int len; // packet length in bytes
   inpstruct   *p;
   int todrop;
   int cautious = 0;

   alarmed_late = FALSE;	// I am not alarmed anymore
   n = inp_ptr - inp_buff;	// number of cells to serve
   while (n > 0)
   {	
      // first, take a random input packet
      if (n > 1)
         p = inp_buff + my_rand() % n;
      else
         p = inp_buff;
         
      if (!typequery(p->pdata, FrameType))
      	 errm1s("%s: arrived data item not of type Frame",name);

      pf = (frame *) p->pdata;
      len = pf->frameLen;  // in bytes
      arrived_packets++;
     
      /////////////////////////////////////////////////////////////
      // here begins the RED code per packet
      /////////////////////////////////////////////////////////////

      // calculate the new average queue size, note that this could 
      // also be done outside the data forwarding path
      if(buff_packet > 0)
      {
         // exponential moving averaging window
         red_avg = (1.0-red_w_q)*red_avg + red_w_q*buff_packet;
      }
      else // queue is empty
      {
	 // m this is the number of packets that could have been
	 // transmitted while
	 // the line was free
      	 // one packet needs slot_per_minpacket slots
	 // here it would also be possible to have a timer which
	 // counts up m
	 // or to implement this using a table lookup
	  double m = (SimTime - red_q_time) / slot_per_minpacket;
	  m = (double)(int) m;
	  red_avg = pow(1.0-red_w_q, m) * red_avg;
      }
         
      // calculate whether to drop the packet or not
      todrop = 0;     
      if(red_min_th <= red_avg && red_max_th > red_avg)
      {
         // we are here in the middle area
	 red_count++;

	 // calculate loss prob
	 pb = red_pmax*(double)(red_avg-red_min_th)/
	         (double)(red_max_th-red_min_th);
	 pa = pb/(1.0-red_count*pb);
	 //pa = pb;
	 
	 // check, if to drop the packet
	 // this is not efficient, better is to determin which
	 // packet to drop (only using pb)
	 if(uniform() <= pa)
	 {
	    todrop = 1;
	    red_count = 0;
	 }
      } // avg is between the threshold
      else if(red_max_th <= red_avg)
      {

         // !!!!! this is not correct !!!!!
	 if(0){
	    pa = pb = red_pmax;
	    if(uniform() <= pa)
	    {
	       todrop = 1;
	       red_count = 0;
	    }
	 }
	 
	 pa = pb = 1.0;
	 todrop = 1;
	 red_count = 0;
      }
      else // this is the area below min_th
      {
	 red_count = -1;  // I am not sure why this is set to -1 here
	 pa = pb = 0.0;
      }


      // this is only a test
      if(cautious)
      {
         if(buff_packet < red_min_th)
	    todrop = 0;
     
      }

      if(use_red == 0)   
         todrop = 0; // if RED is switched off, do not drop packets
 
      // independent of RED we need to drop packets if the buffer
      // is full
      // there are two limitations:
      // - number of packets and
      // - the number of bytes
      if(buff_packet >= maxbuff_packet || buff_byte >= maxbuff_byte)
      {
         todrop = 1;
	 forced_drops++; // forced packet drops due to buffer overflow
	 red_count = 0;
      }


      if(todrop)
      {
         // physically deleting the packet and freeing the memory
         delete p->pdata;
	 dropped_packets++;
	 unforced_drops = dropped_packets - forced_drops;
      }
      else  // enqueue packet, set buffer size (queue-length)
      {
      	 buff_packet++;
	 buff_byte += pf->frameLen;

         q.enqueue(pf);    // enqueue the frame
      }

      /////////////////////////////////////////////////////////
      // here ends the RED code per packet
      /////////////////////////////////////////////////////////
 
      *p = inp_buff[ --n];
      
   } // while(n>0) - as long as there are frames

   inp_ptr = inp_buff;

} // stdred::late(event *)

///////////////////////////////////////////////////////////////
// void stdred::early()
// send packets
//////////////////////////////////////////////////////////////
void stdred::early(event *evt)
{
   frame *pf;

   if(SimTime < TimeSendNextPIR) // not the time to send because of PIR
      return;

   pf = (frame*) q.dequeue();    // dequeue one frame
   if(pf != NULL)    	      	 // a frame is available
   {
      //////////////////////////////////////////////////////////////////
      // here begins the RED code per packet
      //////////////////////////////////////////////////////////////////
      buff_packet--; // one packet less
      buff_byte -= pf->frameLen;
      if(buff_packet == 0) // empty queue
         red_q_time = SimTime;
      
      //////////////////////////////////////////////////////////////////
      // here ends the RED code per packet
      //////////////////////////////////////////////////////////////////

      sent_packets++;
      sent_bytes += pf->frameLen;
      throughput = sent_bytes * 8 / (SimTime*SlotLength);
      utilisation = throughput / PIR;
 
      // calculate next sending time
      if(TimeSendNextPIR < SimTime)
         TimeSendNextPIR = SimTime;

      TimeSendNextPIR = TimeSendNextPIR + pf->frameLen * 8.0 / PIR / SlotLength;
 
      sucs[SucData]->rec(pf, shands[SucData]);  // send it

 
   } // if - frame available

   return;

} // stdred::early()


//////////////////////////////////////////////////////////////
// int stdred::export()
//////////////////////////////////////////////////////////////
int stdred::export(exp_typ   *msg)
{
   int ret;
   ret = baseclass::export(msg) ||
      intScalar(msg, "buff_packet", (int*) &buff_packet) ||
      intScalar(msg, "buff_byte", (int*) &buff_byte) ||
      intScalar(msg, "dropped_packets", (int*) &dropped_packets) ||
      intScalar(msg, "forced_drops", (int*) &forced_drops) ||
      intScalar(msg, "unforced_drops", (int*) &unforced_drops) ||
      intScalar(msg, "sent_packets", (int*) &sent_packets)||
      intScalar(msg, "arrived_packets", (int*) &arrived_packets)||
      doubleScalar(msg, "throughput", (double*) &throughput)||
      doubleScalar(msg, "utilisation", (double*) &utilisation)||
      doubleScalar(msg, "red_avg", (double*) &red_avg)||
      doubleScalar(msg, "red_pa", (double*) &pa)||
      doubleScalar(msg, "red_pb", (double*) &pb);

   return ret; 
     
} // stdred::export()


//////////////////////////////////////////////////////////////
// int stdred::command()
/////////////////////////////////////////////////////////////
int stdred::command(char *s,tok_typ *pv)
{
   if (baseclass::command(s, pv))
      return TRUE;
   
   pv->tok = NILVAR;
   return TRUE;
   
} // stdred::command()

//////////////////////////////////////////////////////////////
// stdred::connect(void)
//////////////////////////////////////////////////////////////
void stdred::connect(void)
{
   baseclass::connect();

} // stdred::connect()
