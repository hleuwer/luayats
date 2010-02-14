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
*************************************************************************/
#ifndef _MUX_PRIO_H_
#define _MUX_PRIO_H_

#include "muxBase.h"

//tolua_begin
class muxPrio: public muxBase {
  typedef muxBase baseclass;

public:
  muxPrio();
  ~muxPrio();
  //tolua_end
  rec_typ REC(data *, int); // REC is a macro normally expanding to rec (for debugging)
  void early(event *);
  void late(event *);
  data *dequeuePrio();
  int export(exp_typ *);
  //tolua_begin
  queue *getQueue(int prio){return &prioQ[prio];}
  int getQueueLen(int prio){return qLens[prio];}
  void setQueueLen(int prio, int len){qLens[prio] = len;}
  void setPrio(int vci, int prio){priorities[vci] = prio;}
  int getPrio(int vci){return priorities[vci];}
  int nprio;   // # of queues
  int act(void);
  //tolua_end
  queue *prioQ;   // the queues
  int *qLens;   // each queue length is coppied -> simple export
  int *priorities;  // mapping vci -> priority
  struct inpPrioStruct: public inpstruct {
    int prio;
  };

  inpPrioStruct *inpPrioBuf;
  inpPrioStruct *inpPrioPtr;

  //tolua_begin
  typedef enum {serverIdling, serverSyncing, serverServing} serverState_t;
  serverState_t serverState;
  int maxArrPrio;
  int syncMode;
};
//tolua_end

#endif // _MUX_PRIO_H_
