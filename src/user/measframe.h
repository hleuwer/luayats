
#ifndef	_MEASFRAME_H_
#define	_MEASFRAME_H_

#include "in1out.h"

//tolua_begin
class	measframe:	public	in1out {
typedef	in1out	baseclass;

public:
   measframe();
   ~measframe();
   
   double meanGoodput5;       // the mean of Delay divided by Length
   double meanGoodput5Invers; // the mean of Length divided by Delay
//tolua_end

   int command(char *, tok_typ *);
   rec_typ REC(data *, int);
   int	export(exp_typ *);
};  //tolua_export

#endif	// _MEASFRAME_H_
