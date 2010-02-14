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
*	Creation:		July 8, 1996
*
*************************************************************************/

/*
*	Multiplexer EPD, purely event-driven
*
*	If the queue occupation reaches THRESH, then *starting* AAL5 frames
*	are dropped. Additionally, corrupted frames are discarded.
*
*	Non-AAL5 cells are queued in an extra queue which always is served first.
*	Exception: due to DF, service of an AAL5 cell is started (CBR queue empty) even if
*	a CBR/VBR cells arrives with this time step. On the other hand, a CBR cell can
*	overtake a AAL5 cell waiting for the start of next output cycle.
*
*	Per default, the last cell of a corrupted or rejected frame is not passed. This
*	can be changed with PASSEOF.
*
*	Synchronous or asynchronous output operation.
*
*	If an AAL5 cell with VCI>MAXVCI is received, then an error message is launched.
*
*	MuxEvtEPD mux: NINP=10, BUFF=1000, {MAXVCI=100,} {SERVICE=10,} MODE={Sync | Async},
*			THRESH=600, BUFCBR=100, {PASSEOF,} OUT=sink;
*			// default MAXVCI: NINP
*			// SERVICE: service takes SERVICE time steps (default: 1)
*			// MODE: synchronous or asynchronous operation of output.
*			//		MODE can be omitted if SERVICE=1
*			// BUFCBR: buffer size for non-AAL5 cells
*			// PASSEOF given: pass last cell of a rejected or
*			//		corrupted frame (default: do not)
*
*	Commands:	see Multiplexer (muxBase.c)
*
*	exported:	see muxBase
*			mux->QLenCBR	// queue length non-AAL5
*/

#include "muxEvtEPD.h"

CONSTRUCTOR(MuxEvtEPD, muxEvtEPD);

/*
*	read create statement
*/
void	muxEvtEPD::addpars(void)
{
	int	i;

	baseclass::addpars();

	if (serviceTime != 1)
	{	skip_word("MODE");
		skip('=');
		if (test_word("Sync"))
		{	skip_word("Sync");
			syncMode = TRUE;
		}
		else if (test_word("Async"))
		{	skip_word("Async");
			syncMode = FALSE;
		}
		else	syntax0("MODE: `Sync' or `Async' expected");
		skip(',');
	}
	else	syncMode = FALSE;	// serviceTime == 1

	epdThresh = read_int("THRESH");
	if (epdThresh <= 0)
		syntax0("invalid THRESH");
	skip(',');

	qCBR.setmax(read_int("BUFCBR"));
	skip(',');

	if (test_word("PASSEOF"))
	{	skip_word("PASSEOF");
		skip(',');
		passEOF = TRUE;
	}
	else	passEOF = FALSE;

	CHECK(vciFirst = new int[max_vci]);
	CHECK(vciOK = new int[max_vci]);
	for (i = 0; i < max_vci; ++i)
		vciFirst[i] = vciOK[i] = TRUE;

	CHECK(inp_buff_CBR = new inpstruct[ninp]);
	inp_ptr_CBR = inp_buff_CBR;
}

/*
*	cell received.
*	AAL5 cells and other cells are already seperated into different buffers
*/
rec_typ	muxEvtEPD::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	iKey)
{
	if (typequery(pd, AAL5CellType))
	{	// AAL5 cell
		inp_ptr->inp = iKey;
		(inp_ptr++)->pdata = pd;
	}
	else	// non-AAL5 cell
	{	inp_ptr_CBR->inp = iKey;
		(inp_ptr_CBR++)->pdata = pd;
	}

	// wake up late() if necessary
	if (needToSchedule)
	{	needToSchedule = FALSE;
		alarml( &evtLate, 0);
	}

	return ContSend;
}

