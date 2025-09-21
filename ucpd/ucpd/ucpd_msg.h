#ifndef UCPD_MSG_H
#define UCPD_MSG_H

#include <stdint.h>

#include "cmsis_gcc.h"

/* Message header values defines */

/* Message size defines */
#define UCPD_MSG_HEADER_SIZE 2u
#define UCPD_MAX_MSG_LEN 30u // 2b for header + 7 * 4b for DOs
#define UCPD_MSG_EXT_HEADER_SIZE 2u
#define UCPD_MAX_EXT_MSG_LEN 260u
#define UCPD_MAX_EXT_MSG_CHUNK_LEN 26u
#define UCPD_MAX_EXT_MSG_LEG_LEN 26u
#define UCPD_TOTAL_MSG_LEN                                                     \
  (UCPD_MSG_HEADER_SIZE + UCPD_MSG_EXT_HEADER_SIZE + UCPD_MAX_EXT_MSG_LEN)
#define UCPD_PDO_SIZE 4u

#define UCPD_MAX_PDO_NUM 7u
#define UCPD_MAX_EPR_PDO_NUM 4u

#define UCPD_EPR_MESSAGE_LEN                                                   \
  (UCPD_PDO_SIZE * (UCPD_MAX_EPR_PDO_NUM + UCPD_MAX_PDO_NUM) +                 \
   UCPD_MSG_EXT_HEADER_SIZE + UCPD_MSG_HEADER_SIZE)

/* Message ID defines*/
/* Control message ID defines */
#define UCPD_GOODCRC_MSG_ID 1u
#define UCPD_ACCEPT_MSG_ID 3u
#define UCPD_REJECT_MSG_ID 4u
#define UCPD_PING_MSG_ID 5u
#define UCPD_PS_RDY_MSG_ID 6u
#define UCPD_GET_SOURCE_CAP_MSG_ID 7u
#define UCPD_GET_SINK_CAP_MSG_ID 8u
#define UCPD_WAIT_MSG_ID 12u
#define UCPD_SOFT_RESET_MSG_ID 13u
#define UCPD_NOT_SUPPORTED_MSG_ID 16u
#define UCPD_GET_SOURCE_CAP_EXT_MSG_ID 17u
#define UCPD_GET_STATUS_MSG_ID 18u
#define UCPD_GET_PPS_STATUS_MSG_ID 20u
#define UCPD_GET_SINK_CAP_EXT_MSG_ID 22u
#define UCPD_GET_SOURCE_INFO_MSG_ID 23u
#define UCPD_GET_REVISION_MSG_ID 24u

typedef uint8_t UCPD_ControlMessageID;

/* Data message ID defines */
#define UCPD_SOURCE_CAPABILITIES_MSG_ID 1u
#define UCPD_REQUEST_MSG_ID 2u
#define UCPD_BIST_MSG_ID 3u
#define UCPD_SINK_CAPABILITIES_MSG_ID 4u
#define UCPD_ALERT_MSG_ID 6u
#define UCPD_EPR_REQUEST_MSG_ID 9u
#define UCPD_EPR_MODE_MSG_ID 10u
#define UCPD_SOURCE_INFO_MSG_ID 11u
#define UCPD_REVISION_MSG_ID 12u

typedef uint8_t UCPD_DataMessageID;

/* Extended control message ID defines */
#define UCPD_EPR_GET_SOURCE_CAP_EXT_MSG_ID 1u
#define UCPD_EPR_GET_SINK_CAP_EXT_MSG_ID 2u
#define UCPD_EPR_KEEP_ALIVE_EXT_MSG_ID 3u
#define UCPD_EPR_KEEP_ALIVE_ACK_EXT_MSG_ID 4u

typedef uint8_t UCPD_ExtendedControlMessageID;

