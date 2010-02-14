///////////////////////////////////////////////////////////////
// fpgared
// muxes several IP connections and marks the packets IN (CLP=0)
//    or OUT (CLP=1)
//
// fpgared <name>
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
#include "fpgared.h"

CONSTRUCTOR(Fpgared,fpgared);
USERCLASS("FPGARED",Fpgared);

//////////////////////////////////////////////////////////////
// void fpgared::init()
// read in the command line arguments
//////////////////////////////////////////////////////////////
void fpgared::init(void)
{
   int i;
	
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


   // calculate the pb approximation
   pb_approx_packet = new double[maxbuff_packet+1];
   pb_approx_packet_exponent = new int[maxbuff_packet+1];
   for(i=0; i<= maxbuff_packet; i++)
   {
      if(i<=red_min_th)
         pb_approx_packet[i] = 0.0;
      else if(red_min_th <= i && red_max_th > i)
	 pb_approx_packet[i] = red_pmax*(double)(i-red_min_th)/
	         (double)(red_max_th-red_min_th);
      else if(red_max_th <= i)
         pb_approx_packet[i] = 1.0 ;
      else // should not happen!
         errm1s("%s: internal error, filling pb-approxfield failed",name);
     
      printf("pb_approx_packet[%d] = %f\n",i,pb_approx_packet[i]);
      
      // calculate the exponent n with pb=pow(2,n)
      // n = log(p)/log(2);
      pb_approx_packet_exponent[i] =
         (int ) round(log(pb_approx_packet[i]) / log(2));
      printf("pb_approx_packet_exponent[%d]=%d\n",
         i,pb_approx_packet_exponent[i]);
      printf("resulting pb[%i]=%f\n",i,pow(2,pb_approx_packet_exponent[i]));

   }
   
   // for bytes: to be added
   

   double TI;
   TI = read_double("STATTIMER");   // a statistics timer in seconds
   if( TI <= 0)
         syntax0("STATTIMER must be > 0 (unit: s)");
   skip(',');

   StatTimerInterval = TimeToSlot(TI);
   TimerIntervalSeconds = TI;
   if(StatTimerInterval < 1)
      StatTimerInterval = 1;


   AvgTimerIntervalSeconds = 1500 * 8 / PIR; // time for one 1500 byte packet
   AvgTimerInterval = TimeToSlot(AvgTimerIntervalSeconds);
   

   output("OUT",SucData);
   
   // generate the inputs
   inputs("I",ninp,-1);

   CHECK(inp_buff = new inpstruct[ninp]);
      inp_ptr = inp_buff;

   eache( &evtPIR); // polling: alarm in each TimeSlot (simple)
   alarml(&evtStatTimer, (time_t) uniform()*StatTimerInterval);
   alarml(&evtAvgTimer, (time_t) uniform()*AvgTimerInterval);
   alarml(&evtRndTimer, 1);

   // RED initialization
   red_avg = 0;
   red_count = 0;
   pb = 0.0;
   pa = 0.0;
   red_maxcount = RED_MAXCOUNT_MAX;
   RandomNumber = 0.5;
   RandomNumberUint = 32768;
   
   // buffer and statistics initialisation
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
   z = 0x2AABAAAB;    // a start value for the shift register random generator
   


} // fpgared::init(void)


///////////////////////////////////////////////////////////////
// void fpgared::REC()
// packets are received, late phase alarmed
//////////////////////////////////////////////////////////////
rec_typ fpgared::REC(data *pd,int i)
{
   inp_ptr->inp = i;
   (inp_ptr++)->pdata = pd;
   if (!alarmed_late)
   {
      alarmed_late = TRUE;
      alarml( &std_evt, 0);
   }
   
   return ContSend;
   
} // fpgared::REC()


