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
*	Creation:		1996
*
*************************************************************************/

/*
*	A GMDP source which can be controlled by the Start-Stop-protocol.
*
*	Additional input:	src->Start
*/

#include "gmdpstop.h"

#define	TRIAL_MAX	(10000)	/* max # of trials to leave a zero bit rate state (GMDP) */

gmdpstop::gmdpstop(void)
{
  send_state = ContSend;
  next_time = 0;
}

gmdpstop::~gmdpstop(void)
{
}

// Every incomming data item is interpreted as a Start message
// REC is a macro normally expanding to rec (for debugging)
rec_typ gmdpstop::REC(data * pd, int)
{
  if (send_state == StopSend) {
    send_state = ContSend;
    if (next_time > SimTime)
      alarme(&std_evt, next_time - SimTime);
    else {
      alarme(&std_evt, 1);
      next_time = SimTime + 1;
    }
  }

  delete pd;

  return ContSend;
}

// Unfortunately, we have to copy gmdp::early() more or less
void gmdpstop::early(event *)
{
  int r, t, st;
  int *p;
  int trials;
  tim_typ tim;

  //    send cell, test return value
  chkStartStop(send_state = suc->rec(new cell(vci), shand));

  //  Count cells
  if (++counter == 0)
    errm1s("%s: overflow of departs", name);

  // When to send next cell?
  if (--cell_cnt > 0)
    // State lasts for more cells
    tim = delta[state];
  else {
    // This was the last cell, determine next state
    t = 0;
    trials = 0;
    st = state;
    for (;;) {
      // Get and transform r.n.
      r = my_rand() % RAND_MODULO;
      p = trafo[st];
      for (st = 0; st < n_stat; ++st)
	if (r < p[st])
	  break;
      if (st >= n_stat)
	errm1s("%s: gmdp::early(): bad state transition", name);

      if (delta[st] != 0)
	break;

      //  In case of a state with zero bit rate, look ahead to find
      //  next cell
      // t += geo1_rand(dists[st]);
      t += tables[st][my_rand() % RAND_MODULO];
      if (++trials >= TRIAL_MAX)
	errm1s2d("%s: gmdp::early(): could not leave state no. %d "
		 "after TRIAL_MAX=%d attempts", name, st + 1, TRIAL_MAX);
    }
    // Non zero bit rate state reached
    // cell_cnt = geo1_rand(dists[st]);
    cell_cnt = tables[st][my_rand() % RAND_MODULO];
    state = st;
    tim = t + delta[st];
  }

  switch (send_state) {
  case ContSend:
    // Register for next cell
    alarme(&std_evt, tim);
    break;
  case StopSend:
    // We have to wait, store the intended send time
    next_time = SimTime + tim;
    break;
  default:
    errm2s("%s: invalid rec() return value from `%s'", name, suc->name);
  }
}

void gmdpstop::restim(void)
{
  if (next_time > SimTime)
    next_time -= SimTime;
  else
    next_time = 0;
}
