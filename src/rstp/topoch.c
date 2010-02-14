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

/* Topolgy Change state machine : s.178 802.1D */
/*kd: s.178 802.1D*/

#include "base.h"
#include "stpm.h"
#include "stp_to.h" /* for STP_OUT_flush_lt */

#define STATES { \
  CHOOSE(INACTIVE),	\
  CHOOSE(LEARNING),	\
  CHOOSE(DETECTED),		\
  CHOOSE(NOTIFIED_TCN),		\
  CHOOSE(NOTIFIED_TC),	\
  CHOOSE(PROPAGATING),		\
  CHOOSE(ACKNOWLEDGED),	\
  CHOOSE(ACTIVE),	\
    }

#define GET_STATE_NAME STP_topoch_get_state_name
#include "choose.h"

#ifndef STRONGLY_SPEC_802_1W
/* 
 * In many kinds of hardware the function
 * STP_OUT_flush_lt is a) is very hard and b) cannot
 * delete learning emtries per port. The alternate
 * method may be used: we don't care operEdge flag here,
 * but clean learning table once for TopologyChange
 * for all ports, except the received port. I am ready to discuss :(
 * See below word STRONGLY_SPEC_802_1W
 */
#warning "STRONGLY_SPEC_802_1W NOT SET - ok"
#else
#warning "STRONGLY_SPEC_802_1W is SET - ok"
static Bool flush(STATE_MACH_T *self, char* reason) 
{
  register PORT_T* port = self->owner.port;
  Bool bret;
 if (port->operEdge) return True;
  if (self->debug) {
    stp_trace("%s (%s, %s, %s, '%s')",
        "flush", port->port_name, port->owner->name,
        LT_FLASH_ONLY_THE_PORT == port->owner->rstp->get_flushtype() ? "self port" : "other ports",
        reason);
  }

  bret = STP_OUT_flush_lt (port->owner->rstp, port->port_index, port->owner->vlan_id,
			   port->owner->rstp->get_flushtype(), reason);
}
#endif

#if 1 /* Do not differentiate between STP and RSTP */
#warning "Don't forget to implement dependency on stpVersion - see #else path"
static Bool fdbFlush(STATE_MACH_T *self, char* reason) 
{
  register PORT_T* port = self->owner.port;
  Bool bret;
#ifdef STP_DBG
  if (self->debug) {
    stp_trace("%s (%s, %s, %s, '%s')",
        "flush", port->port_name, port->owner->name,
        LT_FLASH_ONLY_THE_PORT == port->owner->rstp->get_flushtype() ? "self port" : "other ports",
        reason);
  }
#endif
  bret = STP_OUT_flush_lt (port->owner->rstp, port->port_index, port->owner->vlan_id,
			   port->owner->rstp->get_flushtype(), reason);
  return TRUE;
}

#else  /* Differentiate between STP and RSTP */

static Bool rstpVer (STATE_MACH_T* self)
{
  STPM_T *stpm = self->owner.stpm;
  Bool rstpVersion;
  if (stpm->ForceVersion >= 2) 
    rstpVersion =True;
  else
    rstpVersion = False;
  return rstpVersion;
	
}

static Bool stpVer(STATE_MACH_T* self)
{
  
  STPM_T *stpm = self->owner.stpm;
  Bool stpVersion;
  if (stpm->ForceVersion < 2) 
    stpVersion =True;
  else
    stpVersion = False;
  return stpVersion;
}

static Bool fdbFlush(STATE_MACH_T *self, char* reason)
{
  Bool del;
  register PORT_T* port = self->owner.port;
  
  if (rstpVer(self) == True) {
    del = STP_OUT_flush_lt (port->owner->rstp, port->port_index, port->owner->vlan_id,
			    port->owner->rstp->get_flushtype(), reason);
  }
  if (stpVer(self) == True) {
    del =  STP_OUT_flush_lt (port->owner->rstp, port->port_index, port->owner->vlan_id,
			     port->owner->rstp->get_flushtype(), reason);
  }
  return del;
}
#endif

/* 17.21.18 checked: leu */
static void setTcPropTree (STATE_MACH_T* self, char* reason) 
{
  register PORT_T* port = self->owner.port;
  register PORT_T* tmp;
  
  for (tmp = port->owner->ports; tmp; tmp = tmp->next) {
    if (tmp->port_index != port->port_index)
      tmp->tcProp = True;
  }

#ifndef STRONGLY_SPEC_802_1W
#ifdef DELETEME
#ifdef STP_DBG
  if (self->debug) {
    stp_trace("%s (%s, %s, %s, '%s')",
        "clearFDB", port->port_name, port->owner->name,
        "other ports", reason);
  }
#endif
  STP_OUT_flush_lt (port->owner->rstp, port->port_index, port->owner->vlan_id,
		    port->owner->rstp->get_flushtype(), reason);
#endif
#endif
}

