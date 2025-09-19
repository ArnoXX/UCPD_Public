#include "ucpd_pe_sm.h"
#include "device/trace_utils.h"
#include "device/ucpd_pwr.h"
#include "ucpd/ucpd_dpm.h"
#include "ucpd/ucpd_pe_sm.h"
#include "ucpd/ucpd_port.h"
#include "ucpd/ucpd_tim.h"
#include "ucpd_cnt.h"
#include "ucpd_ctx.h"
#include "ucpd_msg.h"
#include "ucpd_pe_prl.h"
#include "ucpd_prl_sm.h"
#include "ucpd_tim.h"
#include "ucpd_utils.h"

UCPD_PE_SM_SNK_State getPEStateToEnterOnReceive(UCPD_MSG *msg) {

  if (msg->msg.header.extended == 1u) {

    switch (msg->msg.header.message_type) {
    case UCPD_EXTENDED_CONTROL_MSG_ID: {
      if (msg->msg.ext_message.body.epr_ecdb.type ==
          UCPD_EPR_KEEP_ALIVE_ACK_EXT_MSG_ID) {
        TRACE_MSG_RX("EPR keep alive acknowledged\n\r");
        return PE_SNK_STATE_READY;
      }
      return PE_SNK_STATE_NONE;
    }
    case UCPD_EPR_SOURCE_CAPABILITIES_MSG_ID: {
      TRACE_MSG_RX("EPR source capabilities message received\n\r");
      return PE_SNK_STATE_EVALUATE_CAPABILITY;
    }
    default: {
      return PE_SNK_STATE_NONE;
    }
    }
  } else {
    // TODO: finish this
    switch (msg->msg.header.message_type) {
    case UCPD_SOURCE_CAPABILITIES_MSG_ID: {
      TRACE_MSG_RX("Source capabilities message received\n\r");
      return PE_SNK_STATE_EVALUATE_CAPABILITY;
    }
    case UCPD_ACCEPT_MSG_ID: {
      TRACE_MSG_RX("Accept message received\n\r");
      return PE_SNK_STATE_TRANSITION_SINK;
    }
    case UCPD_PS_RDY_MSG_ID: {
      TRACE_MSG_RX("PS ready message received\n\r");
      return PE_SNK_STATE_READY;
    }
    case UCPD_NOT_SUPPORTED_MSG_ID: {
      TRACE_MSG_RX("Not supported message received\n\r");
      return PE_SNK_STATE_NOT_SUPPORTED_RECEIVED;
    }
    case UCPD_REJECT_MSG_ID: {
      TRACE_MSG_RX("Reject message received\n\r");
      return PE_SNK_STATE_NONE;
    }
    case UCPD_EPR_MODE_MSG_ID: {
      if (msg->msg.body.epr_mdo.action == UCPD_EPR_ACTION_ENTER_ACK) {
        TRACE_MSG_RX("EPR enter acknowledged\n\r");
        return PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE;
      } else if (msg->msg.body.epr_mdo.action != UCPD_EPR_ACTION_ENTER_SUCC) {
        TRACE_MSG_RX("EPR enter failed\n\r");
        return PE_SNK_STATE_SEND_SOFT_RESET;
      } else if (msg->msg.body.epr_mdo.action == UCPD_EPR_ACTION_ENTER_SUCC) {
        TRACE_MSG_RX("EPR enter successful\n\r");
        return PE_SNK_STATE_WAIT_FOR_CAPABILITIES;
      } else if (msg->msg.body.epr_mdo.action == UCPD_EPR_ACTION_EXIT) {
        TRACE_MSG_RX("EPR exit message received\n\r");
        return PE_SNK_STATE_EPR_MODE_EXIT_RECEIVED;
      }
      return PE_SNK_STATE_SEND_SOFT_RESET;
    }
    default: {
      // not supported message
      return PE_SNK_STATE_SEND_NOT_SUPPORTED;
    }
    }
  }
  return PE_SNK_STATE_NONE;
}

