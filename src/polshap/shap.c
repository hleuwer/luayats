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
*	Peak rate shaper with buffer:
*
*	Shaper shap: DELTA=10, BUFF=100, OUT=sink;
*			// BUFF=0: "hard" spacing
*/
/*
*	March 12, 1997:
*		Bug fix in early(): setting next_time had been forgotten when queue is emptied
*		(this bug was introduced when shap has been changed to usage of class queue,
*		the old version was correct!)
*			Matthias Baumann
*/

#include "shap.h"

CONSTRUCTOR(Shaper, shap);

void	shap::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	delta = read_int("DELTA");
	skip(',');
	q.setmax(read_int("BUFF"));
	skip(',');

	// read additional parameters of derived classes
	addpars();

	output("OUT");
	stdinp();

	next_time = 0;
}

/*
*	A cell has been arriving.
*/
rec_typ	shap::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	// redundant: typecheck(pd, DataType);

	if (q.isEmpty())
	{	//	buffer empty. Enough time elapsed since last departure?
		if (SimTime >= next_time)
		{	// yes -> pass cell directly
			next_time = SimTime + delta;
			suc->rec(pd, shand);
		}
		else	// no -> queue cell if possible
		{	if (q.enqueue(pd))
				alarme( &std_evt, next_time - SimTime);
			else	// no buffer space -> "hard" spacing
			{	if ( ++counter == 0)
					errm1s("%s: overflow of counter", name);
				delete pd;
			}
		}
	}
	else	//	queue has not been empty. queue cell or discard it.
	{	if ( !q.enqueue(pd))	// buffer full
		{	if ( ++counter == 0)
				errm1s("%s: overflow of counter", name);
			delete pd;
		}
	}

	return ContSend;
}

/*
*	Activation by the kernel: pass next cell.
*/
void	shap::early(
	event	*)
{
	suc->rec(q.dequeue(), shand);
	if ( !q.isEmpty())
		alarme( &std_evt, delta);
	else	next_time = SimTime + delta;
}

/*
*	reset SimTime -> change next_time
*/
void	shap::restim(void)
{
	//	attention, unsigned int!
	if (SimTime > next_time)
		next_time = 0;
	else	next_time -= SimTime;
}

/*
*	Export of variables
*/
int	shap::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "QLen", &q.q_len);	// queue length
}
