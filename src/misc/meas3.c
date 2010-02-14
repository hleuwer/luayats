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
* History: Bug fix in command():
*   MeanIAT() counted wrong, see there
*    M. Baumann, April 18, 1997
*    Thanx to Torsten Mueller who found the bug.
*
* Oct 28, 1997 Meas2 -> Meas3:
*   CTD and IAT measurements now optional.
*   Measurement ranges also have a lower bound.
*   Measurements can be scaled down by an integer factor.
*   MAXVCI added: parallel measurement on multiple channels
*   MAXCID added -> measurements for frames.
*    M. Baumann
*
*************************************************************************/

/*
*
* Measurement device for inter arrival times and "cell" transfer delays:
*
* Meas3 ms:
*   {SLOTLENGTH=<double>},
*   {StatTimer=<double>} (update of Rate in seconds, default = 0.1s)
*  {CTD {=(min,max) {, CTDDIV=10}}}
*   // CTD: provide cumulative mean value and extreme values (min/max)
*   // (updated "on-the-fly" with each arrival)
*   // CTD=(min,max): provide additionally detailed distribution of CTD,
*   // range min ... max (updated "on-the-fly" with each arrival)
*   // CTDDIV given (only together with CTD=(min,max)):
*   // Measured values first are divided by CTDDIV.
*   // CTD=(min,max) specifies the figures after dividing by CTDDIV.
*   // Division influences *only* the detailed distribution.
*   // MinCTD, MaxCTD, and MeanCTD remain unscaled.
*  {, IAT {=(min,max)  {, IATDIV=10}}}
*   // meanings like for CTD
*  {, {VCI | CONNID}=(min,max)}
*   // if neither VCI nor CID are not given, then all data objects
*   // are counted (only one set of statistics, index 0).
*   // VCI: cells are expected, measurement per VCI (min ... max)
*   // CONNID: frames are expected, measurement per connection ID (min ... max)
*  {, ERANGE} { ,OUT=sink } ;
*   // if ERANGE given: out-of-range cells or frames cause an error.
*   //  otherwise: only the overall counter is incremented
*   // if OUT is omitted, then the device is a sink
*
* Commands:
*  // the following variables actually are exported, i.e. they can be visualised
*  // using graphical online displays
*  ms->Count
*   get overall number of arrivals
*  ms->Cout_byte (only for frames)
*  ms->FrameRate (only for frames, takes into account SLOTLENGTH
*  ms->Counts(i)
*   get number of arrivals for this index
*  ms->Counts_byte(i)  NOT YET TESTED
*  ms->CTD(i, tim) // problems with online display
*   get counter for cell transfer delay time tim (scaled by CTDDIV)
*  ms->IAT(i, tim) // problems with online display
*   get counter for inter arrival time tim (scaled by IATDIV)
*  ms->CTDover(i)
*   get overflow counter (index i) for CTD distribution
*  ms->CTDunder(i)
*   get underflow counter (index i) for CTD distribution
*  ms->IATover(i)
*   get overflow counter (index i) for IAT distribution
*  ms->IATunder(i)
*   get underflow counter (index i) for IAT distribution
*  ms->MeanCTD(i)  // current cumulative mean CTD value
*  ms->MaxCTD(i)  // largest encountered CTD
*  ms->MinCTD(i)  // smallest encountered CTD
*  ms->MeanIAT(i)  // current cumulative mean IAT value
*  ms->MaxIAT(i)  // largest encountered IAT
*  ms->MinIAT(i)  // smallest encountered IAT
*
*  Classical commands:
*  ms->ResStats
*   reset all IAT and CTD statistics and all counters
*  ms->ResCount
*   reset overall number of arrivals (inherited from ino)
*/

#include "meas3.h"


