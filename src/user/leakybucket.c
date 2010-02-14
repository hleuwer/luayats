#include "leakybucket.h"
#include <math.h> // for ceil


/////////////////////////////////////////////////////////////////////////////
// Dual Leaky Bucket
/////////////////////////////////////////////////////////////////////////////

// Issue: we need to change the code to be according to the ATMF GCRA
// and to be with integer logic!

// Issue: what is implemented now is a dual token bucket similar to the
// "Two-Rate Three Color Marker" defined in RFC2698
// the counter, however, counts up for each packet (in RFC2698 it counts down)


LeakyBucket::LeakyBucket(void)
{
   value = 0;
   max = 0;
   last_update = 0;
} // LeakyBucket::LeakyBucket(void) - constructor

LeakyBucket::~LeakyBucket(void)
{
}

void LeakyBucket::SetParam(double rate, int burstsize)
{
   decrement_per_slot = rate / 8.0 * SlotLength;
   max = burstsize;
   
   //printf("LeakyBucket::SetParam : decrement_per_slot=%f max=%f\n",
   //    decrement_per_slot, max);
   
} // LeakyBucket::SetParam(double rate, int burstsize)

/////////////////////////////////////////////////////////
// LeakyBucket::PacketArrivalConformance
// A packet of with framelength "bytes" has arrived, is
// this packet conforming?
// The buckets are increased
// Return value:
//  0 - packet is not conforming
//  1 - packet is conforming
/////////////////////////////////////////////////////////
int LeakyBucket::PacketArrivalConformance(int bytes)
{
   update();

   value += bytes;
   
   //printf("value=%f\n",value);
   
   if(value > max)
   {
      value -= bytes;
      return 0;   // not conforming
   }
   else
      return 1;   // conforming

}; // LeakyBucket::PacketArrivalConformance(int bytes)

/////////////////////////////////////////////////////////
// LeakyBucket::Update
// Update the LeakyBucket counter to the current time
// we decrement the counter by n*decrement_per_slot
// decrement_per_slot determines the data rate
/////////////////////////////////////////////////////////
void LeakyBucket::update(void) 
{
   int n;
   n = SimTime - last_update;
   if(n >= 0)
	{
   	value -= (double) (n * decrement_per_slot); 
   	if(value < 0)
      	value = 0;
	} // decrement only 
   
   last_update = SimTime;

} // LeakyBucket::update(void) 


/////////////////////////////////////////////////////////////////////////////
// Dual Leaky Bucket
/////////////////////////////////////////////////////////////////////////////
DualLeakyBucket::DualLeakyBucket(root* owner)
{
  pOwner = owner;
  pLbCir = NULL;
  pLbPir = NULL;
  CHECK(pLbCir = new LeakyBucket());
  CHECK(pLbPir = new LeakyBucket());
} 

DualLeakyBucket::~DualLeakyBucket(void)
{
  if (pLbCir) 
    delete pLbCir;
  if (pLbPir) 
    delete pLbPir;
}

/////////////////////////////////////////////////////////
// DualLeakyBucket::SetParam
// Set the parameters for a dual leaky bucket
/////////////////////////////////////////////////////////
void DualLeakyBucket::SetParam(double cir, int cbs, double pir, int pbs)
{
   pLbCir->SetParam(cir, cbs);
   pLbPir->SetParam(pir, pbs);
}

/////////////////////////////////////////////////////////
// DualLeakyBucket::PacketArrivalConformance(bytes)
// A packet of with framelength "bytes" has arrived, is
// this packet conforming?
// The buckets are increased
// Return value:
// 	TRUE if packet is conforming and need not be dropped
// 	FALSE if packet is not conforming and must be dropped
// 	*PDropPrec = 0 (green), 1 (yellow) or 2 (red)
/////////////////////////////////////////////////////////
int DualLeakyBucket::PacketArrivalConformance(int bytes,int *pDropPrec)
{

// issue: we need to add the color-aware mode
// issue: in the data processing spec I have defined separate handling
//   for PIR and CIR, here it is combined

   int confCir, confPir;
   
   confCir = pLbCir->PacketArrivalConformance(bytes);
   confPir = pLbPir->PacketArrivalConformance(bytes);

	if(action == 0)
	   return TRUE;			//packet always conforming
	else if(action == 1) 	// mark - impelement 2-Rate 3color marker (RFC2698)
	{
	   if(!confPir)			// PIR bucket not conforming
		   *pDropPrec = 2;	// red
		else if(!confCir)		// CIR bucket not conforming
		   *pDropPrec = 1;	// yellow
		else
		   *pDropPrec = 0;	// green
		
		return TRUE;			// packet is conforming and not dropped
	}
	else if (action == 2)	// drop if CIR bucket not conforming
	{
		if(!confCir)			// not conforming to CIR
         return FALSE;
	}
	else if (action == 3)	// drop if PIR bucket not conforming
	{
		if(!confPir)			// PIR bucket not conforming
         return FALSE;		// drop packet
	}
	else
	{
	   errm2s2d("%s: (file %s, line %d) Internal error: policing action %d shall be applied but no rule exists",
	   pOwner->name, __FILE__, __LINE__, action);
	}

	return TRUE;

} // DualLeakyBucket::PacketArrivalConformance(int bytes,int *pDropPrec)

