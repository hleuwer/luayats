#include "aal5sred.h"

CONSTRUCTOR(Aal5sred, aal5sred);
USERCLASS("AAL5SRED", Aal5sred);

/************************************************************************/
/*
*	read definition statement
*/
#if 1
void aal5sred::init(void){}
#else
void	aal5sred::init(void)
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

	red.th_l = read_int("RED_TH_LOW");	// lower threshold for performing RED
	if(red.th_l < 0)
	   syntax0("RED_TH_LOW must be >= 0");
        else if(red.th_l >= q.getmax())
	   syntax0("RED_TH_LOW must be < BUFF");
	skip(',');

	red.th_h = read_int("RED_TH_HIGH");	// lower threshold for performing RED
	if(red.th_h <= red.th_l)
	   syntax0("RED_TH_HIGH be > RED_TH_LOW");
        else if(red.th_h > q.getmax())
	   syntax0("RED_TH_HIGH must be <= BUFF");
	skip(',');

	red.pmax = read_double("RED_PMAX");	// lower threshold for performing RED
	if(red.pmax < 0 || red.pmax > 1)
	   syntax0("RED_PMAX must be in [0,1]");
	skip(',');

	if(test_word("RED_USEAVERAGE"))
	{
	   red.useaverage = read_int("RED_USEAVERAGE");	// lower threshold for performing RED
	   if(red.useaverage < 0 || red.useaverage > 1)
	      syntax0("RED_USEAVERAGE must be in [0,1]");
	   skip(',');
	 }
	 else
	    red.useaverage = 1;

	if(test_word("RED_WQ"))
	{
	   red.w_q = read_double("RED_WQ");
	   if(red.w_q <= 0 || red.w_q > 1)
	      syntax0("RED_WQ must be in (0,1]");
	   skip(',');
	 }

	if (test_word("HEADER"))	// additional headers (e.g LLC/SNAP)
	{	addHeader = read_int("HEADER");
		if (addHeader < 0)
			syntax0("invalid HEADER");
		skip(',');
	}
	else	addHeader = 0;
 
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
}
#endif

/************************************************************************/
/*
*	activation by the kernel.
*	send next cell and register again if sth more to send
*/

void	aal5sred::early(event *)
{
	aal5Cell	*pc;

	pc = new aal5Cell(vci);		// get a new aal5cell

	if(flen <= 0)
	{	// start a new new SDU
		data	*pf = q.first();
		flen = pf->pdu_len() + 8 + addHeader;
					// number of bytes we need in reallity to transmit
					// given SDU (our trailer included)

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
rec_typ aal5sred::REC(data *pd, int key)	// REC is a macro normally expanding to rec (for debugging)
{
   switch (key) {
   case InpData:	//input is data

      // decide, if we have to drop the item
      int drop;

      // use RED (or variant) for early drop
      drop = red.Update(q.q_len);

      if(q.isFull()) // first check for full
         drop = 1;


      if (q.isEmpty() && send_state == ContSend)
         alarme(&std_evt, 1);	// process in next slot (prevents twice send)

      if(drop)
      {
          delete pd;
          if( ++del_cnt == 0)
	     errm1s("%s: overflow of del_cnt", name);
      }
      else
         if (q.enqueue(pd) == FALSE)
            errm1s("%s: internal error in aal5sred::rec(): no space in queue", name);


      return ContSend;

   case InpStart:	// input is Start
      if(send_state == StopSend)
      {
	 send_state = ContSend;
	 if(q.getlen() != 0)
            alarme(&std_evt, 1);	// process in next slot (prevents twice send)
      }
      delete pd;
      return ContSend;

   default:errm1s("%s: internal error: receive data on input with no receive method", name);
	   return ContSend;	// compiler warning, not reached
}

}


void	aal5sred::restim(void)
{
	last_tim = 0-1;
}

int	aal5sred::command(char *s, tok_typ *v)
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

int	aal5sred::export(
	exp_typ	*msg)
{
	return baseclass::export(msg) ||
		intScalar(msg, "QLen", &q.q_len) ||
		doubleScalar(msg, "RedAvg", &red.avg) ||
		doubleScalar(msg, "pa", &red.pa) ||
		intScalar(msg, "CellCount", (int *) &counter) ||
		intScalar(msg, "SDUCount", (int *) &sdu_cnt) ||
		intScalar(msg, "DelCount", (int *) &del_cnt);
}
