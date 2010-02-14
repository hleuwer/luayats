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
#ifndef _MUXAF_H_
#define _MUXAF_H_

#include "muxdf.h"

//tolua_begin
class muxAF: public muxDF {
  typedef muxDF baseclass;

public:
  muxAF(void);
  ~muxAF(void);
  //  int act(void);
  //tolua_end
  void early(event *);
  void late(event *);
  //tolua_begin
  int  q_hi, q_lo;  // see comment to early()
  int  served;   // see comment to early()
};
//tolua_end
#endif // _MUXAF_H_
