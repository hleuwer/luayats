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
*		- synchronous operation of output (see remarks)
*		- Arrival First (AF)
*
*	MuxSyncAF mux: NINP=10, BUFF=10, {MAXVCI=100,} {SERVICE=2,} OUT=sink;
*			// default MAXVCI: NINP
*			// SERVICE: service takes SERVICE time steps
*
*	Commands:	like Multiplexer (see mux.c)
*
*	Remarks:
*	The multiplexer consists of an input queue, and a server.
*	Data items are fed into and taken from the server only at time steps
*	with (SimTime % SERVICE == 0). The server therefore emulates a
*	slower output line with synchronous cycles.
*	If a data item reaches an empty multiplexer with the first time
*	step of an output cycle, then it is immediately placed in the server,
*	from where it is forwarded SERVICE steps later.
*	If a data item reaches an empty multiplexer, but not with
*	beginning of the first step of an output cycle, then it
*	has to wait for service until start of next cycle (while remaining
*	in the queue). Then it is put into the server from where it is
*	forwarded after another SERVICE time steps.
*	A data item approaching a non-empty system has to wait in the queue.
*
*	AF relates to events taking place during the same time step. If the
*	server completes a service, then first new arrivals are considered,
*	and then the server is flushed, and the next data item (if any) is
*	transfered from the queue to the server. This also holds for the
*	case where the data item is transferred from the queue to the server
*	with beginning of a new output cycle (an item did not arrive with cycle
*	start): the transfer occures after having queued the current arrivals.
*
*	AF is implemented as follows:
*	When a service is completed (data item forwarded to successor), or
*	when the start of an output cycle has been reached, then the
*	server normally should not clean the server buffer and try to dequeue
*	the next data. Instead, it should postpone these activities until all
*	arrivals are processed. Since we do not know whether the
*	late() method will be called in the current step (depends on arrivals), we
*	cannot easily postpone start of next service. Therefore we already do
*	what we should not do, and remember the time of this mistake
*	in serviceCompleted.
*
*	Now there is a couple of branches in late(), where we process arrivals:
*
*	1) If serviceCompleted differs from SimTime, then we do not have problems.
*	2) Otherwise:
*	   We imagine the item which just has been finished service, to be still
*	   in the system. This works for three cases:
*	   2a)	ServerIdle
*		We see an empty server, which implies an empty queue (otherwise, the
*		server would have taken an item from the queue). We only can accept
*		the item, if the real queue capacity is at least one, since we normally
*		would have to place the item there. We do not use the
*		queue place, but put the item into the server and start service. If
*		we receive more items in the current step, then the system
*		state is the same as if the server directly would have taken an
*		item from the queue (point 2b).
*	   2b)	ServerServing
*		The server is not empty. Either we have just done the things under
*		2a, or the server directly has taken the next item from the queue.
*		In both cases, we normally should see one more item in the queue.
*		Therefore we logically decrease the queue capacity by one.
*	   2c)	ServerSyncing
*		There is sth in the queue, and the server synchronizes to beginning of
*		next output cycle. No mistakes possible, since the data are still queued.
*/

#include "muxSyncAF.h"

CONSTRUCTOR(MuxSyncAF, muxSyncAF);

/*
*	process arrivals of this step
*/
void	muxSyncAF::late(
	event	*)
{
        inpstruct       *p;
	int		n;
	int		dd;

	needToSchedule = TRUE;

        n = inp_ptr - inp_buff;
        for (;;)
        {       // random choice between arrivals
		if (n > 1)
			p = inp_buff + (my_rand() % n);
		else    p = inp_buff;

		switch (serverState) {
		case serverServing:
			// server is still working on its own, test whether he just
			// has taken a data item from the queue
			if (serviceCompleted == SimTime)
			{	// in this case, we still have to account for the item
				if (q.getlen() >= q.getmax() - 1)
					dropItem(p);
				else	q.enqueue(p->pdata);
			}
			else	// otherwise: normal operation
			{	if (q.enqueue(p->pdata) == FALSE) // buffer overflow
					dropItem(p);
			}
			break;
		case serverSyncing:
			// Server is waiting to start of next cycle:
			// So the server cannot have done sth wrong (data item is still
			// in the queue).
			if (q.enqueue(p->pdata) == FALSE) // buffer overflow
				dropItem(p);
			break;
		case serverIdling:
			// Server is idle. -> the queue is empty, too
			dd = SimTime % serviceTime;
			if (dd)
			{	// We have to wait until beginning of an output cycle,
				// but until there we have to leave the item in the queue!
				// We cannot be in a step where the server was doing
				// sth wrong (dd != 0), therefore no test on serviceCompleted necessary.
				if (q.enqueue(p->pdata))
				{	alarme( &std_evt, serviceTime - dd);
					serverState = serverSyncing;
				}
				else	dropItem(p);	// buffer overflow, server remains idling
			}
			else
			{	if (serviceCompleted == SimTime && q.getmax() < 1)
					// The server freed the server buffer too early,
					// and there is no place left in the queue
					dropItem(p);
				else
				{	// for serviceCompleted == SimTime, q.getmax() >= 1:
					// we make the same mistake as the server itself (early()),
					// but we will account for this under switch case serverServing.
					// for serviceCompleted != SimTime: no problems
					server = p->pdata;
					serverState = serverServing;
					alarme( &std_evt, serviceTime);
				}
			}
			break;
		default:errm1s("%s: internal error: bad state in muxSyncAF::late()", name);
		}

		if (--n == 0)
			break;
		*p = inp_buff[n];
	}
	inp_ptr = inp_buff;
}

/*
*	in serving state:
*		Forward a data item to the successor.
*		Register again, if more data queued.
*	in sync state:
*		take data from queue and enter serving state
*
*	Under all circumstances, we make a mistake which we remember
*	in serviceCompleted.
*	a) service completed, new data dequeued: we should dequeue the
*	   the data after having processed arrivals
*	b) service completed, queue empty -> idle state: the server is
*	   idle, but the server buffer should become available
*	   after having processed arrivals
*	c) start of output cycle reached, enter serving state: we should
*	   dequeue the the data after having processed arrivals
*/
void	muxSyncAF::early(
	event	*)
{
	if (serverState == serverServing)
	{	// "normal" case: forward served data
		suc->rec(server, shand);
		server = q.dequeue();
		if (server)
		{	alarme(&std_evt, serviceTime);	// remain serverServing
		}
		else	serverState = serverIdling;
	}
	else
	{	// we just had to wait for start of next cycle
		server = q.dequeue();		// there should be sth, see late()
		alarme(&std_evt, serviceTime);
		serverState = serverServing;
	}

	serviceCompleted = SimTime;	// remember the mistake
}

void	muxSyncAF::restim()
{
	serviceCompleted = (tim_typ) -1;
}

