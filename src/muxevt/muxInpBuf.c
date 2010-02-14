/************************************************************************
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
*	Creation:		July 9, 1997
*
*	History:
*	September 26, 1997	Bug fixed in late(). It is illegal to
*				send the start message during late(). The
*				fix uses an additional event, and postpones
*				starting the preceeding object to the next
*				time slot. Addititions in early().
*					Matthias Baumann
*
*	October 20, 1997	Queues may count in bytes. This is more
*				realistic when multiplexing frames with
*				different sizes. Turned on with BYTEBUF=...
*				Additional variables in this mode:
*					mx->LossBytes(i)
*					mx->QLenBytes(i)
*						Matthias Baumann
*					
*
*************************************************************************/

#include "muxInpBuf.h"

CONSTRUCTOR(MuxInpBuf, muxInpBuf);

/*
*	A multiplexer with input buffers. Incoming data items are first stored in the input
*	queues. If the server was idling, then at the end of the time step (after all
*	arrivals occured) the server dequeues from one of the input queues (random choice according to
*	uniform distribution). The service takes (PduLenOfItem * BYTEFACT) or SERVICE time steps,
*	afterwards the item is forwarded to the successor of the mux. At the end of a service,
*	the server relaxes for RELAX time step, if RELAX is given. Then it looks into the input
*	queues (again, after all arrivals during the step occured), and begins a new service or enters
*	the idle state.
*	If BSTART is given, then the start-stop protocol is used, and OUTCTRL has to be specified, too.
*	OUTCTRL is given like OUT for Demultiplexer.
*	On the output, start-stop is supported. The control input is mx->Start.
*
*	MuxInpBuf mx:	NINP=<int>,		// # of inputs
*			BUF=<int> | BYTEBUF=<int>,	// buffer size per input:
*						// BUF: buffer "counts" in data objects
*						// BYTEBUF: buffer "counts" in bytes
*			{BSTART=<int>,}		// start buffer size: turns on start-stop-protocol
*			BYTEFACT=<double> | SERVICE=<int>,
*						// either BYTEFACT or SERVICE.
*						// BYTEFACT: number of time steps needed to serve a byte
*						// SERVICE: constant service time (in time steps)
*			{RELAX=<int>,}		// time steps between two services, default: 0
*			{OUTCTRL=<objects>,}	// if BSTART has been given: control inputs of sources
*			OUT=<object>;		// where to send data
*
*	Control input:	mx->Start
*	Exported:	mx->Loss(i)		// losses on input i (data objects)
*			mx->QLen(i)		// current queue length at input i (data objects)
*			only in mode BYTEBUF:
*			mx->LossBytes(i)	// losses on input i (bytes)
*			mx->QLenBytes(i)	// current queue length at input i (bytes)
*	Command:	mx->ResLosses		// reset loss counters
*/

/*
*	read definition statement
*/
void	muxInpBuf::init()
{
	int	bsiz, bstart, i;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	ninp = read_int("NINP");
	if (ninp < 1)
		syntax0("invalid NINP");
	skip(',');

	if (test_word("BUF"))
	{	bsiz = read_int("BUF");
		if (bsiz < 1)
			syntax0("invalid BUF");
		byteBuffer = FALSE;
	}
	else
	{	bsiz = read_int("BYTEBUF");
		if (bsiz < 1)
			syntax0("invalid BYTEBUF");
		byteBuffer = TRUE;
	}
	skip(',');

	if (test_word("BSTART"))
	{	bstart = read_int("BSTART");
		if (bstart >= bsiz || bstart < 0)
			syntax0("invalid BSTART");
		skip(',');
		doStartStop = TRUE;
	}
	else
	{	doStartStop = FALSE;
		bstart = 0;
	}

	CHECK(inpStructs = new inpStruct[ninp]);
	for (i = 0; i < ninp; ++i)
	{	if (byteBuffer)
			inpStructs[i].q.unlimit();
		else	inpStructs[i].q.setmax(bsiz);
		
		inpStructs[i].bSize = bsiz;
		inpStructs[i].bStart = bstart;
		inpStructs[i].precState = ContSend;
	}
	CHECK(candidates = new int[ninp]);

	if (test_word("BYTEFACT"))
	{	serviceStepsPerByte = read_double("BYTEFACT");
		if (serviceStepsPerByte <= 0.0)
			syntax0("invalid BYTEFACT");
		skip(',');
		serviceTime = 0;
	}
	else
	{	serviceTime = read_int("SERVICE");
		if (serviceTime < 1)
			syntax0("invalid SERVICE value");
		skip(',');
	}

	if (test_word("RELAX"))
	{	int	t = read_int("RELAX");
		if (t < 0)
			syntax0("invalid RELAX");
		relaxTime = t;
		skip(',');
	}
	else	relaxTime = 0;		// default: no ralaxation

	addpars();		// parameters of derived classes

	if (doStartStop)	// if BSTART has been given:
	{	outputs("OUTCTRL", ninp, 0);	// control output 1 gets index 1,
						// index 0 is for data output
		skip(',');
	}

	output("OUT", 0);

	input("Start", InpCtrl);
	inputs("I", ninp, -1);

	CHECK(lost = new unsigned int[ninp]);
	CHECK(qLen = new int[ninp]);
	for (i = 0; i < ninp; ++i)
	{	lost[i] = 0;
		qLen[i] = 0;
	}

	if (byteBuffer)
	{	CHECK(qLenBytes = new int[ninp]);
		CHECK(lostBytes = new unsigned int[ninp]);
		for (i = 0; i < ninp; ++i)
		{	qLenBytes[i] = 0;
			lostBytes[i] = 0;
		}
	}
	else	
	{	qLenBytes = NULL;
		lostBytes = NULL;
	}

	pendingStartSignal = -1;	// this says that no start signal is pending
}

