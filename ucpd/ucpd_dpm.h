#ifndef UCPD_DPM_H
#define UCPD_DPM_H

#include "ucpd.h"
#include "ucpd/ucpd_policy.h"
#include "ucpd/ucpd_port.h"


typedef struct
{
	UCPD_PORT_INSTANCE ports[UCPD_PORT_COUNT];
	UCPD_Policy policy;
	UCPD_Callback port_callback;

	void* user_data;

	volatile bool running_policy[UCPD_PORT_COUNT];
} UCPD_DPM;

UCPD_Status UCPD_DPM_Init(UCPD_DPM* dpm, UCPD_Config* config);

UCPD_Status UCPD_DPM_Deinit(UCPD_DPM* dpm);

/* DPM functions */
void UCPD_DPM_EvaluateCapabilities(UCPD_PORT_Number port_number);

void UCPD_DPM_GetCurrentCapabilities(UCPD_PORT_Number port_number);

void UCPD_DPM_PowerTransitionDefault(UCPD_PORT_Number port_number);

void UCPD_DPM_InformPowerTransition(UCPD_PORT_Number port_number);

void UCPD_DPM_GetRevision(UCPD_PORT_Number port_number);

void UCPD_DPM_InformMessageReceived(UCPD_PORT_Number port_number);

#endif // UCPD_DPM_H
