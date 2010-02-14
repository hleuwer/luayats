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
**************************************************************************
*
*	Module authors:		Andreas Teresiak, TUD (diploma thesis)
*				Matthias Baumann, TUD
*	Creation:		Sept 1996
*
*************************************************************************/

/*
*	History:
*	17/10/96:	+ fair share calculation based on number of really active sources,
*			  measured over AI (simple count, no exponential average)
*			  number of active connections: Nactive, recomputed with AI timer expiry
*			- this option is turned ON with the flag DYNFAIRSHARE,
*			  default: fair share based on number of established connections
*			- export of NActive: number of currently active connections
*			+ option BINMODE included: no ER rate calculations
*			- in BINMODE, ERICA only is used to compute ICR at connection setup
*			+ optional parameter ZOL introduced: Z value in case no ABR bandwidth available
*				Matthias Baumann
*
*	July 1, 1997	- Bug fix: computation of ICR in establConn() (see there)
*			  Labeled with: #BuggyInitICR
*				Matthias Baumann
*/

/*
*	ABR multiplexer with ERICA and binary feedback (CI bit) mechanism
*
*	AbrMux mux:	NINP=5,			// number of inputs
*			MAXVCI=10,		// largest VCI
*			BUFFCBR=200,		// buffer size for non-ABR
*			BUFFABR=10000,		// buffer size for ABR, all:
*						// data, in- and out-of-rate RM
*			{BUFFRMCO=1000,}	// threshold for out-rate RM cells
*						// default: BUFFABR
*			{HI_THRESH=8000,}	// congestion indication turned on
*						// default: BUFFABR
*			{LO_THRESH=6000,}	// congestion indication turned off
*						// default: HI_THRESH
*			TBE=2000,		// TBE returned during connection
*						// negotiation
*			AI=30,			// measurment interval (slots)
*						// for ABR input rate
*			{CBRI=50,}		// measurement interval for non-ABR
*						// default: AI
*			{ZOL=12.5,}		// Z value in case no ABR bandwidth available
*						// default: 1000
*			{LINKCR=100000.2,}	// link capacity in cells per second
*						// default: 353207.55 (149.76 Mbit / s)
*			TARGUTIL=0.9,		// target utilisitation of output link
*			{DYNFAIRSHARE,}		// if given: fair share is based on # of active
*						// rather than # of established connections
*			{BINMODE,}		// if given: no ER feedback is provided -> pure CI bit
*			OUTBRMC=bla,		// where to send backward RM cells
*			OUTDATA=blo;		// where to send forward cells (mux output)
*
*	The ABR buffer is shared between all kinds of ABR cells. Exception: out-of-
*	rate RM cells only can occupy BUFFRMCO buffer places.
*	The server priority (when looking for waiting cells) is as follows:
*		1. non-ABR cells
*		2. in-rate RM cells
*		3. out-rate RM cells
*		4. ABR data cells.
*	Therefore, all RM cells turn over ABR data cells.
*
*	The ERICA and the binary feedback mechanism are performed with every forward
*	and backward RM cell. (see compERICA() und compBinary())
*
*	ERICA with CBR / VBR background:
*	For computing fair and VC share, the cell rate currently used by non-ABR traffic
*	is substracted from the target capacity (link capacity times target utilisation). 
*	The measurement period for non-ABR traffic is CBRI slots.
*	See compErica(), and late() (branch keyLoadABR) for computing the Z value.
*
*	For determining ICR at connection establishment, also the ERICA algorithm is used.
*	Instead of the new connection's CCR , its MCR is used in this case. (see establConn())
*/

#include "abrmux.h"

CONSTRUCTOR(AbrMux, abrMux);