/*
*	data or start signal received
*/
rec_typ	muxInpBuf::REC(
	data	*pd,
	int	iKey)
{
	if (iKey == InpCtrl)
	{	// Start signal
		delete pd;
		if (sendingAllowed == StopSend)
		{	sendingAllowed = ContSend;
			// we should be either relaxing or stopped
			switch (serverState) {
			case serverStopped:
				// since we can send at earliest next step (1 step service),
				// we can activate late() already for this slot
				if (needToSchedule)
				{	needToSchedule = FALSE;
					alarml( &std_evt, 0);
//printf("%s: %d: start was scheduled! \n",name, SimTime);
				}
				// until there we remain in serverStopped and go
				// then to serving or idling
				break;
			case serverRelaxing:
				// Stop period ceased before relaxation over.
				// We have enabled sendingAllowed: with the end of relaxation,
				// we can proceed as usual.
				break;
			default:errm1s("%s: internal error: bad state in muxInpBuf::rec()", name);
			}
		}
		return ContSend;
	}

	inpStruct	*pi = &inpStructs[iKey];

	// did we stop the sender? (if no start stop used, then precState remains ContSend)
	if (pi->precState != ContSend)
	{	delete	pd;
		errm1s1d("%s: preceeding object (input %d) did not recognize Stop signal", name, iKey + 1);
		return StopSend;
	}

	// try to enqueue arrival
	int	res;
	if (byteBuffer)
	{	int	l = pd->pdu_len();
		if (qLenBytes[iKey] + l <= pi->bSize)
		{	pi->q.enqueue(pd);
			++qLen[iKey];
			qLenBytes[iKey] += l;
			res = TRUE;
		}
		else	res = FALSE;
	}
	else
	{	if ((res = pi->q.enqueue(pd)) != FALSE)
			++qLen[iKey];	// for exporting the queue lengths
	}
	
	if (res == FALSE)
	{	// no space left
		if ( ++lost[iKey] == 0)
			errm1s1d("%s: overflow of lost[%d]", name, iKey + 1);
		if (byteBuffer)
		{	unsigned int	old;
			old = lostBytes[iKey];
			if ((lostBytes[iKey] += pd->pdu_len()) < old)
				errm1s1d("%s: overflow of lostBytes[%d]", name, iKey + 1);
		}
		delete pd;

		if (doStartStop)
		{	errm1s("%s: internal error in muxInpBuf::rec(): should have stopped in advance",
					name);
			return StopSend;
		}
		else	return ContSend;
	}

	if (serverState == serverIdling)
	{	// in case the server is idling, we have to decide
		// during late() who to serve next (perhaps we get
		// more arrivals during this time step)
		if (needToSchedule)
		{	needToSchedule = FALSE;
			alarml( &std_evt, 0);
		}
	}

	if (doStartStop && (byteBuffer ? (qLenBytes[iKey] >= pi->bSize) : (pi->q.isFull())))
				// we have to stop the sender
	{	pi->precState = StopSend;
//printf("%s:%d: stop %d\n",name, SimTime, iKey + 1);
		return StopSend;
	}
	else	return ContSend;
}

