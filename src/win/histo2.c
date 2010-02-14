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
* Creation:  July 1996
*
* History
* Oct 29, 1997  VAL now also can be an element of a two-dimensional
*    array. exp_typ::calcIdx() used. Code added in the
*    switch in init().
*     Matthias Baumann
*
*************************************************************************/

/*
* A simple graphical measurement display:
*  - produces the distribution of a value and displays it
*
* Histo2 hist: 
*   VAL=mux->QLen,   // the value of which to produce the distrib.
*                    // the object must be defined and is asked by the
*                    // export() method
*   {TITLE="hello world!",} // if omitted, the title equals the object name
*   WIN=(xpos,ypos,width,height), // window position and size
*   NVALS=100       // value range to be displayed (0 ... 99)
*   {,MAXFREQ=20}   // normalisation, MAXFREQ corresponds to the
*                   // window height
*                   // If omitted: autoscale to 1.25 * largest frequency
*   {,DELTA=100}   // interval between two samples (time slots)
*                  // default: 1
*   {, UPDATE=10};   // with every UPDATE-th sample, the window is redrawn.
*                    // default: 1
*
* Commands:
*  histo->Dist(i)
*   // returns the histogramm counter i
*  histo->ResDist
*   // resets all counters
*/

#include "histo.h"
#ifdef __cplusplus
extern "C"
{
#endif
 #include "iup.h"
#ifdef __cplusplus
}
#endif

#include "yats.h"

histo2::histo2()
{
}

histo2::~histo2()
{
  dprintf("Destructor of histo2\n");
  delete[] int_val_ptr;
}

// Init by Lua.
int histo2::act(void)
{
  exp_typ msg;
  int i, j;
  char *errm;
  if (getexp(&msg) == FALSE){
    // TODO: error or return value ?
#if 0
    lua_pushstring(WL, "Export variable not defined.");
    lua_pushstring(WL, exp_varname);
    return 2;
#else
    lua_pushstring(WL, "Export variable not defined.");
    lua_error(WL);
    return 0;
#endif
  } else {
    switch (msg.addrtype) {
    case exp_typ::IntScalar:
      the_val_ptr = msg.pint;
      break;
    case exp_typ::IntArray1:
      i = exp_idx;
      if ((errm = msg.calcIdx( &i, 0)) != NULL){
	lua_pushstring(WL, errm);
	lua_error(WL);
      }
      the_val_ptr = &msg.pint[i];
      break;
    case exp_typ::IntArray2:
      i = exp_idx;
      if ((errm = msg.calcIdx( &i, 0)) != NULL){
	lua_pushstring(WL, errm);
	lua_error(WL);
      }
      j = exp_idx2;
      if ((errm = msg.calcIdx( &j, 1)) != NULL){
	lua_pushstring(WL, errm);
	lua_error(WL);
      }
      the_val_ptr = &msg.ppint[i][j];
      break;
    default:
#if 0
      lua_pushstring("Export variable not defined.");
      lua_pushstring(WL, exp_varname);
      return 2;
#else
      lua_pushstring(WL, "Export variable not defined.");
      lua_error(WL);
      return 0;
#endif
    }
  }
  indicdispl = 0;
  update_cnt = 0;
  CHECK(int_val_ptr = new int[nvals]);
  for (i = 0; i < nvals; ++i)
    int_val_ptr[i] = 0;
  over_under = 0;
  CHECK(xsegs = new XSegment[nvals]);
  
  // create the window, dimensions can be changed
  update_cnt = update - 1;
  late(&std_evt);
  return 0;
}
/*
* Time DELTA expired:
* read the values.
* redraw window, if necessary
*/
void histo2::late(event *)
{
   int i;
   //  printf("histo2: late %x\n", (int) the_val_ptr);
   i = *the_val_ptr;
   if (i < 0 || i >= nvals)
      ++over_under;
   else
      ++int_val_ptr[i];

   //  printf("histo2: check for draw: update = %d\n", update);
   if ( ++update_cnt >= update) {
      update_cnt = 0;
      // DEL: X.sched(); // calling X every times would slow down the simulator
      // (usually, DELTA is 1)
      IupLoopStep();
      if (canvas_mapped) {
	 drawWin(1,1);
      }
   }

   // register for next sample values
   alarml( &std_evt, delta);
   //  printf("histo2: finished late: %d\n", delta);
}

/*
* export the distribution
*/
int histo2::export(exp_typ *msg)
{
   return baseclass::export(msg) ||
          intArray1(msg, "Dist", int_val_ptr, nvals, 0);
}

/*
* Command:
*  hist->ResDist
* resets all counters
*/
void histo2::resDist(void)
{
   int i;

   for (i = 0; i < nvals; ++i)
     int_val_ptr[i] = 0;
}
