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
#ifndef _MUX_H_
#define _MUX_H_

#include "in1out.h"
#include "queue.h"


typedef struct
{
   data *pdata;
   int inp;
} inpstruct;

//tolua_begin
class mux: public in1out
{
      typedef in1out baseclass;

   public:
      mux():event_each(this, 0)
      {
//         event_each = new event(this, 0);
         doParseBufSiz = TRUE; // turn on parsing buffer size by init()
         // can be changed by constructors of derived
         // classes, e.g. muxWFQ
      }
      ~mux();
      int act(void);
//      int cmd(char*);
      int getLoss(int i) { return this->lost[i - 1];}
      int getLossVCI(int i) { return this->lostVCI[i];}
      int getLossTot(void) { return this->lossTot;}
      void resLoss(void);
      int getNinp(void) { return this->ninp;}
      int getMaxVCI(void) { return this->max_vci;}
      void setMaxVCI(int maxvci) {this->max_vci = maxvci;}
//tolua_end
      void early(event *);
      void late(event *);
      rec_typ REC(data *, int); // REC is a macro normally expanding to rec (for debugging)
      int export(exp_typ *);
      void dropItem(inpstruct *);

       // public members
//tolua_begin
      int ninp;    // # of inputs
      int max_vci;
      queue q;   // system queue
//tolua_end
	
      event event_each;   // event for the late() method (called in each slot)

      int doParseBufSiz;   // TRUE: parse the buffer size in init()
      // can be turned off by derived classes, e.g. muxWFQ

      unsigned int *lost;   /* one loss counter per input line */
      unsigned int *lostVCI;  // loss counter per VC
      unsigned lossTot;  // total losses

      inpstruct *inp_buff;  /* buffer for cells arriving in the early slot phase */
      inpstruct *inp_ptr;  /* current position in inp_buff */
//tolua_begin
}; 
//tolua_end
#endif // _MUX_H_