UCPD_PE_SM_SNK_State getPEStateToEnterOnTransmit(UCPD_MSG *msg) {
  // TODO: finish this

  if (msg->msg.header.extended == 1u) {
    msg->msg.header.extended = 0u;

  } else {
    switch (msg->msg.header.message_type) {
    // we do not transition from select capability state on sending the request
    case UCPD_REQUEST_MSG_ID: {
      TRACE_MSG_TX("Request message transmitted\n\r");
      return PE_SNK_STATE_NONE;
    }
    case UCPD_NOT_SUPPORTED_MSG_ID: {
      TRACE_MSG_TX("Not supported message transmitted\n\r");
      return PE_SNK_STATE_READY;
    }
    case UCPD_ACCEPT_MSG_ID: {
      TRACE_MSG_TX("Accept message transmitted\n\r");
      return PE_SNK_STATE_WAIT_FOR_CAPABILITIES;
    }
    case UCPD_SOFT_RESET_MSG_ID: {
      TRACE_MSG_TX("Soft reset message transmitted\n\r");
      return PE_SNK_STATE_NONE;
    }
    case UCPD_EPR_MODE_MSG_ID: {
      if (msg->msg.body.epr_mdo.action == UCPD_EPR_ACTION_EXIT) {
        TRACE_MSG_TX("EPR exit message transmitted\n\r");
        return PE_SNK_STATE_WAIT_FOR_CAPABILITIES;
      }
      return PE_SNK_STATE_NONE;
    }
    case UCPD_EPR_REQUEST_MSG_ID: {
      TRACE_MSG_TX("EPR request message transmitted\n\r");
      return PE_SNK_STATE_NONE;
    }
    default: {
      return PE_SNK_STATE_NONE;
    }
    }
  }
  return PE_SNK_STATE_NONE;
}

extern UCPD_Bool waiting_for_epr;

