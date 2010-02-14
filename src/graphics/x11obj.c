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
*	History:	July 4, 1997:
*			- XClass::allocColor(): more "soft" reaction on
*			  unavailable colours, simply set to white
*			  (instead of terminating)
*				Matthias Baumann
*
*************************************************************************/

/*
*	- a basic class for X11 display objects: X11obj
*	- a class for interfacing the X system: XClass (has only one instance, X)
*		(also provides methods to read and write a simple object location file)
*	- classes for
*		+ a Yes-No-window (YesNoWin)
*		+ an info window (InfoWin)
*		+ a text input window (TextWin)
*/

#include "x11obj.h"

/************************************************************************/
/*
*	X11obj class
*/
X11obj::X11obj(void)
{
	X.init();
}

/*
*	createWin():
*	contains all X library calls to create the window
*/
void	X11obj::createWin(
	unsigned long	eventMask,	// X events to receive
	int		fixDims)	// TRUE: window dimensions fixed
{
	XSizeHints	hint;

	hint.x = xpos; hint.y = ypos;
	hint.width = width; hint.height = height;
	hint.flags = USPosition | USSize;
	if (fixDims)
	{	hint.min_width = width; hint.min_height = height;
		hint.max_width = width; hint.max_height = height;
		hint.flags |= PMinSize | PMaxSize;
	}

	mywindow = XCreateSimpleWindow(X.display, DefaultRootWindow(X.display),
			xpos, ypos, width, height,
			0, BlackPixel(X.display, X.screen), WhitePixel(X.display, X.screen));

	XSetStandardProperties(X.display, mywindow, title, title, None, NULL, 0, &hint);

	XSelectInput(X.display, mywindow, eventMask);

	X.addWin(this, mywindow);

	XMapRaised(X.display, mywindow);
}

/*
*	parse_val():
*	parse a value name and ask the corresponding object for the address,
*	fill the msg structure
*/
void	X11obj::parse_val(
	char	*keyw,
	exp_typ	*msg)
{
	char	*s;
	root	*obj;

	//	ask for the variable address
	s = read_suc(keyw);
	if ((msg->varname = strstr(s, "->")) == NULL)
		syntax1s("%s must contain a `->'", keyw == NULL ? (char *)"variable name" : keyw);
	else	msg->varname += 2;
	msg->ninds = 0;
	if ((obj = find_obj(s)) == NULL)
		syntax2s("%s: could not find object to ask for `%s'", name, s);
	if (obj->export(msg) == FALSE)
		syntax1s("variable `%s' unknown", s);

	delete s;
}

/*
*	process ConfigNotify X events.
*	It requires a little bit work to examine the new absolute
*	window position (needed for the config-file).
*/
void	X11obj::configWin(
	XEvent	*)
{
	int	xx, yy;
	unsigned	int	w, h, bw, dp;
	Window	root;
	Window	parent;
	Window	*children;
	Window	win, win1;
	unsigned int	nchildren;

	xpos = 0;
	ypos = 0;

	//	descend down to the root window and add all x/y values
	win = mywindow;
	do
	{	if (XQueryTree(X.display, win, &root, &parent, &children, &nchildren) == 0)
			errm1s("%s: XQueryTree() failed", name);
#define	AIX_X11_BUG	1
#ifdef	AIX_X11_BUG
		XFree((char *)children);
#else
		XFree(children);
#endif
		if (XGetGeometry(X.display, win, &win1, &xx, &yy, &w, &h, &bw, &dp) == 0)
				errm1s("%s: XGetGeometry() failed", name);
		xpos += xx;
		ypos += yy;
		if (win == mywindow)
		{	width = w;
			height = h;
		}
		win = parent;
	} while (parent != root);

//	printf("%s changed to (%d,%d,%d,%d)\n", name, xpos, ypos, width, height);

	//	write the new values to the config-file
	X.wrDump(name, xpos, ypos, width, height);
}


