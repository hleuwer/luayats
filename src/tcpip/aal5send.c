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
*	Module author:		Gunnar Loewe, TUD (diploma thesis)
*	Creation:		Sept 1996
*
*	History:
*	17/10/96		some cosmetics, class queue used. MBau
*
*	July 10, 1997		- data item embbeding changed (emdedded pointer)
*				- frame is embedded into last cell rather
*				  then the first one.
*				- changes for both: only in early()
*					Matthias Baumann
*
*	October 22, 1997	Optional translation table for conversion of connection IDs
*				into VCIs added. New command SetVCI(cid,vci).
*					Matthias Baumann
*
*	Nov 15, 1997		optional parameter HEADER added.
*					Matthias Baumann
*
*	Dec 12, 1997		option COPYCID added: connection ID of incomming frame
*				is directly used as current VCI
*					Matthias Baumann
*
*	Jan 23, 1998		bug fix: sequence numbers for cells and frames had
*				been common for all connections -> wrong loss measurements
*				in receivers. Now: tables for connnection specific sequence
*				numbers. COPYCID also needs the MAXCID spec.
*					Matthias Baumann
*
*     	June 30, 1999	      	option prinwarning included to switch off
*     	             	        the printing of warnings
*     	             	                Torsten Mueller
*
*     	August 20, 1999	      	option COPYCLP included to allow the copying
*     	             	        of CLP bits from frames to cells
*     	             	                Torsten Mueller
*
*     	August 25, 1999	      	option PT1CLP0 included to allow setting PT=1 cells to CLP=0
*     	             	                Torsten Mueller
*
*************************************************************************/

/*
*	AAL5 sender module
*
*	AAL5Send aal:	{VCI=1 | {MAXCID= 100 {, COPYCID}}},
*						// VCI: VC number of generated cells
*						// MAXCID: length of the table for translation
*						// COPYCID: use frame ID as VCI
*						//	(layer-4 connection ID) -> (VCI)
*						//		(only together with MAXCID)
*			BUF=100,		// input buffer size in frames
*			BSTART=5,		// input buffer occupation at which
*						// to wake up stopped source
*			{HEADER=8,}		// length of additional header (e.g. LLC/SNAP)
*						// default: 0
*     	             	{PRINTWARNING=0|1}   	// print warnings? default: 1
*     	             	{COPYCLP=0|1}   	// copy CLP bit to cells? default: 0
*     	             	{PT1CLP0=0|1}   	// set PT=1 cells (frame boundaries) to CLp=0 (high prio) default: 0
*			OUTDATA=shap,		// where to send cells
*			OUTCTRL=src->Start;	// control input of preceeding network object
*
*	Commands:
*		aal->ResetStat			// resets all statistics variables
*		aal->SetVCI(cid, vci)		// sets translation table entry: connection ID 'cid'
*						// is converted into VCI 'vci'.
*						// ONLY possible if MAXCID has been specified
*		aal->SetCID(cid_old, cid_new)	// frames arriving with connid=cid_old are changed into
*						// into connid=cid_new, default: cid_old=cid_new
*						// ONLY possible if MAXCID has been specified
*
*	Variables exported for use in commands and with measurement devices:
*		aal->Qlen		// current input queue length
*		aal->CellCount		// # of cells sent
*		aal->SDUCount		// # of AAL5 SDUs sent
*		aal->DelCount		// # of input frames discarded due to start-stop violation
*
*	The source recognizes the start-stop protocol at input and output.
*	The name of the data input is aal->Data, the control input is aal->Start.
*	Incomming frames have to implement the interface for data item embedding.
*	Actually, the exact type of received data types is free.
*
*	Mechanism to detect cell losses
*	===============================
*	The incomming frame is embedded into the first cell of the burst generated. Additionally,
*	the first cell carries the SDU sequence number. All cells carry a cell sequence number, therefore
*	the receiver can detect cell loss and flushes his input cell queue when detecting cell loss.
*	The last cell is marked with PT = 1, and this cell containes the cell sequence number of the
*	first cell of the burst. The receiver checks whether the first cell in the input queue has this
*	sequence number. If this holds then the frame has been transmitted succesfully and it can
*	be extracted from the first cell. Otherwise, all cells in the queue are dropped. Thanks to the SDU
*	sequence number, the number of lost SDU also can be detected.
*/

