#include "ucpd_tim.h"
#include "device/trace_utils.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd_ctx.h"

extern UCPD_Bool waiting_for_epr;

static void timer_expired_trace(UCPD_TIM timer) {
  switch (timer) {
  case UCPD_TIM_BIST_CONT_MODE:
    TRACE_TIMER_EXPIRED("BIST_CONT_MODE\n\r");
    break;
  case UCPD_TIM_CHUNK_SENDER_REQUEST:
    TRACE_TIMER_EXPIRED("CHUNK_SENDER_REQUEST\n\r");
    break;
  case UCPD_TIM_CHUNK_SENDER_RESPONSE:
    TRACE_TIMER_EXPIRED("CHUNK_SENDER_RESPONSE\n\r");
    break;
  case UCPD_TIM_CRC_RECEIVE:
    TRACE_TIMER_EXPIRED("CRC_RECEIVE\n\r");
    break;
  case UCPD_TIM_HARD_RESET_COMPLETE:
    TRACE_TIMER_EXPIRED("HARD_RESET_COMPLETE\n\r");
    break;
  case UCPD_TIM_PS_TRANSITION:
    TRACE_TIMER_EXPIRED("PS_TRANSITION\n\r");
    break;
  case UCPD_TIM_SENDER_RESPONSE:
    TRACE_TIMER_EXPIRED("SENDER_RESPONSE\n\r");
    break;
  case UCPD_TIM_SINK_PPS_PERIODIC:
    TRACE_TIMER_EXPIRED("SINK_PPS_PERIODIC\n\r");
    break;
  case UCPD_TIM_SINK_REQUEST:
    TRACE_TIMER_EXPIRED("SINK_REQUEST\n\r");
    break;
  case UCPD_TIM_SINK_WAIT_CAP:
    TRACE_TIMER_EXPIRED("SINK_WAIT_CAP\n\r");
    break;
  case UCPD_TIM_CAD_CCDEB:
    TRACE_TIMER_EXPIRED("CAD_CCDEB\n\r");
    break;
  case UCPD_TIM_CAD_PDDEB:
    TRACE_TIMER_EXPIRED("CAD_PDDEB\n\r");
    break;
  case UCPD_TIM_CAD_ERROR_REC:
    TRACE_TIMER_EXPIRED("CAD_ERROR_REC\n\r");
    break;
  default:
    break;
  }
}

void timer_start_trace(UCPD_TIM timer) {
  switch (timer) {
  case UCPD_TIM_CAD_CCDEB:
    TRACE_TIMER_START("CAD_CCDEB\n\r");
    break;
  case UCPD_TIM_CAD_PDDEB:
    TRACE_TIMER_START("CAD_PDDEB\n\r");
    break;
  case UCPD_TIM_CAD_ERROR_REC:
    TRACE_TIMER_START("CAD_ERROR_REC\n\r");
    break;
  case UCPD_TIM_BIST_CONT_MODE:
    TRACE_TIMER_START("BIST_CONT_MODE\n\r");
    break;
  case UCPD_TIM_CHUNK_SENDER_REQUEST:
    TRACE_TIMER_START("CHUNK_SENDER_REQUEST\n\r");
    break;
  case UCPD_TIM_CHUNK_SENDER_RESPONSE:
    TRACE_TIMER_START("CHUNK_SENDER_RESPONSE\n\r");
    break;
  case UCPD_TIM_CRC_RECEIVE:
    TRACE_TIMER_START("CRC_RECEIVE\n\r");
    break;
  case UCPD_TIM_HARD_RESET_COMPLETE:
    TRACE_TIMER_START("HARD_RESET_COMPLETE\n\r");
    break;
  case UCPD_TIM_PS_TRANSITION:
    TRACE_TIMER_START("PS_TRANSITION\n\r");
    break;
  case UCPD_TIM_SENDER_RESPONSE:
    TRACE_TIMER_START("SENDER_RESPONSE\n\r");
    break;
  case UCPD_TIM_SINK_PPS_PERIODIC:
    TRACE_TIMER_START("SINK_PPS_PERIODIC\n\r");
    break;
  case UCPD_TIM_SINK_REQUEST:
    TRACE_TIMER_START("SINK_REQUEST\n\r");
    break;
  case UCPD_TIM_SINK_WAIT_CAP:
    TRACE_TIMER_START("SINK_WAIT_CAP\n\r");
    break;
  default:
    break;
  }
}

