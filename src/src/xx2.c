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
*	a variation of the BSquelle:
*	The cells within a burst are sent at a randomly choosen instant within one window
*
*	additional parameters:
*		... {CONST,} {NOJITTER,} OUT=line;
*			// CONST: constant rather than geometrically distrib. burst length
*			// NOJITTER: no jitter
*/

#include "xx2.h"

CONSTRUCTOR(Xx2src, xx2);

void	xx2::addpars(void)
{
	baseclass::addpars();

	if (test_word("CONST"))
	{	skip_word("CONST");
		skip(',');
		const_burst_len = (int) (ex + 0.5);
	}
	else	const_burst_len = 0;

	if (test_word("NOJITTER"))
	{	skip_word("NOJITTER");
		skip(',');
		jitter_flag = FALSE;
	}
	else	jitter_flag = TRUE;

	burst_number = 0;
	sequence_number = 0;
	burst_len = 0;		// this causes an error for the first burst
}
	
	
/*
*	only the mode of sending the cells is changed:
*/
void	xx2::early(event *)
{
	tim_typ	tim;

	//	send the cell
	suc->rec(new cellSeq(vci, burst_number, burst_len, sequence_number), shand);

	//	count cells
	if ( ++counter == 0)
		errm1s("%s: overflow of departs", name);

	//	when to send next cell?
	if ( --state > 0)
	{	//	burst has not yet finished
		tim = SimTime + delta;

		if (jitter_flag == TRUE)
		{	//	synchronize to the beginning of the last window
			tim -= tim % delta;
			//	chose a random phase
			tim += my_rand() % delta;
		}

		++sequence_number;
	}
	else	//	this was the last cell of the burst
	{	tim = SimTime + geo1_rand(dist_silence) + delta;

		if (const_burst_len > 0)
			state = const_burst_len;
		else	state = geo1_rand(dist_burst);

		if (jitter_flag == TRUE)
		{	//	synchronize to the beginning of the last window
			tim -= tim % delta;
			//	random phase
			tim += my_rand() % delta;
		}

		sequence_number = 0;
		burst_len = state;
		++burst_number;
	}

	//	register again
	alarme( &std_evt, tim - SimTime);

}

