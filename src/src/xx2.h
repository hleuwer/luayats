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
#ifndef	_XX2_H_
#define	_XX2_H_

#include "bssrc.h"

class	xx2:	public	bssrc {
typedef	bssrc	baseclass;

public:
	void	addpars(void);
	void	early(event *);

	int	const_burst_len;	// greater than 0 if constant burst length

	int	jitter_flag;		// TRUE, if jitter

	int	burst_number;		// # of the current burst
	int	burst_len;		// length of the current burst
	int	sequence_number;	// cell # inside of the current burst
};

#endif	// _XX2_H_
