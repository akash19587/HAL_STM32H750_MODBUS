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
 * File: $Id: portevent_m.c v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"
#include "port.h"
#include "mbconfig.h"
#include "cmsis_os.h"
#include "queue.h"
#include "semphr.h"
#include "cmsis_gcc.h"
//#include "sys_alloc.h"
QueueHandle_t xMasterOsEvent_Req_Res = NULL;//用于应答响应结果事件
QueueHandle_t xMasterOsEvent = NULL;
SemaphoreHandle_t   xMasterRunRes = NULL;

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Defines ------------------------------------------*/
/* ----------------------- Variables ----------------------------------------*/
//static struct rt_semaphore xMasterRunRes;
//static struct rt_event     xMasterOsEvent;
/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBMasterPortEventInit( void )
{
	xMasterOsEvent_Req_Res = xQueueCreate(10, sizeof(_eMBMasterEventType_));
	xMasterOsEvent = xQueueCreate(10, sizeof(_eMBMasterEventType_));
    return TRUE;
}
//extern QueueHandle_t g_mail_scif_ch4_tx;
//#include "scif_ch4.h"
BOOL
xMBMasterPortEventPost( eMBMasterEventType eEvent )
{
	_eMBMasterEventType_ *event_que;
	BaseType_t pxHigherPriorityTaskWoken;
	event_que = (_eMBMasterEventType_ *) malloc(sizeof(_eMBMasterEventType_));
	event_que->mb_event_type = (uint32_t)eEvent;
	if((eEvent == EV_MASTER_PROCESS_SUCESS) || (eEvent == EV_MASTER_ERROR_RESPOND_TIMEOUT) || (eEvent == EV_MASTER_ERROR_RECEIVE_DATA) || (eEvent == EV_MASTER_ERROR_EXECUTE_FUNCTION) || (eEvent == EV_MASTER_ERROR_SEND_FRAME))
	{
		xQueueSend(xMasterOsEvent_Req_Res , (void *)&event_que , osWaitForever);
	}
	else
	{
		xQueueSend(xMasterOsEvent , (void *)&event_que , osWaitForever);
	}
    return TRUE;
}
BOOL
xMBMasterPortEventPost_FromISR( eMBMasterEventType eEvent )
{
	_eMBMasterEventType_ *event_que;
	BaseType_t pxHigherPriorityTaskWoken;
	event_que = (_eMBMasterEventType_ *) malloc(sizeof(_eMBMasterEventType_));
	event_que->mb_event_type = (uint32_t)eEvent;
	if((eEvent == EV_MASTER_PROCESS_SUCESS) || (eEvent == EV_MASTER_ERROR_RESPOND_TIMEOUT) || (eEvent == EV_MASTER_ERROR_RECEIVE_DATA) || (eEvent == EV_MASTER_ERROR_EXECUTE_FUNCTION) || (eEvent == EV_MASTER_ERROR_SEND_FRAME))
	{
		xQueueSendFromISR(xMasterOsEvent_Req_Res , (void *)&event_que , &pxHigherPriorityTaskWoken);
	}
	else
	{
		xQueueSendFromISR(xMasterOsEvent , (void *)&event_que , &pxHigherPriorityTaskWoken);
	}
    return TRUE;
}

//获取发送状态
BOOL
xMBMasterPortEventGet( eMBMasterEventType * eEvent )
{
    uint32_t recvedEvent;
    /* waiting forever OS event */
	_eMBMasterEventType_ *event_que;
	BaseType_t xResult;
	xResult = xQueueReceive(xMasterOsEvent , (void *)&event_que,osWaitForever);
	if(xResult == pdTRUE)
	{
		recvedEvent = event_que->mb_event_type;
	}
	if(event_que != NULL)
	{
		free(event_que);
	}
    /* the enum type couldn't convert to int type */
    switch (recvedEvent)
    {
    case EV_MASTER_READY:
        *eEvent = EV_MASTER_READY;
        break;
    case EV_MASTER_FRAME_RECEIVED:
        *eEvent = EV_MASTER_FRAME_RECEIVED;
        break;
    case EV_MASTER_EXECUTE:
        *eEvent = EV_MASTER_EXECUTE;
        break;
    case EV_MASTER_FRAME_SENT:
        *eEvent = EV_MASTER_FRAME_SENT;
        break;
    case EV_MASTER_ERROR_PROCESS:
        *eEvent = EV_MASTER_ERROR_PROCESS;
        break;
    }
    return TRUE;
}
/**
 * This function is initialize the OS resource for modbus master.
 * Note:The resource is define by OS.If you not use OS this function can be empty.
 *
 */
