#ifndef UCPD_PWR_H
#define UCPD_PWR_H

#include "ucpd/ucpd_port_num.h"
#include "ucpd/ucpd_utils.h"

#define UCPD_PWR_VOLTAGE_LOWER_V_SAFE_0V 0
#define UCPD_PWR_VOLTAGE_UPPER_V_SAFE_5V 5000
#define UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V 800
#define UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V 4750

#define UCPD_PWR_ADC_LOW_THRESHOLD_RISE 167
#define UCPD_PWR_ADC_LOW_THRESHOLD_FALL 160

#define UCPD_PWR_ADC_HIGH_THRESHOLD_RISE 986
#define UCPD_PWR_ADC_HIGH_THRESHOLD_FALL 970

/* Power transition states */
typedef enum {
  UCPD_PWR_STATE_VBUS_ABSENT,
  UCPD_PWR_STATE_VBUS_TRANSIENT,
  UCPD_PWR_STATE_VBUS_PRESENT,
  UCPD_PWR_STATE_NONE
} UCPD_PWR_SM_State;

/* Power transition state machine */
typedef struct {
  UCPD_PWR_SM_State state;
  UCPD_Bool vbus_vS0_to_vS5; // there is not just v5 on the bus, but there was a
                             // transition from v0 to v5
  UCPD_Bool exited_v0; // it means we exited v0 to transient
} UCPD_PWR_SM;

void UCPD_PWR_SM_Init(UCPD_PWR_SM *sm);

void UCPD_PWR_SM_Delta(UCPD_PWR_SM *sm, UCPD_Voltage vbus_voltage);

/* Power management functions */
void UCPD_PWR_Init(UCPD_PORT_Number port_number);

UCPD_Voltage UCPD_PWR_GetVbusVoltage(UCPD_PWR_SM *sm);

// has side effect: sm.vbus_vS0_to_vS5 is set to false when called
UCPD_Bool UCPD_PWR_vS0_to_vS5(UCPD_PORT_Number port_number);

UCPD_Bool UCPD_PWR_IsVbusPresent(UCPD_PORT_Number port_number);

/* Power management hardware abstraction functions */
void UCPD_PWR_ConversionCallback();

#endif // UCPD_PWR_H
