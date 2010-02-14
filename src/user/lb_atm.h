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

#ifndef	_LB_ATM_H_
#define	_LB_ATM_H_

#include "in1out.h"

class	lb_atmf:	public	in1out {
typedef	in1out	baseclass;

public:	
	void	init(void);
	rec_typ	REC(data *, int);
	void	restim(void);
	int	export(exp_typ *);
	int	command(char *, tok_typ *);

	dat_typ	inp_type;	// input data type

private:
	tim_typ	last_time;
	
	double	lb_inc;
	double	lb_siz;
	double	lb_max;	
	double	lb_dec;
	int 	lb_tag;
	
	// Problem Memory Mue 08.06.1998   int	*lbSizStat;
};
	
#endif	// _LB_ATM_H_