/* Extended message ID defines */
#define UCPD_SOURCE_CAPABILITIES_EXT_MSG_ID 1u
#define UCPD_STATUS_MSG_ID 2u
#define UCPD_PPS_STATUS_MSG_ID 12u
#define UCPD_SINK_CAPABILITIES_EXT_MSG_ID 15u
#define UCPD_EXTENDED_CONTROL_MSG_ID 16u
#define UCPD_EPR_SOURCE_CAPABILITIES_MSG_ID 17u

typedef uint8_t UCPD_ExtendedMessageID;

typedef enum {
  UCPD_MSG_CONTROL,
  UCPD_MSG_DATA,
  UCPD_MSG_EXTENDED,
  UCPD_MSG_NONE
} UCPD_MSG_TYPE;

/* Port power role*/

#define UCPD_PORT_POWER_ROLE_SINK 0u
#define UCPD_PORT_POWER_ROLE_SOURCE 1u

/* Spec revision */

#define UCPD_SPEC_REVISION_1_0 0u
#define UCPD_SPEC_REVISION_2_0 1u
#define UCPD_SPEC_REVISION_3_0 2u

/* Port data role */

#define UCPD_PORT_DATA_ROLE_UFP 0u
#define UCPD_PORT_DATA_ROLE_DFP 1u

/* PDO type */
typedef enum {
  UCPD_PDO_TYPE_FIXED_SUPPLY = 0u,
  UCPD_PDO_TYPE_APDO = 3u
} UCPD_PDO_Type;

/* APDO type */
typedef enum {
  UCPD_APDO_TYPE_PPS = 0u,
  UCPD_APDO_TYPE_AVS = 1u
} UCPD_APDO_Type;

/* Alert DO flags */
#define UCPD_ADO_ALERT_TYPE_BAT_STAT_CHANGE (1u << 1u)
#define UCPD_ADO_ALERT_TYPE_OCP (1u << 2u)
// #define UCPD_ADO_ALERT_TYPE_OTP (1u << 3u) - not used for sink
#define UCPD_ADO_ALERT_TYPE_OP_COND_CHANGE (1u << 4u)
#define UCPD_ADO_ALERT_TYPE_SRC_INP_CHANGE (1u << 5u)
#define UCPD_ADO_ALERT_TYPE_OVP (1u << 6u)
#define UCPD_ADO_ALERT_TYPE_EXT_ALERT (1u << 7u)

typedef uint8_t UCPD_AlertType;

/* Extended alert type */
#define UCPD_EXT_ALERT_TYPE_POW_STATE_CHANGE 1u
#define UCPD_EXT_ALERT_TYPE_POW_BUTTON_PRESS 2u
#define UCPD_EXT_ALERT_TYPE_POW_BUTTON_RELEASE 3u
#define UCPD_EXT_ALERT_TYPE_CONTROLLER_INIT_WAKE 4u

typedef uint8_t UCPD_ExtendedAlertType;

/* Source info port type types*/
#define UCPD_SOURCE_INFO_PORT_MANAGED 0u
#define UCPD_SOURCE_INFO_PORT_GUARANTEED 1u

typedef uint8_t UCPD_SourceInfoPortType;

/* PPS status PTF defines */
#define UCPD_PPS_STATUS_PTF_NS 0u
#define UCPD_PPS_STATUS_PTF_NORMAL 1u
#define UCPD_PPS_STATUS_PTF_WARN 2u
#define UCPD_PPS_STATUS_PTF_OT 3u

/* EPR action defines */
#define UCPD_EPR_ACTION_ENTER 1u
#define UCPD_EPR_ACTION_ENTER_ACK 2u
#define UCPD_EPR_ACTION_ENTER_SUCC 3u
#define UCPD_EPR_ACTION_ENTER_FAIL 4u
#define UCPD_EPR_ACTION_EXIT 5u

/* EPR EPRMDO data defines */
#define UCPD_ENTER_FAILED_UNKNOWN 0u
#define UCPD_ENTER_FAILED_CABLE_NOT_EPR 1u
#define UCPD_ENTER_FAILED_SOURCE_VCONN_FAILED 2u
#define UCPD_ENTER_FAILED_SOURCE_EPR_BIT_NOT_SET_RDO 3u
#define UCPD_ENTER_FAILED_SOURCE_UNABLE_TO_ENTER 4u
#define UCPD_ENTER_FAILED_SOURCE_EPR_BIT_NOT_SET_PDO 5u

