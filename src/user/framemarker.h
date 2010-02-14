#ifndef  _FRAMEMARKER_H
#define  _FRAMEMARKER_H

#include "marker.h"

//tolua_begin
class framemarker: public marker
{
   typedef marker baseclass;
   public:

   framemarker();
   ~framemarker();
   
   int maxsize;
   int vlanId;
   int vlanPriority;
   int dropPrecedence;
   int internalDropPrecedence;
//tolua_end

   rec_typ REC(data *, int);    
};  //tolua_export
#endif   // _FRAMEMARKER_H
