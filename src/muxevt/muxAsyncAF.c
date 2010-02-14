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
*	History:
*	May 13, 1998:		optional non-integer service time introduced.
*				Change: at every place where the next service
*				is started, the method nextServiceTime() is
*				called. It implements a bucket mechanism
*				which ensures the required mean service time.
*						Matthias Baumann
*
*
*************************************************************************/

/*
*	Purely event triggered Multiplexer:
*		- asynchronous operation of output (see remarks)
*		- Arrival First (AF)
*
*	MuxAsyncAF mux: NINP=10, BUFF=10, {MAXVCI=100,} {SERVICE=2,} {MEANSERV=3.5,} OUT=sink;
*			// default MAXVCI: NINP
*			// SERVICE: service takes SERVICE time steps
*			// MEANSERV: the mean service time is this amount of time steps.
*			//	MEANSERV overrides SERVICE (in case both are given).
*			//	The mean service time is implemented as a service time
*			//	alternating between the next smaller integer time and
*			//	the next larger one, with the right frequency ratio between both.
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
*	AF relates to events taking place during the same time step. If the
*	server completes a service, then first new arrivals are considered,
*	and then the server is flushed, and the next data item (if any) is
*	transfered from the queue to the server. 
*
*	AF is implemented as follows:
*	When a service is completed (data item forwarded to successor), the
*	server normally should not clean the server buffer and try to dequeue
*	next data. Instead it should postpone these activities until all arrivals
*	have been processed by late().
*	Since we do not know whether the late() method is called
*	in the current step (depends on arrivals), we cannot easily postpone start
*	of next service. Therefore we already do what we should not do, and remember
*	the time of this mistake in serviceCompleted.
*
*	Now there is a couple of branches in late(), where we process arrivals:
*
*	1) If serviceCompleted differs from SimTime, then we do not have problems.
*	2) Otherwise:
*	   We imagine the item which just has been finished service, to be still
*	   in the system. This works for two cases:
*	   2a)	We see an empty server, which implies an empty queue (otherwise, the
*		server would have taken an item from the queue). We only can accept
*		the item, if the real queue capacity is at least one, since we
*		normally would have to place the item there. We do not use the
*		queue place, but put the item into the server and start service. If
*		we receive more items in the current step, then the system
*		state is the same as if the server directly would have taken an
*		item from the queue (point b2).
*	   2b)	The server is not empty. Either we have just done the things under
*		2a, or the server directly has taken the next item from the queue.
*		In both cases, we should see one more item in the queue. Therefore
*		we logically decrease the queue capacity by one.
*/

#include "muxAsyncAF.h"

CONSTRUCTOR(MuxAsyncAF, muxAsyncAF);

//	read a non-integer service time (if present)
void	muxAsyncAF::addpars()
{
	if (test_word("MEANSERV"))
	{	doubleServiceTime = read_double("MEANSERV");
		if (doubleServiceTime < 1.0)
			syntax1s("%s: MEANSERV cannot be smaller than 1.0", name);
		skip(',');
		serviceTime = 0;	// this is the sign that we have a non-int service time

		// init the bucket mechanism
		bucket = 0.0;
		delta_short = (int) doubleServiceTime;
		delta_long = delta_short + 1;
		splash = ((double) delta_long) - doubleServiceTime;
	}
}

/*
*	process arrivals of this step
*/
void	muxAsyncAF::late(
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

		if (serviceCompleted == SimTime)
		{	if (server)
			{	// the item in the server normally shouldn't be there yet,
				// but it still should be in the queue
				if (q.getlen() >= q.getmax() - 1)
					dropItem(p);
				else	q.enqueue(p->pdata);
			}
			else	// server and queue are free, but the server normally
				// still would be occupied
			{	if (q.getmax() < 1)
					dropItem(p);
				else
				{	// we make the same mistake
					// as the server itself (in early())
					server = p->pdata;
					// old: alarme( &std_evt, serviceTime);
					alarme( &std_evt, nextServiceTime());
				}
			}
		}
		else	// no problems
		{	if (server)
			{	if (q.enqueue(p->pdata) == FALSE) // buffer overflow
					dropItem(p);
			}
			else	// server and queue are free
			{	server = p->pdata;
				// old: alarme( &std_evt, serviceTime);
				alarme( &std_evt, nextServiceTime());
			}
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
void	muxAsyncAF::early(
	event	*)
{
	suc->rec(server, shand);
	server = q.dequeue();
	if (server)
		// old: alarme(&std_evt, serviceTime);
		alarme(&std_evt, nextServiceTime());
	serviceCompleted = SimTime;	// remember the mistake we just made
}

void	muxAsyncAF::restim()
{
	serviceCompleted = (tim_typ) -1;
}

