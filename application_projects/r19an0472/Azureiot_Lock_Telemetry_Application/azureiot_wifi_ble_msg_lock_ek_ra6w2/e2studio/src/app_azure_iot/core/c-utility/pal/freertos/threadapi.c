// Copyright (c) Microsoft. All rights reserved.
// Modifications Copyright (c) 2026 Renesas Electronics Corporation
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "azure_c_shared_utility/xlogging.h"

/*Codes_SRS_THREADAPI_FREERTOS_30_001: [ The threadapi_freertos shall implement the method ThreadAPI_Sleep defined in threadapi.h ]*/
#include "azure_c_shared_utility/threadapi.h"
//AZURE_CHANGES[[:
#include "FreeRTOS.h"
#include "task.h"
#include "app_azure_user_conf.h"
//AZURE_CHANGES]]
/*Codes_SRS_THREADAPI_FREERTOS_30_002: [ The ThreadAPI_Sleep shall receive a time in milliseconds. ]*/
/*Codes_SRS_THREADAPI_FREERTOS_30_003: [ The ThreadAPI_Sleep shall stop the thread for the specified time. ]*/
void ThreadAPI_Sleep(unsigned int milliseconds)
{
//AZURE_CHANGES azurefrtoswork[[:
	//vTaskDelay((milliseconds * configTICK_RATE_HZ) / 1000);
	if (milliseconds < 10)
	{
	portTickType xFlashRate, xLastFlashTime;
		//milliseconds = 10;
		//xFlashRate = milliseconds/portTICK_RATE_MS;
		xFlashRate = 1;
	xLastFlashTime = xTaskGetTickCount();
	vTaskDelayUntil( &xLastFlashTime, xFlashRate );
	}
	else
	{
		vTaskDelay((milliseconds * configTICK_RATE_HZ) / 1000);
	}
//AZURE_CHANGES azurefrtoswork]]
}

/*Codes_SRS_THREADAPI_FREERTOS_30_004: [ FreeRTOS is not guaranteed to support threading, so ThreadAPI_Create shall return THREADAPI_ERROR. ]*/
THREADAPI_RESULT ThreadAPI_Create(THREAD_HANDLE* threadHandle, THREAD_START_FUNC func, void* arg)
{
//AZURE_CHANGES azurefrtoswork[[::
#if 0 //AZURE_CHANGES azurefrtoswork
    (void)threadHandle;
    (void)func;
    (void)arg;
    LogError("FreeRTOS does not support multi-threading.");
    return THREADAPI_ERROR;
#endif //AZURE_CHANGES azurefrtoswork

	BaseType_t ret = 0;
	char name[32] = {0,};
	static char threadCnt = 0;
	THREAD_HANDLE parentThread = NULL;
	UBaseType_t parentPriority;
	UBaseType_t thisPriority;

	sprintf(name,"%s_%d", "azrSubT", threadCnt++);
	/*
		task priority must be lower than that of parent thread
	*/
	
	thisPriority = AZURE_SUBTASK_PRIORITY;
	
	ret = xTaskCreate( (void *)func, name, 1024*4/sizeof(portSTACK_TYPE), (void*)arg, thisPriority, threadHandle );
	if ( pdPASS != ret ) {
		LogError("Creation failed (name:\"%s\"");
		return THREADAPI_ERROR;
	}
	else
	{
		LogInfo("thread \"%s\" creation OK (pri=%d)", name, thisPriority);
	}

	return THREADAPI_OK;
////AZURE_CHANGES azurefrtoswork]]
}

/*Codes_SRS_THREADAPI_FREERTOS_30_005: [ FreeRTOS is not guaranteed to support threading, so ThreadAPI_Join shall return THREADAPI_ERROR. ]*/
THREADAPI_RESULT ThreadAPI_Join(THREAD_HANDLE threadHandle, int* res)
{
    (void)threadHandle;
    (void)res;
    LogError("FreeRTOS does not support multi-threading.");
    return THREADAPI_ERROR;
}

/*Codes_SRS_THREADAPI_FREERTOS_30_006: [ FreeRTOS is not guaranteed to support threading, so ThreadAPI_Exit shall do nothing. ]*/
void ThreadAPI_Exit(int res)
{
    (void)res;
    LogError("FreeRTOS does not support multi-threading.");
}
