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
#ifndef	_ABR_SRC_H
#define	_ABR_SRC_H

#include "inxout.h"
#include "queue.h"

class	abrSrc:	public	inxout	{

typedef	inxout	baseclass;

public:
	enum
	{	ACR_timer = 0,
		Trm_timer = 1,
		TCR_timer = 2
	};

		abrSrc(void):
			ACR_evt(this, ACR_timer),
			Trm_evt(this, Trm_timer),
			TCR_evt(this, TCR_timer) {}
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
	void	early(event *);
	int	export(exp_typ *);

	void	establConn(cell *);
	void	releaseConn(data *);
	void	sendInRate(void);
	int	send_RMCI(void);
	int	compACR_tim(void);
	inline	void	recalc_IAT(void);
	inline	void	fill_RMC( rmCell *, int);

	double	ACR;		// cells per sec.
	double	PCR;		// cells per sec.
	double	MCR;		// cells per sec.
	double	ICR;		// cells per sec.
	double	TCR;		// cells per sec.
	double	Trm;		// miliseconds (ATM forum spec)
	double	LinkRate;	// output link rate in cells per sec.
	double	ADTF;		// in seconds
	double	FRTT;		// in micro seconds
	double	CDF;
	double	RIF;
	double	RDF;

	int	CRM;
	int	Nrm;
	int	Mrm;

	int	IR_count;	// number of in-rate cells since last in-rate RM cell
	int	unack;		// number of forward in-rate RM cells cince
				// last backward RM cell received from destination
	rec_typ	my_state;	// are we allowed to send in-rate? (ACR > TCR?)
				// yes: ContSend, no: StopSend
	rec_typ	prec_state;	// state of the preceeding object

	event	ACR_evt;	// timer for next in-rate cell (RM or data)
	event	Trm_evt;	// Trm timer expired (for in-rate RM cells)
	event	TCR_evt;	// 1 / TCR expired (out-rate sending)

	int	ACR_active;	// activity flags: TRUE if timer is active
	int	Trm_active;

	tim_typ	last_IR;	// time of last in-rate cell (RM and data)
	tim_typ	last_RMCI;	// time of last in-rate RM cell
	tim_typ	last_Cell;	// time of last cell (all kinds)

	tim_typ	Trm_tim;	// TRM in time slots
	tim_typ	TCR_tim;	// 1 / TCR in time slots
	tim_typ	ACR_tim;	// current 1 / ACR in time slots
	tim_typ	ADTF_tim;	// 1 / ADTF in time slots

	tim_typ	ACR_tim_hi;	// values to manage non-integer cell spacing for ACR
	tim_typ	ACR_tim_lo;
	double	ACR_splash;
	double	ACR_bucket;

	queue	q;		// input queue
	int	q_start;	// queue length where to wake up stopped data sender

	int	connected;	// TRUE: connection established
	int	autoconn;	// TRUE: establish connection automatically with first cell
	int	first_Cell;	// TRUE: this is the first in-rate cell

	char	**route;	// following routing members
	int	route_len;	// number of members

	unsigned	cntRMCO;	// counter
	unsigned	cntRMCI;
	unsigned	cntData;

	enum
	{	InpData = 0,
		InpRMC = 1,
		InpConnEst = 2,
		InpConnRel = 3
	};
	enum	{SucData = 0, SucCtrl = 1};
};

//	set current ACR_tim (changes sometimes in case of non-integer ACR)
inline	void	abrSrc::recalc_IAT(void)
{
	if ((ACR_bucket += ACR_splash) >= 1.0)
	{	ACR_tim = ACR_tim_lo;
		ACR_bucket -= 1.0;
	}
	else	ACR_tim = ACR_tim_hi;
}

//	prepare an RM cell
inline	void	abrSrc::fill_RMC(
	rmCell	*pc,
	int	clp)
{
	pc->CCR = ACR;
	pc->ER = PCR;
	pc->CI = 0;
	pc->NI = 0;
	pc->DIR = 0;
	pc->BN = 0;
	pc->ident = this;
	pc->CLP = clp;
}

#endif	// _ABR_SRC_H
