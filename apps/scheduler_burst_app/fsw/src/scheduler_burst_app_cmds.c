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

#include "scheduler_burst_app.h"
#include "scheduler_burst_app_cmds.h"
#include "scheduler_burst_app_msgids.h"
#include "scheduler_burst_app_eventids.h"
#include "scheduler_burst_app_version.h"
#include "scheduler_burst_app_tbl.h"
#include "scheduler_burst_app_utils.h"
#include "scheduler_burst_app_msg.h"

#include "sample_lib.h"

#include <string.h>


static uint32 SCHEDULER_BURST_APP_UpdateBurstSpacingWindow(uint16 Count, uint16 SpacingMsec)
{
    uint8  pattern[64];
    uint8  burst_window[32];
    uint32 pressure;
    uint32 copy_len;
    uint32 i;
    uint32 accumulator = 0;

    for (i = 0; i < sizeof(pattern); ++i)
    {
        pattern[i] = (uint8)((Count + SpacingMsec + i) & 0xFFU);
    }

    pressure = (uint32)Count * (uint32)(200U - SpacingMsec);
    copy_len = pressure / 8U;
    if (copy_len > sizeof(pattern))
    {
        copy_len = sizeof(pattern);
    }
    if (SpacingMsec < 200U)
    {
        accumulator ^= copy_len;
    }

    /* Copy burst profile into the local scheduling window. */
    memcpy(burst_window, pattern, copy_len);

    for (i = 0; i < copy_len && i < sizeof(burst_window); ++i)
    {
        accumulator += burst_window[i];
    }

    return accumulator;
}

#define SCHEDULER_BURST_APP_FAST_COMMAND_MSEC 200U
#define SCHEDULER_BURST_APP_MAX_BURST_COUNT   32U

