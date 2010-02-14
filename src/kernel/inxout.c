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
* History:
* Oct 2, 1997  extended syntax: multiple ranges
*    for desrciption of outputs. All new things put
*    into the new method inxout::scanOutputRanges()
*     M. Baumann
*
*************************************************************************/

/*
* class inxout:
* generic class with arbitrarily many inputs and outputs
*
* Services:
* parsing of output names from the input file,
* implementation of the connect() method
*
* interface functions (to be called from an object constructor):
*
* void output(char *keyw, int idx);
*  Read an output name and create an output.
*  If keyw != NULL, then the given keyword is expected.
*  The index in the sucs[] and shands[] vectors is idx.
* void outputs(char *keyw, int nout, int displ);
*  Read a bunch of outputs. Example:
*  Two forms are possible to specify nout = 3 outputs in the input text:
*   OUT=sink[1], sink[2], sink[3] // all outputs are explicitely given
*   OUT=(i: sink[i]) // the template is evaluated for i = 1 to 3,
*      // i must be a declared variable
*   Third version: list of ranges, a range also may consist of a single number.
*   OUT=(i = 1 to 3: he[i], 4: blabla->Start, 5 to 10: he[i-1])
*  The index of an output in the sucs[] and shands[] fields is given by the
*  sum of output number and displ (output number range: 1 ... nout).
*
* more details: see manual
*/


#include "inxout.h"

inxout::inxout()
{
  sucs = NULL;
  shands = NULL;
}
inxout::~inxout()
{
  if (sucs)
    delete[] sucs;
  if (shands)
    delete[] shands;
}
// We need the following dummies in order to avoid link errors
void	inxout::output(char *keyw, int index){
   fprintf(stderr,"called inxout::output() - shouldn't happen!\n");
   exit(1);
}
void	inxout::outputs(char *keyw, int	nout, int displ){
   fprintf(stderr,"called inxout::outputs() - shouldn't happen!\n");
   exit(1);
}
void	inxout::scanOutputRanges(int nout, int displ, tok_typ *pv){
   fprintf(stderr,"called inxout::scanOutputRanges() - shouldn't happen!\n");
   exit(1);
}

void inxout::set_nout(int nout)
{
  this->nout = nout;
  CHECK(sucs = new root*[nout]);
  CHECK(shands = new int[nout]);
}

void inxout::add_output(int index, root *sucs, int shand)
{
  this->sucs[index-1] = sucs;
  this->shands[index-1] = shand;
}

