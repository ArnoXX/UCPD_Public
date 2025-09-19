#include "ucpd/ucpd_pe_prl.h"
#include "device/ucpd_hw_defines.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd_ctx.h"
#include "ucpd_pe_prl.h"

void UCPD_PE_PRL_Handler(UCPD_PORT_Number port_number) {
  // on the level of hardware interrupts have line priorities so even if the
  // software priority is same, one can preempt the other - no idea here which
  // ones will be used on target HW
  BLOCK_EVENT(UCPD_TIMER_EVENT);
  BLOCK_EVENT(UCPD_ADC_EVENT);

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_PE_PRL_CAD_Module *pe_prl_cad = &port->pe_prl_cad;

#if 0
    if(pe_prl->event == PE_PRL_EVENT_HRD_RECEIVED)
    {
    	port->cad_sm.hard_reset = UCPD_TRUE;
    }

    if(port->cad_sm.event != CAD_EVENT_NONE)
    {
        UCPD_CAD_SM_Delta(port_number, port->cad_sm.event, &pe_prl->event);
        port->cad_sm.event = CAD_EVENT_NONE;
    }
    
    if (pe_prl->event != PE_PRL_EVENT_NONE)
    {
        UCPD_PE_PRL_SM_Delta(port_number, pe_prl->event);
        pe_prl->event = PE_PRL_EVENT_NONE;
    }
#endif

  // first check the pwr events for vbus removal
  // may exit earlier if detached
if (pe_prl_cad->pwr_event != PE_PRL_CAD_EVENT_NONE) {
    // cad delta
    // the events are passed through to the PRL and PE as needed through Deltas
    UCPD_CAD_SM_Delta(port_number, pe_prl_cad->pwr_event);

    pe_prl_cad->pwr_event = PE_PRL_CAD_EVENT_NONE;
  }

  // we want to give priority to this one to avoid problems with conn removed
  if(pe_prl_cad->phy_event == CAD_EVENT_CONN_REMOVED)
  {
    UCPD_CAD_SM_Delta(port_number, pe_prl_cad->phy_event);
  }

  // same with cc detach debounce - may exit earlier if detached
  if (pe_prl_cad->timer_event != PE_PRL_CAD_EVENT_NONE) {
    // cad delta
    UCPD_CAD_SM_Delta(port_number, pe_prl_cad->timer_event);
    // prl delta
    UCPD_PRL_SM_Delta(port_number, pe_prl_cad->timer_event);
    // pe delta
    UCPD_PE_SM_Delta(port_number, pe_prl_cad->timer_event);
    pe_prl_cad->timer_event = PE_PRL_CAD_EVENT_NONE;
  }

  // timers take priority over received messages
  // i.e. if we get goodcrc message and corresponding timer expires, we will
  // process the timer first
  if (pe_prl_cad->phy_event != PE_PRL_CAD_EVENT_NONE) {
    // cad delta
    UCPD_CAD_SM_Delta(port_number, pe_prl_cad->phy_event);
    // prl delta
    UCPD_PRL_SM_Delta(port_number, pe_prl_cad->phy_event);

    pe_prl_cad->phy_event = PE_PRL_CAD_EVENT_NONE;
  }

  // dpm queries have lowest priority
  if (pe_prl_cad->dpm_event != PE_PRL_CAD_EVENT_NONE) {
    UCPD_PE_SM_Delta(port_number, pe_prl_cad->dpm_event);

    pe_prl_cad->dpm_event = PE_PRL_CAD_EVENT_NONE;
  }

  UCPD_PHY_Worker_Finished(port_number);
  UNBLOCK_EVENT(UCPD_ADC_EVENT);
  UNBLOCK_EVENT(UCPD_TIMER_EVENT);
}

#if 0
void UCPD_PE_PRL_Init(UCPD_PORT_Number port_number)
{
    UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

    for(UCPD_CNT cnt = 0; cnt < UCPD_CNT_NUMBER; cnt++)
    {
        UCPD_CNT_Init(port_number, cnt);
    }

	port->pe_prl.event = PE_PRL_EVENT_NONE;
    UNBLOCK_EVENT(UCPD_PE_PRL_WORKER_EVENT);
}

void UCPD_PE_PRL_Reset(UCPD_PORT_Number port_number)
{
    UCPD_PRL_SM_Reset(port_number);
    UCPD_PE_SM_Reset(port_number);
}

