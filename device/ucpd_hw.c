#include "ucpd/ucpd_hw.h"
#include "ucpd_hw_defines.h"
#include <stdint.h>
#include <string.h>

/* HW Port handles*/
static UCPD_HW_PORT_Handle port_handles[UCPD_PORT_COUNT];

inline UCPD_HW_PORT_Handle *getPortHandles(void) { return port_handles; }

/* DMA utils */
static inline bool isRxDataTransferBlocked(UCPD_HW_PORT_Handle *port_handle) {
  return ((port_handle->rxData->CCR & DMA_CCR_EN) != DMA_CCR_EN);
}

static inline bool isTxDataTransferBlocked(UCPD_HW_PORT_Handle *port_handle) {
  return ((port_handle->txData->CCR & DMA_CCR_EN) != DMA_CCR_EN);
}

static inline void blockTxDataTransfer(UCPD_HW_PORT_Handle *port_handle) {
  CLEAR_BIT(port_handle->txData->CCR, DMA_CCR_EN);
  while (isTxDataTransferBlocked(port_handle) == false)
    ;
}

static inline void blockRxDataTransfer(UCPD_HW_PORT_Handle *port_handle) {
  CLEAR_BIT(port_handle->rxData->CCR, DMA_CCR_EN);
  while (isRxDataTransferBlocked(port_handle) == false)
    ;
}

static inline void unblockTxDataTransfer(UCPD_HW_PORT_Handle *port_handle) {
  SET_BIT(port_handle->txData->CCR, DMA_CCR_EN);
}

