#include "ucpd.h"
#include "device/trace_utils.h"
#include "ucpd/ucpd_cnt.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_msg.h"
#include "ucpd/ucpd_port.h"
#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_utils.h"

UCPD_Status UCPD_Init(UCPD_PORT_Number port_number, UCPD_Config *config) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;
  if (config == NULL)
    return UCPD_STATUS_INVALID_PARAMETER;
  if (config->policy == NULL)
    return UCPD_STATUS_INVALID_PARAMETER;

  if (UCPD_PORT_Init(port_number) == UCPD_STATUS_INITIALIZED)
    return UCPD_STATUS_INITIALIZED;

  UCPD_DPM_Init(UCPD_CTX_GetDPM(), config->policy);

  TRACE_INFO("PORT Initialized\n\r");

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_InitConfig(UCPD_Config *config) {
  CLEAR_STRUCT_PTR(config);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_Deinit(UCPD_PORT_Number port_number) {

  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  return UCPD_STATUS_SUCCESS;
}

/* Blocking functions */
UCPD_Status UCPD_GetPDOs(UCPD_PORT_Number port_number,
                         UCPD_SRC_PDO **const src_pdos,
                         UCPD_Count *src_pdo_count) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;
  if (src_pdo_count == NULL || src_pdos == NULL)
    return UCPD_STATUS_INVALID_PARAMETER;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  *src_pdo_count = port->snk_src_pdo_count;
  *src_pdos = port->src_pdos;

  return UCPD_STATUS_SUCCESS;
}

/*
@param pdo_number - the PDO number to request
@param max_current - the maximum current to request in 10mA increments
@param operating_current - the operating current to request in 10mA increments
*/
UCPD_Status UCPD_RequestFixedSupply(UCPD_PORT_Number port_number,
                                    UCPD_Count pdo_number,
                                    UCPD_Count max_current,
                                    UCPD_Count operating_current,
                                    UCPD_Bool capability_mismatch) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->running_policy == UCPD_TRUE ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_FIXED_SUPPLY;

  UCPD_POLICY_SelectFixedSupply(port_number, pdo_number, max_current,
                                operating_current, capability_mismatch);

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->sync_operation == UCPD_TRUE)
    ;

  if (port->pe_prl_cad.buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_type !=
      UCPD_PS_RDY_MSG_ID)
    return UCPD_STATUS_FAILURE;

  return UCPD_STATUS_SUCCESS;
}

/*
@param pdo_number - the PDO number to request
@param output_voltage_20mV - the output voltage to request in 20mV increments
@param current_limit_50mA - the current limit to request in 50mA increments
@param capability_mismatch - whether to request a capability mismatch
*/
UCPD_Status UCPD_RequestAPDO(UCPD_PORT_Number port_number,
                             UCPD_Count pdo_number,
                             UCPD_Count output_voltage_20mV,
                             UCPD_Count current_limit_50mA,
                             UCPD_Bool capability_mismatch) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->running_policy == UCPD_TRUE ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_PPS;

  UCPD_POLICY_SelectAPDO(port_number, pdo_number, output_voltage_20mV,
                         current_limit_50mA, capability_mismatch);

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->sync_operation == UCPD_TRUE)
    ;
  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestEPR(UCPD_PORT_Number port_number, UCPD_Count pdo_number,
                            UCPD_Count max_current,
                            UCPD_Count operating_current,
                            UCPD_Bool capability_mismatch) {

  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == UCPD_FALSE)
    return UCPD_STATUS_FAILURE;

  if (port->running_policy == UCPD_TRUE ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR;

  UCPD_POLICY_SelectEPR(port_number, pdo_number, max_current, operating_current,
                        capability_mismatch);

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->sync_operation == UCPD_TRUE)
    ;

  if (port->pe_prl_cad.buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_type !=
      UCPD_PS_RDY_MSG_ID)
    return UCPD_STATUS_FAILURE;

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestAVS(UCPD_PORT_Number port_number,
                            UCPD_Count pdo_number,
                            UCPD_Count out_voltage_100mV,
                            UCPD_Count current_limit_50mA,
                            UCPD_Bool capability_mismatch) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->running_policy == UCPD_TRUE ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_PPS;

  UCPD_POLICY_SelectAVS(port_number, pdo_number, out_voltage_100mV,
                        current_limit_50mA, capability_mismatch);

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->sync_operation == UCPD_TRUE)
    ;
  return UCPD_STATUS_SUCCESS;
}

