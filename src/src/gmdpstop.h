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
*	A GMDP source which can be controlled by the Start-Stop-protocol.
*
*	Additional input:	src->Start
*/
#ifndef GMDPSTOP_INCL
#define GMDPSTOP_INCL

#include "gmdp.h"

#define	TRIAL_MAX	(10000)		/* max # of trials to leave a zero bit rate state (GMDP) */

//tolua_begin
class	gmdpstop: public gmdpsrc 
{
  typedef gmdpsrc	baseclass;
public:
  gmdpstop(void);
  ~gmdpstop(void);
  void restim(void);
  rec_typ send_state;	// running or stopped?
  tim_typ next_time;	// intended time before stop
  //tolua_end
  rec_typ REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
  void early(event *);
}; //tolua_export

#endif
