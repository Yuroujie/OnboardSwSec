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

#include "storage_audit_app.h"
#include "storage_audit_app_cmds.h"
#include "storage_audit_app_msgids.h"
#include "storage_audit_app_eventids.h"
#include "storage_audit_app_version.h"
#include "storage_audit_app_tbl.h"
#include "storage_audit_app_utils.h"
#include "storage_audit_app_msg.h"

#include "sample_lib.h"

#include <string.h>

static uint32 STORAGE_AUDIT_APP_UpdateStorageAuditWindow(uint32 ObjectId, uint32 ByteCount)
{
    uint8  source[96];
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

    /* Copy derived payload into local staging buffers. */
    memcpy(scratch, source, copy_len);
    memmove(mirror, scratch, copy_len);
    memcpy(audit, mirror + (ObjectId & 0x3U), copy_len);
    strncpy((char *)audit, (const char *)source, copy_len);

    for (i = 0; i < copy_len && i < sizeof(scratch); ++i)
    {
        checksum += scratch[i];
        checksum += mirror[i & 0x1FU];
        checksum ^= audit[i & 0x0FU];
    }

    return checksum;
}


CFE_Status_t STORAGE_AUDIT_APP_SendHkCmd(const STORAGE_AUDIT_APP_SendHkCmd_t *Msg)
{
    int i;

    STORAGE_AUDIT_APP_Data.HkTlm.Payload.CommandErrorCounter = STORAGE_AUDIT_APP_Data.ErrCounter;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.CommandCounter      = STORAGE_AUDIT_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(STORAGE_AUDIT_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(STORAGE_AUDIT_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < STORAGE_AUDIT_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(STORAGE_AUDIT_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t STORAGE_AUDIT_APP_NoopCmd(const STORAGE_AUDIT_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    STORAGE_AUDIT_APP_Data.CmdCounter++;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.LastCommand = STORAGE_AUDIT_APP_NOOP_CC;

    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "STORAGE_AUDIT_APP: NOOP command accepted (%s)", STORAGE_AUDIT_APP_VERSION);

    STORAGE_AUDIT_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t STORAGE_AUDIT_APP_ResetCountersCmd(const STORAGE_AUDIT_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    STORAGE_AUDIT_APP_ResetWindowState();
    STORAGE_AUDIT_APP_Data.CmdCounter = 0;
    STORAGE_AUDIT_APP_Data.ErrCounter = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.LastCommand       = STORAGE_AUDIT_APP_RESET_BUFFER_CC;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseCount = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseActionCount = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageObjectCount = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageBytes = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageDirCreated = 0;

    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "STORAGE_AUDIT_APP: buffer reset command");

    STORAGE_AUDIT_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t STORAGE_AUDIT_APP_PushDataCmd(const STORAGE_AUDIT_APP_PushDataCmd_t *Msg)
{
    STORAGE_AUDIT_APP_Data.CmdCounter++;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.LastCommand = STORAGE_AUDIT_APP_STAGE_DATA_CC;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel += Msg->Payload.Amount;

    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "STORAGE_AUDIT_APP: staged %lu units (level=%lu)", (unsigned long)Msg->Payload.Amount,
                      (unsigned long)STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel);

    STORAGE_AUDIT_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t STORAGE_AUDIT_APP_ClearCmd(const STORAGE_AUDIT_APP_ClearCmd_t *Msg)
{
    (void)Msg;

    STORAGE_AUDIT_APP_ResetWindowState();
    STORAGE_AUDIT_APP_Data.CmdCounter++;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseWindowOpen = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.LastCommand = STORAGE_AUDIT_APP_FLUSH_BUFFER_CC;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseActionCount = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageObjectCount = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageBytes = 0;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageDirCreated = 0;

    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "STORAGE_AUDIT_APP: release window cleared");

    STORAGE_AUDIT_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t STORAGE_AUDIT_APP_CreateStoreCmd(const STORAGE_AUDIT_APP_CreateStoreCmd_t *Msg)
{
    (void)Msg;

    STORAGE_AUDIT_APP_Data.CmdCounter++;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.LastCommand = STORAGE_AUDIT_APP_CREATE_STORE_CC;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageDirCreated = 1;

    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "STORAGE_AUDIT_APP: simulated storage directory prepared");

    STORAGE_AUDIT_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t STORAGE_AUDIT_APP_CopyStoreCmd(const STORAGE_AUDIT_APP_CopyStoreCmd_t *Msg)
{
    uint32 byte_count;

    STORAGE_AUDIT_APP_Data.CmdCounter++;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.LastCommand = STORAGE_AUDIT_APP_COPY_STORE_CC;

    if (STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageDirCreated == 0)
    {
        STORAGE_AUDIT_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(STORAGE_AUDIT_APP_VALUE_INF_EID, CFE_EVS_EventType_ERROR,
                          "STORAGE_AUDIT_APP: copy rejected before storage directory setup");
        STORAGE_AUDIT_APP_SendHkCmd(NULL);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    byte_count = Msg->Payload.ByteCount;
    if (byte_count == 0)
    {
        byte_count = 1;
    }

    if (Msg->Payload.Overwrite == 0)
    {
        STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageObjectCount++;
    }

    STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageBytes += byte_count;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel += byte_count;
    STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseActionCount +=
        STORAGE_AUDIT_APP_UpdateStorageAuditWindow(Msg->Payload.ObjectId, byte_count) & 0x1U;

    CFE_EVS_SendEvent(STORAGE_AUDIT_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "STORAGE_AUDIT_APP: copied object %lu (%lu bytes, total=%lu)",
                      (unsigned long)Msg->Payload.ObjectId, (unsigned long)byte_count,
                      (unsigned long)STORAGE_AUDIT_APP_Data.HkTlm.Payload.StorageBytes);

    if (STORAGE_AUDIT_APP_Data.HkTlm.Payload.BufferLevel >= STORAGE_AUDIT_APP_Data.HkTlm.Payload.ReleaseThreshold)
    {
        STORAGE_AUDIT_APP_ProcessStorageRelease();
    }

    STORAGE_AUDIT_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
