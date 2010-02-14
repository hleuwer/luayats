/*************************************************************************
*
*		YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997	Chair for Telecommunications
*				Dresden University of Technology
*				D-01062 Dresden
*				Germany
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
*************************************************************************/

#ifndef	_DATA_H_
#define	_DATA_H_

/*
*	Definition of data classes.
*
*	Unfortunately, derivation relations can not (yet) be examined automatically.
*	If a new data class is added, then therefore a couple of things has to be done:
*
*	1.	Add a new value to the enum dat_typ.
*	2.	The following macros have to be part of each class definition:
*		- BASECLASS(base_class_name): declaration of the base class
*		- CLASS_KEY(dat_typ_value): specification of the associated dat_typ value
*		- NEW_DELETE(number_of_objects_allocated_together): inline new and delete
*		  operators. They decrease the simulation time by approx. 30% and initialize
*		  the 'type' object members.
*		- CLONE(class_name): definition of the method clone() (make a copy of an object)
*	3.	Define the static class member (data *new_class::pool) in data.c,
*		it has been declared by NEW_DELETE().
*	4.	Register the new class in data_classes() (very below). The macro
*		DATA_CLASS(internal_class_name, external_name) uses the information of previous
*		BASECLASS() and CLASS_KEY() statements to examine the derivation relationships.
*	5.	Do not forget to define the method new_class::pdu_len() which returns the
*		logical length of a data item.
*/

/*********************************************************************************************/
/*
*	Definition of the data class key values:
*		a value for each class
*/

//tolua_begin
typedef	enum	{
  UnknownType = -1,
  DataType = 0,
  CellType = 1,
  CellPaylType = 2,
  CellSeqType = 3,
  FrameType = 4,
  TCPIPFrameType = 5,
  TCPACKType = 6,
  RMCellType = 7,
  AAL5CellType = 8,
  FrameSeqType = 9,
  IsaFrameType = 10,
  DQDBSlotType = 11, 
  DMPDUSegType = 12,
  // include new values before _end_type, and adjust _end_type.
  _end_type = 13
} dat_typ;

// =============================================================================
// Basic data class
// =============================================================================
class	data	{
public:
//tolua_end
  
  CLASS_KEY(DataType);
  NEW_DELETE(10000);	// the pool member has to be defined in data.c
  CLONE(data);		// clone() method
  
  // to ensure that every data object has a valid time member:
  // and: initially, there is no data item embedded
//tolua_begin
  inline	data(void){
    time = SimTime;
    //	embedded = NULL; we let do this the first time by the memory allocation
    //		routine, and we reset embedded in ~data() if necessary
    embedded = NULL;
    // would be too dangerous: Suppose somebody defines
    // a data object or sth. derived normally in a block. Then not operator
    // new is called, but the object is placed on the stack. Thus, embedded won't
    // be initialised (only guaranteed for objects taken from our memory pool),
    // and we run in severe trouble in the destructor.
#ifdef	DATA_OBJECT_TRACE
    traceOrigPtr = NULL;	// for data object tracing
#endif	// DATA_OBJECT_TRACE
    // clp = 1;
  }

  // DO NOT delete or change to non-virtual! two purposes:
  // 1.	Due to the late binding, the right delete operator is found -
  //	regardless of the type of pointer used in the delete statement.
  //	See Stroustup: The C++ Programming Language, 2nd ed., p.216
  // 2.	Delete embedded data items (if any).
  // It is NOT NECESSARY to repeat this destructor for derived classes.
  inline virtual ~data(void){
    if (embedded) {
      // We shift this to the constructor, see above.
      // embedded = NULL; // it goes cleaned back into the memory pool
      delete embedded;
    }
  }
  // how long am I?
  virtual size_t pdu_len() {
    return 1;	
    // logical length of data item: 1 byte
  }

