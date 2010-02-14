///////////////////////////////////////////////////////////////////////////////
// 
//    Multiplexer with EPD/PPD and RED discard strategie
//
//    MuxRED mux: NINP=10,    // number of inputs
//     	 BUFF=1000,  	      // buffer size (cells)
//	 {MAXVCI=100,}	      // max. VCI number (default NINP)
//    	 THRESH=600, 	      // when to not accept new frames
//    	 CLP1THRESH = 100,    // when to discard CLP=1 cells
//    	 OUT=sink;
//
//    Commands:	see Multiplexer (mux.c)
// 
///////////////////////////////////////////////////////////////////////////////


#include "redmux.h"


CONSTRUCTOR(MuxRED, muxRED);
USERCLASS("MuxRED", MuxRED);

// read additional (compared to mux) parameters
void muxRED::addpars(void)
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

   red.pstart = read_double("RED_PSTART");	// lower threshold for performing RED
   if(red.pstart < 0 || red.pstart > 1)
      syntax0("RED_PSTART must be in [0,1]");
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


   // the connection parameters
   CHECK(cpar = new RedMuxConnParam* [max_vci]);
   for (i = 0; i < max_vci; ++i)
      CHECK(cpar[i] = new RedMuxConnParam(i));
   
} // init()


void muxRED::late(event*)
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
	    cpar[vc]->vciFirst = FALSE;
	    if (q.getlen() >= epdThresh)
	       cpar[vc]->vciOK = FALSE;
	    else
	       cpar[vc]->vciOK = TRUE;

      	    // use RED (or variant) for early drop
      	    int drop;
	    drop = red.Update(q.q_len);
	    if(drop)
	       cpar[vc]->vciOK = FALSE;

	 }

	 if (pc->pt == 1)	    // next cell will be first cell
	    cpar[vc]->vciFirst = TRUE;

	 if(pc->clp == 1 && q.getlen() > clp1Thresh)
	 {
	    cpar[vc]->vciOK = 0;
	 }

	 if (cpar[vc]->vciOK)
	 {
	    if ( !q.enqueue(pc))    // buffer overflow
	    {
	       dropItem(p);
	       cpar[vc]->lost++;
	       if(pc->clp == 0)
	          cpar[vc]->lost0++;

	       cpar[vc]->vciOK = FALSE;   // drop the rest of the burst
      	    }
	    else  // succesful served
	       cpar[vc]->served++;
	 }
	 else
	 {
	    dropItem(p);
            cpar[vc]->lost++;
	    if(pc->clp == 0)
	       cpar[vc]->lost0++;
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

