#ifndef UCPD_TYPEC_H
#define UCPD_TYPEC_H

#include "ucpd/ucpd_port_num.h"

/* Connection detection state machine states */
typedef enum {
  CAD_STATE_UNATTACHED,
  CAD_STATE_ATTACH_WAIT,
  CAD_STATE_ATTACHED,
  CAD_STATE_ERROR_RECOVERY,
  CAD_STATE_NONE
} UCPD_CAD_SM_State;

/* CAD functions */
/* Init the cable detection */
void UCPD_CAD_Init(UCPD_PORT_Number port_number);

/* Start the cable detection */
void UCPD_CAD_Start(UCPD_PORT_Number port_number);

void UCPD_CAD_SM_Enter(UCPD_PORT_Number port_number, UCPD_CAD_SM_State state);

#endif // UCPD_TYPEC_H
