#ifndef UCPD_HW_H
#define UCPD_HW_H

#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_tim.h"

/* HAL */

typedef enum
{
	PD_EVENT_SOURCE_PHY,
	PD_EVENT_SOURCE_PWR,
	PD_EVENT_SOURCE_TIM,
	PD_EVENT_SOURCE_DPM,
	PD_EVENT_SOURCE_NONE
} UCPD_PD_Event_Source;

/* PD PHY configuration */
void UCPD_PHY_Init(UCPD_PORT_Number port_number);

void UCPD_PHY_Deinit(UCPD_PORT_Number port_number);

void UCPD_PHY_EnableEvents(UCPD_PORT_Number port_number);

void UCPD_PHY_DisableEvents(UCPD_PORT_Number port_number);

/* PD event emitter */
void UCPD_PE_PRL_Event_Emit(UCPD_PORT_Number port_number, UCPD_PD_Event_Source source);

void UCPD_PHY_HRDRS_Completed(UCPD_PORT_Number port_number);

/* PHY interface for UCPD stack */
void UCPD_PHY_SendMessage(UCPD_PORT_Number port_number);

void UCPD_PHY_SendHardReset(UCPD_PORT_Number port_number);

void UCPD_PHY_AssertRd(UCPD_PORT_Number port_number);

void UCPD_PHY_DeassertRd(UCPD_PORT_Number port_number);

void UCPD_PHY_Detach(UCPD_PORT_Number port_number);

void UCPD_PHY_Attach(UCPD_PORT_Number port_number);

void UCPD_PHY_EnterErrorRecovery(UCPD_PORT_Number port_number);

bool UCPD_PHY_IsSinkTxOk(UCPD_PORT_Number port_number);

/* TIM interface for UCPD stack */
void UCPD_HW_Timer_Start(UCPD_PORT_Number port_number, uint32_t timer_channel, UCPD_Time period);

void UCPD_HW_Timer_Stop(UCPD_PORT_Number port_number, uint32_t timer_channel);

void UCPD_HW_Timer_Init(UCPD_PORT_Number port_number);

void UCPD_HW_Timer_Deinit(UCPD_PORT_Number port_number);

/* PWR interface for UCPD stack */
void UCPD_PWR_Init(UCPD_PORT_Number port_number);

void UCPD_PWR_Deinit(UCPD_PORT_Number port_number);

bool UCPD_PWR_vS0_to_vS5(UCPD_PORT_Number port_number);

bool UCPD_PWR_IsVbusPresent(UCPD_PORT_Number port_number);

#endif // UCPD_HW_H
