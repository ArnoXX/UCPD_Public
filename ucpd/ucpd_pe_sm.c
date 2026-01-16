#include "ucpd_pe_sm.h"
#include "ucpd.h"
#include "ucpd/ucpd_dpm.h"
#include "ucpd/ucpd_pe_sm.h"
#include "ucpd/ucpd_tim.h"
#include "ucpd_cnt.h"
#include "ucpd_ctx.h"
#include "ucpd_hw.h"
#include "ucpd_msg.h"
#include "ucpd_pe_prl.h"
#include "ucpd_prl_sm.h"
#include "ucpd_tim.h"
#include "ucpd_utils.h"

UCPD_PE_SM_SNK_State getPEStateToEnterOnReceive(UCPD_MSG* msg, UCPD_EXT_MSG* ext_msg, bool extended_message)
{

	if (extended_message)
	{

		switch (ext_msg->header.message_type)
		{
			case UCPD_EXTENDED_CONTROL_MSG_ID:
			{
				if (ext_msg->body.epr_ecdb.type == UCPD_EPR_KEEP_ALIVE_ACK_EXT_MSG_ID)
				{
					return PE_SNK_STATE_READY;
				}
				return PE_SNK_STATE_NONE;
			}
			case UCPD_EPR_SOURCE_CAPABILITIES_MSG_ID:
			{
				return PE_SNK_STATE_EVALUATE_CAPABILITY;
			}
			default:
			{
				return PE_SNK_STATE_NONE;
			}
		}
	}
	else
	{
		// TODO: finish this
		switch (msg->header.message_type)
		{
			case UCPD_SOURCE_CAPABILITIES_MSG_ID:
			{
				return PE_SNK_STATE_EVALUATE_CAPABILITY;
			}
			case UCPD_ACCEPT_MSG_ID:
			{
				return PE_SNK_STATE_TRANSITION_SINK;
			}
			case UCPD_PS_RDY_MSG_ID:
			{
				return PE_SNK_STATE_READY;
			}
			case UCPD_SOFT_RESET_MSG_ID:
			{
				return PE_SNK_STATE_SOFT_RESET;
			}
			case UCPD_NOT_SUPPORTED_MSG_ID:
			{
				return PE_SNK_STATE_NOT_SUPPORTED_RECEIVED;
			}
			case UCPD_REJECT_MSG_ID:
			{
				return PE_SNK_STATE_NONE;
			}
			case UCPD_EPR_MODE_MSG_ID:
			{
				switch (msg->body.epr_mdo.action)
				{
					case UCPD_EPR_ACTION_ENTER_ACK:
					{
						return PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE;
					}
					case UCPD_EPR_ACTION_ENTER_FAIL:
					{
						return PE_SNK_STATE_SEND_SOFT_RESET;
					}
					case UCPD_EPR_ACTION_ENTER_SUCC:
					{
						return PE_SNK_STATE_WAIT_FOR_CAPABILITIES;
					}
					case UCPD_EPR_ACTION_EXIT:
					{
						return PE_SNK_STATE_EPR_MODE_EXIT_RECEIVED;
					}
					default:
					{
						// not state_none for reset table because spec says "!Success -> Soft Reset", so it is
						// interpreted as definitely going to soft reset
						return PE_SNK_STATE_SEND_SOFT_RESET;
					}
				}
			}
			default:
			{
				// not supported message, but will be handled as a special case, so not returning a send_not_supported
				// state right away
				return PE_SNK_STATE_NONE;
			}
		}
	}
	return PE_SNK_STATE_NONE;
}