static inline void unblockRxDataTransfer(UCPD_HW_PORT_Handle *port_handle) {
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

static inline void UCPD_PHY_SetCC(UCPD_PORT_Number port_number) {
  LL_UCPD_SetCCPin(port_handles[port_number].instance,
                   port_handles[port_number].cc == UCPD_CC1
                       ? LL_UCPD_CCPIN_CC1
                       : LL_UCPD_CCPIN_CC2);
}

void UCPD_PHY_EnableCC(UCPD_PORT_Number port_number, bool both) {
  if (both)
    LL_UCPD_SetccEnable(port_handles[port_number].instance,
                        LL_UCPD_CCENABLE_CC1CC2);
  else
    LL_UCPD_SetccEnable(port_handles[port_number].instance,
                        port_handles[port_number].cc == UCPD_CC1
                            ? LL_UCPD_CCENABLE_CC1
                            : LL_UCPD_CCENABLE_CC2);
}

/* Tx utils */
static inline void UCPD_EnableTransmissionEvents(UCPD_PORT_INSTANCE *port) {
  // todo: allow tx inside the handler with a flag
  port->pe_prl_cad.txAllowed = true;
}

static inline void UCPD_DisableTransmissionEvents(UCPD_PORT_INSTANCE *port) {
  // todo: block tx inside the handler with a flag
  port->pe_prl_cad.txAllowed = false;
}

/* HW interface implementations */
void UCPD_PHY_Init(UCPD_PORT_Number port_number) {

  LL_SYSCFG_DisableDBATT(LL_SYSCFG_UCPD1_STROBE | LL_SYSCFG_UCPD2_STROBE);

  LL_UCPD_InitTypeDef ucpd_settings;

  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_CRC);
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOC);

  /* Configure GPIO pin : VCC (PB6) */
  LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_6);

  LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(GPIOB, LL_GPIO_PIN_6, LL_GPIO_SPEED_FREQ_LOW);
  LL_GPIO_SetPinOutputType(GPIOB, LL_GPIO_PIN_6, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOB, LL_GPIO_PIN_6, LL_GPIO_PULL_NO);
  /* Configure GPIO pin : DB (PC10) */
  LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_10);

  LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_10, LL_GPIO_MODE_OUTPUT);
  LL_GPIO_SetPinSpeed(GPIOC, LL_GPIO_PIN_10, LL_GPIO_SPEED_FREQ_LOW);
  LL_GPIO_SetPinOutputType(GPIOC, LL_GPIO_PIN_10, LL_GPIO_OUTPUT_PUSHPULL);
  LL_GPIO_SetPinPull(GPIOC, LL_GPIO_PIN_10, LL_GPIO_PULL_NO);

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_UCPD1);

  /**UCPD1 GPIO Configuration
  PB15   ------> UCPD1_CC2
  PA8   ------> UCPD1_CC1
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_15;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LL_GPIO_PIN_8;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* UCPD1 DMA Init */

  /* UCPD1_RX Init */
  LL_DMA_SetPeriphRequest(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_2,
                          LL_DMAMUX_REQ_UCPD1_RX);

  LL_DMA_SetDataTransferDirection(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_2,
                                  LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

  LL_DMA_SetChannelPriorityLevel(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_2,
                                 LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_2, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_2,
                          LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_2,
                          LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_2,
                       LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_2,
                       LL_DMA_MDATAALIGN_BYTE);

  /* UCPD1_TX Init */
  LL_DMA_SetPeriphRequest(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_3,
                          LL_DMAMUX_REQ_UCPD1_TX);

  LL_DMA_SetDataTransferDirection(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_3,
                                  LL_DMA_DIRECTION_MEMORY_TO_PERIPH);

  LL_DMA_SetChannelPriorityLevel(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_3,
                                 LL_DMA_PRIORITY_LOW);

  LL_DMA_SetMode(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_3, LL_DMA_MODE_NORMAL);

  LL_DMA_SetPeriphIncMode(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_3,
                          LL_DMA_PERIPH_NOINCREMENT);

  LL_DMA_SetMemoryIncMode(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_3,
                          LL_DMA_MEMORY_INCREMENT);

  LL_DMA_SetPeriphSize(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_3,
                       LL_DMA_PDATAALIGN_BYTE);

  LL_DMA_SetMemorySize(UCPD_DATA_TRANSFER, LL_DMA_CHANNEL_3,
                       LL_DMA_MDATAALIGN_BYTE);

  // init hw handle
  port_handles[port_number] = (UCPD_HW_PORT_Handle){
      .instance = UCPD_INSTANCE,
      .cc = UCPD_CCNONE,
      .rxData = NULL,
      .txData = NULL,
      .pwr_sm = {0},
      .buffers[UCPD_RX_BUFFER_INDEX] = UCPD_MSG_INIT,
      .buffers[UCPD_TX_BUFFER_INDEX] = UCPD_MSG_INIT,
      .buffers[UCPD_DRX_BUFFER_INDEX] = UCPD_MSG_INIT,
      .ext_buffers[UCPD_EXT_RX_BUFFER_INDEX] = UCPD_EXT_MSG_INIT,
      .ext_buffers[UCPD_EXT_TX_BUFFER_INDEX] = UCPD_EXT_MSG_INIT,
  };

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  port->pe_prl_cad.rx_buffer =
      &port_handles[port_number]
           .buffers[port_handles[port_number].rx_buffer_index];
  port->pe_prl_cad.tx_buffer =
      &port_handles[port_number].buffers[UCPD_TX_BUFFER_INDEX];
  port->pe_prl_cad.ext_rx_buffer =
      &port_handles[port_number].ext_buffers[UCPD_EXT_RX_BUFFER_INDEX];
  port->pe_prl_cad.ext_tx_buffer =
      &port_handles[port_number].ext_buffers[UCPD_EXT_TX_BUFFER_INDEX];

  LL_UCPD_StructInit(&ucpd_settings);
  LL_UCPD_Init(port_handles[port_number].instance, &ucpd_settings);
  LL_UCPD_SetRxOrderSet(port_handles[port_number].instance,
                        LL_UCPD_ORDERSET_SOP | LL_UCPD_ORDERSET_HARDRST);

  LL_UCPD_SetccEnable(port_handles[port_number].instance,
                      LL_UCPD_CCENABLE_CC1CC2);

  LL_UCPD_Enable(port_handles[port_number].instance);

  BLOCK_EVENT(UCPD_TIMER_EVENT);
  BLOCK_EVENT(UCPD_ADC_EVENT);
  BLOCK_EVENT(UCPD_HW_PHY_EVENT);
  BLOCK_EVENT(UCPD_HRD_HELPER_EVENT);
  BLOCK_EVENT(UCPD_PE_PRL_WORKER_EVENT);

  EVENT_PRIORITY(UCPD_TIMER_EVENT, UCPD_EVENT_TIM_PRIORITY);
  EVENT_PRIORITY(UCPD_HW_PHY_EVENT, UCPD_EVENT_PHY_PRIORITY);
  EVENT_PRIORITY(UCPD_ADC_EVENT, UCPD_EVENT_ADC_PRIORITY);

  UCPD_PE_PRL_WORKER_INIT();
  UCPD_HRD_HELPER_INIT();
}

void UCPD_PHY_Deinit(UCPD_PORT_Number port_number) {

  UCPD_PHY_DisableEvents(port_number);

  LL_UCPD_Disable(port_handles[port_number].instance);
  LL_UCPD_SetccEnable(port_handles[port_number].instance,
                      LL_UCPD_CCENABLE_NONE);
}

