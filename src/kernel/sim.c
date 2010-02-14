/*************************************************************************
*
*  YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997 Chair for Telecommunications
*    Dresden University of Technology
*    D-01062 Dresden
*    Germany
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
* Module author:  Matthias Baumann, TUD
* Creation:  1996
*
* History:
* 24.10.96: for the EVENT_DEBUG mode, the usage of an event to be registered
*   is checked by means of a flag event::used. This flag is reset
*   in the event constructor. It is set with each event registration
*   (alarme/l(), eache/l(), and cleared with unalarme/l() and when
*   activating the event (only for event triggered activation).
*   These checks replace the older version, where always the event lists
*   have been went through to check on double event usage.
*    Matthias Baumann
*
* Oct 11, 1997 The Sim->Run statement now also accepts SLOTS and DOTS which
*   are not integer multiples of TIME_LEN. The dots are not very exact,
*   they are only printed at the end of TIME_LEN simulated slots.
*   Changes in command(), and in ResetTime().
*    Matthias Baumann
*
*       Oct 12, 2004    Adoption to Lua scripting - Herbert Leuwer
*************************************************************************/

/*
*/

/*
* Event manager: static calendar queue
*
* Time slots are divided into an early and a late phase.
*
* The functions to register for event triggered activation by the kernel (alarme() und alarml())
* are defined inline in defs.h.
*
* The lists of event entries are split into a lot of small lists (like in a
* hash table). The hash value of an entry which determines the list the entry is associated to,
* is given by the modulo of the desired activation time and the number of hash positions.
* Inside a given hash position, lists are unordered (adding of elements simply is
* performed at the head). In case we have enough hash positions (TIME_LEN is large
* enough), we will find only very few event entries looking ahead so far that they
* have to be omitted during the processing of the events of one hash position.
* The field time of the event structure is used to ensure unequivocality (?).
*
* The coordination of the event processing and unalarmx() is a little bit tricky.
* This is due to the fact that we want to be able to delete events registered for the
* _current_ time slot and _current_ phase.
* Before starting processing of the events of one hash position (one time slot and phase),
* we transfer the whole list (i.e. its head pointer) from the hash table position to
* pointer early_now (or late_now). Then we process this list step by step, everytimes
* actualizing early_now (or late_now) before calling the event routine of the object (by this,
* we ensure that an object can imediatly re-register using the same event structure).
* Events which are not destined for the current SimTime (they point into the future, where
* time_to_elapse_until_this_instant % TIME_LEN == 0), are directly put back to the
* hash table entry.
* Unalarmx() now has both lists available to go through: the entry of the hash table (determined
* by the time of the event to delete) and - if not found there - the early_now/late_now pointers.
*/

#include "defs.h"

//#define EVENT_LOG (0) // turn event logging on. The value determines the
// SimTime when to begin logging
#include "sim.h"

// Debugging flag - need to improve this to a full featured log
bool cdebug = false;

// hash tables for event lists
event *eventse[TIME_LEN];
event *eventsl[TIME_LEN];

// lists of objects registered for activation in each time slot
static event *timee = NULL;
static event *timel = NULL;

#ifdef EVENT_DEBUG 
// true if Run command is in execution
static int _sim_run_flag = FALSE;
#endif

// constructor of the simulator: initialisation of hash tables
sim::sim(void)
{
   int i;

   for (i = 0; i < TIME_LEN; ++i)
      eventse[i] = eventsl[i] = NULL;
   name = (char *) "Sim";
   SlotLength = 1e-6; // default slotlength of 1e-6 seconds
   SimTimeReal = 0.0;
}


sim _sim;

root *sim_ptr = &_sim;

tim_typ SimTime; // Simulation clock
double SlotLength; // length of a slot in seconds
double SimTimeReal;
int TimeType; // early or late phase?

static event *early_now = NULL;
static event *late_now = NULL;

static void ResetTime(void);

static int connect_flag = 0; // objects already connected?
int already_connected(void) { return connect_flag;} // used by parse.c::stat()