//	read the WIN(...) part from an object declaration.
//	The values are overridden by those from a config-file (if specified
//	by the Control object)
void	X11obj::rdWinPosSize(void)
{
	skip_word("WIN");
	skip('=');
	skip('(');
	xpos = read_int(NULL);
	skip(',');
	ypos = read_int(NULL);
	skip(',');
	width = read_int(NULL);
	skip(',');
	height = read_int(NULL);
	skip(')');
	if (xpos < 0 || ypos < 0 || width <= 0 || height <= 0)
		syntax0("invalid window dimensions");

	//	values from the config-file override those from the input text.
	X.rdDump(name, &xpos, &ypos, &width, &height);
	if (width <= 0 || height <= 0)
		syntax0("invalid window dimensions read from object location file");
}

/************************************************************************/
/*
*	X
*		interface object to X
*/
#define	BLEN	500
static	char	buff1[BLEN];
static	char	buff2[BLEN];
static	char	fmt[] = "%s\t%05d\t%05d\t%05d\t%05d\n";

XClass	X;	// the only instance of XClass

//	called by main() at end of input text
void	blockWhileWindowsOpen(void)
{
	while (X.xwinList != NULL)
		X.block();
}

XClass::XClass(void)
{
	xwinList = NULL;
	display = NULL;
	dumpfile = NULL;
}

//	set the config-file
void	XClass::setDumpFile(
	char	*s)
{
	dumpfile = s;
}

/*
*	write object location to the config-file (if existing)
*/
void	XClass::wrDump(
	char	*name,
	int	x,
	int	y,
	int 	w,
	int 	h)
{
	FILE	*fp;
	long int	pos;
	int	xx, yy, ww, hh;
	int	dx, dy, dw, dh;
	int	hit;

	if (dumpfile == NULL)
		return;

	if ((fp = fopen(dumpfile, "r+")) == NULL)
	{	if ((fp = fopen(dumpfile, "w")) == NULL)
			errm1s("could not open file `%s' to update object locations", dumpfile);

		fprintf(fp, "#\n#\tYATS object location file.\n#\tIf editing by hand necessary:\n");
		fprintf(fp, "#\t<objectName><tab><xpos>"
				"<tab><ypos><tab><width><tab><height><newLine>\n");
		fprintf(fp, "#\tAll numbers with *exactly* 5 characters, no other white space\n#\n");
		fprintf(fp, "# @CORR@ specifies border widths of the window manager (see Control object)\n");
		fprintf(fp, fmt, "@CORR@", 0, 0, 0, 0);

		fclose(fp);
		wrDump(name, x, y, w, h);
	}

	hit = FALSE;
	dx = dy = dw = dh = 0;

	buff1[BLEN - 1] = 'x';
	for (;;)
	{	pos = ftell(fp);
		//	read a line
		if (fgets(buff1, BLEN, fp) == NULL)
			break;
		if (buff1[BLEN - 1] != 'x')
			errm1s("file %s: line too long", dumpfile);
		//	comment?
		if (buff1[0] == '#')
			continue;

		if (sscanf(buff1, fmt, buff2, &xx, &yy, &ww, &hh) != 5)
			errm1s("file `%s': invalid syntax", dumpfile);

		//	searched object?
		if (strcmp(buff2, name) == 0)
		{	fseek(fp, pos, 0);
			fprintf(fp, fmt, name, x - dx, y - dy, w - dw, h - dh);
			hit = TRUE;
			break;
		}

		//	correction of values?
		if (strcmp(buff2, "@CORR@") == 0)
		{	dx = xx;
			dy = yy;
			dw = ww;
			dh = hh;
		}
	}

	if (hit == FALSE)
		fprintf(fp, fmt, name, x - dx, y - dy, w - dw, h - dh);
	
	fclose(fp);
}

