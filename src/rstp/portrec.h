/**************************************/

/*kd: Port Receive state machine. s.170 802.1D*/

#ifndef _STP_PORTREC_H__
#define _STP_PORTREC_H__

void STP_portrec_enter_state(STATE_MACH_T* s);
Bool STP_portrec_check_conditions (STATE_MACH_T* s);
char* STP_portrec_get_state_name (int state);

#endif       /* _STP_EDGE_H__ */

