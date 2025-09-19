#ifndef UCPD_PRL_SM_H
#define UCPD_PRL_SM_H

#include <stdint.h>

#include "ucpd/ucpd_port_num.h"
#include "ucpd_cnt.h"
#include "ucpd_msg.h"
#include "ucpd_utils.h"

/* Protocol layer state machines states*/

/* Protocol layer chunked Rx state machine states */
typedef enum {
  PRL_RCH_STATE_WAIT_FOR_MESSAGE_FROM_PRL,
  PRL_RCH_STATE_REPORT_ERROR,
  PRL_RCH_STATE_PASS_UP_MESSAGE,
  PRL_RCH_STATE_PROCESSING_EXTENDED_MESSAGE,
  PRL_RCH_STATE_REQUESTING_CHUNK,
  PRL_RCH_STATE_WAITING_CHUNK,
  PRL_RCH_STATE_NONE
} UCPD_PRL_SM_CHUNKED_RX_State;

/* Protocol layer chunked Tx state machine states */
typedef enum {
  PRL_TCH_STATE_WAIT_FOR_MESSAGE_REQUEST_FROM_PE,
  PRL_TCH_STATE_PASS_DOWN_MESSAGE,
  PRL_TCH_STATE_WAIT_FOR_TRANSMISSION_COMPLETE,
  PRL_TCH_STATE_MESSAGE_SENT,
  PRL_TCH_STATE_REPORT_ERROR,
  PRL_TCH_STATE_PREPARE_TO_SEND_CHUNKED_MESSAGE,
  PRL_TCH_STATE_CONSTRUCT_CHUNKED_MESSAGE,
  PRL_TCH_STATE_SENDING_CHUNKED_MESSAGE,
  PRL_TCH_STATE_WAIT_CHUNK_REQUEST,
  PRL_TCH_STATE_MESSAGE_RECEIVED,
  PRL_TCH_STATE_NONE
} UCPD_PRL_SM_CHUNKED_TX_State;

/* Protocol layer chunked message router */
typedef enum {
  PRL_RTR_STATE_WAIT_FOR_MESSAGE_FROM_RRL,
  PRL_RTR_STATE_RX_CHUNKS,
  PRL_RTR_STATE_PING,
  PRL_RTR_STATE_TX_CHUNKS,
  PRL_RTR_STATE_NONE
} UCPD_PRL_SM_CHUNKED_ROUTER_State;

/* Protocol layer transmission state machine states */
typedef enum {
  PRL_TX_STATE_PHY_LAYER_RESET,
  PRL_TX_STATE_DISCARD_MESSAGE,
  PRL_TX_STATE_SNK_START_OF_AMS,
  PRL_TX_STATE_SNK_PENDING,
  PRL_TX_STATE_LAYER_RESET_FOR_TRANSMIT,
  PRL_TX_STATE_WAIT_FOR_MESSAGE_REQUEST,
  PRL_TX_STATE_CONSTRUCT_MESSAGE,
  PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE,
  PRL_TX_STATE_MATCH_MESSAGE_ID,
  PRL_TX_STATE_CHECK_RETRY_COUNTER,
  PRL_TX_STATE_TRANSMISSION_ERROR,
  PRL_TX_STATE_MESSAGE_SENT,
  PRL_TX_STATE_NONE
} UCPD_PRL_SM_TX_State;

/* Protocol layer reception state machine states */
typedef enum {
  PRL_RX_STATE_LAYER_RESET_FOR_RECEIVE,
  PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE,
  PRL_RX_STATE_SEND_GOODCRC,
  PRL_RX_STATE_CHECK_MESSAGE_ID,
  PRL_RX_STATE_STORE_MESSAGE_ID,
  PRL_RX_STATE_NONE
} UCPD_PRL_SM_RX_State;

