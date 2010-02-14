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
*	Module author:		Gunnar Loewe, TUD (diploma thesis)
*	Creation:		Sept 1996
*
*	History:
*	17/10/96		some cosmetics, class uqueue used. MBau
*
*	July 10, 1997		- data item is embedded in last cell of frame,
*				  rather than in first one. only changes in
*				  early(), where data item is forwarded to successor
*						Matthias Baumann
*
*	Oct 22, 1997		Concurrent reassembly of mutliple frames introduced.
*				All variables become arrays, with VCI acting as index.
*				Additionally, the usage of queues has been removed.
*						Matthias Baumann
*
*     	 13.1.98     	      	 COPYCID added Mue
*     	 15.2.2000   	      	 COPYCLP added Mue
*
*************************************************************************/

/*
*	AAL 5 receiver module with concurrent reassembly of mutliple frames.
*
*	AAL5RecMult aal: COPYCID, COPYCLP=<int>, MAXVCI=<int>, OUT=tcp;
*     	       COPYCID: copy the vci of cells into the frame cid
*     	       COPYCLP: copy the clp bit of last cell into clp bit of frame
*
*	Command:
*		aal->ResetStat	// reset all statistics variables
*
*	Variables exported for use in commands or with measurement devices:
*		aal->QLen(vc)		// current cell queue length
*		aal->CellLoss(vc)	// # of cells lost
*		aal->CellCount(vc)	// # of cells received
*		aal->SDULoss(vc)	// # of SDUs lost
*		aal->SDUCount(vc)	// # of SDUs succesfully transmitted
*		aal->DelayMean(vc)	// mean SDU delay (from sending first cell until receiving the last)
*
*	Mechanism to detect cell and frame losses
*	=========================================
*	See AAL5Send in aal5send.c.
*/


#include "aal5recMult.h"

CONSTRUCTOR(Aal5recMult, aal5recMult)

/********************************************************************/
/*
*	read definition statement
*/
void	aal5recMult::init(void)
{
	int	i;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	
	// TEST Mue 13.1.98
	doCopyCid = FALSE;
	if (test_word("COPYCID"))
	{	skip_word("COPYCID");
		doCopyCid = TRUE;
		skip(',');
	}
	// end TEST

	// Mue 15.2.2000
	doCopyClp = FALSE;
	if (test_word("COPYCLP"))
	{
      	    doCopyClp = read_int("COPYCLP");
	    skip(',');
	}
	
	maxvci = read_int("MAXVCI") + 1;
	if (maxvci < 1)
		syntax0("inavlid MAXVCI");
	skip(',');

	output("OUT");
	stdinp();

	CHECK(last_cell = new int [maxvci]);
	for (i = 0; i < maxvci; ++i)
		last_cell[i] = 0;	// first expected cell: sequence # 1

	CHECK(first_cell = new aal5Cell * [maxvci]);
	for (i = 0; i < maxvci; ++i)
		first_cell[i] = NULL;	// next cell is treated as beginning of frame

	CHECK(last_sdu = new int [maxvci]);
	for (i = 0; i < maxvci; ++i)
		last_sdu[i] = 0;	// remember last valid sdu sequence number
	CHECK(sdu_loss = new int [maxvci]);
	for (i = 0; i < maxvci; ++i)
		sdu_loss[i] = 0;	// sdu loss
	CHECK(cell_loss = new int [maxvci]);
	for (i = 0; i < maxvci; ++i)
		cell_loss[i] = 0;	// cell loss
	CHECK(sdu_cnt = new unsigned int [maxvci]);
	for (i = 0; i < maxvci; ++i)
		sdu_cnt[i] = 0;		// sdu counter
	CHECK(cell_cnt = new unsigned int [maxvci]);
	for (i = 0; i < maxvci; ++i)
		cell_cnt[i] = 0;	// cell counter
	CHECK(q_len = new int [maxvci]);
	for (i = 0; i < maxvci; ++i)
		q_len[i] = 0;

	CHECK(delay_mean = new double [maxvci]);
	for (i = 0; i < maxvci; ++i)
		delay_mean[i] = 0.0;
}

/********************************************************************/
/*
*	cell received
*/