void UCPD_PE_SM_Delta(UCPD_PORT_Number port_number,
                      UCPD_PE_PRL_CAD_Event event) {

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  UCPD_PE_PRL_CAD_Module *pe_prl_cad = &port->pe_prl_cad;

  switch (event) {

  case PE_PRL_EVENT_PRL_INIT: {
    UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_STOPPED);
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_STARTUP);

    pe_prl_cad->ams = UCPD_FALSE;
    pe_prl_cad->explicit_contract = UCPD_FALSE;
    pe_prl_cad->ams_first_sent = UCPD_FALSE;
    pe_prl_cad->pps_contract = UCPD_FALSE;
    pe_prl_cad->pps_periodic_expired = UCPD_FALSE;
    pe_prl_cad->hardResetActive = UCPD_FALSE;
    pe_prl_cad->hrdOriginPE = UCPD_FALSE;
    pe_prl_cad->epr_mode = UCPD_FALSE;
    pe_prl_cad->response_expected = UCPD_FALSE;
    pe_prl_cad->entering_epr = UCPD_FALSE;
    pe_prl_cad->extended_message = UCPD_FALSE;

    break;
  }

  case PE_PRL_EVENT_PE_START_SRT: {
    UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_RUNNING);
    break;
  }

  case PE_PRL_EVENT_PE_STOP_SRT: {
    UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_STOPPED);
    break;
  }

  case PE_PRL_EVENT_PE_TRANSMISSION_ERROR: {
    if (pe_prl_cad->snk_state == PE_SNK_STATE_SEND_SOFT_RESET)
      UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_HARD_RESET);
    else if (pe_prl_cad->ams_first_sent == UCPD_FALSE &&
             pe_prl_cad->explicit_contract == UCPD_TRUE)
      UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_READY);
    else
      UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SEND_SOFT_RESET);
    break;
  }

  case PE_PRL_EVENT_PE_MESSAGE_RECEIVED: {



    UCPD_PE_SM_SNK_State state_to_enter =
        getPEStateToEnterOnReceive(&pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]);

    // here the reset table will be applicable

    // first check against the table for reset cases

    // then special cases
    if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_type ==
        UCPD_REJECT_MSG_ID) {
      if (pe_prl_cad->explicit_contract == UCPD_TRUE)
        UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_READY);
      else
        UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);

      return;
    }

    // then enter the state
    if(state_to_enter != PE_SNK_STATE_NONE)
    	UCPD_PE_SM_SNK_Enter(port_number, state_to_enter);

    break;
  }

  case PE_PRL_EVENT_PE_MESSAGE_SENT: {

    if (pe_prl_cad->ams == UCPD_TRUE &&
        pe_prl_cad->ams_first_sent == UCPD_FALSE)
      pe_prl_cad->ams_first_sent = UCPD_TRUE;

    UCPD_PE_SM_SNK_State state_to_enter =
        getPEStateToEnterOnTransmit(&pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]);

    if (state_to_enter != PE_SNK_STATE_NONE)
      UCPD_PE_SM_SNK_Enter(port_number, state_to_enter);

    if (pe_prl_cad->response_expected == UCPD_TRUE) {
      UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_RUNNING);
      pe_prl_cad->response_expected = UCPD_FALSE;
    }

    if (pe_prl_cad->entering_epr == UCPD_TRUE) {
      UCPD_TIM_Start(port_number, UCPD_TIM_SINK_EPR_ENTER);
      pe_prl_cad->entering_epr = UCPD_FALSE;
    }
    break;
  }

  case PE_PRL_EVENT_PE_ERROR_WITH_MESSAGE: {
    //???
    break;
  }

  case PE_PRL_EVENT_PE_HARD_RESET_RECEIVED: {
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_TRANSITION_TO_DEFAULT);
    break;
  }

  case PE_PRL_EVENT_PE_HARD_RESET_SENT: {
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_TRANSITION_TO_DEFAULT);
    break;
  }

  case PE_PRL_EVENT_VBUS_PRESENT: {
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);
    break;
  }

  case PE_PRL_EVENT_SRT_EXPIRED: {
    UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_EXPIRED);
    break;
  }

  case PE_PRL_EVENT_SNK_PPS_PER_TIMEOUT: {
    pe_prl_cad->pps_periodic_expired = UCPD_TRUE;
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_EVALUATE_CAPABILITY);
    break;
  }

  case PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY: {
    port->sync_operation = UCPD_TRUE;
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SEND_EPR_MODE_ENTRY);
    break;
  }

  case PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT: {
    port->sync_operation = UCPD_TRUE;
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SEND_EPR_MODE_EXIT);
    break;
  }

  case PE_PRL_EVENT_EPR_ENTER_TIMEOUT: {
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SEND_SOFT_RESET);
    break;
  }

  case PE_PRL_EVENT_EPR_KEEP_ALIVE_TIMEOUT: {
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_EPR_KEEP_ALIVE);
    break;
  }

  case PE_PRL_EVENT_DPM_REQUEST_FIXED_SUPPLY: {
    port->sync_operation = UCPD_TRUE;
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SELECT_CAPABILITY);
    break;
  }

  case PE_PRL_EVENT_DPM_REQUEST_PPS: {
    port->sync_operation = UCPD_TRUE;

    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SELECT_CAPABILITY);
    break;
  }

  case PE_PRL_EVENT_DPM_REQUEST_EPR: {
    port->sync_operation = UCPD_TRUE;
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SELECT_CAPABILITY);
    break;
  }

  default: {
    break;
  }
  }
}

void UCPD_PE_SM_SRT_Enter(UCPD_PORT_Number port_number,
                          UCPD_PE_SM_SRT_State state) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_PE_PRL_CAD_Module *pe_prl_cad = &port->pe_prl_cad;

  switch (state) {
  case PE_SRT_STATE_STOPPED: {
    UCPD_TIM_Stop(port_number, UCPD_TIM_SENDER_RESPONSE);

    break;
  }

  case PE_SRT_STATE_RUNNING: {
    UCPD_TIM_Stop(port_number, UCPD_TIM_SENDER_RESPONSE);
    UCPD_TIM_Start(port_number, UCPD_TIM_SENDER_RESPONSE);

    break;
  }

  case PE_SRT_STATE_EXPIRED: {
    // TODO: check if hrd is the only one that can be entered
    if (pe_prl_cad->snk_state == PE_SNK_STATE_GET_SOURCE_CAPABILITIES ||
        pe_prl_cad->snk_state == PE_SNK_STATE_WAIT_FOR_GET_STATUS)
      UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_READY);
    else if (pe_prl_cad->snk_state ==
                 PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE ||
             pe_prl_cad->snk_state == PE_SNK_STATE_SEND_EPR_MODE_ENTRY) {

      UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SEND_SOFT_RESET);

      if (port->current_callback != NULL) {
        port->current_callback(UCPD_STATUS_FAILURE);
        port->current_callback = NULL;
      }
      port->sync_operation = UCPD_FALSE;
    } else
      UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_HARD_RESET);

    break;
  }

  default: {
    break;
  }
  };
}

