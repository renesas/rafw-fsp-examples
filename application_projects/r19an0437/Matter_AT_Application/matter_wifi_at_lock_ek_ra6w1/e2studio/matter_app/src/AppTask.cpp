/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *    Copyright (c) 2026 Modified by Renesas Electronics Corporation
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/util/attribute-storage.h>

#include <assert.h>

#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

#include <lib/support/CodeUtils.h>
#include <platform/CHIPDeviceLayer.h>

using namespace chip;
using namespace ::chip::DeviceLayer;

using namespace chip::TLV;
using namespace ::chip::DeviceLayer;

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::Init()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    err = BaseApplication::Init();

    if (err != CHIP_NO_ERROR)
    {
        RENES_LOG("BaseApplication::Init() failed");
        appError(err);
    }

    return err;
}

CHIP_ERROR AppTask::StartAppTask()
{
    return BaseApplication::StartAppTask(AppTaskMain);
}

void AppTask::AppTaskMain(void *pvParameter)
{
    AppEvent event;
    QueueHandle_t sAppEventQueue = *(static_cast<QueueHandle_t *>(pvParameter));

    CHIP_ERROR err = sAppTask.Init();

    if (err != CHIP_NO_ERROR)
    {
        RENES_LOG("AppTask.Init() failed");
        appError(err);
    }

    RENES_LOG("App Task started");


#if defined(__USE_MATTER_DPM_APP__)
    if (RM_PMGR_W_dpm_is_enabled()) 
    {
        RENES_LOG("RM_PMGR_W_dpm_is_enabled");
        static bool flagFirst = false;
        if (RM_PMGR_W_dpm_is_wakeup() && !flagFirst) 
        {
            RENES_LOG("RM_PMGR_W_dpm_is_wakeup")
            flagFirst = true;
            chip::Server::GetInstance().CheckServerReadyEventAlt();
        }
    }
#endif //__USE_MATTER_DPM_APP__

    while (true)
    {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, portMAX_DELAY);

        while (eventReceived == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);

            ChipLogProgress(DeviceLayer, "[%s:%s:%d] eventReceived", __FILENAME__, __func__, __LINE__);
        }
    }
}
