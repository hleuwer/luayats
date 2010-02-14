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
*	Creation:		July 11, 1997
*
*************************************************************************/

/*
*	Purely event triggered Multiplexer, different queues:
*		- synchronous or asynchronous operation of output
*		- Departure First (DF)
*		- A number of queues with absolute priority
*
*	MuxPrio mux: NINP=10, {MAXVCI=100,} {SERVICE=2,} MODE={Async | Sync},
*			NPRIO=3, OUT=sink;
*			// default MAXVCI: NINP
*			// SERVICE: service takes SERVICE time steps
*			// MODE: asynchronous or synchronous output
*			//	MODE can be omitted if SERVICE=1 or omitted
*			// NPRIO: number of queues, 0 ... (NPRIO-1),
*					queue (NPRIO-1) has highest priority
*
*	Commands:	like Multiplexer (see mux.c)
*			mux->SetBuf(prio, bsiz)	// set buffer size of
*						// priority prio to bsiz.
*						// Initially, all sizes are one (!).
*						// Buffer sizes < 1 are impossible.
*			mux->SetPrio(vci, prio)	// put this vci into priority prio.
*						// Initially, a vci has highest
*						// possible prio (NPRIO-1)
*	exported:	mx->PQLen(p)		// queue length of priority p
*
*	On the inputs, only Cells are allowed (need to have a vci to associate
*	to a priority).
*/

#include "muxPrio.h"

CONSTRUCTOR(MuxPrio, muxPrio);

void	muxPrio::addpars()
{
	int	i;

	baseclass::addpars();

	if (serviceTime != 1 || (serviceTime == 1 && test_word("MODE")))
	{	skip_word("MODE");
		skip('=');
		if (test_word("Sync"))
		{	skip_word("Sync");
			syncMode = TRUE;
		}
		else if (test_word("Async"))
		{	skip_word("Async");
			syncMode = FALSE;
		}
		else	syntax0("MODE: `Sync' or `Async' expected");
		skip(',');
	}
	else	syncMode = FALSE;	// serviceTime == 1

	nprio = read_int("NPRIO");
	if (nprio < 1)
		syntax0("invalid NPRIO");
	skip(',');

	CHECK(prioQ = new queue[nprio]);	// queues have zero capacity
	for (i = 0; i < nprio; ++i)
		prioQ[i].setmax(1);		// this is really needed by late()!
	CHECK(qLens = new int[nprio]);
	for (i = 0; i < nprio; ++i)
		qLens[i] = 0;
	CHECK(priorities = new int[max_vci]);
	for (i = 0; i < max_vci; ++i)
		priorities[i] = nprio - 1;	// initially: max priority for all

	CHECK(inpPrioBuf = new inpPrioStruct[ninp]);
	inpPrioPtr = inpPrioBuf;

	maxArrPrio = 0;
}

/*
*	try to dequeue a cell from the queue pool, absolute priorities
*/
inline	data	*muxPrio::dequeuePrio()
{
	int	i;
	for (i = nprio - 1; i >= 0; --i)
		if ( !prioQ[i].isEmpty())
		{	--qLens[i];
			return prioQ[i].dequeue();
		}
	return NULL;
}

/*
*	process arrivals of this step
*/
void	muxPrio::late(
	event	*)
{
        inpPrioStruct	*p;
	int		n;

	needToSchedule = TRUE;

        n = inpPrioPtr - inpPrioBuf;
        for (;;)
        {       // random choice between arrivals
		if (n > 1)
			p = inpPrioBuf + (my_rand() % n);
		else    p = inpPrioBuf;

		if (serverState == serverIdling && p->prio == maxArrPrio)
		{	// Server is idle, i.e. all queues have been empty during early().
			// AND: the priority of this data object is high enough.
			// In addpars() and command(), we ensured queue capacities of at least 1.
			// This is necessary, since we have to queue the object in case we have
			// to wait for sync.
			if (syncMode)
			{	int	dd = SimTime % serviceTime;
				if (dd)
				{	// we have to wait until beginning of an output cycle,
					// but until there we have to leave the item in the queue!
					prioQ[p->prio].enqueue(p->pdata);
					++qLens[p->prio];
					alarme( &std_evt, serviceTime - dd);
					serverState = serverSyncing;
				}
				else	// we have hit the beginning of a cycle
				{	server = p->pdata;
					serverState = serverServing;
					alarme( &std_evt, serviceTime);
				}
			}
			else
			{	server = p->pdata;
				serverState = serverServing;
				alarme( &std_evt, serviceTime);
			}
		}
		else
		{	if (prioQ[p->prio].enqueue(p->pdata))
				++qLens[p->prio];
			else	dropItem(p);	// buffer overflow
		}

		if (--n == 0)
			break;
		*p = inpPrioBuf[n];
	}
	inpPrioPtr = inpPrioBuf;

	maxArrPrio = 0;
}

/*
*	in serving state:
*		Forward a data item to the successor.
*		Register again, if more data queued.
*	in sync state:
*		take data from queue and enter serving state
*/
void	muxPrio::early(
	event	*)
{
	if (serverState == serverServing)
	{	// "normal" case: forward served data
		suc->rec(server, shand);
		server = dequeuePrio();
		if (server)
			alarme(&std_evt, serviceTime);
		else	serverState = serverIdling;
	}
	else
	{	// we just had to wait for start of next cycle
		server = dequeuePrio();	// there should be sth, see late()
		alarme(&std_evt, serviceTime);
		serverState = serverServing;
	}
}

int	muxPrio::command(
	char	*s,
	tok_typ	*pv)
{
	if (baseclass::command(s, pv))
		return TRUE;

	pv->tok = NILVAR;
	if (strcmp(s, "SetBuf") == 0)
	{	skip('(');
		int	prio = read_int(NULL);
		if (prio < 0 || prio >= nprio)
			syntax1s1d("%s: priorities range from 0 to %d", name, nprio - 1);
		skip(',');
		if (prioQ[prio].setmax(read_int(NULL)) == FALSE || prioQ[prio].getmax() < 1)
			// late() really depends on buffer sizes >= 1!
			syntax0("could not set this buffer size");
		skip(')');
	}
	else if (strcmp(s, "SetPrio") == 0)
	{	int	vc, prio;
		skip('(');
		vc = read_int(NULL);
		if (vc < 0 || vc >= max_vci)
			syntax0("invalid VCI");
		skip(',');
		prio = read_int(NULL);
		if (prio < 0 || prio >= nprio)
			syntax0("invalid priority");
		skip(')');
		priorities[vc] = prio;
	}
	else	return FALSE;

	return TRUE;
}

int	muxPrio::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intArray1(msg, "PQLen", qLens, nprio, 0);
}

rec_typ	muxPrio::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	iKey)
{
	int	vc, prio;

	typecheck_i(pd, CellType, iKey);
	vc = ((cell *) pd)->vci;
	if (vc < 0 || vc >= max_vci)
		errm1s2d("%s: illegal VCI=%d received on input %d", name, vc, iKey);

	prio = priorities[vc];
	if (prio > maxArrPrio)
		maxArrPrio = prio;

	inpPrioPtr->pdata = pd;
	inpPrioPtr->inp = iKey;
	(inpPrioPtr++)->prio = prio;

        if (needToSchedule)
        {       needToSchedule = FALSE;
                alarml( &evtLate, 0);
        }

	return ContSend;
}