/*
*	One or more arrivals occured, we have been triggered by rec().
*/
void	muxEvtEPD::late(event *)
{
	int		n, vc;
	aal5Cell	*pc;
	inpstruct	*p;

	needToSchedule = TRUE;

	//	process all arrivals in random order, first non-AAL5
	if ((n = inp_ptr_CBR - inp_buff_CBR) != 0)
	{	for (;;)
		{	if (n > 1)
				p = inp_buff_CBR + (my_rand() % n);
			else	p = inp_buff_CBR;

			if ( !processItem(p->pdata, &qCBR))
				dropItem(p);

			if ( --n == 0)
				break;
			*p = inp_buff_CBR[n];
		}
		inp_ptr_CBR = inp_buff_CBR;
	}
	//	now AAL5
	if ((n = inp_ptr - inp_buff) != 0)
	{	for (;;)
		{	if (n > 1)
				p = inp_buff + (my_rand() % n);
			else	p = inp_buff;

			vc = (pc = (aal5Cell *)p->pdata)->vci;	// data type has been tested in rec()
			if (vc < 0 || vc >= max_vci)
				errm1s2d("%s: AAL5 cell with illegal VCI=%d received (input %d)",
						name, vc, p->inp + 1);
			// at the beginning of a burst: check whether to accept burst
			if (vciFirst[vc])
			{	vciFirst[vc] = FALSE;
				if (q.getlen() >= epdThresh)
					vciOK[vc] = FALSE;
				else	vciOK[vc] = TRUE;
			}
	
			if (pc->pt == 1)		// EOF cell (can be BOF, too!)
			{	vciFirst[vc] = TRUE;	// next cell will be BOF
				if (passEOF || vciOK[vc])
				{	// pass last cell if frame was ok or passEOF is set
					if ( !processItem(pc, &q))
						dropItem(p);
				}
				else	dropItem(p);
			}
			else
			{	// non-EOF cell
				if (vciOK[vc])
				{	if ( !processItem(pc, &q)) // buffer overflow
					{	dropItem(p);
						vciOK[vc] = FALSE;	// drop the rest of the burst
					}
				}
				else	dropItem(p);
			}

			if ( --n == 0)
				break;
			*p = inp_buff[n];
		}
		inp_ptr = inp_buff;
	}
}

/*
*       Put an item into server or the given queue.
*       Returns TRUE on success.
*/
int     muxEvtEPD::processItem(
        data    *pd,
        queue   *pqu)   // where to queue item
{
        if (serverState != serverIdling)
        {       // server is still working on its own
                return pqu->enqueue(pd);
        }
        else
        {       // server is idle
                if (syncMode)
        	{	int     dd = SimTime % serviceTime;
                        if (dd)
			{	// we have to wait until beginning of an output cycle,
                        	// but until there we have to leave the item in the queue!
                        	if (pqu->enqueue(pd))
                        	{       alarme( &std_evt, serviceTime - dd);
                                	serverState = serverSyncing;
                                	return TRUE;
                        	}
                        	return FALSE; // buffer overflow, server remains idling
			}
			// else: same as !syncMode, we can use the server immediately
                }
                server = pd;
                serverState = serverServing;
                alarme( &std_evt, serviceTime);
                return TRUE;
        }
}
 

/*
*	in serving state:
*		Forward a data item to the successor.
*		Register again, if more data queued.
*	in sync state:
*		take data from queue and enter serving state
*
*	If we take an item from a queue, then we first look into qCBR.
*/
void	muxEvtEPD::early(
	event	*)
{
	if (serverState == serverServing)
	{	// "normal" case: forward served data
		suc->rec(server, shand);
		server = qCBR.dequeue();
		if (server == NULL)
			server = q.dequeue();
		if (server)
			alarme(&std_evt, serviceTime);
		else	serverState = serverIdling;
	}
	else
	{	// We just had to wait for start of next cycle
		server = qCBR.dequeue();
		if (server == NULL)
			// There should be sth in one queue, see processItem()
			server = q.dequeue();
		alarme(&std_evt, serviceTime);
		serverState = serverServing;
	}
}


int	muxEvtEPD::export(
	exp_typ	*msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "QLenCBR", &qCBR.q_len);
}
