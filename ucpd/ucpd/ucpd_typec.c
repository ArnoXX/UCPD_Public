#include "ucpd_typec.h"
#include "device/trace_utils.h"
#include "device/ucpd_hw_defines.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd/ucpd_tim.h"
#include "ucpd_ctx.h"
#include "ucpd_pe_prl.h"
#include "ucpd_utils.h"

void UCPD_CAD_Start(UCPD_PORT_Number port_number) {
  UCPD_PHY_EnableEvents(port_number);

  TRACE_INFO("CAD Started\n\r");
}

/* Event handle function */
void UCPD_CAD_SM_Delta(UCPD_PE_PRL_CAD_Module *pe_prl_cad,
                       UCPD_PE_PRL_CAD_Event event) {

  UCPD_CAD_SM_State *state = &pe_prl_cad->cad_state;

  switch (event) {
  case CAD_EVENT_CONN_DETECTED: {
    if (*state == CAD_STATE_UNATTACHED || *state == CAD_STATE_NONE) {
      UCPD_CAD_SM_Enter(pe_prl_cad, CAD_STATE_ATTACH_WAIT);
    } else if (*state == CAD_STATE_ATTACHED &&
               UCPD_PHY_IsSinkTxOk(pe_prl_cad->port_number))
      UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_TX_OK);

    break;
  }

  case CAD_EVENT_CONN_REMOVED: {
    if (*state == CAD_STATE_ATTACH_WAIT) {
      pe_prl_cad->tccDeb = false;

      UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_CAD_CCDEB);
      UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_CAD_PDDEB);
    }

    break;
  }

  case CAD_EVENT_VBUS_DETECTED: {
    if (pe_prl_cad->hardResetActive) {
      pe_prl_cad->hardResetActive = false;
      break;
    }

    if (*state == CAD_STATE_ATTACHED) {
      UCPD_PE_SM_Delta(pe_prl_cad, PE_PRL_EVENT_VBUS_PRESENT);
    }

    if (pe_prl_cad->tccDeb) {
      UCPD_CAD_SM_Enter(pe_prl_cad, CAD_STATE_ATTACHED);

      pe_prl_cad->tccDeb = false;
    } else
      pe_prl_cad->vbusDet = true;

    break;
  }

  case CAD_EVENT_TCDEBOUNCE_TIMEOUT: {
    if (*state != CAD_STATE_ATTACH_WAIT)
      break;

    if (pe_prl_cad->vbusDet) {

      UCPD_CAD_SM_Enter(pe_prl_cad, CAD_STATE_ATTACHED);

    } else
      pe_prl_cad->tccDeb = true;

    break;
  }

  case CAD_EVENT_VBUS_REMOVED: {
    if (pe_prl_cad->hardResetActive)
      break;

    pe_prl_cad->vbusDet = false;
    if (*state == CAD_STATE_ATTACHED)
      UCPD_CAD_SM_Enter(pe_prl_cad, CAD_STATE_UNATTACHED);

    break;
  }

  case CAD_EVENT_ERROR: {
    TRACE_STATE_CHANGE("CAD", "Unknown", "CAD_STATE_ERROR_RECOVERY");
    UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_CAD_CCDEB);
    UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_CAD_PDDEB);

    UCPD_PHY_EnterErrorRecovery(pe_prl_cad->port_number);

    UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_CAD_ERROR_REC);

    UCPD_CTX_GetPortInstance(pe_prl_cad->port_number)->port_handle.cc =
        UCPD_CCNONE;

    UCPD_CAD_SM_Enter(pe_prl_cad, CAD_STATE_ERROR_RECOVERY);

    break;
  }

  case CAD_EVENT_TPDEBOUNCE_TIMEOUT: {

    UCPD_CAD_SM_Enter(pe_prl_cad, CAD_STATE_UNATTACHED);

    break;
  }

  case CAD_EVENT_TERROR_RECOVERY: {
    UCPD_CAD_SM_Enter(pe_prl_cad, CAD_STATE_UNATTACHED);
    break;
  }

  default: {
    break;
  }
  };
}

/* State enter function */
void UCPD_CAD_SM_Enter(UCPD_PE_PRL_CAD_Module *pe_prl_cad,
                       UCPD_CAD_SM_State state) {

  UCPD_CAD_SM_State *current_state = &pe_prl_cad->cad_state;

  switch (state) {
  case CAD_STATE_UNATTACHED: {
    if (*current_state == CAD_STATE_ATTACHED)
      TRACE_STATE_CHANGE("CAD", "CAD_STATE_ATTACHED", "CAD_STATE_UNATTACHED");
    else
      TRACE_STATE_CHANGE("CAD", "CAD_STATE_ATTACH_WAIT",
                         "CAD_STATE_UNATTACHED");

    if (*current_state == CAD_STATE_ATTACHED)
    {
      UCPD_PHY_Detach(pe_prl_cad->port_number);
      pe_prl_cad->phy_event = PE_PRL_CAD_EVENT_NONE;
      pe_prl_cad->timer_event = PE_PRL_CAD_EVENT_NONE;
      pe_prl_cad->pwr_event = PE_PRL_CAD_EVENT_NONE;
    }
    else if (*current_state == CAD_STATE_ERROR_RECOVERY)
      UCPD_PHY_AssertRd(pe_prl_cad->port_number);

    for (UCPD_TIM t = 1u; t < UCPD_TIM_NUMBER; t++) {
      UCPD_TIM_Stop(pe_prl_cad->port_number, t);
    }

    *current_state = state;
    break;
  }

  case CAD_STATE_ATTACH_WAIT: {
    TRACE_STATE_CHANGE("CAD", "CAD_STATE_UNATTACHED", "CAD_STATE_ATTACH_WAIT");

    UCPD_TIM_Stop(pe_prl_cad->port_number, UCPD_TIM_CAD_PDDEB);
    UCPD_TIM_Start(pe_prl_cad->port_number, UCPD_TIM_CAD_CCDEB);
    *current_state = state;
    break;
  }

  case CAD_STATE_ATTACHED: {
    TRACE_STATE_CHANGE("CAD", "CAD_STATE_ATTACH_WAIT", "CAD_STATE_ATTACHED");
    UCPD_PE_PRL_CAD_Reset(pe_prl_cad);
    
    UCPD_PRL_SM_Delta(pe_prl_cad, PE_PRL_EVENT_PRL_INIT);
    UCPD_PE_SM_Delta(pe_prl_cad, PE_PRL_EVENT_PRL_INIT);
    UCPD_PHY_Attach(pe_prl_cad->port_number);

    *current_state = state;
    break;
  }

  case CAD_STATE_ERROR_RECOVERY: {

    *current_state = state;
    break;
  }

  default: {
    break;
  }
  };
}
