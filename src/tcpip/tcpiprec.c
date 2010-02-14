
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
*	Module authors:		Gunnar Loewe, TUD (diploma thesis)
*				Matthias Baumann, TUD
*	Creation:		Sept 1996
*
*	History:
*	July 10, 1997		name changes which result from new member
*				names of frame, tcpipFrame, and tcpAck. All
*				these frame types now havn't anymore a
*				core structure.
*					Matthias Baumann
*	July 18, 1997		layer 4 connection ID of incoming frames
*				is copied into data frames and ACK frames.
*				Adds in init(), rec(), send_ack(), and
*				process_output(). New object mebers:
*				int connID; int connIdInitiaized.
*					Matthias Baumann
*
*	Dec 12, 1997		Check of routing using the object pointers
*				in incoming TCP packets. See the explanation
*				in tcpipsend.c. Also in ACKs, the this pointer
*				is written, allowing the sender to check the
*				back-channel.
*					Matthias Baumann
*
*	Sep 22, 1998		Fixed a problem with sending an ACK in case 
*				of repeated data frames.
*					Joerg Schueler
*     	Nov. 1999             	 Including a Keepalive Timer, this sends an
*     	             	      	 immediate ack - Mü
*
*************************************************************************/

/*************************************************************************
**									**
** Syntax:								**
**	TCPIPrec tcpr:	WND=<receive window size>,			**
**			[PROCTIM=<processing time, millisecs>,]		**
**			[ACKDEL=<delay of delayed ACKs, millisecs>,]	**
**			[PH_EF=<random component of proctime 0/1>,]	**
**			[IACKDEL=<delay of immediate ACKs, millisecs>,]	**
**			[KEEPALIVE=<keepalive timer, millisecs>,]	**
**			OUTDATA=<successor for Data>,			**
**			OUTACK=<input Ack of TCP sender>;		**
**									**
**	Default values:	PROCTIM = 0.3 milliseconds			**
**			ACKDEL = 200 milliseconds			**
**			IACKDEL = PROCTIM				**
**			KEEPALIVE - no keepalive timer	             	**
**									**
**	During connection setup, the following values are given by	**
**	the corresponding TCP sender:					**
**		- Time stamp option on or off				**
**		- MTU size (to derive MSS)				**
**		- duration of one TCP clock tick			**
**		- ATM brutto bit rate (to translate seconds into slots)	**
**	The sender is informed about the window size WND.		**
**									**
*************************************************************************/

/**

Acknowledgement Handling
========================
Delayed ACK:	- If we got new in-sequence data. Done in early(), branch evtProcq.
Immediate ACK:	- If we got out-of-sequence data. Done in early(), branch evtProcq.
		- If the receiver window closed to 0. Done in process_reseq().
		- If the receiver window opened by at least 1 MSS or 50 (35 fo SunOS) per cent of the
		  buffer size (RFC1122, section 4.2.3.3). Done in process_output().
		- If we got a window probe (window is 0). Done in early(), branch evtProcq.

Both ACK types are launched via timers (evtDelAck and evtImAck).

Time Stamp Processing
=====================
Time stamps are processed according to RFC1323 (if turned on at the peer sender).

Central routines
================
rec() (branch: data received):
Incomming packets are simply queued in the processing queue which models the processing delay per packet.
The process which serves the queue is the main part of the method early(), where in the asociated branch
the queue is served. This process is woken up if necessary.

early() (branch: event for serving processing queue)
A packet is taken from the processing queue, and the sequence number and length of the packet are corrected
according to the nxt value already reached, and to the current window size (the latter against misbehaving
sender). If a window probe is encountered, the immediate acknowledge is launched. If the packet begins with
the data expected next, the packet is enqueued into the resequencing queue (to make things more uniform),
and the routine process_reseq() is called (see below). The need for a delayed acknowledge is remembered
in the flag needDelAck. If the packet is out-of-order, it is enqueued in the resequencing queue (at the
right place), and an immediate ACK is caused.

process_reseq():
The resequencing queue is checked. If data can be extracted, they are put into the output buffer (size WND),
and the window is closed appropriately. The window is opened by the routine process_output() which is
invoked by process_reseq() if we are not stopped by the succeeding network object.

process_output():
Sends a packet from the output buffer, if allowed by the succeeding network object. The window is opened
and the new window size is advertized to the sender, following the algorithms for silly window avoidance.
The method is always invoked via an event to ensure that never two packets are sent per time slot.
Receiving a Start signal from the successor starts this timer.

send_ack():
Sends an acknowledge.

Problems
========
1.	The start-stop protocol is not supported on the ACK output.
2.	RFC 1122, section 4.2.3.2, recommends to send an ACK at least for each second full-sized incomming
	segment. This currently only is implicitely realised by the window update normally occuring shortly
	afterwards. Including e.g. a test on two unacknowledged segments into process_reseq() would pose
	a problem: possibly, both a receive ACK and a window update ACK could be created for a segment.
3.	Other open problems are marked with '###'.

**/

