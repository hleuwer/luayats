/*************************************************************************
*
*  YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997 Chair for Telecommunications
*    Dresden University of Technology
*    D-01062 Dresden
*    Germany
*
**************************************************************************
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
**************************************************************************
*
* Module author:  Matthias Baumann, TUD
* Creation:  July 11, 1997
*
*************************************************************************/

/*
* Purely event triggered Multiplexer, different queues:
*  - synchronous or asynchronous operation of output
*  - Departure First (DF)
*  - A number of queues with absolute priority
*
* MuxFrmPrio mux: NINP=10, {MAXINPRIO=8,} {SERVICE=2,} MODE={Async | Sync},
*   NPRIO=3, OUT=sink;
*   // default MAXINPRIO: 8
*   // SERVICE: service takes SERVICE time steps
*   // MODE: asynchronous or synchronous output
*   // MODE can be omitted if SERVICE=1 or omitted
*   // NPRIO: number of queues, 0 ... (NPRIO-1),
*     queue (NPRIO-1) has highest priority
*
* Commands: like Multiplexer (see mux.c)
*   mux->SetBuf(prio, bsiz) // set buffer size of
*      // priority prio to bsiz.
*      // Initially, all sizes are one (!).
*      // Buffer sizes < 1 are impossible.
*   mux->SetPrio(inprio, prio) // put this priority code point into priority prio.
*      // Initially, a priority code points are mapped according to IEEE802.1Q
* exported: mx->PQLen(p)  // queue length of priority p
*
* On the inputs, only Cells are allowed (need to have a prioCodePoint (inprio) to associate
* to a priority). This allows a predecessor to use either VLAN priority or DiffServ DSCP
* or IP precedence or any other classification to set the priority code point.
*
* NOTE: The object performs no classification regarding frame format. It silently assumes
*       that the frame's vlanPriority field is set.
*/

#include "muxFrmPrio.h"

// CONSTRUCTOR(MuxFrmPrio, muxFrmPrio);
muxFrmPrio::muxFrmPrio()
{
}
muxFrmPrio::~muxFrmPrio()
{
  delete[] prioQ;
  delete[] priorities;
  delete[] qLens;
  delete[] qByteLens;
  delete[] lostPRIO;
  delete[] inpPrioBuf;
}
int muxFrmPrio::act(void)
{
  int i;
  baseclass::act();
  CHECK(prioQ = new queue[nprio]); // queues have zero capacity
  for (i = 0; i < nprio; ++i)
    prioQ[i].setmax(1);  // this is really needed by late()!
  CHECK(qLens = new int[nprio]);
  for (i = 0; i < nprio; ++i)
    qLens[i] = 0;
  CHECK(qByteLens = new int[nprio]);
  for (i = 0; i < nprio; ++i)
    qByteLens[i] = 0;
  CHECK(priorities = new int[max_inprio]);
  for (i = 0; i < max_inprio; ++i)
     // initially: prio == inprio
     priorities[i] = i; 
  CHECK(lostPRIO = new unsigned int[nprio]);
  for (i = 0; i < nprio; ++i)
     lostPRIO[i] = 0;
  CHECK(inpPrioBuf = new inpPrioStruct[ninp]);
  inpPrioPtr = inpPrioBuf;
  counter = 0;
  return 0;
}
//
// Try to dequeue a cell from the queue pool, absolute priorities
//
inline data *muxFrmPrio::dequeuePrio() 
{
   int i;
   for (i = nprio - 1; i >= 0; --i)
      if ( !prioQ[i].isEmpty()) {
	 --qLens[i];
	 return prioQ[i].dequeue();
      }
   return NULL;
}

