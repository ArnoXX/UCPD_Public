#include "ucpd/ucpd_dpm.h"
#include "ucpd.h"
#include "ucpd_ctx.h"

void UCPD_DPM_EvaluateCapabilities(UCPD_PORT_Number port_number)
{
	UCPD_DPM* dpm = UCPD_CTX_GetDPM();

	if(dpm->running_policy[port_number]) return;

	dpm->running_policy[port_number] = true;

	dpm->policy(port_number, UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES);

	if (dpm->ports[port_number].pe_prl_cad.choice_valid == false)
	{
		if(dpm->ports[port_number].pe_prl_cad.epr_mode)
			UCPD_POLICY_SelectEPR(port_number, 1u, 300u, 200u, false);
		else
			UCPD_POLICY_SelectFixedSupply(port_number, 1u, 300u, 200u, false);
	}

	dpm->ports[port_number].pe_prl_cad.choice_valid = false;

	dpm->running_policy[port_number] = false;
}

void UCPD_DPM_GetCurrentCapabilities(UCPD_PORT_Number port_number)
{
	UCPD_DPM* dpm = UCPD_CTX_GetDPM();

	if(dpm->running_policy[port_number]) return;

	dpm->running_policy[port_number] = true;

	dpm->policy(port_number, UCPD_POLICY_EVENT_GET_CURRENT_CAPABILITIES);

	dpm->running_policy[port_number] = false;
}

UCPD_Status UCPD_DPM_Init(UCPD_DPM* dpm, UCPD_Config* config)
{
	dpm->policy = config->policy;
	dpm->user_data = config->user_data;
	dpm->port_callback = config->port_callback;

	for (UCPD_PORT_Number port_number = 0; port_number < UCPD_PORT_COUNT; port_number++)
	{
		UCPD_Status status = UCPD_PORT_Init(port_number);
		if (status != UCPD_STATUS_SUCCESS)
		{
			*dpm = (UCPD_DPM){0};
			return status;
		}
	}
	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_DPM_Deinit(UCPD_DPM* dpm)
{

	for (UCPD_PORT_Number port_number = 0; port_number < UCPD_PORT_COUNT; port_number++)
	{
		(void)UCPD_PORT_Deinit(port_number);
	}
	return UCPD_STATUS_SUCCESS;
}

void UCPD_DPM_PowerTransitionDefault(UCPD_PORT_Number port_number)
{
	UCPD_DPM* dpm = UCPD_CTX_GetDPM();

	if(dpm->running_policy[port_number]) return;

	dpm->running_policy[port_number] = true;

	dpm->policy(port_number, UCPD_POLICY_EVENT_POWER_TRANSITION_DEFAULT);

	dpm->running_policy[port_number] = false;
}

void UCPD_DPM_InformPowerTransition(UCPD_PORT_Number port_number)
{
	UCPD_DPM* dpm = UCPD_CTX_GetDPM();

	if(dpm->running_policy[port_number]) return;

	dpm->running_policy[port_number] = true;

	dpm->policy(port_number, UCPD_POLICY_EVENT_POWER_TRANSITION);

	dpm->running_policy[port_number] = false;
}

void UCPD_DPM_GetRevision(UCPD_PORT_Number port_number)
{
	UCPD_DPM* dpm = UCPD_CTX_GetDPM();

	if(dpm->running_policy[port_number]) return;

	dpm->running_policy[port_number] = true;
	dpm->policy(port_number, UCPD_POLICY_EVENT_GET_REVISION);

	dpm->running_policy[port_number] = false;
}


void UCPD_DPM_InformMessageReceived(UCPD_PORT_Number port_number)
{
	UCPD_DPM* dpm = UCPD_CTX_GetDPM();

	if(dpm->running_policy[port_number]) return;

	dpm->running_policy[port_number] = true;

	dpm->policy(port_number, UCPD_POLICY_EVENT_INFORM_MESSAGE_RECEIVED);

	dpm->running_policy[port_number] = false;

	return;
}
