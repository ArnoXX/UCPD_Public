#ifndef UCPD_PE_PRL_CAD_H
#define UCPD_PE_PRL_CAD_H

#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_typec.h"
#include "ucpd_pe_sm.h"
#include "ucpd_prl_sm.h"
#include "ucpd_utils.h"


#define UCPD_BUFFER_COUNT 4u
#define UCPD_RX_BUFFER_INDEX 0u
#define UCPD_TX_BUFFER_INDEX 1u
#define UCPD_EXT_RX_BUFFER_INDEX 2u
#define UCPD_EXT_TX_BUFFER_INDEX 3u

typedef enum {
  /* CAD events */
  PE_PRL_EVENT_PRL_INIT,

  /* PHY events */
  PE_PRL_EVENT_HRD_RECEIVED,
  PE_PRL_EVENT_HRD_SENT,
  PE_PRL_EVENT_MSG_TX_DISCARDED,
  PE_PRL_EVENT_MSG_TX_COMPLETED,
  PE_PRL_EVENT_MSG_RX_COMPLETE,
  PE_PRL_EVENT_TX_NG,
  PE_PRL_EVENT_TX_OK,
  PE_PRL_EVENT_VBUS_PRESENT,

  /* Timer events */
  PE_PRL_EVENT_BIST_CONT_MODE_TIMEROUT,
  PE_PRL_EVENT_SENDER_REQ_TIMER_TIMEOUT,
  PE_PRL_EVENT_CHUNK_RES_TIMER_TIMEOUT,
  PE_PRL_EVENT_CRC_RX_TIMEOUT,
  PE_PRL_EVENT_HARD_RESET_TIMEOUT,
  PE_PRL_EVENT_PST_TRANS_TIMEOUT,
  PE_PRL_EVENT_SRT_EXPIRED,
  PE_PRL_EVENT_SNK_PPS_PER_TIMEOUT,
  PE_PRL_EVENT_EPR_ENTER_TIMEOUT,
  PE_PRL_EVENT_EPR_KEEP_ALIVE_TIMEOUT,

  /* CAD events */
  CAD_EVENT_CONN_DETECTED,
  CAD_EVENT_CONN_REMOVED,
  CAD_EVENT_VBUS_REMOVED,
  CAD_EVENT_VBUS_DETECTED,
  CAD_EVENT_TCDEBOUNCE_TIMEOUT,
  CAD_EVENT_TPDEBOUNCE_TIMEOUT,
  CAD_EVENT_TERROR_RECOVERY,
  CAD_EVENT_ERROR,

  /* Internal events */
  PE_PRL_EVENT_MESSAGE_FROM_PE,
  PE_PRL_EVENT_AMS_FIRST_FROM_PE,
  PE_PRL_EVENT_SOFT_RESET_FROM_PE,
  PE_PRL_EVENT_EPR_ENTRY_FROM_DPM,
  PE_PRL_EVENT_EPR_EXIT_FROM_DPM,

  /* PE events */
  PE_PRL_EVENT_PE_START_SRT,
  PE_PRL_EVENT_PE_STOP_SRT,
  PE_PRL_EVENT_PE_TRANSMISSION_ERROR,
  PE_PRL_EVENT_PE_MESSAGE_RECEIVED,
  PE_PRL_EVENT_PE_MESSAGE_SENT,
  PE_PRL_EVENT_PE_ERROR_WITH_MESSAGE,
  PE_PRL_EVENT_PE_HARD_RESET_RECEIVED,
  PE_PRL_EVENT_PE_HARD_RESET_SENT,
  PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY,
  PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT,
  PE_PRL_EVENT_DPM_REQUEST_FIXED_SUPPLY,
  PE_PRL_EVENT_DPM_REQUEST_PPS,
  PE_PRL_EVENT_DPM_REQUEST_EPR,

  PE_PRL_CAD_EVENT_NONE
} UCPD_PE_PRL_CAD_Event;

typedef struct {
  UCPD_PE_PRL_CAD_Event phy_event;
  UCPD_PE_PRL_CAD_Event pwr_event;
  UCPD_PE_PRL_CAD_Event timer_event;
  UCPD_PE_PRL_CAD_Event dpm_event;

  UCPD_PE_SM_SRT_State srt_state;
  UCPD_PE_SM_SNK_State snk_state;

  UCPD_PRL_SM_CHUNKED_RX_State chunked_rx_state;
  UCPD_PRL_SM_CHUNKED_TX_State chunked_tx_state;
  UCPD_PRL_SM_CHUNKED_ROUTER_State chunked_router_state;
  UCPD_PRL_SM_TX_State tx_state;
  UCPD_PRL_SM_RX_State rx_state;
  UCPD_PRL_SM_HR_State hrd_state;

  UCPD_CAD_SM_State cad_state;

  volatile UCPD_Bool txAllowed;

  UCPD_COUNTER counters[UCPD_CNT_NUMBER];

  // power
  UCPD_Bool vbusDet;
  UCPD_Bool tccDeb;
  UCPD_Bool vS0tovS5trans;
  UCPD_Bool hardResetActive;

  // pe
  UCPD_Bool pps_contract;
  UCPD_Bool explicit_contract;
  UCPD_Bool ams;
  UCPD_Bool ams_first_sent;
  UCPD_Bool pps_periodic_expired;
  UCPD_Bool epr_mode;
  UCPD_Bool response_expected;
  UCPD_Bool entering_epr;

  UCPD_Count chunk_number_expected;
  UCPD_Count chunk_number_to_send;
  UCPD_Count num_bytes_received;
  UCPD_Bool lastChunk;
  UCPD_Bool extended_message;

  // prl
  UCPD_Count storedMID;
  UCPD_Bool midStored;

  UCPD_Count txSize;

  // hard reset
  UCPD_Bool hrdOriginPE;

  UCPD_MSG buffers[UCPD_BUFFER_COUNT];

} UCPD_PE_PRL_CAD_Module;

/* PE_PRL_CAD module functions */
/* Init the PE_PRL_CAD module */
void UCPD_PE_PRL_CAD_Init(UCPD_PORT_Number port_number);

/* Reset the PE_PRL_CAD module */
void UCPD_PE_PRL_CAD_Reset(UCPD_PORT_Number port_number);

/* PE_PRL_CAD event handler - to be called from ISR */
void UCPD_PE_PRL_CAD_Handler(UCPD_PORT_Number port_number);

/* State machine update functions */
/* Processes CAD, PRL and PE state machines */
void UCPD_PE_PRL_CAD_SM_Delta(UCPD_PORT_Number port_number);

/* CAD SM delta function */
void UCPD_CAD_SM_Delta(UCPD_PORT_Number port_number,
                       UCPD_PE_PRL_CAD_Event event);

/* PRL SM delta function */
void UCPD_PRL_SM_Delta(UCPD_PORT_Number port_number,
                       UCPD_PE_PRL_CAD_Event event);

/* PE SM delta function */
void UCPD_PE_SM_Delta(UCPD_PORT_Number port_number,
                      UCPD_PE_PRL_CAD_Event event);

#endif