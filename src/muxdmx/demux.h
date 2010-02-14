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
#ifndef	_DEMUX_H_
#define	_DEMUX_H_

#include "inxout.h"

//tolua_begin
typedef enum {
  rec_cell=0, 
  rec_frame=1
} rec_type_t; // receiving data-type

class	demux:	public	inxout {
typedef	inxout	baseclass;

public:	
	demux();
	~demux();
	char	*special(specmsg *, char *);

	int getOutpVCI(int vc) {return this->outp_tab[vc];}
	void setOutpVCI(int vc, int outp) {this->outp_tab[vc] = outp;}
	int getNewVCI(int vc) {return this->new_vci_tab[vc];}
	void  setNewVCI(int vc, int vci) {this->new_vci_tab[vc] = vci;}
	int act(void);
	void setRecTyp(int typ) {this->rec_type = (rec_type_t) typ;}
	int getRecTyp(void) {return (int) this->rec_type;}


	int		noutp;
	int		nvci;			/* # of VCIs */
//tolua_end

	rec_typ REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
	rec_type_t rec_type;

	int		*outp_tab;		/* which VCI to which output? */
	int		*new_vci_tab;		/* new VCI */
};  //tolua_export

#endif	// _DEMUX_H_
