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
#ifndef	_AAL5RECMULT_H
#define	_AAL5RECMULT_H

#include "in1out.h"
#include "queue.h"

class	aal5recMult:	public in1out
{
typedef	in1out	baseclass;
public:
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)

	int	command(char *, tok_typ *);
	int	export(exp_typ *);

	int	maxvci;			// range of VCIs

	// Statistic	
	int	*q_len;

	int	*cell_loss;		// detected cell loss
	int	*sdu_loss;		// detected loss of AAL SDUs
	int	*last_cell;		// sequence number of last cell
	aal5Cell	**first_cell;	// points to the first cell of the current frame
					// == NULL, if beginning of next frame is expected
	int	*last_sdu;		// sequence number of last valid SDU
	unsigned int	*sdu_cnt;		// counter of AAL SDUs
	unsigned int	*cell_cnt;		// counter of cells

	double	*delay_mean;		// mean delay of AAL SDUs
	int     doCopyCid;		// TRUE: copy VCI of cells into connection ID of packets
	int     doCopyClp;		// TRUE: copy CLP bit of last cell (pt=1) into clp bit of packets
};

#endif	// _AAL5RECMULT_H
