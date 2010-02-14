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
*	History
*	July 28, 1997		Quick hack in establ_conn().
*				ACR_tim is set after compACRtim()
*					MBau
*	Sept 30, 1997		Overflow detection in compACR_tim(). See there.
*					MBau
*
*************************************************************************/

/*
*	ABR source implementing the reference source behaviour of
*	ATMF Traffic Mangmt. Spec. 4.0
*
*	AbrSrc src:	BUFF=1000,		// input buffer size
*			BSTART=500,		// input buffer occupation at which
*						// to restart the stopped sender
*			{LINKCR=100000,}	// output link cell rate (cells per second)
*						// default: 353207.55 (149.76 Mbit/s)
*			MCR=200,		// minimum cell rate (cells per second)
*			PCR=800,		// peak cell rate (cells per second)
*			FRTT=10000,		// fixed round trip time (micro seconds)
*			ROUTE=(1,sink)||		// ABR routing members. First number of members
*			ROUTEARRAY=(n,array) // n=number of routing members and array=array[0]..array[n-1]
*										// with module names of routing members
*			{AUTOCONN,}		// if given, the connection is established with the
*						// first incomming cell
*			OUTCTRL=gmdp->Start,	// control input of the data sender
*						// (for start-stop-protocol)
*			OUTDATA=sink;		// data output
*
*	The source has four inputs:
*			src		// input for data cells
*			src->BRMC	// input for backward RM cells
*			src->Start	// to establish an ABR connection
*			src->Stop	// to release the ABR connection
*	Variables:
*			CountData	int, number of data cells sent
*			CountRMCI	int, number of in-rate RM cells sent
*			CountRMCO	int, number of out-of-rate RM cells sent
*			QLen		int, current input queue length
*			IAT		int, current IAT (permanently changing due to non-integer 1 / ACR)
*			ACR		double, current ACR
*
*	Implementation details:
*
*	The TCR timer for out-rate RM cells is always active. If the time expires, we test
*	whether in-rate cells (RM or data) have been sent since the last out-rate RM cell. If yes,
*	the next out-rate RM cell is postponed appropriatly (TCR timer). If not, the RM cell
*	is sent (restart TCR timer).
*
*	After sending an inrate-cell (ACR increased above TCR ore data became availabe), 
*	the ACR timer is started if something more is to sent (data or in-rate RM according to
*	rule s3a)ii).
*	Additionally, the Trm timer is started with the time difference
*	determined by last in-rate RM cell and Trm, if the Mrm condition of rule s3a)i
*	is fullfilled. Every sent in-rate RM cell stops this timer, since then the Mrm condition
*	can not be true any longer. Upon expiry of the Trm timer, an in-rate RM cell is scheduled
*	according to rule s3a)i. See REMARK below. (Algorithm changed)
*
*	If the source has to stop since ACR becomes too low (below TCR), then both the ACR and the
*	Trm timer are stopped (if active).
*
*	Before sending any cell, we check whether already a cell has been sent during the
*	current time slot. If this is the case, we try it again next slot.
*
*	REMARK:
*	Stopping the Trm timer with every in-rate RM cell is not very efficient, since normally
*	then every in-rate RM cell will do so, and after the next Mrm in-rate cells (data, probably)
*	the Trm timer will start again. Stopping a timer is an expensive operation!!
*	Proposal (not yet implemented):
*	Do not stop Trm with an in-rate RM cell. If the Trm timer expires (activation stays unchanged
*	at the end of sendInRate()), then the condition Mrm is checked.
*	If Mrm condition fullfilled:
*		* if time since last in-rate RM cell >= Trm: send cell (via sendInRate(), stop ACR before)
*		* if time condition not yet fullfilled: restart Trm timer appropriatly (like at the
*		  end of sendInRate()
*	If Mrm not reached:
*		* do nothing. Trm will be started again by sendInRate(), if necessary
*	Since these operations are very similar to those at the end of sendInRate(): in-line method?
*	-> DONE 04.09.96
*/