void UCPD_PHY_EnableEvents(UCPD_PORT_Number port_number) {

  // these may be redundant
  LL_UCPD_EnableIT_TypeCEventCC2(port_handles[port_number].instance);
  LL_UCPD_EnableIT_TypeCEventCC1(port_handles[port_number].instance);

  UCPD_INT_SET(port_handles[port_number].instance->IMR, UCPD_INT_NORMAL_MASK);
  UNBLOCK_EVENT(UCPD_TIMER_EVENT);
  UNBLOCK_EVENT(UCPD_HW_PHY_EVENT);
  UNBLOCK_EVENT(UCPD_ADC_EVENT);
  UNBLOCK_EVENT(UCPD_HRD_HELPER_EVENT);
  UNBLOCK_EVENT(UCPD_PE_PRL_WORKER_EVENT);
}

void UCPD_PHY_DisableEvents(UCPD_PORT_Number port_number) {
  // these may be redundant
  LL_UCPD_DisableIT_TypeCEventCC2(port_handles[port_number].instance);
  LL_UCPD_DisableIT_TypeCEventCC1(port_handles[port_number].instance);

  BLOCK_EVENT(UCPD_HW_PHY_EVENT);
  BLOCK_EVENT(UCPD_TIMER_EVENT);
  BLOCK_EVENT(UCPD_ADC_EVENT);
  BLOCK_EVENT(UCPD_HRD_HELPER_EVENT);
  BLOCK_EVENT(UCPD_PE_PRL_WORKER_EVENT);
}

void UCPD_PHY_SendMessage(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  if (port->pe_prl_cad.txAllowed == false) {
    return;
  }

  UCPD_DisableTransmissionEvents(port);

  LL_UCPD_WriteTxOrderSet(port_handles[port_number].instance,
                          LL_UCPD_ORDERED_SET_SOP);
  LL_UCPD_SetTxMode(port_handles[port_number].instance, LL_UCPD_TXMODE_NORMAL);

  blockTxDataTransfer(&port_handles[port_number]);

  confTxDataTransfer(
      &port_handles[port_number],
      port_handles[port_number].buffers[UCPD_TX_BUFFER_INDEX].buffer,
      port->pe_prl_cad.txSize);

  unblockTxDataTransfer(&port_handles[port_number]);

  LL_UCPD_WriteTxPaySize(port_handles[port_number].instance,
                         port->pe_prl_cad.txSize);
  LL_UCPD_SendMessage(port_handles[port_number].instance);
}

inline void UCPD_PHY_SendHardReset(UCPD_PORT_Number port_number) {
  LL_UCPD_SendHardReset(port_handles[port_number].instance);
}

void UCPD_PHY_AssertRd(UCPD_PORT_Number port_number) {

  LL_UCPD_TypeCDetectionCC2Disable(port_handles[port_number].instance);
  LL_UCPD_TypeCDetectionCC1Disable(port_handles[port_number].instance);

  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_UCPD_SetccEnable(port_handles[port_number].instance,
                      LL_UCPD_CCENABLE_NONE);
  LL_UCPD_SetSNKRole(port_handles[port_number].instance);
  if (UCPD_CCNONE == port_handles[port_number].cc) {
    LL_UCPD_SetccEnable(port_handles[port_number].instance,
                        LL_UCPD_CCENABLE_CC1CC2);
  } else {
    LL_UCPD_SetccEnable(port_handles[port_number].instance,
                        (port_handles[port_number].cc == UCPD_CC1)
                            ? LL_UCPD_CCENABLE_CC1
                            : LL_UCPD_CCENABLE_CC2);
  }

  SET_BIT(SYSCFG->CFGR1, (port_handles[port_number].instance == UCPD1)
                             ? SYSCFG_CFGR1_UCPD1_STROBE
                             : SYSCFG_CFGR1_UCPD2_STROBE);

  LL_mDelay(1);

  LL_UCPD_TypeCDetectionCC2Enable(port_handles[port_number].instance);
  LL_UCPD_TypeCDetectionCC1Enable(port_handles[port_number].instance);
}

