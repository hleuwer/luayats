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
*       Modified:               H. Leuwer
*************************************************************************/

/*
*	MMBP:
*	Burst / silence source with geometrically distributed sojourn times
*	cell distance within the burst is geom. distrib.
*
*	MMBPquelle src: EB=10, ES=10, ED=2, VCI=1, OUT=sink;
*				EB:	mean burst duration (slots)
*				ES:	mean silence duration (slots)
*				ED:	mean cell distance in the burst (slots)
*/

#include "mmbp.h"
mmbpsrc::mmbpsrc(void)
{
}

mmbpsrc::~mmbpsrc(void)
{
}

int mmbpsrc::act(void)
{
  dist_burst = get_geo1_handler(eb);
  dist_silence = get_geo1_handler(es);
  dist_ed = get_geo1_handler(ed);
  
  burst_left = 0;
  alarme( &std_evt, geo1_rand(dist_ed));
  return 0;
}

//
// Early event
//
void mmbpsrc::early(event *)
{
  int iat, left;
  tim_typ	tim;

  // send the cell
  suc->rec(new cell(vci), shand);

  // count cells
  if (++counter == 0)
    errm1s("%s: overflow of departs", name);

  // determine spacing to next cell
  iat = geo1_rand(dist_ed);
  if (iat > burst_left){
    //	burst finishes earlier
    //	go ahaed to end of silence period
    tim = burst_left + geo1_rand(dist_silence);
    //	search instant for sending next cell
    for (;;){
      iat = geo1_rand(dist_ed);
      left = geo1_rand(dist_burst);
      if (left >= iat){
	// an ON period with a cell has been found
	// (we always have a prob. of a "burst" without any cells)
	tim += iat;
	burst_left = left - iat;
	break;
      }
      // this "burst" does not contain any cells
      // -> go further ahead
      tim += left + geo1_rand(dist_silence);
    }
  } else {
    tim = iat;
    burst_left -= iat;
  }
  
  //	register again
  alarme( &std_evt, tim);

}