#include "abrsrc.h"
#include <string.h>
#include <math.h>  // log10 for calculating the string size of an integer

CONSTRUCTOR(AbrSrc, abrSrc);

/*
*	read the definition statement
*/
void	abrSrc::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	q.setmax(read_int("BUFF"));
	skip(',');
	q_start = read_int("BSTART");
	skip(',');

	//	default values:
	Trm = 100.0;
	RIF = 1.0 / 16.0;
	RDF = 1.0 / 16.0;
	CDF = 1.0 / 16.0;
	TCR = 10.0;
	ADTF = 0.5;
	Nrm = 32;
	Mrm = 2;

	if (test_word("LINKCR"))
	{	LinkRate = read_double("LINKCR");
		if (LinkRate <= 0.0)
			syntax0("invalid LINKCR");
		skip(',');
	}
	else	LinkRate = 353207.55;	// 149.76 Mbps

	MCR = read_double("MCR");
	if (MCR < 0.0 || MCR > LinkRate)
		syntax0("0.0 <= MCR <= LINKCR");
	skip(',');
	PCR = read_double("PCR");
	if (PCR < MCR || PCR > LinkRate)
		syntax0("MCR <= PCR <= LINKCR");
	skip(',');
	FRTT = read_double("FRTT");
	if (FRTT <= 0.0)
		syntax0("invalid FRTT");
	skip(',');

	if(test_word("ROUTE"))
	{
		skip_word("ROUTE");
		skip('=');
		skip('(');
		route_len = read_int(NULL);
		if (route_len < 1)
			syntax0("ROUTE must comprise at least one object");
		skip(',');
		CHECK(route = new char * [route_len]);
		for (int i = 0; i < route_len; ++i)
		{	route[i] = read_suc(NULL);
			if (i != route_len - 1)
				skip(',');
		}
		skip(')');
	} // ROUTE is given
	else
	{
	   char *route_array;
	   skip_word("ROUTEARRAY");
	   skip('=');
	   skip('(');
	   route_len = read_int(NULL);
		if (route_len < 1)
			syntax0("ROUTE must comprise at least one object");
		skip(',');
		
	   route_array = read_string(NULL);	// read in the name of the array

	   int i;
	   tok_typ token;
	   
      	   CHECK(route = new char * [route_len]);
	   for(i=0; i<route_len; i++)
	   {
	      // char length of the complete routing member
	      int clen = strlen(route_array) + (int)log10(i+1) + 4; // 2+1+1
	      char *route_member = new char[clen]; 				// alloc. memory
	      sprintf(route_member,"%s[%d]",route_array,i);	// write name on route_member
	      token = eval(route_member, name); 					// look into YATS array
	      
	      if(token.tok == SVAL)
	         route[i] = token.val.s;
	      else
	         syntax1s("%s does not contain string values",route_member);
	      
	      delete route_member;  // dont need this name anymore
	      
	   } // for

	   delete route_array;	    // dont need the name of the array anymore
	   skip(')');
		
	}  // else - if ROUTEARRAY is given
	
	skip(',');

	if (test_word("AUTOCONN"))
	{	skip_word("AUTOCONN");
		skip(',');
		autoconn = TRUE;
	}
	else	autoconn = FALSE;

	output("OUTCTRL", SucCtrl);
	skip(',');
	output("OUTDATA", SucData);

	stdinp();
	input("BRMC", InpRMC);
	input("Start", InpConnEst);
	input("Stop", InpConnRel);

	cntRMCO = 0;
	cntRMCI = 0;
	cntData = 0;

	connected = FALSE;
	prec_state = ContSend;
}
	
