///////////////////////////////////////////////////////////////////////////////
// Multiplexer MuxEWSX
// 
// Command Line:
//    MuxEWSX name: NINP=int 	// number of inputs
//       {, MAXVCI=int}		// number of VCI's - opt., default=NINP
//       , EWSXCONTR=module	// EWSX Control Module 
//       {, DELTA=double}	// delta of output, opt., default=1.0
//       {, BITRATE=double}	// available Bandwidth, opt., default=149.76*1e6
//    	 {, PRIOCLPEPD=0|1}   	// experimental, if set, UBR cells with pt0 are 
//     	             	      	// not subject forEPD, default: not set
//    	 {, EPD_CLP1=0|1}	// experimental, if set, EPD is performed for 
//    	             	        // VBR CLP=1 cells, if buff is above CLP=1 threshold
//     	             	      	// default: not set			 
//    	 {... ABR parameters}	// ABR parameters (optional, but must be used if
//				// if ABR traffic is present!)
//       OUTBACK=module,	// backward output
//       OUT=module;		// normal output
//
// Commands:
//   ->Losses(lo,up)		// Losses from input lo to up
//   ->LossesInp(lo,up)		// see Losses()
//   ->LossesVCI(lo,up)		// Losses from vci lo to up
//   ->ResLoss			// resets loss counter
//   ->SetConn(vci, conntype, bitrate, reservedbuffer)
//    	 sets connectiontype, bitrate and the reserved buffer of connection vci
//    	 this must be called for every connection, can be called several times
//   ->SetAbrFrtt(vci,frtt): sets the fixed round trip time (in seconds) for
//    	 this ABR vci
//   ->SetABRImediateBackConn(vci)
//     	 sets a backward ABR connection with vci, which is connected to the
//    	 ABR connection, which was set as last connection
//   ->SetConnEPD(vci, thresh)	
//    	 connection specific EPD threshold, frames are thrown
//    	 away, if more than thresh cells of this conn. are in the mux
//   ->SetConnCLPThresh(vci, thresh)
//       if more than thresh cells of one connection are in the mux, new cells
//    	 with clp=1 are discarded
//   ->SetConnPPD(vci, ppd)	// Switch ppd on/off with ppd=1/0
//    	 standard is 1=on
///////////////////////////////////////////////////////////////////////////////

#include "ewsx.h"

CONSTRUCTOR(MuxEWSX, muxEWSX);
USERCLASS("MuxEWSX", MuxEWSX);

///////////////////////////////////////////////////////////////
// void muxEWSX::init()
// read in the command line arguments
//////////////////////////////////////////////////////////////
void   muxEWSX::init(void)
{	
   int i;
   char *s;
   root *obj;
   
   skip(CLASS);
   name = read_id(NULL);
   skip(':');
   ninp = read_int("NINP");
   if (ninp < 1)
      syntax0("invalid NINP");
   skip(',');
 
   if (test_word("MAXVCI"))
   {
      max_vci = read_int("MAXVCI") + 1;
      if (max_vci < 1)
         syntax0("invalid MAXVCI");
      skip(',');
   }
   else
      max_vci = ninp + 1;
 
   s = read_suc("EWSXCONTR");
   if ((obj = find_obj(s)) == NULL)
      syntax2s("%s: could not find object `%s'", name, s);
   ewsx = (controlEWSX*) obj;
   skip(',');

   if (test_word("DELTA"))
   {
      outdelta = read_double("DELTA");
      if(outdelta < 1)
         syntax0("DELTA must be >= 1");
      skip(',');
   }
   else
      outdelta = 1.0;

   if (test_word("BITRATE"))
   {
      MaxBandwidth = read_double("BITRATE");
      if(MaxBandwidth <= 0)
         syntax0("BITRATE must be > 0");
      skip(',');
   }
   else
      MaxBandwidth = 149.76 * 1e6;	// STM1
   
   if (test_word("PRIOCLPEPD"))
   {  
      PrioClpEpd = read_int("PRIOCLPEPD");
      PrioClpEpd = 1;
      skip(',');
   }
   else
      PrioClpEpd = 0;

   if(test_word("EPD_CLP1"))
   {
      epdClp1 = read_int("EPD_CLP1");
      if (epdClp1 < 0 || epdClp1 >1)
	 syntax0("EPD_CLP1 must be 0 or 1");
      skip(',');
   }
   else
      epdClp1 = 0;

      
   // initialize ABR
   if (test_word("HI_THRESH"))   
      AbrInit();
	
   // for getting the information from backward ABR RM cells
   output("OUTBACK",SucBACK);
   skip(',');
   output("OUT",SucData);
   
   // generate the inputs
   input("BACK",InpBACK);
   inputs("I",     // input names: ...->I[inp_no]
          ninp,   // ninp inputs
          -1);    // input 1 has input key 0

   CHECK(lost = new unsigned int[ninp]);
   for (i = 0; i < ninp; ++i)
      lost[i] = 0;
   CHECK(lostVCI = new unsigned int[max_vci]);
   for (i = 0; i < max_vci; ++i)
      lostVCI[i] = 0;
   lossTot = 0;
 
   CHECK(inp_buff = new inpstruct[ninp]);
      inp_ptr = inp_buff;

   // the connection parameter
   CHECK(connpar = new EWSXConnParam* [max_vci]);
   for (i = 0; i < max_vci; ++i)
      CHECK(connpar[i] = new EWSXConnParam(i));
   
} // muxEWSX::init(void)

