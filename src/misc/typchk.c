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
*	Class	TypeCheck:
*	Ensure that items are sent in EARLY.
*	Ensure that only one data item is sent per time slot.
*	Check the type of an incomming data item.
*
*	TypeCheck chk: TYPE=Cell, {VCI=1,} OUT=meas;
*
*/

#include "in1out.h"

class	typchk:	public	in1out	{
typedef	in1out	baseclass;

public:
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
	void	restim(void);

	dat_typ	inp_type;

	tim_typ	last_tim;
	int	start_flag;
};

CONSTRUCTOR(Typecheck, typchk);

/*
*	read create statement
*/
void	typchk::init(void)
{
	char	*s;

	skip(CLASS);

	name = read_id(NULL);
	skip(':');

	s = read_word("TYPE");
	if ((inp_type = str2typ(s)) == UnknownType)
		syntax0("unknown data type");
	delete s;
	skip(',');

	if (test_word("VCI"))
	{	vci = read_int("VCI");
		skip(',');
	}
	else	vci = NILVCI;

	// read additional parameters of derived classes
	addpars();

	output("OUT");
	stdinp();

	last_tim = SimTime;
	start_flag = TRUE;	// to tell the first cell in SimTime 0 from others
}

//	Data item arrived. 
//	Check slot phase.
//	Ensure that this is the first in the current time slot.
//	Check the type.
//	If o.k., pass it.
rec_typ	typchk::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	if (TimeType != EARLY)
		errm1s1d("%s: data item received, NOT early phase, SimTime = %u", name, SimTime);

	if (SimTime == last_tim)
	{	if (SimTime != 0 || start_flag == FALSE)
			errm1s1d("%s: two data items received during time slot %u\n", name, SimTime);
		start_flag = FALSE;
	}
	last_tim = SimTime;

	if (vci != NILVCI)
	{	typecheck(pd, CellType);	// first ensure that vci included
		if (vci == ((cell *)pd)->vci)
			typecheck(pd, inp_type);
	}
	else	typecheck(pd, inp_type);

	return suc->rec(pd, shand);
}

void	typchk::restim(void)
{
	last_tim = 0;
	start_flag = TRUE;
}