  dat_typ	type;		// data type, is initialized by alloc_pool()
  tim_typ	time;		// data item creation time
  data	*next;		// used everywhere: for queueing ...
  data	*embedded;	// if != NULL: data item embedded
//tolua_end

  int 	clp;

#ifdef	DATA_OBJECT_TRACE
  char	*traceOrigPtr;	// points to the SetTrace network object
  int	traceSeqNumber;
#endif	// DATA_OBJECT_TRACE
};  //tolua_export

//
// Redefine CLONE(): now defines the clone() method (as before) and additionally
// the empty destruktor which normally should be redundant (work-around for a gcc bug).
// For the 'data' class above, only the clone() method is defined by CLONE().
// Matthias Baumann, June 3, 1998
// 
#ifdef	GCC_DESTRUCTOR_BUG
#undef	CLONE
#define	CLONE(aClass)\
        virtual data *clone() {\
          data    *pd;\
          pd = new aClass( *this);\
          if (embedded)\
            pd->embedded = embedded->clone();\
            return pd;\
        }\
	virtual	~aClass(){}
#endif	// GCC_DESTRUCTOR_BUG


// =============================================================================
// Cell class
// =============================================================================
//tolua_begin
class	cell:	public	data	{
public:
  //tolua_end
  BASECLASS(data);
  CLASS_KEY(CellType);
  NEW_DELETE(10000);	// the pool member has to be defined in data.c
  CLONE(cell);		// clone() method
  
  //tolua_begin
  // to ensure that every cell object has a valid vci member:
  inline cell(int i){ 
    vci = i; clp = 0;
  }
  // how long am I?
  size_t pdu_len() {
    // logical length of cell: 53 bytes
    return 53;	
  }	
  int vci;		// VCI number
};
//tolua_end

// =============================================================================
//	Cells with payload
// =============================================================================
//tolua_begin
class	cellPayl: public	cell {
public:
  //tolua_end
  BASECLASS(cell);
  CLASS_KEY(CellPaylType);
  NEW_DELETE(1000);	// the pool member has to be defined in data.c
  CLONE(cellPayl);
  
  //tolua_begin
  inline	cellPayl(int i): cell(i)	{}
  // logical length of cell: 53 bytes
  size_t	pdu_len() {
    return 53;	
  }
  char payload[48];
};
//tolua_end

// =============================================================================
//	Cells with sequence numbers
// =============================================================================
//tolua_begin
class	cellSeq:	public cell {
public:
  //tolua_end
  BASECLASS(cell);
  CLASS_KEY(CellSeqType);
  NEW_DELETE(10000);	// the pool member has to be defined in data.c
  CLONE(cellSeq);
  //tolua_begin
  
  inline cellSeq(int i, int b, int blen, int s): cell(i) {
    burst_no = b; 
    burst_len = blen; 
    seq_no = s;
  }
  // logical length of cell: 53 bytes
  size_t pdu_len() {
    return 53;	
  }

  int burst_no;	 // # of the burst
  int burst_len; // length of the current burst
  int seq_no;	 // sequence number in the burst
};
//tolua_end

// =============================================================================
//	Frame class
// =============================================================================
//tolua_begin
struct macaddr {
   int mc;   // individual (0) or group (multicast) (1)
   int ul;   // unique global (0) or local (1)
   unsigned int oui;  // organizational uniqueue  id (1st 24 bits)
   unsigned int nic;  // nic (2nd 24 bits)
};

typedef struct macaddr macaddr_t;

char *mac2string(struct macaddr smac, char *cmac);
struct macaddr mac2struct(char *cmac);
char *maci2c(unsigned int mac, struct macaddr *smac);

class	frame:	public	data	{
public:
  //tolua_end
  
  BASECLASS(data);
  CLASS_KEY(FrameType);
  NEW_DELETE(10000);	// the pool member has to be defined in data.c
  CLONE(frame);
  
