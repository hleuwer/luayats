/***************************************************************************
*
* SPANNING TREE ALGORITHM AND PROTOCOL
*
**************************************************************************/
/***************************************************************************
* DEFINED CONSTANTS
**************************************************************************/
#define Zero 0
#define One 1
#define False 0
#define True 1
/** port states. **/
#define Disabled 0		/* (8.4.5) */
#define Listening 1		/* (8.4.2) */
#define Learning 2		/* (8.4.3) */
#define Forwarding 3		/* (8.4.4) */
#define Blocking 4		/* (8.4.1) */
/** BPDU type constants **/
#define Config_bpdu_type 0
#define Tcn_bpdu_type 128
/** pseudo-implementation constants. **/
#define No_of_ports 2
/* arbitrary choice, to allow the code below to compile */
#define All_ports No_of_ports+1
/* ports start at 1, arrays in C start at 0 */
#define Default_path_cost 10
/* arbitrary */
#define Message_age_increment 1
/* minimum increment possible to avoid underestimating age, allows
   for BPDU transmission time */
#define No_port 0
/* reserved value for Bridge's root port parameter indicating no
   root port, used when Bridge is the root */
/***************************************************************************
* TYPEDEFS, STRUCTURES, AND UNION DECLARATIONS
**************************************************************************/
/** basic types. **/
typedef int Int;		/* to align with convention used here for use of
				   case. Types and defined constants have their
				   initial letters capitalized. */
typedef Int Boolean;		/* : (True, False) */
typedef Int State;		/* : (Disabled, Listening, Learning,
				   Forwarding, Blocking) */
/* 
   BPDU encoding types defined in Clause 9, "Encoding of Bridge Protocol
   Data Units" are:
   Protocol_version (9.2.2)
   Bpdu_type (9.2.3)
   Flag (9.2.4)
   Identifier (9.2.5)
   Cost (9.2.6)
   Port_id (9.2.7)
   Time (9.2.8)
*/
#include "stptypes.h"		/* defines BPDU encoding types */
/** Configuration BPDU Parameters (8.5.1) **/
typedef struct {
  Bpdu_type type;
  Identifier root_id;		/* (8.5.1.1) */
  Cost root_path_cost;		/* (8.5.1.2) */
  Identifier bridge_id;		/* (8.5.1.3) */
  Port_id port_id;		/* (8.5.1.4) */
  Time message_age;		/* (8.5.1.5) */
  Time max_age;			/* (8.5.1.6) */
  Time hello_time;		/* (8.5.1.7) */
  Time forward_delay;		/* (8.5.1.8) */
  Flag topology_change_acknowledgment;	/* (8.5.1.9) */
  Flag topology_change;		/* (8.5.1.10) */
} Config_bpdu;

/** Topology Change Notification BPDU Parameters (8.5.2) **/
typedef struct {
  Bpdu_type type;
} Tcn_bpdu;

/** Bridge Parameters (8.5.3) **/
typedef struct {
  Identifier designated_root;	/* (8.5.3.1) */
  Cost root_path_cost;		/* (8.5.3.2) */
  Int root_port;		/* (8.5.3.3) */
  Time max_age;			/* (8.5.3.4) */
  Time hello_time;		/* (8.5.3.5) */
  Time forward_delay;		/* (8.5.3.6) */
  Identifier bridge_id;		/* (8.5.3.7) */
  Time bridge_max_age;		/* (8.5.3.8) */
  Time bridge_hello_time;	/* (8.5.3.9) */
  Time bridge_forward_delay;	/* (8.5.3.10) */
  Boolean topology_change_detected;	/* (8.5.3.11) */
  Boolean topology_change;	/* (8.5.3.12) */
  Time topology_change_time;	/* (8.5.3.13) */
  Time hold_time;		/* (8.5.3.14) */
} Bridge_data;

