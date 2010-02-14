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
#ifndef	_MUX_ASYNC_AF_H_
#define	_MUX_ASYNC_AF_H_

#include "muxBase.h"

class	muxAsyncAF: public muxBase {
typedef	muxBase	baseclass;
public:
		muxAsyncAF()
		{	serviceCompleted = (tim_typ) -1;
		}
	void	early(event *);
	void	late(event *);
	void	restim();

	tim_typ	serviceCompleted;

	inline	tim_typ	nextServiceTime();
	void	addpars();	// read a non-integer service time (if present)

	double	doubleServiceTime;
	tim_typ	delta_short;
	tim_typ	delta_long;
	double  bucket;
	double  splash;

};

inline	tim_typ	muxAsyncAF::nextServiceTime()
{
	if (serviceTime != 0)	// integer service time
		return serviceTime;

	if ((bucket += splash) >= 1.0)
	{	bucket -= 1.0;
		return delta_short;
	}
	return delta_long;
}

#endif	// _MUX_ASYNC_AF_H_

