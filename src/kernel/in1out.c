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
**************************************************************************
*
* Module author:  Matthias Baumann, TUD
* Creation:  1996
*
*************************************************************************/

/*
* class in1out:
* generic class with arbitrarily many inputs and one output
*
* Services:
* parsing of the output from the input file
* implementation of the connect() method (establish connection to the successor)
*
* interface (to be called from an object constructor):
*
* void output(char *keyw);
*  If keyw != NULL, then the given keyword is expected. The output name is read,
*  an output is created.
*
* more details: see manual
*/


#include "in1out.h"

// We need these dummies to avoid link errors in sources that haven't been adopted yet.
void in1out::output(char *keyw)
{
  fprintf(stderr,"called in1out::output() - shouldn't happen!\n");
  exit(1);
}


in1out::in1out(void)
{
  suc = NULL;
}

// set an output
void in1out::set_output(root *suc, int shand)
{
  this->suc = suc;
  this->shand = shand;
}
