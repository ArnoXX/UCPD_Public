#include "ucpd_cnt.h"
#include "device/trace_utils.h"
#include "ucpd/ucpd_ctx.h"


void UCPD_CNT_Init(UCPD_PORT_Number port_number, UCPD_CNT cnt) {
  UCPD_COUNTER *c =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad.counters[cnt];
  CLEAR_STRUCT_PTR(c);

  c->count = 0u;
  c->max = UCPD_COUNTERS_MAX[cnt];
}

void UCPD_CNT_Increment(UCPD_PORT_Number port_number, UCPD_CNT cnt) {
  UCPD_COUNTER *c =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad.counters[cnt];

  // autoreload for messageid only
  c->count = c->count + 1u > c->max && cnt == UCPD_CNT_MESSAGEID ? 0u : c->count + 1u;
}

void UCPD_CNT_Reset(UCPD_PORT_Number port_number, UCPD_CNT cnt) {
  UCPD_COUNTER *c =
      &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad.counters[cnt];

  c->count = 0u;
}

UCPD_Count UCPD_CNT_Get(UCPD_PORT_Number port_number, UCPD_CNT cnt) {
  return UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad.counters[cnt].count;
}

UCPD_Bool UCPD_CNT_Compare(UCPD_PORT_Number port_number, UCPD_CNT cnt,
                           UCPD_Count value) {
  return UCPD_CTX_GetPortInstance(port_number)
                     ->pe_prl_cad.counters[cnt]
                     .count == value
             ? UCPD_TRUE
             : UCPD_FALSE;
}
