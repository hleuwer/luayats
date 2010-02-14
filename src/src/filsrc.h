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
#ifndef	_FILSRC_H_
#define	_FILSRC_H_

#include "in1out.h"

class	filsrc:	public	in1out {
typedef	in1out	baseclass;

public:	
	void	init(void);
	void	early(event *);
	void	late(event *);
private:void	rd_file(void);

	FILE	*fil_fp;
	char	*fil_name;
	int	fil_len;
	int	fil_pos;
	int	rep_cnt;
	int	rep_max;
	tim_typ	*fil_buff;
	tim_typ	*buff_ptr;
	tim_typ	*buff_ptr_max;
};

#endif	// _FILSRC_H_
