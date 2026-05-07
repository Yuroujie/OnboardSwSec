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

#include "downlink_queue_app.h"
#include "downlink_queue_app_cmds.h"
#include "downlink_queue_app_msgids.h"
#include "downlink_queue_app_eventids.h"
#include "downlink_queue_app_version.h"
#include "downlink_queue_app_tbl.h"
#include "downlink_queue_app_utils.h"
#include "downlink_queue_app_msg.h"

#include "sample_lib.h"

#include <string.h>

static uint32 DOWNLINK_QUEUE_APP_UpdateQueueBacklogWindow(uint32 ObjectId, uint32 ByteCount)
{
    uint8  queue[32];
    uint8  source[48];
    uint32 n;
    uint32 i;
    uint32 score = 0;

    for (i = 0; i < sizeof(source); ++i)
    {
        source[i] = (uint8)((ByteCount + i) & 0xFFU);
    }

    n = ByteCount + ObjectId;
    if (n > sizeof(source))
    {
        n = sizeof(source);
    }
    if ((ObjectId & 0x1U) != 0U)
    {
        score ^= n;
    }

    /* Copy queue profile into the local backlog window. */
    memcpy(queue, source, n);

    for (i = 0; i < n && i < sizeof(queue); ++i)
    {
        score += queue[i];
    }

    return score;
}


CFE_Status_t DOWNLINK_QUEUE_APP_SendHkCmd(const DOWNLINK_QUEUE_APP_SendHkCmd_t *Msg)
{
    int i;

    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.CommandErrorCounter = DOWNLINK_QUEUE_APP_Data.ErrCounter;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.CommandCounter      = DOWNLINK_QUEUE_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(DOWNLINK_QUEUE_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(DOWNLINK_QUEUE_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < DOWNLINK_QUEUE_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(DOWNLINK_QUEUE_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t DOWNLINK_QUEUE_APP_NoopCmd(const DOWNLINK_QUEUE_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    DOWNLINK_QUEUE_APP_Data.CmdCounter++;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.LastCommand = DOWNLINK_QUEUE_APP_NOOP_CC;

    CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "DOWNLINK_QUEUE_APP: NOOP command accepted (%s)", DOWNLINK_QUEUE_APP_VERSION);

    DOWNLINK_QUEUE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t DOWNLINK_QUEUE_APP_ResetCountersCmd(const DOWNLINK_QUEUE_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    DOWNLINK_QUEUE_APP_ResetWindowState();
    DOWNLINK_QUEUE_APP_Data.CmdCounter = 0;
    DOWNLINK_QUEUE_APP_Data.ErrCounter = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.LastCommand       = DOWNLINK_QUEUE_APP_RESET_BUFFER_CC;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.BufferLevel = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.ReleaseCount = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.ReleaseActionCount = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageObjectCount = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageBytes = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageDirCreated = 0;

    CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "DOWNLINK_QUEUE_APP: buffer reset command");

    DOWNLINK_QUEUE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t DOWNLINK_QUEUE_APP_PushDataCmd(const DOWNLINK_QUEUE_APP_PushDataCmd_t *Msg)
{
    DOWNLINK_QUEUE_APP_Data.CmdCounter++;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.LastCommand = DOWNLINK_QUEUE_APP_STAGE_DATA_CC;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.BufferLevel += Msg->Payload.Amount;

    CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "DOWNLINK_QUEUE_APP: staged %lu units (level=%lu)", (unsigned long)Msg->Payload.Amount,
                      (unsigned long)DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.BufferLevel);

    DOWNLINK_QUEUE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t DOWNLINK_QUEUE_APP_ClearCmd(const DOWNLINK_QUEUE_APP_ClearCmd_t *Msg)
{
    (void)Msg;

    DOWNLINK_QUEUE_APP_ResetWindowState();
    DOWNLINK_QUEUE_APP_Data.CmdCounter++;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.BufferLevel = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.LastCommand = DOWNLINK_QUEUE_APP_FLUSH_BUFFER_CC;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.ReleaseActionCount = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageObjectCount = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageBytes = 0;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageDirCreated = 0;

    CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "DOWNLINK_QUEUE_APP: release window cleared");

    DOWNLINK_QUEUE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t DOWNLINK_QUEUE_APP_CreateStoreCmd(const DOWNLINK_QUEUE_APP_CreateStoreCmd_t *Msg)
{
    (void)Msg;

    DOWNLINK_QUEUE_APP_Data.CmdCounter++;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.LastCommand = DOWNLINK_QUEUE_APP_CREATE_STORE_CC;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageDirCreated = 1;

    CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "DOWNLINK_QUEUE_APP: simulated storage directory prepared");

    DOWNLINK_QUEUE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t DOWNLINK_QUEUE_APP_CopyStoreCmd(const DOWNLINK_QUEUE_APP_CopyStoreCmd_t *Msg)
{
    uint32 byte_count;

    DOWNLINK_QUEUE_APP_Data.CmdCounter++;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.LastCommand = DOWNLINK_QUEUE_APP_COPY_STORE_CC;

    if (DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageDirCreated == 0)
    {
        DOWNLINK_QUEUE_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_VALUE_INF_EID, CFE_EVS_EventType_ERROR,
                          "DOWNLINK_QUEUE_APP: copy rejected before storage directory setup");
        DOWNLINK_QUEUE_APP_SendHkCmd(NULL);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    byte_count = Msg->Payload.ByteCount;
    if (byte_count == 0)
    {
        byte_count = 1;
    }

    if (Msg->Payload.Overwrite == 0)
    {
        DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageObjectCount++;
    }

    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageBytes += byte_count;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.BufferLevel += byte_count;
    DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.ReleaseActionCount +=
        DOWNLINK_QUEUE_APP_UpdateQueueBacklogWindow(Msg->Payload.ObjectId, byte_count) & 0x1U;

    CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "DOWNLINK_QUEUE_APP: copied object %lu (%lu bytes, total=%lu)",
                      (unsigned long)Msg->Payload.ObjectId, (unsigned long)byte_count,
                      (unsigned long)DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.StorageBytes);

    if (DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.BufferLevel >= DOWNLINK_QUEUE_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        DOWNLINK_QUEUE_APP_ProcessStorageRelease();
    }

    DOWNLINK_QUEUE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
