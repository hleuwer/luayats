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
*	A source which reads IATs from a given file (file format: binary).
*
*	Filesrc src: FILE="infile", { REPEAT=3, } { START=1000000, } {WAIT=1000000,} VCI=1, OUT=sink;
*			// in case of a given REPEAT, the file will be read accordingly often
*			// if START is given, then reading file begins at this IAT 
*			// if WAIT is given, then the first cell is emitted at WAIT + first_IAT
*
*	Implementation detail:
*	Instead of reading every IAT on its own, a buffer of size BUFSIZ is maintained. Although
*	also the stdio library uses buffering, we gain a double speed by avoiding the call of fread()
*	for each cell.
*/

#include "filsrc.h"

CONSTRUCTOR(Filsrc, filsrc);

void	filsrc::init(void)
{
	tim_typ	wait_time;

	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	fil_name = read_string("FILE");

	//	open file and determine its length
	if ((fil_fp = fopen(fil_name, "r")) == NULL)
		syntax2s("%s: could not open file \"%s\"", name, fil_name);
	fseek(fil_fp, 0, 2);	// File-Ende
	fil_len = ftell(fil_fp);
	if (fil_len < 1 || fil_len % sizeof(tim_typ) != 0)
		syntax2s("%s: \"%s\": bad file format", name, fil_name);
	fil_len /= sizeof(tim_typ);
	rewind(fil_fp);
	skip(',');

	//	repetitions wished?
	if (test_word("REPEAT"))
	{	rep_max = read_int("REPEAT");
		skip(',');
	}
	else	rep_max = 1;
	rep_cnt = 0;

	//	start value?
	if (test_word("START"))
	{	fil_pos = read_int("START");
		if (fil_pos >= fil_len)
			syntax1s("%s: START has to be smaller than the file size", name);
		skip(',');
	}
	else	fil_pos = 0;
	fseek(fil_fp, fil_pos * sizeof(tim_typ), 0);

	//	wait a little bit?
	if (test_word("WAIT"))
	{	wait_time = read_int("WAIT");
		skip(',');
	}
	else	wait_time = 0;

	vci = read_int("VCI");
	skip(',');

	// read additional parameters of derived classes
	addpars();

	output("OUT");

	CHECK(fil_buff = new tim_typ[BUFSIZ]);	// our own buffer

	if (wait_time == 0)
		//	begin immediatly
		this->rd_file();	// rd_file() comprises the first alarme()
	else	//	first wait for the specified time
		alarml( &std_evt, wait_time);
}

/*
*	Send a cell and read the next IAT
*/
void	filsrc::early(
	event	*)
{
	if ( ++counter == 0)
		errm1s("%s: overflow of Count", name);

	suc->rec(new cell(vci), shand);

	if (buff_ptr >= buff_ptr_max)
		this->rd_file();	// rd_file() calls alarme()
	else	alarme( &std_evt, *buff_ptr++);
}

/*
*	Waiting time expired.
*	Read first IAT
*/
void	filsrc::late(
	event	*)
{
	this->rd_file();	// rd_file() comprises the first alarme()
}


/*
*	Read the next BUFSIZ values into fil_buff,
*	register the source with alarme() (in case of data available)
*/
void	filsrc::rd_file(void)
{
	int	rd_len;

	if (fil_pos >= fil_len)
	{	if ( ++rep_cnt >= rep_max)
		{	fprintf(stderr, "%s: warning: stopped at SimTime = %d\n",
					name, SimTime);
			return;		// do not register again
		}
		fil_pos = 0;
		rewind(fil_fp);
	}

	if (BUFSIZ > fil_len - fil_pos)
		rd_len = fil_len - fil_pos;
	else	rd_len = BUFSIZ;
	if (fread(fil_buff, sizeof(tim_typ), rd_len, fil_fp) != (unsigned) rd_len)
		errm2s("%s: error reading file \"%s\"", name, fil_name);
	buff_ptr = fil_buff;
	buff_ptr_max = fil_buff + rd_len;
	fil_pos += rd_len;

	alarme( &std_evt, *buff_ptr++);
}

