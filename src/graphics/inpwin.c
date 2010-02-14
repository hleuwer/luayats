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
*	Creation:		March 97
*
*************************************************************************/

#include "inpwin.h"

InputWin::InputWin(
	char	*t,		// title
	char	*pr,		// prompt
	char	*repl,		// place with current reply line
	char	*resbuff,	// where to write result string
	int	*resFlag,	// set to TRUE when string in resbuff is renewed
	int	bl,		// len of buffer
	int	x,
	int	y): popup(t, x, y, 200, 80)
{
	if (bl < 2)
		errm0("internal error: InputWin::InputWin(): blen too small");

	CHECK(buffer = new char[bl]);
	rbuf = resbuff;
	blen = bl;
	prompt = pr;
	reply = repl;

	bpos = 0;
	buffer[bpos] = '_';

	resFlagPtr = resFlag;
}

#include <X11/keysym.h>
void	InputWin::defaultEvt(
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
			if (bpos > 0)
				--bpos;
			break;
		case XK_Return:
			strncpy(rbuf, buffer, bpos);
			rbuf[bpos] = '\0';
			*resFlagPtr = TRUE;
			bpos = 0;
			break;
		default:if (isgraph(b[0]))
			{	buffer[bpos] = b[0];
				if (bpos < blen - 2)	// leave place for the '_'
					++bpos;
			}
		}
		buffer[bpos] = '_';
		drawWin();
	}
}

void	InputWin::drawWin(void)
{
	XClearWindow(X.display, mywindow);

	XDrawImageString(X.display, mywindow, X.stdgc, 10, 15, prompt, strlen(prompt));
	XDrawImageString(X.display, mywindow, X.stdgc, 10, 40, buffer, bpos + 1);
	XDrawImageString(X.display, mywindow, X.stdgc, 10, 65, reply, strlen(reply));
}

#ifdef USELUA
InputWin::~InputWin(void)
{
  delete buffer;
}
#endif