int flushevents(int del)
{
  int i,j;
  int cnt = 0;
  event *ev, *_ev;
  for (j = 0; j < 2; j++){
    for (i = 0; i < TIME_LEN; i++){
      ev = (j == 0) ? eventsl[i] : eventse[i];
      while (ev){
	cnt = cnt + 1;
	_ev = ev->next;
	if (del > 0) {
	  if ((ev->dyn == MAGICEVT) && (ev->dynchk == MAGICEVTCHK)){
	    dprintf("Dynamic event in timeslot %d: deleted.\n", i);
	    delete(ev);
	  } 
	  else if (ev->stat == 12345678) {
	    dprintf("Static event in timeslot %d: not deleted.\n", i);
	  } else {
	    dprintf("Unmarked event in timeslot %d: not deleted.\n", i);
	  }
	}
	ev = _ev;
      }
      if (j == 0)
	eventsl[i] = NULL;
      else
	eventse[i] = NULL;
    }
  }
  timee = NULL;
  timel = NULL;
  early_now = NULL;
  late_now = NULL;
  return cnt;
}
// register for activation in every early slot phase
void eache(
   event *e)
{
#ifdef EVENT_DEBUG
   check_evt(e, 1, Eache);
#endif

   e->next = timee;
   timee = e;
}

// register for activation in every late slot phase
void eachl(
   event *e)
{
#ifdef EVENT_DEBUG
   check_evt(e, 1, Eachl);
#endif

   e->next = timel;
   timel = e;
}

// try to delete an event from a list
static int del_evt(
   event *evt,  // the event
   event **pp) // the list
{
   while ( *pp != NULL)
   {
      if ( *pp == evt) {
         *pp = evt->next;
         return 1;
      }
      pp = & (*pp)->next;
   }
   return 0;
}

/*
* inactivate events - see comments above
*/
void unalarme(
   event *evt)
{
   if (evt->time < SimTime)
      errm1s("internal error: unalarme(): can't retract an activation request "
             "for a slot earlier than SimTime\n"
             "(has been requested by %s)", evt->obj->name);
   if (del_evt(evt, eventse + evt->time % TIME_LEN) ||
         del_evt(evt, &early_now)) {
#ifdef EVENT_DEBUG
      evt->used = FALSE;
#endif

      return ;
   }
   errm1s2d("internal error: unalarme(): could not find event\n"
            "\tobject name: %s\n\tevt->time: %u\n\tevt->key: %d\n",
            evt->obj->name, evt->time, evt->key);
}
void unalarml(
   event *evt)
{
   if (evt->time < SimTime)
      errm1s("internal error: unalarml(): can't retract an activation request "
             "for a slot earlier than SimTime\n"
             "(has been requested by %s)", evt->obj->name);
   if (del_evt(evt, eventsl + evt->time % TIME_LEN) ||
         del_evt(evt, &late_now)) {
#ifdef EVENT_DEBUG
      evt->used = FALSE;
#endif

      return ;
   }
   errm1s2d("internal error: unalarml(): could not find event\n"
            "\tobject name: %s\n\tevt->time: %u\n\tevt->key: %d\n",
            evt->obj->name, evt->time, evt->key);
}

