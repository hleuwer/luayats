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
*	Creation:		July 6, 1997
*
*	History
*	Sept 30, 1997		CONNID added
*					Matthias Baumann
*	Jan 29, 1998		DIST added
*					Torsten Mueller, TUD
*
*************************************************************************/

/*
*	Data2Frame2 xyz: FLENDIST=DIST, {CONNID=10,} OUT=xx;
*			// each incoming data item is converted into a frame of lenght FLEN bytes
*			// frames carry the CONNID, default: 0
*/

#include "in1out.h"

class	dat2fram2: public in1out	{
typedef	in1out	baseclass;
public:

	void	init();
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)

	int	flen;
	int	connID;
	tim_typ	*table;			// Distribution-Table for framelengths
};

CONSTRUCTOR(Data2Frame2, dat2fram2);
USERCLASS("Data2Frame2", Data2Frame2);

void	dat2fram2::init()
{
	char		*s, *err;
	root		*obj;
	GetDistTabMsg	msg;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	s = read_suc("DIST");
	if ((obj = find_obj(s)) == NULL)
		syntax2s("%s: could not find object `%s'", name, s);
	if ((err = obj->special( &msg, name)) != NULL)
		syntax2s("could not get distribution table, reason returned by `%s':\n\t%s",
				s, err);
	table = msg.table;
	delete s;

	skip(',');

        if (test_word("CONNID"))
        {
                connID = read_int("CONNID");
                if(connID < 0)
                   syntax1s1d("%s: CONNID must be >= 0. Actual value: %d", name, connID);
                   
                skip(',');
        }
        else    connID = 0;
 
	output("OUT");
	stdinp();
}

rec_typ	dat2fram2::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	delete	pd;
	flen =  table[my_rand() % RAND_MODULO];
	if(flen <= 0)
	{
	   errm1s("%s: Distribution delivered Framelen <= 0", name);
	}
	
	return suc->rec(new frame(flen, connID), shand);
	
}
