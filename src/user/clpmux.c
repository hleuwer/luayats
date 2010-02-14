///////////////////////////////////////////////////////////////////////////////
// 
//    Multiplexer with EPD/PPD and CLP thresholds
//
//    MuxCLP mux: NINP=<int>, 	 // number of inputs
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
///////////////////////////////////////////////////////////////////////////////

#include "clpmux.h"

CONSTRUCTOR(MuxCLP, muxCLP);
USERCLASS("MuxCLP", MuxCLP);

// read additional (compared to mux) parameters
void muxCLP::addpars(void)
{
   int	i;

   baseclass::addpars();

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
      syntax0("to perform DFBA, FAIR_CLP0 must be set to 0");


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
      else if(red1.th_h > q.getmax())
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
   CHECK(cpar = new ClpMuxConnParam* [max_vci]);
   for (i = 0; i < max_vci; ++i)
   {
      CHECK(cpar[i] = new ClpMuxConnParam(i));
      cpar[i]->maxqlen_epd = q.getmax();
      cpar[i]->maxqlen1_epd = q.getmax();
   }

} // init()


///////////////////////////////////////////////////////////////////////////////
// late(event *)
// a cell has arrived
///////////////////////////////////////////////////////////////////////////////
void muxCLP::late(event*)
{
   int		n;
   inpstruct	*p;

   // process all arrivals in random order
   n = inp_ptr - inp_buff;
   while (n != 0)
   {
      if (n > 1)
      	 p = inp_buff + (my_rand() % n);
      else
      	 p = inp_buff;

      if (typequery(p->pdata, AAL5CellType))
      {
      	 int vc;
	 aal5Cell *pc;

	 vc = (pc = (aal5Cell *) p->pdata)->vci;

	 if (vc < 0 || vc >= max_vci)
	    errm1s2d("%s: AAL5 cell with illegal VCI=%d received on"
	       " input %d", name, vc, p->inp + 1);

      	 cpar[vc]->received++;
	 if(pc->clp == 0)
	    cpar[vc]->received0++;

	 // at the beginning of a burst: check whether to accept burst
	 if (cpar[vc]->vciFirst)
	 {
	    cpar[vc]->vciOK = TRUE;
	    cpar[vc]->vciFirst = FALSE;
	    if (q.getlen() >= epdThresh || cpar[vc]->qlen >= cpar[vc]->maxqlen_epd)
	       cpar[vc]->vciOK = FALSE;

      	    // discard clp1-frames if queue is above the VCI threshold
	    if (pc->clp == 1 && cpar[vc]->qlen >= cpar[vc]->maxqlen1_epd)
	       cpar[vc]->vciOK = FALSE;


      	    // discard clp1-frames if queue is above the threshold
	    if (epdClp1 && pc->clp == 1 && q.getlen() >= clp1Thresh)
	       cpar[vc]->vciOK = FALSE;

	    if(perform_RED1 && pc->clp == 1)
	    {
	       int drop;
	       drop = red1.Update(q.q_len);
	       if(drop)
		  cpar[vc]->vciOK = FALSE;
	    }

      	    // discard clp0-frames if this vc uses more than its fair
	    // share
	    // all cells (including clp=1 cells are considered)
	    // this is the difference to fba0
	    if (fairClp0 &&  pc->clp == 0 && q.getlen() >= clp1Thresh) // FBA with Z=0.8
	    {
	       if( cpar[vc]->qlen * (max_vci-1) / (double)q.getlen() > 
		   0.8*(epdThresh)/(double)(q.getlen())
		  )
	          cpar[vc]->vciOK = FALSE;
	    }
	    

      	    // FBA with Z=0.8 for CLP=0 cells
	    // the buffer between clp1Thresh and epdThresh is considered
	    // to be shared fair only, this is the difference to fairClp0
	    if (fba0 &&  pc->clp == 0 && q.getlen() >= clp1Thresh) 
	    {
	       if( cpar[vc]->qlen0 * (max_vci-1) / (double)q.getlen() > 
	           0.8*(epdThresh-clp1Thresh)/(double)(q.getlen()-clp1Thresh))
	       {
		  cpar[vc]->vciOK = FALSE;
	       }
	    }

	    
	    // perform DFBA, not that DFBA uses per-VC weights
	    if(performDFBA &&pc->clp == 0 && q.getlen() >= clp1Thresh &&
	          q.getlen() < epdThresh)
	    {
	       if(cpar[vc]->qlen > (int)(q.getlen() / (double)(max_vci-1)))
	       {
		  double p;
		  double X = q.getlen();
		  double Xi = cpar[vc]->qlen;
		  double Z = 1.0; //1.0 - (1.0/max_vci);
		  double a = 0.5;
		  //double rat = 1.0/max_vci;
		  double rat = cpar[vc]->SCR_ratio;
		  double L = clp1Thresh;
		  double H = epdThresh;

		  p = Z*(a*(Xi-X*rat)/(X*(1.0-rat)) + (1.0-a)*(X-L)/(H-L));
		  //printf("vc=%d, X=%f, Xi=%f, L=%f, H=%f, rat=%f\n",vc,X,Xi, L,H,rat);
      	          //printf("vc=%d, p=%f\n",vc,p);
      	          // discard frame with the probability
		  if(uniform() <= p)
		    cpar[vc]->vciOK = FALSE; 
		     
	       }
	    
	    } // if DFBA is to be performed

	 }  // if first cell of a frame

	 if (pc->pt == 1)	    // next cell will be first cell
	    cpar[vc]->vciFirst = TRUE;

      	 // if no EPD for CLP=1 cells, then discard cells if above threshold
	 if(!epdClp1 && pc->clp == 1 && q.getlen() > clp1Thresh)
	 {
	    cpar[vc]->vciOK = 0;
	 }

	 // Test Mue 18.02.2000 if (cpar[vc]->vciOK)
	 // accept if VC is ok or if a pt=1 cells (if option has been choosen)
	 if(cpar[vc]->vciOK || (pc->pt == 1 && deliverPt1))
	 {
	    if ( !q.enqueue(pc))    // buffer overflow
	    {
	       cpar[vc]->lost++;
	       if(pc->clp == 0)
	       {
	          cpar[vc]->lost0++;
	       }
	       dropItem(p);
	       cpar[vc]->vciOK = FALSE;   // drop the rest of the burst
      	    }
	    else  // succesful served
	    {
	       cpar[vc]->served++;
	       cpar[vc]->qlen++;
	       if(pc->clp == 0)
	          cpar[vc]->qlen0++;

	    }
	 }
	 else
	 {
            cpar[vc]->lost++;
	    if(pc->clp == 0)
	    {
	       cpar[vc]->lost0++;
	    }
      	    dropItem(p);
	 }

      }
      else
      {
      	 if (q.enqueue(p->pdata) == FALSE)
	    dropItem(p);
      }

      *p = inp_buff[--n];
   }
   inp_ptr = inp_buff;

   if (q.getlen() != 0)
	   alarme( &std_evt, 1);

} // late()


