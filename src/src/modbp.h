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
#ifndef	_MODBP_H_
#define	_MODBP_H_

#include "in1out.h"


class	modBP:	public	in1out {
typedef	in1out	baseclass;

public:
	void	init(void);
	void	early(event *);

	int	n_stat;			/* # of states */
	double	*ed;			/* mean cell distances: in case of 0 bit rate is zero,
					   ex then gives the phase duration */
	tim_typ	**edtables;		// transformation tables for cell distancies.
	tim_typ	**tables;		// transformation tables for sojourn time distributions
	int	state;			/* current state */
	int	time_left;		// the current state still lasts for this number of time slots
	int	**trafo;		/* transition probabilities between states, already stored
					   as PDFs */
};

#endif	// _MODBP_H_
