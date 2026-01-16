#ifndef UCPD_CTX_H
#define UCPD_CTX_H

#include "ucpd/ucpd_dpm.h"
#include "ucpd/ucpd_port.h"

/* Functions that access ucpd context */
UCPD_PORT_INSTANCE* UCPD_CTX_GetPortInstance(UCPD_PORT_Number port_number);

/* DPM functions */
UCPD_DPM* UCPD_CTX_GetDPM();

#endif // UCPD_CTX_H