#include "tcpiprec.h"

tcpiprec::~tcpiprec()
{
}

int tcpiprec::act(void)
{
  ack_seq = 1;		// sequence number of ack packet
  nxt = 1;		// next receive sequence number
  adv = nxt + wnd;	// highest advertised rigth window edge plus 1
  
  connected = FALSE;
  ptrTcpSend = NULL;
  
  needImAck = FALSE;
  needDelAck = FALSE;
  
  tcp_header_len = 20;	// length of TCP header
  ip_header_len = 20;	// length of IP header
  ts_option_len = 12;	// legth of timestamp option field
  
  ts_recent = 0;		// timestamp value of arrived segment
  last_ack = nxt;		// sequence number of byte with timestamp for echo
  
  sockstate = ContSend;
  activeOutput = FALSE;
  lastPackStamp = 0;
  
  reseq_head = NULL;
  
  tcp_now = 1;
  
  // statistics
  arrived_segments = 0;
  arrived_bytes = 0;
  arrived_valid_bytes = 0;
  user_packets = 0;
  user_bytes = 0;
  tp_bytes = 0;
  procq_max_len = 0;
  reseq_max_len = 0;
  reseq_len = 0;
  out_of_order_segments = 0;
  out_of_order_bytes = 0;
  ack_cnt = 0;
  
  throughput = 0.0;
  SDU_delay = 0;			// delay of SDUs
  SDU_delay_mean = 0.0;		// mean delay of SDUs
  SDU_cnt = 0;			// counter for delay statistic calculation
  PDU_delay = 0;			// delay of TCP PDUs
  PDU_delay_mean = 0.0;		// mean delay of TCP PDUs
  PDU_cnt = 0;			// counter for delay statistic calculation
  
  conn_time = 0;			// time for connection establishment
					// (it's just the time the first segment is generated)
  sendtwice = 0;			// only for debugging (avoids sending twice in a slot)
  
  connIdInitialized = FALSE;
  return 0;
}
#if 0
//	Transfered to LUA init

