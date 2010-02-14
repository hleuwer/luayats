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
*	Creation:		Nvo 14, 1997
*
*	History:	
*
*************************************************************************/

/*
*	A statistics object: calculation of confidence intervals
*
*	ConfidObj x {: LEVEL=0.99};
*		// LEVEL of confidence, default: 0.95
*
*	Commands:
*
*	void	x->Add(double)		// add a value
*	int	x->Len			// get current # of values
*	double	x->Val(int)		// get a value (1 ... Len)
*					// do not export: values-pointer may change!
*
*	void	x->Flush		// flush all values (Len := 0)
*
*	double	x->Mean			// mean
*	double	x->Var			// *empirical* variance
*	double	x->Lo			// lower bound of convid interv
*	double	x->Up			// upper bound of convid interv
*	double	x->Width		// Up - Mean
*	double	x->Lo(double)		// level given, may differ from LEVEL
*	double	x->Up(double)		// level given, may differ from LEVEL
*	double	x->Width(double)	// level given, may differ from LEVEL
*       double x->FairInd     	        // Fairnessindex (Jain)
*/

#include "defs.h"

//tolua_begin
class	confidObj: public root {
  typedef	root	baseclass;
public:
  confidObj();
  ~confidObj();
  void add(double v);
  void flush(void) {nVals = 0;}
  int getLen(void) {return nVals;}
  double getVal(int i){return values[i];}
  double getMean(double *pv=0);
  double getVar(double *pm=0);
  double getLo(double);
  double getUp(double);
  double getWidth(double);
  double getMin(void);
  double getMax(void);
  double getFairInd(void);
  double getCorr(int, int);
  //tolua_end
  void	calcConf(double, double *, double *);
  double studentDist(double, int);
  void meanVar(double *, double *);

  double *values;
  //tolua_begin
  double level;
  int nVals;
  int vectLen;
  enum	{vectGran = 10};
};
//tolua_end
