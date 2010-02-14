/////////////////////////////////////////////////////////////////////////////// 
//     MuxEPDPrio
//     
//     Multiplexer with EPD and PPD (see MuxEPD).
//     Additionally there is another threshold for discarding cells with clp=1.
//     This can be used to install a priority system.
//     Also the sourcecode implementaion of EPD is simplified compared to 
//     MuxEPD.
//  
//     MuxEPDPrio mux: NINP=10, BUFF=1000, {MAXVCI=100,} 
//        THRESH=600, THRESHLOW=500,OUT=sink;
//        // default MAXVCI: NINP
//  
//    Commands:	see Multiplexer (mux.c)
/////////////////////////////////////////////////////////////////////////////// 

#include "mux.h"

class connparam {

   public:
   
   connparam(void);
   int vciOK;     // is there no cell lost in the actual frame?
   int vciFirst;  // is the next cell the first of a frame?
};

connparam::connparam(void)
{
   vciOK = TRUE;     // no cell lost at beginning
   vciFirst = TRUE;  // the first cell is the beginning of a frame
}

class	muxEPDPrio: public mux {
typedef	mux	baseclass;
public:

   void	dropItemAAL5(inpstruct *);
   void	addpars(void);
   void	late(event *);

   int	epdThresh;
   int	epdThreshLow;
   connparam *cp;

};

CONSTRUCTOR(MuxEPDPrio, muxEPDPrio);
USERCLASS("MuxEPDPrio", MuxEPDPrio);

void muxEPDPrio::dropItemAAL5(inpstruct *p)
{  
   int vc;
   vc = ((aal5Cell *) p->pdata)->vci;

   cp[vc].vciOK = FALSE;

   baseclass::dropItem(p);
}


/*
*	read create statement
*/
void muxEPDPrio::addpars(void)
{
   baseclass::addpars();

   epdThresh = read_int("THRESH");
   if (epdThresh <= 0)
      syntax0("invalid THRESH");
   skip(',');

   epdThreshLow = read_int("THRESHLOW");
   if (epdThreshLow <= 0)
      syntax0("invalid THRESHLOW");
   skip(',');

   CHECK(cp = new connparam[max_vci+1]);
}

/*
*	Every time slot:
*	In the late slot phase, all cells have arrived and we can apply a fair
*	service strategy: random choice
*/

void	muxEPDPrio::late(event *)
{

   int n;
   inpstruct *p;

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
         

         // losses somwhere else
         /*
         if(pc->cell_seq != cp[vc].cell_seq+1)
         {
            pc->clp = 0;
            cp[vc].vciOK = FALSE;
         }
         cp[vc].cell_seq = pc->cell_seq;

	 */
	 
	 // first cell of a packet?
	 if(cp[vc].vciFirst)
	 {
	    
	    cp[vc].vciFirst = FALSE;
	    
	    // check, if queuelength exceeds one of the thresholds
	    if ( q.getlen() >= epdThresh)
	       cp[vc].vciOK = FALSE;
	    
	    if(pc->clp == 1 && q.getlen() >= epdThreshLow)
	       cp[vc].vciOK = FALSE;

	 } // if - first cell of a packet
	 
	 
	 // End of frame
	 if (pc->pt == 1)
	 {
	       cp[vc].vciOK = TRUE;       // new packet - set to true
	       cp[vc].vciFirst = TRUE;    // next cellt is the first of a new packet
	 } // if - pt=1 cells (last cll of aal5-packet)
         

         if (cp[vc].vciOK ) // || pc->clp == 0 && pass_all_clp )
         {
            if ( !q.enqueue(pc)) // buffer overflow
               dropItemAAL5(p);
         }
         else
            dropItemAAL5(p);

      } // if - AAL5 cell
      else
      {
         if (q.enqueue(p->pdata) == FALSE)
            dropItem(p);
            
      } // else - normal cell

      *p = inp_buff[--n];


   } // while - as long as there are cells
      
   inp_ptr = inp_buff;

   if (q.getlen() != 0)
      alarme( &std_evt, 1);
      
} // late()

