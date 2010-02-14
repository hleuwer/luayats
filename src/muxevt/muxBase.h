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
#ifndef _MUX_BASE_H_
#define _MUX_BASE_H_

#include "in1out.h"
#include "queue.h"
//tolua_begin
class muxBase: public in1out {
  typedef in1out baseclass;

public:
  muxBase(void);
  ~muxBase(void);
  int act(void);
  int getLoss(int i) { return this->lost[i - 1];}
  int getLossVCI(int i) { return this->lostVCI[i];}
  int getLossINPRIO(int i) { return this->lostINPRIO[i];}
  int getLossTot(void) { return this->lossTot;}
  void resLoss(void);
  int getNinp(void) { return this->ninp;}
  int getMaxVCI(void) { return this->max_vci;}
  int getMaxINPRIO(void) { return this->max_inprio;}
  void setMaxVCI(int maxvci) {this->max_vci = maxvci;}
  void setMaxINPRIO(int maxinprio) {this->max_inprio = maxinprio;}
  //tolua_end
  struct inpstruct {
    data *pdata;
    int inp;
  };
  rec_typ REC(data *, int); // REC is a macro normally expanding to rec (for debugging)
  int export(exp_typ *);
  void dropItem(inpstruct *);
  //tolua_begin
  int ninp;     // # of inputs
  int max_vci;  
  int max_inprio;
  event evtLate;  // event for the late() method (called if arrival)

  queue q;   // system queue, does *not* include server

  int needToSchedule;  // TRUE: first arrival during this time step: activate late()
  tim_typ serviceTime;  // length of one output time slot (in simulation time steps)
  //tolua_end
  unsigned int *lost;  /* one loss counter per input line */
  unsigned int *lostVCI; // loss counter per VC
  unsigned int *lostINPRIO; // loss counter per INPRIO
  unsigned lossTot; // total losses
  int doParseBufSiz;
  data *server;  // the server: contains the data item which currently is played out
  inpstruct *inp_buff; /* buffer for cells arriving in the early slot phase */
  inpstruct *inp_ptr; /* current position in inp_buff */
}; //tolua_export

#endif // _MUX_BASE_H_
