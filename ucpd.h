#ifndef UCPD_API_H
#define UCPD_API_H

#include "ucpd/ucpd_msg.h"
#include "ucpd/ucpd_policy.h"
#include "ucpd/ucpd_port_num.h"
#include <stdbool.h>

typedef enum
{
	UCPD_STATUS_NONE,
	UCPD_STATUS_SUCCESS,
	UCPD_STATUS_FAILURE,
	UCPD_STATUS_ERROR,
	UCPD_STATUS_DISABLED,
	UCPD_STATUS_UNITIALIZED,
	UCPD_STATUS_INITIALIZED,
	UCPD_STATUS_INVALID_PARAMETER,
	UCPD_STATUS_INVALID_PORT,
	UCPD_STATUS_DISCONNECTED,
	UCPD_STATUS_ATTACHED,
	UCPD_STATUS_UNATTACHED,
	UCPD_STATUS_BUSY,
	UCPD_STATUS_CANCELLED
} UCPD_Status;

typedef void (*UCPD_Callback)(UCPD_PORT_Number, UCPD_Status);

typedef struct
{
	UCPD_Policy policy;
	void* user_data;
	UCPD_Callback port_callback;
} UCPD_Config;

/* Utility functions */
bool UCPD_IsEPRMode(UCPD_PORT_Number port_number);
bool UCPD_IsPPSMode(UCPD_PORT_Number port_number);
bool UCPD_IsEPRCapable(UCPD_PORT_Number port_number);

uint32_t UCPD_EPREnterFailedReason(UCPD_PORT_Number port_number);

void* UCPD_GetUserData(void);

/* UCPD lifecycle function */

UCPD_Status UCPD_InitConfig(UCPD_Config* config);

UCPD_Status UCPD_Init(UCPD_Config* config);
UCPD_Status UCPD_Deinit(void);

/* API calls */
UCPD_Status UCPD_GetPDOs(UCPD_PORT_Number port_number, const UCPD_SRC_PDO** src_pdos, uint8_t* src_pdo_count);

UCPD_Status UCPD_TriggerPolicy(UCPD_PORT_Number port_number);
UCPD_Status UCPD_TriggerPolicyAsync(UCPD_PORT_Number port_number, UCPD_Callback callback);

UCPD_Status UCPD_EnterEPR(UCPD_PORT_Number port_number, uint8_t power_rating);
UCPD_Status UCPD_ExitEPR(UCPD_PORT_Number port_number);

UCPD_Status UCPD_EnterEPRAsync(UCPD_PORT_Number port_number, uint8_t power_rating, UCPD_Callback callback);
UCPD_Status UCPD_ExitEPRAsync(UCPD_PORT_Number port_number, UCPD_Callback callback);

UCPD_Status UCPD_HardReset(UCPD_PORT_Number port_number);
UCPD_Status UCPD_HardResetAsync(UCPD_PORT_Number port_number, UCPD_Callback callback);

/* Policy helper functions */

bool UCPD_POLICY_SelectFixedSupply(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t max_current_10mA,
                                   uint16_t operating_current_10mA, bool capability_mismatch);

bool UCPD_POLICY_SelectAPDO(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t output_voltage_20mV,
                            uint16_t current_limit_50mA, bool capability_mismatch);

bool UCPD_POLICY_SelectAVS(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t out_voltage_100mV,
                           uint16_t current_limit_50mA, bool capability_mismatch);

bool UCPD_POLICY_SelectSPR_AVS(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t out_voltage_100mV,
                               uint16_t current_limit_50mA, bool capability_mismatch);

bool UCPD_POLICY_SelectEPR(UCPD_PORT_Number port_number, uint8_t pdo_number, uint16_t max_current_10mA,
                           uint16_t operating_current_10mA, bool capability_mismatch);

#endif // UCPD_API_H
