#ifndef UCPD_API_H
#define UCPD_API_H

#include "ucpd/ucpd_cnt.h"
#include "ucpd/ucpd_msg.h"
#include "ucpd/ucpd_policy.h"
#include "ucpd/ucpd_port_num.h"

typedef enum {
  UCPD_STATUS_SUCCESS,
  UCPD_STATUS_FAILURE,
  UCPD_STATUS_ERROR,
  UCPD_STATUS_DISABLED,
  UCPD_STATUS_UNITIALIZED,
  UCPD_STATUS_INITIALIZED,
  UCPD_STATUS_INVALID_PARAMETER,
  UCPD_STATUS_INVALID_PORT,
  UCPD_STATUS_BUSY,
  UCPD_STATUS_NONE
} UCPD_Status;

typedef struct {
  UCPD_Policy policy;
} UCPD_Config;

typedef void (*UCPD_Callback)(UCPD_Status);

/* UCPD port init function */
UCPD_Status UCPD_PORT_Init(UCPD_PORT_Number port_number);

UCPD_Status UCPD_InitConfig(UCPD_Config *config);

UCPD_Status UCPD_Init(UCPD_Config *config);
UCPD_Status UCPD_Deinit(void);


/* Blocking functions */
UCPD_Status UCPD_GetPDOs(UCPD_PORT_Number port_number,
                         UCPD_SRC_PDO **const src_pdos,
                         uint8_t *src_pdo_count);



UCPD_Status UCPD_EnterEPR(UCPD_PORT_Number port_number, uint8_t power_rating);
UCPD_Status UCPD_ExitEPR(UCPD_PORT_Number port_number);





/*
@param pdo_number - the PDO number to request
@param max_current - the maximum current to request in 10mA increments
@param operating_current - the operating current to request in 10mA increments
*/
UCPD_Status UCPD_RequestFixedSupply(UCPD_PORT_Number port_number,
                                    uint8_t pdo_number,
                                    uint16_t max_current_10mA,
                                    uint16_t operating_current_10mA,
                                    bool capability_mismatch);

/*
@param pdo_number - the PDO number to request
@param output_voltage_20mV - the output voltage to request in 20mV increments
@param current_limit_50mA - the current limit to request in 50mA increments
@param capability_mismatch - whether to request a capability mismatch
*/
UCPD_Status UCPD_RequestAPDO(UCPD_PORT_Number port_number,
                             uint8_t pdo_number,
                             uint16_t output_voltage_20mV,
                             uint16_t current_limit_50mA,
                             bool capability_mismatch);

UCPD_Status UCPD_RequestEPR(UCPD_PORT_Number port_number,
                            uint8_t pdo_number,
                            uint16_t max_current_10mA,
                            uint16_t operating_current_10mA,
                            bool capability_mismatch);

UCPD_Status UCPD_RequestAVS(UCPD_PORT_Number port_number,
                            uint8_t pdo_number,
                            uint16_t out_voltage_100mV,
                            uint16_t current_limit_50mA,
                            bool capability_mismatch);


/* Async functions */
/*
@param pdo_number - the PDO number to request
@param max_current - the maximum current to request in 10mA increments
@param operating_current - the operating current to request in 10mA increments
@param callback - the callback to call when the request is complete
*/
UCPD_Status UCPD_RequestFixedSupplyAsync(UCPD_PORT_Number port_number,
                                         uint8_t pdo_number,
                                         uint16_t max_current_10mA,
                                         uint16_t operating_current_10mA,
                                         bool capability_mismatch,
                                         UCPD_Callback callback);

/*
@param pdo_number - the PDO number to request
@param output_voltage_20mV - the output voltage to request in 20mV increments
@param current_limit_50mA - the current limit to request in 50mA increments
@param capability_mismatch - whether to request a capability mismatch
@param callback - the callback to call when the request is complete
*/
UCPD_Status UCPD_RequestAPDOAsync(UCPD_PORT_Number port_number,
                                  uint8_t pdo_number,
                                  uint16_t output_voltage_20mV,
                                  uint16_t current_limit_50mA,
                                  bool capability_mismatch,
                                  UCPD_Callback callback);

UCPD_Status UCPD_RequestEPRAsync(UCPD_PORT_Number port_number,
                                 uint8_t pdo_number,
                                 uint16_t max_current_10mA,
                                 uint16_t operating_current_10mA,
                                 bool capability_mismatch,
                                 UCPD_Callback callback);

UCPD_Status UCPD_RequestAVSAsync(UCPD_PORT_Number port_number,
                                 uint8_t pdo_number,
                                 uint16_t out_voltage_100mV,
                                 uint16_t current_limit_50mA,
                                 bool capability_mismatch,
                                 UCPD_Callback callback);


UCPD_Status UCPD_EnterEPRAsync(UCPD_PORT_Number port_number, uint8_t power_rating, UCPD_Callback callback);
UCPD_Status UCPD_ExitEPRAsync(UCPD_PORT_Number port_number, UCPD_Callback callback);

void UCPD_POLICY_SelectFixedSupply(UCPD_PORT_Number port_number,
                                    uint8_t pdo_number,
                                    uint16_t max_current_10mA,
                                    uint16_t operating_current_10mA,
                                    bool capability_mismatch);

void UCPD_POLICY_SelectAPDO(UCPD_PORT_Number port_number,
                            uint8_t pdo_number,
                            uint16_t output_voltage_20mV,
                            uint16_t current_limit_50mA,
                            bool capability_mismatch);

void UCPD_POLICY_SelectAVS(UCPD_PORT_Number port_number,
                           uint8_t pdo_number,
                           uint16_t out_voltage_100mV,
                           uint16_t current_limit_50mA,
                           bool capability_mismatch);

void UCPD_POLICY_SelectEPR(UCPD_PORT_Number port_number,
                           uint8_t pdo_number,
                           uint16_t max_current_10mA,
                           uint16_t operating_current_10mA,
                           bool capability_mismatch);







#endif // UCPD_API_H