///////////////////////////////////////////////////////////////
// void muxEWSX::initAbr()
// read in the command line arguments for ABR
//////////////////////////////////////////////////////////////
void   muxEWSX::AbrInit(void)
{		

   if (test_word("HI_THRESH"))
   {
      abr.abr_q_hi = read_int("HI_THRESH");
      if (abr.abr_q_hi < 1)
         syntax0("invalid HI_THRESH");
      skip(',');
   }
   else
      abr.abr_q_hi = 0;

   if (test_word("LO_THRESH"))
   {	abr.abr_q_lo = read_int("LO_THRESH");
	   if (abr.abr_q_lo > abr.abr_q_hi)
		   syntax0("invalid LO_THRESH");
	   skip(',');
   }
   else
      abr.abr_q_lo = abr.abr_q_hi;
      
   abr.abr_congested = FALSE;

   abr.TBE = read_int("TBE");
   if (abr.TBE < 1)
      syntax0("invalid TBE");
   skip(',');

   abr.AI_tim = read_int("AI");
   if (abr.AI_tim < 1)
      syntax0("invalid AI");
   skip(',');
   abr.cnt_load_abr = 0;
   abr.Z = 0.0;
   abr.ABRRate = 0.0;

   if (test_word("CBRI"))
   {
      abr.CVBR_load_tim = read_int("CBRI");
      if (abr.CVBR_load_tim < 1)
      	 syntax0("invalid CBRI");
      skip(',');
   }
   else	
      abr.CVBR_load_tim = abr.AI_tim;

   abr.cnt_load_cvbr = 0;
   abr.CVBRRate = 0.0;

   if (test_word("ZOL"))
   {
      abr.Zol = read_double("ZOL");
      if (abr.Zol <= 1.0)
      	 syntax0("invalid ZOL");
      skip(',');
   }
   else	
      abr.Zol = 1e3;

   if (test_word("LINKCR"))
   {
      abr.LinkRate = read_double("LINKCR");
      if (abr.LinkRate <= 0.0)
      	 syntax0("invalid LINKCR");
      skip(',');
   }
   else	
      abr.LinkRate = 353207.55;	// 149.76 Mbit / s

   abr.TargetUtil = read_double("TARGUTIL");
   if (abr.TargetUtil <= 0.0 || abr.TargetUtil > 1.0)
      syntax0("invalid TARGUTIL");
   abr.TargetRate = abr.LinkRate * abr.TargetUtil;
   skip(',');

   if (test_word("DYNFAIRSHARE"))
   {
      skip_word("DYNFAIRSHARE");
      skip(',');
      abr.useNactive = TRUE;
   }
   else
      abr.useNactive = FALSE;

   if (test_word("BINMODE"))
   {
      skip_word("BINMODE");
      skip(',');
      abr.binMode = TRUE;
   }
   else
      abr.binMode = FALSE;

   abr.Nabr = 0;
   abr.Nactive = 0;

   alarml( &ABR_load_evt, abr.AI_tim);
   alarml( &CVBR_load_evt, abr.CVBR_load_tim);
   
} // muxEWSX::initAbr(void)


///////////////////////////////////////////////////////////////
// void muxEWSX::REC()
// cells are received
//////////////////////////////////////////////////////////////
rec_typ muxEWSX::REC(data *pd,int i)
{
   int vc, vc_forw;
   ABRstruct* ptr;

   ///////////////////////////
   // backward cells
   ///////////////////////////
   
   if (i == InpBACK) // backward input ?
   {
      if(typequery(pd, RMCellType)) // an RM Cell ?
      {
         rmCell *pc;
	 pc = (rmCell*) pd;
      	 if(pc->DIR == 1)// a backward RM Cell
	 {
	    if ((vc = pc->vci) < 0 || vc >= max_vci)
	       errm1s1d("%s: VCI of backward RM cell out of range, "\
	          "VCI = %d", name, vc);
	    
	    if ((vc_forw = connpar[vc]->AbrVciData) == 0)
      	       errm1s1d("%s: backward RM cell with VCI = %d received: "
		     	"no backward connection established", name, vc);
	    if(vc_forw < 0 || vc_forw >= max_vci)
	       errm1s1d("%s: internal error: backward RM cell pointed to "\
	             	"Data connection with VCI = %d", name, vc_forw);

   	    if((ptr = connpar[vc_forw]->AbrInfo)==NULL)
      	       errm1s1d("%s: internal error: forward ABR connection with "\
	             	"VCI=%d does not exist",name,vc_forw);
	    
	    //	compute latest ER
	    if (abr.binMode == FALSE)
	    {	AbrCompERICA(ptr);
		    ptr->backwER = pc->ER;
		    if (ptr->ER < pc->ER)
			    pc->ER = ptr->ER;
	    }

	    AbrCompBinary(pc);	 // perform binary feedback

	 } // a backward RM Cell
      
      } // an RM Cell

      return sucs[SucBACK]->rec(pd, shands[SucBACK]);
      
   }  // backward input ?

   ///////////////////////////
   // forward cells
   ///////////////////////////

   typecheck_i(pd, CellType, i);
   cell	*pc = (cell *) pd;
   vc = pc->vci;

   if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: cell with invalid VCI = %d received", name, vc);

   //	perform load measurement
   if (connpar[vc]->AbrInfo!=NULL)
   {
      // an ABR cell
      abr.cnt_load_abr++;
      connpar[vc]->AbrInfo->active = TRUE;   // for calculation of Nactive
      
      if (typequery(pc, RMCellType))
      {
         if(((rmCell *)pc)->DIR == 1)
      	    errm1s1d("%s: backward RM cell arrived on VCI=%d",name,vc);
	 
         connpar[vc]->AbrInfo->forwER = ((rmCell *)pc)->ER;
	 connpar[vc]->AbrInfo->CCR = ((rmCell *)pc)->CCR;
	 
      } // RM Cell
   }
   else
   {
      abr.cnt_load_cvbr++;
   }  // a non-ABR cell


   inp_ptr->inp = i;
   (inp_ptr++)->pdata = pd;
   if (!alarmed_late)
   {
      alarmed_late = TRUE;
      alarml( &evtLate, 0);
   }
   return ContSend;
}

