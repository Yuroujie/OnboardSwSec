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

#include "recorder_cache_app.h"
#include "recorder_cache_app_cmds.h"
#include "recorder_cache_app_utils.h"
#include "recorder_cache_app_eventids.h"
#include "recorder_cache_app_dispatch.h"
#include "recorder_cache_app_tbl.h"
#include "recorder_cache_app_version.h"

RECORDER_CACHE_APP_Data_t RECORDER_CACHE_APP_Data;

#define RECORDER_CACHE_APP_TIMEOUT_MSEC 1000
#define RECORDER_CACHE_APP_DEFAULT_THRESHOLD      10U
#define RECORDER_CACHE_APP_DEFAULT_WINDOW_LEN_SEC 5U

static uint64 RECORDER_CACHE_APP_WindowEpochSec = 0;
static uint64 RECORDER_CACHE_APP_WindowCloseSec = 0;
static uint32 RECORDER_CACHE_APP_WindowLenSec   = RECORDER_CACHE_APP_DEFAULT_WINDOW_LEN_SEC;

static uint64 RECORDER_CACHE_APP_GetMonotonicSec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64)ts.tv_sec;
}

static uint32 RECORDER_CACHE_APP_ParseEnvU32(const char *Name, uint32 DefaultValue)
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

static void RECORDER_CACHE_APP_LoadConfig(void)
{
    RECORDER_CACHE_APP_WindowLenSec =
        RECORDER_CACHE_APP_ParseEnvU32("RECORDER_CACHE_APP_RELEASE_WINDOW_LEN_SEC", RECORDER_CACHE_APP_DEFAULT_WINDOW_LEN_SEC);
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseThreshold =
        RECORDER_CACHE_APP_ParseEnvU32("RECORDER_CACHE_APP_RELEASE_THRESHOLD", RECORDER_CACHE_APP_DEFAULT_THRESHOLD);
}

static bool RECORDER_CACHE_APP_IsReleaseActionLoggingEnabled(void)
{
    const char *mode = getenv("RECORDER_CACHE_APP_RELEASE_ACTION_LOG");
    return (mode != NULL && strcmp(mode, "1") == 0);
}

void RECORDER_CACHE_APP_ResetWindowState(void)
{
    RECORDER_CACHE_APP_WindowEpochSec = RECORDER_CACHE_APP_GetMonotonicSec();
    RECORDER_CACHE_APP_WindowCloseSec = 0;
}

static void RECORDER_CACHE_APP_RecordReleaseAction(void)
{
    if (!RECORDER_CACHE_APP_IsReleaseActionLoggingEnabled())
    {
        return;
    }

    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseActionCount++;
    CFE_EVS_SendEvent(RECORDER_CACHE_APP_VULN_HIT_EID, CFE_EVS_EventType_INFORMATION,
                      "RECORDER_CACHE_APP: release action recorded (%lu)",
                      (unsigned long)RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseActionCount);
}

static void RECORDER_CACHE_APP_PollStorageWindow(void)
{
    uint64 now_sec = RECORDER_CACHE_APP_GetMonotonicSec();

    if (RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseWindowOpen != 0 && RECORDER_CACHE_APP_WindowCloseSec != 0 &&
        now_sec > RECORDER_CACHE_APP_WindowCloseSec)
    {
        RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
        RECORDER_CACHE_APP_WindowCloseSec = 0;
    }

    if (RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseWindowOpen == 0 &&
        RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel >= RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        RECORDER_CACHE_APP_ProcessStorageRelease();
    }

    RECORDER_CACHE_APP_SendHkCmd(NULL);
}

