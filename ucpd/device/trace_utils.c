/*=============================================================================
 * TRACE SYSTEM - Simple Non-Blocking UART Debug Output Implementation (HAL
 *Version)
 *
 * This implements a lightweight string queue that transmits debug messages
 * over UART using DMA and HAL drivers. No formatting, no timestamps - just
 * simple, fast transmission of compile-time string literals with better
 * error handling and easier integration.
 *=============================================================================*/

#include "trace_utils.h"
#include "ucpd_hw_defines.h"

/* USB-PD PDO/APDO types */
#include "ucpd/ucpd_msg.h"

/* STM32 HAL includes for hardware access */
#include "stm32g0xx_hal.h"
#include "stm32g0xx_hal_dma.h"
#include "stm32g0xx_hal_gpio.h"
#include "stm32g0xx_hal_uart.h"


/*=============================================================================
 * HAL HANDLES - Hardware abstraction layer handles
 *=============================================================================*/

/**
 * @brief UART handle for trace output
 */
static UART_HandleTypeDef *trace_uart_handle = NULL;

/**
 * @brief DMA handle for UART transmission
 */
static DMA_HandleTypeDef *trace_dma_handle = NULL;

/*=============================================================================
 * STRING QUEUE - Core data structures
 *=============================================================================*/

/**
 * @brief String queue for non-blocking transmission
 *
 * This is a circular buffer of const char* pointers. Each entry points to
 * a compile-time string literal. The DMA reads directly from these strings,
 * so no copying is needed - very efficient.
 */
static const char *trace_string_queue[TRACE_STRING_QUEUE_SIZE];

/**
 * @brief Queue head index (where new strings are added)
 */
static volatile uint8_t trace_queue_head = 0u;

/**
 * @brief Queue tail index (where DMA reads from)
 */
static volatile uint8_t trace_queue_tail = 0u;

/**
 * @brief DMA transmission state
 * UCPD_TRUE = DMA is currently transmitting
 * UCPD_FALSE = DMA is idle, ready for next string
 */
static volatile UCPD_Bool trace_dma_busy = UCPD_FALSE;

/**
 * @brief Initialization state
 * UCPD_TRUE = TRACE_Init() has been called successfully
 */
static UCPD_Bool trace_initialized = UCPD_FALSE;

/**
 * @brief Last HAL error status
 */
static volatile HAL_StatusTypeDef trace_last_error = HAL_OK;

/* Forward declarations for queue helpers used below */
static inline uint8_t TRACE_NextIndex(uint8_t current);
static inline UCPD_Bool TRACE_IsQueueFull(void);
static inline UCPD_Bool TRACE_IsQueueEmpty(void);
static void TRACE_StartNextTransmission(void);

/* Single shared build buffer for composing PDO/APDO lines */
#if TRACE_ENABLE
#define TRACE_BUILD_BUFFER_LEN 512u
static char trace_build_buffer[TRACE_BUILD_BUFFER_LEN];
#else
#define TRACE_BUILD_BUFFER_LEN 0u
static char trace_build_buffer[TRACE_BUILD_BUFFER_LEN + 1u];
#endif

/* Minimal helpers to build strings without stdio */
static inline void TRACE_BufAppendChar(char *buf, uint16_t *pos, char c) {
  if (*pos < (TRACE_BUILD_BUFFER_LEN - 1u)) {
    buf[*pos] = c;
    (*pos)++;
    buf[*pos] = '\0';
  }
}

void TRACE_BufAppendStr(char *buf, uint16_t *pos, const char *str) {
  uint16_t p = *pos;
  while (str[0] != '\0' && p < (TRACE_BUILD_BUFFER_LEN - 1u)) {
    buf[p++] = *str++;
  }
  buf[p] = '\0';
  *pos = p;
}

void TRACE_BufAppendUInt(char *buf, uint16_t *pos, uint32_t value) {
  /* Convert unsigned int to decimal */
  char tmp[11];
  uint8_t t = 0u;
  if (value == 0u) {
    TRACE_BufAppendChar(buf, pos, '0');
    return;
  }
  while (value > 0u && t < sizeof(tmp)) {
    tmp[t++] = (char)('0' + (value % 10u));
    value /= 10u;
  }
  while (t > 0u) {
    TRACE_BufAppendChar(buf, pos, tmp[--t]);
  }
}

