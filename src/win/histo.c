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
* Oct 29, 1997  VAL now also can be a row of a two-dimensional
*    array. (code added in the switch in init())
*     Matthias Baumann
*
*************************************************************************/

/*
* A simple graphical measurement display:
*  - Histogramm of a distribution
*
* Histogram hist: 
* VALS=ms->IAT,   // the array of values to be displayed.
*                 // the object must be defined and is asked by the
*                 // export() method
* {TITLE="hello world!",}       // if omitted, the title equals the object name
* WIN=(xpos,ypos,width,height), // window position and size
* {MAXFREQ=20,}   // normalisation, MAXFREQ corresponds to the
*                 // window height
*                 // If omitted: autoscale to 1.25 * largest frequency
* DELTA=100;      // interval between two samples (time slots)
*                // with every sample, the window is redrawn.
*/

#include "yats.h"
#include "histo.h"
#ifdef __cplusplus
extern "C"
{
#endif
#include "iup.h"
#include "cd/cd.h"
#include "cd/wd.h"
#ifdef __cplusplus
}
#endif

//
// Ctor
//
histo::histo()
{
}

histo::~histo()
{
	delete[] xsegs;
}

//
// Init by Lua.
//
int histo::act(void)
{
  exp_typ msg;
  int i;
  char *errm;
  if (getexp(&msg) == FALSE){
    lua_pushstring(WL, "Export variable not defined.");
    lua_error(WL);
    return 0;
  } else {
    switch (msg.addrtype) {
    case exp_typ::IntArray1:
      int_val_ptr = msg.pint;
      debug("%s: 1 int_val_ptr=%p", name, int_val_ptr);
      nvals = msg.dimensions[0];
      indicdispl = msg.displacements[0];
      break;
    case exp_typ::IntArray2:
      i = exp_idx;
      if ((errm = msg.calcIdx( &i, 0)) != NULL){
	lua_pushstring(WL, errm);
	lua_error(WL);
      }
      int_val_ptr = msg.ppint[i];
      debug("%s: 2 int_val_ptr=%p",name, int_val_ptr);
      nvals = msg.dimensions[1];
      indicdispl = msg.displacements[1];
      break;
    default:
      lua_pushstring(WL, "Export variable not defined.");
      lua_error(WL);
      return 0;
    }
  }
  CHECK(xsegs = new XSegment[nvals]);

  late(&std_evt);
  return 0;
}

//
// Time DELTA expired: read the values, redraw window
//
void histo::late(event *)
{
   // DEL: X.sched();
   if (canvas_mapped){
     //     cdActivate(canvas);
      drawWin(1,1);
   }
   // register for next sample values
   alarml( &std_evt, delta);
   IupLoopStep();
}

//
// Draw in World coordinates
//
void histo::drawWin(int activate, int clear, int flush)
{
   int i;
   double sumvals;
   double xw, yw;
   
   if (canvas_mapped == false)
      return;
   
   if (int_val_ptr == NULL)
      errm0("internal error: fatal in histo::drawWin()");
   
   if (activate)
      cdCanvasActivate(canvas);

   // DOIT: draw a rectangle and fill it with background color
   if (clear)
      cdCanvasClear(canvas);
   
   sumvals = 0.0;
   for (i = 0; i < nvals; ++i)
      sumvals += (double) int_val_ptr[i];
   if (sumvals == 0.0)
      sumvals = 1.0;
   
   // Autoscale
   if (autoscale) {
      maxfreq = 0.0;
      for (i = 0; i < nvals; ++i)
	 if (int_val_ptr[i] / sumvals > maxfreq)
	    maxfreq = int_val_ptr[i] / sumvals;
      // Let's fill the window only partly
      maxfreq *= AUTOSCALE;
      if (maxfreq <= 0.0)
	 maxfreq = 1.0;
      wdCanvasWindow(canvas, 0, nvals, 0, sumvals*maxfreq);
   } else {
      wdCanvasWindow(canvas, 0, nvals, 0, sumvals);
   }
   wdCanvasCanvas2World(canvas, 1, 1, &xw, &yw);

   char str[50];
   sprintf(str, "min x: %d", 0 + indicdispl);
   
   cdCanvasWriteMode(canvas, textoper);
   cdCanvasFont(canvas, texttypeface, textstyle, textsize);
   cdCanvasForeground(canvas, (unsigned long int) textcolor);
  
   wdCanvasText(canvas, xw * 5 * sx, yw * (height - 15) * sy, str);
  
   sprintf(str, "max x: %d", nvals - 1 + indicdispl);
   wdCanvasText(canvas, xw * 5 * sx, yw * (height - 30) * sy, str);
   sprintf(str, "full y: %g", maxfreq);
   wdCanvasText(canvas, xw * 5 * sx, yw * (height - 45) * sy, str);
  
   cdCanvasWriteMode(canvas, drawoper);
   cdCanvasLineStyle(canvas, drawstyle);
   cdCanvasLineWidth(canvas, drawwidth);
   draw_values(int_val_ptr, drawcolor, nvals, usepoly, polytype, polystep, flush);
}

//
// Store data to file
//
int  histo::store2file(char *snapFile, char *delim)
{
  FILE *fp;
  int i;
  double sumvals;
  char mydelim[16]="\t";
  
  if (!delim) 
    delim = mydelim;
  
  if ((fp = fopen(snapFile, "w+")) == NULL) {
    return -1;
  }
  
  fprintf(fp, "# LUAYATS:\n");
  fprintf(fp, "#\n");
  fprintf(fp, "# Snapshot of Histogram object `%s', TITLE='%s'\n", name, title);
  fprintf(fp, "#\n");
  fprintf(fp, "# SimTime = %d\n", SimTime);
  fprintf(fp, "#\n");
  fprintf(fp, "# first column: value\n");
  fprintf(fp, "# second column: absolute frequency\n");
  fprintf(fp, "# third column: relative frequency\n#\n");
  fprintf(fp, "value%sabsolute_frequency%srelative_frequency\n", delim, delim);
  sumvals = 0.0;
  for (i = 0; i < nvals; ++i)
    sumvals += (double) int_val_ptr[i];
  if (sumvals == 0.0)
    sumvals = 1.0;
  
  for (i = 0; i < nvals; ++i)
    fprintf(fp, "%d%s%d%s%e\n", i + indicdispl, delim,
	    int_val_ptr[i+indicdispl], delim, int_val_ptr[i+indicdispl] / sumvals);
  
  fclose(fp);
  return 0;
}
