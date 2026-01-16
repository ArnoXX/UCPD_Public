#ifndef UCPD_PE_PRL_CAD_H
#define UCPD_PE_PRL_CAD_H

#include "ucpd.h"
#include "ucpd/ucpd_msg.h"
#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_typec.h"
#include "ucpd_pe_sm.h"
#include "ucpd_prl_sm.h"
#include <stdint.h>



typedef enum : uint8_t
{
	PE_PRL_CAD_EVENT_NONE,
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
	PE_PRL_EVENT_DPM_TRIGGER_POLICY,
	PE_PRL_EVENT_DPM_REQUEST_HARD_RESET
} UCPD_PE_PRL_CAD_Event;

typedef struct
{
	UCPD_PORT_Number port_number;

	UCPD_PE_PRL_CAD_Event phy_event;
	UCPD_PE_PRL_CAD_Event pwr_event;
	UCPD_PE_PRL_CAD_Event timer_event;

	UCPD_PE_PRL_CAD_Event current_phy_event;
	UCPD_PE_PRL_CAD_Event current_pwr_event;
	UCPD_PE_PRL_CAD_Event current_timer_event;

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

	volatile bool txAllowed;

	// power
	bool vbusDet;
	bool tccDeb;
	bool vS0tovS5trans;
	bool hardResetActive;

	// pe
	bool pps_contract;
	bool explicit_contract;
	bool ams;
	bool ams_first_sent;
	bool pps_periodic_expired;
	bool epr_mode;
	bool response_expected;
	bool entering_epr;

	uint8_t chunk_number_expected;
	uint8_t chunk_number_to_send;
	uint8_t num_bytes_received;
	bool lastChunk;
	bool extended_message;

	// prl
	uint8_t storedMID;
	bool midStored;
	bool unexpected_message;
	
	uint16_t txSize;

	// hard reset
	bool hrdOriginPE;

	// buffers
	UCPD_MSG* rx_buffer;
	UCPD_MSG* tx_buffer;
	UCPD_EXT_MSG* ext_rx_buffer;
	UCPD_EXT_MSG* ext_tx_buffer;
	

	UCPD_SRC_PDO src_pdos[UCPD_MAX_PDO_NUM + UCPD_MAX_EPR_PDO_NUM];
	uint8_t snk_src_pdo_count;
	uint8_t snk_pdo_count;

	// api
	UCPD_Callback current_callback;
	UCPD_Callback callback;
	volatile bool sync_operation;
	volatile UCPD_Status dpm_request_status;
	volatile bool choice_valid;
	volatile uint32_t epr_enter_failed_reason;
	volatile uint8_t epr_power_rating;
} UCPD_PE_PRL_CAD_Module;

/* PE_PRL_CAD module functions */
/* Init the PE_PRL_CAD module */
void UCPD_PE_PRL_CAD_Init(UCPD_PORT_Number port_number, UCPD_PE_PRL_CAD_Module* pe_prl_cad);

/* Reset the PE_PRL_CAD module */
void UCPD_PE_PRL_CAD_Reset(UCPD_PE_PRL_CAD_Module* pe_prl_cad);

/* PE_PRL_CAD event handler - to be called from ISR */
void UCPD_PE_PRL_Handler(UCPD_PORT_Number port_number);

/* PE functions */
/* PE SM delta function */
void UCPD_PE_SM_Delta(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_PRL_CAD_Event event);

/* SRT enter function */
void UCPD_PE_SM_SRT_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_SM_SRT_State state);

/* SNK enter function */
void UCPD_PE_SM_SNK_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_SM_SNK_State state);

/* SNK exit function */
void UCPD_PE_SM_SNK_Exit(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_SM_SNK_State state);

/* CAD functions*/
/* CAD SM delta function */
void UCPD_CAD_SM_Delta(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_PRL_CAD_Event event);

/* Enter the CAD SM state */
void UCPD_CAD_SM_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_CAD_SM_State state);

/* PRL functions */
/* PRL SM delta function */
void UCPD_PRL_SM_Delta(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_PRL_CAD_Event event);

/* Enter the protocol layer chunked reception state machine state */
void UCPD_PRL_SM_CHUNKED_RX_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PRL_SM_CHUNKED_RX_State state);

/* Enter the protocol layer chunked transmission state machine state */
void UCPD_PRL_SM_CHUNKED_TX_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PRL_SM_CHUNKED_TX_State state);

/* Enter the protocol layer chunked message router state machine state */
void UCPD_PRL_SM_CHUNKED_ROUTER_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PRL_SM_CHUNKED_ROUTER_State state);

/* Enter the protocol layer transmission state machine state */
void UCPD_PRL_SM_TX_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PRL_SM_TX_State state);

/* Enter the protocol layer reception state machine state */
void UCPD_PRL_SM_RX_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PRL_SM_RX_State state);

/* Enter the protocol layer hard reset state machine state */
void UCPD_PRL_SM_HRD_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PRL_SM_HR_State state);

#endif
