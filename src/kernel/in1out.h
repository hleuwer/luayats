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
#ifndef _IN1OUT_H_
#define _IN1OUT_H_

#include "ino.h"

//tolua_begin
class in1out: public ino
{
  typedef ino baseclass;
public:
  in1out();
  ~in1out(){};
  void output(char *);
  root *get_suc(void){ return this->suc;}
  int get_shand(void){ return this->shand;}
  void set_output(root *suc, int shand);
  root *suc;
  int shand;
};
//tolua_end
#endif // _IN1OUT_H_
