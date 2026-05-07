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

#include "storage_audit_app.h"
#include "storage_audit_app_cmds.h"
#include "storage_audit_app_utils.h"
#include "storage_audit_app_eventids.h"
#include "storage_audit_app_dispatch.h"
#include "storage_audit_app_tbl.h"
#include "storage_audit_app_version.h"

STORAGE_AUDIT_APP_Data_t STORAGE_AUDIT_APP_Data;

#define STORAGE_AUDIT_APP_TIMEOUT_MSEC 1000
#define STORAGE_AUDIT_APP_DEFAULT_THRESHOLD      10U
#define STORAGE_AUDIT_APP_DEFAULT_WINDOW_LEN_SEC 5U

static uint64 STORAGE_AUDIT_APP_WindowEpochSec = 0;
static uint64 STORAGE_AUDIT_APP_WindowCloseSec = 0;
static uint32 STORAGE_AUDIT_APP_WindowLenSec   = STORAGE_AUDIT_APP_DEFAULT_WINDOW_LEN_SEC;

static uint64 STORAGE_AUDIT_APP_GetMonotonicSec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64)ts.tv_sec;
}

static uint32 STORAGE_AUDIT_APP_ParseEnvU32(const char *Name, uint32 DefaultValue)
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

static void STORAGE_AUDIT_APP_LoadConfig(void)
{
    STORAGE_AUDIT_APP_WindowLenSec =
        STORAGE_AUDIT_APP_ParseEnvU32("STORAGE_AUDIT_APP_RELEASE_WINDOW_LEN_SEC", STORAGE_AUDIT_APP_DEFAULT_WINDOW_LEN_SEC);
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseThreshold =
        STORAGE_AUDIT_APP_ParseEnvU32("STORAGE_AUDIT_APP_RELEASE_THRESHOLD", STORAGE_AUDIT_APP_DEFAULT_THRESHOLD);
}

static bool STORAGE_AUDIT_APP_IsReleaseActionLoggingEnabled(void)
{
    const char *mode = getenv("STORAGE_AUDIT_APP_RELEASE_ACTION_LOG");
    return (mode != NULL && strcmp(mode, "1") == 0);
}

void STORAGE_AUDIT_APP_ResetWindowState(void)
{
    STORAGE_AUDIT_APP_WindowEpochSec = STORAGE_AUDIT_APP_GetMonotonicSec();
    STORAGE_AUDIT_APP_WindowCloseSec = 0;
}

static void STORAGE_AUDIT_APP_RecordReleaseAction(void)
{
    if (!STORAGE_AUDIT_APP_IsReleaseActionLoggingEnabled())
    {
        return;
    }

    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseActionCount++;
    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_VULN_HIT_EID, CFE_EVS_EventType_INFORMATION,
                      "STORAGE_AUDIT_APP: release action recorded (%lu)",
                      (unsigned long)STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseActionCount);
}

static void STORAGE_AUDIT_APP_PollStorageWindow(void)
{
    uint64 now_sec = STORAGE_AUDIT_APP_GetMonotonicSec();

    if (STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseWindowOpen != 0 && STORAGE_AUDIT_APP_WindowCloseSec != 0 &&
        now_sec > STORAGE_AUDIT_APP_WindowCloseSec)
    {
        STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
        STORAGE_AUDIT_APP_WindowCloseSec = 0;
    }

    if (STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseWindowOpen == 0 &&
        STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel >= STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        STORAGE_AUDIT_APP_ProcessStorageRelease();
    }

    STORAGE_AUDIT_APP_SendHkCmd(NULL);
}

