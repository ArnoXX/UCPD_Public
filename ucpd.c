#include "ucpd.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd/ucpd_msg.h"
#include "ucpd/ucpd_pe_prl.h"
#include "ucpd/ucpd_port.h"
#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_typec.h"
#include <stdint.h>
#include <string.h>

bool UCPD_IsEPRMode(UCPD_PORT_Number port_number)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);
	return port->pe_prl_cad.epr_mode;
}

bool UCPD_IsPPSMode(UCPD_PORT_Number port_number)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);
	return port->pe_prl_cad.pps_contract;
}

bool UCPD_IsEPRCapable(UCPD_PORT_Number port_number)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);
	return port->pe_prl_cad.src_pdos[0].FixedSupply.epr_mode_capable == 1u;
}

uint32_t UCPD_EPREnterFailedReason(UCPD_PORT_Number port_number)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);
	return port->pe_prl_cad.epr_enter_failed_reason;
}

UCPD_Status UCPD_Init(UCPD_Config* config)
{
	if (config == NULL) return UCPD_STATUS_INVALID_PARAMETER;
	if (config->policy == NULL) return UCPD_STATUS_INVALID_PARAMETER;

	UCPD_Status status = UCPD_DPM_Init(UCPD_CTX_GetDPM(), config);
	if (status != UCPD_STATUS_SUCCESS)
	{
		return status;
	}

	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_InitConfig(UCPD_Config* config)
{
	*config = (UCPD_Config){0};

	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_Deinit(void)
{
	UCPD_DPM_Deinit(UCPD_CTX_GetDPM());
	return UCPD_STATUS_SUCCESS;
}

void* UCPD_GetUserData(void)
{
	return UCPD_CTX_GetDPM()->user_data;
}

UCPD_Status UCPD_GetPDOs(UCPD_PORT_Number port_number, const UCPD_SRC_PDO** src_pdos, uint8_t* src_pdo_count)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;
	if (src_pdo_count == NULL || src_pdos == NULL) return UCPD_STATUS_INVALID_PARAMETER;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	*src_pdo_count = port->pe_prl_cad.snk_src_pdo_count;
	*src_pdos = port->pe_prl_cad.src_pdos;

	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_TriggerPolicy(UCPD_PORT_Number port_number)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	if (port->pe_prl_cad.dpm_event != PE_PRL_CAD_EVENT_NONE) return UCPD_STATUS_BUSY;

	if (port->pe_prl_cad.sync_operation || UCPD_CTX_GetDPM()->running_policy[port_number]) return UCPD_STATUS_CANCELLED;

	port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_TRIGGER_POLICY;

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_DPM);

	while (port->pe_prl_cad.sync_operation)
		;

	return port->pe_prl_cad.dpm_request_status;
}

UCPD_Status UCPD_TriggerPolicyAsync(UCPD_PORT_Number port_number, UCPD_Callback callback)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	if (port->pe_prl_cad.dpm_event != PE_PRL_CAD_EVENT_NONE) return UCPD_STATUS_BUSY;

	port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_TRIGGER_POLICY;

	port->pe_prl_cad.callback = callback;

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_DPM);

	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_EnterEPR(UCPD_PORT_Number port_number, uint8_t power_rating)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	if (port->pe_prl_cad.dpm_event != PE_PRL_CAD_EVENT_NONE) return UCPD_STATUS_BUSY;

	if (port->pe_prl_cad.sync_operation || UCPD_CTX_GetDPM()->running_policy[port_number]) return UCPD_STATUS_CANCELLED;

	if (port->pe_prl_cad.epr_mode) return UCPD_STATUS_CANCELLED;

	port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY;

	// just fill out the power rating in the MDO, everything else is filled out inside PE SM
	port->pe_prl_cad.epr_power_rating = power_rating;

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_DPM);

	while (port->pe_prl_cad.sync_operation)
		;

	return port->pe_prl_cad.dpm_request_status;
}

UCPD_Status UCPD_ExitEPR(UCPD_PORT_Number port_number)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	if (port->pe_prl_cad.dpm_event != PE_PRL_CAD_EVENT_NONE) return UCPD_STATUS_BUSY;

	if (port->pe_prl_cad.sync_operation || UCPD_CTX_GetDPM()->running_policy[port_number]) return UCPD_STATUS_CANCELLED;

	if (port->pe_prl_cad.epr_mode == false) return UCPD_STATUS_CANCELLED;

	port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT;

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_DPM);

	while (port->pe_prl_cad.sync_operation)
		;

	return port->pe_prl_cad.dpm_request_status;
}

UCPD_Status UCPD_EnterEPRAsync(UCPD_PORT_Number port_number, uint8_t power_rating, UCPD_Callback callback)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	if (port->pe_prl_cad.epr_mode) return UCPD_STATUS_CANCELLED;

	if (port->pe_prl_cad.dpm_event != PE_PRL_CAD_EVENT_NONE) return UCPD_STATUS_BUSY;

	port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY;

	// just fill out the power rating in the MDO, everything else is filled out inside PE SM
	port->pe_prl_cad.epr_power_rating = power_rating;

	port->pe_prl_cad.callback = callback;

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_DPM);

	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_ExitEPRAsync(UCPD_PORT_Number port_number, UCPD_Callback callback)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	if (port->pe_prl_cad.epr_mode == false) return UCPD_STATUS_CANCELLED;

	if (port->pe_prl_cad.dpm_event != PE_PRL_CAD_EVENT_NONE) return UCPD_STATUS_BUSY;

	port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT;

	port->pe_prl_cad.callback = callback;

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_DPM);

	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_HardReset(UCPD_PORT_Number port_number)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	if (port->pe_prl_cad.dpm_event != PE_PRL_CAD_EVENT_NONE) return UCPD_STATUS_BUSY;

	port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_HARD_RESET;

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_DPM);

	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_HardResetAsync(UCPD_PORT_Number port_number, UCPD_Callback callback)
{
	if (port_number >= UCPD_PORT_COUNT) return UCPD_STATUS_INVALID_PORT;

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;
	if (port->pe_prl_cad.cad_state != CAD_STATE_ATTACHED) return UCPD_STATUS_UNATTACHED;

	if (port->pe_prl_cad.dpm_event != PE_PRL_CAD_EVENT_NONE) return UCPD_STATUS_BUSY;

	port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_HARD_RESET;

	port->pe_prl_cad.callback = callback;

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_DPM);

	return UCPD_STATUS_SUCCESS;
}


