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
*	Module author:		Matthias Baumann, TUD
*	Creation:		Sept 1996
*
*************************************************************************/

/*
*	ABR sink.
*	The sink can act as virtual sink, since the output implements the start-stop
*	protocol.
*	The sink function is very simple: data cells are passed to the next object (queued
*	if necessary), and RM cells are reflected towards the SES. The DIR bit is set to 1,
*	the BN bit is set 0.
*
*	In case the sink is controlled by start-stop, the following mechanisms are used:
*	ER feedback:
*	If the buffer occupation reaches than TARGBUFF, than the ER returned to SES is
*	reduced to TARGUTIL * (current output rate). The current output rate is measured
*	over an interval of AI time slots.
*	Binary feedback:
*	If the buffer occupation becomes larger then HI_THRESH, a congestion flag is set, and
*	the CI bit of RM cells is set until congestion is finished. This occures if the buffer
*	occuptaion reaches LO_THRESH.
*		
*	For the start-stop protocol, an input snk->Start is used.
*
*	!! ATTENTION !!
*	The buffer size limits the TBE value of a connection. It has to be sufficiently
*	large, also if the sink is not controlled by start-stop.
*
*	AbrSink	snk:	BUFF=1000,		// output buffer size
*						// !! ATTENTION !! see above.
*			{TARGBUFF=800,}		// buffer occupation at which to start decreasing ER
*						// default: BUFF / 2
*			{HI_THRESH=900,}	// buffer occupation at which to set congestion
*						// default: BUFF
*			{LO_THRESH=700,}	// buffer occupation at which to clear congestion
*						// default: HI_THRESH
*			{AI=100,}		// measurement interval for output rate
*						// default: 30
*			{LINKCR=100000.2}	// output link cell rate (cells per second)
*						// default: 353207.55 (149.76 Mbit/s)
*			{TARGUTIL=0.9,}		// factor by which to reduce ER below current output
*						// link rate if TARGBUFF is passed
*						// default: 1.0
*			OUTBRMC=bla,		// where to send backward RM cells
*			OUTDATA=blo;		// where to send data
*/

#include "abrsink.h"

CONSTRUCTOR(AbrSink, abrSink);

void	abrSink::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	q.setmax(read_int("BUFF"));
	if (q.getmax() < 1)
		syntax0("invalid BUFF");
	skip(',');

	if (test_word("TARGBUFF"))
	{	TargetBuff = read_int("TARGBUFF");
		if (TargetBuff < 1 || TargetBuff > q.getmax())
			syntax0("invalid TARGBUFF");
		skip(',');
	}
	else
	{	TargetBuff = q.getmax() / 2;
		if (TargetBuff < 1)
			TargetBuff = 1;
	}

	if (test_word("HI_THRESH"))
	{	q_high = read_int("HI_THRESH");
		if (q_high < 1)
			syntax0("invalid HI_THRESH");
		skip(',');
	}
	else	q_high = q.getmax();
	if (test_word("LO_THRESH"))
	{	q_low = read_int("LO_THRESH");
		if (q_low > q_high)
			syntax0("invalid LO_THRESH");
		skip(',');
	}
	else	q_low = q_high;
	congested = FALSE;

	if (test_word("AI"))
	{	AI_tim = read_int("AI");
		if (AI_tim < 1)
			syntax0("invalid AI");
		skip(',');
	}
	else	AI_tim = 30;

	if (test_word("LINKCR"))
	{	LinkRate = read_double("LINKCR");
		if (LinkRate <= 0.0)
			syntax0("invalid LINKCR");
		skip(',');
	}
	else	LinkRate = 353207.55;

	if (test_word("TARGUTIL"))
	{	TargetUtil = read_double("TARGUTIL");
		if (TargetUtil <= 0.0 || TargetUtil > 1.0)
			syntax0("invalid TARGUTIL");
		skip(',');
	}
	else	TargetUtil = 1.0;

	if (test_word("VCI"))
	{
		vci = read_int("VCI");
		if (vci <= 0)
			syntax1d("VCI must be > 0 and was set to %d",vci);
		skip(',');
	}
	else	vci = 0;

	if (test_word("VCIBRMC"))
	{
		vci_brmc = read_int("VCIBRMC");
		if (vci <= 0)
			syntax1d("VCIBRMC must be > 0 and was set to %d",vci_brmc);
		skip(',');
	}
	else	vci_brmc = vci;

	output("OUTBRMC", SucBRMC);
	skip(',');
	output("OUTDATA", SucData);

	stdinp();	// ABR input
	input("Start", InpStart);

	outp_load_cnt = 0;
	OutputRate = 0.0;
	alarml( &AI_evt, AI_tim);

	my_state = ContSend;
	lost = 0;
	cntData = cntRMCI = cntRMCO = 0;

	last_cell = SimTime;
	connected = FALSE;
}