/** Port Parameters (8.5.5) **/
typedef struct {
  Port_id port_id;		/* (8.5.5.1) */
  State state;			/* (8.5.5.2) */
  Int path_cost;		/* (8.5.5.3) */
  Identifier designated_root;	/* (8.5.5.4) */
  Int designated_cost;		/* (8.5.5.5) */
  Identifier designated_bridge;	/* (8.5.5.6) */
  Port_id designated_port;	/* (8.5.5.7) */
  Boolean topology_change_acknowledge;	/* (8.5.5.8) */
  Boolean config_pending;	/* (8.5.5.9) */
  Boolean change_detection_enabled;	/* (8.5.5.10) */
} Port_data;

/** types to support timers for this pseudo-implementation. **/
typedef struct {
  Boolean active;		/* timer in use. */
  Time value;			/* current value of timer, counting up. */
} Timer;

/***************************************************************************
* STATIC STORAGE ALLOCATION
**************************************************************************/
Bridge_data bridge_info;	/* (8.5.3) */
Port_data port_info[All_ports];	/* (8.5.5) */
Config_bpdu config_bpdu[All_ports];
Tcn_bpdu tcn_bpdu[All_ports];
Timer hello_timer;		/* (8.5.4.1) */
Timer tcn_timer;		/* (8.5.4.2) */
Timer topology_change_timer;	/* (8.5.4.3) */
Timer message_age_timer[All_ports];	/* (8.5.6.1) */
Timer forward_delay_timer[All_ports];	/* (8.5.6.2) */
Timer hold_timer[All_ports];	/* (8.5.6.3) */

/***************************************************************************
* CODE
**************************************************************************/
/** Elements of Procedure (8.6) **/
transmit_config(port_no)	/* (8.6.1) */
Int port_no;
{
  if (hold_timer[port_no].active) {	/* (8.6.1.3.1) */
    port_info[port_no].config_pending = True;	/* (8.6.1.3.1) */
  } else {			/* (8.6.1.3.2) */

    config_bpdu[port_no].type = Config_bpdu_type;
    config_bpdu[port_no].root_id = bridge_info.designated_root;
  /* (8.6.1.3.2(a)) */
    config_bpdu[port_no].root_path_cost = bridge_info.root_path_cost;
  /* (8.6.1.3.2(b)) */
    config_bpdu[port_no].bridge_id = bridge_info.bridge_id;
  /* (8.6.1.3.2(c)) */
    config_bpdu[port_no].port_id = port_info[port_no].port_id;
    if (root_bridge()) {
      config_bpdu[port_no].message_age = Zero;	/* (8.6.1.3.2(e)) */
    } else {
      config_bpdu[port_no].message_age = message_age_timer[bridge_info.root_port].value + Message_age_increment;	/* (8.6.1.3.2(f)) */
    }
    config_bpdu[port_no].max_age = bridge_info.max_age;	/* (8.6.1.3.2(g)) */
    config_bpdu[port_no].hello_time = bridge_info.hello_time;
    config_bpdu[port_no].forward_delay = bridge_info.forward_delay;
    config_bpdu[port_no].topology_change_acknowledgment
	= port_info[port_no].topology_change_acknowledge;
  /* (8.6.1.3.2(h)) */
    config_bpdu[port_no].topology_change = bridge_info.topology_change;	/* (8.6.1.3.2(i)) */
    if (config_bpdu[port_no].message_age < bridge_info.max_age) {
      port_info[port_no].topology_change_acknowledge = False;
    /* (8.6.1.3.3) */
      port_info[port_no].config_pending = False;	/* (8.6.1.3.3) */
      send_config_bpdu(port_no, &config_bpdu[port_no]);
      start_hold_timer(port_no);	/* (8.6.3.3(b)) */
    }
  }
}

/* where
   send_config_bpdu(port_no,bpdu)
   Int port_no;
   Config_bpdu *bpdu;
   is a pseudo-implementation specific routine that transmits
   the bpdu on the specified port within the specified time.
 */
/* and */
Boolean root_bridge()
{
  return (bridge_info.designated_root == bridge_info.bridge_id);
}