/*
* reset the simulation clock
*/
static void ResetTime(void)
{
#ifndef USELUA
   extern void broadc_restim(void);
#endif
   int i;
   event *p;
   event **aux;
   static char errm[] = "ResetTime(): internal error: event list inconsistent";

   CHECK(aux = new event * [TIME_LEN]);
   for (i = 0; i < TIME_LEN; ++i)
      aux[i] = NULL;

   // first inform all objects
#ifndef USELUA
   broadc_restim();
#endif
   // then decrement activation times for all registered events,
   // and place events at new place in the calendar.
   for (i = 0; i < TIME_LEN; ++i) {
      p = eventse[i];
      while (p != NULL) {
         if (p->time < SimTime)
            errm0(errm);
         p->time -= SimTime;
         p = p->next;
      }
      // new place in the calendar
      p = eventse[i];
      if (p != NULL)
         aux[p->time % TIME_LEN] = p;
   }

   // write events back to the original calendar
   for (i = 0; i < TIME_LEN; ++i)
      eventse[i] = aux[i];

   // now the same for the late events
   for (i = 0; i < TIME_LEN; ++i)
      aux[i] = NULL;
   for (i = 0; i < TIME_LEN; ++i) {
      p = eventsl[i];
      while (p != NULL) {
         if (p->time < SimTime)
            errm0(errm);
         p->time -= SimTime;
         p = p->next;
      }
      // new place in the calendar
      p = eventsl[i];
      if (p != NULL)
         aux[p->time % TIME_LEN] = p;
   }
   for (i = 0; i < TIME_LEN; ++i)
      eventsl[i] = aux[i];

   // reset time
   SimTime = 0;
   SimTimeReal = 0;
   delete[] aux;
}

#ifdef EVENT_DEBUG
static char *getfunc(
   enum evt_dbg_enum how)
{
   switch (how) {
   case Alarme:
      return "alarme()";
   case Alarml:
      return "alarml()";
   case Eache:
      return "eache()";
   case Eachl:
      return "eachl()";
   }
   return "<unknown>";
}

// check whether an event structure is used twice
void check_evt(
   event *evt,
   tim_typ delta,
   enum evt_dbg_enum how)
{
   static char err[500];

   if (delta == 0 && _sim_run_flag == TRUE && !(TimeType == EARLY && how == Alarml)) { // If the simulator is not running, than delta == 0 is allowed.
      // Additionaly, zero delta may be used in the early phase to set a timer for
      // the late phase of the same slot.
      sprintf(err, "internal error: EVENT_DEBUG ***\n"
              "illegal event registration: DELTA == 0 while simulation running\n"
              "\tregistration with: %s\n"
              "\tobject name: %s\n"
              "\tcurrent SimTime: %u\n",
              getfunc(how), evt->obj->name, SimTime);
      errm0(err);
   }

   if (evt->used) {
      sprintf(err, "internal error: EVENT_DEBUG ***\n"
              "illegal event registration: DOUBLE USAGE of an event structure\n"
              "\tregistration with: %s\n"
              "\tobject name: %s\n"
              "\tcurrent SimTime: %u\n"
              "\told request for activation at: %u\n"
              "\tnew request for activation at: %u\n"
              "\tevent->key: %d\n",
              getfunc(how), evt->obj->name, SimTime, evt->time, SimTime + delta, evt->key);
      errm0(err);
   }

   evt->used = TRUE;
}
#endif