/*
*	read the definition statement
*/
void	abrMux::init(void)
{
	int	i;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	ninp = read_int("NINP");
	CHECK(inp_buff = new cell * [ninp]);
	inp_ptr = inp_buff;
	skip(',');

	max_vci = read_int("MAXVCI") + 1;
	CHECK(abr_table = new ABRstruct * [max_vci]);
	for (i = 0; i < max_vci; ++i)
		abr_table[i] = NULL;
	skip(',');

	q_CVBR.setmax(read_int("BUFFCBR"));
	if (q_CVBR.getmax() < 1)
		syntax0("invalid BUFFCBR");
	skip(',');

	abr_q_max = read_int("BUFFABR");
	if (abr_q_max < 1)
		syntax0("invalid BUFFABR");
	skip(',');
	abr_q_len = 0;

	if (test_word("BUFFRMCO"))
	{	q_RMCO.setmax(read_int("BUFFRMCO"));
		if (q_RMCO.getmax() < 1)
			syntax0("invalid BUFFRMCO");
		skip(',');
	}
	else	q_RMCO.setmax(abr_q_max);

	if (test_word("HI_THRESH"))
	{	abr_q_hi = read_int("HI_THRESH");
		if (abr_q_hi < 1)
			syntax0("invalid HI_THRESH");
		skip(',');
	}
	else	abr_q_hi = abr_q_max;
	if (test_word("LO_THRESH"))
	{	abr_q_lo = read_int("LO_THRESH");
		if (abr_q_lo > abr_q_hi)
			syntax0("invalid LO_THRESH");
		skip(',');
	}
	else	abr_q_lo = abr_q_hi;
	abr_congested = FALSE;

	TBE = read_int("TBE");
	if (TBE < 1)
		syntax0("invalid TBE");
	skip(',');

	AI_tim = read_int("AI");
	if (AI_tim < 1)
		syntax0("invalid AI");
	skip(',');
	cnt_load_abr = 0;
	Z = 0.0;
	ABRRate = 0.0;

	if (test_word("CBRI"))
	{	CVBR_load_tim = read_int("CBRI");
		if (CVBR_load_tim < 1)
			syntax0("invalid CBRI");
		skip(',');
	}
	else	CVBR_load_tim = AI_tim;
	cnt_load_cvbr = 0;
	CVBRRate = 0.0;

	if (test_word("ZOL"))
	{	Zol = read_double("ZOL");
		if (Zol <= 1.0)
			syntax0("invalid ZOL");
		skip(',');
	}
	else	Zol = 1e3;

	if (test_word("LINKCR"))
	{	LinkRate = read_double("LINKCR");
		if (LinkRate <= 0.0)
			syntax0("invalid LINKCR");
		skip(',');
	}
	else	LinkRate = 353207.55;	// 149.76 Mbit / s

	TargetUtil = read_double("TARGUTIL");
	if (TargetUtil <= 0.0 || TargetUtil > 1.0)
		syntax0("invalid TARGUTIL");
	TargetRate = LinkRate * TargetUtil;
	skip(',');

	CHECK(lossData = new unsigned [max_vci]);
	CHECK(lossRMCI = new unsigned [max_vci]);
	CHECK(lossRMCO = new unsigned [max_vci]);
	for (i = 0; i < max_vci; ++i)
		lossData[i] = lossRMCI[i] = lossRMCO[i] = 0;

	if (test_word("DYNFAIRSHARE"))
	{	skip_word("DYNFAIRSHARE");
		skip(',');
		useNactive = TRUE;
	}
	else	useNactive = FALSE;

	if (test_word("BINMODE"))
	{	skip_word("BINMODE");
		skip(',');
		binMode = TRUE;
	}
	else	binMode = FALSE;

	output("OUTBRMC", SucBRMC);
	skip(',');
	output("OUTDATA", SucData);

	input("BRMC", InpBRMC);
	inputs("I", ninp, -1);

	Nabr = 0;
	Nactive = 0;

	eachl( &each_evt);
	alarml( &ABR_load_evt, AI_tim);
	alarml( &CVBR_load_evt, CVBR_load_tim);
}


