///////////////////////////////////////////////////////////////
// AgereTm
// AgereTM muxtcp[i]:
//   NINP=tcpsrc,   	    // number of inputs
//   SCR=scr,       	    // net SCR of the ATM connection
//   PIR=pir,       	    // net pir of the ATM connection
//   IF=0.001,      	    // (only if USEARATE=1) Increase-Factor unwichtig
//   WQ=0.1,        	    // (only if USEARATE=1) factor for moving exp. aver.
//   PIRTHR=0.90,   	    // (only if USEARATE=1) when to switch back to SCR unwichtig
//   USEARATE=0,    	    // use RATE option or not unwichtig 
//   NALLOWEXCESS=10,	    // threshold for using PIR
//   CLP0SHAP=0,    	    // shape to SCR of the connection unwichtig
//   UIOLI=uioli,   	    // use it or loose it policy
//   MTU=1500,      	    // maximum packet size
//   BUFF=10000000, 	    // buffer size
//   OUT=lossclp1[i];	    // next module
//
//
// Commands:
//    SetMaxbuff_p(tq_id,cos_id, maxbuff):
//       set the max. buffer size in packets for Cos-Queue #cos_id
//    	 in traffic queue #tq_id
//    SetMaxbuff_b(tq_id,cos_id, maxbuff):
//       set the max. buffer size in bytes for Cos-Queue #cos_id
//    	 in traffic queue #tq_id
//      



///////////////////////////////////////////////////////////////
#include "agere_tm.h"
// #include <math.h>    // for ceil()

//
// Constructor
//
AgereTm::AgereTm() : evtShapingRate(this, keyShapingRate)
  {
    TimeSendNextShapingRate = -1.0;
    alarmed_late = FALSE;   	 // not alarmed
    alarmed_early = FALSE;
    connected = FALSE;
    buff_p = 0;
    buff_b = 0;
    nTrafficQueue = 0;
    nPolicer = 0;
    
    // isse - this is only needed for RR
    //lastServedQueue = 0;
  }  // Constructor

//
// Destructor
//
AgereTm::~AgereTm()
{
  int i;

  delete scheduler;

  for (i = 0; i <= maxconn; i++)
    delete connpar[i];
  delete connpar;

  for (i = 0; i <= nPolicer; i++)
    delete policer[i];
  delete policer;

  for (i = 0; i <= nTrafficQueue; i++)
    delete TrafficQueue[i];
  delete TrafficQueue;

  if (inp_buff) delete inp_buff;
}
///////////////////////////////////////////////////////////////
// void AgereTmCosQueue::AgereTmCosQueue()
// the constructor and the destructor
//////////////////////////////////////////////////////////////
AgereTmCosQueue::AgereTmCosQueue()
{
  buff_p = 0;
  buff_b = 0;
}

AgereTmCosQueue::~AgereTmCosQueue()
{
}


///////////////////////////////////////////////////////////////
// void AgereTmTrafficQueue::AgereTmTrafficQueue()
// the constructor
//////////////////////////////////////////////////////////////
AgereTmTrafficQueue::AgereTmTrafficQueue(root* owner,
   int queuenumber,int maxbuffer_p, int maxbuffer_b, double shapRate)
{
  int i;
  QueueOwner = owner;
  
  MaxCosQueueNumber = 7;
  maxbuff_p = maxbuffer_p;
  maxbuff_b = maxbuffer_b;
  buff_p = 0;
  buff_b = 0;
  
  expense = 0;
  limit1 =  500;
  limit2 = 1000;
  limit3 = 1500;
  
   for(i=0; i<=MaxCosQueueNumber; i++) {
     // issue: we perform buffer sharing, change required for buffer part.
     CosQueue[i].maxbuff_p = maxbuffer_p;
     CosQueue[i].maxbuff_b = maxbuffer_b;
   }
   
   ShapingRate = shapRate; // by default - shape to the max. speed
   MinimumRate = ShapingRate / queuenumber;	// by default assume equal split
   
} // Constructor

///////////////////////////////////////////////////////////////
// void AgereTmTrafficQueue::~AgereTmTrafficQueue()
// the destructor
//////////////////////////////////////////////////////////////
AgereTmTrafficQueue::~AgereTmTrafficQueue()
{
}

