///////////////////////////////////////////////////////////////
// iwuvbr3
// Interworking Unit, which accepts frames and serves them
// according to SCR and PCR.
// Each connection can set its own SCR_i and PCR_i, which are
// met by the Queues for SCR and PCR
// IWUVBR3 muxtcp[i]:
//   NINP=tcpsrc,   	    // number of inputs
//   SLOTLENGTH=SlotLength, // length of 1 slot (seconds)
//   SCR=scr,       	    // net SCR of the ATM connection
//   PCR=pcr,       	    // net PCR of the ATM connection
//   IF=0.001,      	    // (only if USEARATE=1) Increase-Factor unwichtig
//   WQ=0.1,        	    // (only if USEARATE=1) factor for moving exp. aver. unwichtig 
//   PCRTHR=0.90,   	    // (only if USEARATE=1) when to switch back to SCR unwichtig
//   USEARATE=0,    	    // use RATE option or not unwichtig 
//   NALLOWEXCESS=10,	    // threshold for using PCR
//   CLP0SHAP=0,    	    // shape to SCR of the connection unwichtig
//   UIOLI=uioli,   	    // use it or loose it policy
//   MTU=1500,      	    // maximum packet size
//   BUFF=10000000, 	    // buffer size
//   OUT=lossclp1[i];	    // next module
//
//
// Commands:   SetSCR(id,scr): set the SCR of flow, scr=<double> in bit/s
//    	       SetMaxbuff(id,maxbuff): set the max. buffer for flow #id,
//    	          maxbuff=<int> in packets
//      
///////////////////////////////////////////////////////////////
#include "iwuvbr3.h"
#include <math.h>    // for ceil()


CONSTRUCTOR(Iwuvbr3,iwuvbr3);
USERCLASS("IWUVBR3",Iwuvbr3);

const double eps = 1.0;

///////////////////////////////////////////////////////////////
// void IWUConnParam::IWUConnParam() // the constructor
//////////////////////////////////////////////////////////////
IWUConnParam::IWUConnParam(int flow)
{
   SCR = 0.0;
   PCR = 0.0;
   
   ARate = 1.0;      // >1 to avoid infinity
   LastATime = -1;
   
   SendTimeSCR = 0.0;
   SendTimePCR = 0.0;
   LastSendTimePCR = 0;
   lost = 0;

   CHECK(SIScr = new SortItem(this, flow));
   CHECK(SIPcr = new SortItem(this, flow));

} // Constructor


