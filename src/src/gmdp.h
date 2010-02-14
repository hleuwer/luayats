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
*************************************************************************/
#ifndef	_GMDP_H_
#define	_GMDP_H_

#include "in1out.h"

//tolua_begin
class gmdpsrc: public in1out {
  typedef	in1out	baseclass;
  
public:
  gmdpsrc(void);
  ~gmdpsrc(void);
  void early(event *);
  int act(void);
  void setTable(void *tab, int i){tables[i] = (tim_typ*)tab;}
  void *getTable(int i){return tables[i];}
  void setDelta(int delta, int i){this->delta[i] = delta;}
  int getDelta(int i){return this->delta[i];}
  void setTrans(double trans, int i){this->trans[i] = trans;}
  double getTrans(double trans, int i){return this->trans[i];}
  void allocTables(int n_stat);
  int n_stat;	// # of states 
  //tolua_end
  int *delta;	// cell distances: in case of 0 bit rate is zero,
		// ex then gives the phase duration 
  tim_typ **tables; // transformation tables for sojourn time distributions
  
  int state;	// current state 
  int cell_cnt;	// # of cells yet to be sent in the current state 
  int **trafo;	// transition probabilities between states, already stored
		// as PDFs 
  double *trans;

};//tolua_export

#endif	// _GMDP_H_
