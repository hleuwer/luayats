// module which intelligently marks or queues cells according
// to a leaky bucket
// if a frame had been started with CLP=0 other cells are delayed
// if a frame had been started with CLP=1 it is served quickly with CLP=1


#include "int_pol.h"

CONSTRUCTOR(Intpol, intpol);
USERCLASS("IntPol",Intpol);

/*
*	Constructor
*/
void	intpol::init(void)
{
   skip(CLASS);
   name = read_id(NULL);
   skip(':');

   lb.inc = read_double("INC");
   skip(',');
   lb.max = read_double("LIMIT");
   skip(',');
   q.setmax(read_int("BUFF") + 1);
   skip(',');

   output("OUT");	// one output
   stdinp();	// an input

   alarmed = 0;   // we are not alarmed at the beginning
   newframe = 1;  // the first cell belongs to a new frame
   frame_clp = 0; // the first frame will have clp=0
}

/*
*	Cell has been arriving.
*/
rec_typ	intpol::REC(data *pd,int)
{
   aal5Cell* pc;
   
   typecheck(pd, AAL5CellType);
   
   pc = (aal5Cell*) pd;

   if(q.enqueue(pc) == FALSE)
   {
      delete pc;
      counter++;
   }
   
   if(!alarmed)
   {
      alarme(&std_evt,1);
      alarmed = TRUE;
   }

   return ContSend;
}



/*
*	Activation by the kernel: next cell is conform now
*	early_event method
*/

void intpol::early(event *)
{

   aal5Cell *pc;
   
   alarmed = FALSE;
   pc = (aal5Cell*) q.dequeue();
   
   if(pc == NULL) 
      errm1s("%s: no data item in queue but alarmed", name);

   lb.update(); // manage decrement of bucket
   int next = lb.next();
   int full = lb.isfull();

   if(newframe)
   {
      //printf("neuer Frame\n");
      newframe = 0;
      if(!full) // we can send the next now
         frame_clp = 0;
      else
         frame_clp = 1;
   }
   
   if(frame_clp == 0)
   {
      if(!full)
      {
         pc->clp = 0;
	 lb.increment();   // increment leaky bucket for this cell
	 
	 if(pc->pt == 1)
	    newframe = 1;
	 
	 suc->rec(pc, shand);
	 //printf(" konforme Zelle CLP=0\n");
	 
	 if(q.q_len > 0 && !newframe)
	 {
	    next = lb.next();
	    alarme(&std_evt,next);
	    alarmed = TRUE;
	 }

      }
      else
      {
         //printf("  Zelle zurückgeschrieben\n");
         q.enqHead(pc);
         alarme(&std_evt, next); // alarme for the next possible slot
	 alarmed = TRUE;
      }
   
   }
   else
   {
      if(pc->pt == 1)
	 newframe = 1;
      
      if(lb.value == 0 && newframe == 0) // bucket is really empty and no new frame
      {
         pc->clp = 0;
	 lb.increment();
	 //printf(" nicht konforme Zelle zu CLP=0 geändert\n");
      }
      else
      {
         pc->clp = 1;
	 //printf(" nicht konforme Zelle CLP=1\n");
      }
      
      suc->rec(pc, shand);
      
   } // else - if frame is clp=0
   
   if(!alarmed && q.q_len > 0)
   {
      alarme(&std_evt,1);
      alarmed = TRUE;
   
   }
      
}




/*
*	reset SimTime -> perform decrements
*/
void intpol::restim(void)
{
}


/*
* 	shaper->Restart	 reset all variables, for runs more then 2^32 Slots
*			
*	shaper->Count 	 print out the counter
*	shaper->ResCount reset the counter
*/
int intpol::command(char *s,tok_typ *v)
{
   if (baseclass::command(s, v) == TRUE)
	   return TRUE;

   v->tok = NILVAR;
   if(strcmp(s, "Count") == 0)
   {
      v->tok = IVAL;
      v->val.i = counter;
   }
   else if(strcmp(s, "ResCount") == 0)
      counter = 0;
   else
      return FALSE;

   return
      TRUE;
}

/*
*	export addresses of variables
*/
int	intpol::export(exp_typ *msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "LOSS", (int*) &counter);
}
