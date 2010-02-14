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
* Creation:  Oct 1996
*
*************************************************************************/

/*
* Multiplexer (ARRIVAL FIRST - no additional delay like in Multilpexer):
*
* MuxAF mux: NINP=10, BUFF=10, {MAXVCI=100,} {ACTIVE=2,} OUT=sink;
*   // default MAXVCI: NINP
*   // ACTIVE: only serve during each ACTIVE-th time slot
*
* Commands: see Multiplexer (mux.c)
*/


#include "muxaf.h"

muxAF::muxAF(void)
{
  served = FALSE;  // see comments to early()
  q_hi = q.getmax();
  q_lo = q_hi - 1;
}
muxAF::~muxAF(void)
{
}
#if 0
int muxAF::act(void)
{
  served = FALSE;  // see comments to early()
  q_hi = q.getmax();
  q_lo = q_hi - 1;
  return 0;
}
/*
* read create statement
*/
void muxAF::init(void) {
  baseclass::init();

  served = FALSE;  // see comments to early()
  q_hi = q.getmax();
  q_lo = q_hi - 1;
}
#endif

/*
* Every time slot:
* In the late slot phase, all cells have arrived and we can apply a fair
* service strategy: random choice
* We copy this more or less from the baseclass for performance reasons.
*/

void muxAF::late(event *) {
  int n;

  // process all arrivals in random order
  if ((n = inp_ptr - inp_buff) != 0) {
    inpstruct *p;
    for (;;) {
      if (n > 1)
        p = inp_buff + (my_rand() % n);
      else
        p = inp_buff;
      if (q.enqueue(p->pdata) == FALSE) // buffer overflow
        dropItem(p);
      if (--n == 0)
        break;
      *p = inp_buff[n];
    }
    inp_ptr = inp_buff;
  }

  if (served)  // see comment for early()
  { served = FALSE;
    q.setmax(q_hi);
  }
}

/*
* Every ACTIVE-th time slot: try to send a cell.
* For AF, the cell should be effective for overflow calculation together with arrivals
* of the same slot. Since we have taken the cell from the queue, we reduce the queue
* length by one until we have processed arrivals in late(). This is marked in the flag served.
*/
void muxAF::early(event *) {
  data *p;

  if ((p = q.dequeue()) != NULL) {
    served = TRUE;
    q.setmax(q_lo);
    suc->rec(p, shand);
  }
  alarme( &std_evt, active);
}

