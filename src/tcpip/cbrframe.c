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
* Module author:  Gunnar Loewe, TUD (diploma thesis)
* Creation:  Sept 1996
*
* History
* July 18, 1997  layer 4 connection ID introduced.
*    Optional parameter CONNID. Default value 0.
*
*************************************************************************/

/*
* CBR source for Frames: (!!! only for TESTs !!!) Loewe
*
* CBRFrame src: DELTA=10, LEN=100, {StartTime=,}{EndTime=,} {BYTES=,} OUT=sink;
*
*/

#include "cbrframe.h"

cbrframe::cbrframe()
{
}
cbrframe::~cbrframe()
{
}

int cbrframe::act(void)
{
   alarme( &std_evt, StartTime);
   send_state = ContSend;
   sent = 0;
   return 0;
}

void cbrframe::early(event *)
{
   if ( ++counter == 0)
      errm1s("%s: overflow of departs", name);

   if (sent + pkt_len > bytes)
      pkt_len = bytes - sent;

   sent += pkt_len;

   chkStartStop(send_state = suc->rec(new frame(pkt_len, connID), shand));

   if (send_state == ContSend && EndTime >= SimTime + delta && bytes > sent)
      alarme( &std_evt, delta);
   else if (send_state == StopSend)
      send_time = SimTime + delta;
}

// REC is a macro normally expanding to rec (for debugging)
rec_typ cbrframe::REC(data *pd, int) 
{
   if (send_state == StopSend)
   {
      send_state = ContSend;
      if (SimTime >= send_time)
         alarme(&std_evt, 1);
      else
         alarme(&std_evt, send_time - SimTime);
   }

   delete pd;
   return ContSend;
}

void cbrframe::restim(void)
{
   if (send_time > SimTime)
      send_time -= SimTime;
   else
      send_time = 0;
}
