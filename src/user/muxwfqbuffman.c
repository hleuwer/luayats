///////////////////////////////////////////////////////////////////////////////
// 
//    Multiplexer with WFQ and Buffer Management
//
//    MuxWFQBuffMan mux: NINP=<int>, 	 // number of inputs
//     	 BUFF=<int>,  	      	 // buffer size (cells)
//	 {MAXVCI=<int>,}      	 // max. VCI number (default NINP)
//    	 EPDTHRESH=<int>, 	 // when to not accept new frames (EPD-Thresh)
//    	 CLP1THRESH=<int>,    	 // when to discard CLP=1 cells
//    	 {EPD_CLP1=<bool>,}   	 // perform EPD/PPD for CLP=1 cells ? standard=0
//    	 {DFBA=<bool>,}	      	 // perform DFBA?, standard 0, to perform DFBA,
//    	             	      	 // EPD_CLP1 has to be set to 1, see below
//    	 {FAIR_CLP0=<bool>,}   	 // fair discarding of CLP=0 cells, see below
//    	 {FBA0=<bool>,}      	 // perform FBA, standard 0, see below
//    	 {PERFORM_RED1=<bool>,   // perform RED, standard 0, additional
//    	   ...}          	 // RED arguments, see source
//    	 {DELIVER_PT1=<bool>,}	 // deliver pt=1 cells regardless of thresholds
//    	             	      	 // and EPD? standard=0   
//    	 OUT=<suc>;
//
//    Commands:	see Multiplexer (mux.c)
//       ->SetPar(vci, delta, buff): set the weights for fair queueing and
//    	       the per-VC EPD threshold
//    	 ->SetCLP1threshEPD(vci,thr): set VCI specific CLP-1-EPD threshold,
//    	       performed independent of EPD_CLP1
//    	 ->ResetStat: Reset the statistics for CLR, CLR0, and CLR1
//    	 ->CLR(i): cell loss ratio of all cells of vc i
//    	 ->CLR0(i): cell loss ratio of CLP=0 cells of vc i
//    	 ->CLR0(i): cell loss ratio of CLP=1 cells of vc i
// 
//
//    This mux has evolved from clpmux and ewsxmux.
//    FAIR_CLP0: discarding of CLP=0, if buffer is above clp1thresh
//    	 and the VCI queue (regardless of CLP) is larger than 80% of its
//    	 fair share, EPD is performed (no discarding of partial frames)
//    	 may not be used together with FBA0 and DFBA
//    FBA0: as FAIR_CLP0, but in this case only CLP=0 cells are considered
//    	 for the VCI queue length
//    	 may not be used together with FAIR_CLP0
//    DFBA: may not be used together with FAIR_CLP0
//
//
///////////////////////////////////////////////////////////////////////////////

#include "muxwfqbuffman.h"

CONSTRUCTOR(MuxWFQBuffMan, muxWFQBuffMan);
USERCLASS("MuxWFQBuffMan", MuxWFQBuffMan);

