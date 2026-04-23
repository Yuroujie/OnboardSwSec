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

#include "buffer_mgr_app.h"
#include "buffer_mgr_app_cmds.h"
#include "buffer_mgr_app_utils.h"
#include "buffer_mgr_app_eventids.h"
#include "buffer_mgr_app_dispatch.h"
#include "buffer_mgr_app_tbl.h"
#include "buffer_mgr_app_version.h"

BUFFER_MGR_APP_Data_t BUFFER_MGR_APP_Data;

#define BUFFER_MGR_APP_TIMEOUT_MSEC 1000
#define BUFFER_MGR_APP_DEFAULT_THRESHOLD      10U
#define BUFFER_MGR_APP_DEFAULT_WINDOW_LEN_SEC 5U

static uint64 BUFFER_MGR_APP_WindowEpochSec = 0;
static uint64 BUFFER_MGR_APP_WindowCloseSec = 0;
static uint32 BUFFER_MGR_APP_WindowLenSec   = BUFFER_MGR_APP_DEFAULT_WINDOW_LEN_SEC;

static uint64 BUFFER_MGR_APP_GetMonotonicSec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64)ts.tv_sec;
}

static uint32 BUFFER_MGR_APP_ParseEnvU32(const char *Name, uint32 DefaultValue)
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

static void BUFFER_MGR_APP_LoadConfig(void)
{
    BUFFER_MGR_APP_WindowLenSec =
        BUFFER_MGR_APP_ParseEnvU32("BUFFER_MGR_APP_RELEASE_WINDOW_LEN_SEC", BUFFER_MGR_APP_DEFAULT_WINDOW_LEN_SEC);
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseThreshold =
        BUFFER_MGR_APP_ParseEnvU32("BUFFER_MGR_APP_RELEASE_THRESHOLD", BUFFER_MGR_APP_DEFAULT_THRESHOLD);
}

static bool BUFFER_MGR_APP_IsReleaseActionLoggingEnabled(void)
{
    const char *mode = getenv("BUFFER_MGR_APP_RELEASE_ACTION_LOG");
    return (mode != NULL && strcmp(mode, "1") == 0);
}

void BUFFER_MGR_APP_ResetWindowState(void)
{
    BUFFER_MGR_APP_WindowEpochSec = BUFFER_MGR_APP_GetMonotonicSec();
    BUFFER_MGR_APP_WindowCloseSec = 0;
}

static void BUFFER_MGR_APP_RecordReleaseAction(void)
{
    if (!BUFFER_MGR_APP_IsReleaseActionLoggingEnabled())
    {
        return;
    }

    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseActionCount++;
    CFE_EVS_SendEvent(BUFFER_MGR_APP_VULN_HIT_EID, CFE_EVS_EventType_INFORMATION,
                      "BUFFER_MGR_APP: release action recorded (%lu)",
                      (unsigned long)BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseActionCount);
}

static void BUFFER_MGR_APP_PollStorageWindow(void)
{
    uint64 now_sec = BUFFER_MGR_APP_GetMonotonicSec();

    if (BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseWindowOpen != 0 && BUFFER_MGR_APP_WindowCloseSec != 0 &&
        now_sec > BUFFER_MGR_APP_WindowCloseSec)
    {
        BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
        BUFFER_MGR_APP_WindowCloseSec = 0;
    }

    if (BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseWindowOpen == 0 &&
        BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel >= BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        BUFFER_MGR_APP_ProcessStorageRelease();
    }

    BUFFER_MGR_APP_SendHkCmd(NULL);
}