/* Protocol layer hard reset state machine states */
typedef enum {
  PRL_HR_STATE_RESET_LAYER,
  PRL_HR_STATE_REQUEST_HARD_RESET,
  PRL_HR_STATE_INDICATE_HARD_RESET,
  PRL_HR_STATE_WAIT_FOR_PHY_HARD_RESET_COMPLETE,
  PRL_HR_STATE_PHY_HARD_RESET_REQUESTED,
  PRL_HR_STATE_WAIT_FOR_PE_HARD_RESET_COMPLETE,
  PRL_HR_STATE_PE_HARD_RESET_COMPLETE,
  PRL_HR_STATE_NONE
} UCPD_PRL_SM_HR_State;

/* Protocol layer state machine */
typedef struct {

  UCPD_Bool hrd_origin_pe;

  UCPD_PORT_Number port_number;

  // chunking
  uint8_t chunk_number_to_send;
  uint8_t chunk_expected;
  uint8_t chunking;
  uint8_t abort;
  uint8_t error_with_message;

  UCPD_Bool message_not_sent_retries;
  UCPD_Bool unexpected_or_unrecognized_message;

  UCPD_Bool transmisson_error;
  UCPD_Bool last_chunk;
  UCPD_Bool first_chunk;
  uint16_t bytes_received;
  UCPD_Bool chunk_request;

  // message id stuff

  // message buffers
  UCPD_MSG rxBuffers[2];
#if 0
    UCPD_MSG ext_rxBuf;
    UCPD_MSG ext_txBuf;
#endif
  UCPD_MSG txBuffer;
  uint16_t txSize;
  uint8_t
      rxBufInd; // index of the buffer which is ready to accept data on receive

  /* Message request section */
  UCPD_ControlMessageID ctrlMsgID;
  UCPD_DataMessageID dataMsgID;
  UCPD_ExtendedMessageID extMsgID;

  // request message data
  UCPD_Count selected_pdo; // aka object position
  UCPD_Bool cap_mismatch;
  uint32_t operating_current; // 10mA inc - fixed, 50mA inc - APDO
  uint32_t max_current;       // 10mA inc
  uint32_t output_voltage;    // 20mV inc

  // alert data

  // source info data

  // revision data

  // bist data

} UCPD_PRL_SM;

/* Protocol layer state machine functions */
#if 0
/* Initialize the protocol layer state machine */
void UCPD_PRL_SM_Init(UCPD_PRL_SM *sm);

/* Reset the protocol layer state machine */
void UCPD_PRL_SM_Reset(UCPD_PRL_SM *sm);
#endif
/* State machine state enter functions */
/* Enter the protocol layer chunked reception state machine state */
void UCPD_PRL_SM_CHUNKED_RX_Enter(UCPD_PORT_Number port_number,
                                  UCPD_PRL_SM_CHUNKED_RX_State state);

/* Enter the protocol layer chunked transmission state machine state */
void UCPD_PRL_SM_CHUNKED_TX_Enter(UCPD_PORT_Number port_number,
                                  UCPD_PRL_SM_CHUNKED_TX_State state);

/* Enter the protocol layer chunked message router state machine state */
void UCPD_PRL_SM_CHUNKED_ROUTER_Enter(UCPD_PORT_Number port_number,
                                      UCPD_PRL_SM_CHUNKED_ROUTER_State state);

/* Enter the protocol layer transmission state machine state */
void UCPD_PRL_SM_TX_Enter(UCPD_PORT_Number port_number,
                          UCPD_PRL_SM_TX_State state);

/* Enter the protocol layer reception state machine state */
void UCPD_PRL_SM_RX_Enter(UCPD_PORT_Number port_number,
                          UCPD_PRL_SM_RX_State state);

/* Enter the protocol layer hard reset state machine state */
void UCPD_PRL_SM_HRD_Enter(UCPD_PORT_Number port_number,
                           UCPD_PRL_SM_HR_State state);

/* Message construction */
void constructMessage(UCPD_PORT_Number port_number);

void constructChunk(UCPD_PORT_Number port_number);

#endif // UCPD_PRL_SM_H
