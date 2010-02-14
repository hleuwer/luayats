#include "in1out.h"
#include "queue.h"
#include <math.h> // for ceil

class leakybucket_d {

public:
   double value;
   double max;
   double inc;
   tim_typ last_update;

   leakybucket_d(void);
   void increment(void) {value += inc;};
   void decrement(int n);
   void update(void);
   int next(void);
   int isfull(void);


};

leakybucket_d::leakybucket_d(void)
{
   value = 0;
   max = 0;
   inc = 0;
   last_update = 0;
}

void leakybucket_d::decrement(int n) 
{
   value -= (double) n; 
   if(value < 0)
      value = 0;
}

void leakybucket_d::update(void) 
{
   int n;
   n = SimTime - last_update;
   if(n >= 0)
      decrement(n);
   
   last_update = SimTime;

}

int leakybucket_d::next(void) 
{
   int n;
   //update();
   n = (int) ceil(value - max);
   if(n < 1)
      n = 1;
   return(n);

}

int leakybucket_d::isfull(void)
{
   if(value > max)
      return 1; 
   else
      return 0;
}



class	intpol:	public	in1out {
typedef	in1out	baseclass;

public:	
   void	init(void);
   rec_typ REC(data *, int);
   int command(char *, tok_typ *);
   void restim(void);
   void early(event *);
   int export(exp_typ *);
   
   queue q;
   leakybucket_d lb;

private:
   int alarmed;   // is the queue alarmed
   int newframe;  // will the next cell be a new frame?
   int frame_clp; // what is the clp of this frame

};
	