///////////////////////////////////////////////////////////////
// void iwuvbr3::init()
// read in the command line arguments
//////////////////////////////////////////////////////////////
void iwuvbr3::init(void)
{	
   int i;
   
   skip(CLASS);
   name = read_id(NULL);
   skip(':');
   ninp = read_int("NINP");
   if (ninp < 1)
      syntax0("invalid NINP");
   skip(',');
 
   if (test_word("MAXFLOW"))
   {
      max_flow = read_int("MAXFLOW");
      if (max_flow < 1)
         syntax0("invalid MAXFLOW");
      skip(',');
   }
   else
      max_flow = ninp;
 
  if (test_word("SLOTLENGTH"))
   {
      SlotLength = read_double("SLOTLENGTH");
      if(SlotLength <= 0)
         syntax0("SLOTLENGTH must be > 0");
      skip(',');
   }
   else
      SlotLength = 53.0*8.0 / 149.76 / 1e6;	// STM1
   
   SCR = read_double("SCR");
   if (SCR <= 0)
      syntax0("invalid SCR, must be >0");
   skip(',');

   PCR = read_double("PCR");
   if (PCR <= 0 || PCR < SCR)
      syntax0("invalid PCR: must be >0 and >=SCR");
   skip(',');

   if (test_word("IF"))
   {
      IncreaseFactor = read_double("IF");
      if(IncreaseFactor < 0)
         syntax0("IF be >= 0");
      skip(',');
   }
   else
      IncreaseFactor = 0.1;

   if (test_word("WQ"))
   {
      WQ = read_double("WQ");
      if(WQ < 0 || WQ > 1)
         syntax0("WQ must be 0<=WQ<=1");
      skip(',');
   }
   else
      WQ = 0.2;

   if (test_word("PCRTHR"))
   {
      pcrthr = read_double("PCRTHR");
      if(pcrthr < 0 || pcrthr > 1)
         syntax0("PCRTHR must be 0<=PCRTHR<=1");
      skip(',');
   }
   else
      pcrthr = 0.95;

   if (test_word("USEARATE"))
   {
      UseARate = read_int("USEARATE");
      if( UseARate != 0 && UseARate != 1)
         syntax0("USEARATE must be 0 or 1");
      skip(',');
   }
   else
      UseARate = 1;
   
   UseARate = 0;  // this option is not valid anymore!!!

   if (test_word("NALLOWEXCESS"))
   {
      NAllowExcess = read_int("NALLOWEXCESS");
      if( NAllowExcess < 0)
         syntax0("NALLOWEXCESS must be >0");
      skip(',');
   }
   else
      NAllowExcess=2;

   if (test_word("CLP0SHAP"))
   {
      Clp0Shap = read_int("CLP0SHAP");
      if( Clp0Shap != 0 && Clp0Shap != 1)
         syntax0("CLP0SHAP must be 0 or 1");
      skip(',');
   }
   else
      Clp0Shap=0;
   Clp0Shap = 0;  // this option is not valid anymore!!!

   if (test_word("UIOLI"))
   {
      UseItOrLooseIt = read_int("UIOLI");
      if( UseItOrLooseIt != 0 && UseItOrLooseIt != 1)
         syntax0("UIOLI (Use It Or Loose It) must be 0 or 1");
      skip(',');
   }
   else
      UseItOrLooseIt = 1;

   MTU = read_int("MTU");
   if( MTU < 0)
         syntax0("MTU must be > 0");
   skip(',');

   maxbuff = read_int("BUFF");
   if (maxbuff <= 0)
      syntax0("invalid BUFF: must be >0");
   skip(',');

   output("OUT",SucData);
   
   // generate the inputs
   inputs("I",ninp,-1);

   CHECK(inp_buff = new inpstruct[ninp]);
      inp_ptr = inp_buff;

   // the connection parameter
   CHECK(connpar = new IWUConnParam* [max_flow+1]);
   for (i = 0; i <= max_flow; i++)
   {
      CHECK(connpar[i] = new IWUConnParam(i));
      connpar[i]->maxbuff = maxbuff;
   }

    eache( &evtPCR); // alarm in each TimeSlot (this is very easy to implement)

} // iwuvbr3::init(void)


///////////////////////////////////////////////////////////////
// void iwuvbr3::REC()
// cells are received
//////////////////////////////////////////////////////////////
rec_typ iwuvbr3::REC(data *pd,int i)
{
   int cid;

   typecheck_i(pd, FrameType, i);
   
   frame *pf = (frame *) pd;
   cid = pf->connID;
   if (cid < 0 || cid > max_flow)
      errm1s1d("%s: frame with invalid connID = %d received", name, cid);

   if(pf->frameLen > MTU)
      errm1s2d("%s: frame of length %d arrived, which is larger than MTU=%d", name, pf->frameLen, MTU);

   inp_ptr->inp = i;
   (inp_ptr++)->pdata = pd;
   if (!alarmed_late)
   {
      alarmed_late = TRUE;
      alarml( &std_evt, 0);
   }
   
   return ContSend;
   
} // iwuvbr3::REC()


