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
**************************************************************************
*
*	Module author:		Matthias Baumann, TUD
*	Creation:		1996
*
*	History:
*	July 2, 1997	- input aliasing introduced, changes in
*			  + command(): AliasInput(...) added
*			  + handle(): try to alias at the very beginning
*			  + new method resolveInputAlias()
*				Matthias Baumann
*	Oct 27, 1997	intArray2() and doubleArray2() added
*				Matthias Baumann
*	Dec 9, 1997	- bug fixes in resolveInputAlias() and handle(),
*			  such that memory leaks are avoided
*				Matthias Baumann
*
*************************************************************************/

/*
*	Class ino:
*	Generic class with arbitrarily many inputs.
*	- methods to create inputs
*	- methods for run-time data type checking (inline, in ino.h)
*	- implementation of the handle() method
*	- a command() method to read and reset a counter
*	- a basic routine to read output names.
*	- s standard event structure
*	- methods for exporting int / double Scalars and one-dimansional arrays
*	- export method exporting the address of the counter (IntScalar)
*
*
*
*	Create inputs (to be called in the constructor of a concrete object):
*	void	stdinp(void);
*		create an input with
*			- no name extension, input name equals object name
*			- input key 0
*	void	input(char *ext, int key);
*		create an input with
*			- the given name extension (ext == NULL -> no extension)
*			- the given input key (is lateron returned by handle())
*	void	inputs(char *ext, int ninp, int key_displ);
*		create ninp inputs with
*			- extensions <*ext>[inp_no]  (inp_no ranges from 1 to ninp)
*			- input keys (inp_no + key_displ)
*
*	Run-time data type check:
*	void	typecheck_i(data *pd, dat_typ typ, int key);
*		Checks whether the data type of *pd equals typ or is derived from typ.
*		In case of neither of them, an error message is generated, key (the value
*		received by rec()) is used to identify the input.
*	void	typecheck(data *pd, dat_typ typ);
*		Similar, but without the key parameter. Works fine only for objects with
*		one input.
*
*	Other services:
*	*	a general counter unsigned counter
*	*	a command() method to read and reset the counter, and to alias input names
*	*	an event struct std_evt (initialized by the constructor)
*
*	Basic routine for output name management (used by derived classes):
*	void	cr_outp(int index);
*		Read an output name from the input text and generate an entry in the
*		output name list.
*
*	More detailed:	manual (classes in1out and inxout, both derived from class ino)
*/


#include "ino.h"

//
// Constructor: set default values
//
ino::ino(void): std_evt(this, 0)
{	
  inp_list = NULL;
  outp_list = NULL;
  counter = 0;
  vci = NILVCI;
  
  outp_label.nam = NULL;	// mark that label not yet set
  
  inputAliasList = NULL;
}

ino::~ino(void) 
{
}

void ino::type_err(data	*pd, dat_typ type)
{
  int key;

  if (inp_list != NULL)	{
    if (inp_list->conn_vect == NULL)
      key = inp_list->key_displ;
    else 
      key = 1 + inp_list->key_displ;
  } else
    key = 0;

  this->type_err_i(pd, type, key);
}

void ino::type_err_i(data *pd, dat_typ type, int key)
{
  char	*inp;
  char	*pre;
  struct inp_t *pi;

  pi = inp_list;
  while (pi != NULL) {
    if (pi->conn_vect == NULL){
      if (pi->key_displ == key)
	break;
    } else {
      if (key >= 1 + pi->key_displ && key <= pi->ninp + pi->key_displ)
	break;
    }
    pi = pi->next;
  }
  if (pi == NULL)
    errm4s("an input of %s received data type `%s' - \n"
	   "\tdata type `%s' or derived type expected\n"
	   "*** additionally: inconsistent input list in %s",
	   name, typ2str(pd->type), typ2str(type), name);
  else {
    if (pi->conn_vect == NULL) {
      pre = pi->connect->name;
      if (pi->ext == NULL)
	inp = name;
      else {
	CHECK(inp = new char[strlen(name) + strlen(pi->ext) + 3]);
	sprintf(inp, "%s->%s", name, pi->ext);
      }
    } else {
      pre = pi->conn_vect[key - pi->key_displ - 1]->name;
      CHECK(inp = new char[strlen(name) + strlen(pi->ext) + 50]);
      // should be enough, since only [, ] and an integer number to be added
      sprintf(inp, "%s->%s[%d]", name, pi->ext, key - pi->key_displ);
    }
    errm4s("%s: object `%s' delivered data type `%s' - \n"
	   "\tdata type `%s' or derived type expected",
	   inp, pre, typ2str(pd->type), typ2str(type));
  }
}

// Eexport an integer scalar
int ino::intScalar( exp_typ *msg, const char *nam, int *p)
{
  if (strcmp(msg->varname, nam) == 0 && msg->ninds == 0) {
    msg->addrtype = exp_typ::IntScalar;
    msg->pint = p;
    return TRUE;
  }
  else
    return FALSE;
}

