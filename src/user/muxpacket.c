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
*	Creation:		Oct 1996
*
*	History:	July 2, 1997
*			- bug fix: in init(), the serving flag has not been set
*			  to FALSE.
*				Matthias Baumann
*
*************************************************************************/

/*
*	Multiplexer with arbitrarily distributed server time:
*
*	MuxDist mux: NINP=10, BUFF=10, {MAXVCI=100,} DIST=dist, OUT=sink;
*			// MAXVCI: count losses on 0 <= VCI <= MAXVCI, default: NINP
*			// DIST: distribution object to ask for the distribution
*
*	Server strategy: Departure First
*	Losses are counted per input and additionally per VC if lost data item is a cell and
*	its VCI is in the given range.
*
*	Commands:	like Multiplexer (see mux.c)
*/


#include "muxpacket.h"

muxpacket::muxpacket()
{
}

muxpacket::~muxpacket()
{
}

/*
*	in the late slot phase, all cells have arrived and we can apply a fair
*	service strategy: random choice
*/
void	muxpacket::late(event *)
{
   inpstruct	*p;
   int		n;

   //	process all arrivals in random order
   if ((n = inp_ptr - inp_buff) != 0)
   {
      for (;;)
      {
         if (n > 1)
	    p = inp_buff + (my_rand() % n);
	 else
	    p = inp_buff;
	 
	 if (q.enqueue(p->pdata) == FALSE) // buffer overflow
	    dropItem(p);
	 
	 if ( --n == 0)
	    break;
	 
	 *p = inp_buff[n];
      }
      inp_ptr = inp_buff;
   }

   // serve one cell, if server is free:
   if (serving == FALSE && q.getlen() != 0)
   {
      tim_typ tim;
      frame *pf;

      pf = (frame*) q.first();
      typecheck(pf, FrameType);

      tim = (tim_typ) (pf->pdu_len() * serviceStepsPerByte + 0.5);
      if (tim < 1)
	 tim = 1;
      alarme( &std_evt, tim);
      serving = TRUE;
   }
}

void	muxpacket::early(event *)
{
   suc->rec(q.dequeue(), shand);
   serving = FALSE;
}

//	Transfered to LUA init

/*
*	read create statement
*/

/*
void	muxpacket::addpars(void)
{

   serviceStepsPerByte = read_double("BYTEFACT");
   if (serviceStepsPerByte <= 0.0)
      syntax0("invalid BYTEFACT");
   skip(',');

   serving = FALSE;	// bug fix: July 2, 1997. MBau
   
}
*/