/*****************************************************************************/
/*
*	read the definition statement
*/
/*
void	tcpiprec::init(void)
{
	skip(CLASS);
	name = read_id(NULL);
	skip(':');

	max_win = read_int("WND");
	if(max_win <= 0)
		syntax1s("%s: WND must be greater than 0", name);
	wnd = max_win;
	skip(',');
	
	if(test_word("PROCTIM"))
	{	proctim_secs = read_double("PROCTIM") / 1000;
		if (proctim_secs < 0.0)
			syntax0("invalid PROCTIM");
		skip(',');
	}
	else	proctim_secs = 0.0003;	// default: 300 microsecs
	
	if(test_word("ACKDEL"))
	{	ackdel_secs = read_double("ACKDEL") / 1000;
		if (ackdel_secs < 0.0)
			syntax0("invalid ACKDEL");
		skip(',');
	}
	else	ackdel_secs = 0.2;	// default: 0.2 secs
	
	if(test_word("PH_EF"))			// Phase Effects ON/OFF
	{	if (read_int("PH_EF") == 0)
			ph_efOn = FALSE;
		else	ph_efOn = TRUE;
		skip(',');			// Phase Effects OFF per default
	}
	else 	ph_efOn = FALSE;

	if(test_word("IACKDEL"))
	{	iackdel_secs = read_double("IACKDEL") / 1000;
		if (iackdel_secs < 0.0)
			syntax0("invalid IACKDEL");
		skip(',');
	}
	else	iackdel_secs = proctim_secs;	// default IACKDEL = PROCTIM
	
	if(test_word("KEEPALIVE"))
	{	keepalive_secs = read_double("KEEPALIVE") / 1000.0;
		if (keepalive_secs < 0.0)
			syntax0("invalid KEEPALIVE");
		skip(',');
	}
	else	keepalive_secs = 0;	// default - no keepalive
	
	input("Data", InpData);
	input("Start", InpStart);

	output("OUTDATA", SucData);
	skip(',');
	output("OUTACK", SucAck);

	ack_seq = 1;		// sequence number of ack packet
	nxt = 1;		// next receive sequence number
	adv = nxt + wnd;	// highest advertised rigth window edge plus 1

	connected = FALSE;
	ptrTcpSend = NULL;
	
	needImAck = FALSE;
	needDelAck = FALSE;

	tcp_header_len = 20;	// length of TCP header
	ip_header_len = 20;	// length of IP header
	ts_option_len = 12;	// legth of timestamp option field
	
	ts_recent = 0;		// timestamp value of arrived segment
	last_ack = nxt;		// sequence number of byte with timestamp for echo
		
	sockstate = ContSend;
	activeOutput = FALSE;
	lastPackStamp = 0;

	reseq_head = NULL;
	
	tcp_now = 1;

	// statistics
	arrived_segments = 0;
	arrived_bytes = 0;
	arrived_valid_bytes = 0;
	user_packets = 0;
	user_bytes = 0;
	tp_bytes = 0;
	procq_max_len = 0;
	reseq_max_len = 0;
	reseq_len = 0;
	out_of_order_segments = 0;
	out_of_order_bytes = 0;
	ack_cnt = 0;
	
	throughput = 0.0;
	SDU_delay = 0;			// delay of SDUs
	SDU_delay_mean = 0.0;		// mean delay of SDUs
	SDU_cnt = 0;			// counter for delay statistic calculation
	PDU_delay = 0;			// delay of TCP PDUs
	PDU_delay_mean = 0.0;		// mean delay of TCP PDUs
	PDU_cnt = 0;			// counter for delay statistic calculation
	
	conn_time = 0;			// time for connection establishment
					// (it's just the time the first segment is generated)
	sendtwice = 0;			// only for debugging (avoids sending twice in a slot)

	connIdInitialized = FALSE;
}
*/
#endif

