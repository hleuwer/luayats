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

#include "termstrtstp.h"

#if REMOVE_THIS
void	termStartStop::init()
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	q.setmax(read_int("BUF"));	// input buffer size, may be zero
	skip(',');

	output("OUT");
	stdinp();
	input("Start", InpStart);

	myState = ContSend;
	lastSendTime = SimTime;
}
#endif

int termStartStop::act(void)
{
  myState = ContSend;
  lastSendTime = SimTime;
  return 0;
}

// REC is a macro normally expanding to rec (for debugging)
rec_typ	termStartStop::REC(data	*pd, int	inpKey)
{
  switch (inpKey) {
  case InpStart:
    delete pd;
    if (myState == StopSend) {
      myState = ContSend;
      if (q.getlen() != 0) {
	if (0) //lastSendTime != SimTime)	// probably a bug in tcpipsend.c
	  sendTheGuy(q.dequeue());	// may change our state!
	else	alarme( &std_evt, 1);
      }
    }
    return ContSend;

  case InpData:
    if (q.getlen() != 0 || myState != ContSend)	{
      // we cannot send this guy immediately, try to queue it
      if (q.enqueue(pd) == FALSE) {
	delete pd;
	if ( ++counter == 0)
	  errm1s("%s: overflow of loss counter", name);
      }
      return ContSend;
    }
    // if the queue was empty and we are allowed to send,
    // then we have to check whether we still can send during this slot
    if (lastSendTime == SimTime) {
      // wait a slot.
      if (q.enqueue(pd) == FALSE) {
	delete pd;
	if ( ++counter == 0)
	  errm1s("%s: overflow of loss counter", name);
      } else
	alarme( &std_evt, 1);
    } else
      sendTheGuy(pd);
    return ContSend;

  default:
    errm1s("%s: internal error in termStartStop::rec(): invalid input key", name);
    return StopSend;	// compiler warning
  }
}

void	termStartStop::early(event *)
{
  sendTheGuy(q.dequeue());
}
