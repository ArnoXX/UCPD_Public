#ifndef UCPD_HW_DEF_H
#define UCPD_HW_DEF_H

#include "stm32g0b1xx.h"
#include "stm32g0xx_ll_adc.h"
#include "stm32g0xx_ll_bus.h"
#include "stm32g0xx_ll_dma.h"
#include "stm32g0xx_ll_gpio.h"
#include "stm32g0xx_ll_rcc.h"
#include "stm32g0xx_ll_system.h"
#include "stm32g0xx_ll_tim.h"
#include "stm32g0xx_ll_ucpd.h"
#include "stm32g0xx_ll_utils.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_msg.h"
#include "ucpd/ucpd_pe_prl.h"
#include "ucpd/ucpd_port.h"
#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_tim.h"
#include "ucpd_hw_defines.h"
#include <stdbool.h>

/* Interrupt helper defines */
#define UCPD_EVENT_BASE_PRIORITY 0
#define UCPD_EVENT_PHY_PRIORITY UCPD_EVENT_BASE_PRIORITY
#define UCPD_EVENT_TIM_PRIORITY UCPD_EVENT_BASE_PRIORITY
#define UCPD_EVENT_ADC_PRIORITY UCPD_EVENT_BASE_PRIORITY
#define UCPD_EVENT_WORKER_PRIORITY (UCPD_EVENT_BASE_PRIORITY + 1u)
#define UCPD_EVENT_HRD_HELPER_PRIORITY (UCPD_EVENT_WORKER_PRIORITY + 2u)

#define CHECK_STATUS(status, flag) (flag == (status & flag))
#define CLEAR_STATUS(status, flag) (status &= ~flag)

#define UCPD_INT_SET(mask, val) MODIFY_REG(mask, val, val)

#define UCPD_ENTER_CRITICAL()                                                  \
  uint32_t primask = __get_PRIMASK();                                          \
  __disable_irq()

#define UCPD_EXIT_CRITICAL() __set_PRIMASK(primask)

#define EVENT_PRIORITY(event, priority) NVIC_SetPriority(event, priority);
#define BLOCK_EVENT(event) NVIC_DisableIRQ(event)
#define UNBLOCK_EVENT(event) NVIC_EnableIRQ(event)
#define PEND_EVENT(event) NVIC_SetPendingIRQ(event)
#define CLEAR_EVENT(event) NVIC_ClearPendingIRQ(event);

#define UCPD_CC_DETECTION_ONLY(mask)                                           \
  WRITE_REG(mask, UCPD_IMR_TYPECEVT1IE | UCPD_IMR_TYPECEVT2IE)

#define UCPD_INT_NORMAL_MASK                                                   \
  UCPD_IMR_TYPECEVT1IE | UCPD_IMR_TYPECEVT2IE | UCPD_IMR_RXMSGENDIE |          \
      UCPD_IMR_RXHRSTDETIE | UCPD_IMR_RXORDDETIE | UCPD_IMR_HRSTSENTIE |       \
      UCPD_IMR_TXMSGSENTIE | UCPD_IMR_TXMSGDISCIE

// when PHY ISR pends the worker, it allows only HRD while worker is running
// the purpose is to block any message transmission as soon as HRD comes in
#define UCPD_INT_PHY_WORKER_MASK UCPD_IMR_RXHRSTDETIE

/* STM32 PD Peripheral defines */
#define UCPD_INSTANCE UCPD1
#define UCPD_HW_PHY_EVENT USB_UCPD1_2_IRQn
#define UCPD_HW_PHY_EVENT_ISR USB_UCPD1_2_IRQHandler

/* UCPD Event system ISRs defines */
#define UCPD_PE_PRL_WORKER_EVENT DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQn
#define UCPD_PE_PRL_WORKER_ISR DMA1_Ch4_7_DMA2_Ch1_5_DMAMUX1_OVR_IRQHandler

#define UCPD_PE_PRL_WORKER_INIT()                                              \
  do {                                                                         \
    NVIC_SetPriority(UCPD_PE_PRL_WORKER_EVENT, UCPD_EVENT_WORKER_PRIORITY);    \
    NVIC_ClearPendingIRQ(UCPD_PE_PRL_WORKER_EVENT);                            \
  } while (0)

#define UCPD_HRD_HELPER_EVENT EXTI4_15_IRQn
#define UCPD_HRD_HELPER_ISR EXTI4_15_IRQHandler

#define UCPD_HRD_HELPER_INIT()                                                 \
  do {                                                                         \
    NVIC_SetPriority(UCPD_HRD_HELPER_EVENT, UCPD_EVENT_HRD_HELPER_PRIORITY);   \
    NVIC_ClearPendingIRQ(UCPD_HRD_HELPER_EVENT);                               \
  } while (0)

/* Timer ISR defines for STM32 TIM1 used by UCPD timer system */
#define UCPD_TIMER_INSTANCE TIM1
#define UCPD_TIMER_EVENT TIM1_CC_IRQn
#define UCPD_TIMER_ISR TIM1_CC_IRQHandler

