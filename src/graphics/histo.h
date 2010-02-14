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
#ifndef	_HISTO_H
#define	_HISTO_H

#include "x11obj.h"

class	histo:	public	X11obj	{
typedef	X11obj	baseclass;
public:
	void	init(void);
	void	late(event *);
	void	connect(void) {}

	void	drawWin(void);
	void	mouseEvt(XEvent *);
	void	store2file(char *);
#ifdef USELUA
	int act(void);
#endif
	tim_typ	delta;

	double	maxfreq;
	int	autoscale;

	XSegment	*xsegs;

	int	*int_val_ptr;

	int	nvals;
	int	indicdispl;
};

#define	AUTOSCALE	(1.25)

#ifdef USELUA
class	histo2: public histo {
typedef	histo	baseclass;
public:
	void	init(void);
	void	late(event *);
	int	export(exp_typ *);
	int	command(char *, tok_typ *);
	int     act(void);
	int	update;
	int	update_cnt;
	int	*the_val_ptr;

	int	over_under;
};
#endif

#endif	// _HISTO_H
