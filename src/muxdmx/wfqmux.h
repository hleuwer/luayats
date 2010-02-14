/*************************************************************************
*
*  YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997 Chair for Telecommunications
*    Dresden University of Technology
*    D-01062 Dresden
*    Germany
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
**************************************************************************
*
* Module author:  Sven Forner, TUD (student's thesis)
*    Matthias Baumann, TUD
* Creation:  Oct/Nov 1996
*
* History:
* July 17, 1997:  - different error message for unregistered
*      VCI. (old version: same message as for
*      out-of-range VCI).
*     Matthias Baumann
*
*************************************************************************/

/*
* Multiplexer with Waited Fair Queueing according to J.W. Roberts
*
* MuxWFQ mux: NINP=10, {MAXVCI=100}, OUT=line;
*   // default MAXVCI: NINP
*
* The per-connection parameters are set by command.
* Incomming cells with a VCI not yet set with SetPar() cause an error message.
*
* Commands
* ========
*  mux->SetPar(vci, delta, buff);
*  // set inverse mean cell rate and buffer size for connection vci
*
* Other commands: as Multiplexer, see file "mux.c".
*
* Exportet Variables
* ==================
*  mux->QLenVCI(vc) // input queue length of the vc
*  mux->SpacingTime // current spacing time
*
* WFQ Algorithm
* =============
* See User's manual.
*
* Implementation Details
* ======================
* For each VC, there is a structure (type wfqpar) holding the parameters
* mean cell distance and queue size as well the per-VC queue itself.
* The sort queue at the mux output does not contain cells, but the wfqpar
* structures of those VC which currently have a cell in their queue.
* Therefore the information whether a queue containes cells also says
* whether this VC currently in the sort queue (used in late() where incomming
* cells are put into the right queues).
* When serving the output of the multiplexer, the front wfqpar struct is taken
* from the sort queue, and a cell is dequeued from the associated per-VC queue.
* If this queue afterwards is not yet empty, then the wfqpar struct again
* is queued in the sort queue, according to the new virtual time.
* The virtual time of a connection (i.e. of the first cell in the per-VC queue)
* is stored in the wfqpar member time (inherited from cell).
*
* Since the Spacing Time is growing relatively fast, it is reset when
* exceeding the constant spacTimeMax.
*/

#include "mux.h"
#include "oqueue.h"

//tolua_begin
class wfqpar: public ino {
  typedef ino baseclass;
 public:
  wfqpar(void){}
  ~wfqpar(void){
    //  printf("#2# wfqpar destruct\n");
  }
  // Derived from cell since we want to use a normal queue.
  // The cell member time holds the virtual time of the next cell
  // to be served from this VC.
  // The cell member vci holds the VCI of the connection, needed
  // in early() to update the current queue length.
  int vc;
  queue q;   // a queue per VC
  int delta; // inverse of mean rate of this VC
};

class muxWFQ: public mux {
  typedef mux baseclass;
public:
  muxWFQ();
  ~muxWFQ();
  int act(void);
  void setQueue(int vc, wfqpar *par);
  wfqpar *getQueue(int vc);
  //tolua_end
  void early(event *);
  void late(event *);
  int export(exp_typ *);
  //tolua_begin
  void restim(void);

  tim_typ spacTime; // the central WFQ variable "Spacing Time"
  uoqueue sortq;  // the output sort queue
  //tolua_end
  wfqpar **partab; // per VC a data structure
  int *qLenVCI; // per VC: input queue length
  //tolua_begin
  enum {spacTimeMax = 100000000};
  // when to reset spacTime
};
//tolua_end
