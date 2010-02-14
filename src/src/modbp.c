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
*	Creation:		Jan 1997
*
*************************************************************************/

/*
*	History:	
*	July 1996:	DIST parameter added, M. Baumann
*/

/*
*	"ModBP - Modulated Bernoulli Process":
*		source with arbitrarily distributed phase durations -
*		- geometrical distributions as special case provided
*	During one state, cell distances are geometrically distributed.
*
*	arbitrarily distributed phase durations:
*		ModBP <name>: NSTAT=<# of states>, ED=<list of mean cell spacings>, \
*			DIST=<distributions of state durations>, TRANS=<transition probs.>, \
*			VCI=<vci>, OUT=<next node>;
*	geometrically distributed phase durations:
*		ModBP <name>: NSTAT=<# of states>, ED=<list of mean cell spacings>, \
*			EX=<mean state duration>, TRANS=<transition probs.>, \
*			VCI=<vci>, OUT=<next node>;
*
*		NSTAT:			integer
*		ED:			double
*		DIST:			object
*		EX:			double
*		TRANS:			double
*
*		ED gives the mean cell distances (geometrically distributed), EX or DIST
*		specify the mean state durations (in TIME SLOTS!).
*		TRANS containes the matrix of transition probabilities (row by row).
*		To generate states with bit rate 0, set ED to zero.
*
*	Commands:
*		<Name>->Count
*		<Name>->ResCount
*
*	ATTENTION:
*	The source tries to find the instant of sending next cell with each activation.
*	In case it is impossible to leave a state with bit rate zero, the system will stop.
*	Protection against this in early(): max. number of trials is TRIAL_MAX
*/

#include "modbp.h"

#define	TRANS_ERR	(1.0e-10)	/* max. inconsistency of transition prob. matrix of GMDP sources */
#define	TRIAL_MAX	(10000)		/* max # of trials to leave a zero bit rate state (GMDP) */

CONSTRUCTOR(ModBP, modBP);

/*
*	read create statement
*/
void	modBP::init(void)
{
	extern	tim_typ	*get_geo1_table(int);
	int	i, k;
	double	*trans;
	double	sum;
	tim_typ	tim;
	char	*s, *err;
	root	*obj;
	GetDistTabMsg	msg;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	n_stat = read_int("NSTAT");
	skip(',');

	CHECK(ed = new double[n_stat]);
	CHECK(tables = new tim_typ *[n_stat]);
	CHECK(edtables = new tim_typ *[n_stat]);
	CHECK(trafo = new int *[n_stat]);
	for (i = 0; i < n_stat; ++i)
		CHECK(trafo[i] = new int[n_stat]);

	/* auxiliary array to store transition probs. */
	CHECK(trans = new double[n_stat * n_stat]);

	/*
	*	parse DELTA, EX, and TRANS arrays
	*/
	skip_word("ED");
	skip('=');
	skip('(');
	for (i = 0; i < n_stat; ++i)
	{	ed[i] = read_double(NULL);
		if (ed[i] != 0.0)
			edtables[i] = get_geo1_table(get_geo1_handler(ed[i]));
		else	edtables[i] = NULL;
		if (i != n_stat - 1)
			skip(',');
	}
	skip(')');
	skip(',');

	// geometrical sojourn time distributions?
	if (test_word("EX"))
	{	// yes
		skip_word("EX");
		skip('=');
		skip('(');
		for (i = 0; i < n_stat; ++i)
		{	tables[i] = get_geo1_table(get_geo1_handler(read_double(NULL)));
			if (i != n_stat - 1)
				skip(',');
		}
		skip(')');
	}
	else if (test_word("DIST"))
	{	// get distribution from a distribution object
		skip_word("DIST");
		skip('=');
		skip('(');
		for (i = 0; i < n_stat; ++i)
		{	s = read_suc(NULL);
			if ((obj = find_obj(s)) == NULL)
				syntax2s("%s: could not find object `%s'", name, s);
			if ((err = obj->special( &msg, name)) != NULL)
				syntax2s("could not get distribution table, reason returned by `%s':\n\t%s",
						s, err);
			tables[i] = msg.table;
			delete s;

			if (i != n_stat - 1)
				skip(',');
		}
		skip(')');
	}
	else	syntax0("`EX' or `DIST' expected");
	skip(',');

	skip_word("TRANS");
	skip('=');
	skip('(');
	for (i = 0; i < n_stat * n_stat; ++i)
	{	trans[i] = read_double(NULL);
		if (i != n_stat * n_stat - 1)
			skip(',');
	}
	skip(')');
	skip(',');

	vci = read_int("VCI");
	skip(',');

	// read additional parameters of derived classes
	addpars();

	output("OUT");

	for (i = 0; i < n_stat; ++i)
		if (edtables[i] != NULL)
			break;
	if (i >= n_stat)
		syntax1s("%s: only states with zero bitrate", name);

	/*
	*	transformation table for transition probabilities
	*/
	for (i = 0; i < n_stat; ++i)
	{	sum = 0.0;
		for (k = 0; k < n_stat; ++k)
		{	sum += trans[i * n_stat + k];
			trafo[i][k] = (int) (sum * RAND_MODULO + 0.5);
		}
		if (fabs(1.0 - sum) >= TRANS_ERR)
			syntax1s("%s: transition probabilities inconsistent", name);
	}
	delete trans;

	/* 	randomly choosen starting phase
	*	(for reasons of simplicity uniformly distributed)
	*/
	tim = 0;
	for (;;)
	{	state = my_rand() % n_stat;
		if (edtables[state] != NULL)
			break;		// we have found a state with non-zero bit rate
		tim += tables[state][my_rand() % RAND_MODULO];
	}

	time_left = tables[state][my_rand() % RAND_MODULO];
	// the first cell distance is chosen to ed[state]+1
	if (time_left > ((int)ed[state]) + 1)
		time_left -= ((int)ed[state]) + 1;
	else	time_left = 0;
		
	alarme( &std_evt, tim + ((int)ed[state]) + 1);
}

