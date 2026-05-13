/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2023 Modified by Renesas Electronics Corporation
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
#pragma once

#include "rnDeviceWrapAPIs.h"
#include <app/clusters/door-lock-server/door-lock-server.h>

#if defined(ENABLE_CHIP_SHELL)

class EventData
{
public:
    chip::EventId eventId;
};

class AlarmEventData : public EventData
{
public:
    AlarmCodeEnum alarmCode;
};

class DoorStateEventData : public EventData
{
public:
    DoorStateEnum doorState;
};

CHIP_ERROR RegisterLockEvents();

void EventWorkerFunction(intptr_t context);

#endif //ENABLE_CHIP_SHELL

#if defined (__MATTER_CLI_DEBUG__)

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
bool send_alarm_event(int argc, const char **argv);

bool send_door_state_event(int argc, const char **argv);

#ifdef __cplusplus
}
#endif

#endif //__MATTER_CLI_DEBUG__
