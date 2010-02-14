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

/*
*	signalling source:
*
*	- sends (instantly) signalling messages to the given object
*
*	Signal <object to be signaled> <signals: (old_VCI,new_VCI,output_port), ...>
*/

#include "defs.h"

root	*Sig(void)
{
	WriteVciTabMsg	msg;
	char		*nam, *signame, *err;
	root		*r;

	signame = tval.nam;
	skip(CLASS);
	nam = read_suc(NULL);
	if ((r = find_obj(nam)) == NULL)
		syntax2s("%s: could not find object `%s'", signame, nam);

	for (;;)
	{	skip ('(');
		msg.old_vci = read_int(NULL);
		skip(',');
		msg.new_vci = read_int(NULL);
		skip(',');
		msg.outp = read_int(NULL);
		skip(')');
		if ((err = r->special( &msg, signame)) != NULL)
			syntax2s("could not signal, reason returned by `%s': %s", nam, err);
		if (token != ',')
			break;
		skip(',');
	}

	delete nam;

	return NULL;	//	do not create an object
}
