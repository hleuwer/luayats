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
*	Made by HH, December 1997
*
*************************************************************************/
#ifndef	_VBRFRAME_H
#define	_VBRFRAME_H

#include "in1out.h"

class	vbrframe:	public in1out {
typedef	in1out	baseclass;
public:	
	void	init(void);
	void	early(event *);
	rec_typ	REC(data *, int);
	void	restim(void);
	
	tim_typ	*table_delta;	//distribution table for DELTA - frames distance
	tim_typ	*table_len;	//distribution table for LEN - length of frames
	int	delta_dist;	//existence of delta distribution, default=0
	int	len_dist;	//existence of len distribution
	int	len_dist_count; //first frame taken from distribution
	
	rec_typ	send_state;
	tim_typ	send_time;	// time for next request if stopped
	size_t	pkt_len;	// length of frames to be send
	tim_typ	delta;		// send interval 
	size_t	bytes;		// number of bytes to be send
	size_t	sent;		// number of bytes sent
	tim_typ	StartTime;	// Sim time to start sending
	tim_typ	EndTime;	// Sim time to finish sending
};

#endif	// _VBRFRAME_H
