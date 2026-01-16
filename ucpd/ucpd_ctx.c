#include "ucpd_ctx.h"

static UCPD_DPM __UCPD_DPM;

/* Functions that access ucpd context */
UCPD_PORT_INSTANCE* UCPD_CTX_GetPortInstance(UCPD_PORT_Number port_number)
{
	return &(__UCPD_DPM.ports[port_number]);
}

/* DPM functions */
UCPD_DPM* UCPD_CTX_GetDPM()
{
	return &(__UCPD_DPM);
}
