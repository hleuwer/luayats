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
* History:
* July 4, 1997:  - X.Sched() is only called when redrawing the
*     window, see meter::late()
*    Matthias Baumann
*
* Oct 29, 1997 Value now can be an element of a two-dimensional array.
*   Code added in the switch in init(). exp_typ::calcIdx() used.
*    Matthias Baumann
*
*************************************************************************/


/*
A simple graphical measurement display:
- sliding time history of a value
 
The meter is updated during the late slot phase. Thus, variables manipulated during the late phase can not be displayed exactly with a resolution of 1 time slot (undefined processing
order between variable manipulation and display).
 
Meter m: VAL=ms->Count, // the value to be displayed.
        // the object must be defined and is asked by the
        // export() method
 {TITLE="hello world!",}  // if omitted, the title equals the object name
 WIN=(xpos,ypos,width,height), // window position and size
 MODE={AbsMode | DiffMode},  // AbsMode: display the sample value itself
                            // DiffMode: display differences between   
           // subsequent samples
 {DODRAWING=0|1|2|3,} // 0:only value; 1:only drawing; 2: value and drawing
        // 3:value (only y) and drawing (default 2)
 {DRAWMODE=<bits>,} // can be given as hex number, e.g. 0x0F
        // bit0-drawing,
        // bit1 -y-lastvalue,
        // bit2 -y-lastvaluedesription
        // bit3 -y-fullvalue
        // bit4 -x-value
        // default - all bits set 
 {LINEMODE=0|1,}    // 0: points are drawn, 1: lines are drawn; default 1
   {LINCOLOR=<int>,}    // 1:black (default) 2:red 3:green 4:yellow
 NVALS=100,   // # of sample values displayed
 MAXVAL=20,   // normalisation, MAXVAL corresponds to the window height
 DELTA=100   // interval between two samples (time slots)
 {,UPDATE=5};  // the widow is only updated with every 5th sample
 */

#include "winobj.h"
#include "meter.h"
extern "C"
{
#include "lua.h"
#include "yats.h"
#include "cd/cd.h"
#include "cd/wd.h"
}


meter::meter()
{
}


meter::~meter()
{
  delete[] vals;
  delete[] xsegs;
}


/*
* read the definition statement
*/

int meter::act(void)
{
   int i, j;
   exp_typ msg;
   char *errm;
   int_val_ptr = NULL;
   double_val_ptr = NULL;
   if (getexp(&msg) == FALSE) {
      // TODO: error or return value ?
      lua_pushstring(WL, "Export variable not defined.");
      lua_error(WL);
      return 0;
   } else {
     dprintf("evaluate msg.addrtype\n");
     switch (msg.addrtype) {
      case exp_typ::IntScalar:
         int_val_ptr = msg.pint;
         break;
      case exp_typ::DoubleScalar:
         double_val_ptr = msg.pdbl;
         break;
      case exp_typ::IntArray1:
      case exp_typ::DoubleArray1:
         i = exp_idx;
         if ((errm = msg.calcIdx( &i, 0)) != NULL) {
            lua_pushstring(WL, errm);
            lua_error(WL);
         }
         if (msg.addrtype == exp_typ::IntArray1)
            int_val_ptr = &msg.pint[i];
         else
            double_val_ptr = &msg.pdbl[i];
         break;
      case exp_typ::IntArray2:
      case exp_typ::DoubleArray2:
         i = exp_idx;
         if ((errm = msg.calcIdx( &i, 0)) != NULL) {
            lua_pushstring(WL, errm);
            lua_error(WL);
         }
         j = exp_idx2;
         if ((errm = msg.calcIdx( &j, 1)) != NULL) {
            lua_pushstring(WL, errm);
            lua_error(WL);
         }
         if (msg.addrtype == exp_typ::IntArray2)
            int_val_ptr = &msg.ppint[i][j];
         else
            double_val_ptr = &msg.ppdbl[i][j];
         break;
      default:
         lua_pushstring(WL, "Export variable not defined.");
         lua_error(WL);
         return 0;
      }
   }
   update_cnt = 0;
   CHECK(vals = new double[nvals]);
   first_val = vals;
   last_val = vals + nvals - 1;
   for (i = 0; i < nvals; ++i)
      vals[i] = 0;
   CHECK(xsegs = new XSegment[nvals]);

   the_old_value = 0.0;
   the_value = 0.0;
   lastScanTime = 0;

   // create the window, dimensions can be changed
   //createWin(ExposureMask | ButtonPressMask | StructureNotifyMask, FALSE);

   // first draw
   update_cnt = update - 1;
   late( &std_evt);
   //   printf("done\n");
   return 0;
}

