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
*	History
*	Dec 8, 1997		read_suc(): MAC_ID also accepted
*					Matthias Baumann
*
*************************************************************************/

#include "defs.h"

/*
*	Root class: all methods (restim() and addpars() excluded) contain error meassages
*
*	Interface functions for easy parsing of the input text by constructors
*	and command methods -> see manual
*/
root::root(void)
{
  name = (char *) "<name: unknown>";
#ifdef	RECEIVE_DEBUG
	recDbgList = NULL;
#endif
}
root::~root()
{
  dprintf("%d root destruct: %s\n", SimTime, this->name);
}
void	root::init(void)
{
  errm0("internal error: no init() method defined for this class");
}

//	additional parameters of derived classes: initially empty
void	root::addpars(void) {}

int	root::command(
	char	*,
	tok_typ	*)
{
	return FALSE;
}

int	root::handle(
	char	*s,
	root	*r)
{
	errm3s("%s: do not have any inputs (input %s has been requested by %s)", name, s, r->name);
	return 1;
}

rec_typ	root::REC(	// REC is a macro normally expanding to rec (for debugging)
	data	*,
	int	)
{
	errm1s("%s: can't receive data, probably no rec() method defined", name);
	return ContSend;
}

void	root::connect(void)
{
	errm1s("%s: can't connect, probably no connect() method defined", name);
}

void	root::early(event *)
{
	errm1s("%s: can't run, probably no early() method defined", name);
}

void	root::late(event *)
{
	errm1s("%s: can't run, probably no late() method defined", name);
}

char	*root::special(
	specmsg	*,
	char	*)
{
  return (char *) "do not know what to do on special message";
}

int	root::export(
	exp_typ	*)
{
	return FALSE;
}

void	root::restim(void) {}

#ifdef	RECEIVE_DEBUG
rec_typ	root::recDbg(
	data	*pd,
	int	iKey,
	root	*obj)
{
	recDbgObj	*p;

	if (TimeType != EARLY)
		errm2s1d("internal error: %s: data object received during late slot phase\n"
				"\tsending object: %s (input key = %d)",
					name, obj->name, iKey);

	p = recDbgList;
	while (p != NULL && p->key != iKey)
		p = p->next;
	if (p == NULL)
	{	CHECK(p = new recDbgObj);
		p->next = recDbgList;
		recDbgList = p;
		p->time = SimTime;
		p->key = iKey;
	}
	else
	{	if (p->time == SimTime)
			errm2s1d("internal error %s: two data objects received during one time step\n"
				"\tsending object: %s (input key = %d)",
					name, obj->name, iKey);
		p->time = SimTime;
	}

	return this->recDbgOri(pd, iKey, (void *(*)()) NULL);
}
#endif	// RECEIVE_DEBUG

#ifdef	DATA_OBJECT_TRACE
rec_typ	root::recTrac(
	data	*pd,
	int	iKey,
	root	*obj)
{
	if (pd->traceOrigPtr != NULL)
		dprintf("%s: %s %d (%d)\n", name, pd->traceOrigPtr, pd->traceSeqNumber, SimTime);
	return this->recTracOri(pd, iKey, (void *(*)()) NULL);
}
#endif	// DATA_OBJECT_TRACE
	



/**********************************************************************/

/*
*	Description of interface functions: see manual
*/
#ifndef USELUA
void	skip_word(
	char	*s)
{
	if (s == NULL)
		return;
	if (test_word(s))
	{	if (tval.tok == UID)
			delete tval.val.s;
		scan();
		return;
	}
	syntax1s("`%s' expected", s);
}

int	test_word(
	char	*s)
{
	if (s == NULL)
		return 1;

	switch (token) {
	case UID:
		if (strcmp(tval.val.s, s) == 0)
			return 1;
		break;
	case NILVAR:
	case IVAR:
	case DVAR:
	case SVAR:
	case OBJ:
	case CLASS:
	case MAC_ID:
		if (strcmp(tval.nam, s) == 0)
			return 1;
		break;
	}
	return 0;
}