/* Message structs typedefs */

typedef __PACKED_STRUCT {
  uint16_t message_type : 5u;
  uint16_t port_data_role : 1u;
  uint16_t spec_revision : 2u;
  uint16_t port_power_role : 1u;
  uint16_t message_id : 3u;
  uint16_t number_of_data_objects : 3u;
  uint16_t extended : 1u;
}
UCPD_MSG_HEADER;

static const UCPD_MSG_HEADER UCPD_MSG_HEADER_INIT = {
    .port_data_role = UCPD_PORT_DATA_ROLE_UFP,
    .spec_revision = UCPD_SPEC_REVISION_3_0,
    .port_power_role = UCPD_PORT_POWER_ROLE_SINK};

typedef __PACKED_STRUCT {
  uint16_t data_size : 9u; // number of bytes in message in total, multiple of 4
                           // bytes
uint16_t:
  1u;
  uint16_t request_chunk : 1u;
  uint16_t chunk_number : 4u;
  uint16_t chunked : 1u;
}
UCPD_MSG_EXT_HEADER;

/* Power objects typedefs */
/* Only Fixed supply and PPS APDO PDOs are defined but can be extended */

typedef __PACKED_UNION {
  uint32_t _;
  __PACKED_STRUCT {
    uint32_t max_current10mA : 10u;
    uint32_t voltage50mV : 10u;
    uint32_t peak_current : 2u; // 00 -- IOC == Ipeak
    uint32_t p : 1u;
    uint32_t epr_mode_capable : 1u;           // shall be zero
    uint32_t unchunke_ext_msg_supported : 1u; // shall be zero
    uint32_t dual_role_data : 1u;             // shall be zero
    uint32_t usb_communications_capable : 1u; // shall be zero
    uint32_t unconstrained_power : 1u;        // will be zero
    uint32_t usb_suspend_supported : 1u;      // shall be zero
    uint32_t dual_role_power : 1u;            // shall be zero
    uint32_t h : 2u;
  }
  b;
}
UCPD_SRC_PDO_FIXED_SUPPLY;

typedef __PACKED_UNION {
  __PACKED_UNION {
    UCPD_SRC_PDO_FIXED_SUPPLY FixedSupply;
    __PACKED_STRUCT {
      __PACKED_UNION {
        __PACKED_STRUCT {
          uint32_t max_current50mA : 7u;
        uint32_t:
          1u;
          uint32_t min_voltage100mV : 8u;
        uint32_t:
          1u;
          uint32_t max_voltage100mV : 8u;
        uint32_t:
          2u;
          uint32_t pps_power_limited : 1u; // shall be zero
        uint32_t:
          4u;
        }
        PPS;
        __PACKED_STRUCT {
          uint32_t pdp_1w : 8u;           // PDP in 1W increments
          uint32_t min_voltage100mV : 8u; // Minimum Voltage 100mV
        uint32_t:
          1u;                             // Reserved
          uint32_t max_voltage100mV : 9u; // Maximum Voltage 100mV
          uint32_t peak_current : 2u;     // Peak current
        }
        AVS;
        __PACKED_STRUCT {
        uint32_t:
          28u;
          uint32_t apdo_type : 2u; // 00 -- PPS, 01 -- AVS
        uint32_t:
          2u;
        };
      };
    }
    APDO;
    __PACKED_STRUCT {
    uint32_t:
      30u;
      uint32_t pdo_type : 2u; // 00 -- fixed supply, 11 -- APDO
    };
  };
}
UCPD_SRC_PDO;

