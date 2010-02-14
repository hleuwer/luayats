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
#ifndef	TCPIPSEND_H_
#define	TCPIPSEND_H_

#include "inxout.h"
#include "queue.h"

//tolua_begin
class tcpipsend: public inxout
{
  typedef	inxout	baseclass;
public:
  tcpipsend(void): evtProcq(this, keyProcq),
		   rt_timer(this, keyRTO), 
		   slowtimo(this, keySlowtimo){}
  ~tcpipsend(){};
  int act(void);

  void	early(event *);
  // REC() normally expands to rec() (macro for debugging)
  rec_typ REC(data *, int);
  
  int	export(exp_typ *);
  void	restim(void);
  void	connectact(root *);
  void  resetStat(void);
  enum	TCPsendMode {TCPPlain, TCPRetrans, TCPFastRetrans};
  
  void	calc_send(TCPsendMode);		// calculate data to send now
  void	queue_pkt(int, TCPsendMode);	// generates TCP/IP segments of this data
  void	send_pkt(void);			// sends a packet after processing time expired
  void	process_ack(tcpAck *);		// handles receipt of acknowledges
  
  int     min(int, int);		// calculates the minimum value of two integers
  int     max(int, int);		// calculates the maximum value of two integers
  
  tim_typ ticks_to_slots(double);	// calculates TCP ticks into slots of simulator
  tim_typ slots_to_ticks(tim_typ);
  double slots_to_secs(tim_typ);
  tim_typ secs_to_slots(double);
  
  void	tcp_xmit_timer(tim_typ);	// calculate new RTO timer from RTT
  tim_typ procDelay(void);	// returns a processing delay (includes some random)
  
  char	*rec_name;	// name of receiver part of protocol
  root	*ptrTcpRecv;	// pointer to the peer object
  
  double	tick;		// TCP tick (default 500msec)
  double	bitrate;	// bitrate (brutto) of ATM layer (needed to calculate alarms
				// in slot oriented simulator) in bit/s
  
  double	rto_ub;		// upper bound of rto (default 64sec)
  double	rto_lb;		// lower bound of rto (default 1.5sec)
  
  tim_typ	next_send_time;	// next possible send time after done the processing time:
				// we would like to send, but successor gave StopSend
  tim_typ	proc_time;	// processing time; is an estimator of machine speed
  
  int	max_seg_size;	// MSS
  int	MTU;		// MTU
  
  rec_typ	prec_state;	// state of the preceding object
  rec_typ	send_state;	// my send state (stopped by successor?)
  
  int	max_input;	// size of input buffer
  int	q_start;	// input buffer occupation where to wake up stopped data sender
  int	input_q_len;	// length of input queue (unacked bytes + bytes waiting to be send)
  
  int	nxt_last_rto;	// sequence number of segment retransmitted at to last RTO
  int	abort_cnt;	// counter of failed retransmissions of the same segment
  int	aborted;	// Flag shows the reset of the connection
  int	EndSend;	// Flag shows the end of receiving data from the source 
  //	because of overflow of sequence numbers
  
  int	nagleOff;	// FALSE (!): Nagle alg. on
  int	ph_efOn;	// FALSE (!): Phase effects ON
  int	do_timestamp;	// TRUE: time stamp option acc. to RFC1323 turned on
  int	doFastRetr;	// TRUE: fast retransmission and recovery turned on
  int	doLogRetr;	// TRUE: print log messages for retransmissions
  
  int	ip_header_len;	// length of IP-Header
  int	tcp_header_len;	// length of TCP-Header
  int	ts_option_len;	// length of Timestamp Options
  
  int	nxt;		// next sequence number of TCP
  int	una;		// oldest unacknowledged sequence number
  int	max_sent;	// max. sequence number we have sent: only to check ACKs
  