/*
*	read object location from the config-file (if existing)
*/
void	XClass::rdDump(
	char	*name,
	int	*x,
	int	*y,
	int 	*w,
	int 	*h)
{
	FILE	*fp;
	int	xx, yy, ww, hh;

	if (dumpfile == NULL)
		return;

	if ((fp = fopen(dumpfile, "r")) == NULL)
		return;

	buff1[BLEN - 1] = 'x';
	while (fgets(buff1, BLEN, fp) != NULL)
	{	if (buff1[BLEN - 1] != 'x')
			errm1s("file %s: line too long", dumpfile);
		//	comment?
		if (buff1[0] == '#')
			continue;

		if (sscanf(buff1, fmt, buff2, &xx, &yy, &ww, &hh) != 5)
			errm1s("file `%s': invalid syntax", dumpfile);

		//	searched object?
		if (strcmp(buff2, name) == 0)
		{	*x = xx;
			*y = yy;
			*w = (unsigned int) ww;
			*h = (unsigned int) hh;
			break;
		}
	}

	fclose(fp);
};

/*
*	register a graphical object
*/
void	XClass::addWin(
	X11obj	*obj,
	Window	win)
{
	xwin	*p;

	CHECK(p = new xwin);
	p->next = xwinList;
	xwinList = p;

	p->obj = obj;
	p->win = win;
}

/*
*	unregister a graphical object
*/
void	XClass::delWin(
	Window	win)
{
	xwin	**pp, *p;

	pp = &xwinList;
	while ( *pp != NULL)
	{	if ((*pp)->win == win)
		{	p = *pp;
			*pp = p->next;
			delete p;
			return;
		}
		pp = &((*pp)->next);
	}

	errm0("internal error: XClass::delWin(): could not find specified window");
}

/*
*	Check on pending X events.
*	If any, look for the corresponding object, activate it
*/
void	XClass::sched(void)
{
	xwin	*p;
	XEvent	evt;
	int	nevt;

	if ((nevt = XEventsQueued(X.display, QueuedAfterFlush)) == 0)
		return;

	p = xwinList;
	while (p != NULL)
	{	while (XCheckWindowEvent(X.display, p->win, -1L, &evt) != 0)
		{	switch (evt.type) {
			case Expose:
				p->obj->drawWin();
				break;
			case ButtonPress:
				p->obj->mouseEvt( &evt);
				break;
			case ConfigureNotify:
				p->obj->configWin( &evt);
				break;
			default:p->obj->defaultEvt( &evt);
			}
			--nevt;
		}
		p = p->next;
	}

	//if (nevt > 0) printf("X::sched(): %d X events remaining\n", nevt);
}

/*
*	block until next X event
*/
void	XClass::block(void)
{
	XEvent	evt;

	XPeekEvent(display, &evt);
	sched();
}

/*
*	draw all registered X objects
*/
void	XClass::drawAll(void)
{
	xwin	*p;

	p = xwinList;
	while (p != NULL)
	{	p->obj->drawWin();
		p = p->next;
	}
}

void	XClass::allocColor(
	XColor	*c,
	unsigned short	_red,
	unsigned short	_green,
	unsigned short	_blue,
	char	*)
{
	Colormap	cmap;

	cmap = DefaultColormap(display, DefaultScreen(display));
	c->red = _red;
	c->green = _green;
	c->blue = _blue;
	if (XAllocColor(display, cmap, c) == 0)
	{	// just set it to white
		c->pixel = whitePixel;
		// errm1s("could not allocate colour `%s'", s);
	}
}

