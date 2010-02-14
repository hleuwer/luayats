
#ifndef	_AAL5SENDPRIO_H
#define	_AAL5SENDPRIO_H

#include "aal5send.h"

class	aal5sendprio:	public aal5send
{
typedef	aal5send	baseclass;
public:
	void	early(event *);
	int clp;
};

#endif	// _AAL5SENDPRIO_H
