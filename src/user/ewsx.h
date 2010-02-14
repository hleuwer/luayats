///////////////////////////////////////////////////////////////////////////////
// ewsx.h
// the definition of classes for EWSX
//   - EWSXConnParam - connection parameter
//   - muxEWSX - the EWSX multiplexer
//   - controlEWSX - the EWSX control module
///////////////////////////////////////////////////////////////////////////////

#ifndef   _EWSX_H_
#define   _EWSX_H_

#include "inxout.h"
#include "queue.h"

typedef enum {CBR=1, VBR=2, ABR=3, UBR=4, ABRBRMC=5 } ServiceClassType;


// struct for ABR information (from abrmux)
struct	ABRstruct	{
	double	ER;		// ER we calculated ourselfs
	double	forwER;		// incomming forward ER
	double	backwER;	// incomming backward ER
	double	CCR;
	double	MCR;
	int	active;		// TRUE if cell arrived during current AI
};

///////////////////////////////////////////////////////////////
// Connection Parameter
// the Parameter of a connection in the EWSX-Switch
// is derived from cell in order to be stored in a queue
///////////////////////////////////////////////////////////////
class EWSXConnParam : public cell
{

   public:
   
   EWSXConnParam(int vc): cell(vc) {
      vciOK = TRUE;  	   // no cells are lost in the beginning
      vciFirst = TRUE;	   // first cell is first of a packet
      ServiceClass = UBR;  // Standard is UBR
      qLenVCI = 0;   	      // the queue length
      
      vtime = 0;     	   // virtual time
      vdelta = 0;    	   // virtual delta
      
      epdThreshVCI = 0;	   // no conn-spec.EPD (see connect of ewsx multipl.)
      clpThreshVCI = 0;
      ppd = TRUE;
      ReservedBuff = 0;
		
      AbrInfo = NULL;
      AbrVciData = -1;
   };

   int vciOK;        	// is a cell of the actual frame (not) lost?
   int vciFirst;     	// next cell is the first of a frame
   
   ServiceClassType ServiceClass; // the Serviceclass of this conn.
   
   int qLenVCI;		// number of cells for this connection
   int epdThreshVCI;	// EPD threshold for this connection
   int clpThreshVCI;	// threshold for dropping CLP=1
   int ppd;		// perform PPD?
   uqueue q_vc;		// the queue, where the cells are queued
   
   double vtime;	// the virtual time
   double vdelta;	// the virtual delta of this connection (inverse of the WFQ weight)
   int ReservedBuff;	// the buffer reserved for this connection
	
   ABRstruct* AbrInfo;
   int AbrVciData;	// for the backward RM cells, this is the VCI of data

};




// ABR Parameter
struct AbrParam {

   tim_typ AI_tim;   	   // interval (time slots) for ABR rate measuremet
   tim_typ CVBR_load_tim;  // interval (time slots) for non-ABR rate measuremet

   unsigned cnt_load_abr;  // counter for ABR cell rate measurement
   unsigned cnt_load_cvbr; // counter for non-ABR cell rate measurement

   double Z;         	   // over / underload indication for ERICA
   double Zol;       	   // Z value in case no ABR bandwidth available
   double CVBRRate;  	   // current non-ABR cell rate
   double ABRRate;

   double TargetUtil;	   // target utilistaion of output link
   double LinkRate;  	   // output link rate (cells per second)
   double TargetRate;	   // LinkRate * TargetUtil

   int Nabr;         	   // number of ABR connections
   int Nactive;      	   // # of currently active ABR connections

   int useNactive;   	   // TRUE: compute fair share from Nactive
      	             	   // FALSE: compute it from Nabr
   int binMode;      	   // TRUE: no ER calculations
   int TBE;
   int abr_q_hi;     	   // if crossed upwards, congestion indication
      	             	   // is turned on
   int abr_q_lo;     	   // if crossed downwards, indication turned off
   int abr_congested;	   // TRUE: congestion indication

}; // struct AbrParam


///////////////////////////////////////////////////////////////
// EWSX - Multiplexer
///////////////////////////////////////////////////////////////
class   muxEWSX: public inxout {
typedef   inxout   baseclass;

public:	
   enum
   {
      keyQueue = 1,
      keyLoadABR = 2,
      keyLoadCVBR = 3
   };