typedef __PACKED_UNION {
  __PACKED_UNION {
    __PACKED_STRUCT {
      uint32_t op_current10mA : 10u;
      uint32_t voltage50mV : 10u;
    uint32_t:
      3u;
      uint32_t frs_current : 2u;                // 00 not supported
      uint32_t dual_role_data : 1u;             // shall be zero
      uint32_t usb_communications_capable : 1u; // shall be zero
      uint32_t unconstrained_power : 1u;        // will be zero
      uint32_t higher_capability : 1u;
      uint32_t dual_role_power : 1u; // shall be zero
    }
    FixedSupply;
    __PACKED_STRUCT {
      __PACKED_UNION {
        __PACKED_STRUCT {
          uint32_t max_current50mA : 7u;
        uint32_t:
          1u; // shall be zero
          uint32_t min_voltage100mV : 8u;
        uint32_t:
          1u; // shall be zero
          uint32_t max_voltage100mV : 8u;
        uint32_t:
          3u; // shall be zero
        }
        PPS;
        __PACKED_STRUCT {                 // AVS Sink APDO
          uint32_t pdp_1w : 8u;           // PDP in 1W increments
          uint32_t min_voltage100mV : 8u; // Minimum Voltage 100mV
        uint32_t:
          1u;                             // Reserved
          uint32_t max_voltage100mV : 9u; // Maximum Voltage 100mV
          uint32_t peak_current : 2u;     // Peak current (same coding as Fixed)
        }
        AVS;
        __PACKED_STRUCT {
        uint32_t:
          28u;
          uint32_t apdo_type : 2u; // 00 -- PPS, 01 -- AVS
        };
      };
    }
    APDO;
    __PACKED_STRUCT {
    uint32_t:
      30u;
      uint32_t pdo_type : 2u; // 00 -- fixed supply, 11 -- APDO
    };
  };
}
UCPD_SNK_PDO;

/* Request data object typedef */
typedef __PACKED_UNION {
  __PACKED_UNION {
    __PACKED_STRUCT { // fixed supply
      uint32_t max_current10mA : 10u;
      uint32_t op_current10mA : 10u;
    uint32_t:
      2u;
    }
    FixedSupply;
    __PACKED_STRUCT { // pps apdo
      uint32_t op_current50mA : 7u;
    uint32_t:
      2u;
      uint32_t out_voltage20mV : 12u;
    uint32_t:
      1u;
    }
    PPS;
    __PACKED_STRUCT // avs epr
    {
      uint32_t op_current50mA : 7u;
    uint32_t:
      2u; // shall be zero
      uint32_t out_voltage100mV : 12u;
    uint32_t:
      1u; // reserved
    }
    AVS;
    __PACKED_STRUCT {
    uint32_t:
      22u;
      uint32_t epr_mode_capable : 1u;            // shall be zero
      uint32_t unchunked_ext_msg_supported : 1u; // shall be zero
      uint32_t no_usb_suspend : 1u;
      uint32_t usb_communications_capable : 1u; // shall be zero
      uint32_t capability_mismatch : 1u;
      uint32_t give_back : 1u; // shall be zero
      uint32_t object_pos : 4u;
    };
  };
}
UCPD_SNK_RDO;

/* BIST data objects typedef */
typedef __PACKED_STRUCT {
uint32_t:
  28u;
  uint32_t mode : 4u; // 0101 - carrier mode, 1000 - test data, 1001 - shared
                      // test entry, 1010 - shared test exit
}
UCPD_BDO;

/* Alert data object typedef */
typedef __PACKED_STRUCT {
  uint32_t ext_alert_type : 4u;
uint32_t:
  12u;
  uint32_t hot_sw_bats : 4u;
  uint32_t fixed_bats : 4u;
  uint32_t type : 8u;
}
UCPD_ADO;

/* Source info data object typedef */
typedef __PACKED_STRUCT {
  uint32_t port_reported_pdp : 8u;
  uint32_t port_present_pdp : 8u;
  uint32_t port_max_pdp : 8u;
uint32_t:
  7u;
  uint32_t type : 1u;
}
UCPD_SIDO;