/*
*	addpars(): add parameters to standard mux
*/
void	muxWFQBuffMan::addpars(void)
{
   int	i;

   // read out the queue length
   maxbuff = q.getmax() - 1;
   aktbuff = 0;

   epdThresh = read_int("EPDTHRESH");
   if (epdThresh <= 0)
      syntax0("invalid EPDTHRESH");
   skip(',');

   clp1Thresh = read_int("CLP1THRESH");
   if (clp1Thresh <= 0)
      syntax0("invalid CLP1THRESH");
   skip(',');

   // boolean, should we perform EPD/PPD for CLP=1 packets
   if(test_word("EPD_CLP1"))
   {
      epdClp1 = read_int("EPD_CLP1");
      if (epdClp1 < 0 || epdClp1 >1)
	 syntax0("EPD_CLP1 must be 0 or 1");
      skip(',');
   }
   else
      epdClp1 = 0;

   // perfrom DFBA?
   if(test_word("DFBA"))
   {
      performDFBA = read_int("DFBA");
      if (performDFBA < 0 || performDFBA >1)
	 syntax0("DFBA must be 0 or 1");
      skip(',');
   }
   else
      performDFBA = 0;

   if(performDFBA && !epdClp1)
      syntax0("to perform DFBA, EPD_CLP1 must be set to 1");

   // boolean, discard CLP0 if above trheshold
   if(test_word("FAIR_CLP0"))
   {
      fairClp0 = read_int("FAIR_CLP0");
      if (fairClp0 < 0 || fairClp0 >1)
	 syntax0("FAIR_CLP0 must be 0 or 1");
      skip(',');
   }
   else
      fairClp0 = 0;

   // boolean, discard CLP0 if above trheshold
   if(test_word("FBA0"))
   {
      fba0 = read_int("FBA0");
      if (fba0 < 0 || fba0 >1)
	 syntax0("FBA0 must be 0 or 1");
      skip(',');
   }
   else
      fba0 = 0;

   if(fba0 && fairClp0)
      syntax0("to perform FBA0, FAIR_CLP0 must be set to 0 (or vice versa)");   
   
   if(performDFBA && fairClp0)
      syntax0("to perform DFBA, FAIR_CLP0 must be set to 0 (or vice versa)");


   // read in RED parameter

   if(test_word("PERFORM_RED1"))
   {
      perform_RED1 = read_int("PERFORM_RED1");
      skip(',');
   }
   else
      perform_RED1 = 0;
   
   if(perform_RED1)  
   {

      red1.th_l = read_int("RED1_TH_LOW");	// lower threshold for performing RED
      if(red1.th_l < 0)
	 syntax0("RED1_TH_LOW must be >= 0");
      else if(red1.th_l >= q.getmax())
	 syntax0("RED1_TH_LOW must be < BUFF");
      skip(',');

      red1.th_h = read_int("RED1_TH_HIGH");	// lower threshold for performing RED
      if(red1.th_h <= red1.th_l)
	 syntax0("RED1_TH_HIGH be > RED1_TH_LOW");
      else if(red1.th_h > maxbuff)
	 syntax0("RED1_TH_HIGH must be <= BUFF");
      skip(',');

      red1.pmax = read_double("RED1_PMAX");	// lower threshold for performing RED
      if(red1.pmax < 0 || red1.pmax > 1)
	 syntax0("RED1_PMAX must be in [0,1]");
      skip(',');

      red1.pstart = read_double("RED1_PSTART");	// lower threshold for performing RED
      if(red1.pstart < 0 || red1.pstart > 1)
	 syntax0("RED1_PSTART must be in [0,1]");
      skip(',');

      if(test_word("RED1_USEAVERAGE"))
      {
	 red1.useaverage = read_int("RED1_USEAVERAGE");	// lower threshold for performing RED
	 if(red1.useaverage < 0 || red1.useaverage > 1)
	    syntax0("RED1_USEAVERAGE must be in [0,1]");
	 skip(',');
       }
       else
	  red1.useaverage = 1;

      if(test_word("RED1_WQ"))
      {
	 red1.w_q = read_double("RED1_WQ");
	 if(red1.w_q <= 0 || red1.w_q > 1)
	    syntax0("RED1_WQ must be in (0,1]");
	 skip(',');
       }
   }  // perform RED


   // boolean, should we always deliver pt=1 cells (if global buffer allows)
   if(test_word("DELIVER_PT1"))
   {
      deliverPt1 = read_int("DELIVER_PT1");
      if ( deliverPt1 < 0 || deliverPt1 >1)
	 syntax0("DELIVER_PT11 must be 0 or 1");
      skip(',');
   }
   else
      deliverPt1 = 0;

   // the connection parameters
   CHECK(cpar = new WFQBuffManConnParam* [max_vci]);
   for (i = 0; i < max_vci; ++i)
   {
      CHECK(cpar[i] = new WFQBuffManConnParam(i));
      cpar[i]->maxqlen_epd = maxbuff;
      cpar[i]->maxqlen1_epd = maxbuff;
   }


   // end of clpmux

   spacTime = 0;

   CHECK(qLenVCI = new int[max_vci]);
   CHECK(partab = new wfqpar* [max_vci]);
   for (i = 0; i < max_vci; i++)
   {
      partab[i] = NULL;
      qLenVCI[i] = 0;
   }
   

   
}

