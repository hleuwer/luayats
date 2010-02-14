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
**************************************************************************
*
*	Module author:		Matthias Baumann, TUD
*	Creation:		July 6, 1997
*
*************************************************************************/

/*
*	A class terminating a Start-Stop Protocol. When the predecessor
*	continues to send, if the sucessor has stopped us, then simply the input
*	buffer overflows. But we ourselfs do what the successor expects.
*
*	TermStartStop xyz: BUF=100, OUT=tcpipsend->Data;
*				// BUFF size of input buffer
*		Control input: xyz->Start
*
*	Exported:	xyz->Count	// number of lost data objects
*/

#ifndef TERMSTRTSTP_INCL
#define TERMSTRTSTP_INCL

#include "in1out.h"
#include "queue.h"

//tolua_begin
class termStartStop:	public	in1out	{
  typedef	in1out	baseclass;
public:
  termStartStop(){}
  ~termStartStop(){}
  int act(void);
  rec_typ REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
  void	early(event *);
  inline void	sendTheGuy(data *pd) {
    lastSendTime = SimTime;
    chkStartStop(myState = suc->rec(pd, shand));
    if (myState == ContSend && q.getlen() != 0)
      alarme( &std_evt, 1);
  }

  queue	q;
  rec_typ	myState;
  tim_typ	lastSendTime;
  
  enum {
    InpData = 0, 
    InpStart = 1
  };
};
//tolua_end
#endif
