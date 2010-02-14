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
*	Creation:		July 1996
*
*	History
*	Oct 29, 1997		VAL now also can be an element of a two-dimensional
*				array. exp_typ::calcIdx() used. Code added in the
*				switch in init().
*					Matthias Baumann
*
*************************************************************************/

/*
*	A simple graphical measurement display:
*		- produces the distribution of a value and displays it
*
*	Histo2 hist: VAL=mux->QLen,			// the value of which to produce the distrib.
*							// the object must be defined and is asked by the
*							// export() method
*			{TITLE="hello world!",}		// if omitted, the title equals the object name
*			WIN=(xpos,ypos,width,height),	// window position and size
*			NVALS=100			// value range to be displayed (0 ... 99)
*			{,MAXFREQ=20}			// normalisation, MAXFREQ corresponds to the
*							// window height
*							// If omitted: autoscale to 1.25 * largest frequency
*			{,DELTA=100}			// interval between two samples (time slots)
*							// default: 1
*			{, UPDATE=10};			// with every UPDATE-th sample, the window is redrawn.
*							// default: 1
*
*	Commands:
*		histo->Dist(i)
*			// returns the histogramm counter i
*		histo->ResDist
*			// resets all counters
*/

#include "histo.h"

#ifndef USELUA
class	histo2: public histo {
typedef	histo	baseclass;
public:
	void	init(void);
	void	late(event *);
	int	export(exp_typ *);
	int	command(char *, tok_typ *);

	int	update;
	int	update_cnt;
	int	*the_val_ptr;

	int	over_under;
};
#endif
CONSTRUCTOR(Histo2, histo2);

/*
*	read the definition statement
*/
void	histo2::init(void)
{
	int	i, j;
	int	cont;
	exp_typ	msg;
	char	*errm;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	//	ask for the variable address
	parse_val("VAL", &msg);
	switch (msg.addrtype) {
	case exp_typ::IntScalar:
		the_val_ptr = msg.pint;
		break;
	case exp_typ::IntArray1:
		skip('(');
		i = read_int(NULL);
		if ((errm = msg.calcIdx( &i, 0)) != NULL)
			syntax0(errm);
		skip(')');
		the_val_ptr = &msg.pint[i];
		break;
	case exp_typ::IntArray2:
		skip('(');
		i = read_int(NULL);
		if ((errm = msg.calcIdx( &i, 0)) != NULL)
			syntax0(errm);
		skip(',');
		j = read_int(NULL);
		if ((errm = msg.calcIdx( &j, 1)) != NULL)
			syntax0(errm);
		skip(')');
		the_val_ptr = &msg.ppint[i][j];
		break;
	default:syntax0("VAL is not an integer scalar");
	}
	indicdispl = 0;
	skip(',');

	//	TITLE given?
	if (test_word("TITLE"))
	{	title = read_string("TITLE");
		skip(',');
	}
	else	title = name;

	//	parse window position and size
	rdWinPosSize();
	skip(',');

	nvals = read_int("NVALS");
	if (nvals < 1)
		syntax0("invalid NVALS");

	if (token == ',')
	{	skip(',');
		cont = TRUE;
	}
	else	cont = FALSE;

	if (cont && test_word("MAXFREQ"))
	{	maxfreq = read_double("MAXFREQ");
		if (maxfreq <= 0.0)
			syntax0("MAXFREQ has to be greater than zero");
		autoscale = FALSE;
		if (token == ',')
			skip(',');
		else	cont = FALSE;
	}
	else	autoscale = TRUE;

	//	how often read the value and display it
	if (cont && test_word("DELTA"))
	{	delta = read_int("DELTA");
		if (delta <= 0)
			syntax0("DELTA has to be greater than zero");
		if (token == ',')
			skip(',');
		else	cont = FALSE;
	}
	else	delta = 1;

	if (cont)
	{	update = read_int("UPDATE");
		if (update < 1)
			syntax0("invalid UPDATE");
	}
	else	update = 1;
	update_cnt = 0;

	CHECK(int_val_ptr = new int[nvals]);
	for (i = 0; i < nvals; ++i)
		int_val_ptr[i] = 0;
	over_under = 0;

	CHECK(xsegs = new XSegment[nvals]);

	// create the window, dimensions can be changed
	createWin(ExposureMask | ButtonPressMask | StructureNotifyMask, FALSE);

	// first draw
	update_cnt = update - 1;
	late( &std_evt);
}

/*
*	Time DELTA expired:
*	read the values.
*	redraw window, if necessary
*/
void	histo2::late(
	event	*)
{
	int	i;

	i = *the_val_ptr;
	if (i < 0 || i >= nvals)
		++over_under;
	else	++int_val_ptr[i];

	if ( ++update_cnt >= update)
	{	update_cnt = 0;
		X.sched();	// calling X every times would slow down the simulator
				// (usually, DELTA is 1)
		drawWin();
	}

	//	register for next sample values
	alarml( &std_evt, delta);
}

/*
*	export the distribution
*/
int	histo2::export(
	exp_typ	*msg)
{
	return baseclass::export(msg) ||
		intArray1(msg, "Dist", int_val_ptr, nvals, 0);
}

/*
*	Command:
*		hist->ResDist
*	resets all counters
*/
int	histo2::command(
	char	*s,
	tok_typ	*pv)
{
	int	i;

	if (baseclass::command(s, pv))
		return TRUE;

	pv->tok = NILVAR;
	if (strcmp(s, "ResDist") == 0)
	{	for (i = 0; i < nvals; ++i)
			int_val_ptr[i] = 0;
	}
	else	return FALSE;

	return TRUE;
}
