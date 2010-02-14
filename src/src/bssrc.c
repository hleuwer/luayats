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
*	Burst/Silence source with geometrically distributed phase durations
*
*	BSquelle src: EX=10, ES=50, DELTA=2, VCI=3, OUT=sink;
*				EX:	mean # of cells per ON period
*				ES:	mean duration of silence (in time slots)
*				DELTA:	cell spacing within a burst
*/

// changed 2004-10-15: included deterministic_ex and deterministic_es
// variable.
// If deterministic_es is set to 1, the length of silence is fix,
//   and not chosen from a distribution
// If deterministic_ex is set to 1, the number of cells within a burst
//    is fix and not chosen from a distribution


#include "bssrc.h"


/*
*	read create statement
*/

/*
*	send next cell and register again
*/

bssrc::bssrc()
{
}

bssrc::~bssrc()
{
}

void	bssrc::early(event *)
{
	tim_typ	t;
	//	send cell
	suc->rec(new cell(vci), shand);

	//	count cells
	if ( ++counter == 0)
		errm1s("%s: overflow of departs", name);

	//	when to send next cell?
	if ( --state > 0)
		// burst has not yet been completed
		t = delta;
	else	//	this was the last cell of the burst
	{
	   if(deterministic_es)
			t = (unsigned int) es + delta;
		else
		   t = geo1_rand(dist_silence) + delta;
		
		if(deterministic_ex)
			state = (unsigned int) ex;
		else
		   state = geo1_rand(dist_burst);

	}

	//	register again
	alarme( &std_evt, t);
}


int bssrc::act(void)
{
	int	pos, nxt;

	dist_burst = get_geo1_handler(ex);
	dist_silence = get_geo1_handler(es);

	deterministic_ex = 0;	// included 2004-10-15
	deterministic_es = 0;	// included 2004-10-15


	//	start phase choosen by chance
	pos = my_rand() % (int) (ex * delta + es);
	if (pos < ex * delta)
	{	//	start with burst
		state = pos / delta + 1;
		nxt = pos % delta;
	}
	else	//	start with silence
	{	state = 1;
		nxt = 1 + pos - (int) ex * delta;
	}
	alarme( &std_evt, nxt);		// first cell

   return 0;

} // bssrc::act()


void bssrc::SetDeterministicEx(int det)
{
   deterministic_ex = det;
}

void bssrc::SetDeterministicEs(int det)
{
   deterministic_es = det;
}


//	Transfered to LUA init

/*
void	bssrc::init(void)
{
	int	pos, nxt;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	ex = read_double("EX");
	skip(',');
	es = read_double("ES");
	skip(',');
	delta = read_int("DELTA");
	skip(',');
	vci = read_int("VCI");
	skip(',');

	// read additional parameters of derived classes
	addpars();

	output("OUT");

	dist_burst = get_geo1_handler(ex);
	dist_silence = get_geo1_handler(es);

	//	start phase choosen by chance
	pos = my_rand() % (int) (ex * delta + es);
	if (pos < ex * delta)
	{	//	start with burst
		state = pos / delta + 1;
		nxt = pos % delta;
	}
	else	//	start with silence
	{	state = 1;
		nxt = 1 + pos - (int) ex * delta;
	}
	alarme( &std_evt, nxt);		// first cell
}
*/
