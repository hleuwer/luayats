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
*************************************************************************/
#ifndef	_MMBP_H_
#define	_MMBP_H_

#include "in1out.h"
//tolua_begin
class	mmbpsrc: public	in1out {
typedef	in1out baseclass;

public:	
  mmbpsrc(void);
  ~mmbpsrc(void);
  void early(event *);
  int act(void);
  double eb; /* EB */
  double es; /* ES */
  double ed; /* ED */

  int dist_burst;		/* Code for transformation table EB */
  int dist_silence;		/* Code for transformation table ES */
  int dist_ed;
  
  int burst_left;		/* time remaining for the current burst state */
};
//tolua_end
#endif	// _MMBP_H_
