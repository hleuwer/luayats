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
#ifndef	_SHAP_H_
#define	_SHAP_H_

#include "in1out.h"
#include "queue.h"

class	shap:	public	in1out {
typedef	in1out	baseclass;

public:	
	void	init(void);
	rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
	void	early(event *);
	void	restim(void);
	int	export(exp_typ *);

	tim_typ	next_time;
	int	delta;

	queue	q;
};

#endif	// _SHAP_H_
