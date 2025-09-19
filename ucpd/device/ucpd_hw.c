#include "ucpd/ucpd_hw.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_tim.h"
#include "stm32g0xx_ll_ucpd.h"
#include "trace_utils.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_pe_prl.h"
#include "ucpd/ucpd_port.h"
#include "ucpd/ucpd_prl_sm.h"
#include "ucpd/ucpd_tim.h"
#include "ucpd/ucpd_typec.h"
#include "ucpd/ucpd_utils.h"
#include "ucpd_hw_defines.h"
#include <stdint.h>

/* Handling of hardware specific phy event */
void UCPD_PHY_Handler(void) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(0);

  uint32_t status = LL_UCPD_ReadReg(port->port_handle.instance, SR);

  if ((port->port_handle.instance->IMR & status) == 0u)
    return;

  /* Type C events */
  if (CHECK_STATUS(status, UCPD_SR_TYPECEVT1) ||
      CHECK_STATUS(status, UCPD_SR_TYPECEVT2)) {
    LL_UCPD_ClearFlag_TypeCEventCC1(port->port_handle.instance);
    LL_UCPD_ClearFlag_TypeCEventCC2(port->port_handle.instance);
    UCPD_PD_PHY_Event_Handle(0, PHY_EVENT_CC_CHANGE);
    //        TRACE_DEBUG("CC changed\n\r");
    return;
  }

  /* Hard reset events */
  /* Sent hard reset*/
  if (CHECK_STATUS(status, UCPD_SR_HRSTSENT)) {
    LL_UCPD_ClearFlag_TxHRSTSENT(port->port_handle.instance);
    UCPD_PD_PHY_Event_Handle(0, PHY_EVENT_HRD_SENT);
    TRACE_HW_EVENT("Sent hard reset\n\r");
    return;
  }

  /* Received hard reset */
  if (CHECK_STATUS(status, UCPD_SR_RXHRSTDET)) {
    LL_UCPD_ClearFlag_RxHRST(port->port_handle.instance);

    UCPD_PHY_HRDRS_Received(0);

    TRACE_HW_EVENT("Received hard reset\n\r");
    return;
  }

  /* Hard reset rejected */
  if (CHECK_STATUS(status, UCPD_SR_HRSTDISC)) {
    LL_UCPD_ClearFlag_TxHRSTDISC(port->port_handle.instance);
    TRACE_HW_EVENT("Hard reset rejected\n\r");
    return;
  }

  /* Tx events */
  /* Tx aborted */
  if (CHECK_STATUS(status, UCPD_SR_TXMSGABT)) {
    LL_UCPD_ClearFlag_TxMSGABT(port->port_handle.instance);
    TRACE_HW_EVENT("Transmission aborted\n\r");
    return;
  }

  /* Tx discarded */
  if (CHECK_STATUS(status, UCPD_SR_TXMSGDISC)) {
    LL_UCPD_ClearFlag_TxMSGDISC(port->port_handle.instance);
    UCPD_PHY_TX_Discarded(0);

    TRACE_HW_EVENT("Transmission discarded\n\r");
    return;
  }

  /* Tx completed*/
  if (CHECK_STATUS(status, UCPD_SR_TXMSGSENT)) {
    LL_UCPD_ClearFlag_TxMSGSENT(port->port_handle.instance);

    UCPD_PHY_TX_Complete(0);
    return;
  }

  /* Rx events */
  /* Rx started */
  if (CHECK_STATUS(status, UCPD_SR_RXORDDET)) {
    LL_UCPD_ClearFlag_RxOrderSet(port->port_handle.instance);
    UCPD_PHY_RX_Started(0);
           TRACE_DEBUG("Reception started\n\r");

    return;
  }

  /* Rx completed */
  if (CHECK_STATUS(status, UCPD_SR_RXMSGEND)) {
    LL_UCPD_ClearFlag_RxMsgEnd(port->port_handle.instance);

    UCPD_Bool error = CHECK_STATUS(status, UCPD_SR_RXERR);
    UCPD_PHY_RX_Complete(0, error);

    if (error == UCPD_FALSE)
      UCPD_PD_PHY_Event_Handle(0, PHY_EVENT_MSG_RX_COMPLETE);

           TRACE_DEBUG("Reception complete\n\r");

    return;
  }
}

