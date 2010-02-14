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

/* STP PORT instance : 17.18, 17.15 */

#include "base.h"
#include "stpm.h"
#include "stp_in.h"

#include "rolesel.h" 
#include "portinfo.h"
#include "roletrns.h"
#include "sttrans.h"
#include "topoch.h"
#include "migrate.h"
#include "transmit.h"
#include "p2p.h"
#include "pcost.h"
#include "edge.h"
#include "portrec.h"
#include "brdec.h"

#include "stp_to.h" /* for STP_OUT_get_port_name & STP_OUT_get_port_link_status */
#include "rstp_bridge.h"

Bool portEnabled(PORT_T *self)
{
  if (self->adminEnable && self->macOperational)
    return True;
  else
    return False;
}

PORT_T * STP_port_create (STPM_T* stpm, int port_index)
{
  PORT_T*        self;
  UID_STP_PORT_CFG_T port_cfg;
  register int   iii;
  unsigned short port_prio;

  /* check, if the port has just been added */
  for (self = stpm->ports; self; self = self->next) {
    if (self->port_index == port_index) {
      return NULL;
    }
  }

  STP_NEW_IN_LIST(self, PORT_T, stpm->ports, "port create");

  self->owner = stpm;
  self->machines = NULL;
  self->port_index = port_index;
  self->port_name = strdup (STP_OUT_get_port_name (self->owner->rstp, port_index));
  self->uptime = 0;
#ifdef USELUA
  STP_OUT_get_init_port_cfg (stpm->rstp, stpm->vlan_id, port_index, &port_cfg);
#else
  STP_OUT_get_init_port_cfg (stpm->vlan_id, port_index, &port_cfg);
#endif
  port_prio =                  port_cfg.port_priority;
  self->admin_non_stp =        port_cfg.admin_non_stp;
  self->adminEdge =            port_cfg.admin_edge;
#if 0
  self->autoEdge =               port_cfg.auto_edge;
#endif
  self->adminPCost =           port_cfg.admin_port_path_cost;
  self->adminPointToPointMac = port_cfg.admin_point2point;
  
  self->LinkDelay = DEF_LINK_DELAY;
  self->port_id = (port_prio << 8) + port_index;

  iii = 0;
  self->timers[iii++] = &self->fdWhile;
  self->timers[iii++] = &self->helloWhen;
  self->timers[iii++] = &self->mdelayWhile;
  self->timers[iii++] = &self->rbWhile;
  self->timers[iii++] = &self->rcvdInfoWhile;
  self->timers[iii++] = &self->rrWhile;
  self->timers[iii++] = &self->tcWhile;
  self->timers[iii++] = &self->txCount;
  self->timers[iii++] = &self->lnkWhile;

  /* create and bind port state machines */
  STP_STATE_MACH_IN_LIST(topoch);

  STP_STATE_MACH_IN_LIST(migrate);

  STP_STATE_MACH_IN_LIST(p2p);

#if 0 /* leu: replaced by brdec */
  STP_STATE_MACH_IN_LIST(edge);
#endif                  

  STP_STATE_MACH_IN_LIST(pcost)

  STP_STATE_MACH_IN_LIST(info);
                  
  STP_STATE_MACH_IN_LIST(roletrns);

  STP_STATE_MACH_IN_LIST(sttrans);

  STP_STATE_MACH_IN_LIST(transmit);
                  
  STP_STATE_MACH_IN_LIST(portrec);  /* kd: new */

  STP_STATE_MACH_IN_LIST(brdec);    /* kd: new */

#ifdef STP_DBG
#if 0
  self->roletrns->ignoreHop2State = 14; /* DESIGNATED_PORT; */
  self->info->ignoreHop2State =      3; /* CURRENT */
  self->transmit->ignoreHop2State =  3; /* IDLE */
  self->edge->ignoreHop2State =      0; /* DISABLED; */
#endif

#ifdef STP_DBG_ALL
  self->info->debug = 1;
  self->pcost->debug = 1;
  self->p2p->debug = 1;
  self->edge->debug = 1;
  self->migrate->debug = 1;
  self->sttrans->debug = 1;
  self->topoch->debug = 1;
  self->roletrns->debug = 1;
  self->portrec->debug = 1;
  self->brdec->debug = 1;
  self->sttrans->debug = 1;
#endif

#endif
  return self;
}

