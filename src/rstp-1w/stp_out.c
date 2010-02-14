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

/* This file contains system dependent API
   from the RStp to a operation system (see stp_to.h) */

/* stp_to API for Linux */

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include "base.h"
#include "stpm.h"
#include "stp_in.h"
#include "stp_to.h"
#include "rstp_bridge.h"

/*************
void
stp_trace (const char *format, ...)
{
  #define MAX_MSG_LEN  128
  char     msg[MAX_MSG_LEN];
  va_list  args;

  va_start(args, format);
  vsnprintf (msg, MAX_MSG_LEN-1, format, args);
  printf ("%s\n", msg);
  va_end(args);
  
}
***********/
#ifdef STRONGLY_SPEC_802_1W
int STP_OUT_set_learning (rstp_bridge *rstp, int port_index, int vlan_id, int enable)
{
   return rstp->set_learning(port_index, vlan_id, enable);
}

int STP_OUT_set_forwarding (int port_index, int vlan_id, int enable)
{
   return rstp->set_forwarding(port_index, vlan_id, enable);
}
#else
/* 
 * In many kinds of hardware the state of ports may
 * be changed with another method
 */
int STP_OUT_set_port_state (rstp_bridge *rstp, IN int port_index, IN int vlan_id,
			IN RSTP_PORT_STATE state)
{
   return rstp->set_port_state(port_index, vlan_id, state);
}
#endif

void STP_OUT_get_port_mac (rstp_bridge *rstp, int port_index, unsigned char *mac)
{
   return rstp->get_port_mac(port_index, mac);
}
/* 1- Up, 0- Down */
int STP_OUT_get_port_link_status (rstp_bridge *rstp, int port_index)
{
   return rstp->get_port_link_status(port_index);
}

int STP_OUT_flush_lt (rstp_bridge *rstp, IN int port_index, IN int vlan_id, LT_FLASH_TYPE_T type, 
		      char* reason)
{
   return rstp->flush_fdb(port_index, vlan_id, type, reason);
}

int STP_OUT_set_hardware_mode (rstp_bridge *rstp, int vlan_id, UID_STP_MODE_T mode)
{
   return rstp->set_hardware_mode(rstp, vlan_id, mode);
}


int STP_OUT_tx_bpdu (int port_index, rstp_bridge *rstp, int vlan_id,
		     unsigned char *bpdu, size_t bpdu_len)
{
   return rstp->tx_bpdu(port_index, bpdu, bpdu_len);
}

const char * STP_OUT_get_port_name (IN int port_index)
{
   return rstp->get_port_name(port_index);
}

unsigned long STP_OUT_get_deafult_port_path_cost (IN unsigned int port_index)
{
   return rstp->get_default_port_path_cost(port_index);
}

unsigned long STP_OUT_get_port_oper_speed (unsigned int portNo)
{
  if (portNo <= 2)
    return 1000000L;
  else
    return 1000L;
}

int STP_OUT_get_duplex (rstp_bridge *rstp, IN int port_index)
{
   return rstp->get_duplex(port_index);
}

int
STP_OUT_get_init_stpm_cfg (IN int vlan_id,
                           INOUT UID_STP_CFG_T* cfg)
{
  cfg->bridge_priority =        DEF_BR_PRIO;
  cfg->max_age =                DEF_BR_MAXAGE;
  cfg->hello_time =             DEF_BR_HELLOT;
  cfg->forward_delay =          DEF_BR_FWDELAY;
  cfg->force_version =          NORMAL_RSTP;

  return STP_OK;
}
  

int
STP_OUT_get_init_port_cfg (IN rstp_bridge *rstp, int vlan_id,
                           IN int port_index,
                           INOUT UID_STP_PORT_CFG_T* cfg)
{
  cfg->port_priority =                  DEF_PORT_PRIO;
  cfg->admin_non_stp =                  DEF_ADMIN_NON_STP;
  cfg->admin_edge =                     DEF_ADMIN_EDGE;
  cfg->admin_port_path_cost =           ADMIN_PORT_PATH_COST_AUTO;
  cfg->admin_point2point =              DEF_P2P;

  return STP_OK;
}



