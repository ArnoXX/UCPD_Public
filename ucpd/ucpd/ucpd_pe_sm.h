#ifndef UCPD_PE_SM_H
#define UCPD_PE_SM_H

#include "ucpd_msg.h"
#include "ucpd_port_num.h"
#include "ucpd_utils.h"


/* Policy engine state machines states */
/* Policy engine SenderResponseTimer state machine states */
typedef enum {
  PE_SRT_STATE_STOPPED,
  PE_SRT_STATE_RUNNING,
  PE_SRT_STATE_EXPIRED,
  PE_SRT_STATE_NONE
} UCPD_PE_SM_SRT_State;

/* Policy engine sink port state machine states */
typedef enum {
  // PE sink port states
  PE_SNK_STATE_TRANSITION_TO_DEFAULT,
  PE_SNK_STATE_HARD_RESET,
  PE_SNK_STATE_STARTUP,
  PE_SNK_STATE_DISCOVERY,
  PE_SNK_STATE_WAIT_FOR_CAPABILITIES,
  PE_SNK_STATE_EVALUATE_CAPABILITY,
  PE_SNK_STATE_SELECT_CAPABILITY,
  PE_SNK_STATE_TRANSITION_SINK,
  PE_SNK_STATE_READY,
  PE_SNK_STATE_GET_SOURCE_CAPABILITIES,
  PE_SNK_STATE_GIVE_SINK_CAPABILITIES,
  // SOP sink port soft reset and protocol error states
  PE_SNK_STATE_SOFT_RESET,
  PE_SNK_STATE_SEND_SOFT_RESET,
  // sink port not supported message states
  PE_SNK_STATE_SEND_NOT_SUPPORTED,
  PE_SNK_STATE_CHUNK_RECEIVED,
  PE_SNK_STATE_NOT_SUPPORTED_RECEIVED,
  // sink port received alert message alert states
  PE_SNK_STATE_SOURCE_ALERT_RECEIVED,
  // sink port sends alert message alert states
  PE_SNK_STATE_SEND_SINK_ALERT,
  PE_SNK_STATE_WAIT_FOR_GET_STATUS,
  // sink port sends get source extended capabilities states
  PE_SNK_STATE_GET_SOURCE_CAP_EXT,
  // sink port received get sink extended capabilities states
  PE_SNK_STATE_GIVE_SINK_CAP_EXT,
  // sink port sends get source info states
  PE_SNK_STATE_GET_SOURCE_INFO,
  // sink port sends get status states
  PE_STATE_GET_STATUS,
  // sink port received get sink info states
  PE_STATE_GIVE_STATUS,
  // sink port sends get pps status states
  PE_SNK_STATE_GET_PPS_STATUS,
  // sink port sends get revision states
  PE_STATE_GET_REVISION,
  // sink port receives get revision states
  PE_STATE_GIVE_REVISION,
  // sink BIST carrier mode states
  PE_STATE_BIST_CARRIER_MODE,
  // sink BIST test mode states
  PE_STATE_BIST_TEST_MODE,
  // sink Type C referenced state
  PE_STATE_TYPE_C_ERROR_RECOVERY,

  PE_SNK_STATE_SEND_EPR_MODE_ENTRY,
  PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE,
  PE_SNK_STATE_SEND_EPR_MODE_EXIT,
  PE_SNK_STATE_EPR_MODE_EXIT_RECEIVED,
  PE_SNK_STATE_EPR_KEEP_ALIVE,

  PE_SNK_STATE_NONE
} UCPD_PE_SM_SNK_State;

/* Policy engine state machine */
typedef struct {

  // mb should be bitfields instead of full bools
  UCPD_Bool response_expected;

  UCPD_Bool transmitting_soft_reset;

  UCPD_Bool msg_received_on_err;
  UCPD_Bool msg_first_chu_on_err;
  UCPD_Bool wait_received;

  UCPD_Bool pps_periodic_expired;

  UCPD_Bool pps_contract;
  UCPD_Bool explicit_contract;
  UCPD_Bool ams;
  UCPD_Bool ams_first_sent;

  // last message received
  UCPD_MSG msg;

  // chunk wait expired
  UCPD_Bool chunk_wait_expired;

} UCPD_PE_SM;

/* SRT enter function */
void UCPD_PE_SM_SRT_Enter(UCPD_PORT_Number port_number,
                          UCPD_PE_SM_SRT_State state);

/* SNK enter function */
void UCPD_PE_SM_SNK_Enter(UCPD_PORT_Number port_number,
                          UCPD_PE_SM_SNK_State state);

/* SNK exit function */
void UCPD_PE_SM_SNK_Exit(UCPD_PORT_Number port_number,
                         UCPD_PE_SM_SNK_State state);

#endif // UCPD_PE_SM_H
