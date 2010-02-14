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
*	History:	Bug fix in command():
*			MeanIAT() counted wrong, see there
*				M. Baumann, April 18, 1997
*				Thanx to Torsten Mueller who found the bug.
*
*************************************************************************/

/*
*
*	Measurement device for inter arrival times and cell transfer delays:
*
*	Meas2 ms: MAXCTD=100, MAXIAT=100 { , VCI=1} { ,OUT=sink } ;
*				// if VCI is omitted, then all VCs are included
*				// if OUT is omitted, then the device is a sink
*
*	Commands:
*		<Name>->Count
*			get number of arrivals
*		<Name>->ResCount
*			reset number of arrivals (real command)
*		<Name>->CTD(tim)
*			get counter for cell transfer delay time tim
*		<Name>->IAT(tim)
*			get counter for inter arrival time tim
*		<Name>->ResDists
*			reset IAT and CTD distributions
*		<Name>->CTDover
*			get overflow counter for CTD
*		<Name>->IATover
*			get overflow counter for IAT
*
*		// the following are real commands, not exported variables:
*		// mean values, max and min observed values (all in the range to MAXCTD, MAXIAT)
*		ms->MeanCTD
*		ms->MaxCTD
*		ms->MinCTD
*		ms->MeanIAT
*		ms->MaxIAT
*		ms->MinIAT
*/

#include "meas2.h"

meas2::~meas2()
{
  if (ctd_dist)
    delete ctd_dist;
  if (iat_dist)
    delete iat_dist;
}

int meas2::act(void)
{     
  int i;
  CHECK(ctd_dist = new unsigned int[ctd_max]);
  for (i = 0; i < ctd_max; ++i)
    ctd_dist[i] = 0;
  
  CHECK(iat_dist = new unsigned int[iat_max]);
  for (i = 0; i < iat_max; ++i)
    iat_dist[i] = 0;
  
  ctd_overfl = 0;
  iat_overfl = 0;
  last_time = 0;
  return 0;
}
/*
*	read create statement
*/
#if 0
void	meas2::init(void)
{
	int	i;
	int	cont_flag;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	ctd_max = read_int("MAXCTD") + 1;
	if (ctd_max < 1)
		syntax0("MAXCTD must be greater than zero");
	skip(',');

	iat_max = read_int("MAXIAT") + 1;
	if (iat_max < 1)
		syntax0("MAXIAT must be greater than zero");

	// read additional parameters of derived classes
	addpars();

	//	is there a VCI or an output name?
	if (token == ',')
	{	skip(',');
		cont_flag = TRUE;
	}
	else	cont_flag = FALSE;

	if (cont_flag && test_word("VCI"))
	{	// VCI specification
		vci = read_int("VCI");

		if (token == ',')
			skip(',');
		else	cont_flag = FALSE;
	}
	else	vci = NILVCI;

	if (cont_flag)
		output("OUT");

	if (vci == NILVCI)
		inp_type = DataType;
	else	inp_type = CellType;

	stdinp();
			
	CHECK(ctd_dist = new unsigned int[ctd_max]);
	for (i = 0; i < ctd_max; ++i)
		ctd_dist[i] = 0;

	CHECK(iat_dist = new unsigned int[iat_max]);
	for (i = 0; i < iat_max; ++i)
		iat_dist[i] = 0;

	ctd_overfl = 0;
	iat_overfl = 0;
	last_time = 0;
}
#endif
/*
*	A cell has arrived.
*	Perform measurements in case of right VCI.
*	Pass or terminate cell.
*/

rec_typ	meas2::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	)
{
	tim_typ	tim;

	typecheck(pd, inp_type);	// input data type check

	if (vci == NILVCI || ((cell *) pd)->vci == vci)
	{	//	Cell count
		if ( ++counter == 0)
			errm1s("%s: overflow of counter", name);

		//	Cell Transfer Delay
		tim = SimTime - pd->time;
		if (tim < (unsigned) ctd_max)
		{	if ( ++ctd_dist[tim] == 0)
				errm1s1d("%s: overflow of ctd_dist[%d]", name, tim);
		}
		else if ( ++ctd_overfl == 0)
			errm1s("%s: overflow of ctd_overfl", name);

		//	Inter Arrival Time
		tim = SimTime - last_time;
		last_time = SimTime;
		if (tim < (unsigned) iat_max)
		{	if ( ++iat_dist[tim] == 0)
				errm1s1d("%s: overflow of iat_dist[%d]", name, tim);
		}
		else if ( ++iat_overfl == 0)
			errm1s("%s: overflow of iat_overfl", name);

	}

	if (suc != NULL)
		return suc->rec(pd, shand);
	else
	{	delete pd;
		return ContSend;
	}
}


/*
*	Command procedures: reset counters and calc some statistics values
*/
#ifdef REMOVE_THIS
int	meas2::command(
	char	*s,
	tok_typ	*v)
{
	int	i, cnt;
	double	sum;

	if (baseclass::command(s, v) == TRUE)
		return TRUE;

	v->tok = NILVAR;
	if (strcmp(s, "ResDists") == 0)
	{	ctd_overfl = 0;
		iat_overfl = 0;
		for (i = 0; i < ctd_max; ++i)
			ctd_dist[i] = 0;
		for (i = 0; i < iat_max; ++i)
			iat_dist[i] = 0;
	}
	else if (strcmp(s, "MeanCTD") == 0)
	{	v->tok = DVAL;
		sum = 0.0; cnt = 0;
		for (i = 0; i < ctd_max; ++i)
		{	sum += i * (double) ctd_dist[i];
			cnt += ctd_dist[i];
		}
		v->val.d = sum / cnt;
	}
	else if (strcmp(s, "MeanIAT") == 0)
	{	v->tok = DVAL;
		sum = 0.0; cnt = 0;
		for (i = 0; i < iat_max; ++i)
		{	sum += i * (double) iat_dist[i];
// Bug fixed April 18, 1997:			cnt += ctd_dist[i];
// changed to:		(MBau)
			cnt += iat_dist[i];
		}
		v->val.d = sum / cnt;
	}
	else if (strcmp(s, "MinCTD") == 0)
	{	v->tok = IVAL;
		v->val.i = ctd_max;
		for (i = ctd_max - 1; i >= 0; --i)
			if (ctd_dist[i] != 0)
				v->val.i = i;
	}
	else if (strcmp(s, "MinIAT") == 0)
	{	v->tok = IVAL;
		v->val.i = iat_max;
		for (i = iat_max - 1; i >= 0; --i)
			if (iat_dist[i] != 0)
				v->val.i = i;
	}
	else if (strcmp(s, "MaxCTD") == 0)
	{	v->tok = IVAL;
		v->val.i = 0;
		for (i = 0; i < ctd_max; ++i)
			if (ctd_dist[i] != 0)
				v->val.i = i;
	}
	else if (strcmp(s, "MaxIAT") == 0)
	{	v->tok = IVAL;
		v->val.i = 0;
		for (i = 0; i < iat_max; ++i)
			if (iat_dist[i] != 0)
				v->val.i = i;
	}
	else	return FALSE;

	return	TRUE;
}
#endif
/*
*	export of variables
*/
int	meas2::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg)  ||
		intArray1(msg, "IAT", (int *) iat_dist, iat_max, 0) ||	// IAT table
		intArray1(msg, "CTD", (int *) ctd_dist, ctd_max, 0) ||	// CTD table
		intScalar(msg, "CTDover", (int *) &ctd_overfl) ||
		intScalar(msg, "IATover", (int *) &iat_overfl);
}
