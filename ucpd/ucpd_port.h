#ifndef UCPD_PORT_H
#define UCPD_PORT_H

#include "ucpd/ucpd_cnt.h"
#include "ucpd/ucpd_pe_prl.h"
#include "ucpd/ucpd_tim.h"


/* UCPD port count */
#define UCPD_PORT_COUNT 1

typedef struct
{
	UCPD_PE_PRL_CAD_Module pe_prl_cad;

	// change to multiple when using channels
	// Have to admit, it got me. Unchecked indecies in C are no joke, tried to index with LL_TIM_CHANNEL_CH1, etc.
	// but it was not working.
	// Guess what, those constants are shifted 1u to the left, so powers of 2
	// was overwriting the DMA channel pointer in the port instance handle below
	UCPD_TIM ucpd_tim_at_channel[TIMER_CHANNEL_COUNT];

	UCPD_COUNTER counters[UCPD_CNT_NUMBER];

	bool initialized;
} UCPD_PORT_INSTANCE;


UCPD_Status UCPD_PORT_Init(UCPD_PORT_Number port_number);

UCPD_Status UCPD_PORT_Deinit(UCPD_PORT_Number port_number);

#endif // UCPD_PORT_H