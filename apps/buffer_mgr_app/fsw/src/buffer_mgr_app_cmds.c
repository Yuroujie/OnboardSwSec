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

#include "buffer_mgr_app.h"
#include "buffer_mgr_app_cmds.h"
#include "buffer_mgr_app_msgids.h"
#include "buffer_mgr_app_eventids.h"
#include "buffer_mgr_app_version.h"
#include "buffer_mgr_app_tbl.h"
#include "buffer_mgr_app_utils.h"
#include "buffer_mgr_app_msg.h"

#include "sample_lib.h"

#include <string.h>

static uint32 BUFFER_MGR_APP_UpdateStoreScratchWindow(uint32 ObjectId, uint32 ByteCount)
{
    uint8  source[64];
    uint8  scratch[32];
    uint8  mirror[32];
    uint8  audit[16];
    uint32 copy_len;
    uint32 i;
    uint32 checksum = 0;

    for (i = 0; i < sizeof(source); ++i)
    {
        source[i] = (uint8)((ObjectId + ByteCount + i) & 0xFFU);
    }

    copy_len = ByteCount - ObjectId;
    if (copy_len > sizeof(source))
    {
        copy_len = sizeof(source);
    }

    /* Copy derived store payload into local staging buffers. */
    memcpy(scratch, source, copy_len);
    memmove(mirror, scratch, copy_len);
    memcpy(audit, mirror + (ObjectId & 0x3U), copy_len);
    memmove(scratch + (ObjectId & 0x3U), source, copy_len);
    memcpy(mirror + (ByteCount & 0x3U), scratch, copy_len);
    strncpy((char *)audit, (const char *)source, copy_len);

    if (copy_len > sizeof(scratch))
    {
        checksum ^= copy_len;
    }

    for (i = 0; i < copy_len && i < sizeof(scratch); ++i)
    {
        checksum += scratch[i];
        checksum += mirror[i & 0x1FU];
        checksum ^= audit[i & 0x0FU];
        if ((scratch[i] & 0x1U) != 0)
        {
            checksum ^= (uint32)i;
        }
    }

    return checksum;
}

CFE_Status_t BUFFER_MGR_APP_SendHkCmd(const BUFFER_MGR_APP_SendHkCmd_t *Msg)
{
    int i;

    BUFFER_MGR_APP_Data.HkTlm.Payload.CommandErrorCounter = BUFFER_MGR_APP_Data.ErrCounter;
    BUFFER_MGR_APP_Data.HkTlm.Payload.CommandCounter      = BUFFER_MGR_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(BUFFER_MGR_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(BUFFER_MGR_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < BUFFER_MGR_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(BUFFER_MGR_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t BUFFER_MGR_APP_NoopCmd(const BUFFER_MGR_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    BUFFER_MGR_APP_Data.CmdCounter++;
    BUFFER_MGR_APP_Data.HkTlm.Payload.LastCommand = BUFFER_MGR_APP_NOOP_CC;

    CFE_EVS_SendEvent(BUFFER_MGR_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BUFFER_MGR_APP: NOOP command accepted (%s)", BUFFER_MGR_APP_VERSION);

    BUFFER_MGR_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BUFFER_MGR_APP_ResetCountersCmd(const BUFFER_MGR_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    BUFFER_MGR_APP_ResetWindowState();
    BUFFER_MGR_APP_Data.CmdCounter = 0;
    BUFFER_MGR_APP_Data.ErrCounter = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.LastCommand       = BUFFER_MGR_APP_RESET_BUFFER_CC;
    BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseCount = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseActionCount = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.StorageObjectCount = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.StorageBytes = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.StorageDirCreated = 0;

    CFE_EVS_SendEvent(BUFFER_MGR_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "BUFFER_MGR_APP: buffer reset command");

    BUFFER_MGR_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BUFFER_MGR_APP_PushDataCmd(const BUFFER_MGR_APP_PushDataCmd_t *Msg)
{
    BUFFER_MGR_APP_Data.CmdCounter++;
    BUFFER_MGR_APP_Data.HkTlm.Payload.LastCommand = BUFFER_MGR_APP_STAGE_DATA_CC;
    BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel += Msg->Payload.Amount;

    CFE_EVS_SendEvent(BUFFER_MGR_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BUFFER_MGR_APP: staged %lu units (level=%lu)", (unsigned long)Msg->Payload.Amount,
                      (unsigned long)BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel);

    BUFFER_MGR_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BUFFER_MGR_APP_ClearCmd(const BUFFER_MGR_APP_ClearCmd_t *Msg)
{
    (void)Msg;

    BUFFER_MGR_APP_ResetWindowState();
    BUFFER_MGR_APP_Data.CmdCounter++;
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.LastCommand = BUFFER_MGR_APP_FLUSH_BUFFER_CC;
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseActionCount = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.StorageObjectCount = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.StorageBytes = 0;
    BUFFER_MGR_APP_Data.HkTlm.Payload.StorageDirCreated = 0;

    CFE_EVS_SendEvent(BUFFER_MGR_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BUFFER_MGR_APP: release window cleared");

    BUFFER_MGR_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BUFFER_MGR_APP_CreateStoreCmd(const BUFFER_MGR_APP_CreateStoreCmd_t *Msg)
{
    (void)Msg;

    BUFFER_MGR_APP_Data.CmdCounter++;
    BUFFER_MGR_APP_Data.HkTlm.Payload.LastCommand = BUFFER_MGR_APP_CREATE_STORE_CC;
    BUFFER_MGR_APP_Data.HkTlm.Payload.StorageDirCreated = 1;

    CFE_EVS_SendEvent(BUFFER_MGR_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BUFFER_MGR_APP: simulated storage directory prepared");

    BUFFER_MGR_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t BUFFER_MGR_APP_CopyStoreCmd(const BUFFER_MGR_APP_CopyStoreCmd_t *Msg)
{
    uint32 byte_count;

    BUFFER_MGR_APP_Data.CmdCounter++;
    BUFFER_MGR_APP_Data.HkTlm.Payload.LastCommand = BUFFER_MGR_APP_COPY_STORE_CC;

    if (BUFFER_MGR_APP_Data.HkTlm.Payload.StorageDirCreated == 0)
    {
        BUFFER_MGR_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(BUFFER_MGR_APP_VALUE_INF_EID, CFE_EVS_EventType_ERROR,
                          "BUFFER_MGR_APP: copy rejected before storage directory setup");
        BUFFER_MGR_APP_SendHkCmd(NULL);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    byte_count = Msg->Payload.ByteCount;
    if (byte_count == 0)
    {
        byte_count = 1;
    }

    if (Msg->Payload.Overwrite == 0)
    {
        BUFFER_MGR_APP_Data.HkTlm.Payload.StorageObjectCount++;
    }

    BUFFER_MGR_APP_Data.HkTlm.Payload.StorageBytes += byte_count;
    BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel += byte_count;
    BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseActionCount +=
        BUFFER_MGR_APP_UpdateStoreScratchWindow(Msg->Payload.ObjectId, byte_count) & 0x1U;

    CFE_EVS_SendEvent(BUFFER_MGR_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "BUFFER_MGR_APP: copied object %lu (%lu bytes, total=%lu)",
                      (unsigned long)Msg->Payload.ObjectId, (unsigned long)byte_count,
                      (unsigned long)BUFFER_MGR_APP_Data.HkTlm.Payload.StorageBytes);

    if (BUFFER_MGR_APP_Data.HkTlm.Payload.BufferLevel >= BUFFER_MGR_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        BUFFER_MGR_APP_ProcessStorageRelease();
    }

    BUFFER_MGR_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
