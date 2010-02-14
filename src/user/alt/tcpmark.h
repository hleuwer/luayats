#ifndef  _TCPMARK_H
#define  _TCPMARK_H

#include "inxout.h"
#include "clpmux.h"


class   tcpmark:   public inxout
{
typedef inxout baseclass;
public:
   void init(void);
   rec_typ REC(data *, int);
   void tcpmark::connect(void);

   enum {OutData = 0, OutBack = 1};
   enum {InpData = 0, InpBack = 1}; 

   int mark;   // marking or not
   int vc;
   muxCLP *NextMux;
   char *NextMuxString;
   //int nbytes0;	     	   // number of bytes for this connection
   //int nbytes1;	     	   // number of bytes for this connection
   int counter_bytes;
   int nflows;       	   // number of flows in the ATM connection
   
   double uniform(){return my_rand() / 32767.0;}
};

#endif   // _TCPMARK_H