/*
*	send a cell, register again
*/
void	modBP::early(event *)
{
	int	r, st;
	int	*p;
	int	trials;
	tim_typ	tim;
	tim_typ	next_distance;

	//	send cell
	suc->rec(new cell(vci), shand);

	//	count cells
	if ( ++counter == 0)
		errm1s("%s: overflow of departs", name);

	//	when would we like to send next cell ?
	next_distance = edtables[state][my_rand() % RAND_MODULO];

	//	is this instance still inside off the current phase ?
	if (next_distance <= (tim_typ) time_left)
	{	// this instance still is part of the current phase, we can send the cell
		tim = next_distance;
		time_left -= next_distance;	// reduce the sojourn time still to stay in the current state
	}
	else
	{	//	this was the last cell, determine next state
		tim = time_left;	// the time the current state still lasts
		trials = 0;
		st = state;
		for (;;)
		{	//	get and transform r.n.: transition to next state
			r = my_rand() % RAND_MODULO;
			p = trafo[st];
			for (st = 0; st < n_stat; ++st)
				if (r < p[st])
					break;
			if (st >= n_stat)
				errm1s("%s: modBP::early(): bad state transition", name);

			if (edtables[st] != NULL)	// a state with non-zero bit rate
			{	// we have to check whether we really can send a cell
				time_left = tables[st][my_rand() % RAND_MODULO];
				next_distance = edtables[st][my_rand() % RAND_MODULO];
				if (next_distance <= (tim_typ) time_left)
				{	// the state duration is long enough
					time_left -= next_distance;
					state = st;
					tim += next_distance;
					break;
				}
				else
				{	// state duration too short: we can't send
					tim += time_left;
				}
			}

			else	//	case of a state with zero bit rate
			{	tim += tables[st][my_rand() % RAND_MODULO];
			}

			// avoid endless loops
			if ( ++trials >= TRIAL_MAX)
			    errm1s2d("%s: modBP::early(): could not leave state no. %d "
				"after TRIAL_MAX=%d attempts", name, st + 1, TRIAL_MAX);
		}
	}

	//	register for next cell
	alarme( &std_evt, tim);

}

