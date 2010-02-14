///////////////////////////////////////////////////////////////////////////////////////
// tcpipsendprio.c
// Baustein: TCPIPPRIOsend
// basiert auf tcpipsend
// zusaetzlich zur Funktionalitaet von tcpipsend werden alle
// Übertagungswiederholungen mit CLP=0 versehen.
///////////////////////////////////////////////////////////////////////////////////////

#include "tcpipsendprio.h"

CONSTRUCTOR(Tcpipsendprio, tcpipsendprio);
USERCLASS("TCPIPPRIOsend", Tcpipsendprio);


/********************************************************************/
/*
*	queue a packet to be sent
*/
void	tcpipsendprio::queue_pkt(
	int		len,
	TCPsendMode	mode)
{
	tcpipFrame	*pf;
	int		sdu_len;

	sdu_len = len + tcp_header_len + ip_header_len + (do_timestamp ? 1 : 0) * ts_option_len;

	pf = new tcpipFrame(nxt, sdu_len, this);
	pf->TCPPackStamp = SimTime;	// this time stamp is for calculating the begin of a connection
					// in calculation of throughput of this connection

	
	// TEST Mue - hohe Prioritaet fuer Retransmissions
	if(mode != TCPPlain)
	   pf->clp = 0;
	
	if (do_timestamp)		// if timestamp is switched on (RFC1323)
		pf->TCPtimestamp = tcp_now;
	else	pf->TCPtimestamp = 0;

	// if no RTT measuremnt running and no retransmission (Karn's algorithm):
	// start measurement
	if (rtt == 0 && mode == TCPPlain)
	{	rtseq = pf->TCPseq;
		rtt = 1;	// set rtt = 1 means that this segment is now measured
	}

	// If the retrans timer is off and this is not a retransmission, restart it.
	// In case of a retransmission, it will be started afterwards, see early()
	// ### P10_: shouldn't this be done when really sending the packet ???
	if (active_rt_timer == FALSE && mode != TCPRetrans)
		// for fast retransmit, we have to start the timer (has been stopped, see process_ack())
	{	rto_val = rto_calc;
	//{	rto_val = (tim_typ)(rto_calc * (1000 + my_rand() % 1000) / 1000.0);
		// printf("%s: starting RT timer with rto_val = %d\n", name, rto_val);
		if(SimTime < SimTime + rto_val)
			alarme( &rt_timer, rto_val);
		else	errm1s("%s: want to alarm an event later then maximum SimTime\n", name);
		active_rt_timer = TRUE;
	}

	// add packet at tail of the processing queue
	pf->TCPSendStamp = (tim_typ) mode;	// mark whether retransmission or not
	procq.enqueue(pf);

	// Log processing queue
	procq_max_len = max(procq_max_len, procq.getlen());

	if( !active_procq)
	{	// Delay as though I am processing the packet, then handle the packet
		if(send_state == ContSend)
		{	active_procq = TRUE;
			alarme( &evtProcq, procDelay());
		}
		else	next_send_time = SimTime + procDelay();
	}

}	// end of tcpipsend::queue_pkt()

