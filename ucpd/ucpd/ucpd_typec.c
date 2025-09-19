#include "ucpd_typec.h"
#include "device/trace_utils.h"
#include "device/ucpd_hw_defines.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd/ucpd_tim.h"
#include "ucpd_ctx.h"
#include "ucpd_pe_prl.h"
#include "ucpd_utils.h"

void UCPD_CAD_Init(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->port_handle.instance = UCPD_INSTANCE;
  port->port_handle.cc = UCPD_CCNONE;

  UCPD_PHY_Init(port_number);

  UCPD_PHY_AssertRd(port_number);
}

void UCPD_CAD_Start(UCPD_PORT_Number port_number) {
  UCPD_PHY_EnableEvents(port_number);

  TRACE_INFO("CAD Started\n\r");
}

/* Event handle function */
void UCPD_CAD_SM_Delta(UCPD_PORT_Number port_number,
                       UCPD_PE_PRL_CAD_Event event) {
  UCPD_PE_PRL_CAD_Module *pe_prl_cad =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;
  UCPD_CAD_SM_State *state = &pe_prl_cad->cad_state;

  switch (event) {
  case CAD_EVENT_CONN_DETECTED: {
    if (*state == CAD_STATE_UNATTACHED || *state == CAD_STATE_NONE) {
      UCPD_CAD_SM_Enter(port_number, CAD_STATE_ATTACH_WAIT);
    } else if (*state == CAD_STATE_ATTACHED &&
               UCPD_PHY_IsSinkTxOk(port_number) == UCPD_TRUE)
      UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_TX_OK);

    break;
  }

  case CAD_EVENT_CONN_REMOVED: {
    if (*state == CAD_STATE_ATTACH_WAIT) {
      pe_prl_cad->tccDeb = UCPD_FALSE;

      UCPD_TIM_Stop(port_number, UCPD_TIM_CAD_CCDEB);
      UCPD_TIM_Start(port_number, UCPD_TIM_CAD_PDDEB);
    }

    break;
  }

  case CAD_EVENT_VBUS_DETECTED: {
    if (pe_prl_cad->hardResetActive == UCPD_TRUE) {
      pe_prl_cad->hardResetActive = UCPD_FALSE;
      break;
    }

    if (*state == CAD_STATE_ATTACHED) {
      UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_VBUS_PRESENT);
    }

    if (pe_prl_cad->tccDeb == UCPD_TRUE) {
      UCPD_CAD_SM_Enter(port_number, CAD_STATE_ATTACHED);

      pe_prl_cad->tccDeb = UCPD_FALSE;
    } else
      pe_prl_cad->vbusDet = UCPD_TRUE;

    break;
  }

  case CAD_EVENT_TCDEBOUNCE_TIMEOUT: {
    if (*state != CAD_STATE_ATTACH_WAIT)
      break;

    if (pe_prl_cad->vbusDet == UCPD_TRUE) {
      pe_prl_cad->vbusDet = UCPD_FALSE;

      UCPD_CAD_SM_Enter(port_number, CAD_STATE_ATTACHED);

     
    } else
      pe_prl_cad->tccDeb = UCPD_TRUE;

    break;
  }

  case CAD_EVENT_VBUS_REMOVED: {
    if (pe_prl_cad->hardResetActive == UCPD_TRUE)
      break;

    pe_prl_cad->vbusDet = UCPD_FALSE;
    if (*state == CAD_STATE_ATTACHED)
      UCPD_CAD_SM_Enter(port_number, CAD_STATE_UNATTACHED);

    break;
  }

  case CAD_EVENT_ERROR: {
    TRACE_STATE_CHANGE("CAD", "Unknown", "CAD_STATE_ERROR_RECOVERY");
    UCPD_TIM_Stop(port_number, UCPD_TIM_CAD_CCDEB);
    UCPD_TIM_Stop(port_number, UCPD_TIM_CAD_PDDEB);

    UCPD_PHY_EnterErrorRecovery(port_number);

    UCPD_TIM_Start(port_number, UCPD_TIM_CAD_ERROR_REC);

    UCPD_CTX_GetPortInstance(port_number)->port_handle.cc = UCPD_CCNONE;

    *state = CAD_STATE_ERROR_RECOVERY;

    break;
  }

  case CAD_EVENT_TPDEBOUNCE_TIMEOUT: {

    UCPD_CAD_SM_Enter(port_number, CAD_STATE_UNATTACHED);

    break;
  }

  case CAD_EVENT_TERROR_RECOVERY: {
    UCPD_CAD_SM_Enter(port_number, CAD_STATE_UNATTACHED);
    break;
  }

  default: {
    break;
  }
  };
}

/* State enter function */
void UCPD_CAD_SM_Enter(UCPD_PORT_Number port_number, UCPD_CAD_SM_State state) {

  UCPD_CAD_SM_State *current_state =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad.cad_state;

  switch (state) {
  case CAD_STATE_UNATTACHED: {
    if (*current_state == CAD_STATE_ATTACHED)
      TRACE_STATE_CHANGE("CAD", "CAD_STATE_ATTACHED", "CAD_STATE_UNATTACHED");
    else
      TRACE_STATE_CHANGE("CAD", "CAD_STATE_ATTACH_WAIT",
                         "CAD_STATE_UNATTACHED");

    if (*current_state == CAD_STATE_ATTACHED)
      UCPD_PHY_Detach(port_number);
    else if (*current_state == CAD_STATE_ERROR_RECOVERY)
      UCPD_PHY_AssertRd(port_number);

    for (UCPD_TIM t = UCPD_TIM_BIST_CONT_MODE; t < UCPD_TIM_NUMBER; t++) {
      UCPD_TIM_Stop(port_number, t);
    }

    

    *current_state = state;
    break;
  }

  case CAD_STATE_ATTACH_WAIT: {
    TRACE_STATE_CHANGE("CAD", "CAD_STATE_UNATTACHED", "CAD_STATE_ATTACH_WAIT");

    UCPD_TIM_Stop(port_number, UCPD_TIM_CAD_PDDEB);
    UCPD_TIM_Start(port_number, UCPD_TIM_CAD_CCDEB);
    *current_state = state;
    break;
  }

  case CAD_STATE_ATTACHED: {
    TRACE_STATE_CHANGE("CAD", "CAD_STATE_ATTACH_WAIT", "CAD_STATE_ATTACHED");
    UCPD_PRL_SM_Delta(port_number, PE_PRL_EVENT_PRL_INIT);
    UCPD_PE_SM_Delta(port_number, PE_PRL_EVENT_PRL_INIT);
    UCPD_PHY_Attach(port_number);

    *current_state = state;
    break;
  }

  default: {
    break;
  }
  };
}
