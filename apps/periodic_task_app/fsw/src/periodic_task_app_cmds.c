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

#include "periodic_task_app.h"
#include "periodic_task_app_cmds.h"
#include "periodic_task_app_msgids.h"
#include "periodic_task_app_eventids.h"
#include "periodic_task_app_version.h"
#include "periodic_task_app_tbl.h"
#include "periodic_task_app_utils.h"
#include "periodic_task_app_msg.h"

#include "sample_lib.h"

#include <string.h>

#define PERIODIC_TASK_APP_FAST_COMMAND_MSEC 200U
#define PERIODIC_TASK_APP_MAX_BURST_COUNT   32U

static uint32 PERIODIC_TASK_APP_UpdateBurstLoadWindow(uint16 RequestedCount, uint16 SpacingMsec)
{
    uint8  pattern[64];
    uint8  burst_window[32];
    uint8  replay_window[32];
    uint8  audit_window[16];
    uint32 pressure;
    uint32 copy_len;
    uint32 i;
    uint32 accumulator = 0;

    for (i = 0; i < sizeof(pattern); ++i)
    {
        pattern[i] = (uint8)((RequestedCount + SpacingMsec + i) & 0xFFU);
    }

    pressure = (uint32)RequestedCount * (uint32)(PERIODIC_TASK_APP_FAST_COMMAND_MSEC - SpacingMsec);
    copy_len = pressure / 8U;
    if (copy_len > sizeof(pattern))
    {
        copy_len = sizeof(pattern);
    }

    /* Copy burst profile into local telemetry windows. */
    memcpy(burst_window, pattern, copy_len);
    memmove(replay_window, burst_window, copy_len);
    memcpy(audit_window, replay_window + (SpacingMsec & 0x3U), copy_len);

    for (i = 0; i < copy_len && i < sizeof(burst_window); ++i)
    {
        accumulator += burst_window[i];
        accumulator += replay_window[i & 0x1FU];
        accumulator ^= audit_window[i & 0x0FU];
        if (burst_window[i] > PERIODIC_TASK_APP_FAST_COMMAND_MSEC)
        {
            accumulator ^= (uint32)RequestedCount;
        }
    }

    return accumulator;
}

