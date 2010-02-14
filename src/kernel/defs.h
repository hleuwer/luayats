/*************************************************************************
*
*  YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997 Chair for Telecommunications
*    Dresden University of Technology
*    D-01062 Dresden
*    Germany
*
**************************************************************************
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*************************************************************************/

#ifndef _DEFS_H_
#define _DEFS_H_

//
// Global header file which contains information of the simulation kernel
// NOTE: It does not provide information about derived classes
//

//
// DATA_OBJECT_TRACE: enables data object tracing
//

// #define DATA_OBJECT_TRACE (1)


//
// If EVENT_DEBUG is defined, then the event manager checks that
//         a) no zero delta times are used when calling alarme() / alarml().
//     There are two exceptions:
//    - alarme/l( ..., 0) is legal if currently no Sim->Run is in progress
//      (e.g. during init() and command())
//    - calling alarml( ..., 0) during the early phase (registration for the
//      late phase of the same slot) is allowed
//         b) an event structure is not used twice by alarme(), alarml(), eache(), or eachl()
// 
//  Turning the EVENT_DEBUG on slows the simulator down by approx. 10 per cent.
// 

// #define EVENT_DEBUG 1

//
// If RECEIVE_DEBUG is turned on, then all calls of rec() methods are checked
// against two rules:
//  a) sending is only allowed during the early phase of a time step
//  b) only one data object may be transmitted per time step and input
//
// All violations cause an error message. This slows the simulator down by approx. 50 per cent.
//
// Usage of RECEIVE_DEBUG requires that all rec() methods are declared (in the class definition)
// and defined (code of the method body) using the macro name REC instead of the real name rec.
// Otherwise, syntax errors are generated during compilation.
// Calls of rec() methods, however, must *not* be changed. Otherwise, syntax errors are generated
// during compilation.
//

//#define RECEIVE_DEBUG 1

#ifdef DATA_OBJECT_TRACE
#define rec(x,y) recTrac((x), (y), this)
#define REC(x,y) recTracOri(x,y,void *(*)())
// The third parameter (void *(*)()) generates a syntax error if
// REC is used when calling a rec() method. A type name as argument is only
// allowed for method declaration and method definition (unused argument).
#else // DATA_OBJECT_TRACE

#ifdef RECEIVE_DEBUG
#define rec(x,y) recDbg((x),(y),this)
#define REC(x,y) recDbgOri(x,y,void *(*)())
// The third parameter (void *(*)()) generates a syntax error if
// REC is used when calling a rec() method. A type name as argument is only
// allowed for method declaration and method definition (unused argument).
#else // RECEIVE_DEBUG
#define REC(x,y) rec(x,y)
#endif // RECEIVE_DEBUG

#endif // DATA_OBJECT_TRACE

#ifdef EBZAW
#include <iostream.h>
#endif

//
// Standard headers
//
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>

//
// Be sure to have these defined.
//

#ifndef TRUE
#define TRUE (1)
#define FALSE (0)
#endif

//
// Debugging Output
//

#define DEBUG 1

#ifdef DEBUG
#define show(val, fmt) fprintf(stderr, #val " = %" #fmt "\n", val)
#define print(str) fprintf(stderr, #str "\n")
#else
#define show(val, fmt) /* void */
#define print(str) /* void */
#endif

//
// A simple Lua debug printf controlled via C or Lua
//
#define dprintf(fmt, ...) ((cdebug == true) ? printf(fmt, ## __VA_ARGS__) : 0)

//
// Macros enabling the use of a lua logger from C
// - Use only in non realtime parts of code - large overhead !!!
//
extern void write_log(const char *, const char *, ...);
#define fatal(fmt, ...) write_log("fatal", fmt, ## __VA_ARGS__);
#define error(fmt, ...) write_log("error", fmt, ## __VA_ARGS__);
#define warn(fmt, ...) write_log("warn", fmt, ## __VA_ARGS__);
#define info(fmt, ...) write_log("info", fmt, ## __VA_ARGS__);
#define debug(fmt, ...) write_log("debug", fmt, ## __VA_ARGS__);

//
// Yats wanted us to NOT use these. However, we need them for 
// some 'new' 'delete' overloading (e.g. events). Keep it commented out. 
//
#if 0
// please do not use any longer
#define malloc(x) PleaseDoNotUseMalloc
#define free(x)  PleaseDoNotUseFree
#endif

//
// Get dimension of an array.
//
#define DIM(x)  (sizeof(x) / sizeof((x)[0]))

