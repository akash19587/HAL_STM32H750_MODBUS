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
 * File: $Id: portserial_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */

#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "mbconfig.h"

#include "usart.h"
#include "cmsis_os.h"
#include "queue.h"
#include "semphr.h"
SemaphoreHandle_t   event_serial = NULL;
#define MODBUS_TRANS_TASK_STACK   (256)
#define MODBUS_TRANS_TASK_PRIO osPriorityNormal2
TaskHandle_t MODBUS_TRANS_Task_Handler;


#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
BOOL prvvUARTTxReadyISR_Master(void);

/*
* fucntion: 串口发射任务
* by liuzhiheng 2021-03-05
*/
static void serial_soft_trans_irq(void const *pArg)
{
	BaseType_t xresult;
	while(1)
	{
		xresult = xSemaphoreTake(event_serial, osWaitForever);
		if(xresult != pdTRUE)
		{
			continue;
		}
		while(prvvUARTTxReadyISR_Master() == 1){};//一直发送数据完成
	}
}
/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
        eMBParity eParity)
{
//初始化配置
//创建串口事件
    event_serial = xSemaphoreCreateBinary();
//创建发送任务
    /** create task */
	xTaskCreate((TaskFunction_t )serial_soft_trans_irq,
                (const char*    )"serial_soft_trans_irq",
                (uint16_t       )MODBUS_TRANS_TASK_STACK,
                (void*          )NULL,
                (UBaseType_t    )MODBUS_TRANS_TASK_PRIO,
                (TaskHandle_t*  )&MODBUS_TRANS_Task_Handler);

	return TRUE;
}

void vMBMasterPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
	/* If xRXEnable enable serial receive interrupts. If xTxENable enable
	 * transmitter empty interrupts.
	 */
	 if(TRUE==xRxEnable)
	{
		HAL_NVIC_EnableIRQ(UART5_IRQn);
	}
	else
	{
		HAL_NVIC_DisableIRQ(UART5_IRQn);
	}
	if(TRUE==xTxEnable)
	{
		BaseType_t	  os_status;
		if(event_serial != NULL)
		{
			os_status = xSemaphoreGive(event_serial);
			if (os_status != pdTRUE)
			{
			}
		}
	}
	else
	{
	}
}

void vMBMasterPortClose(void)
{

}

BOOL xMBMasterPortSerialPutByte(CHAR ucByte)
{
	uint32_t time_out = 0;
	UART5->TDR = ucByte;
	time_out = HAL_GetTick();
	while((UART5->ISR&0X40)==0)
	{
		if((time_out - HAL_GetTick()) > 500)
		{//100超时
			break;
		}
	}
	return TRUE;
}

BOOL xMBMasterPortSerialGetByte(CHAR * pucByte)
{
#if 1
	uint8_t Res = 0;
	if(UART5->ISR & (1<<5))//如果接受完成
	{
		Res = UART5->RDR;	//读取数据寄存�?,清SR中断标志
	}
#endif
	scif_ch4_rx(&Res, 1);

	*pucByte = Res;
	return TRUE;
}

/*
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
BOOL prvvUARTTxReadyISR_Master(void)
{
	BOOL empty = FALSE;
	if(pxMBMasterFrameCBTransmitterEmpty != NULL)
	{
    	empty = pxMBMasterFrameCBTransmitterEmpty();
	}
	return empty;
}

/*
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvUARTRxISR_Master(unsigned char data)
{
	if(pxMBMasterFrameCBByteReceived != NULL)
	{
    	pxMBMasterFrameCBByteReceived(data);
	}
}


#endif
