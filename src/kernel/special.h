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

#ifndef	_SPECIAL_H_
#define	_SPECIAL_H_

#include "defs.h"
//
// - Definition of the classes for communication over the root::special() method.
// - Definition of return values for the rec() method
// - Definition of argument structure for the export() method
//


//
// Communication between objects with the special() method
// - add a new value to enum specmsg_typ for every new purpose
//
//tolua_begin
typedef	enum	{
  WriteVciTabType, // Write the vci table: for Demux
  GetDistTabType,  // Get the pointer to the r.n. distribution table
  ABRConReqType,
  ABRConFinType,
  TCPConReqType
} specmsg_typ;

//
// basic class for special()
//
class	specmsg	{
public:
  inline specmsg(specmsg_typ t)	{ type = t; }
  // message type
  specmsg_typ type;		
};

//
// Class for writing the VCI table of a demux
//
class	WriteVciTabMsg: public specmsg {
public:
  inline WriteVciTabMsg(void): specmsg(WriteVciTabType) {}
  inline ~WriteVciTabMsg(){};
  int old_vci;	// old VCI
  int new_vci;	// new VCI
  int outp;	// direct to this output
};

//
//	Class for importing the r.n. table of a Distribution object
//
class GetDistTabMsg: public specmsg {
public:
  inline GetDistTabMsg(void): specmsg(GetDistTabType) {}
  inline ~GetDistTabMsg(){};
  inline void setTable(void *table){table = table;}
  inline void* getTable(void){return table;}
//tolua_end
//private:
  tim_typ *table;
}; //tolua_export

//
//	Class to establish an ABR connection
//
class ABRConReqMsg: public specmsg {
public:
  inline ABRConReqMsg(void): specmsg(ABRConReqType) {}
  inline ~ABRConReqMsg(){};
  int Requ_Flag;	// connection refused? (!= 0)
  char *refuser;	// name of refuser
  char **routp;	        // pointer to the routing names
  int numb_RoutMemb;	// number of route members (without source)
  int akt_pointer;	// next routing member
  int next_att;	        // after SimTime + next_att try to connect again
  
  int VCI;		// for conn.disting.in muxer
  int TBE;		// negotiation-param.
  double ICR;		// -"-,initial cell rate
  double  PCR;		// nessecary?
  double MCR;		// if a obj. can not support MCR->conn.refuse
  
};

//
//	Class to release an ABR connection
//
class ABRConFinMsg: public specmsg	{
public:
  inline ABRConFinMsg(void):specmsg(ABRConFinType) {}
  inline ~ABRConFinMsg(){};
  int Finish_Flag;	// connection release refused? (!= 0)
  char *refuser;	// name of refuser
  char **routp;	        // routing members
  int numb_RoutMemb;	// number of route members (without source)
  int akt_pointer;	// next routing member
  int next_attempt;	// try again after this period of time
  
  int VCI;		// for conn.disting.in muxer
  
};

//tolua_begin
//
// Class for TCP connection request
//
class TCPConReqMsg: public specmsg	{
public:
  inline TCPConReqMsg(void): specmsg(TCPConReqType) {}
  inline ~TCPConReqMsg(){};
  int wnd;
  int MTU;
  int TS;
  double tick;
  double bitrate;
  
  root	*ptrTcpSend;
};
//tolua_end

//
// Return values of the rec()-method
//

#ifdef	NEVER
typedef	enum	{
  // Standard return value:
  ContSend,	// data item accepted, no further action
  
  // Return value for the Start-Stop protocol:
  StopSend	// data item accepted, 
                // but this was the last item before input buffer overflow
} rec_typ;
#endif

//tolua_begin
//
// Extended definition of the rec() return value
//
typedef int rec_typ;

typedef enum {	

  // the data item has been accepted completely, we can continue to send
  ContSend = -2,

  // data item accepted completely, but we must stop to send
  StopSend = -1
	
  // all other values (only >= 0):
  // We must stop to send, and *logically* the data item has been
  // rejected. This means, that the *physical* data object remains
  // at the receiver, but the receiver has only accepted the amount of
  // associated data given by the return value. This may include that the 
  // receiver accepted only a part of the data, or even nothing (retval == 0).
  // In case all data have been accepted, but we shall stop to send,
  // the receiver *must* return StopSend.
} startstop_typ;

//
// Argument structure for the export() method
//
struct	exp_typ	{
  //tolua_end
  typedef enum	{
    IntScalar,
    IntArray1,
    IntArray2,
    DoubleScalar,
    DoubleArray1,
    DoubleArray2
  } ExpAddrType;

  //tolua_begin 
  char	*varname;		// name of the desired variable
  //tolua_end
  int	ninds;			// number of indizes specified
  
  enum {EXP_DIM_MAX = 3};
  int indices[EXP_DIM_MAX];	  // the specified indizes
  int dimensions[EXP_DIM_MAX];    // dimensions of the array the returned pointer points to
  int displacements[EXP_DIM_MAX]; // for each returned dimension: diplacement
                                  // between a value and its position in the array
  
  // the address type
  ExpAddrType addrtype;	          
  // address itself
  union	{
    int	*pint;
    int	**ppint;
    double *pdbl;
    double **ppdbl;
  };
  
  // An auxiliary method to check and shift indices.
  // - defined and commented in data.c
  char	*calcIdx(int *, int);

}; //tolua_export

#endif	// _SPECIAL_H_