///////////////////////////////////////////////////////////////
// void muxEWSX::late()
// now the arrived cells are in the inp_buff and have to be
// served
//////////////////////////////////////////////////////////////
void   muxEWSX::late(event *evt)
{
   int      n, vc;
   inpstruct   *p;
   cell   *pc;
   int todrop;
   EWSXConnParam *cpar;

   switch (evt->key) {
   case keyLoadABR:	// ABR load measurement
      // Z = (ABR load) / (available capacity)
      if (abr.CVBRRate >= abr.TargetRate)
         abr.Z = abr.Zol;	// nothing left for ABR
      else
         abr.Z = (abr.cnt_load_abr / (double) abr.AI_tim)
      	          / ((abr.TargetRate - abr.CVBRRate) / abr.LinkRate);
      abr.ABRRate = abr.LinkRate * abr.cnt_load_abr / (double) abr.AI_tim;
      abr.cnt_load_abr = 0;

      if (abr.useNactive)
      {
         int i;
	 abr.Nactive = 0;
	 for(i=0;i<max_vci;i++)
	 {
	    if(connpar[i]->AbrInfo != 0 &&
	       connpar[i]->AbrInfo->active)
	    {
	       abr.Nactive++;
	       connpar[i]->AbrInfo->active = FALSE;
	    }
	 }
      } // if - useNactive

      alarml( &ABR_load_evt, abr.AI_tim);
      return;
   case keyLoadCVBR:	// non-ABR load measurement
	   abr.CVBRRate = abr.LinkRate * abr.cnt_load_cvbr / 
	    (double) abr.CVBR_load_tim;
	   abr.cnt_load_cvbr = 0;
	   alarml( &CVBR_load_evt, abr.CVBR_load_tim);
	   return;
   case keyQueue:		// queue arrived cells
	   break;	// continuation below
   default:errm1s("internal error: %s: muxEWSX::late(): invalid event key",
      	          name);
   } // switch

   /////////////////////////////////////////////////////////
   // serve cells
   /////////////////////////////////////////////////////////
   alarmed_late = FALSE;	// I am not alarmed anymore

   n = inp_ptr - inp_buff;	// number of cells to serve
   while (n > 0)
   {	
      if (n > 1)
         p = inp_buff + my_rand() % n;
      else
         p = inp_buff;
         
      pc = (cell *) p->pdata;
      vc = pc->vci;
      // the check, if this VCI is valid is done in REC()
      cpar = connpar[vc];

      //////////////////////////////////////////////////
      // check, if cells have to be dropped
      //////////////////////////////////////////////////
      todrop = 0;	// think positive!
      
      if(  *pqlen >= maxbuff)	// no buffer available
      {
         todrop = 1;
      
	 // try to push out other cells with less priority (only for CLP=0 cells)
	 // CLP=0 added 24.07.1998
	 if(connpar[vc]->ServiceClass == VBR && p->pdata->clp==0)
	 {
            ((controlEWSX*)ewsx)->pushout();
            if(  *pqlen < maxbuff)	// buffer available now
               todrop = 0;
	 }
      } // if - no buffer available
      
      // threshold for CLP=1 cells reached?, then drop then
      if( connpar[vc]->ServiceClass == VBR && p->pdata->clp==1 )
      {
         if(*pqlen >= clpThreshGlobal || cpar->qLenVCI >= cpar->clpThreshVCI)
	 {
	    if(!epdClp1)   // if no EPD for CLP=1 cells
               todrop = 1;
	    else     	   // perform EPD for CLP=1 cells
	    {
	       if(connpar[vc]->vciFirst)
	          todrop = 1;
	    }
	 }
      }
      
      // try to push out UBR, if AAL-5 threshold is reached
      // added 24.07.1998
      if( connpar[vc]->ServiceClass == VBR && p->pdata->clp==0 )
      {
	 if(connpar[vc]->vciFirst && *pqlen >= epdThreshGlobal)
	 {
	    todrop = 1;
            ((controlEWSX*)ewsx)->pushout();
            if(  *pqlen < epdThreshGlobal)	// buffer available now
               todrop = 0;
	 } // if - first cell of a packet?
      }
      
      // check, whether EPD or PPD can accept the cell
      if( !todrop  && typequery(p->pdata, AAL5CellType) )
      {
         aal5Cell *pc5;
	 pc5 = (aal5Cell*) p->pdata;
	 
         if(!PrioClpEpd)
	    todrop = check_drop_aal5(p->pdata, cpar);
	 else
	 {
            if( connpar[vc]->ServiceClass == UBR && p->pdata->clp==0 &&
	         pc5->pt==0)
	       ;
	    else
               todrop = check_drop_aal5(p->pdata, cpar);
	 }
      }

      //////////////////////////////////////////////////
      // drop or serve the cell
      //////////////////////////////////////////////////
      if(todrop)
      {
	 //if(p->pdata->clp == 0 && cpar->vciOK == TRUE)
	 //if(p->pdata->clp == 0)
	 //   printf("drop clp=0 (VCI=%d, QLEN=%d)\n",vc, *pqlen);

         if(typequery(p->pdata, AAL5CellType) )
            cpar->vciOK = FALSE;

            
         dropItem(p);
      }
      else
      {
         // if the VC-Queue is empty, this VC is not in the sort queue 
         if (cpar->q_vc.isEmpty())
         {
            // cpar->time = spacTime + cpar->delta;
            cpar->vtime = spacTime + cpar->vdelta;
            cpar->time = (time_t) cpar->vtime;
            q.enqTime(cpar);
         }
         
	 if (typequery(pc, RMCellType))
	 {
	    cell *pc_help;

	    // search the last RM Cell and queue in behind
	    pc_help = (cell*) cpar->q_vc.first();
	    if(pc_help == NULL)
	       cpar->q_vc.enqueue(pc); // empty queue
	    else
	    {
	       // search where to enqueue
	       while(pc_help != NULL && typequery(pc_help, RMCellType))
	          pc_help = (cell*) cpar->q_vc.sucOf(pc_help);

	       if(pc_help == NULL) // there are only RMCells in the queue
	          cpar->q_vc.enqueue(pc);
	       else
	          cpar->q_vc.enqPrec(pc,pc_help);

	    } // else - there are cells in the queue
	 } // if - if RMCell
	 else
	    cpar->q_vc.enqueue(pc);
	 
	 (cpar->qLenVCI)++;
	 (*pqlen)++;
	 qlen_local++;
	 
      } // else - not to drop

      *p = inp_buff[ --n];
      
   } // while(n>0) - as long as there are cells

   inp_ptr = inp_buff;
   if ( !q.isEmpty() )
      if(!alarmed_early)
      {
         time_t delta;
         delta = calc_alarm_time();
         alarme( &std_evt, delta); 
         alarmed_early = TRUE;
      }
   

} // muxEWSX::late(event *)


