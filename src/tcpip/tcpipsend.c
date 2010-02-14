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
**************************************************************************
*
*	Module authors:	Gunnar Loewe, TUD (diploma thesis)
*			Matthias Baumann, TUD
*	Creation:	Sept 1996
*
*	History:
*	July 10, 1997	name changes which result from new member
*			names of frame, tcpipFrame, and tcpAck. All
*			these frame types now havn't anymore a
*			core structure.
*				Matthias Baumann
*	July 18, 1997	connID introduced. The connection ID read
*			from incoming data frames is copied into
*			generated TCP/IP frames. Adds in init(),
*			rec(), and send_pkt(). Additional obj. members:
*			int connID; int connIdInitialized;
*				Matthias Baumann
*
*	Sept 30, 1997	retransmission log messages are preceeded by
*			"# ". The message for connection reset due
*			to 12 failures is only printed, if LOGRETR
*			turned on.	Matthias Baumann
*
*	Nov 25, 1997	successful fast retransmission logged, if
*			LOGRETR turned on.
*			Count of really retransm. segments. -> RexmSegs
*				Matthias Baumann
*
*	Nov 26, 1997	cwnd_d exported -> CWND_D
*			   Matthias Baumann
*
*	Dec 12, 1997	During connection setup, the this pointer is passed
*			to the peer receiver. Then, in each packet sent, also
*			the this pointer is written. This allows the receiver to
*			check incoming packets. The same accounts for received
*			ACKs (procedure the other way around).
*			   Matthias Baumann
*
*************************************************************************/

/*
*	TCP/IP module: sender
*/

/*************************************************************************
**									**
** Syntax:								**
**	TCPIPsend tcps:							**
**		BUF=<upper bound of send buffer in bytes>,		**
**		[BSTART=<lower bound for Start-Stop-protocol>,]		**
**		[MTU=<maximum transmission unit in bytes,]		**
**		[TS=<timestamp option RFC1323: on=1 / off=0>,]		**
**		[NAGLE=<Nagle Algor. on=1 / off=0>,]			**
**		[PH_EF=<random comp of PROCTIM on=1 / off=0>,]		**
**		[FRETR=<Fast retransm. / recov. on=1 / off=0>,]		**
**		[BITRATE=<bitrate on ATM layer in Mbit/sec>,]		**
**		[PROCTIM=<processing time in msec>,]			**
**		[TICK=<time of TCP tick in msec>,]			**
**		[RTOMIN=<minimum duration retransmission timer, msec>,]	**
**		[OQWM=<output queue water mark for new data>,]		**
**		[LOGRETR=<print retransm to std output on/off>,]	**
**		REC=<TCP receiver>,					**
**		OUTDATA=<successor for Data>,				**
**		OUTCTRL=<input Start of preceeding object>;		**
**									**
**	Default values:							**
**		MTU:	9180 bytes					**
**		BSTART:	BUF - 4096 bytes				**
**		TS:	on						**
**		NAGLE:	on						**
**		FRETR:	on						**
**		BITRATE:149.76 Mbit/s (STM-1)				**
**		PROCTIM:0.3 msec					**
**		TICK:	500 msec					**
**		RTOMIN:	1500 msec					**
**		OQWM:	2 packets					**
**			// this means that taking new data from the	**
**			// input buffer is stopped if the output queue	**
**			// length reaches 2				**
**		LOGRETR:off						**
**									**
**	During connection setup, the following values are passed to	**
**	the corresponding TCP receiver:					**
**		MTU							**
**		TS							**
**		BITRATE							**
**		TICK							**
**	The receiver is asked for the receiver buffer size (WND)	**
**									**
*************************************************************************/

/**

Implemented Algorithms
======================
- Slow start and congestion avoidance
- Silly window avoidance (RFC1122, section 4.2.3.4)
- Nagle's algorithm (can be turned off, default: on) (RFC1122, section 4.2.3.4)
- Karn's algorithm (no RTT measurement during retransmission)
- Fast retransmission and recovery (can be turned off, default: on)
- RTT measurement with TS option according to RFC 1323 (can be turned off,
   default: on)

Per packet sent, a processing delay of PROCTIM is included. The exact value is
a random value between PROCTIM and 1.1 times PROCTIM. This avoids
synchronisation effects between different TCP connections.

NOT Implemented
===============
Timer for zero window probe: if the ACK opening the window again is lost,
then the connection falls asleep forever.

Central Routines
================
calc_send():
Is always called when we possibly can transmit new data. Checks whether new data
can be sent (silly window avoidance and Nagle's algorithm) and segments data
according to MSS. Packets are enqueued in the output queue (which models the
processing delay) via queue_pkt().

queue_pkt():
Creates a packet and enqueues it in the output queue. Provided that no packets
are already waiting in the queue, and that we can send (not stopped by
succeeding network object), the process send_pkt() is woken up.

send_pkt():
Is called to really emit a packet. Is always invoked via the timer modelling the
processing delay. This timer is activated by queue_pkt(), and upon receipt of a
start signal (in case we wanted to send and have been stopped). Send_pkt()
itself reactivates the timer if the packet just sent was not the last in the
queue and we are allowed to continue.

process_ack():
Processes incoming acknowledges. Includes code for:
- fast retransmission / recovery
- receiver window update
- algorithm for opening congestion window (slow start and congestion avoidance)
- calculation of new RTO estimate (only in case of acknowleding new data)
The retransmission timer is stopped, and restarted provided there is something
on the way. If something has changed, calc_send() is called.


Problems
========
There is a general problem with the combination of processing queue and start-
stop protocol. If the successor stops the module, then currently only the
process of send_ptk() is stopped. Calc_send(), however, can continue to send.
The result is that the processing queue acts like a large network buffer:
the queue is filled until the window limit is reached.
Proposal:
Set a threshold for the length of the processing queue. If it is reached,
calc_send() returns and sets the flag (only if not a retransmission).
If the flag is set and send_pkt() reduces the queue length below the threshold,
 calc_send() is called again.	Included (0.5.11.96, MBau)
   - marked with #ifdef PROCQTHRESH,
   - the threshold is procqThresh = 2 (init())

Smaller detailed problems are marked in the code with "###". Additionally,
they are labeled with Px_ (x is the number).

- P1_:   Why is fast retransm not done when rt_timer not active ?
         Since RT timer off implicitely means that there is nothing on the way
	 (also could test una == nxt)
- P2_:	 should we open cwnd and restart the retransmision timer when receiving
      	 an ACK which does not acknowledge new data but only changes the wnd?
- P3_:	 appearently, we take one tick too much when calculating a new RTO.
- P4_:	 if we got new data acked, is it then possible that the ack is larger
      	 than nxt ? Yes. If we had e.g a RTO, reset nxt, and now receive the
	 ACK we were waiting for.
- P5_:	 which role does the seq number of an ACK play ?
- P6_:	 why is cwnd only opened in case wnd < cwnd ?
- P7_:	 how can it be that the seq of the data in the input buffer is lower
      	 than nxt ? It should NOT be possible.
- P9_:	 we should set max_sent when really sending a packet in send_pkt().
- P10_:	 should we start the retransmission timer when enqueueing a packet
      	 in the output queue, or rather when really sending it ?
- P11_:  reduce ssthresh with fast retransmission?
	 YESSSS ...: otherwise we would re-reach the congestion region
	 immediately, and probably fast	retransmission then will fail
	 (too much will be lost)

***/


