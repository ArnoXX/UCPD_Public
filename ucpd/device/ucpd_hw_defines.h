#ifndef UCPD_HW_DEF_H
#define UCPD_HW_DEF_H

// #include "stm32g0xx_ll_ucpd.h"
#include "stm32g0b1xx.h"

/* Interrupt utilities */

#define CHECK_STATUS(status, flag) (flag == (status & flag))
#define CLEAR_STATUS(status, flag) (status &= ~flag)

/* STM32 PD Peripheral driver instance */
#define UCPD_INSTANCE_TYPE UCPD_TypeDef
#define UCPD_INSTANCE UCPD1

#define UCPD_HW_PHY_EVENT USB_UCPD1_2_IRQn
#define UCPD_HW_PHY_EVENT_HANDLER UCPD_PHY_Handler

/* Event system ISRs defines */
/* UCPD WORKER */
#define UCPD_PE_PRL_WORKER_EVENT DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQn

#define UCPD_PE_PRL_WORKER_INIT()                                              \
  do {                                                                         \
    NVIC_SetPriority(UCPD_PE_PRL_WORKER_EVENT, 1);                             \
    NVIC_ClearPendingIRQ(UCPD_PE_PRL_WORKER_EVENT);                            \
    NVIC_EnableIRQ(UCPD_PE_PRL_WORKER_EVENT);                                  \
  } while (0)

#define UCPD_PE_PRL_WORKER_ROUTINE UCPD_PE_PRL_Handler

/* UCPD HARD RESET HELPER */
#define UCPD_HARD_RESET_HELPER_EVENT TIM14_IRQn

#define UCPD_HARD_RESET_HELPER_INIT()                                          \
  do {                                                                         \
    NVIC_SetPriority(UCPD_HARD_RESET_HELPER_EVENT, 2);                         \
    NVIC_ClearPendingIRQ(UCPD_HARD_RESET_HELPER_EVENT);                        \
    NVIC_EnableIRQ(UCPD_HARD_RESET_HELPER_EVENT);                              \
  } while (0)

#define UCPD_HARD_RESET_HELPER_ROUTINE UCPD_HRD_Helper

/* Timer ISR defines for STM32 TIM1 used by UCPD timer system */
#define UCPD_TIMER_INSTANCE TIM1
#define UCPD_TIMER_EVENT TIM1_CC_IRQn
#define UCPD_TIMER_EVENT_HANDLER UCPD_TIM_Handler

/* Event system functions defines */
#define EVENT_PRIORITY(event, priority) NVIC_SetPriority(event, priority);
#define BLOCK_EVENT(event) NVIC_DisableIRQ(event)
#define UNBLOCK_EVENT(event) NVIC_EnableIRQ(event)
#define PEND_EVENT(event) NVIC_SetPendingIRQ(event)
#define CLEAR_EVENT(event) NVIC_ClearPendingIRQ(event);

/* DMA defines */
#define UCPD_DATA_TRANSFER_CONTROL DMA_Channel_TypeDef

#define UCPD_DATA_TRANSFER_RX_CHANNEL DMA1_Channel2
#define UCPD_DATA_TRANSFER_TX_CHANNEL DMA1_Channel3

#define UCPD_DATA_TRANSFER DMA1

/* Critical section defines */
#define UCPD_ENTER_CRITICAL()                                                  \
  uint32_t primask = __get_PRIMASK();                                          \
  __disable_irq()

#define UCPD_EXIT_CRITICAL() __set_PRIMASK(primask)

/* ADC defines for VBUS monitoring */
#define UCPD_ADC_INSTANCE ADC1
#define UCPD_ADC_VBUS_CHANNEL ADC_CHANNEL_9 /* PB1 - VBUS sense pin */
#define UCPD_ADC_VBUS_RANK ADC_REGULAR_RANK_1

/* ADC GPIO configuration */
#define UCPD_ADC_VBUS_PORT GPIOB
#define UCPD_ADC_VBUS_PIN GPIO_PIN_1

/* ADC interrupt defines */
#define UCPD_ADC_EVENT ADC1_COMP_IRQn
#define UCPD_ADC_EVENT_HANDLER ADC1_COMP_IRQHandler

/* USB C cc defines */
#define UCPD_CCNONE 0x00u
#define UCPD_CC1 0x01u
#define UCPD_CC2 0x02u

#define UCPD_TYPEC_CC_TYPE uint32_t

#define UCPD_CC_DETECT_VOPEN(val, pin) (val == LL_UCPD_SNK_##pin##_VOPEN)

#define UCPD_CC_CHECK_TX_OK(status, pin)                                       \
  ((status & UCPD_SR_TYPEC_VSTATE_##pin) == LL_UCPD_SNK_##pin##_VRP30A)

// for enabling only type c cc events interrupts
#define UCPD_CC_DETECTION_ONLY(mask)                                           \
  WRITE_REG(mask, UCPD_IMR_TYPECEVT1IE | UCPD_IMR_TYPECEVT2IE)

#define UCPD_INT_NORMAL_MASK                                                   \
  UCPD_IMR_TYPECEVT1IE | UCPD_IMR_TYPECEVT2IE | UCPD_IMR_RXMSGENDIE |          \
      UCPD_IMR_RXHRSTDETIE | UCPD_IMR_RXORDDETIE | UCPD_IMR_HRSTSENTIE |       \
      UCPD_IMR_TXMSGSENTIE | UCPD_IMR_TXMSGDISCIE

#define UCPD_INT_PHY_WORKER_MASK                                               \
  UCPD_IMR_RXHRSTDETIE

#define UCPD_INT_SET(mask, val) MODIFY_REG(mask, val, val)

/* UCPD port instance define */
typedef struct {
  UCPD_INSTANCE_TYPE *instance;

  UCPD_DATA_TRANSFER_CONTROL *rxData;
  UCPD_DATA_TRANSFER_CONTROL *txData;

  UCPD_TYPEC_CC_TYPE cc;

} UCPD_HW_PORT_Handle;

/* DMA data transfer control */
void blockTxDataTransfer(UCPD_HW_PORT_Handle *port_handle);

void blockRxDataTransfer(UCPD_HW_PORT_Handle *port_handle);

void unblockTxDataTransfer(UCPD_HW_PORT_Handle *port_handle);

void unblockRxDataTransfer(UCPD_HW_PORT_Handle *port_handle);

void confTxDataTransfer(UCPD_HW_PORT_Handle *port_handle, uint8_t *buffer,
                        uint32_t size);

void confRxDataTransfer(UCPD_HW_PORT_Handle *port_handle, uint8_t *buffer,
                        uint32_t size);

#endif // UCPD_HW_DEF_H
