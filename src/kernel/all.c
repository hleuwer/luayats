/*************************************************************************
*
*		YATS - Yet Another Tiny Simulator
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
*       Modified:               2005 H. Leuwer
*
*************************************************************************/

//
// A couple of error routines
//

#include "defs.h"
extern "C" {
#include "lua.h"
}
extern lua_State *WL;
static	void	header(void);
static	void	trailer(void);

char *strsave(char *str)
{
  char *s;
  
  CHECK(s = new char[strlen(str) + 1]);
  strcpy(s, str);
  return s;
}

char *strnsave(char *str,int len)
{
  char	*s;
  
  CHECK(s = new char[len + 1]);
  strncpy(s, str, len);
  s[len] = 0;
  return s;
}

void errm0(const char *m)
{
  header();
  lua_pushfstring(WL, "%s", m);
  trailer();
}

void errm1s(const char *m,	char *s1)
{
  header();
  lua_pushfstring(WL, m, s1);
  trailer();
}

void errm2s(const char *m, char *s1,char *s2)
{
  header();
  lua_pushfstring(WL, m, s1, s2);
  trailer();
}

void errm3s(const char *m, char *s1, char *s2, char *s3)
{
  header();
  lua_pushfstring(WL, m, s1, s2, s3);
  trailer();
}

void errm4s(const char *m,char *s1, char *s2, char	*s3, char *s4)
{
  header();
  lua_pushfstring(WL, m, s1, s2, s3, s4);
  trailer();
}

void errm5s(const char *m, char *s1, char *s2, char *s3, char *s4, char *s5)
{
  header();
  lua_pushfstring(WL, m, s1, s2, s3, s4, s5);
  trailer();
}

void errm1d(const char *m,	int d1)
{
  header();
  lua_pushfstring(WL, m, d1);
  trailer();
}

void errm2d(const char *m, int d1, int d2)
{
  header();
  lua_pushfstring(WL, m, d1, d2);
  trailer();
}

void errm1s1d(const char *m, char *s1, int	d1)
{
  header();
  lua_pushfstring(WL, m, s1, d1);
  trailer();
}

void errm1d1s(const char *m, int d1, char	*s1)
{
  header();
  lua_pushfstring(WL, m, d1, s1);
  trailer();
}

void errm1s2d(const char *m, char *s1, int	d1,int d2)
{
  header();
  lua_pushfstring(WL, m, s1, d1, d2);
  trailer();
}

void errm2s1d(const char *m, char	*s1, char *s2, int d2)
{
  header();
  lua_pushfstring(WL, m, s1, s2, d2);
  trailer();
}

void errm2s2d(const char *m, char *s1, char *s2, int d1, int d2)
{
  header();
  lua_pushfstring(WL, m, s1, s2, d1, d2);
  trailer();
}

static void trailer(void)
{
  lua_pushstring(WL, "\nNOTE: This is a Luayats internal error!\n      Depending on the error you probably better restart the application.");
  lua_concat(WL, 2);
  lua_error(WL);
}

static	void	header(void)
{
}

