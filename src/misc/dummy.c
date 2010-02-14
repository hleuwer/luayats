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
*	Creation:		July 1, 1997
*
*************************************************************************/

/*
*	Dummy object, just for connection of two other objects. This is
*	sometimes needed to handle object names in larger model networks.
*
*	DummyObj dmy: OUT=xyz;	// incomming data items are only forwarded.
*				// NO OTHER OPERATION. (question of speed)
*/

#include "in1out.h"
#include "dummy.h"

dummyObj::dummyObj()
{
}
dummyObj::~dummyObj()
{
}



/*
*	data item received. Just forward it to next object.
*/
rec_typ	dummyObj::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	)
{
	return suc->rec(pd, shand);
}
