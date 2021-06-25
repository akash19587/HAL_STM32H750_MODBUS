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

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "usart.h"
/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
	 if(TRUE==xRxEnable)
    {
		#if(USER_USE_PORT == USER_USE_PORT1)
    	HAL_NVIC_EnableIRQ(USART1_IRQn);
		#elif(USER_USE_PORT == USER_USE_PORT5)
    	HAL_NVIC_EnableIRQ(UART5_IRQn);
		#endif
    }
    else
    {
		#if(USER_USE_PORT == USER_USE_PORT1)
    	HAL_NVIC_DisableIRQ(USART1_IRQn);
		#elif(USER_USE_PORT == USER_USE_PORT5)
    	HAL_NVIC_DisableIRQ(UART5_IRQn);
		#endif
    }
    if(TRUE==xTxEnable)
    {
    }
    else
    {
	}
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
//指定port进行初始化配置
	return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
	#if(USER_USE_PORT == USER_USE_PORT1)
	HAL_UART_Transmit(&huart1, (uint8_t *)&ucByte, 1 , 0xffff);
	#elif(USER_USE_PORT == USER_USE_PORT5)
	HAL_UART_Transmit(&huart5, (uint8_t *)&ucByte, 1 , 0xffff);
	#endif
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
#if(USER_USE_PORT == USER_USE_PORT1)
	* pucByte = uart1RxBuffer;
#elif(USER_USE_PORT == USER_USE_PORT5)
	* pucByte = uart5RxBuffer;
#endif
	return TRUE;

}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}
#if 0
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        prvvUARTRxISR();
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
#endif
