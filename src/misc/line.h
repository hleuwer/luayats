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
#ifndef _LINE_H_
#define _LINE_H_

#include "in1out.h"
#include "queue.h"

//tolua_begin
class line: public in1out
{
   typedef in1out baseclass;

 public:
   line();
   ~line();
   int act(void);
   tim_typ delay;  // delay in slots
//tolua_end
   
   void early(event *);
   // REC is a macro normally expanding to rec (for debugging)
   rec_typ REC(data *, int); 
   void restim(void);

   // we need a seperate struct, sinc we can not overwrite the
   // time member of the queued data items (we need a time member
   // for the correct delay).
   // The struct is dervied from data, so we can use the normal uqueue
   
   class lineItem: public data 
      {
      public:
	 data *item;  // the data item to be queued
      };
   
   static lineItem *lineItemPool; // Our own pool of lineItems.
   // LineItmes are allocated via standard new,
   // but never given back (are never deleted).
   // The pool is shared by all lines.
   

   uqueue q;  // system queue
};  //tolua_export


#endif // _LINE_H_
