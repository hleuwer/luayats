#ifndef	_REDSPECIAL_H
#define	_REDSPECIAL_H


class redclass_special
{
   public: 
   
   int th_l;
   int th_h;
   double pmax;
   double pstart;
   double w_q;    // weight of last vaule for exponential averaging
   double avg;    // average queue size
   int count;
   int useaverage;
   
   double pb,pa;
   
   
   redclass_special()
   {
      pmax = 0.03; w_q = 0.002;  avg = 0; 
      count = -1, th_l = th_h = 0; pstart = 0.0;
      useaverage=1;
   }
   
   double uniform(){return my_rand() / 32767.0;} 
   
   int Update(int qlen)
   {
      if(useaverage)
         return UpdateWithAverage(qlen); 
      else
         return UpdateWithCurrent(qlen); 
   
   }
   
   
   int UpdateWithAverage(int qlen)
   {
   
      int drop = 0;

      avg = (1-w_q)*avg + w_q*qlen; // update avg queue size
      

      if(avg < th_l)
      {
      	 count++;
 
	 // check, if to drop the packet
	 if(uniform() <= pstart)
	 {
	    drop = 1;
	    count = 0;
	 }
      
      }
      if(th_l <= avg && th_h > avg)
      {
         count++;
	 
	 // calculate loss prob
	 pb = pmax*(double)(avg-th_l)/(double)(th_h-th_l);
	 pa = pb; // /(1.0-count*pb);
	 
	 // check, if to drop the packet
	 if(uniform() <= pa)
	 {
	    drop = 1;
	    count = 0;
	 }
      } // avg is between the threshold
      else if(th_h <= avg)
      {
         drop = 1;
	 count = 0;
      }
      else
         count = -1;
   
      return drop;
   
   }; // UpdateWithAverage


   int UpdateWithCurrent(int qlen)
   {
      int drop = 0;
   
      if (qlen < th_l){
         drop = 0;
      } else {
	if (qlen > th_h){
	  if(uniform() <= pmax)
	    drop = 1;
	  else {
	    double ploss;
	    
	    ploss = pmax * (double)(qlen-th_l) / (double)( th_h - th_l);
	    if(uniform() <= ploss)
	      drop = 1;
	    else
	      drop = 0;
	  }
	}
      }      
      return drop;
      
   } // UpdateWithCurrent
   
   
   
   
}; // class redclass

#endif	// _RED_H