CFE_Status_t SCHEDULER_BURST_APP_SendHkCmd(const SCHEDULER_BURST_APP_SendHkCmd_t *Msg)
{
    int i;

    SCHEDULER_BURST_APP_Data.HkTlm.Payload.CommandErrorCounter = SCHEDULER_BURST_APP_Data.ErrCounter;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.CommandCounter      = SCHEDULER_BURST_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(SCHEDULER_BURST_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(SCHEDULER_BURST_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < SCHEDULER_BURST_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(SCHEDULER_BURST_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t SCHEDULER_BURST_APP_NoopCmd(const SCHEDULER_BURST_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    SCHEDULER_BURST_APP_Data.CmdCounter++;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastCommand = SCHEDULER_BURST_APP_NOOP_CC;

    CFE_EVS_SendEvent(SCHEDULER_BURST_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "SCHEDULER_BURST_APP: NOOP command accepted (%s)", SCHEDULER_BURST_APP_VERSION);

    SCHEDULER_BURST_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t SCHEDULER_BURST_APP_ResetCountersCmd(const SCHEDULER_BURST_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    SCHEDULER_BURST_APP_ResetWindowState();
    SCHEDULER_BURST_APP_Data.CmdCounter = 0;
    SCHEDULER_BURST_APP_Data.ErrCounter = 0;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.UpdateStep           = 1;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastCommand          = SCHEDULER_BURST_APP_RESET_STATE_CC;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.TickCount            = 0;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.ScheduledActionCount = 0;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.SimTrafficCount      = 0;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.BurstCommandCount    = 0;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastSimMsgId         = 0;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastSimCmdCode       = 0;

    CFE_EVS_SendEvent(SCHEDULER_BURST_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "SCHEDULER_BURST_APP: state reset command");

    SCHEDULER_BURST_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t SCHEDULER_BURST_APP_SimTrafficCmd(const SCHEDULER_BURST_APP_SimTrafficCmd_t *Msg)
{
    SCHEDULER_BURST_APP_Data.CmdCounter++;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastCommand = SCHEDULER_BURST_APP_SIM_TRAFFIC_CC;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.SimTrafficCount++;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastSimMsgId = Msg->Payload.MsgId;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastSimCmdCode = Msg->Payload.CmdCode;

    if (Msg->Payload.DelayMsec <= SCHEDULER_BURST_APP_FAST_COMMAND_MSEC)
    {
        SCHEDULER_BURST_APP_Data.HkTlm.Payload.BurstCommandCount++;
    }

    CFE_EVS_SendEvent(SCHEDULER_BURST_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "SCHEDULER_BURST_APP: simulated traffic msg=%u cc=%u len=%u delay=%u",
                      (unsigned int)Msg->Payload.MsgId, (unsigned int)Msg->Payload.CmdCode,
                      (unsigned int)Msg->Payload.MsgLength, (unsigned int)Msg->Payload.DelayMsec);

    SCHEDULER_BURST_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t SCHEDULER_BURST_APP_RunBurstCmd(const SCHEDULER_BURST_APP_RunBurstCmd_t *Msg)
{
    uint16 i;
    uint16 count = Msg->Payload.Count;

    if (count > SCHEDULER_BURST_APP_MAX_BURST_COUNT)
    {
        count = SCHEDULER_BURST_APP_MAX_BURST_COUNT;
    }

    SCHEDULER_BURST_APP_Data.CmdCounter++;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastCommand = SCHEDULER_BURST_APP_RUN_BURST_CC;
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.ScheduledActionCount +=
        SCHEDULER_BURST_APP_UpdateBurstSpacingWindow(Msg->Payload.Count, Msg->Payload.SpacingMsec) & 0x1U;

    for (i = 0; i < count; ++i)
    {
        SCHEDULER_BURST_APP_Data.HkTlm.Payload.SimTrafficCount++;
        if (Msg->Payload.SpacingMsec <= SCHEDULER_BURST_APP_FAST_COMMAND_MSEC)
        {
            SCHEDULER_BURST_APP_Data.HkTlm.Payload.BurstCommandCount++;
        }

        SCHEDULER_BURST_APP_ProcessTick();
    }

    CFE_EVS_SendEvent(SCHEDULER_BURST_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "SCHEDULER_BURST_APP: simulated burst processed %u commands at %u ms spacing",
                      (unsigned int)count, (unsigned int)Msg->Payload.SpacingMsec);

    SCHEDULER_BURST_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t SCHEDULER_BURST_APP_ProcessCmd(const SCHEDULER_BURST_APP_ProcessCmd_t *Msg)
{
    (void)Msg;

    SCHEDULER_BURST_APP_Data.CmdCounter++;
    if (SCHEDULER_BURST_APP_Data.HkTlm.Payload.UpdateStep < 10)
    {
        SCHEDULER_BURST_APP_Data.HkTlm.Payload.UpdateStep++;
    }
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastCommand = SCHEDULER_BURST_APP_INCREASE_STEP_CC;

    CFE_EVS_SendEvent(SCHEDULER_BURST_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "SCHEDULER_BURST_APP: update step increased to %u", SCHEDULER_BURST_APP_Data.HkTlm.Payload.UpdateStep);

    SCHEDULER_BURST_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t SCHEDULER_BURST_APP_DisplayParamCmd(const SCHEDULER_BURST_APP_DisplayParamCmd_t *Msg)
{
    (void)Msg;

    SCHEDULER_BURST_APP_Data.CmdCounter++;
    if (SCHEDULER_BURST_APP_Data.HkTlm.Payload.UpdateStep > 1)
    {
        SCHEDULER_BURST_APP_Data.HkTlm.Payload.UpdateStep--;
    }
    SCHEDULER_BURST_APP_Data.HkTlm.Payload.LastCommand = SCHEDULER_BURST_APP_DECREASE_STEP_CC;

    CFE_EVS_SendEvent(SCHEDULER_BURST_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "SCHEDULER_BURST_APP: update step decreased to %u", SCHEDULER_BURST_APP_Data.HkTlm.Payload.UpdateStep);

    SCHEDULER_BURST_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
