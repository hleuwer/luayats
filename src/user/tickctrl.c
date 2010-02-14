///////////////////////////////////////////////////////////////////////////////
// TickCtrl
// Module, which emulates the behaviour of an operating system
// If the system becomes empty during an interval, new arriving data items
// are served at the beginning of the next interval
// Usage:
// TickCtrl name: TICKDIST=<dist>, {BUFF=<int>, BSTART=<int>, OUTCTRL=<suc>}
//                {CONTPROC=<int>}, OUTDATA=<suc>;
// Parameters:
//    TICKDIST: distribution, which gives length of the interval (slots)
//    BUFF: (opt.) max. size of buffer (int)
//    BSTART: (opt.) buffer level, when to wake up the predecessor (SS-Protocol)
//    OUTCTRL: (opt.) output, where to write wakeup signal (SS-Protocol)
//    OFFINT: (opt. = 1) time interval, when going off is detected
//    CONTPROC: (opt. = 0) number of contending processes
//    PHASE: (opt. = random) phase of start		
//    OUTDATA: output, where to send data
//
// Exported Variables
//    ->Count: number of served cells
//    ->QLen: actual value the queue length
///////////////////////////////////////////////////////////////////////////////

#include "tickctrl.h"

#if 0
CONSTRUCTOR(Tickctrl, tickctrl);
USERCLASS("TickCtrl", Tickctrl);

void tickctrl::init(void)
{
   char *s, *err;
   root *obj;
   GetDistTabMsg msg;
   
   skip(CLASS);
   name = read_id(NULL);
   skip(':');
   
   s = read_suc("TICKDIST");
   if ((obj = find_obj(s)) == NULL)
      syntax2s("%s: could not find object `%s'", name, s);
   if ((err = obj->special( &msg, name)) != NULL)
      syntax2s("could not get distribution table, reason returned by `%s':\n\t%s",
	        s, err);
   table = msg.table;
   delete s;
   skip(',');
   
   if(test_word("BUFF"))
   {
      q_max = read_int("BUFF");
      if (q_max <= 0)
	 syntax0("BUFF has to be greater than zero");
      skip(',');

      q_start = read_int("BSTART");
      if (q_start < 0 || q_start >= q_max)
	 syntax0("invalid BSTART value");
      skip(',');

      output("OUTCTRL", SucCtrl);
      skip(',');
   }
   else
      q_max = 0;

   if(test_word("OFFINT"))
   {
      off_int = read_int("OFFINT");
      if(off_int <= 0)
         syntax0("OFFINT must be >= 1");
      skip(',');
   }
   else
      off_int = 1;

   if(test_word("CONTPROC"))
   {
      contproc = read_int("CONTPROC");
      if(contproc < 0)
         syntax0("CONTPROC has to be positive");
      skip(',');
   }
   else
      contproc = 0;
      
    if(test_word("PHASE"))
   {
      phase = read_int("PHASE");
      if (phase == 0) phase = 1;
      if(phase < 0)
         syntax0("PHASE has to be positive");
      skip(',');
   }
   else     // we choose a random value if phase is not included 
   {
	   phase = table[my_rand() % RAND_MODULO];
	   phase = (int) ((double)my_rand() / (double)RAND_MODULO * (double) phase) + 1;
   }	   
   
   // read additional parameters of derived classes
   addpars();

   output("OUTDATA", SucData);

   input(NULL, InpData);	// input DATA
   if(q_max > 0)
      input("Start", InpStart);	// input START

   prec_state = ContSend;
   send_state = ContSend;
   alarmed = FALSE;
   
   // take a random value from the distribution and alarm at a random value
   // between 0 and this value

   alarml(&evtTick, phase);  // alarm acording to the desired phase
   
   old_tm = phase;
   next_time = phase + 1;
   
   actproc = contproc; // do not send in the beginning
   
   counter = 0;
   last_received = 0;
      
} // init
#endif


tickctrl::tickctrl():evtTick(this, keyTick)
{
   q_max = 0;
}
tickctrl::~tickctrl()
{
}

