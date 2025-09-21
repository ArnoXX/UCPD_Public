#include "device/ucpd_pwr.h"
#include "device/trace_utils.h"
#include "stm32g0xx_ll_adc.h"
#include "ucpd/ucpd_ctx.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd/ucpd_port.h"
#include "ucpd/ucpd_utils.h"
#include "ucpd_pwr.h"


static inline void AWD_Disable() {
  LL_ADC_ClearFlag_AWD1(ADC1);
  LL_ADC_DisableIT_AWD1(ADC1);
}

static inline void AWD_EnableHigh() {
  LL_ADC_SetAnalogWDMonitChannels(ADC1, LL_ADC_AWD1, LL_ADC_AWD_CHANNEL_9_REG);
  LL_ADC_SetAnalogWDThresholds(ADC1, LL_ADC_AWD1, LL_ADC_AWD_THRESHOLD_HIGH,
                               4095); // High
  LL_ADC_SetAnalogWDThresholds(ADC1, LL_ADC_AWD1, LL_ADC_AWD_THRESHOLD_LOW,
                               UCPD_PWR_ADC_HIGH_THRESHOLD_RISE); // Low
  LL_ADC_ClearFlag_AWD1(ADC1);
  LL_ADC_EnableIT_AWD1(ADC1);
}

static inline void AWD_EnableLow() {
  LL_ADC_SetAnalogWDMonitChannels(ADC1, LL_ADC_AWD1, LL_ADC_AWD_CHANNEL_9_REG);
  LL_ADC_SetAnalogWDThresholds(ADC1, LL_ADC_AWD1, LL_ADC_AWD_THRESHOLD_HIGH,
                               UCPD_PWR_ADC_LOW_THRESHOLD_RISE); // High
  LL_ADC_SetAnalogWDThresholds(ADC1, LL_ADC_AWD1, LL_ADC_AWD_THRESHOLD_LOW,
                               0); // Low

  LL_ADC_ClearFlag_AWD1(ADC1);
  LL_ADC_EnableIT_AWD1(ADC1);
}

void UCPD_PWR_SM_Init(UCPD_PWR_SM *sm) {

  AWD_Disable();

  sm->state = UCPD_PWR_STATE_NONE;
  sm->exited_v0 = false;
  sm->vbus_vS0_to_vS5 = false;

  if (LL_ADC_IsEnabled(ADC1) == 0) {
    LL_ADC_EnableInternalRegulator(ADC1);
    for (volatile uint32_t i = 0; i < (SystemCoreClock / 1000000) * 30; ++i) {
      __NOP();
    }

    LL_ADC_StartCalibration(ADC1);
    while (LL_ADC_IsCalibrationOnGoing(ADC1)) {
    }

    LL_ADC_ClearFlag_ADRDY(ADC1);
    LL_ADC_Enable(ADC1);
    while (!LL_ADC_IsActiveFlag_ADRDY(ADC1)) {
    }
  }

  if (LL_ADC_REG_IsConversionOngoing(ADC1) == 0) {
    LL_ADC_REG_StartConversion(ADC1);
  }

  uint16_t voltage = UCPD_PWR_GetVbusVoltage(sm);

  if (voltage > UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V) {
    TRACE_INFO("VBUS Initially Present\n\r");
    sm->state = UCPD_PWR_STATE_VBUS_PRESENT;
  } else if (voltage < UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V) {
    TRACE_INFO("VBUS Initially Absent\n\r");
    sm->state = UCPD_PWR_STATE_VBUS_ABSENT;
  } else {
    TRACE_INFO("VBUS Initially Transient\n\r");
    sm->state = UCPD_PWR_STATE_VBUS_TRANSIENT;
    sm->vbus_vS0_to_vS5 = true;
  }

  AWD_EnableLow();

  LL_ADC_ClearFlag_AWD1(ADC1);
  LL_ADC_EnableIT_AWD1(ADC1);

  TRACE_INFO("PWR Initialized\n\r");
}


