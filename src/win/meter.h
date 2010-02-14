#ifndef METER_INCL
#define METER_INCL

#include "defs.h"
#include "winobj.h"
#include "yats.h"

//tolua_begin
class meter: public winobj
{
      typedef winobj baseclass;
   public:
      meter();
      ~meter();	

      int act(void);
      void drawWin(int activate = 1, int clear = 1, int flush = 1);
      //      double getVal(int index){return double_val_ptr[index];}
      double getVal(int index){return vals[index];}
      int getFirst(void){return (first_val - double_val_ptr);}
      int getLast(void){return (last_val - double_val_ptr);}
      int update;
      int nvals;
            
      display_mode_type display_mode;
      int drawmode;
      int linemode;
      int linecolor;

      // In order to control this via Lua.
      int do_drawing;
      double maxval;
      int update_cnt;
      tim_typ delta;

      int store2file(char *, char *);

      tim_typ lastScanTime;      

//tolua_end      

      double *vals;      // earlier exported to tolua as "double vals[_nvals]";
      
      void late(event *);
      void connect(void) {}


      XSegment *xsegs;

      double the_old_value;
      double the_value;

      int *int_val_ptr;
      double *double_val_ptr;


      double *first_val;
      double *last_val;
};  //tolua_export

#endif
