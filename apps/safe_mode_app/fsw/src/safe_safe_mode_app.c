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

#include "safe_mode_app.h"
#include "safe_mode_app_cmds.h"
#include "safe_mode_app_utils.h"
#include "safe_mode_app_eventids.h"
#include "safe_mode_app_dispatch.h"
#include "safe_mode_app_tbl.h"
#include "safe_mode_app_version.h"

SAFE_SAFE_MODE_APP_Data_t SAFE_SAFE_MODE_APP_Data;
void SAFE_SAFE_MODE_APP_Main(void)
{
    CFE_Status_t     status;
    CFE_SB_Buffer_t *SBBufPtr;

    CFE_ES_PerfLogEntry(SAFE_SAFE_MODE_APP_PERF_ID);

    status = SAFE_SAFE_MODE_APP_Init();
    if (status != CFE_SUCCESS)
    {
        SAFE_SAFE_MODE_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
    }

    while (CFE_ES_RunLoop(&SAFE_SAFE_MODE_APP_Data.RunStatus) == true)
    {
        CFE_ES_PerfLogExit(SAFE_SAFE_MODE_APP_PERF_ID);

        status = CFE_SB_ReceiveBuffer(&SBBufPtr, SAFE_SAFE_MODE_APP_Data.CommandPipe, CFE_SB_PEND_FOREVER);

        CFE_ES_PerfLogEntry(SAFE_SAFE_MODE_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            SAFE_SAFE_MODE_APP_TaskPipe(SBBufPtr);
        }
        else
        {
            CFE_EVS_SendEvent(SAFE_SAFE_MODE_APP_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "MODE APP: SB Pipe Read Error, App Will Exit");

            SAFE_SAFE_MODE_APP_Data.RunStatus = CFE_ES_RunStatus_APP_ERROR;
        }
    }

    CFE_ES_PerfLogExit(SAFE_SAFE_MODE_APP_PERF_ID);

    CFE_ES_ExitApp(SAFE_SAFE_MODE_APP_Data.RunStatus);
}

CFE_Status_t SAFE_SAFE_MODE_APP_Init(void)
{
    CFE_Status_t status;
    char         VersionString[SAFE_SAFE_MODE_APP_CFG_MAX_VERSION_STR_LEN];

    memset(&SAFE_SAFE_MODE_APP_Data, 0, sizeof(SAFE_SAFE_MODE_APP_Data));

    SAFE_SAFE_MODE_APP_Data.RunStatus = CFE_ES_RunStatus_APP_RUN;

    status = CFE_EVS_Register(NULL, 0, CFE_EVS_EventFilter_BINARY);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Mode Management App: Error Registering Events, RC = 0x%08lX\n",
                             (unsigned long)status);
    }
    else
    {
        CFE_MSG_Init(CFE_MSG_PTR(SAFE_SAFE_MODE_APP_Data.HkTlm.TelemetryHeader), CFE_SB_ValueToMsgId(SAFE_SAFE_MODE_APP_HK_TLM_MID),
                     sizeof(SAFE_SAFE_MODE_APP_Data.HkTlm));
        SAFE_SAFE_MODE_APP_Data.HkTlm.Payload.PolicyEnabled = 1;
        SAFE_SAFE_MODE_APP_Data.HkTlm.Payload.MonitorActiveCount = 176;

        status = CFE_SB_CreatePipe(&SAFE_SAFE_MODE_APP_Data.CommandPipe, SAFE_SAFE_MODE_APP_PLATFORM_PIPE_DEPTH,
                                   SAFE_SAFE_MODE_APP_PLATFORM_PIPE_NAME);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SAFE_SAFE_MODE_APP_CR_PIPE_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Mode Management App: Error creating SB Command Pipe, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SAFE_SAFE_MODE_APP_SEND_HK_MID), SAFE_SAFE_MODE_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SAFE_SAFE_MODE_APP_SUB_HK_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Mode Management App: Error Subscribing to HK request, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_SB_Subscribe(CFE_SB_ValueToMsgId(SAFE_SAFE_MODE_APP_CMD_MID), SAFE_SAFE_MODE_APP_Data.CommandPipe);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SAFE_SAFE_MODE_APP_SUB_CMD_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Mode Management App: Error Subscribing to Commands, RC = 0x%08lX",
                              (unsigned long)status);
        }
    }

    if (status == CFE_SUCCESS)
    {
        status = CFE_TBL_Register(&SAFE_SAFE_MODE_APP_Data.TblHandles[0], "PolicyTable", sizeof(SAFE_SAFE_MODE_APP_ExampleTable_t),
                                  CFE_TBL_OPT_DEFAULT, SAFE_SAFE_MODE_APP_TblValidationFunc);
        if (status != CFE_SUCCESS)
        {
            CFE_EVS_SendEvent(SAFE_SAFE_MODE_APP_TABLE_REG_ERR_EID, CFE_EVS_EventType_ERROR,
                              "Mode Management App: Error Registering Policy Table, RC = 0x%08lX",
                              (unsigned long)status);
        }
        else
        {
            status = CFE_TBL_Load(SAFE_SAFE_MODE_APP_Data.TblHandles[0], CFE_TBL_SRC_FILE, SAFE_SAFE_MODE_APP_PLATFORM_TABLE_FILE);
        }

        CFE_Config_GetVersionString(VersionString, SAFE_SAFE_MODE_APP_CFG_MAX_VERSION_STR_LEN, "Mode Management App",
                                    SAFE_SAFE_MODE_APP_VERSION,
                                    SAFE_SAFE_MODE_APP_BUILD_CODENAME, SAFE_SAFE_MODE_APP_LAST_OFFICIAL);

        CFE_EVS_SendEvent(SAFE_SAFE_MODE_APP_INIT_INF_EID, CFE_EVS_EventType_INFORMATION,
                          "Mode Management App initialized.%s", VersionString);
    }

    return status;
}