void UCPD_PHY_DeassertRd(UCPD_PORT_Number port_number) {
  LL_UCPD_TypeCDetectionCC2Disable(port_handles[port_number].instance);
  LL_UCPD_TypeCDetectionCC1Disable(port_handles[port_number].instance);

  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_UCPD_SetccEnable(port_handles[port_number].instance,
                      LL_UCPD_CCENABLE_NONE);

  SET_BIT(SYSCFG->CFGR1, (port_handles[port_number].instance == UCPD1)
                             ? SYSCFG_CFGR1_UCPD1_STROBE
                             : SYSCFG_CFGR1_UCPD2_STROBE);

  LL_mDelay(1);

  LL_UCPD_TypeCDetectionCC2Enable(port_handles[port_number].instance);
  LL_UCPD_TypeCDetectionCC1Enable(port_handles[port_number].instance);
}

void UCPD_PHY_Detach(UCPD_PORT_Number port_number) {
  LL_UCPD_RxDMADisable(port_handles[port_number].instance);
  LL_UCPD_TxDMADisable(port_handles[port_number].instance);
  LL_UCPD_RxDisable(port_handles[port_number].instance);

  CLEAR_STATUS(port_handles[port_number].instance->SR,
               port_handles[port_number].instance->SR);
  CLEAR_EVENT(UCPD_HW_PHY_EVENT);
  UCPD_CC_DETECTION_ONLY(port_handles[port_number].instance->IMR);

  UCPD_PHY_EnableCC(port_number, true);

  port_handles[port_number].cc = UCPD_CCNONE;
}

void UCPD_PHY_Attach(UCPD_PORT_Number port_number) {
  port_handles[port_number].rxData = UCPD_DATA_TRANSFER_RX_CHANNEL;
  port_handles[port_number].txData = UCPD_DATA_TRANSFER_TX_CHANNEL;

  confRxDataTransfer(
      &port_handles[port_number],
      port_handles[port_number]
          .buffers[1u - port_handles[port_number].rx_buffer_index]
          .buffer,
      UCPD_TOTAL_MSG_LEN);
  unblockRxDataTransfer(&port_handles[port_number]);

  confTxDataTransfer(
      &port_handles[port_number],
      port_handles[port_number].buffers[UCPD_TX_BUFFER_INDEX].buffer, 0);

  UCPD_INT_SET(port_handles[port_number].instance->IMR, UCPD_INT_NORMAL_MASK);

  UCPD_PHY_SetCC(port_number);

  UCPD_PHY_EnableCC(port_number, false);

  LL_UCPD_SetRxMode(port_handles[port_number].instance, LL_UCPD_RXMODE_NORMAL);
  LL_UCPD_RxDMAEnable(port_handles[port_number].instance);
  LL_UCPD_TxDMAEnable(port_handles[port_number].instance);
  LL_UCPD_RxEnable(port_handles[port_number].instance);
}

void UCPD_PHY_HRDRS_Completed(UCPD_PORT_Number port_number) {
  LL_UCPD_TxDMAEnable(port_handles[port_number].instance);
  UCPD_EnableTransmissionEvents(UCPD_CTX_GetPortInstance(port_number));
  UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad.hardResetActive =
      false; // reset the flag; enabling events disabled earlier

  UNBLOCK_EVENT(UCPD_HW_PHY_EVENT);
  UNBLOCK_EVENT(UCPD_TIMER_EVENT);
}

bool UCPD_PHY_IsSinkTxOk(UCPD_PORT_Number port_number) {
  return UCPD_CC_CHECK_TX_OK(port_handles[port_number].instance->SR, CC1) ||
         UCPD_CC_CHECK_TX_OK(port_handles[port_number].instance->SR, CC2);
}

/* Physical events */
static inline void UCPD_PHY_TX_Discarded(UCPD_PORT_Number port_number) {
  UCPD_PD_PHY_Event_Handle(port_number, PHY_EVENT_MSG_TX_DISCARDED);
}

void UCPD_PHY_HRDRS_Received(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  UCPD_INT_SET(port_handles[port_number].instance->IMR,
               UCPD_INT_PHY_WORKER_MASK);

  port->pe_prl_cad.hardResetActive = true;

  // disable tx
  UCPD_DisableTransmissionEvents(
      port); // just in case this happens before the worker reaches the send
             // message routine

  LL_UCPD_TxDMADisable(port_handles[port_number].instance);
  blockTxDataTransfer(&port_handles[port_number]);

  // pend the hard reset helper
  PEND_EVENT(UCPD_HRD_HELPER_EVENT);
}

