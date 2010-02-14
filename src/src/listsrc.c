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
*
*	ListSrc src: N=3, DELTA=(1,2,5), VCI=1, OUT=line;
*
*	Sends N cells with the given IATs.
*/

#include "listsrc.h"

listsrc::listsrc()
{
	counter = 0;
	ncounter = counter;
	rep = false;
	tims = NULL;
}

listsrc::~listsrc()
{
  delete tims;
}

void	listsrc::early(
	event *)
{
	suc->rec(new cell(vci), shand);
	counter++;
	//	printf("ncounter=%d ntim=%d tick=%d\n", ncounter, ntim, tims[ncounter]);
	if (rep == false){
	  if ( (int) ++ncounter < ntim)
	    alarme( &std_evt, tims[ncounter]);
	} else {
	  alarme( &std_evt, tims[ncounter]);
	  if (++ncounter == ntim)
	    ncounter = 0;
	}
}

void listsrc::SetTims(int i, int delta)
{
	if(i == 0)		
		CHECK(tims = new tim_typ [ntim]);	

	tims[i] = delta;
}

