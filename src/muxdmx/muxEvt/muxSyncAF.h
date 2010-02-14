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
#ifndef	_MUX_SYNC_AF_H_
#define	_MUX_SYNC_AF_H_

#include "muxBase.h"

class	muxSyncAF:	public	muxBase {
typedef	muxBase	baseclass;

public:	
		muxSyncAF()
		{	serverState = serverIdling;	// serve idle
			serviceCompleted = (tim_typ) -1;
		}
	void	early(event *);
	void	late(event *);
	void	restim();

	enum	{serverIdling, serverSyncing, serverServing} serverState;
	tim_typ	serviceCompleted;
};

#endif	// _MUX_SYNC_AF_H_