Boolean supersedes_port_info(port_no, config)	/* (8.6.2.2) */
Int port_no;
Config_bpdu *config;
{
  return ((config->root_id < port_info[port_no].designated_root	/* (8.6.2.2 a) */
	  )
	  || ((config->root_id == port_info[port_no].designated_root)
	      && ((config->root_path_cost < port_info[port_no].designated_cost	/* (8.6.2.2 b) */
		  )
		  ||
		  ((config->root_path_cost
		    == port_info[port_no].designated_cost)
		   && ((config->bridge_id < port_info[port_no].designated_bridge	/* (8.6.2.2 c) */
		       )
		       || ((config->bridge_id == port_info[port_no].designated_bridge)	/* (8.6.2.2 d) */
			   &&((config->bridge_id != bridge_info.bridge_id)	/* (8.6.2.2 d1) */
			      ||(config->port_id <= port_info[port_no].designated_port)	/* (8.6.2.2 d2) */
			   ))))))
      );
}

record_config_information(port_no, config)	/* (8.6.2) */
Int port_no;
Config_bpdu *config;
{
  port_info[port_no].designated_root = config->root_id;	/* (8.6.2.3.1) */
  port_info[port_no].designated_cost = config->root_path_cost;
  port_info[port_no].designated_bridge = config->bridge_id;
  port_info[port_no].designated_port = config->port_id;
  start_message_age_timer(port_no, config->message_age);	/* (8.6.2.3.2) */
}

record_config_timeout_values(config)	/* (8.6.3) */
Config_bpdu *config;
{
  bridge_info.max_age = config->max_age;	/* (8.6.3.3) */
  bridge_info.hello_time = config->hello_time;
  bridge_info.forward_delay = config->forward_delay;
  bridge_info.topology_change = config->topology_change;
}

config_bpdu_generation()
{				/* (8.6.4) */
  Int port_no;

  for (port_no = One; port_no <= No_of_ports; port_no++) {	/* (8.6.4.3) */
    if (designated_port(port_no)	/* (8.6.4.3) */
	&&(port_info[port_no].state != Disabled)
	) {
      transmit_config(port_no);	/* (8.6.4.3) */
    }				/* (8.6.1.2) */
  }
}

/* where */
Boolean designated_port(port_no)
Int port_no;
{
  return ((port_info[port_no].designated_bridge == bridge_info.bridge_id)
	  &&
	  (port_info[port_no].designated_port
	   == port_info[port_no].port_id)
      );
}

reply(port_no)			/* (8.6.5) */
Int port_no;
{
  transmit_config(port_no);	/* (8.6.5.3) */
}

transmit_tcn()
{				/* (8.6.6) */
  Int port_no;

  port_no = bridge_info.root_port;
  tcn_bpdu[port_no].type = Tcn_bpdu_type;
  send_tcn_bpdu(port_no, &tcn_bpdu[bridge_info.root_port]);	/* (8.6.6.3) */
}

/* where
   send_tcn_bpdu(port_no,bpdu)
   Int port_no;
   Tcn_bpdu *bpdu;
   is a pseudo-implementation-specific routine that transmits
   the bpdu on the specified port within the specified time.
 */
configuration_update()
{				/* (8.6.7) */
  root_selection();		/* (8.6.7.3.1) */
/* (8.6.8.2) */
  designated_port_selection();	/* (8.6.7.3.2) */
/* (8.6.9.2) */
}

