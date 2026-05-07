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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "beacon_task_app.h"
#include "beacon_task_app_cmds.h"
#include "beacon_task_app_utils.h"
#include "beacon_task_app_eventids.h"
#include "beacon_task_app_dispatch.h"
#include "beacon_task_app_tbl.h"
#include "beacon_task_app_version.h"

BEACON_TASK_APP_Data_t BEACON_TASK_APP_Data;

#define BEACON_TASK_APP_TIMEOUT_MSEC 1000
#define BEACON_TASK_APP_WINDOW_DEFAULT_PERIOD_SEC   8U
#define BEACON_TASK_APP_WINDOW_DEFAULT_OPEN_LEN_SEC 2U

static uint64 BEACON_TASK_APP_WindowEpochSec = 0;

static uint64 BEACON_TASK_APP_GetMonotonicSec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64)ts.tv_sec;
}

static uint32 BEACON_TASK_APP_ParseEnvU32(const char *Name, uint32 DefaultValue)
{
    const char *text = getenv(Name);
    char *      end_ptr = NULL;
    unsigned long parsed = 0;

    if (text != NULL && text[0] != '\0')
    {
        parsed = strtoul(text, &end_ptr, 0);
        if (end_ptr != text)
        {
            return (uint32)parsed;
        }
    }

    return DefaultValue;
}

static bool BEACON_TASK_APP_IsWindowEnabled(void)
{
    const char *mode   = getenv("BEACON_TASK_APP_WINDOW_PROFILE");
    const char *active = getenv("BEACON_TASK_APP_WINDOW_ACTIVE");

    return (mode != NULL && strcmp(mode, "periodic") == 0 && active != NULL && strcmp(active, "1") == 0);
}

static bool BEACON_TASK_APP_IsActionLoggingEnabled(void)
{
    const char *mode = getenv("BEACON_TASK_APP_ACTION_LOGGING");
    return (mode != NULL && strcmp(mode, "1") == 0);
}

void BEACON_TASK_APP_ResetWindowState(void)
{
    BEACON_TASK_APP_WindowEpochSec = BEACON_TASK_APP_GetMonotonicSec();
}

static bool BEACON_TASK_APP_SchedulerWindowOpen(void)
{
    uint32 period_sec;
    uint32 phase_sec;
    uint32 open_len_sec;
    uint32 elapsed_sec;

    if (!BEACON_TASK_APP_IsWindowEnabled())
    {
        return true;
    }

    if (BEACON_TASK_APP_WindowEpochSec == 0)
    {
        BEACON_TASK_APP_ResetWindowState();
    }

    period_sec = BEACON_TASK_APP_ParseEnvU32("BEACON_TASK_APP_WINDOW_PERIOD_SEC", BEACON_TASK_APP_WINDOW_DEFAULT_PERIOD_SEC);
    phase_sec = BEACON_TASK_APP_ParseEnvU32("BEACON_TASK_APP_WINDOW_PHASE_SEC", 0U);
    open_len_sec = BEACON_TASK_APP_ParseEnvU32("BEACON_TASK_APP_WINDOW_OPEN_LEN_SEC", BEACON_TASK_APP_WINDOW_DEFAULT_OPEN_LEN_SEC);
    elapsed_sec = (uint32)(BEACON_TASK_APP_GetMonotonicSec() - BEACON_TASK_APP_WindowEpochSec);

    if (period_sec == 0U || open_len_sec == 0U)
    {
        return false;
    }

    return (((elapsed_sec + period_sec - phase_sec) % period_sec) < open_len_sec);
}

static void BEACON_TASK_APP_RecordScheduledAction(void)
{
    if (!BEACON_TASK_APP_IsActionLoggingEnabled())
    {
        return;
    }

    BEACON_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount++;
    CFE_EVS_SendEvent(BEACON_TASK_APP_VULN_HIT_EID, CFE_EVS_EventType_INFORMATION,
                      "BEACON_TASK_APP: scheduled task action recorded (%lu)",
                      (unsigned long)BEACON_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount);
}

