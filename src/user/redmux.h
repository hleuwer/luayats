#ifndef  _MUXRED_H
#define  _MUXRED_H

#include "mux.h"
#include "red_special.h"



class RedMuxConnParam : public cell
{
   public:
   
   RedMuxConnParam(int vc): cell(vc)
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


class	muxRED: public mux
{
   
   typedef mux baseclass;
   public:

      void addpars(void);
      void late(event *);
      
      // Connection parameter
      RedMuxConnParam **cpar;
      redclass_special red;

      int epdThresh;
      int clp1Thresh;

};

#endif   // _MUXRED_H