void UCPD_PE_SM_SNK_Enter(UCPD_PORT_Number port_number,
                          UCPD_PE_SM_SNK_State state) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_PE_PRL_CAD_Module *pe_prl_cad = &port->pe_prl_cad;

  UCPD_PE_SM_SNK_Exit(port_number, pe_prl_cad->snk_state);
  pe_prl_cad->snk_state = state;

  switch (state) {

  case PE_SNK_STATE_STARTUP: {

    
    pe_prl_cad->hrdOriginPE = UCPD_FALSE;
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_DISCOVERY);

    break;
  }

  case PE_SNK_STATE_DISCOVERY: {

    if (UCPD_PWR_vS0_to_vS5(port_number) == UCPD_TRUE)
      UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);

    break;
  }

  case PE_SNK_STATE_WAIT_FOR_CAPABILITIES: {
    pe_prl_cad->explicit_contract = UCPD_FALSE;


    UCPD_TIM_Start(port_number, UCPD_TIM_SINK_WAIT_CAP);


    if (port->current_callback != NULL) {
      if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.extended == 0u &&
          pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_type ==
              UCPD_REJECT_MSG_ID)
        port->current_callback(UCPD_STATUS_FAILURE);
      else
        port->current_callback(UCPD_STATUS_SUCCESS);

      port->current_callback = NULL;
    }

    port->sync_operation = UCPD_FALSE;
    break;
  }

  case PE_SNK_STATE_EVALUATE_CAPABILITY: {
    UCPD_CNT_Reset(port_number, UCPD_CNT_HARD_RESET);

    pe_prl_cad->ams = UCPD_TRUE;

    if (pe_prl_cad->pps_periodic_expired == UCPD_FALSE) {
      UCPD_MSG *msg = &(pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]);
      UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
      UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_AMS_FIRST_FROM_PE);

      if (msg->msg.header.extended == 0u &&
          msg->msg.header.message_type == UCPD_SOURCE_CAPABILITIES_MSG_ID) {
        port->snk_src_pdo_count = msg->msg.header.number_of_data_objects;
        COPY_RAW(msg->msg.body.src_pdos, port->src_pdos,
                 sizeof(UCPD_SRC_PDO) * port->snk_src_pdo_count);
      } else if (msg->msg.header.extended == 1u &&
                 msg->msg.header.message_type ==
                     UCPD_EPR_SOURCE_CAPABILITIES_MSG_ID) {
        uint8_t count =
            msg->msg.ext_message.ext_header.data_size / UCPD_PDO_SIZE;
        if (count > (UCPD_MAX_PDO_NUM + UCPD_MAX_EPR_PDO_NUM))
          count = (UCPD_MAX_PDO_NUM + UCPD_MAX_EPR_PDO_NUM);
        port->snk_src_pdo_count = count;
        COPY_RAW(msg->msg.ext_message.body.epr_pdos, port->src_pdos,
                 sizeof(UCPD_SRC_PDO) * port->snk_src_pdo_count);
      }

      msg->msg.header.extended = 0u;

       TRACE_PrintSrcPDOs(port->src_pdos, port->snk_src_pdo_count);
    }

    pe_prl_cad->pps_periodic_expired = UCPD_FALSE;

    UCPD_DPM_EvaluateCapabilities(port_number);

    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_SELECT_CAPABILITY);

    break;
  }

  case PE_SNK_STATE_SELECT_CAPABILITY: {
    // TODO: take response form DPM and pass it to prl
    // make the DPM response to a request data for prl
    // this will be a Request message

    uint8_t pdo_index =
        pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.body.snk_rdo.object_pos -
        1u;



    // I do not see a reason we cannot enter PPS contract in EPR mode
    // so just check if we are in EPR mode
    if (pe_prl_cad->epr_mode == UCPD_TRUE) {
      UCPD_CNT_Increment(port_number, UCPD_CNT_MESSAGEID);
      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.body.epr_request.src_pdo =
          port->src_pdos[pdo_index];

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type =
          UCPD_EPR_REQUEST_MSG_ID;

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
          .msg.header.number_of_data_objects = 2u;

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
          .msg.body.epr_request.snk_rdo.epr_mode_capable = 1u;

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.extended = 0u;

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.body.epr_request.snk_rdo.capability_mismatch = 1u;



      pe_prl_cad->txSize =
          sizeof(UCPD_SNK_RDO) + sizeof(UCPD_SRC_PDO) + UCPD_MSG_HEADER_SIZE;

      pe_prl_cad->response_expected = UCPD_TRUE;


    } else {
      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type =
          UCPD_REQUEST_MSG_ID;

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
          .msg.header.number_of_data_objects = 1u;

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
                .msg.body.snk_rdo.epr_mode_capable = 1u;

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.extended = 0u;

      pe_prl_cad->txSize = sizeof(UCPD_SNK_RDO) + UCPD_MSG_HEADER_SIZE;

      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.body.snk_rdo.capability_mismatch = 1u;

      pe_prl_cad->response_expected = UCPD_TRUE;

      if (port->src_pdos[pdo_index].pdo_type == UCPD_PDO_TYPE_APDO &&
          port->src_pdos[pdo_index].APDO.apdo_type == UCPD_APDO_TYPE_PPS)
        pe_prl_cad->pps_contract = UCPD_TRUE;
      else
        pe_prl_cad->pps_contract = UCPD_FALSE;
    }

    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_MESSAGE_FROM_PE);

    break;
  }

  case PE_SNK_STATE_TRANSITION_SINK: {
    UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_STOPPED);
    if (pe_prl_cad->epr_mode == UCPD_TRUE)
      UCPD_TIM_Start(port_number, UCPD_TIM_SINK_EPR_TRANSITION);
    else
      UCPD_TIM_Start(port_number, UCPD_TIM_PS_TRANSITION);
    break;
  }

  case PE_SNK_STATE_READY: {

    pe_prl_cad->ams = UCPD_FALSE;
    pe_prl_cad->explicit_contract = UCPD_TRUE;
    pe_prl_cad->ams_first_sent = UCPD_FALSE;

    if (pe_prl_cad->pps_contract == UCPD_TRUE)
      UCPD_TIM_Start(port_number, UCPD_TIM_SINK_PPS_PERIODIC);
    
    if(pe_prl_cad->epr_mode == UCPD_TRUE)
      UCPD_TIM_Start(port_number, UCPD_TIM_SINK_EPR_KEEP_ALIVE);

    if (port->current_callback != NULL) {
      if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_type ==
          UCPD_PS_RDY_MSG_ID)
        port->current_callback(UCPD_STATUS_SUCCESS);
      else
        port->current_callback(UCPD_STATUS_FAILURE);

      port->current_callback = NULL;
    }
    port->sync_operation = UCPD_FALSE;
    break;
  }

  case PE_SNK_STATE_HARD_RESET: {
    UCPD_CNT_Increment(port_number, UCPD_CNT_HARD_RESET);

    pe_prl_cad->hardResetActive = UCPD_TRUE;
    pe_prl_cad->hrdOriginPE = UCPD_TRUE;

    pe_prl_cad->pps_contract = UCPD_FALSE;
    pe_prl_cad->ams = UCPD_FALSE;
    pe_prl_cad->ams_first_sent = UCPD_FALSE;
    pe_prl_cad->epr_mode = UCPD_FALSE;

    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_HRD_RECEIVED);

    break;
  }

  case PE_SNK_STATE_TRANSITION_TO_DEFAULT: {
    pe_prl_cad->explicit_contract = UCPD_FALSE;
    pe_prl_cad->pps_contract = UCPD_FALSE;
    pe_prl_cad->ams = UCPD_FALSE;
    pe_prl_cad->ams_first_sent = UCPD_FALSE;
    pe_prl_cad->hrdOriginPE = UCPD_FALSE;
    pe_prl_cad->extended_message = UCPD_FALSE;

    // DONE:request dpm power transition to default, whatever that means
    UCPD_DPM_PowerTransitionDefault(port_number);

    // DONE?:reset hardware, whatever that means

    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_STARTUP);
    break;
  }

    /* soft reset states */

  case PE_SNK_STATE_SOFT_RESET: {
    // DONE:request prl to send accept
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type =
        UCPD_ACCEPT_MSG_ID;

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
        .msg.header.number_of_data_objects = 0;

    pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE;

    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_MESSAGE_FROM_PE);
    break;
  }

  case PE_SNK_STATE_NONE: {

    break;
  }

  case PE_SNK_STATE_SEND_SOFT_RESET: {
    // TODO:reset prl
    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_PRL_INIT);

    // DONE:request prl to send soft reset
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type =
        UCPD_SOFT_RESET_MSG_ID;

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
        .msg.header.number_of_data_objects = 0;

    pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE;

    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_MESSAGE_FROM_PE);

    break;
  }

    /* not supported states */

  case PE_SNK_STATE_NOT_SUPPORTED_RECEIVED: {
    // DONE: inform dpm not supported received

    if (UCPD_DPM_InformMessageReceived(port_number))
      UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_READY);

    break;
  }

  case PE_SNK_STATE_SEND_NOT_SUPPORTED: {
    // DONE:tell prl to send not supported message
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type =
        UCPD_NOT_SUPPORTED_MSG_ID;

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
        .msg.header.number_of_data_objects = 0;

    pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE;

    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_MESSAGE_FROM_PE);
    break;
  }

  case PE_SNK_STATE_SEND_EPR_MODE_ENTRY: {
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type =
        UCPD_EPR_MODE_MSG_ID;
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
        .msg.header.number_of_data_objects = 1;
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.body.epr_mdo.action =
        UCPD_EPR_ACTION_ENTER;
    // operational pdp should be set by policy
    pe_prl_cad->response_expected = UCPD_TRUE;
    pe_prl_cad->entering_epr = UCPD_TRUE;

    pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE + sizeof(UCPD_EPRMDO);

    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_MESSAGE_FROM_PE);

    break;
  }

  case PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE: {
    break;
  }

  case PE_SNK_STATE_SEND_EPR_MODE_EXIT: {
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type =
        UCPD_EPR_MODE_MSG_ID;

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
        .msg.header.number_of_data_objects = 1;
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.body.epr_mdo.action =
        UCPD_EPR_ACTION_EXIT;

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.body.epr_mdo.data = 0u;

    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_MESSAGE_FROM_PE);
    break;
  }

  case PE_SNK_STATE_EPR_MODE_EXIT_RECEIVED: {
    pe_prl_cad->epr_mode = UCPD_FALSE;
    UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);
    break;
  }

  case PE_SNK_STATE_EPR_KEEP_ALIVE: {
    /* Build full extended message in EXT TX buffer; PRL will chunk into TX buffer. */
    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX].msg.header.message_type =
        UCPD_EXTENDED_CONTROL_MSG_ID;
    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX].msg.header.extended = 1u;

    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX]
        .msg.ext_message.ext_header.chunk_number = 0u;
    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX]
        .msg.ext_message.ext_header.chunked = 1u;
    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX]
        .msg.ext_message.ext_header.data_size = 2u;
    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX]
        .msg.ext_message.ext_header.request_chunk = 0u;
    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX].msg.header.number_of_data_objects = 1u;

    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX].msg.ext_message.body.epr_ecdb.type =
        UCPD_EPR_KEEP_ALIVE_EXT_MSG_ID;
    pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX].msg.ext_message.body.epr_ecdb.data = 0u;

    pe_prl_cad->response_expected = UCPD_TRUE;

    pe_prl_cad->extended_message = UCPD_TRUE;

    /* Let PRL compute txSize from per-chunk assembly. */
    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_MESSAGE_FROM_PE);
    break;
  }