  int	wnd;		// receivers advertised window
  int	max_sendwnd;	// largest window ever advertised by the other end
  int	ssthresh;	// Slow Start Threshhold
  int	cwnd;		// Congestion Window
  double	cwnd_d;		// aux variable: contains the "exact" cwnd value, from
				// which the one to use is derived (see process_ack()).
				// ATTENTION: do always update cwnd_d when changing cwnd,
				//	exception: opening cwnd in process_ack()
  
  int	seqLastWndUpd;	// Packet sequence number at last window update
  int	ackLastWndUpd;	// Packet acknowledgement number at last window update
  int	rexmtthresh;	// number of duplicate ACKs before fast retransmission
  int	dupacks;	// counter of consecutive duplicate ACKs (Fast Retransmit)
  
  int	SA;		// scaled average of RTT
  int	SD;		// scaled variance of RTT
  
  event	evtProcq;		// event to activate output queue
  int	active_procq;		// flag if proc_queue timer is active
  
  event	rt_timer;		// event for retransmission timer
  int	active_rt_timer;	// State of retransmission timer (on,off)
  
  event	slowtimo;		// event for slow timer (standard 500msec)
  // (this timer is always active)
  
  int	rtt;		// round trip time in ticks, usage:
                        // if zero: no measurement is in progress
                        // otherwise: measurement running
  int	rtseq;		// sequence number of rtt measurement currently in progress
  
  tim_typ	rto_calc;	// calculated retransmission timeout (from measurement or timestamp)
  tim_typ	rto_val;	// value currently used for retransmission timeout
				// (is doubled with every timeout)
  
  int	tcp_now;	// connection time in ticks
  
  uqueue	procq;		// output processing queue
  
  enum	{SucData=0, SucCtrl=1};
  enum	{InpData = 0, InpStart = 1, InpAck = 2};
  
  
  // Statistic
  int	received_bytes;		// total number of received user bytes
  int	xmitted_bytes;		// total number of xmitted bytes including headers
  int	xmitted_user_bytes;	// total number of xmitted bytes excluding headers
  int	xmitted_segments;	// total number of xmitted segments
  int	rexmitted_segments;	// # of rexmitted segments
  int	rexmitted_bytes;	// # of rexmitted bytes
  int	rexmto;			// # of retransmission timeouts
  int	procq_max_len;		// maximum length of processing queue in # of segments
  int	inputq_max_len;		// maximum length of input queue in bytes
  int	received_acks;		// # of ACKs
  int	ackbyte;		// # of acked bytes
  int	acktoomuch;		// # of ACKs that acked data that did not even sent
  double	rexmission_percentage;	// percentage of rexmissions
  // Test Mue ABORT_CNT 15.04.1998
  int	StatAbortCnt[13];	// number of unsuccessful retransmissions with abort_cnt of i
  
  int	wnd_min;		// minimum value of receivers advertised window
  int	wnd_max;		// maximum value of receivers advertised window
  
  tim_typ	rtt_calc;		// calculated rtt (round-trip time)
  tim_typ	rtt_min;		// minimum rtt
  tim_typ	rtt_max;		// maximum rtt
  
  tim_typ	rtt_real;		// real rtt (directly measured in the simulator).
  tim_typ	rtt_real_min;
  tim_typ	rtt_real_max;
  
  tim_typ	rto_min;		// minimum rto
  tim_typ	rto_max;		// maximum rto
  
  int	connID;			// ID of layer 4 connection
  int	connIdInitialized;	// TRUE: connID initialized
  
  unsigned	segsReallyRetransmitted;
  unsigned	maxByteReallySent;
  
  
#define	PROCQTHRESH	(1)
#ifdef	PROCQTHRESH
  int	calcSendStopped;	// TRUE: we have stopped enqueuing into
  // processing queue
  int	procqThresh;		// when to begin to do this
#endif

  enum	{keyProcq = 1, keyRTO = 2, keyPersist = 3, keySlowtimo = 4};
  
};
//tolua_end


#endif	// TCPIPSEND_H_
