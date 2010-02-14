#ifndef   _AGERETM_H_
#define   _AGERETM_H_

#include <stdlib.h>
#include <math.h>
#include "inxout.h"
#include "queue.h"
#include "oqueue.h"
#include "leakybucket.h"
#include "mux.h"

class AgereTm;
class AgereTmCosQueue;
class AgereTmTrafficQueue;
class SdwrrScheduler;
class AgereTmConnParam;
// class SortItem;

// class scheduler {
//  
//    public:
//       scheduler(int n){
//          nTrafficQueue = n;
//          CHECK(TrafficQueue = new uqueue* [nqueues+1]);
//       } // constructor
//    
//    int nTrafficQueue;	// # of traffic queues
// 	AgereTmTrafficQueue **TrafficQueue;
// 
//    virtual AgereTmTrafficQueue* next(void);	// returns a pointer to the next queue
// 
//    // connect an external queue with the i-th queue in the scheduler
//    int connectQueue(int i, uqueue* q){
//       if(i>=0 && i<= nqueues) {
//          CHECK(TrafficQueue[i] = q);
// 	 return TRUE;
//       } else {
//          return FALSE;
//       }
//    } // connectQueue()
//   
//};
// 

///////////////////////////////////////////////////////////////
// ageretm
// The Agere Traffic Manager unit
// - N traffic queues are scheduled with SDWRR scheduling
// - each traffic queue has 8 CoS queues served with strict priority
// - the priority is derived from frame->VlanPriority
// - each CoS queue has buffer management:
//     - tail drop or WRED
// according to SCR and PIR, each connection can set its own
// SCR and PIR, which are met by the Queues for SCR and PIR
///////////////////////////////////////////////////////////////

//tolua_begin
class AgereTm : public inxout {
  typedef   inxout   baseclass;
  
public:   
  
  enum {keyShapingRate = 1};
  AgereTm();
  ~AgereTm();
  
  int ninp;         	// # of inputs
  int nTrafficQueue;	// # of traffic queues
  int nPolicer;     	// number of dual leaky buckets
  int maxconn;		// maximum number of connections
  int maxbuff_p;    	// maximum buffer size in packets
  int maxbuff_b;    	// maximum buffer size in bytes
  double ShapingRate;  // the ShapingRate of the scheduler 
  int act(void);
  int buff_p;       	// actual buffer size in packets
  int buff_b;       	// actual buffer size in bytes
  //tolua_end

  SdwrrScheduler *scheduler; 		// the scheduler

  int connected;
  
  struct inpstruct
  {
    data *pdata;
    int inp;
  };
  
  
  //int lastServedQueue;	
  
  event evtShapingRate;            // Event to call the output with ShapingRate

  double TimeSendNextShapingRate;  // the SimTime when the next frame can
                                   // be sent according to ShapingRate
#if 0
  uqueue QSort;	// sort queue for implementing SDWRR scheduling
                // this queue schedules the traffic queues
                // issue: I need 4 of them
#endif

  int alarmed_late; 	// alarmed for late phase?
  int alarmed_early; 	// alarmed for elary phase?

  // We expose the the following to Lua via package file for direct access.
  AgereTmTrafficQueue **TrafficQueue;	// the traffic queues
  AgereTmConnParam **connpar;		// the connection parameters
  DualLeakyBucket **policer;      	// the leaky buckets
  
  enum {SucData = 0};
  inpstruct *inp_buff;	   // buffer for cells arriving in the early slot phase
  inpstruct *inp_ptr;	   //current position in inp_buff
  
  // Methods
  rec_typ REC(data*,int);
  void late(event*);
  void early(event *);
  inline void	dropItem(inpstruct *);
  int export(exp_typ *);
  //tolua_begin
  inline double SlotToTime(int t){return t * SlotLength;};
  //tolua_end
  
  // sets and gets
  /*    AgereTmTrafficQueue *getTq(int id){return this->TrafficQueue[id];} */
  /*    AgereTmConnParam *getConnParam(int id){return this->connpar[id];} */
  /*    DualLeakyBucket *getPolicer(int id){return this->policer[id];} */
  //   SdwrrScheduler *getScheduler(void){return this->scheduler;}
  // commands
#if 0
  void TqCosSetMaxBuff_p(int tqid, int cosid, int maxbuff_p);
  void TqCosSetMaxBuff_b(int tqid, int cosid, int maxbuff_b);
  void TqSetMaxBuff_p(int tqid, int maxbuff_p);
  void TqSetMaxBuff_b(int tqid, int maxbuff_b);
  void TqSetSdwrrParam(int tqid, int limit1, int limit2, int limit3);
  void SchedSdwrrSetServiceQuantum(int sq);
  void SchedSdwrrSetMaxPduSize(int maxPduSize);
  void TqSetShapingRate(int tqid, double rate);
  void TqSetMinimumRate(int tqid, double rate);
  void ConnSetDestinationQueue(int cid, int tqid);
  void ConnSetDestinationPolicer(int cid, int polid);
  void PolicerSetRate(int polid, int cir, int cbs, int pir, int pbs);
  void PolicerSetAction(int polid, int action);
#endif
  
  
  
};  //tolua_export