void UCPD_PWR_SM_Delta(UCPD_PWR_SM *sm, uint16_t vbus_voltage) {
  UCPD_PORT_INSTANCE *port = UCPD_CTX_GetPortInstance(0);

  switch (sm->state) {
  case UCPD_PWR_STATE_VBUS_ABSENT: {
    if (vbus_voltage <= UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V)
      break;
    else if (vbus_voltage < UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V) {

      sm->state = UCPD_PWR_STATE_VBUS_TRANSIENT;
      // when going to transient we mark that we only exited v0, we did not go
      // to v5 yet
      sm->exited_v0 = true;
      sm->vbus_vS0_to_vS5 = false;
      AWD_EnableLow();
    } else if (vbus_voltage >= UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V) {
      TRACE_INFO("PWR VBUS Detected\n\r");
      sm->state = UCPD_PWR_STATE_VBUS_PRESENT;
      sm->exited_v0 = false;
      sm->vbus_vS0_to_vS5 = true;
      port->pe_prl_cad.pwr_event = CAD_EVENT_VBUS_DETECTED;

      UCPD_PE_PRL_Event_Emit(PD_EVENT_SOURCE_PWR);
      AWD_EnableHigh();
    }
    break;
  }
  case UCPD_PWR_STATE_VBUS_TRANSIENT: {
    if (vbus_voltage >= UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V) {
      TRACE_INFO("PWR VBUS Detected\n\r");
      sm->state = UCPD_PWR_STATE_VBUS_PRESENT;
      // notify only if we previously were in v0
      if (sm->exited_v0 ) {
        sm->exited_v0 = false;
        sm->vbus_vS0_to_vS5 = true;
        // notify vbus present
        port->pe_prl_cad.pwr_event = CAD_EVENT_VBUS_DETECTED;

        UCPD_PE_PRL_Event_Emit(PD_EVENT_SOURCE_PWR);
      }

      AWD_EnableHigh();
    } else if (vbus_voltage <= UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V) {
      TRACE_INFO("PWR VBUS Removed\n\r");
      sm->state = UCPD_PWR_STATE_VBUS_ABSENT;
      // notify only if we previously were in v5
      if (sm->exited_v0 == false) {
        sm->exited_v0 = false;
        sm->vbus_vS0_to_vS5 = false;
        // notify vbus absent
        port->pe_prl_cad.pwr_event = CAD_EVENT_VBUS_REMOVED;

        UCPD_PE_PRL_Event_Emit(PD_EVENT_SOURCE_PWR);
      }

      AWD_EnableLow();

    } else {
      if(sm->exited_v0 ) {
        AWD_EnableLow();
      }
      else {
        AWD_EnableHigh();
      }

    }// else - still transient
    break;
  }
  case UCPD_PWR_STATE_VBUS_PRESENT: {
    if (vbus_voltage >= UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V)
      break;
    else if (vbus_voltage > UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V) {
      sm->state = UCPD_PWR_STATE_VBUS_TRANSIENT;
      AWD_EnableHigh();
    } else if (vbus_voltage <= UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V) {
      TRACE_INFO("PWR VBUS Removed\n\r");
      sm->state = UCPD_PWR_STATE_VBUS_ABSENT;
      sm->vbus_vS0_to_vS5 = false;
      port->pe_prl_cad.pwr_event = CAD_EVENT_VBUS_REMOVED;

      UCPD_PE_PRL_Event_Emit(PD_EVENT_SOURCE_PWR);

      AWD_EnableLow();
    }

    break;
  }
  default: {
    TRACE_ERROR("PWR SM Invalid state\n\r");
    break;
  }
  }
}

void UCPD_PWR_ConversionCallback() {

  AWD_Disable();

  UCPD_PWR_SM *sm = &(UCPD_CTX_GetPortInstance(0)->pwr_sm);

  uint16_t vbus_voltage = UCPD_PWR_GetVbusVoltage(sm);

  UCPD_PWR_SM_Delta(sm, vbus_voltage);
}

bool UCPD_PWR_vS0_to_vS5(UCPD_PORT_Number port_number) {
  UCPD_PWR_SM *sm = &(UCPD_CTX_GetPortInstance(port_number)->pwr_sm);

  bool ret = sm->vbus_vS0_to_vS5;
  sm->vbus_vS0_to_vS5 = false;
  return ret;
}

uint16_t UCPD_PWR_GetVbusVoltage(UCPD_PWR_SM *sm) {
  uint32_t adc_value = LL_ADC_REG_ReadConversionData12(ADC1);
  if (adc_value > 4095u) {
    TRACE_ERROR("ADC value out of range\n\r");
    while (1)
      ;
  }

  uint32_t millivolts = (adc_value * 19717UL) / 4095UL;

  if (millivolts > 20000UL) {
    millivolts = 20000UL;
  }

  return (uint16_t)millivolts;
}

bool UCPD_PWR_IsVbusPresent(UCPD_PORT_Number port_number) {
  UCPD_PWR_SM *sm = &(UCPD_CTX_GetPortInstance(port_number)->pwr_sm);

  return sm->state == UCPD_PWR_STATE_VBUS_PRESENT;
}
