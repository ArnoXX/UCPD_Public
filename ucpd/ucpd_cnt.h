#ifndef UCPD_CNT_H
#define UCPD_CNT_H

#include "ucpd/ucpd_port_num.h"
#include <stdbool.h>
#include <stdint.h>

/* Counter values defines */
#define UCPD_CNT_N_BUSY 5u
#define UCPD_CNT_N_CAPS 50u
#define UCPD_CNT_N_DISCOVER_IDENTITY 20u
#define UCPD_CNT_N_HARD_RESET 2u
#define UCPD_CNT_N_MESSAGEID 7u
#define UCPD_CNT_N_RETRY 2u

/* Counters enum typedef */
typedef enum
{
	UCPD_CNT_BUSY,
	UCPD_CNT_CAPS,
	UCPD_CNT_DISCOVER_IDENTITY,
	UCPD_CNT_HARD_RESET,
	UCPD_CNT_MESSAGEID,
	UCPD_CNT_RETRY,
	UCPD_CNT_NUMBER
} UCPD_CNT;

/* Counter to parameter mapping */
/* Indexed by UCPD_CNT */
static const uint8_t UCPD_COUNTERS_MAX[UCPD_CNT_NUMBER] = {
    UCPD_CNT_N_BUSY,       UCPD_CNT_N_CAPS,      UCPD_CNT_N_DISCOVER_IDENTITY,
    UCPD_CNT_N_HARD_RESET, UCPD_CNT_N_MESSAGEID, UCPD_CNT_N_RETRY};

/* Counter struct typedef */
typedef struct
{
	uint8_t count;
	uint8_t max : 7u;
} UCPD_COUNTER;

/* Counter initialization function */
void UCPD_CNT_Init(UCPD_PORT_Number port_number, UCPD_CNT cnt);

/* Counter increment function */
void UCPD_CNT_Increment(UCPD_PORT_Number port_number, UCPD_CNT cnt);

/* Counter reset function */
void UCPD_CNT_Reset(UCPD_PORT_Number port_number, UCPD_CNT cnt);

/* Counter get value function */
uint8_t UCPD_CNT_Get(UCPD_PORT_Number port_number, UCPD_CNT cnt);

/* Counter compare function */
bool UCPD_CNT_Compare(UCPD_PORT_Number port_number, UCPD_CNT cnt, uint8_t value);

#endif // UCPD_CNT_H
