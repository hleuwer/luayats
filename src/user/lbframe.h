#ifndef	_LB_FRAME_H_
#define	_LB_FRAME_H_

#include "in1out.h"

//tolua_begin
class	lbframe: public in1out {
typedef	in1out	baseclass;

public:	
   lbframe();
   ~lbframe();

//	some of the private members made public so LUA init can access them

   int lb_tag;       // tagging (1) or discarding (0)?
   dat_typ inp_type;	// input data type
   double lb_inc;    // increment per cell
   double lb_siz;    // actual size of the bucket
   double lb_max;    // bucket size
   double lb_dec;    // decrement per time slot
   tim_typ last_time;	// last compliance time

   // Parameters for frame-awareness
   int vci_first;    // is the next cell the first of a new frame?
   int cellnumber;   // cells in the actual frame
   int vci_clp;      // is used for passed: 1->passed=0; 0->passed=1
   int lb_mfs;       // maximum frame size
      
//tolua_end   
   rec_typ REC(data *, int);
   void	restim(void);
   int export(exp_typ *);
   int command(char *, tok_typ *);

};  //tolua_export
	
#endif	// _LB_FRAME_H_