#if 0
  case PE_SNK_STATE_GET_SOURCE_CAPABILITIES: {
    // DONE: ask prl to send get source cap message
    prl_sm->ctrlMsgID = UCPD_GET_SOURCE_CAP_MSG_ID;
    pe_sm->response_expected = UCPD_TRUE;

    UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PASS_DOWN_MESSAGE);

    UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_RUNNING);

    break;
  }

        case PE_SNK_STATE_GIVE_SINK_CAPABILITIES:
        {
            //DONE:get capabilities from dpm
            //TODO: make capabilities into data for prl
            UCPD_DPM_GetCurrentCapabilities(port_number);


            //DONE:send capabilities to prl

            prl_sm->dataMsgID = UCPD_SOURCE_CAPABILITIES_MSG_ID;
            //TODO: check if this is correct
            pe_sm->response_expected = UCPD_FALSE;


            UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PASS_DOWN_MESSAGE);

            break;
        }

        /* alert received states */

        case PE_SNK_STATE_SOURCE_ALERT_RECEIVED:
        {
            //DONE:inform dpm alert received
            if(UCPD_DPM_InformMessageReceived(port_number))
                break;

            if(/*TODO:check for status request*/1u)
                UCPD_PE_SM_SNK_Enter(port_number, PE_STATE_GET_STATUS);
            else
                UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_READY);

            break;
        }

        /* alert send states */

        case PE_SNK_STATE_SEND_SINK_ALERT:
        {
            //DONE:tell prl to send alert
            prl_sm->dataMsgID = UCPD_ALERT_MSG_ID;

            UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PASS_DOWN_MESSAGE);
            break;
        }

        case PE_SNK_STATE_WAIT_FOR_GET_STATUS:
        {
            UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_RUNNING);

            break;
        }

        /* get source ext caps states */
