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

#include "EventHandlerLibShell.h"
#include "AppTask.h"
#include <lib/support/CodeUtils.h>

constexpr uint8_t lockEndpoint = 1;

#if defined(ENABLE_CHIP_SHELL)

#include "lib/shell/Engine.h"
#include "lib/shell/commands/Help.h"

#include "app/server/Server.h"
#include <platform/CHIPDeviceLayer.h>
#include <platform/CommissionableDataProvider.h>
#include "rnDeviceDataProvider.h"

using namespace chip;
using namespace chip::app;
using namespace chip::DeviceLayer;
using namespace Clusters::DoorLock;
using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

Engine sShellDoorlockSubCommands;
Engine sShellDoorlockEventSubCommands;
Engine sShellDoorlockEventAlarmSubCommands;
Engine sShellDoorlockEventDoorCtrLockSubCommands;
Engine sShellDoorlockEventDoorCtrUnLockSubCommands;
Engine sShellDoorlockEventDoorStateSubCommands;

Engine sShellCustomSubCommands;

Engine sShellConfigSetSubCommands;
Engine sShellConfigSetVIDSubCommands;
Engine sShellConfigSetPIDSubCommands;
Engine sShellConfigSetHWVerSubCommands;
Engine sShellConfigSetPinCodeSubCommands;
Engine sShellConfigSetDiscriminatorSubCommands;


/********************************************************
 * Doorlock shell functions
 *********************************************************/

CHIP_ERROR DoorlockHelpHandler(int argc, char ** argv)
{
    sShellDoorlockSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR DoorlockCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return DoorlockHelpHandler(argc, argv);
    }

    return sShellDoorlockSubCommands.ExecCommand(argc, argv);
}

/********************************************************
 * Event shell functions
 *********************************************************/

CHIP_ERROR EventHelpHandler(int argc, char ** argv)
{
    sShellDoorlockEventSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR EventDoorlockCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return EventHelpHandler(argc, argv);
    }

    return sShellDoorlockEventSubCommands.ExecCommand(argc, argv);
}

/********************************************************
 * Alarm shell functions
 *********************************************************/

CHIP_ERROR AlarmHelpHandler(int argc, char ** argv)
{
    sShellDoorlockEventAlarmSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR AlarmEventHandler(int argc, char ** argv)
{

    if (argc == 0)
    {
        return AlarmHelpHandler(argc, argv);
    }
    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }

    AlarmEventData * data = Platform::New<AlarmEventData>();
    data->eventId         = Events::DoorLockAlarm::Id;
    data->alarmCode       = static_cast<AlarmCodeEnum>(atoi(argv[0]));

    DeviceLayer::PlatformMgr().ScheduleWork(EventWorkerFunction, reinterpret_cast<intptr_t>(data));

    return CHIP_NO_ERROR;
}

/********************************************************
 * Door state shell functions
 *********************************************************/
CHIP_ERROR DoorCtrLockEventHandler(int argc, char ** argv)
{
    DoorStateEventData * data = Platform::New<DoorStateEventData>();
    data->eventId             = Events::DoorStateChange::Id;
    data->doorState           = static_cast<DoorStateEnum>(1);

    DeviceLayer::PlatformMgr().ScheduleWork(EventWorkerFunction, reinterpret_cast<intptr_t>(data));

    return CHIP_NO_ERROR;
}

CHIP_ERROR DoorCtrUnLockEventHandler(int argc, char ** argv)
{
    DoorStateEventData * data = Platform::New<DoorStateEventData>();
    data->eventId             = Events::DoorStateChange::Id;
    data->doorState           = static_cast<DoorStateEnum>(0);

    DeviceLayer::PlatformMgr().ScheduleWork(EventWorkerFunction, reinterpret_cast<intptr_t>(data));

    return CHIP_NO_ERROR;
}

CHIP_ERROR DoorStateHelpHandler(int argc, char ** argv)
{
    sShellDoorlockEventDoorStateSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR DoorStateEventHandler(int argc, char ** argv)
{

    if (argc == 0)
    {
        return DoorStateHelpHandler(argc, argv);
    }
    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }

    DoorStateEventData * data = Platform::New<DoorStateEventData>();
    data->eventId             = Events::DoorStateChange::Id;
    data->doorState           = static_cast<DoorStateEnum>(atoi(argv[0]));

    DeviceLayer::PlatformMgr().ScheduleWork(EventWorkerFunction, reinterpret_cast<intptr_t>(data));

    return CHIP_NO_ERROR;
}

/**
 * @brief configures lock matter shell
 *
 */