///////////////////////////////////////////////////////////////
// AgereTmTrafficQueue::CheckEnqueuable(frame*)
// check whether the frame can be enqueued
// Return value: TRUE if successful, FALSE if not successful
//////////////////////////////////////////////////////////////
int AgereTmTrafficQueue::CheckEnqueueable(frame* pf)
{
  // issue: add code to determine the Cos Queue number
  int CosQueueNumber = 0;
  int ret = TRUE;
  
  CosQueueNumber = pf->vlanPriority;
  if(CosQueueNumber < 0 || CosQueueNumber > MaxCosQueueNumber)
    {
      errm2s2d("%s (%s line %d): frame with vlanPriority=%d cannot be enqueued into CoS queues",
	       QueueOwner->name,__FILE__, __LINE__, CosQueueNumber);
      
      ret = FALSE;
    }   
  
  if(buff_p >= maxbuff_p)
    ret = FALSE;
  if(buff_b >= maxbuff_b)
    ret = FALSE;
  if(CosQueue[CosQueueNumber].buff_p >= CosQueue[CosQueueNumber].maxbuff_p)
    ret = FALSE;
  if(CosQueue[CosQueueNumber].buff_b >= CosQueue[CosQueueNumber].maxbuff_b)
    ret = FALSE;
  
  return ret;
  
} // AgereTmTrafficQueue::CheckEnqueueable(frame* pf)


///////////////////////////////////////////////////////////////
// AgereTmTrafficQueue::enqueue(frame*)
// enqueue a frame
// if not backlogged, register at the scheduler if
// Return value: TRUE if successful, FALSE if not successful
//////////////////////////////////////////////////////////////
void AgereTmTrafficQueue::Enqueue(frame* pf)
{
  int CosQueueNumber;
  int len;
  
  len = pf->frameLen;
  CosQueueNumber = pf->vlanPriority;
  
  if(CosQueueNumber < 0 || CosQueueNumber > MaxCosQueueNumber)
    {
      errm1s1d("%s: frame with vlanPriority=%d cannot be enqueued",
	       QueueOwner->name,CosQueueNumber);
      return;
    }
  
  CosQueue[CosQueueNumber].q_data.enqueue(pf);
  
  if(buff_p <= 0)	// the queue was empty before (not backlogged)
    scheduler->enqueueQueue(this);	// register at the scheduler
  
  buff_p++;
  buff_b+=len;
  CosQueue[CosQueueNumber].buff_p++;
  CosQueue[CosQueueNumber].buff_b+=len;
  
  
} // AgereTmTrafficQueue::enqueue(frame* pf)

///////////////////////////////////////////////////////////////
// AgereTmTrafficDequeue::dequeue(frame*)
// dequeue a frame
// Return value: TRUE if successful, FALSE if not successful
//////////////////////////////////////////////////////////////
frame* AgereTmTrafficQueue::Dequeue()
{
  int CosQueueNumber = 0;
  int len, i;
  frame* pf;
  
  // strict priorty: select the queue with the highest number which is backlogged
  for(i=MaxCosQueueNumber; i>0; i--)
    {
      if(CosQueue[i].buff_p > 0) // backlogged?
	{
	  CosQueueNumber = i;
	  break;
	}
    }
  
  pf = (frame*) CosQueue[CosQueueNumber].q_data.dequeue();
  if(pf != NULL)
    {
      len = pf->frameLen;
      buff_p--;
      buff_b-=len;
      CosQueue[CosQueueNumber].buff_p--;
      CosQueue[CosQueueNumber].buff_b-=len;
    }
  
  return pf;
  
} // AgereTmTrafficQueue::enqueue(frame* pf)


///////////////////////////////////////////////////////////////
// SdwrrScheduler::SdwrrScheduler()
//  constructor
//////////////////////////////////////////////////////////////
SdwrrScheduler::SdwrrScheduler(root* owner,
			       AgereTmTrafficQueue **tq, int nqueue)
{
  int i;
  
  SchedulerOwner = owner;
  TrafficQueue = tq;
  nTrafficQueue = nqueue;
  currentList = 0;
  enqueueList = 2;
  
  for(i=0; i<nTrafficQueue; i++){
    TrafficQueue[i]->scheduler = this;	// register
  }
  maxPduSize = 1500;
  maxWeight = 100;
  
  eachl(&std_evt);
};

