/////////////////////////////////////////////////////////////////////////////
// Marker
// Module, which marks all incoming data units with the given CLP
// marks only if DOMARK=1
// Marker <name>: {DOMARK=<int>,} CLP=<int>, OUT=<suc>;
/////////////////////////////////////////////////////////////////////////////


#include "marker.h"

marker::marker()
{
}

marker::~marker()
{
}

/////////////////////////////////////////////////////////////////////////////
// rec()
/////////////////////////////////////////////////////////////////////////////
rec_typ marker::REC(data *pd,int)
{
   if(domark && markclp >= 0)
      pd->clp = markclp;	 // mark the data item
   return suc->rec(pd, shand);	 // send it

} // marker::REC()
