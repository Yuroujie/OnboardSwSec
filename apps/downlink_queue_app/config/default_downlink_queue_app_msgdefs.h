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

/**
 * @file
 *   Specification for the DOWNLINK_QUEUE_APP command and telemetry
 *   message payload and constant definitions.
 */
#ifndef DEFAULT_DOWNLINK_QUEUE_APP_MSGDEFS_H
#define DEFAULT_DOWNLINK_QUEUE_APP_MSGDEFS_H

#include "common_types.h"
#include "downlink_queue_app_fcncodes.h"

typedef struct DOWNLINK_QUEUE_APP_StageData_Payload
{
    uint32 Amount; /**< Amount of buffered data to stage */
} DOWNLINK_QUEUE_APP_StageData_Payload_t;

typedef struct DOWNLINK_QUEUE_APP_CopyStore_Payload
{
    uint32 ObjectId;  /**< Simulated file or table object identifier */
    uint32 ByteCount; /**< Simulated bytes copied into the storage area */
    uint8  Overwrite; /**< Nonzero when the simulated copy overwrites an object */
    uint8  Reserved[3];
} DOWNLINK_QUEUE_APP_CopyStore_Payload_t;

/*************************************************************************/
/*
** Type definition (Storage buffer app housekeeping)
*/

typedef struct DOWNLINK_QUEUE_APP_HkTlm_Payload
{
    uint8 CommandCounter;
    uint8 CommandErrorCounter;
    uint8 ReleaseWindowOpen;
    uint8 LastCommand;
    uint32 BufferLevel;
    uint32 ReleaseThreshold;
    uint32 ReleaseCount;
    uint32 ReleaseActionCount;
    uint32 StorageObjectCount;
    uint32 StorageBytes;
    uint8 StorageDirCreated;
} DOWNLINK_QUEUE_APP_HkTlm_Payload_t;

#endif
