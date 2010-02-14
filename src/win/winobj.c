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
**************************************************************************
*
* Module author:  Matthias Baumann, TUD
* Creation:  July 1996
*
* History: July 4, 1997:
*   - XClass::allocColor(): more "soft" reaction on
*     unavailable colours, simply set to white
*     (instead of terminating)
*    Matthias Baumann
*
*************************************************************************/

/*
* - a basic class for X11 display objects: X11obj
* - a class for interfacing the X system: XClass (has only one instance, X)
*  (also provides methods to read and write a simple object location file)
* - classes for
*  + a Yes-No-window (YesNoWin)
*  + an info window (InfoWin)
*  + a text input window (TextWin)
*/
#include "winobj.h"
extern "C"
{
}
#include "lua.h"
#include "cd/cd.h"
#include "cd/wd.h"
#include "cd/cdnative.h"
#include "cd/cdiup.h"
#include "cd/cdlua.h"
#include "iup/iup.h"


/*
* X11obj class
*/

// Constructor.
winobj::winobj()
{
  canvas_mapped = false;
  sx = 1;
  sy = 1;
}

winobj::~winobj()
{
  unmap();
}

// Unmap the drawing canvas.
void winobj::unmap()
{
  if (canvas_mapped == true){
    debug("Unmapping canvas from '%s'", this->name);
    if (cvtype == CDTYPE_DBUFFER)
      cdKillCanvas(canvas);
    if (cvtype == CDTYPE_SERVERIMG){
      cdKillCanvas(canvas);
      cdKillImage(simg);
    }
    cdKillCanvas(bcanvas);
    canvas_mapped = false;
  }
  debug("Unmapping canvas from '%s' done!", this->name);
}

// Get value pointers to display from export expression.
int winobj::getexp(exp_typ *msg)
{
  msg->varname = this->exp_varname;
  msg->ninds = 0;
  return this->exp_obj->export(msg);
}

void winobj::set_textsize(int fontsize){
   textsize = fontsize;
   switch (fontsize){
   case 2:
     texttypeface = (char*) "Helvetica";
      textstyle = CD_BOLD;
      textsize = CD_LARGE;
      break;
   case 0:
      texttypeface = (char*) "Helvetica";
      textstyle = CD_BOLD;
      textsize = CD_SMALL;
      break;
   case 1:
   default:
      texttypeface = (char*) "Helvetica";
      textstyle = CD_PLAIN;
      textsize = CD_STANDARD;
      break;
   }
   cdCanvasFont(canvas, texttypeface, textstyle, textsize);
}

// Map the drawing canvas created under Lua to this object.
// void *winobj::map(void *iupcanvas, void *wid,
//                   int width, int height, int xpos, int ypos, 
// 		  int wrtype = CD_HELVETICA,
// 		  int wrface = CD_BOLD,
// 		  int wrsize = 12,
// 		  int wrcol = CD_BLACK, 
// 		  int wrmode = CD_REPLACE, 
// 		  int fgcol = CD_BLACK,
// 		  int fgmode = CD_REPLACE, 
// 		  int fgstyle = CD_CONTINUOUS,
// 		  int fgwidth = 1,
// 		  int bgcol = CD_WHITE, 
// 		  int opacity = CD_TRANSPARENT 
// 		  )
cdCanvas *winobj::map(void *iupcanvas, void *wid,
                  int width, int height, int xpos, int ypos, 
		  const char *wrtypeface,
		  int wrstyle,
		  int wrsize,
		  void *wrcol,
		  int wrmode,
		  void *fgcol,
		  int fgmode,
		  int fgstyle,
		  int fgwidth,
		  void *bgcol,
		  int opacity,
		  int usepoly,
		  int polytype,
		  int polystep,
		  int canvastype
		  )
{
   cvtype =canvastype;
   // Our displaying canvas
   debug("Mapping canvas to '%s'", this->name);
#if 1
   if (0){
   } else if (wid){ 
      bcanvas = cdCreateCanvas(CD_NATIVEWINDOW, wid);
      dprintf("2 bcanvas: %p\n", (void *)bcanvas);
   } else if (iupcanvas){
      bcanvas = cdCreateCanvas(CD_IUP, iupcanvas);
      dprintf("1 iupcanvas: %p bcanvas: %p\n", (void*) iupcanvas, (void *)bcanvas);
   } else
      errm1s("%s: Missing `canvas' or `wid'.", this->name);
#else
   canvas = (cdCanvas*) iupcanvas;
#endif   
   if (cvtype == CDTYPE_DBUFFER) {
      // Background Drawing with double buffer
      canvas = cdCreateCanvas(CD_DBUFFER, bcanvas);
   } else if (cvtype == CDTYPE_SERVERIMG){
      // Background Drawing with server image
      simg = cdCanvasCreateImage(bcanvas, width, height);
      canvas = cdCreateCanvas(CD_IMAGE, simg);
   } else {
      canvas = bcanvas;
   }
   if (!canvas){
      return NULL;
   }
   cdCanvasActivate(canvas);
   drawoper = fgmode;
   drawcolor = fgcol;
   drawstyle = fgstyle;
   drawwidth = fgwidth;
   textoper = wrmode;
   textcolor = wrcol;
   textsize = wrsize;
   textstyle = wrstyle;
   texttypeface = (char *) wrtypeface;
   bgcolor = bgcol;
   bgopacity = opacity;
   this->usepoly = usepoly;
   this->polytype = polytype;
   this->polystep = polystep;
   cdCanvasBackground(canvas, (unsigned long int) bgcolor);
   cdCanvasForeground(canvas, (unsigned long int) drawcolor);
   cdCanvasWriteMode(canvas, drawoper);
   cdCanvasBackOpacity(canvas, bgopacity);
   cdCanvasFont(canvas, texttypeface, textstyle, textsize);
   
   // cdCanvasClear(canvas);
   canvas_mapped = true;
   this->width = width;
   this->height = height;
   this->xpos = xpos;
   this->ypos = ypos;
   this->vp.xmin = 0;
   this->vp.xmax = width;
   this->vp.ymin = 0;
   this->vp.ymax = height;
   return canvas;
}

