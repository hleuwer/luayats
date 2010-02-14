
/************************************************************************/
/*									*/
/*				YATS					*/
/*									*/
/*			Yet Another Tiny Simulator			*/
/*									*/
/*		- a simple, small and fast ATM simulation tool -	*/
/*									*/
/*	(c):	Matthias Baumann, Dresden University of Technology	*/
/*									*/
/************************************************************************/
/*									*/
/*	Double-GCRA-Shaper 						*/
/*									*/
/*	Generic Cell Rate Algorithm  (GCRA )  -  ATM Forum		*/
/*									*/
/*	Version: Virtual Scheduling Algorithm				*/ 
/*									*/
/*		      designed for YATS					*/
/*									*/
/*		Thomas Beckert, Dresden University of Technology	*/
/*				september, 1996				*/
/*									*/
/************************************************************************/
/*
*	Dual Leaky Bucket with GCRA !
*
*	Dgcra_Shaper shaper: INC_P=10, LIMIT_P=20, INC_S=50, LIMIT_S=50, BUFF=30, { VCI=1, } OUT=sink;
*			//	INC_P:		Peak Rate 
*			//	LIMIT_P:	Peak bucket size
*			//	INC_S:		Sustainalble Rate
*			//	LIMIT_S:	Sustainable bucket size
*			//	BUFF:   	FIFO buffer size
*			//	if no VCI is given, then all VCs are subject to policing
*
*	Commands are:
*			-->Count   : returns with the counter value 
*			-->ResCount: resets the countervalue
*			-->Restart : resets the Dual Leaky Bucket, sets all values like the begining
*	Exports:
*			-->LOSS
*			-->Q_LEN
*
*	Algorithm at the arrival instant:
*		Test the confirmation of the cell with the two GCRA's
*		store nonconforming cells in buffer and calculate next 
*		conforming time
*			
*/

#include "dgcra_sh.h"

CONSTRUCTOR(Dgcra_Shaper, dgcra_s);
USERCLASS("DGCRAShaper",Dgcra_Shaper);

/*
*	Constructor
*/
void	dgcra_s::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	Inc_p = read_double("INC_P");
	skip(',');
	Limit_p = read_double("LIMIT_P");
	skip(',');
	Inc_s = read_double("INC_S");
	skip(',');
	Limit_s = read_double("LIMIT_S");
	skip(',');
	q_max = read_int("BUFF");
	skip(',');

	//	VCI given?
	if (test_word("VCI"))
	{	vci = read_int("VCI");
		skip(',');
	}
	else	vci = NILVCI;

	if (vci != NILVCI)
		inp_type = CellType;
	else	inp_type = DataType;
	
	output("OUT");	// one output
	stdinp();	// an input
	
	// initialise the variables

	Inc_p_real = Inc_p - int(Inc_p);
	Inc_s_real = Inc_s - int(Inc_s);
	Inc_p_error = 0.0;
	Inc_s_error = 0.0;

	q_len = 0;
	TAT_p = 0.0;
	TAT_s = 0.0;
}