//
// Macro to make new save
//
#define CHECK(x) ((x) == NULL ? errm1s("out of memory: %s", (char*) #x) : (void) 0)

//
// Make a string save by making a copy
//
char *strsave(char *);
char *strnsave(char *, int);

//
// Error messages with different arguments (without reference to the input text)
// Note: all of them go to a Lua error handler.
//tolua_begin
void errm0(const char *);
void errm1s(const char *, char *);
void errm2s(const char *, char *, char *);
void errm3s(const char *, char *, char *, char *);
void errm4s(const char *, char *, char *, char *, char *);
void errm5s(const char *, char *, char *, char *, char *, char *);
void errm1d(const char *, int);
void errm2d(const char *, int, int);
void errm1s1d(const char *, char *, int);
void errm1d1s(const char *, int, char *);
void errm1s2d(const char *, char *, int, int);
void errm2s1d(const char *, char *, char *, int);
void errm2s2d(const char *, char *, char *, int, int);
//tolua_end

// !!! TO DELETE !!!
// We keep them for those objects that still try to scan the script file.
// Once they are ported. We do not need these functions any more.
//
void syntax(void);
void syntax0(char *);
void syntax1s(char *, char *);
void syntax2s(char *, char *, char *);
void syntax3s(char *, char *, char *, char *);
void syntax4s(char *, char *, char *, char *, char *);
void syntax5s(char *, char *, char *, char *, char *, char *);
void syntax1s1d(char *, char *, int);
void syntax1d(char *, int);

/**************************************************************************/
/*
* Type declarations
*/

/*
* Type of the simulation clock
*/
//tolua_begin
typedef unsigned int tim_typ;

extern tim_typ SimTime; // simulation time
extern double SlotLength; // length of a slot in seconds
extern double SimTimeReal;
extern int TimeType; // early or late slot phase?
//tolua_end
/**************************************************************************/
// definition of common data classes
//
#define BASECLASS(cl) typedef cl _baseclass_
#define CLASS_KEY(key) enum {_cl_key_ = key}
#define NEW_DELETE(gran)\
 static data *pool;\
 inline void *operator new(size_t siz)\
  { extern void *alloc_pool(size_t, int, dat_typ);\
   data *pd;\
   if ((pd = pool) == NULL)\
    pd = pool = (data *) alloc_pool(siz, gran, (dat_typ) _cl_key_);\
   pool = pool->next;\
   return pd;\
  }\
 inline void operator delete(void *p)\
  {\
   ((data *)p)->next = pool;\
   pool = (data *)p;\
  }
#define CLONE(aClass)\
 virtual data *clone()\
 { data *pd;\
  pd = new aClass( *this);\
  if (embedded)\
   pd->embedded = embedded->clone();\
  return pd;\
 }
#define DATA_CLASS(cl, name)\
 { extern void add_class(char *, dat_typ, dat_typ);\
   add_class((char*)name, (dat_typ) cl::_cl_key_, (dat_typ) cl::_baseclass_::_cl_key_);	\
 }

class root;
#include "data.h"

#undef DATA_CLASS
#undef CLONE
#undef NEW_DELETE
#undef CLASS_KEY
#undef BASECLASS

extern char *typ2str(dat_typ); // convert a dat_typ to string
extern dat_typ str2typ(char *); // the other direction
extern int type_check_table[_end_type][_end_type];

/**************************************************************************/
// definition of the argument classes for the root::special()-method
// definition of the root::rec() return values
// definition of the root::export() argument structure
#include "special.h"

/*
* Root class
*/
typedef struct _t_t_ tok_typ;
class event;

//tolua_begin
class root {
public:
  root(void);  // init object name
  virtual ~root(void);
  //tolua_end
  virtual void init(void);  // read statement for object definition
  virtual void addpars(void);  // evaluation of parameters by derived classes
  virtual int command(char *, tok_typ *); // perform a command
  virtual void connect(void);  // connect to the next node
  virtual int handle(char *, root *); // return the key number of the given input name
  //tolua_begin
  virtual rec_typ REC(data *, int); // Receive data. REC is a macro normally expanding to rec.

  // Activation by the kernel:
  virtual void early(event *);  // first slot phase
  virtual void late(event *);  // second phase
  virtual void restim(void);  // information about reset of SimTime
  //tolua_end
  virtual char *special(specmsg *, char *);
  // all special things ...
  // return != NULL: error description

  virtual int export(exp_typ *); // export adresses of variables:
  // e.g. for graphical display
  //tolua_begin
  //  char *name;   // object name
  root *next;
  char *name;
  tim_typ time;
  //tolua_end
#ifdef DATA_OBJECT_TRACE

