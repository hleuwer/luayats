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
#ifndef	_CBR_H_
#define	_CBR_H_

#include "in1out.h"

//tolua_begin
class	cbrsrc:	public in1out {
typedef	in1out	baseclass;

public:
	cbrsrc();
	~cbrsrc();
	int act(void) {alarme(&std_evt, my_rand() % delta);return 0;}
	int	delta;
//tolua_end
	
	void	early(event *);
};  //tolua_export

#endif	// _CBR_H_
