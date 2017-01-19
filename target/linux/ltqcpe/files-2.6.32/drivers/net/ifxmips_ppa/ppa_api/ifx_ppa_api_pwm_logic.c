/******************************************************************************
**
** FILE NAME    : ifx_ppa_api_pws.c
** PROJECT      : UEIP
** MODULES      : PPA API (Power Saving APIs)
**
** DATE         : 16 DEC 2009
** AUTHOR       : Shao Guohua
** DESCRIPTION  : PPA Protocol Stack Power Saving Logic API Implementation
** COPYRIGHT    :              Copyright (c) 2009
**                          Lantiq Deutschland GmbH
**                   Am Campeon 3; 85579 Neubiberg, Germany
**
**   For licensing information, see the file 'LICENSE' in the root folder of
**   this software module.
**
** HISTORY
** $Date        $Author            $Comment
** 16 DEC 2009  Shao Guohua        Initiate Version
*******************************************************************************/
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <asm/time.h>

/*
 *  PPA Specific Head File
 */
#include <net/ifx_ppa_api.h>
#include <net/ifx_ppa_ppe_hal.h>
#include "ifx_ppa_api_misc.h"
#include "ifx_ppa_api_session.h"
#include "ifx_ppe_drv_wrapper.h"


static IFX_PMCU_STATE_t ppa_psw_stat_curr   = IFX_PMCU_STATE_D0;



int32_t ppa_pwm_logic_init(void)
{
    return IFX_SUCCESS;
}

IFX_PMCU_STATE_t ppa_pwm_get_current_status(int32_t flag)
{
    return ppa_psw_stat_curr;
}

int32_t ppa_pwm_set_current_state(IFX_PMCU_STATE_t e_state, int32_t flag)
{
    return IFX_SUCCESS;
}

//currently, we only accept D0( fully on) and D3 (off) states. We take D1 & D2 as D3
//int32_t inline ppa_pwm_turn_state_on(IFX_PMCU_STATE_t cur_state, IFX_PMCU_STATE_t new_state)
//{
//    return (cur_state != IFX_PMCU_STATE_D0 && new_state == IFX_PMCU_STATE_D0);
//}

int32_t inline ppa_pwm_turn_state_off(IFX_PMCU_STATE_t cur_state, IFX_PMCU_STATE_t new_state)
{
    return (cur_state == IFX_PMCU_STATE_D0 && new_state != IFX_PMCU_STATE_D0);
}

int32_t ppa_pwm_pre_set_current_state(IFX_PMCU_STATE_t e_state, int32_t flag)
{
    if(ppa_pwm_turn_state_off(ppa_psw_stat_curr, e_state)){
        return (ppa_get_hw_session_cnt() > 0 ? IFX_FAILURE : IFX_SUCCESS);
    }

    return IFX_SUCCESS;
}

int32_t ppa_pwm_post_set_current_state(IFX_PMCU_STATE_t e_state, int32_t flag)
{
    unsigned long sys_flag;

    local_irq_save(sys_flag);
    if ( ppa_psw_stat_curr != e_state )
    {
        int pwm_level = e_state == IFX_PMCU_STATE_D0 ? IFX_PPA_PWM_LEVEL_D0 : IFX_PPA_PWM_LEVEL_D3;

         ifx_ppa_drv_ppe_clk_change(pwm_level, 0);

        ppa_psw_stat_curr = e_state;
    }
    local_irq_restore(sys_flag);

    return IFX_SUCCESS;
}