/*
*	Cell has been arriving.
*/
rec_typ	dgcra_s::REC(
	data	*pd,
	int)
{
	typecheck(pd, inp_type);

	//	police this VC?
	if (vci != NILVCI && vci != ((cell *) pd)->vci)
	{	//	no -> pass cell directly
		return suc->rec(pd, shand);
	}

//----- at the arrival of the first cell: TAT = arrivaltime of these cell

	if ( TAT_p == 0.0 )
		TAT_p = SimTime;

	if ( TAT_s == 0.0 )
		TAT_s = SimTime;

//-------------------------------------------------------------------
//----- there are stored cells ? ------------------------------------

	if ( q_len == 0 )   // no cells in buffer ?
	{ 
	  // control the cellconformation !
	  // ---- TAT > ta(k) + Limit ? -----------------------------

		delta_time_p = int(TAT_p - SimTime - Limit_p);
		if (TAT_p - SimTime - Limit_p > delta_time_p)
			++delta_time_p; 

		delta_time_s = int(TAT_s - SimTime - Limit_s);
		if (TAT_s - SimTime - Limit_s > delta_time_s)
			++delta_time_s;	

		if ( delta_time_p > delta_time_s )
			delta_time = delta_time_p;
		else 
			delta_time = delta_time_s;

		if ( delta_time > 0 )
		{
		  // yes: Cell is not conform, storing 
			
			if ( q_max > 0 )  // buffer aviable ?
			{ 
			  // buffer available, store first cell
				q_len = 1;
				q_first = q_last = pd;
			
			  // perform early event on next conforming time :
				alarme( &std_evt, delta_time );

			}
			else
			{  
			  // no buffer available, delete cells
				if ( ++counter == 0 )
					errm1s("%s: overflow of counter", name);
					delete pd;
			}
		}
		else
		{
		  // no: Cell is conform, transmission 
			
		  // ---- TAT < ta(k) ?	-----------------------------
			
			if ( TAT_p < SimTime )
				TAT_p = SimTime;
			if ( TAT_s < SimTime )
				TAT_s = SimTime;

		  // calculate next TAT
			TAT_p += Inc_p;
			TAT_s += Inc_s;
			
		  // send cell
			return suc->rec(pd, shand);
		}
	}
	else
	{
	  // cells in buffer yet, store next cell
		
		if ( q_len >= q_max )
		{  
		  // buffer overflow -> delete cell
			if ( ++counter == 0 )
				errm1s("%s: overflow of counter", name);
			delete pd;
		}
		else
		{
		 // still bufferspace aviable: store cell
				++q_len;
				q_last->next = pd;
				q_last = pd;

		}
	}


	return ContSend;
	
}



/*
*	Activation by the kernel: next cell is conform now
*	early_event method
*/

void	dgcra_s::early(
	event	*)
{

	TAT_p += Inc_p;  // 
	TAT_s += Inc_s;  // 

	// calculate next conforming time	

	delta_time_p = int(TAT_p - SimTime - Limit_p);
	if (TAT_p - SimTime - Limit_p > delta_time_p)
		++delta_time_p;

	delta_time_s = int(TAT_s - SimTime - Limit_s);
	if (TAT_s - SimTime -Limit_s > delta_time_s)
		++delta_time_s;


	if ( --q_len == 0)    // last cell in the queue
	{
			suc->rec(q_first, shand);
	}
	else
	{
	 // there are more cells : register again for sending the next

		data	*pd;
		pd = q_first;

		 // set the next cell to the first	

		q_first = q_first->next;
	 
		 // send cell 

		suc->rec(pd, shand);
	
		 // perform early event on next conforming time

		if ( delta_time_p > delta_time_s )
			alarme( &std_evt, delta_time_p);
		else 
			alarme( &std_evt, delta_time_s);

	}
}




/*
*	reset SimTime -> perform decrements
*/
void	dgcra_s::restim(void)
{
	if ( TAT_p < SimTime )
	{
		TAT_p = 0;
	}
	else
		TAT_p -= SimTime;

	if ( TAT_s < SimTime )
	{
		TAT_s = 0;
	}
	else
		TAT_s -= SimTime;



}


/*
* 	shaper->Restart	 reset all variables, for runs more then 2^32 Slots
*			
*	shaper->Count 	 print out the counter
*	shaper->ResCount reset the counter
*/
int	dgcra_s::command(
	char	*s,
	tok_typ	*v)
{
	data  *pd;	

	if (baseclass::command(s, v) == TRUE)
		return TRUE;

	v->tok = NILVAR;
	if (strcmp(s, "Restart") == 0)
	{
		TAT_p = TAT_s = 0;
		counter = 0;
		
		// inactivate registred events 
		if ( q_len > 0 )
			unalarme( &std_evt );

		while ( q_len > 0)
		{
		 // delete all cells in buffer
			pd = q_first;
			q_first = q_first->next;
			delete pd;
			--q_len;
		}
	}
	else
		if(strcmp(s, "Count") == 0)
		{	v->tok = IVAL;
			v->val.i = counter;
		}
		else 
		if (strcmp(s, "ResCount") == 0)
			counter = 0;
		else	return FALSE;

	return	TRUE;
}

/*
*	export addresses of variables
*/
int	dgcra_s::export(exp_typ *msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "LOSS", (int*) &counter);
		intScalar(msg, "Q_LEN", &q_len);
}