void UCPD_PHY_RX_Complete(UCPD_PORT_Number port_number, bool error) {
  (void)error;

  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);

  UCPD_EnableTransmissionEvents(port);

  blockRxDataTransfer(&port_handles[port_number]);

  // chunk of EPR source capabilities is the biggest message we expect to
  // receive, so the size of the standard buffer is enough
  confRxDataTransfer(&port_handles[port_number],
                     port_handles[port_number]
                         .buffers[port_handles[port_number].rx_buffer_index]
                         .buffer,
                     UCPD_MAX_MSG_LEN);

  port_handles[port_number].rx_buffer_index =
      1u - port_handles[port_number].rx_buffer_index;
  unblockRxDataTransfer(&port_handles[port_number]);
}

static inline void UCPD_PHY_RX_Started(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  UCPD_DisableTransmissionEvents(port);
}

void UCPD_PHY_TX_Complete(UCPD_PORT_Number port_number) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(port_number);
  blockTxDataTransfer(&port_handles[port_number]);
  UCPD_EnableTransmissionEvents(port);

  UCPD_PD_PHY_Event_Handle(port_number, PHY_EVENT_MSG_TX_COMPLETED);
}

void UCPD_PHY_EnterErrorRecovery(UCPD_PORT_Number port_number) {
  LL_UCPD_SetSRCRole(port_handles[port_number].instance);
  LL_UCPD_SetRpResistor(port_handles[port_number].instance,
                        LL_UCPD_RESISTOR_NONE);
  LL_UCPD_SetccEnable(port_handles[port_number].instance,
                      LL_UCPD_CCENABLE_NONE);
  SET_BIT(SYSCFG->CFGR1, (port_handles[port_number].instance == UCPD1)
                             ? SYSCFG_CFGR1_UCPD1_STROBE
                             : SYSCFG_CFGR1_UCPD2_STROBE);
  LL_UCPD_RxDisable(port_handles[port_number].instance);

  port_handles[port_number].cc = UCPD_CCNONE;
}

/* Handling of hardware specific phy event */
void UCPD_PHY_Handler(UCPD_PORT_Number port_number) {

  uint32_t status = LL_UCPD_ReadReg(port_handles[port_number].instance, SR);

  if ((port_handles[port_number].instance->IMR & status) == 0u)
    return;

  /* Type C events */
  if (CHECK_STATUS(status, UCPD_SR_TYPECEVT1) ||
      CHECK_STATUS(status, UCPD_SR_TYPECEVT2)) {
    LL_UCPD_ClearFlag_TypeCEventCC1(port_handles[port_number].instance);
    LL_UCPD_ClearFlag_TypeCEventCC2(port_handles[port_number].instance);
    UCPD_PD_PHY_Event_Handle(port_number, PHY_EVENT_CC_CHANGE);
    return;
  }

  /* Hard reset events */
  /* Sent hard reset*/
  if (CHECK_STATUS(status, UCPD_SR_HRSTSENT)) {
    LL_UCPD_ClearFlag_TxHRSTSENT(port_handles[port_number].instance);
    UCPD_PD_PHY_Event_Handle(port_number, PHY_EVENT_HRD_SENT);

    return;
  }

  /* Received hard reset */
  if (CHECK_STATUS(status, UCPD_SR_RXHRSTDET)) {
    LL_UCPD_ClearFlag_RxHRST(port_handles[port_number].instance);
    UCPD_PHY_HRDRS_Received(port_number);

    return;
  }

  /* Hard reset rejected */
  if (CHECK_STATUS(status, UCPD_SR_HRSTDISC)) {
    LL_UCPD_ClearFlag_TxHRSTDISC(port_handles[port_number].instance);
    return;
  }

  /* Tx events */
  /* Tx aborted */
  if (CHECK_STATUS(status, UCPD_SR_TXMSGABT)) {
    LL_UCPD_ClearFlag_TxMSGABT(port_handles[port_number].instance);
    return;
  }

  /* Tx discarded */
  if (CHECK_STATUS(status, UCPD_SR_TXMSGDISC)) {
    LL_UCPD_ClearFlag_TxMSGDISC(port_handles[port_number].instance);
    UCPD_PHY_TX_Discarded(port_number);

    return;
  }

  /* Tx completed*/
  if (CHECK_STATUS(status, UCPD_SR_TXMSGSENT)) {
    LL_UCPD_ClearFlag_TxMSGSENT(port_handles[port_number].instance);

    UCPD_PHY_TX_Complete(port_number);
    return;
  }

  /* Rx events */
  /* Rx started */
  if (CHECK_STATUS(status, UCPD_SR_RXORDDET)) {
    LL_UCPD_ClearFlag_RxOrderSet(port_handles[port_number].instance);
    UCPD_PHY_RX_Started(port_number);

    return;
  }

  /* Rx completed */
  if (CHECK_STATUS(status, UCPD_SR_RXMSGEND)) {
    LL_UCPD_ClearFlag_RxMsgEnd(port_handles[port_number].instance);

    bool error = CHECK_STATUS(status, UCPD_SR_RXERR);
    UCPD_PHY_RX_Complete(port_number, error);

    if (error == false)
      UCPD_PD_PHY_Event_Handle(port_number, PHY_EVENT_MSG_RX_COMPLETE);

    return;
  }
}

