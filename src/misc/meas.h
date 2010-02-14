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
#ifndef	_MEAS_H_
#define	_MEAS_H_

#include "in1out.h"

//tolua_begin
class	meas:	public	in1out {
  typedef	in1out	baseclass;
  
 public:	
  meas();
  ~meas();
  rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
  int	export(exp_typ *);
  int getDist(int idx) {return this->dist[idx];}
  void resDist(void){
    int i;
    greater_cnt = 0;
    for (i=0; i < maxtim; i++) dist[i] = 0;
  }
  int act(void){
    int i;
    if (vci != NILVCI)
      inp_type = CellType;
    else
      inp_type = DataType;
    CHECK(dist = new unsigned int[maxtim]);
    for (i = 0; i < maxtim; ++i)
      dist[i] = 0;
    return 0;
  }
  dat_typ	inp_type;		// type of input data
  
  int		maxtim;		/* max. cell transfer time */
  unsigned	greater_cnt;	/* counter for not registered times */
  //tolua_end
  unsigned	*dist;		/* CTDs */
}; //tolua_export


#endif	// _MEAS_H_