CFE_Status_t RECORDER_CACHE_APP_ProcessStorageRelease(void)
{
    if (RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel < RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel -= RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseThreshold;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseCount++;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 1;
    RECORDER_CACHE_APP_RecordReleaseAction();
    RECORDER_CACHE_APP_WindowCloseSec = RECORDER_CACHE_APP_GetMonotonicSec() +
                                  ((RECORDER_CACHE_APP_WindowLenSec > 0U) ? (RECORDER_CACHE_APP_WindowLenSec - 1U) : 0U);

    CFE_EVS_SendEvent(RECORDER_CACHE_APP_WINDOW_OPEN_EID, CFE_EVS_EventType_INFORMATION,
                      "RECORDER_CACHE_APP: release window opened for %lu sec", (unsigned long)RECORDER_CACHE_APP_WindowLenSec);
    CFE_EVS_SendEvent(RECORDER_CACHE_APP_RELEASE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "RECORDER_CACHE_APP: buffered release executed (remaining=%lu, releases=%lu)",
                      (unsigned long)RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel,
                      (unsigned long)RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseCount);

    return CFE_SUCCESS;
}

void RECORDER_CACHE_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(RECORDER_CACHE_APP_PERF_ID);

    status = RECORDER_CACHE_APP_Init();
    if (status != CFE_SUCCESS)
    {
        RECORDER_CACHE_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&RECORDER_CACHE_APP_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(RECORDER_CACHE_APP_PERF_ID);

        status = CFE_SB_ReceiveBuffer(&SBBufPtr, RECORDER_CACHE_APP_Data.CommandPipe, RECORDER_CACHE_APP_TIMEOUT_MSEC);

        CFE_ES_PerfLogEntry(RECORDER_CACHE_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            RECORDER_CACHE_APP_TaskPipe(SBBufPtr);
        }
        else if (status == CFE_SB_TIME_OUT)
        {
            RECORDER_CACHE_APP_PollStorageWindow();
        }
        else
        {
            CFE_EVS_SendEvent(RECORDER_CACHE_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "RECORDER_CACHE_APP: SB Pipe Read Error, App Will Exit");

            RECORDER_CACHE_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(RECORDER_CACHE_APP_PERF_ID);

    CFE_ES_ExitApp(RECORDER_CACHE_APP_Data.RunStatus);
}

CFE_Status_t RECORDER_CACHE_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[RECORDER_CACHE_APP_CFG_MAX_VERSION_STR_LEN];

    memset(&RECORDER_CACHE_APP_Data, 0, sizeof(RECORDER_CACHE_APP_Data));

    RECORDER_CACHE_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Storage Buffer App: Error Registering Events, RC = 0x%08lX\n",
                             (unsigned long)status);
    }
    else
    {
        CFE_MSG_Init(CFE_MSG_PTR(RECORDER_CACHE_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(RECORDER_CACHE_APP_HK_TLM_MID),
                     sizeof(RECORDER_CACHE_APP_Data.HkTlm));
        RECORDER_CACHE_APP_LoadConfig();
        RECORDER_CACHE_APP_ResetWindowState();

        status = CFE_SB_CreatePipe(&RECORDER_CACHE_APP_Data.CommandPipe, RECORDER_CACHE_APP_PLATFORM_PIPE_DEPTH,
                                   RECORDER_CACHE_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(RECORDER_CACHE_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error creating SB Command Pipe, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(RECORDER_CACHE_APP_SEND_HK_MID), RECORDER_CACHE_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(RECORDER_CACHE_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Subscribing to HK request, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(RECORDER_CACHE_APP_CMD_MID), RECORDER_CACHE_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(RECORDER_CACHE_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Subscribing to Commands, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_Register(&RECORDER_CACHE_APP_Data.TblHandles[0], "BufferPolicyTable",
                                  sizeof(RECORDER_CACHE_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, RECORDER_CACHE_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(RECORDER_CACHE_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Registering Buffer Policy Table, RC = 0x%08lX",
                              (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(RECORDER_CACHE_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, RECORDER_CACHE_APP_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, RECORDER_CACHE_APP_CFG_MAX_VERSION_STR_LEN, "Storage Buffer App",
                                    RECORDER_CACHE_APP_VERSION,
                                    RECORDER_CACHE_APP_BUILD_CODENAME, RECORDER_CACHE_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(RECORDER_CACHE_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "Storage Buffer App initialized.%s", VersionString);
    }

    return status;
}