void sim::ResetTime_(void)
{
   ResetTime();
}
static int SimStopCommand;
// Stop current run of simulation
void sim::stop(void)
{
   SimStopCommand = 1;
}
void sim::reset(int reset){}
int sim::run(int slots){ return this->run(slots, slots - 1);}
int sim::run(int slots, int dots)
{
   event **pe, **plt;
   event **end_mark;
   event *p;
   int nSlots;
   int nDots;  // distance between two dots
   int nextDot; // when print next dot
   int dotsPerLine; // dots on the current line
   nSlots = slots;
   nDots = dots;
   nextDot = nSlots - nDots;
   dotsPerLine = 0;

#ifdef EVENT_DEBUG

   _sim_run_flag = TRUE;
#endif

   SimStopCommand = 0;
   // simulate the wished number of loops
   while ((nSlots > 0) && (SimStopCommand == 0)) { // determine which slots to simulate during this loop
      int aux;
      aux = SimTime % TIME_LEN; // where to start to simulate
      pe = eventse + aux;
      plt = eventsl + aux;
      aux = TIME_LEN - aux;  // how much to simulate at most
      if (nSlots > aux) {
         end_mark = pe + aux;
         nSlots -= aux;
      } else {
         end_mark = pe + nSlots;
         nSlots = 0;
      }

      // once over the calculated range of the calendar:
      while (pe < end_mark) { // early slot phase
         TimeType = EARLY;
         // event triggered activation: take event list
         if ((early_now = *pe) != NULL) { // mark list as empty
            *pe = NULL;
            // process now list entries
            // warning: during processing of one entry it is possible
            // that new events are generated (and they can be destined for
            // the same hash position!)
            do { // first delete the event from the chain in early_now:
               // it could be tried to delete it yet
               p = early_now;
               early_now = early_now->next;
               if (p->time == SimTime) { // the time is o.k. -> activate the object
#ifdef EVENT_DEBUG
                  p->used = FALSE;
#endif
#ifdef EVENT_LOG

                  if (SimTime >= EVENT_LOG) {
                     dprintf("%u: %s->earlyEvt(beg ... ", SimTime, p->obj->name);
                     fflush(stdout);
                  }
#endif
                  p->obj->early(p);
#ifdef EVENT_LOG

                  if (SimTime >= EVENT_LOG)
                     dprintf("end)\n");
#endif

               } else { // time is not o.k. -> leave the event in the list
                  // (pe points to the head of the right hash position)
                  p->next = *pe;
                  *pe = p;
               }
            } while (early_now != NULL);
         }
         ++pe;
         // activation in each time slot:
         if ((p = timee) != NULL) {
            do {
#ifdef EVENT_LOG
               if (SimTime >= EVENT_LOG) {
                  dprintf("%u: %s->earlyTim(beg ... ", SimTime, p->obj->name);
                  fflush(stdout);
               }
#endif
               p->obj->early(p);
#ifdef EVENT_LOG

               if (SimTime >= EVENT_LOG)
                  dprintf("end)\n");
#endif

            } while ((p = p->next) != NULL);
         }

         // now the same for the late slot phase
         TimeType = LATE;
         // event triggered activation:
         // do not load late_now earlier: otherwise, calling alarml(..., 0) in
         // the early phase (of the same slot) does not work
         if ((late_now = *plt) != NULL) {
            *plt = NULL;
            do {
               p = late_now;
               late_now = late_now->next;
               if (p->time == SimTime) {
#ifdef EVENT_DEBUG
                  p->used = FALSE;
#endif
#ifdef EVENT_LOG

                  if (SimTime >= EVENT_LOG) {
                     dprintf("%u: %s->lateEvt(beg ... ", SimTime, p->obj->name);
                     fflush(stdout);
                  }
#endif
                  p->obj->late(p);
#ifdef EVENT_LOG

                  if (SimTime >= EVENT_LOG)
                     dprintf("end)\n");
#endif

               } else {
                  p->next = *plt;
                  *plt = p;
               }
            } while (late_now != NULL);
         }
         ++plt;
         // each time slot
         if ((p = timel) != NULL) {
            do {
#ifdef EVENT_LOG
               if (SimTime >= EVENT_LOG) {
                  dprintf("%u: %s->lateTim(beg ... ", SimTime, p->obj->name);
                  fflush(stdout);
               }
#endif
               p->obj->late(p);
#ifdef EVENT_LOG

               if (SimTime >= EVENT_LOG)
                  dprintf("end)\n");
#endif

            } while ((p = p->next) != NULL);
         }

         if ( ++SimTime == 0)
            errm1s("%s: overflow of SimTime", name);

         SimTimeReal = SimTime * SlotLength; // issue: very expensive!
      }

      // write outstanding dots if DOTS != 0
      while (nSlots <= nextDot) {
         putchar ('.');
         nextDot -= nDots;
         if ( ++dotsPerLine >= 50) {
            dprintf("\t%u\n", SimTime);
            dotsPerLine = 0;
         }
         fflush(stdout);
      }
   }

   if (nDots != 0 && nextDot > 0)
      putchar ('\n');
   fflush(stdout);
   SimStopCommand = 0;
#ifdef EVENT_DEBUG

   _sim_run_flag = FALSE;
#endif

   return TRUE;
}
