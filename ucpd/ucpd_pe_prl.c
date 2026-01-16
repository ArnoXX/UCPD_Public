#include "ucpd/ucpd_pe_prl.h"
#include "ucpd/ucpd_typec.h"
#include "ucpd_ctx.h"
#include "ucpd_pe_prl.h"
#include "ucpd_port_num.h"
#include <string.h>

void UCPD_PE_PRL_CAD_Init(UCPD_PORT_Number port_number, UCPD_PE_PRL_CAD_Module* pe_prl_cad)
{
	*pe_prl_cad = (UCPD_PE_PRL_CAD_Module){.port_number = port_number,
	                                       .cad_state = CAD_STATE_UNATTACHED,
	                                       .rx_buffer = NULL,
	                                       .tx_buffer = NULL,
	                                       .ext_rx_buffer = NULL,
	                                       .ext_tx_buffer = NULL};
}

void UCPD_PE_PRL_CAD_Reset(UCPD_PE_PRL_CAD_Module* pe_prl_cad)
{
	UCPD_PORT_Number port_number = pe_prl_cad->port_number;

	UCPD_PE_PRL_CAD_Init(port_number, pe_prl_cad);
}

void UCPD_PE_PRL_Handler(UCPD_PORT_Number port_number)
{

	UCPD_PE_PRL_CAD_Module* pe_prl_cad = &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;

	// first check the pwr events for vbus removal
	// may exit earlier if detached
	// if hard reset is ongoing it will do nothing
	if (pe_prl_cad->current_pwr_event != PE_PRL_CAD_EVENT_NONE)
	{
		UCPD_CAD_SM_Delta(pe_prl_cad, pe_prl_cad->current_pwr_event);
		pe_prl_cad->current_pwr_event = PE_PRL_CAD_EVENT_NONE;
	}

	// after vbus removal we want to give priority to connection removed and hard reset
	if (pe_prl_cad->current_phy_event == CAD_EVENT_CONN_REMOVED)
	{
		UCPD_CAD_SM_Delta(pe_prl_cad, pe_prl_cad->current_phy_event);
		pe_prl_cad->current_phy_event = PE_PRL_CAD_EVENT_NONE;
	}

	if (pe_prl_cad->current_phy_event == PE_PRL_EVENT_HRD_RECEIVED)
	{
		UCPD_PRL_SM_Delta(pe_prl_cad, pe_prl_cad->current_phy_event);
		pe_prl_cad->current_phy_event = PE_PRL_CAD_EVENT_NONE;
		pe_prl_cad->current_timer_event = PE_PRL_CAD_EVENT_NONE;
	}

	// next go the timers, before the messages
	if (pe_prl_cad->current_timer_event != PE_PRL_CAD_EVENT_NONE)
	{
		UCPD_CAD_SM_Delta(pe_prl_cad, pe_prl_cad->current_timer_event);
		UCPD_PRL_SM_Delta(pe_prl_cad, pe_prl_cad->current_timer_event);
		UCPD_PE_SM_Delta(pe_prl_cad, pe_prl_cad->current_timer_event);
		pe_prl_cad->current_timer_event = PE_PRL_CAD_EVENT_NONE;
	}

	// timers take priority over received and transmitted messages even though interrupts are on the same level
	// i.e. if we get goodcrc message is received and corresponding timer expires, we will
	// process the timer first
	if (pe_prl_cad->current_phy_event != PE_PRL_CAD_EVENT_NONE)
	{
		UCPD_CAD_SM_Delta(pe_prl_cad, pe_prl_cad->current_phy_event);
		UCPD_PRL_SM_Delta(pe_prl_cad, pe_prl_cad->current_phy_event);

		pe_prl_cad->current_phy_event = PE_PRL_CAD_EVENT_NONE;
	}

	// for first ever call of async api during operation
	// also covers the case when callback is set to null in api call
	if (pe_prl_cad->current_callback == NULL) pe_prl_cad->current_callback = pe_prl_cad->callback;

	// dpm requests have lowest priority
	if (pe_prl_cad->dpm_event == PE_PRL_CAD_EVENT_NONE) return;

	// process events only if in ready state and there is an event to process
	if (pe_prl_cad->snk_state == PE_SNK_STATE_READY)
	{
		// setting to none before delta in case callback contains calls to async functions
		UCPD_PE_PRL_CAD_Event copy = pe_prl_cad->dpm_event;
		pe_prl_cad->dpm_event = PE_PRL_CAD_EVENT_NONE;
		UCPD_PE_SM_Delta(pe_prl_cad, copy);

		// if callback is null it means no async api calls were made inside policy or callback
		// if callback is not null it means we have an inflight dpm request that was made inside policy or callback that
		// will be processed next time PE_SNK_STATE_READY state is reached
		pe_prl_cad->current_callback = pe_prl_cad->callback;
		pe_prl_cad->callback = NULL;
	}

	// do not cancel the request, just wait until the state machine is in ready PE_SNK_STATE_READY state
}
