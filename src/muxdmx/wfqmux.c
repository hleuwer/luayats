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
* Module author:  Sven Forner, TUD (student's thesis)
*    Matthias Baumann, TUD
* Creation:  Oct/Nov 1996
*
* History:
* July 17, 1997:  - different error message for unregistered
*      VCI. (old version: same message as for
*      out-of-range VCI).
*     Matthias Baumann
*
*************************************************************************/

/*
* Multiplexer with Waited Fair Queueing according to J.W. Roberts
*
* MuxWFQ mux: NINP=10, {MAXVCI=100}, OUT=line;
*   // default MAXVCI: NINP
*
* The per-connection parameters are set by command.
* Incomming cells with a VCI not yet set with SetPar() cause an error message.
*
* Commands
* ========
*  mux->SetPar(vci, delta, buff);
*  // set inverse mean cell rate and buffer size for connection vci
*
* Other commands: as Multiplexer, see file "mux.c".
*
* Exportet Variables
* ==================
*  mux->QLenVCI(vc) // input queue length of the vc
*  mux->SpacingTime // current spacing time
*
* WFQ Algorithm
* =============
* See User's manual.
*
* Implementation Details
* ======================
* For each VC, there is a structure (type wfqpar) holding the parameters
* mean cell distance and queue size as well the per-VC queue itself.
* The sort queue at the mux output does not contain cells, but the wfqpar
* structures of those VC which currently have a cell in their queue.
* Therefore the information whether a queue containes cells also says
* whether this VC currently in the sort queue (used in late() where incomming
* cells are put into the right queues).
* When serving the output of the multiplexer, the front wfqpar struct is taken
* from the sort queue, and a cell is dequeued from the associated per-VC queue.
* If this queue afterwards is not yet empty, then the wfqpar struct again
* is queued in the sort queue, according to the new virtual time.
* The virtual time of a connection (i.e. of the first cell in the per-VC queue)
* is stored in the wfqpar member time (inherited from cell).
*
* Since the Spacing Time is growing relatively fast, it is reset when
* exceeding the constant spacTimeMax.
*/

#include "wfqmux.h"

muxWFQ::muxWFQ()
{
}
muxWFQ::~muxWFQ()
{
  //  printf("#1# muxWFQ destruct\n");
  delete[] partab;
  delete[] qLenVCI;
}

void muxWFQ::setQueue(int vc, wfqpar *par)
{
  par->vc = vc;
  partab[par->vc] = par;
}

wfqpar * muxWFQ::getQueue(int vc)
{
  return partab[vc];
}

int muxWFQ::act(void)
{
  int i;
  dprintf("%s: act(): Finishing max_vci=%d\n", name, max_vci);
  baseclass::act();
  spacTime = 0;
  CHECK(qLenVCI = new int[max_vci]);
  CHECK(partab = new wfqpar * [max_vci]);
  for (i = 0; i < max_vci; ++i) {
    partab[i] = NULL;
    qLenVCI[i] = 0;
  }
  return 0;
}
#if 0
wfqpar* muxWFQ::createQueue(int vc)
{
  wfqpar *par;
  CHECK(par = new wfqpar(vc));
  partab[vc] = par;
  return par;
}
#endif
/*
* Place incomming cells in the right buffer.
* Two cases:
* 1. There are still cells in the per-VC input queue. This means that
*  this VC is still enqueued in the sort queue. So we only have
*  to put the cell into the per-VC queue.
* 2. The input queue of the VC is empty. This means that the VC currently
*  *not* is in the sort queue. So we have to include it at the right
*  position.
*/
void muxWFQ::late(event *) 
{
  int n, vc;
  cell *pc;
  wfqpar *ppar;
  inpstruct *p;

  // Handle inputs in random order:
  // Normally, the result should not depend on the order we handle the inputs. But
  // this will not hold in case we have the same VCI on more than one input.
  // Who knows ...
  n = inp_ptr - inp_buff;
  while (n > 0) {
    if (n > 1)
      p = inp_buff + my_rand() % n;
    else
      p = inp_buff;

    typecheck_i(p->pdata, CellType, p->inp);
    pc = (cell *) p->pdata;
    vc = pc->vci;
    if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: cell with invalid VCI = %d received", name, vc);
    if ((ppar = partab[vc]) == NULL)
      errm1s1d("%s: cell with unregistered VCI = %d received", name, vc);

    if (ppar->q.isEmpty()) { // This means that this VC currently is not in the sort queue,
      // the input buffer is empty. So we have to enqueue it.
      ppar->time = spacTime + ppar->delta;
      sortq.enqTime(ppar);
    }

    // It is impossible that the enqueueing fails in case we just have put
    // the VC in the sort queue: in command(), we have ensured a queue limit of
    // at least one.
    if ( !ppar->q.enqueue(pc))
      dropItem(p);
    else
      ++qLenVCI[vc];

    *p = inp_buff[ --n];
  }

  inp_ptr = inp_buff;

  if ( !sortq.isEmpty())
    alarme( &std_evt, 1);
}

/*
* send a cell
*/
void muxWFQ::early(event *) 
{
  wfqpar *p;

  // dequeue the VC from the sort queue, take a cell from the corresponding
  // queue and send the cell.
  p = (wfqpar *)sortq.dequeue();
  spacTime = p->time;
  suc->rec(p->q.dequeue(), shand);
  qLenVCI[p->vc]--;

  if (spacTime > spacTimeMax){
    dprintf("%s: restim()\n", name);
    restim(); // reset the spacing time if too large
  }
  // if the per-VC queue is not yet empty: enqueue it again
  if ( !p->q.isEmpty()) {
    p->time = spacTime + p->delta;
    sortq.enqTime(p);
  }
}

/*
* export the per-VC queue length and spacing time
*/
int muxWFQ::export(exp_typ *msg) 
{
  return baseclass::export(msg) ||
         intArray1(msg, "QLenVCI", qLenVCI, max_vci, 0) ||
         intScalar(msg, "SpacingTime", (int *) &spacTime);
}

#if 0
/*
* command:
* mux->SetPar(vci, delta, buff)
*  // set the WFQ parameters of VC vci
*/
int muxWFQ::command(
  char *s,
  tok_typ *pv) {
  int vc;

  if (baseclass::command(s, pv))
    return TRUE;

  pv->tok = NILVAR;
  if (strcmp(s, "SetPar") == 0) {
    skip('(');
    vc = read_int(NULL);
    if (vc < 0 || vc >= max_vci)
      syntax1d("VCI %d is out-of-range", vc);
    if (partab[vc] != NULL)
      syntax1d("VCI %d already has been initialized", vc);

    CHECK(partab[vc] = new wfqpar(vc));
    skip(',');
    if ((partab[vc]->delta = read_int(NULL)) < 1)
      syntax0("invalid DELTA value");
    skip(',');
    partab[vc]->q.setmax(read_int(NULL));
    if (partab[vc]->q.getmax() < 1)
      syntax0("invalid buffer size");
    skip(')');
  } else
    return FALSE;

  return TRUE;
}
#endif
/*
* SimTime has been reset or spacing time reached limit spacTimeMax:
* Reset SpacTime
*/
void muxWFQ::restim(void) {
  int i;

  for (i = 0; i < max_vci; ++i) {
    if (partab[i] == NULL || partab[i]->q.isEmpty())
      continue;

    if (partab[i]->time < spacTime)
      errm1s("%s: internal error on reset spacing time", name);

    partab[i]->time -= spacTime;
  }

  spacTime = 0;
}