void UCPD_PHY_TX_Discarded(UCPD_PORT_Number port_number) {
  UCPD_PD_PHY_Event_Handle(port_number, PHY_EVENT_MSG_TX_DISCARDED);
}

void UCPD_EnableTransmissionEvents(UCPD_PORT_INSTANCE *port) {
  // todo: allow tx inside the handler with a flag
  port->pe_prl_cad.txAllowed = UCPD_TRUE;
}

void UCPD_DisableTransmissionEvents(UCPD_PORT_INSTANCE *port) {
  // todo: block tx inside the handler with a flag
  port->pe_prl_cad.txAllowed = UCPD_FALSE;
}

void UCPD_PHY_HRDRS_Received(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.hardResetActive = UCPD_TRUE;

  // disable tx
  UCPD_DisableTransmissionEvents(
      port); // maaaaaaaybe this will happen before the send message routine
             // checks the flag
  blockTxDataTransfer(&port->port_handle); // if not, disable DMA so it cannot
                                           // start transmitting

  // block tim and adc
  // block will not work because worker will enable, setting lower priority
  BLOCK_EVENT(UCPD_TIMER_EVENT);
  BLOCK_EVENT(UCPD_ADC_EVENT);

  EVENT_PRIORITY(UCPD_ADC_EVENT, 3);
  EVENT_PRIORITY(UCPD_TIMER_EVENT, 3);

  UNBLOCK_EVENT(UCPD_TIMER_EVENT);
  UNBLOCK_EVENT(UCPD_ADC_EVENT);

  // pend helper
  PEND_EVENT(UCPD_HARD_RESET_HELPER_EVENT);
}

void UCPD_HRD_Helper() { UCPD_PD_PHY_Event_Handle(0, PHY_EVENT_HRD_RECIEVED); }

void UCPD_PHY_Init(UCPD_PORT_Number port_number) {
  LL_UCPD_InitTypeDef ucpd_settings;

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  LL_UCPD_StructInit(&ucpd_settings);
  LL_UCPD_Init(port->port_handle.instance, &ucpd_settings);
  LL_UCPD_SetRxOrderSet(port->port_handle.instance,
                        LL_UCPD_ORDERSET_SOP | LL_UCPD_ORDERSET_HARDRST);

  LL_UCPD_SetccEnable(port->port_handle.instance, LL_UCPD_CCENABLE_CC1CC2);

  LL_UCPD_Enable(port->port_handle.instance);

  BLOCK_EVENT(UCPD_TIMER_EVENT);
  BLOCK_EVENT(UCPD_ADC_EVENT);
  BLOCK_EVENT(UCPD_HW_PHY_EVENT);

  EVENT_PRIORITY(UCPD_TIMER_EVENT, 1);
  EVENT_PRIORITY(UCPD_HW_PHY_EVENT, 0);
  EVENT_PRIORITY(UCPD_ADC_EVENT, 2);

  UNBLOCK_EVENT(UCPD_TIMER_EVENT);
  UNBLOCK_EVENT(UCPD_ADC_EVENT);
  UNBLOCK_EVENT(UCPD_HW_PHY_EVENT);

  UCPD_HARD_RESET_HELPER_INIT();
  UCPD_PE_PRL_WORKER_INIT();
}

void UCPD_PHY_HRDRS_Completed(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  // no need to do this because it is done in the send procedure
  // confTxDataTransfer(&port->port_handle, port->pe_prl.prl_sm.txBuffer.buffer,
  // 0); unblockTxDataTransfer(&port->port_handle);

  UCPD_INT_SET(port->port_handle.instance->IMR, UCPD_INT_NORMAL_MASK);

  BLOCK_EVENT(UCPD_TIMER_EVENT);
  BLOCK_EVENT(UCPD_ADC_EVENT);
  EVENT_PRIORITY(UCPD_ADC_EVENT, 2);
  EVENT_PRIORITY(UCPD_TIMER_EVENT, 1);
  UNBLOCK_EVENT(UCPD_TIMER_EVENT);
  UNBLOCK_EVENT(UCPD_ADC_EVENT);
}

