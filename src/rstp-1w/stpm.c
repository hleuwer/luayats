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

 /* STP machine instance : bridge per VLAN: 17.17 */
 
#include "base.h"
#include "stpm.h"
#include "stp_to.h" /* for STP_OUT_flush_lt */
#include "rstp_bridge.h"

static int
_stp_stpm_init_machine (STATE_MACH_T* self)
{
  self->State = BEGIN;
  (*(self->concreteEnterState)) (self);
  return 0;
}

static int
_stp_stpm_iterate_machines (STPM_T* self,
                           int (*iter_callb) (STATE_MACH_T*),
                           Bool exit_on_non_zero_ret)
{
  register STATE_MACH_T* stater;
  register PORT_T*       port;
  int                    iret, mret = 0;

  /* state machines per bridge */
  for (stater = self->machines; stater; stater = stater->next) {
    iret = (*iter_callb) (stater);
    if (exit_on_non_zero_ret && iret)
      return iret;
    else
      mret += iret;
  }

  /* state machines per port */
  for (port = self->ports; port; port = port->next) {
    for (stater = port->machines; stater; stater = stater->next) {
      iret = (*iter_callb) (stater);
      if (exit_on_non_zero_ret && iret)
        return iret;
      else
        mret += iret;
    }
  }
  
  return mret;
}

static void
_stp_stpm_init_data (STPM_T* self)
{
  STP_VECT_create (&self->rootPrio,
                   &self->BrId,
                   0,
                   &self->BrId,
                   0, 0);

  self->BrTimes.MessageAge = 0;

  STP_copy_times (&self->rootTimes, &self->BrTimes);
}

static unsigned char
_check_topoch (STPM_T* self)
{
  register PORT_T*  port;
  
  for (port = self->ports; port; port = port->next) {
    if (port->tcWhile) {
      return 1;
    }
  }
  return 0;
}

void
STP_stpm_one_second (STPM_T* param)
{
  STPM_T*           self = (STPM_T*) param;
  register PORT_T*  port;
  register int      iii;

  if (STP_ENABLED != self->admin_state) return;

  for (port = self->ports; port; port = port->next) {
    for (iii = 0; iii < TIMERS_NUMBER; iii++) {
      if (*(port->timers[iii]) > 0) {
        (*port->timers[iii])--;
      }
    }    
    port->uptime++;
  }

  STP_stpm_update (self);
  self->Topo_Change = _check_topoch (self);
  if (self->Topo_Change) {
    self->Topo_Change_Count++;
    self->timeSince_Topo_Change = 0;
  } else {
    self->Topo_Change_Count = 0;
    self->timeSince_Topo_Change++;
  }
}

STPM_T*
STP_stpm_create (rstp_bridge *rstp, int vlan_id, char* name)
{
  STPM_T* self;

  STP_NEW_IN_LIST(self, STPM_T, rstp->bridges, "stp instance");
  self->rstp = rstp;
  self->admin_state = STP_DISABLED;
  
  self->vlan_id = vlan_id;
  if (name) {
    STP_STRDUP(self->name, name, "stp bridge name");
  }

  self->machines = NULL;
  self->ports = NULL;

  STP_STATE_MACH_IN_LIST(rolesel);

#ifdef STP_DBG
  /* self->rolesel->debug = 2;  */
#endif

  return self;
}

int
STP_stpm_enable (STPM_T* self, UID_STP_MODE_T admin_state)
{
  int rc = 0;

  if (admin_state == self->admin_state) {
    /* nothing to do :) */
    return 0;
  }

  if (STP_ENABLED == admin_state) {
    rc = STP_stpm_start (self);
    self->admin_state = admin_state;
  } else {
    self->admin_state = admin_state;
    STP_stpm_stop (self);
  }
  
  return rc;
}

