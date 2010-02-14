///////////////////////////////////////////////////////////////////////////////
//
//    Leaky Bucket Frame:
//  
//     LeakyBucketFrame lbframe: BITRATE=<double>, SCR=<double>, MBS=<double>,
//        MFS=<int>, TAG=0|1, {TAG_PT1=0|1}{ VCI=1, } OUT=sink;
// 
//        BITRATE - maximum Bitrate of line
//        MCR - Minimum Cell Rate
//        MBS - Maximum Burst Size
//    	  MFS - Maximum Frame Size
//        TAG - not conforming cells are: 0: discarded, 1: tagged (CLP=1)
//    	  TAG_PT1 - tag pt1 cells, default: no
//        VCI - perform only on this VCI, if not given on all cells
//        OUT - next module
//
//    This module realizes the F-GCRA algorithm described in ATMF Traffic
//    Management Spec 4.1 om Appendix VI, p. 98.
//    The module has evolved from the standard leaky bucket and
//    the LeakyBucket_ATMF (lb_atm.c)
//
//    Created 2000
//    Serious Bug fix 23/08/2000
///////////////////////////////////////////////////////////////////////////////
  
#include "lbframe.h"

lbframe::lbframe()
{
	inp_type = AAL5CellType;
}

lbframe::~lbframe()
{
}

////////////////////////////////////////////////////////////////////////
// receive a cell
////////////////////////////////////////////////////////////////////////
rec_typ	lbframe::REC(data *pd,int)
{
   typecheck(pd, inp_type);
   aal5Cell* pc = (aal5Cell*) pd;

   //	police this VC?
   if (vci != NILVCI && vci !=  pc->vci)
   {
      // no -> pass cell directly
      return suc->rec(pc, shand);
   }

   ///////////////////////////////
   // first cell of a frame?
   ///////////////////////////////
   if(vci_first)
   {
      cellnumber = 0;

      if(pc->clp == 0)
      	 vci_clp = 0;
      else if(pc->clp == 1)
         vci_clp = 1;
      else
      	 errm1s1d("%s: wrong value (%d) for CLP",name,vci_clp);

   }  // if - this is the first cell of the frame

   // copy the CLP of this VC
   pc->clp = vci_clp;

   // check for max. frame size
   cellnumber++;
   if(cellnumber > lb_mfs)
      errm1s1d("%s: cellnumber in packet %d > MFS",name,cellnumber);


   ////////////////////////////////////////////////////
   // now perform leaky bucket test
   ////////////////////////////////////////////////////
   //	Policing: perform decrements since last arrival
   double Xh;
   
   if(vci_first)
   {
      Xh = lb_siz - (SimTime - last_time);
      if(Xh > lb_max || pc->clp==1)    // bucket overflow?
      {
         //if(Xh > lb_max)
	 //   printf("remarked\n");
      
         vci_clp = 1;	// passed = false

	 if ( ++counter == 0)
            errm1s("%s: overflow of counter", name);
      }
      else
         vci_clp = 0;	// passed = true
      
      if(vci_clp == 0)
      {
   	 // realisiert lb_siz = max(0, Xh) + lb_inc;
         if(Xh > 0)
	    lb_siz = Xh + lb_inc;
	 else
	    lb_siz = 0 + lb_inc;

	 last_time = SimTime;
      }
   }
   else
   {
      if(vci_clp == 0)
      {
         Xh = lb_siz - (SimTime - last_time);
	 
      	 // realisiert lb_siz = max(0, Xh) + lb_inc;
         if(Xh > 0)
	    lb_siz = Xh + lb_inc;
	 else
	    lb_siz = 0 + lb_inc;
	 
         last_time = SimTime;
      }
   
   } // else - arrival of subsequent cell
   

   // is the next cell the first?
   if(pc->pt == 1)
      vci_first = 1;
   else
      vci_first = 0;


   if(vci_clp == 1) // not passed
   {
      if (lb_tag == 1) // tagging
      {
      	 pc->clp = 1;   // tag this cell
      }
      else // dropping
      {
         delete pc;
	 return ContSend;
      }
   }

   return suc->rec(pc, shand);

} // ::rec()


////////////////////////////////////////////////////////////////////////
// reset SimTime -> perform decrements
////////////////////////////////////////////////////////////////////////
void lbframe::restim(void)
{
   lb_siz -= (double(SimTime - last_time)) * lb_dec;
   if (lb_siz < 0.0)
      lb_siz = 0.0;
   last_time = 0;
}

////////////////////////////////////////////////////////////////////////
// export()
////////////////////////////////////////////////////////////////////////
int lbframe::export(exp_typ *msg)
{
   return baseclass::export(msg) ||
      doubleScalar(msg, "LbSize", &lb_siz);
}

////////////////////////////////////////////////////////////////////////
// command()
////////////////////////////////////////////////////////////////////////
int lbframe::command(char *s,tok_typ *v)
{
   if (baseclass::command(s, v))
      return TRUE;

   v->tok = NILVAR;
   return FALSE;
}

////////////////////////////////////////////////////////////////////////
// read input line
////////////////////////////////////////////////////////////////////////

//	Transfered to LUA init
/*
void lbframe::init(void)
{

   double lb_bitrate;
   double lb_scr;
   double lb_mbs;

   skip(CLASS);
   name = read_id(NULL);
   skip(':');

   lb_bitrate = read_double("BITRATE");
   if (lb_bitrate <= 0.0)
	errm1s (" %s: The BITRATE must be > 0.0", name);
   skip(',');

   // note, that we call the SCR now MCR (according to GFR)
   lb_scr = read_double("MCR");
   if (lb_scr <= 0.0)
	errm1s (" %s: MCR must be > 0.0", name);

   if (lb_scr > lb_bitrate)
	errm1s (" %s: The MCR must be <= BITRATE", name);
   skip(',');

   lb_mbs = read_double("MBS");
   if (lb_mbs < 1.0)
	errm1s (" %s: The MBS must be >= 1.0", name);
   skip(',');

   lb_mfs = (int) read_double("MFS");
   if (lb_mfs < 1.0)
	errm1s (" %s: The MFS must be >= 1.0", name);
   skip(',');

   lb_tag = read_int("TAG");
   skip(',');

   //	VCI given?
   if (test_word("VCI"))
   {
      vci = read_int("VCI");
      skip(',');
   }
   else	vci = NILVCI;

   inp_type = AAL5CellType;    // can only accept AAL5 cells

   // read additional parameters of derived classes
   addpars();

   output("OUT");	// one output
   stdinp();	// an input

   lb_inc = lb_bitrate / lb_scr;
   lb_max = lb_mbs * lb_bitrate / lb_scr;

   lb_dec = 1.0;
   lb_siz = 0.0;
   last_time = 0;

   vci_first = 1;
   cellnumber = 0;
   vci_clp = 0;

} // init()
*/
