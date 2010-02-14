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
*	History:
*	July 4, 1997: 	- X.Sched() is only called when redrawing the
*			  window, see meter::late()
*				Matthias Baumann
*
*	Oct 29, 1997	Value now can be an element of a two-dimensional array.
*			Code added in the switch in init(). exp_typ::calcIdx() used.
*				Matthias Baumann
*
*************************************************************************/


/*
A simple graphical measurement display:
- sliding time history of a value

The meter is updated during the late slot phase. Thus, variables manipulated during the late phase can not be displayed exactly with a resolution of 1 time slot (undefined processing
order between variable manipulation and display).

Meter	m: VAL=ms->Count,	// the value to be displayed.
								// the object must be defined and is asked by the
								// export() method
	{TITLE="hello world!",}		// if omitted, the title equals the object name
	WIN=(xpos,ypos,width,height),	// window position and size
	MODE={AbsMode | DiffMode},		// AbsMode: display the sample value itself
      	      	             	// DiffMode: display differences between  	
											// subsequent samples
	{DODRAWING=0|1|2|3,} // 0:only value; 1:only drawing; 2: value and drawing
								// 3:value (only y) and drawing (default 2)
	{DRAWMODE=<bits>,}	// can be given as hex number, e.g. 0x0F
								// bit0-drawing,
								// bit1 -y-lastvalue,
								// bit2 -y-lastvaluedesription
								// bit3 -y-fullvalue
								// bit4 -x-value
								// default - all bits set	
	{LINEMODE=0|1,}   	// 0: points are drawn, 1: lines are drawn; default 1
  	{LINCOLOR=<int>,}    // 1:black (default) 2:red 3:green 4:yellow
	NVALS=100,			// # of sample values displayed
	MAXVAL=20,			// normalisation, MAXVAL corresponds to the window height
	DELTA=100			// interval between two samples (time slots)
	{,UPDATE=5};		// the widow is only updated with every 5th sample
 */

 
#include "x11obj.h"

#ifdef USELUA
#include "yats.h"
#else
class	meter:	public	X11obj	{
typedef	X11obj	baseclass;
public:
	void	init(void);
	void	late(event *);
	void	connect(void) {}

	void	drawWin(void);
	void	mouseEvt(XEvent *);
	void	store2file(char *);

	enum	{
		AbsMode,
		DiffMode
	}	display_mode;
	int    drawmode;
	int    linemode;
	int    linecolor;
	tim_typ	delta;
	int	update;
	int	update_cnt;

	double	maxval;

	XSegment	*xsegs;

	double	the_old_value;
	double	the_value;

	int	*int_val_ptr;
	double	*double_val_ptr;

	double	*vals;
	double	*first_val;
	double	*last_val;
	int	nvals;

	tim_typ	lastScanTime;
};
#endif

CONSTRUCTOR(Meter, meter);

