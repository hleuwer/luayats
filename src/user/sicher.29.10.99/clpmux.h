#ifndef  _MUXCLP_H
#define  _MUXCLP_H

#include "mux.h"

class MuxConnParam : public cell
{
   public:
   
   MuxConnParam(int vc): cell(vc)
   {
      vciOK = TRUE;  	   // no cells are lost in the beginning
      vciFirst = TRUE;	   // first cell is first of a packet
      received = 0;
      lost = 0;
      received0 = 0;
      lost0 = 0;
      served = 0;
   };

   int vciOK;        	// is a cell of the actual frame (not) lost?
   int vciFirst;     	// next cell is the first of a frame
   int received;
   int lost;
   int received0;
   int lost0;
   int served;
};



class	muxCLP: public mux
{
   typedef mux baseclass;
   public:
      void addpars(void);
      void late(event *);
      double getCLR(int vc);
      double getCLR0(int vc);
      double getCLR1(int vc);
      int getBytes(int vc);
      int getServedBytes(int vc);
      // Connection parameter
      MuxConnParam **cpar;

      int epdThresh;
      int clp1Thresh;
      //int *vciFirst;  // TRUE: first cell of burst
      //int *vciOK;     // TRUE: cells can be queued
};

#endif   // _MUXCLP_H
