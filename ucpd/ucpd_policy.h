#ifndef UCPD_POLICY_H
#define UCPD_POLICY_H

#include "ucpd/ucpd_port_num.h"

typedef enum
{
	UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES,
	UCPD_POLICY_EVENT_GET_CURRENT_CAPABILITIES,
	UCPD_POLICY_EVENT_POWER_TRANSITION_DEFAULT,
	UCPD_POLICY_EVENT_POWER_TRANSITION,
	UCPD_POLICY_EVENT_GET_REVISION,
	UCPD_POLICY_EVENT_INFORM_MESSAGE_RECEIVED,

} UCPD_Policy_Event;

// Policy just responds to events - evaluate capabilities, get revision, etc.
// It must not call for fixed/pps requests or epr enter/exit requests
// Actually it cannot do that since all API calls check if the policy is currently being invoked
// either through global port object of special functions

typedef void (*UCPD_Policy)(UCPD_PORT_Number port_number, UCPD_Policy_Event event);

#endif