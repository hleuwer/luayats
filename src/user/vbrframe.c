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
*	Module author:		Halid Hrasnica, TUD
*	Creation:		December 1997
*
*************************************************************************/

/*
*	VBR (variable bit rate) source for frames:	
*
*	VBRFrame src: DELTA=10, LEN=100, {StartTime=,}{EndTime=,} OUT=sink;
*		      DELTADIST=, LENDIST=
*
*	DELTA - arrival time between the frames
*	LEN - length of the arrived frame
*
*	DELTA can be constant value (DELTA=10) or given distribution (DELTADIST=dd)
*	It's the same with LEN (LEN or LENDIST)
*/

#include "vbrframe.h"


CONSTRUCTOR(Vbrframe, vbrframe);
USERCLASS("VBRFrame", Vbrframe);

void	vbrframe::init(void)
{
	char	*s, *err;
	root	*obj;
	GetDistTabMsg	msg;
	
	delta_dist=0;	   //reset of distribution existence identification
	len_dist=0;	   //reset of distribution existence identification
	len_dist_count=0;  //reset of identification "first packet from given distribution"
	
	skip(CLASS);
	name = read_id(NULL);
	skip(':');
	
	if (test_word ("DELTA"))	//load constant DELTA
	{
		delta = read_int("DELTA");
		delta_dist = 0;		//DELTA is sonstant
	}
	else if (test_word ("DELTADIST")) //load distribution for DELTA
	{
	
		s = read_suc("DELTADIST");
		if ((obj = find_obj(s)) == NULL)
			syntax2s("%s: could not find object `%s'", name, s);
		if ((err = obj->special( &msg, name)) != NULL)
			syntax2s("could not get distribution table, reason returned by `%s':\n\t%s",
				s, err);
	table_delta = msg.table; 	//distribution in table
	delta_dist = 1;		 	//DELTA is distributed
	delete s;
	}
	else syntax0(" `DELTA' or `DELTADIST' expected");
	
	if (delta_dist == 1) //take a value for DELTA from distribution if it's distributed
	{
		delta = table_delta[my_rand() % RAND_MODULO];
	}
	
	skip(',');
	
	if (test_word ("LEN"))	//load constant LEN
	{
		pkt_len = read_int("LEN");
		len_dist = 0;
	}
	else if (test_word ("LENDIST"))	//load distributed LEN
	{
	
		s = read_suc("LENDIST");
		if ((obj = find_obj(s)) == NULL)
			syntax2s("%s: could not find object `%s'", name, s);
		if ((err = obj->special( &msg, name)) != NULL)
			syntax2s("could not get distribution table, reason returned by `%s':\n\t%s",
				s, err);
	table_len = msg.table;
	len_dist = 1;
	delete s;
	}
	else syntax0(" `LEN' or `LENDIST' expected");
	
	skip(',');
	if(test_word("StartTime"))
	{
		StartTime = read_int("StartTime");
		skip(',');
	}
	else	StartTime = delta;
	if(StartTime == 0)
//		StartTime = my_rand() % 500000;	// 0 <= my_rand() <= approx. 32000
		StartTime = 50 * (my_rand() % 10000);
	if(test_word("EndTime"))
	{
		EndTime = read_int("EndTime");
		skip(',');
	}
	else	EndTime = 0-delta;
	
	if (len_dist == 1)	//take a value for LEN if it's distributed
	{
		pkt_len = table_len[my_rand() % RAND_MODULO];
		len_dist_count = 1;
	}
	
	bytes = 0-pkt_len;
	
	output("OUT");
	input("CTRL",0);

	alarme( &std_evt, StartTime);
	send_state = ContSend;
	sent = 0;
}

void	vbrframe::early(event *)
{
	if ( ++counter == 0)
		errm1s("%s: overflow of departs", name);
	
	if (len_dist == 1)	//take next value for LEN
	{
		if (len_dist_count == 0) //skip the next value for LEN if it's the first frame
		{
			pkt_len = table_len[my_rand() % RAND_MODULO];
			bytes = 0-pkt_len;
		}
		else	len_dist_count=0;
	}
	
	sent += pkt_len;

	chkStartStop(send_state = suc->rec(new frame(pkt_len), shand));
	
	if (delta_dist == 1)	//take next value for DELTA
	{
		delta = table_delta[my_rand() % RAND_MODULO];
	}

	if(send_state == ContSend && EndTime >= SimTime + delta && bytes > sent)
		alarme( &std_evt, delta);
	else if(send_state == StopSend)
		send_time = SimTime + delta;
}

rec_typ	vbrframe::REC(data *pd, int)
{
	if (send_state == StopSend)
	{	send_state = ContSend;
		if(SimTime >= send_time)
			alarme(&std_evt, 1);
		else	alarme(&std_evt, send_time - SimTime);
	}

	delete pd;
	return ContSend;
}

void	vbrframe::restim(void)
{
	if(send_time > SimTime)
		send_time -= SimTime;
	else	send_time = 0;
}