///////////////////////////////////////////////////////////////////////////////
// early(event *)
// serve one data item
///////////////////////////////////////////////////////////////////////////////
void muxCLP::early(event *)
{
   cell *pc;
   pc = (cell*) q.dequeue();
   int vc = pc->vci;

   cpar[vc]->qlen--;
   if(pc->clp==0)
     cpar[vc]->qlen0--; 

   suc->rec(pc, shand);
}


///////////////////////////////////////////////////////////////////////////////
// ResetStatVC(vc)
///////////////////////////////////////////////////////////////////////////////
void muxCLP::ResetStatVC(int vc)
{
   if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: ResetVC with illegal VC=%d called", name, vc);

   cpar[vc]->received = 0;
   cpar[vc]->lost = 0;
   cpar[vc]->received0 = 0;
   cpar[vc]->lost0 = 0;
   cpar[vc]->served = 0;

} // ResetVC(vc)


///////////////////////////////////////////////////////////////////////////////
// command()
///////////////////////////////////////////////////////////////////////////////
int muxCLP::command(char *s,tok_typ *pv)
{
   int vc;
   double clr;
   
   if (baseclass::command(s, pv))
      return TRUE;
   
   if(!strcmp(s, "SetCLP1threshEPD")) // VCI-specific CLP-1-EPD threshold
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
      return TRUE;

   }
   else if(!strcmp(s, "SetCLP0threshEPD")) // VCI-specific CLP-1-EPD threshold
   {
      skip('(');
      vc = read_int(NULL);
      if(vc < 0 || vc >= max_vci)
         syntax1s1d("%s: invalid VCI used: %d", name, vc);

      skip(',');
      cpar[vc]->maxqlen_epd = read_int(NULL);
      if ( cpar[vc]->maxqlen_epd < 1)
         syntax1s("%s: VCI-specific CLP0-TRHESHOLD must be > 0", name);

      skip(')');
      return TRUE;

   }
   else if(!strcmp(s, "SetSCR"))
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

      pv->tok = NILVAR;
      return TRUE;
   }
   else if(!strcmp(s, "ResetStat"))
   {
      for(vc=0; vc < max_vci; vc++)
         ResetStatVC(vc);

      pv->tok = NILVAR;
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

   
   return FALSE;

}  // muxCLP::command

///////////////////////////////////////////////////////////////////////////////
// connect
///////////////////////////////////////////////////////////////////////////////
void muxCLP::connect(void)
{
   int i;
   int SCR_given = 0;

   baseclass::connect();

   // check, if there are SCRs given
   for(i=1;i < max_vci; i++)
      if(cpar[i]->SCR > 0)
         SCR_given = 1;
   
   if(SCR_given)
   {
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
	 cpar[i]->SCR_ratio = cpar[i]->SCR / reserved_SCR;

   }
   else  // nothing given
   {
      reserved_SCR = 1.0;
      for(i=1;i < max_vci; i++)
      {
         cpar[i]->SCR_ratio = 1.0/max_vci;
         cpar[i]->SCR = 1.0/max_vci;	 
      }
   }


} // muxCLP::connect()



