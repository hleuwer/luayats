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
#ifndef	_AAL5REC_H
#define	_AAL5REC_H

#include "in1out.h"
#include "queue.h"

class	aal5rec:	public in1out
{
typedef	in1out	baseclass;
public:
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)

	int	command(char *, tok_typ *);
	int	export(exp_typ *);

	uqueue	q;			// queue to store cells until frame extraction

	// Statistic	
	int	cell_loss;		// detected cell loss
	int	sdu_loss;		// detected loss of AAL SDUs
	int	last_cell;		// sequence number of last cell
	int	last_sdu;		// sequence number of last valid SDU
	size_t	sdu_cnt;		// counter of AAL SDUs

	tim_typ	delay;			// delay of AAL SDUs
	double	delay_mean;		// mean delay of AAL SDUs
};

#endif	// _AAL5REC_H