#if 1
        case PE_SNK_STATE_GET_SOURCE_CAP_EXT:
        {
            //not supported 
            break;
            //tell prl to send get source cap ext
            prl_sm->ctrlMsgID = UCPD_GET_SOURCE_CAP_EXT_MSG_ID;

            UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_RUNNING);

            UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PREPARE_TO_SEND_CHUNKED_MESSAGE);

            break;
        }
#endif


        /* sink ext cap states */
        
        case PE_SNK_STATE_GIVE_SINK_CAP_EXT:
        {
            //DONE:get ext caps from dpm
            //TODO: make ext caps into data for prl
            UCPD_DPM_GetCurrentExtendedCapabilities(port_number);

            //DONE:tell prl to send ext caps
            prl_sm->extMsgID = UCPD_SINK_CAPABILITIES_EXT_MSG_ID;

            UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PREPARE_TO_SEND_CHUNKED_MESSAGE);

            break;
        }

        /* source info states */
#if 1
        case PE_SNK_STATE_GET_SOURCE_INFO:
        {
            //DONE:tell prl to send get src info msg
            prl_sm->ctrlMsgID = UCPD_GET_SOURCE_INFO_MSG_ID;

            UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_RUNNING);

            UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PASS_DOWN_MESSAGE);

            break;
        }
