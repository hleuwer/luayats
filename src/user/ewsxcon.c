/*************************************************************************
*
*      YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997   Chair for Telecommunications
*            Dresden University of Technology
*            D-01062 Dresden
*            Germany
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
*   Module author:      Matthias Baumann, TUD
*   Creation:      July 8, 1996
*
*************************************************************************/

/*
*   Multiplexer EPD, purely event-driven
*
*   If the queue occupation reaches THRESH, then *starting* AAL5 frames
*   are dropped. Additionally, corrupted frames are discarded.
*
*   Non-AAL5 cells are queued in an extra queue which always is served first.
*   Exception: due to DF, service of an AAL5 cell is started (CBR queue empty) even if
*   a CBR/VBR cells arrives with this time step. On the other hand, a CBR cell can
*   overtake a AAL5 cell waiting for the start of next output cycle.
*
*   Per default, the last cell of a corrupted or rejected frame is not passed. This
*   can be changed with PASSEOF.
*
*   Synchronous or asynchronous output operation.
*
*   If an AAL5 cell with VCI>MAXVCI is received, then an error message is launched.
*
*   MuxEvtEPD mux: NINP=10, BUFF=1000, {MAXVCI=100,} {SERVICE=10,} MODE={Sync | Async},
*         THRESH=600, BUFCBR=100, {PASSEOF,} OUT=sink;
*         // default MAXVCI: NINP
*         // SERVICE: service takes SERVICE time steps (default: 1)
*         // MODE: synchronous or asynchronous operation of output.
*         //      MODE can be omitted if SERVICE=1
*         // BUFCBR: buffer size for non-AAL5 cells
*         // PASSEOF given: pass last cell of a rejected or
*         //      corrupted frame (default: do not)
*
*   Commands:   see Multiplexer (muxBase.c)
*
*   exported:   see muxBase
*         mux->QLenCBR   // queue length non-AAL5
*/

#include "ewsx.h"

CONSTRUCTOR(ControlEWSX, controlEWSX);
USERCLASS("ContrEWSX", ControlEWSX);

void controlEWSX::init(void)
{
	
   skip(CLASS);
   name = read_id(NULL);
   skip(':');

   maxmux = read_int("MAXMUX");
   if (maxmux <= 0)
      syntax1s("%s: MAXMUX must be > 0", name);
   
   skip(',');

   if (test_word("EPDTHRESH"))
   {
      epdThreshGlobal = read_int("EPDTHRESH");
      if(epdThreshGlobal <= 0)
         syntax0("EPDTHRESH must be > 0");
      skip(',');
   }
   else
      epdThreshGlobal = 0;	// mark no epd

   if (test_word("CLPTHRESH"))
   {
      clpThreshGlobal = read_int("CLPTHRESH");
      if(clpThreshGlobal <= 0)
         syntax0("CLPTHRESH must be > 0");
      skip(',');
   }
   else
      clpThreshGlobal = 0;	// mark no global dropping of clp=1 cells

   buff = read_int("BUFF");
   ReservedBuff = 0;

   if(epdThreshGlobal > buff)
      syntax0("EPDTHRESH must be <= BUFF");
   if(epdThreshGlobal == 0)
      epdThreshGlobal = buff + 1;	// never reach epd

   if(clpThreshGlobal > buff)
      syntax0("CLPTHRESH must be <= BUFF");
   if(clpThreshGlobal == 0)
      clpThreshGlobal = buff + 1;	// never reach


   pmux = new muxEWSX*[maxmux];
   
   qlen = 0;
   nmux = 0;

} // controlEWSX::init(void)


int controlEWSX::export(exp_typ *msg)
{
   return baseclass::export(msg) ||
      intScalar(msg, "QLen", &qlen);
}

//////////////////////////////////////////////////////////////
// controlEWSX::mux_register(muxEWSX* pointmux)
// during connect the muxes call this method with their
// this-pointer.
// So the control module gets knowledge about the muxes
//////////////////////////////////////////////////////////////
void controlEWSX::mux_register(muxEWSX* pointmux)
{
   pmux[nmux] = pointmux;
   nmux++;
   
   // check the buffer
   ReservedBuff += pointmux->ReservedBuff;
   if(ReservedBuff > buff)
      errm1s2d("%s: more buffer reserved than availabe: ReservedBuff=%d,"\
         "available buff=%d", name, ReservedBuff, buff);
   
} // controlEWSX::mux_register()

//////////////////////////////////////////////////////////////
// controlEWSX::pushout()
// try's to pushout the cells of one connection
// ### PROBLEM - only UBR cells are pushed out now
//////////////////////////////////////////////////////////////
void controlEWSX::pushout(void)
{
   int start;
   int next;
   ServiceClassType sc;
   int i;
   
   start = my_rand() % nmux;
   
   sc = UBR;
   for(i=1; i<= nmux; i++)
   {
      next = (start+i) % nmux;
      // pushout the cells
      if(pmux[next]->pushout(sc))
         return;	// successful
            
   } // for - try each multiplexer

   
} // controlEWSX::pushout();