/*=============================================================================
 * QUEUE MANAGEMENT - Internal helper functions
 *=============================================================================*/

/**
 * @brief Calculate next queue index with wraparound
 * @param current Current index
 * @return Next index (wraps to 0 at queue size)
 */
static inline uint8_t TRACE_NextIndex(uint8_t current) {
  return (current + 1u) & (TRACE_STRING_QUEUE_SIZE - 1u);
}

/**
 * @brief Check if queue is full
 * @return UCPD_TRUE if no space for new strings
 */
static inline UCPD_Bool TRACE_IsQueueFull(void) {
  return (TRACE_NextIndex(trace_queue_head) == trace_queue_tail) ? UCPD_TRUE
                                                                 : UCPD_FALSE;
}

/**
 * @brief Check if queue is empty
 * @return UCPD_TRUE if no strings pending
 */
static inline UCPD_Bool TRACE_IsQueueEmpty(void) {
  return (trace_queue_head == trace_queue_tail) ? UCPD_TRUE : UCPD_FALSE;
}

/**
 * @brief Start DMA transmission of next queued string using HAL
 *
 * This function dequeues the next string and starts DMA transmission using
 * HAL_UART_Transmit_DMA(). Much simpler than manual LL DMA setup.
 */
static void TRACE_StartNextTransmission(void) {
  const char *str;
  uint16_t length;
  HAL_StatusTypeDef hal_status;

  /* Safety check - should not happen if called correctly */
  if (trace_dma_busy == UCPD_TRUE || TRACE_IsQueueEmpty() == UCPD_TRUE ||
      trace_uart_handle == NULL || trace_dma_handle == NULL) {
    return;
  }

  /* Get next string from queue */
  str = trace_string_queue[trace_queue_tail];
  trace_queue_tail = TRACE_NextIndex(trace_queue_tail);

  /* Calculate string length manually (no strlen dependency) */
  length = 0u;
  while (str[length] != '\0' && length < 1000u) { /* 1000 = safety limit */
    length++;
  }

  /* Start DMA transmission using HAL - much simpler! */
  if (length > 0u) {
    /* Mark DMA as busy before starting */
    trace_dma_busy = UCPD_TRUE;

    /* HAL handles all the complex DMA setup for us */
    hal_status =
        HAL_UART_Transmit_DMA(trace_uart_handle, (uint8_t *)str, length);

    if (hal_status != HAL_OK) {
      /* Transmission failed - mark as not busy and save error */
      trace_dma_busy = UCPD_FALSE;
      trace_last_error = hal_status;

      /* Try next string if queue has more */
      if (TRACE_IsQueueEmpty() == UCPD_FALSE) {
        TRACE_StartNextTransmission();
      }
    }
  } else {
    /* Empty string - skip and try next */
    if (TRACE_IsQueueEmpty() == UCPD_FALSE) {
      TRACE_StartNextTransmission();
    }
  }
}

/*=============================================================================
 * PUBLIC API IMPLEMENTATION
 *=============================================================================*/

HAL_StatusTypeDef TRACE_Init(UART_HandleTypeDef *uart_handle,
                             DMA_HandleTypeDef *dma_handle) {
  /* Only initialize once */
  if (trace_initialized == UCPD_TRUE) {
    return HAL_OK;
  }

  /* Initialize hardware components references */
  trace_uart_handle = uart_handle;
  trace_dma_handle = dma_handle;

  /* Reset queue state */
  trace_queue_head = 0u;
  trace_queue_tail = 0u;
  trace_dma_busy = UCPD_FALSE;
  trace_last_error = HAL_OK;

  /* Mark as initialized */
  trace_initialized = UCPD_TRUE;

  /* Send initialization message */
  TRACE_QueueString("TRACE: HAL system initialized\n\r");

  return HAL_OK;
}