#endif

#if 0
        /* get status states */

        case PE_STATE_GET_STATUS:
        {
            //tell prl to send get status
            pe_prl->prl_sm.ctrlMsgID = UCPD_GET_STATUS_MSG_ID;

            UCPD_PE_SM_SRT_Enter(pe_prl, PE_SRT_STATE_RUNNING);

            UCPD_PRL_SM_CHUNKED_TX_Enter(pe_prl, PRL_TCH_STATE_PREPARE_TO_SEND_CHUNKED_MESSAGE);

            break;
        }
#endif
        /* give status states */

#if 0
        case PE_STATE_GIVE_STATUS:
        {
            //get status from dpm
            //tell prl to send status
            pe_prl->prl_sm.extMsgID = UCPD_STATUS_MSG_ID;

            UCPD_PRL_SM_CHUNKED_TX_Enter(pe_prl, PRL_TCH_STATE_PREPARE_TO_SEND_CHUNKED_MESSAGE);

            break;
        }
#endif
        /* get pps status states */

        case PE_SNK_STATE_GET_PPS_STATUS:
        {
            //DONE:
            // tell prl to send get pps status msg
            prl_sm->ctrlMsgID = UCPD_GET_PPS_STATUS_MSG_ID;

            UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_RUNNING);

            UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PASS_DOWN_MESSAGE);

            break;
        }

        /* get rev states */

        case PE_STATE_GET_REVISION:
        {
            //DONE:tell prl to send get revision
            prl_sm->ctrlMsgID = UCPD_GET_REVISION_MSG_ID;

            UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PASS_DOWN_MESSAGE);

            break;
        }

        /* give revision states */

        case PE_STATE_GIVE_REVISION:
        {
            //DONE:get revision from dpm
            //TODO: make revision into data for prl
            UCPD_DPM_GetRevision(port_number);

            //DONE:tell prl to send revision
            prl_sm->dataMsgID = UCPD_REVISION_MSG_ID;

            UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_PASS_DOWN_MESSAGE);
            break;
        }
        
        /* bist states */

        case PE_STATE_BIST_CARRIER_MODE:
        {
            //DONE:tell prl to enter bist carrier mode

            UCPD_TIM_Reset(port_number, UCPD_TIM_BIST_CONT_MODE);
            UCPD_TIM_Start(port_number, UCPD_TIM_BIST_CONT_MODE);

            break;
        }

        case PE_STATE_BIST_TEST_MODE:
        {
            //TODO:laaaaaaaaaaaaaaaaaaaaaater tell prl to enter bist test mode

            break;
        }