/*
*	establish an ABR connection. The routing members are informed via
*	the special() method.
*/
void	abrSrc::establConn(
	cell	*pc)
{
	ABRConReqMsg	msg;
	root		*r;
	char		*err;

	if (connected)
		errm1s("%s: connection request received, but already connection existing", name);

	if ((r = find_obj(route[0])) == NULL)
		errm2s("%s: could not find routing member `%s'", name, route[0]);

	msg.routp = route;
	msg.numb_RoutMemb = route_len;
	msg.akt_pointer = 1;
	msg.Requ_Flag = 0;
	msg.refuser = "<unknown>";
	msg.ICR = PCR;
	msg.PCR = PCR;
	msg.TBE = 16777215;	// default ATM-Forum
	msg.MCR = MCR;
	msg.VCI = vci = pc->vci;

	//	connection negotiation
	if ((err = r->special( &msg,name)) != NULL)	// special method unsucessful
		errm3s("%s: ConnRequest_Error, reason returned by `%s': %s", name, route[0], err);

	if (msg.Requ_Flag != 0)	// connection refused
		errm2s1d("%s: no connection possible, refused by: %s, next attempt: %d",
					name, msg.refuser, msg.next_att);
	if (msg.akt_pointer != msg.numb_RoutMemb)
		errm1s("%s: incomplete connection establishment", name);
	
	CRM = msg.TBE / Nrm;	
	if (msg.ICR < msg.TBE / FRTT)
		ICR = msg.ICR;
	else	ICR = msg.TBE / FRTT;
	ACR = ICR;

if (ACR == 0.0)		// QUICK HACK. This is to avoid the (ACR==0.0) branch in compACR_tim().
	ACR = PCR / 1e6;

	// if (ACR < MCR || ACR > PCR)
	//	errm1s("%s: connection negotiation resulted in invalid ACR", name);
		 
	IR_count = 0;
	unack = 0;

	compACR_tim();	// set ACR_tim, lo, hi, splash and bucket
	ACR_tim = ACR_tim_hi; // QUICK HACK. May be, compACR_tim() did not set it the first time.
	ACR_active = FALSE;

	Trm_tim = ((tim_typ) ((Trm / 1000.0) / (1.0 / LinkRate))) + 1;	// Trm im msec!
	Trm_active = FALSE;

	TCR_tim = ((tim_typ) (LinkRate / TCR)) + 1;
	alarme( &TCR_evt, TCR_tim);
	
	ADTF_tim = ((tim_typ) (ADTF / (1.0 / LinkRate))) + 1;	// ADTF in seconds!

	last_Cell = last_IR = last_RMCI = SimTime;

	if (ACR > TCR)
		my_state = ContSend;
	else	my_state = StopSend;

	connected = TRUE;
	first_Cell = TRUE;

	if (TCR_tim == 0)
		errm1s("internal error: %s: TCR_tim == 0", name);
	if (ACR_tim == 0)
//		fprintf(stderr, "ACR=%e, ACR_tim_hi = %d, ACR_tim_lo = %d, PCR= %e\n",
//				ACR, ACR_tim_hi, ACR_tim_lo, PCR),
		errm1s("internal error: %s: ACR_tim == 0", name);
	if (Trm_tim == 0)
		errm1s("internal error: %s: Trm_tim == 0", name);

	// printf("%s: TCR_tim = %u, ACR_tim = %u, Trm_tim = %u\n", name, TCR_tim, ACR_tim, Trm_tim);
}

/*
*	release the connection
*/
void	abrSrc::releaseConn(
	data	*)
{
	errm1s("%s: sorry, connection release not yet implemented", name);
}

/*
*	Cell received.
*/