///////////////////////////////////////////////////////////////
// SdwrrScheduler::~SdwrrScheduler()
//  destructor
//////////////////////////////////////////////////////////////
SdwrrScheduler::~SdwrrScheduler()
{
}

///////////////////////////////////////////////////////////////
// SdwrrScheduler::calculateLimits()
//////////////////////////////////////////////////////////////

// calculate all the limits
void SdwrrScheduler::calculateLimits(void)
{
  int i;
  double sumOfMinimumRates = 0.0;
  double wmax, wmin;
  
  // calculate max. allowed weight
  maxWeight = 3.0 * serviceQuantum / (double) maxPduSize;
  
  //printf("maxWeight = %f\n", maxWeight);
  
  for(i=0;i<nTrafficQueue;i++)
    {
      sumOfMinimumRates += TrafficQueue[i]->MinimumRate;
      if(TrafficQueue[i]->MinimumRate <= 0)
	errm1s1d("%s: TrafficQueue[%d]->MinimumRate is 0",
		 SchedulerOwner->name, i);
    }
  
  // printf("sumOfMinimumRates=%f\n",sumOfMinimumRates);
  
  // determine the weights for each connection and the maximum of it
  wmax = 0;
  
  //#warning "HOW DOES THIS WORK - DO WE REALLY NEED -D_ISOC99_SOURCE ?"
  wmin = (double) INFINITY;
  
  for(i=0;i<nTrafficQueue;i++)
    {
      TrafficQueue[i]->weight =
	TrafficQueue[i]->MinimumRate / sumOfMinimumRates;
      info("first_weight[%d] = %f",i, TrafficQueue[i]->weight);
      if(TrafficQueue[i]->weight > wmax)
	wmax = TrafficQueue[i]->weight;
      if(TrafficQueue[i]->weight < wmin)
	wmin = TrafficQueue[i]->weight;
    }
  
  info("wmax=%f, wmin=%f", wmax,wmin);
  
  // the ratio wmax/wmin determines the difference between
  // the rates of the traffic queues
  // the higher this ratio, the higher must the serviceQuantum be to
  // ensure that the 
  // Issue: THIS IS ONLY A TEST
  maxWeight = wmax / wmin;
  serviceQuantum = (int)(maxWeight * maxPduSize / 3.0);
  
  for(i=0;i<nTrafficQueue;i++)
    {
      TrafficQueue[i]->weight *= (maxWeight/wmax);	// scale to the max allowed Weight
      
      TrafficQueue[i]->limit1 = (int)(TrafficQueue[i]->weight * maxPduSize*1.0/3.0);
      TrafficQueue[i]->limit2 = (int)(TrafficQueue[i]->weight * maxPduSize*2.0/3.0);
      TrafficQueue[i]->limit3 = (int)(TrafficQueue[i]->weight * maxPduSize*3.0/3.0);
      
      info("weight[%d]=%f, l1=%d, l2=%d, l3=%d",i,
	     TrafficQueue[i]->weight,
	     TrafficQueue[i]->limit1,
	     TrafficQueue[i]->limit2,
	     TrafficQueue[i]->limit3);	
    }
  
} // calculate()