/*
*	read the definition statement
*/
void	meter::init(void)
{
	int	i, j;
	exp_typ	msg;
	char	*errm;
#ifndef USELUA
	// In order to control this via Lua.
	int do_drawing;
#endif
	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	//	ask for the variable address
	int_val_ptr = NULL;
	double_val_ptr = NULL;
	parse_val("VAL", &msg);
	switch (msg.addrtype) {
	case exp_typ::IntScalar:
		int_val_ptr = msg.pint;
		break;
	case exp_typ::DoubleScalar:
		double_val_ptr = msg.pdbl;
		break;
	case exp_typ::IntArray1:
	case exp_typ::DoubleArray1:
		skip('(');
		i = read_int(NULL);
		if ((errm = msg.calcIdx( &i, 0)) != NULL)
			syntax0(errm);
		skip(')');
		if (msg.addrtype == exp_typ::IntArray1)
			int_val_ptr = &msg.pint[i];
		else	double_val_ptr = &msg.pdbl[i];
		break;
	case exp_typ::IntArray2:
	case exp_typ::DoubleArray2:
		skip('(');
		i = read_int(NULL);
		if ((errm = msg.calcIdx( &i, 0)) != NULL)
			syntax0(errm);
		skip(',');
		j = read_int(NULL);
		if ((errm = msg.calcIdx( &j, 1)) != NULL)
			syntax0(errm);
		skip(')');
		if (msg.addrtype == exp_typ::IntArray2)
			int_val_ptr = &msg.ppint[i][j];
		else	double_val_ptr = &msg.ppdbl[i][j];
		break;
	default:syntax0("VAL is neither an integer nor a double scalar");
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

	//	examine the display mode
	skip_word("MODE");
	skip('=');
	if (test_word("AbsMode"))
	{	skip_word("AbsMode");
		display_mode = AbsMode;
	}
	else if (test_word("DiffMode"))
	{	skip_word("DiffMode");
		display_mode = DiffMode;
	}
	else	syntax0("unknown mode");
	skip(',');

	if (test_word("DODRAWING"))
	{
	    do_drawing = read_int("DODRAWING");
	    skip(',');
	    if(do_drawing < 0 || do_drawing >3)
 	       syntax0("DODRAWING must be 0,1,2,3");
	}
	else
	   do_drawing = 2;


	if (test_word("DRAWMODE"))
	{
	    drawmode = (int) read_double("DRAWMODE");	// we read double to allow hex values
	    skip(',');
	    if(drawmode < 0)
 	       syntax0("DRAWMODE is a bitvector and must be >0");
	}
	else
	{
	   switch(do_drawing) {
		case 0: drawmode = 0x02; break;	// only y-value
		case 1: drawmode = 0x01; break;	// only drawing
		case 2: drawmode = 0xFF; break;	// value and drawing (default)
		case 3: drawmode = 0x0F; break;	// only drawing
		default: drawmode = 0xFF; break;
		}
	}


	if (test_word("LINEMODE"))
	{
	    linemode = read_int("LINEMODE");
	    skip(',');
	    if(linemode < 0 || linemode > 1)
 	       syntax0("LINEMODE must be 0 or 1");
	}
	else
	   linemode = 1;
	   
	if (test_word("LINECOLOR"))
	{
	    linecolor = read_int("LINECOLOR");
	    skip(',');
	}
	else
	   linecolor = 1; // black
	   
	   
	//	number of values, normatisation constant
	nvals = read_int("NVALS");
	if (nvals < 1)
		syntax();
	skip(',');
	maxval = read_double("MAXVAL");
	if (maxval <= 0.0)
		syntax0("MAXVAL has to be greater than zero");
	skip(',');

	//	how often read the value, how often display it
	delta = read_int("DELTA");
	if (delta <= 0)
		syntax0("DELTA has to be greater than zero");

	if (token == ',')
	{	skip(',');
		update = read_int("UPDATE");
		if (update <= 0)
			syntax0("UPDATE has to be greater than zero");
	}
	else	update = 1;
	update_cnt = 0;

	CHECK(vals = new double[nvals]);
	first_val = vals;
	last_val = vals + nvals - 1;
	for (i = 0; i < nvals; ++i)
		vals[i] = 0;
	CHECK(xsegs = new XSegment[nvals]);

	the_old_value = 0.0;
	the_value = 0.0;
	lastScanTime = 0;

	// create the window, dimensions can be changed
	createWin(ExposureMask | ButtonPressMask | StructureNotifyMask, FALSE);

	// first draw
	update_cnt = update - 1;
	late( &std_evt);
}

/*
*	Time DELTA expired:
*	read the value, redraw window if necessary
*/
void	meter::late(
	event	*)
{

//	X.sched();	// shifted down: only called when window is redrawn
			// July 4, MBau

	//	rotate value buffer pointers
	last_val = first_val++;
	if (first_val == vals + nvals)
		first_val = vals;

	//	read new value
	if (int_val_ptr != NULL)
		the_value = (double) *int_val_ptr;
	else if (double_val_ptr != NULL)
		the_value = *double_val_ptr;
	lastScanTime = SimTime;

	//	what to display depends on the mode
	switch (this->display_mode) {
	case AbsMode:
		*last_val = the_value;
		the_old_value = the_value;
		break;
	case DiffMode:
		*last_val = the_value - the_old_value;
		the_old_value = the_value;
		break;
	default:
	  printf("display_mode = %d 0x%08X\n", display_mode, (int) display_mode);
	  errm1s("%s: internal error: invalid display_mode in meter::late()", name);
	}

	//	update window?
	if ( ++update_cnt == update)
	{	update_cnt = 0;
		X.sched();
		drawWin();
	}

	//	register for next sample value
	alarml( &std_evt, delta);

}

//	draw lines and digits:
//	perhaps, it is inefficient to clear the whole window and redraw it completely.
//	but for the moment, it works ...
void	meter::drawWin(void)
{
     int	i, y;
     double	*pvals;
     char	str[50];

     XClearWindow(X.display, mywindow);



   if(drawmode & 0x01)	// if graphics is to be drawn
   {
      pvals = first_val;
      for (i = 0; i < nvals; ++i)
      {	xsegs[i].x1 = xsegs[i].x2 = (int) (width * i / (double) nvals);
	     xsegs[i].y1 = height;
	     y = (int) (height * (1.0 - *pvals / maxval));
	     if (y > height)
		     y = height;
	     else if (y < 0)
		     y = 0;
	     xsegs[i].y2 = y;
	     if ( ++pvals == vals + nvals)
		     pvals = vals;
	    
	     if(linemode == 0) // draw only a point
	        xsegs[i].y1 = xsegs[i].y2;
      }
      
      switch(linecolor)
      {
         case 2: XDrawSegments(X.display, mywindow, X.fgRed, xsegs, nvals);
	    break;
         case 3: XDrawSegments(X.display, mywindow, X.fgGreen, xsegs, nvals);
	    break;
         case 4: XDrawSegments(X.display, mywindow, X.fgYellow, xsegs, nvals);
	    break;
         default: XDrawSegments(X.display, mywindow, X.stdgc, xsegs, nvals);
	     break;
      }
   
   }


	// draw first the x and then overwrite (if too few space) with y-values
   if(drawmode & 0x10)	// the x-value, description, and full value
	{
		sprintf(str, "last x: slot %u", lastScanTime);
		XDrawImageString(X.display, mywindow, X.stdgc, width - 120, 15, str, strlen(str));
		sprintf(str, "full x: %u slots", (unsigned int) nvals * delta);
		XDrawImageString(X.display, mywindow, X.stdgc, width - 120, 30, str, strlen(str));	  
	}

	if((drawmode & 0x02) && !(drawmode &0x04))	// the y-value
	{
     sprintf(str, "%g", *last_val);
     XDrawImageString(X.display, mywindow, X.stdgc, 5, 15, str, strlen(str));
	}

	if(drawmode & 0x04)	// the y-value and description (overwrites)
	{
   	sprintf(str, "last y: %g", *last_val);
      XDrawImageString(X.display, mywindow, X.stdgc, 5, 15, str, strlen(str));
	}

	if(drawmode & 0x08)	// the y-value and description
	{
     sprintf(str, "full y: %g", maxval);
     XDrawImageString(X.display, mywindow, X.stdgc, 5, 30, str, strlen(str));
	}
 
}

void	meter::mouseEvt(
	XEvent	*)
{
	char	buffer[50];

	TextWin	tmp("Snapshot", "Please enter file name:", buffer, 50, xpos, ypos);
	if (buffer[0] != 0)
		store2file(buffer);
}

void	meter::store2file(
	char	*snapFile)
{
	int	i;
	FILE	*fp;
	double	*pv;

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

	fprintf(fp, "#\n#\tYATS:\n#\tSnapshot of Meter object `%s', TITLE=\"%s\"\n#\n", name, title);
	fprintf(fp, "#\tfirst column: SimTime\n");
	fprintf(fp, "#\tsecond column: value\n#\n");

	pv = first_val;
	i = nvals - 1;
	while (i >= 0)
	{	if (lastScanTime >= delta * i)
			fprintf(fp, "%d\t%e\n", lastScanTime - delta * i, *pv);
		--i;
		if ( ++pv >= vals + nvals)
			pv = vals;
	}

	fclose(fp);
}