void UCPD_PHY_EnableEvents(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  UCPD_INT_SET(port->port_handle.instance->IMR, UCPD_INT_NORMAL_MASK);

  // these may be redundant, but I am lazy to check
  LL_UCPD_EnableIT_TypeCEventCC2(port->port_handle.instance);
  LL_UCPD_EnableIT_TypeCEventCC1(port->port_handle.instance);

  UNBLOCK_EVENT(UCPD_TIMER_EVENT);
  UNBLOCK_EVENT(UCPD_HW_PHY_EVENT);
  UNBLOCK_EVENT(UCPD_ADC_EVENT);
  UNBLOCK_EVENT(UCPD_HARD_RESET_HELPER_EVENT);
}

void UCPD_PD_PHY_Event_Handle(UCPD_PORT_Number port_number,
                              UCPD_PHY_Event event) {

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  // UCPD_INT_SET(port->port_handle.instance->IMR, UCPD_INT_HARD_RESET_MASK);

  switch (event) {
  case PHY_EVENT_CC_CHANGE: {
    port->pe_prl_cad.phy_event = getCADEventFromCC(port_number);
    break;
  }

  case PHY_EVENT_HRD_SENT: {
    port->pe_prl_cad.phy_event = PE_PRL_EVENT_HRD_SENT;
    break;
  }

  case PHY_EVENT_HRD_RECIEVED: {
    port->pe_prl_cad.phy_event = PE_PRL_EVENT_HRD_RECEIVED;
    port->pe_prl_cad.hrdOriginPE = UCPD_FALSE;
    break;
  }

  case PHY_EVENT_MSG_TX_DISCARDED: {
    port->pe_prl_cad.phy_event = PE_PRL_EVENT_MSG_TX_DISCARDED;
    break;
  }

  case PHY_EVENT_MSG_TX_COMPLETED: {
    port->pe_prl_cad.phy_event = PE_PRL_EVENT_MSG_TX_COMPLETED;
    break;
  }

  case PHY_EVENT_MSG_RX_COMPLETE: {
    port->pe_prl_cad.phy_event = PE_PRL_EVENT_MSG_RX_COMPLETE;
    break;
  }

  default: {

    break;
  }
  }

  UCPD_INT_SET(port->port_handle.instance->IMR, UCPD_INT_PHY_WORKER_MASK);
  UCPD_PE_PRL_Event_Emit(PD_EVENT_SOURCE_PHY);
}

void UCPD_PHY_Worker_Finished(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  
  UCPD_INT_SET(port->port_handle.instance->IMR, UCPD_INT_NORMAL_MASK);
}

void UCPD_PE_PRL_Event_Emit(UCPD_PD_Event_Source source) {
  PEND_EVENT(UCPD_PE_PRL_WORKER_EVENT);
}

static UCPD_MSG msg_double;

void UCPD_PHY_RX_Complete(UCPD_PORT_Number port_number, UCPD_Bool error) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  UCPD_EnableTransmissionEvents(port);

  blockRxDataTransfer(&port->port_handle);

  COPY_RAW(msg_double.buffer, port->pe_prl_cad.buffers[UCPD_RX_BUFFER_INDEX].buffer, UCPD_MAX_MSG_LEN);

  CLEAR_STRUCT_VAL(msg_double);

  // chunk of EPR source capabilities is the biggest message we expect to
  // receive, so the size of the standard buffer is enough
  confRxDataTransfer(&port->port_handle,
                     msg_double.buffer,
                     UCPD_MAX_MSG_LEN);

  unblockRxDataTransfer(&port->port_handle);
}

void UCPD_PHY_RX_Started(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_DisableTransmissionEvents(port);
}

void UCPD_PHY_TX_Complete(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  blockTxDataTransfer(&port->port_handle);
  UCPD_EnableTransmissionEvents(port);

  UCPD_PD_PHY_Event_Handle(0, PHY_EVENT_MSG_TX_COMPLETED);
}

