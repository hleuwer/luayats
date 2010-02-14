/*************************************************************************
*
*  YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*    Copyright (C) 1995-1997 Chair for Telecommunications
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
#ifndef _INXOUT_H_
#define _INXOUT_H_

#include "ino.h"

//tolua_begin
class inxout: public ino
{
  typedef ino baseclass;

public:
  inxout();
  ~inxout();
  void set_nout(int nout);
  void add_output(int index, root *sucs, int shand);
  root* get_suc(int index){ return this->sucs[index - 1];}
  int get_shand(int index){ return this->shands[index - 1];}
  //tolua_end
  void output(char *, int);
  void outputs(char *, int, int);
  void scanOutputRanges(int, int, tok_typ *);
  //tolua_begin
  int nout;
  //tolua_end
  root **sucs;
  int *shands;
}; //tolua_export

#endif // _INXOUT_H_