#endif
  default: {
    break;
  }
  }
}

void UCPD_PE_SM_SNK_Exit(UCPD_PORT_Number port_number,
                         UCPD_PE_SM_SNK_State state) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_PE_PRL_CAD_Module *pe_prl_cad = &port->pe_prl_cad;

  switch (state) {
  case PE_SNK_STATE_EVALUATE_CAPABILITY: {
    UCPD_TIM_Stop(port_number, UCPD_TIM_SINK_WAIT_CAP);

    break;
  }

  case PE_SNK_STATE_TRANSITION_SINK: {
    // request dpm to transition to new power
    UCPD_TIM_Stop(port_number, UCPD_TIM_PS_TRANSITION);

    break;
  }

  case PE_SNK_STATE_READY: {
    // notify prl of ams if initiated
    pe_prl_cad->ams = UCPD_TRUE;
    if (pe_prl_cad->pps_contract == UCPD_TRUE) {
      pe_prl_cad->pps_contract = UCPD_FALSE;
      UCPD_TIM_Stop(port_number, UCPD_TIM_SINK_PPS_PERIODIC);
    }

    break;
  }
  case PE_SNK_STATE_TRANSITION_TO_DEFAULT: {
    UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_PE_HARD_RESET_COMPLETE);
    break;
  }

  case PE_SNK_STATE_SEND_EPR_MODE_ENTRY: {
    break;
  }

  case PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE: {
    pe_prl_cad->epr_mode = UCPD_TRUE;
    port->sync_operation = UCPD_FALSE;
    if (port->current_callback != NULL) {
      port->current_callback(UCPD_STATUS_SUCCESS);
      port->current_callback = NULL;
    }
    UCPD_TIM_Stop(port_number, UCPD_TIM_SINK_EPR_ENTER);
    break;
  }

  case PE_SNK_STATE_SEND_EPR_MODE_EXIT: {
    pe_prl_cad->epr_mode = UCPD_FALSE;
    port->sync_operation = UCPD_FALSE;
    if (port->current_callback != NULL) {
      port->current_callback(UCPD_STATUS_SUCCESS);
      port->current_callback = NULL;
    }
    break;
  }

  case PE_SNK_STATE_EPR_MODE_EXIT_RECEIVED: {

    break;
  }

  case PE_SNK_STATE_EPR_KEEP_ALIVE: {
    break;
  }
#if 0
        case PE_SNK_STATE_GET_SOURCE_CAPABILITIES:
        {
            //pass source caps to dpm or nothing

            break;
        }

        case PE_SNK_STATE_TRANSITION_TO_DEFAULT:
        {
            //tell prl hard reset complete

            break;
        }

        case PE_SNK_STATE_GET_SOURCE_CAP_EXT:
        {
            //pass source caps ext to dpm or nothing

            break;
        }

        case PE_SNK_STATE_GET_SOURCE_INFO:
        {
            //pass source info to dpm or nothing

            break;
        }

        case PE_STATE_GET_STATUS:
        {
            //pass status to dpm or nothing

            break;
        }

        case PE_SNK_STATE_GET_PPS_STATUS:
        {
            //pass pps status to dpm or nothing

            break;
        }

        case PE_STATE_GET_REVISION:
        {
            //pass revision to dpm or nothing

            break;
        }
#endif
  default: {
    break;
  }
  };
}
