/*************************************************************************
*
*		YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997	Chair for Telecommunications
*				Dresden University of Technology
*				D-01062 Dresden
*				Germany
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
#ifndef	_INO_H_
#define	_INO_H_

#include "defs.h"

//
// Input management structur
//
struct inp_t {
  char *ext;		// == NULL -> no extension
  root **conn_vect;	// vector of connection pointers
                        // == NULL for simple inputs
  int key_displ;	// simple inputs: input key
                        // otherwise:     input_key - input_number
  union {
    int	ninp;		// otherwise:     number of inputs
    root *connect;	// simple inputs: connection pointer
  };
  struct inp_t *next;
};

//
// Output management structur
//
struct outp_t	{
  char *name; // output name
  int index;  // index of the output in the output field
              // (used by class inxout)
  struct outp_t	*next;
};

//
// class ino
//
//tolua_begin
class ino: public root {
  typedef root baseclass;
  
public:	
  ino(void);
  ~ino(void);
//tolua_end
  int export(exp_typ *);
  int intScalar(exp_typ *, const char *, int *);
  int intArray1(exp_typ *, const char *, int *, int, int);
  int intArray2(exp_typ *, const char *, int **, int, int, int, int);
  int doubleScalar(exp_typ *, const char *, double *);
  int doubleArray1(exp_typ *, const char *, double *, int, int);
  int doubleArray2(exp_typ *, const char *, double **, int, int, int, int);

  // empty routines, because real implementations want to scan the old yats script
  // but excluded all the old parsing stuff. These routines will vanish once we
  // have removed all the init and connect routines in objects. Be patient :-).
  void input(char *, int){}
  void stdinp(void){}
  void inputs(char *, int, int){}
  void cr_outp(int){}

  void type_err(data *, dat_typ);
  void type_err_i(data *, dat_typ, int);
  
  inline void typecheck(data *pd, dat_typ type)
  {
    if (type_check_table[type][pd->type] == FALSE)
      type_err(pd, type);
  }

  inline void typecheck_i(data *pd, dat_typ type, int i)
  {
    if (type_check_table[type][pd->type] == FALSE)
      type_err_i(pd, type, i);
  }

  inline int typequery(data *pd, dat_typ type)
  {
    return type_check_table[type][pd->type];
  }

  inline void chkStartStop(rec_typ	x)
  {
    if (x != ContSend && x != StopSend)
      errm1s("%s: internal error: rec() method of successor did not "
	     "return neither ContSend nor StopSend", (char*) name);
  }
//tolua_begin
  inline unsigned int getCounter(void) {return counter;}
  inline void resCounter(void) {counter = 0;}
  inline void setCounter(unsigned int n) {counter = n;}
  inline unsigned int getVCI(void) {return vci;}
  inline void setVCI(int vci) {this->vci = vci;}

  unsigned int counter;	// a counter for common use
  unsigned int ival0;
  unsigned int ival1;
  unsigned int ival2;
  unsigned int ival3;
  unsigned int ival4;
  double dval0;
  double dval1;
  double dval2;
  double dval3;
  double dval4;
  int vci;	
  event std_evt;	// an event for common use
//tolua_end
  struct inp_t *inp_list;	// list of inputs
  struct outp_t	*outp_list;	// list of outputs

  tok_typ outp_label;	// place of the first output definition
                        // used for error messages

  struct inputAlias {
    char *origName;
    char *aliasName;
    inputAlias *next;
  };

  inputAlias *inputAliasList;		// list of input alias names
  char *resolveInputAlias(char *, int);// try to resolve input alias
}; //tolua_export

#endif	// _INO_H_