root_selection()
{				/* (8.6.8) */
  Int root_port;
  Int port_no;

  root_port = No_port;
  for (port_no = One; port_no <= No_of_ports; port_no++) {	/* (8.6.8.3.1) */
    if (((!designated_port(port_no))
	 && (port_info[port_no].state != Disabled)
	 && (port_info[port_no].designated_root < bridge_info.bridge_id)
	)
	&& ((root_port == No_port)
	    || (port_info[port_no].designated_root < port_info[root_port].designated_root	/* (8.6.8.3.1(a)) */
	    )
	    ||
	    ((port_info[port_no].designated_root
	      == port_info[root_port].designated_root)
	     &&
	     (((port_info[port_no].designated_cost
		+ port_info[port_no].path_cost)
	       < (port_info[root_port].designated_cost + port_info[root_port].path_cost)	/* (8.6.8.3.1(b)) */
	      )
	      ||
	      (((port_info[port_no].designated_cost
		 + port_info[port_no].path_cost)
		==
		(port_info[root_port].designated_cost
		 + port_info[root_port].path_cost)
	       )
	       && ((port_info[port_no].designated_bridge < port_info[root_port].designated_bridge)	/* (8.6.8.3.1(c)) */
		   ||
		   ((port_info[port_no].designated_bridge
		     == port_info[root_port].designated_bridge)
		    && ((port_info[port_no].designated_port < port_info[root_port].designated_port)	/* (8.6.8.3.1(d)) */
			||
			((port_info[port_no].designated_port
			  == port_info[root_port].designated_port)
			 && (port_info[port_no].port_id < port_info[root_port].port_id)	/* (8.6.8.3.1(e)) */
			))))))))) {
      root_port = port_no;
    }
  }
  bridge_info.root_port = root_port;	/* (8.6.8.3.1) */
  if (root_port == No_port) {	/* (8.6.8.3.2) */
    bridge_info.designated_root = bridge_info.bridge_id;
  /* (8.6.8.3.2(a)) */
    bridge_info.root_path_cost = Zero;	/* (8.6.8.3.2(b)) */
  } else {			/* (8.6.8.3.3) */

    bridge_info.designated_root = port_info[root_port].designated_root;
  /* (8.6.8.3.3(a)) */
    bridge_info.root_path_cost = (port_info[root_port].designated_cost + port_info[root_port].path_cost);	/* (8.6.8.3.3(b)) */
  }
}

designated_port_selection()
{				/* (8.6.9) */
  Int port_no;

  for (port_no = One; port_no <= No_of_ports; port_no++) {	/* (8.6.9.3) */
    if (designated_port(port_no)	/* (8.6.9.3 a) */
	||(port_info[port_no].designated_root != bridge_info.designated_root	/* (8.6.9.3 b) */
	)
	|| (bridge_info.root_path_cost < port_info[port_no].designated_cost)	/* (8.6.9.3 c) */
	||
	((bridge_info.root_path_cost == port_info[port_no].designated_cost)
	 && ((bridge_info.bridge_id < port_info[port_no].designated_bridge)	/* (8.6.9.3 d) */
	     ||
	     ((bridge_info.bridge_id
	       == port_info[port_no].designated_bridge)
	      && (port_info[port_no].port_id <= port_info[port_no].designated_port)	/* (8.6.9.3 e) */
	     )))) {
      become_designated_port(port_no);	/* (8.6.10.2 a) */
    }
  }
}

become_designated_port(port_no)	/* (8.6.10) */
Int port_no;
{
  port_info[port_no].designated_root = bridge_info.designated_root;
/* (8.6.10.3 a) */
  port_info[port_no].designated_cost = bridge_info.root_path_cost;
/* (8.6.10.3 b) */
  port_info[port_no].designated_bridge = bridge_info.bridge_id;
/* (8.6.10.3 c) */
  port_info[port_no].designated_port = port_info[port_no].port_id;
/* (8.6.10.3 d) */
}

port_state_selection()
{				/* (8.6.11) */
  Int port_no;

  for (port_no = One; port_no <= No_of_ports; port_no++) {
    if (port_no == bridge_info.root_port) {	/* (8.6.11.3 a) */
      port_info[port_no].config_pending = False;	/* (8.6.11.3 a1) */
      port_info[port_no].topology_change_acknowledge = False;
      make_forwarding(port_no);	/* (8.6.11.3 a2) */
    } else if (designated_port(port_no)) {	/* (8.6.11.3 b) */
      stop_message_age_timer(port_no);	/* (8.6.11.3 b1) */
      make_forwarding(port_no);	/* (8.6.11.3 b2) */
    } else {			/* (8.6.11.3 c) */

      port_info[port_no].config_pending = False;	/* (8.6.11.3 c1) */
      port_info[port_no].topology_change_acknowledge = False;
      make_blocking(port_no);	/* (8.6.11.3 c2) */
    }
  }
}

make_forwarding(port_no)	/* (8.6.12) */
Int port_no;
{
  if (port_info[port_no].state == Blocking) {	/* (8.6.12.3) */
    set_port_state(port_no, Listening);	/* (8.6.12.3 a) */
    start_forward_delay_timer(port_no);	/* (8.6.12.3 b) */
  }
}