meas3::~meas3()
{
  int idx;
  
  if (minCTD)
    delete minCTD;
  if (maxCTD)
    delete maxCTD;
  if (meanCTD)
    delete meanCTD;
  if (ctd_overfl)
    delete ctd_overfl;
  if (ctd_underfl)
    delete ctd_underfl;
  
  if (ctd_dist){
    if (ctd_max != 0){
      for (idx = 0; idx < idx_max; ++idx) 
	delete ctd_dist[idx];
    }
    delete ctd_dist;
  }
  if (last_time)
    delete last_time;
  if (minIAT)
    delete minIAT;
  if (maxIAT)
    delete maxIAT;
  if (meanIAT)
    delete meanIAT;
  
  if (iat_overfl)
    delete iat_overfl;
  if (iat_underfl)
    delete iat_underfl;
  
  if (iat_dist) {
    if (iat_max != 0) {
      for (idx = 0; idx < idx_max; ++idx)
	delete iat_dist[idx];
    }
    delete iat_dist;
  }
  
  delete counters;
  delete counters_byte;
}


// Init by Lua - only stuff that can only be done in C.
int meas3::act(void)
{
   int i, idx;
   if (doMeanCTD) {
      CHECK(minCTD = new tim_typ [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         minCTD[idx] = ~0;
      CHECK(maxCTD = new tim_typ [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         maxCTD[idx] = 0;
      CHECK(meanCTD = new double [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         meanCTD[idx] = 0.0;
   } else {
      minCTD = NULL;
      maxCTD = NULL;
      meanCTD = NULL;
   }

   if (ctd_max != 0) {
      CHECK(ctd_overfl = new unsigned int [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         ctd_overfl[idx] = 0;
      CHECK(ctd_underfl = new unsigned int [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         ctd_underfl[idx] = 0;
      CHECK(ctd_dist = new unsigned int * [idx_max]);
      for (idx = 0; idx < idx_max; ++idx) {
         CHECK(ctd_dist[idx] = new unsigned int[ctd_max]);
         for (i = 0; i < ctd_max; ++i)
            ctd_dist[idx][i] = 0;
      }
   } else {
      ctd_overfl = NULL;
      ctd_underfl = NULL;
      ctd_dist = NULL;
   }

   if (doMeanIAT) {
      CHECK(last_time = new tim_typ [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         last_time[idx] = 0;
      CHECK(minIAT = new tim_typ [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         minIAT[idx] = ~0;
      CHECK(maxIAT = new tim_typ [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         maxIAT[idx] = 0;
      CHECK(meanIAT = new double [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         meanIAT[idx] = 0.0;
   } else {
      last_time = NULL;
      minIAT = NULL;
      maxIAT = NULL;
      meanIAT = NULL;
   }
   if (iat_max != 0) { // this implies: doMeanIAT (last_time needed)
      CHECK(iat_overfl = new unsigned int [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         iat_overfl[idx] = 0;
      CHECK(iat_underfl = new unsigned int [idx_max]);
      for (idx = 0; idx < idx_max; ++idx)
         iat_underfl[idx] = 0;
      CHECK(iat_dist = new unsigned int * [idx_max]);
      for (idx = 0; idx < idx_max; ++idx) {
         CHECK(iat_dist[idx] = new unsigned int[iat_max]);
         for (i = 0; i < iat_max; ++i)
            iat_dist[idx][i] = 0;
      }

   } else {
      iat_overfl = NULL;
      iat_underfl = NULL;
      iat_dist = NULL;
   }

   CHECK(counters = new unsigned int [idx_max]);
   for (idx = 0; idx < idx_max; ++idx)
      counters[idx] = 0;

   // added Mue 2003-09-04
   // to count bytes for frames
   CHECK(counters_byte = new unsigned int [idx_max]);
   for (idx = 0; idx < idx_max; ++idx)
      counters_byte[idx] = 0;

   counter_byte = 0;
   return 0;
}

/*
* A data object has arrived.
* Perform measurements in case of in-range ID.
* Pass or terminate data object.
*/

rec_typ meas3::REC( // REC is a macro normally expanding to rec (for debugging)
   data *pd,
   int )
{
   tim_typ tim;
   int idx, cnt;
   unsigned oldcounter_byte;
   int len;

   typecheck(pd, inp_type); // input data type check


   if (typequery(pd, FrameType)) {
      len = ((frame *) pd)->frameLen;

      oldcounter_byte = counter_byte;
      counter_byte += len;
      if ( counter_byte < oldcounter_byte)
         errm1s("%s: overflow of counter_byte", name);

      FrameRate = (double)counter_byte / (double)SimTime / SlotLength * 8.0;
   }

   switch (inp_type) {
   case DataType:
      idx = 0; // idx_max == 1, see init()
      break;
   case CellType:
      idx = ((cell *) pd)->vci - idx_min;
      if (idx < 0 || idx >= idx_max) {
         if (doRangeError)
            errm1s1d("%s: cell with out-of-range VCI %d received",
                     name, idx + idx_min);
         else
            goto xit; // only count counter
      }
      break;
   case FrameType:
      idx = ((frame *) pd)->connID - idx_min;
      len = ((frame *) pd)->frameLen;
      oldcounter_byte = counter_byte;

      if (idx < 0 || idx >= idx_max) {
         if (doRangeError)
            errm1s1d("%s: frame with out-of-range connection ID %d received",
                     name, idx + idx_min);
         else {
            oldcounter_byte = counters_byte[idx];
            if ((counters_byte[idx] += len) < oldcounter_byte)
               errm1s1d("%s: overflow of counters_byte[%d]", name, idx);
            goto xit; // only count counter
         }
      }
      break;
   default:
      errm1s("internal error: %s: bad inp_type in meas3::rec()", name);
      idx = 0; // not reached
   }


   // data count
   if ((cnt = ++counters[idx]) == 0)
      errm1s1d("%s: overflow of counters[%d]", name, idx);

   // Cell Transfer Delay
   if (meanCTD) {
      tim = (SimTime - pd->time);

      // min , max , mean values
      if (tim < minCTD[idx])
         minCTD[idx] = tim;
      if (tim > maxCTD[idx])
         maxCTD[idx] = tim;
      meanCTD[idx] = ((cnt - 1) * meanCTD[idx] + tim) / (double) cnt;

      if (ctd_dist) { // detailed distribution
         if ((tim /= (tim_typ) ctd_div) < (tim_typ) ctd_min) {
            if ( ++ctd_underfl[idx] == 0)
               errm1s1d("%s: overflow of ctd_underfl[%d]", name, idx);
         } else if ((tim -= (tim_typ) ctd_min) >= (tim_typ) ctd_max) {
            if ( ++ctd_overfl[idx] == 0)
               errm1s1d("%s: overflow of ctd_overfl[%d]", name, idx);
         } else {
            if ( ++ctd_dist[idx][tim] == 0)
               errm1s2d("%s: overflow of ctd_dist[%d][%d]",
                        name, idx, tim);
         }
      }
   }

   // Inter Arrival Time
   if (meanIAT) {
      tim = (SimTime - last_time[idx]);
      last_time[idx] = SimTime;

      // min , max , mean values
      if (tim < minIAT[idx])
         minIAT[idx] = tim;
      if (tim > maxIAT[idx])
         maxIAT[idx] = tim;
      meanIAT[idx] = ((cnt - 1) * meanIAT[idx] + tim) / (double) cnt;

      if (iat_dist) { // detailed distribution
         if ((tim /= (tim_typ) iat_div) < (tim_typ) iat_min) {
            if ( ++iat_underfl[idx] == 0)
               errm1s1d("%s: overflow of iat_underfl[%d]", name, idx);
         } else if ((tim -= (tim_typ) iat_min) >= (tim_typ) iat_max) {
            if ( ++iat_overfl[idx] == 0)
               errm1s1d("%s: overflow of iat_overfl[%d]", name, idx);
         } else {
            if ( ++iat_dist[idx][tim] == 0)
               errm1s2d("%s: overflow of iat_dist[%d][%d]",
                        name, idx, tim);
         }
      }
   }

xit:  // for out-of-range data objects we directly jump here
   if ( ++counter == 0)
      errm1s("%s: overflow of counter", name);

   if (suc != NULL)
      return suc->rec(pd, shand);
   else {
      delete pd;
      return ContSend;
   }
}


/*
* Command procedures: reset counters
*/
int meas3::command(
   char *s,
   tok_typ *v)
{
   if (baseclass::command(s, v) == TRUE)
      return TRUE;

   v->tok = NILVAR;
   if (strcmp(s, "ResStats") == 0) {
      for (int idx = 0; idx < idx_max; ++idx) {
         if (ctd_dist) {
            ctd_overfl[idx] = 0;
            ctd_underfl[idx] = 0;
            for (int i = 0; i < ctd_max; ++i)
               ctd_dist[idx][i] = 0;
         }
         if (meanCTD) {
            minCTD[idx] = ~0;
            maxCTD[idx] = 0;
            meanCTD[idx] = 0.0;
         }
         if (iat_dist) {
            iat_overfl[idx] = 0;
            iat_underfl[idx] = 0;
            for (int i = 0; i < iat_max; ++i)
               iat_dist[idx][i] = 0;
         }
         if (meanIAT) {
            minIAT[idx] = ~0;
            maxIAT[idx] = 0;
            meanIAT[idx] = 0.0;
         }
         counters[idx] = 0;
         counters_byte[idx] = 0;
      }
      counter = 0;
      counter_byte = 0;
   } else
      return FALSE;

   return TRUE;
}

////////////////////////////////////////////////////////////////////
// late()
// - perform Timers and
////////////////////////////////////////////////////////////////////
void meas3::late(event *evt)
{
   /////////////////////////////////////////////////////////////////
   // Statistics Timer
   /////////////////////////////////////////////////////////////////
   if (evt->key == keyStatTimer) {
      FrameRate = (double)counter_byte / (double)SimTime / SlotLength * 8.0;

      alarml(&evtStatTimer, StatTimerInterval); // alarm again
      return ;

   } // Statistics Timer
   else
      errm1s("%s: internal error: an event arrived in late which has not been defined",
             name);


} // meas3::late(event *)


/*
* export of variables
*/
int meas3::export(
   exp_typ *msg)
{
  return (baseclass::export(msg) ||
          intScalar(msg, "Count_byte", (int *) &counter_byte) ||
          doubleScalar(msg, "FrameRate", (double *) &FrameRate) ||
          intArray1(msg, "Counts", (int *) counters, idx_max, idx_min) ||
          intArray1(msg, "Counts_byte", (int *) counters_byte, idx_max, idx_min) ||
          (ctd_dist && intArray2(msg, "CTD", (int **) ctd_dist, idx_max, idx_min, ctd_max, ctd_min)) ||
          (ctd_overfl && intArray1(msg, "CTDover", (int *) ctd_overfl, idx_max, idx_min)) ||
          (ctd_underfl && intArray1(msg, "CTDunder", (int *) ctd_underfl, idx_max, idx_min)) ||
          (minCTD && intArray1(msg, "MinCTD", (int *) minCTD, idx_max, idx_min)) ||
          (maxCTD && intArray1(msg, "MaxCTD", (int *) maxCTD, idx_max, idx_min)) ||
          (meanCTD && doubleArray1(msg, "MeanCTD", meanCTD, idx_max, idx_min)) ||
          (iat_dist && intArray2(msg, "IAT", (int **) iat_dist, idx_max, idx_min, iat_max, iat_min)) ||
          (iat_overfl && intArray1(msg, "IATover", (int *) iat_overfl, idx_max, idx_min)) ||
          (iat_underfl && intArray1(msg, "IATunder", (int *) iat_underfl, idx_max, idx_min)) ||
          (minIAT && intArray1(msg, "MinIAT", (int *) minIAT, idx_max, idx_min)) ||
          (maxIAT && intArray1(msg, "MaxIAT", (int *) maxIAT, idx_max, idx_min)) ||
          (meanIAT && doubleArray1(msg, "MeanIAT", meanIAT, idx_max, idx_min)));
}
