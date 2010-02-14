/*************************************************************************
*
*		YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997	Chair for Telecommunications
*				Dresden University of Technology
*				D-01062 Dresden
*				Germany
*
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
*	Module author:		Herbert Leuwer, Marconi Ondata GmbH
*	Creation:		2004
*
*************************************************************************/
#ifndef	_WINOBJ_H_
#define	_WINOBJ_H_

extern "C" {
#include "lua.h"
#include "iup/iup.h"
#include "cd/cd.h"
#include "cd/wd.h"
#include "cd/cddbuf.h"
#include "cd/cdimage.h"
#include "cd/cdirgb.h"
}
#include "inxout.h"

typedef struct {
  short x1, x2, y1, y2;
} XSegment;

struct viewport {
  int xmin;
  int xmax;
  int ymin;
  int ymax;
};

//tolua_begin
#define CDTYPE_IUP (1)
#define CDTYPE_DBUFFER (2)
#define CDTYPE_SERVERIMG (3)
#define CDTYPE_CLIENTIMG (4)
class	winobj:	public	inxout	{
typedef	inxout	baseclass;
public:
  winobj();
  ~winobj();
  void set_textsize(int);
  cdCanvas *map(void *iupcanvas, void *wid,
	    int width, int height, int xpos, int ypos, 
	    const char *wrtypeface = "Helvetica",
	    int wrstyle = CD_BOLD,
	    int wrsize = 10,
	    void *wrcol = (void *) CD_BLACK, 
	    int wrmode = CD_REPLACE, 
	    void *fgcol =  (void *) CD_BLACK,
	    int fgmode = CD_REPLACE, 
	    int fgstyle = CD_CONTINUOUS,
	    int fgwidth = 1,
	    void *bgcol =  (void *) CD_WHITE, 
	    int opacity = CD_TRANSPARENT,
	    int usepoly = 0,
	    int polytype = CD_OPEN_LINES,
	    int polystep = 0,
	    int canvastype = CDTYPE_SERVERIMG);
//   void * map(void *, void*, 
// 	     int, int, int, int, 
// 	     int, int, int, int, 
// 	     int, int, int, int,
// 	     int, int, int);
  void unmap(void);
  void draw_values(int *, void *color, int nvals, int usepoly, int polytype, int polystep, int flush);
  void draw_values(double *, double *, void *color, int nvals, int usepoly, int polytype, int polystep, int flush);
  virtual void drawWin(int activate = 1, int clear = 1, int flush = 1){}
  int getexp(exp_typ *);
  cdCanvas* setCanvas(cdCanvas * cv){
     if (canvas_mapped == false)
	return NULL;
     cdCanvas *ocv = canvas;
     canvas = cv;
     return ocv;
  }
  cdCanvas* getCanvas(void){
     if (canvas_mapped == true)
	return canvas;
     else
	return NULL;
  }
  int activateCanvas(void){
     if (canvas_mapped == true)
	return cdCanvasActivate(canvas);
     else
	return CD_ERROR;
  }
  void setScale(double scx, double scy){
     sx = scx;
     sy = scy;
  }
  void setViewport(int xmin, int xmax, int ymin, int ymax){
    vp.xmin = xmin;
    vp.xmax = xmax;
    vp.ymin = ymin;
    vp.ymax = ymax;
    wdCanvasViewport(canvas, vp.xmin, vp.xmax, vp.ymin, vp.ymax);
  }
  root *exp_obj;
  char *exp_varname;
  int exp_idx;
  int exp_idx2;
  char *title;
  int cvtype;
  cdCanvas *canvas;
  cdCanvas *bcanvas;
  cdImage *simg;
  bool canvas_mapped;
  int drawoper;
  int drawstyle;
  int drawwidth;
  int textoper;
  char *texttypeface;
  int textstyle;
  void *textcolor;
  void *bgcolor;
  void *drawcolor;
  int textsize;
  int bgopacity;
  int width, height, xpos, ypos;
  int usepoly, polytype, polystep;
  double sx, sy;
  double xlast, ylast;
  //tolua_end
  struct viewport vp;
  //tolua_begin
};
//tolua_end
#endif 
