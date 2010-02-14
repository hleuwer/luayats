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
*	An object providing a r.n. transformation table to other objects.
*	There are several possibilities to specify the distribution:
*
*	Distribution dist: FILE="file";
*		// read distribution from an ASCII file, file format:
*		// # for every X value (ascending order, values with zero p(x) can be omitted):
*		//	x	p	# comment
*	Distribution dist: TABLE=(2,0.5),(5,0.5);
*		// distribution directly given in the simulator input file
*	Distribution dist: DISTRIB(i = 1 to imax, 1.0/imax);
*		// distribution specified by a formula
*	Distribution dist: GEOMETRIC(E=20.5);
*		// geometrical distribution (shifted by one, i.e. p(0) == 0)
*	Distribution dist: BINOMIAL(N=10,P=0.3);
*		// binomial distribution (shifted by one, i.e. p(0) == 0)
*
*	In all cases, the sum of the values is checked against accuracy DISTR_ERR
*/

#include "distrib.h"

distrib::distrib()
{
}

distrib::~distrib()
{
}

// Binomial coefficient (k over r)
double distrib::binom(int k, int r)
{
  double retv;
  int	i;
  retv = 1.0;
  for (i = k - r + 1; i <= k; ++i)
    retv *= ((double) i) / ((double) (i - k + r));
  return retv;
} /* binom() */

