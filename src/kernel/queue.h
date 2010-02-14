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

#ifndef	_QUEUE_H_
#define	_QUEUE_H_

/*
*	Data item queues with unlimited and limited capacity.
*	Methods for FIFO queueing are virtually as fast as programmed "by hand".
*	Methods for time-sorted queueing and random access are not as optimal.
*
*	Unlimited queue:	class	uqueue
*	Limited queue:		class	queue
*				Inherits methods from uqueue and complements them
*				with overflow check.
*/

/*
INITIALISATION
==============
Class uqueue
------------
	uqueue::uqueue(void);
	Constructor. Sets current length to zero.
Class queue
-----------
	queue::queue(int mx = 0);
	Constructor. Sets current length to zero, limit to mx. Default mx: 0.
	Mx < 0 is corrected to 0 (use unlimit() afterwards).
int	queue::setmax(int mx);
	Changes the queue limit to mx.
	Returns	TRUE:	o.k.
		FALSE:	current queue length is larger than mx, limit not changed.
void	queue::unlimit(void);
	Deletes the capacity limitation of a queue. If possible, the class uqueue should
	be used instead.

ENQUEUEING
==========
Class uqueue
------------
void	uqueue::enqueue(data *pd);
	Enqueues the given item at the tail of the queue.
void	uqueue::enqHead(data *pd);
	Enqueues the given item at the head of the queue.
void	uqueue::enqTime(data *pd);
	Enques the item in front of the first item with a time member not smaller
	than pd->time (creates a sorted queue with increasing time stamps). If
	pd->time is larger than all other times, pd is added at the tail.

int	uqueue::enqPrec(data *pd, data *ref);
	Looks for ref and enqueues pd in front of ref.
	Returns	TRUE:	o.k.
		FALSE:	ref not found in the queue
int	uqueue::enqSuc(data *pd, data *ref);
	Looks for ref and enqueues *pd behind ref.
	Returns	TRUE:	o.k.
		FALSE:	ref not found in the queue
Class queue
-----------
All uqueue methods are also available for queue. They first check on overflow and then
call uqueue's method. All methods return int:
	FALSE 	on overflow or unsuccesssfull uqueue method,
	TRUE	otherwise.

DEQUEUEING
==========
Classes uqueue and queue
------------------------
data	*(u)queue::dequeue(void);
	Dequeues and returns the item at the front. An empty queue returns NULL.
data	*(u)queue::deqTail(void);
	Dequeues and returns the item at the tail. An empty queue returns NULL.
data	*(u)queue::deqTime(tim_typ tim);
	Dequeues and returns the first item with a time member not smaller than tim.
	Returns NULL if no appropriate item found.
data	*(u)queue::deqThis(data *pd);
	Dequeues the specified item. Returns pd if found in the queue, otherwise NULL.

INFORMATION
===========
Classes uqueue and queue
------------------------
int	(u)queue::isEmpty(void);
	Returns TRUE if queue is empty, FALSE otherwise.
int	(u)queue::isQueued(data *pd);
	Returns TRUE if *pd is queued, FALSE otherwise.
int	(u)queue::getlen(void);
	Returns the current queue length.

data	*(u)queue::first(void);
	Returns the item which would be returned by queue::deq(), but does
	*not* actually dequeue it. Returns NULL in case the queue was empty.
data	*(u)queue::last(void);
	Returns the last item of the queue. It is *not* dequeued.
	Returns NULL if the queue was empty.

data	*(u)queue::sucOf(data *pd);
	Returns the item behind the specified one. Returns NULL if pd not
	found or pd is the last item of the queue.
data	*(u)queue::precOf(data *pd);
	Returns the item in front of the specified one. Returns NULL if pd not
	found or pd is the first item of the queue.

int	(u)queue::resCursor();
	Initializes the internal cursor for subsequent calls of getNext(). Returns
	TRUE on success, FALSE otherwise (queue empty).
data	*(u)queue::getNext();
	Each call returns the pointer to the next data object queued. Returns NULL,
	if end of queue reached. The first call returns the front of the queue.
	Prior to the first call of getNext(), resCursor() *has to*
	be called. If resCursor() wasn't successful, getNext() *must not* be called.
	Between resCursor() and all subsequent calls of getNext(), no enqueueing
	or dequeueing actions are allowed.

Class queue
-----------
int	queue::isFull(void);
	Returns TRUE if queue is full, FALSE otherwise.
int	queue::getmax(void);
	Returns the current queue limit. In case unlimit() was called in advance,
	-1 is returned.

Private methods
===============
data	*uqueue::findPrec(data *pd);
	Looks for the preceedor of the specified item. If found, the preceedor's
	address is returned. Otherwise NULL.
data	*uqueue::findPrecTime(tim_typ tim);
	Looks for the preceedor of the first item with a time member not smaller than tim.
	If found, the reference is returned. Otherwise NULL.

Implementation details
======================
Implementation is optimized for the limited FIFO queue. Since a q_len counter is maintained
anyway, extra marking of the last item by next == NULL is omitted. On the other hand, unlimited
and sorted queues are probably a little bit slower.
Using a complete data struct as head (from which actually only next is used) eases operations:
the head does not need "special treatment".

*/


