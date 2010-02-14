#ifndef DISTSRC_H
#define DISTSRC_H

#include "defs.h"
#include "in1out.h"
#include "special.h"

//tolua_begin
class distsrc: public in1out {
  typedef	in1out baseclass;
  
public:
  distsrc();
  ~distsrc();
  void	early(event *);
  int act(void);
  void setTable(void *tab){table = (tim_typ*)tab;}
  void *getTable(void){return table;}
//tolua_end
  tim_typ *table;
//tolua_begin
  root *dist;
}; 
//tolua_end

#endif
