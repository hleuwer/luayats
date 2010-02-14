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
*	Test:
*	Shaper controlling the preceeding object with the Start-Stop protocol
*
*	ShapCtrl sh:	DELTA=2,		// data spacing
*			BUFF=10,		// own input buffer size
*			BSTART=5,		// buffer occupation at which Start is generated
*			OUTCTRL=src->Stop,	// control input of the source
*			OUTDATA=meas;
*
*	A Stop-Message is sent (i.e. returned by the rec() method), if an incomming data item
*	fills the buffer completely.
*	Start is generated, if sending a data item reduces the buffer occupation to BSTART.
*/

#include "shapctrl.h"

CONSTRUCTOR(Shapctrl, shapctrl);

void	shapctrl::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	delta = read_int("DELTA");
	skip(',');
	q_max = read_int("BUFF");
	if (q_max <= 0)
		syntax0("BUFF has to be greater than zero");
	skip(',');

	q_start = read_int("BSTART");
	if (q_start < 0 || q_start >= q_max)
		syntax0("invalid BSTART value");
	skip(',');

	output("OUTCTRL", SucCtrl);
	skip(',');

	// read additional parameters of derived classes
	addpars();

	output("OUTDATA", SucData);
	stdinp();

	q_len = 0;
	next_time = 0;
	prec_state = ContSend;
}

/*
*	A data item has been arriving.
*/
rec_typ	shapctrl::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	// redundant: typecheck(pd, DataType);

	if (prec_state == StopSend)
	{	fprintf(stderr, "%s: preceeding object did not recognize the Stop signal\n", name);
		delete pd;
		return StopSend;
	}

	if (q_len == 0)
	{	//	buffer empty. Enough time elapsed since last departure?
		if (SimTime >= next_time)
		{	// yes -> pass data item directly
			next_time = SimTime + delta;
			sucs[SucData]->rec(pd, shands[SucData]);
		}
		else	// no -> queue data item
		{	q_len = 1;
			q_first = q_last = pd;
			alarme( &std_evt, next_time - SimTime);
		}
	}
	else	//	queue has not been empty. queue data item.
	{	if (q_len >= q_max)	// buffer full
		{	// Mmh, something went wrong ...
			// We should have stopped in advance!
			errm1s("internal error: %s: fatal in shapctrl::rec()", name);
		}
		else			// queue data item
		{	++q_len;
			q_last->next = pd;
			q_last = pd;
		}
	}

	if (q_len >= q_max)
	{	//	buffer now full, stop the sender
		prec_state = StopSend;
		return StopSend;
	}
	else	return ContSend;
}

/*
*	Activation by the kernel: pass next data item.
*/
void	shapctrl::early(
	event	*)
{
	if ( --q_len == 0)
	{	// last data item in the queue
		next_time = SimTime + delta;
		sucs[SucData]->rec(q_first, shands[SucData]);
	}
	else	// there are more data items -> register again for sending the next
	{	data	*pd;
		pd = q_first;
		q_first = q_first->next;
		sucs[SucData]->rec(pd, shands[SucData]);
		alarme( &std_evt, delta);
	}

	if (q_len == q_start && prec_state == StopSend)
	{	// again place in the buffer -> wake up the data sender
		sucs[SucCtrl]->rec(new data, shands[SucCtrl]);
		prec_state = ContSend;
	}
}

/*
*	reset SimTime -> change next_time
*/
void	shapctrl::restim(void)
{
	//	attention, unsigned int!
	if (SimTime > next_time)
		next_time = 0;
	else	next_time -= SimTime;
}

/*
*	Export of variables
*/
int	shapctrl::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "QLen", &q_len);	// queue length
}
