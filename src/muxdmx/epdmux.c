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
*	Creation:		Nov 1996
*
*	History:
*	July 17, 1997		- slight rearrangements in the loop
*				  of late(). An out-of-range VCI of an
*				  AAL5 cell now causes an error message
*				  (old version: silently treated it as non-AAL5)
*					Matthias Baumann
*
*************************************************************************/

/*
*	Multiplexer EPD
*	If the queue occupation reaches THRESH, then *starting* AAL5 frames
*	are dropped. Additionally, corrupted frames are discarded.
*
*	MuxEPD mux: NINP=10, BUFF=1000, {MAXVCI=100,} THRESH=600, OUT=sink;
*			// default MAXVCI: NINP
*
*	Commands:	see Multiplexer (mux.c)
*/


#include "mux.h"

class	muxEPD: public mux {
typedef	mux	baseclass;
public:
	void	addpars(void);
	void	late(event *);

	int	epdThresh;
	int	*vciFirst;	// TRUE: first cell of burst
	int	*vciOK;		// TRUE: cells can be queued
};

CONSTRUCTOR(MuxEPD, muxEPD);

/*
*	read create statement
*/
void	muxEPD::addpars(void)
{
	int	i;

	baseclass::addpars();

	epdThresh = read_int("THRESH");
	if (epdThresh <= 0)
		syntax0("invalid THRESH");
	skip(',');

	CHECK(vciFirst = new int[max_vci]);
	CHECK(vciOK = new int[max_vci]);
	for (i = 0; i < max_vci; ++i)
		vciFirst[i] = vciOK[i] = TRUE;
}

/*
*	Every time slot:
*	In the late slot phase, all cells have arrived and we can apply a fair
*	service strategy: random choice
*/

void	muxEPD::late(event *)
{
	int		n;
	inpstruct	*p;

	//	process all arrivals in random order
	n = inp_ptr - inp_buff;
	while (n != 0)
	{	if (n > 1)
			p = inp_buff + (my_rand() % n);
		else	p = inp_buff;

		if (typequery(p->pdata, AAL5CellType))
		{	int		vc;
			aal5Cell	*pc;
			vc = (pc = (aal5Cell *) p->pdata)->vci;
			if (vc < 0 || vc >= max_vci)
				errm1s2d("%s: AAL5 cell with illegal VCI=%d received on"
					" input %d", name, vc, p->inp + 1);

			// at the beginning of a burst: check whether to accept burst
			if (vciFirst[vc])
			{	vciFirst[vc] = FALSE;
				if (q.getlen() >= epdThresh)
					vciOK[vc] = FALSE;
				else	vciOK[vc] = TRUE;
			}

			if (pc->pt == 1)	// next cell will be first cell
				vciFirst[vc] = TRUE;

			if (vciOK[vc])
			{	if ( !q.enqueue(pc)) // buffer overflow
				{	dropItem(p);
					vciOK[vc] = FALSE;	// drop the rest of the burst
				}
			}
			else	dropItem(p);

		}
		else
		{	if (q.enqueue(p->pdata) == FALSE)
				dropItem(p);
		}

		*p = inp_buff[--n];
	}
	inp_ptr = inp_buff;

	if (q.getlen() != 0)
		alarme( &std_evt, 1);
}


