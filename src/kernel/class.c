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
*	Module author:		Matthias Baumann, TUD
*	Creation:		1996
*
*************************************************************************/

/*
*	Definition of network object classes
*/

#include "defs.h"

#define	BAUST(name, creat)	{\
		extern	root	*creat(void);\
		extern	void	usr_class(char *, root *(*)(void));\
		usr_class((char *)name, creat);}
#ifdef	USERCLASS
#undef	USERCLASS
#endif
#define	USERCLASS(xxx, yyy)	BAUST(xxx,yyy)

/*
*	Here to add new classes (this is the only place besides the makefile!)
*/
void	user_classes(void)
{
	BAUST("ValArray", ValArray);
	BAUST("MacroShell", MacroShell);
	BAUST("ConfidObj", ConfidObj);

	BAUST("CBRquelle", Cbrsrc);
	BAUST("GEOquelle", Geosrc);
	BAUST("BSquelle", Bssrc);
	BAUST("MMBPquelle", Mmbpsrc);
	BAUST("ModBP", ModBP);
	BAUST("GMDPquelle", Gmdpsrc);
	BAUST("xx2", Xx2src);
	BAUST("Filsrc", Filsrc);
	BAUST("ListSrc", Listsrc);
	BAUST("Senke", Sink);
	BAUST("SinkTrace", SinkTrace);
	BAUST("Multiplexer", Mux);
	BAUST("MuxAF", MuxAF);
	BAUST("MuxDF", MuxDF);
	BAUST("MuxDist", MuxDist);
	BAUST("Demultiplexer", Demux);
	BAUST("Signal", Sig);
	BAUST("Messung", Meas);
	BAUST("Meas2", Meas2);
	BAUST("Meas3", Meas3);
	BAUST("Leitung", Line);
	BAUST("Shaper", Shaper);
	BAUST("Shaper2", Shaper2);
	BAUST("ShapCtrl", Shapctrl);
	BAUST("LeakyBucket", LeakyB);
	BAUST("IP2ATM", Ip2atm);

	BAUST("TypeCheck", Typecheck);
	BAUST("TimeStamp", Timestamp);
	BAUST("DummyObj", DummyObj);

	BAUST("Distribution", Distrib);
	BAUST("DistSrc", Distsrc);

	BAUST("GmdpStop", Gmdpstop);

	BAUST("xx2sink", Xx2sink);
#ifdef USE_GRAPHICS
	BAUST("Meter", Meter);
	BAUST("Histogram", Histo);
	BAUST("Histo2", Histo2);
	BAUST("Control", Control);
#endif
	BAUST("AbrSrc", AbrSrc);
	BAUST("AbrMux", AbrMux);
	BAUST("AbrSink", AbrSink);

	BAUST("AAL5Send", Aal5send);
	BAUST("AAL5Rec", Aal5rec);
	BAUST("AAL5RecMult", Aal5recMult);

	BAUST("CBRFrame", Cbrframe);
	BAUST("TCPIPsend", Tcpipsend);
	BAUST("TCPIPrec", Tcpiprec);
	BAUST("Data2Frame", Data2Frame);
	BAUST("TermStartStop", TermStartStop);

	BAUST("MuxEPD", MuxEPD);
	BAUST("MuxWFQ", MuxWFQ);

	BAUST("MuxSyncDF", MuxSyncDF);
	BAUST("MuxSyncAF", MuxSyncAF);
	BAUST("MuxAsyncDF", MuxAsyncDF);
	BAUST("MuxAsyncAF", MuxAsyncAF);
	BAUST("MuxEvtEPD", MuxEvtEPD);
	BAUST("MuxPrio", MuxPrio);
	BAUST("MuxInpBuf", MuxInpBuf);

	// DO NOT DELETE THIS LABEL (used by make config).
	// ALL LINES BELOW THIS LABEL ARE OVERRIDDEN BY make config.
	//@MAKE_CONFIG@//
	// included by make config:
// ../src/user/aal5sendprio.c:
USERCLASS("AAL5PrioSend", Aal5sendprio);
// ../src/user/aal5sred.c:
USERCLASS("AAL5SRED", Aal5sred);
// ../src/user/agere_tm.c:
USERCLASS("AgereTM",Ageretm);
// ../src/user/clpmux.c:
USERCLASS("MuxCLP", MuxCLP);
// ../src/user/d2frame2.c:
USERCLASS("Data2Frame2", Data2Frame2);
// ../src/user/data2frs.c:
USERCLASS("Data2frs", Data2frs);
// ../src/user/dgcra_sh.c:
USERCLASS("DGCRAShaper",Dgcra_Shaper);
// ../src/user/distsrc2.c:
USERCLASS("DistSrc2", Distsrc2);
// ../src/user/epdmuxprio.c:
USERCLASS("MuxEPDPrio", MuxEPDPrio);
// ../src/user/ewsxcon.c:
USERCLASS("ContrEWSX", ControlEWSX);
// ../src/user/ewsxmux.c:
USERCLASS("MuxEWSX", MuxEWSX);
// ../src/user/fpgared.c:
USERCLASS("FPGARED",Fpgared);
// ../src/user/framemarker.c:
USERCLASS("FrameMarker",FrameMarker);
// ../src/user/framrecv.c:
USERCLASS("FrameRecv", Framerecv);
// ../src/user/framsend.c:
USERCLASS("FrameSend", Framesend);
// ../src/user/int_pol.c:
USERCLASS("IntPol",Intpol);
// ../src/user/iwumark.c:
USERCLASS("IWUMARK",Iwumark);
// ../src/user/iwumark_lb.c:
USERCLASS("IWUMARK_LB",Iwumark_lb);
// ../src/user/iwuubr.c:
USERCLASS("IWUUBR",Iwuubr);
// ../src/user/iwuvbr3.c:
USERCLASS("IWUVBR3",Iwuvbr3);
// ../src/user/lb_atm.c:
USERCLASS("LeakyBucket_ATMF",LeakyB_ATMF);
// ../src/user/lbframe.c:
USERCLASS("LeakyBucketFrame",LeakyBucketFrame);
// ../src/user/leakybucket.c:
// ../src/user/lossclp1.c:
USERCLASS("LossCLP1",Lossclp1);
// ../src/user/marker.c:
USERCLASS("Marker",Marker);
// ../src/user/measframe.c:
USERCLASS("MeasFrame", Measframe);
// ../src/user/muxpacket.c:
USERCLASS("MuxPacket", Muxpacket);
// ../src/user/muxwfqbuffman.c:
USERCLASS("MuxWFQBuffMan", MuxWFQBuffMan);
// ../src/user/redmux.c:
USERCLASS("MuxRED", MuxRED);
// ../src/user/setTrace.c:
USERCLASS("SetTrace", SetTrace);
// ../src/user/stdred.c:
USERCLASS("STDRED",Stdred);
// ../src/user/tcpiplfnsend.c:
USERCLASS("TCPIPLfnSend", Tcpiplfnsend);
// ../src/user/tcpipsendprio.c:
USERCLASS("TCPIPPRIOsend", Tcpipsendprio);
// ../src/user/vbrframe.c:
USERCLASS("VBRFrame", Vbrframe);
// ../src/user/websource.c:
USERCLASS("WebSrc", Websrc);
}
