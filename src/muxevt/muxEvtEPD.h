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
#ifndef	_MUX_EVT_EPD_H_
#define	_MUX_EVT_EPD_H_

#include "muxBase.h"

class	muxEvtEPD: public muxBase {
typedef	muxBase	baseclass;
public:
		muxEvtEPD()
		{	serverState = serverIdling;     // serve idle
		}
	void	addpars(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
	void	early(event *);
	void	late(event *);
	int	processItem(data *, queue *);
	int	export(exp_typ *);

	queue	qCBR;		// queue for non-AAL5 cells, high priority

	int	passEOF;	// TRUE: pass last cell of a rejected or currupted frame

	int	epdThresh;	// when to start dropping frames
	int	*vciFirst;	// TRUE: first cell of burst
	int	*vciOK;		// TRUE: cells can be queued

	enum	{serverIdling, serverSyncing, serverServing} serverState;

	inpstruct	*inp_buff_CBR, *inp_ptr_CBR;	// input buffer for non-AAL5 cells

	int	syncMode;	// TRUE: MODE=Sync
};

#endif	// _MUX_EVT_EPD_H_
