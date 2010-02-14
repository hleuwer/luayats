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
*	Transformation of uniformly distributed random numbers into geometrically distrib. r.n.
*
*	Somebody needing geometrically distributed numbers registers with
*		handle = get_geo1_handler(double mean_value);
*	R.n's afterwards are obtained by
*		val = geo1_rand(handle);
*
*	For GMDPquelle: export of the transformation table:
*		table = get_geo1_table(handle);
*/

#include "defs.h"

#define	MAX_GEO1	(100)	// max. # of distributions managed by geo1_rand()

static	int	*distrib[MAX_GEO1];
static	double	expect[MAX_GEO1];
static	int	dist_cnt = 0;


int get_geo1_handler(double expct)
{
  int pos, k;
  double p, q, P, E, z;
  if (expct < 1.0)
    errm0("get_geo1_handler(): Expectation of GEO1 may not be lower than 1.0");
  for (k = 0; k < dist_cnt; ++k)
    if (expect[k] == expct)
      return k;	// we already have a transformation table

  if (dist_cnt >= MAX_GEO1)
    errm1d("get_geo1_handler(): too many Geo1-distributions, MAX_GEO1 = %d", MAX_GEO1);

  expect[dist_cnt] = expct;
  CHECK(distrib[dist_cnt] = new int[RAND_MODULO]);

  /*
   *	transformation of r.n. according to:
   *		Gnedenko, Handbuch Bedientheorie, S.354
   */

  E = 0.0;
  q = (expct - 1.0) / expct;
  k = 1;
  p = 1.0 - q;	/* probability mass function of k = 1 */
  P = p;		/* distribution function of k = 1 */
  for (pos = 0; pos < RAND_MODULO; ++pos) {
    z = (pos + 0.5) / (double) RAND_MODULO;
    /* Z: the r.n. uniformly distributed in [0,1] */
    while (P < z){
      /* look for the smallest k, for which the distribution function is
	 not smaller than z */
      ++k;
      p *= q;	// probab. mass function of k
      P += p;	// distrib. function of k
    }
    // this k is assigned to the trafo table
    distrib[dist_cnt][pos] = k;
    E += (double) k;
  }
  /*
    fprintf(stderr, "get_geo1_handler(): new transformation table with\n"
    "\tE[] = %e\n\tmax[] = %d\n\tP = %e\n", E / RAND_MODULO, k, P);
  */
  
  return dist_cnt++ ;
}

int geo1_rand(int type)
{
  return distrib[type][my_rand() % RAND_MODULO];
}

/*
*	Export of a transformation table
*/
tim_typ	*get_geo1_table(int i)
{
  return (tim_typ *) distrib[i];
}


#ifdef	USE_MY_RAND
/*
*	This is the random number generator of IBM (AIX) and DEC (OSF/1)
*	We reimplement it to be independent of different versions on different platforms
*/

static long int	next = 1;

int	my_rand(void)
{
  next = next * 1103515245 + 12345;
  return (next >> 16) & 32767;
}

// included by Mue: 29.10.1999
double uniform()
{
  return my_rand() / 32767.0;
}


void my_srand(int seed)
{
  next = seed;
}

#endif	/* USE_MY_RAND */

/*
*	initialize the r.n. generator
*/

void my_randomize(void)
{
  time_t	tim;
  
  (void) time( &tim);
  my_srand((unsigned) tim);
}