make_blocking(port_no)		/* (8.6.13) */
Int port_no;
{
  if ((port_info[port_no].state != Disabled)
      && (port_info[port_no].state != Blocking)
      ) {			/* (8.6.13.3) */
    if ((port_info[port_no].state == Forwarding)
	|| (port_info[port_no].state == Learning)
	) {
      if (port_info[port_no].change_detection_enabled == True)
      /* (8.5.5.10) */
      {
	topology_change_detection();	/* (8.6.13.3 a) */
      }				/* (8.6.14.2.3) */
    }
    set_port_state(port_no, Blocking);	/* (8.6.13.3 b) */
    stop_forward_delay_timer(port_no);	/* (8.6.13.3 c) */
  }
}

/* where */
set_port_state(port_no, state)
Int port_no;
State state;
{
  port_info[port_no].state = state;
}

topology_change_detection()
{				/* (8.6.14) */
  if (root_bridge()) {		/* (8.6.14.3 a) */
    bridge_info.topology_change = True;	/* (8.6.14.3 a1) */
    start_topology_change_timer();	/* (8.6.14.3 a2) */
  } else if (bridge_info.topology_change_detected == False) {	/* (8.6.14.3 b) */
    transmit_tcn();		/* (8.6.14.3 b1) */
    start_tcn_timer();		/* (8.6.14.3 b2) */
  }
  bridge_info.topology_change_detected = True;	/* (8.6.14.3 c) */
}

topology_change_acknowledged()
{				/* (8.6.15) */
  bridge_info.topology_change_detected = False;	/* (8.6.15.3 a) */
  stop_tcn_timer();		/* (8.6.15.3 b) */
}

acknowledge_topology_change(port_no)	/* (8.6.16) */
Int port_no;
{
  port_info[port_no].topology_change_acknowledge = True;	/* (8.6.16.3 a) */
  transmit_config(port_no);	/* (8.6.16.3 b) */
}

/** Operation of the Protocol (8.7) **/
received_config_bpdu(port_no, config)	/* (8.7.1) */
Int port_no;
Config_bpdu *config;
{
  Boolean root;

  root = root_bridge();
  if (port_info[port_no].state != Disabled) {
    if (supersedes_port_info(port_no, config)) {	/* (8.7.1.1) *//* (8.6.2.2) */
      record_config_information(port_no, config);	/* (8.7.1.1 a) */
    /* (8.6.2.2) */
      configuration_update();	/* (8.7.1.1 b) */
    /* (8.6.7.2 a) */
      port_state_selection();	/* (8.7.1.1 c) */
    /* (8.6.11.2 a) */
      if ((!root_bridge()) && root) {	/* (8.7.1.1 d) */
	stop_hello_timer();
	if (bridge_info.topology_change_detected) {	/* (8.7.1.1 e) */
	  stop_topology_change_timer();
	  transmit_tcn();	/* (8.6.6.1) */
	  start_tcn_timer();
	}
      }
      if (port_no == bridge_info.root_port) {
	record_config_timeout_values(config);	/* (8.7.1.1 e) */
      /* (8.6.3.2) */
	config_bpdu_generation();	/* (8.6.4.2 a) */
	if (config->topology_change_acknowledgment) {	/* (8.7.1.1 g) */
	  topology_change_acknowledged();	/* (8.6.15.2) */
	}
      }
    } else if (designated_port(port_no)) {	/* (8.7.1.2) */
      reply(port_no);		/* (8.7.1.2) */
    /* (8.6.5.2) */
    }
  }
}

received_tcn_bpdu(port_no, tcn)	/* (8.7.2) */
Int port_no;
Tcn_bpdu *tcn;
{
  if (port_info[port_no].state != Disabled) {
    if (designated_port(port_no)) {
      topology_change_detection();	/* (8.7.2 a) */
    /* (8.6.14.2.1) */
      acknowledge_topology_change(port_no);	/* (8.7.2 b) */
    }				/* (8.6.16.2) */
  }
}