//////////////////////////////////////////////////////////////
// int muxEWSX::check_drop_aal5(data *pd, EWSXConnParam *cpar)
// checks, if the AAL-5 cell has to be dropped because of EPD,
// the queue-length is not checked
// return-value:1 - Zell has to be dropped
//		0 - Zell has not to be dropped
//////////////////////////////////////////////////////////////
inline int muxEWSX::check_drop_aal5(data *pd, EWSXConnParam *cpar)
{
   aal5Cell* aal5pc;
   int todrop;
   
   todrop = 0;       	      // think positive
   aal5pc = (aal5Cell*) pd;
   
   // first cell of a packet?
   if(cpar->vciFirst)
   {
      cpar->vciFirst = FALSE; // the next is not the first anymore

      if ( *pqlen >= epdThreshGlobal || // changed 24.07.1998 - differentiating UBR, VBR and CLP added
           (cpar->qLenVCI >= cpar->epdThreshVCI && cpar->ServiceClass == UBR ) ||
	   (cpar->qLenVCI >= cpar->epdThreshVCI && cpar->ServiceClass == VBR && aal5pc->clp == 1)
	 )
      {
         cpar->vciOK = FALSE;
	 todrop = 1;
      }
   
   } // if - first cell of a packet?
   
   // a pt-1 cell   
   if (aal5pc->pt == 1)
   {
      cpar->vciFirst = TRUE;  // the next cell is the first in a packet
      cpar->vciOK = TRUE;     // tranport this cell (and the following)
      todrop = 0;    	     
         
   } // if - pt=1 cells (last cll of aal5-packet)
   else
   {
      if (! cpar->vciOK && cpar->ppd )
         todrop = 1;	// drop cells of destroyed packets (if not pt=1)
         
   } // else - pt=0 cells
         
      
   return(todrop);

} // muxEWSX::check_drop_aal5()


