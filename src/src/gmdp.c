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
*	History:	
*	July 1996:	DIST parameter added, M. Baumann
*/

/*
*	GMDP source with arbitrarily distributed phase durations -
*		- geometrical distributions as special case provided
*
*	arbitrarily distributed phase durations:
*		GMDPquelle <name>: NSTAT=<# of states>, DELTA=<list of cell spacings>, \
*			DIST=<distributions of # of cells per state>, TRANS=<transition probs.>, \
*			VCI=<vci>, OUT=<next node>;
*	geometrically distributed phase durations:
*		GMDPquelle <name>: NSTAT=<# of states>, DELTA=<list of cell spacings>, \
*			EX=<mean # of cells per state>, TRANS=<transition probs.>, \
*			VCI=<vci>, OUT=<next node>;
*
*		GMDPquelle gmdp: NSTAT=2, DELTA=(1,2), EX=(3.5,4.6), TRANS=(0,1,1,0), VCI=1, OUT=sink;
				// geometrically distributed phase durations
*		GMDPquelle gmdp: NSTAT=2, DELTA=(1,2), DIST=(dist1,dist2), TRANS=(0,1,1,0), VCI=1, OUT=sink;
*				// dist1 and dist2 are distribution objects defined in advance
*
*		NSTAT and DELTA:	integer
*		DIST:			object
		EX:			double
*		TRANS:			double
*
*		DELTA gives the cell distances,
*		TRANS containes the matrix of transition probabilities (row by row).
*		To generate states with bit rate 0, set DELTA to zero. DIST in this case depicts the
*		silence duration distribution.
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

#include "gmdp.h"

#define	TRANS_ERR	(1.0e-10)	/* max. inconsistency of transition prob. matrix of GMDP sources */
#define	TRIAL_MAX	(10000)		/* max # of trials to leave a zero bit rate state (GMDP) */


/*
*	read create statement
*/
gmdpsrc::gmdpsrc(void)
{
  trans = NULL;
}
gmdpsrc::~gmdpsrc(void)
{
  int i;

  for (i = 0; i < n_stat; i++)
    delete[] trafo[i];
  delete[] trafo;
  delete[] tables;
  delete[] delta;
  
}

void gmdpsrc::allocTables(int n_stat)
{
  int i;

  CHECK(delta = new int[n_stat]);
  CHECK(tables = new tim_typ *[n_stat]);
  CHECK(trafo = new int *[n_stat]);
  for (i = 0; i < n_stat; ++i)
    CHECK(trafo[i] = new int[n_stat]);
  
  /* auxiliary array to store transition probs. */
  CHECK(trans = new double[n_stat * n_stat]);
}

int gmdpsrc::act(void)
{
  int i, k;
  double sum;
  tim_typ tim;
  
  // transformation table for transition probabilities
  for (i = 0; i < n_stat; ++i) {
    sum = 0.0;
    for (k = 0; k < n_stat; ++k) {
      sum += trans[i * n_stat + k];
      trafo[i][k] = (int) (sum * RAND_MODULO + 0.5);
    }
    if (fabs(1.0 - sum) >= TRANS_ERR)
      errm1s("%s: transition probabilities inconsistent", name);
  }
  delete[] trans;
  trans = NULL;
	
  // 	randomly choosen starting phase
  //	(for reasons of simplicity uniformly distributed)
  //
  tim = 0;
  for (;;) {
    state = my_rand() % n_stat;
    if (delta[state] != 0)
      break;
    //tim += geo1_rand(dists[state]);
    tim += tables[state][my_rand() % RAND_MODULO];
  }

  // cell_cnt = geo1_rand(dists[state]);
  cell_cnt = tables[state][my_rand() % RAND_MODULO];
	
  alarme( &std_evt, tim + (my_rand() % delta[state]));
  return 0;
}
#if 0
void	gmdp::init(void)
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

	CHECK(delta = new int[n_stat]);
	CHECK(tables = new tim_typ *[n_stat]);
	CHECK(trafo = new int *[n_stat]);
	for (i = 0; i < n_stat; ++i)
		CHECK(trafo[i] = new int[n_stat]);

	/* auxiliary array to store transition probs. */
	CHECK(trans = new double[n_stat * n_stat]);

	/*
	*	parse DELTA, EX, and TRANS arrays
	*/
	skip_word("DELTA");
	skip('=');
	skip('(');
	for (i = 0; i < n_stat; ++i)
	{	delta[i] = read_int(NULL);
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
		if (delta[i] != 0)
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
		if (delta[state] != 0)
			break;
		//tim += geo1_rand(dists[state]);
		tim += tables[state][my_rand() % RAND_MODULO];
	}

	//cell_cnt = geo1_rand(dists[state]);
	cell_cnt = tables[state][my_rand() % RAND_MODULO];
	
	alarme( &std_evt, tim + (my_rand() % delta[state]));
}
#endif
/*
*	send a cell, register again
*/
void gmdpsrc::early(event *)
{
  int	r, t, st;
  int	*p;
  int	trials;
  tim_typ tim;
  
  // send cell
  suc->rec(new cell(vci), shand);
  
  // count cells
  if (++counter == 0)
    errm1s("%s: overflow of departs", name);
  
  //	when to send next cell?
  if (--cell_cnt > 0)
    //	state lasts for more cells
    tim = delta[state];
  else {
    //	this was the last cell, determine next state
    t = 0;
    trials = 0;
    st = state;
    for (;;) {
      // get and transform r.n.
      r = my_rand() % RAND_MODULO;
      p = trafo[st];
      for (st = 0; st < n_stat; ++st)
	if (r < p[st])
	  break;
      if (st >= n_stat)
	errm1s("%s: gmdp::early(): bad state transition", name);
      
      if (delta[st] != 0)
	break;
      
      // in case of a state with zero bit rate, look ahead to find 
      // next cell
      // t += geo1_rand(dists[st]);
      t += tables[st][my_rand() % RAND_MODULO];
      if ( ++trials >= TRIAL_MAX)
	errm1s2d("%s: gmdp::early(): could not leave state no. %d "
		 "after TRIAL_MAX=%d attempts", name, st + 1, TRIAL_MAX);
    }
    // non zero bit rate state reached
    // cell_cnt = geo1_rand(dists[st]);
    cell_cnt = tables[st][my_rand() % RAND_MODULO];
    state = st;
    tim = t + delta[st];
  }
  
  //	register for next cell
  alarme( &std_evt, tim);
  
}

