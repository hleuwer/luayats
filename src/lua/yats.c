#include "defs.h"
#include "sim.h"
#include "yats.h"
#include "inxout.h"
#include "in1out.h"
extern "C" {
#include "stdlib.h"
#include "stdio.h"
#include "stdarg.h"
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include "tolua++.h"
#include "yats_bind.h"

extern void my_randomize(void);
extern void fill_type_check_table(void);

lua_State *WL;

char msg[1024*8];

void workerentry(void)
{
  debug("WORKERENTRY");
  lua_getglobal(globalL, "_SIMCO");
  WL = lua_tothread(globalL, 1);
}
void workerexit(void)
{
  debug("WORKEREXIT");
  WL = globalL;
}

void delete_object(root *obj)
{
   delete(obj);
}

#if 0
void start_lua(char *fname)
{
  int status;
  L = lua_open();
  WL = L;
  lua_gc(L, LUA_GCSTOP, 0);
  luaL_openlibs(L);
  lua_gc(L, LUA_GCRESTART, 0);
#if 0
  luaopen_base(L);
  luaopen_table(L);
  luaopen_string(L);
  luaopen_io(L);
  luaopen_debug(L);
  luaopen_loadlib(L);
#endif
  tolua_yats_open(L);
#if 1
  status = luaL_loadfile(L, fname);
  lua_getglobal(L, "yats_errorhandler");
  lua_insert(L, 1);  /* put it under chunk and args */
  if (status == 0){
    /* Compilation o.k. - execute */
    status = lua_pcall(L, 0, 0, 1); 
  } else {
    /* Compilation error - error string is on stack */
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  }
  lua_remove(L,1);
#else
  lua_dofile(L, fname);
#endif
  lua_close(L);
}
#endif
//////////////////////////////////////////////////////////////////////
// lua1out and luaxout: Lua Object base classes
static void callback_early(root *obj, event *ev, const char *cl)
{
  // Call a lua routine: _YATSEARLY(yatsobj, event)
  lua_pushstring(WL, "__YATSEARLY");
  lua_gettable(WL, LUA_GLOBALSINDEX);
  // yatsobj on stack
  tolua_pushusertype(WL, obj, cl);
  // ev on stack
  tolua_pushusertype(WL, ev, "event");
  // call the lua function
  lua_call(WL, 2, 0);
}

static void callback_late(root *obj, event *ev, const char *cl)
{
  // Call a lua routine: _YATSEARLY(yatsobj, event)
  lua_pushstring(WL, "__YATSLATE");
  lua_gettable(WL, LUA_GLOBALSINDEX);
  // yatsobj on stack
  tolua_pushusertype(WL, obj, cl);
  // ev on stack
  tolua_pushusertype(WL, ev, "event");
  // call the lua function
  lua_call(WL, 2, 0);
} 

static rec_typ callback_rec(root *obj, data *pd, int i, const char *cl)
{
  rec_typ rv;
  // Call a lua routine: _YATSEARLY(yatsobj, event)
  lua_pushstring(WL, "__YATSREC");
  lua_gettable(WL, LUA_GLOBALSINDEX);
  // yatsobj on stack
  tolua_pushusertype(WL, obj, cl);
  // pd on stack
  tolua_pushusertype(WL, pd, "data");
  // ev on stack
  lua_pushnumber(WL, i);
  // call the lua function
  lua_call(WL, 3, 1);
  // retrieve result from top of stack
  rv = (rec_typ) lua_tonumber(WL, -1);
  // Adjust the stack
  lua_pop(WL, 1);
  return rv;
} 
lua1out::lua1out(void)
{
  debug("Constructor: lua1out\n");
}

void lua1out::early(event *ev)
{
  callback_early(this, ev, "lua1out");
}

void lua1out::late(event *ev)
{
  callback_late(this, ev, "lua1out");
}

rec_typ lua1out::REC(data *pd, int i)
{
  return callback_rec(this, pd, i, "lua1out");
}

void luaxout::early(event *ev)
{
  callback_early(this, ev, "luaxout");
}

void luaxout::late(event *ev)
{
  callback_late(this, ev, "luaxout");
}

rec_typ luaxout::REC(data *pd, int i)
{
  return callback_rec(this, pd, i, "luaxout");
}

void write_log(const char *level, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsprintf(msg, fmt, ap);
  va_end(ap);                             // <empty>
  lua_pushstring(WL, "yats");             // 'yats'
  lua_gettable(WL, LUA_GLOBALSINDEX);     // yats
  lua_pushstring(WL, "clog");             // yats, 'clog'
  lua_gettable(WL, -2);                   // yats, clog
  lua_remove(WL, -2);                     // clog
  lua_pushstring(WL, level);              // clog, 'level'
  lua_gettable(WL, -2);                   // clog, func
  lua_insert(WL, -2);                     // func, clog
  lua_pushstring(WL, msg);                // func, clog, msg
  lua_call(WL, 2, 0);                     // <empty>
}

static const luaL_reg R[] = {
   {"yats", tolua_yats_open},
   {NULL, NULL}
};

LUALIB_API int luaopen_luayats_core(lua_State *L)
{
   // Some basic initializations
   my_randomize();
   data_classes();
   fill_type_check_table();

   luaL_openlib(L, "yats", R, 0);
   lua_pushliteral(L, "version");
   lua_pushliteral(L, "2.0");
   lua_settable(L, -3);
   return 1;
}