//*****************************************************************************/
// something received
//*****************************************************************************/
// REC is a macro normally expanding to rec (for debugging)
rec_typ	tcpiprec::REC(data *pd,	int key)
{
  switch (key) {
  case InpData:	// data input, continuation below
    break;
  case InpStart:	// is interpreted as a START signal
    			// means the application is able to read data again
    if (sockstate == StopSend) {
      sockstate = ContSend;
      if ( !outputq.isEmpty() && !activeOutput)	{
	alarme( &evtOutput, 1);
	activeOutput = TRUE;
      }
    }
    delete pd;
    return ContSend;
  default:errm1s("%s: undefined input", name);
    return StopSend; // not reached
  }
  
  // input DATA, all other cases already returned
  tcpipFrame *pf = (tcpipFrame *) pd;
  typecheck_i(pd, TCPIPFrameType, key);
  
  // added July 18, 1997
  if (connIdInitialized == FALSE) {
    connIdInitialized = TRUE;
    connID = pf->connID;
  } else if (connID != pf->connID)
    errm1s2d("%s: illegal change of incoming connection ID, old: %d, new: %d",
	     name, connID, pf->connID);
  // end of add July 18. MBau
  
  if ( !connected)
    errm1s("%s: packet received, but not connected to a TCP sender", name);
  // check the routing: does this packet stem from our peer?
  if (pf->sendingObj != ptrTcpSend)
    errm3s("%s: connected to sender `%s', but packet received from `%s'",
	   name, ptrTcpSend->name, pf->sendingObj->name);
      
  // drop the headers
  pf->frameLen -= tcp_header_len + ip_header_len + (do_timestamp ? 1: 0) * ts_option_len;
  if (pf->frameLen < 0)
    pf->frameLen = 0;

  if(arrived_segments == 0 && pf->TCPseq == 1)	// this is the first segment
    conn_time = pf->TCPPackStamp;	// connection opens at the time, sender sent the first segment
		
  // Log the arrival of the packet
  ++arrived_segments;
      
  // Queue the received packet for processing
  if (procq.isEmpty())	//wake up after proc_time
    alarme( &evtProcq, procDelay(proc_time));
  procq.enqueue(pf);
		
  // Log processing_queue
  if (procq_max_len < procq.getlen())
    procq_max_len = procq.getlen();

  // log the delay of the TCP PDU
  if (SimTime >= pf->TCPSendStamp) {
    PDU_delay = SimTime - pf->TCPSendStamp;
    PDU_delay_mean = (PDU_delay_mean * PDU_cnt + PDU_delay) / (double)(PDU_cnt + 1);
  }
  else
    fprintf(stderr, "%s: SimTime has been reset, reusing last PDU_delay", name);

  ++PDU_cnt;
  
  return	ContSend;
  
} // end of tcpiprec::rec()
  