/***************************************************************************************
*
*	Queue with unlimited capacity
*
***************************************************************************************/
//tolua_begin
class	uqueue	{
public:
	inline	uqueue(void)
	{	q_len = 0;
	}

	inline	void	enqueue(data *pd)
	{	if (q_len++ == 0)
			q_head.next = q_last = pd;
		else
		{	q_last->next = pd;
			q_last = pd;
		}
	}
	inline	void	enqHead(data *pd)
	{	if (q_len++ == 0)
			q_head.next = q_last = pd;
		else
		{	pd->next = q_head.next;
			q_head.next = pd;
		}
	}
	inline	void	enqTime(data *pd)
	{	data	*p;
		// do not inc q_len until call of findPrecTime() !
		if ((p = findPrecTime(pd->time)) == NULL)
			// queue empty or new largest time.
			// See remark three lines above.
			enqueue(pd);
		else
		{	// it is sure: p can't be the last item
			pd->next = p->next;
			p->next = pd;
			++q_len;
		}
	}

	inline	int	enqPrec(data *pd, data *ref)
	{	data	*p;
		if ((p = findPrec(ref)) == NULL)
			return FALSE;
		++q_len;
		// sure: p can't be the last item (ref follows)
		p->next = pd;
		pd->next = ref;
		return TRUE;
	}
	inline	int	enqSuc(data *pd, data *ref)
	{	if (isQueued(ref) == FALSE)
			return FALSE;
		// q_len must be at least 1 (since ref was found)
		++q_len;
		if (ref == q_last)
		{	// ref was the last, add at tail
			q_last->next = pd;
			q_last = pd;
		}
		else
		{	// include after ref
			pd->next = ref->next;
			ref->next = pd;
		}
		return TRUE;
	}

	inline	data	*dequeue(void)
	{	if (q_len-- == 0)
		{	q_len = 0;
			return NULL;
		}
		data	*pd = q_head.next;
		q_head.next = pd->next;
		return pd;
	}
	inline	data	*deqTail(void)
	{	return deqThis(q_last);
	}
	inline	data	*deqTime(tim_typ tim)
	{	data	*p, *p2;
		if ((p = findPrecTime(tim)) == NULL)
			return NULL;
		if ((p2 = p->next) == q_last)
			// dequeue from tail
			q_last = p;
		else	p->next = p2->next;
		--q_len;
		return p2;
	}
	inline	data	*deqThis(data *pd)
	{	data	*p;
		if ((p = findPrec(pd)) == NULL)
			return NULL;
		if (pd == q_last)
			// dequeue from tail
			q_last = p;
		else	p->next = pd->next;
		--q_len;
		return pd;
	}
		
