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

#include "recorder_cache_app.h"
#include "recorder_cache_app_cmds.h"
#include "recorder_cache_app_msgids.h"
#include "recorder_cache_app_eventids.h"
#include "recorder_cache_app_version.h"
#include "recorder_cache_app_tbl.h"
#include "recorder_cache_app_utils.h"
#include "recorder_cache_app_msg.h"

#include "sample_lib.h"

#include <string.h>

static uint32 RECORDER_CACHE_APP_UpdateRecordCacheWindow(uint32 ObjectId, uint32 ByteCount)
{
    uint8  record[64];
    uint8  cache[32];
    uint32 record_len;
    uint32 i;
    uint32 score = 0;

    for (i = 0; i < sizeof(record); ++i)
    {
        record[i] = (uint8)((ObjectId ^ ByteCount ^ i) & 0xFFU);
    }

    record_len = ByteCount + (ObjectId & 0x0FU);
    if (record_len > sizeof(record))
    {
        record_len = sizeof(record);
    }
    if ((ObjectId & 0x1U) != 0U)
    {
        score ^= record_len;
    }

    /* Copy record bytes into the local cache window. */
    memcpy(cache, record, record_len);

    for (i = 0; i < record_len && i < sizeof(cache); ++i)
    {
        score += cache[i];
    }

    return score;
}


CFE_Status_t RECORDER_CACHE_APP_SendHkCmd(const RECORDER_CACHE_APP_SendHkCmd_t *Msg)
{
    int i;

    RECORDER_CACHE_APP_Data.HkTlm.Payload.CommandErrorCounter = RECORDER_CACHE_APP_Data.ErrCounter;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.CommandCounter      = RECORDER_CACHE_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(RECORDER_CACHE_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(RECORDER_CACHE_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < RECORDER_CACHE_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(RECORDER_CACHE_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t RECORDER_CACHE_APP_NoopCmd(const RECORDER_CACHE_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    RECORDER_CACHE_APP_Data.CmdCounter++;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.LastCommand = RECORDER_CACHE_APP_NOOP_CC;

    CFE_EVS_SendEvent(RECORDER_CACHE_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "RECORDER_CACHE_APP: NOOP command accepted (%s)", RECORDER_CACHE_APP_VERSION);

    RECORDER_CACHE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t RECORDER_CACHE_APP_ResetCountersCmd(const RECORDER_CACHE_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    RECORDER_CACHE_APP_ResetWindowState();
    RECORDER_CACHE_APP_Data.CmdCounter = 0;
    RECORDER_CACHE_APP_Data.ErrCounter = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.LastCommand       = RECORDER_CACHE_APP_RESET_BUFFER_CC;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseCount = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseActionCount = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageObjectCount = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageBytes = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageDirCreated = 0;

    CFE_EVS_SendEvent(RECORDER_CACHE_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "RECORDER_CACHE_APP: buffer reset command");

    RECORDER_CACHE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t RECORDER_CACHE_APP_PushDataCmd(const RECORDER_CACHE_APP_PushDataCmd_t *Msg)
{
    RECORDER_CACHE_APP_Data.CmdCounter++;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.LastCommand = RECORDER_CACHE_APP_STAGE_DATA_CC;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel += Msg->Payload.Amount;

    CFE_EVS_SendEvent(RECORDER_CACHE_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "RECORDER_CACHE_APP: staged %lu units (level=%lu)", (unsigned long)Msg->Payload.Amount,
                      (unsigned long)RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel);

    RECORDER_CACHE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t RECORDER_CACHE_APP_ClearCmd(const RECORDER_CACHE_APP_ClearCmd_t *Msg)
{
    (void)Msg;

    RECORDER_CACHE_APP_ResetWindowState();
    RECORDER_CACHE_APP_Data.CmdCounter++;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.LastCommand = RECORDER_CACHE_APP_FLUSH_BUFFER_CC;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseActionCount = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageObjectCount = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageBytes = 0;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageDirCreated = 0;

    CFE_EVS_SendEvent(RECORDER_CACHE_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "RECORDER_CACHE_APP: release window cleared");

    RECORDER_CACHE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t RECORDER_CACHE_APP_CreateStoreCmd(const RECORDER_CACHE_APP_CreateStoreCmd_t *Msg)
{
    (void)Msg;

    RECORDER_CACHE_APP_Data.CmdCounter++;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.LastCommand = RECORDER_CACHE_APP_CREATE_STORE_CC;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageDirCreated = 1;

    CFE_EVS_SendEvent(RECORDER_CACHE_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "RECORDER_CACHE_APP: simulated storage directory prepared");

    RECORDER_CACHE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t RECORDER_CACHE_APP_CopyStoreCmd(const RECORDER_CACHE_APP_CopyStoreCmd_t *Msg)
{
    uint32 byte_count;

    RECORDER_CACHE_APP_Data.CmdCounter++;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.LastCommand = RECORDER_CACHE_APP_COPY_STORE_CC;

    if (RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageDirCreated == 0)
    {
        RECORDER_CACHE_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(RECORDER_CACHE_APP_VALUE_INF_EID, CFE_EVS_EventType_ERROR,
                          "RECORDER_CACHE_APP: copy rejected before storage directory setup");
        RECORDER_CACHE_APP_SendHkCmd(NULL);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    byte_count = Msg->Payload.ByteCount;
    if (byte_count == 0)
    {
        byte_count = 1;
    }

    if (Msg->Payload.Overwrite == 0)
    {
        RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageObjectCount++;
    }

    RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageBytes += byte_count;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel += byte_count;
    RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseActionCount +=
        RECORDER_CACHE_APP_UpdateRecordCacheWindow(Msg->Payload.ObjectId, byte_count) & 0x1U;

    CFE_EVS_SendEvent(RECORDER_CACHE_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "RECORDER_CACHE_APP: copied object %lu (%lu bytes, total=%lu)",
                      (unsigned long)Msg->Payload.ObjectId, (unsigned long)byte_count,
                      (unsigned long)RECORDER_CACHE_APP_Data.HkTlm.Payload.StorageBytes);

    if (RECORDER_CACHE_APP_Data.HkTlm.Payload.BufferLevel >= RECORDER_CACHE_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        RECORDER_CACHE_APP_ProcessStorageRelease();
    }

    RECORDER_CACHE_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