/*
*	Place incomming cells in the right buffer.
*	Two cases:
*	1.	There are still cells in the per-VC input queue. This means that
*		this VC is still enqueued in the sort queue. So we only have
*		to put the cell into the per-VC queue.
*	2.	The input queue of the VC is empty. This means that the VC currently
*		*not* is in the sort queue. So we have to include it at the right
*		position.
*/
void muxWFQBuffMan::late(event*)
{
   int	n, vc = 0;
   aal5Cell *pc = NULL;
   wfqpar *ppar = NULL;
   inpstruct *p;
   int toqueue;

   n = inp_ptr - inp_buff;
   while (n > 0)
   {
      if (n > 1)
      	 p = inp_buff + my_rand() % n;
      else
         p = inp_buff;

      toqueue = 0;   // think negative

      if (typequery(p->pdata, AAL5CellType))
      {
	 pc = (aal5Cell*) p->pdata;
	 vc = pc->vci;

	 if (vc < 0 || vc >= max_vci)
      	    errm1s1d("%s: cell with invalid VCI = %d received", name, vc);

	 if ((ppar = partab[vc]) == NULL)
      	    errm1s1d("%s: cell with unregistered VCI = %d received", name, vc);

      	 cpar[vc]->received++;
	 if(pc->clp == 0)
	    cpar[vc]->received0++;

	 // at the beginning of a burst: check whether to accept burst
	 if (cpar[vc]->vciFirst)
	 {
	    cpar[vc]->vciOK = TRUE;
	    cpar[vc]->vciFirst = FALSE;
	    if (aktbuff >= epdThresh || cpar[vc]->qlen >= cpar[vc]->maxqlen_epd)
	       cpar[vc]->vciOK = FALSE;

      	    // discard clp1-frames if queue is above the VCI threshold
	    if (pc->clp == 1 && cpar[vc]->qlen >= cpar[vc]->maxqlen1_epd)
	       cpar[vc]->vciOK = FALSE;


      	    // discard clp1-frames if queue is above the threshold
	    if (epdClp1 && pc->clp == 1 && aktbuff >= clp1Thresh)
	       cpar[vc]->vciOK = FALSE;
	    
	    if(perform_RED1 && pc->clp == 1)
	    {
	       int drop;
	       drop = red1.Update(aktbuff);
	       if(drop)
		  cpar[vc]->vciOK = FALSE;
	    }

      	    // discard clp0-frames if this vc uses more than its fair
	    // share
	    // all cells (including clp=1 cells are considered)
	    // this is the difference to fba0
	    if (fairClp0 &&  pc->clp == 0 && aktbuff >= clp1Thresh) // FBA with Z=0.8
	    {
	       if( cpar[vc]->qlen * (max_vci-1) / (double)aktbuff > 
	           0.8*(epdThresh)/(double)(aktbuff))
	       {
		  cpar[vc]->vciOK = FALSE;
	       }
	    }

      	    // FBA with Z=0.8 for CLP=0 cells
	    // the buffer between clp1Thresh and epdThresh is considered
	    // to be shared fair only, this is the difference to fairClp0
	    if (fba0 &&  pc->clp == 0 && aktbuff >= clp1Thresh) 
	    {
	       if( cpar[vc]->qlen0 * (max_vci-1) / (double)aktbuff > 
	           0.8*(epdThresh-clp1Thresh)/(double)(aktbuff-clp1Thresh))
	       {
		  cpar[vc]->vciOK = FALSE;
	       }
	    }
	    
	    // perform DFBA, note that DFBA uses per-VC weights
	    if(performDFBA &&pc->clp == 0 && aktbuff >= clp1Thresh &&
	          aktbuff < epdThresh)
	    {
	       if(cpar[vc]->qlen > (int)(aktbuff / (double)(max_vci-1)))
	       {
		  double p;
		  double X = aktbuff;
		  double Xi = cpar[vc]->qlen;
		  double Z = 1.0; //1.0 - (1.0/max_vci);
		  double a = 0.5;
		  //double rat = 1.0/max_vci;
		  double rat = cpar[vc]->SCR_ratio;
		  double L = clp1Thresh;
		  double H = epdThresh;

		  p = Z*(a*(Xi-X*rat)/(X*(1.0-rat)) +(1-a)*(X-L)/(H-L));

      	          // discard frame with the probability
		  if(uniform() <= p)
		    cpar[vc]->vciOK = FALSE; 
		     
	       }
	    
	    } // if DFBA is to be performed
	    
	    
	 }  // if first cell of a frame

	 if (pc->pt == 1)	    // next cell will be first cell
	    cpar[vc]->vciFirst = TRUE;

      	 // if no EPD for CLP=1 cells, then discard cells if above threshold
	 if(!epdClp1 && pc->clp == 1 && aktbuff > clp1Thresh)
	 {
	    cpar[vc]->vciOK = 0;
	 }

	 // Test Mue 18.02.2000 if (cpar[vc]->vciOK)
	 // accept if VC is ok or if a pt=1 cells (if option has been choosen)
	 if (cpar[vc]->vciOK || (pc->pt == 1 && deliverPt1))
	 {
	    if ( aktbuff >= maxbuff || ppar->q.getlen() >= ppar->q.getmax()-1) // buffer overflow
	    {
	       dropItem(p);
	       cpar[vc]->lost++;
	       if(pc->clp == 0)
	       {
	          cpar[vc]->lost0++;
	       }

	       cpar[vc]->vciOK = FALSE;   // drop the rest of the burst
      	    }
	    else  // succesful served
	    {
	    
	       toqueue = 1;   // queue it later
	       aktbuff++;
	    
	       cpar[vc]->served++;
	       cpar[vc]->qlen++;
	       if(pc->clp == 0)
	          cpar[vc]->qlen0++;
	    }
	 }
	 else
	 {
	    dropItem(p);
            cpar[vc]->lost++;
	    if(pc->clp == 0)
	    {
	       cpar[vc]->lost0++;
	    }
	 }

      }  // if - AAL-5 cell
      else
      {
	 errm1s1d("%s: sorry, can only serve AAL5 cells, other cell received on VCI %d", name, vc);
      	 
      	 //if (q.enqueue(p->pdata) == FALSE)
	 //   dropItem(p);
      }



      if (ppar->q.isEmpty() && toqueue)
      {
      
      	 // checking, if we find the item
	 //if(sortq.isQueued(ppar))
	 //   errm1s("%s: internal errror in late: should not be queued", name);
 
      
      	 // This means that this VC currently is not in the sort queue,
	 // the input buffer is empty. So we have to enqueue it.
      	 ppar->time = spacTime + ppar->delta;
	 sortq.enqTime(ppar);
      }

      // It is impossible that the enqueueing fails in case we just have put
      // the VC in the sort queue: in command(), we have ensured a queue limit of
      // at least one.
      if(toqueue)
      {
	 if ( !ppar->q.enqueue(pc))
	    errm1s1d("%s: internal errror: must drop cell from VC %d", name, vc);
	 else
      	    ++qLenVCI[vc];
      }

      *p = inp_buff[ --n];
   }

   inp_ptr = inp_buff;
   if ( !sortq.isEmpty())
      alarme( &std_evt, 1);

} // late()

