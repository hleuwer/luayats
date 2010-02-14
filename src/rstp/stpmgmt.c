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

/* This file contains API from an operation system to the RSTP library */

#include "base.h"
#include "stpm.h"
#include "stp_in.h" /* for bridge defaults */
#include "stp_to.h"
#include "rstp_bridge.h"

int STP_IN_stpm_create (rstp_bridge *rstp, int vlan_id, char* name, BITMAP_T* port_bmp)
{
  register STPM_T*  self;
  int               err_code;
  UID_STP_CFG_T		init_cfg;
		   
  stp_trace ("STP_IN_stpm_create(%s)", name);

  init_cfg.field_mask = BR_CFG_ALL;
  STP_OUT_get_init_stpm_cfg (rstp, vlan_id, &init_cfg);
  init_cfg.field_mask = 0;

  RSTP_CRITICAL_PATH_START;  
  self = (STPM_T*) stp_in_stpm_create (rstp, vlan_id, name, port_bmp, &err_code);
  if (self) {
    self->BrId.prio = init_cfg.bridge_priority;
    self->BrTimes.MaxAge = init_cfg.max_age;
    self->BrTimes.HelloTime = init_cfg.hello_time;
    self->BrTimes.ForwardDelay = init_cfg.forward_delay;
    self->ForceVersion = (PROTOCOL_VERSION_T) init_cfg.force_version;
  }

  RSTP_CRITICAL_PATH_END;
  return err_code;  
}

int STP_IN_stpm_delete (rstp_bridge *rstp, int vlan_id)
{
  register STPM_T* self;
  int iret = 0;

  RSTP_CRITICAL_PATH_START;  
  self = stpapi_stpm_find (rstp, vlan_id);

  if (! self) { /* it had not yet been created :( */
    iret = STP_Vlan_Had_Not_Yet_Been_Created;
  } else {

    if (STP_ENABLED == self->admin_state) {
      if (0 != STP_stpm_enable (self, STP_DISABLED)) {/* can't disable :( */
        iret = STP_Another_Error;
      } else
        STP_OUT_set_hardware_mode (rstp, self->vlan_id, STP_DISABLED);
    }

    if (0 == iret) {
      STP_stpm_delete (self);   
    }
  }
  RSTP_CRITICAL_PATH_END;
  return iret;
}

int STP_IN_stpm_get_vlan_id_by_name (rstp_bridge *rstp, char* name, int* vlan_id)
{
  register STPM_T* stpm;
  int iret = STP_Cannot_Find_Vlan;

  RSTP_CRITICAL_PATH_START;  
  for (stpm = STP_stpm_get_the_list (rstp); stpm; stpm = stpm->next) {
    if (stpm->name && ! strcmp (stpm->name, name)) {
      *vlan_id = stpm->vlan_id;
      iret = 0;
      break;
    }
  }
  RSTP_CRITICAL_PATH_END;

  return iret;
}
    

Bool STP_IN_get_is_stpm_enabled (rstp_bridge *rstp, int vlan_id)
{
  STPM_T* self;
  Bool iret = False;

  RSTP_CRITICAL_PATH_START;  
  self = stpapi_stpm_find (rstp, vlan_id);
  
  if (self) { 
    if (self->admin_state == STP_ENABLED) {
      iret = True;
    }
  } else {
    ;   /* it had not yet been created :( */
  }
  
  RSTP_CRITICAL_PATH_END;
  return iret;
}

int STP_IN_stop_all (rstp_bridge *rstp)
{
  register STPM_T* stpm;

  RSTP_CRITICAL_PATH_START;
  
  for (stpm = STP_stpm_get_the_list (rstp); stpm; stpm = stpm->next) {
    if (STP_DISABLED != stpm->admin_state) {
      STP_OUT_set_hardware_mode (rstp, stpm->vlan_id, STP_DISABLED);
      STP_stpm_enable (stpm, STP_DISABLED);
    }
  }

  RSTP_CRITICAL_PATH_END;
  return 0;
} 

int STP_IN_delete_all (rstp_bridge *rstp)
{
  register STPM_T* stpm;

  RSTP_CRITICAL_PATH_START;
  for (stpm = STP_stpm_get_the_list (rstp); stpm; stpm = stpm->next) {
    STP_stpm_enable (stpm, STP_DISABLED);
    STP_stpm_delete (stpm);
  }

  RSTP_CRITICAL_PATH_END;
  return 0;
}

