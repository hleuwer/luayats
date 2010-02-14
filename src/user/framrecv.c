/*************************************************************************
*
*     YATS - Yet Another Tiny Simulator
*
**************************************************************************
*
*     Copyright (C) 1995-1997 Chair for Telecommunications
*           Dresden University of Technology
*           D-01062 Dresden
*           Germany
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
*  Module author:    Torsten Mueller, TU-Dresden
*  Creation:         27.06.1997
*  Last Modified:
*
*************************************************************************/

/* for a description see framerecv.txt */

#include "framrecv.h"

CONSTRUCTOR(Framerecv, framerecv)
USERCLASS("FrameRecv", Framerecv);

//////////////////////////////////////////////////////////////////////
//  initialization
//////////////////////////////////////////////////////////////////////
void   framerecv::init(void)
{
   skip(CLASS);
   name = read_id(NULL);
   skip(':');

   input ("Data", InpData);            // DATA input
   input ("Core", InpCore);            // CORE-DATA input
   output("OUT");                      // output
   
   counter = 0;
   byte_to_send = 0;
   alarmed = 0;

} // framerecv::init()

//////////////////////////////////////////////////////////////////////
//  a frame has arrived
//////////////////////////////////////////////////////////////////////
rec_typ   framerecv::REC(data *pd, int key)
{
   frame *pf;

   typecheck(pd, FrameType);
   pf = (frame*) pd;
   
   switch (key)
   {
      case InpCore:
         
         if(pf->pdu_len() <= 0)
            errm1s("%s: Frame with PDU-LEN <= 0 arrived at Input Core", name);
      
         inqueue.enqueue(pd);
         return ContSend;

      case InpData:
         
         if(pf->pdu_len() <= 0)
            errm1s("%s: Frame with PDU-LEN <= 0 arrived at Input Data", name);
         
         byte_to_send += pf->pdu_len();

         if(byte_to_send < 0)
            errm1s("%s: overflow of byte_to_send: too much bytes in queue", name);
         
         delete pd;
         
         if(!alarmed)
         {
            alarme(&std_evt, 1);
            alarmed = 1;
         }
         
         return ContSend;

      default:
         errm1s("%s: Internal error: Receive data on input with no receive method.", name);
         return ContSend;

   }
}

//////////////////////////////////////////////////////////////////////
//  alarmed in the early phase
//////////////////////////////////////////////////////////////////////
void	framerecv::early(event *)
{
   frame *pf;
   
   pf = (frame*) inqueue.first();
   if(pf == NULL)
   {
      fprintf(stderr, "Simtime=%d, byte_to_send: %d\n",SimTime, byte_to_send);
      errm1s("%s: Internal error: no data in inqueue but order to send", name);
   }
         
   alarmed = 0;
   
   if(byte_to_send >= (int) pf->pdu_len())
   {
      inqueue.dequeue();
      byte_to_send -= pf->pdu_len();
      
      if(byte_to_send < 0)
         errm1s("%s: Internal error: Less than 0 bytes to send", name);
      
      if(suc->rec(pf, shand) != ContSend)
         errm1s("%s: Internal error: sucs did not receive frame", name);
      
      counter++;
   }
   
   if(byte_to_send > 0)
   {
      alarme(&std_evt, 1);
      alarmed = 1;
   }      

}  // framerecv::early()

