#ifndef   _STDRED_H_
#define   _STDRED_H_

#include "inxout.h"
#include "queue.h"

///////////////////////////////////////////////////////////////
// class stdred
///////////////////////////////////////////////////////////////
class stdred : public inxout
{
   typedef   inxout   baseclass;

   public:   

   enum {keyPIR = 1, keyStatTimer = 2};

   stdred() : evtPIR(this, keyPIR), evtStatTimer(this, keyStatTimer)
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
   event evtStatTimer;	      // Statistics Timer (for providing fairness)
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
   int red_q_time;   	// the time when the queue became empty
   double pa, pb;


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
}; // class stdred


#endif  // _STDRED_H_
