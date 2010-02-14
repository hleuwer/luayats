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
*	July 10, 1997		- data item is embedded in last cell of frame,
*				  rather than in first one. only changes in
*				  early(), where data item is forwarded to successor
*						Matthias Baumann
*
*************************************************************************/

/*
*	History:
*	17/10/96:		some cosmetics, class uqueue used. MBau
*/

/*
*	AAL 5 receiver module.
*
*	AAL5Rec aal: OUT=tcp;
*
*	Command:
*		aal->ResetStat	// reset all statistics variables
*
*	Variables exported for use in commands or with measurement devices:
*		aal->QLen	// current cell queue length
*		aal->CellLoss	// # of cells lost
*		aal->CellCount	// # of cells received
*		aal->SDULoss	// # of SDUs lost
*		aal->SDUCount	// # of SDUs succesfully transmitted
*		aal->DelayMean	// mean SDU delay (from sending first cell until receiving the last)
*
*	Mechanism to detect cell and frame losses
*	=========================================
*	See AAL5Send in aal5send.c.
*/


#include "aal5rec.h"

CONSTRUCTOR(Aal5rec, aal5rec)

/********************************************************************/
/*
*	read definition statement
*/
void	aal5rec::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	
	output("OUT");
	stdinp();

	last_cell = 0;
	last_sdu = 0;	// remember last valid sdu sequence number
	sdu_loss = 0;	// sdu loss
	cell_loss = 0;	// cell loss
	sdu_cnt = 0;	

	delay_mean = 0.0;
}

/********************************************************************/
/*
*	cell received
*/

rec_typ	aal5rec::REC(data *pd, int)	// REC is a macro normally expanding to rec (for debugging)
{
	aal5Cell	*pc = (aal5Cell *) pd;

	typecheck(pc, AAL5CellType);

	if ( ++counter == 0)
		errm1s("%s: overflow of cell counter", name);

	if( ++last_cell != pc->cell_seq)	// wrong cell (cell loss) -> empty queue
	{	cell_loss += pc->cell_seq - last_cell;	// log the detected lost cells
		last_cell = pc->cell_seq;
		
		// drop all cells in queue
		while ((pd = q.dequeue()) != NULL)
			delete pd;
	}

	//queue the cell
	q.enqueue(pc);

	if(pc->pt == 1)		// end of SAR-SDU
	{	aal5Cell	*phead = (aal5Cell *) q.first();	// there should be at last one cell

		if(pc->first_cell == phead->cell_seq)	// SDU is valid, no pt=1 cell was lost
		{	// forward data item embedded in last cell
			if (pc->embedded)
			{	suc->rec(pc->embedded, shand);
				pc->embedded = NULL;	// this is important, otherwise pc->embedded
							// is deleted by its receiver *and*
							// when deleting pc !!
			}
			else	errm1s("%s: no data item embedded in last cell of frame", name);
			
			// log the loss of AAL SDUs if any
			sdu_loss = sdu_loss + phead->sdu_seq - last_sdu - 1;
			last_sdu = phead->sdu_seq;	// store this sdu sequence number for statistic

			// log the delay of AAL SDU
			++sdu_cnt;
			delay = SimTime - phead->time;	// the delay spans from generating
							// the first cell by the sender
							// until delivering the frame to user
			delay_mean = (delay_mean * (sdu_cnt - 1) + delay) / sdu_cnt;
		}

		// empty the queue
		while ((pd = q.dequeue()) != NULL)
			delete pd;
	}
		
	return ContSend;
}


/********************************************************************/

int	aal5rec::command(char *s, tok_typ *v)
{
	if (baseclass::command(s, v) == TRUE)
		return TRUE;

	v->tok = NILVAR;
	if(strcmp(s, "ResetStat") == 0)		// resets the statistic
	{	cell_loss = 0;
		sdu_loss = 0;
		sdu_cnt = 0;
		counter = 0;
		delay_mean = 0.0;
	}
	else	return FALSE;

	return TRUE;
}


/********************************************************************/
/*
*	export addresses of variables
*/
int	aal5rec::export(exp_typ *msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "QLen", &q.q_len) ||
		intScalar(msg, "CellLoss", &cell_loss) ||
		intScalar(msg, "CellCount", (int *) &counter) ||	// additional: aal->Counter
		intScalar(msg, "SDULoss", &sdu_loss) ||
		intScalar(msg, "SDUCount", (int *) &sdu_cnt) ||
		doubleScalar(msg, "DelayMean", &delay_mean);
}

