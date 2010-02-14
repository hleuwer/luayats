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
#ifndef _HISTO_H
#define _HISTO_H

#include "winobj.h"

extern "C" {
 #include "lua.h"
}

extern lua_State *L;

//tolua_begin
class histo: public winobj
{
      typedef winobj baseclass;
   public:
      histo();
      ~histo();		

      double maxfreq;
      int autoscale;
      int act(void);
      void drawWin(int activate = 1, int clear = 1, int flush = 1);
      int store2file(char *, char *);
      tim_typ delta;
      int nvals;
      int getVal(int index){return int_val_ptr[index];}
//tolua_end                  

      void late(event *);
      void connect(void) {}

      XSegment *xsegs;

      int *int_val_ptr;

      int indicdispl;
};  //tolua_export

#define AUTOSCALE (1.25)

//tolua_begin
class histo2: public histo
{
      typedef histo baseclass;
   public:
      histo2();
      ~histo2();

      int act(void);      
      int update;
      void resDist(void);
//tolua_end

      void late(event *);
      int export(exp_typ *);

      int update_cnt;
      int *the_val_ptr;

      int over_under;
};  //tolua_export
#endif // _HISTO_H