///////////////////////////////////////////////////////////////
// SdwrrScheduler::dequeueFrame()
//////////////////////////////////////////////////////////////
frame* SdwrrScheduler::dequeueFrame(void)
{
  AgereTmTrafficQueue* tq;
  frame *pf;
  int new_expense;
  int advance;
  int queue_currentlist = currentList;
  
  // dprintf("currentlist = %d\n", currentList);
  
  // read out the next traffic queue from the current list
  tq = (AgereTmTrafficQueue*) list[currentList].dequeue();
  
  if(tq == NULL)
    {
      // if nothing is in this queue - find the next
      currentList = findNextNonemptyList(currentList);
      tq = (AgereTmTrafficQueue*) list[currentList].dequeue(); 
      if(tq == NULL) // there is no backlogged connection
	{
	  return NULL;
	}
    }
  
  pf = tq->Dequeue();	// dequeue a frame
  if(pf == NULL)
    errm1s("%s: internal error: no frame in traffic queue\n",
	   SchedulerOwner->name);	
  
  new_expense = tq->expense + pf->frameLen;
  
  if ( new_expense > tq->limit3){
    advance = 3;
    tq->expense = new_expense - tq->limit3;
  } else if (new_expense > tq->limit2) {
    advance = 2;
    tq->expense = new_expense - tq->limit2;
  } else if (new_expense > tq->limit1) {
    advance = 1;
    tq->expense = new_expense - tq->limit1;
  } else {
    advance = 0;
    tq->expense = new_expense; 
  }
  
  // calculate list where to add queue (Mod 4)
  queue_currentlist = (queue_currentlist + advance) & 0x3;   
  
  
  // is the scheduler above its limit?
  // if yes - put it into the sds
  
  
  // now - enqueue this traffic queue again
  if(tq->buff_p > 0)	// traffic queue will be backlogged after one
                        // packet is taken out
    {
      list[queue_currentlist].enqueue(tq); 	// enqueue again at the tail
                                                // of queue_currentlist
    }
  
  // Go to the next non-empty list
  currentList = findNextNonemptyList(currentList);
  
  return pf;	// return the frame
  
}; // SdwrrScheduler::dequeueFrame(void)

///////////////////////////////////////////////////////////////
// SdwrrScheduler::enqueueQueue(AgereTmTrafficQueue *tq)
//////////////////////////////////////////////////////////////
void SdwrrScheduler::enqueueQueue(AgereTmTrafficQueue *tq)
{
  if(tq != NULL)
    {
      list[(currentList+enqueueList)&0x03].enqueue(tq);
    }
};

///////////////////////////////////////////////////////////////
// SdwrrScheduler::findNextNonemptyList()
//////////////////////////////////////////////////////////////
int SdwrrScheduler::findNextNonemptyList(int startlist)
{
  int i;
  // if all are empty, we are at currentlist
  for(i=0; i<=3; i++)
    {
      if(! list[(startlist+i) & 0x03].isEmpty())
	break;
    }
  return ((startlist+i) & 0x03);
  
};


///////////////////////////////////////////////////////////////
// void AgereTmConnParam::AgereTmConnParam() // the constructor
//////////////////////////////////////////////////////////////
AgereTmConnParam::AgereTmConnParam(int vlanId)
{
  served_p = 0;
  served_b = 0;
  lost_p = 0;
  lost_b = 0;
  
  destinationQueue = -1; // this means - not set
  destinationPolicer = -1;
  
} // Constructor

///////////////////////////////////////////////////////////////
// void AgereTmConnParam::~AgereTmConnParam() // the destructor
//////////////////////////////////////////////////////////////
AgereTmConnParam::~AgereTmConnParam()
{
}

///////////////////////////////////////////////////////////////
// void AgereTm::init()
// read in the command line arguments
//	Transfered to LUA init !!!
//////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
// void AgereTm::REC()
// frames are received
// put the frames into the input buffer
//////////////////////////////////////////////////////////////
rec_typ AgereTm::REC(data *pd,int i)
{
  int vlanId, len;
  int destinationPolicer, policingResult;
  int internalDropPrecedence;
  int conforming;
  
  typecheck_i(pd, FrameType, i);
  
  frame *pf = (frame *) pd;
  vlanId = pf->vlanId;
  if (vlanId < 0 || vlanId > maxconn)
    errm1s1d("%s: frame with invalid vlanID = %d received", name, vlanId);
  len = pf->frameLen;
  
  ////////////////////////////////////////////////
  // call dual leaky buckets
  // issue: need to add leaky bucket for interface
  destinationPolicer = connpar[vlanId]->destinationPolicer;
  if(destinationPolicer < 0)
    errm2s2d("%s: (file %s, line %d) frame with vlanID=%d received but no destination policer has been set",
	     name, __FILE__, __LINE__, vlanId);
  
  internalDropPrecedence = pf->dropPrecedence;
  conforming = policingResult = policer[destinationPolicer]
    ->PacketArrivalConformance(len, &internalDropPrecedence);
  
  if(!conforming)	// for dropping modes - if packet is not conforming - drop
    {
      delete pd;
      return ContSend;
    }
  
  // for marking modes, write drop precedence to packet
  pf->internalDropPrecedence = internalDropPrecedence;
  
  // dprintf("packet arrived, VLAN=%d, length=%d, policer=%d, dropPrec=%d\n",
  // vlanId, len, destinationPolicer, internalDropPrecedence);
  
  inp_ptr->inp = i;
  (inp_ptr++)->pdata = pd;
  if (!alarmed_late)
    {
      alarmed_late = TRUE;
      alarml( &std_evt, 0);
    }
  
  return ContSend;
  
} // AgereTm::REC()


