/*************************************************************************
*
*		YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997	Chair for Telecommunications
*				Dresden University of Technology
*				D-01062 Dresden
*				Germany
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
#ifndef	_TICKCTRL_H_
#define	_TICKCTRL_H_

#include "inxout.h"
#include "queue.h"

//tolua_begin
class	tickctrl: public inxout	{
   typedef	inxout	baseclass;

public:	
   enum {
      keyTick = 1
   };

   tickctrl();
   ~tickctrl();
   int act(void);
   event evtTick;	// event for the late() method (called if arrival)
   void setTable(void *tab){table = (tim_typ*)tab;}
   void *getTable(void){return table;}
   void restim(void);
//tolua_end   
   rec_typ REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
   void early(event *);
   void late(event *);	
   int export(exp_typ *);
//tolua_begin
   int contproc;     	// number of contending processes
   int actproc;      	// the active process now (0 is this one)
   tim_typ next_time;	// the next time, where we can send after timeout
   tim_typ last_received;     // the time, when the last data has been received
   tim_typ off_int;  // the time interval, when going off is detected
   tim_typ old_tm;
   int     phase;

   uqueue q;
   int alarmed;      	// is early alarmed?

   int	q_max;
   int	q_start;	// queue length at which to wake up the sender
   rec_typ prec_state;	// state of the preceeding object
   rec_typ send_state;	// running or stopped?

   enum	{SucData=0, SucCtrl=1};
   enum	{InpData = 0, InpStart = 1};
	
//tolua_end
   tim_typ *table;   	// the distribution of the tick value
}; //tolua_export

#endif	// _TICKCTRL_H_
