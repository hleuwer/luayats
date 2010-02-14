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
#ifndef	_AAL5SEND_H
#define	_AAL5SEND_H

#include "inxout.h"
#include "queue.h"

class	aal5send:	public inxout
{
   typedef	inxout	baseclass;
 public:
   void	init(void);
   void	early(event *);
   rec_typ REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
   
   int	command(char *, tok_typ *);
   int	export(exp_typ *);
   
   void	restim(void);	// resets the time dependent values
   
   queue	q;		// input queue
   int	q_start;	// queue length at which to wake up the sender
   
   int	fixVCI;		// TRUE: do always use the VCI specified in definition statement
   int     new_cid; 	// the new CID
   int     CopyClp;        // must clp bit be copied?
   int     NewClp;         // the new clp bit
   int     Pt1Clp0;        // must Pt=1 cells be set to CLP=0?
   int	maxcid;		// range of layer-4 connection IDs
   int	*translationTabVCI;// translation connection ID -> VCI
   // NULL: copy connection ID of packets into VCI of cells
   int	*translationTabCID;// translation connection ID -> new ID
   
   
   
   int	*cellSeqTab, *curCellSeq;	// cell sequence numbers (one for each connection)
   int	*sduSeqTab, *curSduSeq;		// SDU seq no
   
   size_t	sdu_cnt;	// SDUs sent
   size_t	del_cnt;	// counter of not transmitted frames because of not minded StopSend
   
   int	flen;		// number of bytes still to send for the current frame
   int	first_seq;	// first cell sequence number of an AAL-PDU:
   // is retransmitted in last cell
   
   rec_typ	prec_state;	// state of the preceeding object
   rec_typ	send_state;	// state of this object
   
   int     addHeader;      // length of additional header (e.g LLC/SNAP)
   
   enum	{SucData = 0, SucCtrl = 1};
   enum	{InpData = 0, InpStart = 1};
   
   tim_typ	last_tim;	// when sent last (only a check to prevent sending twice in a slot)
   
   int printwarning;  // print warning messages (start/stop signal not recognized)?
   
};

#endif	// _AAL5SEND_H
