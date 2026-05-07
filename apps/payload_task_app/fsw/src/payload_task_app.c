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

#include "payload_task_app.h"
#include "payload_task_app_cmds.h"
#include "payload_task_app_utils.h"
#include "payload_task_app_eventids.h"
#include "payload_task_app_dispatch.h"
#include "payload_task_app_tbl.h"
#include "payload_task_app_version.h"

PAYLOAD_TASK_APP_Data_t PAYLOAD_TASK_APP_Data;

#define PAYLOAD_TASK_APP_TIMEOUT_MSEC 1000
#define PAYLOAD_TASK_APP_WINDOW_DEFAULT_PERIOD_SEC   8U
#define PAYLOAD_TASK_APP_WINDOW_DEFAULT_OPEN_LEN_SEC 2U

static uint64 PAYLOAD_TASK_APP_WindowEpochSec = 0;

static uint64 PAYLOAD_TASK_APP_GetMonotonicSec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64)ts.tv_sec;
}

static uint32 PAYLOAD_TASK_APP_ParseEnvU32(const char *Name, uint32 DefaultValue)
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

static bool PAYLOAD_TASK_APP_IsWindowEnabled(void)
{
    const char *mode   = getenv("PAYLOAD_TASK_APP_WINDOW_PROFILE");
    const char *active = getenv("PAYLOAD_TASK_APP_WINDOW_ACTIVE");

    return (mode != NULL && strcmp(mode, "periodic") == 0 && active != NULL && strcmp(active, "1") == 0);
}

static bool PAYLOAD_TASK_APP_IsActionLoggingEnabled(void)
{
    const char *mode = getenv("PAYLOAD_TASK_APP_ACTION_LOGGING");
    return (mode != NULL && strcmp(mode, "1") == 0);
}

void PAYLOAD_TASK_APP_ResetWindowState(void)
{
    PAYLOAD_TASK_APP_WindowEpochSec = PAYLOAD_TASK_APP_GetMonotonicSec();
}

static bool PAYLOAD_TASK_APP_SchedulerWindowOpen(void)
{
    uint32 period_sec;
    uint32 phase_sec;
    uint32 open_len_sec;
    uint32 elapsed_sec;

    if (!PAYLOAD_TASK_APP_IsWindowEnabled())
    {
        return true;
    }

    if (PAYLOAD_TASK_APP_WindowEpochSec == 0)
    {
        PAYLOAD_TASK_APP_ResetWindowState();
    }

    period_sec = PAYLOAD_TASK_APP_ParseEnvU32("PAYLOAD_TASK_APP_WINDOW_PERIOD_SEC", PAYLOAD_TASK_APP_WINDOW_DEFAULT_PERIOD_SEC);
    phase_sec = PAYLOAD_TASK_APP_ParseEnvU32("PAYLOAD_TASK_APP_WINDOW_PHASE_SEC", 0U);
    open_len_sec = PAYLOAD_TASK_APP_ParseEnvU32("PAYLOAD_TASK_APP_WINDOW_OPEN_LEN_SEC", PAYLOAD_TASK_APP_WINDOW_DEFAULT_OPEN_LEN_SEC);
    elapsed_sec = (uint32)(PAYLOAD_TASK_APP_GetMonotonicSec() - PAYLOAD_TASK_APP_WindowEpochSec);

    if (period_sec == 0U || open_len_sec == 0U)
    {
        return false;
    }

    return (((elapsed_sec + period_sec - phase_sec) % period_sec) < open_len_sec);
}

static void PAYLOAD_TASK_APP_RecordScheduledAction(void)
{
    if (!PAYLOAD_TASK_APP_IsActionLoggingEnabled())
    {
        return;
    }

    PAYLOAD_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount++;
    CFE_EVS_SendEvent(PAYLOAD_TASK_APP_VULN_HIT_EID, CFE_EVS_EventType_INFORMATION,
                      "PAYLOAD_TASK_APP: scheduled task action recorded (%lu)",
                      (unsigned long)PAYLOAD_TASK_APP_Data.HkTlm.Payload.ScheduledActionCount);
}

