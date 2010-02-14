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
*	Measurment device: (only cell count and Cell Transfer Delay)
*
*	Messung <name>: VCI=<vci>, MAXTIM=<maxtim>, OUT=<next node>
*	or:
*	Messung <name>: VCI=<vci>, MAXTIM=<maxtim>
*
*	Cells are counted.
*	Transfer times between 0 and (excluding) maxtim are collected.
*	In case of no OUT, all cells are terminated.
*
*	If VCI=-1, all cells are included in measurement
*
*	Commands:
*		<Name>->Count
*			return cell count
*		<Name>->ResCount
*			reset cell count
*		<Name>->Dist(tim)
*			return for CTD tim
*		<Name>->ResDist
*			reset all counters
*/

#include "meas.h"

// CONSTRUCTOR(Meas, meas);

meas::meas()
{
}

meas::~meas()
{
  if (dist)
    delete dist;
}
/*
*	Cell has arrived.
*	Perform measurement, if VCI o.k.
*	Pass cell, if OUT given
*/
rec_typ	meas::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	)
{
	tim_typ	dt;

	typecheck(pd, inp_type);

	if (vci == NILVCI || ((cell *) pd)->vci == vci)
	{	if ( ++counter == 0)
			errm1s("%s: overflow of arrivals", name);
		dt = SimTime - pd->time;
		if (dt < (unsigned) maxtim)
		{	if ( ++dist[dt] == 0)
				errm1s1d("%s: overflow of dist[%d]", name, dt);
		}
		else if ( ++greater_cnt == 0)
			errm1s("%s: overflow of greater_cnt", name);
	}

	if (suc != NULL)
		return suc->rec(pd, shand);
	else
	{	delete pd;
		return ContSend;
	}
}


/*
*	Command procedures
*/

#ifndef USELUA
int	meas::command(
	char	*s,
	tok_typ	*v)
{
	int	i;

	// commands Count und ResCount
	if (baseclass::command(s, v) == TRUE)
		return TRUE;

	v->tok = NILVAR;
	if (strcmp(s, "ResDist") == 0)
	{	greater_cnt = 0;
		for (i = 0; i < maxtim; ++i)
			dist[i] = 0;
	}
	else	return FALSE;

	return TRUE;
}
#endif
/*
*	export of variables
*/
int	meas::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intArray1(msg, "Dist", (int *) dist, maxtim, 0);	// IAT table
}
