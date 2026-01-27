#include "ucpd.h"
#include "ucpd/ucpd_msg.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

volatile bool block = true;

void EPRExitCallback(UCPD_PORT_Number port_number, UCPD_Status status) {}

void EPREnterCallback(UCPD_PORT_Number port_number, UCPD_Status status) {
  switch (status) {
  case UCPD_STATUS_SUCCESS: {
    break;
  }

  case UCPD_STATUS_FAILURE: {
    (void)UCPD_EPREnterFailedReason(port_number);

    break;
  }

  default: {
    break;
  }
  }
}

void SimplePolicy(UCPD_PORT_Number port_number, UCPD_Policy_Event event) {
  switch (event) {
  case UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES: {
    UCPD_POLICY_SelectFixedSupply(port_number, 1u, 100u, 100u, false);
    break;
  }

  default:
    break;
  }
}

void MaxSPRPolicy(UCPD_PORT_Number port_number, UCPD_Policy_Event event) {
  const UCPD_SRC_PDO *src_pdo = NULL;
  uint8_t pdo_count = 0u;
  UCPD_GetPDOs(port_number, &src_pdo, &pdo_count);

  switch (event) {
  case UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES: {
    uint8_t max_spr = 0u;
    while (src_pdo[max_spr].pdo_type == UCPD_PDO_TYPE_FIXED_SUPPLY &&
           max_spr < pdo_count)
      max_spr++;

    UCPD_POLICY_SelectFixedSupply(port_number, max_spr, 100u, 100u, false);

    break;
  }

  default:
    break;
  }
}

void PPSPolicy(UCPD_PORT_Number port_number, UCPD_Policy_Event event) {
  const UCPD_SRC_PDO *src_pdo = NULL;
  uint8_t pdo_count = 0u;
  UCPD_GetPDOs(port_number, &src_pdo, &pdo_count);

  switch (event) {
  case UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES: {
    for (uint8_t i = 0u; i < pdo_count; i++) {
      if (src_pdo[i].pdo_type == UCPD_PDO_TYPE_APDO) {
        UCPD_POLICY_SelectAPDO(port_number, i + 1u,
                               src_pdo[i].APDO.PPS.min_voltage100mV * 5u,
                               src_pdo[i].APDO.PPS.max_current50mA, false);
        return;
      }
    }
    break;
  }

  default:
    break;
  }
}

void PPSImmediatePolicyCallback(UCPD_PORT_Number port_number,
                                UCPD_Status status) {}

void PPSImmediatePolicy(UCPD_PORT_Number port_number, UCPD_Policy_Event event) {
  const UCPD_SRC_PDO *src_pdo = NULL;
  uint8_t pdo_count = 0u;
  UCPD_GetPDOs(port_number, &src_pdo, &pdo_count);

  static double time = 0;

  switch (event) {
  case UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES: {
    for (uint8_t i = 0u; i < pdo_count; i++) {
      if (src_pdo[i].pdo_type == UCPD_PDO_TYPE_APDO &&
          src_pdo[i].APDO.apdo_type == UCPD_APDO_TYPE_PPS) {
        uint16_t min = src_pdo[i].APDO.PPS.min_voltage100mV * 5u;
        uint16_t max = src_pdo[i].APDO.PPS.max_voltage100mV * 5u;
        uint16_t a = (uint16_t)((sin(time) + 1) / 2 * (max - min) + min);
        time += 0.05;
        UCPD_POLICY_SelectAPDO(port_number, i + 1u, a,
                               src_pdo[i].APDO.PPS.max_current50mA, false);

        UCPD_TriggerPolicyAsync(port_number, PPSImmediatePolicyCallback);
        return;
      }
    }
    break;
  }

  default:
    break;
  }
}

void EPRPolicy(UCPD_PORT_Number port_number, UCPD_Policy_Event event) {
  switch (event) {
  case UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES: {
    if (UCPD_IsEPRMode(port_number)) {
      UCPD_POLICY_SelectEPR(port_number, 1u, 300u, 150, false);
    } else {
      UCPD_POLICY_SelectFixedSupply(port_number, 1u, 100u, 100u, false);
      if (UCPD_IsEPRCapable(port_number))
        (void)UCPD_EnterEPRAsync(0, 120, EPREnterCallback);
    }

    break;
  }

  default:
    break;
  }
}

void EPRPingPongPolicy(UCPD_PORT_Number port_number, UCPD_Policy_Event event) {

  switch (event) {
  case UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES: {
    if (UCPD_IsEPRMode(port_number)) {
      UCPD_POLICY_SelectEPR(port_number, 1u, 300u, 150, false);
      (void)UCPD_ExitEPRAsync(port_number, EPRExitCallback);
    } else {
      UCPD_POLICY_SelectFixedSupply(port_number, 1u, 100u, 100u, false);
      if (UCPD_IsEPRCapable(port_number))
        (void)UCPD_EnterEPRAsync(0, 120, EPREnterCallback);
    }

    break;
  }

  default:
    break;
  }
}

void HardResetPolicy(UCPD_PORT_Number port_number, UCPD_Policy_Event event) {
  switch (event) {
  case UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES: {
    UCPD_POLICY_SelectFixedSupply(port_number, 1u, 100u, 100u, false);
    UCPD_HardResetAsync(port_number, NULL);
    break;
  }

  default:
    break;
  }
}

void MultipleInflightCallsPolicy(UCPD_PORT_Number port_number,
                                 UCPD_Policy_Event event) {
  switch (event) {
  case UCPD_POLICY_EVENT_EVALUATE_CAPABILITIES: {
    UCPD_POLICY_SelectFixedSupply(port_number, 1u, 100u, 100u, false);
    UCPD_TriggerPolicyAsync(port_number, NULL);
    UCPD_EnterEPRAsync(port_number, 120, NULL); // will do nothing
    UCPD_HardResetAsync(port_number, NULL);     // will do nothing
    break;
  }

  default:
    break;
  }
}

void PortCallback(UCPD_PORT_Number port_number, UCPD_Status status) {}

void RunExamples(void) {

  UCPD_Config config;
  UCPD_InitConfig(&config);
  config.policy = MaxSPRPolicy;
  config.port_callback = PortCallback;

  UCPD_Init(&config);

  while (block) {
  }
  block = true;

  UCPD_Deinit();
}