/*
*	initialize X connection
*/
void	XClass::init(void)
{
	if (display == NULL)
	{	if ((display = XOpenDisplay("")) == NULL)
			syntax0("could not open display");
		screen = DefaultScreen(display);

		whitePixel = WhitePixel(display, screen);
		blackPixel = BlackPixel(display, screen);

		stdgc = XCreateGC(display, DefaultRootWindow(display), 0, 0);
		XSetForeground(display, stdgc, blackPixel);
		XSetBackground(display, stdgc, whitePixel);
		XSetLineAttributes(display, stdgc, 0, LineSolid, CapRound, JoinRound);

		allocColor( &red, 60000, 30000, 30000, "red");
		allocColor( &green, 30000, 60000, 30000, "green");
		allocColor( &yellow, 60000, 60000, 0, "yellow");

		fgRed = XCreateGC(display, DefaultRootWindow(display), 0, 0);
		XSetForeground(display, fgRed, red.pixel);
		XSetBackground(display, fgRed, whitePixel);
		bgRed = XCreateGC(display, DefaultRootWindow(display), 0, 0);
		XSetForeground(display, bgRed, blackPixel);
		XSetBackground(display, bgRed, red.pixel);

		fgGreen = XCreateGC(display, DefaultRootWindow(display), 0, 0);
		XSetForeground(display, fgGreen, green.pixel);
		XSetBackground(display, fgGreen, whitePixel);
		bgGreen = XCreateGC(display, DefaultRootWindow(display), 0, 0);
		XSetForeground(display, bgGreen, blackPixel);
		XSetBackground(display, bgGreen, green.pixel);

		fgYellow = XCreateGC(display, DefaultRootWindow(display), 0, 0);
		XSetForeground(display, fgYellow, yellow.pixel);
		XSetBackground(display, fgYellow, whitePixel);
		bgYellow = XCreateGC(display, DefaultRootWindow(display), 0, 0);
		XSetForeground(display, bgYellow, blackPixel);
		XSetBackground(display, bgYellow, yellow.pixel);
	}
}


/************************************************************************/
/*
*	popup and yes-no windows
*/
popup::popup(
	char	*s,
	int	x,
	int	y,
	int	w,
	int	h)
{
	entered = FALSE;
	xpos = x; ypos = y;
	width = w; height = h;
	title = name = s;
	createWin(ExposureMask | ButtonPressMask | KeyPressMask, TRUE);

}
popup::~popup(void)
{
	XDestroyWindow(X.display, mywindow);
	X.delWin(mywindow);
}

YesNoWin::YesNoWin(
	char	*s,
	int	x,
	int	y,
	int	*res): popup("Prompt", x, y, 150, 60)
{
	prompt = s;
	do	X.block();
	while	(entered == FALSE);
	*res = result;
}
void	YesNoWin::drawWin(void)
{
	static	char	yes[] = "Yes";
	static	char	no[] = "No";

	XClearWindow(X.display, mywindow);

	XDrawImageString(X.display, mywindow, X.stdgc, 5, 17, prompt, strlen(prompt));

	XFillRectangle(X.display, mywindow, X.fgGreen, 10, 25, 60, 30);
	XDrawRectangle(X.display, mywindow, X.stdgc, 10, 25, 60, 30);
	XDrawImageString(X.display, mywindow, X.bgGreen, 30, 45, yes, strlen(yes));
	XFillRectangle(X.display, mywindow, X.fgRed, 80, 25, 60, 30);
	XDrawRectangle(X.display, mywindow, X.stdgc, 80, 25, 60, 30);
	XDrawImageString(X.display, mywindow, X.bgRed, 103, 45, no, strlen(no));
}
void	YesNoWin::mouseEvt(
	XEvent	*evt)
{
	if (evt->type != ButtonPress)
		return;
	if (evt->xbutton.x >= 10 && evt->xbutton.x < 70 &&
		evt->xbutton.y >= 25 && evt->xbutton.y < 55)
	{	result = TRUE;
		entered = TRUE;
	}
	else if (evt->xbutton.x >= 80 && evt->xbutton.x < 140 &&
		evt->xbutton.y >= 25 && evt->xbutton.y < 55)
	{	result = FALSE;
		entered = TRUE;
	}
}

