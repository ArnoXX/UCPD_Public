#include "device/ucpd_hw_defines.h"
#include "ucpd/ucpd_hw.h"



void UCPD_PWR_SM_Delta(UCPD_PORT_Number port_number, UCPD_PWR_SM* sm, uint16_t vbus_voltage)
{
	UCPD_PORT_INSTANCE* port = UCPD_CTX_GetPortInstance(port_number);

	switch (sm->state)
	{
		case UCPD_PWR_STATE_VBUS_ABSENT:
		{
			if (vbus_voltage <= UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V)
				break;
			else if (vbus_voltage < UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V)
			{

				sm->state = UCPD_PWR_STATE_VBUS_TRANSIENT;
				// when going to transient we mark that we only exited v0, we did not go
				// to v5 yet
				sm->exited_v0 = true;
				sm->vbus_vS0_to_vS5 = false;
				AWDEnable(false);
			}
			else if (vbus_voltage >= UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V)
			{
				sm->state = UCPD_PWR_STATE_VBUS_PRESENT;
				sm->exited_v0 = false;
				sm->vbus_vS0_to_vS5 = true;
				port->pe_prl_cad.pwr_event = CAD_EVENT_VBUS_DETECTED;

				UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_PWR);
				AWDEnable(true);
			}
			break;
		}
		case UCPD_PWR_STATE_VBUS_TRANSIENT:
		{
			if (vbus_voltage >= UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V)
			{
				sm->state = UCPD_PWR_STATE_VBUS_PRESENT;
				// notify only if we previously were in v0
				if (sm->exited_v0)
				{
					sm->exited_v0 = false;
					sm->vbus_vS0_to_vS5 = true;
					// notify vbus present
					port->pe_prl_cad.pwr_event = CAD_EVENT_VBUS_DETECTED;

					UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_PWR);
				}

				AWDEnable(true);
			}
			else if (vbus_voltage <= UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V)
			{
				sm->state = UCPD_PWR_STATE_VBUS_ABSENT;
				// notify only if we previously were in v5
				if (sm->exited_v0 == false)
				{
					sm->exited_v0 = false;
					sm->vbus_vS0_to_vS5 = false;
					// notify vbus absent
					port->pe_prl_cad.pwr_event = CAD_EVENT_VBUS_REMOVED;

					UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_PWR);
				}

				AWDEnable(false);
			}
			else
			{
				if (sm->exited_v0)
				{
					AWDEnable(false);
				}
				else
				{
					AWDEnable(true);
				}

			} // else - still transient
			break;
		}
		case UCPD_PWR_STATE_VBUS_PRESENT:
		{
			if (vbus_voltage >= UCPD_PWR_VOLTAGE_LOWER_V_SAFE_5V)
				break;
			else if (vbus_voltage > UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V)
			{
				sm->state = UCPD_PWR_STATE_VBUS_TRANSIENT;
				AWDEnable(true);
			}
			else if (vbus_voltage <= UCPD_PWR_VOLTAGE_UPPER_V_SAFE_0V)
			{
				sm->state = UCPD_PWR_STATE_VBUS_ABSENT;
				sm->vbus_vS0_to_vS5 = false;
				port->pe_prl_cad.pwr_event = CAD_EVENT_VBUS_REMOVED;

				UCPD_PE_PRL_Event_Emit(port_number, PD_EVENT_SOURCE_PWR);

				AWDEnable(false);
			}

			break;
		}
		default:
		{
			break;
		}
	}
}

bool UCPD_PWR_IsVbusPresent(UCPD_PORT_Number port_number)
{
	UCPD_PWR_SM* sm = &getPortHandles()[port_number].pwr_sm;

	return sm->state == UCPD_PWR_STATE_VBUS_PRESENT;
}

// has side effect: sm.vbus_vS0_to_vS5 is set to false when called
bool UCPD_PWR_vS0_to_vS5(UCPD_PORT_Number port_number)
{
	UCPD_PWR_SM* sm = &getPortHandles()[port_number].pwr_sm;

	bool ret = sm->vbus_vS0_to_vS5;
	sm->vbus_vS0_to_vS5 = false;
	return ret;
}