/*
*	a cell has arrived
*	forward:	store cell in the input buffer
*			performe counting for load measurment
*	backward:	process RM cell, pass cell directly
*/
rec_typ	abrMux::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	i)
{
	int	aVci;

	if (i == InpBRMC)
	{	// backward RM cell input
		rmCell	*pc;
		ABRstruct	*ptr;

		typecheck_i(pd, RMCellType, i);
 		pc = (rmCell *) pd;

		if (pc->DIR != 1)
			errm1s("%s: forward RM cell received on backward input", name);
		if ((aVci = pc->vci) < 0 || aVci >= max_vci)
			errm1s1d("%s: VCI of backward RM cell out of range, VCI = %d", name, aVci);
		if ((ptr = abr_table[aVci]) == NULL)
			errm1s1d("%s: backward RM cell with VCI = %d received: "
					"no connection established", name, aVci);

		//	compute latest ER
		if (binMode == FALSE)
		{	compERICA(ptr);
			ptr->backwER = pc->ER;
			if (ptr->ER < pc->ER)
				pc->ER = ptr->ER;
		}

		//	perform binary feedback
		compBinary(pc);

		// printf("%s: %d: ER = %e, Z = %e\n", name, SimTime, pc->ER, Z);
		return sucs[SucBRMC]->rec(pc, shands[SucBRMC]);
	}

	typecheck_i(pd, CellType, i);
	cell	*pc = (cell *) pd;
	aVci = pc->vci;

	//	perform load measurement
	if (aVci >= 0 && aVci < max_vci && abr_table[aVci] != NULL)
	{	// an ABR cell
		++cnt_load_abr;
		abr_table[aVci]->active = TRUE;		// for calculation of Nactive

		if (typequery(pc, RMCellType))
		{	abr_table[aVci]->forwER = ((rmCell *)pc)->ER;
			abr_table[aVci]->CCR = ((rmCell *)pc)->CCR;
		}
	}
	else
	{	// a non-ABR cell
		++cnt_load_cvbr;
	}

	// store cell
	*inp_ptr++ = pc;

	return ContSend;
}

