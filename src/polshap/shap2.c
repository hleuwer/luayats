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
*	Shaper2 shap: DELTA=10.2, BUFF=100, OUT=sink;
*			// DELTA can be double
*			// BUFF=0: "hard" spacing
*/

#include "shap2.h"

shap2::shap2()
{
}

shap2::~shap2()
{
}


//	Transfered to LUA init

/*
void	shap2::init(void)
{
	double	delta;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	delta = read_double("DELTA");
	if (delta < 1.0)
		syntax1s("%s: DELTA lower than 1.0: impossible", name);
	skip(',');
	q_max = read_int("BUFF");
	skip(',');

	// read additional parameters of derived classes
	addpars();

	output("OUT");
	stdinp();

	q_len = 0;
	next_time = 0;

	bucket = 0.0;
	delta_short = (int) delta;
	delta_long = delta_short + 1;
	splash = ((double) delta_long) - delta;
}
*/

/*
*	A cell has been arriving.
*/


rec_typ	shap2::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	// redundant: typecheck(pd, DataType);

	if (q_len == 0)
	{	//	buffer empty. Enough time elapsed since last departure?
		if (SimTime >= next_time)
		{	// yes -> pass cell directly
			if (SimTime > next_time)
			{	// a real pause has occured, reset the bucket
				bucket = 0.0;
			}
			if ((bucket += splash) >= 1.0)
			{	next_time = SimTime + delta_short;
				bucket -= 1.0;
			}
			else	next_time = SimTime + delta_long;
			
			suc->rec(pd, shand);
		}
		else	// no -> queue cell if possible
		{	if (q_max > 0)
			{	q_len = 1;
				q_first = q_last = pd;
				alarme( &std_evt, next_time - SimTime);
			}
			else	// no buffer space -> "hard" spacing
			{	if ( ++counter == 0)
					errm1s("%s: overflow of counter", name);
				delete pd;
			}
		}
	}
	else	//	queue has not been empty. queue cell or discard it.
	{	if (q_len >= q_max)	// buffer full
		{	if ( ++counter == 0)
				errm1s("%s: overflow of counter", name);
			delete pd;
		}
		else			// queue cell
		{	++q_len;
			q_last->next = pd;
			q_last = pd;
		}
	}

	return ContSend;
}

/*
*	Activation by the kernel: pass next cell.
*/
void	shap2::early(
	event	*)
{

	if ( --q_len == 0)
	{	// last cell in the queue
		if ((bucket += splash) >= 1.0)
		{	next_time = SimTime + delta_short;
			bucket -= 1.0;
		}
		else	next_time = SimTime + delta_long;
		suc->rec(q_first, shand);
	}
	else	// there are more cells -> register again for sending the next
	{	data	*pd;
		pd = q_first;
		q_first = (cell *) q_first->next;
		suc->rec(pd, shand);
		if ((bucket += splash) >= 1.0)
		{	alarme( &std_evt, delta_short);
			bucket -= 1.0;
		}
		else	alarme( &std_evt, delta_long);
		
	}
}

/*
*	reset SimTime -> change next_time
*/
void	shap2::restim(void)
{
	//	attention, unsigned int!
	if (SimTime > next_time)
		next_time = 0;
	else	next_time -= SimTime;
}

/*
*	Export of variables
*/
int	shap2::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "QLen", &q_len);	// queue length
}
