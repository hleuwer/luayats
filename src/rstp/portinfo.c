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
#define this _this
#include "base.h"
#include "stpm.h"


/* The Port Information State Machine : 17.21 */
/*kd: unter hinzufügte man die neu Namen, die im 802.1D definieren*/
#define STATES { \
  CHOOSE(DISABLED), \
  CHOOSE(ENABLED),  \
  CHOOSE(AGED),     \
  CHOOSE(UPDATE),   \
  CHOOSE(CURRENT),  \
  CHOOSE(RECEIVE),  \
  CHOOSE(AGREEMENT),    \
  CHOOSE(SUPERIOR_DESIGNATED),   \
  CHOOSE(REPEATED_DESIGNATED),   \
  CHOOSE(INFERIOR_DESIGNATED),  \
  CHOOSE(NOT_DESIGNATED),   \
  CHOOSE(OTHER),   \
    }

#define GET_STATE_NAME STP_info_get_state_name
#include "choose.h"

/******************************************************************/
void _stp_dump (char* title, unsigned char* buff, int len)
{
  register int iii;
  
  printf ("\n%s:", title);
  for (iii = 0; iii < len; iii++) {
    if (! (iii % 24)) Print ("\n%6d:", iii);
    if (! (iii % 8)) Print (" ");
    Print ("%02lx", (unsigned long) buff[iii]);
  }
  Print ("\n");
}

