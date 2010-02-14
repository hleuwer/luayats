#ifndef	_AAL5SRED_H
#define	_AAL5SRED_H

#include "inxout.h"
#include "queue.h"
#include "red.h"




class	aal5sred:	public inxout
{
typedef	inxout	baseclass;
public:
	void	init(void);
	void	early(event *);
	rec_typ REC(data *, int);	// REC is a macro normally expanding to rec (for debugging)

	int	command(char *, tok_typ *);
	int	export(exp_typ *);

	void	restim(void);	// resets the time dependent values

	queue	q;		// input queue
	int	q_start;	// queue length at which to wake up the sender

	int	fixVCI;		// TRUE: do always use the VCI specified in definition statement
	int     new_cid; 	// the new CID
	int	maxcid;		// range of layer-4 connection IDs
	int	*translationTabVCI;// translation connection ID -> VCI
				// NULL: copy connection ID of packets into VCI of cells
	int	*translationTabCID;// translation connection ID -> new ID

				

	int	*cellSeqTab, *curCellSeq;	// cell sequence numbers (one for each connection)
	int	*sduSeqTab, *curSduSeq;		// SDU seq no

	size_t	sdu_cnt;	// SDUs sent
	size_t	del_cnt;	// counter of not transmitted frames because of not minded StopSend

	int	flen;		// number of bytes still to send for the current frame
	int	first_seq;	// first cell sequence number of an AAL-PDU:
				// is retransmitted in last cell
	
	rec_typ	prec_state;	// state of the preceeding object
	rec_typ	send_state;	// state of this object

	int     addHeader;      // length of additional header (e.g LLC/SNAP)

	enum	{SucData = 0, SucCtrl = 1};
	enum	{InpData = 0, InpStart = 1};
	
	tim_typ	last_tim;	// when sent last (only a check to prevent sending twice in a slot)
	
	redclass red;
	
};

#endif	// _AAL5SRED_H