#include "aal5send.h"

CONSTRUCTOR(Aal5send, aal5send);

/************************************************************************/
/*
*	read definition statement
*/
void	aal5send::init(void)
{
	int	i;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	vci = 0;
	translationTabVCI = NULL;
	translationTabCID = NULL;
	
	fixVCI = FALSE;

	if (test_word("VCI"))
	{	vci = read_int("VCI");		// reads the VCI for the connection
		fixVCI = TRUE;
		maxcid = 1;
		skip(',');
	}
	else if (test_word("MAXCID"))
	{	maxcid = read_int("MAXCID") + 1;
		if (maxcid < 1)
			syntax0("invalid MAXCID");
		skip(',');

		if (test_word("COPYCID"))
		{	skip_word("COPYCID");
			skip(',');	// translationTabVCI remains NULL
		}
		else
		{	CHECK(translationTabVCI = new int [maxcid]);
			CHECK(translationTabCID = new int [maxcid]);
			for (int i = 0; i < maxcid; ++i)
			{
				translationTabVCI[i] = 0;
				translationTabCID[i] = i;	// no change as default
			}
		}
	}
	else	syntax0("`VCI', or `MAXCID' expected");

	CHECK(sduSeqTab = new int [maxcid]);
	for (i = 0; i < maxcid; ++i)
		sduSeqTab[i] = 0;
	CHECK(cellSeqTab = new int [maxcid]);
	for (i = 0; i < maxcid; ++i)
		cellSeqTab[i] = 0;

	curSduSeq = sduSeqTab;		// for safety, and in case of fixVCI
	curCellSeq = cellSeqTab;	// for safety, and in case of fixVCI

	q.setmax(read_int("BUF"));	// length of input queue in numbers of frames
	if (q.getmax() < 1)
		syntax0("invalid BUF");
	skip(',');

	q_start = read_int("BSTART");	// lower bound of input buffer, needed for Start signal
	skip(',');

	if (test_word("HEADER"))	// additional headers (e.g LLC/SNAP)
	{	addHeader = read_int("HEADER");
		if (addHeader < 0)
			syntax0("invalid HEADER");
		skip(',');
	}
	else	addHeader = 0;
	
	if (test_word("PRINTWARNING"))	// print out warnings?
	{	printwarning = read_int("PRINTWARNING");
		if ((printwarning < 0) || (printwarning > 1))
			syntax0("PRINTWARNING must be 0 or 1");
		skip(',');
	}
	else
	   printwarning = 1;  // default is printing

	if (test_word("COPYCLP"))	// copy CLP bit
	{	CopyClp = read_int("COPYCLP");
		if (CopyClp != 0 && CopyClp != 1)
			syntax0("COPYCLP must be 0 or 1");
		skip(',');
	}
	else
	   CopyClp = 0;  // no copy is default

	if (test_word("PT1CLP0"))	// set PT=1 cells to CLP=0
	{	Pt1Clp0 = read_int("PT1CLP0");
		if (Pt1Clp0 != 0 && Pt1Clp0 != 1)
			syntax0("PT1CLP0 must be 0 or 1");
		skip(',');
	}
	else
	   Pt1Clp0 = 0;  // no copy is default

 
	output("OUTDATA", SucData);	// output DATA
	skip(',');
	output("OUTCTRL", SucCtrl);	// output CTRL

	input("Data", InpData);		// input DATA
	input("Start", InpStart);	// input START

	del_cnt=0;			// counter of deleted frames,
					// if preceeding object did not recognize the Stop signal

	prec_state = ContSend;		// state of the preceeding object (Start-Stop-Protocol)
	send_state = ContSend;		// necessary?
					// YES, in case we get a frame and a start signal in the
					// first slot (see rec(InpStart) - test on send_state).
	
	sdu_cnt = 0;
	flen = 0;
	last_tim = 0-1;
	new_cid = -1;
	NewClp = 0;  	// default is to set CLP bit to 0
}