/*
*	send a cell
*/
void muxWFQBuffMan::early(event *)
{
   wfqpar *p;
   cell *pc;

   // dequeue the VC from the sort queue, take a cell from the corresponding
   // queue and send the cell.
   p = (wfqpar *) sortq.dequeue();
   if(p == NULL)
      errm1s("%s: internal error in early: no item in sortqueue", name);

   spacTime = p->time;
   
   pc = (cell*) p->q.first();
   if(pc == NULL)
      errm1s("%s: internal error in early: no cell in per-vc queue", name);
   
   int vc = pc->vci;

   cpar[vc]->qlen--;
   if(pc->clp==0)
     cpar[vc]->qlen0--; 
   
   suc->rec(p->q.dequeue(), shand);
   --qLenVCI[p->vci];
   aktbuff--;
   if(aktbuff < 0)
      errm1s("%s: internal error in early: aktbuff < 0", name);

   if (spacTime > spacTimeMax)
      restim();	// reset the spacing time if too large

   // if the per-VC queue is not yet empty: enqueue it again
   if ( !p->q.isEmpty())
   {
      p->time = spacTime + p->delta;
      sortq.enqTime(p);
   }
}

/*
*	export the per-VC queue length and spacing time
*/
int muxWFQBuffMan::export(exp_typ *msg)
{
   return
      baseclass::export(msg) ||
      intArray1(msg, "QLenVCI", qLenVCI, max_vci, 0) ||
      intScalar(msg, "SpacingTime", (int *) &spacTime);
}

///////////////////////////////////////////////////////////////////////////////
// ResetStatVC(vc)
///////////////////////////////////////////////////////////////////////////////
void muxWFQBuffMan::ResetStatVC(int vc)
{
   if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: ResetVC with illegal VC=%d called", name, vc);

   cpar[vc]->received = 0;
   cpar[vc]->lost = 0;
   cpar[vc]->received0 = 0;
   cpar[vc]->lost0 = 0;
   cpar[vc]->served = 0;

} // ResetVC(vc)