CFE_Status_t STORAGE_AUDIT_APP_ProcessStorageRelease(void)
{
    if (STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel < STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel -= STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseThreshold;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseCount++;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 1;
    STORAGE_AUDIT_APP_RecordReleaseAction();
    STORAGE_AUDIT_APP_WindowCloseSec = STORAGE_AUDIT_APP_GetMonotonicSec() +
                                  ((STORAGE_AUDIT_APP_WindowLenSec > 0U) ? (STORAGE_AUDIT_APP_WindowLenSec - 1U) : 0U);

    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_WINDOW_OPEN_EID, CFE_EVS_EventType_INFORMATION,
                      "STORAGE_AUDIT_APP: release window opened for %lu sec", (unsigned long)STORAGE_AUDIT_APP_WindowLenSec);
    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_RELEASE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "STORAGE_AUDIT_APP: buffered release executed (remaining=%lu, releases=%lu)",
                      (unsigned long)STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel,
                      (unsigned long)STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseCount);

    return CFE_SUCCESS;
}

void STORAGE_AUDIT_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(STORAGE_AUDIT_APP_PERF_ID);

    status = STORAGE_AUDIT_APP_Init();
    if (status != CFE_SUCCESS)
    {
        STORAGE_AUDIT_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&STORAGE_AUDIT_APP_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(STORAGE_AUDIT_APP_PERF_ID);

        status = CFE_SB_ReceiveBuffer(&SBBufPtr, STORAGE_AUDIT_APP_Data.CommandPipe, STORAGE_AUDIT_APP_TIMEOUT_MSEC);

        CFE_ES_PerfLogEntry(STORAGE_AUDIT_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            STORAGE_AUDIT_APP_TaskPipe(SBBufPtr);
        }
        else if (status == CFE_SB_TIME_OUT)
        {
            STORAGE_AUDIT_APP_PollStorageWindow();
        }
        else
        {
            CFE_EVS_SendEvent(STORAGE_AUDIT_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "STORAGE_AUDIT_APP: SB Pipe Read Error, App Will Exit");

            STORAGE_AUDIT_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(STORAGE_AUDIT_APP_PERF_ID);

    CFE_ES_ExitApp(STORAGE_AUDIT_APP_Data.RunStatus);
}

CFE_Status_t STORAGE_AUDIT_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[STORAGE_AUDIT_APP_CFG_MAX_VERSION_STR_LEN];

    memset(&STORAGE_AUDIT_APP_Data, 0, sizeof(STORAGE_AUDIT_APP_Data));

    STORAGE_AUDIT_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Storage Buffer App: Error Registering Events, RC = 0x%08lX\n",
                             (unsigned long)status);
    }
    else
    {
        CFE_MSG_Init(CFE_MSG_PTR(STORAGE_AUDIT_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(STORAGE_AUDIT_APP_HK_TLM_MID),
                     sizeof(STORAGE_AUDIT_APP_Data.HkTlm));
        STORAGE_AUDIT_APP_LoadConfig();
        STORAGE_AUDIT_APP_ResetWindowState();

        status = CFE_SB_CreatePipe(&STORAGE_AUDIT_APP_Data.CommandPipe, STORAGE_AUDIT_APP_PLATFORM_PIPE_DEPTH,
                                   STORAGE_AUDIT_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(STORAGE_AUDIT_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error creating SB Command Pipe, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(STORAGE_AUDIT_APP_SEND_HK_MID), STORAGE_AUDIT_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(STORAGE_AUDIT_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Subscribing to HK request, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(STORAGE_AUDIT_APP_CMD_MID), STORAGE_AUDIT_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(STORAGE_AUDIT_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Subscribing to Commands, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_Register(&STORAGE_AUDIT_APP_Data.TblHandles[0], "BufferPolicyTable",
                                  sizeof(STORAGE_AUDIT_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, STORAGE_AUDIT_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(STORAGE_AUDIT_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Storage Buffer App: Error Registering Buffer Policy Table, RC = 0x%08lX",
                              (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(STORAGE_AUDIT_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, STORAGE_AUDIT_APP_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, STORAGE_AUDIT_APP_CFG_MAX_VERSION_STR_LEN, "Storage Buffer App",
                                    STORAGE_AUDIT_APP_VERSION,
                                    STORAGE_AUDIT_APP_BUILD_CODENAME, STORAGE_AUDIT_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(STORAGE_AUDIT_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "Storage Buffer App initialized.%s", VersionString);
    }

    return status;
}