//
// Drawing for histogram: integer values
//
void winobj::draw_values(int *values, void *color, int nvals, int usepoly, 
			 int polytype, int polystep, int flush)
{
   int i;
   int xl = 0;
   int yl= 0;
   int y = 0;

   cdCanvasForeground(canvas, (unsigned long int) color);

   if (usepoly){
      if (polytype == CD_BEZIER)
	 cdCanvasBegin(canvas, CD_BEZIER);
      else
	 cdCanvasBegin(canvas, CD_OPEN_LINES);
      //      wdCanvasVertex(canvas, 0, 1);
   }
   for (i = 0; i < nvals; ++i) {
      y = *(values + i);
      if (usepoly){
	 if (polystep == 1){
	    wdCanvasVertex(canvas, i, yl);
	 } else if (polystep == -1) {
	    wdCanvasVertex(canvas, xl, y);
	 } 
	 wdCanvasVertex(canvas, i, y);
      } else {
#if 0
	 wdCanvasLine(canvas, i, 0, i, y);
#else
	 wdCanvasLine(canvas,xl, yl, i, y);
#endif
      }
    xl = i;
    yl = y;
  }
   if (usepoly){
      cdCanvasEnd(canvas);
   }
   if (flush){
    if (cvtype == CDTYPE_DBUFFER)
      cdCanvasFlush(canvas);
    else if (cvtype == CDTYPE_SERVERIMG){
      cdCanvasPutImageRect(bcanvas, simg, 0, 0, 0, 0, 0, 0);
    }
  }
}

//
// Drawing for meter: double values
//
void winobj::draw_values(double *values, double *first_val, void *color, 
			 int nvals, int usepoly, int polytype, int polystep, int flush)
{
  int i;
  double y;
  double *pvals;
  xlast = 0;
  ylast = 0;
  cdCanvasForeground(canvas, (unsigned long int) color);

  if (usepoly){
    if (polytype == CD_BEZIER)
       cdCanvasBegin(canvas, CD_BEZIER);
    else
       cdCanvasBegin(canvas, CD_OPEN_LINES);
    wdCanvasVertex(canvas, 0, ylast);
  }
  pvals = first_val;
  for (i = 0; i < nvals; ++i) {
     y = *pvals;
     if (usepoly){
	if (polystep == 1){
	   wdCanvasVertex(canvas, i, ylast);
	} else if (polystep == -1) {
	   wdCanvasVertex(canvas, xlast, y);
	} 
	wdCanvasVertex(canvas, i, y);
     } else {
#if 0
	wdCanvasLine(canvas, i, 0, i, y);
#else
	wdCanvasLine(canvas, xlast, ylast, i, y);
#endif
     }
     if (++pvals == values + nvals)
	pvals = values;
     xlast = i;
     ylast = y;
  }
  if (usepoly){
     cdCanvasEnd(canvas);
  }
  if (flush){
     if (cvtype == CDTYPE_DBUFFER)
	cdCanvasFlush(canvas);
     else if (cvtype == CDTYPE_SERVERIMG){
	cdCanvasPutImageRect(bcanvas, simg, 0, 0, 0, 0, 0, 0);
    }
  }
}