UCPD_PE_SM_SNK_State getPEStateToEnterOnTransmit(UCPD_MSG* msg, UCPD_EXT_MSG* ext_msg, bool extended_message)
{
	// TODO: finish this

	if (extended_message)
	{
		msg->header.extended = 0u;
	}
	else
	{
		switch (msg->header.message_type)
		{
			// we do not transition from select capability state on sending the request
			case UCPD_REQUEST_MSG_ID:
			{
				return PE_SNK_STATE_NONE;
			}
			case UCPD_NOT_SUPPORTED_MSG_ID:
			{
				return PE_SNK_STATE_READY;
			}
			case UCPD_ACCEPT_MSG_ID:
			{
				return PE_SNK_STATE_WAIT_FOR_CAPABILITIES;
			}
			case UCPD_SOFT_RESET_MSG_ID:
			{
				return PE_SNK_STATE_NONE;
			}
			case UCPD_EPR_MODE_MSG_ID:
			{
				if (msg->body.epr_mdo.action == UCPD_EPR_ACTION_EXIT)
				{
					return PE_SNK_STATE_WAIT_FOR_CAPABILITIES;
				}
				return PE_SNK_STATE_NONE;
			}
			case UCPD_EPR_REQUEST_MSG_ID:
			{
				return PE_SNK_STATE_NONE;
			}
			default:
			{
				return PE_SNK_STATE_NONE;
			}
		}
	}
	return PE_SNK_STATE_NONE;
}

