#ifndef   _IWUVBR3_H_
#define   _IWUVBR3_H_

#include "inxout.h"
#include "queue.h"

class iwuvbr3;
class IWUConnParam;
class SortItem;

///////////////////////////////////////////////////////////////
// iwuvbr3
// Interworking Unit, which accepts frames and lets them out
// according to SCR and PCR, each connection can set its own
// SCR and PCR, which are met by the Queues for SCR and PCR
///////////////////////////////////////////////////////////////
class iwuvbr3 : public inxout {
   typedef   inxout   baseclass;

   public:   

   enum {keyPCR = 1};

   iwuvbr3() : evtPCR(this, keyPCR)
   {
      TimeSendNextPCR = -1.0;
      TimeSendNextSCR = -1.0;
      alarmed_late = FALSE;   	 // not alarmed
      alarmed_early = FALSE;
      SCRReserved = 0;
      connected = FALSE;
      buff=0;
   }  // Constructor
   
   struct inpstruct
   {
      data *pdata;
      int inp;
   };

   int ninp;         	// # of inputs
   int max_flow;      	// number of Flows
   int connected;
   int maxbuff;
   int buff;

   event evtPCR;	// Event to call the output with PCR

   uqueue QSortScr;	// sort queue for SCR
   uqueue QSortPcr;	// sort queue for PCR

   double IncreaseFactor;  // how much to increase PCR every packet
   double WQ;        	// Weighting parameter for expo mov average or ARate
   double pcrthr;    	// threshold, at which rate fall back to switch off PCR
   int NAllowExcess; 	// at how many packets to start allow PCR
   int Clp0Shap;     	// shaping of clp=0 packets yes/no
   int MTU;          	// maximum size of packets
   int UseARate;     	// use the calculation of ARate or not?
   int UseItOrLooseIt;  // Use It or Loose It policy
   
   int alarmed_late; 	// alarmed for late phase?
   int alarmed_early; 	// alarmed for elary phase?

   IWUConnParam **connpar;

   double TimeSendNextPCR;
   double TimeSendNextSCR;
   
   double SlotLength;	// Length of 1 Slot in seconds
   double SCR;       	// SCR of the connection
   double PCR;       	// PCR of the connection
   double SCRReserved;  // reserved SCR Rate

   enum {SucData = 0};
   inpstruct *inp_buff;	   // buffer for cells arriving in the early slot phase
   inpstruct *inp_ptr;	   //current position in inp_buff

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

}; // class iwuvbr2


///////////////////////////////////////////////////////////////
// class IWUConnParam
// Connection parameters for iwuvbr3
// Contains two SortItems, which are used in the queues for
// SCR and PCR
///////////////////////////////////////////////////////////////
class IWUConnParam {

   public:
   
   IWUConnParam(int flow);

   uqueue q_data;    	// the queue, where the data (frames) are queued
   SortItem *SIScr;  	// to sort for SCR
   SortItem *SIPcr;  	// to sort for PCR

   //int QLenByte;	// QLen in bytes for this flow
   
   double SCR;       	// the SCR of this flow (fixed)
   double PCR;       	// the PCR of this flow (variable)
   double ARate;     	// arriving rate of this flow
   int LastATime;    	// the last arriving time of this flow
   
   double SendTimeSCR;  // the next sending time of this flow regarding SCR
   double SendTimePCR;  // the next sending time of this flow regarding PCR
   int LastSendTimePCR; // the last sending time of the PCR queue

   int lost;
   int maxbuff;
};


///////////////////////////////////////////////////////////////
// class SortItem
// These items are used in the SortQueues for SCR and PCR
// they point to the connection parameter
///////////////////////////////////////////////////////////////
class SortItem : public cell {

   public:
   
   SortItem(IWUConnParam *cp, int flow): cell(flow)
   {
      cpar = cp;

   } // Constructor
 
   IWUConnParam *cpar;

};

#endif  // _IWUVBR3_H_