//*****************************************************************************/
// A timer expired
//*****************************************************************************/
void  tcpiprec::early(event *evt)
{
  switch (evt->key) {
  case keyProcq:
    break;	// processing queue, continuation below
    
  case keyImAck:	// timer for immediate ACK
    needImAck = FALSE;
		// we also doe the job of the delayed ACK
    if (needDelAck) {
      unalarme( &evtDelAck);
      needDelAck = FALSE;
    }
    send_ack();
    return;

  case keyTick:	// clock tick
    ++tcp_now;
    alarme( &evtTick, ticks_to_slots(1));
    
    // Update throughput value. netto throughput in bit per sec
    if (SimTime > conn_time)
      throughput = tp_bytes / slots_to_secs(SimTime - conn_time) * 8;
    return;
    
  case keyDelAck:	// timer for delayed ACK
    needDelAck = FALSE;
    // if an immediate ACK is on the way, we skip this delayed one ...
    // ### we also could say: use this ACK and drop the immediate one ???
    if ( !needImAck)
      send_ack();
    return;
    
  case keyOutput:	// send next data to user
    activeOutput = FALSE;
    process_output();
    return;
    
  case keyKeepAlive:	// Keep Alive Timer
    if(keepalive_secs > 0)
      alarme( &evtKeepAlive, secs_to_slots(keepalive_secs));
    
    if ( arrived_segments > 0 && !needImAck) {
      needImAck = TRUE;
      alarme( &evtImAck, procDelay(iack_delay));
    }
    return;
    
  default:errm1s("%s: in tcpiprec::early(): internal error: unknown event type", name);
    return;
  }

  // process waiting packets, all other cases already are returned
  
  tcpipFrame	*pf;     
  if ((pf = (tcpipFrame *) procq.dequeue()) == NULL)
    errm1s("%s: internal error: tcpipsend::early(): queuing error in processing queue", name);
  
  if ( !procq.isEmpty())
    alarme( &evtProcq, procDelay(proc_time));

  // Log real arrived length of packet
  if(arrived_bytes > arrived_bytes + pf->frameLen)
    fprintf(stderr, "%s: overflow of statistic: arrived_bytes > %d\n", name, arrived_bytes);
  else
    arrived_bytes += pf->frameLen;


  //test for window probe
  if (wnd == 0 && pf->frameLen == 1 && pf->TCPseq == nxt){
    // it is only a window probe, discard packet and send a window update
    // send an immediate ACK
    if ( !needImAck) {
      needImAck = TRUE;
      alarme( &evtImAck, procDelay(iack_delay));
    }
    
    delete pf;
    return;
  }


  // trim off portions of packet: repeated data ?
  // we got repeated data
  if (pf->TCPseq < nxt)	{
    if(pf->frameLen < nxt - pf->TCPseq) 
      pf->frameLen = 0;	// nothing new
    else
      pf->frameLen -= (nxt - pf->TCPseq);
  
  pf->TCPseq = nxt;
  }

  // more data than we have actually allowed?
  if (pf->TCPseq + pf->frameLen > nxt + wnd)	// yes: truncate the portion too much
    pf->frameLen -= (pf->TCPseq + pf->frameLen) - (nxt + wnd);

  if (pf->frameLen == 0) {
    // it's probably a fully repeated packet
    if ( !needImAck) {
      // we have to send an ACK !!!!
      needImAck = TRUE;				
      alarme( &evtImAck, procDelay(iack_delay));
    }
    // now it is not useful anymore
    delete pf;
    return;
  }  // added on 22/09/98 by Joerg Schueler
	
  if (pf->frameLen < 0)	{
    // it's probably rubbish data, so we don't ACK this  
    errm1s1d("%s: internal error: tcpipsend::early(): Frame Lenght < 0 not possible: value=%d", 
	     name, pf->frameLen);	
    delete pf;
    return;
  } 
	     
  // Log valid arrived length of packet
  if(arrived_valid_bytes > arrived_valid_bytes + pf->frameLen)
    fprintf(stderr, "%s: overflow of statistic: arrived_valid_bytes > %d\n", name, arrived_valid_bytes);
  else
    arrived_valid_bytes += pf->frameLen;

  // Log resequencing queue.
  // all segements are queued, regardless whether they overlap with already queued ones.
  // This is lateron checked by process_reseq()
  ++reseq_len;
  if(reseq_max_len < reseq_len)
    reseq_max_len = reseq_len;
  
  if(pf->TCPseq == nxt)	{
    // it is just the expected segment
    // insert pf at head of the resequencing queue
    pf->next = reseq_head;
    reseq_head = pf;

    lastPackStamp = pf->TCPPackStamp;
    // evaluation of time stamp
    if(do_timestamp)
      if (last_ack >= pf->TCPseq && last_ack < pf->TCPseq + pf->frameLen)
	// it's the timestamp for echo
	// conditions: see RFC1323, section 3.4
	ts_recent = pf->TCPtimestamp;

    process_reseq();

    // send a delayed ACK:
    // if an immediate ACK is registered, we will use this instead
    if (needImAck == FALSE && !needDelAck){
      needDelAck = TRUE;
      alarme( &evtDelAck, ack_delay);
    }
  } else {
    // out-of-order segment
    // Log number and number of bytes of out-of-order segment
    ++out_of_order_segments;
    out_of_order_bytes += pf->frameLen;
    
    // insert pf at correct position in resequencing queue
    if (reseq_head != NULL) {
      //there are segments in queue
      if (pf->TCPseq < reseq_head->TCPseq) {
	//add segment to head of queue
	pf->next = reseq_head;
	reseq_head = pf;
      } else {
	// We have to walk through the queue.
	// Enqueue the segment in front of the first segment with seq larger
	// then the new seq. If not found, at the end. Tricky.
	// Overlapping (even complete) is recognized by process_reseq()
	tcpipFrame	*pk;
	for (pk = reseq_head; pk->next != NULL; pk = (tcpipFrame *)pk->next)
	  if (pk->TCPseq <= pf->TCPseq &&
	      pf->TCPseq < ((tcpipFrame *)pk->next)->TCPseq)
	    break;
	pf->next = pk->next;
	pk->next = pf;
      }
    } else {
      // end of: there are segments in reseq_queue
      // simple: the queue was empty
      reseq_head = pf;
      pf->next = NULL;
    }
    
    // send an immediate ACK because of out-of-order packet
    if ( !needImAck) {
      needImAck = TRUE;
      alarme( &evtImAck, procDelay(iack_delay));
    }

  } // end of out-of-order segment
  
} // end of early()


