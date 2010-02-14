#ifndef	_DATA2FRS_H_
#define	_DATA2FRS_H_

#include "in1out.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

//tolua_begin
class	data2frs: public in1out {
   typedef	in1out	baseclass;

   public:	        
	data2frs();
	~data2frs();
		
        int maxframe;
        int framesize;
        int framenr;
//tolua_end

	int REC(data*, int);        
   private:

};  //tolua_export

#endif	// _DATA2FRS_H_