CHIP_ERROR RegisterLockEvents()
{
    static const shell_command_t sDoorlockSubCommands[] = { { &DoorlockHelpHandler, "help", "Usage: doorlock <subcommand>" },
                                                            { &EventDoorlockCommandHandler, "event",
                                                              " Usage: doorlock event <subcommand>" } };

    static const shell_command_t sDoorlockEventSubCommands[] = {
        { &EventHelpHandler, "help", "Usage : doorlock event <subcommand>" },
        { &AlarmEventHandler, "lock-alarm", "Sends lock alarm event to lock app" },
        { &DoorCtrLockEventHandler, "lock", "Sends door state change to lock to lock app" },
        { &DoorCtrUnLockEventHandler, "unlock", "Sends door state change to unlock to lock app" },
        { &DoorStateEventHandler, "door-state-change", "Sends door state change event to lock app" }
    };

    static const shell_command_t sDoorlockEventAlarmSubCommands[] = { { &AlarmHelpHandler, "help",
                                                                        "Usage : doorlock event lock-alarm AlarmCode" } };

    static const shell_command_t sDoorlockEventDoorStateSubCommands[] = {
        { &DoorStateHelpHandler, "help", "Usage : doorlock event door-state-change DoorState" }
    };

    static const shell_command_t sDoorLockCommand = { &DoorlockCommandHandler, "doorlock",
                                                      "doorlock commands. Usage: doorlock <subcommand>" };

    sShellDoorlockEventAlarmSubCommands.RegisterCommands(sDoorlockEventAlarmSubCommands, ArraySize(sDoorlockEventAlarmSubCommands));
    sShellDoorlockEventDoorStateSubCommands.RegisterCommands(sDoorlockEventDoorStateSubCommands,
                                                             ArraySize(sDoorlockEventDoorStateSubCommands));
    sShellDoorlockEventSubCommands.RegisterCommands(sDoorlockEventSubCommands, ArraySize(sDoorlockEventSubCommands));
    sShellDoorlockSubCommands.RegisterCommands(sDoorlockSubCommands, ArraySize(sDoorlockSubCommands));

    Engine::Root().RegisterCommands(&sDoorLockCommand, 1);

    return CHIP_NO_ERROR;
}

/********************************************************
 * Custom shell functions
 *********************************************************/
CHIP_ERROR CustomHelpHandler(int argc, char ** argv)
{
    sShellCustomSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR CustomCommand(int argc, char ** argv)
{
    if (argc == 0)
    {
        return CustomHelpHandler(argc, argv);
    }
    return sShellCustomSubCommands.ExecCommand(argc, argv);
}

CHIP_ERROR CustomExitHandler(int argc, char ** argv)
{
    extern int shell_enable;
    shell_enable = 0;
    return CHIP_NO_ERROR;
}

CHIP_ERROR RegisterCustomEvents()
{
    static const shell_command_t sCustomSubCommands[] = {
        { &CustomHelpHandler, "help", "Usage: custom <subcommand>" },
        { &CustomExitHandler, "exit", "exit from matter shell, will go to custom shell" },
    };

    static const shell_command_t sCustomCommand = { &CustomCommand, "custom", "Exit to custom shell" };

    sShellCustomSubCommands.RegisterCommands(sCustomSubCommands, ArraySize(sCustomSubCommands));
    Engine::Root().RegisterCommands(&sCustomCommand, 1);

    return CHIP_NO_ERROR;
}

/********************************************************
 * ConfigSet shell functions
 *********************************************************/

CHIP_ERROR ConfigSetHelpHandler(int argc, char ** argv)
{
    sShellConfigSetSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigSetCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return ConfigSetHelpHandler(argc, argv);
    }

    return sShellConfigSetSubCommands.ExecCommand(argc, argv);
}

CHIP_ERROR ConfigSetVIDHelpHandler(int argc, char ** argv)
{
    sShellConfigSetVIDSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigSetVIDHandler(int argc, char ** argv)
{

    if (argc == 0)
    {
        return ConfigSetVIDHelpHandler(argc, argv);
    }

    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }

    unsigned long long arg = strtoull(argv[0], nullptr, 10);
    if (!CanCastTo<uint32_t>(arg))
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    uint32_t vendorId = static_cast<uint32_t>(arg);
    ChipLogProgress(Zcl, "%s, vendorId %d", __func__, vendorId);

    CHIP_ERROR error = Rn::rnDeviceDataProvider::GetDeviceDataProvider().SetVendorId(vendorId);
    streamer_printf(streamer_get(), "matter configset vid 0x%x", error);
    return error;
}

