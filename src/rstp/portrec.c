/**************************************************/

/*Die neue ZustandMaschine ist aus 802.1D*/

/**************************************************/
#define this _this
#include "base.h"
#include "stpm.h"

/* The Port Receive State Machine: 17.23 aus 802.1D*/
#define STATES { \
  CHOOSE(DISCARD),   \
  CHOOSE(RECEIVE),    \
}
  
#define GET_STATE_NAME STP_portrec_get_state_name
#include "choose.h"

#define DEFAULT_LINK_DELAY  3
#define MigrateTime 3 /* 17,13.9 checked: leu */

/* 17.21.22 checked: leu */
static Bool updtBPDUVersion (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;

  if (BPDU_TOPO_CHANGE_TYPE == port->msgBpduType) {
    port->rcvdSTP = True;
  }

  if (port->msgBpduVersion < 2) {
    port->rcvdSTP = True;
  }
  
  if (BPDU_RSTP == port->msgBpduType) {
    port->rcvdRSTP = True;
  }

  return True;
}

void STP_portrec_enter_state (STATE_MACH_T* this)
{
  register PORT_T*	port = this->owner.port;
  
  switch (this->State) {
  case BEGIN:
  case DISCARD:
    port->rcvdBpdu = False;
    port->rcvdRSTP = False;
    port->rcvdSTP = False;
    port->rcvdMsg = False;
    port->edgeDelayWhile = MigrateTime;
    break;
  case RECEIVE:
    updtBPDUVersion(this);
    port->operEdge =False;
    port->rcvdBpdu = False;
    port->rcvdMsg = True;
    port->edgeDelayWhile = MigrateTime;
    break;
  }
}

Bool STP_portrec_check_conditions (STATE_MACH_T* this)
{
  register PORT_T* port = this->owner.port;
  
  if ((!portEnabled(port) && ((port->edgeDelayWhile != MigrateTime) || 
			      port->rcvdBpdu)) || BEGIN == this->State) {
    return STP_hop_2_state (this, DISCARD);
  }
  
  switch (this->State) {
  case DISCARD:
    if (port->rcvdBpdu && portEnabled(port)){
      return STP_hop_2_state (this, RECEIVE);
    }
    break;
  case RECEIVE:
    if (port->rcvdBpdu && portEnabled(port) && !port->rcvdMsg) {
      return STP_hop_2_state (this, RECEIVE);
    }
    break;
  }
  return False;
}
