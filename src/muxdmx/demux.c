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
*	Module author:		Matthias Baumann, TUD
*	Creation:		1996
*
*************************************************************************/

/*
*	Demultiplexer with VCI signalling
*
*	- an incomming cell is passed to an output according to its VCI
*	- VCIs are translated
*	- in case of a cell having a VCI for which no action was defined by signalling,
*	  an error message with hard stop is generated
*
*	- no commands
*	- new: 	Demux can used by FrameType with connID to selecting frames 
*		with  object "Signal->(old,new,output)"
*
*	Demultiplexer demux:	{FRAMETYPE,}	// demultiplexing of FrameType ( else CellType)
*						// using connID 
*						// and the object "Signal->(old,new,output)"
*						// to create a transfer-table 
*				MAXVCI=100,	// new: can use for max.connID in FrameType
*				NOUT=3,
*				OUT=sink[1], sink[2], sink[3];
*
*	or (equivalent, if names follow a regular structure):
*	Demultiplexer demux:	{FRAMETYPE,}
*				MAXVCI=100,
*				NOUT=3,
*				OUT=(i: sink[i]);
*			// MAXVCI:	largest input VCI (connID) number to be handled 
*			// NOUT:	# of outputs
*			// in case of a given counter variable (it has to be defined in advance),
*			// output names are generated by the template given
*
*
*
*/

#include "demux.h"


demux::demux()
{
}

demux::~demux()
{
	delete outp_tab;
	delete new_vci_tab;
}	


/*
*	read input statement
*/
/*
*	cell(frame) has arrived.
*	translate VCI (connID), pass the cell(frame) to the right output
*/
rec_typ demux::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*pd,
	int	)
{
  if (rec_type == rec_cell)	
  { 	
  cell	*pcell = (cell *) pd;
	int	outp, vc;

	typecheck(pcell, CellType);	// test on data input type cell

	if ((vc = pcell->vci) < 0 || vc >= nvci)
		errm1s1d("%s: VCI range from 0 to %d\n", name, nvci - 1);

	if ((outp = outp_tab[vc]) < 0 || outp >= noutp)
	{	if (outp == NILVCI)
			errm1s1d("%s: an input cell carried an unassigned VCI of %d",
					name, vc);
		else	errm1s2d("%s: outp_tab inconsistent, outp_tab[%d] = %d",
					name, vc, outp);
	}
	pcell->vci = new_vci_tab[vc];
	return sucs[outp]->rec(pcell, shands[outp]);
  }
  else // ( rec_type == rec_frame) was seted
  {	
  	frame	*pframe = (frame *) pd;
  	int	outp, vc;
  	
  	typecheck(pframe, FrameType); // test on data input type frame

	if ((vc = pframe->connID) < 0 || vc >= nvci)
		errm1s1d("%s: connID of frame range from 0 to %d\n", name, nvci - 1);

	if ((outp = outp_tab[vc]) < 0 || outp >= noutp)
	{	if (outp == NILVCI)
			errm1s1d("%s: an input frame carried an unassigned connID of %d\n",
					name, vc);
		else	errm1s2d("%s: outp_tab inconsistent, outp_tab[%d] = %d\n",
					name, vc, outp);
	}
	pframe->connID = new_vci_tab[vc];
	return sucs[outp]->rec(pframe, shands[outp]);
  }
}

/*
*	Signalling:
*	generate entries in output_tab[] and new_vci_tab[]
*/
char	*demux::special(
	specmsg	*msg,
	char	*)
{
	int		outp, vc;
	WriteVciTabMsg	*sig;
	static	char	vci_err[] = "available VCI range from 0 to %d";
	static	char	nout_err[] = "output numbers range from 1 to %d";
	char		*err;

	if (msg->type != WriteVciTabType)
		return "wrong type of special message";

	sig = (WriteVciTabMsg *) msg;

	if ((vc = sig->old_vci) < 0 || vc >= nvci)
	{	CHECK(err = new char[strlen(vci_err) + 50]);
		sprintf(err, vci_err, nvci - 1);
		return err;
	}

	if ((outp = sig->outp - 1) < 0 || outp >= noutp)
	{	CHECK(err = new char[strlen(nout_err) + 50]);
		sprintf(err, nout_err, noutp);
		return err;
	}

	outp_tab[vc] = outp;
	new_vci_tab[vc] = sig->new_vci;

	return NULL;
}

// was previously in 	demux.h

int demux::act(void)
{
	  int i;
	  CHECK(outp_tab = new int[nvci]);
	  for (i = 0; i < nvci; ++i)
	    outp_tab[i] = NILVCI;
	  CHECK(new_vci_tab = new int[nvci]);
	  return 0;
}