void UCPD_PE_SM_Delta(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_PRL_CAD_Event event)
{
	UCPD_MSG* rx_msg = pe_prl_cad->rx_buffer;
	UCPD_MSG* tx_msg = pe_prl_cad->tx_buffer;
	UCPD_EXT_MSG* ext_rx_msg = pe_prl_cad->ext_rx_buffer;
	UCPD_EXT_MSG* ext_tx_msg = pe_prl_cad->ext_tx_buffer;

	switch (event)
	{

		case PE_PRL_EVENT_PRL_INIT:
		{

			UCPD_PE_SM_SRT_Enter(pe_prl_cad, PE_SRT_STATE_STOPPED);
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_STARTUP);
			break;
		}

		case PE_PRL_EVENT_PE_START_SRT:
		{
			UCPD_PE_SM_SRT_Enter(pe_prl_cad, PE_SRT_STATE_RUNNING);
			break;
		}

		case PE_PRL_EVENT_PE_STOP_SRT:
		{
			UCPD_PE_SM_SRT_Enter(pe_prl_cad, PE_SRT_STATE_STOPPED);
			break;
		}

		case PE_PRL_EVENT_PE_TRANSMISSION_ERROR:
		{
			if (pe_prl_cad->snk_state == PE_SNK_STATE_SEND_SOFT_RESET)
				UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_HARD_RESET);
			else if (pe_prl_cad->ams_first_sent == false && pe_prl_cad->explicit_contract)
				UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_READY);
			else
				UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_SEND_SOFT_RESET);
			break;
		}

		case PE_PRL_EVENT_PE_ERROR_WITH_MESSAGE:
		{
			// for now unexpected messages are handled in the same way as regular messages
			UCPD_PE_SM_Delta(pe_prl_cad, PE_PRL_EVENT_PE_MESSAGE_RECEIVED);
			break;
		}

		case PE_PRL_EVENT_PE_MESSAGE_RECEIVED:
		{

			UCPD_PE_SM_SNK_State state_to_enter =
			    getPEStateToEnterOnReceive(rx_msg, ext_rx_msg, pe_prl_cad->extended_message);

			pe_prl_cad->extended_message = false;

			// special case of reject message that depends on explicit contract
			// needs to go before the reset table check
			if (rx_msg->header.extended == false && rx_msg->header.message_type == UCPD_REJECT_MSG_ID)
			{

				if (pe_prl_cad->explicit_contract)
					UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_READY);
				else
				{
					UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);
				}

				return;
			}

			// here the reset table will be applicable
			// unrecognized/unsupported message received
			if (state_to_enter == PE_SNK_STATE_NONE)
			{
				switch (pe_prl_cad->snk_state)
				{
					// will catch unrecognized or unsupported messages
					// unexpected messages are caught in the rx chunking state machine and handled in
					// PE_PRL_EVENT_PE_ERROR_WITH_MESSAGE
					case PE_SNK_STATE_READY:
					{
						UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_SEND_NOT_SUPPORTED);
						break;
					}
					case PE_SNK_STATE_TRANSITION_SINK:
					{
						UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_SEND_SOFT_RESET);
						break;
					}
					default:
					{
						UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_HARD_RESET);
						break;
					}
				}
				return;
			}

			if (rx_msg->header.extended == true && rx_msg->header.message_type == UCPD_EPR_MODE_MSG_ID)
			{
				if (rx_msg->body.epr_mdo.action == UCPD_EPR_ACTION_ENTER_FAIL)
					pe_prl_cad->epr_enter_failed_reason = rx_msg->body.epr_mdo.data;
			}

			// then enter the state
			if (state_to_enter != PE_SNK_STATE_NONE) UCPD_PE_SM_SNK_Enter(pe_prl_cad, state_to_enter);

			break;
		}

		case PE_PRL_EVENT_PE_MESSAGE_SENT:
		{

			if (pe_prl_cad->ams && pe_prl_cad->ams_first_sent == false) pe_prl_cad->ams_first_sent = true;

			UCPD_PE_SM_SNK_State state_to_enter =
			    getPEStateToEnterOnTransmit(tx_msg, ext_tx_msg, pe_prl_cad->extended_message);

			pe_prl_cad->extended_message = false;

			if (state_to_enter != PE_SNK_STATE_NONE) UCPD_PE_SM_SNK_Enter(pe_prl_cad, state_to_enter);

			if (pe_prl_cad->response_expected)
			{
				UCPD_PE_SM_SRT_Enter(pe_prl_cad, PE_SRT_STATE_RUNNING);
				pe_prl_cad->response_expected = false;
			}

			if (pe_prl_cad->entering_epr)
			{
				UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_SINK_EPR_ENTER);
				pe_prl_cad->entering_epr = false;
			}
			break;
		}

		case PE_PRL_EVENT_PE_HARD_RESET_RECEIVED:
		{
			if (pe_prl_cad->current_callback != NULL)
			{
				pe_prl_cad->current_callback(pe_prl_cad->port_number, UCPD_STATUS_CANCELLED);
				pe_prl_cad->current_callback = NULL;
			}

			pe_prl_cad->dpm_request_status = UCPD_STATUS_CANCELLED;
			pe_prl_cad->sync_operation = false;

			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_TRANSITION_TO_DEFAULT);
			break;
		}

		case PE_PRL_EVENT_PE_HARD_RESET_SENT:
		{
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_TRANSITION_TO_DEFAULT);
			break;
		}

		case PE_PRL_EVENT_VBUS_PRESENT:
		{
			if (pe_prl_cad->snk_state == PE_SNK_STATE_DISCOVERY)
				UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);

			break;
		}

		case PE_PRL_EVENT_SRT_EXPIRED:
		{
			UCPD_PE_SM_SRT_Enter(pe_prl_cad, PE_SRT_STATE_EXPIRED);
			break;
		}

		case PE_PRL_EVENT_SNK_PPS_PER_TIMEOUT:
		{
			pe_prl_cad->pps_periodic_expired = true;
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_EVALUATE_CAPABILITY);
			break;
		}

		case PE_PRL_EVENT_PST_TRANS_TIMEOUT:
		{
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_HARD_RESET);
			break;
		}

		case PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_ENTRY:
		{
			pe_prl_cad->sync_operation = true;
			tx_msg->body.epr_mdo = (UCPD_EPRMDO){
			    .action = UCPD_EPR_ACTION_ENTER,
			    .data = pe_prl_cad->epr_power_rating,
			};
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_SEND_EPR_MODE_ENTRY);
			break;
		}

		case PE_PRL_EVENT_DPM_REQUEST_EPR_MODE_EXIT:
		{
			pe_prl_cad->sync_operation = true;
			tx_msg->body.epr_mdo = (UCPD_EPRMDO){.action = UCPD_EPR_ACTION_EXIT};
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_SEND_EPR_MODE_EXIT);
			break;
		}

		case PE_PRL_EVENT_EPR_ENTER_TIMEOUT:
		{
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_SEND_SOFT_RESET);
			break;
		}

		case PE_PRL_EVENT_EPR_KEEP_ALIVE_TIMEOUT:
		{
			if (pe_prl_cad->epr_mode) UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_EPR_KEEP_ALIVE);
			break;
		}
		case PE_PRL_EVENT_DPM_TRIGGER_POLICY:
		{
			pe_prl_cad->sync_operation = true;
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_EVALUATE_CAPABILITY);
			break;
		}
		case PE_PRL_EVENT_DPM_REQUEST_HARD_RESET:
		{
			pe_prl_cad->sync_operation = true;
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_HARD_RESET);
			break;
		}

		default:
		{
			break;
		}
	}
}

