#include "ucpd/ucpd_port.h"
#include "ucpd.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_dpm.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd/ucpd_pe_prl.h"


UCPD_Status UCPD_PORT_Init(UCPD_PORT_Number port_number) {

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  if (port->initialized)
    return UCPD_STATUS_INITIALIZED;

  *port = (UCPD_PORT_INSTANCE){0};

  // init pe_prl_cad
  UCPD_PE_PRL_CAD_Init(port_number, &port->pe_prl_cad);

  // init hw handle
  port->port_handle = (UCPD_HW_PORT_Handle){
      .instance = UCPD_INSTANCE,
      .cc = UCPD_CCNONE,
      .rxData = NULL,
      .txData = NULL,
  };

  UCPD_PHY_Init(port_number);

  UCPD_PHY_AssertRd(port_number);

  // init PWR
  UCPD_PWR_SM_Init(&port->pwr_sm);

  // init timers
  UCPD_TIM_Init(port_number);

  // init counters
  for (UCPD_CNT cnt = 0; cnt < UCPD_CNT_NUMBER; cnt++) {
    UCPD_CNT_Init(port_number, cnt);
  }

  UCPD_CAD_Start(port_number); // move this somewhere else

  port->initialized = true;

  return UCPD_STATUS_SUCCESS;
}