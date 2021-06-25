/*
* funtion: modbus主机通讯，任务处理及其事件
* by liuzhiheng 2021-03-02
*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "semphr.h"
#include <stdio.h>
#include "cmsis_os.h"
#include "stdlib.h"
#include "mb.h"
#include "mb_m.h"
#include "mbframe.h"

#define MODBUS_MASTER_TASK_STACK   (256)
#define MODBUS_MASTER_TASK_PRIO osPriorityRealtime7
TaskHandle_t MODBUS_MASTER_Task_Handler;

#define MODBUS_MASTER_TEST_TASK_STACK   (256)
#define MODBUS_MASTER_TEST_TASK_PRIO osPriorityRealtime6
TaskHandle_t MODBUS_MASTER_TEST_Task_Handler;
USHORT  usModbusUserData[1024] = {1,2,3,4,5,6,7,8,9,10};




/* function:Modbus主机轮训任务
 *
 *
 *
 *
*/
static void thread_entry_ModbusMasterPoll(void const *pArg)
{
	eMBMasterInit(MB_ASCII, 2, 115200,  MB_PAR_NONE);
	eMBMasterEnable();

	while (1)
	{
		eMBMasterPoll();
		//osDelay(1000); //DELAY_MB_MASTER_POLL
	}
}

/*==================================================================
* Function     : modbus_task_create
* Description  : modbus任务创建
* Input Para   :
* Output Para  :
* Return Value :
==================================================================*/
void modbus_task_create(void)
{
    /** create task */
	xTaskCreate((TaskFunction_t )thread_entry_ModbusMasterPoll,
                (const char*    )"thread_entry_ModbusMasterPoll",
                (uint16_t       )MODBUS_MASTER_TASK_STACK,
                (void*          )NULL,
                (UBaseType_t    )MODBUS_MASTER_TASK_PRIO,
                (TaskHandle_t*  )&MODBUS_MASTER_Task_Handler);

    return 0;
}/* End of function graphics_create_task() */


