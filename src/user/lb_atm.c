/*************************************************************************
*
*		YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997	Chair for Telecommunications
*				Dresden University of Technology
*				D-01062 Dresden
*				Germany
*
**************************************************************************
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
**************************************************************************
*
*	Module author:		Matthias Baumann, TUD
*     	             	        Marianna Stantcheva (1998)
*	Creation:		1996
*
*	History:		March 24, 97: MBau
*				LbStat added (bucket occupation seen
*				at arrival instants, only for "good" cells)
*				exported:	lb->LbStat(i)
*						absol. frequency of occup i
*				command:	lb->ResLbStat;
*     	             	      	Stantcheva, M.: 
*
*************************************************************************/

/*
*	Leaky Bucket ATM:
*
*	LeakyBucket_ATM lbatm: BITRATE=<double>, SCR=<double>, MBS=<double>,
*     	           TAG=1, { VCI=1, } OUT=sink;
*     	    BITRATE - maximum Bitrate of line
*     	    SCR - Sustainable Cell Rate
*     	    MBS - Maximum Burst Size
*     	    TAG - not conforming cells are: 0: discarded, 1: tagged (CLP=1)
*     	    VCI - perform only on this VCI, if not given on all cells
*     	    OUT - next module
* 
*	Algorithm at the arrival instant:
*		Perform decrements accumulated since last arrival.
*		Test if the incoming cell "sees" bucket overflow 
*			Yes: tag cell and pass cell to next object (TAG=1) 
*     	             	or delete cell (TAG!=1), no bucket increment.
*			No: perform increment, pass cell to next object.
*  
*/

#include "lb_atm.h"

CONSTRUCTOR(LeakyB_ATMF, lb_atmf);
USERCLASS("LeakyBucket_ATMF",LeakyB_ATMF);

/*
*	read input line
*/
void	lb_atmf::init(void)
{

	double	lb_bitrate;
	double	lb_scr;
	double	lb_mbs;


	skip(CLASS);
	name = read_id(NULL);
	skip(':');

 	lb_bitrate = read_double("BITRATE");
 	if (lb_bitrate <= 0.0)
 	     errm1s (" %s: The BITRATE must be > 0.0", name);
	skip(',');
	
	lb_scr = read_double("SCR");
	if (lb_scr <= 0.0)
	     errm1s (" %s: SCR must be > 0.0", name);
	
	if (lb_scr > lb_bitrate)
	     errm1s (" %s: The SCR must be <= BITRATE", name);
	skip(',');
	
	lb_mbs = read_double("MBS");
	if (lb_mbs < 1.0)
	     errm1s (" %s: The MBS must be >= 1.0", name);
	skip(',');
	
 	lb_tag = read_int("TAG");
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

	// read additional parameters of derived classes
	addpars();
	
	output("OUT");	// one output
	stdinp();	// an input

	lb_inc = lb_bitrate / lb_scr;
 	lb_max = lb_mbs * lb_bitrate / lb_scr;


 	lb_dec = 1.0;
	lb_siz = 0.0;
	last_time = 0;

/* Problem Memory Mue 08.06.1998
	CHECK(lbSizStat = new int [int(lb_max) + 1]); // histogram array with integer bins
	for (int i = 0; i <= int(lb_max); ++i)
		lbSizStat[i] = 0;
*/
}

/*
*	Cell has been arriving.
*/
rec_typ	lb_atmf::REC(
	data	*pd,
	int)
{
	typecheck(pd, inp_type);

	//	police this VC?
	if (vci != NILVCI && vci != ((cell *) pd)->vci)
	{	//	no -> pass cell directly
		return suc->rec(pd, shand);
	}
	
	if (pd->clp == 1)
	   return suc->rec(pd, shand);

	//	Policing: perform decrements since last arrival
	lb_siz -= (double(SimTime - last_time)) * lb_dec;
	if (lb_siz < 0.0)
		lb_siz = 0.0;
	last_time = SimTime;

	//	test on overflow	
	if (lb_siz > lb_max)
	{	//	overflow -> tag or delete cell
		if ( ++counter == 0)
			errm1s("%s: overflow of counter", name);

		if (lb_tag == 1)
		{
		    pd->clp=1;
	        }
		else 
		{
		   delete pd;
		   return ContSend;
		}
	}
	else	// still place in the bucket
	{
		// Problem Memory Mue 08.06.1998 ++lbSizStat[int(lb_siz)]; //  log onto the lb_siz-th bin of the histogram after incrementation
		lb_siz += lb_inc;
	} 

return suc->rec(pd, shand);
}

/*
*	reset SimTime -> perform decrements
*/
void	lb_atmf::restim(void)
{
	lb_siz -= (double(SimTime - last_time)) * lb_dec;
	if (lb_siz < 0.0)
		lb_siz = 0.0;
	last_time = 0;
}

int	lb_atmf::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		doubleScalar(msg, "LbSize", &lb_siz);
		// Problem Memory Mue 08.06.1998
		// intArray1(msg, "LbStat", lbSizStat, int(lb_max) + 1, 0);
}

int	lb_atmf::command(
	char	*s,
	tok_typ	*v)
{
	if (baseclass::command(s, v))
		return TRUE;

	v->tok = NILVAR;
	/* Problem Memory Mue 08.06.1998
	if (strcmp(s, "ResLbStat") == 0)
	{	int	i;
		for (i = 0; i <= int(lb_max); ++i)
			lbSizStat[i] = 0;
		return TRUE;
	}
	*/
	return FALSE;
}
