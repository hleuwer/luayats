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
*************************************************************************/
#ifndef	_X11OBJ_H_
#define	_X11OBJ_H_

#include "inxout.h"

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}
class	X11obj:	public	inxout	{
typedef	inxout	baseclass;
public:
		X11obj(void);

	void	rdWinPosSize(void);		// parse window position and size (WIN=(...))
	void	createWin(unsigned long, int);	// create the window

	void	parse_val(char *, exp_typ *);	// for displays: parse the value name

	virtual	void	drawWin(void){}		// called on Expose
	virtual	void	mouseEvt(XEvent *){}	// called on ButtonPress
	virtual	void	configWin(XEvent *);	// called on ConfigureNotify
	virtual	void	defaultEvt(XEvent *){}	// called on all other X Events
#ifdef USELUA
	int getexp(exp_typ *);
	root *exp_obj;
	char *exp_varname;
	int exp_idx;
	int exp_idx2;
#endif
	char	*title;	// window title

	int	xpos;	// window dimensions
	int	ypos;
	int	width;
	int	height;

	Window	mywindow;
};

/*
*	Class to manage X connection
*/
class	XClass	{
public:
		XClass(void);
	void	init(void);
	void	sched(void);
	void	block(void);
	void	addWin(X11obj *, Window);
	void	delWin(Window);

	void	drawAll(void);

	void	wrDump(char *, int, int, int, int);
	void	rdDump(char *, int *, int *, int *, int *);
	void	setDumpFile(char *);
	char	*dumpfile;

	Display	*display;
	int	screen;
	XColor	red, green, yellow;
	unsigned long	whitePixel, blackPixel;
	GC	stdgc;
	GC	fgRed, bgRed;
	GC	fgGreen, bgGreen;
	GC	fgYellow, bgYellow;

	void	allocColor(XColor *, unsigned short, unsigned short, unsigned short, char *);

//private:
	struct	xwin	{
		X11obj	*obj;
		Window	win;
		xwin	*next;
	};
	xwin	*xwinList;

};

extern	XClass	X;

class	popup: public X11obj {
public:
		popup(char *, int, int, int, int);
	virtual	~popup(void);

	int	entered;
	char	*prompt;
};
class	YesNoWin: public popup {
public:
		YesNoWin(char *, int, int, int *);
	void	mouseEvt(XEvent *);
	void	drawWin(void);

	int	result;
};


#include <ctype.h>
class	TextWin: public popup {
public:
		TextWin(char *, char *, char *, int, int, int);
	void	drawWin(void);
	void	mouseEvt(XEvent *);
	void	defaultEvt(XEvent *);

	char	*buffer;
	int	blen;
	int	bpos;
	char	theChar;
	int	result;

	XComposeStatus	cs;
};

class	InfoWin: public popup {
public:
		InfoWin(char *, int, int);
	void	drawWin(void);
	void	mouseEvt(XEvent *);
};
#endif	// _X11OBJ_H_
