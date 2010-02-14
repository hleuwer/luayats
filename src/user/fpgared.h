#ifndef   _FPGARED_H_
#define   _FPGARED_H_

#include <math.h>  // necessary for log and round

#include "inxout.h"
#include "queue.h"

///////////////////////////////////////////////////////////////
// class fpgared
///////////////////////////////////////////////////////////////
class fpgared : public inxout
{
   typedef   inxout   baseclass;

   public:   

   enum {keyPIR = 1, keyStatTimer = 2, keyAvgTimer = 3, keyRndTimer = 4};

   fpgared() : evtPIR(this, keyPIR), evtStatTimer(this, keyStatTimer),
       evtAvgTimer(this, keyAvgTimer), evtRndTimer(this, keyRndTimer)
   {
      TimeSendNextPIR = 0;
      alarmed_late = FALSE;   	 // not alarmed
      alarmed_early = FALSE;
      
   }  // Constructor
   
   struct inpstruct
   {
      data *pdata;
      int inp;
   };

   int ninp;         	// number of inputs
   int max_flow;      	// number of Flows
   int maxbuff_packet; 	// max buffer size (packets)
   int buff_packet;    	// buffer fill level (packets)
   int maxbuff_byte; 	// max buffer size (byte)
   int buff_byte;    	// buffer fill level (byte)
   double slot_per_minpacket;  // the slots for a packet of minimum size


   event evtPIR;	      // Event to call the output with PCR
   event evtStatTimer;	      // Statistics Timer (for statistics)
   event evtAvgTimer;	      // Averaging Timer
   event evtRndTimer;	      // RandomGenerator Timer

   uqueue q;	     	      // the queue with the packets

   int use_red;      	      // determines whether RED is used or not
   
   // RED parameters
   int red_min_th;
   int red_max_th;  
   double red_pmax;     
   double red_w_q;
   
   // RED variables
   double red_avg;
   double red_count;
   double pb, pa;
   double *pb_approx_packet;  // this is the field which contains the
      	             	      // pb approximation per packet;
   int *pb_approx_packet_exponent;
      	             	      // the exponent n for which holds pb=pow(2,n)
			      
   double *pb_approx_byte;    // this is the field which contains the
      	             	      // pb approximation per byte;

   double RandomNumber;
   unsigned int RandomNumberUint;
   unsigned long int z;       // a linear feedback shift register for rn gen.
   int red_maxcount;
   static const int RED_MAXCOUNT_MAX = 100000;

   // statistics
   int dropped_packets;
   int forced_drops;
   int unforced_drops;
   int sent_packets;
   int sent_bytes;   // probably to short!
   int arrived_packets;
   double throughput;
   double utilisation;
      	          
   int StatTimerInterval;     	 // length of Timer-Interval in slots
   double TimerIntervalSeconds;  // Timer interval in seconds

   int AvgTimerInterval;     	 // length of Avg Timer-Interval in slots
   double AvgTimerIntervalSeconds;  // Timer interval in seconds
   
   int alarmed_late; 	      // alarmed for late phase?
   int alarmed_early; 	      // alarmed for elary phase?

   double TimeSendNextPIR;    // when to send the next packet according to PIR
   double SlotLength;	      // Length of 1 Slot in seconds
   double PIR;       	      // PIR of the connection (bit/s)
   double actual_rate;	      // the actual rate

   enum {SucData = 0};
   inpstruct *inp_buff;	   // buffer for cells arriving in the early slot phase
   inpstruct *inp_ptr;	   //current position in inp_buff

   // Methods
   void	init(void);
   rec_typ REC(data*,int);
   void late(event*);
   void early(event *);
   int export(exp_typ *);
   int command(char *,tok_typ *);
   void connect(void);
   inline double SlotToTime(int t){return t*SlotLength;};
   inline int TimeToSlot(double t){return((int) (t/SlotLength));};
   double uniform(){return my_rand() / 32767.0;}
}; // class fpgared


#endif  // _FPGARED_H_