/* Async functions */
/*
@param pdo_number - the PDO number to request
@param max_current - the maximum current to request in 10mA increments
@param operating_current - the operating current to request in 10mA increments
@param callback - the callback to call when the request is complete
*/
UCPD_Status UCPD_RequestFixedSupplyAsync(UCPD_PORT_Number port_number,
                                         UCPD_Count pdo_number,
                                         UCPD_Count max_current,
                                         UCPD_Count operating_current,
                                         UCPD_Bool capability_mismatch,
                                         UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->running_policy == UCPD_TRUE || port->current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_FIXED_SUPPLY;

  UCPD_POLICY_SelectFixedSupply(port_number, pdo_number, max_current,
                                operating_current, capability_mismatch);

  port->current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

/*
@param pdo_number - the PDO number to request
@param output_voltage_20mV - the output voltage to request in 20mV increments
@param current_limit_50mA - the current limit to request in 50mA increments
@param capability_mismatch - whether to request a capability mismatch
@param callback - the callback to call when the request is complete
*/
UCPD_Status UCPD_RequestAPDOAsync(UCPD_PORT_Number port_number,
                                  UCPD_Count pdo_number,
                                  UCPD_Count output_voltage_20mV,
                                  UCPD_Count current_limit_50mA,
                                  UCPD_Bool capability_mismatch,
                                  UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->running_policy == UCPD_TRUE || port->current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_PPS;

  UCPD_POLICY_SelectAPDO(port_number, pdo_number, output_voltage_20mV,
                         current_limit_50mA, capability_mismatch);

  port->current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestEPRAsync(UCPD_PORT_Number port_number,
                                 UCPD_Count pdo_number, UCPD_Count max_current,
                                 UCPD_Count operating_current,
                                 UCPD_Bool capability_mismatch,
                                 UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == UCPD_FALSE)
    return UCPD_STATUS_FAILURE;

  if (port->running_policy == UCPD_TRUE ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR;

  UCPD_POLICY_SelectEPR(port_number, pdo_number, max_current, operating_current,
                        capability_mismatch);

  port->current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestAVSAsync(UCPD_PORT_Number port_number,
                                 UCPD_Count pdo_number,
                                 UCPD_Count out_voltage_100mV,
                                 UCPD_Count current_limit_50mA,
                                 UCPD_Bool capability_mismatch,
                                 UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->running_policy == UCPD_TRUE || port->current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_PPS;

  UCPD_POLICY_SelectAVS(port_number, pdo_number, out_voltage_100mV,
                        current_limit_50mA, capability_mismatch);

  port->current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_EnterEPR(UCPD_PORT_Number port_number,
                          UCPD_Count power_rating) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == UCPD_TRUE)
    return UCPD_STATUS_SUCCESS;

  if (port->running_policy == UCPD_TRUE ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY || port->pe_prl_cad.explicit_contract == UCPD_FALSE)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY;

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].msg.body.epr_mdo.data =
      power_rating;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->sync_operation == UCPD_TRUE)
    ;

  if (port->pe_prl_cad.epr_mode == UCPD_FALSE)
    return UCPD_STATUS_FAILURE;

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_ExitEPR(UCPD_PORT_Number port_number) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == UCPD_FALSE)
    return UCPD_STATUS_SUCCESS;

  if (port->running_policy == UCPD_TRUE ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY || port->pe_prl_cad.explicit_contract == UCPD_FALSE)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->sync_operation == UCPD_TRUE)
    ;

  if (port->pe_prl_cad.epr_mode == UCPD_TRUE)
    return UCPD_STATUS_FAILURE;

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_EnterEPRAsync(UCPD_PORT_Number port_number,
                               UCPD_Count power_rating,
                               UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == UCPD_TRUE)
    return UCPD_STATUS_SUCCESS;

  if (port->running_policy == UCPD_TRUE || port->current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY || port->pe_prl_cad.explicit_contract == UCPD_FALSE)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY;

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].msg.body.epr_mdo.data =
      power_rating;

  port->current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_ExitEPRAsync(UCPD_PORT_Number port_number,
                              UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized != UCPD_TRUE)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == UCPD_FALSE)
    return UCPD_STATUS_SUCCESS;

  if (port->running_policy == UCPD_TRUE || port->current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY || port->pe_prl_cad.explicit_contract == UCPD_FALSE)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT;

  port->current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

void UCPD_POLICY_SelectFixedSupply(UCPD_PORT_Number port_number,
                                   UCPD_Count pdo_number,
                                   UCPD_Count max_current,
                                   UCPD_Count operating_current,
                                   UCPD_Bool capability_mismatch) {

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].msg.body.snk_rdo.object_pos =
      pdo_number;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.snk_rdo.FixedSupply.max_current10mA = max_current;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.snk_rdo.FixedSupply.op_current10mA = operating_current;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.snk_rdo.capability_mismatch =
      capability_mismatch == UCPD_TRUE ? 1u : 0u;
}

void UCPD_POLICY_SelectAPDO(UCPD_PORT_Number port_number, UCPD_Count pdo_number,
                            UCPD_Count output_voltage_20mV,
                            UCPD_Count current_limit_50mA,
                            UCPD_Bool capability_mismatch) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].msg.body.snk_rdo.object_pos =
      pdo_number;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.snk_rdo.PPS.out_voltage20mV = output_voltage_20mV;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.snk_rdo.PPS.op_current50mA = current_limit_50mA;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.snk_rdo.capability_mismatch =
      capability_mismatch == UCPD_TRUE ? 1u : 0u;
}

void UCPD_POLICY_SelectAVS(UCPD_PORT_Number port_number,
                           UCPD_Count pdo_number,
                           UCPD_Count out_voltage_100mV,
                           UCPD_Count current_limit_50mA,
                           UCPD_Bool capability_mismatch) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].msg.body.epr_request.snk_rdo.object_pos =
      pdo_number;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.epr_request.snk_rdo.AVS.op_current50mA = current_limit_50mA;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.epr_request.snk_rdo.AVS.out_voltage100mV = out_voltage_100mV;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.epr_request.snk_rdo.capability_mismatch =
      capability_mismatch == UCPD_TRUE ? 1u : 0u;
}

void UCPD_POLICY_SelectEPR(UCPD_PORT_Number port_number, UCPD_Count pdo_number,
                           UCPD_Count max_current, UCPD_Count operating_current,
                           UCPD_Bool capability_mismatch) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.epr_request.snk_rdo.object_pos = pdo_number;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.epr_request.snk_rdo.FixedSupply.max_current10mA = max_current;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.epr_request.snk_rdo.FixedSupply.op_current10mA = operating_current;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX]
      .msg.body.epr_request.snk_rdo.capability_mismatch =
      capability_mismatch == UCPD_TRUE ? 1u : 0u;
}
