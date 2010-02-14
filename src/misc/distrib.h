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

#ifndef _DISTRIB_H_
#define _DISTRIB_H_

#include "defs.h"
#include <string>
#include <stdio.h>
#define	DISTR_ERR	(1.0e-5)	// max. inconsistency of the given distribution

//tolua_begin
enum spec_mode {
  FileMode,
  TableMode,
  DistMode,
  GeoMode,
  BinomialMode
};

class distrib: public root {
  typedef	root	baseclass;
  
public:
  distrib();
  ~distrib();
  char *special(specmsg *, char *);
  double binom(int k, int r);
  //  void calc_table(FILE *, enum spec_mode);
  tim_typ table[RAND_MODULO];
  //tolua_end
  
  char *s;
  
};  //tolua_export

// static	double	binom(int, int);

/******************************************************************************************/
//	skip comments in a file
//	return 0 if EOF
//	return 1 otherwise
// static	int	skip_comment(
// 	FILE	*fp)
// {
// 	int	c;

// 	for (;;)
// 	{	switch (c = getc(fp)) {
// 		case ' ':
// 		case '\t':
// 		case '\n':
// 			continue;
// 		case EOF:
// 			return 0;
// 		case '#':
// 			do	if ((c = getc(fp)) == EOF)
// 					return 0;
// 			while (c != '\n');
// 			continue;
// 		default:ungetc(c, fp);
// 			return 1;
// 		}
// 	}
// }


#endif // _DISTRIB_H_