  //tolua_begin
  inline	frame(int l, int cid = 0)
  {
    frameLen = l;
    connID = cid;
    clp = 1; // standard frames are best effort
    tpid = 0;
    vlanId = 0;
    vlanPriority = 0;
    dropPrecedence = 0;
    prioCodePoint = 0;
    internalDropPrecedence = 0;
  }
  // how long am I?
  size_t pdu_len() {
    return frameLen;
  }
  int frameLen;     	// length of frame (bytes)
  int connID;	     	// a higher-layer connection ID
  // Ethernet Header
  // only for simple tasks: 
  // 0xFFFFFFFFL = broadcast
  // MSB=1: multicast
  // MSB=0: unicast
  unsigned int smac;             
  unsigned int dmac;
  int tpid;
  int vlanId;	     	// VLAN ID, added Mue 2003-08-31
  int vlanPriority; 	// VLAN priority, added Mue 2003-08-31
  int tpid2;
  int vlanId2;
  int vlanPriority2;
  int dropPrecedence;	// Drop precedence, added Mue 2003-08-31;
  int internalDropPrecedence;
  int prioCodePoint;
  root *sender;
};

//tolua_end

// =============================================================================
//	TCP/IP-Frames
// =============================================================================
//tolua_begin
class	tcpipFrame:	public	frame {
public:
  //tolua_end
  
  BASECLASS(frame);
  CLASS_KEY(TCPIPFrameType);
  NEW_DELETE(1000);	// the pool member has to be defined in data.c
  CLONE(tcpipFrame);
  
  //tolua_begin
  inline	tcpipFrame(int seq, int dlen, root *po): frame(dlen) {
    TCPseq = seq;
    sendingObj = po;
  }
  
  int TCPseq;		// TCP sequence number
  tim_typ TCPtimestamp;	// for TCP timestamp option
  tim_typ TCPPackStamp;	// for SDU delay measurement
  tim_typ TCPSendStamp;	// for PDU delay measurement
  
  root	*sendingObj;
};
//tolua_end

// =============================================================================
//	TCP-ACKs
// =============================================================================
//tolua_begin
class	tcpAck:	public frame {
public:
  //tolua_end
  
  BASECLASS(frame);
  CLASS_KEY(TCPACKType);
  NEW_DELETE(100);	// the pool member has to be defined in data.c
  CLONE(tcpAck);

  //tolua_begin	
  inline tcpAck(int s, int a, int w, root *po): frame(48) {
    TCPAseq = s;
    TCPAack = a; 
    TCPAwnd = w;
    sendingObj = po;
    clp = 0;
  }
  int TCPAseq;
  int TCPAack;
  int TCPAwnd;
  tim_typ TCPAecr;
  tim_typ TCPAPackStamp;
  root	*sendingObj;
};
//tolua_end

// =============================================================================
//	Frames with sequence number
// =============================================================================
//tolua_begin
class	frameSeq:	public	frame	{
public:
  //tolua_end
  
  BASECLASS(frame);
  CLASS_KEY(FrameSeqType);
  NEW_DELETE(10000);	// the pool member has to be defined in data.c
  CLONE(frameSeq);
  
  //tolua_begin	
  inline	frameSeq(int l, int seq) : frame(l){
    seq_no = seq; 
  }
  int	seq_no;
};
//tolua_end

// =============================================================================
//	Isabel Frames with sequence number, video frame number, and end of video frame mark
// =============================================================================
//tolua_begin
class isaFrame: public frame {
public:
  //tolua_end
  BASECLASS(frame);
  CLASS_KEY(IsaFrameType);
  NEW_DELETE(10000);	// the pool member has to be defined in data.c
  CLONE(isaFrame);
  
  //tolua_begin	
  inline isaFrame(int l):frame(l){}
  
  int frameSeqNo;
  int endOfVideoFrame;
  int firstFrameNo;
  int vidFrameNo;

};
//tolua_end

// =============================================================================
//	Ressource management cells (for ABR)
// =============================================================================
//tolua_begin
class rmCell: public cell {
public:
//tolua_end