/*
*   in serving state:
*      Forward a data item to the successor.
*      Register again, if more data queued.
*   in sync state:
*      take data from queue and enter serving state
*/
void   muxEWSX::early(   event   *)
{
   EWSXConnParam *p;
   //data *pc;
   cell *pc;
   int vc;
   
   // dequeue the VC from the sort queue, take a cell from the corresponding
   // queue and send the cell.
   p = (EWSXConnParam *) q.dequeue();
   if(!p) // changed 24.07.1998 - this may happen, when all cells are discarded by VQD
   {
      alarmed_early = FALSE;	// there is nothing to serve
      return;
   }

   spacTime = p->time;		// #### time oder vtime???
   pc = (cell*) p->q_vc.dequeue();

   if(!pc)
      errm1s("%s: early(): internal Problem 3: cant dequeue item from connection queue", name);

   vc = pc->vci;

   
   // forward RM cell ?
   if (typequery(pc, RMCellType) && ((rmCell *)pc)->DIR == 0)
   {
      rmCell *pcrm;
      ABRstruct	*ptr;

      pcrm = (rmCell *) pc;
      ptr = connpar[vc]->AbrInfo;
      if(ptr == NULL)
         errm1s1d("%s: early(): no ABR information for VCI=%d", name, vc);

      // compute ER
      if (abr.binMode == FALSE)
      {	
         AbrCompERICA(ptr);
	 ptr->forwER = pcrm->ER;
	 if (pcrm->ER > ptr->ER)
	    pcrm->ER = ptr->ER;
      }

      // perform binary feedback
      AbrCompBinary(pcrm);
   } // if - forward RM cell


   sucs[SucData]->rec(pc, shands[SucData]);
   (connpar[p->vci]->qLenVCI)--;
   (*pqlen)--;
   qlen_local--;

   if (spacTime > spacTimeMax)
      restim();	// reset the spacing time if too large
      
   // if the per-VC queue is not yet empty: enqueue it again
   if (!p->q_vc.isEmpty())
   {	
      //p->time = spacTime + p->delta;
      p->vtime += p->vdelta;
      p->time = (time_t) p->vtime;
      q.enqTime(p);
   }   
   
   if (!q.isEmpty())
   {
      time_t delta;
      delta = calc_alarm_time();
      alarme( &std_evt, delta);	// alarm for the next slot
      alarmed_early = TRUE;
   }
   else
      alarmed_early = FALSE;	// there is nothing to serve


} // muxEWSX::early()


//////////////////////////////////////////////////////////////
// time_t muxEWSX::calc_alarm_time(void)
// calculates the next time, when it is possible to alarm in
// the early phase to send cells.
// The value TimeSendNext (which stores the next possible time
// to send a cell, double) is updated, so it is assumed, that
// the cell is really sent after calling this function
// return-value: delta. when to alarm in the early phase
//////////////////////////////////////////////////////////////
inline time_t muxEWSX::calc_alarm_time(void)
{

   time_t delta;
   delta = 1;

   if(SimTime+1 >= TimeSendNext)	// i am allowed to send NOW
   {
   
      TimeSendNext = SimTime + 1 + outdelta;
      
   } // if - i am allowed to send NOW
   else				// i must send later
   {
      delta = ((time_t) ceil(TimeSendNext) ) - SimTime;
      
      /* Optimization: should never happen Mue 09.06.98
      if(delta <= 0)
         errm1s1d("%s: internal Problem: delta=%d, must be >0\n", name, delta);
      */
      
      TimeSendNext += outdelta;

   } // else - i must send later
   
   return(delta);

} // muxEWSX::calc_alarm_time(void)


//////////////////////////////////////////////////////////////
// int muxEWSX::export()
//////////////////////////////////////////////////////////////
int muxEWSX::export(exp_typ   *msg)
{
   return baseclass::export(msg) ||
      intScalar(msg, "QLen", (int *) &qlen_local) ||
      doubleScalar(msg, "Z", &abr.Z) || 
      doubleScalar(msg, "CRABR", &abr.ABRRate) ||     
      intScalar(msg, "LossTot", (int *) &lossTot) ||
      intArray1(msg, "Loss", (int *) lost, ninp, 1) ||
      intArray1(msg, "LossInp", (int *) lost, ninp, 1) ||
      intArray1(msg, "LossVCI", (int *) lostVCI, max_vci, 0);
      
} // muxEWSX::export()


