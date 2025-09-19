#include "ucpd/ucpd_port.h"
#include "ucpd.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_dpm.h"
#include "ucpd/ucpd_pe_prl.h"

UCPD_Status UCPD_PORT_Init(UCPD_PORT_Number port_number) {
 

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if(port->initialized == UCPD_TRUE)
    return UCPD_STATUS_INITIALIZED;

  CLEAR_STRUCT_PTR(port);

  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].msg.header.port_data_role =
      UCPD_PORT_DATA_ROLE_UFP;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].msg.header.spec_revision =
      UCPD_SPEC_REVISION_3_0;
  port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].msg.header.port_power_role =
      UCPD_PORT_POWER_ROLE_SINK;

  port->pe_prl_cad.phy_event = PE_PRL_CAD_EVENT_NONE;
  port->pe_prl_cad.pwr_event = PE_PRL_CAD_EVENT_NONE;
  port->pe_prl_cad.timer_event = PE_PRL_CAD_EVENT_NONE;

  for(UCPD_CNT cnt = 0; cnt < UCPD_CNT_NUMBER; cnt++)
    {
        UCPD_CNT_Init(port_number, cnt);
    }

  // Init CAD
  UCPD_CAD_Init(port_number);

  UCPD_TIM_Init(port_number);

  UCPD_PWR_Init(port_number);

  UCPD_CAD_Start(port_number);

  port->initialized = UCPD_TRUE;

  return UCPD_STATUS_SUCCESS;
}