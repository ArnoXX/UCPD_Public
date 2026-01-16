#ifndef USPD_TIM_H
#define USPD_TIM_H

#include "ucpd/ucpd_pe_prl.h"
#include "ucpd/ucpd_port_num.h"
#include <stdint.h>

/* Timer parameters defined in ms */
#define UCPD_TBIST_CONT_MODE 60u
#define UCPD_TCHUNK_SENDER_REQUEST 30u
#define UCPD_TCHUNK_SENDER_RESPONSE 30u
#define UCPD_THARD_RESET_COMPLETE 5u
#define UCPD_TPPS_REQUEST 10000u
#define UCPD_TPS_TRANSITION 500u
#define UCPD_TRECIEIVE 1u
#define UCPD_TSENDER_RESPONSE 33u
#define UCPD_TSINK_REQUEST 5u
#define UCPD_TTYPEC_SINK_WAIT_CAP 620u
#define UCPD_TEPR_ENTER 500u
#define UCPD_TEPR_KEEP_ALIVE 375u
#define UCPD_TPS_TRANSITION_EPR 925u
#define UCPD_TNONE 0u

#define UCPD_TCCDDEBOUNCE 200u
#define UCPD_TPDDEBOUNCE 20u
#define UCPD_TERROR_RECOVERY 26u

#define TIMER_CHANNEL_COUNT 4u

#define UCPD_TIMER_CHANNEL_1 0u
#define UCPD_TIMER_CHANNEL_2 1u
#define UCPD_TIMER_CHANNEL_3 2u
#define UCPD_TIMER_CHANNEL_4 3u
#define UCPD_TIMER_INVALID_CHANNEL 0xFFu

#define UCPD_GET_HW_CHANNEL(timer_channel) (1U << (timer_channel * 4))

typedef uint16_t UCPD_Time;

/* Timers enum typedef */
typedef enum
{
	UCPD_TIM_NONE = 0u,
	UCPD_TIM_BIST_CONT_MODE,
	UCPD_TIM_CHUNK_SENDER_REQUEST,
	UCPD_TIM_CHUNK_SENDER_RESPONSE,
	UCPD_TIM_CRC_RECEIVE,
	UCPD_TIM_HARD_RESET_COMPLETE,
	UCPD_TIM_PS_TRANSITION,
	UCPD_TIM_SENDER_RESPONSE,
	UCPD_TIM_SINK_PPS_PERIODIC,
	UCPD_TIM_SINK_REQUEST,
	UCPD_TIM_SINK_WAIT_CAP,
	UCPD_TIM_CAD_CCDEB,
	UCPD_TIM_CAD_PDDEB,
	UCPD_TIM_CAD_ERROR_REC,
	UCPD_TIM_SINK_EPR_ENTER,
	UCPD_TIM_SINK_EPR_KEEP_ALIVE,
	UCPD_TIM_SINK_EPR_TRANSITION,
	UCPD_TIM_NUMBER, // must be the last one
} UCPD_TIM;

/* Timer to parameter mapping */
/* Indexed by UCPD_TIM */
static const UCPD_Time UCPD_TIMERS_PERIODS[UCPD_TIM_NUMBER] = {UCPD_TNONE,
                                                               UCPD_TBIST_CONT_MODE,
                                                               UCPD_TCHUNK_SENDER_REQUEST,
                                                               UCPD_TCHUNK_SENDER_RESPONSE,
                                                               UCPD_TRECIEIVE,
                                                               UCPD_THARD_RESET_COMPLETE,
                                                               UCPD_TPS_TRANSITION,
                                                               UCPD_TSENDER_RESPONSE,
                                                               UCPD_TPPS_REQUEST,
                                                               UCPD_TSINK_REQUEST,
                                                               UCPD_TTYPEC_SINK_WAIT_CAP,
                                                               UCPD_TCCDDEBOUNCE,
                                                               UCPD_TPDDEBOUNCE,
                                                               UCPD_TERROR_RECOVERY,
                                                               UCPD_TEPR_ENTER,
                                                               UCPD_TEPR_KEEP_ALIVE,
                                                               UCPD_TPS_TRANSITION_EPR};