/*
* Time DELTA expired:
* read the value, redraw window if necessary
*/
void meter::late(event *)
{

   // X.sched(); // shifted down: only called when window is redrawn
   // July 4, MBau

   // rotate value buffer pointers
   last_val = first_val++;
   if (first_val == vals + nvals)
      first_val = vals;

   // read new value
   if (int_val_ptr != NULL)
      the_value = (double) * int_val_ptr;
   else if (double_val_ptr != NULL)
      the_value = *double_val_ptr;
   lastScanTime = SimTime;

   // what to display depends on the mode
   switch (this->display_mode) {
   case AbsMode:
      *last_val = the_value;
      the_old_value = the_value;
      break;
   case DiffMode:
      *last_val = the_value - the_old_value;
      the_old_value = the_value;
      break;
   default:
      errm1s("%s: internal error: invalid display_mode in meter::late()", name);
   }

   // update window?
   if ( ++update_cnt == update) {
      update_cnt = 0;
      //    X.sched();
      IupLoopStep();
      if (canvas_mapped)
	 drawWin(1,1);
   }

   // register for next sample value
   alarml( &std_evt, delta);

}

// draw lines and digits:
// perhaps, it is inefficient to clear the whole window and redraw it completely.
// but for the moment, it works ...
void meter::drawWin(int activate, int clear, int flush)
{
  char str[50];
  double xw, yw;

  if (canvas_mapped == false){
    return;
  }
  if (activate)
     cdCanvasActivate(canvas);

  if (clear)
     cdCanvasClear(canvas);
  
  cdCanvasWriteMode(canvas, textoper);
  cdCanvasFont(canvas, texttypeface, textstyle, textsize);
  cdCanvasForeground(canvas, (unsigned long int) textcolor);
  
  wdCanvasWindow(canvas, 0, nvals, 0, maxval);
  wdCanvasCanvas2World(canvas, 1, 1, &xw, &yw);

  // draw first the x and then overwrite (if too few space) with y-values
  if (drawmode & 0x10) {
     // the x-value, description, and full value
     sprintf(str, "last x: slot %u", lastScanTime);
     // cdCanvasText(canvas, width - 120, height - 15, str);
     wdCanvasText(canvas, xw * (vp.xmax - 120 * sx), yw * (vp.ymax - 15 * sy), str);
     sprintf(str, "full x: %u slots", (unsigned int) nvals * delta);
     // cdCanvasText(canvas, width - 120, height - 30, str);
     wdCanvasText(canvas, xw * (vp.xmax - 120 * sx), yw * (vp.ymax - 30 * sy), str);
  }
  
  if ((drawmode & 0x02) && !(drawmode &0x04)) {
     // the y-value
     sprintf(str, "%g", *last_val);
     //     cdCanvasText(canvas, 5, height - 15, str);
     wdCanvasText(canvas, xw * 5 * sx, yw * (vp.ymax - 15 * sy), str);
  }
  
  if (drawmode & 0x04) {
     // the y-value and description (overwrites)
     sprintf(str, "last y: %g", *last_val);
     // cdCanvasText(canvas, 5, height - 15, str);
     wdCanvasText(canvas, xw * 5 * sx, yw * (vp.ymax - 15 * sy), str);
  }
  
  if (drawmode & 0x08) {
     // the y-value and description
     sprintf(str, "full y: %g", maxval);
     // cdCanvasText(canvas, 5, height - 30, str);
     wdCanvasText(canvas, xw * 5 * sx, yw * (vp.ymax - 30 * sy), str);
  }
  
  if (drawmode & 0x01) {
     // wdCanvasWindow(canvas, 0, nvals, 0, maxval);
     // wdCanvasViewport(canvas, vp.xmin, vp.xmax, vp.ymin, vp.ymax);
     cdCanvasWriteMode(canvas, drawoper);
     cdCanvasLineStyle(canvas, drawstyle);
     cdCanvasLineWidth(canvas, drawwidth);
     
     switch (linecolor) {
     case 2:
	draw_values(vals, first_val, (void *)CD_RED, nvals, usepoly, polytype, polystep, flush);
	break;
     case 3:
	draw_values(vals, first_val, (void *) CD_GREEN, nvals, usepoly, polytype, polystep, flush);
	break;
     case 4:
	draw_values(vals, first_val, (void *) CD_YELLOW, nvals, usepoly, polytype, polystep, flush);
	break;
     default:
	draw_values(vals, first_val, drawcolor, nvals, usepoly, polytype, polystep, flush);
	break;
     }
  }
}

int meter::store2file( char *snapFile, char *delim)
{
   int i;
   FILE *fp;
   double *pv;
   char mydelim[16]="\t";

   if (!delim) 
     delim = mydelim;

   if ((fp = fopen(snapFile, "w+")) == NULL) 
     return -1;

   fprintf(fp, "# LUAYATS:\n");
   fprintf(fp, "#\n");
   fprintf(fp, "# Snapshot of Meter object `%s', TITLE='%s'\n#\n", name, title);
   fprintf(fp, "#\n");
   fprintf(fp, "# first column: SimTime\n");
   fprintf(fp, "# second column: value\n");
   fprintf(fp, "#\n");
   fprintf(fp, "SimTime%svalue\n", delim);
   pv = first_val;
   i = nvals - 1;
   while (i >= 0) {
     if (lastScanTime >= delta * i)
       fprintf(fp, "%d%s%e\n", lastScanTime - delta * i, delim, *pv);
     --i;
     if ( ++pv >= vals + nvals)
       pv = vals;
   }

   fclose(fp);
   return 0;
}