hello_timer_expiry()
{				/* (8.7.3) */
  config_bpdu_generation();	/* (8.6.4.2 b) */
  start_hello_timer();
}

message_age_timer_expiry(port_no)	/* (8.7.4) */
Int port_no;
{
  Boolean root;

  root = root_bridge();
  become_designated_port(port_no);	/* (8.7.4 a) */
/* (8.6.10.2 b) */
  configuration_update();	/* (8.7.4 b) */
/* (8.6.7.2 b) */
  port_state_selection();	/* (8.7.4 c) */
/* (8.6.11.2 b) */
  if ((root_bridge()) && (!root)) {	/* (8.7.4 d) */
    bridge_info.max_age = bridge_info.bridge_max_age;	/* (8.7.4 d1) */
    bridge_info.hello_time = bridge_info.bridge_hello_time;
    bridge_info.forward_delay = bridge_info.bridge_forward_delay;
    topology_change_detection();	/* (8.7.4 d2) */
  /* (8.6.14.2.4) */
    stop_tcn_timer();		/* (8.7.4 d3) */
    config_bpdu_generation();	/* (8.7.4 d4) */
    start_hello_timer();
  }
}

forward_delay_timer_expiry(port_no)	/* (8.7.5) */
Int port_no;
{
  if (port_info[port_no].state == Listening) {	/* (8.7.5 a) */
    set_port_state(port_no, Learning);	/* (8.7.5 a1) */
    start_forward_delay_timer(port_no);	/* (8.7.5 a2) */
  } else if (port_info[port_no].state == Learning) {	/* (8.7.5 b) */
    set_port_state(port_no, Forwarding);	/* (8.7.5 b1) */
    if (designated_for_some_port()) {	/* (8.7.5 b2) */
      if (port_info[port_no].change_detection_enabled == True)
      /* (8.5.5.10) */
      {
	topology_change_detection();	/* (8.6.14.2.2) */
      }
    }
  }
}

/* where */
Boolean designated_for_some_port()
{
  Int port_no;

  for (port_no = One; port_no <= No_of_ports; port_no++) {
    if (port_info[port_no].designated_bridge == bridge_info.bridge_id) {
      return (True);
    }
  }
  return (False);
}

tcn_timer_expiry()
{				/* (8.7.6) */
  transmit_tcn();		/* (8.7.6 a) */
  start_tcn_timer();		/* (8.7.6 b) */
}

topology_change_timer_expiry()
{				/* (8.7.7) */
  bridge_info.topology_change_detected = False;	/* (8.7.7 a) */
  bridge_info.topology_change = False;	/* (8.7.7 b) */
}

hold_timer_expiry(port_no)	/* (8.7.8) */
Int port_no;
{
  if (port_info[port_no].config_pending) {
    transmit_config(port_no);	/* (8.6.1.2) */
  }
}

/** Management of the Bridge Protocol Entity (8.8) **/
initialisation()
{				/* (8.8.1) */
  Int port_no;

  bridge_info.designated_root = bridge_info.bridge_id;	/* (8.8.1 a) */
  bridge_info.root_path_cost = Zero;
  bridge_info.root_port = No_port;
  bridge_info.max_age = bridge_info.bridge_max_age;	/* (8.8.1 b) */
  bridge_info.hello_time = bridge_info.bridge_hello_time;
  bridge_info.forward_delay = bridge_info.bridge_forward_delay;
  bridge_info.topology_change_detected = False;	/* (8.8.1 c) */
  bridge_info.topology_change = False;
  stop_tcn_timer();
  stop_topology_change_timer();
  for (port_no = One; port_no <= No_of_ports; port_no++) {	/* (8.8.1 d) */
    initialize_port(port_no);
  }
  port_state_selection();	/* (8.8.1 e) */
  config_bpdu_generation();	/* (8.8.1 f) */
  start_hello_timer();
}

/*
 */