UCPD_PE_PRL_CAD_Event getCADEventFromCC(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_PE_PRL_CAD_Event event = CAD_EVENT_CONN_REMOVED;
  UCPD_INSTANCE_TYPE *instance = port->port_handle.instance;

  port->port_handle.cc = UCPD_CCNONE;

  uint32_t CC1_val = instance->SR & UCPD_SR_TYPEC_VSTATE_CC1;
  uint32_t CC2_val = instance->SR & UCPD_SR_TYPEC_VSTATE_CC2;

  if (UCPD_CC_DETECT_VOPEN(CC1_val, CC1) && !UCPD_CC_DETECT_VOPEN(CC2_val, CC2))
    port->port_handle.cc = UCPD_CC2;
  else if (!UCPD_CC_DETECT_VOPEN(CC1_val, CC1) &&
           UCPD_CC_DETECT_VOPEN(CC2_val, CC2))
    port->port_handle.cc = UCPD_CC1;
  else
    return event;

  event = CAD_EVENT_CONN_DETECTED;

  return event;
}

UCPD_Bool isRxDataTransferBlocked(UCPD_HW_PORT_Handle *port_handle) {
  return ((port_handle->rxData->CCR & DMA_CCR_EN) == DMA_CCR_EN) ? UCPD_FALSE
                                                                 : UCPD_TRUE;
}

UCPD_Bool isTxDataTransferBlocked(UCPD_HW_PORT_Handle *port_handle) {
  return ((port_handle->txData->CCR & DMA_CCR_EN) == DMA_CCR_EN) ? UCPD_FALSE
                                                                 : UCPD_TRUE;
}

void blockTxDataTransfer(UCPD_HW_PORT_Handle *port_handle) {
  CLEAR_BIT(port_handle->txData->CCR, DMA_CCR_EN);
  while (isTxDataTransferBlocked(port_handle) == UCPD_FALSE)
    ;
}

void blockRxDataTransfer(UCPD_HW_PORT_Handle *port_handle) {
  CLEAR_BIT(port_handle->rxData->CCR, DMA_CCR_EN);
  while (isRxDataTransferBlocked(port_handle) == UCPD_FALSE)
    ;
}

void unblockTxDataTransfer(UCPD_HW_PORT_Handle *port_handle) {
  SET_BIT(port_handle->txData->CCR, DMA_CCR_EN);
}

void unblockRxDataTransfer(UCPD_HW_PORT_Handle *port_handle) {
  SET_BIT(port_handle->rxData->CCR, DMA_CCR_EN);
}

void confTxDataTransfer(UCPD_HW_PORT_Handle *port_handle, uint8_t *buffer,
                        uint32_t size) {
  WRITE_REG(port_handle->txData->CPAR,
            (uint32_t)&(port_handle->instance->TXDR));
  WRITE_REG(port_handle->txData->CMAR, (uint32_t)buffer);
  WRITE_REG(port_handle->txData->CNDTR, size);
}

void confRxDataTransfer(UCPD_HW_PORT_Handle *port_handle, uint8_t *buffer,
                        uint32_t size) {
  WRITE_REG(port_handle->rxData->CPAR,
            (uint32_t)&(port_handle->instance->RXDR));
  WRITE_REG(port_handle->rxData->CMAR, (uint32_t)buffer);
  WRITE_REG(port_handle->rxData->CNDTR, size);
}

void UCPD_PHY_SendMessage(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  if (port->pe_prl_cad.txAllowed == UCPD_FALSE) {
    return;
  }

  UCPD_DisableTransmissionEvents(port);

  LL_UCPD_WriteTxOrderSet(port->port_handle.instance, LL_UCPD_ORDERED_SET_SOP);
  LL_UCPD_SetTxMode(port->port_handle.instance, LL_UCPD_TXMODE_NORMAL);

  blockTxDataTransfer(&port->port_handle);

  confTxDataTransfer(&port->port_handle,
                     port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].buffer,
                     port->pe_prl_cad.txSize);

  unblockTxDataTransfer(&port->port_handle);

  LL_UCPD_WriteTxPaySize(port->port_handle.instance, port->pe_prl_cad.txSize);
  LL_UCPD_SendMessage(port->port_handle.instance);
}

void UCPD_PHY_SendHardReset(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  LL_UCPD_SendHardReset(port->port_handle.instance);
}

void UCPD_PHY_StartBIST2(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  LL_UCPD_SetTxMode(port->port_handle.instance, LL_UCPD_TXMODE_BIST_CARRIER2);
  LL_UCPD_SendMessage(port->port_handle.instance);
}

