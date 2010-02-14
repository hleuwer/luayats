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
* Creation:  July 6, 1997
*
*************************************************************************/

/*
* Base class for multiplexers
*
* XXYYZZ mux: NINP=10, BUFF=10, {MAXVCI=100,} {SERVICE=2,} OUT=sink;
*   // default MAXVCI: NINP
*   // SERVICE: service of one data item takes SERVICE steps
*
* Commands: like Multiplexer (see mux.c)
*/

#include "muxBase.h"


muxBase::muxBase(void):evtLate(this, 0)
{
}

muxBase::~muxBase(void)
{
  delete[] lost;
  delete[] lostVCI;
  delete[] lostINPRIO;
  delete[] inp_buff;
}

int muxBase::act(void)
{
  int i;
  CHECK(lost = new unsigned int[ninp]);
  for (i = 0; i < ninp; ++i)
    lost[i] = 0;
  CHECK(lostVCI = new unsigned int[max_vci]);
  for (i = 0; i < max_vci; ++i)
    lostVCI[i] = 0;
  CHECK(lostINPRIO = new unsigned int[max_inprio]);
  for (i = 0; i < max_inprio; ++i)
    lostINPRIO[i] = 0;
  lossTot = 0;

  CHECK(inp_buff = new inpstruct[ninp]);
  inp_ptr = inp_buff;
  server = NULL;
  return 0;
}

//
// Data item received, buffer it.
// Wake up the late() method, if not yet done.
//
rec_typ muxBase::REC( // REC is a macro normally expanding to rec (for debugging)
  data *pd,
  int i) {
  inp_ptr->inp = i;
  (inp_ptr++)->pdata = pd;

  if (needToSchedule) {
    needToSchedule = FALSE;
    alarml( &evtLate, 0);
  }

  return ContSend;
}

//
// Register a loss and delete the data item
//
void muxBase::dropItem(inpstruct *p) 
{
  if ( ++lost[p->inp] == 0)
    errm1s1d("%s: overflow of LossInp[%d]", name, p->inp + 1);

  if (typequery(p->pdata, CellType)) {
    int vc;
    vc = ((cell *) p->pdata)->vci;
    if (vc >= 0 && vc < max_vci && ++lostVCI[vc] == 0)
      errm1s1d("%s: overflow of LossVCI[%d]", name, vc);
  }
  if (typequery(p->pdata, FrameType)) {
     int inprio;
     inprio = ((frame *) p->pdata)->prioCodePoint;
     if (inprio >= 0 && inprio < max_inprio && ++lostINPRIO[inprio] == 0)
	errm1s1d("%s: overflow of LossINPRIO[%d]", name, inprio);
  }

  if ( ++lossTot == 0)
    errm1s("%s: overflow of LossTot", name);

  delete p->pdata;
}

void muxBase::resLoss(void)
{
  int i;
  //  printf("reset losses %d %d\n", max_vci, ninp);
  for (i = 0; i < ninp; ++i)
    this->lost[i] = 0;
  for (i = 0; i < max_vci; ++i)
    lostVCI[i] = 0;
  lossTot = 0;
}

//
// export addresses of variables
//
int muxBase::export(
  exp_typ *msg) {
  return baseclass::export(msg) ||
         intScalar(msg, "QLen", &q.q_len) ||
         intScalar(msg, "LossTot", (int *) &lossTot) ||
         intArray1(msg, "Loss", (int *) lost, ninp, 1) ||
         intArray1(msg, "LossInp", (int *) lost, ninp, 1) ||
         intArray1(msg, "LossVCI", (int *) lostVCI, max_vci, 0);
}

