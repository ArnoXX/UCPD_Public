#include "ucpd_prl_sm.h"
#include "device/trace_utils.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd/ucpd_pe_prl.h"
#include "ucpd/ucpd_port.h"
#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_tim.h"
#include "ucpd_cnt.h"
#include "ucpd_ctx.h"
#include "ucpd_msg.h"
#include "ucpd_utils.h"

void UCPD_PRL_SM_Delta(UCPD_PORT_Number port_number,
                       UCPD_PE_PRL_CAD_Event event) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_PE_PRL_CAD_Module *pe_prl_cad = &port->pe_prl_cad;

  switch (event) {
  case PE_PRL_EVENT_PRL_INIT: {
    UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_NONE);
    UCPD_PRL_SM_CHUNKED_ROUTER_Enter(port_number,
                                     PRL_RTR_STATE_WAIT_FOR_MESSAGE_FROM_RRL);
    UCPD_PRL_SM_CHUNKED_RX_Enter(port_number,
                                 PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL);
    UCPD_PRL_SM_CHUNKED_TX_Enter(
        port_number, PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE);
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST);
    UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);

    UCPD_CNT_Reset(port_number, UCPD_CNT_MESSAGEID);
    UCPD_CNT_Reset(port_number, UCPD_CNT_RETRY);

    pe_prl_cad->hrdOriginPE = UCPD_FALSE;
    pe_prl_cad->storedMID = 0u;
    pe_prl_cad->midStored = UCPD_FALSE;

    CLEAR_STRUCT_PTR(pe_prl_cad->buffers + UCPD_TX_BUFFER_INDEX);
    CLEAR_STRUCT_PTR(pe_prl_cad->buffers + UCPD_RX_BUFFER_INDEX);

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.port_data_role =
        UCPD_PORT_DATA_ROLE_UFP;
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.spec_revision =
        UCPD_SPEC_REVISION_3_0;
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.port_power_role =
        UCPD_PORT_POWER_ROLE_SINK;

    TRACE_DEBUG("PRL RESET DONE\n\r");

    break;
  }

  // Tx events
  case PE_PRL_EVENT_MSG_TX_DISCARDED: { // done

    if (pe_prl_cad->rx_state == PRL_RX_STATE_SEND_GOODCRC)
      UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);
    else if (pe_prl_cad->tx_state == PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE) {
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CHECK_RETRY_COUNTER);
    }
    break;
  }

  case PE_PRL_EVENT_MSG_TX_COMPLETED: { // done
    if (pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type ==
            UCPD_GOODCRC_MSG_ID &&
        pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
                .msg.header.number_of_data_objects == 0u) {
      // TRACE_DEBUG("SENT GOODCRC\n\r");
      UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_CHECK_MESSAGE_ID);
    }

    else {

      UCPD_TIM_Start(port_number, UCPD_TIM_CRC_RECEIVE);
    }

    break;
  }

  case PE_PRL_EVENT_TX_OK: { // done
    if (pe_prl_cad->tx_state == PRL_TX_STATE_SNK_PENDING) {
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CONSTRUCT_MESSAGE);
    }

    break;
  }

  // Rx events
  case PE_PRL_EVENT_MSG_RX_COMPLETE: { // done

    if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]
                .msg.header.number_of_data_objects != 0u ||
        pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_type !=
            UCPD_GOODCRC_MSG_ID) {
      if (pe_prl_cad->tx_state == PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE) {
        // unexpected message
        // not sure what to put here for now
        TRACE_ERROR("EXPECTED GOODCRC, RECEIVED ANOTHER MESSAGE\n\r");
        break;
      }

      if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_type ==
          UCPD_SOFT_RESET_MSG_ID)
        UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_LAYER_RESET_FOR_RECEIVE);
      else
        UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_SEND_GOODCRC);

    } else {
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_MATCH_MESSAGE_ID);
    }

    break;
  }

  // HRD events
  case PE_PRL_EVENT_HRD_RECEIVED: { // done
    UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_RESET_LAYER);
    break;
  }

  case PE_PRL_EVENT_HRD_SENT: { // done
    UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_PHY_HARD_RESET_REQUESTED);
    break;
  }

  // Internal events - PRL delta will be called directly from PE delta
  case PE_PRL_EVENT_MESSAGE_FROM_PE: {
    /* Decide path based on whether PE prepared an extended message in EXT TX
     * buffer. */

    if (pe_prl_cad->extended_message == UCPD_TRUE) {
      pe_prl_cad->extended_message = UCPD_FALSE;
      /* Extended message present: always use chunked TX path (single or
       * multi-chunk). */
      if (pe_prl_cad->chunked_rx_state ==
          PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL) {
        UCPD_PRL_SM_CHUNKED_TX_Enter(
            port_number, PRL_TCH_STATE_PREPARE_TO_SEND_CHUNKED_MESSAGE);
      } else {
        UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_REPORT_ERROR);
      }
    } else {
      /* Non-extended: pass down directly. */
      UCPD_PRL_SM_CHUNKED_TX_Enter(port_number,
                                   PRL_TCH_STATE_PASS_DOWN_MESSAGE);
    }

    break;
  }

  case PE_PRL_EVENT_AMS_FIRST_FROM_PE: { // done
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_SNK_START_OF_AMS);
    break;
  }

  case PE_PRL_EVENT_SOFT_RESET_FROM_PE: { // done
    UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_LAYER_RESET_FOR_TRANSMIT);
    UCPD_PRL_SM_CHUNKED_TX_Enter(
        port_number, PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE);
    UCPD_PRL_SM_CHUNKED_RX_Enter(port_number,
                                 PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL);
    UCPD_PRL_SM_CHUNKED_ROUTER_Enter(port_number,
                                     PRL_RTR_STATE_WAIT_FOR_MESSAGE_FROM_RRL);

    break;
  }

  // Timer events
  case PE_PRL_EVENT_CRC_RX_TIMEOUT: { // done
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CHECK_RETRY_COUNTER);
    break;
  }

  case PE_PRL_EVENT_SENDER_REQ_TIMER_TIMEOUT: { // done
    if (pe_prl_cad->chunk_number_to_send - 1u == 0u)
      UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_MESSAGE_SENT);
    else
      UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_REPORT_ERROR);
    break;
  }

  case PE_PRL_EVENT_CHUNK_RES_TIMER_TIMEOUT: { // done
    UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_REPORT_ERROR);
    break;
  }

  case PE_PRL_EVENT_HARD_RESET_TIMEOUT: { // done
    // just in case timer expires and the hard reset is sent at the same time
    if (pe_prl_cad->phy_event == PE_PRL_EVENT_HRD_SENT)
      pe_prl_cad->phy_event = PE_PRL_CAD_EVENT_NONE;

    UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_PHY_HARD_RESET_REQUESTED);
    break;
  }

  default: {
    break;
  }
  }
}