void timer_stop_trace(UCPD_TIM timer) {
  switch (timer) {
  case UCPD_TIM_CAD_CCDEB:
    TRACE_TIMER_STOP("CAD_CCDEB\n\r");
    break;
  case UCPD_TIM_BIST_CONT_MODE:
    TRACE_TIMER_STOP("BIST_CONT_MODE\n\r");
    break;
  case UCPD_TIM_CHUNK_SENDER_REQUEST:
    TRACE_TIMER_STOP("CHUNK_SENDER_REQUEST\n\r");
    break;
  case UCPD_TIM_CHUNK_SENDER_RESPONSE:
    TRACE_TIMER_STOP("CHUNK_SENDER_RESPONSE\n\r");
    break;
  case UCPD_TIM_CRC_RECEIVE:
    TRACE_TIMER_STOP("CRC_RECEIVE\n\r");
    break;
  case UCPD_TIM_HARD_RESET_COMPLETE:
    TRACE_TIMER_STOP("HARD_RESET_COMPLETE\n\r");
    break;
  case UCPD_TIM_PS_TRANSITION:
	  waiting_for_epr = UCPD_FALSE;
    TRACE_TIMER_STOP("PS_TRANSITION\n\r");
    break;
  case UCPD_TIM_SENDER_RESPONSE:
    TRACE_TIMER_STOP("SENDER_RESPONSE\n\r");
    break;
  case UCPD_TIM_SINK_PPS_PERIODIC:
    TRACE_TIMER_STOP("SINK_PPS_PERIODIC\n\r");
    break;
  case UCPD_TIM_SINK_REQUEST:
    TRACE_TIMER_STOP("SINK_REQUEST\n\r");
    break;
  case UCPD_TIM_SINK_WAIT_CAP:
    TRACE_TIMER_STOP("SINK_WAIT_CAP\n\r");
    break;
  case UCPD_TIM_CAD_PDDEB:
    TRACE_TIMER_STOP("CAD_PDDEB\n\r");
    break;
  case UCPD_TIM_CAD_ERROR_REC:
    TRACE_TIMER_STOP("CAD_ERROR_REC\n\r");
    break;
  default:
    break;
  }
}

void UCPD_TIM_Start(UCPD_PORT_Number port_number, UCPD_TIM timer) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  port->ucpd_tim_at_channel[UCPD_TIMER_CHANNEL[timer]] = timer;
  UCPD_HW_Timer_Start(UCPD_GET_HW_CHANNEL(UCPD_TIMER_CHANNEL[timer]), UCPD_TIMERS_PERIODS[timer]);
  timer_start_trace(timer);
}

void UCPD_TIM_Stop(UCPD_PORT_Number port_number, UCPD_TIM timer) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  if (timer < UCPD_TIM_NUMBER &&
      port->ucpd_tim_at_channel[UCPD_TIMER_CHANNEL[timer]] == timer) {
    UCPD_HW_Timer_Stop(UCPD_GET_HW_CHANNEL(UCPD_TIMER_CHANNEL[timer]));
    port->ucpd_tim_at_channel[UCPD_TIMER_CHANNEL[timer]] = UCPD_TIM_NUMBER;
    timer_stop_trace(timer);
  }
}

void UCPD_TIM_Expired(UCPD_PORT_Number port_number, uint32_t timer_channel) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_TIM timer = port->ucpd_tim_at_channel[timer_channel];
  if (timer < UCPD_TIM_NUMBER) {

    port->ucpd_tim_at_channel[timer_channel] = UCPD_TIM_NUMBER;

    UCPD_PE_PRL_CAD_Module *pe_prl = &port->pe_prl_cad;
    UCPD_PE_PRL_CAD_Event event = UCPD_TIMERS_EVENTS[timer];

    pe_prl->timer_event = event;

    timer_expired_trace(timer);

    UCPD_PE_PRL_Event_Emit(PD_EVENT_SOURCE_TIM);
  }
}

void UCPD_TIM_Init(UCPD_PORT_Number port_number) {
  UCPD_HW_Timer_Init();
  TRACE_INFO("TIM Initialized\n\r");
}