void vMBMasterOsResInit( void )
{
    xMasterRunRes = xSemaphoreCreateCounting(1 , 1);
}

/**
 * This function is take Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be just return TRUE.
 *
 * @param lTimeOut the waiting time.
 *
 * @return resource taked result
 */
BOOL xMBMasterRunResTake( LONG lTimeOut )
{
    /*If waiting time is -1 .It will wait forever */
	BaseType_t xresult;
	xresult = xSemaphoreTake(xMasterRunRes, lTimeOut);
    return (xresult != pdTRUE)? FALSE : TRUE ;
}

/**
 * This function is release Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be empty.
 *
 */
void vMBMasterRunResRelease( void )
{
    /* release resource */
	xSemaphoreGive(xMasterRunRes);
}

/**
 * This is modbus master respond timeout error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBRespondTimeout(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
    //rt_event_send(&xMasterOsEvent, EV_MASTER_ERROR_RESPOND_TIMEOUT);
	xMBMasterPortEventPost(EV_MASTER_ERROR_RESPOND_TIMEOUT);

    /* You can add your code under here. */

}

/**
 * This is modbus master receive data error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBReceiveData(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
    //rt_event_send(&xMasterOsEvent, EV_MASTER_ERROR_RECEIVE_DATA);
	xMBMasterPortEventPost(EV_MASTER_ERROR_RECEIVE_DATA);

    /* You can add your code under here. */

}

/**
 * This is modbus master execute function error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBExecuteFunction(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
    //rt_event_send(&xMasterOsEvent, EV_MASTER_ERROR_EXECUTE_FUNCTION);
	xMBMasterPortEventPost(EV_MASTER_ERROR_EXECUTE_FUNCTION);
    /* You can add your code under here. */

}

/**
 * This is modbus master request process success callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 */
void vMBMasterCBRequestScuuess( void ) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
    //rt_event_send(&xMasterOsEvent, EV_MASTER_PROCESS_SUCESS);
	xMBMasterPortEventPost(EV_MASTER_PROCESS_SUCESS);

    /* You can add your code under here. */

}
/**
 * This is modbus master request process success callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 */
void vMBMasterSendframeError( void ) {
    /**
     * @note This code is use OS's event mechanism for modbus master protocol stack.
     * If you don't use OS, you can change it.
     */
	xMBMasterPortEventPost(EV_MASTER_ERROR_SEND_FRAME);

    /* You can add your code under here. */

}


/**
 * This function is wait for modbus master request finish and return result.
 * Waiting result include request process success, request respond timeout,
 * receive data error and execute function error.You can use the above callback function.
 * @note If you are use OS, you can use OS's event mechanism. Otherwise you have to run
 * much user custom delay for waiting.
 *
 * @return request error code
 */
 //发起后的接收状态
eMBMasterReqErrCode eMBMasterWaitRequestFinish( void ) {
    eMBMasterReqErrCode    eErrStatus = MB_MRE_NO_ERR;
    uint32_t recvedEvent;
    /* waiting for OS event */
	eMBMasterEventType *event_que;
	BaseType_t xResult;
	xResult = xQueueReceive(xMasterOsEvent_Req_Res , (void *)&event_que,osWaitForever);
	if(xResult == pdTRUE)
	{
		recvedEvent = *event_que;
	}
	if(event_que != NULL)
	{
		sys_free(event_que);
	}
    switch (recvedEvent)
    {
	    case EV_MASTER_PROCESS_SUCESS:
	        break;
	    case EV_MASTER_ERROR_RESPOND_TIMEOUT:
	    {
	        eErrStatus = MB_MRE_TIMEDOUT;
	        break;
	    }
	    case EV_MASTER_ERROR_RECEIVE_DATA:
	    {
	        eErrStatus = MB_MRE_REV_DATA;
	        break;
	    }
	    case EV_MASTER_ERROR_EXECUTE_FUNCTION:
	    {
	        eErrStatus = MB_MRE_EXE_FUN;
	        break;
	    }
		case EV_MASTER_ERROR_SEND_FRAME:
		{
			eErrStatus = MB_MRE_SEND_FRAME;
			break;
		}
    }
    return eErrStatus;
}

#endif