void TRACE_QueueString(const char *str) {
  /* Check if system is initialized and enabled */
  if (trace_initialized == UCPD_FALSE || TRACE_ENABLE == 0u ||
      trace_uart_handle == NULL || trace_dma_handle == NULL) {
    return;
  }

  /* Null pointer check */
  if (str == ((void *)0)) {
    return;
  }

  /* Enter critical section for queue manipulation */
  UCPD_ENTER_CRITICAL();

  /* Check if queue has space */
  if (TRACE_IsQueueFull() == UCPD_FALSE) {
    /* Add string to queue */
    trace_string_queue[trace_queue_head] = str;
    trace_queue_head = TRACE_NextIndex(trace_queue_head);

    /* Start transmission if DMA is idle */
    if (trace_dma_busy == UCPD_FALSE && TRACE_IsQueueEmpty() == UCPD_FALSE) {
      TRACE_StartNextTransmission();
    }
  }
  /* If queue is full, string is silently dropped - non-blocking behavior */

  UCPD_EXIT_CRITICAL();
}

UCPD_Bool TRACE_IsQueueReady(void) {
  return (TRACE_IsQueueFull() == UCPD_FALSE) ? UCPD_TRUE : UCPD_FALSE;
}

uint8_t TRACE_GetQueueCount(void) {
  uint8_t count;

  UCPD_ENTER_CRITICAL();

  if (trace_queue_head >= trace_queue_tail) {
    count = trace_queue_head - trace_queue_tail;
  } else {
    count = (TRACE_STRING_QUEUE_SIZE - trace_queue_tail) + trace_queue_head;
  }

  UCPD_EXIT_CRITICAL();

  return count;
}

void TRACE_Flush(void) {
  /* Wait until all strings are transmitted */
  while (trace_dma_busy == UCPD_TRUE || TRACE_IsQueueEmpty() == UCPD_FALSE) {
    /* Busy wait - use sparingly! */
    __NOP();
  }
}

HAL_StatusTypeDef TRACE_GetLastError(void) { return trace_last_error; }

/*=============================================================================
 * HAL CALLBACKS - Called automatically by HAL when events occur
 *=============================================================================*/

void TRACE_HAL_TxCpltCallback(UART_HandleTypeDef *huart) {
  /* Verify this is our UART instance */
  if (huart->Instance == TRACE_UART_INSTANCE) {
    /* Mark DMA as no longer busy */
    trace_dma_busy = UCPD_FALSE;

    /* Start next transmission if queue has more strings */
    if (TRACE_IsQueueEmpty() == UCPD_FALSE) {
      TRACE_StartNextTransmission();
    }
  }
}

void TRACE_HAL_ErrorCallback(UART_HandleTypeDef *huart) {
  /* Verify this is our UART instance */
  if (huart->Instance == TRACE_UART_INSTANCE) {
    /* Save error status and mark DMA as not busy */
    trace_last_error = HAL_ERROR;
    trace_dma_busy = UCPD_FALSE;

    /* Try to recover by attempting next transmission */
    if (TRACE_IsQueueEmpty() == UCPD_FALSE) {
      TRACE_StartNextTransmission();
    }
  }
}

/*=============================================================================
 * DMA INTERRUPT HANDLER - Uses HAL for proper state management
 *=============================================================================*/

const char *TRACE_GetSystemStatus(void) {
  /* Check HAL UART and DMA states for debugging */
  if (trace_uart_handle == NULL || trace_dma_handle == NULL)
    return "TRACE: REFERENCES NOT SET\n\r";

  HAL_UART_StateTypeDef uart_state = HAL_UART_GetState(trace_uart_handle);
  HAL_DMA_StateTypeDef dma_state = HAL_DMA_GetState(trace_dma_handle);

  /* Return compile-time status strings based on HAL states */
  if (uart_state == HAL_UART_STATE_BUSY_TX && dma_state == HAL_DMA_STATE_BUSY) {
    return "TRACE: UART_TX_BUSY+DMA_BUSY\n\r";
  } else if (uart_state == HAL_UART_STATE_BUSY_TX) {
    return "TRACE: UART_TX_BUSY\n\r";
  } else if (dma_state == HAL_DMA_STATE_BUSY) {
    return "TRACE: DMA_BUSY\n\r";
  } else if (uart_state == HAL_UART_STATE_ERROR) {
    return "TRACE: UART_ERROR\n\r";
  } else if (uart_state == HAL_UART_STATE_READY &&
             dma_state == HAL_DMA_STATE_READY) {
    return "TRACE: READY\n\r";
  } else {
    return "TRACE: UNKNOWN_STATE\n\r";
  }
}