rec_typ	aal5recMult::REC(data *pd, int)	// REC is a macro normally expanding to rec (for debugging)
{
	aal5Cell	*pc = (aal5Cell *) pd;
	aal5Cell	*pfirst;
	int		vc;

	typecheck(pd, AAL5CellType);

	vc = pc->vci;
	if (vc < 0 || vc >= maxvci)
		errm1s2d("%s: out-of-range VCI=%d received, MAXVCI=%d", name, vc, maxvci - 1);

	if ( ++counter == 0)
		errm1s("%s: overflow of cell counter", name);
	if ( ++cell_cnt[vc] == 0)
		errm1s1d("%s: overflow of cell counter for VCI %d", name, vc);

	if( ++last_cell[vc] != pc->cell_seq)
	{	// wrong cell (cell loss)
		cell_loss[vc] += pc->cell_seq - last_cell[vc];	// log the detected lost cells
		last_cell[vc] = pc->cell_seq;

		// this cell now is seen as beginning of a frame
		if (first_cell[vc] != NULL)	// maybe, a whole frame has been lost (EPD)
			delete first_cell[vc];
		pfirst = first_cell[vc] = pc;

		q_len[vc] = 1;
	}
	else
	{	// expected cell arrived.
		if ((pfirst = first_cell[vc]) == NULL)
			pfirst = first_cell[vc] = pc;
		++q_len[vc];
	}

	if (pc->pt == 1)		// end of SAR-SDU. May be the first cell, too.
	{	if(pc->first_cell == pfirst->cell_seq)
		{	// SDU is valid, no pt=1 cell was lost
			// forward data item embedded in last cell
			if (pc->embedded)
			{
				// TEST Mue, 13.1.98
				if(doCopyCid)
				{
				   typecheck(pc->embedded, FrameType);
				   ((frame *)pc->embedded)->connID = pc->vci;
				}
				// end TEST

				// Mue, 15.2.2000
				if(doCopyClp)
				{
				   typecheck(pc->embedded, FrameType);
				   ((frame *)pc->embedded)->clp = pc->clp;
				}

			
				suc->rec(pc->embedded, shand);
				pc->embedded = NULL;	// this is important, otherwise pc->embedded
							// is deleted by its receiver *and*
							// when deleting pc !!
			}
			else	errm1s("%s: no data item embedded in last cell of frame", name);
			
			// log the loss of AAL SDUs if any
			sdu_loss[vc] += pfirst->sdu_seq - last_sdu[vc] - 1;
			last_sdu[vc] = pfirst->sdu_seq;	// store this sdu sequence number for statistic

			// log the delay of AAL SDU
			++sdu_cnt[vc];
			tim_typ	delay = SimTime - pfirst->time;
						// the delay spans from generating
						// the first cell by the sender
						// until delivering the frame to user
			delay_mean[vc] = (delay_mean[vc] * (sdu_cnt[vc] - 1) + delay) / sdu_cnt[vc];
		}

		if (pc != pfirst)	// be aware of one-cell frames
			delete pc;

		delete pfirst;
		first_cell[vc] = NULL;	// next cell is beginning of frame

		q_len[vc] = 0;
	}
	else
	{	if (pc != pfirst)	// do not delete, if stored as first cell of frame
			delete pc;
	}
	

	return ContSend;
}


/********************************************************************/

int	aal5recMult::command(char *s, tok_typ *v)
{
	if (baseclass::command(s, v) == TRUE)
		return TRUE;

	v->tok = NILVAR;
	if(strcmp(s, "ResetStat") == 0)		// resets the statistic
	{	for (int i = 0; i < maxvci; ++i)
		{	cell_loss[i] = 0;
			cell_cnt[i] = 0;
			sdu_loss[i] = 0;
			sdu_cnt[i] = 0;
			delay_mean[i] = 0.0;
			counter = 0;
		}
	}
	else	return FALSE;

	return TRUE;
}


/********************************************************************/
/*
*	export addresses of variables
*/
int	aal5recMult::export(exp_typ *msg)
{
	return	baseclass::export(msg) ||
		intArray1(msg, "QLen", q_len, maxvci, 0) ||
		intArray1(msg, "CellLoss", cell_loss, maxvci, 0) ||
		intArray1(msg, "CellCount", (int *) cell_cnt, maxvci, 0) ||	// additional: aal->Counter
		intArray1(msg, "SDULoss", sdu_loss, maxvci, 0) ||
		intArray1(msg, "SDUCount", (int *) sdu_cnt, maxvci, 0) ||
		doubleArray1(msg, "DelayMean", delay_mean, maxvci, 0);
}