///////////////////////////////////////////////////////////////
// void iwuvbr3::late()
//////////////////////////////////////////////////////////////
void   iwuvbr3::late(event *evt)
{

   frame *pf;
   int n;
   int cid, len;
   IWUConnParam *cpar;
   inpstruct   *p;
   int todrop;

   alarmed_late = FALSE;	// I am not alarmed anymore

   n = inp_ptr - inp_buff;	// number of cells to serve
   while (n > 0)
   {	
      if (n > 1)
         p = inp_buff + my_rand() % n;
      else
         p = inp_buff;
         
      pf = (frame *) p->pdata;
      cid = pf->connID;    // the check, if this ID is valid is done in REC()
      len = pf->frameLen;
      cpar = connpar[cid];
      
      cpar->PCR = PCR;  // is not used anymore, but we set it - how knows!

      //////////////////////////////////////////////////
      // check, if cells have to be dropped
      //////////////////////////////////////////////////
      if(buff < maxbuff)
         todrop = 0;
      else
         todrop = 1;

      // check local queue
      if(cpar->q_data.getlen() > cpar->maxbuff)
         todrop = 1;


      if(todrop)
         dropItem(p);
      else
      {
         // if the Data-Queue is empty, this flow is not in the sort queues
         if (cpar->q_data.isEmpty())
         {
	    // use it or loose it policy
	    if(UseItOrLooseIt)
	       if(cpar->SendTimeSCR < SimTime)
	          cpar->SendTimeSCR = SimTime;
	    
	    // Enqueue in the SCR Queue
	    cpar->SIScr->time = (time_t) ceil(cpar->SendTimeSCR);
            QSortScr.enqTime(cpar->SIScr);
	    
         } // - if - no data in queue

// 	 if(cpar->q_data.getlen() >= NAllowExcess -1 )
// 	 {
// 	    // now I have to enqueue int the PCR queue again,
// 	    // because PCR may be changed
// 	    QSortPcr.deqThis(cpar->SIPcr);
// 	    cpar->SendTimePCR = cpar->LastSendTimePCR + len * 8.0 /
// 	       cpar->PCR / SlotLength;
// 	    if(cpar->SendTimePCR < SimTime)
// 	       cpar->SendTimePCR = SimTime;	 // use it or loose it
//       	    cpar->SIPcr->time = (time_t) ceil(cpar->SendTimePCR);
// 	    QSortPcr.enqTime(cpar->SIPcr);   // enqueue again in sort queue
// 	       
// 	 } // else - there are data in the queue
	 
	 // if the flow is not in the queue, but would be after enqueuing
	 if(cpar->q_data.getlen() == NAllowExcess -1 )
	 {
	    QSortPcr.deqThis(cpar->SIPcr);   // this should not be necessary
	    QSortPcr.enqueue(cpar->SIPcr);   // enqueue in the PCR queue
	       
	 }



	 // now enqueue the frame
	 cpar->q_data.enqueue(pf);
	 buff++;
 
      } // else - not to drop

      *p = inp_buff[ --n];
      
   } // while(n>0) - as long as there are frames

   inp_ptr = inp_buff;

} // iwuvbr3::late(event *)