/* HW Timer Implementation */
void UCPD_HW_Timer_Start(UCPD_PORT_Number port_number, uint32_t timer_channel,
                         UCPD_Time period) {
  (void)port_number;
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

void UCPD_HW_Timer_Stop(UCPD_PORT_Number port_number, uint32_t timer_channel) {
  (void)port_number;
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

void UCPD_TIM_Handler(UCPD_PORT_Number port_number) {
  (void)port_number;
  uint32_t sr = UCPD_TIMER_INSTANCE->SR;
  uint32_t dier = UCPD_TIMER_INSTANCE->DIER;

  // IN THEORY one timer overwriting the event of another timer in the
  // pe_prl_cad should not occur, but keeping in mind

  if ((sr & TIM_SR_CC1IF) && (dier & TIM_DIER_CC1IE)) {
    LL_TIM_ClearFlag_CC1(UCPD_TIMER_INSTANCE);
    UCPD_TIM_Expired(port_number, UCPD_TIMER_CHANNEL_1);
  }

  if ((sr & TIM_SR_CC2IF) && (dier & TIM_DIER_CC2IE)) {
    LL_TIM_ClearFlag_CC2(UCPD_TIMER_INSTANCE);
    UCPD_TIM_Expired(port_number, UCPD_TIMER_CHANNEL_2);
  }

  if ((sr & TIM_SR_CC3IF) && (dier & TIM_DIER_CC3IE)) {
    LL_TIM_ClearFlag_CC3(UCPD_TIMER_INSTANCE);
    UCPD_TIM_Expired(port_number, UCPD_TIMER_CHANNEL_3);
  }

  if ((sr & TIM_SR_CC4IF) && (dier & TIM_DIER_CC4IE)) {
    LL_TIM_ClearFlag_CC4(UCPD_TIMER_INSTANCE);
    UCPD_TIM_Expired(port_number, UCPD_TIMER_CHANNEL_4);
  }
}

/* Timer hardware initialization for UCPD timer system */
void UCPD_HW_Timer_Init(UCPD_PORT_Number port_number) {
  (void)port_number;
  // Use SystemCoreClock (standard STM32 variable) for timer clock calculation
  // For STM32G0, TIM1 typically runs at system clock frequency
  uint32_t timer_clock = SystemCoreClock;

  // Calculate prescaler for 1ms tick (1000 Hz timer frequency)
  uint32_t prescaler = (timer_clock / 1000) - 1;
  uint32_t autoreload = 0xFFFF;

  LL_RCC_SetTIMClockSource(LL_RCC_TIM1_CLKSOURCE_PCLK1);

  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM1);

  // Stop timer first to ensure clean state
  LL_TIM_DisableCounter(UCPD_TIMER_INSTANCE);

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

  EVENT_PRIORITY(UCPD_TIMER_EVENT, UCPD_EVENT_TIM_PRIORITY);
}

inline void UCPD_HW_Timer_Deinit(UCPD_PORT_Number port_number) {
  (void)port_number;
  LL_TIM_DisableCounter(UCPD_TIMER_INSTANCE);
}

/* PWR implementation */
static inline void AWD_Disable() {
  LL_ADC_ClearFlag_AWD1(UCPD_ADC_INSTANCE);
  LL_ADC_DisableIT_AWD1(UCPD_ADC_INSTANCE);
}

void AWDEnable(bool high) {
  if (high) {
    LL_ADC_SetAnalogWDMonitChannels(UCPD_ADC_INSTANCE, LL_ADC_AWD1,
                                    LL_ADC_AWD_CHANNEL_9_REG);
    LL_ADC_SetAnalogWDThresholds(UCPD_ADC_INSTANCE, LL_ADC_AWD1,
                                 LL_ADC_AWD_THRESHOLD_HIGH,
                                 4095); // High
    LL_ADC_SetAnalogWDThresholds(UCPD_ADC_INSTANCE, LL_ADC_AWD1,
                                 LL_ADC_AWD_THRESHOLD_LOW,
                                 UCPD_PWR_ADC_HIGH_THRESHOLD_RISE); // Low
  } else {
    LL_ADC_SetAnalogWDMonitChannels(UCPD_ADC_INSTANCE, LL_ADC_AWD1, LL_ADC_AWD_CHANNEL_9_REG);
	LL_ADC_SetAnalogWDThresholds(UCPD_ADC_INSTANCE, LL_ADC_AWD1, LL_ADC_AWD_THRESHOLD_HIGH,
	                             UCPD_PWR_ADC_LOW_THRESHOLD_RISE); // High
	LL_ADC_SetAnalogWDThresholds(UCPD_ADC_INSTANCE, LL_ADC_AWD1, LL_ADC_AWD_THRESHOLD_LOW,
	                             0); // Low

    
  }
  LL_ADC_ClearFlag_AWD1(UCPD_ADC_INSTANCE);
    LL_ADC_EnableIT_AWD1(UCPD_ADC_INSTANCE);
}

void UCPD_PWR_Init(UCPD_PORT_Number port_number) {
  UCPD_PWR_SM *sm = &port_handles[port_number].pwr_sm;

  LL_ADC_InitTypeDef ADC_InitStruct = {0};
  LL_ADC_REG_InitTypeDef ADC_REG_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* Peripheral clock enable */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC);

  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOB);
  /**ADC1 GPIO Configuration
  PB1   ------> ADC1_IN9
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_1;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  LL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* ADC1 interrupt Init */

  /** Configure the global features of the ADC (Clock, Resolution, Data
   * Alignment and number of conversion)
   */

  ADC_InitStruct.Clock = LL_ADC_CLOCK_SYNC_PCLK_DIV4;
  ADC_InitStruct.Resolution = LL_ADC_RESOLUTION_12B;
  ADC_InitStruct.DataAlignment = LL_ADC_DATA_ALIGN_RIGHT;
  ADC_InitStruct.LowPowerMode = LL_ADC_LP_MODE_NONE;
  LL_ADC_Init(ADC1, &ADC_InitStruct);
  LL_ADC_REG_SetSequencerConfigurable(ADC1, LL_ADC_REG_SEQ_CONFIGURABLE);

  while (LL_ADC_IsActiveFlag_CCRDY(ADC1) == 0) {
  }
  /* Clear flag ADC channel configuration ready */
  LL_ADC_ClearFlag_CCRDY(ADC1);
  ADC_REG_InitStruct.TriggerSource = LL_ADC_REG_TRIG_SOFTWARE;
  ADC_REG_InitStruct.SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE;
  ADC_REG_InitStruct.ContinuousMode = LL_ADC_REG_CONV_CONTINUOUS;
  ADC_REG_InitStruct.DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE;
  ADC_REG_InitStruct.Overrun = LL_ADC_REG_OVR_DATA_OVERWRITTEN;
  LL_ADC_REG_Init(ADC1, &ADC_REG_InitStruct);
  LL_ADC_SetOverSamplingScope(ADC1, LL_ADC_OVS_DISABLE);
  LL_ADC_SetTriggerFrequencyMode(ADC1, LL_ADC_CLOCK_FREQ_MODE_HIGH);
  LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_COMMON_1,
                                       LL_ADC_SAMPLINGTIME_160CYCLES_5);
  LL_ADC_SetSamplingTimeCommonChannels(ADC1, LL_ADC_SAMPLINGTIME_COMMON_2,
                                       LL_ADC_SAMPLINGTIME_160CYCLES_5);
  LL_ADC_DisableIT_EOC(ADC1);
  LL_ADC_DisableIT_EOS(ADC1);

  /* Enable ADC internal voltage regulator */
  LL_ADC_EnableInternalRegulator(ADC1);
  /* Delay for ADC internal voltage regulator stabilization. */
  /* Compute number of CPU cycles to wait for, from delay in us. */
  /* Note: Variable divided by 2 to compensate partially */
  /* CPU processing cycles (depends on compilation optimization). */
  /* Note: If system core clock frequency is below 200kHz, wait time */
  /* is only a few CPU processing cycles. */
  __IO uint32_t wait_loop_index;
  wait_loop_index = ((LL_ADC_DELAY_INTERNAL_REGUL_STAB_US *
                      (SystemCoreClock / (100000 * 2))) /
                     10);
  while (wait_loop_index != 0) {
    wait_loop_index--;
  }

  /** Configure Analog WatchDog 1
   */
  LL_ADC_SetAnalogWDMonitChannels(ADC1, LL_ADC_AWD1, LL_ADC_AWD_CHANNEL_9_REG);
  LL_ADC_ConfigAnalogWDThresholds(ADC1, LL_ADC_AWD1, 3000, 1000);
  LL_ADC_EnableIT_AWD1(ADC1);

  /** Configure Regular Channel
   */
  LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_9);

  /* Poll for ADC channel configuration ready */
  while (LL_ADC_IsActiveFlag_CCRDY(ADC1) == 0) {
  }
  /* Clear flag ADC channel configuration ready */
  LL_ADC_ClearFlag_CCRDY(ADC1);
  LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_9,
                                LL_ADC_SAMPLINGTIME_COMMON_1);

  AWD_Disable();

  sm->state = UCPD_PWR_STATE_NONE;
  sm->exited_v0 = false;
  sm->vbus_vS0_to_vS5 = false;

  if (LL_ADC_IsEnabled(UCPD_ADC_INSTANCE) == 0) {
    LL_ADC_EnableInternalRegulator(UCPD_ADC_INSTANCE);
    for (volatile uint32_t i = 0; i < (SystemCoreClock / 1000000) * 30; ++i) {
      __NOP();
    }

    LL_ADC_StartCalibration(UCPD_ADC_INSTANCE);
    while (LL_ADC_IsCalibrationOnGoing(UCPD_ADC_INSTANCE)) {
    }

    LL_ADC_ClearFlag_ADRDY(UCPD_ADC_INSTANCE);
    LL_ADC_Enable(UCPD_ADC_INSTANCE);
    while (!LL_ADC_IsActiveFlag_ADRDY(UCPD_ADC_INSTANCE)) {
    }
  }

  if (LL_ADC_REG_IsConversionOngoing(UCPD_ADC_INSTANCE) == 0) {
    LL_ADC_REG_StartConversion(UCPD_ADC_INSTANCE);
  }

  uint16_t voltage = UCPD_PWR_GetVbusVoltage(sm);

  if (voltage > UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V) {
    sm->state = UCPD_PWR_STATE_VBUS_PRESENT;
  } else if (voltage < UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V) {
    sm->state = UCPD_PWR_STATE_VBUS_ABSENT;
  } else {
    sm->state = UCPD_PWR_STATE_VBUS_TRANSIENT;
    sm->vbus_vS0_to_vS5 = true;
  }

  LL_mDelay(1);

  AWDEnable(false);

  LL_ADC_ClearFlag_AWD1(UCPD_ADC_INSTANCE);
  LL_ADC_EnableIT_AWD1(UCPD_ADC_INSTANCE);
}

