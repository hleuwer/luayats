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
*************************************************************************/

/*
*	A simple control display for YATS:
*		- the simulation time is displayed
*		- YATS can be stopped and started with a mouse click on the window
*		- the simulation speed can be decreased
*
*	The control object stops the simulator in the early slot phase. It is not defined,
*	which of the events scheduled for this instant are already processed and which not.
*	Since the Meter and Histogram objects, however, display their values in the late phase,
*	it is ensured that all meters and histograms are synchronized during a sleep of YATS.
*
*	Control: POS=(50,50), DELTA=100 {,SLEEP=10000}, {FILE="DUMPFILE" {,CORR=(5,30)}}; 
*			// POS: position of the control window
*			// DELTA: how often (time slots) to redraw the simulation time
*			//	(this also are the instants where the mouse click is recognized)
*			// SLEEP (if given): sleep for this amount of micro seconds after each window
*			//	actualisation
*			// FILE (if given): file where positions and sizes of graphical displays are
*			//	stored. The values stored there overwrite the input file specs.
*			//	CORR:	gives the x and y position differencies caused by the
*			//		extra windows of the window manager (borders). The default
*			//		values are CORR=(5,30).
*/

#include "x11obj.h"

#include <sys/time.h>
#include <fcntl.h>

#ifdef	EBZAW
#include <sys/select.h>
#endif

#ifndef USELUA

#define	WIDTH	(150)
#define	HEIGHT	(80)

class	control:	public	X11obj	{
typedef	X11obj	baseclass;

public:
	void	init(void);
	void	drawWin(void);
	void	mouseEvt(XEvent *);
	void	connect(void) {}
	void	early(event *);

	int	delta;
	int	delay;

	int	running;

	struct	timeval	tftarget;
};
#else
#include "yats.h"
#endif
CONSTRUCTOR(Control, control);
#ifndef USELUA
root	*controlObjPtr = NULL;	// for others who want to check on a control object
#endif
void	control::init(void)
{
	int	cont;

	if (controlObjPtr != NULL)
		syntax0("only one Control window allowed");
	controlObjPtr = this;

	skip(CLASS);
	title = name = "YATS";
	skip(':');

	skip_word("POS");
	skip('=');
	skip('(');
	xpos = read_int(NULL);
	skip(',');
	ypos = read_int(NULL);
	skip(')');

	skip(',');
	delta = read_int("DELTA");

	if (token == ',')
	{	skip(',');
		cont = TRUE;
	}
	else	cont = FALSE;

	delay = 0;
	if (cont && test_word("SLEEP"))
	{	delay = read_int("SLEEP");
		if (token == ',')
			skip(',');
		else	cont = FALSE;
	}

	if (cont && test_word("FILE"))
	{	int	dx, dy;

		X.setDumpFile(read_string("FILE"));
		if (token == ',')
		{	skip(',');
			skip_word("CORR");
			skip('=');
			skip('(');
			dx = read_int(NULL);
			skip(',');
			dy = read_int(NULL);
			skip(')');
		}
		else
		{	dx = 6;
			dy = 25;
		}
		X.wrDump("@CORR@", dx, dy, 0, 0);
		X.rdDump("YATS", &xpos, &ypos, &width, &height);
	}

	//	reset and fix window dimensions
	width = WIDTH;
	height = HEIGHT;
	createWin(ExposureMask | ButtonPressMask | StructureNotifyMask, TRUE);

	running = FALSE;
	alarme( &std_evt, 0);

}

//	draw the window contents
void	control::drawWin(void)
{
	char	str[50];
	static	char	stop[] = "STOP";
	static	char	start[] = "START";
	static	char	xit[] = "EXIT";

	XClearWindow(X.display, mywindow);

	sprintf(str, "SimTime: %u", SimTime);
	XDrawImageString(X.display, mywindow, X.stdgc, 20, 17, str, strlen(str));
	sprintf(str, "SimTime/s: %.3f", SimTimeReal);
	XDrawImageString(X.display, mywindow, X.stdgc, 20, 70, str, strlen(str));

	if (running == TRUE)
		XFillRectangle(X.display, mywindow, X.fgYellow, 10, 25, 60, 30);
	else	XFillRectangle(X.display, mywindow, X.fgGreen, 10, 25, 60, 30);

	XFillRectangle(X.display, mywindow, X.fgRed, 80, 25, 60, 30);

	XDrawRectangle(X.display, mywindow, X.stdgc, 10, 25, 60, 30);
	XDrawRectangle(X.display, mywindow, X.stdgc, 80, 25, 60, 30);

	if (running == TRUE)
		XDrawImageString(X.display, mywindow, X.bgYellow, 29, 45, stop, strlen(stop));
	else	XDrawImageString(X.display, mywindow, X.bgGreen, 25, 45, start, strlen(start));

	XDrawImageString(X.display, mywindow, X.bgRed, 99, 45, xit, strlen(xit));
}

//	mouse event
void	control::mouseEvt(
	XEvent	*evt)
{
	if (evt->type != ButtonPress)
		return;
	if (evt->xbutton.x >= 10 && evt->xbutton.x < 70 &&
		evt->xbutton.y >= 25 && evt->xbutton.y < 55)
	{	if (running == FALSE)
			running = TRUE;
		else	running = FALSE;
		drawWin();
	}
	else if (evt->xbutton.x >= 80 && evt->xbutton.x < 140 &&
		evt->xbutton.y >= 25 && evt->xbutton.y < 55)
	{	int	res;
		YesNoWin tmp("      Really Exit?", xpos + 10, ypos + 10, &res);
		if (res == TRUE)
			exit(0);
	}
}
/*
*	- redraw the window
*	- stop the simulation if state stop
*	- wait, if SLEEP was given and we are too fast
*/
void	control::early(
	event	*)
{
	int	was_stopped;

	X.sched();

	if (running == FALSE)
	{	X.drawAll();	// update all windows,
				// also display new state and time
		do	X.block();
		while	(running == FALSE);
		gettimeofday( &tftarget, NULL);
		was_stopped = TRUE;
	}
	else	was_stopped = FALSE;
		
	drawWin();	// display new state and time
	alarme( &std_evt, delta);

	if (delay > 0 && was_stopped == FALSE)
	{
		struct	timeval	t2;
	
		/*
		*	algorithm for sleeping taken from mpeg2play
		*/

		// compute new target time
  		tftarget.tv_usec += delay;
		while (tftarget.tv_usec >= 1000000)
		{	tftarget.tv_usec -= 1000000;
			++tftarget.tv_sec;
		}
	
		// here we are
		gettimeofday( &t2, NULL);

		// the difference
  		t2.tv_usec = tftarget.tv_usec - t2.tv_usec;
		t2.tv_sec  = tftarget.tv_sec  - t2.tv_sec;
		while (t2.tv_usec < 0)
		{	t2.tv_usec += 1000000;
			t2.tv_sec--;
		}

		// See if we are already lagging behind
		if (t2.tv_sec < 0 || (t2.tv_sec == 0 && t2.tv_usec <= 0))
			;	// void
		else	// Spin for awhile
			select(0,NULL,NULL,NULL,&t2);
	}
}


