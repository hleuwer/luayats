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
*************************************************************************/

#include "defs.h"
#include "data.h"

/************************************************************************/
/*
*	Auxiliary functions for data class derivation and data type checking
*/

/*
*	Global table describing derivation relations
*/
int	type_check_table[_end_type][_end_type];

struct	deriv_t	{
	const char	*name;
	dat_typ	type;
	struct	deriv_t	*derived;	// list of derived classes
	struct	deriv_t	*next;		// next class in the same hierarchy level
					// (all derived from the same class)
	struct	deriv_t	*lin_list;	// linear list of all classes
};

//	class data is the root data class
static	struct	deriv_t	root_data_type = {"Data", DataType, NULL, NULL, NULL};

//	a linear list of all data classes:
static	struct	deriv_t	*lin_list = &root_data_type;

static	int	dat_typ_val = 1;

//	find_type(type, start)
//	search for the given type, start at the given point
//	in the class hierarchy
static	struct	deriv_t	*find_type(
	dat_typ	type,
	struct	deriv_t	*start)
{
	struct	deriv_t	*p;

	if (start->type == type)
		return start;

	start = start->derived;
	while (start != NULL)
	{	p = find_type(type, start);
		if (p != NULL)
			return p;
		start = start->next;
	}
	return	NULL;
}

//	declare type as to be derived from base
void	add_class(
	char	*name,
	dat_typ	type,
	dat_typ	base)
{
	struct	deriv_t	*p1, *p2;
	
	if (type != dat_typ_val++ )
		errm0("internal error: add_class():\n"
		"calling sequence in data_classes() does not correspond to enum dat_typ");
	p1 = find_type(base, &root_data_type);
	if (p1 == NULL)
		errm1s("internal error: add_class():\n"
			"could not find the base class type given for data class `%s'", name);
	CHECK(p2 = new struct deriv_t);
	p2->name = name;
	p2->type = type;
	p2->next = p1->derived;
	p1->derived = p2;
	p2->derived = NULL;

	p2->lin_list = lin_list;
	lin_list = p2;
}

//	is_derived()
//		returns 1	if delivered is the same type as expected,
//				or expected is a base class of delivered
//		returns 0	otherwise
static	int	is_derived(
	dat_typ	expect,
	dat_typ	deliv)
{
	struct	deriv_t	*p1;

	p1 = find_type(expect, &root_data_type);
	if (p1 == NULL)
	  errm1s("internal error: is_derived(): data type `%s' not found", (char *) typ2str(expect));
	if (find_type(deliv, p1) != NULL)
		return 1;
	return 0;
}

//	find the dat_typ representation of a data type given as string
dat_typ	str2typ(
	char	*s)
{
	struct	deriv_t	*p;

	p = lin_list;

	while (p != NULL)
	{	if (strcmp(p->name, s) == 0)
			return p->type;
		p = p->lin_list;
	}
	return UnknownType;
}

//	find the string representation of a data type given as dat_typ
char	*typ2str(dat_typ type)
{
  struct	deriv_t	*p;
  
  p = lin_list;
  
  while (p != NULL) {
    if (p->type == type)
      return (char*) p->name;
    p = p->lin_list;
  }
	return (char*) "UnknownType";
}

/*
*	Fill the global type check table
*/
void	fill_type_check_table(void)
{
	int	i, k;
	//	printf("%d %d\n", dat_typ_val, _end_type);
	if (dat_typ_val != _end_type)
		errm0("internal error: _end_type in enum dat_typ has not been set correctly");

	for (i = 0; i < _end_type; ++i)
		for (k = 0; k < _end_type; ++k)
			if (is_derived((dat_typ) k, (dat_typ) i))
			{	type_check_table[k][i] = TRUE;
//				fprintf(stderr, "`%s' is derived from `%s'\n",
//					typ2str((dat_typ)i), typ2str((dat_typ)k));
			}
			else	type_check_table[k][i] = FALSE;
}
