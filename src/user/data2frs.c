/************************************************************************/
/*									*/
/*				YATS					*/
/*									*/
/*			Yet Another Tiny Simulator			*/
/*									*/
/*	YATS is a simple, but fast discrete-event simulation tool,	*/
/*	tailored for investigations of ATM networks.			*/
/*	It is grown up at the Chair for Telecommunications, Dresden	*/
/*	University of Technology during 1995/1996.			*/
/*	The software is free. Please do not delete this lines and	*/
/*	add your name when changing the code.				*/
/*									*/
/*	Module author:		Torsten MÆller				*/
/*	Creation:		1997					*/
/*									*/
/************************************************************************/

/* for a description see data2frs.txt */

#include "data2frs.h"

data2frs::data2frs()
{
}

data2frs::~data2frs()
{
}


/**************************************************************
* send a cell
* this is done by the event scheduler
* the activation for this time is done by with late 
***************************************************************/
int data2frs::REC(data    *pd,int)
{

   delete(pd);

   if ( ++counter == 0)
   {
      errm1s("%s: overflow of Count", name);
   }   

   // create a frame with a sequence number of length msg->len and sequence number msg->nr
   suc->rec(new frameSeq(framesize,framenr), shand);
   
   
   if( (framenr++) > maxframe)
      framenr = 0;

   return ContSend;

   
}  // data2frs::rec()

//	Transfered to LUA init

/*
void	data2frs::init(void)
{

   skip(CLASS);
   name = read_id(NULL);
   skip(':');
   
   framesize = read_int("FRAMESIZE");
   skip(',');
   maxframe = read_int("MAXSNR");
   skip(',');

   // read additional parameters of derived classes
   addpars();

   output("OUT");
   stdinp();               // input for request cells

   framenr = 0;
   
}  // data2frs::init()
*/
