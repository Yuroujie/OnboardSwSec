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

#include "beacon_task_app.h"
#include "beacon_task_app_cmds.h"
#include "beacon_task_app_msgids.h"
#include "beacon_task_app_eventids.h"
#include "beacon_task_app_version.h"
#include "beacon_task_app_tbl.h"
#include "beacon_task_app_utils.h"
#include "beacon_task_app_msg.h"

#include "sample_lib.h"

#include <string.h>


static uint32 BEACON_TASK_APP_UpdateCadenceDriftState(uint16 CmdCode, uint16 DelayMsec)
{
    uint32 drift;
    uint32 score = 0;

    drift = (uint32)(200U - DelayMsec);
    if (CmdCode > 0U)
    {
        score = drift + CmdCode;
    }

    return score;
}

#define BEACON_TASK_APP_FAST_COMMAND_MSEC 200U
#define BEACON_TASK_APP_MAX_BURST_COUNT   32U

CFE_Status_t BEACON_TASK_APP_SendHkCmd(const BEACON_TASK_APP_SendHkCmd_t *Msg)
{
    int i;

    BEACON_TASK_APP_Data.HkTlm.Payload.CommandErrorCounter = BEACON_TASK_APP_Data.ErrCounter;
    BEACON_TASK_APP_Data.HkTlm.Payload.CommandCounter      = BEACON_TASK_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(BEACON_TASK_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(BEACON_TASK_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < BEACON_TASK_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(BEACON_TASK_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t BEACON_TASK_APP_NoopCmd(const BEACON_TASK_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    BEACON_TASK_APP_Data.CmdCounter++;
    BEACON_TASK_APP_Data.HkTlm.Payload.LastCommand = BEACON_TASK_APP_NOOP_CC;

    CFE_EVS_SendEvent(BEACON_TASK_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BEACON_TASK_APP: NOOP command accepted (%s)", BEACON_TASK_APP_VERSION);

    BEACON_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BEACON_TASK_APP_ResetCountersCmd(const BEACON_TASK_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    BEACON_TASK_APP_ResetWindowState();
    BEACON_TASK_APP_Data.CmdCounter = 0;
    BEACON_TASK_APP_Data.ErrCounter = 0;
    BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep           = 1;
    BEACON_TASK_APP_Data.HkTlm.Payload.LastCommand          = BEACON_TASK_APP_RESET_STATE_CC;
    BEACON_TASK_APP_Data.HkTlm.Payload.TickCount            = 0;
    BEACON_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount = 0;
    BEACON_TASK_APP_Data.HkTlm.Payload.SimTrafficCount      = 0;
    BEACON_TASK_APP_Data.HkTlm.Payload.BurstCommandCount    = 0;
    BEACON_TASK_APP_Data.HkTlm.Payload.LastSimMsgId         = 0;
    BEACON_TASK_APP_Data.HkTlm.Payload.LastSimCmdCode       = 0;

    CFE_EVS_SendEvent(BEACON_TASK_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "BEACON_TASK_APP: state reset command");

    BEACON_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BEACON_TASK_APP_SimTrafficCmd(const BEACON_TASK_APP_SimTrafficCmd_t *Msg)
{
    BEACON_TASK_APP_Data.CmdCounter++;
    BEACON_TASK_APP_Data.HkTlm.Payload.LastCommand = BEACON_TASK_APP_SIM_TRAFFIC_CC;
    BEACON_TASK_APP_Data.HkTlm.Payload.SimTrafficCount++;
    BEACON_TASK_APP_Data.HkTlm.Payload.LastSimMsgId = Msg->Payload.MsgId;
    BEACON_TASK_APP_Data.HkTlm.Payload.LastSimCmdCode = Msg->Payload.CmdCode;
    BEACON_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount +=
        BEACON_TASK_APP_UpdateCadenceDriftState(Msg->Payload.CmdCode, Msg->Payload.DelayMsec) & 0x1U;

    if (Msg->Payload.DelayMsec <= BEACON_TASK_APP_FAST_COMMAND_MSEC)
    {
        BEACON_TASK_APP_Data.HkTlm.Payload.BurstCommandCount++;
    }

    CFE_EVS_SendEvent(BEACON_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BEACON_TASK_APP: simulated traffic msg=%u cc=%u len=%u delay=%u",
                      (unsigned int)Msg->Payload.MsgId, (unsigned int)Msg->Payload.CmdCode,
                      (unsigned int)Msg->Payload.MsgLength, (unsigned int)Msg->Payload.DelayMsec);

    BEACON_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BEACON_TASK_APP_RunBurstCmd(const BEACON_TASK_APP_RunBurstCmd_t *Msg)
{
    uint16 i;
    uint16 count = Msg->Payload.Count;

    if (count > BEACON_TASK_APP_MAX_BURST_COUNT)
    {
        count = BEACON_TASK_APP_MAX_BURST_COUNT;
    }

    BEACON_TASK_APP_Data.CmdCounter++;
    BEACON_TASK_APP_Data.HkTlm.Payload.LastCommand = BEACON_TASK_APP_RUN_BURST_CC;

    for (i = 0; i < count; ++i)
    {
        BEACON_TASK_APP_Data.HkTlm.Payload.SimTrafficCount++;
        if (Msg->Payload.SpacingMsec <= BEACON_TASK_APP_FAST_COMMAND_MSEC)
        {
            BEACON_TASK_APP_Data.HkTlm.Payload.BurstCommandCount++;
        }

        BEACON_TASK_APP_ProcessTick();
    }

    CFE_EVS_SendEvent(BEACON_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BEACON_TASK_APP: simulated burst processed %u commands at %u ms spacing",
                      (unsigned int)count, (unsigned int)Msg->Payload.SpacingMsec);

    BEACON_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BEACON_TASK_APP_ProcessCmd(const BEACON_TASK_APP_ProcessCmd_t *Msg)
{
    (void)Msg;

    BEACON_TASK_APP_Data.CmdCounter++;
    if (BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep < 10)
    {
        BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep++;
    }
    BEACON_TASK_APP_Data.HkTlm.Payload.LastCommand = BEACON_TASK_APP_INCREASE_STEP_CC;

    CFE_EVS_SendEvent(BEACON_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BEACON_TASK_APP: update step increased to %u", BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep);

    BEACON_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BEACON_TASK_APP_DisplayParamCmd(const BEACON_TASK_APP_DisplayParamCmd_t *Msg)
{
    (void)Msg;

    BEACON_TASK_APP_Data.CmdCounter++;
    if (BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep > 1)
    {
        BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep--;
    }
    BEACON_TASK_APP_Data.HkTlm.Payload.LastCommand = BEACON_TASK_APP_DECREASE_STEP_CC;

    CFE_EVS_SendEvent(BEACON_TASK_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BEACON_TASK_APP: update step decreased to %u", BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep);

    BEACON_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