///////////////////////////////////////////////////////////////
// void AgereTm::late()
// read the packets from the input buffer and write it
// to the traffic queue (this handles the splitting to the CoS
// queues
//////////////////////////////////////////////////////////////
void AgereTm::late(event *evt)
{

  frame *pf;
  int n;
  int destinationTrafficQueue;
  int vlanId, len;
  inpstruct   *p;
  int todrop;
  
  alarmed_late = FALSE;	// I am not alarmed anymore
  
  n = inp_ptr - inp_buff;	// number of cells to serve
  while (n > 0)
    {	
      if (n > 1)
	p = inp_buff + my_rand() % n;
      else
	p = inp_buff;
      
      pf = (frame *) p->pdata;
      vlanId = pf->vlanId;    // the check, if this ID is valid is done in REC()
      len = pf->frameLen;
      
      destinationTrafficQueue = connpar[vlanId]->destinationQueue;
      if(destinationTrafficQueue < 0)
	errm1s1d("%s: frame with vlanID=%d received but no destination traffic queue has been set", name, vlanId);
      
      
      //////////////////////////////////////////////////
      // check, if frames have to be dropped
      //////////////////////////////////////////////////
      todrop = 0; // think positive
      
      if(buff_p >= maxbuff_p)
	todrop = 1;
      if(buff_b >= maxbuff_b)
	todrop = 1;
      
      // check local queue
      if(TrafficQueue[destinationTrafficQueue]->CheckEnqueueable(pf) == FALSE)
	todrop = 1;
      
      
      if(todrop)
	{
	  dropItem(p);
	}
      else
	{
	  // now enqueue the frame
	  TrafficQueue[destinationTrafficQueue]->Enqueue(pf);
	  buff_p++;
	  buff_b+=len;
	  
	  if(!alarmed_early)
	    {
	      alarme( &evtShapingRate, 1);	// alarm in next slot
	      alarmed_early = TRUE;
	      //dprintf("alarmed from late\n");
	    }
	  
	} // else - not to drop
      
      *p = inp_buff[ --n];
      
    } // while(n>0) - as long as there are frames
  
  inp_ptr = inp_buff;
  
} // AgereTm::late(event *)


