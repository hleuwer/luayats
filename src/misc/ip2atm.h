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
#ifndef	_IP2ATM_H_
#define	_IP2ATM_H_

#include "in1out.h"

class	ip2atm:	public in1out {
typedef	in1out	baseclass;

public:
	void	init(void);
	void	early(event *);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)

	void	start(void);

	int	b_len;		// burst length
	int	delta;		// cell spacing

	cell	*q_head;	// first element of the queue
	cell	*q_tail;	// last element

	int	send_cnt;	// number of cells already sent in the current burst
};

#endif	// _IP2ATM_H_