	inline	int	isEmpty(void)
	{	return q_len == 0;
	}
	inline	int	isQueued(data *pd)
	{	int	i;
		data	*p = q_head.next;
		for (i = 0; i < q_len; ++i)
		{	if (p == pd)
				return TRUE;
			else	p = p->next;
		}
		return FALSE;
	}

	inline	int	getlen(void)
	{	return q_len;
	}
	inline	data	*first(void)
	{	return q_len == 0 ? (data *) 0 : q_head.next;
	}
	inline	data	*last(void)
	{	return q_len == 0 ? (data *) 0 : q_last;
	}
	inline	data	*sucOf(data *pd)
	{	if ( !isQueued(pd) || pd == q_last)
			return NULL;
		return pd->next;
	}
	inline	data	*precOf(data *pd)
	{	data	*p2;
		if ((p2 = findPrec(pd)) == NULL || p2 == &q_head)
			return NULL;
		return p2;
	}

	inline	int	resCursor()
	{	if (q_len == 0)
			return FALSE;
		iterCursor = &q_head;
		return TRUE;
	}
	inline	data	*getNext()
	{	if (iterCursor == q_last)
			return NULL;
		iterCursor = iterCursor->next;
		return iterCursor;
	}
//tolua_end
protected:
	//	ATTENTION:
	//	These methods return the pointer to q_head if the desired
	//	item is the first of the queue.	So do not use from outside.
	inline	data	*findPrec(data *pd)
	{	int	i;
		data	*p = &q_head;
		for (i = 0; i < q_len; ++i)
		{	if (p->next == pd)
				return p;
			else	p = p->next;
		}
		return NULL;
	}
	inline	data	*findPrecTime(tim_typ tim)
	{	int	i;
		data	*p = &q_head;
		for (i = 0; i < q_len; ++i)
		{	if (p->next->time >= tim)
				return p;
			else	p = p->next;
		}
		return NULL;
	}

//	the current length is not private, since it is sometimes exported for displaying.
//tolua_begin
public:
	int	q_len;
	//tolua_end
protected:
	data	*q_last;
	data	q_head;
	data	*iterCursor;	// for getNext()
};//tolua_export

/***************************************************************************************
*
*	Queue with limited capacity
*
***************************************************************************************/
//tolua_begin
class	queue: public uqueue	{
public:
	inline	queue(int mx = 0)
	{	q_max = mx;
		if (q_max < 0)	// to set unlimited mode, use unlimit() or use uqueue
			q_max = 0;
	}
	inline	int	setmax(int	mx)
	{	if (mx < q_len)
			return FALSE;
		q_max = mx;
		return TRUE;
	}
	inline	void	unlimit(void)
	{	q_max = -1;
	}

	inline	int	enqueue(data *pd)
	{	if (q_len == q_max)
			return FALSE;
		if (q_len++ == 0)
			q_head.next = q_last = pd;
		else
		{	q_last->next = pd;
			q_last = pd;
		}
		return TRUE;
	}
	inline	int	enqHead(data *pd)
	{	if (q_len == q_max)
			return FALSE;
		if (q_len++ == 0)
			q_head.next = q_last = pd;
		else
		{	pd->next = q_head.next;
			q_head.next = pd;
		}
		return TRUE;
	}
	inline	int	enqTime(data *pd)
	{	if (q_len == q_max)
			return FALSE;
		uqueue::enqTime(pd);
		return TRUE;
	}
		
	inline	int	enqPrec(data *pd, data *ref)
	{	if (q_len == q_max)
			return FALSE;
		return uqueue::enqPrec(pd, ref);
	}
	inline	int	enqSuc(data *pd, data *ref)
	{	if (q_len == q_max)
			return FALSE;
		return uqueue::enqSuc(pd, ref);
	}

	inline	int	isFull(void)
	{	return q_len == q_max;
	}
	inline	int	getmax(void)
	{	return q_max;
	}
	//tolua_end
protected:
	int	q_max;
}; //tolua_export
#endif