rec_typ	abrSrc::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	i)
{	
	double	old_ACR;
	rmCell	*pc;

	typecheck_i(pd, CellType, i);

	if (connected == FALSE)
	{	if (i == 0 && autoconn)
			// auto connect was turned on
			// establish connection and proceed as usual
			establConn((cell *) pd);
		else if (i != InpConnEst)
			errm1s("%s: cell received, but no connection established", name);
	}
	else if (((cell *) pd)->vci != vci)
		errm1s1d("%s: cell with VCI = %d received", name, ((cell *) pd)->vci);

	switch (i) {
	case InpData:	// a data cell
		if (prec_state == StopSend)
			errm1s("%s: preceeding object did not recognize Stop signal", name);
		if (q.enqueue(pd) == FALSE)
			errm1s("internal error: %s: fatal in rec(), queue overflow", name);
		if (q.isFull())
			prec_state = StopSend;
		if (my_state == ContSend && ACR_active == FALSE)
			// if ACR is active, then sending in-rate is already in progress
			sendInRate();
		return prec_state;
	case InpRMC:	// a backward RM cell
		typecheck_i(pd, RMCellType, i);
		pc = (rmCell *) pd;
		if (pc->ident != this)
			errm1s("%s: RM cell with invalid connection code received", name);
		if (pc->DIR != 1)
			errm1s("%s: forward RM cell received", name);

		// for rule s6:
		if (pc->BN == 0)
			unack = 0;	// a RM cell from DES received

		/*
		*	compute new ACR
		*/
		old_ACR = ACR;
		//	rule s8
		if (pc->CI != 0)
		{	//	reduce ACR
			ACR -= ACR * RDF;
			if (ACR < MCR)
				ACR = MCR;
		}
		else if (pc->NI == 0)	// CI is zero (due to first if)
		{	//	increase ACR
			ACR += PCR * RIF;
			if (ACR > PCR)
				ACR = PCR;
		}
		//	rule s9
		if (ACR > pc->ER && pc->ER >= MCR)
			ACR = pc->ER;

	
		if (ACR != old_ACR)	// ACR changed
		{	int	tim_changed;
			// printf("%s: %d: adjusting ACR\n", name, SimTime);
			tim_changed = compACR_tim();
			switch (my_state) {
			case StopSend:
				if (ACR > TCR)
				{	my_state = ContSend;
					if (ACR_active == FALSE && !q.isEmpty())
					// if ACR is active, then sending in-rate
					// is already in progress
						sendInRate();
				}
				break;
			case ContSend:
				if (ACR <= TCR)
				{	my_state = StopSend;
					// stop all in-rate activities
					if (ACR_active)
					{	unalarme( &ACR_evt);
						ACR_active = FALSE;
					}
					if (Trm_active)
					{	unalarme( &Trm_evt);
						Trm_active = FALSE;
					}
				}
				else
				{	if (ACR_active && tim_changed)	// timer values changed
					{	//	adjust ACR timer
						unalarme( &ACR_evt);
						if (last_IR + ACR_tim > SimTime)
							alarme( &ACR_evt, last_IR + ACR_tim - SimTime);
						else	alarme( &ACR_evt, 1);
					}
				}
				break;
			default:errm1s("internal error: %s: fatal in rec() switch my_state", name);
			}
		}
		delete pd;
		return ContSend;
	case InpConnEst:	// Start input
		establConn((cell *) pd);
		delete pd;
		return ContSend;
	case InpConnRel:	// Stop input
		releaseConn(pd);
		delete pd;
		return ContSend;
	default:errm1s("internal error: %S: rec(): invalid input key", name);
		return StopSend;	// compiler warning
	}
}