  rec_typ recTrac(data *, int, root *);
#endif // DATA_OBJECT_TRACE

#ifdef RECEIVE_DEBUG

  struct recDbgObj {
    int key;
    tim_typ time;
    recDbgObj *next;
  };
  recDbgObj *recDbgList;
  rec_typ recDbg(data *, int, root *);
#endif // RECEIVE_DEBUG

#ifdef USELUA
  //tolua_begin
  virtual int act(void) {
    return 0;
  }
  //tolua_end
#endif
}
; // end definition class root

#if 0
class block: public root {
  block() {}
  ~block() {}
}
;
#endif
/*
* Data structure for the event manager
*/
//tolua_begin
#define MAGICEVT    (0x22091991)
#define MAGICEVTCHK (0xccf6e66e)

class event {
public:
  // forbid uninitialized events
  inline event(root *o, int k) {
    obj = o;
    key = k;
#ifdef EVENT_DEBUG
    used = FALSE;
#endif

  }
  ~event() {}
  //tolua_end
  inline void *operator new(size_t siz) {
    event *p;
    CHECK(p = (event *)malloc(siz));
    p->dyn = MAGICEVT;
    p->dynchk = MAGICEVTCHK;
    p->next = NULL;
    return (void *) p;
  }
  inline void operator delete(void *p, size_t siz) {
    free(p);
  }
  //tolua_begin
  root *obj;  // the object to activate
  tim_typ time;  // the time of activation
  int key;  // tell different timers from each other
  event *next;  // for the event lists
  int stat;
  unsigned int dyn;
  unsigned int dynchk;
  //tolua_end
#ifdef EVENT_DEBUG

  int used;  // TRUE: event is currently used by the scheduler
#endif
}
; //tolua_export


/*
* Data structures for parser and scanner
*/
typedef struct _m_t_ mac_typ;
struct _t_t_ {
  int tok;   // token type
  char *nam;   // name
  struct _t_t_ *next;
  struct _t_t_ *ref;  // pointer to the genuine memory place, needed to make assignments
  // to variables
  unsigned long blockID; // block ID where symbol has been declared
  union {
    int i;  // interger value
    double d;  // double
    char *s;  // string
    mac_typ *m;  // pointer to macro data
    root *o;  // pointer to an object
    root *(*creat)(void);// pointer to the interface routine of a class constructor
  } val;
};

/*
* Structure to manage macros
*/

struct _m_t_ {
  tok_typ *args;  // argument names
  int argc;  // # of arguments
  tok_typ lab;  // starting point of the macro body
};

/**************************************************************************/
/*
* commonly usefull functions
*/
// symbol table
tok_typ *get_sym(char *, unsigned long i = 0, int k = TRUE);
tok_typ *def_sym(tok_typ *, int i = FALSE); // i == TRUE: define global variable
void era_sym(tok_typ *);
root *find_obj(char *);

// Scanner:
int scan(void);
void skip(int);
void get_pos(tok_typ *); // store the current position in the input text
void set_pos(tok_typ *); // set the position in the input text

// parsing of arguments of class constructors and commands
int int_expr(void);
double double_expr(void);
char *string_expr(void);
int read_int(char *);
double read_double(char *);
char *read_string(char *);
char *read_id(char *);
char *read_suc(char *);
int test_word(char *);
void skip_word(char *);
char *read_word(char *);
// assign values to variables, evaluate expressions
int set_int(char *, int);
int set_double(char *, double);
int set_string(char *, char *);
tok_typ eval(char *, char *);

// interface to the event manager
//tolua_begin
void unalarme(event *);
void unalarml(event *);
void eache(event *);
void eachl(event *);
//tolua_end
// Random Variables
#define USE_MY_RAND (1)

#ifdef USE_MY_RAND
int my_rand(void);
// included by Mue: 29.10.1999
double uniform(); // defined in geo1.c
void my_srand(int);
#else /* USE_MY_RAND */
extern "C" long int random(void);
extern "C" int srandom(int);
inline int my_rand(void) {
  return random();
}
inline void my_srand(int i) {
  (void) srandom(i);
}
#endif /* USE_MY_RAND */

#define rand()  PleaseUseMy_RandInstead
#define random() PleaseUseMy_RandInstead
#define srand(x) PleaseUseMy_SrandInstead
#define srandom(x) PleaseUseMy_SrandInstead

