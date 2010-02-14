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
*	Creation:		1996
*
*	History:		March 24, 97: MBau
*				LbStat added (bucket occupation seen
*				at arrival instants, only for "good" cells)
*				exported:	lb->LbStat(i)
*						absol. frequency of occup i
*				command:	lb->ResLbStat;
*
*************************************************************************/

/*
*	Leaky Bucket:
*
*	LeakyBucket lb: INC=10, DEC=5, SIZE=20, { VCI=1, } OUT=sink;
*			//	INC:	increment performed with each active cell
*			//	DEC:	decrement performed with each time slot
*			//	SIZE:	bucket size
*			//	if no VCI is given, then all VCs are subject to policing
*
*	Algorithm at the arrival instant:
*		Perform decrements accumulated since last arrival.
*		Test, whther adding the increment would yield a bucket overflow.
*			Yes: discard cell (no bucket incr).
*			No: perform increment, pass cell to next object.
*/

#include "leakyb.h"

/*
*	read input line
*/

lb::lb()
{
}

lb::~lb()
{
}

//	Transfered to LUA init and SetPrivateParams(int inc, int dec, int max)

/*
void	lb::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	lb_inc = read_int("INC");
	skip(',');
	lb_dec = read_int("DEC");
	skip(',');
	lb_max = read_int("SIZE");
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

	lb_siz = 0;
	last_time = 0;

	CHECK(lbSizStat = new int [lb_max + 1]);
	for (int i = 0; i <= lb_max; ++i)
		lbSizStat[i] = 0;
}
*/
int      lb::act(void)
{
	CHECK(lbSizStat = new int [lb_max + 1]);
	for (int i = 0; i <= lb_max; ++i)
		lbSizStat[i] = 0;
	return 0;
}

void     lb::SetPrivateParams(int inc, int dec, int max)
{
	lb_inc = inc;
	lb_dec = dec;
	lb_max = max;

        if (vci != NILVCI)
                inp_type = CellType;
        else    inp_type = DataType;

        lb_siz = 0;
        last_time = 0;

        CHECK(lbSizStat = new int [lb_max + 1]);
        for (int i = 0; i <= lb_max; ++i)
                lbSizStat[i] = 0;
}


/*
*	Cell has been arriving.
*/

rec_typ	lb::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	typecheck(pd, inp_type);

	//	police this VC?
	if (vci != NILVCI && vci != ((cell *) pd)->vci)
	{	//	no -> pass cell directly
		return suc->rec(pd, shand);
	}

	//	Policing: perform decrements since last arrival
	lb_siz -= (SimTime - last_time) * lb_dec;
	if (lb_siz < 0)
		lb_siz = 0;
	last_time = SimTime;

	//	test on overflow
	if (lb_siz + lb_inc > lb_max)
	{	//	overflow -> discard cell
		if ( ++counter == 0)
			errm1s("%s: overflow of counter", name);
		delete pd;
		return ContSend;
	}
	else	// still place in the bucket
	{	/*
		*	log the bucket size
		*/
		++lbSizStat[lb_siz];

		lb_siz += lb_inc;
		return suc->rec(pd, shand);
	}
}

/*
*	reset SimTime -> perform decrements
*/
void	lb::restim(void)
{
	lb_siz -= (SimTime - last_time) * lb_dec;
	if (lb_siz < 0)
		lb_siz = 0;
	last_time = 0;
}

int	lb::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "LbSize", &lb_siz) ||
		intArray1(msg, "LbStat", lbSizStat, lb_max + 1, 0);
}

int	lb::command(
	char	*s,
	tok_typ	*v)
{
	if (baseclass::command(s, v))
		return TRUE;

	v->tok = NILVAR;
	if (strcmp(s, "ResLbStat") == 0)
	{	int	i;
		for (i = 0; i <= lb_max; ++i)
			lbSizStat[i] = 0;
		return TRUE;
	}
	return FALSE;
}
