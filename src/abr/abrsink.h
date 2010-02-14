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
#ifndef	_ABR_SINK_H
#define	_ABR_SINK_H

#include "inxout.h"
#include "queue.h"

class	abrSink: public inxout {
typedef	inxout	baseclass;
public:
		abrSink(void): AI_evt(this, 1) {}
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
	void	early(event *);
	void	late(event *);
	int	export(exp_typ *);
	char	*special(specmsg *, char *);

	tim_typ	AI_tim;		// measuremnt interval for output rate
	int	outp_load_cnt;	// counter for this purpose
	event	AI_evt;		// event for this purpose

	double	LinkRate;	// cells per sec.
	double	TargetUtil;	// target urtil. of output link
	double	OutputRate;	// current output rate

	queue	q;		// ouptut queue

	int	TargetBuff;	// target output queue occupation
	int	q_high;		// queue ocuup. at which turn on CI
	int	q_low;		// queue ocuup. at which turn off CI

	rec_typ	my_state;	// may we send or are we stopped?
	int	congested;

	unsigned	lost;	// loss counter
	unsigned	cntData;	// arrivals
	unsigned	cntRMCI;
	unsigned	cntRMCO;

	tim_typ	last_cell;	// when did we send the last cell?

	double	MCR;		// cells per sec.
	double	PCR;		// cells per sec.

	int	connected;	// TRUE: connection established
	int	vci_brmc;

	enum	{InpStart = -1};
	enum	{SucData = 0, SucBRMC = 1};
};

#endif	// _ABR_SINK_H