rec_typ	abrSink::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	i)
{
	if (i == InpStart)
	{	// we can continue to send
		delete	pd;
		if (my_state == StopSend)
		{	my_state = ContSend;
			if ( !q.isEmpty())
				alarme( &std_evt, 1);
		}
		return ContSend;
	}

	if (connected == FALSE)
		errm1s("%s: no connection established, but cell received", name);

	typecheck_i(pd, CellType, i);
	if (((cell *)pd)->vci != vci)
		errm1s2d("%s: forward cell with unknown VCI = %d received. (expected was %d)",
				name, ((cell *)pd)->vci, vci);

	if (typequery(pd, RMCellType))
	{	// RM cell received: send it back to SES
		rmCell	*pc = (rmCell *) pd;
		double	ER;

		if (pc->DIR != 0)
			errm1s1d("%s: backward RM cell with VCI %d received", name,((cell *)pd)->vci );

		pc->DIR = 1;	//	change DIR bit
		pc->BN = 0;	//	mark that cell now comes from DES
		
		//	congested?
		if (congested)
		{	if (q.getlen() <= q_low)
				congested = FALSE;
			else	pc->CI = 1;
		}
		else if (q.getlen() > q_high)
		{	congested = TRUE;
			pc->CI = 1;
		}

		//	compute possible ER
		if (q.getlen() >= TargetBuff)
			ER = OutputRate * TargetUtil;
		else	ER = PCR;	// source will increase by RIF!
		if (ER < MCR)
			ER = MCR;

		if (pc->ER > ER)	// correct ER in RM cell
			pc->ER = ER;

		// count cells
		if (pc->CLP)
		{	if ( ++cntRMCO == 0)
				errm1s("%s: overflow of CountRMCO", name);
		}
		else
		{	if ( ++cntRMCI == 0)
				errm1s("%s: overflow of CountRMCI", name);
		}
		
		pc->vci = vci_brmc;	// new VCI
		sucs[SucBRMC]->rec(pc, shands[SucBRMC]);
		return ContSend;
	}
	
	// an ABR data cell
	if ( ++cntData == 0)
		errm1s("%s: overflow of CountData", name);

	if (my_state == ContSend)
	{	if (q.isEmpty())
		{	if (last_cell == SimTime)
			{	// we have sent already -> wait
				q.enqueue(pd);	// capacity at least one!
				alarme( &std_evt, 1);
			}
			else
			{	chkStartStop(my_state = sucs[SucData]->rec(pd, shands[SucData]));
				++outp_load_cnt;
				last_cell = SimTime;
			}
		}
		else if (q.enqueue(pd) == FALSE)
		{	delete pd;
			if ( ++lost == 0)
				errm1s("%s: overflow of lost", name);
		}
	}
	else if (q.enqueue(pd) == FALSE)
	{	delete pd;
		if ( ++lost == 0)
			errm1s("%s: overflow of Loss", name);
	}

	return ContSend;
}

/*
*	send next cell from queue
*/
void	abrSink::early(
	event	*)
{
	data	*pd;

	if ((pd = q.dequeue()) == NULL)
		errm1s("internal error: %s: abrSink::early(): no data available", name);

	chkStartStop(my_state = sucs[SucData]->rec(pd, shands[SucData]));
	++outp_load_cnt;
	last_cell = SimTime;

	if (my_state == ContSend && !q.isEmpty())
		alarme( &std_evt, 1);
}

/*
*	AI timer expired
*/
void	abrSink::late(
	event	*)
{
	OutputRate = LinkRate * outp_load_cnt / (double) AI_tim;
	outp_load_cnt = 0;
	alarml( &AI_evt, AI_tim);
}

/*
*	connection establishment and release
*/
char	*abrSink::special(
	specmsg	*msg,
	char	*)
{
	if (msg->type == ABRConFinType)
		errm1s("%s: sorry, connection release not yet implemented", name);
	if (msg->type != ABRConReqType)
		return "wrong type of special message";
	if (connected)
		errm1s1d("%s: can't establish connection: connection (vci = %d) established already",
				name, vci);

	ABRConReqMsg	*pmsg;
	pmsg = ((ABRConReqMsg *)msg);

	if (pmsg->akt_pointer != pmsg->numb_RoutMemb)
		errm1s("%s: invalid connection request: need to be last routing member", name);

	if (pmsg->MCR > LinkRate)
	{	pmsg->Requ_Flag = 1;
		pmsg->refuser = name;
		return NULL;
	}
	MCR = pmsg->MCR;

	if (pmsg->PCR > LinkRate)
		pmsg->PCR = LinkRate;
	PCR = pmsg->PCR;
	if (pmsg->ICR > PCR)
		pmsg->ICR = PCR;
	if (pmsg->TBE > q.getmax())
		pmsg->TBE = q.getmax();

	if(vci == 0)	// only, if VCI was not set in command line
	   vci = pmsg->VCI;
	if(vci_brmc == 0)	// only, if VCIBRMC was not set in command line
	   vci_brmc = vci;

	
	connected = TRUE;

	return NULL;
}

/*
*	export variables
*/
int	abrSink::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "QLen", &q.q_len) ||
		intScalar(msg, "Loss", (int *) &lost) ||

		intScalar(msg, "CountData", (int *) &cntData) ||
		intScalar(msg, "CountRMCI", (int *) &cntRMCI) ||
		intScalar(msg, "CountRMCO", (int *) &cntRMCO) ||

		doubleScalar(msg, "CROut", &OutputRate);
}