void UCPD_PWR_Deinit(UCPD_PORT_Number port_number) {
  UCPD_PWR_SM *sm = &port_handles[port_number].pwr_sm;

  AWD_Disable();

  LL_ADC_DisableIT_AWD1(UCPD_ADC_INSTANCE);
  LL_ADC_Disable(UCPD_ADC_INSTANCE);

  sm->state = UCPD_PWR_STATE_NONE;
  sm->exited_v0 = false;
  sm->vbus_vS0_to_vS5 = false;
}

void UCPD_PWR_ConversionCallback(UCPD_PORT_Number port_number) {

  AWD_Disable();

  UCPD_PWR_SM *sm = &port_handles[port_number].pwr_sm;

  uint16_t vbus_voltage = UCPD_PWR_GetVbusVoltage(sm);

  UCPD_PWR_SM_Delta(port_number, sm, vbus_voltage);
}

uint16_t UCPD_PWR_GetVbusVoltage(UCPD_PWR_SM *sm) {
  (void)sm;

  uint32_t adc_value = LL_ADC_REG_ReadConversionData12(UCPD_ADC_INSTANCE);
  if (adc_value > 4095u) {
    while (1)
      ;
  }

  uint32_t millivolts = (adc_value * 19717UL) / 4095UL;

  if (millivolts > 20000UL) {
    millivolts = 20000UL;
  }

  return (uint16_t)millivolts;
}

void UCPD_HW_PHY_EVENT_ISR(void) { UCPD_PHY_Handler(0); }

void UCPD_PE_PRL_WORKER_ISR(void) { UCPD_Worker(0); }

void UCPD_HRD_HELPER_ISR(void) { UCPD_ISR_HRD_Helper(0); }

void UCPD_TIMER_ISR(void) { UCPD_TIM_Handler(0); }

void UCPD_ADC_ISR(void) {
  if (LL_ADC_IsActiveFlag_AWD1(ADC1)) {
    LL_ADC_ClearFlag_AWD1(ADC1);

    UCPD_PWR_ConversionCallback(0);
  }
}