/*
*	A timer expired
*/
void	abrSrc::early(
	event	*evt)
{
	switch (evt->key) {
	case ACR_timer:		// send in-rate (RM or data)
		ACR_active = FALSE;
		sendInRate();	// restarts ACR timer if necessary
		return;
	case TCR_timer:		// send out-rate RM cell, restart timer
		if (last_IR + Trm_tim > SimTime)
		{	// we postpone sending RMCO, since in-rate cells have been sent in between
			alarme( &TCR_evt, last_IR + Trm_tim - SimTime);
		}
		else if (last_Cell == SimTime)
		{	// already a cell sent this slot: try again next slot
			alarme( &TCR_evt, 1);
		}
		else	// no in-rate cells sent since last RMCO
		{	rmCell	*pc;
			pc = new rmCell(vci);
			fill_RMC(pc, 1);	// CLP = 1 -> RMCO

			// printf("%s: %d: sending a RMCO with vci %d\n", name, SimTime, vci);
			sucs[SucData]->rec(pc, shands[SucData]);
			last_Cell = SimTime;
			if ( ++cntRMCO == 0)
				errm1s("%s: overflow of CountRMCO", name);

			alarme( &TCR_evt, TCR_tim);
		}
		return;
	case Trm_timer:
		// Change due to remark very above: with a RMCI, the Trm
		// timer is not stopped. So we have to check the conditions for
		// rule s3a)i.
		if (IR_count <= Mrm)
			// an RMCI in between. If the Mrm condition is fulfilled again,
			// the Trm timer is restarted by sendInRate()
			Trm_active = FALSE;
		else if (last_RMCI + Trm_tim <= SimTime)
		{	// o.k., all conditions met, schedule the RMCI
			Trm_active = FALSE;
			if (ACR_active)
			{	unalarme( &ACR_evt);
				ACR_active = FALSE;
			}
			sendInRate();	// send the cell a.s.a.p.
		}
		else	// Trm condition not yet met -> postpone cell
			alarme( &Trm_evt, last_RMCI + Trm_tim - SimTime);
		return;
	default:errm1s("internal error: %s: bad evt->key in early()", name);
	}
}


/*
*	send an in-rate cell (RM or data)
*	MAKE SURE that ACR does not run when calling!
*/
void	abrSrc::sendInRate(void)
{
	if (ACR_active)
		errm1s("internal error: %s: abrSrc::sendInRate() ACR timer is ractive", name);

	if (last_Cell == SimTime)
	{	//	already a cell sent this slot: try again next slot
		alarme( &ACR_evt, 1);
		ACR_active = TRUE;
		return;
	}

	if (last_IR + ACR_tim > SimTime)
	// we have to wait a little bit, may happen when called from
	// early() (Trm expiry) or rec() (data became available, or ACR now larger than TCR)
	{	alarme( &ACR_evt, last_IR + ACR_tim - SimTime);
		ACR_active = TRUE;
		return;
	}

	//	we can send imediatly
	data	*pd;
	if (first_Cell)
	{	// send first cell as in-rate RM cell, register ACR timer for next cell
		first_Cell = FALSE;
		if (send_RMCI() == FALSE)
			// ACR became too slow, stop in-rate sending
			// (my_state is StopSend, timers are turned off)
			return;
		alarme( &ACR_evt, ACR_tim);
		ACR_active = TRUE;
	}
	else if (IR_count >= Nrm || (last_RMCI + Trm_tim <= SimTime && IR_count > Mrm))
	{	// send in-rate RM cell, register ACR timer for next cell (if any)
		if (send_RMCI() == FALSE)
			// ACR became too slow, stop in-rate sending
			// (my_state is StopSend, timers are turned off)
			return;
		if ( !q.isEmpty())
		{	alarme( &ACR_evt, ACR_tim);
			ACR_active = TRUE;
		}
	}
	else if ((pd = q.dequeue()) != NULL)
	{	// data available, send them
		// printf("%s: %d: sending a data cell with vci %d\n", name, SimTime, ((cell *)pd)->vci);
		if ( ++cntData == 0)
			errm1s("%s: overflow of CountData", name);
		sucs[SucData]->rec(pd, shands[SucData]);
		last_IR = last_Cell = SimTime;
		recalc_IAT();	// we have sent an in-rate cell: recompute IAT_bucket ...

		IR_count++;

		//	wake up stopped data sender?
		if (prec_state == StopSend && q.getlen() == q_start)
		{	sucs[SucCtrl]->rec(new data, shands[SucCtrl]);
			prec_state = ContSend;
		}

		//	restart ACR timer if more data available
		//	or an in-rate RM cell can be sent next (rule s3a)ii
		if ( !q.isEmpty() || IR_count >= Nrm)
		{	alarme( &ACR_evt, ACR_tim);
			ACR_active = TRUE;
		}
	}

	//	Check whether we can send an in-rate RM cell according to rule s3a).i
	if (Trm_active == FALSE && IR_count > Mrm)
	{	// the Mrm counter is o.k., start the timer with the time resting
		if (last_RMCI + Trm_tim <= SimTime)
			alarme( &Trm_evt, 1);
		else	alarme( &Trm_evt, last_RMCI + Trm_tim - SimTime);
		Trm_active = TRUE;
	}
}