/*=============================================================================
 * PDO/APDO PRINTING HELPERS
 *=============================================================================*/

static void TRACE_PrintSrcPDO_Internal(const UCPD_SRC_PDO *pdo, uint8_t index) {
  if (pdo == ((void *)0)) {
    return;
  }
  uint16_t pos = 0u;
  trace_build_buffer[0] = '\0';

  /* Prefix */
  TRACE_BufAppendStr(trace_build_buffer, &pos, "PDO ");
  TRACE_BufAppendUInt(trace_build_buffer, &pos, (uint32_t)index + 1u);
  TRACE_BufAppendStr(trace_build_buffer, &pos, ": ");

  if (pdo->pdo_type == (uint32_t)UCPD_PDO_TYPE_FIXED_SUPPLY) {
    uint32_t mv = (uint32_t)pdo->FixedSupply.b.voltage50mV * 50u;
    uint32_t ma = (uint32_t)pdo->FixedSupply.b.max_current10mA * 10u;
    TRACE_BufAppendStr(trace_build_buffer, &pos, "FIXED ");
    TRACE_BufAppendUInt(trace_build_buffer, &pos, mv);
    TRACE_BufAppendStr(trace_build_buffer, &pos, "mV ");
    TRACE_BufAppendUInt(trace_build_buffer, &pos, ma);
    TRACE_BufAppendStr(trace_build_buffer, &pos, "mA");
  } else if (pdo->pdo_type == (uint32_t)UCPD_PDO_TYPE_APDO) {
    if (pdo->APDO.apdo_type == (uint32_t)UCPD_APDO_TYPE_PPS) {
      uint32_t vmin_mv = (uint32_t)pdo->APDO.PPS.min_voltage100mV * 100u;
      uint32_t vmax_mv = (uint32_t)pdo->APDO.PPS.max_voltage100mV * 100u;
      uint32_t imax_ma = (uint32_t)pdo->APDO.PPS.max_current50mA * 50u;
      TRACE_BufAppendStr(trace_build_buffer, &pos, "APDO ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, vmin_mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "-");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, vmax_mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mV ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, imax_ma);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mA");
    } else {
      /* AVS EPR APDO */
      uint32_t vmin_mv = (uint32_t)pdo->APDO.AVS.min_voltage100mV * 100u;
      uint32_t vmax_mv = (uint32_t)pdo->APDO.AVS.max_voltage100mV * 100u;
      uint32_t pdp_w = (uint32_t)pdo->APDO.AVS.pdp_1w;
      TRACE_BufAppendStr(trace_build_buffer, &pos, "AVS ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, vmin_mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "-");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, vmax_mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mV PDP=");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, pdp_w);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "W");
    }
  } else {
    TRACE_BufAppendStr(trace_build_buffer, &pos, "UNKNOWN");
  }

  TRACE_BufAppendStr(trace_build_buffer, &pos, "\n\r");
  TRACE_QueueString(trace_build_buffer);
}