/************************************************************************/
/*
*	activation by the kernel.
*	send next cell and register again if sth more to send
*/

void	aal5send::early(event *)
{
	aal5Cell	*pc;

	pc = new aal5Cell(vci);		// get a new aal5cell

	if(flen <= 0)
	{	// start a new new SDU
		data	*pf = q.first();
		flen = pf->pdu_len() + 8 + addHeader;
					// number of bytes we need in reallity to transmit
					// given SDU (our trailer included)
	        if(CopyClp)
		   NewClp = pf->clp;

		if (fixVCI == FALSE)
		{	// for generation of multiple streams: set pointers to sequence numbers
			typecheck_i(pf, FrameType, InpData);
			int	cid = ((frame *) pf)->connID;

			if (cid < 0 || cid >= maxcid)
				errm1s2d("%s: out-of-range connection ID of %d received, MAXCID=%d",
						name, cid, maxcid - 1);
			curCellSeq = &cellSeqTab[cid];
			curSduSeq = &sduSeqTab[cid];

			// set current VCI
			if (translationTabVCI)	// translationTabCID is used only together with this!
			{
				// we shall translate the layer-4 connection ID into a new VCI
				pc->vci = vci = translationTabVCI[cid];	// correct VCI set above
				((frame *) pf)->connID = translationTabCID[cid];
			}
			else	// we shall use the layer-4 connection ID as VCI
				pc->vci = vci = cid;	// correct also the VCI set above
			
		} // different VCI's may arrive
		else
		{
		  typecheck_i(pf, FrameType, InpData);
		  if(new_cid != -1)
		    ((frame *) pf)->connID = new_cid; // change CID if necessary
		
		} // else - a fixed VCI
		  // else: nothing else to do, pointers point to first table entries, VCI unchanged

		first_seq = pc->cell_seq = ++*curCellSeq;	// increase sequence number of cell
						// store sequence number of first cell in SDU
		pc->sdu_seq = ++*curSduSeq;	// sequence number of AAL SDU

		++sdu_cnt;			// # of SDUs for statistic
	}
	else	// not the first cell:	increase sequence number of cell
		pc->cell_seq = ++*curCellSeq;

	if((flen -= 48) <= 0)	// last cell of frame, may equal the first one!
	{	pc->pt = 1;
		pc->first_cell = first_seq;	// this was the number of the first cell

		// take frame from input queue and embed it into last cell
		pc->embedded = q.dequeue();

		if(q.getlen() == q_start && prec_state == StopSend)
		{	// wake up stopped data sender
			sucs[SucCtrl]->rec(new data, shands[SucCtrl]);
			prec_state = ContSend;
		}
	}
	else	pc->pt = 0;

	pc->clp = NewClp;
	if(pc->pt == 1 && Pt1Clp0) // set frame boundaries to CLP=0 (if required)
	   pc->clp = 0;
	chkStartStop(send_state = sucs[SucData]->rec(pc, shands[SucData]));	// send cell

	// log the transmitted cell
	if( ++counter == 0)
		errm1s("%s: overflow of departs", name);
	
/*
	if (SimTime == last_tim)	// check if module tries to send twice in one slot
		errm1s1d("%s: twice sent in %d\n", name, SimTime);
	last_tim = SimTime;
*/
	
	if (send_state == ContSend && q.getlen() != 0)
		alarme(&std_evt, 1);
}


