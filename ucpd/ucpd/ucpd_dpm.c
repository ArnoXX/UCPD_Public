#include "ucpd/ucpd_dpm.h"
#include "ucpd.h"
#include "ucpd_ctx.h"

void UCPD_DPM_EvaluateCapabilities(UCPD_PORT_Number port_number) {
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  dpm->running_policy = true;

  dpm->policy(port_number, UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES);

  dpm->running_policy = false;
}

void UCPD_DPM_GetCurrentCapabilities(UCPD_PORT_Number port_number) {
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  dpm->running_policy = true;

  dpm->policy(port_number, UCPD_POLICY_EVENT_GET_CURRENT_CAPABILITIES);

  dpm->running_policy = false;
}

UCPD_Status UCPD_DPM_Init(UCPD_DPM *dpm, UCPD_Policy policy) {

  for (UCPD_PORT_Number port_number = 0; port_number < UCPD_PORT_COUNT; port_number++) {
    UCPD_Status status = UCPD_PORT_Init(port_number);
    if (status != UCPD_STATUS_SUCCESS) {
      return status;
    }
  }

  dpm->policy = policy;

  return UCPD_STATUS_SUCCESS;
}

void UCPD_DPM_PowerTransitionDefault(UCPD_PORT_Number port_number) {
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  dpm->running_policy = true;

  dpm->policy(port_number, UCPD_POLICY_EVENT_POWER_TRANSITION_DEFAULT);

  dpm->running_policy = false;
}

void UCPD_DPM_GetRevision(UCPD_PORT_Number port_number) {
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  dpm->running_policy = true;
  dpm->policy(port_number, UCPD_POLICY_EVENT_GET_REVISION);

  dpm->running_policy = false;
}

// TODO: maybe change later
// The boolean return is to indicate whether the PE should get back the control
// after dpm is done
bool UCPD_DPM_InformMessageReceived(UCPD_PORT_Number port_number) {
  UCPD_DPM *dpm = UCPD_CTX_GetDPM();

  dpm->running_policy = true;

  dpm->policy(port_number, UCPD_POLICY_EVENT_INFORM_MESSAGE_RECEIVED);

  dpm->running_policy = false;

  return true;
}