/* State change handlers */
void UCPD_PRL_SM_RX_Enter(UCPD_PORT_Number port_number,
                          UCPD_PRL_SM_RX_State state) {
  UCPD_PE_PRL_CAD_Module *pe_prl_cad =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;

  pe_prl_cad->rx_state = state;

  switch (state) {
  case PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE: {

    break;
  }

  case PRL_RX_STATE_LAYER_RESET_FOR_RECEIVE: {
    UCPD_CNT_Reset(port_number, UCPD_CNT_MESSAGEID);
    pe_prl_cad->tx_state = PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST;
    pe_prl_cad->midStored = UCPD_FALSE;
    pe_prl_cad->storedMID = 0u;

    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST);

    UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_SEND_GOODCRC);

    break;
  }

  case PRL_RX_STATE_SEND_GOODCRC: {
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_type =
        UCPD_GOODCRC_MSG_ID;

    if(pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_type == 17u && pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.number_of_data_objects == 7u)
    {
      int a = 0;
    }
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_id =
        pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_id;

    

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX]
        .msg.header.number_of_data_objects = 0u;

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.extended = 0u;

    for (uint8_t i = 2; i < UCPD_EPR_MESSAGE_LEN - UCPD_MSG_HEADER_SIZE; i++)
      pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].buffer[i] = 0u;

    pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE;

    UCPD_PHY_SendMessage(port_number);
    break;
  }

  case PRL_RX_STATE_CHECK_MESSAGE_ID: {
    if (pe_prl_cad->midStored == UCPD_FALSE ||
        pe_prl_cad->storedMID !=
            pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_id)
      UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_STORE_MESSAGE_ID);
    else
      UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);

    break;
  }

  case PRL_RX_STATE_STORE_MESSAGE_ID: {
    pe_prl_cad->tx_state = PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST;

    UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_DISCARD_MESSAGE);

    pe_prl_cad->storedMID =
        pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_id;
    pe_prl_cad->midStored = UCPD_TRUE;

    // inform higher level message received
    if (pe_prl_cad->chunked_tx_state !=
        PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE)
      UCPD_PRL_SM_CHUNKED_ROUTER_Enter(port_number, PRL_RTR_STATE_TX_CHUNKS);
    else
      UCPD_PRL_SM_CHUNKED_ROUTER_Enter(port_number, PRL_RTR_STATE_RX_CHUNKS);

    break;
  }

  default: {
    break;
  }
  };
}

