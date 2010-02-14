#ifndef  _MUXCLP_H
#define  _MUXCLP_H

#include "mux.h"
#include "red_special.h"

class ClpMuxConnParam : public cell
{
   public:
   
   ClpMuxConnParam(int vc): cell(vc)
   {
      vciOK = TRUE;  	   // no cells are lost in the beginning
      vciFirst = TRUE;	   // first cell is first of a packet
      received = 0;
      lost = 0;
      received0 = 0;
      lost0 = 0;
      served = 0;

      qlen  = 0;
      qlen0 = 0;
      
      SCR = 0;
   };

   int vciOK;        	// is a cell of the actual frame (not) lost?
   int vciFirst;     	// next cell is the first of a frame
   int received;
   int lost;
   int received0;
   int lost0;
   int served;
   int qlen;         	// actual queuelength
   int qlen0;        	// actual queuelength of CLP=0 cells
   int maxqlen_epd;  	// maximal vci queue length (perform EPD then)
   int maxqlen1_epd;  	// maximal vci queue length for CLP1 (perform EPD then)
   
   double SCR;       	// reserved SCR
   double SCR_ratio; 	// the ratio of this VC
};


class	muxCLP: public mux
{
   
   typedef mux baseclass;
   public:

      void addpars(void);
      void late(event *);
      void early(event *);
      void ResetStatVC(int vc);
      int command(char*,tok_typ*);
      void connect(void);
      double reserved_SCR;    	      	 // the sum of the reserved rates
      
      // Connection parameter
      ClpMuxConnParam **cpar;

      int epdThresh; 	// EPD threshold for CLP=0 cells
      int clp1Thresh;	// when to discard CLP=1 cells (see also epdClp1)
      int epdClp1;   	// boolean, do we perform EPD/PPD for CLP=1 ?
      int fairClp0;   	// boolean, discard clp0 if above threshold 
      int fba0;      	// boolean, should we perform fba for clp=0 cells?
      int deliverPt1;	// deliver pt=1 cells always (if glob. buffer allows)
      int performDFBA;	// perform DFBA?
      int perform_RED1; // perform RED for CLP=1 cells?
      
      redclass_special red1;
};

#endif   // _MUXCLP_H