   muxEWSX() : evtLate(this, keyQueue),
      	       ABR_load_evt(this, keyLoadABR),
	       CVBR_load_evt(this, keyLoadCVBR)
   {   

      alarmed_early = FALSE;  	 // not alarmed
      alarmed_late = FALSE;   	 // not alarmed
      
      ewsx = NULL;
      pqlen = NULL;
      qlen_local = 0;
      maxbuff = 0;
      epdThreshGlobal = 0;
      clpThreshGlobal = 0;
            
      spacTime = 0;
      
      TimeSendNext = -1.0;
           
      connected = 0;		// i am not connected at the beginning
      CellsPushedOut = 0;
      
      ReservedBuff = 0;		// all buffer available at the beginning
      ReservedBandwidth= 0;	// all bandwidth available at the beginning

      AbrVciLast = -1;
      
   } // constructor
   
   struct inpstruct
   {
      data *pdata;
      int inp;
   };
   
   int ninp;         	// # of inputs
   int max_vci;      	// number of VCI
   
   event evtLate;	// event for the late() method (called if arrival)
   event ABR_load_evt;	// timer for measuring current ABR cell rate
   event CVBR_load_evt;	// timer for measuring current non-ABR cell rate

   uqueue q;		// the sort queue
   uqueue q_abrrm;   	// ABR RM Cells

   unsigned int *lost;	   // one loss counter per input line
   unsigned int *lostVCI;  // loss counter per VC
   unsigned lossTot; 	   // total losses
   
   inpstruct *inp_buff;	   // buffer for cells arriving in the early slot phase
   inpstruct *inp_ptr;	   //current position in inp_buff

   enum {InpBACK = -1};
   enum {SucData = 0, SucBACK = 1};
   
   int alarmed_early;
   int alarmed_late;

   // Connection parameter
   EWSXConnParam **connpar;

   root *ewsx;			// pointer to ewsxcontrol
   int *pqlen;			// pointer to the queue-length
   int qlen_local;   	        // local queue length
   int maxbuff;
   int epdThreshGlobal;   	// when to start dropping frames
   int clpThreshGlobal;		// when to start dropping CLP==1 cells
   int PrioClpEpd;   	        // do not perform EPD for UBR CLP=0 cells
   int epdClp1;      	        // perform EPD for VBR CLP=1 cells
   
   // WFQ-Parameter
   tim_typ spacTime;		// the central WFQ variable "Spacing Time"
   enum	{spacTimeMax = 100000000};   // when to reset spacTime
   
   // Parameter for output bandwidth
   double TimeSendNext;
   double outdelta;

   int connected;
   int CellsPushedOut;
   
   int ReservedBuff;
   double ReservedBandwidth;
   double MaxBandwidth;
	
   // global ABR Param
   AbrParam abr;
   int AbrVciLast;   // last ABR data connection, if a BRMC-connection in the
      	             // backward dir. is setup, this VCI is assumed to be
		     // the data vci
   
   // methods   
   void	init(void);
   inline void	dropItem(inpstruct *);
   rec_typ REC(data*,int);
   void late(event*);
   inline int check_drop_aal5(data*, EWSXConnParam*);
   void early(event *);
   inline time_t calc_alarm_time(void);
   int export(exp_typ *);
   int command(char *,tok_typ *);
   void connect(void);
   void restim(void);
   int pushout(ServiceClassType);
   char* special(specmsg*, char*);
	
   // Methods for ABR
   void	AbrInit(void);
   inline void	AbrCompERICA(ABRstruct *ptr);
   inline void	AbrCompBinary(rmCell	*pc);
   char* AbrEstablConn(ABRConReqMsg*,char *);

};


///////////////////////////////////////////////////////////////
// EWSX - Control Unit
///////////////////////////////////////////////////////////////
class   controlEWSX: public ino {
typedef   ino   baseclass;
public:
   controlEWSX() {}
   void mux_register(muxEWSX *);
   
   void	init();
   //int	command(char *, tok_typ *);
   void	connect() {};
   int export(exp_typ *msg);
   void pushout(void);
   
   muxEWSX** pmux;	// field of pointer to the muxes
   int nmux;		// number of muxes already connected to the control center
   int maxmux;		// maximal number of muxes
   
   int qlen;		// queue-length
   int buff;		// buffer available
   int ReservedBuff;
   int epdThreshGlobal;	// Threshold for performing EPD
   int clpThreshGlobal;	// Threshold for dropping CLP==1 cells

};


#endif   // _EWSX_H_
