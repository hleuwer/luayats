#ifndef YATS_INCL_
#define YATS_INCL_
#include "in1out.h"
#include "inxout.h"
#include "winobj.h"
#include <sys/time.h>

extern "C"
{
#include "lua.h"
}
extern lua_State *globalL;
extern lua_State *WL;
//tolua_begin
void delete_object(root *);
void workerentry(void);
void workerexit(void);
class lua1out: public in1out
{
public:
   lua1out(void);
   ~lua1out(){};
   void early(event *);
   void late(event *);
   rec_typ REC(data *, int);
   int dbg;
};

class luaxout: public inxout
{
public:
   luaxout(void){}
   ~luaxout(){}
   void early(event *);
   void late(event *);
   rec_typ REC(data *, int);
};

typedef enum {
   AbsMode = 0,
   DiffMode = 1
} display_mode_type;

typedef enum {
   LineMode = 0,
   FillMode = 1
} line_mode_type;
//tolua_end

#endif
