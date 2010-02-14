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
* History: June 19, 1997:
*   Memory management improved (less memory).
*   The lineItem structs are allocated via new (not
*   overloaded), but they are never deleted. Instead they
*   are stored in lineItemPool for further usage. This
*   pool is shared by all lines.
*    Matthias Baumann
*
*************************************************************************/

/*
* Line with delay:
*
* Leitung <name>: DELAY=<delay in slots>, OUT=<next node>
*
* The object holds cells in an own queue, therefore an object is registered only once
* at the kernel.
*/

#include "line.h"

line::line()
{
}

line::~line()
{
}

line::lineItem *line::lineItemPool = NULL;


// Initialisation by Lua
int line::act(void)
{
  return 0;
}


// Activation by kernel.
// Pass cell.
// If more cells are queued, register again for the next.
void line::early(event *)
{
   lineItem *p = (lineItem *) q.dequeue();

   suc->rec(p->item, shand);

   // the lineItem remains in our pool, and is not deleted
   p->next = lineItemPool;
   lineItemPool = p;

   if ( !q.isEmpty())
      alarme( &std_evt, q.first()->time - SimTime);
}

/*
* cell received.
*/

rec_typ line::REC( // REC is a macro normally expanding to rec (for debugging)
   data *pd,
   int )
{
   lineItem *pi;

   if ((pi = lineItemPool) == NULL)
      CHECK(pi = new lineItem); // The operator new is not overloaded,
   // but after a short time we should have enough
   // items in the pool.
   // The "new" causes data::data() to be called
   // (which sets time), this overhead too should
   // vanish when the lineItemPool is "large" enough
   else
      lineItemPool = (lineItem *) lineItemPool->next;

   pi->item = pd;
   if (q.isEmpty())
      alarme( &std_evt, delay); // we do not need to set the time
   else
      pi->time = SimTime + delay;
   q.enqueue(pi);

   return ContSend;
}


/*
* We store our own times.
* recalculate them if SimTime is reset.
*/
void line::restim(void)
{
   data *pd, *pfirst;

   if ((pfirst = q.first()) == NULL)
      return ;

   // "rotate" the queue
   do {
      pd = q.dequeue(); // take item from front
      pd->time -= SimTime;
      q.enqueue(pd);  // append item at the end
   } while (q.first() != pfirst); // stop if gone through once
}