#include "tcpipsend.h"

#if 0
void	tcpipsend::init(void)
{
  skip(CLASS);
  name = read_id(NULL);
  skip(':');
  
  max_input = read_int("BUF");		//length of input queue in bytes
  if (max_input <= 0)
    syntax0("invalid BUF");
  skip(',');
  
  if(test_word("BSTART"))
    {
      q_start = read_int("BSTART");
      if (q_start < 0)
	syntax0("invalid BSTART");
      skip(',');
   }
  else
    {
      if(max_input < 4096) // this models the copy rule of SunOS 4.1.x
	q_start = 0;
      else
	q_start = max_input - 4096;
    }
  
  if(test_word("MTU"))
    {
      MTU = read_int("MTU");  //max. size of TCP-frame without header!
      if(MTU <= 0)
	syntax1s("%s: MTU must be greater than 0\n", name);
      if(MTU > 65535)
	syntax1s("%s: MTU must be lower than 65536\n", name);
      skip(',');	
   }
  else
    MTU = 9180;			// standard MTU for IP over ATM

  if(test_word("TS"))
    {
      do_timestamp = read_int("TS");	// timestamp option (RFC 1323) ON/OFF
      skip(',');
      if(do_timestamp != 0)
	do_timestamp = TRUE;
      else
	do_timestamp = FALSE;
    }
  else
    do_timestamp = TRUE;	    // timestamp option is on per default
  
  if(test_word("NAGLE"))	    // Nagle Alg. ON/OFF
    {	
      if (read_int("NAGLE") != 0)
	nagleOff = FALSE;
      else
	nagleOff = TRUE;
      skip(',');
    }
   else
     nagleOff = FALSE;		    // Nagle Alg. on per default
  
  if(test_word("PH_EF"))	    // Phase Effects ON/OFF
    {
      if (read_int("PH_EF") == 0)
	ph_efOn = FALSE;
      else
	ph_efOn = TRUE;
      skip(',');		    // Phase Effects OFF per default
    }
  else 	ph_efOn = FALSE;
  
  if (test_word("FRETR"))
    {	if (read_int("FRETR") != 0)
		   doFastRetr = TRUE;
    else	doFastRetr = FALSE;
    skip(',');
    }
  else
    doFastRetr = TRUE;      	 // fast retransm. and recovery on per default
  
  if(test_word("BITRATE"))
    {
      bitrate = read_double("BITRATE");	// bitrate of ATM layer in Mbit/sec
      bitrate *= 1E+6;
      if(bitrate <= 0.0)
	syntax1s("%s: BITRATE must be greater than 0\n", name);
      skip(',');
    }
  else
    bitrate = 149760000; // default bitrate is 149.76Mbit/sec
  
  if(test_word("PROCTIM"))
    {
      double prsec = read_double("PROCTIM") / 1000;   // PROCTIM in msec
      if (prsec < 0.0)
	syntax0("invalid PROCTIM");
      proc_time = secs_to_slots(prsec);
      skip(',');
    }
  else
    proc_time = secs_to_slots(0.0003);
  
  if(test_word("TICK"))
    {
      tick = read_double("TICK");   // time tick in msec
      tick /= 1000;
      if (tick <= 0.0)
	syntax0("invalid TICK");
      skip(',');
    }
  else
    tick = .5;  // default value of time tick is 500 msec
  
  if(test_word("RTOMIN"))
    {
      rto_lb = read_double("RTOMIN");	// RTO min in msec
      rto_lb /= 1000;
      if (rto_lb <= 0.0)
	syntax0("invalid RTOMIN");
      skip(',');
    }
  else
    rto_lb = 1.5;  // default value of RTO min is 1500 msec
  
#ifdef	PROCQTHRESH
  if(test_word("OQWM"))
    {
      procqThresh = read_int("OQWM");
      if (procqThresh <= 0)
	syntax0("invalid OQWM");
      skip(',');
    }
  else
    procqThresh = 2;
  calcSendStopped = FALSE;
#endif
  
  if (test_word("LOGRETR"))
    {
      if (read_int("LOGRETR") != 0)
	doLogRetr = TRUE;
      else
	doLogRetr = FALSE;
      skip(',');
    }
  else
    doLogRetr = FALSE;	// retransmission log off per default
  
  rec_name = read_suc("REC");	// TCP receiver
  skip(',');
  
  output("OUTDATA", SucData);
  skip(',');
  output("OUTCTRL", SucCtrl);
  
  input("Data", InpData);
  input("Start", InpStart);
  input("Ack", InpAck);
  
  nxt_last_rto = 0;	// variables to model connection reset
  abort_cnt = 0;
  aborted = FALSE;
  EndSend = FALSE;	// end of accept. data because of preventing an overflow
  
  /*
   *	TCP default values
   */
  ip_header_len = 20;	// length of IP-header
  tcp_header_len = 20;	// length of TCP-header
  ts_option_len = 12;	// length of timestamp options
  rto_ub = 64;		// upper bound of rto value (default 64 sec)
  // above: rto_lb = 1.5;	// lower bound of rto value (default 1.5 sec)
  double	rto_calc_init = 3.0;	// RFC1122, section 4.2.3.1
  rexmtthresh = 3;	// number of duplicate ACKs before fast retransmission
  
  nxt = 1;		// first sequence number used by TCP
  una = 1;		// UNAcknoledged Sequence Number
  
  if(MTU <= tcp_header_len + ip_header_len +
     (do_timestamp ? 1 : 0) * ts_option_len)
    syntax1s("%s: MTU must be greater than sum of all "
	     "header lengths\n", name);
  
  max_seg_size = MTU -
    (tcp_header_len + ip_header_len + (do_timestamp ? 1 : 0) * ts_option_len);
  
  rto_calc = secs_to_slots(rto_calc_init);
  rto_val = rto_calc;
  
  rtt = 0;		// no RTT measurement in progress
  rtt_calc = 0;
  rtt_real = 0;
  SA = SD = 0;
  
  cwnd = max_seg_size;
  cwnd_d = (double) cwnd;
  
  seqLastWndUpd = 0;
  ackLastWndUpd = 0;
  input_q_len = 0;
  max_sent = 0;
  dupacks = 0;
  
  prec_state = ContSend;	 // state of preceeding object
  send_state = ContSend;	 // send state of this object
  
  active_rt_timer = FALSE;
  active_procq = FALSE;
  
  tcp_now = 1;		      	 // start of time ...
  alarme( &slowtimo, my_rand() % ticks_to_slots(1));
  next_send_time = 0;
  
  received_bytes = 0;
  xmitted_bytes = 0;
  xmitted_user_bytes = 0;
  xmitted_segments = 0;
  rexmitted_segments = 0;
  rexmitted_bytes = 0;
  rexmto = 0;
  procq_max_len = 0;
  inputq_max_len = 0;
  received_acks = 0;
  rexmission_percentage = 0;
  
  int	HugeVal = (int)(((unsigned) -1) >> 1);	// largest pos. int?
  
  wnd_min = HugeVal;
  wnd_max = 0;
  rtt_min = HugeVal;
  rtt_max = 0;
  rtt_real_min = HugeVal;
  rtt_real_max = 0;
  rto_min = HugeVal;
  rto_max = 0;
  
  connIdInitialized = FALSE;	// added July 18, 1997. MBau
  
  segsReallyRetransmitted = 0;	// added Nov 25, 1997. MBau
  maxByteReallySent = 0;
  
  ptrTcpRecv = NULL;		// pointer to peer object
  
  // Test Mue ABORT_CNT 15.04.1998
  int i;
  for(i=1; i<13; i++)
    StatAbortCnt[i] = 0;
  
}  
// end of init()
#endif

