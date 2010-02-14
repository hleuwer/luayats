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
*	Oct 29, 1997		VAL now also can be a row of a two-dimensional
*				array. (code added in the switch in init())
*					Matthias Baumann
*
*************************************************************************/

/*
*	A simple graphical measurement display:
*		- Histogramm of a distribution
*
*	Histogram hist: VALS=ms->IAT,			// the array of values to be displayed.
*							// the object must be defined and is asked by the
*							// export() method
*			{TITLE="hello world!",}		// if omitted, the title equals the object name
*			WIN=(xpos,ypos,width,height),	// window position and size
*			{MAXFREQ=20,}			// normalisation, MAXFREQ corresponds to the
*							// window height
*							// If omitted: autoscale to 1.25 * largest frequency
*			DELTA=100;			// interval between two samples (time slots)
*							// with every sample, the window is redrawn.
*/

#include "histo.h"


CONSTRUCTOR(Histo, histo);

/*
*	read the definition statement
*/
void	histo::init(void)
{
	exp_typ	msg;
	int	i;
	char	*errm;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	//	ask for the variable address
	parse_val("VALS", &msg);
	switch (msg.addrtype) {
	case exp_typ::IntArray1:
		int_val_ptr = msg.pint;
		nvals = msg.dimensions[0];
		indicdispl = msg.displacements[0];
		break;
	case exp_typ::IntArray2:
		skip('(');
		i = read_int(NULL);
		if ((errm = msg.calcIdx( &i, 0)) != NULL)
			syntax0(errm);
		skip(')');
		int_val_ptr = msg.ppint[i];
		nvals = msg.dimensions[1];
		indicdispl = msg.displacements[1];
		break;
	default:syntax0("VALS is not an integer array");
		return; // not reached
	}

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

	if (test_word("MAXFREQ"))
	{	maxfreq = read_double("MAXFREQ");
		if (maxfreq <= 0.0)
			syntax0("MAXFREQ has to be greater than zero");
		skip(',');
		autoscale = FALSE;
	}
	else	autoscale = TRUE;

	//	how often read the value and display it
	delta = read_int("DELTA");
	if (delta <= 0)
		syntax0("DELTA has to be greater than zero");

	CHECK(xsegs = new XSegment[nvals]);

	// create the window, dimensions can be changed
	createWin(ExposureMask | ButtonPressMask | StructureNotifyMask, FALSE);

	// first draw
	late( &std_evt);
}

/*
*	Time DELTA expired:
*	read the values, redraw window
*/
void	histo::late(
	event	*)
{

	X.sched();

	drawWin();

	//	register for next sample values
	alarml( &std_evt, delta);
}

//	draw lines and digits:
//	perhaps, it is inefficient to clear the whole window and redraw it completely.
//	but for the moment, it works ...
void	histo::drawWin(void)
{
	int	i, y;
	double	sumvals;

	if (int_val_ptr == NULL)
		errm0("internal error: fatal in histo::drawWin()");

	XClearWindow(X.display, mywindow);

	sumvals = 0.0;
	for (i = 0; i < nvals; ++i)
		sumvals += (double) int_val_ptr[i];
	if (sumvals == 0.0)
		sumvals = 1.0;

	if (autoscale)
	{	maxfreq = 0.0;
		for (i = 0; i < nvals; ++i)
			if (int_val_ptr[i] / sumvals > maxfreq)
				maxfreq = int_val_ptr[i] / sumvals;
		maxfreq *= AUTOSCALE;
		if (maxfreq <= 0.0)
			maxfreq = 1.0;
	}
		
	for (i = 0; i < nvals; ++i)
	{	xsegs[i].x1 = xsegs[i].x2 = (int) (width * i / (double) nvals);
		xsegs[i].y1 = height;
		y = (int) (height * (1.0 - (int_val_ptr[i] / sumvals) / maxfreq));
		if (y > height)
			y = height;
		else if (y < 0)
			y = 0;
		xsegs[i].y2 = y;
	}
	XDrawSegments(X.display, mywindow, X.stdgc, xsegs, nvals);

	char	str[50];
	sprintf(str, "min x: %d", 0 + indicdispl);
	XDrawImageString(X.display, mywindow, X.stdgc, 5, 15, str, strlen(str));
	sprintf(str, "max x: %d", nvals - 1 + indicdispl);
	XDrawImageString(X.display, mywindow, X.stdgc, 5, 30, str, strlen(str));
	sprintf(str, "full y: %g", maxfreq);
	XDrawImageString(X.display, mywindow, X.stdgc, width - 120, 15, str, strlen(str));
}

void	histo::mouseEvt(
	XEvent	*)
{
	char	buffer[50];

	TextWin	tmp("Snapshot", "Please enter file name:", buffer, 50, xpos, ypos);
	if (buffer[0] != 0)
		store2file(buffer);
}

void	histo::store2file(
	char	*snapFile)
{
	FILE	*fp;
	int	i;
	double	sumvals;

	if ((fp = fopen(snapFile, "r")) != NULL)
	{	fclose(fp);
		YesNoWin tmp("Overwrite existing file?", xpos + 50, ypos + 100, &i);
		if (i == FALSE)
			return;
	}

	if ((fp = fopen(snapFile, "w")) == NULL)
	{	InfoWin	tmp("  Could not open file", xpos + 50, ypos + 100);
		return;
	}

	fprintf(fp, "#\n#\tYATS:\n#\tSnapshot of Histogram object `%s', TITLE=\"%s\"\n", name, title);
	fprintf(fp, "#\tSimTime = %d\n#\n", SimTime);
	fprintf(fp, "#\tfirst column: value\n");
	fprintf(fp, "#\tsecond column: absolute frequency\n");
	fprintf(fp, "#\tthird column: relative frequency\n#\n");

	sumvals = 0.0;
	for (i = 0; i < nvals; ++i)
		sumvals += (double) int_val_ptr[i];
	if (sumvals == 0.0)
		sumvals = 1.0;

	for (i = 0; i < nvals; ++i)
		fprintf(fp, "%d\t%d\t%e\n", i + indicdispl,
				int_val_ptr[i], int_val_ptr[i] / sumvals);

	fclose(fp);
}
