/*************************************************************************
*
*  YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997 Chair for Telecommunications
*    Dresden University of Technology
*    D-01062 Dresden
*    Germany
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
#ifndef _MUXDIST_H
#define _MUXDIST_H

#include "mux.h"
//tolua_begin
class muxDist: public mux {
  typedef mux baseclass;
public:
  muxDist(void);
  ~muxDist(void);
  void setTable(void *tab){table = (tim_typ*)tab;}
  void *getTable(void){return table;}
  int  serving; // TRUE: server busy
  //tolua_end
  
  void early(event *);
  void late(event *);

  tim_typ  *table;  // serving time distribution
}; //tolua_export

#endif // _MUXDIST_H
