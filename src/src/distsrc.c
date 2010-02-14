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

//
// A source generating cells according to a given distribution
//
// DistSrc src2: DIST=src1, VCI=1, OUT=meas2;
//               use the distribution from src1,which 
//               has to be defined in advance (Distribution object)
//

#include "distsrc.h"


/******************************************************************************************/

distsrc::distsrc()
{
}

distsrc::~distsrc()
{
}

void distsrc::early(event *)
{
  if ( ++counter == 0)
    errm1s("%s: overflow of counter", name);
  suc->rec(new cell(vci), shand);
  // next registration
  alarme( &std_evt, table[my_rand() % RAND_MODULO]);
}

int distsrc::act(void)
{
  // first registration
  alarme( &std_evt, table[my_rand() % RAND_MODULO]);
  return 0;
}