///////////////////////////////////////////////////////////////
// void iwuvbr3::early()
// look in the softqueues, send cells and alarm again with
// PCR
//////////////////////////////////////////////////////////////
void iwuvbr3::early(event *evt)
{
   IWUConnParam *cpar;
   frame *pf;
   int cid;
   int len;
   SortItem *SItem;
   int sendclp;


   if(SimTime < TimeSendNextPCR) // not the time to send because of PCR
      return;

   // first, I have to decide, if to send CLP=0 or CLP=1
   if(SimTime >= TimeSendNextSCR)
   {
      sendclp = 0;
      SItem = (SortItem*) QSortScr.first();
      if(!SItem)
         sendclp = 1;
   
   }  // if - allowed to send CLP=0
   else
   {
      sendclp = 1;
   } // else - must send CLP=1


   ////////////////////////////////////////////////////////////////
   // now it is checked, if I can send with CLP=0 or 1, now send
   if(sendclp == 0)  // can send with clp=0
   {
      SItem = (SortItem*) QSortScr.dequeue();
      cpar = SItem->cpar;
   }
   else // must send with clp = 1
   {
      SItem = (SortItem*) QSortPcr.first();
      if(!SItem)  // nothing in the queue
         return;

      // not for PCR!!!
      //if(SItem->time > SimTime)  // this is not the time to send for this item
      //   return;

      // now it is sure, that I can send with CLP=1
      SItem = (SortItem*) QSortPcr.dequeue();
      cpar = SItem->cpar;
 
   } // else - must send with CLP=1


   // take a packet from data queue and send it
   pf = (frame*) cpar->q_data.dequeue();
   if(!pf)
      errm1s("%s: early(): internal Problem 3: cant dequeue item from \
              connection queue", name);

   cid = pf->connID;
   len = pf->frameLen;
   pf->clp = sendclp;
   sucs[SucData]->rec(pf, shands[SucData]);  // send it
   buff--;

   // now, due to PCR, the output cannot send some time
   TimeSendNextPCR = TimeSendNextPCR + len * 8.0 / PCR / SlotLength;
   
   // if we have sent with CLP=0 we cannot send CLP=0 some time
   if(sendclp == 0)
      TimeSendNextSCR = TimeSendNextSCR + len * 8.0 / SCR / SlotLength;

   // we cannot accumulate SCR sendings
   if(TimeSendNextSCR < SimTime)
      TimeSendNextSCR = SimTime;

   // added Mue 21.6.00
   // we cannot accumulate PCR sendings
   if(TimeSendNextPCR < SimTime)
      TimeSendNextPCR = SimTime;
   
   // if the data queue is not yet empty: enqueue it again
   if(sendclp == 0)
   {
      cpar->SendTimeSCR += len * 8.0 / cpar->SCR / SlotLength;
      if(UseItOrLooseIt)
      	 if(cpar->SendTimeSCR < SimTime)
            cpar->SendTimeSCR = SimTime;  // use it or loose it
      cpar->SIScr->time = (time_t) ceil(cpar->SendTimeSCR);
      if (!cpar->q_data.isEmpty())
         QSortScr.enqTime(cpar->SIScr);   // enqueue again in sort queue
      
   }
   else
   {
//       cpar->LastSendTimePCR = SimTime; // recognize last sending time
//       cpar->SendTimePCR += len * 8.0 / PCR / SlotLength;
//       if(cpar->SendTimePCR < SimTime)
//          cpar->SendTimePCR = SimTime;  // use it or loose it
//       cpar->SIPcr->time = (time_t) ceil(cpar->SendTimePCR);
//       if (!cpar->q_data.isEmpty())
//          QSortPcr.enqTime(cpar->SIPcr);   // enqueue again in sort queue

      QSortPcr.enqueue(cpar->SIPcr);   // enqueue again in PCR queue at the end
	 
   }

   // if there are no data, take the flow out of both sort queues
   if (cpar->q_data.isEmpty())
   {
      QSortScr.deqThis(cpar->SIScr);
      QSortPcr.deqThis(cpar->SIPcr);
      
   } // if - no data in queue
   
   // if there are only 2 packets for this flow, then always use the SCR queue
   if(cpar->q_data.getlen() < NAllowExcess)
      QSortPcr.deqThis(cpar->SIPcr);
   

   return;

} // iwuvbr3::early()


//////////////////////////////////////////////////////////////
// int iwuvbr3::export()
//////////////////////////////////////////////////////////////
int iwuvbr3::export(exp_typ   *msg)
{
   int ret = FALSE;
   int i;

   ret = baseclass::export(msg) ||
      intScalar(msg, "QLen", (int*) &buff);
   
   
   if(ret)
      return ret;


   for(i=0;i<=max_flow;i++)
   {
      char str[20];

      sprintf(str,"PCR_%d",i);
      ret = ret || doubleScalar(msg, str, & connpar[i]->PCR);
      if(ret)
         return ret;

      sprintf(str,"ARate_%d",i);
      ret = ret || doubleScalar(msg, str, & connpar[i]->ARate);
      if(ret)
         return ret;

   }

   return ret;





      
} // iwuvbr3::export()