int tcpipsend::act(void) 
{
  // variables to model connection reset
  nxt_last_rto = 0;
  abort_cnt = 0;
  aborted = FALSE;
  EndSend = FALSE;	// end of accept. data because of preventing an overflow
  
  //
  //	TCP default values
  //
  ip_header_len = 20;	// length of IP-header
  tcp_header_len = 20;	// length of TCP-header
  ts_option_len = 12;	// length of timestamp options
  rto_ub = 64;		// upper bound of rto value (default 64 sec)
  // above: rto_lb = 1.5;	// lower bound of rto value (default 1.5 sec)
  double	rto_calc_init = 3.0;	// RFC1122, section 4.2.3.1
  rexmtthresh = 3;	// number of duplicate ACKs before fast retransmission
  
  nxt = 1;		// first sequence number used by TCP
  una = 1;		// UNAcknoledged Sequence Number
  
  if(MTU <= tcp_header_len + ip_header_len +
     (do_timestamp ? 1 : 0) * ts_option_len)
    errm1s("%s: MTU must be greater than sum of all "
	     "header lengths\n", name);
  
  max_seg_size = MTU -
    (tcp_header_len + ip_header_len + (do_timestamp ? 1 : 0) * ts_option_len);
  
  rto_calc = secs_to_slots(rto_calc_init);
  rto_val = rto_calc;
  
  rtt = 0;		// no RTT measurement in progress
  rtt_calc = 0;
  rtt_real = 0;
  SA = SD = 0;
  
  cwnd = max_seg_size;
  cwnd_d = (double) cwnd;
  
  seqLastWndUpd = 0;
  ackLastWndUpd = 0;
  input_q_len = 0;
  max_sent = 0;
  dupacks = 0;
  
  prec_state = ContSend;	 // state of preceeding object
  send_state = ContSend;	 // send state of this object
  
  active_rt_timer = FALSE;
  active_procq = FALSE;
  
  tcp_now = 1;		      	 // start of time ...
  alarme( &slowtimo, my_rand() % ticks_to_slots(1));
  next_send_time = 0;
  
  received_bytes = 0;
  xmitted_bytes = 0;
  xmitted_user_bytes = 0;
  xmitted_segments = 0;
  rexmitted_segments = 0;
  rexmitted_bytes = 0;
  rexmto = 0;
  procq_max_len = 0;
  inputq_max_len = 0;
  received_acks = 0;
  rexmission_percentage = 0;
  
  int	HugeVal = (int)(((unsigned) -1) >> 1);	// largest pos. int?
  
  wnd_min = HugeVal;
  wnd_max = 0;
  rtt_min = HugeVal;
  rtt_max = 0;
  rtt_real_min = HugeVal;
  rtt_real_max = 0;
  rto_min = HugeVal;
  rto_max = 0;
  
  connIdInitialized = FALSE;	// added July 18, 1997. MBau
  
  segsReallyRetransmitted = 0;	// added Nov 25, 1997. MBau
  maxByteReallySent = 0;
  
  ptrTcpRecv = NULL;		// pointer to peer object
  
  // Test Mue ABORT_CNT 15.04.1998
  int i;
  for(i=1; i<13; i++)
    StatAbortCnt[i] = 0;
  return 0;
}


