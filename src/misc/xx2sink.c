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
*	Creation:		1996
*
*************************************************************************/

/*
*	A sink recognizing sequence numbers
*
*	For each errored burst the number of missing cells is printed to stdout.
*	Output is started after having received the command
*		src->Start;
*/

#include "sink.h"

class	xx2snk:	public	sink {
typedef	sink	baseclass;

public:
	void	addpars(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
	int	command(char *, tok_typ *);

	int	burst_no;
	int	burst_len;
	int	burst_cnt;

	int	out_flag;
};

CONSTRUCTOR(Xx2sink, xx2snk);

void	xx2snk::addpars(void)
{
	burst_no = burst_cnt = burst_len = 0;
	out_flag = FALSE;
}

rec_typ	xx2snk::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int)
{
	cellSeq	*pc = (cellSeq *) pd;

	typecheck(pd, CellSeqType);

	if ( ++counter == 0)
		errm1s("%s: overflow of counter", name);

	if (pc->burst_no != burst_no)
	{	if (burst_cnt != burst_len && out_flag == TRUE)
			printf("%d\n", burst_len - burst_cnt);
		burst_no = pc->burst_no;
		burst_len = pc->burst_len;
		burst_cnt = 1;	// this was the first cell of the new burst
	}
	else	++burst_cnt;
		
	delete pd;

	return ContSend;
}

int	xx2snk::command(
	char	*s,
	tok_typ	*v)
{
	if (baseclass::command(s, v) == TRUE)
		return TRUE;

	v->tok = NILVAR;
	if (strcmp(s, "Start") == 0)
		out_flag = TRUE;
	else	return FALSE;

	return TRUE;
}

