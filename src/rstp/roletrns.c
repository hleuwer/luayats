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

/* Port Role Transitions state machine : 17.24 */

/*kd:
Diese Teil impliziert die "Port Role Transitions state machine".
Der Unterscied zwischen alten und neuen Standert impliziert die verschiedene Realisierung "Alternate and Backup ports.
Andere VerschiedenHeiten sind nicht so wichtig */


#define this _this 
#include "base.h"

#include "stpm.h"

/*kd: hinzufügte die neue Funktionen, die im 801.2 benützen*/
#define STATES {\
   CHOOSE(DISABLE_PORT),  \
   CHOOSE(DISABLED_PORT), \
   CHOOSE(DESIGNATED_DISCARD),  \
   CHOOSE(ALTERNATE_PROPOSED), \
   CHOOSE(ALTERNATE_AGREED),  \
   CHOOSE(ALTERNATE_PORT), \
   CHOOSE(BLOCK_PORT), \
   CHOOSE(INIT_PORT),       \
   CHOOSE(BLOCKED_PORT),    \
   CHOOSE(BACKUP_PORT),     \
   CHOOSE(ROOT_PROPOSED),   \
   CHOOSE(ROOT_AGREED),     \
   CHOOSE(REROOT),      \
   CHOOSE(ROOT_PORT),       \
   CHOOSE(REROOTED),        \
   CHOOSE(ROOT_LEARN),      \
   CHOOSE(ROOT_FORWARD),    \
   CHOOSE(DESIGNATED_PROPOSE),  \
   CHOOSE(DESIGNATED_SYNCED),   \
   CHOOSE(DESIGNATED_RETIRED),  \
   CHOOSE(DESIGNATED_PORT), \
   CHOOSE(DESIGNATED_LISTEN),   \
   CHOOSE(DESIGNATED_LEARN),    \
   CHOOSE(DESIGNATED_FORWARD),  \
}

#define GET_STATE_NAME STP_roletrns_get_state_name
#include "choose.h"

#define MigrateTime 3 


/* 17.21.14 checked: leu */
static void
setSyncTree (STATE_MACH_T *this)
{
  register PORT_T* port;

  for (port = this->owner.port->owner->ports; port; port = port->next) {
    port->sync = True; 
  }
}

/* 17.21.15 checked: leu */
static void
setReRootTree (STATE_MACH_T *this)
{
  register PORT_T* port;

  for (port = this->owner.port->owner->ports; port; port = port->next) {
    port->reRoot = True; 
  }
}

#ifdef DELETEME
static Bool
compute_all_synced (PORT_T* this)
{
  register PORT_T* port;

  for (port = this->owner->ports; port; port = port->next) {
    if (port->port_index == this->port_index) continue;
    if (! port->synced) {
        return False;
    }
  }
  return True;
}
#else
/* 17.20.3 checked: leu */
static Bool compute_allSynced (PORT_T* this)
{
  register PORT_T* port;

  for (port = this->owner->ports; port; port = port->next) {
#if 0
    if (port->port_index == this->port_index) continue;
#endif
    if (!port->selected || 
	(port->role != port->selectedRole) ||
	(!port->synced && (port->role != RootPort))){
        return False;
    }
  }
  return True;
}
#endif

/* 17.20.10 checked: leu */
static Bool 
compute_reRooted (PORT_T* this)
{
  register PORT_T* port;

  for (port = this->owner->ports; port; port = port->next) {
    if (port->port_index == this->port_index) continue;
    if (port->rrWhile) {
      return False;
    }
  }
  return True;
}

/***************************************************************/
static unsigned short
forward_del (PORT_T* this)
{
	register STPM_T* stpm;
	register unsigned short retval;
	stpm = this->owner;
	if (this->sendRSTP == True) {
        	retval = stpm->rootTimes.HelloTime; 
	} else {
		retval = stpm->rootTimes.ForwardDelay; 
	}
	return retval;
}
	


/****************************************************************/
static Bool
edgedel (PORT_T* this)
{
	register STPM_T* stpm;
	register unsigned short EdgeDelay;
	stpm = this->owner;
	if (this->operPointToPointMac == True)
	{
		EdgeDelay = MigrateTime;
	}
	else
	{
		EdgeDelay = stpm->rootTimes.ForwardDelay;
	}
	return EdgeDelay;
}



/*****************************************************************/
static Bool
rstpVer (STATE_MACH_T* self)
{

    STPM_T *stpm = self->owner.stpm;
	Bool rstpVersion;
	if (stpm->ForceVersion >= 2) 
		rstpVersion =True;
	else
		rstpVersion = False;
	return rstpVersion;
	
}


/*****************************************************************/