initialize_port(port_no)
Int port_no;
{
  become_designated_port(port_no);	/* (8.8.1 d1) */
  set_port_state(port_no, Blocking);	/* (8.8.1 d2) */
  port_info[port_no].topology_change_acknowledge = False;
/* (8.8.1 d3) */
  port_info[port_no].config_pending = False;	/* (8.8.1 d4) */
  port_info[port_no].change_detection_enabled = True;	/* (8.8.1 d8) */
  stop_message_age_timer(port_no);	/* (8.8.1 d5) */
  stop_forward_delay_timer(port_no);	/* (8.8.1 d6) */
  stop_hold_timer(port_no);	/* (8.8.1 d7) */
}

enable_port(port_no)		/* (8.8.2) */
Int port_no;
{
  initialize_port(port_no);
  port_state_selection();	/* (8.8.2 g) */
}

/*
 */
disable_port(port_no)		/* (8.8.3) */
Int port_no;
{
  Boolean root;

  root = root_bridge();
  become_designated_port(port_no);	/* (8.8.3 a) */
  set_port_state(port_no, Disabled);	/* (8.8.3 b) */
  port_info[port_no].topology_change_acknowledge = False;	/* (8.8.3 c) */
  port_info[port_no].config_pending = False;	/* (8.8.3 d) */
  stop_message_age_timer(port_no);	/* (8.8.3 e) */
  stop_forward_delay_timer(port_no);	/* (8.8.3 f) */
  configuration_update();	/* (8.8.3 g) */
  port_state_selection();	/* (8.8.3 h) */
  if ((root_bridge()) && (!root)) {	/* (8.8.3 i) */
    bridge_info.max_age = bridge_info.bridge_max_age;	/* (8.8.3 i1) */
    bridge_info.hello_time = bridge_info.bridge_hello_time;
    bridge_info.forward_delay = bridge_info.bridge_forward_delay;
    topology_change_detection();	/* (8.8.3 i2) */
    stop_tcn_timer();		/* (8.8.3 i3) */
    config_bpdu_generation();	/* (8.8.3 i4) */
    start_hello_timer();
  }
}

set_bridge_priority(new_bridge_id)	/* (8.8.4) */
Identifier new_bridge_id;	/* (8.8.4 a) */
{
  Boolean root;
  Int port_no;

  root = root_bridge();
  for (port_no = One; port_no <= No_of_ports; port_no++) {	/* (8.8.4 b) */
    if (designated_port(port_no)) {
      port_info[port_no].designated_bridge = new_bridge_id;
    }
  }
  bridge_info.bridge_id = new_bridge_id;	/* (8.8.4 c) */
  configuration_update();	/* (8.8.4 d) */
  port_state_selection();	/* (8.8.4 e) */
  if ((root_bridge()) && (!root)) {	/* (8.8.4 f) */
    bridge_info.max_age = bridge_info.bridge_max_age;	/* (8.8.4 f1) */
    bridge_info.hello_time = bridge_info.bridge_hello_time;
    bridge_info.forward_delay = bridge_info.bridge_forward_delay;
    topology_change_detection();	/* (8.8.4 f2) */
    stop_tcn_timer();		/* (8.8.4 f3) */
    config_bpdu_generation();	/* (8.8.4 f4) */
    start_hello_timer();
  }
}

set_port_priority(port_no, new_port_id)	/* (8.8.5) */
Int port_no;
Port_id new_port_id;		/* (8.8.5 a) */
{
  if (designated_port(port_no)) {	/* (8.8.5 b) */
    port_info[port_no].designated_port = new_port_id;
  }
  port_info[port_no].port_id = new_port_id;	/* (8.8.5 c) */
  if ((bridge_info.bridge_id	/* (8.8.5 d) */
       == port_info[port_no].designated_bridge)
      && (port_info[port_no].port_id < port_info[port_no].designated_port)
      ) {
    become_designated_port(port_no);	/* (8.8.5 d1) */
    port_state_selection();	/* (8.8.5 d2) */
  }
}

/*
 */
set_path_cost(port_no, path_cost)	/* (8.8.6) */
Int port_no;
Cost path_cost;
{
  port_info[port_no].path_cost = path_cost;	/* (8.8.6 a) */
  configuration_update();	/* (8.8.6 b) */
  port_state_selection();	/* (8.8.6 c) */
}

/*
 */
