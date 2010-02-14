#ifndef  _MARKER_H
#define  _MARKER_H

#include "in1out.h"

//tolua_begin
class marker: public in1out
{
   typedef in1out baseclass;
   public:
      marker();	
      ~marker();
      
      int domark;    // really mark?
      int markclp;   // what to mark
//tolua_end

      rec_typ REC(data *, int);    
};  //tolua_export

#endif   // _MARKER_H