  BASECLASS(cell);
  CLASS_KEY(RMCellType);
  NEW_DELETE(10000);
  CLONE(rmCell);
  
  //tolua_begin
  inline rmCell(int i): cell(i)	{}
  size_t pdu_len()
  {
    return 53;	
  }

  void *ident;			// connection ID - the this pointer of the source
					// actually root *, but root is still unknown ...
  int CI;
  int NI;
  int DIR;
  int BN;
  int CLP;	// Flags
  //tolua_end	
  double CCR, ER;
};  //tolua_export

// =============================================================================
//	AAL5 cells with payload
// =============================================================================
//tolua_begin
class aal5Cell:	public cell {
public:
  //tolua_end
  
  BASECLASS(cell);
  CLASS_KEY(AAL5CellType);
  NEW_DELETE(1000);	// the pool member has to be defined in data.c
  CLONE(aal5Cell);
  
  //tolua_begin
  inline aal5Cell(int i): cell(i) {}
  size_t	pdu_len() {
    return 53;	
  }
  int sdu_seq;	// sequence number of AAL SDU
  int cell_seq;	// sequence number of cell
  int first_cell;	// sequence number of first cell in AAL SDU
  int pt;		// 1: end of message
};
//tolua_end

// =============================================================================
//	DQDB: DQDB-TimeSlot 
// =============================================================================
class dqdbSlot:	public cell {
public:
  BASECLASS(cellPayl);
  CLASS_KEY(DQDBSlotType);
  NEW_DELETE(1000);	
  
  inline dqdbSlot(int i): cell(i) {}
  // logical length of DQDB-Slot: 53 or 69 bytes
  size_t pdu_len() {
    return 69;	
  }
  CLONE(dqdbSlot);
	
  int connID;		// segment adress write in from segment source 
                        // used to selected reciving DPDU-segments
  int slot_type;	// QA-Slot or PA-slot
  int busy;		// busy PA-Slot or Free Slot
  int req;		// lowest Request-bit(0) from Downstrem
	
	
};

// =============================================================================
//	DQDB: DMPDU use as segment payload in DQDB-TimeSlot
// =============================================================================
class dmpduSeg:	public	data {
public:
  BASECLASS(data);
  CLASS_KEY(DMPDUSegType);
  NEW_DELETE(10000);	// the pool member has to be defined in data.c
  
  // to ensure that every segment object has a valid segmment adress member:
  inline	dmpduSeg(int i)	{ connID = i;	}
  
  // logical length of DMPDUsegment: 48 or 64 bytes
  size_t pdu_len() {
    return 64;
  }
  CLONE(dmpduSeg);

  int sdu_seq;	// sequence number of MAC SDU
  int seg_seq;	// sequence number of segment
  int first_seg;	// sequence number of first cell in MAC SDU
  int pt;		// 1: end of message
  int connID;		// internal  adress of DMPDU_segment
};

// =============================================================================
//	Registration of data classes
// =============================================================================
//tolua_begin
inline	void	data_classes(void)
{
  // the class data is predefined with the name "Data"
  // !! follow the declaration order in enum dat_typ !!
  DATA_CLASS(cell, "Cell");
  DATA_CLASS(cellPayl, "CellPayload");
  DATA_CLASS(cellSeq, "CellSequence");
  DATA_CLASS(frame,  "Frame");
  DATA_CLASS(tcpipFrame, "TCPIPFrame");
  DATA_CLASS(tcpAck, "TCPAcknowledge");
  DATA_CLASS(rmCell, "RMCell");
  DATA_CLASS(aal5Cell, "AAL5Cell");
  DATA_CLASS(frameSeq, "FrameSeq");
  DATA_CLASS(isaFrame, "IsabelFrame");
  DATA_CLASS(dqdbSlot, "DQDBSlot");
  DATA_CLASS(dmpduSeg, "DMPDUSeg");
}
//tolua_end
#endif	// _DATA_H_
