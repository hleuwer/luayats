///////////////////////////////////////////////////////////////////////////////////////
// aal5sendprio.c
// Baustein: AAL5PrioSend
// basiert auf aal5send
// zusaetzlich zur Funktionalitaet von aal5send wird das clp-Feld der Frames
// ausgelesen und in die gesendeten Zellen uebertragen
///////////////////////////////////////////////////////////////////////////////////////


#include "aal5sendprio.h"

CONSTRUCTOR(Aal5sendprio, aal5sendprio);
USERCLASS("AAL5PrioSend", Aal5sendprio);

/************************************************************************/
/*
*	activation by the kernel.
*	send next cell and register again if sth more to send
*/

void	aal5sendprio::early(event *)
{
	aal5Cell	*pc;

	pc = new aal5Cell(vci);		// get a new aal5cell

	if(flen <= 0)
	{	// start a new new SDU
		data	*pf = q.first();
		flen = pf->pdu_len() + 8 + addHeader;
					// number of bytes we need in reallity to transmit
					// given SDU (our trailer included)

      	        //clp = pf->clp;	 // set var clp to that of the frame
		// Test Mue 11.11.1999
		if(flen > 200)
		   clp = 1;
	        else
		   clp = 0;

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

        pc->clp = clp;	// set CLP to that of frame

      	 // Test Mue 11.11.1999
	 if(pc->pt == 1)
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


