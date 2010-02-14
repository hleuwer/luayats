/************************************************************************ 
 * RSTP library - Rapid Spanning Tree (802.1t, 802.1w) 
 * Copyright (C) 2001-2003 Optical Access 
 * Author: Alex Rozin 
 * 
 * This file is part of RSTP library. 
 * 
 * RSTP library is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License as published by the 
 * Free Software Foundation; version 2.1 
 * 
 * RSTP library is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser 
 * General Public License for more details. 
 * 
 * You should have received a copy of the GNU Lesser General Public License 
 * along with RSTP library; see the file COPYING.  If not, write to the Free 
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 
 * 02111-1307, USA. 
 **********************************************************************/

/* Generic (abstract) state machine : 17.13, 17.14 */

#include "base.h"
#include "statmch.h"

#if STP_DBG
#  include "stpm.h"
#endif

STATE_MACH_T * STP_state_mach_create (void (*concreteEnterState) (STATE_MACH_T*),
				      Bool (*concreteCheckCondition) (STATE_MACH_T*),
				      char *(*concreteGetStatName) (int),
				      void *owner, char *name)
{
  STATE_MACH_T *self;

  STP_MALLOC(self, STATE_MACH_T, "state machine");
 
  self->State = BEGIN;
  self->name = (char*) strdup (name);
  self->changeState = False;
#if STP_DBG
  self->debug = False;
  self->ignoreHop2State = BEGIN;
#endif
  self->concreteEnterState = concreteEnterState;
  self->concreteCheckCondition = concreteCheckCondition;
  self->concreteGetStatName = concreteGetStatName;
  self->owner.owner = owner;

  return self;
}
                              
void STP_state_mach_delete (STATE_MACH_T *self)
{
  free (self->name);
  STP_FREE(self, "state machine");
}

Bool STP_check_condition (STATE_MACH_T* self)
{
  Bool bret;

  bret = (*(self->concreteCheckCondition)) (self);
  if (bret) {
    self->changeState = True;
  }
  
  return bret;
}

int STP_enter_state(STATE_MACH_T *self)
{
  /* Perform state action if state has just changed */
  if (self->changeState){
    (*(self->concreteEnterState))(self);
    self->changeState = False;
    return 1;
  }
  return 0;
}
        
Bool STP_change_state (STATE_MACH_T* self)
{
  register int number_of_loops;

  for (number_of_loops = 0; ; number_of_loops++) {
    if (! self->changeState) return number_of_loops;
    (*(self->concreteEnterState)) (self);
    self->changeState = False;
    STP_check_condition (self);
  }

  return number_of_loops;
}

Bool STP_hop_2_state (STATE_MACH_T* self, unsigned int new_state)
{
#ifdef STP_DBG
  switch (self->debug) {
  case 0: break;
  case 1:
    if (new_state == self->State || new_state == self->ignoreHop2State) 
      break;
    stp_trace ("%-8s(%s-%s): %s=>%s",
	       self->name,
	       *self->owner.port->owner->name ? self->owner.port->owner->name : "Glbl",
	       self->owner.port->port_name,
	       (*(self->concreteGetStatName)) (self->State),
	       (*(self->concreteGetStatName)) (new_state));
    break;
  case 2:
    if (new_state == self->State) break;
    stp_trace ("%s(%s): %s=>%s", 
	       self->name,
	       *self->owner.stpm->name ? self->owner.stpm->name : "Glbl",
	       (*(self->concreteGetStatName)) (self->State),
	       (*(self->concreteGetStatName)) (new_state));
    break;
  }
#endif
  
  self->State = new_state;
  self->changeState = True;
  return True;
}

