/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
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
 * File: $Id$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "tim.h"
/* ----------------------- static functions ---------------------------------*/
static void prvvTIMERExpiredISR( void );
TIM_HandleTypeDef TIM5_Handler;
extern void prvvTIMERExpiredISR_Master(void);

/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBPortTimersInit( USHORT usTim1Timerout50us )
{
#if 0
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	NVIC_InitTypeDef NVIC_InitStructure;
  TIM_DeInit(TIM2);
#if 0
    TIM_TimeBaseStructure.TIM_Period = 0x7E54;        //CLK==24MHz ((1000000000/9600)*11*3.5)/(1000/24) == 0x7e54
    TIM_TimeBaseStructure.TIM_Prescaler = 0x3;
#endif
  // ?????????し?????????7200/72M = 0.0001,????100us????????1
  //10us x 50 = 5ms,??5ms????????
  TIM_TimeBaseStructure.TIM_Period = 100;//超时时间是00
  TIM_TimeBaseStructure.TIM_Prescaler = (7200 - 1);
	TIM_TimeBaseStructure.TIM_ClockDivision = 0x0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn; //定时器2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //抢占优先级1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;  //子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//  TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
  return TRUE;
  #endif
  __HAL_RCC_TIM5_CLK_ENABLE();
  TIM5_Handler.Instance=TIM5;//240m
  TIM5_Handler.Init.Prescaler=1119;//240 000  / 20 000 ; 50us
  TIM5_Handler.Init.CounterMode=TIM_COUNTERMODE_UP;
  TIM5_Handler.Init.Period=usTim1Timerout50us; 					   //
  TIM5_Handler.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_Base_Init(&TIM5_Handler);
  HAL_TIM_Base_Start_IT(&TIM5_Handler);

  HAL_NVIC_SetPriority(TIM5_IRQn,1,15);
  HAL_NVIC_EnableIRQ(TIM5_IRQn);

}


inline void
vMBPortTimersEnable(  )
{

    /* Enable the timer with the timeout passed to xMBPortTimersInit( ) */
	__HAL_TIM_SetCounter(&TIM5_Handler, 0);
	HAL_NVIC_EnableIRQ(TIM5_IRQn);
	HAL_TIM_Base_Start(TIM5);
}

inline void
vMBPortTimersDisable(  )
{
	__HAL_TIM_SetCounter(&TIM5_Handler, 0);
	HAL_NVIC_DisableIRQ(TIM5_IRQn);
	HAL_TIM_Base_Stop(TIM5);
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
static void prvvTIMERExpiredISR( void )
{
    ( void )pxMBPortCBTimerExpired(  );
}

void TIM5_IRQHandler(void)
{
    //prvvTIMERExpiredISR();
    HAL_TIM_IRQHandler(&TIM5_Handler);
}