//*****************************************************************************/
//	extract data from resequencing queue.
//	is only called if segment in right order arrived
//*****************************************************************************/
void tcpiprec::process_reseq(void)
{
  tcpipFrame  *pk;
  
  pk = reseq_head;
  // Process as many packets as possible from the reseq queue.
  // As long as we have packets and they do not produce a hole:
  while (pk != NULL && pk->TCPseq <= nxt) {
    
    // cut the portion which is below the nxt we have already reached
    if (pk->frameLen < nxt - pk->TCPseq)
      pk->frameLen = 0;
    else
      pk->frameLen -= (nxt - pk->TCPseq);
    
    pk->TCPseq = nxt;
 
    // close window
    if((wnd -= pk->frameLen) < 0)
      // this should have been avoided by the packet truncation
      // performed by rec()
      errm1s("%s: internal error: close wnd < 0\n", name);
		
    // mark the progress
    nxt += pk->frameLen;
    
    // log the SDU delay 
    if (SimTime >= pk->TCPPackStamp){
      SDU_delay = SimTime - pk->TCPPackStamp;
      SDU_delay_mean = (SDU_delay_mean * SDU_cnt + SDU_delay) / (double)(SDU_cnt + 1);
    }
    else
      fprintf(stderr, "%s: SimTime has been reset, reusing last SDU_delay", name);
	
    ++SDU_cnt;
    
    reseq_head = (tcpipFrame *)pk->next;
    
    // queue the packet in output queue
    if (pk->frameLen > 0)
      outputq.enqueue(pk);
    else
      delete pk;
    
    pk = reseq_head;
    --reseq_len;
  }

  // wake up output process
  if( !activeOutput && !outputq.isEmpty() && sockstate == ContSend){
    alarme( &evtOutput, 1);
    activeOutput = TRUE;
  }

  // did we close the window ?
  if (wnd == 0)	{
    // send immediate ACK (Null-window update)
    if ( !needImAck){
      needImAck = TRUE;
      alarme( &evtImAck, procDelay(iack_delay));
    }
  }
}

//*****************************************************************************/
// send data to the user
//*****************************************************************************/
void tcpiprec::process_output()
{
  tcpipFrame	*pk;
  int		adv_val;
  
  if (outputq.isEmpty() || activeOutput || sockstate == StopSend)
    errm1s("%s: internal error: tcpiprec::proc_outp(): illegal call of method", name);
  
  pk = (tcpipFrame *) outputq.dequeue();
  
  // Log transmitted data
  ++user_packets;
  if(user_bytes > user_bytes + pk->frameLen)
    fprintf(stderr, "%s: overflow of statistic: user_bytes + length = %d +%d\n",
	    name, user_bytes, pk->frameLen);
  else {
    user_bytes += pk->frameLen;
    tp_bytes  += pk->frameLen;
  }

  // Update throughput value.
  throughput = tp_bytes / slots_to_secs(SimTime - conn_time) * 8;	// netto throughput in bit per sec
  
  // open window
  wnd += pk->frameLen;
  if(wnd > max_win)
    wnd = max_win;
  
  // added July 18, 1997: copy connection ID into outgoing frames
  if ( !connIdInitialized)
    errm1s("%s: internal error in tcpiprec::send_ack(): connection ID not "
	   "initialized", name);
  // end of add July 18. MBau. The connID is given in the 'new' expression next statement.
  
  chkStartStop(sockstate = sucs[SucData]->rec(new frame(pk->frameLen, connID), shands[SucData]));
  
  /*
   *	Silly window avoidance (RFC1122, section 4.2.3.3):
   *	Send only an update if the window edge shifts by at least
   *	one maximum sized segment or at least 50% (SunOS 35%)
   */
  adv_val = wnd - (adv - nxt);	// this is the shift compared to ther last ACK
  if(adv_val >= max_seg_size || 2 * adv_val >= max_win) {
    // TEST Mue || wnd > (max_win - max_seg_size)
    // advertised window opens by 1 MSS or 
    //  advertises a window of least as 50% of max_win
    // if(adv_val >= max_seg_size || adv_val >= .35*max_win) // the same for SunOS
    if ( !needImAck) {
      needImAck = TRUE;
      alarme( &evtImAck, procDelay(iack_delay));
    }
  }
	
  if ( !outputq.isEmpty() && sockstate == ContSend){
    activeOutput = TRUE;
    alarme( &evtOutput, 1);
  }

  delete pk;
}