void UCPD_PE_SM_SRT_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_SM_SRT_State state)
{

	switch (state)
	{
		case PE_SRT_STATE_STOPPED:
		{
			UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_SENDER_RESPONSE);

			break;
		}

		case PE_SRT_STATE_RUNNING:
		{
			UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_SENDER_RESPONSE);
			UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_SENDER_RESPONSE);

			break;
		}

		case PE_SRT_STATE_EXPIRED:
		{
			// TODO: check if hrd is the only one that can be entered
			if (pe_prl_cad->snk_state == PE_SNK_STATE_GET_SOURCE_CAPABILITIES ||
			    pe_prl_cad->snk_state == PE_SNK_STATE_WAIT_FOR_GET_STATUS)
				UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_READY);
			else if (pe_prl_cad->snk_state == PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE ||
			         pe_prl_cad->snk_state == PE_SNK_STATE_SEND_EPR_MODE_ENTRY)
			{

				UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_SEND_SOFT_RESET);
			}
			else
				UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_HARD_RESET);

			break;
		}

		default:
		{
			break;
		}
	};
}

extern bool waiting_for_epr;

void UCPD_PE_SM_SNK_Enter(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_SM_SNK_State state)
{

	UCPD_MSG* rx_msg = pe_prl_cad->rx_buffer;
	UCPD_MSG* tx_msg = pe_prl_cad->tx_buffer;
	UCPD_EXT_MSG* ext_tx_msg = pe_prl_cad->ext_tx_buffer;
	UCPD_EXT_MSG* ext_rx_msg = pe_prl_cad->ext_rx_buffer;

	UCPD_PE_SM_SNK_Exit(pe_prl_cad, pe_prl_cad->snk_state);
	pe_prl_cad->snk_state = state;

	switch (state)
	{

		case PE_SNK_STATE_STARTUP:
		{

			pe_prl_cad->hrdOriginPE = false;
			pe_prl_cad->epr_mode = false;
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_DISCOVERY);

			break;
		}

		case PE_SNK_STATE_DISCOVERY:
		{

			if (UCPD_PWR_vS0_to_vS5(pe_prl_cad->port_number))
			{
				UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);
			}

			break;
		}

		case PE_SNK_STATE_WAIT_FOR_CAPABILITIES:
		{
			pe_prl_cad->explicit_contract = false;

			UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_SINK_WAIT_CAP);

			break;
		}

		case PE_SNK_STATE_EVALUATE_CAPABILITY:
		{
			UCPD_CNT_Reset(pe_prl_cad->port_number, UCPD_CNT_HARD_RESET);

			pe_prl_cad->ams = true;

			if (pe_prl_cad->pps_periodic_expired == false)
			{
				UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_AMS_FIRST_FROM_PE);

				if (rx_msg->header.extended == false && rx_msg->header.message_type == UCPD_SOURCE_CAPABILITIES_MSG_ID)
				{
					pe_prl_cad->snk_src_pdo_count = rx_msg->header.number_of_data_objects;
					COPY_RAW(rx_msg->body.src_pdos, pe_prl_cad->src_pdos,
					         sizeof(UCPD_SRC_PDO) * pe_prl_cad->snk_src_pdo_count);
				}
				else if (ext_rx_msg->header.extended == true &&
				         ext_rx_msg->header.message_type == UCPD_EPR_SOURCE_CAPABILITIES_MSG_ID)
				{
					uint8_t count = ext_rx_msg->ext_header.data_size / UCPD_PDO_SIZE;
					if (count > (UCPD_MAX_PDO_NUM + UCPD_MAX_EPR_PDO_NUM))
						count = (UCPD_MAX_PDO_NUM + UCPD_MAX_EPR_PDO_NUM);
					pe_prl_cad->snk_src_pdo_count = count;
					COPY_RAW(ext_rx_msg->body.epr_pdos, pe_prl_cad->src_pdos,
					         sizeof(UCPD_SRC_PDO) * pe_prl_cad->snk_src_pdo_count);
				}
			}

			pe_prl_cad->pps_periodic_expired = false;

			if (pe_prl_cad->explicit_contract)
			{
				pe_prl_cad->dpm_request_status = UCPD_STATUS_SUCCESS;

				pe_prl_cad->sync_operation = false;

				if (pe_prl_cad->current_callback != NULL)
				{
					pe_prl_cad->current_callback(pe_prl_cad->port_number, UCPD_STATUS_SUCCESS);

					pe_prl_cad->current_callback = NULL;
				}
			}

			UCPD_DPM_EvaluateCapabilities(pe_prl_cad->port_number);

			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_SELECT_CAPABILITY);

			break;
		}

		case PE_SNK_STATE_SELECT_CAPABILITY:
		{

			uint8_t pdo_index = tx_msg->body.snk_rdo.object_pos - 1u;

			// PPS is not available in EPR mode - handled by select capability API functions
			if (pe_prl_cad->epr_mode)
			{
				UCPD_CNT_Increment(pe_prl_cad->port_number, UCPD_CNT_MESSAGEID);
				tx_msg->header.message_type = UCPD_EPR_REQUEST_MSG_ID;

				tx_msg->header.number_of_data_objects = 2u;

				pe_prl_cad->txSize = sizeof(UCPD_SNK_RDO) + sizeof(UCPD_SRC_PDO) + UCPD_MSG_HEADER_SIZE;
			}
			else
			{
				tx_msg->header.message_type = UCPD_REQUEST_MSG_ID;

				tx_msg->header.number_of_data_objects = 1u;

				pe_prl_cad->txSize = sizeof(UCPD_SNK_RDO) + UCPD_MSG_HEADER_SIZE;

				if (pe_prl_cad->src_pdos[pdo_index].pdo_type == UCPD_PDO_TYPE_APDO &&
				    pe_prl_cad->src_pdos[pdo_index].APDO.apdo_type == UCPD_APDO_TYPE_PPS)
					pe_prl_cad->pps_contract = true;
				else
					pe_prl_cad->pps_contract = false;
			}

			pe_prl_cad->response_expected = true;

			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_MESSAGE_FROM_PE);

			break;
		}

		case PE_SNK_STATE_TRANSITION_SINK:
		{
			UCPD_PE_SM_SRT_Enter(pe_prl_cad, PE_SRT_STATE_STOPPED);
			if (pe_prl_cad->epr_mode)
				UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_SINK_EPR_TRANSITION);
			else
				UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_PS_TRANSITION);
			break;
		}

		case PE_SNK_STATE_READY:
		{

			pe_prl_cad->ams = false;
			pe_prl_cad->explicit_contract = true;
			pe_prl_cad->ams_first_sent = false;

			if (pe_prl_cad->pps_contract) UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_SINK_PPS_PERIODIC);

			if (pe_prl_cad->epr_mode) UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_SINK_EPR_KEEP_ALIVE);

			break;
		}

		case PE_SNK_STATE_HARD_RESET:
		{
			UCPD_CNT_Increment(pe_prl_cad->port_number, UCPD_CNT_HARD_RESET);

			pe_prl_cad->hrdOriginPE = true;

			pe_prl_cad->pps_contract = false;
			pe_prl_cad->hardResetActive = true;
			pe_prl_cad->ams = false;
			pe_prl_cad->ams_first_sent = false;
			pe_prl_cad->epr_mode = false;

			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_HRD_RECEIVED);

			break;
		}

		case PE_SNK_STATE_TRANSITION_TO_DEFAULT:
		{
			if (pe_prl_cad->epr_mode) UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_SINK_EPR_KEEP_ALIVE);

			pe_prl_cad->explicit_contract = false;
			pe_prl_cad->pps_contract = false;
			pe_prl_cad->ams = false;
			pe_prl_cad->ams_first_sent = false;
			pe_prl_cad->hrdOriginPE = false;
			pe_prl_cad->extended_message = false;

			if (pe_prl_cad->current_callback != NULL)
			{
				pe_prl_cad->current_callback(pe_prl_cad->port_number, UCPD_STATUS_SUCCESS);
				pe_prl_cad->current_callback = NULL;
			}

			pe_prl_cad->sync_operation = false;

			// DONE:request dpm power transition to default, whatever that means
			UCPD_DPM_PowerTransitionDefault(pe_prl_cad->port_number);

			// DONE?:reset hardware, whatever that means

			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_STARTUP);
			break;
		}

			/* soft reset states */

		case PE_SNK_STATE_SOFT_RESET:
		{
			// DONE:request prl to send accept
			*tx_msg = UCPD_MSG_INIT;
			tx_msg->header.message_type = UCPD_ACCEPT_MSG_ID;

			tx_msg->header.number_of_data_objects = 0;

			pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE;

			if (pe_prl_cad->current_callback != NULL)
			{
				pe_prl_cad->current_callback(pe_prl_cad->port_number, UCPD_STATUS_CANCELLED);
				pe_prl_cad->current_callback = NULL;
			}

			pe_prl_cad->dpm_request_status = UCPD_STATUS_CANCELLED;
			pe_prl_cad->sync_operation = false;

			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_MESSAGE_FROM_PE);
			break;
		}

		case PE_SNK_STATE_NONE:
		{

			break;
		}

		case PE_SNK_STATE_SEND_SOFT_RESET:
		{
			// TODO:reset prl
			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_PRL_INIT);

			// DONE:request prl to send soft reset
			*tx_msg = UCPD_MSG_INIT;
			tx_msg->header.message_type = UCPD_SOFT_RESET_MSG_ID;

			tx_msg->header.number_of_data_objects = 0u;

			pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE;

			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_MESSAGE_FROM_PE);

			break;
		}

			/* not supported states */

		case PE_SNK_STATE_NOT_SUPPORTED_RECEIVED:
		{
			// DONE: inform dpm not supported received

			UCPD_DPM_InformMessageReceived(pe_prl_cad->port_number);
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_READY);

			break;
		}

		case PE_SNK_STATE_SEND_NOT_SUPPORTED:
		{
			// DONE:tell prl to send not supported message
			*tx_msg = UCPD_MSG_INIT;
			tx_msg->header.message_type = UCPD_NOT_SUPPORTED_MSG_ID;

			tx_msg->header.number_of_data_objects = 0u;

			pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE;

			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_MESSAGE_FROM_PE);
			break;
		}

		case PE_SNK_STATE_SEND_EPR_MODE_ENTRY:
		{
			tx_msg->header = UCPD_MSG_HEADER_INIT;
			tx_msg->header.message_type = UCPD_EPR_MODE_MSG_ID;
			tx_msg->header.number_of_data_objects = 1u;

			// operational pdp is already set in the enter API function
			pe_prl_cad->response_expected = true;
			pe_prl_cad->entering_epr = true;

			pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE + sizeof(UCPD_EPRMDO);

			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_MESSAGE_FROM_PE);

			break;
		}

		case PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE:
		{
			break;
		}

		case PE_SNK_STATE_SEND_EPR_MODE_EXIT:
		{
			// exit message is fully filled out here, nothing is done in the exit API function
			tx_msg->header = UCPD_MSG_HEADER_INIT;
			tx_msg->header.message_type = UCPD_EPR_MODE_MSG_ID;
			tx_msg->header.number_of_data_objects = 1u;
			tx_msg->body.epr_mdo = (UCPD_EPRMDO){.action = UCPD_EPR_ACTION_EXIT};

			pe_prl_cad->txSize = UCPD_MSG_HEADER_SIZE + sizeof(UCPD_EPRMDO);

			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_MESSAGE_FROM_PE);

			break;
		}

		case PE_SNK_STATE_EPR_MODE_EXIT_RECEIVED:
		{
			pe_prl_cad->epr_mode = false;
			UCPD_PE_SM_SNK_Enter(pe_prl_cad, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);
			break;
		}

		case PE_SNK_STATE_EPR_KEEP_ALIVE:
		{
			/* Build full extended message in EXT TX buffer; PRL will chunk into TX
			 * buffer. */
			*ext_tx_msg = UCPD_EXT_MSG_INIT;
			ext_tx_msg->header.message_type = UCPD_EXTENDED_CONTROL_MSG_ID;
			ext_tx_msg->header.extended = true;
			ext_tx_msg->header.number_of_data_objects = 1u;

			ext_tx_msg->ext_header = (UCPD_MSG_EXT_HEADER){.chunked = true, .data_size = 2u};

			ext_tx_msg->body.epr_ecdb = (UCPD_ECDB){.type = UCPD_EPR_KEEP_ALIVE_EXT_MSG_ID};

			pe_prl_cad->response_expected = true;
			pe_prl_cad->extended_message = true;

			/* Let PRL compute txSize from per-chunk assembly. */
			UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_MESSAGE_FROM_PE);
			break;
		}
		default:
		{
			break;
		}
	}
}

