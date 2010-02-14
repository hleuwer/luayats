
#include "rstp_bridge.h"
#include "stdlib.h"
#include "string.h"
#include "yats.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "lua.h"
#include "lauxlib.h"
#ifdef __cplusplus
}
#endif

// Utilities

// Convert a given char array to a Lua string - embedded 0 allowed.
static int c2lua(lua_State *L)
{
  char *str, *buf;
  int len;
  
  str = (char *) lua_tostring(L, 1);
  if (lua_isnil(L, -2))
    len = strlen(str);
  else
    len = (int) lua_tonumber(L, 2);
  
  CHECK(buf = new char[len]);
  memcpy(buf, str, len);
  lua_pushlstring(L, buf, len);
  delete buf;
  return 1;
}

// Convert a bridge id to a pair of port and mac address
static int brid2lua(lua_State *L)
{
  UID_BRIDGE_ID_T *id;
  id = (UID_BRIDGE_ID_T *) tolua_tousertype(L, 1, 0);
  if (id == NULL){
    lua_pushnil(L);
    lua_pushstring(L, "invalid bridgeid");
    return 2;
  } else {
    lua_pushnumber(L, id->prio);
    lua_pushlstring(L, (const char *) id->addr, 6);
    return 2;
  }
}

// Utility functions
static const luaL_reg funcs[] = {
  {"c2lua", c2lua},
  {"brid2lua", brid2lua},
  {NULL, NULL}
};

// Open library with utility functions
LUALIB_API int luaopen_rstp(lua_State *L)
{
  luaL_openlib(L, "yats", funcs, 0);
  return 1;
}
// RSTP_BRIDGE class
int speed2pcost(int speed)
{
   int lret;
   if (speed < 10L) {         /* < 10Mb/s */
        lret = 20000000;
    } else if (speed <= 10L) {        /* 10 Mb/s  */
        lret = 2000000;        
    } else if (speed <= 100L) {       /* 100 Mb/s */
        lret = 200000;     
    } else if (speed <= 1000L) {      /* 1 Gb/s */
        lret = 20000;      
    } else if (speed <= 10000L) {     /* 10 Gb/s */
        lret = 2000;       
    } else if (speed <= 100000L) {    /* 100 Gb/s */
        lret = 200;        
    } else if (speed <= 1000000L) {   /* 1 GTb/s */
        lret = 20;     
    } else if (speed <= 10000000L) {  /* 10 Tb/s */
        lret = 2;      
    } else   /* ??? */                        { /* > Tb/s */
        lret = 1;       
    }
   return lret;
}
//
// Constructor of RSTP bridge
//
rstp_bridge::rstp_bridge(root *node, int nports, double speed[_nports], int duplex[_nports], 
char *basemac)
{
   int i;
   unsigned char wmac[6];

   memcpy(wmac, basemac, 6);

   // Init bridge globals
   max_port = nports;

#ifdef STP_DBG
   dbg_rstp_deny = 0;
#endif
   
   tev = RSTP_EVENT_LAST_DUMMY;
   nev = 0;
   bridges = NULL;

   // init default MAC table flushing in case of  port state changes via STP
   flushtype = LT_FLASH_ALL_PORTS_EXCLUDE_THIS;

   // reference to the Luayats node object
   this->node = node;

   // allocate string for error messages
   CHECK(errmsg = new char[512]);
   *errmsg = '\0';


   // init dynamic entries
   CHECK(this->mac = new unsigned char *[nports]);
   CHECK(this->portname = new char *[nports]);
   for (i = 0; i < nports; i++){
      CHECK(this->mac[i] = new unsigned char[6]);
      CHECK(this->portname[i] = new char[NAME_LEN]);
   }
   CHECK(this->speed = new unsigned long[nports]);
   CHECK(this->pathcost = new unsigned long [nports]);
   CHECK(this->duplex = new int[nports]);

   // init defaults
   for (i = 0; i < nports; i++){
      this->speed[i] = (unsigned long) speed[i];
      this->pathcost[i] = speed2pcost(this->speed[i]);
      this->duplex[i] = duplex[i];
      wmac[5] = wmac[5] + 1;
      memcpy(this->mac[i], wmac, 6);
      snprintf(this->portname[i], NAME_LEN-1, "%s.p%d", node->name,i+1);
   }
   
}