/************************************************************************/
/*
*	frame or start signal received.
*/
rec_typ aal5send::REC(data *pd, int key)	// REC is a macro normally expanding to rec (for debugging)
{
	switch (key) {
	case InpData:	//input is data

		if(prec_state == StopSend) 	// preceeding object ignores stop signal
		{	
		     	if(printwarning)
			   fprintf(stderr, "%s: preceeding object did not recognize the Stop signal, SimTime=%d\n",
					 name, SimTime);
			delete pd;
			if( ++del_cnt == 0)
				errm1s("%s: overflow of del_cnt", name);
			return StopSend;
		}

		if (q.isEmpty() && send_state == ContSend)
			alarme(&std_evt, 1);	// process in next slot (prevents twice send)
		if (q.enqueue(pd) == FALSE)
			errm1s("%s: internal error in aal5send::rec(): no space in queue", name);
	
		if(q.isFull())	// queue is full now
		{	prec_state = StopSend;
			return StopSend;
		}
		
		return ContSend;

	case InpStart:	// input is Start
		if(send_state == StopSend)
		{	send_state = ContSend;
			if(q.getlen() != 0)
				alarme(&std_evt, 1);	// process in next slot (prevents twice send)
		}
		delete pd;
		return ContSend;
	default:errm1s("%s: internal error: receive data on input with no receive method", name);
		return ContSend;	// compiler warning, not reached
	}

}


void	aal5send::restim(void)
{
	last_tim = 0-1;
}

int	aal5send::command(char *s, tok_typ *v)
{
	if (baseclass::command(s, v))
		return TRUE;

	v->tok = NILVAR;
	if(strcmp(s, "ResetStat") == 0)		// resets the statistic
	{	del_cnt = 0;
		sdu_cnt = 0;
		counter = 0;
	}
	else if (strcmp(s, "SetVCI") == 0)
	{	//	xyz->SetVCI(cid, vci);
		if (translationTabVCI == NULL)
			syntax1s("%s: no CID->VCI translation table available", name);

		int	cid;
		skip('(');
		cid = read_int(NULL);
		if (cid < 0 || cid >= maxcid)
			syntax1d("out-of-range connection ID (MAXCID=%d)", maxcid - 1);
		skip(',');
		translationTabVCI[cid] = read_int(NULL);
		skip(')');
	}
	else if (strcmp(s, "SetCID") == 0)
	{	
		if (translationTabCID == NULL)
			syntax1s("%s: no CID->CID translation table available", name);

		int	cid;
		skip('(');
		cid = read_int(NULL);
		if (cid < 0 || cid >= maxcid)
			syntax1d("out-of-range connection ID (MAXCID=%d)", maxcid - 1);
		skip(',');
		translationTabCID[cid] = read_int(NULL);
		skip(')');
	}
	else if (strcmp(s, "SetNewCID") == 0)
	{	
		if(fixVCI == FALSE)
		   syntax0("this command can only be used if a fixed VCI is given");
		
		skip('(');
		new_cid = read_int(NULL);
		if (new_cid < 0)
      	          syntax1d("CID must be >= 0, you tried %d", new_cid);

		skip(')');
	}
	else if (strcmp(s, "SetNewVCI") == 0)
	{	
		if(fixVCI == FALSE)
		   syntax0("this command can only be used if a fixed VCI is given");
		
		skip('(');
		vci = read_int(NULL);
		if (vci < 0)
      	          syntax1d("VCI must be >= 0, you tried %d", vci);

		skip(')');
	}

	else	return FALSE;

	return TRUE;
}

int	aal5send::export(
	exp_typ	*msg)
{
	return baseclass::export(msg) ||
		intScalar(msg, "QLen", &q.q_len) ||
		intScalar(msg, "CellCount", (int *) &counter) ||
		intScalar(msg, "SDUCount", (int *) &sdu_cnt) ||
		intScalar(msg, "DelCount", (int *) &del_cnt);
}
