/*************************************************************************
*
*		Luayats - Yet Another Tiny Simulator with Lua
*
**************************************************************************
*
*     Copyright (C) 1995-2005	Chair for Telecommunications
*				Dresden University of Technology
*				D-01062 Dresden
*				Germany
*                               H. Leuwer 
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
*       Modified:               2005 H. Leuwer (leu)
*
*************************************************************************/
#include "stdio.h"
#include "defs.h"

extern void my_randomize(void);
extern void fill_type_check_table(void);
extern int _main(int , char **);

int	main(int argc, char **argv)
{
  // Init r.n. generator
  my_randomize();		
  
  // Scan data type inheritance relations (defined in data.h)
  data_classes();
  
  // Fill table for data type check 
  fill_type_check_table();
  
  // Start the main program
  _main(argc, argv);
  return 0;
}