/*
*	command:
*	mux->SetPar(vci, delta, buff)
*		// set the WFQ parameters of VC vci
*/
int muxWFQBuffMan::command(char *s,tok_typ *pv)
{
   int	vc;
   double clr;

   if (baseclass::command(s, pv))
	   return TRUE;

   pv->tok = NILVAR;
   if (strcmp(s, "SetPar") == 0) // for scheduling
   {
      skip('(');
      vc = read_int(NULL);
      if (vc < 0 || vc >= max_vci)
      	 syntax1d("VCI %d is out-of-range", vc);
      if (partab[vc] != NULL)
      	 syntax1d("VCI %d already has been initialized", vc);

      CHECK(partab[vc] = new wfqpar(vc));
      skip(',');
      if ((partab[vc]->delta = read_int(NULL)) < 1)
      	 syntax1d("invalid DELTA value: %d",partab[vc]->delta);
      skip(',');
      partab[vc]->q.setmax((int)1e6);  // big value
      cpar[vc]->maxqlen_epd = read_int(NULL);
      if ( cpar[vc]->maxqlen_epd < 1)
      	 syntax0("invalid per-VC EPD threshold");
      skip(')');
   }
   else if(!strcmp(s, "SetCLP1threshEPD")) // VCI-specific CLP-1-EPD threshold
   {
      skip('(');
      vc = read_int(NULL);
      if(vc < 0 || vc >= max_vci)
         syntax1s1d("%s: invalid VCI used: %d", name, vc);

      skip(',');
      cpar[vc]->maxqlen1_epd = read_int(NULL);
      if ( cpar[vc]->maxqlen1_epd < 1)
         syntax1s("%s: VCI-specific CLP1-TRHESHOLD must be > 0", name);

      skip(')');

   }
   else if(!strcmp(s, "SetSCR")) // for buffer management
   {
      skip('(');
      vc = read_int(NULL);
      if(vc < 0 || vc >= max_vci)
         syntax1s1d("%s: invalid VCI used: %d", name, vc);

      skip(',');
      double rate = read_double(NULL);
      if(rate <= 0)
         syntax1s("%s: SCR must be > 0", name);

      cpar[vc]->SCR = rate;
      
      skip(')');

   }
   else if(!strcmp(s, "ResetStat"))
   {
      for(vc=0; vc < max_vci; vc++)
         ResetStatVC(vc);

      return TRUE;

   }
   else if(!strcmp(s, "CLR"))
   {
      skip('(');
      vc = read_int(NULL);
      if(vc < 0 || vc >= max_vci)
         syntax1s1d("%s: invalid VCI used: %d", name, vc);

      if(cpar[vc]->received > 0)
         clr = cpar[vc]->lost / (double)cpar[vc]->received;
      else
         clr = 0.0;

      skip(')');

      pv->val.d = clr;
      pv->tok = DVAL;
      return TRUE;
   }
   else if(!strcmp(s, "CLR0"))
   {
      skip('(');
      vc = read_int(NULL);
      if(vc < 0 || vc >= max_vci)
         syntax1s1d("%s: invalid VCI used: %d", name, vc);

      if(cpar[vc]->received0 > 0)    
      	 clr = cpar[vc]->lost0 / (double)cpar[vc]->received0;
      else
      	 clr = 0.0;

      skip(')');

      pv->val.d = clr;
      pv->tok = DVAL;
      return TRUE;
   }
   else if(!strcmp(s, "CLR1"))
   {
      skip('(');
      vc = read_int(NULL);
      if(vc < 0 || vc >= max_vci)
         syntax1s1d("%s: invalid VCI used: %d", name, vc);

      if(cpar[vc]->received - cpar[vc]->received0 > 0)
      	 clr = (cpar[vc]->lost - cpar[vc]->lost0) / 
         (double)(cpar[vc]->received - cpar[vc]->received0);
      else
         clr = 0.0;

      skip(')');

      pv->val.d = clr;
      pv->tok = DVAL;
      return TRUE;
   }
   else
      return FALSE;

   return TRUE;
}

/*
*	SimTime has been reset or spacing time reached limit spacTimeMax:
*	Reset SpacTime
*/
void muxWFQBuffMan::restim(void)
{
   int i;

   for (i = 0; i < max_vci; ++i)
   {
      if (partab[i] == NULL || partab[i]->q.isEmpty())
      	 continue;

      if (partab[i]->time < spacTime)
         errm1s("%s: internal error on reset spacing time", name);

      partab[i]->time -= spacTime;
   }

   spacTime = 0;
}


///////////////////////////////////////////////////////////////////////////////
// connect
///////////////////////////////////////////////////////////////////////////////
void muxWFQBuffMan::connect(void)
{
   int i;
   //   int SCR_given = 0;

   baseclass::connect();

   // sum up all SCRs given
   reserved_SCR = 0.0;
   for(i=1;i < max_vci; i++)
   {
      double rate = cpar[i]->SCR;
      if(rate <= 0)
      	 errm1s1d("%s: SCR for connection %d not given",name,i);
      reserved_SCR += rate;
   }

   for(i=1;i < max_vci; i++)
   {
      cpar[i]->SCR_ratio = cpar[i]->SCR / reserved_SCR;
   }

} // muxWFQBuffMan::connect()