//********************************************************************/
// establish connection to TCP receiver, ask for window size
//********************************************************************/
void tcpipsend::connectact(root *peer)
{
   TCPConReqMsg	tcp_msg;
   char		*err;

   tcp_msg.wnd = 0;	// values to share with receiver in connect method
   tcp_msg.TS = do_timestamp;
   tcp_msg.MTU = MTU;
   tcp_msg.tick = tick;
   tcp_msg.bitrate = bitrate;
   tcp_msg.ptrTcpSend = this;

   // inform receive part of connection parameters: timestamp option, MTU
   if ((err = peer->special(&tcp_msg, name)) != NULL)
      errm3s("%s: could not establish TCP connection, reason \
      	 returned by `%s': %s", name, rec_name, err);

   // get receivers window size
   wnd = tcp_msg.wnd;

   ssthresh = wnd / 2;
   max_sendwnd = wnd;
   wnd_min = wnd;
   wnd_max = wnd;
}
#if 0
void tcpipsend::connect()
{
   root		*rec_obj;
   TCPConReqMsg	tcp_msg;
   char		*err;

   baseclass::connect();

   // find receive part of TCP
   if((rec_obj = find_obj(rec_name)) == NULL)
 	   errm2s("%s: could not find object `%s'", name, rec_name);	
   ptrTcpRecv = rec_obj;

   tcp_msg.wnd = 0;	// values to share with receiver in connect method
   tcp_msg.TS = do_timestamp;
   tcp_msg.MTU = MTU;
   tcp_msg.tick = tick;
   tcp_msg.bitrate = bitrate;
   tcp_msg.ptrTcpSend = this;

   // inform receive part of connection parameters: timestamp option, MTU
   if ((err = rec_obj->special(&tcp_msg, name)) != NULL)
      errm3s("%s: could not establish TCP connection, reason \
      	 returned by `%s': %s", name, rec_name, err);

   // get receivers window size
   wnd = tcp_msg.wnd;

   ssthresh = wnd / 2;
   max_sendwnd = wnd;
   wnd_min = wnd;
   wnd_max = wnd;
}
#endif
/********************************************************************/
/*
*	a timer has expired
*/
void tcpipsend::early(event *evt)
{
   switch (evt->key) {
   case keyProcq:		// run processing queue
      active_procq = FALSE;
      send_pkt();
      break;

   case keyRTO:		// Retransmission Timeout
      active_rt_timer = FALSE;
      nxt = una;  
      rtt = 0;		// Karn's Algorithm

      if (doLogRetr)
      	 printf("# %s at %d: retransmission timeout, rto_tim = %u\n",
	    name, SimTime, rto_val);

      if (nxt == nxt_last_rto)	// no progress since last time-out
      {
      	 if ( ++abort_cnt >= 13) // reset connection after 12 failed retransm.
	 {
	    if (doLogRetr)
	       fprintf(stderr, "# %s: end of transmission because of 12 failed "
	          "retransmissions of segment with seq=%d, SimTime=%d\n",
		  name, nxt, SimTime);
	    aborted = TRUE;		// means that the connection is closed
	    unalarme(&slowtimo);
	    if (active_procq)
	    {	unalarme( &evtProcq);
		    active_procq = FALSE;
	    }

	    // Test Mue ABORT_CNT 15.04.1998
	    StatAbortCnt[13-1]++;

	    return;
	 }

	 // Test Mue ABORT_CNT 15.04.1998
	 StatAbortCnt[abort_cnt-1]++;
      }
      else
      	 abort_cnt = 0;	// we made a transmission progress

      nxt_last_rto = nxt;

      // Reset ssthresh and congestion window size
      {
      	 int win;
	 win = min(wnd, cwnd) / 2 / max_seg_size;
	 win = max(win, 2);
	 ssthresh = win * max_seg_size;	// approx. half of old win
	 cwnd = max_seg_size;	// set cwnd back to 1 pkt
	 cwnd_d = (double) cwnd;
	 /*
	 if (doLogRetr)
	 printf("# %s at %d: retransmission timeout, ssthresh = %d\n",
	    name, SimTime, ssthresh);
	 */
      }

      calc_send(TCPRetrans);	// we do retransmission

      // Log retransmission timeout
      ++rexmto;

      // register another timeout using exponential backoff
      rto_val = min(rto_val * 2, secs_to_slots(rto_ub)); 
      // Double retrans timeout, recognize upper bound

      if (active_rt_timer)
      	 unalarme( &rt_timer);
      else
      	 active_rt_timer = TRUE;

      if(SimTime < SimTime + rto_val)
      	 alarme(&rt_timer, rto_val);
      else
      	 errm1s("%s: want to alarm an event later then maximum SimTime\n",
	    name);

      break;	// end of Retransmission Timeout


   case keySlowtimo:	// 500msec timer
      if(rtt > 0)	// if RTT is currently being measured:
      	 ++rtt;	// inc rtt
      ++tcp_now;	// inc clock
      alarme( &slowtimo, ticks_to_slots(1));
      
      break;

   default:errm1s("%s: internal error: tcpipsend::early(): unknown event type",
      name);
   }  // switch

}  // end of function void tcpipsend::early(event *)