//*****************************************************************************/
//	send an ACK
//*****************************************************************************/
void tcpiprec::send_ack(void)
{
  tcpAck	*pa;
  
  pa = new tcpAck(ack_seq++, nxt, wnd, this);
  pa->TCPAPackStamp = lastPackStamp;
  
  if(SimTime == sendtwice)
    errm1s("%s internal error in tcpiprec::send_ack(): two ACKs per time slot", name);
  sendtwice = SimTime;
  
  if (do_timestamp){
    // algorithm: see RFC1323, section 3.4
    pa->TCPAecr = ts_recent;
    last_ack = nxt;
  } else
    pa->TCPAecr = 0;

  // added July 18, 1997: copy connection ID into outgoing frames
  if ( !connIdInitialized)
    errm1s("%s: internal error in tcpiprec::send_ack(): connection ID not "
	   "initialized", name);
  pa->connID = connID;
  // end of add July 18. MBau
  
  sucs[SucAck]->rec(pa, shands[SucAck]);
  
  if (nxt + wnd > adv)
    adv = nxt + wnd;
  // Log ACK
  ++ack_cnt;
}

//*****************************************************************************/
// connection establishment:
// determine timestamp option, MTU size, tick, bitrate,
// return wnd to sender
//*****************************************************************************/

char *tcpiprec::special(specmsg *msg,char *)
{
  if (msg->type != TCPConReqType)
    return "wrong type of special message";
  
  if (connected)
    return "already connected to a TCP sender";
  connected = TRUE;

  TCPConReqMsg	*pmsg;
  
  pmsg = ((TCPConReqMsg *)msg);
  pmsg->wnd = wnd;
  do_timestamp = pmsg->TS;
  MTU = pmsg->MTU;
  max_seg_size = MTU - (tcp_header_len + ip_header_len + (do_timestamp ? 1 : 0) * ts_option_len);
  tick = pmsg->tick;
  bitrate = pmsg->bitrate;
  ptrTcpSend = pmsg->ptrTcpSend;

  proc_time = secs_to_slots(proctim_secs);
  ack_delay = secs_to_slots(ackdel_secs);
  iack_delay = secs_to_slots(iackdel_secs);
  
  // prevents synchronisation of different
  alarme( &evtTick, my_rand() % ticks_to_slots(1));
  //  TCP connections
  
  if(keepalive_secs > 0)
    alarme( &evtKeepAlive, secs_to_slots(uniform() * keepalive_secs));
  
  return NULL;
}

void tcpiprec::resetStat(void)
{
  arrived_segments = 0;
  arrived_bytes = 0;
  arrived_valid_bytes = 0;
  user_packets = 0;
  user_bytes = 0;
  tp_bytes = 0;
  procq_max_len = 0;
  reseq_max_len = 0;
  out_of_order_segments = 0;
  out_of_order_bytes = 0;
  ack_cnt = 0;
  throughput = 0;
  SDU_delay_mean = 0.0;
  SDU_cnt = 0;
  PDU_delay_mean = 0.0;
  PDU_cnt = 0;
  conn_time = SimTime;
}
#if 0
//*****************************************************************************/
//	command ...->ResetStat
//*****************************************************************************/
*/
int	tcpiprec::command(char *s, tok_typ *v)
{
	if (baseclass::command(s, v))
		return TRUE;

	v->tok = NILVAR;
	if(strcmp(s, "ResetStat") == 0)
	{
		arrived_segments = 0;
		arrived_bytes = 0;
		arrived_valid_bytes = 0;
		user_packets = 0;
		user_bytes = 0;
		tp_bytes = 0;
		procq_max_len = 0;
		reseq_max_len = 0;
		out_of_order_segments = 0;
		out_of_order_bytes = 0;
		ack_cnt = 0;
		throughput = 0;
		SDU_delay_mean = 0.0;
		SDU_cnt = 0;
		PDU_delay_mean = 0.0;
		PDU_cnt = 0;
		conn_time = SimTime;
	}
	else	return FALSE;

	return TRUE;
}
#endif

