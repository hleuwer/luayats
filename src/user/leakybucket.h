#ifndef   _LEAKYBUCKET_H_
#define   _LEAKYBUCKET_H_

#include "defs.h"


// class Policer
// {
//    public:
// 
//    virtual void SetParam(double cir, int cbs, double pir, int pbs, double slotlength);
//    virtual int PacketArrivalConformance(int bytes);
// 	
// };

//tolua_begin
class LeakyBucket
{
public:
  LeakyBucket(void);	//constructor
  ~LeakyBucket(void);

  double value;  // one token represents one byte
  double max;    // the maximum size of the bucket, if above -> not conforming
  tim_typ last_update;       // time when the bucket was last updated
  double decrement_per_slot; // according to the wished rate, we need to
                             // increment for each slot
  
  void SetParam(double rate, int burstsize);
  int PacketArrivalConformance(int bytes);
  //tolua_end
private:
  void update(void);
  
}; //tolua_export

//tolua_begin
class DualLeakyBucket
{
 public:
  
  DualLeakyBucket(root* owner);
  ~DualLeakyBucket(void);

  root* pOwner; // the owner object of this dual leaky bucket
  LeakyBucket *pLbCir;
  LeakyBucket *pLbPir;
    
  void SetParam(double cir, int cbs, double pir, int pbs);
  int PacketArrivalConformance(int bytes, int *dropPrec);
  int action; // the action what to do
  int SetAction(int a)
  {
    action = a;
    if(action <0 || action >3)
      return FALSE;
    else
      return TRUE;
  };
};
//tolua_end
#endif  // _LEAKYBUCKET_H_