void
STP_stpm_delete (STPM_T* self)
{
  register STPM_T*       tmp;
  register STPM_T*       prev;
  register STATE_MACH_T* stater;
  register PORT_T*       port;
  register void*         pv;
  register rstp_bridge *rstp = self->rstp;

  STP_stpm_enable (self, STP_DISABLED);
  
  for (stater = self->machines; stater; ) {
    pv = (void*) stater->next;
    STP_state_mach_delete (stater);
    self->machines = stater = (STATE_MACH_T*) pv;
  }

  for (port = self->ports; port; ) {
    pv = (void*) port->next;
    STP_port_delete (port);
    self->ports = port = (PORT_T*) pv;
  }
  prev = NULL;
  for (tmp = rstp->bridges; tmp; tmp = tmp->next) {
    if (tmp->vlan_id == self->vlan_id) {
      if (prev) {
        prev->next = self->next;
      } else {
        rstp->bridges=self->next;
      }
      
      if (self->name)
        STP_FREE(self->name, "stp bridge name");
      STP_FREE(self, "stp instance");
      break;
    }
    prev = tmp;
  }
}

int
STP_stpm_start (STPM_T* self)
{
  register PORT_T* port;

  if (! self->ports) { /* there are not any ports :( */
    return STP_There_Are_No_Ports;
  }

  if (! STP_compute_bridge_id (self)) {/* can't compute bridge id ? :( */
    return STP_Cannot_Compute_Bridge_Prio;
  }

  /* check, that the stpm has unique bridge Id */
  if (0 != STP_stpm_check_bridge_priority (self)) {
    /* there is an enabled bridge with same ID :( */
    return STP_Invalid_Bridge_Priority;
  }

  _stp_stpm_init_data (self);

  for (port = self->ports; port; port = port->next) {
    STP_port_init (port, self, True);
  }

#ifndef STRONGLY_SPEC_802_1W
  /* A. see comment near STRONGLY_SPEC_802_1W in topoch.c */
  /* B. port=0 here means: delete for all ports */
#ifdef STP_DBG
  stp_trace("%s (all, start stpm)",
        "clearFDB");
#endif

  STP_OUT_flush_lt (self->rstp, 0, self->vlan_id, LT_FLASH_ONLY_THE_PORT, "start stpm");
#endif

  _stp_stpm_iterate_machines (self, _stp_stpm_init_machine, False);
  STP_stpm_update (self);

  return 0;
}

void
STP_stpm_stop (STPM_T* self)
{
}

int
STP_stpm_update (STPM_T* self) /* returns number of loops */
{
  register Bool     need_state_change;
  register int      number_of_loops = 0;

  need_state_change = False; 
  
  for (;;) {/* loop until not need changes */
    need_state_change = _stp_stpm_iterate_machines (self,
                                                   STP_check_condition,
                                                   True);
    if (! need_state_change) return number_of_loops;

    number_of_loops++;
    /* here we know, that at least one stater must be
       updated (it has changed state) */
    number_of_loops += _stp_stpm_iterate_machines (self,
                                                  STP_change_state,
                                                  False);

  }

  return number_of_loops;
}

BRIDGE_ID *
STP_compute_bridge_id (STPM_T* self)
{
  register PORT_T* port;
  register PORT_T* min_num_port = 0;
  int              port_index = 0;

  for (port = self->ports; port; port = port->next) {
    if (! port_index || port->port_index < port_index) {
      min_num_port = port;
      port_index = port->port_index;
    }
  }

  if (! min_num_port) return NULL; /* IMHO, it may not be */

  STP_OUT_get_port_mac (self->rstp, min_num_port->port_index, self->BrId.addr);

  return &self->BrId;
}

STPM_T*
STP_stpm_get_the_list (rstp_bridge *rstp)
{
  return rstp->bridges;
}

void
STP_stpm_update_after_bridge_management (STPM_T* self)
{
  register PORT_T* port;

  for (port = self->ports; port; port = port->next) {
    port->reselect = True;
    port->selected = False;
  }
}
int
STP_stpm_check_bridge_priority (STPM_T* self)
{
  register STPM_T* oth;

  for (oth = self->rstp->bridges; oth; oth = oth->next) {
    if (STP_ENABLED == oth->admin_state && oth != self &&
        ! STP_VECT_compare_bridge_id (&self->BrId, &oth->BrId)) {
      if (self->vlan_id == oth->vlan_id)
	return STP_Invalid_Bridge_Priority;
      else
	return 0;
    }
  }
  return 0;
}

const char*
STP_stpm_get_port_name_by_id (STPM_T* self, PORT_ID port_id)
{
  register PORT_T* port;

  for (port = self->ports; port; port = port->next) {
    if (port_id == port->port_id) {
        return port->port_name;
    }
  }

  return "Undef?";
}





