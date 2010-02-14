/************************************************************************ 
 * RSTP library - Rapid Spanning Tree (802.1t, 802.1w) 
 * Copyright (C) 2001-2003 Optical Access 
 * Author: Dmitry Klebanov, Herbert Leuwer 
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

/* Bridge Detection State Machine: 17.23 aus 802.1D*/
#define STATES { \
  CHOOSE(EDGE),   \
  CHOOSE(NOT_EDGE),    \
}
  
#define GET_STATE_NAME STP_brdec_get_state_name
#include "choose.h"
#include "statmch.h"

void STP_brdec_enter_state (STATE_MACH_T* this)
{
  register PORT_T *port = this->owner.port;
  
  switch (this->State) {
  case BEGIN:
  case EDGE:
    port->operEdge = True;
    break;

  case NOT_EDGE:
    port->operEdge = False;
    break;
  }
}
	
Bool STP_brdec_check_conditions (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  
  if (port->adminEdge && BEGIN == this->State) 
    return STP_hop_2_state (this, EDGE);

  if (!port->adminEdge && BEGIN == this->State) 
    return STP_hop_2_state (this, NOT_EDGE);
  
  switch (this->State) {

  case EDGE:
    if ((!portEnabled(port) && !port->adminEdge) || !port->operEdge) {
      return STP_hop_2_state (this, NOT_EDGE);
    }
    break;

  case NOT_EDGE:
    if ((!portEnabled(port) && port->adminEdge) || 
	((port->edgeDelayWhile == 0) &&
	 port->autoEdge && port->sendRSTP && port->proposing)){
      return STP_hop_2_state (this, EDGE);
    }
    break;
  }
  return False;
}
