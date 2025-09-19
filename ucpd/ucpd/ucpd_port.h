#ifndef UCPD_PORT_H
#define UCPD_PORT_H

#include "device/ucpd_hw_defines.h"
#include "device/ucpd_pwr.h"
#include "ucpd/ucpd_cnt.h"
#include "ucpd/ucpd_pe_prl.h"
#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_tim.h"
#include "ucpd.h"

/* UCPD port count */
#define UCPD_PORT_COUNT 1

typedef struct {
  
  UCPD_PORT_Number port_number;

  UCPD_PE_PRL_CAD_Module pe_prl_cad;

  UCPD_PWR_SM pwr_sm;

  

  // change to multiple when using channels
  // Have to admit, it got me. Unchecked indecies in C is no joke, Tried to index with LL_TIM_CHANNEL_CH1, etc.
  // but it was not working.
  // Guess what, those constants are shifted 1u to the left, so powers of 2
  // was overwriting the DMA channel pointer in the port instance handle below
  UCPD_TIM ucpd_tim_at_channel[TIMER_CHANNEL_COUNT];

  UCPD_HW_PORT_Handle port_handle;

  // data storage
#ifdef UCPD_ENABLE_SINK_PDO_STORAGE
  UCPD_SNK_PDO snk_pdos[UCPD_MAX_PDO_NUM];
#endif
  UCPD_SRC_PDO src_pdos[UCPD_MAX_PDO_NUM + UCPD_MAX_EPR_PDO_NUM];
  UCPD_Count snk_src_pdo_count;
  UCPD_Count snk_pdo_count;

  volatile UCPD_Bool running_policy;
  UCPD_Callback current_callback;
  volatile UCPD_Bool sync_operation;
  UCPD_Bool initialized;


} UCPD_PORT_INSTANCE;

#endif // UCPD_PORT_H