//////////////////////////////////////////////////////////////
// int muxEWSX::command()
// the commands:
//    - SetConnWFQ -> initial setting of a connection, the
//         connection weight (relative) has to be set
//    - SetConnClass -> Sets the service class
//    - SetConnEPD -> Sets a connection specific EPD level
/////////////////////////////////////////////////////////////
int muxEWSX::command(char *s,tok_typ *pv)
{
   int vc;	
   int lo, up;
   int i;

   
   if (baseclass::command(s, pv))
      return TRUE;
   
   pv->tok = NILVAR;

   if (strcmp(s, "Losses") == 0 || strcmp(s, "LossesInp") == 0)
   {
      skip('(');
      lo = read_int(NULL);
      skip(',');
      up = read_int(NULL);
      skip(')');
      --lo;
      --up;
      if (lo < 0 || up >= ninp)
         syntax1s1d("%s: input numbers range from 1 to %d", name, ninp);

      pv->val.i = 0;
      for (i = lo; i <= up; ++i)
         pv->val.i += lost[i];
      pv->tok = IVAL;
      return	TRUE;
   }
   else if (strcmp(s, "LossesVCI") == 0)
   {
      skip('(');
      lo = read_int(NULL);
      skip(',');
      up = read_int(NULL);
      skip(')');
      if (lo < 0 || up >= max_vci)
         syntax1s1d("%s: VC numbers range from 0 to %d", name, max_vci - 1);

      pv->val.i = 0;
      for (i = lo; i <= up; ++i)
         pv->val.i += lostVCI[i];
      pv->tok = IVAL;
      return	TRUE;
   }
   else if (strcmp(s, "ResLoss") == 0)
   {
      for (i = 0; i < ninp; ++i)
         lost[i] = 0;
      for (i = 0; i < max_vci; ++i)
         lostVCI[i] = 0;
      lossTot = 0;
      pv->tok = NILVAR;
      return TRUE;
   }
   else if(strcmp(s, "SetConn") == 0)
   {
      ServiceClassType serviceclass;
   
      if(connected)
         syntax0("Using this command is not allowed after connecting");
      
      skip('(');
      vc = read_int(NULL);
      if (vc < 0 || vc >= max_vci)
         syntax1d("VCI %d is out-of-range", vc);
      if ( connpar[vc] == NULL)
         CHECK(connpar[vc] = new EWSXConnParam(vc));
      skip(',');

      // Connection-Class
      serviceclass = (ServiceClassType) read_int(NULL);
      
      if(! (serviceclass == UBR ||
            serviceclass == VBR ||
	    serviceclass == ABR ||
	    serviceclass == ABRBRMC
	    )
	 )
         syntax0("only UBR and VBR service-class is implemented");

      connpar[vc]->ServiceClass = serviceclass;
		
      skip(',');

      // WFQ-Delta
      double old_delta;
      old_delta = connpar[vc]->vdelta;
      if ((connpar[vc]->vdelta = read_double(NULL)) < 1)
         syntax0("invalid DELTA value");
      // here vdelta is still the rate!
      ReservedBandwidth += connpar[vc]->vdelta - old_delta;
      if(ReservedBandwidth > MaxBandwidth)
         syntax0("in summary asked for more Bandwidth than available");
      skip(',');

      // reserved Buffer
      int old_reserved = connpar[vc]->ReservedBuff;
      if (( connpar[vc]->ReservedBuff = read_int(NULL)) < 0)
         syntax0("invalid value for reserved Buffer");
      
      ReservedBuff += connpar[vc]->ReservedBuff - old_reserved;
       
      skip(')');

      if(AbrVciLast != -1)	// I wait for the establ. of a Backw. conn
         syntax0("the establishment of a BRMC connection on backward \
	 is expected now");
	 
		
      if(serviceclass == ABR)
      {
	 CHECK(connpar[vc]->AbrInfo = new ABRstruct);
	 AbrVciLast = vc; // need the vci for the backward direction
	 connpar[vc]->AbrInfo->MCR = connpar[vc]->vdelta / 53.0 / 8;
	 connpar[vc]->AbrInfo->CCR = connpar[vc]->AbrInfo->MCR;
      }

   }
   else if(strcmp(s, "SetABRImediateBackConn") == 0)
   {
      if(connected)
         syntax0("Using this command is not allowed after connecting");
      
      skip('(');
      vc = read_int(NULL);
      if (vc < 0 || vc >= max_vci)
         syntax1d("VCI %d is out-of-range", vc);
      if ( connpar[vc] == NULL)
         CHECK(connpar[vc] = new EWSXConnParam(vc));

      skip(')');

      if(AbrVciLast == -1)
	 syntax0("directly before establishing a BRMC connection "\
	         "the establishment of a ABR data connection is expected");

      connpar[vc]->AbrVciData = AbrVciLast;
      
      //printf("%s connection pair: ABR=%d BRMC=%d\n",name, AbrVciLast, vc);
		
      AbrVciLast = -1;

   }   
   else if (strcmp(s, "SetAbrFrtt") == 0)
   {
      double frtt;
      double ICR;
      
      skip('(');
      vc = read_int(NULL);
      if (vc < 0 || vc >= max_vci)
         syntax1d("VCI %d is out-of-range", vc);
      if ( connpar[vc] == NULL)
         syntax1d("VCI %d not initialized", vc);
      if ( connpar[vc]->AbrInfo == NULL)
         syntax1d("ABR info of VCI %d not initialized", vc);

      skip(',');

      frtt = read_double(NULL);
      if(frtt < 0)
         syntax0("FRTT must be >= 0");

      skip(')');
      
      ICR = abr.TBE / frtt;
      connpar[vc]->AbrInfo->forwER = ICR;
      connpar[vc]->AbrInfo->CCR = ICR;
      connpar[vc]->AbrInfo->backwER = ICR;
            
      abr.Nabr++;
      AbrCompERICA(connpar[vc]->AbrInfo);
      abr.Nabr--;
           
   }
   else if (strcmp(s, "SetConnEPD") == 0)
   {
      if(connected)
         syntax0("Using this command is not allowed after connecting");
      
      skip('(');
      vc = read_int(NULL);
      if (vc < 0 || vc >= max_vci)
         syntax1d("VCI %d is out-of-range", vc);
      if ( connpar[vc] == NULL)
         syntax1d("VCI %d not initialized", vc);
      skip(',');

      connpar[vc]->epdThreshVCI = read_int(NULL);
      if(connpar[vc]->epdThreshVCI < 0)
         syntax0("connection specific EPD threshold must be >= 1");

      skip(')');      
   }
   else if (strcmp(s, "SetConnCLPThresh") == 0)
   {
      if(connected)
         syntax0("Using this command is not allowed after connecting");
      
      skip('(');
      vc = read_int(NULL);
      if (vc < 0 || vc >= max_vci)
         syntax1d("VCI %d is out-of-range", vc);
      if ( connpar[vc] == NULL)
         syntax1d("VCI %d not initialized", vc);
      skip(',');

      connpar[vc]->clpThreshVCI = read_int(NULL);
      if(connpar[vc]->clpThreshVCI < 0)
         syntax0("connection specific threshold for dropping CLP=1 "\
	         "cells must be >= 1");

      skip(')');      
   }
   else if (strcmp(s, "SetConnPPD") == 0)
   {
      if(connected)
         syntax0("Using this command is not allowed after connecting");
      
      skip('(');
      vc = read_int(NULL);
      if (vc < 0 || vc >= max_vci)
         syntax1d("VCI %d is out-of-range", vc);
      if ( connpar[vc] == NULL)
         syntax1d("VCI %d not initialized", vc);
      skip(',');

      connpar[vc]->ppd = read_int(NULL);
      if(connpar[vc]->ppd < 0 || connpar[vc]->ppd > 1)
         syntax0("connection specific PPD must be 0 or 1");

      skip(')');      
   }

   else
      return FALSE;

   return TRUE;
   
} // muxEWSX::command()