/* Revision data object typedef */
typedef __PACKED_STRUCT {
uint32_t:
  16u;
  uint32_t version_minor : 4u;
  uint32_t version_major : 4u;
  uint32_t revision_minor : 4u;
  uint32_t revision_major : 4u;
}
UCPD_RMDO;

/* Status data block define */
/* TODO: define later */
typedef __PACKED_STRUCT { uint8_t status[7]; }
UCPD_SDB;

/* PPS status data block define */
typedef __PACKED_STRUCT {
  uint16_t output_voltage20mV; // 0xFFFF - no support
  uint8_t output_current50mA;  // 0xFF - no support
uint8_t:
  1u;
  uint8_t ptf : 2u;
  uint8_t omf : 1u;
uint8_t:
  4u;
}
UCPD_PPS_SDB;

/* Sink capabilities extended data block define */
typedef __PACKED_STRUCT {
  uint16_t vid;
  uint16_t pid;
  uint32_t xid;
  uint8_t fw_revision;
  uint8_t hw_revision;
  uint8_t skedb_version;
  uint8_t load_step : 2u;
uint8_t:
  6u;
  __PACKED_STRUCT {
    uint16_t per_overload : 5u;
    uint16_t overload_per : 6u;
    uint16_t duty_cycle : 4u;
    uint16_t vbus_tol : 1u;
  }
  sink_load_char;
  uint8_t compliance;
  uint8_t touch_temp;
  __PACKED_STRUCT {
    uint8_t fix_bat : 4u;
    uint8_t hot_sw_bat : 4u;
  }
  bat_info;
  __PACKED_STRUCT {
    uint8_t pps_charge : 1u;
    uint8_t vbus_pow : 1u;
    uint8_t mains_pow : 1u;
    uint8_t bat_pow : 1u;
    uint8_t bat_unlim : 1u;
    uint8_t avs_sup : 1u;
  uint8_t:
    2u;
  }
  sink_modes;
  uint8_t snk_min_pdp : 7u;
uint8_t:
  1u;
  uint8_t snk_op_pdp : 7u;
uint8_t:
  1u;
  uint8_t snk_max_pdp : 7u;
uint8_t:
  1u;
  uint8_t epr[3]; // not used
}
UCPD_SKEDB;

/* Extended control data block define */
typedef __PACKED_STRUCT {
  uint8_t type;
  uint8_t data;
}
UCPD_ECDB;

typedef __PACKED_STRUCT {
uint32_t:
  16u;
  uint32_t data : 8u;
  uint32_t action : 8u;
}
UCPD_EPRMDO;

typedef __PACKED_STRUCT {
  UCPD_SNK_RDO snk_rdo;
  UCPD_SRC_PDO src_pdo;
}
UCPD_EPR_REQUEST;

typedef __PACKED_UNION {
  UCPD_SNK_PDO snk_pdos[UCPD_MAX_PDO_NUM];
  UCPD_SNK_RDO snk_rdo;
  UCPD_SRC_PDO src_pdos[UCPD_MAX_PDO_NUM];
  UCPD_SRC_PDO epr_pdos[UCPD_MAX_PDO_NUM + UCPD_MAX_EPR_PDO_NUM];
  UCPD_ECDB epr_ecdb;
  UCPD_EPRMDO epr_mdo;
  UCPD_EPR_REQUEST epr_request;
}
UCPD_MSG_BODY;

typedef __PACKED_STRUCT {
  UCPD_MSG_EXT_HEADER ext_header;
  UCPD_MSG_BODY body;
}
UCPD_EXT_MSG;

/* Generalized message typedef */
typedef __PACKED_UNION {
  uint8_t buffer[UCPD_EPR_MESSAGE_LEN];
  __PACKED_STRUCT {
    UCPD_MSG_HEADER header;
    __PACKED_UNION {
      UCPD_EXT_MSG ext_message;
      UCPD_MSG_BODY body;
    };
  };
}
UCPD_MSG;

static const UCPD_MSG UCPD_MSG_INIT = {.header = UCPD_MSG_HEADER_INIT};

#endif // UCPD_MSG_H
