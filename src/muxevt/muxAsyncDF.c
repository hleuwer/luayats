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
*	Purely event triggered Multiplexer:
*		- asynchronous operation of output (see remarks)
*		- Departure First (DF)
*
*	MuxAsyncDF mux: NINP=10, BUFF=10, {MAXVCI=100,} {SERVICE=2,} OUT=sink;
*			// default MAXVCI: NINP
*			// SERVICE: service takes SERVICE time steps
*
*	Commands:	like Multiplexer (see mux.c)
*
*	Remarks:
*	The multiplexer consists of an input queue, and a server.
*	Data items are fed into the server when it is free. They are
*	forwarded SERVICE time steps later. The server therefore emulates a
*	slower output line with asynchronous cycles.
*	If a data item reaches an empty multiplexer, then it is
*	immediately placed in the server, from where it is forwarded SERVICE steps later.
*	A data item approaching a non-empty system has to wait in the queue.
*
*	DF relates to events taking place during the same time step. If the
*	server begins a new service, then first the next data item is taken from the
*	queue (if any), and then new arrivals are considered.
*	This is implemented by the fact that the early() method always
*	is called prior to late().
*/

#include "muxAsyncDF.h"

CONSTRUCTOR(MuxAsyncDF, muxAsyncDF);

/*
*	process arrivals of this step
*/
void	muxAsyncDF::late(
	event	*)
{
        inpstruct       *p;
	int		n;

	needToSchedule = TRUE;

        n = inp_ptr - inp_buff;
        for (;;)
        {       // random choice between arrivals
		if (n > 1)
			p = inp_buff + (my_rand() % n);
		else    p = inp_buff;

		if (server)
		{	// server is still working on its own
			if (q.enqueue(p->pdata) == FALSE) // buffer overflow
				dropItem(p);
		}
		else
		{	// server is idle
			server = p->pdata;
			alarme( &std_evt, serviceTime);
		}

		if (--n == 0)
			break;
		*p = inp_buff[n];
	}
	inp_ptr = inp_buff;
}

/*
*	Forward a data item to the successor.
*	Register again, if more data queued.
*/
void	muxAsyncDF::early(
	event	*)
{
	suc->rec(server, shand);
	server = q.dequeue();
	if (server)
		alarme(&std_evt, serviceTime);
}