void UCPD_PRL_SM_TX_Enter(UCPD_PORT_Number port_number,
                          UCPD_PRL_SM_TX_State state) {
  UCPD_PE_PRL_CAD_Module *pe_prl_cad =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;

  pe_prl_cad->tx_state = state;

  switch (state) {
  case PRL_TX_STATE_PHY_LAYER_RESET: {

    break;
  }

  case PRL_TX_STATE_DISCARD_MESSAGE: {
    // UCPD_CNT_Increment(port_number, UCPD_CNT_MESSAGEID);
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST);

    break;
  }

  case PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST: {
    UCPD_CNT_Reset(port_number, UCPD_CNT_RETRY);

    break;
  }

  case PRL_TX_STATE_LAYER_RESET_FOR_TRANSMIT: {
    UCPD_CNT_Reset(port_number, UCPD_CNT_MESSAGEID);
    UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);

    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CONSTRUCT_MESSAGE);

    break;
  }

  case PRL_TX_STATE_SNK_START_OF_AMS: {

    break;
  }

  case PRL_TX_STATE_SNK_PENDING: {

    if (UCPD_PHY_IsSinkTxOk(port_number) == UCPD_TRUE) {
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CONSTRUCT_MESSAGE);
    }

    break;
  }

  case PRL_TX_STATE_CONSTRUCT_MESSAGE: {

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.message_id =
        UCPD_CNT_Get(port_number, UCPD_CNT_MESSAGEID);

    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.port_data_role =
        UCPD_PORT_DATA_ROLE_UFP;
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.spec_revision =
        UCPD_SPEC_REVISION_3_0;
    pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX].msg.header.port_power_role =
        UCPD_PORT_POWER_ROLE_SINK;

    // txsize and payload are set by PE or chunked TX state machine

    UCPD_PHY_SendMessage(port_number);

    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE);

    break;
  }

  case PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE: {
    // crc receive is started when hw signals that message is sent, not here
    break;
  }

  case PRL_TX_STATE_MATCH_MESSAGE_ID: {

    UCPD_TIM_Stop(port_number, UCPD_TIM_CRC_RECEIVE);
    if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.message_id ==
        UCPD_CNT_Get(port_number, UCPD_CNT_MESSAGEID))
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_MESSAGE_SENT);
    else
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CHECK_RETRY_COUNTER);

    break;
  }

  case PRL_TX_STATE_CHECK_RETRY_COUNTER: {
    UCPD_CNT_Increment(port_number, UCPD_CNT_RETRY);
    if (UCPD_CNT_Get(port_number, UCPD_CNT_RETRY) <= UCPD_CNT_N_RETRY)
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CONSTRUCT_MESSAGE);

    else
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_TRANSMISSION_ERROR);

    break;
  }

  case PRL_TX_STATE_TRANSMISSION_ERROR: {
    UCPD_CNT_Increment(port_number, UCPD_CNT_MESSAGEID);
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST);

    UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_REPORT_ERROR);

    break;
  }

  case PRL_TX_STATE_MESSAGE_SENT: {
    UCPD_CNT_Increment(port_number, UCPD_CNT_MESSAGEID);
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST);

    // inform PE of message sent
    if (pe_prl_cad->chunked_rx_state == PRL_RCH_STATE_REQUESTING_CHUNK)
      UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_WAITING_CHUNK);
    else if (pe_prl_cad->chunked_tx_state ==
             PRL_TCH_STATE_SENDING_CHUNKED_MESSAGE) {
      if (pe_prl_cad->lastChunk == UCPD_TRUE)
        UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_MESSAGE_SENT);
      else
        UCPD_PRL_SM_CHUNKED_TX_Enter(port_number,
                                     PRL_TCH_STATE_WAIT_CHUNK_REQUEST);
    } else if (pe_prl_cad->chunked_tx_state ==
               PRL_TCH_STATE_WAIT_FOR_TRANSMISSION_COMPLETE)
      UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_MESSAGE_SENT);

    break;
  }

  default: {
    break;
  }
  };
}

