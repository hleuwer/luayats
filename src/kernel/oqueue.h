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

#ifndef	_OQUEUE_H_
#define	_OQUEUE_H_

#include "defs.h"

// DESCRIPTION: see data.h - queues for data items 
// Queue with unlimited capacity
//tolua_begin
class	uoqueue {
public:
  inline uoqueue(void){q_len = 0;}
  inline ~uoqueue() {}
  inline void enqueue(root *obj)
  {	
    if (q_len++ == 0)
      q_head.next = q_last = obj;
    else {
      q_last->next = obj;
      q_last = obj;
    }
  }
  inline void enqHead(root *obj)
  {
    if (q_len++ == 0)
      q_head.next = q_last = obj;
    else {
      obj->next = q_head.next;
      q_head.next = obj;
    }
  }
  inline	void	enqTime(root *obj)
  {
    root	*p;
    // do not inc q_len until call of findPrecTime() !
    if ((p = findPrecTime(obj->time)) == NULL)
      // queue empty or new largest time.
      // See remark three lines above.
      enqueue(obj);
    else {
      // it is sure: p can't be the last item
      obj->next = p->next;
      p->next = obj;
      ++q_len;
    }
  }

  inline int enqPrec(root *obj, root *ref)
  {	
    root *p;
    if ((p = findPrec(ref)) == NULL)
      return FALSE;
    ++q_len;
    // sure: p can't be the last item (ref follows)
    p->next = obj;
    obj->next = ref;
    return TRUE;
  }

  inline int enqSuc(root *obj, root *ref)
  {	
    if (isQueued(ref) == FALSE)
      return FALSE;
    // q_len must be at least 1 (since ref was found)
    ++q_len;
    if (ref == q_last)
      {	// ref was the last, add at tail
	q_last->next = obj;
	q_last = obj;
      }
    else {
      // include after ref
      obj->next = ref->next;
      ref->next = obj;
    }
    return TRUE;
  }

  inline root *dequeue(void)
  {
    if (q_len-- == 0)
      {	q_len = 0;
      return NULL;
      }
    root	*obj = q_head.next;
    q_head.next = obj->next;
    return obj;
  }

  inline root *deqTail(void) {return deqThis(q_last);}

  inline root *deqTime(tim_typ tim)
  {	
    root *p, *p2;
    if ((p = findPrecTime(tim)) == NULL)
      return NULL;
    if ((p2 = p->next) == q_last)
      // dequeue from tail
      q_last = p;
    else
      p->next = p2->next;
    --q_len;
    return p2;
  }
	
  inline root *deqThis(root *obj)
  {
    root	*p;
    if ((p = findPrec(obj)) == NULL)
      return NULL;
    if (obj == q_last)
      // dequeue from tail
      q_last = p;
    else	p->next = obj->next;
    --q_len;
    return obj;
  }
		
  inline int isEmpty(void){return q_len == 0;}

  inline int isQueued(root *obj)
  {	
    int	i;
    root	*p = q_head.next;
    for (i = 0; i < q_len; ++i){
      if (p == obj)
	return TRUE;
      else	p = p->next;
    }
    return FALSE;
  }

  inline	int	getlen(void){return q_len;}
  inline	root	*first(void)
  {
    return q_len == 0 ? (root *) 0 : q_head.next;
  }
  inline root *last(void)
  {
    return q_len == 0 ? (root *) 0 : q_last;
  }
  inline root *sucOf(root *obj)
  {
    if ( !isQueued(obj) || obj == q_last)
      return NULL;
    return obj->next;
  }
  inline	root	*precOf(root *obj)
  {	
    root *p2;
    if ((p2 = findPrec(obj)) == NULL || p2 == &q_head)
      return NULL;
    return p2;
  }
  
  inline int resCursor()
  {
    if (q_len == 0)
      return FALSE;
    iterCursor = &q_head;
    return TRUE;
  }

  inline	root	*getNext()
  {
    if (iterCursor == q_last)
      return NULL;
    iterCursor = iterCursor->next;
    return iterCursor;
  }
//tolua_end
protected:
  //	ATTENTION:
  //	These methods return the pointer to q_head if the desired
  //	item is the first of the queue.	So do not use from outside.
  inline	root	*findPrec(root *obj)
  {
    int	i;
    root *p = &q_head;
    for (i = 0; i < q_len; ++i){	
      if (p->next == obj)
	return p;
      else	p = p->next;
    }
    return NULL;
  }
  
  inline root *findPrecTime(tim_typ tim)
  {
    int	i;
    root	*p = &q_head;
    for (i = 0; i < q_len; ++i){	
      if (p->next->time >= tim)
	return p;
      else
	p = p->next;
    }
    return NULL;
  }

//	the current length is not private, since it is sometimes exported for displaying.
//tolua_begin
public:
  int dummy;
  int	q_len;
  //tolua_end
protected:
  root	*q_last;
  root	q_head;
  root	*iterCursor;	// for getNext()
};//tolua_export

/***************************************************************************************
*
*	Queue with limited capacity
*
***************************************************************************************/
//tolua_begin
class	oqueue: public uoqueue	{
public:
  inline oqueue(int mx = 0)
  {	
    q_max = mx;
    if (q_max < 0)	// to set unlimited mode, use unlimit() or use uqueue
      q_max = 0;
  }
  inline ~oqueue(){};

  inline int setmax(int	mx)
  {
    if (mx < q_len)
      return FALSE;
    q_max = mx;
    return TRUE;
  }
  inline void unlimit(void){q_max = -1;}

  inline int enqueue(root *obj)
  {
    if (q_len == q_max)
      return FALSE;
    if (q_len++ == 0)
      q_head.next = q_last = obj;
    else {
      q_last->next = obj;
      q_last = obj;
    }
    return TRUE;
  }
	
  inline int enqHead(root *obj)
  {	
    if (q_len == q_max)
      return FALSE;
    if (q_len++ == 0)
      q_head.next = q_last = obj;
    else {
      obj->next = q_head.next;
      q_head.next = obj;
    }
    return TRUE;
  }
	
  inline int enqTime(root *obj)
  {
    if (q_len == q_max)
      return FALSE;
    uoqueue::enqTime(obj);
    return TRUE;
  }
		
  inline int enqPrec(root *obj, root *ref)
  {	
    if (q_len == q_max)
      return FALSE;
    return uoqueue::enqPrec(obj, ref);
  }
  inline int enqSuc(root *obj, root *ref)
  {	
    if (q_len == q_max)
      return FALSE;
    return uoqueue::enqSuc(obj, ref);
  }
  
  inline int isFull(void){return q_len == q_max;}
  inline int getmax(void){return q_max;}
  //tolua_end
protected:
  int	q_max;
}; //tolua_export
#endif