//
// Destructor of RSTP bridge
//
#if 1
rstp_bridge::~rstp_bridge(void)
{
   int i;

   // free dynamic memory
   delete duplex;
   delete pathcost;
   delete speed;
   for (i = 0; i < max_port; i++){
      delete mac[i];
      delete portname[i];
   }
   delete portname;
   delete mac;
   delete errmsg;
}
#else
rstp_bridge::~rstp_bridge(void)
{
   int i;

   // free dynamic memory
   delete errmsg;
   delete portname;
   delete speed;
   delete pathcost;
   for (i = 0; i < max_port; i++)
      delete mac[i];
   delete mac;
}
#endif
// Start the bridge
char *rstp_bridge::start(int vlan_id, int prio, char *memberset)
{
   BITMAP_T  ports;
   UID_STP_CFG_T uid_cfg;
   register int  iii;

   /* send HANDSHAKE */
#if 0
   stp_cli_init ();
#endif

   this->init(max_port);
   BitmapClear(&enabled_ports);
   BitmapClear(&ports);
   for (iii = 1; iii <= max_port; iii++) {
     if ((memberset == NULL) || (*(memberset + iii - 1) == '1'))
       BitmapSetBit(&ports, iii - 1);
   }
  
  uid_cfg.field_mask = BR_CFG_STATE | BR_CFG_PRIO;
  uid_cfg.stp_enabled = STP_ENABLED;
  uid_cfg.bridge_priority = prio;
  snprintf(uid_cfg.vlan_name, NAME_LEN - 1, "%s:%d", node->name, vlan_id);  
  //  strncpy(uid_cfg.vlan_name, node->name, NAME_LEN-1);
  iii = STP_IN_stpm_set_cfg (this, vlan_id, &ports, &uid_cfg);
  if (STP_OK != iii) {
     sprintf(errmsg, "FATAL: can't enable:%s",
	     STP_IN_get_error_explanation (iii));
    return errmsg;
  }
  return NULL;
}

// Register a Lua callback function
void rstp_bridge::setcallback(char *key, lua_Object func)
{
   lua_pushstring(WL, key);
   lua_pushvalue(WL, func);
   lua_settable(WL, LUA_REGISTRYINDEX);
}


// Receive a BPDU and forward it to the control plane
void rstp_bridge::rx_bpdu(int vlan_id, int port_index, unsigned char *bpdu, size_t len)
{
   // Strip the MAC header
   STP_IN_rx_bpdu(this, vlan_id, port_index, (BPDU_T*) (bpdu + sizeof (MAC_HEADER_T)), len -  sizeof(MAC_HEADER_T));
   //   delete bpdu;
}

#define PROLOG(s) char *cbs = s; \
                  lua_pushstring(WL, s);\
                  lua_gettable(WL, LUA_REGISTRYINDEX);\
                  if (lua_isfunction(WL, -1)){ \
                    tolua_pushusertype(WL, this, "rstp_bridge")

#define EPILOG(arg, ret, err) lua_call(WL, arg, ret);\
                            } else {\
                              lua_pushliteral(WL, cbs);\
                              lua_error(WL); \
                            } \
                            return err

// Shutdown the bridge
char * rstp_bridge::shutdown(int vlan_id)
{
  int       rc;

  rc = STP_IN_stpm_delete (this, vlan_id);
  if (STP_OK != rc) {
     sprintf(errmsg, "FATAL: can't delete:%s\n",
	     STP_IN_get_error_explanation (rc));
     return errmsg;
  }
  return NULL;
}

int rstp_bridge::flush_fdb(int port_index, int vlan_id, LT_FLASH_TYPE_T typ, char *reason)
{
   char *cbs = "cb_flush_fdb"; 
   lua_pushstring(WL, cbs);
   lua_gettable(WL, LUA_REGISTRYINDEX);
   if (lua_isfunction(WL, -1)){ 
      tolua_pushusertype(WL, this, "rstp_bridge");
      lua_pushnumber(WL, port_index);
      lua_pushnumber(WL, vlan_id);
      lua_pushnumber(WL, (int) typ);
      lua_pushstring(WL, reason);
      lua_call(WL, 5, 0);
   } else {
      lua_pushstring(WL, cbs);
      lua_error(WL); 
   } 
   return STP_OK;
}

int rstp_bridge::get_mac(int port_index, unsigned char *mac)
{
   memcpy(mac, this->mac[port_index - 1], 6);
   return STP_OK;
}

unsigned long rstp_bridge::get_speed(int port_index)
{
   return speed[port_index-1];
}

unsigned long rstp_bridge::get_init_port_pathcost(int port_index)
{
   return pathcost[port_index-1];
}

int rstp_bridge::get_linkstatus(int port_index)
{
  if (BitmapGetBit (&enabled_ports, (port_index - 1))) 
     return 1;
  else
     return 0;
}

int rstp_bridge::get_duplex(int port_index)
{
   return duplex[port_index-1];
}

int rstp_bridge::set_learning(int port_index, int vlan_id, int enable)
{
   char *cbs = "cb_learning"; 
   lua_pushstring(WL, cbs);
   lua_gettable(WL, LUA_REGISTRYINDEX);
   if (lua_isfunction(WL, -1)){ 
      tolua_pushusertype(WL, this, "rstp_bridge");
      lua_pushnumber(WL, port_index);
      lua_pushnumber(WL, vlan_id);
      lua_pushnumber(WL, enable);
      lua_call(WL, 4, 0);
   } else {
      lua_pushstring(WL, cbs);
      lua_error(WL); 
   } 
   return STP_OK;
}

