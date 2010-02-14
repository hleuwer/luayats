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
*	Creation:		Jan 1998
*
*************************************************************************/

/*
*	Set the trace pointer of a data object.
*	In each incoming data object, the pointer traceOrigPtr is set to the name of the trace object.
*	Additionally, the trace sequence number is coppied into the data object and incremented.
*	All subsequent network objects will generate trace messages for the marked objects.
*
*	ONLY works if DATA_OBJECT_TRACE has been defined in src/kernel/defs.h,
*	otherwise no action is performed.
*
*	SetTrace tr: {VCI=1, | CONNID=3,} OUT=line;
*		// neither VCI nor CONNID given: all data objects are marked
*		// VCI given: cells expected, only cells on this VC are marked
*		// CONNID given: frames expected, only frames with this CONNID are marked
*/
#ifdef USELUA
#include "setTrace.h"
#else
#include "in1out.h"

class	setTrace:	public	in1out {
typedef	in1out	baseclass;

public:
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)

	dat_typ	inputType;
	int	id;
	int	seqNo;
};
#endif
CONSTRUCTOR(SetTrace, setTrace);
USERCLASS("SetTrace", SetTrace);

#ifdef USELUA
int setTrace::act(void)
{
   return 0;
}
#else
void	setTrace::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	if (test_word("VCI"))
	{	id = read_int("VCI");
		inputType = CellType;
		skip(',');
	}
	else if (test_word("CONNID"))
	{	id = read_int("CONNID");
		inputType = FrameType;
		skip(',');
	}
	else	inputType = DataType;

	seqNo = 0;

	stdinp();
	output("OUT");
}
#endif
rec_typ	setTrace::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*d,
	int	)
{

#ifdef	DATA_OBJECT_TRACE
	typecheck(d, inputType);

	switch (inputType) {
	case CellType:
		if (((cell *) d)->vci == id)
			d->traceOrigPtr = name;
		break;
	case FrameType:
		if (((frame *) d)->connID == id)
			d->traceOrigPtr = name;
		break;
	default:
		d->traceOrigPtr = name;
		break;
	}

	d->traceSeqNumber = ++seqNo;
#endif	// DATA_OBJECT_TRACE
	return suc->rec(d, shand);
}

