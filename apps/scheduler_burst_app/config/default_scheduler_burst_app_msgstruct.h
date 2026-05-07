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
 *   Specification for the SCHEDULER_BURST_APP command and telemetry
 *   message data types.
 *
 * @note
 *   Constants and enumerated types related to these message structures
 *   are defined in scheduler_burst_app_msgdefs.h.
 */
#ifndef DEFAULT_SCHEDULER_BURST_APP_MSGSTRUCT_H
#define DEFAULT_SCHEDULER_BURST_APP_MSGSTRUCT_H

/************************************************************************
 * Includes
 ************************************************************************/

#include "scheduler_burst_app_mission_cfg.h"
#include "scheduler_burst_app_msgdefs.h"
#include "cfe_msg_hdr.h"

/*************************************************************************/

/*
** The following commands all share the "NoArgs" format
**
** They are each given their own type name matching the command name, which
** allows them to change independently in the future without changing the prototype
** of the handler function
*/
typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SCHEDULER_BURST_APP_NoopCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SCHEDULER_BURST_APP_ResetCountersCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SCHEDULER_BURST_APP_ProcessCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SCHEDULER_BURST_APP_DisplayParamCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
    SCHEDULER_BURST_APP_SimTraffic_Payload_t Payload; /**< \brief Command payload */
} SCHEDULER_BURST_APP_SimTrafficCmd_t;

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
    SCHEDULER_BURST_APP_RunBurst_Payload_t Payload; /**< \brief Command payload */
} SCHEDULER_BURST_APP_RunBurstCmd_t;

/*************************************************************************/
/*
** Type definition (Counter App housekeeping)
*/

typedef struct
{
    CFE_MSG_CommandHeader_t CommandHeader; /**< \brief Command header */
} SCHEDULER_BURST_APP_SendHkCmd_t;

typedef struct
{
    CFE_MSG_TelemetryHeader_t  TelemetryHeader; /**< \brief Telemetry header */
    SCHEDULER_BURST_APP_HkTlm_Payload_t Payload;         /**< \brief Telemetry payload */
} SCHEDULER_BURST_APP_HkTlm_t;

#endif /* SCHEDULER_BURST_APP_MSGSTRUCT_H */