void UCPD_PHY_StopBIST2(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  while (isRxDataTransferBlocked(&port->port_handle) == UCPD_FALSE)
    ;
}

void UCPD_PHY_AssertRd(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  LL_UCPD_TypeCDetectionCC2Disable(port->port_handle.instance);
  LL_UCPD_TypeCDetectionCC1Disable(port->port_handle.instance);

  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_UCPD_SetccEnable(port->port_handle.instance, LL_UCPD_CCENABLE_NONE);
  LL_UCPD_SetSNKRole(port->port_handle.instance);
  if (UCPD_CCNONE == port->port_handle.cc) {
    LL_UCPD_SetccEnable(port->port_handle.instance, LL_UCPD_CCENABLE_CC1CC2);
  } else {
    LL_UCPD_SetccEnable(port->port_handle.instance,
                        (port->port_handle.cc == UCPD_CC1)
                            ? LL_UCPD_CCENABLE_CC1
                            : LL_UCPD_CCENABLE_CC2);
  }

  SET_BIT(SYSCFG->CFGR1, (port->port_handle.instance == UCPD1)
                             ? SYSCFG_CFGR1_UCPD1_STROBE
                             : SYSCFG_CFGR1_UCPD2_STROBE);
  HAL_Delay(1);

  LL_UCPD_TypeCDetectionCC2Enable(port->port_handle.instance);
  LL_UCPD_TypeCDetectionCC1Enable(port->port_handle.instance);
}

void UCPD_PHY_SetCC(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  LL_UCPD_SetCCPin(port->port_handle.instance, port->port_handle.cc == UCPD_CC1
                                                   ? LL_UCPD_CCPIN_CC1
                                                   : LL_UCPD_CCPIN_CC2);
}

void UCPD_PHY_EnableCC(UCPD_PORT_Number port_number, UCPD_Bool both) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  if (both == UCPD_TRUE)
    LL_UCPD_SetccEnable(port->port_handle.instance, LL_UCPD_CCENABLE_CC1CC2);
  else
    LL_UCPD_SetccEnable(port->port_handle.instance,
                        port->port_handle.cc == UCPD_CC1
                            ? LL_UCPD_CCENABLE_CC1
                            : LL_UCPD_CCENABLE_CC2);
}

void UCPD_PHY_Detach(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  LL_UCPD_RxDMADisable(port->port_handle.instance);
  LL_UCPD_TxDMADisable(port->port_handle.instance);
  LL_UCPD_RxDisable(port->port_handle.instance);

  UCPD_CC_DETECTION_ONLY(port->port_handle.instance->IMR);

  UCPD_PHY_EnableCC(port_number, UCPD_TRUE);

  port->port_handle.cc = UCPD_CCNONE;
}

void UCPD_PHY_Attach(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  port->port_handle.rxData = UCPD_DATA_TRANSFER_RX_CHANNEL;
  port->port_handle.txData = UCPD_DATA_TRANSFER_TX_CHANNEL;

  confRxDataTransfer(&port->port_handle,
                     msg_double.buffer,
                     UCPD_TOTAL_MSG_LEN);
  unblockRxDataTransfer(&port->port_handle);

  confTxDataTransfer(&port->port_handle,
                     port->pe_prl_cad.buffers[UCPD_TX_BUFFER_INDEX].buffer, 0);

  UCPD_INT_SET(port->port_handle.instance->IMR, UCPD_INT_NORMAL_MASK);

  UCPD_PHY_SetCC(port_number);

  UCPD_PHY_EnableCC(port_number, UCPD_FALSE);

  LL_UCPD_SetRxMode(port->port_handle.instance, LL_UCPD_RXMODE_NORMAL);
  LL_UCPD_RxDMAEnable(port->port_handle.instance);
  LL_UCPD_TxDMAEnable(port->port_handle.instance);
  LL_UCPD_RxEnable(port->port_handle.instance);
}

UCPD_Bool UCPD_PHY_IsSinkTxOk(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  return UCPD_CC_CHECK_TX_OK(port->port_handle.instance->SR, CC1) ||
         UCPD_CC_CHECK_TX_OK(port->port_handle.instance->SR, CC2);
}

