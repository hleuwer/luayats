
#ifndef  _IACTSRC_H
#define  _IACTSRC_H
 
#include "inxout.h"
 
class   iactsrc:   public inxout
{
typedef inxout baseclass;
public:
   void init(void);
   rec_typ REC(data *, int);
   void    early(event *);
 
   enum {OutData = 0, OutAck = 1};
   enum {SucData = 0, SucAck = 1};
   
   tim_typ *table;
   tim_typ end_time;

};
 
#endif   // _IACTSRC_H



CONSTRUCTOR(Iactsrc, iactsrc);
USERCLASS("IactSrc", Iactsrc);

//////////////////////////////////////////////////////////////////////////
//  initialization
//////////////////////////////////////////////////////////////////////////
void   iactsrc::init(void)
{
	char		*s, *err;
	root		*obj;
	GetDistTabMsg	msg;
	tim_typ start_time = 0;
	
	end_time  = 0-1;
	
	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	s = read_suc("DIST");
	if ((obj = find_obj(s)) == NULL)
		syntax2s("%s: could not find object `%s'", name, s);
	if ((err = obj->special( &msg, name)) != NULL)
		syntax2s("could not get distribution table, reason returned by `%s':\n\t%s",
				s, err);
	table = msg.table;
	delete s;

	skip(',');
	vci = read_int("VCI");
	skip(',');

	if( test_word("STARTTIME"))
	{
	   int start_time_in;
	   
	   start_time_in = read_int("STARTTIME");
	   if( start_time_in < -1)
	      syntax1s("%s: StartTime must be >= 0 or -1 for start random start", name);
	   skip(',');
	   
	   if(start_time_in > 0)
	      start_time = start_time_in;
	   else if(start_time_in == 0)
	      start_time = 1;
	   else
	      start_time = 0;	// random start (in was -1)
	}

	if( test_word("ENDTIME"))
	{
	   int end_time_in;
	   end_time_in = read_int("ENDTIME");
	   if(end_time_in <= 0)
	      syntax1s("%s: EndTime must be > 0", name);
	   skip(',');
	   end_time = end_time_in;

	}

	// read additional parameters of derived classes
	addpars();

	// first registration
	if(start_time == 0)
	   start_time = table[my_rand() % RAND_MODULO];	// random registration

	if(start_time <= 0)
	   start_time = 1;
	
	if(start_time <= end_time)
	   alarme( &std_evt, start_time);	// first registration


   output("OUTACK", OutAck);		// output ACK-DATA
   skip(',');
   output("OUTDATA", OutData);	// output DATA

   stdinp();
  
   counter = 0;

} // iactsrc::init()

//////////////////////////////////////////////////////////////////////////
//  receiving an Ack
//////////////////////////////////////////////////////////////////////////
rec_typ iactsrc::REC(data *pd, int key)
{
   // now I am informed, the data is arrived
   // I have to send the next data after thinking time
   tim_typ think_time;
   think_time = table[my_rand() % RAND_MODULO];
   if(think_time <= 0)
      think_time = 1;
        
   if(SimTime+think_time <= end_time)
      alarme( &std_evt, think_time);	// next registration

   // send data to sink
   return sucs[SucAck]->rec(pd, shands[SucAck]);

} // iactsrc::REC()

//////////////////////////////////////////////////////////////////////////
//  sending the data item
//////////////////////////////////////////////////////////////////////////
void iactsrc::early(event	*)
{
	if ( ++counter == 0)
	   errm1s("%s: overflow of counter", name);
	sucs[SucData]->rec(new cell(vci), shands[SucData]);

} // iactsrc::early()
