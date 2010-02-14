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

// changed 2004-10-15: included deterministic_behaviour variable.
// If deterministic_behaviour is set to 1, the burst and silence
// lengths are fix, not chosen from a distribution

#ifndef	_BSSRC_H_
#define	_BSSRC_H_

#include "in1out.h"

//tolua_begin
class	bssrc:	public	in1out {	
typedef	in1out	baseclass;

public:
	bssrc();
	~bssrc();	

	void	SetDeterministicEx(int det); // included 2004-10-29
	void	SetDeterministicEs(int det); // included 2004-10-29

	int act(void);

	double		ex;		
	double		es;	
	int		delta;
	int		dist_burst;
	int		dist_silence;
	int		state;			/* current state:
						* is decremented with each sent cell. If
						* state = 0, the current burst is finished
						*/
	int deterministic_ex;	// included 2004-10-15
	int deterministic_es;	// included 2004-10-15
//tolua_end

	void	early(event *);
};  //tolua_export

#endif	// _BSSRC_H_