static void TRACE_PrintSnkPDO_Internal(const UCPD_SNK_PDO *pdo, uint8_t index) {
  if (pdo == ((void *)0)) {
    return;
  }
  uint16_t pos = 0u;
  trace_build_buffer[0] = '\0';

  /* Prefix */
  TRACE_BufAppendStr(trace_build_buffer, &pos, "PDO ");
  TRACE_BufAppendUInt(trace_build_buffer, &pos, (uint32_t)index + 1u);
  TRACE_BufAppendStr(trace_build_buffer, &pos, ": ");

  if (pdo->pdo_type == (uint32_t)UCPD_PDO_TYPE_FIXED_SUPPLY) {
    uint32_t mv = (uint32_t)pdo->FixedSupply.voltage50mV * 50u;
    uint32_t ma = (uint32_t)pdo->FixedSupply.op_current10mA * 10u;
    TRACE_BufAppendStr(trace_build_buffer, &pos, "FIXED ");
    TRACE_BufAppendUInt(trace_build_buffer, &pos, mv);
    TRACE_BufAppendStr(trace_build_buffer, &pos, "mV ");
    TRACE_BufAppendUInt(trace_build_buffer, &pos, ma);
    TRACE_BufAppendStr(trace_build_buffer, &pos, "mA");
  } else if (pdo->pdo_type == (uint32_t)UCPD_PDO_TYPE_APDO) {
    if (pdo->APDO.apdo_type == (uint32_t)UCPD_APDO_TYPE_PPS) {
      uint32_t vmin_mv = (uint32_t)pdo->APDO.PPS.min_voltage100mV * 100u;
      uint32_t vmax_mv = (uint32_t)pdo->APDO.PPS.max_voltage100mV * 100u;
      uint32_t imax_ma = (uint32_t)pdo->APDO.PPS.max_current50mA * 50u;
      TRACE_BufAppendStr(trace_build_buffer, &pos, "APDO ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, vmin_mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "-");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, vmax_mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mV ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, imax_ma);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mA");
    } else {
      /* AVS EPR APDO */
      uint32_t vmin_mv = (uint32_t)pdo->APDO.AVS.min_voltage100mV * 100u;
      uint32_t vmax_mv = (uint32_t)pdo->APDO.AVS.max_voltage100mV * 100u;
      uint32_t pdp_w = (uint32_t)pdo->APDO.AVS.pdp_1w;
      TRACE_BufAppendStr(trace_build_buffer, &pos, "AVS ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, vmin_mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "-");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, vmax_mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mV PDP=");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, pdp_w);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "W");
    }
  } else {
    TRACE_BufAppendStr(trace_build_buffer, &pos, "UNKNOWN");
  }

  TRACE_BufAppendStr(trace_build_buffer, &pos, "\n\r");
  TRACE_QueueString(trace_build_buffer);
}

void TRACE_PrintSrcPDO(const UCPD_SRC_PDO *pdo, uint8_t index) {
  if (pdo == ((void *)0)) {
    return;
  }
  TRACE_PrintSrcPDO_Internal(pdo, index);
}

void TRACE_PrintSrcPDOs(const UCPD_SRC_PDO *pdos, uint8_t count) {
  if (pdos == ((void *)0)) {
    return;
  }
  uint16_t pos = 0u;
  trace_build_buffer[0] = '\0';
  for (uint8_t i = 0u; i < count; i++) {
    /* Prefix */
    TRACE_BufAppendStr(trace_build_buffer, &pos, "PDO ");
    TRACE_BufAppendUInt(trace_build_buffer, &pos, (uint32_t)i + 1u);
    TRACE_BufAppendStr(trace_build_buffer, &pos, ": ");

    const UCPD_SRC_PDO *pdo = &pdos[i];
    if (pdo->pdo_type == (uint32_t)UCPD_PDO_TYPE_FIXED_SUPPLY) {
      uint32_t mv = (uint32_t)pdo->FixedSupply.b.voltage50mV * 50u;
      uint32_t ma = (uint32_t)pdo->FixedSupply.b.max_current10mA * 10u;
      TRACE_BufAppendStr(trace_build_buffer, &pos, "FIXED ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mV ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, ma);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mA");
    } else if (pdo->pdo_type == (uint32_t)UCPD_PDO_TYPE_APDO) {
      if (pdo->APDO.apdo_type == (uint32_t)UCPD_APDO_TYPE_PPS) {
        uint32_t vmin_mv = (uint32_t)pdo->APDO.PPS.min_voltage100mV * 100u;
        uint32_t vmax_mv = (uint32_t)pdo->APDO.PPS.max_voltage100mV * 100u;
        uint32_t imax_ma = (uint32_t)pdo->APDO.PPS.max_current50mA * 50u;
        TRACE_BufAppendStr(trace_build_buffer, &pos, "APDO ");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, vmin_mv);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "-");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, vmax_mv);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "mV ");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, imax_ma);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "mA");
      } else {
        /* AVS EPR APDO */
        uint32_t vmin_mv = (uint32_t)pdo->APDO.AVS.min_voltage100mV * 100u;
        uint32_t vmax_mv = (uint32_t)pdo->APDO.AVS.max_voltage100mV * 100u;
        uint32_t pdp_w = (uint32_t)pdo->APDO.AVS.pdp_1w;
        TRACE_BufAppendStr(trace_build_buffer, &pos, "AVS ");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, vmin_mv);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "-");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, vmax_mv);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "mV PDP=");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, pdp_w);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "W");
      }
    } else {
      TRACE_BufAppendStr(trace_build_buffer, &pos, "UNKNOWN");
    }

    /* Line break between objects */
    TRACE_BufAppendStr(trace_build_buffer, &pos, "\n\r");
    if (pos >= (TRACE_BUILD_BUFFER_LEN - 8u)) {
      break; /* prevent overflow if too many */
    }
  }
  TRACE_QueueString(trace_build_buffer);
}

