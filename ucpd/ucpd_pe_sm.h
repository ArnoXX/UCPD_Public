#ifndef UCPD_PE_SM_H
#define UCPD_PE_SM_H

#include <stdint.h>

/* Policy engine state machines states */
/* Policy engine SenderResponseTimer state machine states */
typedef enum : uint8_t
{
	PE_SRT_STATE_NONE,
	PE_SRT_STATE_STOPPED,
	PE_SRT_STATE_RUNNING,
	PE_SRT_STATE_EXPIRED
} UCPD_PE_SM_SRT_State;

/* Policy engine sink port state machine states */
typedef enum : uint8_t
{
	PE_SNK_STATE_NONE,
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

	// epr mode states
	PE_SNK_STATE_SEND_EPR_MODE_ENTRY,
	PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE,
	PE_SNK_STATE_SEND_EPR_MODE_EXIT,
	PE_SNK_STATE_EPR_MODE_EXIT_RECEIVED,
	PE_SNK_STATE_EPR_KEEP_ALIVE
} UCPD_PE_SM_SNK_State;

#endif // UCPD_PE_SM_H