void UCPD_PRL_SM_CHUNKED_RX_Enter(UCPD_PORT_Number port_number,
                                  UCPD_PRL_SM_CHUNKED_RX_State state) {
  UCPD_PE_PRL_CAD_Module *pe_prl_cad =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;

  pe_prl_cad->chunked_rx_state = state;

  switch (state) {
  case PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL: {

    CLEAR_STRUCT_PTR(&pe_prl_cad->buffers[UCPD_EXT_RX_BUFFER_INDEX]);

    break;
  }

  case PRL_RCH_STATE_REPORT_ERROR: {
    // inform PE of error
    // pass message if there is any - add later
    UCPD_PRL_SM_CHUNKED_RX_Enter(port_number,
                                 PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL);
    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_TRANSMISSION_ERROR);

    break;
  }

  case PRL_RCH_STATE_PROCESSING_EXTENDED_MESSAGE: {

    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_STOP_SRT);
    UCPD_TIM_Stop(port_number, UCPD_TIM_CHUNK_SENDER_RESPONSE);

    if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]
            .msg.ext_message.ext_header.chunk_number == 0u) {
      pe_prl_cad->chunk_number_expected = 0u;
      pe_prl_cad->num_bytes_received = 0u;

      CLEAR_STRUCT_PTR(&pe_prl_cad->buffers[UCPD_EXT_RX_BUFFER_INDEX]);

      // copies the headers and the first DO, first copy in the next if is
      // redundant but its alright
      pe_prl_cad->buffers[UCPD_EXT_RX_BUFFER_INDEX] =
          pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX];
    }

    if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]
            .msg.ext_message.ext_header.chunk_number ==
        pe_prl_cad->chunk_number_expected) {

      // black magic, even gonna keep the curly braces

      uint8_t remaining = pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]
                              .msg.ext_message.ext_header.data_size -
                          pe_prl_cad->num_bytes_received;

      uint8_t bytes_to_copy = remaining > UCPD_MAX_EXT_MSG_CHUNK_LEN
                                  ? UCPD_MAX_EXT_MSG_CHUNK_LEN
                                  : remaining;

      COPY_RAW(pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].buffer + 4u,
               pe_prl_cad->buffers[UCPD_EXT_RX_BUFFER_INDEX].buffer +
                   pe_prl_cad->num_bytes_received + 4u,
               bytes_to_copy);

      pe_prl_cad->num_bytes_received += bytes_to_copy;
      pe_prl_cad->chunk_number_expected += 1u;

    } else {
      UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_REPORT_ERROR);
      break;
    }

    if (pe_prl_cad->num_bytes_received >=
        pe_prl_cad->buffers[UCPD_EXT_RX_BUFFER_INDEX]
            .msg.ext_message.ext_header.data_size) {
      // copy message from ext buffer to rx buffer

      COPY_RAW(pe_prl_cad->buffers[UCPD_EXT_RX_BUFFER_INDEX].buffer,
               pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].buffer,
               sizeof(pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]));

      UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_PASS_UP_MESSAGE);
    }

    else
      UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_REQUESTING_CHUNK);

    break;
  }

  case PRL_RCH_STATE_REQUESTING_CHUNK: {
    // stop srt

    // construct and pass to prl a chunk request message

    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_STOP_SRT);

    UCPD_MSG *msg = &pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX];
    UCPD_MSG *ext_msg = &pe_prl_cad->buffers[UCPD_EXT_RX_BUFFER_INDEX];

    msg->msg.header.message_type = ext_msg->msg.header.message_type;
    msg->msg.header.extended = 1u;
    msg->msg.header.number_of_data_objects = 1u;
    msg->msg.ext_message.ext_header = ext_msg->msg.ext_message.ext_header;

    msg->msg.ext_message.ext_header.chunk_number =
        pe_prl_cad->chunk_number_expected;
    msg->msg.ext_message.ext_header.request_chunk = 1u;
    msg->msg.ext_message.ext_header.data_size = 0u;
    msg->msg.ext_message.ext_header.chunked = 1u;

    // make sure the last two bytes are 0
    msg->buffer[UCPD_MSG_HEADER_SIZE + UCPD_MSG_EXT_HEADER_SIZE] = 0u;
    msg->buffer[UCPD_MSG_HEADER_SIZE + UCPD_MSG_EXT_HEADER_SIZE + 1u] = 0u;

    pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE + UCPD_MSG_EXT_HEADER_SIZE + 2u;

    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CONSTRUCT_MESSAGE);

    break;
  }

  case PRL_RCH_STATE_WAITING_CHUNK: {

    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_START_SRT);
    UCPD_TIM_Start(port_number, UCPD_TIM_CHUNK_SENDER_RESPONSE);

    break;
  }

  case PRL_RCH_STATE_PASS_UP_MESSAGE: {

    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_MESSAGE_RECEIVED);

    UCPD_PRL_SM_CHUNKED_RX_Enter(port_number,
                                 PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL);

    break;
  }

  default: {
    break;
  }
  };
}

