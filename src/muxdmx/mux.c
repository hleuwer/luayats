/*************************************************************************
*
*  YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*    Copyright (C) 1995-1997 Chair for Telecommunications
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
* Creation:  1996
*
*************************************************************************/

/*
* Multiplexer:
* Multiplexer mux: NINP=10, BUFF=10, {MAXVCI=4,} OUT=sink;
*   // default MAXVCI: NINP
*
* Commands:
*
* provided by command():
* mux->Losses '(' lo ',' up ')'
* mux->LossesInp '(' lo ',' up ')'
*  // sum of losses from input lo to up
* mux->LossesVCI '(' lo ',' up ')'
*  // sum of losses from VCI lo to up
* mux->ResLoss
*  // reset loss counters
*
* provided by export():
* mux->Loss(i)
* mux->LossInp(i)
*  // loss on one input
* mux->LossVCI(i)
*  // loss on one VCI
* mux->LossTot
*  // total loss
* mux->QLen
*  // current queue length
*/

/*
* History:
* 26.11.95:
* - there is still a little bug here:
* The waiting time inside the mux is obviously at least one time slot. This is due to
* the fact that a cell is only sent in the _next_ time slot, if cells have been in the
* buffer at the end of the _last_ time slot. Actually, we should send a cell in the next
* slot, if after receiving all cells in the current slot there is at least one in the buffer
* at the end of the _current_ slot. What's about q_max in this case (change it ore not)?
*
* 24.08.96:
* Like it is now (without changes), an Arrival First strategy is implemented. The sojourn time is
* one slot too much. If we would register for sending in case a cell is in the system at the
* end of late(), then this would be Departure First with the right sojourn time.
* 24.08.96:
* The queue class used instead of own poiters. AF like before.
*
* 25.10.96:
* In rec(): TESTLT() und typecheck_i() deleted. Per-VC count of losses introduced.
* New counter LossTot for total loss introduced. Extra method for registering loss: thus it
* can be reused by derived classes (loss is - hopefully - rare, so that the function call will
* not decrease performance).
*  Matthias Baumann
*/


#include "mux.h"

mux::~mux(void)
{
//	delete &(event_each);
	delete[] lost;
	delete[] lostVCI;
	delete[] inp_buff;	
}


// Mux init by Lua.
int mux::act(void){
  int i;
  CHECK(lost = new unsigned int[ninp]);
  for (i = 0; i < ninp; ++i)
    lost[i] = 0;
  CHECK(lostVCI = new unsigned int[max_vci]);
  for (i = 0; i < max_vci; ++i)
    lostVCI[i] = 0;
  lossTot = 0;
  
  CHECK(inp_buff = new inpstruct[ninp]);
  inp_ptr = inp_buff;
  
  eachl( &event_each);	// event initialized by mux::mux()
  return 0;
}


/*
* a cell has arrived.
* store the cell and the number of the input line.
*/
rec_typ mux::REC( // REC is a macro normally expanding to rec (for debugging)
   data *pd,
   int i)
{
   inp_ptr->inp = i;
   (inp_ptr++)->pdata = pd;

   return ContSend;
}

/*
* in the late slot phase, all cells have arrived and we can apply a fair
* service strategy: random choice
*/
void mux::late(event *)
{
   inpstruct *p;
   int old_q_len, n;

   old_q_len = q.getlen();

   // process all arrivals in random order
   if ((n = inp_ptr - inp_buff) != 0) {
      do {
         if (n > 1)
            p = inp_buff + (my_rand() % n);
         else
            p = inp_buff;
         if (q.enqueue(p->pdata) == FALSE) // buffer overflow
            dropItem(p);
         *p = inp_buff[--n];
      } while (n != 0);
      inp_ptr = inp_buff;
   }

   // serve one cell:
   // we can not do that immediatly, but in the early phase of the next time slot
   // -> see early()
   if (old_q_len != 0)
      alarme( &std_evt, 1);
}

/*
* register a loss and delete the data item
*/
void mux::dropItem(inpstruct *p)
{
   if ( ++lost[p->inp] == 0)
      errm1s1d("%s: overflow of LossInp[%d]", name, p->inp + 1);

   if (typequery(p->pdata, CellType)) {
      int vc;
      vc = ((cell *) p->pdata)->vci;
      if (vc >= 0 && vc < max_vci && ++lostVCI[vc] == 0)
         errm1s1d("%s: overflow of LossVCI[%d]", name, vc);
   }

   if ( ++lossTot == 0)
      errm1s("%s: overflow of LossTot", name);

   delete p->pdata;
}

/*
* send a served data item
*/
void mux::early(event *)
{
   suc->rec(q.dequeue(), shand);
}

void mux::resLoss(void)
{
  int i;
  //  printf("reset losses %d %d\n", max_vci, ninp);
  for (i = 0; i < ninp; ++i)
    this->lost[i] = 0;
  for (i = 0; i < max_vci; ++i)
    lostVCI[i] = 0;
  lossTot = 0;
}

/*
* command
*/

/*

#ifndef USELUA
int mux::command(
   char *s,
   tok_typ *v)
{
   int lo, up;
   int i;

   if (baseclass::command(s, v) == TRUE)
      return TRUE;

   if (strcmp(s, "Losses") == 0 || strcmp(s, "LossesInp") == 0) {
      skip('(');
      lo = read_int(NULL);
      skip(',');
      up = read_int(NULL);
      skip(')');

      --lo;
      --up;
      if (lo < 0 || up >= ninp)
         syntax1s1d("%s: input numbers range from 1 to %d", name, ninp);

      v->val.i = 0;
      for (i = lo; i <= up; ++i)
         v->val.i += lost[i];
      v->tok = IVAL;
      return TRUE;
   }
   if (strcmp(s, "LossesVCI") == 0) {
      skip('(');
      lo = read_int(NULL);
      skip(',');
      up = read_int(NULL);
      skip(')');

      if (lo < 0 || up >= max_vci)
         syntax1s1d("%s: VC numbers range from 0 to %d", name, max_vci - 1);

      v->val.i = 0;
      for (i = lo; i <= up; ++i)
         v->val.i += lostVCI[i];
      v->tok = IVAL;
      return TRUE;
   } else if (strcmp(s, "ResLoss") == 0) {
      for (i = 0; i < ninp; ++i)
         lost[i] = 0;
      for (i = 0; i < max_vci; ++i)
         lostVCI[i] = 0;
      lossTot = 0;
      v->tok = NILVAR;
      return TRUE;
   } else
      return FALSE;
}
#endif

*/

/*
* export addresses of variables
*/
int mux::export(
   exp_typ *msg)
{
   return baseclass::export(msg) ||
          intScalar(msg, "QLen", &q.q_len) ||
          intScalar(msg, "LossTot", (int *) &lossTot) ||
          intArray1(msg, "Loss", (int *) lost, ninp, 1) ||
          intArray1(msg, "LossInp", (int *) lost, ninp, 1) ||
          intArray1(msg, "LossVCI", (int *) lostVCI, max_vci, 0);
}