bool UCPD_POLICY_SelectFixedSupply(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t max_current_10mA,
                                   uint16_t operating_current_10mA, bool capability_mismatch)
{

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (port->pe_prl_cad.epr_mode) return false;
	port->pe_prl_cad.choice_valid = true;

	*port->pe_prl_cad.tx_buffer =
	    (UCPD_MSG){.header = UCPD_MSG_HEADER_INIT,
               .body = (UCPD_MSG_BODY){.snk_rdo = (UCPD_SNK_RDO){.epr_mode_capable = true,
                                                                 .object_pos = pdo_number,
                                                                 .capability_mismatch = capability_mismatch}}};

	port->pe_prl_cad.tx_buffer->body.snk_rdo.FixedSupply.max_current10mA = max_current_10mA;
	port->pe_prl_cad.tx_buffer->body.snk_rdo.FixedSupply.op_current10mA = operating_current_10mA;

	return true;
}

bool UCPD_POLICY_SelectAPDO(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t output_voltage_20mV,
                            uint16_t current_limit_50mA, bool capability_mismatch)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (port->pe_prl_cad.epr_mode) return false;
	port->pe_prl_cad.choice_valid = true;

	*port->pe_prl_cad.tx_buffer =
	    (UCPD_MSG){.header = UCPD_MSG_HEADER_INIT,
               .body = (UCPD_MSG_BODY){.snk_rdo = (UCPD_SNK_RDO){.epr_mode_capable = true,
                                                                 .object_pos = pdo_number,
                                                                 .capability_mismatch = capability_mismatch}}};

	port->pe_prl_cad.tx_buffer->body.snk_rdo.PPS.out_voltage20mV = output_voltage_20mV;
	port->pe_prl_cad.tx_buffer->body.snk_rdo.PPS.op_current50mA = current_limit_50mA;

	return true;
}

bool UCPD_POLICY_SelectAVS(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t out_voltage_100mV,
                           uint16_t current_limit_50mA, bool capability_mismatch)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (port->pe_prl_cad.epr_mode == false) return false;
	port->pe_prl_cad.choice_valid = true;

	*port->pe_prl_cad.ext_tx_buffer =
	    (UCPD_EXT_MSG){.header = UCPD_MSG_HEADER_INIT,
               .body = (UCPD_MSG_BODY){.epr_request = (UCPD_EPR_REQUEST){
                                           .src_pdo = port->pe_prl_cad.src_pdos[pdo_number - 1u],
                                           .snk_rdo = (UCPD_SNK_RDO){.epr_mode_capable = true,
                                                                     .object_pos = pdo_number,
	                                                                     .capability_mismatch = capability_mismatch}}}};

	port->pe_prl_cad.tx_buffer->body.epr_request.snk_rdo.AVS.op_current50mA = current_limit_50mA;
	port->pe_prl_cad.tx_buffer->body.epr_request.snk_rdo.AVS.out_voltage100mV = out_voltage_100mV;

	return true;
}

bool UCPD_POLICY_SelectSPR_AVS(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t out_voltage_100mV,
                               uint16_t current_limit_50mA, bool capability_mismatch)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (port->pe_prl_cad.epr_mode) return false;
	port->pe_prl_cad.choice_valid = true;

	*port->pe_prl_cad.tx_buffer =
	    (UCPD_MSG){.header = UCPD_MSG_HEADER_INIT,
               .body = (UCPD_MSG_BODY){.snk_rdo = (UCPD_SNK_RDO){.epr_mode_capable = true,
                                                                 .object_pos = pdo_number,
                                                                 .capability_mismatch = capability_mismatch}}};

	port->pe_prl_cad.tx_buffer->body.snk_rdo.AVS.op_current50mA = current_limit_50mA;
	port->pe_prl_cad.tx_buffer->body.snk_rdo.AVS.out_voltage100mV = out_voltage_100mV;

	return true;
}

bool UCPD_POLICY_SelectEPR(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t max_current_10mA,
                           uint16_t operating_current_10mA, bool capability_mismatch)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (port->pe_prl_cad.epr_mode == false) return false;
	port->pe_prl_cad.choice_valid = true;

	*port->pe_prl_cad.tx_buffer =
	    (UCPD_MSG){.header = UCPD_MSG_HEADER_INIT,
               .body = (UCPD_MSG_BODY){.epr_request = (UCPD_EPR_REQUEST){
                                           .src_pdo = port->pe_prl_cad.src_pdos[pdo_number - 1u],
                                           .snk_rdo = (UCPD_SNK_RDO){.epr_mode_capable = true,
                                                                     .object_pos = pdo_number,
	                                                                     .capability_mismatch = capability_mismatch}}}};

	port->pe_prl_cad.tx_buffer->body.epr_request.snk_rdo.FixedSupply.op_current10mA =
	    operating_current_10mA;
	port->pe_prl_cad.tx_buffer->body.epr_request.snk_rdo.FixedSupply.max_current10mA =
	    max_current_10mA;

	return true;
}
