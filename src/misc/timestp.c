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
*	History:	July 2, 1997
*			- empty command() (returning always FALSE) deleted.
*			  This was done in order to inherit e.g. AliasInput()
*				Matthias Baumann
*
*************************************************************************/

/*
*	Class to set time stamps in data items:
*
*	TimeStamp ts: {VCI=1,} OUT=line;
*			// in case no VCI given, all time stamps are affected
*/

#include "in1out.h"

class	timestamp:	public	in1out	{
typedef	in1out	baseclass;

public:
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)

	dat_typ	inp_type;
};

CONSTRUCTOR(Timestamp, timestamp);


void	timestamp::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	if (test_word("VCI"))
	{	vci = read_int("VCI");
		skip(',');
	}
	else	vci = NILVCI;
	if (vci != NILVCI)
		inp_type = CellType;
	else	inp_type = DataType;

	// read additional parameters of derived classes
	addpars();

	output("OUT");
	stdinp();
}

//	Data arrived.
rec_typ	timestamp::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	typecheck(pd, inp_type);
	if (vci == NILVCI || vci == ((cell *) pd)->vci)
		pd->time = SimTime;

	return suc->rec(pd, shand);
}

