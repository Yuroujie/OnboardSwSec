/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

#include "payload_task_app.h"
#include "payload_task_app_cmds.h"
#include "payload_task_app_msgids.h"
#include "payload_task_app_eventids.h"
#include "payload_task_app_version.h"
#include "payload_task_app_tbl.h"
#include "payload_task_app_utils.h"
#include "payload_task_app_msg.h"

#include "sample_lib.h"

#include <string.h>

#define PAYLOAD_TASK_APP_FAST_COMMAND_MSEC 200U
#define PAYLOAD_TASK_APP_MAX_BURST_COUNT   32U

CFE_Status_t PAYLOAD_TASK_APP_SendHkCmd(const PAYLOAD_TASK_APP_SendHkCmd_t *Msg)
{
    int i;

    PAYLOAD_TASK_APP_Data.HkTlm.Payload.CommandErrorCounter = PAYLOAD_TASK_APP_Data.ErrCounter;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.CommandCounter      = PAYLOAD_TASK_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(PAYLOAD_TASK_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(PAYLOAD_TASK_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < PAYLOAD_TASK_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(PAYLOAD_TASK_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t PAYLOAD_TASK_APP_NoopCmd(const PAYLOAD_TASK_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    PAYLOAD_TASK_APP_Data.CmdCounter++;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastCommand = PAYLOAD_TASK_APP_NOOP_CC;

    CFE_EVS_SendEvent(PAYLOAD_TASK_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PAYLOAD_TASK_APP: NOOP command accepted (%s)", PAYLOAD_TASK_APP_VERSION);

    PAYLOAD_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PAYLOAD_TASK_APP_ResetCountersCmd(const PAYLOAD_TASK_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    PAYLOAD_TASK_APP_ResetWindowState();
    PAYLOAD_TASK_APP_Data.CmdCounter = 0;
    PAYLOAD_TASK_APP_Data.ErrCounter = 0;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep           = 1;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastCommand          = PAYLOAD_TASK_APP_RESET_STATE_CC;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.TickCount            = 0;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount = 0;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.SimTrafficCount      = 0;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.BurstCommandCount    = 0;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastSimMsgId         = 0;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastSimCmdCode       = 0;

    CFE_EVS_SendEvent(PAYLOAD_TASK_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "PAYLOAD_TASK_APP: state reset command");

    PAYLOAD_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PAYLOAD_TASK_APP_SimTrafficCmd(const PAYLOAD_TASK_APP_SimTrafficCmd_t *Msg)
{
    PAYLOAD_TASK_APP_Data.CmdCounter++;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastCommand = PAYLOAD_TASK_APP_SIM_TRAFFIC_CC;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.SimTrafficCount++;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastSimMsgId = Msg->Payload.MsgId;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastSimCmdCode = Msg->Payload.CmdCode;

    if (Msg->Payload.DelayMsec <= PAYLOAD_TASK_APP_FAST_COMMAND_MSEC)
    {
        PAYLOAD_TASK_APP_Data.HkTlm.Payload.BurstCommandCount++;
    }

    CFE_EVS_SendEvent(PAYLOAD_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PAYLOAD_TASK_APP: simulated traffic msg=%u cc=%u len=%u delay=%u",
                      (unsigned int)Msg->Payload.MsgId, (unsigned int)Msg->Payload.CmdCode,
                      (unsigned int)Msg->Payload.MsgLength, (unsigned int)Msg->Payload.DelayMsec);

    PAYLOAD_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PAYLOAD_TASK_APP_RunBurstCmd(const PAYLOAD_TASK_APP_RunBurstCmd_t *Msg)
{
    uint16 i;
    uint16 count = Msg->Payload.Count;

    if (count > PAYLOAD_TASK_APP_MAX_BURST_COUNT)
    {
        count = PAYLOAD_TASK_APP_MAX_BURST_COUNT;
    }

    PAYLOAD_TASK_APP_Data.CmdCounter++;
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastCommand = PAYLOAD_TASK_APP_RUN_BURST_CC;

    for (i = 0; i < count; ++i)
    {
        PAYLOAD_TASK_APP_Data.HkTlm.Payload.SimTrafficCount++;
        if (Msg->Payload.SpacingMsec <= PAYLOAD_TASK_APP_FAST_COMMAND_MSEC)
        {
            PAYLOAD_TASK_APP_Data.HkTlm.Payload.BurstCommandCount++;
        }

        PAYLOAD_TASK_APP_ProcessTick();
    }

    CFE_EVS_SendEvent(PAYLOAD_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PAYLOAD_TASK_APP: simulated burst processed %u commands at %u ms spacing",
                      (unsigned int)count, (unsigned int)Msg->Payload.SpacingMsec);

    PAYLOAD_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PAYLOAD_TASK_APP_ProcessCmd(const PAYLOAD_TASK_APP_ProcessCmd_t *Msg)
{
    (void)Msg;

    PAYLOAD_TASK_APP_Data.CmdCounter++;
    if (PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep < 10)
    {
        PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep++;
    }
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastCommand = PAYLOAD_TASK_APP_INCREASE_STEP_CC;

    CFE_EVS_SendEvent(PAYLOAD_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PAYLOAD_TASK_APP: update step increased to %u", PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep);

    PAYLOAD_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PAYLOAD_TASK_APP_DisplayParamCmd(const PAYLOAD_TASK_APP_DisplayParamCmd_t *Msg)
{
    (void)Msg;

    PAYLOAD_TASK_APP_Data.CmdCounter++;
    if (PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep > 1)
    {
        PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep--;
    }
    PAYLOAD_TASK_APP_Data.HkTlm.Payload.LastCommand = PAYLOAD_TASK_APP_DECREASE_STEP_CC;

    CFE_EVS_SendEvent(PAYLOAD_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PAYLOAD_TASK_APP: update step decreased to %u", PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep);

    PAYLOAD_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