/*
*	a timer has expired
*/
void	abrMux::late(
	event	*evt)
{
	int	aVci;
	cell	*pc;
	int	n;
	cell	**p;

	//	what to do?
	switch (evt->key) {
	case keyLoadABR:	// ABR load measurement
		// Z = (ABR load) / (available capacity)
		if (CVBRRate >= TargetRate)
//			Z = 1e3;	// nothing left for ABR
			Z = Zol;	// nothing left for ABR
		else	Z = (cnt_load_abr / (double) AI_tim)
				/ ((TargetRate - CVBRRate) / LinkRate);
		ABRRate = LinkRate * cnt_load_abr / (double) AI_tim;
		cnt_load_abr = 0;

		if (useNactive)
		{	ABRstruct	**pt;
			Nactive = 0;
			for (pt = abr_table + max_vci - 1; pt >= abr_table; --pt)
				if ( *pt != NULL && (*pt)->active)
				{	++Nactive;
					(*pt)->active = FALSE;
				}
		}

		alarml( &ABR_load_evt, AI_tim);
		return;
	case keyLoadCVBR:	// non-ABR load measurement
		CVBRRate = LinkRate * cnt_load_cvbr / (double) CVBR_load_tim;
		cnt_load_cvbr = 0;
		alarml( &CVBR_load_evt, CVBR_load_tim);
		return;
	case keyQueue:		// queue arrived cells
		break;	// continuation below
	default:errm1s("internal error: %s: abrMux::late(): invalid event key", name);
	}

	//	process all arrivals in random order
	n = inp_ptr - inp_buff;
	while (n != 0)
	{	// chose a cell
		if (n > 1)
			p = inp_buff + (my_rand() % n);
		else	p = inp_buff;
		pc = *p;

		if ((aVci = pc->vci) < 0 || aVci >= max_vci || abr_table[aVci] == NULL)
		{	// a CBR / VBR cell
			if (q_CVBR.enqueue(pc) == FALSE)	// queue was full -> loss
				drop(pc, lossData, "LossData");
			
		}
		else if (typequery(pc, RMCellType) == FALSE)
		{	// an ABR data cell
			if (abr_q_len >= abr_q_max)
				drop(pc, lossData, "LossData");
			else
			{	q_ABR.enqueue(pc);
				++abr_q_len;
			}
		}
		else if (((rmCell *)pc)->CLP == 0)
		{	// an in-rate RM cell
			if (abr_q_len >= abr_q_max)
				drop(pc, lossRMCI, "LossRMCI");
			else
			{	q_RMCI.enqueue(pc);
				++abr_q_len;
			}
		}
		else	// an out-of-rate RM cell
		{	if (abr_q_len >= abr_q_max || q_RMCO.enqueue(pc) == FALSE)
				drop(pc, lossRMCO, "LossRMCO");
			else	++abr_q_len;
		}
			
		*p = inp_buff[--n];
	}

	inp_ptr = inp_buff;

	//	do we have cells in the queues?
	//	priority of serving:
	//		1. CBR / VBR
	//		2. in-rate RM cells
	//		3. out-of-rate RM cells
	//		4. ABR data cells
	if ((server = (cell *) q_CVBR.dequeue()) == NULL)
	{	if ((server = (cell *) q_RMCI.dequeue()) != NULL ||
			(server = (cell *) q_RMCO.dequeue()) != NULL ||
			(server = (cell *) q_ABR.dequeue()) != NULL)
		{	--abr_q_len;
			alarme( &std_evt, 1);
		}
	}
	else	alarme( &std_evt, 1);

}	// end of late()

/*
*	send a cell.
*	If it is a RM cell, process according to ERICA and binary method
*/
void	abrMux::early(
	event	*)
{
	int	aVci;

	aVci = server->vci;
	if (aVci < 0 || aVci >= max_vci || abr_table[aVci] == NULL)
	{	// non-ABR
		sucs[SucData]->rec(server, shands[SucData]);
		return;
	}

	if (typequery(server, RMCellType))	// RM cell
	{	rmCell	*pc;
		ABRstruct	*ptr;

 		pc = (rmCell *) server;
		if (pc->DIR != 0)
			errm1s1d("%s: backward RM cell received on forward input, VCI = %d", name, aVci);

		ptr = abr_table[aVci];

		// compute ER
		if (binMode == FALSE)
		{	compERICA(ptr);
			ptr->forwER = pc->ER;
			if (pc->ER > ptr->ER)
				pc->ER = ptr->ER;
		}

		// perform binary feedback
		compBinary(pc);
	}

	// send the ABR cell
	sucs[SucData]->rec(server, shands[SucData]);
}

/*
*	drop a cell, count the loss
*/
void	abrMux::drop(
	cell		*pc,
	unsigned	*lost,
	char		*s)
{
	int	aVci = pc->vci;

	if (aVci >= 0 && aVci < max_vci)
	{	if ( ++lost[aVci] == 0)
			errm2s1d("%s: overflow of %s[%d]", name, s, aVci);
	}

	delete	pc;
}

/*
*	connection establishment and release
*/
char    *abrMux::special(
	specmsg	*msg,
	char	*caller)
{
	switch (msg->type) {
	case ABRConReqType:
		return establConn((ABRConReqMsg *) msg, caller);
	case ABRConFinType:
		return releaseConn((ABRConFinMsg *) msg, caller);
	default:return "wrong type of special message";
	}
}