// Export an one-dimensional array of integers
int ino::intArray1( exp_typ *msg, const char *nam, int *p, int dim, int displ)		// displacement between index and x-value
{
  if (strcmp(msg->varname, nam) == 0 && msg->ninds == 0) {
    msg->addrtype = exp_typ::IntArray1;
    msg->pint = p;
    msg->dimensions[0] = dim;
    msg->displacements[0] = displ;
    return TRUE;
  } else
    return FALSE;
}

// Export a two-dimensional array of integers
int ino::intArray2(
		   exp_typ	*msg,
		   const char	*nam,
		   int	**pp,
		   int	dimX,		// x-dimension of the array
		   int	displX,		// displacement between x-index and x-value
		   int	dimY,		// y-dimension of the array
		   int	displY)		// displacement between y-index and y-value
{
  if (strcmp(msg->varname, nam) == 0 && msg->ninds == 0) {
    msg->addrtype = exp_typ::IntArray2;
    msg->ppint = pp;
    msg->dimensions[0] = dimX;
    msg->dimensions[1] = dimY;
    msg->displacements[0] = displX;
    msg->displacements[1] = displY;
    return TRUE;
  } else
    return FALSE;
}

// Export a double scalar
int	ino::doubleScalar(
			  exp_typ	*msg,
			  const char	*nam,
			  double	*p)
{
  if (strcmp(msg->varname, nam) == 0 && msg->ninds == 0){
    msg->addrtype = exp_typ::DoubleScalar;
    msg->pdbl = p;
    return TRUE;
  } else
    return FALSE;
}

// Export an one-dimensional array of double values
int ino::doubleArray1(
		      exp_typ	*msg,
		      const char	*nam,
		      double	*p,
		      int	dim,		// dimension of the array
		      int	displ)		// displacement between index and x-value
{
  if (strcmp(msg->varname, nam) == 0 && msg->ninds == 0){
    msg->addrtype = exp_typ::DoubleArray1;
    msg->pdbl = p;
    msg->dimensions[0] = dim;
    msg->displacements[0] = displ;
    return TRUE;
  } else
    return FALSE;
}

// Export a two-dimensional array of double values
int ino::doubleArray2(
		      exp_typ	*msg,
		      const char	*nam,
		      double	**pp,
		      int	dimX,		// x-dimension of the array
		      int	displX,		// displacement between x-index and x-value
		      int	dimY,		// y-dimension of the array
		      int	displY)		// displacement between y-index and y-value
{
  if (strcmp(msg->varname, nam) == 0 && msg->ninds == 0) {
    msg->addrtype = exp_typ::DoubleArray2;
    msg->ppdbl = pp;
    msg->dimensions[0] = dimX;
    msg->dimensions[1] = dimY;
    msg->displacements[0] = displX;
    msg->displacements[1] = displY;
    return TRUE;
  } else
    return FALSE;
}

// Export address of the counter
int ino::export(exp_typ	*arg)
{
  return baseclass::export(arg) ||
    intScalar(arg, "Count", (int *) &counter) ||
    intScalar(arg, "ival0", (int *) &ival0) ||
    intScalar(arg, "ival1", (int *) &ival1) ||
    intScalar(arg, "ival2", (int *) &ival2) ||
    intScalar(arg, "ival3", (int *) &ival3) ||
    intScalar(arg, "ival4", (int *) &ival4);
}

//
// try to alias an input name
// Return: old name if not found, orig name on success
//
// On success, a dynamic instance of the orig name is returned which
// probably never will be freed. This is not nice ...
//	-> bug fix Dec 9, 1997
//
char *ino::resolveInputAlias(char *s, int nam_len)
{
  inputAlias *pal;
  char *aliasName, *origName;

  if (s[nam_len] == '\0')
    // no input extension given
    aliasName = (char *) "";
  else if (strncmp(s + nam_len, "->", 2) == 0){
    aliasName = s + nam_len + 2;
    if (aliasName[0] == '\0')
      return s;	// strange ...
  } else
    return s;		// strange ...
  
  pal = inputAliasList;
  while (pal != NULL) {
    if (strcmp(pal->aliasName, aliasName) == 0){
      // we have found an alias entry.
      if (pal->origName[0] == '\0')
	// the orig name equals the object name
	return strsave(name);	// make a copy, such that handle() can free
      // the memory. Bug fix Dec 9, 1997
      else {
	CHECK(origName = new char[nam_len + strlen(pal->origName) + 3]);
	// 3: two chars for '->', one char terminating '\0'
	strcpy(origName, name);
	strcat(origName, "->");
	strcat(origName, pal->origName);
	
	return origName;
      }
    }
    pal = pal->next;
  }
  
  return s;	// nothing found
}