////////////////////////////////////////////////////////////////////
// void fpgared::late()
// - perform Timers and
// - read packets from input buffers and write them to the queue
//    mark packets with priorities according to their share of bw.
////////////////////////////////////////////////////////////////////
void   fpgared::late(event *evt)
{
   /////////////////////////////////////////////////////////////////
   // RandomNumberTimer
   // this is a very simple implementation of a LFSR shift register
   // for random number generation. Note that the random numbers
   // are highly correlated (not good), but for our purposes, it should
   // be good enough and it is fairly simple to implement
   /////////////////////////////////////////////////////////////////
   if(evt->key == keyRndTimer)
   {

      unsigned long int p1, p2, p3, p4, newbit;

      z = z >> 1;    // shift
      p1 = (z & (0x00000001 << 31)) >>31; 
      p2 = (z & (0x00000001 << 6)) >>6;   
      p3 = (z & (0x00000001 << 5)) >>5;      
      p4 = (z & (0x00000001 << 1)) >>1;     

      newbit = (p1 + p2 + p3 + p4)&0x01; // XOR
      newbit = newbit << 31;
      z = z & 0x7FFFFFFF;  // set left bit to 0
      z = z | newbit;	   // set left bit to newbit

      if(z == 0)
         errm1s("%s: internal error: z==0 (should not happen, restart)", name);

      // nothing defined yet
      alarml(&evtRndTimer, 1); // alarm again
      return;

   } // RandomNumber Timer


   /////////////////////////////////////////////////////////////////
   // Statistics Timer has expired
   /////////////////////////////////////////////////////////////////
   if(evt->key == keyStatTimer)
   {
      // nothing defined yet
      alarml(&evtStatTimer, StatTimerInterval); // alarm again
      return;

   } // Statistics Timer


   /////////////////////////////////////////////////////////////////
   // Averaging Timer has expired
   /////////////////////////////////////////////////////////////////
   if(evt->key == keyAvgTimer)
   {
      // calculate average queue   
      red_avg = (1.0-red_w_q)*red_avg + red_w_q*buff_packet;
      int red_avg_int = (int) red_avg;
      
      // lookup of pb in approximation table
      if(red_avg_int < 0)
         errm1s("%s: internal error, avg is <0", name);
      if(red_avg_int >= maxbuff_packet)
         errm1s("%s: internal error, avg is >maxbuff_packet", name);     

      pb = pb_approx_packet[red_avg_int];
      
      if(pb > 0) // here we could also use the avg or the exponent
      {
          unsigned long long x;  // 64 bit ?
      
          //red_maxcount = int(2 * RandomNumber / pb);
	  //printf("orig maxcount = %d ", red_maxcount);
	 
	  // in the following the equation
	  // red_maxcount = int(2 * RandomNumber / pb);
	  // is implemented by shifting operations
	  //    RandomNumberUint = unsigned integer with [0,65536]]
	  //    pb = pow(2,-pb_approx_packet_exponent[red_avg_int])

	  x = RandomNumberUint; // 16 bit uniform distributed from [0,65536]
	  x = x << 1;  // *2
	  
	  // division by pb, note: the exp is 0 or negative
	  x = x << -pb_approx_packet_exponent[red_avg_int];
	  
	  // transfer RandomNumberUint from [0,65536] to [0,1]
	  x = x >> 16; 
	  x = x & 0xFFFF;  // make an 16 bit integer from it
	  red_maxcount = (int) x;

	  //printf("new maxcount = %d\n", red_maxcount);
	 
      }
      else
         red_maxcount = RED_MAXCOUNT_MAX; // more or less no loss
      
      alarml(&evtAvgTimer, AvgTimerInterval); // alarm again
      return;

   } // Averaging Timer


   ///////////////////////
   // normal data
   ///////////////////////
   frame *pf;
   int n;
   int len; // packet length in bytes
   inpstruct   *p;
   int todrop;
   int cautious = 1;

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

      todrop = 0;
      red_count++;   // Note: if avg<thmin red_count is set to 0 by AvgTimer
      
      if(red_count > red_maxcount && red_count > 3) // 3 - for shaping
	 todrop = 1;
          
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
      }


      if(todrop)
      {
         // physically deleting the packet and freeing the memory
         delete p->pdata;
	 dropped_packets++;
	 unforced_drops = dropped_packets - forced_drops;
	 
	 red_count = 0;
	 red_maxcount = RED_MAXCOUNT_MAX; // make sure: no loss until recalc. 
	 //RandomNumber = uniform();  // new RandomNumber - used by AvgTimer
	 //RandomNumberUint = (unsigned int) 2*my_rand();
	 
	 // take a 16 bit RandomNumber from the 32 bit z register
	 RandomNumberUint = (z & 0xFFFF); 
	 RandomNumber = RandomNumberUint / double(0xFFFF);
	 if(RandomNumber > 1)
      	    errm1s("%s: internal error, RandomNumber > 1", name);	    
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

} // fpgared::late(event *)

///////////////////////////////////////////////////////////////
// void fpgared::early()
// send packets
//////////////////////////////////////////////////////////////
void fpgared::early(event *evt)
{
   frame *pf;

   if(SimTime < TimeSendNextPIR) // not the time to send because of PIR
      return;

   pf = (frame*) q.dequeue();    // dequeue one frame
   if(pf != NULL)    	      	 // a frame is available
   {

      // decrement the queue sizes
      buff_packet--; // one packet less
      buff_byte -= pf->frameLen;

      // statistics
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

} // fpgared::early()


//////////////////////////////////////////////////////////////
// int fpgared::export()
//////////////////////////////////////////////////////////////
int fpgared::export(exp_typ   *msg)
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
      intScalar(msg, "red_maxcount", (int*) &red_maxcount)||
      doubleScalar(msg, "throughput", (double*) &throughput)||
      doubleScalar(msg, "utilisation", (double*) &utilisation)||
      doubleScalar(msg, "red_avg", (double*) &red_avg)||
      doubleScalar(msg, "red_pa", (double*) &pa)||
      doubleScalar(msg, "red_pb", (double*) &pb);

   return ret; 
     
} // fpgared::export()


//////////////////////////////////////////////////////////////
// int fpgared::command()
/////////////////////////////////////////////////////////////
int fpgared::command(char *s,tok_typ *pv)
{
   if (baseclass::command(s, pv))
      return TRUE;
   
   pv->tok = NILVAR;
   return TRUE;
   
} // fpgared::command()

//////////////////////////////////////////////////////////////
// fpgared::connect(void)
//////////////////////////////////////////////////////////////
void fpgared::connect(void)
{
   baseclass::connect();

} // fpgared::connect()
