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
* Multiplexer (DEPARTURE FIRST):
*
* MuxDF mux: NINP=10, BUFF=10, {MAXVCI=100,} {ACTIVE=2,} OUT=sink;
*   // default MAXVCI: NINP
*   // ACTIVE: only serve during each ACTIVE-th time slot
*
* Commands: like Multiplexer (see mux.c)
*/

/*
* History:
* 25.10.96: MuxDF now derived from mux
*    Matthias Baumann
*/

#include "muxdf.h"

CONSTRUCTOR(MuxDF, muxDF);

muxDF::muxDF(void)
{
}
muxDF::~muxDF(void)
{
}
#if 0
/*
* read create statement
*/
void muxDF::addpars(void) {
  baseclass::addpars();

  if (test_word("ACTIVE")) {
    active = read_int("ACTIVE");
    if (active < 1)
      syntax0("invalid ACTIVE");
    skip(',');
  } else
    active = 1;

  alarme( &std_evt, active);
}
#endif
/*
* Every time slot:
* In the late slot phase, all cells have arrived and we can apply a fair
* service strategy: random choice
*/

void muxDF::late(event *) {
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
}

/*
* Every ACTIVE-th time slot: try to send a cell.
* Since this is done prior to copying cells from the input buffer to the queue,
* DEPARTURE FIRST is realized.
*/
void muxDF::early(event *) {
  data *p;

  if ((p = q.dequeue()) != NULL)
    suc->rec(p, shand);
  alarme( &std_evt, active);
}

