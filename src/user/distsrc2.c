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
*	Changes:		Torsten Mueller(3/98): Start and Endtime added
*
*************************************************************************/

/*
*	A source generating cells according to a given distribution
*
*	DistSrc src2: DIST=src1, VCI=1, {STARTTIME=100,} {ENDTIME=100000, }OUT=meas2;
*			// use the distribution from src1.
*			// src1 has to be defined in advance (Distribution object)
*			// Start to send at STARTTIME and STOP at ENDTIME
*/

#include "in1out.h"

class	distsrc2:	public in1out	{
typedef	in1out	baseclass;

public:
	void	init(void);
	void	early(event *);

	tim_typ	*table;
	tim_typ end_time;
};

CONSTRUCTOR(Distsrc2, distsrc2);
USERCLASS("DistSrc2", Distsrc2);

/******************************************************************************************/
/*
*	read create statement
*/
void	distsrc2::init(void)
{
	char		*s, *err;
	root		*obj;
	GetDistTabMsg	msg;
	tim_typ start_time = 0;
	
	end_time  = 0-1;
	
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
	vci = read_int("VCI");
	skip(',');

	if( test_word("STARTTIME"))
	{
	   int start_time_in;
	   
	   start_time_in = read_int("STARTTIME");
	   if( start_time_in < -1)
	      syntax1s("%s: StartTime must be >= 0 or -1 for start random start", name);
	   skip(',');
	   
	   if(start_time_in > 0)
	      start_time = start_time_in;
	   else if(start_time_in == 0)
	      start_time = 1;
	   else
	      start_time = 0;	// random start (in was -1)
	}

	if( test_word("ENDTIME"))
	{
	   int end_time_in;
	   end_time_in = read_int("ENDTIME");
	   if(end_time_in <= 0)
	      syntax1s("%s: EndTime must be > 0", name);
	   skip(',');
	   end_time = end_time_in;

	}

	// read additional parameters of derived classes
	addpars();

	output("OUT");

	// first registration
	
	if(start_time == 0)
	   start_time = table[my_rand() % RAND_MODULO];	// random registration

	if(start_time <= 0)
	   start_time = 1;
	
	if(start_time <= end_time)
	   alarme( &std_evt, start_time);	// first registration
	   
}

/******************************************************************************************/
//
//	Send next Cell
//
void	distsrc2::early(
	event	*)
{
        tim_typ evt_time;
        
	if ( ++counter == 0)
		errm1s("%s: overflow of counter", name);
	suc->rec(new cell(vci), shand);

        evt_time = table[my_rand() % RAND_MODULO];
        if(evt_time <= 0)
           evt_time = 1;

   	if(SimTime+evt_time <= end_time)
           alarme( &std_evt, evt_time);	// next registration

}
