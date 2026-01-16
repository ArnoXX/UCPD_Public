#include "ucpd/ucpd_port.h"
#include "ucpd.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd/ucpd_pe_prl.h"
#include <stddef.h>

UCPD_Status UCPD_PORT_Init(UCPD_PORT_Number port_number)
{

	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (port->initialized) return UCPD_STATUS_INITIALIZED;

	*port = (UCPD_PORT_INSTANCE){0};

	// init pe_prl_cad
	UCPD_PE_PRL_CAD_Init(port_number, &port->pe_prl_cad);

	UCPD_PHY_Init(port_number);

	if (port->pe_prl_cad.rx_buffer == NULL || port->pe_prl_cad.tx_buffer == NULL ||
	    port->pe_prl_cad.ext_rx_buffer == NULL || port->pe_prl_cad.ext_tx_buffer == NULL)
	{
		UCPD_PHY_Deinit(port_number);
		return UCPD_STATUS_ERROR;
	}

	UCPD_PHY_AssertRd(port_number);

	// init PWR
	UCPD_PWR_Init(port_number);

	// init timers
	UCPD_TIM_Init(port_number);

	// init counters
	for (UCPD_CNT cnt = 0; cnt < UCPD_CNT_NUMBER; cnt++)
	{
		UCPD_CNT_Init(port_number, cnt);
	}

	UCPD_CAD_Start(port_number);

	port->initialized = true;

	return UCPD_STATUS_SUCCESS;
}

UCPD_Status UCPD_PORT_Deinit(UCPD_PORT_Number port_number)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	if (!port->initialized) return UCPD_STATUS_UNITIALIZED;

	UCPD_PHY_DeassertRd(port_number);

	UCPD_PHY_Deinit(port_number);

	UCPD_TIM_Deinit(port_number);

	UCPD_PWR_Deinit(port_number);

	port->initialized = false;

	return UCPD_STATUS_SUCCESS;
}
