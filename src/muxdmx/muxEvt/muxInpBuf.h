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
#ifndef	_MUX_INP_BUF_H
#define	_MUX_INP_BUF_H

#include "inxout.h"
#include "queue.h"

class	muxInpBuf: public inxout	{
typedef	inxout	baseclass;

public:
		muxInpBuf():	evtServ(this, keyEvtServ), evtStart(this, keyEvtStart)
		{	sendingAllowed = ContSend;
			serverState = serverIdling;
			needToSchedule = TRUE;
		}
	void	init();
	rec_typ	rec(data *, int);
	void	early(event *);
	void	late(event *);
	int	command(char *, tok_typ *);

	int	export(exp_typ *);

	event	evtServ;
	event	evtStart;

	int	ninp;			// # of inputs

	struct	inpStruct {
		queue	q;		// a queue per input
		int	bSize;		// buffer size, used for the byte-buffer version
		int	bStart;		// buffer occupations where prec objects
					// may continue to send
		rec_typ	precState;	// states of preceeding objects
	}	*inpStructs;

	rec_typ	sendingAllowed;		// are we allowed to send (if we wish so)?
	int	*candidates;		// inputs which have sth queued (used in late())
	data	*server;		// holds the data item in service
	int	needToSchedule;		// TRUE: late() isn't yet scheduled
	int	doStartStop;		// TRUE: use start-stop protocol on inputs
	tim_typ	relaxTime;		// time steps to relax between two services
	tim_typ	serviceTime;		// constant service time. if 0 then use serviceStepsPerByte
	double	serviceStepsPerByte;	// factor between data length in bytes and service time
	unsigned int	*lost;		// losses on inputs
	int		*qLen;		// queue occupation, coppied for easy export

	int	byteBuffer;		// TRUE: the buffer size counts in bytes
					// FALSE: buffer size counts in data objects

	int	*qLenBytes;
	unsigned int	*lostBytes;	// both used for the byte-buffer version

	int	pendingStartSignal;	// number of the input which to wake up next

	enum	{
		serverIdling,
		serverServing,
		serverRelaxing,
		serverStopped
	}	serverState;

	enum	{InpCtrl = -1};		// constant for control input

	enum	{keyEvtServ = 0, keyEvtStart = 1};
};

#endif	// _MUX_INP_BUF_H
