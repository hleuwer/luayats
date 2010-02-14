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
*************************************************************************/

/*
*	Base class for multiplexers
*
*	XXYYZZ mux: NINP=10, BUFF=10, {MAXVCI=100,} {SERVICE=2,} OUT=sink;
*			// default MAXVCI: NINP
*			// SERVICE: service of one data item takes SERVICE steps
*
*	Commands:	like Multiplexer (see mux.c)
*/

#include "muxBase.h"


/*
*	read definition statement
*/
void    muxBase::init(void)
{
        int     i;
 
        skip(CLASS);
        name = read_id(NULL);
        skip(':');
        ninp = read_int("NINP");
        if (ninp < 1)
                syntax0("invalid NINP");
        skip(',');
 
        if (doParseBufSiz)
                // this can be turned off by derived classes
        {       q.setmax(read_int("BUFF"));	// the queue does *not* comprise the server
		if (q.getmax() < 1)	// do not delete this test. Some derived classes rely on it.
			syntax0("invalid BUFF value");
                skip(',');
        }
 
        if (test_word("MAXVCI"))
        {       max_vci = read_int("MAXVCI") + 1;
                if (max_vci < 1)
                        syntax0("invalid MAXVCI");
                skip(',');
        }
        else    max_vci = ninp + 1;
 
        if (test_word("SERVICE"))
        {       serviceTime = read_int("SERVICE");
                if (serviceTime < 1)
                        syntax0("invalid SERVICE");
                skip(',');
        }
        else    serviceTime = 1;

        // read additional parameters of derived classes
        addpars();
 
        output("OUT");
 
        inputs("I",     // input names: ...->I[inp_no]
                ninp,   // ninp inputs
                -1);    // input 1 has input key 0

        CHECK(lost = new unsigned int[ninp]);
        for (i = 0; i < ninp; ++i)
                lost[i] = 0;
        CHECK(lostVCI = new unsigned int[max_vci]);
        for (i = 0; i < max_vci; ++i)
                lostVCI[i] = 0;
        lossTot = 0;
 
        CHECK(inp_buff = new inpstruct[ninp]);
        inp_ptr = inp_buff;
 
	needToSchedule = TRUE;		// late() not yet registered
	server = NULL;			// server idle
}

/*
*	Data item received, buffer it.
*	Wake up the late() method, if not yet done.
*/
rec_typ	muxBase::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	i)
{
        inp_ptr->inp = i;
        (inp_ptr++)->pdata = pd;

	if (needToSchedule)
	{	needToSchedule = FALSE;
		alarml( &evtLate, 0);
	}
 
        return ContSend;
}

/*
*	register a loss and delete the data item
*/
void	muxBase::dropItem(
	inpstruct	*p)
{
	if ( ++lost[p->inp] == 0)
		errm1s1d("%s: overflow of LossInp[%d]", name, p->inp + 1);

	if (typequery(p->pdata, CellType))
	{	int	vc;
		vc = ((cell *) p->pdata)->vci;
		if (vc >= 0 && vc < max_vci && ++lostVCI[vc] == 0)
			errm1s1d("%s: overflow of LossVCI[%d]", name, vc);
	}

	if ( ++lossTot == 0)
		errm1s("%s: overflow of LossTot", name);

	delete p->pdata;
}

/*
*	commands
*/
int	muxBase::command(
	char	*s,
	tok_typ	*v)
{
	int	lo, up;
	int	i;

	if (baseclass::command(s, v) == TRUE)
		return TRUE;

	if (strcmp(s, "Losses") == 0 || strcmp(s, "LossesInp") == 0)
	{	skip('(');
		lo = read_int(NULL);
		skip(',');
		up = read_int(NULL);
		skip(')');
	
		--lo;
		--up;
		if (lo < 0 || up >= ninp)
			syntax1s1d("%s: input numbers range from 1 to %d", name, ninp);
	
		v->val.i = 0;
		for (i = lo; i <= up; ++i)
			v->val.i += lost[i];
		v->tok = IVAL;
		return	TRUE;
	}
	if (strcmp(s, "LossesVCI") == 0)
	{	skip('(');
		lo = read_int(NULL);
		skip(',');
		up = read_int(NULL);
		skip(')');
	
		if (lo < 0 || up >= max_vci)
			syntax1s1d("%s: VC numbers range from 0 to %d", name, max_vci - 1);
	
		v->val.i = 0;
		for (i = lo; i <= up; ++i)
			v->val.i += lostVCI[i];
		v->tok = IVAL;
		return	TRUE;
	}
	else if (strcmp(s, "ResLoss") == 0)
	{	for (i = 0; i < ninp; ++i)
			lost[i] = 0;
		for (i = 0; i < max_vci; ++i)
			lostVCI[i] = 0;
		lossTot = 0;
		v->tok = NILVAR;
		return TRUE;
	}
	else	return FALSE;
}


/*
*	export addresses of variables
*/
int	muxBase::export(
	exp_typ *msg)
{
	return	baseclass::export(msg) ||
		intScalar(msg, "QLen", &q.q_len) ||
		intScalar(msg, "LossTot", (int *) &lossTot) ||
		intArray1(msg, "Loss", (int *) lost, ninp, 1) ||
		intArray1(msg, "LossInp", (int *) lost, ninp, 1) ||
		intArray1(msg, "LossVCI", (int *) lostVCI, max_vci, 0);
}