/*
*	send an in-rate RM cell
*	keep track of the ACR!
*/
int	abrSrc::send_RMCI(void)
{	
	rmCell	*pc;
	int	retv = TRUE;

	//	rule s5: reduce ACR to ICR if we have been silent for a long time
	if (SimTime - last_RMCI > ADTF_tim && ACR > ICR)
	{	ACR = ICR;
		compACR_tim();
		if (ACR <= TCR)	// normally, this should not happen
		{	my_state = StopSend;
			retv = FALSE;
		}
	}

	//	rule s6: reduce ACR in case of missing backward RM cells
	if (unack >= CRM)
	{	ACR -= ACR * CDF;
		if (ACR < MCR)
			ACR = MCR;
		compACR_tim();
		if (ACR <= TCR)	// this is possible
		{	my_state = StopSend;
			retv = FALSE;
		}
	}
	++unack;

	pc = new rmCell(vci);
	fill_RMC(pc, 0);	// CLP = 0 -> RMCI

	// printf("%s: %d sending a RMCI with vci %d\n", name, SimTime, vci);

	// send cell
	sucs[SucData]->rec(pc, shands[SucData]);
	last_Cell = last_IR = last_RMCI = SimTime;
	recalc_IAT();	// we have sent an in-rate cell: recompute IAT_bucket ...

	if ( ++cntRMCI == 0)
		errm1s("%s: overflow of CountRMCI", name);

	IR_count = 1;

	return retv;	// return FALSE: ACR is too slow, stop in-rate sending
}

//	calculate new ACR timer values if ACR has changed
int	abrSrc::compACR_tim(void)
{
	tim_typ	old_tim_lo;

	if (ACR < 0.0)
		errm1s("internal error: %s: avrSrc::compACR_tim(): ACR < 0.0", name);
	if (ACR > LinkRate)
		errm1s("internal error: %s: avrSrc::compACR_tim(): ACR > LINKCR", name);

	old_tim_lo = ACR_tim_lo;
	if (ACR == 0.0)
	{	// These values are not effective, since the source is stopped anyway.
		// With the next ACR != 0, however, we should recognize a change
		ACR_tim_hi = (tim_typ) -1;
		ACR_tim_lo = (tim_typ) -1;
		ACR_splash = 0.0;
		if (old_tim_lo != ACR_tim_lo)
			return TRUE;
		return FALSE;
	}

	//	Take care with overflows. For very low ACRs, the ratio LinkRate / ACR
	//	may become too large.
	//		MBau, Sept 30, 1997
	if (LinkRate / ACR < 1e9)
		ACR_tim_lo = (tim_typ) (LinkRate / ACR);
	else	ACR_tim_lo = (tim_typ) 1e9;

	ACR_tim_hi = ACR_tim_lo + 1;
	ACR_splash = ((double) ACR_tim_hi) - (LinkRate / ACR);
	if (old_tim_lo != ACR_tim_lo)	// restart bucket if hi and lo have changed
	{	ACR_bucket = 0.0;
		ACR_tim = ACR_tim_hi;
		return TRUE;	// timer values changed
	}
	return FALSE;		// timer values unchanged
}


//	export addresses of variables
int	abrSrc::export(
	exp_typ *msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "QLen", &q.q_len) ||
		intScalar(msg, "IAT", (int *) &ACR_tim) ||
		intScalar(msg, "CountRMCO", (int *) &cntRMCO) ||
		intScalar(msg, "CountRMCI", (int *) &cntRMCI) ||
		intScalar(msg, "CountData", (int *) &cntData) ||
		doubleScalar(msg, "ACR", &ACR);
}

