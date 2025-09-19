#ifndef TRACE_UTILS_H
#define TRACE_UTILS_H

#include "ucpd/ucpd_utils.h"
#include <stdint.h>


/* USB-PD message/type definitions for PDO/APDO structures */
#include "ucpd/ucpd_msg.h"

/* STM32 HAL includes for type definitions */
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_uart.h"

/*=============================================================================
 * TRACE SYSTEM - Simple Non-Blocking UART Debug Output (HAL Version)
 *
 * This module provides a lightweight, interrupt-safe tracing system that:
 * - Queues compile-time strings for DMA transmission over UART
 * - Never blocks (drops messages if queue full)
 * - Provides semantic macros for different trace categories
 * - Uses minimal memory (just array of const char pointers)
 * - Zero-copy design (DMA reads strings directly)
 * - Uses HAL drivers for easier integration and better error handling
 *=============================================================================*/

/* Trace system configuration */
#define TRACE_ENABLE 1u
#define TRACE_UART_ENABLE 1u
#define TRACE_DMA_ENABLE 1u

/* String queue size - must be power of 2 for efficient modulo */
#if TRACE_ENABLE
#define TRACE_STRING_QUEUE_SIZE 32u
#else
#define TRACE_STRING_QUEUE_SIZE 0u
#endif

/* UART configuration for STM32G0B1RE debug output */
#define TRACE_UART_INSTANCE USART2
#define TRACE_UART_BAUDRATE 115200u

/* UART GPIO pins (PA2=TX, PA3=RX) */
#define TRACE_UART_TX_PORT GPIOA
#define TRACE_UART_TX_PIN GPIO_PIN_2
#define TRACE_UART_RX_PORT GPIOA
#define TRACE_UART_RX_PIN GPIO_PIN_3
#define TRACE_UART_AF GPIO_AF1_USART2

/* DMA configuration - use Channel 3 to avoid conflict with UCPD (uses 1,2) */
#define TRACE_DMA_INSTANCE DMA1
#define TRACE_DMA_CHANNEL DMA1_Channel3
#define TRACE_DMA_REQUEST DMA_REQUEST_USART2_TX
#define TRACE_DMA_IRQn DMA1_Channel2_3_IRQn

/*=============================================================================
 * SEMANTIC TRACE MACROS - Main API for different trace categories
 *
 * These macros provide meaningful names for different types of debug output.
 * All strings are compile-time constants - no runtime formatting.
 *=============================================================================*/

#if TRACE_ENABLE

/* Event system tracing - for UCPD event queue operations */
#define TRACE_EVENT_EMIT(event_type, event_name)                               \
  TRACE_QueueString("EVENT: " event_type " - " event_name)

#define TRACE_EVENT_PROCESS(event_name)                                        \
  TRACE_QueueString("PROCESS: " event_name)

/* State machine transitions - for PE/PRL/CAD state changes */
#define TRACE_STATE_CHANGE(sm_name, old_state, new_state)                      \
  TRACE_QueueString("STATE: " sm_name " [" old_state "] -> [" new_state "]\n"  \
                                                                        "\r")

/* Timer operations - for UCPD timer system */
#define TRACE_TIMER_START(timer_name)                                          \
  TRACE_QueueString("TIMER_START: " timer_name)

#define TRACE_TIMER_STOP(timer_name)                                           \
  TRACE_QueueString("TIMER_STOP: " timer_name)

#define TRACE_TIMER_EXPIRED(timer_name)                                        \
  TRACE_QueueString("TIMER_EXPIRED: " timer_name)

/* Hardware events - for UCPD peripheral interrupts */
#define TRACE_HW_EVENT(event_desc) TRACE_QueueString("HW: " event_desc)

#define TRACE_HW_IRQ(irq_name) TRACE_QueueString("IRQ: " irq_name)

/* USB-C CC line status - for Type-C detection */
#define TRACE_CC_STATE(cc1_state, cc2_state)                                   \
  TRACE_QueueString("CC: CC1=" cc1_state " CC2=" cc2_state)

/* Message transmission - for PD protocol messages */
#define TRACE_MSG_TX(msg_type) TRACE_QueueString("TX: " msg_type)

#define TRACE_MSG_RX(msg_type) TRACE_QueueString("RX: " msg_type)

/* DMA operations - for data transfer debugging */
#define TRACE_DMA_START(direction) TRACE_QueueString("DMA_START: " direction)

#define TRACE_DMA_COMPLETE(direction)                                          \
  TRACE_QueueString("DMA_COMPLETE: " direction)

/* General purpose tracing - for any debug message */
#define TRACE_INFO(message) TRACE_QueueString("INFO: " message)

#define TRACE_ERROR(message) TRACE_QueueString("ERROR: " message)

#define TRACE_DEBUG(message) TRACE_QueueString("DEBUG: " message)

#else
/* Trace disabled - all macros compile to nothing */
#define TRACE_EVENT_EMIT(event_type, event_name)
#define TRACE_EVENT_PROCESS(event_name)
#define TRACE_STATE_CHANGE(sm_name, old_state, new_state)
#define TRACE_TIMER_START(timer_name)
#define TRACE_TIMER_STOP(timer_name)
#define TRACE_TIMER_EXPIRED(timer_name)
#define TRACE_HW_EVENT(event_desc)
#define TRACE_HW_IRQ(irq_name)
#define TRACE_CC_STATE(cc1_state, cc2_state)
#define TRACE_MSG_TX(msg_type)
#define TRACE_MSG_RX(msg_type)
#define TRACE_DMA_START(direction)
#define TRACE_DMA_COMPLETE(direction)
#define TRACE_INFO(message)
#define TRACE_ERROR(message)
#define TRACE_DEBUG(message)
#endif

