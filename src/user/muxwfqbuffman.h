#ifndef  _MUXWFQBUFFMAN_H
#define  _MUXWFQBUFFMAN_H


#include "mux.h"
#include "red_special.h"

class WFQBuffManConnParam : public cell
{
   public:
   
   WFQBuffManConnParam(int vc): cell(vc)
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



class muxWFQBuffMan: public mux
{
   typedef mux baseclass;

   public:
      struct wfqpar: public cell
      {
	 wfqpar(int vc): cell(vc) {}

	 queue	q;	// a queue per VC
	 int	delta;	// inverse of mean rate of this VC
      };

      // turn off parsing of "BUFF=..." by mux::init()
      // so that we can reuse mux::init()
      muxWFQBuffMan(void)
      {	
         doParseBufSiz = TRUE;
      }

      void addpars(void);
      void early(event *);
      void late(event *);
      void ResetStatVC(int vc);
      int command(char *, tok_typ *);
      int export(exp_typ *);
      void restim(void);
      void connect(void);

      tim_typ spacTime; // the central WFQ variable "Spacing Time"
      wfqpar **partab;	// per VC a data structure
      uqueue sortq;  	// the output sort queue
      int *qLenVCI;  // per VC: input queue length

      enum {spacTimeMax = 100000000}; // when to reset spacTime
      
      
      double reserved_SCR;    	      	 // the sum of the reserved rates
      
      // Connection parameter
      WFQBuffManConnParam **cpar;

      int epdThresh; 	// EPD threshold for CLP=0 cells
      int clp1Thresh;	// when to discard CLP=1 cells (see also epdClp1)
      int epdClp1;   	// boolean, do we perform EPD/PPD for CLP=1 ?
      int fairClp0;   	// boolean, discard clp0 if above threshold 
      int fba0;      	// boolean, should we perform fba for clp=0 cells?
      int deliverPt1;	// deliver pt=1 cells always (if glob. buffer allows)
      int performDFBA;	// perform DFBA?
      int perform_RED1; // perform RED for CLP=1 cells?
      
      redclass_special red1;

      
      int aktbuff;
      int maxbuff;
      
      
};

#endif   // _MUXWFQBUFFMAN_H
