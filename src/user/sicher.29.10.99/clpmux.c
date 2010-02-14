///////////////////////////////////////////////////////////////////////////////
// 
//    Multiplexer with EPD/PPD and CLP thresholds
//
//    MuxCLP mux: NINP=10,    // number of inputs
//     	 BUFF=1000,  	      // buffer size (cells)
//	 {MAXVCI=100,}	      // max. VCI number (default NINP)
//    	 THRESH=600, 	      // when to not accept new frames
//    	 CLP1THRESH = 100,    // when to discard CLP=1 cells
//    	 OUT=sink;
//
//    Commands:	see Multiplexer (mux.c)
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

   epdThresh = read_int("THRESH");
   if (epdThresh <= 0)
      syntax0("invalid THRESH");
   skip(',');

   clp1Thresh = read_int("CLP1THRESH");
   if (clp1Thresh <= 0)
      syntax0("invalid CLP1THRESH");
   skip(',');


   // the connection parameters
   CHECK(cpar = new MuxConnParam* [max_vci]);
   for (i = 0; i < max_vci; ++i)
      CHECK(cpar[i] = new MuxConnParam(i));

}


void muxCLP::late(event *)
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


///////////////////////////////////////////////////////////////////////////////
// getCLR(vc)
// returns the CLR of the given VC (0 if no cells were received)
///////////////////////////////////////////////////////////////////////////////
double muxCLP::getCLR(int vc)
{
   if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: getCLR with illegal VC=%d called", name, vc);
      
   if(cpar[vc]->received > 0)
      return(cpar[vc]->lost / (double)cpar[vc]->received);
   else
      return 0.0;


} // getCLR()

double muxCLP::getCLR0(int vc)
{
   if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: getCLR with illegal VC=%d called", name, vc);
      
   if(cpar[vc]->received0 > 0)
      return(cpar[vc]->lost0 / (double)cpar[vc]->received0);
   else
      return 0.0;

}  // getCLR0()

double muxCLP::getCLR1(int vc)
{
   int received1;
   int lost1;

   if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: getCLR with illegal VC=%d called", name, vc);

   received1 = cpar[vc]->received - cpar[vc]->received0;
   lost1 = cpar[vc]->lost - cpar[vc]->lost0;
   
   if(received1 > 0)
      return(lost1 / (double)received1);
   else
      return 0.0;

} // getCLR1()

///////////////////////////////////////////////////////////////////////////////
// getBytes(vc)
///////////////////////////////////////////////////////////////////////////////
int muxCLP::getBytes(int vc)
{
   if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: getBytes with illegal VC=%d called", name, vc);

   int bytes = cpar[vc]->received * 48;
   return bytes;

} // getBytes(vc)

///////////////////////////////////////////////////////////////////////////////
// getServedBytes(vc)
///////////////////////////////////////////////////////////////////////////////
int muxCLP::getServedBytes(int vc)
{
   if (vc < 0 || vc >= max_vci)
      errm1s1d("%s: getServedBytes with illegal VC=%d called", name, vc);

   int bytes = cpar[vc]->served * 48;
   return bytes;

} // getServedBytes(vc)