//////////////////////////////////////////////////////////////
// muxEWSX::connect(void)
// during connecting the muxes have to register at the control
// object and give the this-pointer.
// Also the pointer to the queue-length, the size of the buffer
// and the global AAL-5 threshold for EPD has to be read out
//////////////////////////////////////////////////////////////
void muxEWSX::connect(void)
{
   int i;
   double sum;

   baseclass::connect();
   controlEWSX *p = (controlEWSX *)ewsx;
   if( ewsx == NULL)
      errm1s("%s: EWSX-Controll not initialized", name);
   
   p->mux_register(this);	// register in Control
   pqlen = & (p->qlen);		// Pointer to Queue-Length
   maxbuff = p->buff;
   
   epdThreshGlobal = p->epdThreshGlobal;
   clpThreshGlobal = p->clpThreshGlobal;   

   // Calculation of vdelta from the weight (given width command)
   // sum up all and calculate the connections part
   sum = 0.0;
   for(i=1;i < max_vci; ++i)
      sum += connpar[i]->vdelta;
   for(i=1;i < max_vci; ++i)
      connpar[i]->vdelta = sum / connpar[i]->vdelta;

   // checking of connection specific EPD and CLP=1 thresholds
   for(i=1;i < max_vci; ++i)
   {
      if(connpar[i]->epdThreshVCI > maxbuff)
         errm1s2d("%s: connection specific EPD threshold for connection %d "\
            " must be <= maxbuff=%d (see ->SetConnEPD)", name, i, maxbuff);
      if(connpar[i]->epdThreshVCI == 0)
         connpar[i]->epdThreshVCI = maxbuff+1;	// switched off (standard)

      if(connpar[i]->clpThreshVCI > maxbuff)
         errm1s2d("%s: connection spec. threshold. for dropping CLP=1 cells "\
	    "of connection %d must be <= maxbuff=%d (see ->SetConnCLPThresh)",
	    name, i, maxbuff);
      if(connpar[i]->clpThreshVCI == 0)
         connpar[i]->clpThreshVCI = maxbuff+1;	// switched off (standard)
         
   } // for - every connection

   connected = 1;	// to show, that i am connected now

} // muxEWSX::connect()


///////////////////////////////////////////////////////////////
// muxEWSX::restim(void)
// this is called, when the time to ask for an event is >
// as the max time
//////////////////////////////////////////////////////////////
void muxEWSX::restim(void)
{
   int i;
   
   for (i = 0; i < max_vci; ++i)
   {
      if (connpar[i] == NULL || connpar[i]->q_vc.isEmpty())
         continue;
      
      if (connpar[i]->time < spacTime)
         errm1s("%s: internal error on reset spacing time", name);
      
      connpar[i]->time -= spacTime;
   }
   
   spacTime = 0;
   
} // void muxEWSX::restim(void)


///////////////////////////////////////////////////////////////
// muxEWSX::pushout(ServiceClassType serviceclass)
// try's to push out the cells of a connection of type
// serviceclass
//////////////////////////////////////////////////////////////
int muxEWSX::pushout(ServiceClassType serviceclass)
{
   EWSXConnParam* sortitem;
   cell* cellitem;
   int vc;
   
   sortitem = (EWSXConnParam*) q.last();
   
   while(sortitem != NULL)
   {
      vc = sortitem->vci;
      if(connpar[vc]->ServiceClass == serviceclass)
      {
         q.deqThis(sortitem);
         
         // now dequeue and delete all cells in the queue
         while( (cellitem = (cell*) sortitem->q_vc.dequeue()) != NULL)
         {
            if( ++lostVCI[vc] == 0)
               errm1s1d("%s: overflow of LossVCI[%d]", name, vc);

            if( ++lossTot == 0)
               errm1s("%s: overflow of LossTot", name);
            
            if( ++CellsPushedOut == 0)
               errm1s("%s: overflow of PushedOutCells", name);
            
            (*pqlen)--;	// decrease queue length;
	    qlen_local--;
            
            delete cellitem;
         }

         connpar[vc]->qLenVCI = 0;
         connpar[vc]->vciOK = FALSE;

         
         return(TRUE);
         
      } // if - I have found an appropriate connection
      else
         sortitem = (EWSXConnParam*) q.precOf(sortitem);
      
   } // while - there are connections in the sortqueue

   return(FALSE);	// no success

} // void muxEWSX::pushout()