/* DMA defines */
#define UCPD_DATA_TRANSFER_RX_CHANNEL DMA1_Channel2
#define UCPD_DATA_TRANSFER_TX_CHANNEL DMA1_Channel3

#define UCPD_DATA_TRANSFER DMA1

/* ADC defines */
#define UCPD_ADC_INSTANCE ADC1
#define UCPD_ADC_VBUS_CHANNEL ADC_CHANNEL_9 /* PB1 - VBUS sense pin */
#define UCPD_ADC_VBUS_RANK ADC_REGULAR_RANK_1

#define UCPD_ADC_VBUS_PORT GPIOB
#define UCPD_ADC_VBUS_PIN GPIO_PIN_1
#define UCPD_ADC_EVENT ADC1_COMP_IRQn
#define UCPD_ADC_ISR ADC1_COMP_IRQHandler

/* USB C cc defines */
#define UCPD_CCNONE 0x00u
#define UCPD_CC1 0x01u
#define UCPD_CC2 0x02u

#define UCPD_CC_DETECT_VOPEN(val, pin) (val == LL_UCPD_SNK_##pin##_VOPEN)

#define UCPD_CC_CHECK_TX_OK(status, pin)                                       \
  ((status & UCPD_SR_TYPEC_VSTATE_##pin) == LL_UCPD_SNK_##pin##_VRP30A)

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

/* Message buffer defines */
#define UCPD_BUFFER_COUNT 3u
#define UCPD_RX_BUFFER_INDEX 0u
#define UCPD_DRX_BUFFER_INDEX 1u
#define UCPD_TX_BUFFER_INDEX 2u
#define UCPD_EXT_BUFFER_COUNT 2u
#define UCPD_EXT_RX_BUFFER_INDEX 0u
#define UCPD_EXT_TX_BUFFER_INDEX 1u

/* PWR defines*/
/* Power transition states */
typedef enum {
  UCPD_PWR_STATE_NONE,
  UCPD_PWR_STATE_VBUS_ABSENT,
  UCPD_PWR_STATE_VBUS_TRANSIENT,
  UCPD_PWR_STATE_VBUS_PRESENT
} UCPD_PWR_SM_State;

/* Power transition state machine */
typedef struct {
  UCPD_PWR_SM_State state;
  bool vbus_vS0_to_vS5; // there is not just v5 on the bus, but there was a
                        // transition from v0 to v5
  bool exited_v0;       // it means we exited v0 to transient
} UCPD_PWR_SM;

#define UCPD_PWR_VOLTAGE_LOWER_V_SAFE_0V 0
#define UCPD_PWR_VOLTAGE_UPPER_V_SAFE_5V 5000
#define UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V 800
#define UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V 4750

#define UCPD_PWR_ADC_LOW_THRESHOLD_RISE 167
#define UCPD_PWR_ADC_LOW_THRESHOLD_FALL 160

#define UCPD_PWR_ADC_HIGH_THRESHOLD_RISE 986
#define UCPD_PWR_ADC_HIGH_THRESHOLD_FALL 970

void UCPD_PWR_SM_Delta(UCPD_PORT_Number port_number, UCPD_PWR_SM *sm,
                       uint16_t vbus_voltage);

/* Power management functions */
uint16_t UCPD_PWR_GetVbusVoltage(UCPD_PWR_SM *sm);

/* Power management hardware abstraction functions */
void UCPD_PWR_ConversionCallback(UCPD_PORT_Number port_number);

/* UCPD physical port handle define */
typedef struct {
  UCPD_TypeDef *instance;
  UCPD_PWR_SM pwr_sm;

  DMA_Channel_TypeDef *rxData;
  DMA_Channel_TypeDef *txData;

  uint32_t cc;

  volatile uint8_t rx_buffer_index;

  UCPD_MSG buffers[UCPD_BUFFER_COUNT];
  UCPD_EXT_MSG ext_buffers[UCPD_EXT_BUFFER_COUNT];

} UCPD_HW_PORT_Handle;

UCPD_HW_PORT_Handle* getPortHandles(void);

/* Function defines */

void UCPD_Worker(UCPD_PORT_Number port_number);

void UCPD_PHY_Handler(UCPD_PORT_Number port_number);

void UCPD_TIM_Handler(UCPD_PORT_Number port_number);

void UCPD_ISR_HRD_Helper(UCPD_PORT_Number port_number);

UCPD_PE_PRL_CAD_Event getCADEventFromCC(UCPD_PORT_Number port_number);

void UCPD_Worker(UCPD_PORT_Number port_number);

void UCPD_ISR_HRD_Helper(UCPD_PORT_Number port_number);

void AWDEnable(bool high);

void UCPD_PD_PHY_Event_Handle(UCPD_PORT_Number port_number,
                              UCPD_PHY_Event event);

/* Static function defines */

#endif // UCPD_HW_DEF_H