int tickctrl::act(void)
{
   if (phase == -1){
      phase = table[my_rand() % RAND_MODULO];
      phase = (int) ((double)my_rand() / (double)RAND_MODULO * (double) phase) + 1;
   } 
   prec_state = ContSend;
   send_state = ContSend;
   alarmed = FALSE;
   alarml(&evtTick, phase);
   old_tm = phase;
   next_time = phase + 1;
   actproc = contproc;
   counter = 0;
   last_received = 0;
   return 0;
}
////////////////////////////////////////////////////////////
// tickctrl::REC()
// data item has been arriving.
////////////////////////////////////////////////////////////
rec_typ	tickctrl::REC(data *pd,int key)
{

   switch(key)
   {
      // I got data
      case InpData:
      
      if (prec_state == StopSend)
      {
	 fprintf(stderr, "%s: preceeding object did not recognize the Stop signal\n", name);
	 delete pd;
	 return StopSend;
      }

      q.enqueue(pd); // write data into the queue

      if(!alarmed)  // solo si no estamos enviando datos ya
      {
	 tim_typ next;
	 
	 if((SimTime - last_received <= off_int) && (actproc == 0)) // compara dos paquetes consecutivos
	    next = 1;
	 else
	    next = next_time - SimTime;
	 	 	    
	 if(next >= 1)			// alarma para enviar segun next
	    alarme(&std_evt, next);
	 else
	    errm1s("%s: internal error: would have to alarme in the past", name);

	 alarmed = TRUE;
	 
      } // if not alarmed
      
      last_received = SimTime; // remember last receiving time

      if (q.getlen() >= q_max && q_max > 0)
      {
	 //buffer now full, stop the sender, if start/stop is implemented
	 prec_state = StopSend;
	 return StopSend;
      }
      else
	 return ContSend;
	 
   
      // I got a start signal
      case InpStart:
	 if (send_state == StopSend)
	 {
	    send_state = ContSend;
	    
	    // wake up for next tick if data are there
	    if(!alarmed && q.getlen() > 0)
	    {
	       tim_typ next;
      	       next = next_time - SimTime;
	       if(next <= 0)
	          errm1s("%s: internal error: would have to alarme in the past", name);

      	       alarme(&std_evt, next);
	       alarmed = TRUE;
	    }
	 }

	 delete pd;

	 return ContSend;
      
      default: errm1s("%s: internal error: receive data on input with no receive method", name);
         return ContSend;

   } // switch

} // rec()


////////////////////////////////////////////////////////////
// tickctrl::early()
// send the next data item
////////////////////////////////////////////////////////////
void tickctrl::early(event *)
{
   data* pd;

   if (actproc != 0) 
      errm1s("%s: you are not active but you are sending", name);
   
   // dequeue from the queue
   pd = q.dequeue(); 

   // send the item now
   if(pd != NULL) {
      chkStartStop(send_state = sucs[SucData]->rec(pd, shands[SucData]));
      if (++counter == 0)
         errm1s("%s: overflow of counter", name); 
   } else
      errm1s("%s: alarmed, but no data in queue", name); 

   // alarm again, if something is in the queue and we are the active process
   alarmed = FALSE;
   
   if (q.getlen() > 0 && actproc == 0) { 
      // active and data available
      if (send_state == ContSend || q_max == 0) {
	 alarme(&std_evt, 1); // alarm immediately
	 alarmed = TRUE;
      }
   } 

   if (q.getlen() == q_start  && prec_state == StopSend){
		sucs[SucCtrl]->rec(new data, shands[SucCtrl]);
		prec_state = ContSend;
   }

} // early()

////////////////////////////////////////////////////////////
// tickctrl::late()
// realizes the tick timer
////////////////////////////////////////////////////////////
void tickctrl::late(event *)
{
   actproc++;	
   
   if(actproc > contproc){
      // active
      tim_typ thistick = table[my_rand() % RAND_MODULO];
      alarml(&evtTick, thistick);
      
      actproc = 0;
      old_tm = thistick;
      next_time = SimTime + ((contproc+1) * thistick) + 1;
#if 0
      printf("#### %d active: old_tm=%d thistick=%d qlen=%d\n", SimTime, old_tm, thistick, q.getlen());
#endif
      if(q.getlen() > 0 && !alarmed) {
	 // alarm immediately to wake up again
         alarme(&std_evt, 1); 
	 alarmed = TRUE;
      }
   } else 
      // inactive
      alarml(&evtTick, old_tm);

} // late()

////////////////////////////////////////////////////////////
// reset SimTime -> change next_time
////////////////////////////////////////////////////////////
void tickctrl::restim(void)
{
   errm1s("%s: restime not implemented", name); 
}

////////////////////////////////////////////////////////////
// Export of variables
////////////////////////////////////////////////////////////
int tickctrl::export(exp_typ *msg)
{
	return	baseclass::export(msg) ||
	   intScalar(msg, "QLen", (int *) &q.q_len);
}