int rstp_bridge::set_forwarding(int port_index, int vlan_id, int enable)
{
   char *cbs = "cb_forwarding"; 
   lua_pushstring(WL, cbs);
   lua_gettable(WL, LUA_REGISTRYINDEX);
   if (lua_isfunction(WL, -1)){ 
      tolua_pushusertype(WL, this, "rstp_bridge");
      lua_pushnumber(WL, port_index);
      lua_pushnumber(WL, vlan_id);
      lua_pushnumber(WL, enable);
      lua_call(WL, 4, 0);
   } else {
      lua_pushstring(WL, cbs);
      lua_error(WL); 
   } 
   return STP_OK;
}

int rstp_bridge::set_portstate(int port_index, int vlan_id, RSTP_PORT_STATE state)
{
   char *cbs = "cb_portstate"; 
   lua_pushstring(WL, cbs);
   lua_gettable(WL, LUA_REGISTRYINDEX);
   if (lua_isfunction(WL, -1)){ 
      tolua_pushusertype(WL, this, "rstp_bridge");
      lua_pushnumber(WL, port_index);
      lua_pushnumber(WL, vlan_id);
      lua_pushnumber(WL, state);
      //      printf("Calling %s\n", cbs);
      lua_call(WL, 4, 0);
   } else {
     //      printf("ERROR %s\n", cbs);
      lua_pushstring(WL, cbs);
      lua_error(WL); 
   } 
   return STP_OK;
}

int rstp_bridge::set_hardware_mode(int vlan_id, UID_STP_MODE_T mode)
{
   char *cbs = "cb_hardware_mode"; 
   lua_pushstring(WL, cbs);
   lua_gettable(WL, LUA_REGISTRYINDEX);
   if (lua_isfunction(WL, -1)){ 
      tolua_pushusertype(WL, this, "rstp_bridge");
      lua_pushnumber(WL, vlan_id);
      lua_pushnumber(WL, mode);
      lua_call(WL, 3, 0);
   } else {
      lua_pushstring(WL, cbs);
      lua_error(WL); 
   } 
   return STP_OK;
}

const char *rstp_bridge::get_portname(int port_index)
{
   return portname[port_index-1];
}

int rstp_bridge::get_init_stpm_cfg(int vlan_id, UID_STP_CFG_T *cfg)
{
  cfg->bridge_priority =        DEF_BR_PRIO;
  cfg->max_age =                DEF_BR_MAXAGE;
  cfg->hello_time =             DEF_BR_HELLOT;
  cfg->forward_delay =          DEF_BR_FWDELAY;
  cfg->force_version =          NORMAL_RSTP;
  return STP_OK;
}

int rstp_bridge::get_init_port_cfg(int port_index, int vlan_id, UID_STP_PORT_CFG_T *cfg)
{
  cfg->port_priority =                  DEF_PORT_PRIO;
  cfg->admin_non_stp =                  DEF_ADMIN_NON_STP;
  cfg->admin_edge =                     DEF_ADMIN_EDGE;
  cfg->admin_port_path_cost =           ADMIN_PORT_PATH_COST_AUTO;
  cfg->admin_point2point =              DEF_P2P;
  return STP_OK;
}

// Transmit a BPDU
// We carry the frame as Lua string, that is allowed to contain zeros
int rstp_bridge::tx_bpdu(int port_index, int vlan_id, unsigned char *bpdu, size_t len)
{
   char *cbs = "cb_tx_bpdu"; 
   //   RBPDU_T *sbpdu;
   //   CHECK(sbpdu = new RBPDU_T);
   //   memcpy(sbpdu, bpdu, sizeof(RBPDU_T));
   lua_pushstring(WL, cbs);
   lua_gettable(WL, LUA_REGISTRYINDEX);
   if (lua_isfunction(WL, -1)){ 
      tolua_pushusertype(WL, this, "rstp_bridge");
      lua_pushnumber(WL, port_index);
      lua_pushnumber(WL, vlan_id);
      lua_pushlstring(WL, (char *) bpdu, len);
      //      tolua_pushusertype(WL, (RBPDU_T*) sbpdu, "RBPDU_T");
      lua_pushnumber(WL, len);
      lua_call(WL, 5, 0);
   } else {
      lua_pushstring(WL, cbs);
      lua_error(WL); 
   } 
   return STP_OK;
}
#if 0
void *rstp_bridge::new_container(char *which)
{
   void *ptr;
   if (!strcmp(which, "stmpcfg")){
      ptr = lua_newuserdata(WL, sizeof(UID_STP_CFG_T));
      CHECK(ptr = new UID_STP_CFG_T);
   } else if (!strcmp(which, "stmpstate")){
      CHECK(ptr = new UID_STP_STATE_T);
   } else if (!strcmp(which, "portcfg")){
      CHECK(ptr = new UID_STP_PORT_CFG_T);
   } else if (!strcmp(which, "portstate")){
      CHECK(ptr = new UID_STP_PORT_STATE_T);
   }
   return ptr;
}
void rstp_bridge::delete_container(void *ptr)
{
   delete ptr;
}
#endif
