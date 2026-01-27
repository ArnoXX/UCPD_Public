#include "device/ucpd_hw_defines.h"
#include "ucpd/ucpd_hw.h"


/* CC utils */
UCPD_PE_PRL_CAD_Event getCADEventFromCC(UCPD_PORT_Number port_number)
{
	UCPD_PE_PRL_CAD_Event event = CAD_EVENT_CONN_REMOVED;
	UCPD_TypeDef* instance = getPortHandles()[port_number].instance;

	getPortHandles()[port_number].cc = UCPD_CCNONE;

	uint32_t CC1_val = instance->SR & UCPD_SR_TYPEC_VSTATE_CC1;
	uint32_t CC2_val = instance->SR & UCPD_SR_TYPEC_VSTATE_CC2;

	if (UCPD_CC_DETECT_VOPEN(CC1_val, CC1) && !UCPD_CC_DETECT_VOPEN(CC2_val, CC2))
		getPortHandles()[port_number].cc = UCPD_CC2;
	else if (!UCPD_CC_DETECT_VOPEN(CC1_val, CC1) && UCPD_CC_DETECT_VOPEN(CC2_val, CC2))
		getPortHandles()[port_number].cc = UCPD_CC1;
	else
		return event;

	event = CAD_EVENT_CONN_DETECTED;

	return event;
}
/* PHY events handler */
void UCPD_PD_PHY_Event_Handle(UCPD_PORT_Number port_number, UCPD_PHY_Event event)
{

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	switch (event)
	{
		case PHY_EVENT_CC_CHANGE:
		{
			port->pe_prl_cad.phy_event = getCADEventFromCC(port_number);
			break;
		}

		case PHY_EVENT_HRD_SENT:
		{
			port->pe_prl_cad.phy_event = PE_PRL_EVENT_HRD_SENT;
			break;
		}

		case PHY_EVENT_HRD_RECIEVED:
		{
			port->pe_prl_cad.phy_event = PE_PRL_EVENT_HRD_RECEIVED;
			port->pe_prl_cad.hrdOriginPE = false;
			break;
		}

		case PHY_EVENT_MSG_TX_DISCARDED:
		{
			port->pe_prl_cad.phy_event = PE_PRL_EVENT_MSG_TX_DISCARDED;
			break;
		}

		case PHY_EVENT_MSG_TX_COMPLETED:
		{
			port->pe_prl_cad.phy_event = PE_PRL_EVENT_MSG_TX_COMPLETED;
			break;
		}

		case PHY_EVENT_MSG_RX_COMPLETE:
		{
			port->pe_prl_cad.phy_event = PE_PRL_EVENT_MSG_RX_COMPLETE;
			break;
		}

		default:
		{

			break;
		}
	}

	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_PHY);
}

/* PE/PRL/CAD worker wrapper */
void UCPD_Worker(UCPD_PORT_Number port_number)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	UCPD_PE_PRL_CAD_Module* pe_prl = &port->pe_prl_cad;

	BLOCK_EVENT(UCPD_TIMER_EVENT);
	BLOCK_EVENT(UCPD_ADC_EVENT);
	UCPD_INT_SET(getPortHandles()[port_number].instance->IMR, UCPD_INT_PHY_WORKER_MASK);

	// When any of PHY, PWR or TIM set the event and then pend the worker, it
	// may already be in the middle of checking the events
	// So we only set the actual events that the worker will see
	pe_prl->current_phy_event = pe_prl->phy_event;
	pe_prl->current_pwr_event = pe_prl->pwr_event;
	pe_prl->current_timer_event = pe_prl->timer_event;

	pe_prl->phy_event = PE_PRL_CAD_EVENT_NONE;
	pe_prl->pwr_event = PE_PRL_CAD_EVENT_NONE;
	pe_prl->timer_event = PE_PRL_CAD_EVENT_NONE;

	pe_prl->rx_buffer = &getPortHandles()[port_number].buffers[getPortHandles()[port_number].rx_buffer_index];
	pe_prl->tx_buffer = &getPortHandles()[port_number].buffers[UCPD_TX_BUFFER_INDEX];
	pe_prl->ext_rx_buffer = &getPortHandles()[port_number].ext_buffers[UCPD_EXT_RX_BUFFER_INDEX];
	pe_prl->ext_tx_buffer = &getPortHandles()[port_number].ext_buffers[UCPD_EXT_TX_BUFFER_INDEX];

	// if hard reset is coming from PE, do not block the timer and phy events
	// if it comes from the hardware, we do not want the timer or phy to interfere with the hard reset process
	if (!(port->pe_prl_cad.hardResetActive) || (port->pe_prl_cad.hardResetActive && port->pe_prl_cad.hrdOriginPE))
	{
		UCPD_INT_SET(getPortHandles()[port_number].instance->IMR, UCPD_INT_NORMAL_MASK);
		UNBLOCK_EVENT(UCPD_TIMER_EVENT);
	}
	else
	{
		pe_prl->current_timer_event = PE_PRL_CAD_EVENT_NONE;
	}

	UNBLOCK_EVENT(UCPD_ADC_EVENT);

	UCPD_PE_PRL_Handler(port_number);
}

/* HRD helper */
void UCPD_ISR_HRD_Helper(UCPD_PORT_Number port_number)
{
	BLOCK_EVENT(UCPD_PE_PRL_WORKER_EVENT);
	BLOCK_EVENT(UCPD_TIMER_EVENT);
	BLOCK_EVENT(UCPD_ADC_EVENT); // will get unblocked by the worker anyway
	BLOCK_EVENT(UCPD_HW_PHY_EVENT);
	UNBLOCK_EVENT(UCPD_PE_PRL_WORKER_EVENT);

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	port->pe_prl_cad.phy_event = PE_PRL_EVENT_HRD_RECEIVED;
	UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_PHY);
}

inline void UCPD_PE_PRL_Event_Emit(UCPD_PORT_Number port_number,
                                          UCPD_PD_Event_Source source) {
  (void)port_number;
  (void)source;

  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);
}