static const uint8_t UCPD_TIMER_CHANNEL[UCPD_TIM_NUMBER] = {
    UCPD_TIMER_INVALID_CHANNEL, UCPD_TIMER_CHANNEL_1, UCPD_TIMER_CHANNEL_1, UCPD_TIMER_CHANNEL_2, UCPD_TIMER_CHANNEL_1,
    UCPD_TIMER_CHANNEL_1,       UCPD_TIMER_CHANNEL_1, UCPD_TIMER_CHANNEL_1, UCPD_TIMER_CHANNEL_2, UCPD_TIMER_CHANNEL_1,
    UCPD_TIMER_CHANNEL_1,       UCPD_TIMER_CHANNEL_4, UCPD_TIMER_CHANNEL_4, UCPD_TIMER_CHANNEL_1, UCPD_TIMER_CHANNEL_2,
    UCPD_TIMER_CHANNEL_2,       UCPD_TIMER_CHANNEL_1};

/* Aggregate event typedef */
typedef enum
{
	UCPD_EVENT_DST_PE_PRL,
	UCPD_EVENT_DST_CAD,
} UCPD_EVENT_DST;

/* Timer to event mapping */
/* Indexed by UCPD_TIM. Fill with appropriate UCPD_EVENT values as needed. */
static const UCPD_PE_PRL_CAD_Event UCPD_TIMERS_EVENTS[UCPD_TIM_NUMBER] = {
    /* UCPD_TIM_NONE */ PE_PRL_CAD_EVENT_NONE,
    /* UCPD_TIM_BIST_CONT_MODE */ PE_PRL_EVENT_BIST_CONT_MODE_TIMEROUT,
    /* UCPD_TIM_CHUNK_SENDER_REQUEST */ PE_PRL_EVENT_SENDER_REQ_TIMER_TIMEOUT,
    /* UCPD_TIM_CHUNK_SENDER_RESPONSE */ PE_PRL_EVENT_CHUNK_RES_TIMER_TIMEOUT,
    /* UCPD_TIM_CRC_RECEIVE */ PE_PRL_EVENT_CRC_RX_TIMEOUT,
    /* UCPD_TIM_HARD_RESET_COMPLETE */ PE_PRL_EVENT_HARD_RESET_TIMEOUT,
    /* UCPD_TIM_PS_TRANSITION */ PE_PRL_EVENT_PST_TRANS_TIMEOUT,
    /* UCPD_TIM_SENDER_RESPONSE */ PE_PRL_EVENT_SRT_EXPIRED,
    /* UCPD_TIM_SINK_PPS_PERIODIC */ PE_PRL_EVENT_SNK_PPS_PER_TIMEOUT,
    /* UCPD_TIM_SINK_REQUEST */ PE_PRL_EVENT_SRT_EXPIRED,
    /* UCPD_TIM_SINK_WAIT_CAP */ PE_PRL_EVENT_SRT_EXPIRED, // TODO: check if
                                                           // this is correct
    /* UCPD_TIM_CAD_CCDEB */ CAD_EVENT_TCDEBOUNCE_TIMEOUT,
    /* UCPD_TIM_CAD_PDDEB */ CAD_EVENT_TPDEBOUNCE_TIMEOUT,
    /* UCPD_TIM_CAD_ERROR_REC */ CAD_EVENT_TERROR_RECOVERY,
    /* UCPD_TIM_SINK_EPR_ENTER */ PE_PRL_EVENT_EPR_ENTER_TIMEOUT,
    /* UCPD_TIM_SINK_EPR_KEEP_ALIVE */ PE_PRL_EVENT_EPR_KEEP_ALIVE_TIMEOUT,
    /* UCPD_TIM_SINK_EPR_TRANSITION */ PE_PRL_EVENT_PST_TRANS_TIMEOUT,
};

/* Initialize timer */
void UCPD_TIM_Init(UCPD_PORT_Number port_number);

void UCPD_TIM_Deinit(UCPD_PORT_Number port_number);

/* Reset timer function - resets the timer, does not stop it */
void UCPD_TIM_Reset(UCPD_PORT_Number port_number, UCPD_TIM timer);

/* Start timer function */
void UCPD_TIM_Start(UCPD_PORT_Number port_number, UCPD_TIM timer);

/* Stop timer function - stops the timer, does not reset it */
void UCPD_TIM_Stop(UCPD_PORT_Number port_number, UCPD_TIM timer);

/* Timer expiration function - to be called when timer expiration is registered
 */
void UCPD_TIM_Expired(UCPD_PORT_Number port_number, uint32_t timer_channel);

#endif // USPD_TIM_H