CFE_Status_t BUFFER_MGR_APP_ProcessStorageRelease(void)
{
    if (BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel < BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel -= BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseThreshold;
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseCount++;
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 1;
    BUFFER_MGR_APP_RecordReleaseAction();
    BUFFER_MGR_APP_WindowCloseSec = BUFFER_MGR_APP_GetMonotonicSec() +
                                  ((BUFFER_MGR_APP_WindowLenSec > 0U) ? (BUFFER_MGR_APP_WindowLenSec - 1U) : 0U);

    CFE_EVS_SendEvent(BUFFER_MGR_APP_WINDOW_OPEN_EID, CFE_EVS_EventType_INFORMATION,
                      "BUFFER_MGR_APP: release window opened for %lu sec", (unsigned long)BUFFER_MGR_APP_WindowLenSec);
    CFE_EVS_SendEvent(BUFFER_MGR_APP_RELEASE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BUFFER_MGR_APP: buffered release executed (remaining=%lu, releases=%lu)",
                      (unsigned long)BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel,
                      (unsigned long)BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseCount);

    return CFE_SUCCESS;
}

void BUFFER_MGR_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(BUFFER_MGR_APP_PERF_ID);

    status = BUFFER_MGR_APP_Init();
    if (status != CFE_SUCCESS)
    {
        BUFFER_MGR_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&BUFFER_MGR_APP_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(BUFFER_MGR_APP_PERF_ID);

        status = CFE_SB_ReceiveBuffer(&SBBufPtr, BUFFER_MGR_APP_Data.CommandPipe, BUFFER_MGR_APP_TIMEOUT_MSEC);

        CFE_ES_PerfLogEntry(BUFFER_MGR_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            BUFFER_MGR_APP_TaskPipe(SBBufPtr);
        }
        else if (status == CFE_SB_TIME_OUT)
        {
            BUFFER_MGR_APP_PollStorageWindow();
        }
        else
        {
            CFE_EVS_SendEvent(BUFFER_MGR_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "BUFFER_MGR_APP: SB Pipe Read Error, App Will Exit");

            BUFFER_MGR_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(BUFFER_MGR_APP_PERF_ID);

    CFE_ES_ExitApp(BUFFER_MGR_APP_Data.RunStatus);
}

CFE_Status_t BUFFER_MGR_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[BUFFER_MGR_APP_CFG_MAX_VERSION_STR_LEN];

    memset(&BUFFER_MGR_APP_Data, 0, sizeof(BUFFER_MGR_APP_Data));

    BUFFER_MGR_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Storage Buffer App: Error Registering Events, RC = 0x%08lX\n",
                             (unsigned long)status);
    }
    else
    {
        CFE_MSG_Init(CFE_MSG_PTR(BUFFER_MGR_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(BUFFER_MGR_APP_HK_TLM_MID),
                     sizeof(BUFFER_MGR_APP_Data.HkTlm));
        BUFFER_MGR_APP_LoadConfig();
        BUFFER_MGR_APP_ResetWindowState();

        status = CFE_SB_CreatePipe(&BUFFER_MGR_APP_Data.CommandPipe, BUFFER_MGR_APP_PLATFORM_PIPE_DEPTH,
                                   BUFFER_MGR_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(BUFFER_MGR_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error creating SB Command Pipe, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(BUFFER_MGR_APP_SEND_HK_MID), BUFFER_MGR_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(BUFFER_MGR_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Subscribing to HK request, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(BUFFER_MGR_APP_CMD_MID), BUFFER_MGR_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(BUFFER_MGR_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Subscribing to Commands, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_Register(&BUFFER_MGR_APP_Data.TblHandles[0], "BufferPolicyTable",
                                  sizeof(BUFFER_MGR_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, BUFFER_MGR_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(BUFFER_MGR_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Registering Buffer Policy Table, RC = 0x%08lX",
                              (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(BUFFER_MGR_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, BUFFER_MGR_APP_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, BUFFER_MGR_APP_CFG_MAX_VERSION_STR_LEN, "Storage Buffer App",
                                    BUFFER_MGR_APP_VERSION,
                                    BUFFER_MGR_APP_BUILD_CODENAME, BUFFER_MGR_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(BUFFER_MGR_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "Storage Buffer App initialized.%s", VersionString);
    }

    return status;
}