void UCPD_PE_SM_SNK_Exit(UCPD_PE_PRL_CAD_Module* pe_prl_cad, UCPD_PE_SM_SNK_State state)
{

	switch (state)
	{
		case PE_SNK_STATE_EVALUATE_CAPABILITY:
		{
			UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_SINK_WAIT_CAP);

			break;
		}

		case PE_SNK_STATE_TRANSITION_SINK:
		{
			// request dpm to transition to new power
			if (!pe_prl_cad->epr_mode)
				UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_PS_TRANSITION);
			else
				UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_SINK_EPR_TRANSITION);

			UCPD_DPM_InformPowerTransition(pe_prl_cad->port_number);
			break;
		}

		case PE_SNK_STATE_READY:
		{
			// notify prl of ams if initiated
			pe_prl_cad->ams = true;
			if (pe_prl_cad->pps_contract)
			{
				pe_prl_cad->pps_contract = false;
				UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_SINK_PPS_PERIODIC);
			}

			break;
		}
		case PE_SNK_STATE_TRANSITION_TO_DEFAULT:
		{
			UCPD_PRL_SM_HRD_Enter(pe_prl_cad, PRL_HR_STATE_PE_HARD_RESET_COMPLETE);
			break;
		}

		case PE_SNK_STATE_SEND_EPR_MODE_ENTRY:
		{
			break;
		}

		case PE_SNK_STATE_EPR_MODE_ENTRY_WAIT_FOR_RESPONSE:
		{
			pe_prl_cad->epr_mode = true;
			if (pe_prl_cad->current_callback != NULL)
			{
				pe_prl_cad->current_callback(pe_prl_cad->port_number, UCPD_STATUS_SUCCESS);
				pe_prl_cad->current_callback = NULL;
			}
			pe_prl_cad->dpm_request_status = UCPD_STATUS_SUCCESS;
			pe_prl_cad->sync_operation = false;

			UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_SINK_EPR_ENTER);
			break;
		}

		case PE_SNK_STATE_SEND_EPR_MODE_EXIT:
		{
			pe_prl_cad->epr_mode = false;

			if (pe_prl_cad->current_callback != NULL)
			{
				pe_prl_cad->current_callback(pe_prl_cad->port_number, UCPD_STATUS_SUCCESS);
				pe_prl_cad->current_callback = NULL;
			}

			pe_prl_cad->dpm_request_status = UCPD_STATUS_SUCCESS;
			pe_prl_cad->sync_operation = false;
			UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_SINK_EPR_KEEP_ALIVE);
			break;
		}

		case PE_SNK_STATE_EPR_MODE_EXIT_RECEIVED:
		{

			break;
		}

		case PE_SNK_STATE_EPR_KEEP_ALIVE:
		{
			break;
		}

		default:
		{
			break;
		}
	};
}