/* 17.21.7 checked leu */
static unsigned int newTcWhile (STATE_MACH_T* self) 
{
  register PORT_T* port = self->owner.port;
  unsigned int retval;

  if (port->tcWhile == 0) {
    if (port->sendRSTP){
      retval = port->tcWhile = port->owner->rootTimes.HelloTime + 1;
      port->newInfo = True;
      return retval;
    } else {
      retval = port->tcWhile = port->owner->rootTimes.MaxAge + port->owner->rootTimes.ForwardDelay;
      return retval;
    }
  } else 
    return port->tcWhile;
}

void STP_topoch_enter_state (STATE_MACH_T* self)
{
  register PORT_T*      port = self->owner.port;

  switch (self->State) {
  case BEGIN:
  case INACTIVE:
#ifdef STRONGLY_SPEC_802_1W
    flush (self, "topoch INACTIVE");
#endif
    port->tcWhile = 0;
    fdbFlush(self, "topoch INACTIVE");  
    port->tcAck = False;
    break;

  case LEARNING:
    port->rcvdTc = False;
    port->rcvdTcn = False;
    port->rcvdTcAck = False;
    port->rcvdTc = False;
    port->tcProp = False;
    break;

  case ACTIVE:
    break;

  case DETECTED:
    newTcWhile (self);
#ifdef STP_DBG
    if (self->debug) 
      stp_trace("DETECTED: tcWhile=%d on port %s", 
		port->tcWhile, port->port_name);
#endif
    setTcPropTree (self, "DETECTED");
    port->newInfo = True;  
    break;

  case NOTIFIED_TC:
    port->rcvdTcn = port->rcvdTc = False;
    if (port->role == DesignatedPort) {
      port->tcAck = True;
    }
    setTcPropTree (self, "NOTIFIED_TC");
    break;

  case PROPAGATING:
    newTcWhile (self);
#ifdef STP_DBG
    if (self->debug) 
      stp_trace("PROPAGATING: tcWhile=%d on port %s", 
		port->tcWhile, port->port_name);
#endif
#ifdef STRONGLY_SPEC_802_1W
    flush (self, "topoch PROPAGATING");
#endif
    fdbFlush(self, "topoch PROPAGATING");
    port->tcProp = False;
    break;

  case ACKNOWLEDGED:
    port->tcWhile = 0;
#ifdef STP_DBG
    if (self->debug) 
      stp_trace("ACKNOWLEDGED: tcWhile=%d on port %s", 
		port->tcWhile, port->port_name);
#endif
    port->rcvdTcAck = False;
    break;

  case NOTIFIED_TCN:
    newTcWhile (self);
#ifdef STP_DBG
    if (self->debug) 
      stp_trace("NOTIFIED_TCN: tcWhile=%d on port %s", 
		port->tcWhile, port->port_name);
#endif
    break;
  };
}

Bool STP_topoch_check_conditions (STATE_MACH_T* self)
{
  register PORT_T*      port = self->owner.port;
  
  if (BEGIN == self->State) {
    return STP_hop_2_state (self, INACTIVE);
  }
  
  switch (self->State) {
  case INACTIVE:
    if (port->learn)
      return STP_hop_2_state (self, LEARNING);
    break;

  case LEARNING:
    if ((port->role == RootPort || port->role == DesignatedPort) && (port->forward && !port->operEdge)) 
      return STP_hop_2_state (self, DETECTED);
    if (port->rcvdTc || port->rcvdTcn || port->rcvdTcAck ||
	port->tcProp)
      return STP_hop_2_state (self, LEARNING);
    if ((port->role != RootPort) && (port->role != DesignatedPort) && !(port->learn || port->learning) 
	&& !(port->rcvdTc || port->rcvdTcn || port->rcvdTcAck || port->tcProp))
      return STP_hop_2_state (self, INACTIVE);
    break;

  case ACTIVE:
    if ((port->role != RootPort && (port->role != DesignatedPort)) || port->operEdge)
      return STP_hop_2_state (self, LEARNING);
    if (port->rcvdTcn)
      return STP_hop_2_state (self, NOTIFIED_TCN);
    if (port->rcvdTc)
      return STP_hop_2_state (self, NOTIFIED_TC);
    if (port->tcProp && !port->operEdge)
      return STP_hop_2_state (self, PROPAGATING);
    if (port->rcvdTcAck)
      return STP_hop_2_state (self, ACKNOWLEDGED);
    break;

  case DETECTED:
    return STP_hop_2_state (self, ACTIVE);

  case NOTIFIED_TC:
    return STP_hop_2_state (self, ACTIVE);

  case PROPAGATING:
    return STP_hop_2_state (self, ACTIVE);

  case ACKNOWLEDGED:
    return STP_hop_2_state (self, ACTIVE);

  case NOTIFIED_TCN:
    return STP_hop_2_state (self, NOTIFIED_TC);
  };

  return False;
}