void UCPD_PHY_EnterErrorRecovery(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  LL_UCPD_SetSRCRole(port->port_handle.instance);
  LL_UCPD_SetRpResistor(port->port_handle.instance, LL_UCPD_RESISTOR_NONE);
  LL_UCPD_SetccEnable(port->port_handle.instance, LL_UCPD_CCENABLE_NONE);
  SET_BIT(SYSCFG->CFGR1, (port->port_handle.instance == UCPD1)
                             ? SYSCFG_CFGR1_UCPD1_STROBE
                             : SYSCFG_CFGR1_UCPD2_STROBE);
  LL_UCPD_RxDisable(port->port_handle.instance);
}

void UCPD_HW_Timer_Start(uint32_t timer_channel, UCPD_Time period) {
  uint16_t now = LL_TIM_GetCounter(UCPD_TIMER_INSTANCE);
  uint16_t when = now + period + 1u;
  switch (timer_channel) {
  case LL_TIM_CHANNEL_CH1:
    LL_TIM_ClearFlag_CC1(UCPD_TIMER_INSTANCE);
    LL_TIM_OC_SetCompareCH1(UCPD_TIMER_INSTANCE, when);
    LL_TIM_EnableIT_CC1(UCPD_TIMER_INSTANCE);
    break;
  case LL_TIM_CHANNEL_CH2:
    LL_TIM_ClearFlag_CC2(UCPD_TIMER_INSTANCE);
    LL_TIM_OC_SetCompareCH2(UCPD_TIMER_INSTANCE, when);
    LL_TIM_EnableIT_CC2(UCPD_TIMER_INSTANCE);
    break;
  case LL_TIM_CHANNEL_CH3:
    LL_TIM_ClearFlag_CC3(UCPD_TIMER_INSTANCE);
    LL_TIM_OC_SetCompareCH3(UCPD_TIMER_INSTANCE, when);
    LL_TIM_EnableIT_CC3(UCPD_TIMER_INSTANCE);
    break;
  case LL_TIM_CHANNEL_CH4:
    LL_TIM_ClearFlag_CC4(UCPD_TIMER_INSTANCE);
    LL_TIM_OC_SetCompareCH4(UCPD_TIMER_INSTANCE, when);
    LL_TIM_EnableIT_CC4(UCPD_TIMER_INSTANCE);
    break;
  }
}

void UCPD_HW_Timer_Stop(uint32_t timer_channel) {
  switch (timer_channel) {
  case LL_TIM_CHANNEL_CH1:
    LL_TIM_DisableIT_CC1(UCPD_TIMER_INSTANCE);
    LL_TIM_ClearFlag_CC1(UCPD_TIMER_INSTANCE);
    break;
  case LL_TIM_CHANNEL_CH2:
    LL_TIM_DisableIT_CC2(UCPD_TIMER_INSTANCE);
    LL_TIM_ClearFlag_CC2(UCPD_TIMER_INSTANCE);
    break;
  case LL_TIM_CHANNEL_CH3:
    LL_TIM_DisableIT_CC3(UCPD_TIMER_INSTANCE);
    LL_TIM_ClearFlag_CC3(UCPD_TIMER_INSTANCE);
    break;
  case LL_TIM_CHANNEL_CH4:
    LL_TIM_DisableIT_CC4(UCPD_TIMER_INSTANCE);
    LL_TIM_ClearFlag_CC4(UCPD_TIMER_INSTANCE);
    break;
  }
}

void UCPD_TIM_Handler(void) {
  uint32_t sr = UCPD_TIMER_INSTANCE->SR;
  uint32_t dier = UCPD_TIMER_INSTANCE->DIER;

  // IN PRACTICE one timer overwriting the event of another timer in the
  // pe_prl_cad should not be a problem, but keeping in mind

  if ((sr & TIM_SR_CC1IF) && (dier & TIM_DIER_CC1IE)) {
    LL_TIM_ClearFlag_CC1(UCPD_TIMER_INSTANCE);
    UCPD_TIM_Expired(0, UCPD_TIMER_CHANNEL_1);
  }

  if ((sr & TIM_SR_CC2IF) && (dier & TIM_DIER_CC2IE)) {
    LL_TIM_ClearFlag_CC2(UCPD_TIMER_INSTANCE);
    UCPD_TIM_Expired(0, UCPD_TIMER_CHANNEL_2);
  }

  if ((sr & TIM_SR_CC3IF) && (dier & TIM_DIER_CC3IE)) {
    LL_TIM_ClearFlag_CC3(UCPD_TIMER_INSTANCE);
    UCPD_TIM_Expired(0, UCPD_TIMER_CHANNEL_3);
  }

  if ((sr & TIM_SR_CC4IF) && (dier & TIM_DIER_CC4IE)) {
    LL_TIM_ClearFlag_CC4(UCPD_TIMER_INSTANCE);
    UCPD_TIM_Expired(0, UCPD_TIMER_CHANNEL_4);
  }
}

