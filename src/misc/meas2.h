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
#ifndef	_MEAS2_H_
#define	_MEAS2_H_

#include "in1out.h"

//tolua_begin
class	meas2:	public	in1out {
  typedef	in1out	baseclass;
  
public:
  meas2(){}
  ~meas2();
  int act(void);
  int getCTD(int i){return this->ctd_dist[i];}
  int getIAT(int i){return this->iat_dist[i];}
  dat_typ		inp_type;		// type of input data
  
  int		ctd_max;		// max cell transfer delay time
  int		iat_max;		// max inter arrival time
  unsigned	iat_overfl;		// overflow of IAT
  unsigned	ctd_overfl;		// overflow of CTD
  //tolua_end
  unsigned	*ctd_dist;		// CTDs 
  unsigned	*iat_dist;		// IATs 

  rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
  int	export(exp_typ *);
  
  tim_typ		last_time;
}; //tolua_export


#endif	// _MEAS2_H_
