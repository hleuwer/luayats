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
*************************************************************************/
#ifndef _MEAS3_H_
#define _MEAS3_H_

#include "in1out.h"

//tolua_begin
typedef enum {
   keyStatTimer = 2
} key_stat_timer_type;

class meas3: public in1out
{
      typedef in1out baseclass;

   public:

      meas3() : evtStatTimer(this, keyStatTimer) {};
      ~meas3();
      event evtStatTimer;       // Statistics Timer

      dat_typ inp_type;  // type of input data

      int idx_max;  // range of VCI / CONNID
      int idx_min;
      int doRangeError;  // TRUE: generate out-of-range errors

      int ctd_max;  // max cell transfer delay time in distribution
      int ctd_min;
      int ctd_div;  // scaling factor for CTD distrib
      int iat_max;  // max inter arrival time in distribution
      int iat_min;
      int iat_div;  // scaling factor for IAT distrib
      
      double FrameRate;   // the actual data of all frames rate
      

      int StatTimerInterval; // length of Timer-Interval in slots
      inline int TimeToSlot(double t){ return ((int) (t / SlotLength));};
      int act(void);
      int doMeanCTD;
      int doMeanIAT;
      double StatTimer;
      int getCountByteSum(void){ return this->counter_byte;}
      double getFrameRate(void){ return this->FrameRate;}
      int getCount(int i){ return this->counters[i];}
      int getCountByte(int i){ return this->counters_byte[i];}

      int getCTDDist(int i, int j){ return this->ctd_dist[i][j];}
      int getCTDover(int i){ return this->ctd_overfl[i];}
      int getCTDunder(int i){ return this->ctd_underfl[i];}
      int getMinCTD(int i){ return this->minCTD[i];}
      int getMaxCTD(int i){ return this->maxCTD[i];}
      double getMeanCTD(int i){ return this->meanCTD[i];}

      int getIATDist(int i, int j){ return this->iat_dist[i][j];}
      int getIATover(int i){ return this->iat_overfl[i];}
      int getIATunder(int i){ return this->iat_underfl[i];}
      int getMinIAT(int i){ return this->minIAT[i];}
      int getMaxIAT(int i){ return this->maxIAT[i];}
      double getMeanIAT(int i){ return this->meanIAT[i];}
//tolua_end
      
      int command(char *, tok_typ *);
      rec_typ REC(data *, int); // REC is a macro normally expanding to rec (for debugging)
      void late(event *evt);
      int export(exp_typ *);

      // statistics values
      unsigned *counters;
      unsigned *counters_byte;
      unsigned counter_byte;
      unsigned **ctd_dist;  // CTDs (complete distribs)
      unsigned **iat_dist;  // IATs (complete distribs)
      unsigned *iat_overfl; // overflow of IAT
      unsigned *iat_underfl; // underflow
      unsigned *ctd_overfl; // overflow of CTD
      unsigned *ctd_underfl; // underflow

      tim_typ *minCTD;  // minimum encountered CTD
      tim_typ *maxCTD;  // max
      double *meanCTD; // cumulative mean value CTD

      tim_typ *minIAT;  // the same for IAT
      tim_typ *maxIAT;
      double *meanIAT;

      tim_typ *last_time; // time of last arrival
};  //tolua_export


#endif // _MEAS3_H_
