/////////////////////////////////////////////////////////////////////////////
// FrameMarker
// Module, which marks all incoming data units with the given fields
// for clp marking: only frames which are >= maxsize are marked with clp
//
// Marker <name>:
//    {DOMARK=<int>}
//    {CLP=<int>,}: if not given, clp will not be set
//    {MAXSIZE=<int>,}: if not given, clp will not be set
//    {vlanId=<int>,}: if <0 or if not given, vlanId will not be set
//    {vlanPriority=<int>,}: if <0 or if not given, vlanPriority will not be set
//    {dropPrecedence=<int>,}: if <0 or if not given, dropPrecedence will not be set
//    {internalDropPrecedence=<int>,}: if <0 or not given, internalDropPrecedence will not be set
//    OUT=<suc>;
/////////////////////////////////////////////////////////////////////////////


#include "framemarker.h"

framemarker::framemarker()
{
}

framemarker::~framemarker()
{
}

/////////////////////////////////////////////////////////////////////////////
// init()
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// rec()
/////////////////////////////////////////////////////////////////////////////


/*
void framemarker::addpars(void)
{
   // read additional parameters of derived classes
   baseclass::addpars();

   if (test_word("MAXSIZE"))
   {
      maxsize = read_int("MAXSIZE");
      if(maxsize < 0)
         syntax0("MAXSIZE must be >=0");
      skip(',');
   }
   else
      maxsize = 10000;

   if (test_word("vlanId"))
   {
      vlanId = read_int("vlanId");
      skip(',');
   }
   else
      vlanId = -1;

   if (test_word("vlanPriority"))
   {
      vlanPriority = read_int("vlanPriority");
      skip(',');
   }
   else
      vlanPriority = -1;

   if (test_word("dropPrecedence"))
   {
      dropPrecedence = read_int("dropPrecedence");
      skip(',');
   }
   else
      dropPrecedence = -1;

   if (test_word("internalDropPrecedence"))
   {
      internalDropPrecedence = read_int("internalDropPrecedence");
      skip(',');
   }
   else
      internalDropPrecedence = -1;


} // framemarker:init()
*/

rec_typ framemarker::REC(data *pd,int)
{

   typecheck(pd, FrameType);
   frame *pf = (frame*) pd;
   
   // check if smaller than maxsize and if marking is to be performed
   if(domark && pf->frameLen <= maxsize && markclp >= 0)
      pd->clp = markclp;	 // mark the data item

   if(vlanId >= 0)
      pf->vlanId = vlanId;

   if(vlanPriority >= 0)
      pf->vlanPriority = vlanPriority;

   if(dropPrecedence >= 0)
      pf->dropPrecedence = dropPrecedence;

   if(internalDropPrecedence >= 0)
      pf->internalDropPrecedence = internalDropPrecedence;

   return suc->rec(pd, shand);	 // send it

} // marker::REC()