/********************************************************************/
/*
*	something received
*/
rec_typ	tcpipsend::REC(data *pd,int key)
{
   switch (key) {
   case InpData:	 // INPUT DATA
   {
      frame *pf = (frame *) pd;

      typecheck_i(pd, FrameType, key);

      // added July 18, 1997:
      if (connIdInitialized == FALSE)
      {
      	 connID = pf->connID;
	 connIdInitialized = TRUE;
      }
      else if (connID != pf->connID)
      	 errm1s2d("%s: illegal change of incoming connection ID, \
	    old: %d, new: %d", name, connID, pf->connID);
      // end add July 18, 1997. MBau

      if (prec_state == StopSend)
      {
      	 fprintf(stderr, "%s: preceeding object did not recognize the Stop \
	    signal\n", name);
	 delete pf;
	 return StopSend;
      }

      if (aborted)
      {
      	 prec_state = StopSend;
	 delete pf;
	 return StopSend;
      }

      // prevent overflow of sequence numbers
      if (received_bytes > received_bytes + pf->frameLen + wnd_max)
      {
      	 fprintf(stderr, "%s: end of transmission because of sequence number"
	    "overflow, received_bytes=%d, SimTime=%d\n",
	    name, received_bytes, SimTime);
	 prec_state = StopSend;
	 EndSend = TRUE;   // means that no data from input DATA is excepted
	 delete pf;
	 return StopSend;
      }
      // Log the received data
      received_bytes += pf->frameLen;

      // queue buffer (frame) in input queue (add the length to queue length)
      if(input_q_len > input_q_len + pf->frameLen)
	 errm1s("%s: internal error: overflow of input_q_len (decrease START "
	    "value or send in smaller parts)", name);
      else
      	 input_q_len += pf->frameLen;

      if(max_input <= input_q_len)	// input buffer now full: stop sender
      	 prec_state = StopSend;

      // Log the input queue length
      inputq_max_len = max(inputq_max_len, input_q_len);

      calc_send(TCPPlain);

      delete pf;
      return prec_state;
   }	// end of input DATA

   case InpStart:		//input ...->Start
      // accepts any DataType
      // we are able to send: wake up send_pkt() if not busy
      if (send_state == StopSend)
      {
      	 send_state = ContSend;
	 if( !active_procq && !procq.isEmpty() && !aborted)
	 {
	    // wake up the output queue
	    if (next_send_time > SimTime)
      	       alarme( &evtProcq, next_send_time - SimTime);
	    else
	       alarme( &evtProcq, 1);
	    active_procq = TRUE;
	 }
      }
      delete pd;
      return ContSend;
      // end of input ->Start

   case InpAck:			//input ACK
      typecheck_i(pd, TCPACKType, key);

      // should not be possible at all, but who knows ...
      if (ptrTcpRecv == NULL)
      	 errm2s("%s: not yet connected to peer receiver, but ACK received from "
	    " `%s'", name, ((tcpAck *) pd)->sendingObj->name);

      // but this is possible, if the routing is wrong
      if (ptrTcpRecv != ((tcpAck *) pd)->sendingObj)
	 errm3s("%s: connected to peer receiver `%s', but ACK received from "
	    "`%s'", name, ptrTcpRecv->name, ((tcpAck *) pd)->sendingObj->name);

      if(aborted)	// connection has been closed by us
      {
      	 delete pd;
	 return StopSend;
      }
      process_ack((tcpAck *) pd);
      delete pd;
      return ContSend;

   default:
      errm1s("%s: internal error: tcpsend::rec(): invalid input key", name);
      return StopSend;	// compiler warning, not reached
   }

}  // end of function tcpipsend::rec(data *, int)


/*
*	process an acknowlege
*/
void tcpipsend::process_ack(tcpAck *pa)
{
   if(pa->TCPAack <= una)
   {
      // no new data acknowldeged
      if(pa->TCPAwnd == wnd)	// no window update
      {
      	 if ( !doFastRetr)
	    return;		// ignore this ACK if fast retr not turned on

	 // Fast Retransmit and Fast Recovery Algorithms implemented
	 if( !active_rt_timer || pa->TCPAack != una || wnd == 0)
	    // ### P1_: why !active_rt_timer ???
	    // rt_timer inactive means that there is noth. on the way (### P1_)
	    // wnd == 0 is for window probe
	    dupacks = 0;
	 else if( ++dupacks == rexmtthresh)
      	 // it's a completely duplicate ACK: increase dupacks
	 {	
	    /*
	    *	Fast Retransmission
	    */
	    int	old_next;
	    int	win;

	    if (doLogRetr)
      	       printf("# %s at %d: fast retransmission\n", name, SimTime);

	    old_next = nxt;	// store old nxt

	    // reduce slow Start Threshhold like after rt_timer expired
	    // ### P11_: reduce ssthresh with fast retransmission ???
	    // YESSSS ...: otherwise we would reach the congestion region
	    // with the same speed again, and probably fast retrans then fails
	    win = min(wnd, cwnd) / 2 / max_seg_size;
	    win = max(win, 2);
	    ssthresh = win * max_seg_size;	
	    if(active_rt_timer)	// turn off retransmission timer
	    {
	       active_rt_timer = FALSE;
	       unalarme(&rt_timer);
	    }

	    rtt = 0; // turns rtt measurement off (Karn's algorithmus)
	    nxt = pa->TCPAack;
	    cwnd = max_seg_size;	// set cwnd to one segment
	    cwnd_d = (double) cwnd;

	    calc_send(TCPFastRetrans);	// retransmit missing segment

	    /*
	    *	Fast Recovery Algorithm:
	    *	congestion window is set to ssthresh plus 
	    *	the # of segments that the other end has cached
	    *	in resequencing queue
	    */
	    cwnd = ssthresh + max_seg_size * dupacks;
	    cwnd_d = (double) cwnd;

	    if(old_next > nxt)
      	       nxt = old_next;	// because nxt was modified by calc_send()

	    calc_send(TCPPlain);	// may be, window now is larger

	 }	// # of duplicate ACKs reached rexmtthresh (fast retransmit)

	 else if (dupacks > rexmtthresh)  // # of dupl. ACKs exceeds rexmtthresh
	 {
	    cwnd += max_seg_size;  // because one segment has left the network
	    cwnd_d = (double) cwnd;
	    calc_send(TCPPlain);
	 }  // # of duplicate ACKs exceeds rexmtthresh

	 return;  // do neither open cwnd nor restart retransmission timer

      }	// end of: no ack of new data, no window update
      else
      	 dupacks = 0;	// no ack of new data, but we got a window update
	 // ### P2_: should we open cwnd and restart the retransmision timer ???
      	 // currently, we do it.

   } // end of (pa->TCPAack <= una): no ack of new data
   else	// the ACK acknowledges new data
   {
      int acked;

      if (doFastRetr && dupacks >= rexmtthresh && cwnd > ssthresh)	
      {	// first nonduplicate ACK -> fast recovery is complete
	 if (doLogRetr)
	    printf("# %s at %d: FROK\n", name, SimTime);
	 cwnd = ssthresh;  // if cwnd exceeds ssthresh set it back
	 cwnd_d = (double) cwnd;
      }

      dupacks = 0;

      if (pa->TCPAack > max_sent + 1)
      	 // other end is acknowledging data that TCP hasn't even sent
	 errm1s("%s: received acknowledge for byte I haven't yet sent", name);
   /*
      {	//  probably occurs on high-speed connection
	 //   when the seq # wrap and a missing ACK reappears later [Stevens95]
	 ++acktoomuch;			
	 nxt = pa->TCPAack;
      }
   */

      // now it is an acceptable ACK
      acked = pa->TCPAack - una; // calculate # of bytes acknowledged

      // Log acknowledged bytes and segments
      ++received_acks;
      ackbyte += acked;

      // log real RTT
      if (SimTime >= pa->TCPAPackStamp)
      	 rtt_real = SimTime - pa->TCPAPackStamp;	
      else
      	 fprintf(stderr, "%s: SimTime has been reset - reusing old rtt_real",
	    name);
      rtt_real_min = min(rtt_real_min, rtt_real);
      rtt_real_max = max(rtt_real_max, rtt_real);

      // evaluate timestamp
      // ### P3_: don't we give a time of one tick too much ???
      if (do_timestamp)
      	 tcp_xmit_timer(tcp_now - pa->TCPAecr + 1);
      else if (rtt && pa->TCPAack > rtseq)
      {
      	 tcp_xmit_timer(rtt);
	 rtt = 0; 	// rtt measurement for this packet is complete
      }

      // update nxt and una
      if (nxt < pa->TCPAack)
      	 // this is possible e.g. if we just had a RTO (and reset nxt),
	 // but now we received the ACK we were waiting for (### P4_)
      	 nxt = pa->TCPAack;

      una = pa->TCPAack;

      if (input_q_len >= acked)
      {
      	 input_q_len -= acked;
	 if (prec_state == StopSend && input_q_len <= q_start &&
	    !EndSend && !aborted)
	 {  // wake up process on send buffer (ContSend)
	    sucs[SucCtrl]->rec(new data, shands[SucCtrl]);
	    prec_state = ContSend;
	 }
      }		
      else
      	 errm1s("%s: internal error: tcpipsend::process_ack(): "
	    "input_q_len < acked", name);

   }	// (pa->TCPAack > una), something new acknowledged

   // update window information
   // ### P5_: which role does the ack seq number play ???
   if (seqLastWndUpd < pa->TCPAseq || (seqLastWndUpd == pa->TCPAseq &&
      (ackLastWndUpd < pa->TCPAack || (ackLastWndUpd == pa->TCPAack &&
      pa->TCPAwnd > wnd))))
   {
      wnd = pa->TCPAwnd;
      seqLastWndUpd = pa->TCPAseq;
      ackLastWndUpd = pa->TCPAack;

      // Log Window
      max_sendwnd = max(max_sendwnd, wnd);
      wnd_max = max(wnd_max, wnd);
      wnd_min = min(wnd_min, wnd);
   }

   // Open congestion window using Van Jacobson's strategy
   // ### P6_: why only in case wnd is not too small ???
   if (cwnd < wnd)
   {
      if (cwnd < ssthresh)
      	 cwnd_d += (double)(max_seg_size);
	 // increment it by 1 packet (multiplicative)
      else
      	 cwnd_d += (double)(max_seg_size)*(double)(max_seg_size) / cwnd_d;
	 // increment it by 1/cwnd packets (additive)
      	 // wrong:
	 // cwnd_d += (double)(max_seg_size)*(double)(max_seg_size) /
	 //    cwnd_d + (double)(max_seg_size) / 8; 
	 //    is wrong, but it is implemented in BSD since 4.3BSD Reno
	 //    and is still in 4.4BSD and Net/3
   }

   // set cwnd to least # of FULL packets allowed by cwnd_d
   cwnd = (((int)(cwnd_d))/max_seg_size) * max_seg_size;

   // stop retrans timer
   if(active_rt_timer)
   {
      unalarme(&rt_timer);
      active_rt_timer = FALSE;
   }

   // If the retransm. queue is not empty, restart the retransmission timer 
   if(una < nxt)
   {
      rto_val = rto_calc;
      //{ rto_val = (tim_typ)(rto_calc * (1000 + my_rand() % 1000) / 1000.0);
      if(SimTime < SimTime + rto_val)
      	 alarme( &rt_timer, rto_val);
      else
      	 errm1s("%s: want to alarm an event later then maximum SimTime\n",
	    name);
      active_rt_timer = TRUE; 
   }

   calc_send(TCPPlain);

} // end of tcpipsend::process_ack()



