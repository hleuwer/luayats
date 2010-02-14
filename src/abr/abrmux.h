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
#ifndef	_MUX_ABR_H
#define	_MUX_ABR_H

#include "inxout.h"
#include "queue.h"

struct	ABRstruct	{
	double	ER;		// ER we calculated ourselfs
	double	forwER;		// incomming forward ER
	double	backwER;	// incomming backward ER
	double	CCR;
	double	MCR;
	int	active;		// TRUE if cell arrived during current AI
};

class	abrMux: public inxout	{
typedef	inxout	baseclass;
public:
	enum
	{	keyQueue = 1,
		keyLoadABR = 2,
		keyLoadCVBR = 3
	};
		abrMux(void):
			each_evt(this, keyQueue),
			ABR_load_evt(this, keyLoadABR),
			CVBR_load_evt(this, keyLoadCVBR) {}

	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
	void	late(event *);
	void	early(event *);
	char	*special(specmsg *, char *);
	int	export(exp_typ *);

	void	drop(cell *, unsigned *, char *);
	char	*establConn(ABRConReqMsg *, char *);
	char	*releaseConn(ABRConFinMsg *, char *);
	inline	void	compERICA(ABRstruct *);
	inline	void	compBinary(rmCell *);

	queue	q_CVBR;		// queue vor CBR / VBR cells
	uqueue	q_ABR;		// queue for ABR data cells
	uqueue	q_RMCI;		// queue for in-rate RM cells
	queue	q_RMCO;		// queue for out-of-rate RM cells

	int	abr_q_max;	// max. compound queue length of
				// q_ABR, q_RMCI, and q_RMCO
	int	abr_q_len;	// current length of the compound queue

	cell	*server;	// cell in the server, NULL if none

	event	each_evt;	// every timeslot: call late() to queue cells arrived
				// in the early phase
	event	ABR_load_evt;	// timer for measuring current ABR cell rate
	event	CVBR_load_evt;	// timer for measuring current non-ABR cell rate

	tim_typ	AI_tim;			// interval (time slots) for ABR rate measuremet
	tim_typ	CVBR_load_tim;		// interval (time slots) for non-ABR rate measuremet

	unsigned	cnt_load_abr;	// counter for ABR cell rate measurement
	unsigned	cnt_load_cvbr;	// counter for non-ABR cell rate measurement

	double	Z;			// over / underload indication for ERICA
	double	Zol;			// Z value in case no ABR bandwidth available
	double	CVBRRate;		// current non-ABR cell rate
	double	ABRRate;

	enum	{InpBRMC = -1};		// "normal" inputs: >= 0
	enum	{SucData = 0, SucBRMC = 1};	// for the outputs

	ABRstruct	**abr_table;	// management data for ABR
	int		max_vci;	// length of abr_table[] and lossXYZ[]

	int		ninp;		// number of inputs
	cell		**inp_buff;	// array to store incomming cells
	cell		**inp_ptr;	// current pointer to this array

	double	TargetUtil;		// target utilistaion of output link
	double	LinkRate;		// output link rate (cells per second)
	double	TargetRate;		// LinkRate * TargetUtil

	unsigned	*lossData;	// a loss counter per VC
	unsigned	*lossRMCI;
	unsigned	*lossRMCO;

	int	Nabr;			// number of ABR connections
	int	Nactive;		// # of currently active ABR connections

	int	useNactive;		// TRUE: compute fair share from Nactive
					// FALSE: compute it from Nabr
	int	binMode;		// TRUE: no ER calculations

	int	TBE;

	int	abr_q_hi;		// if crossed upwards, congestion indication
					// is turned on
	int	abr_q_lo;		// if crossed downwards, indication turned off
	int	abr_congested;		// TRUE: congestion indication
};

//	compute the new ER (according to ERICA)
inline	void	abrMux::compERICA(
	ABRstruct	*ptr)
{
	double	fairShare, VCShare;

	if (TargetRate > CVBRRate)
	{	if (useNactive)
		{	if (Nactive == 0)
				fairShare = TargetRate - CVBRRate;
			else	fairShare = (TargetRate - CVBRRate) / Nactive;
		}
		else	fairShare = (TargetRate - CVBRRate) / Nabr;
	}
	else	fairShare = 0.0;

	if (Z > 0.0)
	{	VCShare = ptr->CCR / Z;
		if (VCShare > fairShare)
			ptr->ER = VCShare;
		else	ptr->ER = fairShare;
	}
	else	ptr->ER = fairShare;

	if (ptr->ER < ptr->MCR)
		ptr->ER = ptr->MCR;
}

//	process RM cell according to binary feedback
inline	void	abrMux::compBinary(
	rmCell	*pc)
{
	if (abr_congested)
	{	if (abr_q_len <= abr_q_lo)
			abr_congested = FALSE;
		else	pc->CI = 1;
	}
	else if (abr_q_len > abr_q_hi)
	{	abr_congested = TRUE;
		pc->CI = 1;
	}
}

#endif	// _MUX_ABR_H