//
//	export the distribution table
//
char *distrib::special(specmsg *msg, char *)
{
  if (msg->type != GetDistTabType)
    return "wrong type of special message";
  
  ((GetDistTabMsg *)msg)->table = table;
  return NULL;
}
#if 0
// skip comments in a file
//   return 0 if EOF, 1 otherwise
static int skip_comment(FILE *fp)
{
  int c;

  for (;;){
    switch (c = getc(fp)) {
    case ' ':
    case '\t':
    case '\n':
      continue;
    case EOF:
      return 0;
    case '#':
      do
	if ((c = getc(fp)) == EOF)
	  return 0;
      while (c != '\n');
      continue;
    default:ungetc(c, fp);
      return 1;
    }
  }
}
#endif
//
//	establish distribution table
//
#if 0
char *distrib::calc_table(FILE *fp, enum spec_mode mode)
{
  static char low_err[] = "X-value lower than 1 encountered";
  static char nonasc_err[] = "non-ascending X-values encountered";
  static char read_err[] = "error reading file";
  static char prob_err[] = "invalid probability encountered";
  static char consist_err[] = "distribution inconsistent (PDF of largest X is not equal to one)";
  
  int	pos;
  tim_typ	x, xmin, xmax, xold;
  double	sum, prob, pbin, qgeo;
  double	Pdf, z;
  
  xmin = xmax = 1;	// against compiler warning
  pbin = qgeo = 1.0;	// against compiler warning
  //	
  //		First check given distributions
  //	
  sum = 0.0;
  xold = 0;
  switch (mode) {
  case FileMode:
    while (skip_comment(fp) == 1){
      if (fscanf(fp, "%u%lg", &x, &prob) != 2)
	return read_err;
      if (x < 1)
	return low_err
	  if (x <= xold)
	    return noasc_err;
      xold = x;
      
      if (prob < 0.0 || prob > 1.0)
	return prob_err;
      sum += prob;
    }
    rewind(fp);	// for second reading
    break;

  case TableMode:
    return "Not yet implemented"
    break;
  case DistMode:
    xmin = this->xmin;
    xmax = this->xmax;

    // for each x: evaluate expression
    for (x = xmin; x <= xmax; ++x){
      //	evaluate expression
      prob = this->prob;
      if (prob < 0.0 || prob > 1.0)
	return prob_err;
      sum += prob;
    }
    break;
  case GeoMode: 
    {
      double expct;
      expct = this->expct;
      if (expct < 1.0)
	return "E can't be smaller than 1.0";
      qgeo = (expct - 1.0) / expct;
    }
    sum = 1.0;	// hopefully
    break;
  case BinomialMode:
    xmax = this->xmax;
    if ((xmax = this->xmax) <= 0)
      return "N has to be larger than zero";
    if ((pbin = this->pbin) < 0.0 || pbin > 1.0)
      return prob_err;
    sum = 1.0;	// hopefully
    break;
  default:
    return "internal error: distrib::calc_table(): invalid mode";
  }
  
  if (fabs(1.0 - sum) > DISTR_ERR)
    return consist_err;
  
  CHECK(table = new tim_typ[RAND_MODULO]);
  
  //	
  // Establish r.n. transformation table
  //	
  // Transformation algorithm like in geo1.c
  Pdf = 0.0;
  x = 0;
  for (pos = 0; pos < RAND_MODULO; ++pos) {
    z = (pos + 0.5) / (double) RAND_MODULO;
    // Z: the r.n. uniformly distributed in [0,1] 
    while (Pdf < z){
      // Look for the smallest X, for which the distribution function is
      // not smaller than z 
      switch (mode) {
      case FileMode:
	if (skip_comment(fp) != 1 || fscanf(fp, "%u%lg", &x, &prob) != 2)
	  return read_err
	break;
      case TableMode:
	return "Not yet implemented";
	break;
      case DistMode:
	if (x == 0)
	  x = xmin;
	else if (x < xmax)
	  ++x;
	else
	  return consist_err;
	set_pos( &label);
	prob = this->prob;
	break;
      case GeoMode:
	if (x == 0){
	  prob = 1.0 - qgeo;
	  x = 1;
	} else {
	  ++x;
	  prob *= qgeo;
	}
	break;
      case BinomialMode:
	if (x == 0)
	  x = 1;
	else if (x <= xmax)
	  ++x;
	else	
	  return "internal error: distrib::calc_table(): fatal in binomial";
	prob = binom(xmax, x - 1) * pow(pbin, (double) x - 1)
	  * pow(1.0 - pbin, (double) xmax - (x - 1));
	break;
      }
      Pdf += prob / sum;
    }
    // this X is assigned to the trafo table
    table[pos] = x;
  }
  
  //	clean up:
  switch (mode) {
  case TableMode:	
    // may be, there are more table entries with e.g. p == 0.0
    return "Not yet implemented."
    break;
  case DistMode:
  default:
    break;
  }
  
}
#endif
#if 0
void distrib::FileMode(char *fname)			// NOT FINISHED !!!
{
  
  FILE	*fp;
  s = new char[strlen(fname)];
  strcpy(s, fname);
  --  printf("s=%s\n", s);
  
  fp = fopen(s, "r");
  
  delete s;
  calc_table(fp, FileMode);
  fclose(fp);
  printf("FILE closed!");
}
#endif
#if 0
void	distrib::init(void)
{
  char	*s;
  FILE	*fp;
  
  skip(CLASS);
  name = read_id(NULL);
  skip(':');
  
  if (test_word("FILE"))
    {	s = read_string("FILE");
    if ((fp = fopen(s, "r")) == NULL)
      syntax1s("could not open file `%s'", s);
    delete s;
    calc_table(fp, FileMode);
    fclose(fp);
    }
  else if (test_word("TABLE"))
    {	skip_word("TABLE");
    skip('=');
    calc_table(NULL, TableMode);
    }
  else if (test_word("DISTRIB"))
    {	skip_word("DISTRIB");
    calc_table(NULL, DistMode);
    }
  else if (test_word("GEOMETRIC"))
    {	skip_word("GEOMETRIC");
    calc_table(NULL, GeoMode);
    }
  else if (test_word("BINOMIAL"))
    {	skip_word("BINOMIAL");
    calc_table(NULL, BinomialMode);
    }
  else	syntax0("`FILE', `TABLE', `DISTRIB', or `BINOMIAL' expected");
  
}

#endif