void
STP_roletrns_enter_state (STATE_MACH_T* this)
{
  register PORT_T*           port = this->owner.port;
  register STPM_T*           stpm;
  
  stpm = port->owner;
  
  switch (this->State) {
  case BEGIN:
  case INIT_PORT:
#if 1 /* due 802.1y Z.4 */
    port->role = DisabledPort;
#else
    port->role = port->selectedRole = DisabledPort;
    port->reselect = True;
#endif
    port->learn = False;/*v INIT_PORT 802.1D */
    port->forward = False; /*v INIT_PORT 802.1D */
    port->synced = False; /* in INIT */
    port->sync = True; /* in INIT */
    port->reRoot = True; /* in INIT_PORT */
    port->rrWhile = stpm->rootTimes.ForwardDelay;
    port->fdWhile = stpm->rootTimes.MaxAge; /*kd: Zeile ist geändert. s.175 (Disabled Port Role)*/
    port->rbWhile = 0; 
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("after init", port);
#endif
    break;
    /*case BLOCK_PORT:*/ /*kd: Im neu ZustandMaschine das algoritm ist die andere. s 175. 801.2D*/
  case DISABLE_PORT:
    port->role = port->selectedRole;
    port->learn = False;
    port->forward = False;
    break;
  case DISABLED_PORT:                 /*kd: s 175 801.2D*/
    port->fdWhile = stpm->rootTimes.MaxAge;
    port->synced = True; /* In BLOCKED_PORT */
    port->rrWhile = 0;
    port->sync = False; 
    port->reRoot = False; /* BLOCKED_PORT */
    break;
    
    /* 17.23.2 */
  case ROOT_PROPOSED:
    /* setSyncBridge (this);*/
    setSyncTree (this); /*kd: Die Zeile ist aus 802.1D*/
    port->proposed = False;
#ifdef STP_DBG
    if (this->debug) 
      STP_port_trace_flags ("ROOT_PROPOSED", port);
#endif
    break;
  case ROOT_AGREED:
    port->proposed = False;
    port->sync = False; /* in ROOT_AGREED */
    port->agree = True; /* In ROOT_AGREED */  /*kd: aus 801.2D*/
    port->newInfo = True;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("ROOT_AGREED", port);
#endif
    break;
  case REROOT:
    /* setReRootBridge (this);*/ 
    setReRootTree (this);
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("REROOT", port);
#endif
    break;
  case ROOT_PORT:
    port->role = RootPort;
    port->rrWhile = stpm->rootTimes.ForwardDelay;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("ROOT_PORT", port);
#endif
    break;
  case REROOTED:
    port->reRoot = False; /* In REROOTED */
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("REROOTED", port);
#endif
    break;
  case ROOT_LEARN:
    port->fdWhile = stpm->rootTimes.ForwardDelay;
    port->learn = True;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("ROOT_LEARN", port);
#endif
    break;
  case ROOT_FORWARD:
    port->fdWhile = 0;
    port->forward = True;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("ROOT_FORWARD", port);
#endif
    break;
    
    /* 17.23.3 */
  case DESIGNATED_PROPOSE:
    port->proposing = True; /* in DESIGNATED_PROPOSE */
    port->edgeDelayWhile = edgedel(port);/*Die neue Zeile aus 801.2D*/
    port->newInfo = True;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("DESIGNATED_PROPOSE", port);
#endif
    break;
  case DESIGNATED_SYNCED:
    port->rrWhile = 0;
    port->synced = True; /* DESIGNATED_SYNCED */
    port->sync = False; /* DESIGNATED_SYNCED */
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("DESIGNATED_SYNCED", port);
#endif
    break;
  case DESIGNATED_RETIRED:
    port->reRoot = False; /* DESIGNATED_RETIRED */
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("DESIGNATED_RETIRED", port);
#endif
    break;
  case DESIGNATED_PORT:
    port->role = DesignatedPort;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("DESIGNATED_PORT", port);
#endif
    break;
    /* case DESIGNATED_LISTEN:*/ 
  case DESIGNATED_DISCARD: 						/*kd: 801.2D s 176*/
    port->learn = False;
    port->forward = False;
    port->disputed = False;
    port->fdWhile = stpm->rootTimes.ForwardDelay;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("DESIGNATED_DISCARD", port);
#endif
    break;
  case DESIGNATED_LEARN:
    port->learn = True;
    port->fdWhile = stpm->rootTimes.ForwardDelay;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("DESIGNATED_LEARN", port);
#endif
    break;
  case DESIGNATED_FORWARD:
    port->forward = True;
    port->agreed = port->sendRSTP; /*kd: Die neue Zeile aus 801.2D.*/
    port->fdWhile = 0;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("DESIGNATED_FORWARD", port);
#endif
    break;
    
    
    /*kd: Die neue Teil für Alternate und Backup Port roles*/
    /**********************************************/
    
    
  case ALTERNATE_PROPOSED:
    setSyncTree (this); 					/*kd: 801.2D s 177 Alternate and Backup Port roles*/
    port->proposed = False;
#ifdef STP_DBG
    if (this->debug) 
      STP_port_trace_flags ("ALTERNATE_PROPOSED", port);
#endif
    break;
  case ALTERNATE_AGREED:
    port->proposed = False;
    port->agree = True; 
    port->newInfo = True; 
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("ALTERNATE_AGREED", port);
#endif
    break;
  case ALTERNATE_PORT:
    port->fdWhile = forward_del(port);
    port->synced = True;
    port->rrWhile = 0;
    port->sync = False;
    port->reRoot = False;
#ifdef STP_DBG
    if(this->debug)
      STP_port_trace_flags ("ALTERNATE_PORT", port);
#endif
    break;
  case BLOCK_PORT:
    port->role = port->selectedRole;
    port->learn = False;
    port->forward = False;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("BLOCK_PORT", port);
#endif
    break;
    
  case BACKUP_PORT:
    port->rbWhile = 2 * stpm->rootTimes.HelloTime;
#ifdef STP_DBG
    if (this->debug)
      STP_port_trace_flags ("BACKUP_PORT", port);
#endif
    break;
  }
}