//*****************************************************************************/
//	export addresses of variables
//*****************************************************************************/
int tcpiprec::export(exp_typ *msg)
{
  return baseclass::export(msg) ||
    doubleScalar(msg, "THROUGHPUT", &throughput) ||
    doubleScalar(msg, "SDU_DELAY", &SDU_delay_mean) ||
    doubleScalar(msg, "PDU_DELAY", &PDU_delay_mean) ||
    intScalar(msg, "RESQ_LEN", &reseq_len) ||
    intScalar(msg, "RESQ_MAX_LEN", &reseq_max_len) ||
    intScalar(msg, "PRCQ_LEN", &procq.q_len) ||
    intScalar(msg, "PRCQ_MAX_LEN", &procq_max_len) ||
    intScalar(msg, "ACK_CNT", &ack_cnt) ||
    intScalar(msg, "WND", &wnd) ||
    intScalar(msg, "ARRIVED_SEGMENTS", &arrived_segments) ||
    intScalar(msg, "ARRIVED_BYTES", &arrived_bytes) ||
    intScalar(msg, "ARRIVED_VALID_BYTES", &arrived_valid_bytes) ||
    intScalar(msg, "USER_PACKETS", &user_packets) ||
    intScalar(msg, "USER_BYTES", &user_bytes) ||
    intScalar(msg, "OUT_OF_ORDER_SEGMENTS", &out_of_order_segments) ||
    intScalar(msg, "OUT_OF_ORDER_BYTES", &out_of_order_bytes);
}

//*****************************************************************************/
// Auxiliary routines
//*****************************************************************************/
#if 0
tim_typ  tcpiprec::ticks_to_slots(double d)
{
  tim_typ result;
  
  if((result = (tim_typ) (d * tick * bitrate / 53 / 8)) < 1)
    result = 1;
  return result;
} 

double  tcpiprec::slots_to_secs(tim_typ t)
{
  return (double)t / bitrate * 53 * 8;
}

tim_typ	tcpiprec::secs_to_slots(double s)
{
  tim_typ result;
  
  if((result = (tim_typ) (s * bitrate / 53. / 8.)) < 1)
    result = 1;	// avoid slots < 1
  return result;
}
#else
tim_typ	tcpiprec::ticks_to_slots(double d)
{
	tim_typ result;
  
	if((result = (tim_typ) (d * tick / SlotLength)) < 1)
		result = 1;	// avoid slots < 1
	return result;
}  
double	tcpiprec::slots_to_secs(tim_typ t)
{
	return (double)t * SlotLength;
}

tim_typ	tcpiprec::secs_to_slots(double s)
{
	tim_typ result;

	if((result = (tim_typ) (s / SlotLength)) < 1)
		result = 1;	// avoid slots < 1
	return result;
}
#endif
tim_typ	tcpiprec::procDelay(tim_typ tim)
{
  if (ph_efOn == TRUE)
    return (tim_typ) (proc_time); // proctime  
  else
    return (tim_typ) (proc_time * (1000.0 + my_rand() % 100) / 1000.0); // proctime + random  (Random seed ON)
}


//*****************************************************************************/
// Reset a timer
//*****************************************************************************/
void	tcpiprec::restim(void)
{
  conn_time = 0;
  tp_bytes = 0;
  fprintf(stderr, "%s: statistic of throughput is reset on ResetTime\n", name);
}

