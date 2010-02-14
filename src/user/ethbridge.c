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
*   We handle all frame handling in the context of data reception and
*   directly forward the frames to the output multiplexer.
*   See the Luayats documentation for a detailed description of the 
*   architecture.
*
*************************************************************************/

#include "ethbridge.h"
#include "sim.h"

#define NOW() SimTime
//
// Filtering Database FdB
//
// Constructor
filterdb::filterdb(int ndb, int size, int agetime, root *node){
   int i;
   numdb = ndb;
   numentries = size;
   CHECK(tab = new macentry_t*[ndb]);
   for (i=0; i < ndb; i++) {
      CHECK(tab[i] = new macentry[numentries]);
      flush(i);
   }
   agetime = agetime;
   node = node;
}

// Destructor
filterdb::~filterdb(){
   int i;
   for (i=0; i < numdb; i++){
      delete[] tab[i];
   }
   delete[] tab;
}

//
// Lookup
//
unsigned int filterdb::lookup(int db, unsigned int mac, macentry_t *retval)
{
   macentry_t *entry;
   entry = &tab[db][mac & 0x7FFFFFFF];
   if (entry->valid > 0) {
      // update age
      entry->age = entry->age - (NOW() - entry->birth);
      // too old?
      if (entry->age < 0){
	 // yes
	 entry->valid = 0;
	 return 0;
      }
      // no
      if (retval)
	 memcpy(retval, entry, sizeof(macentry_t));
      return entry->portvec;
   } else
      return 0;
}

//
// add an entry
//
void filterdb::add(int db, unsigned int mac, unsigned int portvec, int age)
{
   macentry_t *entry = &tab[db][mac & 0x7FFFFFFF];
   if (mac & 0x80000000L)
      entry->mc = 1;
   else 
      entry->mc = 0;
   entry->portvec = portvec;
   entry->valid = 1;
   entry->age = age;
   entry->birth = NOW();
}

//
// refresh age of an entry
//
void filterdb::refresh(int db, unsigned int mac, int age)
{
   macentry_t *entry = &tab[db][mac & 0x7FFFFFFF];
   if (!entry->valid)
      errm1s("%s: internal error: tried to refresh invalid mac address entry", node->name);
   entry->age = age;
   entry->birth = NOW(); 
}

//
// purge an entry
//
void filterdb::purge(int db, unsigned int mac)
{
   macentry_t *entry = &tab[db][mac & 0x7FFFFFFF];
   entry->age = 0;
   entry->birth = 0;
   entry->valid = 0;
}

//
// flush a single database
//
void filterdb::flush(int db)
{
   memset(tab[db], 0, numentries*sizeof(macentry_t));
}

void filterdb::agecycle()
{
   int i, j;
   macentry_t *entry;
   for (i=0; i < numdb; i++)
      for (j=0; j < numentries;j++){
	 entry = &tab[i][j];
	 if ((entry->valid > 0) && (entry->age > 0)){
	    entry->age -= NOW() - entry->birth;
	    if (entry->age < 0){
	       entry->valid = 0;
	    }
	 }
      }
}
//
// Constructor
//
ethbridge::ethbridge(int nports)
{
   numports = nports;
   // output multiplexers - need to create them here
   // in order to do initialization in Lua constructor
   CHECK(out_mux = new muxFrmPrio*[numports]);
   // create FdB
}

//
// Destructor
//
ethbridge::~ethbridge()
{
   delete[] inp_buff;
   delete[] out_mux;
   delete[] defprio;
   delete[] defvid;
   delete[] defdb;
   delete[] incount;
   delete[] outcount;
   delete fdb;
}

//
// Initializer
//
int ethbridge::act(void)
{
   int i;
   // input buffers
   CHECK(inp_buff = new inpstruct[numports]);
   CHECK(fdb = new filterdb(numdb, numentries, agetime, this));
   CHECK(defprio = new int[numports]);
   CHECK(defvid = new int[numports]);
   CHECK(defdb = new int[numports]);
   for (i = 0; i < numports; i++){
      defprio[i] = 0;
      defvid[i] = 1;
      defdb[i] = 0;
   }
   CHECK(incount = new stats_t[numports]);
   CHECK(outcount = new stats_t[numports]);
   memset(incount, 0, sizeof(stats_t)*numports);
   memset(outcount, 0, sizeof(stats_t)*numports);
   return 0;
}

//
// Receive handling
// - Note: we handle all the switching stuff here and
//         collect data at the input of the output mux
//
rec_typ ethbridge::REC(data *pd, int portno)
{
   unsigned int dmac, smac;
   unsigned int portvec;
   frame *pfn, *pf;
   int mcast = 0;
   int db = 0;
   int i;
   // input data check
   typecheck_i(pd, FrameType, portno);
   pf = (frame *) pd;
   dmac = pf->dmac;
   smac = pf->smac;

   // PRIORITY
   if (pf->tpid != 0x8100){
      // no vlan tag ==> default prio
      pf->prioCodePoint = defprio[portno];
   } else {
      // vlan tag ==> prio from frame
      pf->prioCodePoint = pf->vlanPriority;
   }

   // MAC DATABASE
   db = defdb[portno];
   if (db > (numdb - 1))
      errm1s("%s: internal error: invalid mac database index", name);

   // LEARN - source address lookup
   if ((smac & 0x80000000L) == 0) {
      incount[portno].uc++;
      // we learn only unicast addresses
      if (fdb->lookup(db, smac, NULL) == 0){
	 // no match found => learn this address: key = smac; val = db, port
	 fdb->add(db, smac, 1 << portno, agetime);
      } else {
	 // match found => update age counter
	 fdb->refresh(0, smac, agetime);
      }
   } else {
      if (smac == 0xFFFFFFFFL)
	 incount[portno].bc++;
      else
	 incount[portno].mc++;
   }

   if (smac == 0xFFFFFFFFL){
      portvec = (1 << numports) - 1;
   } else {
      // FORWARDING - destination address lookup
      if ((portvec = fdb->lookup(db, dmac, NULL)) == 0){
	 // no match found => flood
	 incount[portno].fl++;
	 portvec = (1 << numports) - 1;
      } 
   }

   // forward 
   for (i = 0; i < numports; i++){
      if (portvec & (1<<i)){
	 if (mcast == 0){
	    // first output port
	    // - simply forward
	    
	    out_mux[i]->rec(pd, i);
	    mcast++; // next needs data replication
	 } else {
	    // second to n-th output port:
	    // - duplicate and forward
	    mcast++;
	    pfn = new frame(pf->frameLen, pf->connID);
	    // copy all header fields
	    pfn->smac = pf->smac;
	    pfn->dmac = pf->dmac;
	    pfn->tpid = pf->tpid;
	    pfn->vlanId = pf->vlanId;
	    pfn->vlanPriority = pf->vlanPriority;
	    pfn->tpid2 = pf->tpid2;
	    pfn->vlanId2 = pf->vlanId2;
	    pfn->vlanPriority2 = pf->vlanPriority2;
	    pfn->dropPrecedence = pf->dropPrecedence;
	    pfn->prioCodePoint = pf->prioCodePoint;
	    pfn->sender = pf->sender;
	    // handle rest on output
	    out_mux[i]->rec(pfn, i);
	 }
      }
   }
   return ContSend;
}