/* Timer hardware initialization for UCPD timer system */
void UCPD_HW_Timer_Init(void) {
  // Use SystemCoreClock (standard STM32 variable) for timer clock calculation
  // For STM32G0, TIM1 typically runs at system clock frequency
  uint32_t timer_clock = SystemCoreClock;

  // Calculate prescaler for 1ms tick (1000 Hz timer frequency)
  uint32_t prescaler = (timer_clock / 1000) - 1;
  uint32_t autoreload = 0xFFFF;

  // Stop timer first to ensure clean state
  LL_TIM_DisableCounter(UCPD_TIMER_INSTANCE);

  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);
  LL_TIM_SetPrescaler(UCPD_TIMER_INSTANCE, prescaler);
  LL_TIM_SetAutoReload(UCPD_TIMER_INSTANCE, autoreload);
  LL_TIM_SetCounter(UCPD_TIMER_INSTANCE, 0xFFFF);
  LL_TIM_SetCounterMode(TIM1, LL_TIM_COUNTERMODE_UP);

  LL_TIM_OC_SetMode(UCPD_TIMER_INSTANCE, LL_TIM_CHANNEL_CH1,
                    LL_TIM_OCMODE_FROZEN);
  LL_TIM_CC_DisableChannel(UCPD_TIMER_INSTANCE,
                           LL_TIM_CHANNEL_CH1); // ensure off until used
  LL_TIM_OC_SetMode(UCPD_TIMER_INSTANCE, LL_TIM_CHANNEL_CH2,
                    LL_TIM_OCMODE_FROZEN);
  LL_TIM_CC_DisableChannel(UCPD_TIMER_INSTANCE, LL_TIM_CHANNEL_CH2);
  LL_TIM_OC_SetMode(UCPD_TIMER_INSTANCE, LL_TIM_CHANNEL_CH3,
                    LL_TIM_OCMODE_FROZEN);
  LL_TIM_CC_DisableChannel(UCPD_TIMER_INSTANCE, LL_TIM_CHANNEL_CH3);
  LL_TIM_OC_SetMode(UCPD_TIMER_INSTANCE, LL_TIM_CHANNEL_CH4,
                    LL_TIM_OCMODE_FROZEN);
  LL_TIM_CC_DisableChannel(UCPD_TIMER_INSTANCE, LL_TIM_CHANNEL_CH4);

  LL_TIM_EnableCounter(UCPD_TIMER_INSTANCE);

  EVENT_PRIORITY(UCPD_TIMER_EVENT, 1);
  UNBLOCK_EVENT(UCPD_TIMER_EVENT);

  // /* Disable ALL interrupts to ensure clean state */
  // LL_TIM_DisableIT_UPDATE(UCPD_TIMER_INSTANCE);
  // LL_TIM_DisableIT_BRK(UCPD_TIMER_INSTANCE);
  // LL_TIM_DisableIT_COM(UCPD_TIMER_INSTANCE);
  // LL_TIM_DisableIT_TRIG(UCPD_TIMER_INSTANCE);

  // /* Clear ALL interrupt flags */
  // LL_TIM_ClearFlag_UPDATE(UCPD_TIMER_INSTANCE);
  // LL_TIM_ClearFlag_BRK(UCPD_TIMER_INSTANCE);
  // LL_TIM_ClearFlag_COM(UCPD_TIMER_INSTANCE);
  // LL_TIM_ClearFlag_TRIG(UCPD_TIMER_INSTANCE);
}