//
// Process arrivals of this step
//
void muxFrmPrio::late(event *) 
{
   inpPrioStruct *p;
   int  n;
   tim_typ tim;

   needToSchedule = TRUE;
   
   n = inpPrioPtr - inpPrioBuf;
   // random choice between arrivals
   for (;;) {       
      if (n > 1)
	 p = inpPrioBuf + (my_rand() % n);
      else
	 p = inpPrioBuf;
      
      if (serverState == serverIdling && p->prio == maxArrPrio) { 
	 // Server is idle, i.e. all queues have been empty during early().
	 // AND: the priority of this data object is high enough.
	 // In addpars() and command(), we ensured queue capacities of at least 1.
	 // This is necessary, since we have to queue the object in case we have
	 // to wait for sync.
	 if (syncMode) {
	    // In sync mode the queues are serviced at the beginning of fixed 
	    // time intervals (constant frame rate).
	    int dd = SimTime % serviceTime;
	    if (dd) { 
	       // we have to wait until beginning of an output cycle,
	       // but until there we have to leave the item in the queue!
	       prioQ[p->prio].enqueue(p->pdata);
	       ++qLens[p->prio];
	       alarme( &std_evt, serviceTime - dd);
	       serverState = serverSyncing;
	    } else {
	       // we have hit the beginning of a cycle
	       server = p->pdata;
	       serverState = serverServing;
	       alarme( &std_evt, serviceTime);
	    }
	 } else {
	    // In async mode the queues are serviced according to a given bitrate.
	    // The frame length plays a role.
	    server = p->pdata;
	    serverState = serverServing;
	    tim = (tim_typ) (p->pdata->pdu_len() * 8 / serviceRate + 0.5);
	    if (tim < 1)
	       tim = 1;
	    alarme( &std_evt, tim);
	 }
      } else {
	 if (prioQ[p->prio].enqueue(p->pdata)){
	    ++qLens[p->prio];
	    qByteLens[p->prio] += p->pdata->pdu_len();
	 } else {
	    // buffer overflow
	    dropItem(p);
	    ++lostPRIO[p->prio];
	 }
      }
      
      if (--n == 0)
	 break;
      *p = inpPrioBuf[n];
   }
   inpPrioPtr = inpPrioBuf;
   
   maxArrPrio = 0;
}

// 
// In serving state:
//  Forward a data item to the successor.
//  Register again, if more data queued.
// In sync state:
//  take data from queue and enter serving state
// 
void muxFrmPrio::early(event *) {
   tim_typ tim;
   if (serverState == serverServing) { 
      // "normal" case: forward served data
      ++counter;
      suc->rec(server, shand);
      server = dequeuePrio();
      if (server){
	 if (syncMode){
	    tim = serviceTime;
	 } else {
	    tim = (tim_typ) (server->pdu_len() * 8 / serviceRate + 0.5);
	 }
	 alarme(&std_evt, tim);
      } else 
	 serverState = serverIdling;
   } else { 
      // we just had to wait for start of next cycle
      // there should be sth, see late()
      server = dequeuePrio(); 
      alarme(&std_evt, serviceTime);
      serverState = serverServing;
   }
}

int muxFrmPrio::export(exp_typ *msg) 
{
   return baseclass::export(msg) ||
      intArray1(msg, "PQLen", qLens, nprio, 0) ||
      intArray1(msg, "PQByteLen", qByteLens, nprio, 0);
}

// REC is a macro normally expanding to rec (for debugging)
rec_typ muxFrmPrio::REC(data *pd, int iKey) 
{
   int inprio, prio;
   typecheck_i(pd, FrameType, iKey);
   // The mux takes it's input priority directly from the rame
   inprio = ((frame *) pd)->prioCodePoint;
   if (inprio < 0 || inprio >= max_inprio)
      errm1s2d("%s: illegal INPRIO=%d received on input %d", name, inprio, iKey);
   
   prio = priorities[inprio];
   if (prio > maxArrPrio)
      maxArrPrio = prio;
   
   inpPrioPtr->pdata = pd;
   inpPrioPtr->inp = iKey;
   (inpPrioPtr++)->prio = prio;
   
   if (needToSchedule) {
      needToSchedule = FALSE;
      alarml( &evtLate, 0);
   }
   
   return ContSend;
}