void UCPD_PRL_SM_CHUNKED_ROUTER_Enter(UCPD_PORT_Number port_number,
                                      UCPD_PRL_SM_CHUNKED_ROUTER_State state) {
  UCPD_PE_PRL_CAD_Module *pe_prl_cad =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;

  switch (state) {
  case PRL_RTR_STATE_WAIT_FOR_MESSAGE_FROM_RRL: {

    break;
  }

  case PRL_RTR_STATE_RX_CHUNKS: {
    // here be black magic

    if (pe_prl_cad->chunked_rx_state != PRL_RCH_STATE_WAITING_CHUNK &&
        pe_prl_cad->chunked_rx_state != PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL)
      UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_REPORT_ERROR);

    else if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.extended ==
             0u) {
      if (pe_prl_cad->chunked_rx_state == PRL_RCH_STATE_WAITING_CHUNK ||
          pe_prl_cad->chunked_rx_state == PRL_RCH_STATE_REQUESTING_CHUNK)
        UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_REPORT_ERROR);
      else
        UCPD_PRL_SM_CHUNKED_RX_Enter(port_number,
                                     PRL_RCH_STATE_PASS_UP_MESSAGE);
    } else {
      if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]
              .msg.ext_message.ext_header.chunked == 0u)
        UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_REPORT_ERROR);

      else if (pe_prl_cad->chunked_rx_state ==
                   PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL ||
               pe_prl_cad->chunked_rx_state == PRL_RCH_STATE_WAITING_CHUNK)

        UCPD_PRL_SM_CHUNKED_RX_Enter(port_number,
                                     PRL_RCH_STATE_PROCESSING_EXTENDED_MESSAGE);
    }

    UCPD_PRL_SM_CHUNKED_ROUTER_Enter(port_number,
                                     PRL_RTR_STATE_WAIT_FOR_MESSAGE_FROM_RRL);

    break;
  }

  case PRL_RTR_STATE_TX_CHUNKS: {
    // here be slightly less black magic

    if (pe_prl_cad->chunked_tx_state != PRL_TCH_STATE_WAIT_CHUNK_REQUEST)
      UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_MESSAGE_RECEIVED);
    else if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX].msg.header.extended ==
                 1u &&
             pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]
                     .msg.ext_message.ext_header.request_chunk == 1u) {
      if (pe_prl_cad->buffers[UCPD_RX_BUFFER_INDEX]
              .msg.ext_message.ext_header.chunk_number ==
          pe_prl_cad->chunk_number_to_send) {
        UCPD_PRL_SM_CHUNKED_TX_Enter(port_number,
                                     PRL_TCH_STATE_CONSTRUCT_CHUNKED_MESSAGE);
      } else
        UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_REPORT_ERROR);
    } else {
      UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_MESSAGE_RECEIVED);
    }

    UCPD_PRL_SM_CHUNKED_ROUTER_Enter(port_number,
                                     PRL_RTR_STATE_WAIT_FOR_MESSAGE_FROM_RRL);

    break;
  }

  default: {
    break;
  }
  };
}