/********************************************************************/

void tcpipsend::tcp_xmit_timer(tim_typ rtt_l)
{
   // Recalculate RTO if clock info in packet or rtt measurement
   // Algorithm is outlined in Van Jacobson's paper at SIGCOMM '88

   int M;

   M = rtt_l;

   // log measured rtt
   rtt_calc = rtt_l;
   rtt_min = min(rtt_min, rtt_calc);
   rtt_max = max(rtt_max, rtt_calc);

   if (SA == 0)
   {
      SA = (M << 3);	// set up initial SA (don't include default rto)
      SD = (SA >> 1);
   }
   M -= (SA >> 3);
   SA += M;
   if (M < 0)
      M = -M;
   M -= (SD >> 2);
   SD += M;
   rto_calc = ticks_to_slots((double)((SA >> 3) + SD)); 

   rto_calc = min(rto_calc, secs_to_slots(rto_ub));  // be aware of bounds
   rto_calc = max(rto_calc, secs_to_slots(rto_lb));

   // Log rto_calc
   rto_min = min(rto_min, rto_calc);
   rto_max = max(rto_max, rto_calc);
}
		    

/********************************************************************/
/*
*	look if we can send something
*/
void	tcpipsend::calc_send(
	TCPsendMode	mode)
{
   int	max_to_send_offset, max_buf_offset;
   int	len;

   // calculate upper bound of window to send
   max_to_send_offset = una + min(wnd, cwnd) - 1;

   // calculate upper bound of data user has given me
   max_buf_offset = una + input_q_len - 1;
   if(max_buf_offset < nxt - 1)  // ### P7_: possible ???
      	             	      	 // it should not.
      errm1s("%s: internal error in tcpipsend::calc_send(): "
      	 "impossible condition", name);
	 // max_buf_offset = nxt - 1;

   // eventually we can send the min of both
   max_to_send_offset = min(max_to_send_offset, max_buf_offset);

   if (max_to_send_offset - nxt + 1 > max_seg_size && mode != TCPPlain)
      // this would confuse the timer handling in send_pkt()
      errm1s("%s: internal error: tcpipsend::calc_send(): "
	      "retransmission segment larger than MSS", name);

   // now send all the data we can
   while (nxt <= max_to_send_offset)
   {
#ifdef	PROCQTHRESH
      // Stop enqueueing if procqThresh is reached, set calcSendStopped.
      // Not for retransmissions (they ignore the threshold).
      if (mode == TCPPlain && procq.getlen() >= procqThresh)
      {	calcSendStopped = TRUE;
	      // printf("%s: sending stopped due to procqThresh\n", name);
	      break;
      }
#endif
      len = min(max_seg_size, max_to_send_offset - nxt + 1);

      /*
      *	Silly window avoidance according to RFC 1122, section 4.2.3.4,
      *	combined with Nagle's algorithm (also section 4.2.3.4)
      */
      if (mode != TCPPlain ||	       // retransmissions are always sent
	  len == max_seg_size ||       // condidition (1)
	  len >= max_sendwnd / 2 ||    // condition (3), but modified
				       // (no influence of Nagle alg.)
      	  ((nagleOff || (una == nxt)) && max_to_send_offset == max_buf_offset))
				       // condition (2)
      {
      	 queue_pkt(len, mode);
	 nxt += len;
      }
      else
      	 break;
   }

   // ### P9_: shouldn't this be done when really sending the packet ???
   max_sent = max(max_sent, nxt - 1);

}	// end of tcpipsend::calcsend()