/* 17.21.8 checked leu */
static RCVD_MSG_T rcvInfo (STATE_MACH_T* this)
{
  int   bridcmp;
  register PORT_T* port = this->owner.port;
  
  if (port->msgBpduType == BPDU_TOPO_CHANGE_TYPE) {
#ifdef STP_DBG
    if (this->debug) {
      stp_trace ("%s", "OtherInfo:BPDU_TOPO_CHANGE_TYPE");
    }
#endif
    return OtherInfo;
  }
  
  port->msgPortRole = RSTP_PORT_ROLE_UNKN;
  
  if (BPDU_RSTP == port->msgBpduType) {
    port->msgPortRole = (port->msgFlags & PORT_ROLE_MASK) >> PORT_ROLE_OFFS;
  }
  
  /* 17.21.8: a) 
   * msgPrio superior to portPrio: < 0 OR
   * msgPrio same as portPrio: == 0
   * Note leu: We compare only the following compontents here:
   *           vec = (R:RPC:D:PD:PB)
   *         frist clause: superior
   *           msgPrio  = (R:RPC:D:PD:-)
   *           portPrio = (R:RPC:D:PD:-)  
   *         second clause: same as - why exclude R, RPC and PB ???
   *           msgPrio  = (-:-:D:PD:-)
   *           portPrio = (-:-:D:PD:-)  
   *
   */
  if (RSTP_PORT_ROLE_DESGN == port->msgPortRole ||
      BPDU_CONFIG_TYPE == port->msgBpduType) {
    bridcmp = STP_VECT_compare_vector(&port->msgPrio, &port->portPrio);
    /* Clause a) superior info ? */
    if (((bridcmp < 0) ||
        ((STP_VECT_compare_bridge_id(&port->msgPrio.design_bridge,
				     &port->portPrio.design_bridge) == 0) &&
         (port->msgPrio.design_port == port->portPrio.design_port))) ||
	/* Clause b) times changed */
	((bridcmp == 0) && 
         (STP_compare_times(&port->msgTimes, &port->portTimes) != 0))) {
#ifdef STP_DBG
      if (this->debug) {
	stp_trace ("SuperiorDesignatednfo: bridcmp=%d", (int) bridcmp); 
      }
#endif
      return SuperiorDesignatedInfo;
    }
  }
  
  /* 17.21.8: b) 
   * msgPrio same as portPrio: == 0
   * msgTimes same as portTimes: == 0
   */
  if (BPDU_CONFIG_TYPE == port->msgBpduType ||
      RSTP_PORT_ROLE_DESGN == port->msgPortRole) {
    if ((STP_VECT_compare_vector(&port->msgPrio, &port->portPrio) == 0) &&
        (STP_compare_times(&port->msgTimes, &port->portTimes) == 0)) {
#ifdef STP_DBG
      if (this->debug) {
	stp_trace ("%s", "RepeatedDesignatedInfo"); 
      }
#endif
      return RepeatedDesignatedInfo;
    }
  }
  
  /* 17.21.8: c) 
   * msgPrio worse than portPrio: > 0
   */
  if (RSTP_PORT_ROLE_DESGN == port->msgPortRole   ||    
      BPDU_CONFIG_TYPE == port->msgBpduType) {
    bridcmp = STP_VECT_compare_vector(&port->msgPrio, &port->portPrio);
    if (bridcmp > 0) {
#ifdef STP_DBG
      if (this->debug) {
	stp_trace ("%s %d", "InferiorDesignatedInfo", (int) bridcmp); 
      }
#endif
      return InferiorDesignatedInfo;
    }
  }
  
  /* 17.21.8: d) 
   * msgPrio same as or worse than portPrio: >= 0
   */

  if (RSTP_PORT_ROLE_ROOT == port->msgPortRole ||
      RSTP_PORT_ROLE_ALTBACK == port->msgPortRole ||
      BPDU_CONFIG_TYPE == port->msgBpduType) {
    bridcmp = STP_VECT_compare_vector (&port->msgPrio, &port->portPrio);
#if 0
    if ((bridcmp >= 0) &&
        (STP_compare_times (&port->msgTimes, &port->portTimes) == 0)) {
#else
      if (bridcmp >= 0){
#endif
#ifdef STP_DBG
      if (this->debug) {
	stp_trace ("%s", "InferiorRootAlternateInfo"); 
      }
#endif
      return InferiorRootAlternateInfo;	
    }	
  }	  
#ifdef STP_DBG
  if (this->debug) {
    stp_trace ("%s", "OtherInfo");
  }
#endif
  return OtherInfo; /* kd: die neue Name*/
}


/******************************************************************/
/* 17.21.11 checked leu */
static Bool recordProposal (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  
  if (RSTP_PORT_ROLE_DESGN == port->msgPortRole &&
      (PROPOSAL_BIT & port->msgFlags))        {
    port->proposed = True;
    return True;
  }
  return False;
}

/******************************************************************/
/* 17.21.1 checked leu */
static Bool betterorsameInfo (STATE_MACH_T* this, INFO_IS_T newInfoIs)
{
  register PORT_T* port = this->owner.port;
  int bridcmp_a, bridcmp_b;
  bridcmp_a = STP_VECT_compare_vector(&port->msgPrio, &port->portPrio);
  bridcmp_b = STP_VECT_compare_vector(&port->designPrio, &port->portPrio);
  if (((newInfoIs == Received && port->infoIs == Received) &&
       (bridcmp_a <= 0)) || 
      ((newInfoIs == Mine && port->infoIs == Mine) &&
       (bridcmp_b <= 0))){
#ifdef STP_DBG
    if (port->info->debug){
      stp_trace("port %s has better or same info: %s True agree=%d cmp(msg,port)=%d cmp(des,port)=%d", 
		port->port_name, newInfoIs == Mine ? "Mine" : "Received", 
		port->agree, bridcmp_a, bridcmp_b);
    }
#endif
    return True; 
  } else {
#ifdef STP_DBG
    if (port->info->debug){
      stp_trace("port %s has worse info: %s False agree=%d cmp(msg,port)=%d cmp(des,port)=%d", 
		port->port_name, newInfoIs == Mine ? "Mine" : "Received", 
		port->agree, bridcmp_a, bridcmp_b);
    }
#endif
    return False; 
  }
}

/****************************************************************/
/* 17.21.12 checked leu */
static Bool recordPriority (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  
  STP_VECT_copy (&port->portPrio, &port->msgPrio);
  return True;
}

/*******************************************************************/
/* 17.21.10 checked leu */
static Bool recordDispute (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  register Bool rd = False;
  if ((BPDU_RSTP == port->msgBpduType) && (port->msgFlags & LEARN_BIT)){
    rd = port->agreed = True; 
    port->proposing = False;
    port->disputed = True;
#ifdef STP_DBG
    if (this->debug) {
      stp_trace ("port %s sets agreed and resets proposing", port->port_name);
    }
#endif
  }
  return rd;
}

/****************************************************************/
/* 17.21.13 checked: leu */
static unsigned short recordTimes (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  
  port->portTimes.MessageAge = port->msgTimes.MessageAge;
  port->portTimes.MaxAge = port->msgTimes.MaxAge;
  port->portTimes.ForwardDelay = port->msgTimes.ForwardDelay;
  if (port->msgTimes.HelloTime > 1)
    port->portTimes.HelloTime = port->msgTimes.HelloTime;
  else
    port->portTimes.HelloTime = 1;

  return 0;
}

/****************************************************************/
/* 17.21.17 checked: leu */
static Bool setTcFlags (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;

  if (BPDU_TOPO_CHANGE_TYPE == port->msgBpduType) {
#ifdef STP_DBG
      if (this->debug) {
        stp_trace ("port %s rx rcvdTcn", port->port_name);
      }
#endif
    port->rcvdTcn = True;
  } else {
    if (TOLPLOGY_CHANGE_BIT & port->msgFlags) {
#ifdef STP_DBG
      if (this->debug) {
        stp_trace ("(%s-%s) rx rcvdTc 0X%lx",
            port->owner->name, port->port_name,
            (unsigned long) port->msgFlags);
      }
#endif
      port->rcvdTc = True;
    }
    if (TOLPLOGY_CHANGE_ACK_BIT & port->msgFlags) {
#ifdef STP_DBG
      if (this->debug) {
        stp_trace ("port %s rx rcvdTcAck 0X%lx",
            port->port_name,
            (unsigned long) port->msgFlags);
      }
#endif
      port->rcvdTcAck = True;
    }
  }
  return True;
}

/*****************************************************************/
/* 17.29.11 checked leu */
static Bool rstpVer (STATE_MACH_T* this)
{
  STPM_T *stpm = this->owner.stpm;
  Bool rstpVersion;
  if (stpm->ForceVersion >= 2) 
    rstpVersion =True;
  else
    rstpVersion = False;
  return rstpVersion;
  
}

/*****************************************************************/
/* 17.21.9 checked leu */
static Bool recordAgreement(STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  register Bool  ra;
  if (rstpVer(this) == True && port->operPointToPointMac == True
      && (AGREEMENT_BIT & port->msgFlags)) {
    ra = port->agreed = True; 
    ra = port->proposing = False;
  }
  else
    ra = port->agreed = False;
  return ra;
}	

/****************************************************************/
/* 17.21.23: checked leu */
static Bool updtRcvdInfoWhile (STATE_MACH_T* this)
{
  register int eff_age;
  register PORT_T* port = this->owner.port;
  
  eff_age = port->portTimes.MessageAge + 1;
  
  if (eff_age <= port->portTimes.MaxAge) {
    port->rcvdInfoWhile = 3 * port->portTimes.HelloTime;
#ifdef STP_DBG
#if 0
    stp_trace ("port %s: MessageAge=%d EffectiveAge=%d",
	       port->port_name, 
	       (int) port->portTimes.MessageAge,
	       (int) eff_age);
#endif
#endif
  } else {
    port->rcvdInfoWhile = 0;
#ifdef STP_DBG
    stp_trace ("port %s: MaxAge=%d MessageAge=%d HelloTime=%d rcvdInfoWhile=null !",
	       port->port_name,
	       (int) port->portTimes.MaxAge,
	       (int) port->portTimes.MessageAge,
	       (int) port->portTimes.HelloTime);
#endif
  }
  return True;
}

/****************************************************************/
void STP_info_rx_bpdu (PORT_T* port, struct stp_bpdu_t* bpdu, size_t len)
{  
#ifdef STP_DBG_BPDU
  _stp_dump ("\nall BPDU", ((unsigned char*) bpdu) - 12, len + 12);
  _stp_dump ("ETH_HEADER", (unsigned char*) &bpdu->eth, 5);
  _stp_dump ("BPDU_HEADER", (unsigned char*) &bpdu->hdr, 4);
  printf ("protocol=%02x%02x version=%02x bpdu_type=%02x\n",
	  bpdu->hdr.protocol[0], bpdu->hdr.protocol[1],
	  bpdu->hdr.version, bpdu->hdr.bpdu_type);
  
  _stp_dump ("\nBPDU_BODY", (unsigned char*) &bpdu->body, sizeof (BPDU_BODY_T) + 2);
  printf ("flags=%02x\n", bpdu->body.flags);
  _stp_dump ("root_id", bpdu->body.root_id, 8);
  _stp_dump ("root_path_cost", bpdu->body.root_path_cost, 4);
  _stp_dump ("bridge_id", bpdu->body.bridge_id, 8);
  _stp_dump ("port_id", bpdu->body.port_id, 2);
  _stp_dump ("message_age", bpdu->body.message_age, 2);
  _stp_dump ("max_age", bpdu->body.max_age, 2);
  _stp_dump ("hello_time", bpdu->body.hello_time, 2);
  _stp_dump ("forward_delay", bpdu->body.forward_delay, 2);
  _stp_dump ("ver_1_len", bpdu->ver_1_len, 2);
#endif
  
  /* check bpdu type */
  switch (bpdu->hdr.bpdu_type) {

  case BPDU_CONFIG_TYPE:
    port->rx_cfg_bpdu_cnt++;
#ifdef STP_DBG 
    if (port->info->debug) 
      stp_trace ("CfgBpdu on port %s", port->port_name);
#endif
    if (port->admin_non_stp) return;
    port->rcvdBpdu = True;
    break;

  case BPDU_TOPO_CHANGE_TYPE:
    port->rx_tcn_bpdu_cnt++;
#ifdef STP_DBG 
    if (port->info->debug)
      stp_trace ("TcnBpdu on port %s", port->port_name);
#endif
    if (port->admin_non_stp) return;
    port->rcvdBpdu = True;
    port->msgBpduVersion = bpdu->hdr.version;
    port->msgBpduType = bpdu->hdr.bpdu_type;
    return;

  case BPDU_RSTP:
    port->rx_rstp_bpdu_cnt++;
    if (port->admin_non_stp) return;
    if (port->owner->ForceVersion >= NORMAL_RSTP) {
      port->rcvdBpdu = True;
    } else {          
      return;
    }
#ifdef STP_DBG 
    if (port->info->debug)
      stp_trace ("BPDU_RSTP on port %s", port->port_name);
#endif
    break;

  default:
    stp_trace ("RX undef bpdu type=%d", (int) bpdu->hdr.bpdu_type);
    return;

  }
  
  port->msgBpduVersion = bpdu->hdr.version;
  port->msgBpduType =    bpdu->hdr.bpdu_type;
  port->msgFlags =       bpdu->body.flags;
  
  STP_VECT_get_vector (&bpdu->body, &port->msgPrio);
  port->msgPrio.bridge_port = port->port_id;
  
  STP_get_times (&bpdu->body, &port->msgTimes);
}

/****************************************************************/
void STP_info_enter_state (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  
  switch (this->State) {
  case BEGIN:
    port->rcvdMsg = OtherInfo;
    port->msgBpduType = 0xff;
    port->msgPortRole = RSTP_PORT_ROLE_UNKN;
    port->msgFlags = 0;
    
    /* clear port statistics */
    port->rx_cfg_bpdu_cnt =
      port->rx_rstp_bpdu_cnt =
      port->rx_tcn_bpdu_cnt = 0;
    
  case DISABLED:
    port->rcvdMsg = False;
    port->agreed = port->proposing =  
      port->proposed = port->agree = False;
    port->rcvdInfoWhile = 0;
    port->infoIs = Disabled;
    port->reselect = True;
    port->selected = False;
    break;

  case ENABLED: /* IEEE 802.1y, 17.21, Z.14 */
    STP_VECT_copy (&port->portPrio, &port->designPrio);
    STP_copy_times (&port->portTimes, &port->designTimes);
    break;

  case AGED:
    port->infoIs = Aged;
    port->reselect = True;
    port->selected = False;
    break;

  case UPDATE:
    port->proposed = port->proposing = False;
    port->agreed = port->agreed && betterorsameInfo (this, Mine);
    port->synced = port->synced && port->agreed;
    STP_VECT_copy (&port->portPrio, &port->designPrio);
    STP_copy_times (&port->portTimes, &port->designTimes);
    port->updtInfo = False;
    port->infoIs = Mine;
    port->newInfo = True;
#ifdef STP_DBG
    if (this->debug) {
      STP_VECT_br_id_print ("updated: portPrio.design_bridge",
                            &port->portPrio.design_bridge, True);
    }
#endif
    break;

  case CURRENT:
    break;

  case RECEIVE:
    port->rcvdInfo = rcvInfo (this);
    
    break;

  case SUPERIOR_DESIGNATED:
    port->agreed = False;
    port->proposing = False;
    recordProposal (this);
    setTcFlags (this);
    port->agree = port->agree && betterorsameInfo (this, Received);
    recordPriority (this);
    recordTimes (this);
    updtRcvdInfoWhile (this);
    port->infoIs = Received;
    port->reselect = True;
    port->selected = False;
    port-> rcvdMsg = False;
#ifdef STP_DBG
    if (this->debug) {
      STP_VECT_br_id_print ("stored: portPrio.design_bridge",
                            &port->portPrio.design_bridge, True);
      stp_trace ("proposed=%d on port %s",
		 (int) port->proposed, port->port_name);
    }
#endif
    break;

  case REPEATED_DESIGNATED:
    recordProposal (this);
    setTcFlags (this);
    updtRcvdInfoWhile (this);
    port->rcvdMsg = False;
    break;
    
  case INFERIOR_DESIGNATED:
    recordDispute (this);
    port->rcvdMsg = False;
    break;

  case NOT_DESIGNATED:
    recordAgreement (this);
    setTcFlags(this);
    port->rcvdMsg = False;
    break;

  case OTHER:
    port->rcvdMsg = False;
    break;
  }
}

/****************************************************************/
Bool STP_info_check_conditions (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  
  if ((! portEnabled(port) && port->infoIs != Disabled) || BEGIN == this->State) {
    return STP_hop_2_state (this, DISABLED);
  }
  
  switch (this->State) {
  case DISABLED:
    if (port->rcvdMsg) {
      return STP_hop_2_state (this, DISABLED);
    }
    if (portEnabled(port)) {
      return STP_hop_2_state (this, AGED);
    }
    break; 
    
  case ENABLED: 
    return STP_hop_2_state (this, AGED);
    break; 
    
  case AGED:
    if (port->selected && port->updtInfo) {
      return STP_hop_2_state (this, UPDATE);
    }
    break;

  case UPDATE: 
    return STP_hop_2_state (this, CURRENT); 
    break;

  case CURRENT:
    if (port->selected && port->updtInfo) {
      return STP_hop_2_state (this, UPDATE);
    }      
    
    if (Received == port->infoIs &&
	port->rcvdInfoWhile==0   &&
	! port->updtInfo         &&
	! port->rcvdMsg) {
      return STP_hop_2_state (this, AGED);
    }
    if (port->rcvdMsg && !port->updtInfo) {
      return STP_hop_2_state (this, RECEIVE);
    }
    break;

  case RECEIVE:
    switch (port->rcvdInfo) {
    case SuperiorDesignatedInfo:
      return STP_hop_2_state (this, SUPERIOR_DESIGNATED);
    case RepeatedDesignatedInfo:
      return STP_hop_2_state (this, REPEATED_DESIGNATED);
    case InferiorDesignatedInfo:
      return STP_hop_2_state (this, INFERIOR_DESIGNATED);
    case InferiorRootAlternateInfo:
      return STP_hop_2_state (this, NOT_DESIGNATED);
    case OtherInfo:
      return STP_hop_2_state (this, OTHER);
    }
    break;
    
  case SUPERIOR_DESIGNATED:
    return STP_hop_2_state (this, CURRENT);
    break;

  case REPEATED_DESIGNATED:
    return STP_hop_2_state (this, CURRENT);
    break;

  case INFERIOR_DESIGNATED:
    return STP_hop_2_state (this, CURRENT);
    break;

  case NOT_DESIGNATED:
    return STP_hop_2_state (this, CURRENT);
    break;

  case OTHER:
    return STP_hop_2_state (this, CURRENT);
    break;
  }
  return False;
}


