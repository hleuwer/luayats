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
*	Bernoulli source
*
*	GEOquelle src: ED=4, VCI=1, OUT=sink;
*			// ED:	mean inter departure time (in slots)
*/

#include "geosrc.h"


/*
*	read create statement
*/

geosrc::geosrc()
{
}

geosrc::~geosrc()
{
}

	
/*
*	Activation by kernel - send cell and register again
*/
void	geosrc::early(event *)
{
	if ( ++counter == 0)
		errm1s("%s: overflow of departs", name);

	suc->rec(new cell(vci), shand);

	alarme( &std_evt, geo1_rand(dist));
}



int geosrc::act(void)
{
	dist = get_geo1_handler(ed);

	/* first registration for activation */
	alarme( &std_evt, geo1_rand(dist));

   return 0;

} // geosrc::act()

//	Transfered to LUA init
/*
void	geosrc::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	ed = read_double("ED");
	skip(',');
	vci = read_int("VCI");
	skip(',');

	// read additional parameters of derived classes
	addpars();

	output("OUT");

	dist = get_geo1_handler(ed);

//	 first registration for activation   
	alarme( &std_evt, geo1_rand(dist));
}
*/