CHIP_ERROR ConfigSetPIDHelpHandler(int argc, char ** argv)
{
    sShellConfigSetPIDSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigSetPIDHandler(int argc, char ** argv)
{

    if (argc == 0)
    {
        return ConfigSetPIDHelpHandler(argc, argv);
    }

    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }
    unsigned long long arg = strtoull(argv[0], nullptr, 10);
    if (!CanCastTo<uint16_t>(arg))
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    uint16_t productId = static_cast<uint16_t>(arg);
    ChipLogProgress(Zcl, "%s, Product Id %d", __func__, productId);

    CHIP_ERROR error = Rn::rnDeviceDataProvider::GetDeviceDataProvider().SetProductId(productId);
    streamer_printf(streamer_get(), "matter configset pid 0x%x", error);
    return error;
}

CHIP_ERROR ConfigSetHWVerHelpHandler(int argc, char ** argv)
{
    sShellConfigSetHWVerSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigSetHWVerHandler(int argc, char ** argv)
{

    if (argc == 0)
    {
        return ConfigSetHWVerHelpHandler(argc, argv);
    }

    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }

    unsigned long long arg = strtoull(argv[0], nullptr, 10);
    if (!CanCastTo<uint16_t>(arg))
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    uint16_t hwver = static_cast<uint16_t>(arg);
    ChipLogProgress(Zcl, "%s, HW Version %d", __func__, hwver);

    CHIP_ERROR error = Rn::rnDeviceDataProvider::GetDeviceDataProvider().SetHardwareVersion(hwver);
    streamer_printf(streamer_get(), "matter configset hwver 0x%x", error);
    return error;
}

CHIP_ERROR ConfigSetPinCodeHelpHandler(int argc, char ** argv)
{
    sShellConfigSetPinCodeSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigSetPinCodeHandler(int argc, char ** argv)
{

    if (argc == 0)
    {
        return ConfigSetPinCodeHelpHandler(argc, argv);
    }

    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }

    unsigned long long arg = strtoull(argv[0], nullptr, 10);
    if (!CanCastTo<uint32_t>(arg))
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    uint32_t setupPinCode = static_cast<uint32_t>(arg);
    ChipLogProgress(Zcl, "%s, pincode %d", __func__, setupPinCode);

    CHIP_ERROR error = DeviceLayer::GetCommissionableDataProvider()->SetSetupPasscode(setupPinCode);
    streamer_printf(streamer_get(), "matter configset pincode 0x%x", error);
    return error;
}

