#ifndef   _IWUMARK_H_
#define   _IWUMARK_H_

#include "inxout.h"
#include "queue.h"

///////////////////////////////////////////////////////////////
// class IWUMARKConnParam
// Connection parameters for iwumark
///////////////////////////////////////////////////////////////
class IWUMARKConnParam
{
   public:
   
   IWUMARKConnParam(int flow)
   {
      bytes_sent = 0;
      bytes_sent0 = 0;
      lost = 0;
      actual_rate = 0.0;
      reserved_rate = 0.0;
      flow_ratio = 0.0;
      flow_ratio0 = 0.0;
      p = 0.0;
   };
   
   int bytes_sent;
   int bytes_sent0;
   int lost;         	// lost packets
   double actual_rate;
   double reserved_rate;
   double p;
   
   double flow_ratio;	// ratio of this flow compared to whole VC
   double flow_ratio0;
};

///////////////////////////////////////////////////////////////
// class iwumark
///////////////////////////////////////////////////////////////
class iwumark : public inxout
{
   typedef   inxout   baseclass;

   public:   

   enum {keyPCR = 1, keyStatTimer = 2};

   iwumark() : evtPCR(this, keyPCR), evtStatTimer(this, keyStatTimer)
   {
      TimeSendNextPCR = -1.0;
      alarmed_late = FALSE;   	 // not alarmed
      alarmed_early = FALSE;
      buff=0;

      actual_rate = 0;
      bytes_sent0 = 0;
      bytes_sent = 0;
      TimeLastUpdate = 0;
      
   }  // Constructor
   
   struct inpstruct
   {
      data *pdata;
      int inp;
   };

   int ninp;         	// number of inputs
   int max_flow;      	// number of Flows
   int nactive;      	// number of active connections
   int maxbuff;      	// max buffer size (packets)
   int buff;         	// buffer fill level (packets)

   event evtPCR;	      // Event to call the output with PCR
   event evtStatTimer;	      // Statistics Timer (for providing fairness)
   uqueue q;	     	      // the queue with the packets
   IWUMARKConnParam **connpar; // the connection parameter
   double WQ;        	      // weighting parameter for expo mov average of
      	             	      // connpar->flow_ratio
   int StatTimerInterval;     // length of Timer-Interval in slots
   double TimerIntervalSeconds; // Timer interval in seconds
   int TimeLastUpdate;	      // last update of statistics information (slots)
   int MarkMethod;   	      // the marking method to implement (0: nothing)
   
   int alarmed_late; 	      // alarmed for late phase?
   int alarmed_early; 	      // alarmed for elary phase?

   double TimeSendNextPCR;    // when to send the next packet according to PCR
   double SlotLength;	      // Length of 1 Slot in seconds
   double PCR;       	      // PCR of the connection (bit/s)
   double SCR;       	      // SCR of the connection (bit/s)
   double actual_rate;	      // the actual rate

   enum {SucData = 0};
   inpstruct *inp_buff;	   // buffer for cells arriving in the early slot phase
   inpstruct *inp_ptr;	   //current position in inp_buff

   int bytes_sent0;  	// bytes sent with clp=0 only
   int bytes_sent;  	// bytes sent (clp=1 and clp=0)

   // Methods
   void	init(void);
   rec_typ REC(data*,int);
   void late(event*);
   void early(event *);
   inline void	dropItem(inpstruct *);
   int export(exp_typ *);
   int command(char *,tok_typ *);
   void connect(void);
   inline double SlotToTime(int t){return t*SlotLength;};
   inline int TimeToSlot(double t){return((int) (t/SlotLength));};
   double uniform(){return my_rand() / 32767.0;}
}; // class iwumark


#endif  // _IWUMARK_H_