TextWin::TextWin(
	char	*t,	// title
	char	*pr,	// prompt text
	char	*buff,	// where to write result
	int	bl,	// len of buffer
	int	x,
	int	y): popup(t, x, y, 200, 80)
{
	if (bl < 2)
		errm0("internal error: TextWin::TextWin(): blen too small");

	buffer = buff;
	blen = bl;
	prompt = pr;

	bpos = 0;
	buffer[bpos] = '_';
	result = TRUE;
	for (;;)
	{	theChar = 0;
		do	X.block();
		while	(theChar == 0);

		if (theChar == '\r')
		{	buffer[bpos] = 0;
			if (result != TRUE)
				buffer[0] = 0;
			--bpos;		// ensure right display until object destrcuction 
			return;
		}
		else if (theChar == '\b')
		{	if (bpos > 0)
				--bpos;
		}
		else if (isgraph(theChar))
		{	buffer[bpos] = theChar;
			if (bpos < blen - 2)	// leave place for the '_'
				++bpos;
		}
		buffer[bpos] = '_';
		drawWin();
	}
}
	
void	TextWin::drawWin(void)
{
	static	char	ok[] = "OK";
	static	char	cancel[] = "Cancel";

	XClearWindow(X.display, mywindow);

	XDrawImageString(X.display, mywindow, X.stdgc, 10, 15, prompt, strlen(prompt));
	XDrawImageString(X.display, mywindow, X.stdgc, 10, 40, buffer, bpos + 1);

	XFillRectangle(X.display, mywindow, X.fgGreen, 40, 55, 40, 20);
	XDrawRectangle(X.display, mywindow, X.stdgc, 40, 55, 40, 20);
	XDrawImageString(X.display, mywindow, X.bgGreen, 55, 70, ok, strlen(ok));
	XFillRectangle(X.display, mywindow, X.fgRed, 120, 55, 40, 20);
	XDrawRectangle(X.display, mywindow, X.stdgc, 120, 55, 40, 20);
	XDrawImageString(X.display, mywindow, X.bgRed, 123, 70, cancel, strlen(cancel));
}

#include <X11/keysym.h>
void	TextWin::defaultEvt(
	XEvent	*evt)
{
	char	b[10];
	KeySym	key;

	if (evt->type != KeyPress)
		return;

	if (XLookupString((XKeyEvent *) evt, b, 10, &key, &cs) != 0)
	{	switch (key) {
		case XK_BackSpace:
		case XK_Delete:
			theChar = '\b';
			break;
		case XK_Return:
			theChar = '\r';
			break;
		default:theChar = b[0];
			break;
		}
	}
}

void	TextWin::mouseEvt(
	XEvent	*evt)
{
	if (evt->type != ButtonPress)
		return;
	if (evt->xbutton.x >= 40 && evt->xbutton.x < 80 &&
		evt->xbutton.y >= 55 && evt->xbutton.y < 75)
	{	theChar = '\r';
	}
	else if (evt->xbutton.x >= 120 && evt->xbutton.x < 160 &&
		evt->xbutton.y >= 55 && evt->xbutton.y < 75)
	{	theChar = '\r';
		result = FALSE;
	}
}

InfoWin::InfoWin(
	char	*s,
	int	x,
	int	y): popup("Info", x, y, 150, 60)
{
	prompt = s;

	do	X.block();
	while	(entered == FALSE);
}
void	InfoWin::drawWin(void)
{
	static	char	ok[] = "OK";

	XClearWindow(X.display, mywindow);

	XDrawImageString(X.display, mywindow, X.stdgc, 5, 17, prompt, strlen(prompt));

	XFillRectangle(X.display, mywindow, X.fgGreen, 50, 25, 60, 30);
	XDrawRectangle(X.display, mywindow, X.stdgc, 50, 25, 60, 30);
	XDrawImageString(X.display, mywindow, X.bgGreen, 75, 45, ok, strlen(ok));
}
void	InfoWin::mouseEvt(
	XEvent	*evt)
{
	if (evt->type != ButtonPress)
		return;
	if (evt->xbutton.x >= 50 && evt->xbutton.x < 110 &&
		evt->xbutton.y >= 25 && evt->xbutton.y < 55)
			entered = TRUE;
}
