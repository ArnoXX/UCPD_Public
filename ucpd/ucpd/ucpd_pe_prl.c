#include "ucpd/ucpd_pe_prl.h"
#include "device/ucpd_hw_defines.h"
#include "ucpd/ucpd_hw.h"
#include "ucpd/ucpd_typec.h"
#include "ucpd_msg.h"
#include "ucpd_pe_prl.h"
#include "ucpd_port_num.h"
#include "ucpd_ctx.h"


void UCPD_PE_PRL_CAD_Init(UCPD_PORT_Number port_number, UCPD_PE_PRL_CAD_Module *pe_prl_cad)
{
    *pe_prl_cad = (UCPD_PE_PRL_CAD_Module){
      .port_number = port_number,
      .cad_state = CAD_STATE_UNATTACHED,
      .buffers = {
        [UCPD_TX_BUFFER_INDEX] = UCPD_MSG_INIT,
        [UCPD_EXT_TX_BUFFER_INDEX] = UCPD_MSG_INIT,
        [UCPD_RX_BUFFER_INDEX] = UCPD_MSG_INIT,
        [UCPD_EXT_RX_BUFFER_INDEX] = UCPD_MSG_INIT,
      }
    };
}

void UCPD_PE_PRL_CAD_Reset(UCPD_PE_PRL_CAD_Module *pe_prl_cad)
{
    UCPD_PORT_Number port_number = pe_prl_cad->port_number;

    UCPD_PE_PRL_CAD_Init(port_number, pe_prl_cad);
}

void UCPD_PE_PRL_Handler(UCPD_PORT_Number port_number) {

  UCPD_PE_PRL_CAD_Module* pe_prl_cad = &UCPD_CTX_GetPortInstance(port_number)->pe_prl_cad;

  // on the level of hardware, interrupts have line priorities so even if the
  // software priority is same, one can preempt the other - no idea here which
  // ones will be used on target HW
  BLOCK_EVENT(UCPD_TIMER_EVENT);
  BLOCK_EVENT(UCPD_ADC_EVENT);
  
  // first check the pwr events for vbus removal
  // may exit earlier if detached
if (pe_prl_cad->pwr_event != PE_PRL_CAD_EVENT_NONE) {
    UCPD_CAD_SM_Delta(pe_prl_cad, pe_prl_cad->pwr_event);
    pe_prl_cad->pwr_event = PE_PRL_CAD_EVENT_NONE;
  }

  // we want to give priority to connection removed and hard reset
  if(pe_prl_cad->phy_event == CAD_EVENT_CONN_REMOVED)
  {
    UCPD_CAD_SM_Delta(pe_prl_cad, pe_prl_cad->phy_event);
    pe_prl_cad->phy_event = PE_PRL_CAD_EVENT_NONE;
  }

  if(pe_prl_cad->phy_event == PE_PRL_EVENT_HRD_RECEIVED)
  {
    UCPD_PRL_SM_Delta(pe_prl_cad, pe_prl_cad->phy_event);
    pe_prl_cad->phy_event = PE_PRL_CAD_EVENT_NONE;
    pe_prl_cad->timer_event = PE_PRL_CAD_EVENT_NONE;
  }

  // same with cc detach debounce - may exit earlier if detached
  if (pe_prl_cad->timer_event != PE_PRL_CAD_EVENT_NONE) {
    UCPD_CAD_SM_Delta(pe_prl_cad, pe_prl_cad->timer_event);
    UCPD_PRL_SM_Delta(pe_prl_cad, pe_prl_cad->timer_event);
    UCPD_PE_SM_Delta(pe_prl_cad, pe_prl_cad->timer_event);
    pe_prl_cad->timer_event = PE_PRL_CAD_EVENT_NONE;
  }

  // timers take priority over received messages
  // i.e. if we get goodcrc message and corresponding timer expires, we will
  // process the timer first
  if (pe_prl_cad->phy_event != PE_PRL_CAD_EVENT_NONE) {
    UCPD_CAD_SM_Delta(pe_prl_cad, pe_prl_cad->phy_event);
    UCPD_PRL_SM_Delta(pe_prl_cad, pe_prl_cad->phy_event);

    pe_prl_cad->phy_event = PE_PRL_CAD_EVENT_NONE;
  }

  // dpm queries have lowest priority
  if (pe_prl_cad->dpm_event != PE_PRL_CAD_EVENT_NONE) {
    UCPD_PE_SM_Delta(pe_prl_cad, pe_prl_cad->dpm_event);

    pe_prl_cad->dpm_event = PE_PRL_CAD_EVENT_NONE;
  }

  UCPD_PHY_Worker_Finished(pe_prl_cad->port_number);
  UNBLOCK_EVENT(UCPD_ADC_EVENT);
  UNBLOCK_EVENT(UCPD_TIMER_EVENT);
}


