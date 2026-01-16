#include "ucpd_tim.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd_ctx.h"



void UCPD_TIM_Start(UCPD_PORT_Number port_number, UCPD_TIM timer)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);
	if (timer == UCPD_TIM_NONE || timer == UCPD_TIM_NUMBER || port->ucpd_tim_at_channel[UCPD_TIMER_CHANNEL[timer]] == timer)
	{
		return;
	}
	port->ucpd_tim_at_channel[UCPD_TIMER_CHANNEL[timer]] = timer;
	UCPD_HW_Timer_Start(port_number, UCPD_GET_HW_CHANNEL(UCPD_TIMER_CHANNEL[timer]), UCPD_TIMERS_PERIODS[timer]);
}

void UCPD_TIM_Stop(UCPD_PORT_Number port_number, UCPD_TIM timer)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);
	if (timer == UCPD_TIM_NONE || timer == UCPD_TIM_NUMBER)
	{
		return;
	}

	if (port->ucpd_tim_at_channel[UCPD_TIMER_CHANNEL[timer]] == timer)
	{
		UCPD_HW_Timer_Stop(port_number, UCPD_GET_HW_CHANNEL(UCPD_TIMER_CHANNEL[timer]));
		port->ucpd_tim_at_channel[UCPD_TIMER_CHANNEL[timer]] = UCPD_TIM_NONE;
	}
}

void UCPD_TIM_Expired(UCPD_PORT_Number port_number, uint32_t timer_channel)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);
	UCPD_TIM timer = port->ucpd_tim_at_channel[timer_channel];
	if (timer < UCPD_TIM_NUMBER)
	{

		port->ucpd_tim_at_channel[timer_channel] = UCPD_TIM_NONE;

		UCPD_PE_PRL_CAD_Module* pe_prl = &port->pe_prl_cad;
		UCPD_PE_PRL_CAD_Event event = UCPD_TIMERS_EVENTS[timer];

		pe_prl->timer_event = event;


		UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_TIM);
	}
}

void UCPD_TIM_Init(UCPD_PORT_Number port_number)
{
	UCPD_HW_Timer_Init(port_number);
}

void UCPD_TIM_Deinit(UCPD_PORT_Number port_number)
{
	UCPD_HW_Timer_Deinit(port_number);
}

