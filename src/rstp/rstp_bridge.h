/*************************************************************************
*
*  Luayats - Yet Another Tiny Simulator 
*
**************************************************************************
*
*    Copyright (C) 1995-2005 
*    - Chair for Telecommunications
*      Dresden University of Technolog, D-01062 Dresden, Germany
*    - Marconi Ondata GmbH, D-71522 Backnang, Germany
*   
**************************************************************************
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*************************************************************************
*
*   Module author: Herbert Leuwer
*
*************************************************************************
*
*   Module description: Rapid Spanning Tree Protocol Bridge
*
*   The actual protocol machine is based on RSTPLIB by Alex Rozin.
*   This module provides a wrapper class 'rstp_bridge' around the 
*   state machines.
*   See the Luayats documentation for a detailed description of the 
*   architecture.
*
*************************************************************************/

#ifndef _RSTP_BRIDGE_H
#define _RSTP_BRIDGE_H

#include "stdlib.h"
#include "defs.h"
#include "base.h"
#include "bitmap.h"
#include "stpm.h"
#include "uid_stp.h"
#include "stp_in.h"
#include "stp_to.h"
#include "stp_bpdu.h"
#include "tolua++.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "lua.h"
#ifdef __cplusplus
}
#endif
#ifdef this
#undef this
#endif

#define lua_Object int
#ifndef _nports
#define _nports
#endif

LUALIB_API int luaopen_rstp(lua_State *L);

//tolua_begin
int speed2pcost(int speed);

// A couple of encapsulating helper classes to simplify Lua interfacing
class stpcfg { 
 public:
   stpcfg(){CHECK(val = new UID_STP_CFG_T);}
   ~stpcfg(){delete val;}
   UID_STP_CFG_T *val;
};
class stpmstate {
 public:
   stpmstate(){CHECK(val = new UID_STP_STATE_T);}
   ~stpmstate(){delete val;}
   UID_STP_STATE_T *val;
};
class portcfg {
 public:
   portcfg(){CHECK(val = new UID_STP_PORT_CFG_T);}
   ~portcfg(){delete val;}
   UID_STP_PORT_CFG_T *val;
   void setbm(int bm){
     this->val->port_bmp.part0 = (unsigned long) bm;
   }
};
 
class portstate {
 public:
   portstate(){CHECK(val = new UID_STP_PORT_STATE_T);};
   ~portstate(){delete val;}
   UID_STP_PORT_STATE_T *val;
};

class macaddress {
 public:
  inline macaddress(int b1,int b2,int b3,int b4,int b5,int b6){
    address[0] = (char ) b1;
    address[1] = (char ) b2;
    address[2] = (char ) b3;
    address[3] = (char ) b4;
    address[4] = (char ) b5;
    address[5] = (char ) b6;
  }
  inline ~macaddress(){}
  char address[7];
};

// RSTP class: keeps all the variables that are global in the original code.
class rstp_bridge {
 public:
   // Constructor - receive a reference to the Luayats node object.
  //   rstp_bridge(root *node, int nports, double speed[_nports], int duplex[_nports], 
  //	       char *basemac);
   rstp_bridge(root *node, int nports, char *basemac);

   // Destructor
   ~rstp_bridge(void);
   

   //tolua_end
#ifdef STP_DBG
   int dbg_rstp_deny;
#endif
   //tolua_begin

   // Start the RSTP instances
   char *start(int vlan_id = 0, int prio = 32768, char *memberset = NULL);

   // Shutdown the RSTP instances
   char* shutdown(int vlan_id = 0);

   // Register a callback function
   void setcallback(char *key, lua_Object func);
   
   // Get reference to the bridge shell (the Luayats node object).
   inline root *get_parent(void){return node;}

   // Flushing method: set and get
   inline void set_flushtype(LT_FLASH_TYPE_T typ){flushtype = typ;}
   inline LT_FLASH_TYPE_T get_flushtype(void){ return flushtype;}

   //Input API - only convenience wrappers to RSTPLIB's input API routines
   // 1. Create/Delete/Start/Stop
   inline void init(int max_port_index){
      STP_IN_init(this, max_port_index);
   }

   // Create a STP state machine
   inline int create_stpm(int vlan_id, char *name, BITMAP_T* port_bmp){
      return STP_IN_stpm_create(this, vlan_id, name, port_bmp);
   }

   // Delete a STP state machine
   inline int delete_stpm(int vlan_id){
      return STP_IN_stpm_delete(this, vlan_id);
   }

   // Stop all STP state machines
   inline int stop_all(void){return STP_IN_stop_all(this);}

   // Delete all STP state machines
   inline int delete_all(void){return STP_IN_delete_all(this);}

   // 2. "Get" management <= STP
   inline bool is_stpm_enabled(int vlan_id){
     return (bool) STP_IN_get_is_stpm_enabled(this, vlan_id);
   }

   // Get VID from name
   inline int get_vlanid_by_name(char *name){
      int rv, vlan_id;
      rv = STP_IN_stpm_get_vlan_id_by_name(this, name, &vlan_id);
      return vlan_id;
   }
   
   // Get name from VID
   inline char *get_name_by_vlanid(int vlan_id){
      int rv;
      rv = STP_IN_stpm_get_name_by_vlan_id(this, vlan_id, strbuf, sizeof(strbuf));
      return strbuf;
   }

