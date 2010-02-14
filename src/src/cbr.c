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
*	CBR source:
*
*	CBRquelle src: DELTA=10, VCI=1, OUT=sink;
*			// DELTA: cell spacing
*
*	25.04.96:	additional command src->Restart
*/

#include "cbr.h"

cbrsrc::cbrsrc()
{
}

cbrsrc::~cbrsrc()
{
}

void	cbrsrc::early(event *)
{
	if ( ++counter == 0)
		errm1s("%s: overflow of departs", name);

	suc->rec(new cell(vci), shand);

	alarme( &std_evt, delta);
}
