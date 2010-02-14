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
*************************************************************************/

#ifndef	_INPWIN_H_
#define	_INPWIN_H_

#include "x11obj.h"

class	InputWin: public popup {
public:
		InputWin(char *t, char *pr, char *repl, char *resbuff, int *resfl, int, int, int);
#ifdef USELUA
		~InputWin(void);
#endif
	void	drawWin(void);
	void	defaultEvt(XEvent *);

	char	*buffer;	// own aux buffer
	char	*rbuf;		// result buffer provided by client
	int	blen;
	int	bpos;

	char	*reply;		// current reply string of client

	int	*resFlagPtr;	// used to signal new string to the client

	XComposeStatus	cs;
};
#endif	// _INPWIN_H_