int	read_int(
	char	*key)
{
	extern	int	int_expr(void);

	if (key != NULL)
	{	skip_word(key);
		skip('=');
	}
	return int_expr();
}

double	read_double(
	char	*key)
{
	extern	double	double_expr(void);

	if (key != NULL)
	{	skip_word(key);
		skip('=');
	}
	return double_expr();
}

char	*read_string(
	char	*key)
{
	extern	char	*string_expr(void);

	if (key != NULL)
	{	skip_word(key);
		skip('=');
	}
	return string_expr();
}

char	*read_id(
	char	*key)
{
	extern	tok_typ	parse_id(char *);
	tok_typ	v, pos;

	if (key != NULL)
	{	skip_word(key);
		skip('=');
	}

	get_pos( &pos);
	v = parse_id("identifier expected");
	switch (v.tok){
	case NILVAR:
	case IVAR:
	case DVAR:
	case SVAR:
		set_pos( &pos);
		syntax1s("`%s' already has been defined as variable", v.nam);
	case OBJ:
		set_pos( &pos);
		syntax1s("`%s' already has been defined as object", v.nam);
	case UID:
		break;
	default:syntax0("identifier expected");
	}
	return v.val.s;
}


char	*read_suc(
	char	*key)
{
	extern	tok_typ	parse_id(char *);
	extern	char	*cat1(char *, char *);
	extern	char	*cat2(char *, char *);
	tok_typ	v;
	char	*s, *id;
	int	i;

	if (key != NULL)
	{	skip_word(key);
		skip('=');
	}

	id = "";	// because of the compiler warning: he does not "understand"
			// the if (i == 0)
	for (i = 0; i < 2; ++i)
	{	v = parse_id("identifier expected");
		switch (v.tok){
		case NILVAR:
		case IVAR:
		case DVAR:
		case SVAR:
		case OBJ:
		case MAC_ID:	// MACID added Dec. 1997, MBau
			s = strsave(v.nam);
			break;
		case UID:
			s = v.val.s;
			break;
		default:syntax();
			s = "";	// avoid compiler warning (actually not reached)
		}
		if (i == 0)
			id = s;
		else	id = cat2(id, s);

		if (token == PTR)
		{	id = cat1(id, "->");
			scan();
		}
		else	break;
	}

	return id;
}

char	*read_word(
	char	*key)
{
	char	*s;

	if (key != NULL)
	{	skip_word(key);
		skip('=');
	}

	switch (token){
	case NILVAR:
	case IVAR:
	case DVAR:
	case SVAR:
	case OBJ:
		s = strsave(tval.nam);
		break;
	case UID:
		s = tval.val.s;
		break;
	default:syntax();
		s = "";	// avoid compiler warning (actually not reached)
	}

	scan();

	return s;
}

static	tok_typ	*assign(
	char	*id)
{
	tok_typ	*pv;

	if ((pv = get_sym(id)) == NULL)
		return NULL;
	switch (pv->tok) {
	case SVAR:
		delete pv->val.s;
		// fall through
	case NILVAR:
	case IVAR:
	case DVAR:
		break;
	default:return NULL;
	}
	return pv;
}

int	set_int(
	char	*id,
	int	i)
{
	tok_typ	*pv;

	if ((pv = assign(id)) == NULL)
		return 0;
	pv->val.i = i;
	pv->tok = IVAR;
	return 1;
}

int	set_double(
	char	*id,
	double	d)
{
	tok_typ	*pv;

	if ((pv = assign(id)) == NULL)
		return 0;
	pv->val.d = d;
	pv->tok = DVAR;
	return 1;
}

int	set_string(
	char	*id,
	char	*s)
{
	tok_typ	*pv;

	if ((pv = assign(id)) == NULL)
		return 0;
	pv->val.s = strsave(s);
	pv->tok = SVAR;
	return 1;
}
#endif