CFE_Status_t PERIODIC_TASK_APP_SendHkCmd(const PERIODIC_TASK_APP_SendHkCmd_t *Msg)
{
    int i;

    PERIODIC_TASK_APP_Data.HkTlm.Payload.CommandErrorCounter = PERIODIC_TASK_APP_Data.ErrCounter;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.CommandCounter      = PERIODIC_TASK_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(PERIODIC_TASK_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(PERIODIC_TASK_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < PERIODIC_TASK_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(PERIODIC_TASK_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t PERIODIC_TASK_APP_NoopCmd(const PERIODIC_TASK_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    PERIODIC_TASK_APP_Data.CmdCounter++;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastCommand = PERIODIC_TASK_APP_NOOP_CC;

    CFE_EVS_SendEvent(PERIODIC_TASK_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PERIODIC_TASK_APP: NOOP command accepted (%s)", PERIODIC_TASK_APP_VERSION);

    PERIODIC_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PERIODIC_TASK_APP_ResetCountersCmd(const PERIODIC_TASK_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    PERIODIC_TASK_APP_ResetWindowState();
    PERIODIC_TASK_APP_Data.CmdCounter = 0;
    PERIODIC_TASK_APP_Data.ErrCounter = 0;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.UpdateStep           = 1;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastCommand          = PERIODIC_TASK_APP_RESET_STATE_CC;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.TickCount            = 0;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount = 0;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.SimTrafficCount      = 0;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.BurstCommandCount    = 0;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastSimMsgId         = 0;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastSimCmdCode       = 0;

    CFE_EVS_SendEvent(PERIODIC_TASK_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "PERIODIC_TASK_APP: state reset command");

    PERIODIC_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PERIODIC_TASK_APP_SimTrafficCmd(const PERIODIC_TASK_APP_SimTrafficCmd_t *Msg)
{
    PERIODIC_TASK_APP_Data.CmdCounter++;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastCommand = PERIODIC_TASK_APP_SIM_TRAFFIC_CC;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.SimTrafficCount++;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastSimMsgId = Msg->Payload.MsgId;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastSimCmdCode = Msg->Payload.CmdCode;

    if (Msg->Payload.DelayMsec <= PERIODIC_TASK_APP_FAST_COMMAND_MSEC)
    {
        PERIODIC_TASK_APP_Data.HkTlm.Payload.BurstCommandCount++;
    }

    CFE_EVS_SendEvent(PERIODIC_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PERIODIC_TASK_APP: simulated traffic msg=%u cc=%u len=%u delay=%u",
                      (unsigned int)Msg->Payload.MsgId, (unsigned int)Msg->Payload.CmdCode,
                      (unsigned int)Msg->Payload.MsgLength, (unsigned int)Msg->Payload.DelayMsec);

    PERIODIC_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PERIODIC_TASK_APP_RunBurstCmd(const PERIODIC_TASK_APP_RunBurstCmd_t *Msg)
{
    uint16 i;
    uint16 count = Msg->Payload.Count;

    if (count > PERIODIC_TASK_APP_MAX_BURST_COUNT)
    {
        count = PERIODIC_TASK_APP_MAX_BURST_COUNT;
    }

    PERIODIC_TASK_APP_Data.CmdCounter++;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastCommand = PERIODIC_TASK_APP_RUN_BURST_CC;
    PERIODIC_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount +=
        PERIODIC_TASK_APP_UpdateBurstLoadWindow(Msg->Payload.Count, Msg->Payload.SpacingMsec) & 0x1U;

    for (i = 0; i < count; ++i)
    {
        PERIODIC_TASK_APP_Data.HkTlm.Payload.SimTrafficCount++;
        if (Msg->Payload.SpacingMsec <= PERIODIC_TASK_APP_FAST_COMMAND_MSEC)
        {
            PERIODIC_TASK_APP_Data.HkTlm.Payload.BurstCommandCount++;
        }

        PERIODIC_TASK_APP_ProcessTick();
    }

    CFE_EVS_SendEvent(PERIODIC_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PERIODIC_TASK_APP: simulated burst processed %u commands at %u ms spacing",
                      (unsigned int)count, (unsigned int)Msg->Payload.SpacingMsec);

    PERIODIC_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PERIODIC_TASK_APP_ProcessCmd(const PERIODIC_TASK_APP_ProcessCmd_t *Msg)
{
    (void)Msg;

    PERIODIC_TASK_APP_Data.CmdCounter++;
    if (PERIODIC_TASK_APP_Data.HkTlm.Payload.UpdateStep < 10)
    {
        PERIODIC_TASK_APP_Data.HkTlm.Payload.UpdateStep++;
    }
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastCommand = PERIODIC_TASK_APP_INCREASE_STEP_CC;

    CFE_EVS_SendEvent(PERIODIC_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PERIODIC_TASK_APP: update step increased to %u", PERIODIC_TASK_APP_Data.HkTlm.Payload.UpdateStep);

    PERIODIC_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t PERIODIC_TASK_APP_DisplayParamCmd(const PERIODIC_TASK_APP_DisplayParamCmd_t *Msg)
{
    (void)Msg;

    PERIODIC_TASK_APP_Data.CmdCounter++;
    if (PERIODIC_TASK_APP_Data.HkTlm.Payload.UpdateStep > 1)
    {
        PERIODIC_TASK_APP_Data.HkTlm.Payload.UpdateStep--;
    }
    PERIODIC_TASK_APP_Data.HkTlm.Payload.LastCommand = PERIODIC_TASK_APP_DECREASE_STEP_CC;

    CFE_EVS_SendEvent(PERIODIC_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "PERIODIC_TASK_APP: update step decreased to %u", PERIODIC_TASK_APP_Data.HkTlm.Payload.UpdateStep);

    PERIODIC_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