   // Get an error explanation
   inline const char *get_error_explanation(int err){
      return STP_IN_get_error_explanation(err);
   }


   // Get Spanning Tree FSM configuration
   // inline void *new_container(char *which);

   inline UID_STP_CFG_T *get_stpmcfg(int vlan_id, UID_STP_CFG_T *cfg){
     if (STP_IN_stpm_get_cfg(this, vlan_id, cfg) == 0)
       return cfg;
     else
       return NULL;
   }

   // Get Spanning Tree FSM status
   inline UID_STP_STATE_T *get_stpmstate(int vlan_id, UID_STP_STATE_T *state){
     if (STP_IN_stpm_get_state(this, vlan_id, state) == 0)
       return state;
     else
       return NULL;
   }

   // Get port Configuration
   //inline UID_STP_PORT_CFG_T *get_portcfg(int vlan_id, int port_index, UID_STP_PORT_CFG_T *cfg){
   //   STP_IN_port_get_cfg(this, vlan_id, port_index, cfg);
   //   return cfg;
   //}

   // Get port status
   inline UID_STP_PORT_STATE_T *get_portstate(int vlan_id, UID_STP_PORT_STATE_T *state){
     if (STP_IN_port_get_state(this, vlan_id, state) == 0)
       return state;
     else
       return NULL;
   }

   // 3. "Set" management => STP
   // Set Spanning Tree FSM configuration
   inline int set_stpm_cfg(int vlan_id, BITMAP_T *port_bmp, UID_STP_CFG_T *cfg){
      return STP_IN_stpm_set_cfg(this, vlan_id, port_bmp, cfg);
   }

   // Set port ocnfiguration
   inline int set_port_cfg(int vlan_id, UID_STP_PORT_CFG_T* cfg){
      return STP_IN_set_port_cfg(this, vlan_id, cfg);
   }


   // 4. "Physical" events => STP
   // Give the Spanning Tree FSMs a notion of time.
   int one_second(void){
      return STP_IN_one_second(this);
   }

   // Enable/disable given port
   inline int enable_port(int port_index, bool ena){
      if (ena == true){
	 BitmapSetBit(&enabled_ports, port_index - 1);
      } else {
	 BitmapClearBit(&enabled_ports, port_index - 1);
      }
      return STP_IN_enable_port(this, port_index, (ena == true) ? 1 : 0);
   }

   // Link Up event
   inline int linkUp(int port_index){
     // leu:  return STP_IN_enable_port(this, port_index, 1);
     return STP_IN_up_port(this, port_index, 1);
   }
   
   // Link Down event
   inline int linkDown(int port_index){
     //leu:   return STP_IN_enable_port(this, port_index, 0);
     return STP_IN_up_port(this, port_index, 0);
   }

   // Change a port's speed
   inline int change_port_speed(int port_index, long speed){
      return STP_IN_changed_port_speed(this, port_index, speed);
   }

   // Change a port's duplex mode
   inline int change_port_duplex(int port_index){
      return STP_IN_changed_port_duplex(this, port_index);
   }

   // Check header of a BPDU
   inline int check_bpdu_header(BPDU_T *bpdu, size_t len){
      return STP_IN_check_bpdu_header(bpdu, len);
   }

   // Hand-over a received BPDU to the spanning tree FSMs.
   void rx_bpdu(int vlan_id, int port_index, unsigned char *bpdu, size_t len);

   // Turn on tracing of port state machines */
   void set_port_trace(char *mach_name, int vlan_id, int port_index, int enadis);
   void set_stpm_trace(int vlan_id, int enadis);
   // Output API: 
   int flush_fdb(int port_index, int vlan_id, LT_FLASH_TYPE_T typ, char *reason);
   int get_mac(int port_index, unsigned char *mac);
   unsigned long get_speed(int port_index);
   void set_speed(int port_index, unsigned long speed);
   int get_linkstatus(int port_index);
   void set_duplex(int port_index, int duplex);
   int get_duplex(int port_index);
   int set_learning(int port_index, int vlan_id, int enable);
   int set_forwarding(int port_index, int vlan_id, int enable);
   int set_portstate(int port_index, int vlan_id, RSTP_PORT_STATE state);
   int tx_bpdu(int port_index, int vlan_id, unsigned char *bpdu, size_t len);
   const char *get_portname(int port_index);
   unsigned long get_init_port_pathcost(int port_index);
   int get_init_stpm_cfg(int vlan_id, UID_STP_CFG_T *cfg);
   int get_init_port_cfg(int port_index, int vlan_id, UID_STP_PORT_CFG_T *cfg);
   int set_hardware_mode(int vlan_id, UID_STP_MODE_T mode);

   STPM_T *bridges;      // pointer to bridges - one per VLAN
   int max_port;         // Max. # of ports in total
   //tolua_end
   int nev;              // event handling
   RSTP_EVENT_T tev;     // event handling
   int debug;
 private:
   BITMAP_T enabled_ports;   // bitmap of enabled ports
   // -- Global variables of the RSTPLIB package. 
   //    We need to instantiate them in a class
   char *errmsg;             // buffer to error messages
   char strbuf[64];          // general string buffer
   root *node;               // reference to Luayats node object
   unsigned char **mac;
   char **portname;
   unsigned long *speed;
   unsigned long *pathcost;
   int *duplex;
   LT_FLASH_TYPE_T flushtype; // defines type of address table flushing
}; //tolua_export
#endif
