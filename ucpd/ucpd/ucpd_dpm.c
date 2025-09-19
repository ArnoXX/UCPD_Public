#include "ucpd/ucpd_dpm.h"
#include "stm32g0xx_hal.h"
#include "ucpd_ctx.h"

void UCPD_DPM_EvaluateCapabilities(UCPD_PORT_Number port_number) {
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  port->running_policy = UCPD_TRUE;

  dpm->policy(port_number, UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES);

  port->running_policy = UCPD_FALSE;
}

void UCPD_DPM_GetCurrentCapabilities(UCPD_PORT_Number port_number) {
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  port->running_policy = UCPD_TRUE;

  dpm->policy(port_number, UCPD_POLICY_EVENT_GET_CURRENT_CAPABILITIES);

  port->running_policy = UCPD_FALSE;
}

void UCPD_DPM_Init(UCPD_DPM *dpm, UCPD_Policy policy) {
  dpm->policy = policy;
}

void UCPD_DPM_PowerTransitionDefault(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  port->running_policy = UCPD_TRUE;

  dpm->policy(port_number, UCPD_POLICY_EVENT_POWER_TRANSITION_DEFAULT);

  port->running_policy = UCPD_FALSE;
}

void UCPD_DPM_GetRevision(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  port->running_policy = UCPD_TRUE;
  dpm->policy(port_number, UCPD_POLICY_EVENT_GET_REVISION);

  port->running_policy = UCPD_FALSE;
}

// TODO: maybe change later
// The boolean return is to indicate whether the PE should get back the control
// after dpm is done
UCPD_Bool UCPD_DPM_InformMessageReceived(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  port->running_policy = UCPD_TRUE;

  dpm->policy(port_number, UCPD_POLICY_EVENT_INFORM_MESSAGE_RECEIVED);

  port->running_policy = UCPD_FALSE;

  return UCPD_TRUE;
}