///////////////////////////////////////////////////////////////
// void AgereTm::early()
// look in the softqueues, send cells and alarm again with
// ShapingRate
//////////////////////////////////////////////////////////////
void AgereTm::early(event *evt)
{
  frame *pf;
  int vlanId;
  int len;
  int alarmDistance;	// distance from now when to alarm
  
  alarmed_early = FALSE;	// I am not alarmed in early phase anymore
  
  if(SimTime < TimeSendNextShapingRate) // not the time to send because of ShapingRate
    {
      // alarm at the correct time
      alarmDistance = (int)TimeSendNextShapingRate - SimTime;
      if(alarmDistance <= 0)
	alarmDistance = 1;
      
      alarme( &evtShapingRate, alarmDistance);
      alarmed_early = TRUE;
      return;
    }
  
  // take a packet from data queue and send it
  pf = scheduler->dequeueFrame();
  
  if(pf)
    {
      vlanId = pf->vlanId; // the check, if this ID is valid is done in REC()
      len = pf->frameLen;
      sucs[SucData]->rec(pf, shands[SucData]);  // send it
      
      buff_p--;
      buff_b-=len;
      connpar[vlanId]->served_p++;
      connpar[vlanId]->served_b+=len;
      
      // use it or loose it strategy
      // if nobody has sent something, this data rate is lost
      // otherwise, after a pause of all source, we would send with very high rate
      if(TimeSendNextShapingRate < SimTime)
	TimeSendNextShapingRate = SimTime;
      
      // after a packet has been sent, the output cannot send some time
      // due to ShapingRate
      TimeSendNextShapingRate=TimeSendNextShapingRate + len*8.0/ShapingRate/SlotLength;
      
      // alarm again
      alarmDistance = (int)TimeSendNextShapingRate - SimTime;
      if(alarmDistance <= 0)
	alarmDistance = 1;
      alarme( &evtShapingRate, alarmDistance);
      alarmed_early = TRUE;
      
    } // if there is a frame
  else
    {
      // there was no packet in the queue, but from Shaping Rate I am allowed
      // to send a packet
      // if there is a packet in another queue hen alarm for the next timeslot
      if(buff_p > 0) // there are packets
	{
	  alarme( &evtShapingRate, 1);	// issue - slightly less than ShapingRate
	  alarmed_early = TRUE;
	}
    }	// no packet in the visited queue
  
  
  return;
  
} // AgereTm::early()


//////////////////////////////////////////////////////////////
// int AgereTm::export()
//////////////////////////////////////////////////////////////
int AgereTm::export(exp_typ *msg)
{
  int ret = FALSE;
  int i,k;
  char str[60];	// Issue: this may be dangerous for memory
  
  ret = baseclass::export(msg) ||
    intScalar(msg, "buff_p", (int*) &buff_p) ||
    intScalar(msg, "buff_b", (int*) &buff_b);
  
  sprintf(str,"sched.sdwrrt.currentList");
  ret = ret || intScalar(msg, str, (int*) & scheduler->currentList);
  
  sprintf(str,"sched.sdwrrt.enqueueList");
  ret = ret || intScalar(msg, str, (int*) & scheduler->enqueueList);
  
  sprintf(str,"sched.sdwrr.serviceQuantum");
  ret = ret || intScalar(msg, str, (int*) & scheduler->serviceQuantum);
  
  sprintf(str,"sched.sdwrr.maxPduSize");
  ret = ret || intScalar(msg, str, (int*) & scheduler->maxPduSize);
  
  sprintf(str,"sched.sdwrr.maxWeight");
  ret = ret || doubleScalar(msg, str, (double*) & scheduler->maxWeight);
  
  if(ret)
    return ret;		
  
  
  if(ret)
    return ret;
  
  for(i=0;i<nTrafficQueue;i++)
    {
      sprintf(str,"tq[%d].buff_p",i);
      ret = ret || intScalar(msg, str, (int*) & TrafficQueue[i]->buff_p);
      
      sprintf(str,"tq[%d].buff_b",i);
      ret = ret || intScalar(msg, str, (int*) & TrafficQueue[i]->buff_b);
      
      sprintf(str,"tq[%d].sdwrr.limit1",i);
      ret = ret || intScalar(msg, str, (int*) & TrafficQueue[i]->limit1);
      
      sprintf(str,"tq[%d].sdwrr.limit2",i);
      ret = ret || intScalar(msg, str, (int*) & TrafficQueue[i]->limit2);
      
      sprintf(str,"tq[%d].sdwrr.limit3",i);
      ret = ret || intScalar(msg, str, (int*) & TrafficQueue[i]->limit3);
      
      sprintf(str,"tq[%d].sdwrr.weight",i);
      ret = ret || doubleScalar(msg, str, (double*) & TrafficQueue[i]->weight);
      
      if(ret)
	return ret;
      
      for(k=0;k<=7;k++)
	{
	  sprintf(str,"tq[%d].cos[%d].buff_p",i,k);
	  ret = ret ||
	    intScalar(msg, str, (int*) & TrafficQueue[i]->CosQueue[k].buff_p);
	  if(ret)
	    return ret;
	  
	  sprintf(str,"tq[%d].cos[%d].buff_b",i,k);
	  ret = ret || intScalar(msg, str, (int*) & TrafficQueue[i]->CosQueue[k].buff_b);
	  if(ret)
	    return ret;
	  
	} // for all Cos queues (0-7)
    } // for all traffic queues
  
  for(i=0;i<=maxconn;i++)
    {
      sprintf(str,"conn[%d].lost_p",i);
      ret = ret || intScalar(msg, str, (int*) & connpar[i]->lost_p);
      if(ret)
	return ret;
      
      sprintf(str,"conn[%d].lost_b",i);
      ret = ret || intScalar(msg, str, (int*) & connpar[i]->lost_b);
      if(ret)
	return ret;
      
      sprintf(str,"conn[%d].served_p",i);
      ret = ret || intScalar(msg, str, (int*) & connpar[i]->served_p);
      if(ret)
	return ret;
      
      sprintf(str,"conn[%d].served_b",i);
      ret = ret || intScalar(msg, str, (int*) & connpar[i]->served_b);
      if(ret)
	return ret;
    }
  
  return ret;
  
} // AgereTm::export()



