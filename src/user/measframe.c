///////////////////////////////////////////////////////////////////////////////
// MeasFrame
// 
// Command Line:
//    MeasFrame name: OUT=module;
//
// Commands:
//    ->Goodput5      	// mean Goodput5 (=Packetlength / Transmission time)
//    	 (in byte/slots)
//    ->Goodput5Invers  // mean Invers of Goodput5 (in slots/byte)
///////////////////////////////////////////////////////////////////////////////

#include "measframe.h"

measframe::measframe()
{
}

measframe::~measframe()
{
}


//	Should be transfered to LUA init

/*
*	read create statement
*/
/*
void measframe::init(void)
{
   skip(CLASS);
   name = read_id(NULL);
   skip(':');

   output("OUT");

   stdinp();
   
   meanGoodput5 = 0.0;
   meanGoodput5Invers = 0.0;
   counter = 0;
   
}
*/

/*
*	A data object has arrived.
*/

rec_typ	measframe::REC(data *pd,int)
{
   frame *pf;
   double gp5;

   typecheck(pd, FrameType);   // input data type check
   
   if (++counter == 0)
      errm1s("%s: overflow of counter", name);

   pf = (frame*) pd;
   gp5 = (double) pf->frameLen / (double)(SimTime - pf->time);

   meanGoodput5 = ((counter - 1) * meanGoodput5 + gp5) / (double) counter;
   meanGoodput5Invers = ((counter - 1) * meanGoodput5Invers + 1.0/gp5) / (double) counter;

   return suc->rec(pd, shand);

}


/*
*	Command procedures: reset counters
*/
int	measframe::command(
	char	*s,
	tok_typ	*v)
{
   if (baseclass::command(s, v) == TRUE)
      return TRUE;

   v->tok = NILVAR;
   if (strcmp(s, "ResStats") == 0)
   {
      meanGoodput5 = 0.0;
      meanGoodput5Invers = 0.0;
      counter = 0;
   }
   else	
      return FALSE;

   return
      TRUE;
}

/*
*	export of variables
*/
int measframe::export(exp_typ *msg)
{
    return baseclass::export(msg)  ||
       doubleScalar(msg, "Goodput5", &meanGoodput5) ||
       doubleScalar(msg, "Goodput5Invers", &meanGoodput5Invers);
}