void UCPD_PRL_SM_CHUNKED_TX_Enter(UCPD_PORT_Number port_number,
                                  UCPD_PRL_SM_CHUNKED_TX_State state) {
  UCPD_PE_PRL_CAD_Module *pe_prl_cad =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;

  pe_prl_cad->chunked_tx_state = state;
  switch (state) {
  case PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE: {

    break;
  }

  case PRL_TCH_STATE_PASS_DOWN_MESSAGE: {
    // tell PRL that there is a message request
    // the message should be mostly constructed by PE

    UCPD_PRL_SM_CHUNKED_TX_Enter(port_number,
                                 PRL_TCH_STATE_WAIT_FOR_TRANSMISSION_COMPLETE);

    if (pe_prl_cad->tx_state == PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST)
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CONSTRUCT_MESSAGE);
    else
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_SNK_PENDING);

    break;
  }

  case PRL_TCH_STATE_PREPARE_TO_SEND_CHUNKED_MESSAGE: {

    pe_prl_cad->chunk_number_to_send = 0u;
    pe_prl_cad->lastChunk = UCPD_FALSE;

    UCPD_PRL_SM_CHUNKED_TX_Enter(port_number,
                                 PRL_TCH_STATE_CONSTRUCT_CHUNKED_MESSAGE);
    break;
  }

  case PRL_TCH_STATE_MESSAGE_RECEIVED: {
    CLEAR_STRUCT_PTR(&pe_prl_cad->buffers[UCPD_EXT_RX_BUFFER_INDEX]);

    UCPD_PRL_SM_CHUNKED_TX_Enter(
        port_number, PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE);

    UCPD_PRL_SM_CHUNKED_ROUTER_Enter(port_number, PRL_RTR_STATE_RX_CHUNKS);

    break;
  }

  case PRL_TCH_STATE_WAIT_FOR_TRANSMISSION_COMPLETE: {
    // nothing to see here

    break;
  }

  case PRL_TCH_STATE_MESSAGE_SENT: {
    // tell PE the job's done

    UCPD_PRL_SM_CHUNKED_TX_Enter(
        port_number, PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE);

    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_MESSAGE_SENT);

    break;
  }

  case PRL_TCH_STATE_REPORT_ERROR: {
    // tell PE the job's not done

    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_TRANSMISSION_ERROR);

    UCPD_PRL_SM_CHUNKED_TX_Enter(
        port_number, PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE);

    break;
  }

  case PRL_TCH_STATE_CONSTRUCT_CHUNKED_MESSAGE: {
    // construct the next extended chunk from the TX buffer
    UCPD_MSG *src = &pe_prl_cad->buffers[UCPD_EXT_TX_BUFFER_INDEX];
    UCPD_MSG *tx = &pe_prl_cad->buffers[UCPD_TX_BUFFER_INDEX];

    // requires extended message prepared in dedicated buffer - done by PE
    uint16_t total = src->msg.ext_message.ext_header.data_size;
    uint16_t bytes_sent = (uint16_t)(pe_prl_cad->chunk_number_to_send *
                                     UCPD_MAX_EXT_MSG_CHUNK_LEN);
    uint16_t remaining = (uint16_t)(total - bytes_sent);

    uint16_t chunk_ndo =
        remaining >= UCPD_MAX_EXT_MSG_CHUNK_LEN
            ? 7u
            : ((remaining + UCPD_MSG_EXT_HEADER_SIZE + 3u) >> 2u);

    uint16_t padding = (remaining + UCPD_MSG_EXT_HEADER_SIZE) % 4;
    uint16_t bytes_to_copy = remaining >= UCPD_MAX_EXT_MSG_CHUNK_LEN
                                 ? UCPD_MAX_EXT_MSG_CHUNK_LEN
                                 : remaining;

    // set extended header for this chunk - just copy the header and ext header
    // from the ext tx buffer
    tx->msg.header = src->msg.header;
    tx->msg.ext_message.ext_header = src->msg.ext_message.ext_header;

    tx->msg.header.number_of_data_objects = chunk_ndo;
    tx->msg.ext_message.ext_header.chunk_number =
        pe_prl_cad->chunk_number_to_send;

    // Copy chunk payload from ext TX buffer into the start of body for TX
    COPY_RAW(((uint8_t *)&src->msg.ext_message.body) + bytes_sent,
             &tx->msg.ext_message.body, bytes_to_copy);

    // zero pad bytes after payload to reach 4-byte boundary
    for (uint16_t i = 0; i < padding; i++)
      ((uint8_t *)&tx->msg.ext_message.body)[i + bytes_to_copy] = 0u;

    pe_prl_cad->lastChunk =
        (bytes_sent + bytes_to_copy) >= total ? UCPD_TRUE : UCPD_FALSE;
    pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE + UCPD_MSG_EXT_HEADER_SIZE +
                         bytes_to_copy + padding;

    UCPD_PRL_SM_CHUNKED_TX_Enter(port_number,
                                 PRL_TCH_STATE_SENDING_CHUNKED_MESSAGE);

    if (pe_prl_cad->tx_state == PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST) {
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CONSTRUCT_MESSAGE);
    } else {
      // basically the same as pass down message
      // pend transmission and wait for TX_OK to actually start sending
      UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_SNK_PENDING);
    }

    break;
  }

  case PRL_TCH_STATE_SENDING_CHUNKED_MESSAGE: {
    // waiting for prl to transmit

    break;
  }

  case PRL_TCH_STATE_WAIT_CHUNK_REQUEST: {

    pe_prl_cad->chunk_number_to_send++;

    UCPD_TIM_Start(port_number, UCPD_TIM_CHUNK_SENDER_REQUEST);

    break;
  }

  default: {
    break;
  }
  };
}