CHIP_ERROR ConfigSetDiscriminatorHelpHandler(int argc, char ** argv)
{
    sShellConfigSetDiscriminatorSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR ConfigSetDiscriminatorHandler(int argc, char ** argv)
{

    if (argc == 0)
    {
        return ConfigSetDiscriminatorHelpHandler(argc, argv);
    }

    if (argc >= 2)
    {
        ChipLogError(Zcl, "Too many arguments provided to function %s, line %d", __func__, __LINE__);
        return APP_ERROR_TOO_MANY_SHELL_ARGUMENTS;
    }

    unsigned long long arg = strtoull(argv[0], nullptr, 10);
    if (!CanCastTo<uint16_t>(arg))
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    uint16_t setupDiscriminator = static_cast<uint16_t>(arg);
    ChipLogProgress(Zcl, "%s, disc %d", __func__, setupDiscriminator);

    CHIP_ERROR error = DeviceLayer::GetCommissionableDataProvider()->SetSetupDiscriminator(setupDiscriminator);
    streamer_printf(streamer_get(), "matter configset disc 0x%x", error);
    return error;
}


CHIP_ERROR RegisterConfigSetEvents()
{
    static const shell_command_t sConfigSetSubCommands[] = {
        { &ConfigSetHelpHandler, "help", "Usage : cfgset <subcommand>" },
        { &ConfigSetVIDHandler, "vid", "Set Vendor Id" },
        { &ConfigSetPIDHandler, "pid", "Set Product Id" },
        { &ConfigSetHWVerHandler, "hwver", "Set HardwareVersion" },
        { &ConfigSetPinCodeHandler, "pincode", "Set PinCode" },
        { &ConfigSetDiscriminatorHandler, "disc", "Set Discriminator" }
    };

    static const shell_command_t sConfigSetVIDSubCommands[] = { { &ConfigSetVIDHelpHandler, "help",
                                                                        "Usage : config vid VendorId value" } };
    static const shell_command_t sConfigSetPIDSubCommands[] = { { &ConfigSetVIDHelpHandler, "help",
                                                                        "Usage : config pid ProductId value" } };
    static const shell_command_t sConfigSetHWVerSubCommands[] = { { &ConfigSetVIDHelpHandler, "help",
                                                                        "Usage : config hwver HardwareVersion value" } };
    static const shell_command_t sConfigSetPinCodeSubCommands[] = { { &ConfigSetVIDHelpHandler, "help",
                                                                        "Usage : config pincode PinCode value" } };
    static const shell_command_t sConfigSetDiscriminatorSubCommands[] = { { &ConfigSetVIDHelpHandler, "help",
                                                                        "Usage : config disc Discriminator value" } };

    static const shell_command_t sConfigSetCommand = { &ConfigSetCommandHandler, "configset",
                                                      "configset commands. Usage: config <subcommand>" };

    sShellConfigSetVIDSubCommands.RegisterCommands(sConfigSetVIDSubCommands, ArraySize(sConfigSetVIDSubCommands));
    sShellConfigSetPIDSubCommands.RegisterCommands(sConfigSetPIDSubCommands, ArraySize(sConfigSetPIDSubCommands));
    sShellConfigSetHWVerSubCommands.RegisterCommands(sConfigSetHWVerSubCommands, ArraySize(sConfigSetHWVerSubCommands));
    sShellConfigSetPinCodeSubCommands.RegisterCommands(sConfigSetPinCodeSubCommands, ArraySize(sConfigSetPinCodeSubCommands));
    sShellConfigSetDiscriminatorSubCommands.RegisterCommands(sConfigSetDiscriminatorSubCommands, ArraySize(sConfigSetDiscriminatorSubCommands));

    sShellConfigSetSubCommands.RegisterCommands(sConfigSetSubCommands, ArraySize(sConfigSetSubCommands));

    Engine::Root().RegisterCommands(&sConfigSetCommand, 1);

    return CHIP_NO_ERROR;
}

CHIP_ERROR DeviceFactoryCommandHandler(int argc, char ** argv)
{
	factory_reset(1);

    return CHIP_NO_ERROR;
}

CHIP_ERROR RegisterDeviceFactoryEvents()
{
    static const shell_command_t sConfigSetCommand = { &DeviceFactoryCommandHandler, "factory",
                                                      "nvram factory commands. Usage: factory <no subcommand>" };

    Engine::Root().RegisterCommands(&sConfigSetCommand, 1);

    return CHIP_NO_ERROR;
}

void EventWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "EventWorkerFunction - Invalid work data"));

    EventData * data = reinterpret_cast<EventData *>(context);

    switch (data->eventId)
    {
    case Events::DoorLockAlarm::Id: {
        AlarmEventData * alarmData = reinterpret_cast<AlarmEventData *>(context);
        DoorLockServer::Instance().SendLockAlarmEvent(lockEndpoint, alarmData->alarmCode);
        break;
    }

    case Events::DoorStateChange::Id: {
        DoorStateEventData * doorStateData = reinterpret_cast<DoorStateEventData *>(context);
        if (doorStateData->doorState == DoorStateEnum::kDoorOpen)
            AppTask::GetAppTask().ActionRequest(AppEvent::kEventType_Button, LockManager::UNLOCK_ACTION);
        else
            AppTask::GetAppTask().ActionRequest(AppEvent::kEventType_Button, LockManager::LOCK_ACTION);
        DoorLockServer::Instance().SetDoorState(lockEndpoint, doorStateData->doorState);
        break;
    }

    default: {
        ChipLogError(Zcl, "Invalid Event Id %s, line %d", __func__, __LINE__);
        break;
    }
    }
}

#endif //ENABLE_CHIP_SHELL

#if defined (__MATTER_CLI_DEBUG__)

#ifdef __cplusplus
extern "C" {
#endif

bool send_alarm_event(int argc, const char **argv)
{
    AlarmCodeEnum alarmCode;

    if (argc != 2)
    {
        printf("Please provide correct arguments\n");
        return pdFALSE;
    }
    alarmCode = static_cast<AlarmCodeEnum>(atoi(argv[1]));
    DoorLockServer::Instance().SendLockAlarmEvent(lockEndpoint, alarmCode);
    return pdTRUE;
}

bool send_door_state_event(int argc, const char **argv)
{
    DoorStateEnum doorState;

    if (argc != 2)
    {
        printf("Please provide correct arguments\n");
        return pdFALSE;
    }
    doorState = static_cast<DoorStateEnum>(atoi(argv[1]));
    if (doorState == DoorStateEnum::kDoorOpen)
        AppTask::GetAppTask().ActionRequest(AppEvent::kEventType_Button, LockManager::UNLOCK_ACTION);
    else
        AppTask::GetAppTask().ActionRequest(AppEvent::kEventType_Button, LockManager::LOCK_ACTION);
    DoorLockServer::Instance().SetDoorState(lockEndpoint, doorState);
    return pdTRUE;
}

#ifdef __cplusplus
}
#endif

#endif //__MATTER_CLI_DEBUG__
