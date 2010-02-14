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
*	Creation:		July 6, 1997
*
*	History
*	Sept 30, 1997		CONNID added
*					Matthias Baumann
*
*************************************************************************/

/*
*	Data2Frame xyz: FLEN=20, {CONNID=10,} OUT=xx;
*			// each incoming data item is converted into a frame of lenght FLEN bytes
*			// frames carry the CONNID, default: 0
*/

#include "in1out.h"
#include "dat2fram.h"

dat2fram::dat2fram()
{
}

dat2fram::~dat2fram()
{
}

// REC is a macro normally expanding to rec (for debugging)
rec_typ	dat2fram::REC(data *pd, int)
{
   frame *frm = new frame(flen, connID);
   frm->prioCodePoint = pcp;
   frm->vlanPriority = vlanprio;
   frm->vlanId = vlanid;
   frm->dropPrecedence = dscp;
   frm->tpid = tpid;
   frm->smac = smac;
   frm->dmac = dmac;
   frm->vlanId2 = vlanid2;
   frm->vlanPriority2 = vlanprio2;
   frm->tpid2 = tpid2;
   delete pd;
   return suc->rec(frm, shand);
   //   return suc->rec(new frame(flen, connID), shand);
}