void UCPD_PRL_SM_HRD_Enter(UCPD_PORT_Number port_number,
                           UCPD_PRL_SM_HR_State state) {
  UCPD_PE_PRL_CAD_Module *pe_prl_cad =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;

  pe_prl_cad->hrd_state = state;

  switch (state) {
  case PRL_HR_STATE_RESET_LAYER: {
    UCPD_CNT_Reset(port_number, UCPD_CNT_MESSAGEID);
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST);
    UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);

    if (pe_prl_cad->hrdOriginPE == UCPD_FALSE)
      UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_INDICATE_HARD_RESET);
    else
      UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_REQUEST_HARD_RESET);

    break;
  }

  case PRL_HR_STATE_REQUEST_HARD_RESET: {
    UCPD_PHY_SendHardReset(port_number);

    UCPD_PRL_SM_HRD_Enter(port_number,
                          PRL_HR_STATE_WAIT_FOR_PHY_HARD_RESET_COMPLETE);

    break;
  }

  case PRL_HR_STATE_WAIT_FOR_PHY_HARD_RESET_COMPLETE: {
    UCPD_TIM_Start(port_number, UCPD_TIM_HARD_RESET_COMPLETE);

    break;
  }

  case PRL_HR_STATE_PHY_HARD_RESET_REQUESTED: {
    UCPD_TIM_Stop(port_number, UCPD_TIM_HARD_RESET_COMPLETE);
    UCPD_PRL_SM_HRD_Enter(port_number,
                          PRL_HR_STATE_WAIT_FOR_PE_HARD_RESET_COMPLETE);

    // tell PE hard reset request has been sent
    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_HARD_RESET_SENT);
    break;
  }

  case PRL_HR_STATE_WAIT_FOR_PE_HARD_RESET_COMPLETE: {
    UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_PE_HARD_RESET_COMPLETE);

    break;
  }

  case PRL_HR_STATE_INDICATE_HARD_RESET: {
    UCPD_PRL_SM_HRD_Enter(port_number,
                          PRL_HR_STATE_WAIT_FOR_PE_HARD_RESET_COMPLETE);

    // inform PE of hard reset
    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PE_HARD_RESET_RECEIVED);
    break;
  }

  case PRL_HR_STATE_PE_HARD_RESET_COMPLETE: {
    UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);
    UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST);
    UCPD_PRL_SM_CHUNKED_ROUTER_Enter(port_number,
                                     PRL_RTR_STATE_WAIT_FOR_MESSAGE_FROM_RRL);
    UCPD_PRL_SM_CHUNKED_RX_Enter(port_number,
                                 PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL);
    UCPD_PRL_SM_CHUNKED_TX_Enter(
        port_number, PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE);

    UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_NONE);

    break;
  }

  default: {
    break;
  }
  };
}
