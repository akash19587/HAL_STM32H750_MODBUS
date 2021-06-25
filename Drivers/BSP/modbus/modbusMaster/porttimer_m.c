/*
 * FreeModbus Libary: RT-Thread Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"
#include "mbconfig.h"
#include "main.h"
#include "stm32h750xx.h"
#include "cmsis_os.h"
/* ----------------------- static functions ---------------------------------*/

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Variables ----------------------------------------*/
static ULONG usT35TimeOut50us;

osTimerId_t port_timermID;

const osTimerAttr_t port_timerm_attributes = {
  .name = "port_timerm"
};
 extern void prvvTIMERExpiredISR_Master(void);

/*
* function : 初始化timer5定时器，定时溢出 200us溢出中断
* by liuzhiheng 2021-03-03
*/
void tim_init(void)
{
	port_timermID = osTimerNew(prvvTIMERExpiredISR_Master, osTimerPeriodic, NULL, &port_timerm_attributes);
}
/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit(USHORT usTimeOut50us)
{
	usT35TimeOut50us = usTimeOut50us;
	tim_init();
	return TRUE;
}
void vMBMasterPortTimersEnable(void )
{
    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
	osTimerStart(port_timermID , 1000);//1s的超时时间
}
//receive time out
void vMBMasterPortTimersT35Enable()
{
	#if 0
    rt_tick_t timer_tick = (50 * usT35TimeOut50us)
            / (1000 * 1000 / RT_TICK_PER_SECOND);

    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_T35);

    rt_timer_control(&timer, RT_TIMER_CTRL_SET_TIME, &timer_tick);

    rt_timer_start(&timer);
	#else
	osTimerStart(port_timermID , usT35TimeOut50us);
    vMBMasterSetCurTimerMode(MB_TMODE_T35);
	#endif
}
//set broadcast response timeout
void vMBMasterPortTimersConvertDelayEnable()
{
	#if 0
    rt_tick_t timer_tick = MB_MASTER_DELAY_MS_CONVERT * RT_TICK_PER_SECOND / 1000;

    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);

    rt_timer_control(&timer, RT_TIMER_CTRL_SET_TIME, &timer_tick);

    rt_timer_start(&timer);
	#else
	osTimerStart(port_timermID , MB_MASTER_DELAY_MS_CONVERT);
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);
	#endif
}
//set response time out , can just diffrent time
void vMBMasterPortTimersRespondTimeoutEnable()
{
	#if 0
    rt_tick_t timer_tick = MB_MASTER_TIMEOUT_MS_RESPOND * RT_TICK_PER_SECOND / 1000;

    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);

    rt_timer_control(&timer, RT_TIMER_CTRL_SET_TIME, &timer_tick);

    rt_timer_start(&timer);
	#else
	osTimerStart(port_timermID , MB_MASTER_TIMEOUT_MS_RESPOND);//设置应答超时等待时间
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);
	#endif
}

void vMBMasterPortTimersDisable()
{
#if 0
	rt_timer_stop(&timer);
#else
	osTimerStop(port_timermID);
#endif
}

void prvvTIMERExpiredISR_Master(void)
{
	if(pxMBMasterPortCBTimerExpired != NULL)
	{
    	(void) pxMBMasterPortCBTimerExpired();
	}
}


#endif