/*=============================================================================
 * CORE TRACE FUNCTIONS - Internal implementation
 *=============================================================================*/

/**
 * @brief Initialize the trace system
 *
 * Sets up UART2, GPIO pins, and DMA for debug output using HAL drivers.
 * Call once at system startup before using trace macros.
 *
 * @return HAL_OK if initialization successful, HAL_ERROR otherwise
 */
HAL_StatusTypeDef TRACE_Init(UART_HandleTypeDef *uart_handle,
                             DMA_HandleTypeDef *dma_handle);

/**
 * @brief Queue a string for transmission
 * @param str Pointer to compile-time string constant
 *
 * Adds string to transmission queue. If queue is full, message is dropped.
 * This function is interrupt-safe and never blocks.
 *
 * Note: String must remain valid until transmission completes (use string
 * literals)
 */
void TRACE_QueueString(const char *str);

/**
 * @brief Check if trace queue has space
 * @return UCPD_TRUE if queue can accept more strings, UCPD_FALSE if full
 */
UCPD_Bool TRACE_IsQueueReady(void);

/**
 * @brief Get number of pending strings in queue
 * @return Number of strings waiting for transmission
 */
uint8_t TRACE_GetQueueCount(void);

/**
 * @brief Flush all pending trace output
 *
 * Blocks until all queued strings have been transmitted.
 * Use sparingly - prefer the non-blocking nature of the trace system.
 */
void TRACE_Flush(void);

/**
 * @brief Get last HAL error status from trace operations
 * @return HAL_StatusTypeDef indicating last error
 */
HAL_StatusTypeDef TRACE_GetLastError(void);

/*=============================================================================
 * HAL CALLBACKS - Must be implemented in your main application
 *=============================================================================*/

/**
 * @brief HAL UART DMA TX Complete callback
 * @param huart UART handle pointer
 *
 * This function is called automatically by HAL when DMA transmission completes.
 * You can implement this in your main application, or use the provided
 * TRACE_HAL_TxCpltCallback() wrapper.
 *
 * Example in your main.c:
 *
 * void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
 *     if (huart->Instance == USART2) {
 *         TRACE_HAL_TxCpltCallback(huart);
 *     }
 *     // Handle other UART instances...
 * }
 */
void TRACE_HAL_TxCpltCallback(UART_HandleTypeDef *huart);

/**
 * @brief HAL UART error callback
 * @param huart UART handle pointer
 *
 * Called when UART transmission error occurs. Optional to implement.
 */
void TRACE_HAL_ErrorCallback(UART_HandleTypeDef *huart);

/*=============================================================================
 * DMA INTERRUPT HANDLER - Must be called from your shared DMA ISR
 *=============================================================================*/

/**
 * @brief DMA completion interrupt handler for trace system
 *
 * This function handles DMA Channel 3 interrupts and is safe to call
 * from a shared DMA1_Channel2_3_IRQHandler. It uses HAL_DMA_IRQHandler()
 * internally for proper HAL state management.
 *
 * Example usage in your main interrupt handler file:
 *
 * void DMA1_Channel2_3_IRQHandler(void) {
 *     // Handle UCPD DMA Channel 2 interrupts first
 *     if (LL_DMA_IsActiveFlag_TC2(DMA1)) {
 *         // Your UCPD DMA handling here
 *         LL_DMA_ClearFlag_TC2(DMA1);
 *         // ... UCPD specific handling ...
 *     }
 *
 *     // Handle Trace DMA Channel 3 interrupts using HAL
 *     TRACE_DMA_IRQHandler();
 * }
 */
void TRACE_DMA_IRQHandler(void);

/**
 * @brief Get DMA and UART status for debugging
 * @return Debug status string (compile-time constant)
 *
 * Returns current status of HAL handles for debugging.
 * Can be used with TRACE_DEBUG() to monitor system state.
 */
const char *TRACE_GetSystemStatus(void);

/*=============================================================================
 * PDO/APDO PRINTING HELPERS - Public tracing convenience APIs
 *=============================================================================*/

/**
 * @brief Print a single Source PDO/APDO entry
 * @param pdo Pointer to source PDO
 * @param index Zero-based PDO index to display
 */
void TRACE_PrintSrcPDO(const UCPD_SRC_PDO *pdo, uint8_t index);

/**
 * @brief Print an array of Source PDO/APDO entries
 * @param pdos Pointer to first source PDO
 * @param count Number of PDOs
 */
void TRACE_PrintSrcPDOs(const UCPD_SRC_PDO *pdos, uint8_t count);

/**
 * @brief Print a single Sink PDO/APDO entry
 * @param pdo Pointer to sink PDO
 * @param index Zero-based PDO index to display
 */
void TRACE_PrintSnkPDO(const UCPD_SNK_PDO *pdo, uint8_t index);

/**
 * @brief Print an array of Sink PDO/APDO entries
 * @param pdos Pointer to first sink PDO
 * @param count Number of PDOs
 */
void TRACE_PrintSnkPDOs(const UCPD_SNK_PDO *pdos, uint8_t count);

#endif // TRACE_UTILS_H