/*
*	late(): try to dequeue a data item and begin service on success
*/
//	we get here if 
//	1)	we had an arrival while server idling, or
//	2)	relaxation completed, and we want to serve new data. We do *not*
//		take new data in early(), since we then might be unfair against sources still sending
//		during the same step. (we are still in state serverRelaxing)
//	3)	we are allowed to send again, and we try it. (we are still in state serverStopped)
void	muxInpBuf::late(
	event	*)
{
	int		i, n;


//printf("%s: %d: in late arrived \n",name, SimTime);
	needToSchedule = TRUE;

	// decide on who to serve next (if any),
	// that's difficult. For a first version, we do it by chance.
	n = 0;
	for (i = 0; i < ninp; ++i)
		if ( !inpStructs[i].q.isEmpty())
			candidates[n++] = i;
	if (n == 0)
	{	// nobody has sth to send
		serverState = serverIdling;
//printf("%d: idle\n", SimTime);
		return;
	}

	if (n == 1)
		i = candidates[0];
	else	i = candidates[my_rand() % n];

	server = inpStructs[i].q.dequeue();
	--qLen[i];
	if (byteBuffer)
		qLenBytes[i] -= server->pdu_len();

//printf("%d: serve %d\n", SimTime, i + 1);

#ifdef	NEVERDEF
	This is a bug. We are not allowed to send the start signal during the late slot phase.
	if (doStartStop)
	{	inpStruct	*pi = &inpStructs[i];
		if (pi->precState == StopSend && pi->q.getlen() == pi->bStart)
		{	// we can wake up this guy
			pi->precState = ContSend;
			sucs[i + 1]->rec(new data, shands[i + 1]);	// sucs[0] is data output
		}
	}
#endif	// NEVERDEF

	// and this is the bug fix. We use a new event to send the start signal during the early
	// phase of the next slot. We remember the input number in pendingStartSignal.
	//	Matthias Baumann, Sept. 26, 1997
	if (doStartStop)
	{	inpStruct       *pi = &inpStructs[i];
                if (pi->precState == StopSend && 
			(byteBuffer ? (qLenBytes[i] <= pi->bStart) : (pi->q.getlen() == pi->bStart)))
                {       // we can wake up this guy, but not immediately. Start a timer.
			if (pendingStartSignal != -1)
				errm1s("%s: internal error in muxInpBuf::late(): already start signal pending",
						name);
			pendingStartSignal = i;
			alarme( &evtStart, 1);
		}
	}

	tim_typ	tim;

	if (serviceTime != 0)
		tim = serviceTime;	// SERVICE has been specified -> constant service time
	else	// service time depends on PDU length
	{	tim = (tim_typ) (server->pdu_len() * serviceStepsPerByte + 0.5);
					// + 0.5: round instad of truncate
		if (tim < 1)
			tim = 1;
	}

	alarme( &evtServ, tim);
	serverState = serverServing;
}

/*
*	early(): either a service is completed, or relaxation time expired
*	Or: we have to wake up a predecessor (event evtStart, Bug fix from Sept 26, 1997)
*/
void	muxInpBuf::early(
	event	*evt)
{
	if (evt->key == keyEvtStart)
	{	// during late(), we recognized that a predecessor can be started
		if (pendingStartSignal == -1)
			errm1s("%s: internal error in muxInpBuf::early(): no start signal pending",
					name);
		inpStructs[pendingStartSignal].precState = ContSend;
		sucs[pendingStartSignal + 1]->rec(new data, shands[pendingStartSignal + 1]);
						// sucs[0] is data output
		pendingStartSignal = -1;	// nothing pending anymore
		return;
	}

	if (serverState == serverServing)
	{	// service completed, forward data.
		sendingAllowed = sucs[0]->rec(server, shands[0]);
		// enter relax state. We recognise stop signal at the end of relaxation
		if (relaxTime > 0)
		{	serverState = serverRelaxing;
			alarme( &evtServ, relaxTime);
			return;
		}
		// else: continue as if relaxation finished
	}

	// we reach this point if relaxation period finished or no relaxation given
	if (sendingAllowed != StopSend)
	{	// Relaxation completed, look for next data in late().
		// We don't look for data here for reasons of fairness- perhaps
		// there are still data arriving during this step.
		if (needToSchedule)
		{	needToSchedule = FALSE;
			alarml( &std_evt, 0);
		}
	}
	else	serverState = serverStopped;
}


int	muxInpBuf::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intArray1(msg, "Loss", (int *) lost, ninp, 1) ||
		intArray1(msg, "QLen", qLen, ninp, 1) ||
		(qLenBytes != NULL && intArray1(msg, "QLenBytes", qLenBytes, ninp, 1)) ||
		(lostBytes != NULL && intArray1(msg, "LossBytes", (int *) lostBytes, ninp, 1));
}

int	muxInpBuf::command(
	char	*s,
	tok_typ	*p)
{
	if (baseclass::command(s, p))
		return TRUE;

	p->tok = NILVAR;
	if (strcmp(s, "ResLosses") == 0)
	{	int	i;
		for (i = 0; i < ninp; ++i)
			lost[i] = 0;
		if (lostBytes != NULL)
			for (i = 0; i < ninp; ++i)
				lostBytes[i] = 0;
	}
	else	return FALSE;

	return TRUE;
}
