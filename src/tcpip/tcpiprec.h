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
#ifndef	TCPIPREC_H_
#define	TCPIPREC_H_

#include "inxout.h"
#include "queue.h"

//tolua_begin
class	tcpiprec:	public inxout
{
  typedef	inxout	baseclass;
public:
  tcpiprec(void): evtImAck(this, keyImAck), 
		  evtTick(this, keyTick),
		  evtDelAck(this, keyDelAck), 
		  evtOutput(this, keyOutput),
		  evtProcq(this, keyProcq),
		  evtKeepAlive(this, keyKeepAlive)
  {
    ptrTcpSend = NULL;		
    sockstate = ContSend;		
    reseq_head = NULL;
  }
  ~tcpiprec();
  int act(void);
  void resetStat(void);
  int	max_win;	// window high water mark
  int	wnd;		// current window size
  double	proctim_secs;	// time for processing a segment, seconds
  double	ackdel_secs;	// delay for send a delayed ACK, seconds		
  int	ph_efOn;		// Phase effects ON/OFF
  double	iackdel_secs;	// delay for send an immediate ACK, seconds
  double  keepalive_secs;	 // Keep-Alive in Seconds
  int	ack_seq;	// sequence number of ACK packetnxt
  int	nxt;		// next receive sequence number
  int	adv;		// highest advertised sequence number + 1
  int	connected;	// TRUE: we are connected to a TCP sender
  int     needImAck; 	// Flag if an immediate ACK will be generated
  // also shows that the timer evtImAck is registered
  int	needDelAck;	// Flag if a delayed ACK will be generated
  // also shows that the timer evtDelAck is registered
  int	tcp_header_len;	// length of TCP header
  int	ip_header_len;	// length of IP header
  int	ts_option_len;	// length of Timestamp Options
  
  int	ts_recent;	// copy of the most-recent valid timestamp
  int	last_ack;	// value of byte for echo of timestamp
  rec_typ	sockstate;	// state of this object (StopSend or ContSend) for output to user
  int	activeOutput;	// event evtOutput is registered -> output is in progress
  tim_typ	lastPackStamp;	// PackStamp field of last received segment for echo to sender
  int	tcp_now;	// 500msec timer
  
  // statistics
  int	arrived_segments;	// # of arrived segments
  int	arrived_bytes;		// # of arrived bytes
  int	arrived_valid_bytes;	// # of valid arrived bytes
  int	user_packets;		// # of successful transmitted segments to user
  int	user_bytes;		// # of successful transmitted bytes to user
  int	procq_max_len;		// maximum length of processing queue
  int	reseq_max_len;		// maximum length of resequencing queue
  int	reseq_len;		// length of resequencing queue
  int	out_of_order_segments;	// # of arrived out-of-order segments
  int	out_of_order_bytes;	// # of arrived out-of-order bytes
  int	ack_cnt;		// # of ACKs sent
  
  int	tp_bytes;		// # of successful transmitted bytes to user (used in throughput calc)
  
  int	SDU_delay;		// delay of current SDU
  double	SDU_delay_mean;		// mean delay of SDUs
  int	SDU_cnt;		// counter for delay statistic calculation
  int	PDU_delay;		// delay of current PDU
  double	PDU_delay_mean;		// mean delay of PDUs
  int	PDU_cnt;		// counter for delay statistic calculation
  
  double	throughput;		// throughput of connection in bits/sec
  
  tim_typ	conn_time;		// offset when connection is ESTABLISHED
  
  tim_typ sendtwice; //TEST
  
  int	connIdInitialized;
  int	connID;
  
  enum	{SucData = 0, SucAck = 1};	// maybe not needed any more
  enum	{InpData = 0, InpStart = 1};	// maybe not needed any more
  
  enum {
    keyImAck = 1,
    keyTick = 2,
    keyDelAck = 3,
    keyOutput = 4,
    keyProcq = 5,
    keyKeepAlive = 6
  };
  
  rec_typ	REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)
  void    early(event *);
#if 0 
  //tolua_end
  int	command(char *, tok_typ *);
  //tolua_begin
#endif
  int	export(exp_typ *);
  char  *special(specmsg *, char *);
  void	restim(void);
  
  void	process_reseq(void);	// handles resequencing queue (reseq)
  void    process_output(void);	// sends a frame to the user
  void    send_ack(void);		// sends an acknowledge
  
  tim_typ	ticks_to_slots(double);
  double	slots_to_secs(tim_typ);
  tim_typ	secs_to_slots(double);
  tim_typ	procDelay(tim_typ);
  
  double	tick;		// time of tick in msec
  double	bitrate;	// bitrate of ATM layer (z.B. 155.52Mbit/s)
  
  
  root	*ptrTcpSend;	// pointer to the sender.
  
  
  
  int	max_seg_size;	// MSS
  int	do_timestamp;	// flag if timestamp option (RFC1323) is on or off
  int	MTU;		// maximum transmission unit
  
  tim_typ	ack_delay;	// delay for send a delayed ACK, slots
	
  
  tim_typ	proc_time;	// time for processing a segment, slots
  
  tim_typ	iack_delay;	// delay for immediate ACKs; !!! needed to avoid synchronisation effects
				// because of copy strategy on socket interface with Start-Stop-Protocol
  
  tcpipFrame	*reseq_head;	// queue of out-of-order packets waiting to be processed
  
  uqueue	procq;		// queue of incoming packets waiting to be processed
  uqueue	outputq;	// queue of packets waiting to be read by user process
  
  event	evtImAck;	// event for immediate ACK
  event	evtTick;	// event for slow timer (default 500msec)
  event	evtDelAck;	// event for fast timer (default 200msec for delayed ACK)
  event	evtOutput;	// event for processing the output queue
  event	evtProcq;	// event to activate processing queue
  event	evtKeepAlive;	// event for Keep Alive Timer
};
//tolua_end

#endif	// TCPIPREC_H_
