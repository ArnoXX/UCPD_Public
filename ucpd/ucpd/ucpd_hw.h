#ifndef UCPD_HW_H
#define UCPD_HW_H

#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_tim.h"


/* HAL */
/* PD events */
typedef enum {
  PHY_EVENT_CC_CHANGE,
  PHY_EVENT_HRD_RECIEVED,
  PHY_EVENT_HRD_SENT,
  PHY_EVENT_MSG_TX_DISCARDED,
  PHY_EVENT_MSG_TX_COMPLETED,
  PHY_EVENT_MSG_RX_COMPLETE,
  PHY_EVENT_NONE
} UCPD_PHY_Event;

typedef enum {
  PD_EVENT_SOURCE_PHY,
  PD_EVENT_SOURCE_PWR,
  PD_EVENT_SOURCE_TIM,
  PD_EVENT_SOURCE_NONE
} UCPD_PD_Event_Source;

/* PD PHY configuration */
void UCPD_PHY_Init(UCPD_PORT_Number port_number);

void UCPD_PHY_EnableEvents(UCPD_PORT_Number port_number);

/* PD event handler */
void UCPD_PD_PHY_Event_Handle(UCPD_PORT_Number port_number,
                              UCPD_PHY_Event event);

/* PD event emitter */
void UCPD_PE_PRL_Event_Emit(UCPD_PD_Event_Source source);

/* PHY event handlers */
void UCPD_PHY_Handler(void);

/* Main PHY event handler */
void UCPD_PHY_Event_Handler(void);

/* Individual events handlers */
void UCPD_PHY_RX_Complete(UCPD_PORT_Number port_number, UCPD_Bool error);

void UCPD_PHY_TX_Complete(UCPD_PORT_Number port_number);

void UCPD_PHY_HRDRS_Received(UCPD_PORT_Number port_number);

void UCPD_PHY_HRDRS_Completed(UCPD_PORT_Number port_number);

void UCPD_PHY_HRDRS_Rejected(UCPD_PORT_Number port_number);

void UCPD_PHY_RX_Started(UCPD_PORT_Number port_number);

void UCPD_PHY_TX_Discarded(UCPD_PORT_Number port_number);

void UCPD_PHY_HRDRS_Detected(UCPD_PORT_Number port_number);

void UCPD_PHY_CC_Event(UCPD_PORT_Number port_number);

void UCPD_PHY_Worker_Finished(UCPD_PORT_Number port_number);

UCPD_PE_PRL_CAD_Event getCADEventFromCC(UCPD_PORT_Number port_number);

/* PHY messages */

void UCPD_PHY_SendMessage(UCPD_PORT_Number port_number);

void UCPD_PHY_SendHardReset(UCPD_PORT_Number port_number);

void UCPD_PHY_StartBIST2(UCPD_PORT_Number port_number);

void UCPD_PHY_StopBIST2(UCPD_PORT_Number port_number);

void UCPD_PHY_AssertRd(UCPD_PORT_Number port_number);

void UCPD_PHY_DeassertRd(UCPD_PORT_Number port_number);

void UCPD_PHY_SetCC(UCPD_PORT_Number port_number);

void UCPD_PHY_EnableCC(UCPD_PORT_Number port_number, UCPD_Bool both);

void UCPD_PHY_Detach(UCPD_PORT_Number port_number);

void UCPD_PHY_Attach(UCPD_PORT_Number port_number);

UCPD_Bool UCPD_PHY_IsSinkTxOk(UCPD_PORT_Number port_number);

void UCPD_PHY_EnableRX(void);

void UCPD_PHY_DisableRX(void);

void UCPD_PHY_EnterErrorRecovery(UCPD_PORT_Number port_number);

/* Timer functions */
void UCPD_TIM_Handler(void);

void UCPD_HW_Timer_Start(uint32_t timer_channel, UCPD_Time period);

void UCPD_HW_Timer_Stop(uint32_t timer_channel);

void UCPD_HW_Timer_Init(void);

#endif // UCPD_HW_H
