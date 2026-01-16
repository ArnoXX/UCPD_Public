#include "ucpd_cnt.h"
#include "ucpd/ucpd_ctx.h"
#include <stdint.h>

void UCPD_CNT_Init(UCPD_PORT_Number port_number, UCPD_CNT cnt)
{
	UCPD_COUNTER* c = &UCPD_CTX_GetPortInstance(port_number)->counters[cnt];
	*c = (UCPD_COUNTER){0};

	c->count = 0u;
	c->max = UCPD_COUNTERS_MAX[cnt];
}

void UCPD_CNT_Increment(UCPD_PORT_Number port_number, UCPD_CNT cnt)
{
	UCPD_COUNTER* c = &UCPD_CTX_GetPortInstance(port_number)->counters[cnt];

	// autoreload for messageid only
	c->count = c->count + 1u > c->max && cnt == UCPD_CNT_MESSAGEID ? 0u : c->count + 1u;
}

void UCPD_CNT_Reset(UCPD_PORT_Number port_number, UCPD_CNT cnt)
{
	UCPD_COUNTER* c = &UCPD_CTX_GetPortInstance(port_number)->counters[cnt];

	c->count = 0u;
}

uint8_t UCPD_CNT_Get(UCPD_PORT_Number port_number, UCPD_CNT cnt)
{
	return UCPD_CTX_GetPortInstance(port_number)->counters[cnt].count;
}
