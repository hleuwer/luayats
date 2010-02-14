/*************************************************************************
*
*  Luayats - Yet Another Tiny Simulator 
*
**************************************************************************
*
*    Copyright (C) 1995-2005 
*    - Chair for Telecommunications
*      Dresden University of Technolog, D-01062 Dresden, Germany
*    - Marconi Ondata GmbH, D-71522 Backnang, Germany
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
*************************************************************************
*
*   Module author: Herbert Leuwer
*
*************************************************************************
*
*   Module description: Simple transparent bridge
*
*   See the Luayats documentation for a detailed description of the 
*   architecture.
*
*************************************************************************/
#ifndef ETH_BRIDGE_INCL
#define ETH_BRIDGE_INCL

#include "defs.h"
#include "inxout.h"
#include "mux.h"
#include "muxFrmPrio.h"

//tolua_begin
struct macentry {
   int valid;              // marks the entry to be valid
   int mc;                 // multicast entry if set to 1
   unsigned int portvec;   // the port vector
   tim_typ age;                // the age of the entry
   tim_typ birth;              // the birthday of the entry
   int locked;             // locked flag
};
typedef struct macentry macentry_t;

class filterdb {
public:
   filterdb(int ndb, int size, int agetime, root *node);
   ~filterdb();
   unsigned int lookup(int db, unsigned int mac, macentry_t *retval);
   void add(int db, unsigned int mac, unsigned int portvec, int age);
   void purge(int db, unsigned int mac);
   void flush(int db);
   void refresh(int db, unsigned int mac, int age);
   void agecycle(void);
   int numdb;       // number of data bases
   int numentries;  // total number of entries per database
   int agetime;
   //tolua_end
private:
   struct macentry **tab;
   root *node;
   
}; //tolua_export

typedef struct ethstats {
   int count;
   int uc;
   int mc;
   int bc;
   int fl;
} stats_t;

//tolua_begin
class ethbridge : public inxout {
   typedef inxout baseclass;
   
public:
   ethbridge(int nports);
   ~ethbridge();
   int act(void);
   rec_typ rec(data *pd, int portno);
   int agetime; // agetime in slots
   muxFrmPrio *getMux(int portno) {return out_mux[portno-1];}
   int setMux(int portno, muxFrmPrio *mx){out_mux[portno-1] = mx; return 1;}
   int getInCount(int portno) {return incount[portno-1].count;}
   int getInUCount(int portno) {return incount[portno-1].uc;}
   int getInMCount(int portno) {return incount[portno-1].mc;}
   int getInBCount(int portno) {return incount[portno-1].bc;}
   int getInFCount(int portno) {return incount[portno-1].fl;}
   int getOutCount(int portno) {return outcount[portno-1].count;}
   int getOutUCount(int portno) {return outcount[portno-1].uc;}
   int getOutMCount(int portno) {return outcount[portno-1].mc;}
   int getOutBCount(int portno) {return outcount[portno-1].bc;}
   int getOutFCount(int portno) {return outcount[portno-1].fl;}
   int numdb;
   int numentries;
   int numports;
   int numprios;
   int numinprios;
   filterdb *fdb;
//tolua_end
private:
   int *defprio;
   int *defvid;
   int *defdb;
   int *portcounter;
   stats_t *incount;
   stats_t *outcount;
   muxFrmPrio **out_mux;
   inpstruct *inp_buff;
}; // tolua_export
#endif
