#include "ucpd.h"
#include "device/trace_utils.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_msg.h"
#include "ucpd/ucpd_port.h"
#include "ucpd/ucpd_port_num.h"
#include <stdint.h>

UCPD_Status UCPD_Init(UCPD_Config *config) {
  if (config == NULL)
    return UCPD_STATUS_INVALID_PARAMETER;
  if (config->policy == NULL)
    return UCPD_STATUS_INVALID_PARAMETER;



  UCPD_Status status = UCPD_DPM_Init(UCPD_CTX_GetDPM(), config->policy);
  if (status != UCPD_STATUS_SUCCESS) {
    return status;
  }

  TRACE_INFO("PORT Initialized\n\r");

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_InitConfig(UCPD_Config *config) {
  *config = (UCPD_Config){0};

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_Deinit(void) {

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_GetPDOs(UCPD_PORT_Number port_number,
                         UCPD_SRC_PDO **const src_pdos,
                         uint8_t *src_pdo_count) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;
  if (src_pdo_count == NULL || src_pdos == NULL)
    return UCPD_STATUS_INVALID_PARAMETER;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  *src_pdo_count = port->pe_prl_cad.snk_src_pdo_count;
  *src_pdos = port->pe_prl_cad.src_pdos;

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestFixedSupply(UCPD_PORT_Number port_number,
                                    uint8_t pdo_number,
                                    uint16_t max_current_10mA,
                                    uint16_t operating_current_10mA,
                                    bool capability_mismatch) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_FIXED_SUPPLY;

  UCPD_POLICY_SelectFixedSupply(port_number, pdo_number, max_current_10mA,
                                operating_current_10mA, capability_mismatch);

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->pe_prl_cad.sync_operation)
    ;

  if (port->pe_prl_cad.buffers[UCPD_RX_BUFFER_INDEX].header.message_type !=
      UCPD_PS_RDY_MSG_ID)
    return UCPD_STATUS_FAILURE;

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestAPDO(UCPD_PORT_Number port_number, uint8_t pdo_number,
                             uint16_t output_voltage_20mV,
                             uint16_t current_limit_50mA,
                             bool capability_mismatch) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_PPS;

  UCPD_POLICY_SelectAPDO(port_number, pdo_number, output_voltage_20mV,
                         current_limit_50mA, capability_mismatch);

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->pe_prl_cad.sync_operation)
    ;
  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestEPR(UCPD_PORT_Number port_number, uint8_t pdo_number,
                            uint16_t max_current_10mA,
                            uint16_t operating_current_10mA,
                            bool capability_mismatch) {

  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == false)
    return UCPD_STATUS_FAILURE;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR;

  UCPD_POLICY_SelectEPR(port_number, pdo_number, max_current_10mA,
                        operating_current_10mA, capability_mismatch);

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->pe_prl_cad.sync_operation)
    ;

  if (port->pe_prl_cad.buffers[UCPD_RX_BUFFER_INDEX].header.message_type !=
      UCPD_PS_RDY_MSG_ID)
    return UCPD_STATUS_FAILURE;

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestAVS(UCPD_PORT_Number port_number, uint8_t pdo_number,
                            uint16_t out_voltage_100mV,
                            uint16_t current_limit_50mA,
                            bool capability_mismatch) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_PPS;

  UCPD_POLICY_SelectAVS(port_number, pdo_number, out_voltage_100mV,
                        current_limit_50mA, capability_mismatch);

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->pe_prl_cad.sync_operation)
    ;
  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestFixedSupplyAsync(UCPD_PORT_Number port_number,
                                         uint8_t pdo_number,
                                         uint16_t max_current_10mA,
                                         uint16_t operating_current_10mA,
                                         bool capability_mismatch,
                                         UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_FIXED_SUPPLY;

  UCPD_POLICY_SelectFixedSupply(port_number, pdo_number, max_current_10mA,
                                operating_current_10mA, capability_mismatch);

  port->pe_prl_cad.current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status
UCPD_RequestAPDOAsync(UCPD_PORT_Number port_number, uint8_t pdo_number,
                      uint16_t output_voltage_20mV, uint16_t current_limit_50mA,
                      bool capability_mismatch, UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_PPS;

  UCPD_POLICY_SelectAPDO(port_number, pdo_number, output_voltage_20mV,
                         current_limit_50mA, capability_mismatch);

  port->pe_prl_cad.current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestEPRAsync(UCPD_PORT_Number port_number,
                                 uint8_t pdo_number, uint16_t max_current_10mA,
                                 uint16_t operating_current_10mA,
                                 bool capability_mismatch,
                                 UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == false)
    return UCPD_STATUS_FAILURE;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR;

  UCPD_POLICY_SelectEPR(port_number, pdo_number, max_current_10mA,
                        operating_current_10mA, capability_mismatch);

  port->pe_prl_cad.current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_RequestAVSAsync(UCPD_PORT_Number port_number,
                                 uint8_t pdo_number, uint16_t out_voltage_100mV,
                                 uint16_t current_limit_50mA,
                                 bool capability_mismatch,
                                 UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_PPS;

  UCPD_POLICY_SelectAVS(port_number, pdo_number, out_voltage_100mV,
                        current_limit_50mA, capability_mismatch);

  port->pe_prl_cad.current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_EnterEPR(UCPD_PORT_Number port_number, uint8_t power_rating) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode)
    return UCPD_STATUS_SUCCESS;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY ||
      port->pe_prl_cad.explicit_contract == false)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY;

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].header = UCPD_MSG_HEADER_INIT;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.epr_mdo = (UCPD_EPRMDO){
      .action = UCPD_EPR_ACTION_ENTER,
      .data = power_rating,
  };

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->pe_prl_cad.sync_operation)
    ;

  if (port->pe_prl_cad.epr_mode == false)
    return UCPD_STATUS_FAILURE;

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_ExitEPR(UCPD_PORT_Number port_number) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == false)
    return UCPD_STATUS_SUCCESS;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY ||
      port->pe_prl_cad.explicit_contract == false)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT;

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].header = UCPD_MSG_HEADER_INIT;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.epr_mdo = (UCPD_EPRMDO){
      .action = UCPD_EPR_ACTION_EXIT
  };


  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  while (port->pe_prl_cad.sync_operation)
    ;

  if (port->pe_prl_cad.epr_mode)
    return UCPD_STATUS_FAILURE;

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_EnterEPRAsync(UCPD_PORT_Number port_number,
                               uint8_t power_rating, UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode)
    return UCPD_STATUS_SUCCESS;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY ||
      port->pe_prl_cad.explicit_contract == false)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY;

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].header = UCPD_MSG_HEADER_INIT;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.epr_mdo = (UCPD_EPRMDO){
      .action = UCPD_EPR_ACTION_ENTER,
      .data = power_rating,
  };

  port->pe_prl_cad.current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_ExitEPRAsync(UCPD_PORT_Number port_number,
                              UCPD_Callback callback) {
  if (port_number >= UCPD_PORT_COUNT)
    return UCPD_STATUS_INVALID_PORT;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (!port->initialized)
    return UCPD_STATUS_UNITIALIZED;

  if (port->pe_prl_cad.epr_mode == false)
    return UCPD_STATUS_SUCCESS;

  if (UCPD_CTX_GetDPM()->running_policy ||
      port->pe_prl_cad.current_callback != NULL ||
      port->pe_prl_cad.snk_state != PE_SNK_STATE_READY ||
      port->pe_prl_cad.explicit_contract == false)
    return UCPD_STATUS_BUSY;

  port->pe_prl_cad.dpm_event = PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT;

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].header = UCPD_MSG_HEADER_INIT;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.epr_mdo = (UCPD_EPRMDO){
      .action = UCPD_EPR_ACTION_EXIT
  };


  port->pe_prl_cad.current_callback = callback;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  return UCPD_STATUS_SUCCESS;
}