void STP_port_init (PORT_T* self, STPM_T* stpm, Bool check_link)
{
  if (check_link) {
    self->adminEnable = STP_OUT_get_port_link_status (stpm->rstp, self->port_index);
    STP_VECT_create (&self->designPrio,
		     &stpm->BrId,
		     0,
		     &stpm->BrId,
		     self->port_id,
		     self->port_id);
    STP_copy_times (&self->designTimes, &stpm->rootTimes);
  }

  /* reset timers */
  self->fdWhile =
  self->helloWhen =
  self->mdelayWhile =
  self->rbWhile =
  self->rcvdInfoWhile =
  self->rrWhile =
  self->tcWhile =
  self->txCount = 0;

  self->msgPortRole = RSTP_PORT_ROLE_UNKN;
  self->selectedRole = DisabledPort;
  self->sendRSTP = True;
  self->operSpeed = STP_OUT_get_port_oper_speed (self->owner->rstp, self->port_index);
  self->p2p_recompute = True;
}

void STP_port_delete (PORT_T* self)
{
  STPM_T*                   stpm;
  register PORT_T*          prev;
  register PORT_T*          tmp;
  register STATE_MACH_T*    stater;
  register void*            pv;

  stpm = self->owner;

  free (self->port_name);
  for (stater = self->machines; stater; ) {
    pv = (void*) stater->next;
    STP_state_mach_delete (stater);
    stater = (STATE_MACH_T*) pv;
  }
                 
  prev = NULL;
  for (tmp = stpm->ports; tmp; tmp = tmp->next) {
    if (tmp->port_index == self->port_index) {
      if (prev) {
        prev->next = self->next;
      } else {
        stpm->ports = self->next;
      }
      STP_FREE(self, "stp instance");
      break;
    }
    prev = tmp;
  }
}

int STP_port_rx_bpdu (PORT_T* self, BPDU_T* bpdu, size_t len)
{
  STP_info_rx_bpdu (self, bpdu, len);
  return 0;
}

#ifdef STP_DBG
int STP_port_trace_state_machine (PORT_T* self, char* mach_name, int enadis, int vlan_id)
{
  register struct state_mach_t* stater;

  for (stater = self->machines; stater; stater = stater->next) {
    if (! strcmp (mach_name, "all") || ! strcmp (mach_name, stater->name)) {
      if (self->debug)
	stp_trace ("port %s on %s trace %-8s (was %s) now %s",
		   self->port_name, self->owner->name,
		   stater->name,
		   stater->debug ? " enabled" :"disabled",
		   enadis        ? " enabled" :"disabled");
      stater->debug = enadis;
    }
  }
  return 0;
}

void STP_port_trace_flags (char* title, PORT_T* self)
{
/* it may be opened for more deep debugging */
#ifdef STP_DBG_FLAGS 
  unsigned long flag = 0L;
  
  if (self->reRoot)   flag |= 0x000001L;
  if (self->sync)     flag |= 0x000002L;
  if (self->synced)   flag |= 0x000004L;
  
  if (self->proposed)  flag |= 0x000010L;
  if (self->proposing) flag |= 0x000020L;
  if (self->agreed)    flag |= 0x000040L;
  if (self->updtInfo)  flag |= 0x000080L;
  
  if (self->operEdge)   flag |= 0x000100L;
  stp_trace ("         %-12s: flags=0x%04lx port=%s", title, flag, self->port_name);
#endif
}

#endif