/*
*	establisch a connection
*/
char	*abrMux::establConn(
	ABRConReqMsg	*pmsg,
	char		*)
{
	char	*err;
	char	*abr_suc_nam;
	root	*r;
	int	aVci;
	ABRstruct	*ptr;
	double	rate;
	int	i;

	if (pmsg->akt_pointer > pmsg->numb_RoutMemb)
		errm1s("internal error: %s : routing-pointer > routing-member", name);
	if (pmsg->akt_pointer == pmsg->numb_RoutMemb)
		errm1s("%s: need a successor for ABR routing", name);

	aVci = pmsg->VCI;
	if (aVci < 0 || aVci >= max_vci)
		return "VCI out of range";
	if (abr_table[aVci] != NULL)
		return "already a connection established";

	CHECK(abr_table[aVci] = new ABRstruct);
	ptr = abr_table[aVci];

	// first check whether MCR available
	ptr->MCR = pmsg->MCR;
	rate = 0.0;
	for (i = 0; i < max_vci; ++i)
		if (abr_table[i] != NULL)
			rate += abr_table[i]->MCR;
	if (rate > TargetRate)
	{	delete	ptr;
		abr_table[aVci] = NULL;
		return "MCR not available";
	}


	ptr->forwER = pmsg->ICR;

	// compute ICR according to ERICA, CCR set to MCR
	ptr->CCR = ptr->MCR;
	
	++Nabr;	// pretend to have established already
	compERICA(ptr);
	--Nabr;

/*	this is wrong: it would mean that the source everytimes has to start at most with
	this rate (after having been silent for awhile). MBau, July 1, 1997
#BuggyInitICR
	// reduce wished ICR, if necessary
	if (ptr->ER < pmsg->ICR)
		pmsg->ICR = ptr->ER;
#BuggyInitICR (end)
*/

	//	adjust TBE
	if (pmsg->TBE > TBE )
		pmsg->TBE = TBE;

	// try to establish connection in the next objects
	abr_suc_nam = pmsg->routp[pmsg->akt_pointer];
	if((r = find_obj(abr_suc_nam)) == NULL)	// r is next ABR object
		errm2s("%s: could not find object `%s' (next element in ABR route)", name, abr_suc_nam);	 
	pmsg->akt_pointer++; 

	if ((err = r->special(pmsg, name)) != NULL )
		errm3s("%s: could not establish  ABR connection,reason returned by `%s': %s",
				name, abr_suc_nam, err);

	if (pmsg -> Requ_Flag == 1)
	{	// any successor has connection refused
		delete	ptr;
		abr_table[aVci] = NULL;
		return NULL;
	}

	//	all others also could establish connection
	//	set CCR to final ICR
	ptr->backwER = ptr->CCR = pmsg->ICR;

	ptr->active = FALSE;
	++Nabr;	// now there is one more abr connection in this muxer

	return NULL;
}	// end of establConn()

/*
*	release the connection
*/
char	*abrMux::releaseConn(
	ABRConFinMsg	*,
	char		*)
{
	errm1s("%s: sorry, connection release not yet implemented", name);
	return NULL;	// compiler warning
}

/*
*	export addresses of variables
*/
int	abrMux::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intArray1(msg, "LossData", (int *) lossData, max_vci, 0) ||
		intArray1(msg, "LossRMCI", (int *) lossRMCI, max_vci, 0) ||
		intArray1(msg, "LossRMCO", (int *) lossRMCO, max_vci, 0) ||

		intScalar(msg, "QLenNABR", &q_CVBR.q_len) ||
		intScalar(msg, "QLenABR", &abr_q_len) ||
		intScalar(msg, "QLenABRData", &q_ABR.q_len) ||
		intScalar(msg, "QLenRMCI", &q_RMCI.q_len) ||
		intScalar(msg, "QLenRMCO", &q_RMCO.q_len) ||

		doubleScalar(msg, "CRABR", &ABRRate) ||
		doubleScalar(msg, "CRNABR", &CVBRRate) ||
		doubleScalar(msg, "Z", &Z) ||

		intScalar(msg, "NActive", &Nactive);
}
