#ifndef UCPD_TYPEC_H
#define UCPD_TYPEC_H

#include "ucpd/ucpd_port_num.h"

/* Connection detection state machine states */
typedef enum
{
	CAD_STATE_NONE,
	CAD_STATE_UNATTACHED,
	CAD_STATE_ATTACH_WAIT,
	CAD_STATE_ATTACHED,
	CAD_STATE_ERROR_RECOVERY
} UCPD_CAD_SM_State;

/* CAD functions */

/* Start the cable detection */
void UCPD_CAD_Start(UCPD_PORT_Number port_number);

#endif // UCPD_TYPEC_H
