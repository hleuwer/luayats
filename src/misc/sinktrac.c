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
*	Creation:		March 1997
*
*************************************************************************/

/*
*	A sink:	- count incomming cells
*		- write IATs to a trace file (ASCII)
*
*	SinkTrace sink: FILE="trace.dat";
*/

#include "in1out.h"

class	sinktr:	public	in1out {
typedef	in1out	baseclass;

public:
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)

	char	*filnam;
	FILE	*filfp;

	tim_typ	lastArrival;
};


CONSTRUCTOR(SinkTrace, sinktr);

void	sinktr::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	filnam = read_string("FILE");
	if ((filfp = fopen(filnam, "w")) == NULL)
		syntax2s("%s: could not open file `%s' for writing", name, filnam);

	stdinp();
	lastArrival = 0;
}

rec_typ	sinktr::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*d,
	int	)
{
	typecheck(d, DataType);

	if ( ++counter == 0)
		errm1s("%s: overflow of arrivals", name);
	delete d;

	if (fprintf(filfp, "%u\n", SimTime - lastArrival) < 1)
		errm2s("%s: error writing file `%s'", name, filnam);
	lastArrival = SimTime;

	return ContSend;
}