///////////////////////////////////////////////////////////////
// class AgereTmCosQueue
// The 8 CoS queues which are connected to the traffic queues
///////////////////////////////////////////////////////////////

//tolua_begin
class AgereTmCosQueue {

public:
  
  AgereTmCosQueue();
  ~AgereTmCosQueue();
  
  //AgereTmCosQueue(int queuenumber);
  
  uqueue q_data; // the CoS queues, where the data (frames) are queued
  int maxbuff_p; // the maximum buffer size in packets
  int maxbuff_b; // the maximum buffer size in bytes
  int buff_p;    // the actual buffer size in packets
  int buff_b;    // the actual buffer size in bytes
};
//tolua_end


///////////////////////////////////////////////////////////////
// class AgereTmTrafficQueue
// The traffic queues
///////////////////////////////////////////////////////////////

//tolua_begin
class AgereTmTrafficQueue : public ino {
  
public:
  
  // constructor
  AgereTmTrafficQueue(root* owner, int queuenumber, int maxbuffer_p, int maxbuffer_b, double shapRate);
  ~AgereTmTrafficQueue();	
  
  int MaxCosQueueNumber;
  int maxbuff_p;
  int maxbuff_b;
  int buff_p;
  int buff_b;
  
  // SDWRR parameters
  int expense;
  int limit1;
  int limit2;
  int limit3;
  
  double MinimumRate;	// the minimum wished Rate	to be scheduled
  double ShapingRate;	// the ShapingRate of this scheduler (the max. Rate)
  double weight;
  
  
  root* QueueOwner; // the owner object of this traffic queue
  AgereTmCosQueue CosQueue[8];
  SdwrrScheduler *scheduler; // the scheduler which schedules this queue
  //tolua_end
  
  int CheckEnqueueable(frame *pf);
  void Enqueue(frame* pf);
  frame* Dequeue(void);   
};  //tolua_export
// class AtereTmTrafficQueue


///////////////////////////////////////////////////////////////
// class SdwrrScheduler
// a scheduler with SDWRR scheduling discipline
///////////////////////////////////////////////////////////////

//tolua_begin
class SdwrrScheduler : public ino {
  
public:
  
  SdwrrScheduler(root* owner,AgereTmTrafficQueue **tq, int nqueue); // constr.
  ~SdwrrScheduler();
  
  int nTrafficQueue;		// # of traffic queues
  int serviceQuantum;	// this is the amount of data in bytes which is
                        // served in each list for the connection with the
                        // largest weight until switched to the next list
                        // serviceQuantum=MaxWeight * MaxPDU / 3
  int maxPduSize;
  double maxWeight; 	// Issue: must this be int or double?
  void calculateLimits(void);
  //tolua_end
  
  frame *dequeueFrame(void);
  int findNextNonemptyList(int startlist);	// find the next non-empty list
                                                // beginning from the startlist
  void enqueueQueue(AgereTmTrafficQueue *tq);	// enqueue new traffic queue
  
  void late(event* evt)	// called each time slot
  {
    AgereTmTrafficQueue *tq;
    
    tq = (AgereTmTrafficQueue*) sds.first();
    if(tq == NULL)
      return;
    
    if(SimTime >= tq->time) // if this queue is allowed to transmit
      {
	// take traffic queue from shared dynamic scheduler
	tq = (AgereTmTrafficQueue*) sds.dequeue();
	enqueueQueue(tq); 	// enqueue the traffic queue in the 4 lists
      }
  };
  
  root* SchedulerOwner; 	// the owner object of this scheduler queue
  AgereTmTrafficQueue **TrafficQueue;
  uoqueue list[4];		// the 4 lists
  uoqueue sds; 			// the shared dynamic scheduler
  // for rate limiting of traffic queues
  int currentList;
  int enqueueList;
};  //tolua_export
// class SdwrrScheduler


///////////////////////////////////////////////////////////////
// class AgereTmConnParam
// the connections parameter
///////////////////////////////////////////////////////////////

//tolua_begin
class AgereTmConnParam {
  
public:
  
  AgereTmConnParam(int conn);
  ~AgereTmConnParam();
  int served_p;
  int served_b;
  int lost_p;
  int lost_b;
  int destinationQueue;   // the destination traffic queue
  int destinationPolicer; // the number of the DualLeakyBucket
  
};
//tolua_end

// ///////////////////////////////////////////////////////////////
// // class SortItem
// // These items are used in the SortQueues for SCR and PIR
// // they point to the connection parameter
// ///////////////////////////////////////////////////////////////
// class SortItem : public cell {
// 
//    public:
//    
//    SortItem(AgereTmTrafficQueue *tp, int queuenumber): cell(queuenumber)
//    {
//       TrafficQueue = tq;
// 
//    } // Constructor
//  
//    AgereTmTrafficQueue *TrafficQueue;
// 
// };

#endif  // _AGERETM_H_