CFE_Status_t BEACON_TASK_APP_ProcessTick(void)
{
    if (!BEACON_TASK_APP_SchedulerWindowOpen())
    {
        ++BEACON_TASK_APP_Data.ErrCounter;
        CFE_EVS_SendEvent(BEACON_TASK_APP_WINDOW_REJECT_EID, CFE_EVS_EventType_INFORMATION,
                          "BEACON_TASK_APP: tick deferred by scheduler window");
        BEACON_TASK_APP_SendHkCmd(NULL);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    BEACON_TASK_APP_Data.HkTlm.Payload.TickCount += BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep;
    BEACON_TASK_APP_RecordScheduledAction();
    BEACON_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

void BEACON_TASK_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(BEACON_TASK_APP_PERF_ID);

    status = BEACON_TASK_APP_Init();
    if (status != CFE_SUCCESS)
    {
        BEACON_TASK_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&BEACON_TASK_APP_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(BEACON_TASK_APP_PERF_ID);

        status = CFE_SB_ReceiveBuffer(&SBBufPtr, BEACON_TASK_APP_Data.CommandPipe, BEACON_TASK_APP_TIMEOUT_MSEC);

        CFE_ES_PerfLogEntry(BEACON_TASK_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            BEACON_TASK_APP_TaskPipe(SBBufPtr);
        }
        else if (status == CFE_SB_TIME_OUT)
        {
            BEACON_TASK_APP_ProcessTick();
        }
        else
        {
            CFE_EVS_SendEvent(BEACON_TASK_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "COUNTER APP: SB Pipe Read Error, App Will Exit");

            BEACON_TASK_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(BEACON_TASK_APP_PERF_ID);

    CFE_ES_ExitApp(BEACON_TASK_APP_Data.RunStatus);
}

CFE_Status_t BEACON_TASK_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[BEACON_TASK_APP_CFG_MAX_VERSION_STR_LEN];

    memset(&BEACON_TASK_APP_Data, 0, sizeof(BEACON_TASK_APP_Data));

    BEACON_TASK_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Periodic Task App: Error Registering Events, RC = 0x%08lX\n",
                             (unsigned long)status);
    }
    else
    {
        CFE_MSG_Init(CFE_MSG_PTR(BEACON_TASK_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(BEACON_TASK_APP_HK_TLM_MID),
                     sizeof(BEACON_TASK_APP_Data.HkTlm));
        BEACON_TASK_APP_Data.HkTlm.Payload.UpdateStep = 1;
        BEACON_TASK_APP_ResetWindowState();

        status = CFE_SB_CreatePipe(&BEACON_TASK_APP_Data.CommandPipe, BEACON_TASK_APP_PLATFORM_PIPE_DEPTH,
                                   BEACON_TASK_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(BEACON_TASK_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Periodic Task App: Error creating SB Command Pipe, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(BEACON_TASK_APP_SEND_HK_MID), BEACON_TASK_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(BEACON_TASK_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Periodic Task App: Error Subscribing to HK request, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(BEACON_TASK_APP_CMD_MID), BEACON_TASK_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(BEACON_TASK_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Periodic Task App: Error Subscribing to Commands, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_Register(&BEACON_TASK_APP_Data.TblHandles[0], "RateProfileTable",
                                  sizeof(BEACON_TASK_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, BEACON_TASK_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(BEACON_TASK_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Periodic Task App: Error Registering Rate Profile Table, RC = 0x%08lX",
                              (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(BEACON_TASK_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, BEACON_TASK_APP_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, BEACON_TASK_APP_CFG_MAX_VERSION_STR_LEN, "Periodic Task App",
                                    BEACON_TASK_APP_VERSION,
                                    BEACON_TASK_APP_BUILD_CODENAME, BEACON_TASK_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(BEACON_TASK_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "Periodic Task App initialized.%s", VersionString);
    }

    return status;
}
