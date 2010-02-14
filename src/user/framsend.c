/*************************************************************************
*
*     YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997 Chair for Telecommunications
*           Dresden University of Technology
*           D-01062 Dresden
*           Germany
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
*  Module author:    Torsten Mueller, TU-Dresden
*  Creation:         27.06.1997
*  Last Modified:
*
*************************************************************************/

/*a description can be found in framsend.txt */

#include "framsend.h"

CONSTRUCTOR(Framesend, framesend);
USERCLASS("FrameSend", Framesend);

//////////////////////////////////////////////////////////////////////////
//  initialization
//////////////////////////////////////////////////////////////////////////
void   framesend::init(void)
{

   // read parameters
   skip(CLASS);
   name = read_id(NULL);
   skip(':');


   output("OUTCORE", OutCore);         // output CORE-DATA
   skip(',');
   output("OUTDATA", OutData);         // output DATA

  stdinp();
  
  counter = 0;

}

//////////////////////////////////////////////////////////////////////////
//  receiving a frame
//////////////////////////////////////////////////////////////////////////
rec_typ   framesend::REC(data *pd, int key)
{
   data *newframe;
   
   if(++counter == 0)
      errm1s("%s: overflow of departs", name);

   newframe = pd->clone();
   
   if(newframe == NULL)
      errm1s("%s: creation of new frame not successful", name);

   // send old data to core
   sucs[SucCore]->rec(pd, shands[SucCore]);
   
   // send new (the same) data to the next layer
   return sucs[SucData]->rec(newframe, shands[SucData]);

}

