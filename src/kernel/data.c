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
*	Oct 2, 1997		initialisation of embedded pointer
*				added in alloc_pool()
*					Matthias Baumann
*
*	Oct 29, 1997		exp_typ::calcIdx() added
*					Matthias Baumann
*
*************************************************************************/

/*
*	auxiliary things for data classes:
*		- the pointers for memory management,
*		  and the universal memory allocation routine itself
*
*	method exp_typ::calcIdx()
*/
#include "defs.h"
#include "data.h"

/************************************************************************/
/*
*	Memory management for the data classes:
*		a memory pool pointer for each data class
*/
data	*data::pool = NULL;
data	*cell::pool = NULL;
data	*cellPayl::pool = NULL;
data	*cellSeq::pool = NULL;
data	*frame::pool = NULL;
data	*tcpipFrame::pool = NULL;
data	*tcpAck::pool = NULL;
data	*rmCell::pool = NULL;
data	*aal5Cell::pool = NULL;
data	*frameSeq::pool = NULL;
data	*isaFrame::pool = NULL;
data	*dqdbSlot::pool = NULL;
data	*dmpduSeg::pool = NULL;

/************************************************************************/
/*
*	Class independent memory allocation of a bundle of objects:
*	Exclusion of the real memory allocation from inline new operator,
*	because (hopefully) seldom called
*/

void	*alloc_pool(
	size_t	siz,		// size of an object
	int	gran,		// # of objects to be allocated
	dat_typ	type)		// object type
{
	int	rel;
	char	*p;
	int	i;

	dprintf("alloc_pool(): allocating %d objects of type %s (size %d)\n",
	          	       gran, typ2str(type), (int) siz);

	if (siz % sizeof(char) != 0)
		errm0("internal error: alloc_pool()");
	rel = siz / sizeof(char);

	CHECK(p = new char[gran * rel]);
	for (i = 0; i < gran - 1; ++i)
	{	((data *)(p + i * rel))->next = (data *) (p + (i + 1) * rel);
		((data *)(p + i * rel))->type = type;
		((data *)(p + i * rel))->embedded = NULL;
	}
	((data *)(p + i * rel))->next = NULL;
	((data *)(p + i * rel))->type = type;
	((data *)(p + i * rel))->embedded = NULL;

	return	p;
}


/*
*	Check and shift (according to the diplacement) a given index into an array.
*
*	returns NULL on success, returns a pointer to an error message otherwise
*/

char	*exp_typ::calcIdx(
	int	*pi,		// the given index, here also the shifted one is returned
	int	level)		// the index level (first index: 0, second one: 1, ...)
{
	switch (addrtype) {
	case IntArray1:
	case DoubleArray1:
		if (level != 0)
		  return (char *) "internal error: exp_typ::getIdx(): bad level";
		break;
	case IntArray2:
	case DoubleArray2:
		if (level < 0 || level > 1)
		  return (char *) "internal error: exp_typ::getIdx(): bad level";
		break;
	default:return (char *) "internal error: exp_typ::getIdx(): bad addrtype";
	}

	if ( *pi < displacements[level] || *pi >= dimensions[level] + displacements[level])
	{	static	char	errm[100];
		sprintf(errm, "invalid index %d specified, expected: %d ... %d", *pi,
			displacements[level], dimensions[level] + displacements[level] - 1);
		return errm;
	}

	*pi -= displacements[level];
	return NULL;
}

char *mac2string(struct macaddr smac, char *cmac){
  *cmac = smac.mc + smac.ul*2 + ((smac.oui >> 16) & 0xfc);
   *(cmac + 1) = (smac.oui >> 8) & 0xff;
   *(cmac + 2) = smac.oui & 0xff;
   *(cmac + 3) = (smac.nic >> 16) & 0xff;
   *(cmac + 4) = (smac.nic >> 8) & 0xff;
   *(cmac + 5) = smac.nic & 0xff;
   return cmac;
}
struct macaddr mac2struct(char *cmac){
   struct macaddr smac;
   smac.mc = cmac[0] & 0x01;
   smac.ul = (cmac[0] >> 1) & 0x01;
   smac.oui = (cmac[0] << 16) + (cmac[1] << 8) + cmac[2];
   smac.nic = (cmac[3] << 16) + (cmac[4] << 8) + cmac[5];
   return smac;
}
char *maci2c(unsigned int mac, struct macaddr *smac){
   if (mac == 0xFFFFFFFFL){
      // broadcast
      smac->mc = 1;
      smac->ul = 0;
      smac->oui = 0xfcffff; 
   } else if (mac & 0x80000000L){
      // multicast
      smac->mc = 1;
      smac->ul = 0;
      smac->oui = ((~((unsigned int)(mac))) >> 16) & 0xffff;
      smac->nic = (~((unsigned int)(mac))) & 0xffff;
   } else {
      // unicast
      smac->mc = 0;
      smac->ul = 0;
      smac->oui = (((unsigned int) mac) >> 16) & 0xffff;
      smac->nic = ((unsigned int)(mac)) & 0xffff;
   }
   return NULL;
}
