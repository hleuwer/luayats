#include "defs.h"

#define jiffies SimTime
#include "br_private.h"

class bridge_stp: public inxout {
 public:
  bridge_stp(int nports){nports = nports;br = new net_bridge}
  net_bridge *br;
};

class bridge_port: public root {
 public:
  bridge_port(bridge_stp *b){sbr = b; br = sbr->br}
  struct net_bridge *br;
  bridge_stp *sbr;
}