void UCPD_POLICY_SelectFixedSupply(UCPD_PORT_Number port_number,
                                   uint8_t pdo_number,
                                   uint16_t max_current_10mA,
                                   uint16_t operating_current_10mA,
                                   bool capability_mismatch) {

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX] =
      (UCPD_MSG){.header = UCPD_MSG_HEADER_INIT,
                 .body = (UCPD_MSG_BODY){
                     .snk_rdo = (UCPD_SNK_RDO){
                         .epr_mode_capable = true,
                         .object_pos = pdo_number,
                         .capability_mismatch = capability_mismatch}}};


    port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.snk_rdo.FixedSupply.max_current10mA = max_current_10mA;
    port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.snk_rdo.FixedSupply.op_current10mA = operating_current_10mA;
}

//should not be called when in epr mode
void UCPD_POLICY_SelectAPDO(UCPD_PORT_Number port_number, uint8_t pdo_number,
                            uint16_t output_voltage_20mV,
                            uint16_t current_limit_50mA,
                            bool capability_mismatch) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX] =
      (UCPD_MSG){.header = UCPD_MSG_HEADER_INIT,
                 .body = (UCPD_MSG_BODY){
                     .snk_rdo = (UCPD_SNK_RDO){
                         .epr_mode_capable = true,
                         .object_pos = pdo_number,
                         .capability_mismatch = capability_mismatch}}};


  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.snk_rdo.PPS.out_voltage20mV = output_voltage_20mV;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.snk_rdo.PPS.op_current50mA = current_limit_50mA;
}

void UCPD_POLICY_SelectAVS(UCPD_PORT_Number port_number, uint8_t pdo_number,
                           uint16_t out_voltage_100mV,
                           uint16_t current_limit_50mA,
                           bool capability_mismatch) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX] =
      (UCPD_MSG){.header = UCPD_MSG_HEADER_INIT,
                 .body = (UCPD_MSG_BODY){
                     .epr_request = (UCPD_EPR_REQUEST){
                         .src_pdo = port->pe_prl_cad.src_pdos[pdo_number - 1u],
                         .snk_rdo = (UCPD_SNK_RDO){
                             .epr_mode_capable = true,
                             .object_pos = pdo_number,
                             .capability_mismatch = capability_mismatch}}}};


  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.epr_request.snk_rdo.AVS.op_current50mA = current_limit_50mA;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.epr_request.snk_rdo.AVS.out_voltage100mV = out_voltage_100mV;
}

void UCPD_POLICY_SelectEPR(UCPD_PORT_Number port_number, uint8_t pdo_number,
                           uint16_t max_current_10mA,
                           uint16_t operating_current_10mA,
                           bool capability_mismatch) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX] =
      (UCPD_MSG){.header = UCPD_MSG_HEADER_INIT,
                 .body = (UCPD_MSG_BODY){
                     .epr_request = (UCPD_EPR_REQUEST){
                         .src_pdo = port->pe_prl_cad.src_pdos[pdo_number - 1u],
                         .snk_rdo = (UCPD_SNK_RDO){
                             .epr_mode_capable = true,
                             .object_pos = pdo_number,
                             .capability_mismatch = capability_mismatch}}}};

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.epr_request.snk_rdo.FixedSupply.op_current10mA = operating_current_10mA;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].body.epr_request.snk_rdo.FixedSupply.max_current10mA = max_current_10mA;
}