enable_change_detection(port_no)	/* (8.8.7) */
Int port_no;
{
  port_info[port_no].change_detection_enabled = True;
}

/*
 */
disable_change_detection(port_no)	/* (8.8.8) */
Int port_no;
{
  port_info[port_no].change_detection_enabled = False;
}

/** pseudo-implementation-specific timer running support **/
tick()
{
  Int port_no;

  if (hello_timer_expired()) {
    hello_timer_expiry();
  }
  if (tcn_timer_expired()) {
    tcn_timer_expiry();
  }
  if (topology_change_timer_expired()) {
    topology_change_timer_expiry();
  }
  for (port_no = One; port_no <= No_of_ports; port_no++) {
    if (message_age_timer_expired(port_no)) {
      message_age_timer_expiry(port_no);
    }
  }
  for (port_no = One; port_no <= No_of_ports; port_no++) {
    if (forward_delay_timer_expired(port_no)) {
      forward_delay_timer_expiry(port_no);
    }
    if (hold_timer_expired(port_no)) {
      hold_timer_expiry(port_no);
    }
  }
}

/* where */
start_hello_timer()
{
  hello_timer.value = (Time) Zero;
  hello_timer.active = True;
}

stop_hello_timer()
{
  hello_timer.active = False;
}

Boolean hello_timer_expired()
{
  if (hello_timer.active
      && (++hello_timer.value >= bridge_info.hello_time)) {
    hello_timer.active = False;
    return (True);
  }
  return (False);
}

start_tcn_timer()
{
  tcn_timer.value = (Time) Zero;
  tcn_timer.active = True;
}

stop_tcn_timer()
{
  tcn_timer.active = False;
}

Boolean tcn_timer_expired()
{
  if (tcn_timer.active
      && (++tcn_timer.value >= bridge_info.bridge_hello_time)) {
    tcn_timer.active = False;
    return (True);
  }
  return (False);
}

start_topology_change_timer()
{
  topology_change_timer.value = (Time) Zero;
  topology_change_timer.active = True;
}

stop_topology_change_timer()
{
  topology_change_timer.active = False;
}

Boolean topology_change_timer_expired()
{
  if (topology_change_timer.active
      && (++topology_change_timer.value
	  >= bridge_info.topology_change_time)
      ) {
    topology_change_timer.active = False;
    return (True);
  }
  return (False);
}

start_message_age_timer(port_no, message_age)
Int port_no;
Time message_age;
{
  message_age_timer[port_no].value = message_age;
  message_age_timer[port_no].active = True;
}

stop_message_age_timer(port_no)
Int port_no;
{
  message_age_timer[port_no].active = False;
}

Boolean message_age_timer_expired(port_no)
Int port_no;
{
  if (message_age_timer[port_no].active &&
      (++message_age_timer[port_no].value >= bridge_info.max_age)) {
    message_age_timer[port_no].active = False;
    return (True);
  }
  return (False);
}

start_forward_delay_timer(port_no)
Int port_no;
{
  forward_delay_timer[port_no].value = Zero;
  forward_delay_timer[port_no].active = True;
}

stop_forward_delay_timer(port_no)
Int port_no;
{
  forward_delay_timer[port_no].active = False;
}

Boolean forward_delay_timer_expired(port_no)
Int port_no;
{
  if (forward_delay_timer[port_no].active &&
      (++forward_delay_timer[port_no].value >= bridge_info.forward_delay))
  {
    forward_delay_timer[port_no].active = False;
    return (True);
  }
  return (False);
}

start_hold_timer(port_no)
Int port_no;
{
  hold_timer[port_no].value = Zero;
  hold_timer[port_no].active = True;
}

stop_hold_timer(port_no)
Int port_no;
{
  hold_timer[port_no].active = False;
}

Boolean hold_timer_expired(port_no)
Int port_no;
{
  if (hold_timer[port_no].active &&
      (++hold_timer[port_no].value >= bridge_info.hold_time)) {
    hold_timer[port_no].active = False;
    return (True);
  }
  return (False);
}

/** pseudo-implementation specific transmit routines **/
#include ôtransmit.cö