void UCPD_PE_PRL_SM_Delta(UCPD_PORT_Number port_number, UCPD_PE_PRL_Event event)
{

    UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);
    UCPD_PE_PRL_Module* pe_prl = &port->pe_prl;
    switch(event)
    {
        case PE_PRL_EVENT_PRL_INIT:
        {
            UCPD_PE_PRL_Reset(port_number);

            break;
        }

        case PE_PRL_EVENT_HRD_RECEIVED:
        {
            UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_RESET_LAYER);
            break;
        }

        case PE_PRL_EVENT_HRD_SENT:
        case PE_PRL_EVENT_HARD_RESET_TIMEOUT:
        {
            UCPD_PRL_SM_HRD_Enter(port_number, PRL_HR_STATE_PHY_HARD_RESET_REQUESTED);
            break;
        }

        case PE_PRL_EVENT_MSG_TX_DISCARDED:
        case PE_PRL_EVENT_CRC_RX_TIMEOUT:
        {
            if(pe_prl->prl_sm.rx_state == PRL_RX_STATE_SEND_GOODCRC)
            {
                UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_WAIT_FOR_PHY_MESSAGE);
            } 
            else if (pe_prl->prl_sm.tx_state == PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE)
            {
                UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CHECK_RETRY_COUNTER);
            }
            break;
        }

        case PE_PRL_EVENT_MSG_TX_COMPLETED:
        {
            if(pe_prl->prl_sm.txBuffer.msg.header.message_type == UCPD_GOODCRC_MSG_ID)
            {
                UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_CHECK_MESSAGE_ID);
            }
            else if (pe_prl->prl_sm.tx_state == PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE)
            {
                UCPD_TIM_Start(pe_prl->port_number, UCPD_TIM_CRC_RECEIVE);
            }
            break;
        }

        case PE_PRL_EVENT_MSG_RX_COMPLETE:
        {
            if(pe_prl->prl_sm.rxBuffers[1u - pe_prl->prl_sm.rxBufInd].msg.header.message_type == UCPD_SOFT_RESET_MSG_ID)
            {
                UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_LAYER_RESET_FOR_TRANSMIT);
                UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_LAYER_RESET_FOR_RECEIVE);
                
            }
            else if ((pe_prl->prl_sm.rxBuffers[1u - pe_prl->prl_sm.rxBufInd].msg.header.message_type == UCPD_GOODCRC_MSG_ID) &&
                    (pe_prl->prl_sm.tx_state == PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE))
            {
                UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_MATCH_MESSAGE_ID);
            }
            else
            {
                if(pe_prl->prl_sm.tx_state != PRL_TX_STATE_WAIT_FOR_PHY_RESPONSE)
                    pe_prl->prl_sm.unexpected_or_unrecognized_message = UCPD_TRUE;

                UCPD_PRL_SM_RX_Enter(port_number, PRL_RX_STATE_SEND_GOODCRC);
            }

            break;
        }

        //reminder: already passed the CAD
        case PE_PRL_EVENT_TX_NG:
        {
            
            break;
        }

        case PE_PRL_EVENT_TX_OK:
        {
            if(pe_prl->prl_sm.tx_state == PRL_TX_STATE_SNK_PENDING)
            {
                UCPD_PRL_SM_TX_Enter(port_number, PRL_TX_STATE_CONSTRUCT_MESSAGE);
            }
            break;
        }

        case PE_PRL_EVENT_VBUS_PRESENT:
        {
            if(pe_prl->pe_sm.snk_state == PE_SNK_STATE_DISCOVERY)
            {
                UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_WAIT_FOR_CAPABILITIES);
            }
            break;
        }


        
        case PE_PRL_EVENT_BIST_CONT_MODE_TIMEROUT:
        {
            UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_TRANSITION_TO_DEFAULT);
            break;
        }

        case PE_PRL_EVENT_SENDER_REQ_TIMER_TIMEOUT:
        {
            if(pe_prl->prl_sm.rxBuffers[1u - pe_prl->prl_sm.rxBufInd].msg.ext_msg.ext_header.chunk_number == 0)
            {
                UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_MESSAGE_SENT);
            }
            else
            {
                UCPD_PRL_SM_CHUNKED_TX_Enter(port_number, PRL_TCH_STATE_REPORT_ERROR);
            }
            break;
        }

        case PE_PRL_EVENT_CHUNK_RES_TIMER_TIMEOUT:
        {
            pe_prl->pe_sm.chunk_wait_expired = UCPD_TRUE;
            UCPD_PRL_SM_CHUNKED_RX_Enter(port_number, PRL_RCH_STATE_REPORT_ERROR);
            break;
        }


        case PE_PRL_EVENT_PST_TRANS_TIMEOUT:
        {
            if(UCPD_CNT_Get(port_number, UCPD_CNT_HARD_RESET) <= UCPD_CNT_N_HARD_RESET)
            {
                UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_HARD_RESET);
            }
            break;
        }

        case PE_PRL_EVENT_SRT_EXPIRED:
        {
            UCPD_PE_SM_SRT_Enter(port_number, PE_SRT_STATE_EXPIRED);
            break;
        }

        case PE_PRL_EVENT_SNK_PPS_PER_TIMEOUT:
        {
            pe_prl->pe_sm.pps_periodic_expired = UCPD_TRUE;
            UCPD_PE_SM_SNK_Enter(port_number, PE_SNK_STATE_EVALUATE_CAPABILITY);
            break;
        }

        default:
        {
            break;
        }

    }
}

#endif