int get_geo1_handler(double);
tim_typ *get_geo1_table(int);
int geo1_rand(int);

/**************************************************************************/

// global variables
extern int token;  // next input token
extern tok_typ tval;  // its attributs

/**************************************************************************/
/*
* Token values:
* do not declare values >= 0 - the routine skip() displays the desired token
* as character if the token value > 0
*/
/* already declared identifiers and composed tokens like '->' */
#define IVAR (-1)  /* Variable, currently contains int */
#define DVAR (-2)  /* Variable, currently contains double */
#define SVAR (-3)  /* Variable, currently contains string */
#define IVAL (-4)  /* int value */
#define DVAL (-5)  /* double value */
#define SVAL (-6)  /* String */
#define MAC_ID (-7)  /* Macro identifier */
#define OBJ (-8)  /* object identifier */
#define CLASS (-9)  /* class id */
#define END (-10)  /* end ofd input */
#define UID (-11)  /* (yet) unknown identifier */
#define EQ (-12)  /* == */
#define NE (-13)  /* != */
#define LE (-14)  /* <= */
#define GE (-15)  /* >= */
#define PTR (-16)  /* -> */
#define AND (-17)  /* && */
#define OR (-18)  /* || */
#define NILVAR (-19)  /* noninitialised variable,

macro with unassigned return value, or
                             command without return value
                                                    (neither of them can't be used in expressions)
                                                     */
                                                     /* key words */
#define VAR (-100)
#define PRINT (-101)
#define IF (-102)
#define ELSE (-103)
#define FOR (-104)
#define TO (-105)
#define MACRO (-106)
#define FOREACH (-107)
#define WHILE (-108)
#define DEREF (-109)
#define STRING (-110)
#define INT (-111)
#define DOUBLE (-112)
#define SYSTEM (-113)
#define ENV (-114)
#define EXIT (-115)
#define POW (-116)
#define EXP (-117)
#define LOG (-118)
#define SQRT (-119)
#define SWITCH (-120)
#define CASE (-121)
#define DEFAULT (-122)
#define RAND (-123)
#define REF (-124)
#define LIT (-125)
#define GLOBAL (-126)


                                                     /**************************************************************************/
                                                     /*
                                                     * Other constants
                                                     */
#define TXT_MAX (70000)  // max length of input text (with includes)
                                                     //tolua_begin
#define TIME_LEN (100000)  // basic length for running the simulator

#define NILVCI (-1)  // common constant for an unknown VCI

                                                     // first and second slot phases
#define EARLY (1)
#define LATE (2)

#define RAND_MODULO (16384) // modulo value for masking the random number generator in geo1_rand()
                                                     // -> length of the transformation tables
                                                     //tolua_end

                                                     /**************************************************************************/
                                                     // Macro to easily define the interface routine of a class constructor
#define CONSTRUCTOR(func, cl) root *func(void)\
{ root *r;\
CHECK(r = new cl);\
return r;\
}

                                                     /**************************************************************************/
                                                     // Registration for events: inline
                                                     // registration for the first slot phase
#ifdef EVENT_DEBUG
                                                     enum evt_dbg_enum
                                                     { Alarme,
                                                     Alarml,
                                                     Eache,
                                                     Eachl
                                                     };
                                                     extern void check_evt(event *, tim_typ, enum evt_dbg_enum);
#endif
                                                     //tolua_begin
                                                     inline void alarme(
                                                     event *evt,
                                                     tim_typ delta)
                                                     {
                                                     event **e;
                                                     extern event *eventse[];

#ifdef EVENT_DEBUG
                                                     check_evt(evt, delta, Alarme);
#endif

                                                     e = eventse + ((evt->time = SimTime + delta) % TIME_LEN);
                                                     evt->next = *e;
                                                     *e = evt;
                                                     }

                                                     // registration for the second slot phase
                                                     inline void alarml(
                                                     event *evt,
                                                     tim_typ delta)
                                                     {
                                                     event **e;
                                                     extern event *eventsl[];

#ifdef EVENT_DEBUG
                                                     check_evt(evt, delta, Alarml);
#endif

                                                     e = eventsl + ((evt->time = SimTime + delta) % TIME_LEN);
                                                     evt->next = *e;
                                                     *e = evt;
                                                     }
                                                     //tolua_end

                                                     // For usage in source code files for user defined object classes.
                                                     // Make config copies the lines to src/kernel/class.c, where this macro is overriden.
#define USERCLASS(xxx,yyy) /* void */

                                                     extern bool cdebug;
#endif // _DEFS_H_