///////////////////////////////////////////////////////////////
// AgereTm::dropItem(inpstruct *p)
//	register a loss and delete the data item
//////////////////////////////////////////////////////////////
inline void AgereTm::dropItem(inpstruct *p)
{
  int vlanId, len;
  vlanId = ((frame *) p->pdata)->vlanId;
  len = ((frame *) p->pdata)->frameLen;   
  connpar[vlanId]->lost_p++;
  connpar[vlanId]->lost_b+=len;
  
  delete p->pdata;
  
} // void AgereTm::dropItem()

int AgereTm::act(void)
{
  int i;
  CHECK(inp_buff = new inpstruct[ninp]);
  evtShapingRate.stat = 12345678;
  inp_ptr = inp_buff;

  // generate the traffic queues and set logical values
  // in general we need only nTrafficQueue (and not +1)
  CHECK(TrafficQueue = new AgereTmTrafficQueue* [nTrafficQueue+1]);
  for (i = 0; i <= nTrafficQueue; i++)
    {
      // issue: we perform buffer sharing, change required for buffer part.
      CHECK(TrafficQueue[i] =
	    new AgereTmTrafficQueue(this, i, maxbuff_p, maxbuff_b, ShapingRate));
    }
  // generate the dual leaky buckets
  // in general we need only nDualLeakyBucket (and not +1)
  CHECK(policer = new DualLeakyBucket* [nPolicer+1]);
  for (i = 0; i <= nPolicer; i++)
    {
      CHECK(policer[i] = new DualLeakyBucket(this));
    }
  
  // the connection parameter
  CHECK(connpar = new AgereTmConnParam* [maxconn+1]);
  for (i = 0; i <= maxconn; i++)
    {
      CHECK(connpar[i] = new AgereTmConnParam(i));
      connpar[i]->lost_p = 0;
      connpar[i]->served_p = 0;
    }
  
  // initialise the SDWRR Scheduler
  CHECK(scheduler = new SdwrrScheduler(this, TrafficQueue, nTrafficQueue));
  
  alarme( &evtShapingRate, 1);	// alarm in the next slot
  alarmed_early = TRUE;

  return 0;
}

#if 0
AgereTm::void TqCosSetMaxBuff_p(int tqid, int cosid, int maxbuff_p)
{
  if (tq < 0 || tq >= nTrafficQueue)
    syntax1d("TrafficQueueID %d is out-of-range", tq);
  skip(',');
  CosQueueNumber = cosid;
  if (CosQueueNumber < 0 || CosQueueNumber > 7)
    syntax1d("CoS ID %d is out-of-range, must be in [0,7]", tq);
  TrafficQueue[tq]->CosQueue[CosQueueNumber].maxbuff_p = maxbuff_p;
  if ((TrafficQueue[tq]->CosQueue[CosQueueNumber].maxbuff_p) <= 0)
    syntax0("Maxbuff_p must be > 0");
  if (TrafficQueue[tq]->CosQueue[CosQueueNumber].maxbuff_p > maxbuff_p)
    syntax1d("max. buffer (packets) of %d is >maxbuff of the Traffic Manager",tq);
}
#endif