Bool
STP_roletrns_check_conditions (STATE_MACH_T* this)
{
  register PORT_T           *port = this->owner.port;
  register STPM_T           *stpm;
  Bool                      allSynced;
  Bool                      reRooted;
  
  stpm = port->owner;
  
  if (BEGIN == this->State) {
    return STP_hop_2_state (this, INIT_PORT);
  }
  
  if (port->role != port->selectedRole &&
      port->selected &&
      ! port->updtInfo) {
    switch (port->selectedRole) {
    case AlternatePort:
    case BackupPort:
#ifdef STP_DBG 
      if (this->debug) {
	stp_trace ("hop to BLOCK_PORT role=%d selectedRole=%d",
		   (int) port->role, (int) port->selectedRole);
      }
#endif
      return STP_hop_2_state (this, BLOCK_PORT);
    case DisabledPort:
      return STP_hop_2_state (this, DISABLE_PORT);         /*kd: sehe die 17.20, 17.21, 17.22, 17.23 figuren aus 801.2D*/
    case RootPort:
      return STP_hop_2_state (this, ROOT_PORT);
    case DesignatedPort:
      return STP_hop_2_state (this, DESIGNATED_PORT);
      
    default:
      return False;
    }
  }
  
  allSynced = compute_allSynced (port);

  switch (this->State) {
    /* 17.23.1 */
  case INIT_PORT:
    return STP_hop_2_state (this, DISABLE_PORT); /*kd: die Änderung aus 802.1D*/
  case DISABLE_PORT:
    if (!port->selected || port->updtInfo) break;
    if (!port->learning && !port->forwarding) {
      return STP_hop_2_state (this, DISABLED_PORT);
    }
    break;
  case DISABLED_PORT:
    if (!port->selected || port->updtInfo) break;
    if (port->fdWhile != stpm->rootTimes.MaxAge ||
	port->sync                ||
	port->reRoot              ||
	!port->synced) {
      return STP_hop_2_state (this, DISABLED_PORT);
    }
    break;
    /* 17.23.2 */
  case ROOT_PROPOSED: /**/
    return STP_hop_2_state (this, ROOT_PORT);
  case ROOT_AGREED:
    return STP_hop_2_state (this, ROOT_PORT);
  case REROOT:
    return STP_hop_2_state (this, ROOT_PORT);
  case ROOT_PORT:
    if (!port->selected || port->updtInfo) break;
    if (!port->forward && !port->reRoot) {
      return STP_hop_2_state (this, REROOT);
    }
    if ((allSynced && !port->agree) || (port->proposed && port->agree)) {
      return STP_hop_2_state (this, ROOT_AGREED);
    }
    if (port->proposed && !port->agree) {
      return STP_hop_2_state (this, ROOT_PROPOSED);
    }
    
    reRooted = compute_reRooted (port);
    if (((!port->fdWhile || 
	 ((reRooted && port->rbWhile==0))) && rstpVer(this)) &&
/*kd: "rstpVersion" True if Protocol Version is greater than or equal to 2*/
	port->learn && !port->forward) {
      return STP_hop_2_state (this, ROOT_FORWARD);
    }
    if ((!port->fdWhile || 
	 ((reRooted && !port->rbWhile) && rstpVer(this))) &&       /*kd: Jetz steht die alter parametr "ForceVersion>=2, aber die neu rstpVerson stehen muss. Beide haben die gleichen Bedeutung, und Ich lasse die alte"*/
	!port->learn) {
      return STP_hop_2_state (this, ROOT_LEARN);
    }
    
    if (port->reRoot && port->forward) {
      return STP_hop_2_state (this, REROOTED);
    }
    if (port->rrWhile != stpm->rootTimes.ForwardDelay) {
      return STP_hop_2_state (this, ROOT_PORT);
    }
    break;
  case REROOTED:
    return STP_hop_2_state (this, ROOT_PORT);
  case ROOT_LEARN:
    return STP_hop_2_state (this, ROOT_PORT);
  case ROOT_FORWARD:
    return STP_hop_2_state (this, ROOT_PORT);
    /* 17.29.3 */ /**/
  case DESIGNATED_PROPOSE:
    return STP_hop_2_state (this, DESIGNATED_PORT);
  case DESIGNATED_SYNCED:
    return STP_hop_2_state (this, DESIGNATED_PORT);
  case DESIGNATED_RETIRED:
    return STP_hop_2_state (this, DESIGNATED_PORT);
  case DESIGNATED_PORT:
    if (!port->selected || port->updtInfo) 
      break;
    
    if (!port->forward && !port->agreed && !port->proposing && !port->operEdge) {
      return STP_hop_2_state (this, DESIGNATED_PROPOSE);
    }
    
    if (port->rrWhile ==0 && port->reRoot) {
      return STP_hop_2_state (this, DESIGNATED_RETIRED);
    }
    
    if (!port->learning && !port->forwarding && !port->synced) {
      return STP_hop_2_state (this, DESIGNATED_SYNCED);
    }
    
    if (port->agreed && !port->synced) {
      return STP_hop_2_state (this, DESIGNATED_SYNCED);
    }
    if (port->operEdge && !port->synced) {
      return STP_hop_2_state (this, DESIGNATED_SYNCED);
    }
    if (port->sync && port->synced) {
      return STP_hop_2_state (this, DESIGNATED_SYNCED);
    }
    
    if ((port->fdWhile ==0 || port->agreed || port->operEdge) &&
	(port->rrWhile==0  || !port->reRoot) &&
	!port->sync &&
	(port->learn && !port->forward)) {
      return STP_hop_2_state (this, DESIGNATED_FORWARD);
    }
    if ((port->fdWhile ==0 || port->agreed || port->operEdge) &&
	(port->rrWhile ==0  || !port->reRoot) &&
	!port->sync && !port->learn) {
      return STP_hop_2_state (this, DESIGNATED_LEARN);
    }
    if (((port->sync && !port->synced) ||
	 (port->reRoot && port->rrWhile !=0) || port->disputed) &&
	!port->operEdge && (port->learn || port->forward)) {
      return STP_hop_2_state (this, DESIGNATED_DISCARD);/*izmeneno*/
    }
    break;
  case DESIGNATED_DISCARD:
    return STP_hop_2_state (this, DESIGNATED_PORT);
  case DESIGNATED_LEARN:
    return STP_hop_2_state (this, DESIGNATED_PORT);
  case DESIGNATED_FORWARD:
    return STP_hop_2_state (this, DESIGNATED_PORT);
    
    /*kd: Für Alternate and Backup port teil*/
    /****************************************/
  case ALTERNATE_PROPOSED:
    return STP_hop_2_state (this, ALTERNATE_PORT);
  case ALTERNATE_AGREED:
    return STP_hop_2_state (this, ALTERNATE_PORT);
  case BLOCK_PORT:
    return STP_hop_2_state (this, ALTERNATE_PORT);
  case BACKUP_PORT:
    return STP_hop_2_state (this, ALTERNATE_PORT);
    
  case ALTERNATE_PORT:
    if (!port->selected || port->updtInfo) 
      break;
    
    if (!port->agree && port->proposed) {
      return STP_hop_2_state (this, ALTERNATE_PROPOSED);
    }
    if ((allSynced && !port->agree) || (port->proposed && port->agree)) {
      return STP_hop_2_state (this, ALTERNATE_AGREED);
    }
    if (port->rbWhile != 2 * (unsigned int) stpm->rootTimes.HelloTime &&
	port->role == BackupPort) {
      return STP_hop_2_state (this, BACKUP_PORT);
    }
    
    
    if ((port->fdWhile != forward_del(port)) ||
	port->sync                ||
	port->reRoot              ||
	!port->synced) {
      return STP_hop_2_state (this, ALTERNATE_PORT);
    }
    break;
  }
  
  return False;
}