CFE_Status_t PAYLOAD_TASK_APP_ProcessTick(void)
{
    if (!PAYLOAD_TASK_APP_SchedulerWindowOpen())
    {
        ++PAYLOAD_TASK_APP_Data.ErrCounter;
        CFE_EVS_SendEvent(PAYLOAD_TASK_APP_WINDOW_REJECT_EID, CFE_EVS_EventType_INFORMATION,
                          "PAYLOAD_TASK_APP: tick deferred by scheduler window");
        PAYLOAD_TASK_APP_SendHkCmd(NULL);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    PAYLOAD_TASK_APP_Data.HkTlm.Payload.TickCount += PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep;
    PAYLOAD_TASK_APP_RecordScheduledAction();
    PAYLOAD_TASK_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

void PAYLOAD_TASK_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(PAYLOAD_TASK_APP_PERF_ID);

    status = PAYLOAD_TASK_APP_Init();
    if (status != CFE_SUCCESS)
    {
        PAYLOAD_TASK_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&PAYLOAD_TASK_APP_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(PAYLOAD_TASK_APP_PERF_ID);

        status = CFE_SB_ReceiveBuffer(&SBBufPtr, PAYLOAD_TASK_APP_Data.CommandPipe, PAYLOAD_TASK_APP_TIMEOUT_MSEC);

        CFE_ES_PerfLogEntry(PAYLOAD_TASK_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            PAYLOAD_TASK_APP_TaskPipe(SBBufPtr);
        }
        else if (status == CFE_SB_TIME_OUT)
        {
            PAYLOAD_TASK_APP_ProcessTick();
        }
        else
        {
            CFE_EVS_SendEvent(PAYLOAD_TASK_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "COUNTER APP: SB Pipe Read Error, App Will Exit");

            PAYLOAD_TASK_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(PAYLOAD_TASK_APP_PERF_ID);

    CFE_ES_ExitApp(PAYLOAD_TASK_APP_Data.RunStatus);
}

CFE_Status_t PAYLOAD_TASK_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[PAYLOAD_TASK_APP_CFG_MAX_VERSION_STR_LEN];

    memset(&PAYLOAD_TASK_APP_Data, 0, sizeof(PAYLOAD_TASK_APP_Data));

    PAYLOAD_TASK_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Periodic Task App: Error Registering Events, RC = 0x%08lX\n",
                             (unsigned long)status);
    }
    else
    {
        CFE_MSG_Init(CFE_MSG_PTR(PAYLOAD_TASK_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(PAYLOAD_TASK_APP_HK_TLM_MID),
                     sizeof(PAYLOAD_TASK_APP_Data.HkTlm));
        PAYLOAD_TASK_APP_Data.HkTlm.Payload.UpdateStep = 1;
        PAYLOAD_TASK_APP_ResetWindowState();

        status = CFE_SB_CreatePipe(&PAYLOAD_TASK_APP_Data.CommandPipe, PAYLOAD_TASK_APP_PLATFORM_PIPE_DEPTH,
                                   PAYLOAD_TASK_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(PAYLOAD_TASK_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Periodic Task App: Error creating SB Command Pipe, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(PAYLOAD_TASK_APP_SEND_HK_MID), PAYLOAD_TASK_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(PAYLOAD_TASK_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Periodic Task App: Error Subscribing to HK request, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(PAYLOAD_TASK_APP_CMD_MID), PAYLOAD_TASK_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(PAYLOAD_TASK_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Periodic Task App: Error Subscribing to Commands, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_Register(&PAYLOAD_TASK_APP_Data.TblHandles[0], "RateProfileTable",
                                  sizeof(PAYLOAD_TASK_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, PAYLOAD_TASK_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(PAYLOAD_TASK_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Periodic Task App: Error Registering Rate Profile Table, RC = 0x%08lX",
                              (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(PAYLOAD_TASK_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, PAYLOAD_TASK_APP_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, PAYLOAD_TASK_APP_CFG_MAX_VERSION_STR_LEN, "Periodic Task App",
                                    PAYLOAD_TASK_APP_VERSION,
                                    PAYLOAD_TASK_APP_BUILD_CODENAME, PAYLOAD_TASK_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(PAYLOAD_TASK_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "Periodic Task App initialized.%s", VersionString);
    }

    return status;
}