void TRACE_PrintSnkPDO(const UCPD_SNK_PDO *pdo, uint8_t index) {
  if (pdo == ((void *)0)) {
    return;
  }
  TRACE_PrintSnkPDO_Internal(pdo, index);
}

void TRACE_PrintSnkPDOs(const UCPD_SNK_PDO *pdos, uint8_t count) {
  if (pdos == ((void *)0)) {
    return;
  }
  uint16_t pos = 0u;
  trace_build_buffer[0] = '\0';
  for (uint8_t i = 0u; i < count; i++) {
    /* Prefix */
    TRACE_BufAppendStr(trace_build_buffer, &pos, "PDO ");
    TRACE_BufAppendUInt(trace_build_buffer, &pos, (uint32_t)i + 1u);
    TRACE_BufAppendStr(trace_build_buffer, &pos, ": ");

    const UCPD_SNK_PDO *pdo = &pdos[i];
    if (pdo->pdo_type == (uint32_t)UCPD_PDO_TYPE_FIXED_SUPPLY) {
      uint32_t mv = (uint32_t)pdo->FixedSupply.voltage50mV * 50u;
      uint32_t ma = (uint32_t)pdo->FixedSupply.op_current10mA * 10u;
      TRACE_BufAppendStr(trace_build_buffer, &pos, "FIXED ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, mv);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mV ");
      TRACE_BufAppendUInt(trace_build_buffer, &pos, ma);
      TRACE_BufAppendStr(trace_build_buffer, &pos, "mA");
    } else if (pdo->pdo_type == (uint32_t)UCPD_PDO_TYPE_APDO) {
      if (pdo->APDO.apdo_type == (uint32_t)UCPD_APDO_TYPE_PPS) {
        uint32_t vmin_mv = (uint32_t)pdo->APDO.PPS.min_voltage100mV * 100u;
        uint32_t vmax_mv = (uint32_t)pdo->APDO.PPS.max_voltage100mV * 100u;
        uint32_t imax_ma = (uint32_t)pdo->APDO.PPS.max_current50mA * 50u;
        TRACE_BufAppendStr(trace_build_buffer, &pos, "APDO ");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, vmin_mv);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "-");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, vmax_mv);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "mV ");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, imax_ma);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "mA");
      } else {
        /* AVS EPR APDO */
        uint32_t vmin_mv = (uint32_t)pdo->APDO.AVS.min_voltage100mV * 100u;
        uint32_t vmax_mv = (uint32_t)pdo->APDO.AVS.max_voltage100mV * 100u;
        uint32_t pdp_w = (uint32_t)pdo->APDO.AVS.pdp_1w;
        TRACE_BufAppendStr(trace_build_buffer, &pos, "AVS ");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, vmin_mv);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "-");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, vmax_mv);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "mV PDP=");
        TRACE_BufAppendUInt(trace_build_buffer, &pos, pdp_w);
        TRACE_BufAppendStr(trace_build_buffer, &pos, "W");
      }
    } else {
      TRACE_BufAppendStr(trace_build_buffer, &pos, "UNKNOWN");
    }

    /* Line break between objects */
    TRACE_BufAppendStr(trace_build_buffer, &pos, "\n\r");
    if (pos >= (TRACE_BUILD_BUFFER_LEN - 8u)) {
      break; /* prevent overflow if too many */
    }
  }
  TRACE_QueueString(trace_build_buffer);
}
