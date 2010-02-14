/*
*	Tested for class queue:
*		- empty queue,
*		- queue with one item,
*		- queue with 5 items: beginning, middle and end of queue.
*	enqueue()
*	enqHead()
*	enqTime()
*	enqPrec()
*	enqSuc()
*
*	dequeue()
*	deqTail()
*	deqTime()
*	deqThis()
*
*	isEmpty()
*	isFull()
*	isQueued()
*	first()
*	last()
*
*	sucOf()
*	precOf()
*/

#include <stdio.h>
#define	TRUE	1
#define	FALSE	0

typedef	int	tim_typ;

class	data	{
public:
		data(int t = 0)	{time = t;}

	tim_typ	time;
	data	*next;
};

#include "queue.h"

main()
{
	queue	q(10);
	int	i;
	data	*pd[50];

	for (i = 0; i < 5; ++i)
		q.enqueue(pd[i] = new data(2 * (i + 1)));

#if	0
	printf("res = %s\n", q.isFull() == 0 ? "FALSE" : "TRUE");
#else
	data	*p = q.precOf(pd[5]);
	if (p == NULL)
		printf("res = NULL\n");
	else	printf("res = %d\n", p->time);
#endif
	
	q.enqueue(new data(-50));

	printf("len = %d\n", q.getlen());
	while (q.getlen() != 0)
		printf("\t%d", q.dequeue()->time);
	printf("\n");
}