/********************************************************************/
/*
*	queue a packet to be sent
*/
void tcpipsend::queue_pkt(int len,TCPsendMode mode)
{
   tcpipFrame	*pf;
   int		sdu_len;

   sdu_len = len + tcp_header_len + ip_header_len +
      (do_timestamp ? 1 : 0) * ts_option_len;

   pf = new tcpipFrame(nxt, sdu_len, this);
   pf->TCPPackStamp = SimTime;
      // this time stamp is for calculating the begin of a connection
      // in calculation of throughput of this connection

   if (do_timestamp)		// if timestamp is switched on (RFC1323)
      pf->TCPtimestamp = tcp_now;
   else
      pf->TCPtimestamp = 0;

   // if no RTT measuremnt running and no retransmission (Karn's algorithm):
   // start measurement
   if (rtt == 0 && mode == TCPPlain)
   {
      rtseq = pf->TCPseq;
      rtt = 1;	// set rtt = 1 means that this segment is now measured
   }

   // If the retrans timer is off and this is not a retransmission, restart it.
   // In case of a retransmission, it will be started afterwards, see early()
   // ### P10_: shouldn't this be done when really sending the packet ???
   if (active_rt_timer == FALSE && mode != TCPRetrans)
      // for fast retransmit, we have to start the timer (has been stopped,
      // see process_ack())
   {
      rto_val = rto_calc;
      //{ rto_val = (tim_typ)(rto_calc * (1000 + my_rand() % 1000) / 1000.0);
      // printf("%s: starting RT timer with rto_val = %d\n", name, rto_val);

      if(SimTime < SimTime + rto_val)
      	 alarme( &rt_timer, rto_val);
      else
      	 errm1s("%s: want to alarm an event later then maximum SimTime\n",
	    name);
      active_rt_timer = TRUE;
   }

   // add packet at tail of the processing queue
   pf->TCPSendStamp = (tim_typ) mode;	// mark whether retransmission or not
   procq.enqueue(pf);

   // Log processing queue
   procq_max_len = max(procq_max_len, procq.getlen());

   if( !active_procq)
   {
      // Delay as though I am processing the packet, then handle the packet
      if(send_state == ContSend)
      {	active_procq = TRUE;
	      alarme( &evtProcq, procDelay());
      }
      else
      	 next_send_time = SimTime + procDelay();
   }

}  // end of tcpipsend::queue_pkt()


/********************************************************************/   
/*
*	send the first packet from the processing queue
*/
void tcpipsend::send_pkt(void)
{
   // called by event evtProcq
   // as well as by a received Start signal (input ..->Start)

   // When I get here, I have already done the processing time for the
   // pkt at the head of the queue.

   if (send_state == StopSend || active_procq || procq.isEmpty())
      errm1s("%s: internal error: tcpipsend::send_pkt(): illegal call of "
      	 "method", name);

   tcpipFrame	*pf;
   if((pf = (tcpipFrame *) procq.dequeue()) == NULL)
      errm1s("%s: internal error: tcpipsend::process_ptk: no packet in queue",
      	 name);

   unsigned	ll;
   ll = pf->frameLen - (tcp_header_len + ip_header_len +
      (do_timestamp ? 1 : 0) * ts_option_len);
   if (pf->TCPseq + ll - 1 > maxByteReallySent)
      maxByteReallySent = pf->TCPseq + ll -1;
   else
      ++segsReallyRetransmitted;

   // Log number of xmitted bytes and segments
   ++xmitted_segments;
   if(xmitted_bytes > xmitted_bytes + pf->frameLen)
      errm1s1d("%s: overflow of statistic: xmitted_bytes > %d\n",
      	 name, xmitted_bytes);
   else
      xmitted_bytes += pf->frameLen;

   // log the xmitted user bytes
   xmitted_user_bytes += pf->frameLen - (tcp_header_len + ip_header_len);
   if (do_timestamp)
      xmitted_user_bytes -= ts_option_len;

   // log rexmitted bytes and segments
   if (((TCPsendMode)pf->TCPSendStamp) != TCPPlain)
	   // this is a retransmitted segment
   {
      rexmitted_bytes += pf->frameLen;
      ++rexmitted_segments;
   }

   // write the statistical field for QoS delay measurement
   pf->TCPSendStamp = SimTime;

   // calculate retransmission percentage
   rexmission_percentage = (double)rexmitted_bytes / (double)xmitted_user_bytes;

   if ( !connIdInitialized)
   errm1s("%d: internal error in tcpipsend::send_pkt(): connID not initialized",
      name);
   pf->connID = connID;	// added: July 18, 1997. MBau
			// copy ID of data frames into IP frames

   // send the packet
   chkStartStop(send_state = sucs[SucData]->rec(pf, shands[SucData]));

   if ( !procq.isEmpty())
   {
      switch (send_state) {
      case ContSend:	// continue to send
	 alarme( &evtProcq, procDelay()); // active_procq has been checked above
	 active_procq = TRUE;
	 break;
      case StopSend:	// store time where we wanted to send
	 next_send_time = SimTime + procDelay();
	 break;
      default:
      	 errm1s("%s: internal error: tcpipsend::send_pkt(): "
      	    "invalid send_state", name);
      }
   }

#ifdef	PROCQTHRESH
   // re-run calc_send() in case it has been stopped due to procq threshold
   if (calcSendStopped && procq.getlen() < procqThresh)
   {
      calcSendStopped = FALSE;
      calc_send(TCPPlain);	// may set calcSendStopped again
   }
#endif

}	// end of tcpipsend::send_pkt(void)


