#ifndef UCPD_DPM_H
#define UCPD_DPM_H

#include "ucpd/ucpd_policy.h"
#include "ucpd/ucpd_port.h"
#include "ucpd.h"

typedef struct {
  UCPD_PORT_INSTANCE ports[UCPD_PORT_COUNT];
  UCPD_Policy policy;
  bool running_policy;
} UCPD_DPM;

UCPD_Status UCPD_DPM_Init(UCPD_DPM *dpm, UCPD_Policy policy);

/* DPM functions */
void UCPD_DPM_EvaluateCapabilities(UCPD_PORT_Number port_number);

void UCPD_DPM_GetCurrentCapabilities(UCPD_PORT_Number port_number);

void UCPD_DPM_PowerTransitionDefault(UCPD_PORT_Number port_number);

void UCPD_DPM_GetRevision(UCPD_PORT_Number port_number);

// TODO: maybe change later
// The boolean return is to indicate whether the PE should get back the control
// after dpm is done
bool UCPD_DPM_InformMessageReceived(UCPD_PORT_Number port_number);

#endif // UCPD_DPM_H
