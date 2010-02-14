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

/* This file contains prototypes for system dependent API
   from the RSTP to an operation system */
  
#ifndef _STP_OUT_H__
#define _STP_OUT_H__

/* In the best case: clean all Learning entries with
 the vlan_id and the port (if 'exclude'=0) or for all ports,
 exclude the port (if ''exclude'=1). If 'port'=0, delete all
 entries with the vlan_id, don't care to 'exclude'  */
//tolua_begin
typedef enum {
  LT_FLASH_ALL_PORTS_EXCLUDE_THIS,
  LT_FLASH_ONLY_THE_PORT
} LT_FLASH_TYPE_T;
//tolua_end

#ifdef USELUA
#include "rstp_bridge.h"
#define STP_OUT_flush_lt(rstp, port, vid, typ, reason)  rstp->flush_fdb(port, vid, typ, reason)
#define STP_OUT_get_port_mac(rstp, port, mac)           rstp->get_mac(port, mac)
#define STP_OUT_get_port_oper_speed(rstp, port)         rstp->get_speed(port)
#define STP_OUT_get_port_link_status(rstp, port)        rstp->get_linkstatus(port)
#define STP_OUT_get_duplex(rstp, port)                  rstp->get_duplex(port)
#define STP_OUT_set_learning(rstp, port, vid, ena)      rstp->set_learning(port, vid, ena)
#define STP_OUT_set_forwarding(rstp, port, vid, ena)    rstp->set_forwarding(port, vid, ena)
#define STP_OUT_set_port_state(rstp, port, vid, state)   rstp->set_portstate(port, vid, state)
#define STP_OUT_tx_bpdu(rstp, port, vid, bpdu, len)     rstp->tx_bpdu(port, vid, bpdu, len)
#define STP_OUT_get_port_name(rstp, port)               rstp->get_portname(port)
#define STP_OUT_get_init_port_pathcost(rstp, port)      rstp->get_init_port_pathcost(port)
#define STP_OUT_get_init_stpm_cfg(rstp, vid, cfg)       rstp->get_init_stpm_cfg(vid, cfg)
#define STP_OUT_get_init_port_cfg(rstp, port, vid, cfg) rstp->get_init_port_cfg(port, vid, cfg)
#define STP_OUT_set_hardware_mode(rstp, vid, mode)      rstp->set_hardware_mode(vid, mode)
#else
int STP_OUT_flush_lt (IN int port_index, IN int vlan_id,
		      IN LT_FLASH_TYPE_T type, IN char* reason);

/* for bridge id calculation */
void STP_OUT_get_port_mac (IN int port_index, OUT unsigned char* mac);
unsigned long STP_OUT_get_port_oper_speed (IN unsigned int portNo);
/* 1- Up, 0- Down */
int STP_OUT_get_port_link_status (IN rstp_bridge *rstp, IN int port_index);
/* 1- Full, 0- Half */
int STP_OUT_get_duplex (IN int port_index);

#ifdef STRONGLY_SPEC_802_1W
int STP_OUT_set_learning (IN int port_index, IN int vlan_id, IN int enable);
int STP_OUT_set_forwarding (IN int port_index, IN int vlan_id, IN int enable);
#else
/*
 * In many kinds of hardware the state of ports may
 * be changed with another method
 */
int STP_OUT_set_port_state (IN int port_index, IN int vlan_id, IN RSTP_PORT_STATE state);
#endif

int STP_OUT_set_hardware_mode (int vlan_id, UID_STP_MODE_T mode);
int STP_OUT_tx_bpdu (IN int port_index, IN rstp_bridge *rstp, int vlan_id,
		     IN unsigned char* bpdu,
		     IN size_t bpdu_len);
const char *STP_OUT_get_port_name (IN int port_index);
int STP_OUT_get_init_stpm_cfg (IN int vlan_id,
			       INOUT UID_STP_CFG_T* cfg);
int STP_OUT_get_init_port_cfg (IN rstp_bridge *rstp, int vlan_id,
			       IN int port_index,
			       INOUT UID_STP_PORT_CFG_T* cfg);
#endif
#endif /* _STP_OUT_H__ */