/********************************************************************/
void tcpipsend::restim(void)
{
   if(next_send_time > SimTime)
      next_send_time -= SimTime;
   else
      next_send_time = 0;
   rtt = 0;	// no rtt measurement now
}

void tcpipsend::resetStat()
{
  received_bytes = 0;
  xmitted_bytes = 0;
  xmitted_user_bytes = 0;
  xmitted_segments = 0;
  rexmitted_segments = 0;
  segsReallyRetransmitted = 0;
  rexmitted_bytes = 0;
  rexmto = 0;
  procq_max_len = 0;
  inputq_max_len = 0;
  received_acks = 0;
  rexmission_percentage = 0;
  // Test Mue ABORT_CNT 15.04.1998
  int i;
  for(i=0; i<13; i++)
    StatAbortCnt[i]=0;
}
#if 0
/********************************************************************/
/*
*	command: reset statistics
*/
int tcpipsend::command(char *s, tok_typ *v)
{
   if (baseclass::command(s, v))
	   return TRUE;

   v->tok = NILVAR;
   if(strcmp(s, "ResetStat") == 0)
   {
      received_bytes = 0;
      xmitted_bytes = 0;
      xmitted_user_bytes = 0;
      xmitted_segments = 0;
      rexmitted_segments = 0;
      segsReallyRetransmitted = 0;
      rexmitted_bytes = 0;
      rexmto = 0;
      procq_max_len = 0;
      inputq_max_len = 0;
      received_acks = 0;
      rexmission_percentage = 0;
      // Test Mue ABORT_CNT 15.04.1998
      int i;
      for(i=0; i<13; i++)
	 StatAbortCnt[i]=0;
   }
   else
      return FALSE;

   return TRUE;

}
#endif
/********************************************************************/
/*
*	export addresses of variables
*/
int tcpipsend::export(exp_typ *msg)
{
   return
      baseclass::export(msg) ||
      intScalar(msg, "NXT", &nxt) ||
      intScalar(msg, "UNA", &una) ||
      intScalar(msg, "RTT", (int*)&rtt_calc) ||
      intScalar(msg, "RTTreal", (int*)&rtt_real) ||
      intScalar(msg, "RTOcalc", (int*)&rto_calc) ||
      intScalar(msg, "CWND", &cwnd) ||
      intScalar(msg, "SSTHRESH", &ssthresh) ||      
      doubleScalar(msg, "CWND_D", &cwnd_d) ||
      intScalar(msg, "WND", &wnd) ||
      intScalar(msg, "WND_MAX", &wnd_max) ||
      intScalar(msg, "WND_MIN", &wnd_min) ||
      intScalar(msg, "INPQ", &input_q_len) ||
      intScalar(msg, "INPQ_LEN", &input_q_len) ||
      intScalar(msg, "INPQ_MAX_LEN", &inputq_max_len) ||
      intScalar(msg, "PRCQ_LEN", &procq.q_len) ||
      intScalar(msg, "PRCQ_MAX_LEN", &procq_max_len) ||
      intScalar(msg, "REXMTO", &rexmto) ||	// retransmission time outs
      intScalar(msg, "RECEIVED_BYTES", &received_bytes) ||
      intScalar(msg, "XMITTED_BYTES", &xmitted_bytes) ||
      intScalar(msg, "XMITTED_USER_BYTES", &xmitted_user_bytes) ||
      intScalar(msg, "XMITTED_SEGMENTS", &xmitted_segments) ||
      intScalar(msg, "REXMITTED_BYTES", &rexmitted_bytes) ||
      intScalar(msg, "REXMITTED_SEGMENTS", &rexmitted_segments) ||
      intScalar(msg, "RexmSegs", (int *) &segsReallyRetransmitted) ||
      intScalar(msg, "RECEIVED_ACKS", &received_acks) ||
      doubleScalar(msg, "REX_PERC", &rexmission_percentage) || 
      // Test Mue ABORT_CNT 15.04.1998
      intArray1(msg, "AbortCount", (int *) StatAbortCnt, 13, 1);
}


int	tcpipsend::min(int i1, int i2)
{
	return i1 <= i2 ? i1 : i2;
}
int	tcpipsend::max(int i1, int i2)
{
	return i1 >= i2 ? i1 : i2;
}
#if 0
tim_typ	tcpipsend::ticks_to_slots(double d)
{
	tim_typ result;
  
	if((result = (tim_typ) (d * tick * bitrate / 53. / 8.)) < 1)
		result = 1;	// avoid slots < 1
	return result;
}  
tim_typ	tcpipsend::slots_to_ticks(tim_typ t)
{
	return (size_t) ((double)t / tick / bitrate * 53. * 8.);
}

double	tcpipsend::slots_to_secs(tim_typ t)
{
	return (double)t / bitrate * 53. * 8.;
}

tim_typ	tcpipsend::secs_to_slots(double s)
{
	tim_typ result;

	if((result = (tim_typ) (s * bitrate / 53. / 8.)) < 1)
		result = 1;	// avoid slots < 1
	return result;
}
#else
tim_typ	tcpipsend::ticks_to_slots(double d)
{
	tim_typ result;
  
	if((result = (tim_typ) (d * tick / SlotLength)) < 1)
		result = 1;	// avoid slots < 1
	return result;
}  
tim_typ	tcpipsend::slots_to_ticks(tim_typ t)
{
	return (size_t) ((double)t / tick * SlotLength);
}

double	tcpipsend::slots_to_secs(tim_typ t)
{
	return (double)t * SlotLength;
}

tim_typ	tcpipsend::secs_to_slots(double s)
{
	tim_typ result;

	if((result = (tim_typ) (s / SlotLength)) < 1)
		result = 1;	// avoid slots < 1
	return result;
}
#endif
tim_typ	tcpipsend::procDelay(void) // change made by Joerg Schueler
{
	if (ph_efOn == TRUE)
	   return (tim_typ) (proc_time); // proctime  
	else
	   return (tim_typ) (proc_time * (1000.0 + my_rand() % 100) / 1000.0); // proctime + random  (Random seed ON)
}

