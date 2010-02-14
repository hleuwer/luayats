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
*	Creation:		1996
*
*************************************************************************/

/*
*	A source translating an incomming cell to a burst
*
*	IP2ATM src: DELTA=15, BLEN=172, OUT=sink;
*			// DELTA:	cell spacing within the burst
*			// BLEN:	burst length
*
*	The vci number of all cells of a burst is set according to
*	the request cell.
*	A request arriving while the last one is not yet fully processed,
*	is queued internally (infinite queue size).
*/

#include "ip2atm.h"

CONSTRUCTOR(Ip2atm, ip2atm);

/*
*	read create statement
*/

void	ip2atm::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	delta = read_int("DELTA");
	skip(',');
	b_len = read_int("BLEN");
	skip(',');

	// read additional parameters of derived classes
	addpars();

	output("OUT");
	stdinp();		// input for request cells

	q_head = NULL;
}

/*
*	cell has arrived: new request to send a burst
*/
rec_typ	ip2atm::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	cell	*pc = (cell *) pd;

	typecheck(pc, CellType);

	// We use the cell directly for the internal queue (member c_next for chaining).
	// The cell is freed, if the burst has been transmitted.
	if (q_head != NULL)
	{	// the queue was not empty, i.e. the source is running -> append the request
		q_tail->next = pc;
		q_tail = pc;
	}
	else
	{	// the source had a nap -> activate it
		q_head = q_tail = pc;
		this->start();
	}

	return ContSend;
}

/*
*	activation by the kernel: send a cell
*/
void	ip2atm::early(event *)
{
	cell	*pc;

	// send cell
	suc->rec(new cell(vci), shand);

	if ( ++counter == 0)
		errm1s("%s: overflow of departs", name);

	if (++send_cnt < b_len)
	{	// there are more cells in the current burst
		alarme( &std_evt, delta);
		return;
	}

	//	more requests in the queue?
	pc = q_head;
	if (q_head == q_tail)
		q_head = NULL;		// no
	else
	{	q_head = (cell *) q_head->next;  // yes
		this->start();		// register again for transmission
	}
	delete pc;			// this was the old request cell
}

/*
*	Start transmission of a burst:
*		the first cell is sent with timer expiry
*/

void	ip2atm::start(void)
{
	vci = q_head->vci;
	send_cnt = 0;
	alarme( &std_evt, delta);
}