///////////////////////////////////////////////////////////////
// muxEWSX::dropItem(inpstruct *p)
//	register a loss and delete the data item
//////////////////////////////////////////////////////////////
inline void muxEWSX::dropItem(inpstruct *p)
{
   if ( ++lossTot == 0)
	   errm1s("%s: overflow of LossTot", name);

   // lossTot will always overflow first, that is why the
   // the others are not checked against overflow
   ++lost[p->inp];

   int	vc;
   vc = ((cell *) p->pdata)->vci;
   ++lostVCI[vc];

   delete p->pdata;

} // void muxEWSX::dropItem()


///////////////////////////////////////////////////////////////
// muxEWSX::special(specmsg *msg,char *caller)
//	react to special messages from other modules
//////////////////////////////////////////////////////////////
char *muxEWSX::special(specmsg *msg,char *caller)
{
   switch (msg->type)
   {

      case ABRConReqType:
      	 return AbrEstablConn((ABRConReqMsg *) msg, caller);

      //case ABRConFinType:
      //   return releaseConn((ABRConFinMsg *) msg, caller);
	      
   default:
      return "wrong type of special message";
   }
	
} // char *muxEWSX::special()


///////////////////////////////////////////////////////////////
// *muxEWSX::AbrEstablConn(ABRConReqMsg	*pmsg,char *)
// establish an ABR connection
//////////////////////////////////////////////////////////////
char	*muxEWSX::AbrEstablConn(ABRConReqMsg *pmsg, char*)
{
   char	*err;
   char	*abr_suc_nam;
   root	*r;

   if (pmsg->akt_pointer > pmsg->numb_RoutMemb)
      errm1s("internal error: %s : routing-pointer > routing-member", name);
   if (pmsg->akt_pointer == pmsg->numb_RoutMemb)
      errm1s("%s: need a successor for ABR routing", name);

   //	adjust TBE
   if (pmsg->TBE > abr.TBE )
      pmsg->TBE = abr.TBE;


   // try to establish connection in the next objects
   abr_suc_nam = pmsg->routp[pmsg->akt_pointer];

   if((r = find_obj(abr_suc_nam)) == NULL)	// r is next ABR object
      errm2s("%s: could not find object `%s' (next element in ABR route)",
         name, abr_suc_nam);	 
   pmsg->akt_pointer++; 

   if ((err = r->special(pmsg, name)) != NULL )
      errm3s("%s: could not establish  ABR connection,reason returned "\
             "by `%s': %s", name, abr_suc_nam, err);

   if (pmsg -> Requ_Flag == 1) // any successor has connection refused
      return NULL;

   abr.Nabr++;	// now there is one more abr connection in this muxer

   return NULL;

} // *muxEWSX::AbrEstablConn()


///////////////////////////////////////////////////////////////
// inline void muxEWSX::AbrCompERICA(ABRstruct *ptr)
//	compute the new ER (according to ERICA)
//////////////////////////////////////////////////////////////
inline void muxEWSX::AbrCompERICA(ABRstruct *ptr)
{
   double fairShare, VCShare;

   if (abr.TargetRate > abr.CVBRRate)
   {
      if (abr.useNactive)
      {
         if (abr.Nactive == 0)
	    fairShare = abr.TargetRate - abr.CVBRRate;
	 else
	    fairShare = (abr.TargetRate - abr.CVBRRate) / abr.Nactive;
      }
      else
      	 fairShare = (abr.TargetRate - abr.CVBRRate) / abr.Nabr;
   }
   else	
      fairShare = 0.0;

   if (abr.Z > 0.0)
   {
      VCShare = ptr->CCR / abr.Z;
      if (VCShare > fairShare)
         ptr->ER = VCShare;
      else
         ptr->ER = fairShare;
   }
   else
      ptr->ER = fairShare;

   if (ptr->ER < ptr->MCR)
      ptr->ER = ptr->MCR;
		
} // muxEWSX::AbrCompERICA()


///////////////////////////////////////////////////////////////
// inline void muxEWSX::AbrCompBinary(rmCell *pc)
// process RM cell according to binary feedback
//////////////////////////////////////////////////////////////
inline void muxEWSX::AbrCompBinary(rmCell *pc)
{
   if(pqlen == NULL)
      errm1s("%s: in AbrCompBinary(): pqlen not initialized",name);

   if (abr.abr_congested)
   {
      if (*pqlen <= abr.abr_q_lo)
         abr.abr_congested = FALSE;
      else
         pc->CI = 1;
   }
   else if ( *pqlen > abr.abr_q_hi)
   {
      abr.abr_congested = TRUE;
      	 pc->CI = 1;
   }

} // muxEWSX::AbrCompBinary()