//////////////////////////////////////////////////////////////
// int iwuvbr3::command()
/////////////////////////////////////////////////////////////
int iwuvbr3::command(char *s,tok_typ *pv)
{

   int cid;
   
   if (baseclass::command(s, pv))
      return TRUE;
   
   pv->tok = NILVAR;

   if(strcmp(s, "SetSCR") == 0)
   {
      if(connected)
         syntax0("Using this command is not allowed after connecting");
      
      skip('(');
      cid = read_int(NULL);
      if (cid < 0 || cid > max_flow)
         syntax1d("connID %d is out-of-range", cid);
		
      skip(',');

      if(connpar[cid]->SCR != 0)
         syntax1d("SCR of flow %d already set",cid);

      if ((connpar[cid]->SCR = read_double(NULL)) <= 0)
         syntax0("invalid SCR value");
      SCRReserved += connpar[cid]->SCR;
      
      if(SCRReserved > SCR)
         syntax0("in summary asked for more Bandwidth than available");

      skip(')');
   }
   else if(strcmp(s, "SetPCR") == 0)
   {
      if(connected)
         syntax0("Using this command is not allowed after connecting");
      
      skip('(');
      cid = read_int(NULL);
      if (cid < 0 || cid > max_flow)
         syntax1d("connID %d is out-of-range", cid);
		
      skip(',');

      if(connpar[cid]->PCR != 0)
         syntax1d("PCR of flow %d already set",cid);

      if ((connpar[cid]->PCR = read_double(NULL)) <= 0)
         syntax0("invalid PCR value");
      if (connpar[cid]->PCR > PCR)
         syntax1d("PCR of flow %d is >PCR of VBR connection",cid);
      
      skip(')');
   }
   else if(strcmp(s, "SetMaxbuff") == 0)
   {
      skip('(');
      cid = read_int(NULL);
      if (cid < 0 || cid > max_flow)
         syntax1d("connID %d is out-of-range", cid);
		
      skip(',');

      if ((connpar[cid]->maxbuff = read_int(NULL)) <= 0)
         syntax0("Maxbuff must be > 0");
      if (connpar[cid]->maxbuff > maxbuff)
         syntax1d("max. buffer of %d is >maxbuff of the IWU",cid);
      
      skip(')');
   }
   else
      return FALSE;

   return TRUE;
   
} // iwuvbr3::command()


//////////////////////////////////////////////////////////////
// iwuvbr3::connect(void)
//////////////////////////////////////////////////////////////
void iwuvbr3::connect(void)
{
   int i;

   baseclass::connect();

   // checking of connection specific EPD and CLP=1 thresholds
   for(i=1;i <= max_flow; i++)
   {
      if(connpar[i]->SCR <= 0)
         errm1s1d("%s: SCR of flow %d not set", name, i);

      if(connpar[i]->PCR <= 0)
         errm1s1d("%s: PCR of flow %d not set", name, i);

   } // for - every flow

   connected = 1;	// to show, that i am connected now

} // muxEWSX::connect()


///////////////////////////////////////////////////////////////
// iwuvbr3::dropItem(inpstruct *p)
//	register a loss and delete the data item
//////////////////////////////////////////////////////////////
inline void iwuvbr3::dropItem(inpstruct *p)
{
   //if ( ++lossTot == 0)
   //	   errm1s("%s: overflow of LossTot", name);

   // lossTot will always overflow first, that is why the
   // the others are not checked against overflow
   //++lost[p->inp];

   int cid;
   cid = ((frame *) p->pdata)->connID;
   connpar[cid]->lost++;

   delete p->pdata;

} // void iwuvbr3::dropItem()
