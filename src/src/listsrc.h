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
#ifndef	_LISTSRC_H_
#define	_LISTSRC_H_

/*
*
*	ListSrc src: N=3, DELTA=(1,2,5), VCI=1, OUT=line;
*
*	Sends N cells with the given IATs.
*/

#include "in1out.h"

//tolua_begin
class	listsrc:	public	in1out	{
  typedef	in1out	baseclass;
public:
  listsrc();
  ~listsrc();
  
  int act(void) {alarme(&std_evt, tims[0]);return 0;}
  
  void	SetTims(int, int);
  int	ntim;
  bool    rep;
  int ncounter;
  //tolua_end
  tim_typ	*tims;
  void	early(event *);
};  //tolua_export

#endif	// _LISTSRC_H_
