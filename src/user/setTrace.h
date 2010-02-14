#ifndef SETTRACE_INCL
#define SETTRACE_INCL

#include "in1out.h"

//tolua_begin
class	setTrace: public in1out {
typedef	in1out	baseclass;

public:
 int	act(void);
 rec_typ REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
 
 dat_typ inputType;
 int id;
 int seqNo;
};
//tolua_end
#endif
