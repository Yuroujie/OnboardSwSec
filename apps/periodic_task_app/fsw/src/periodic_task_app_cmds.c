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

    CFE_EVS_SendEvent(PERIODIC_TASK_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "PERIODIC_TASK_APP: state reset command");

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
