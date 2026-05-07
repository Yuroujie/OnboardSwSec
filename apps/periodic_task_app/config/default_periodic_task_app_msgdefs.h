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
 *   Specification for the PERIODIC_TASK_APP command and telemetry
 *   message payload and constant definitions.
 */
#ifndef DEFAULT_PERIODIC_TASK_APP_MSGDEFS_H
#define DEFAULT_PERIODIC_TASK_APP_MSGDEFS_H

#include "common_types.h"
#include "periodic_task_app_fcncodes.h"

typedef struct PERIODIC_TASK_APP_DisplayParam_Payload
{
    uint32 ValU32;                                    /**< 32 bit unsigned integer value */
    int16  ValI16;                                    /**< 16 bit signed integer value */
    char   ValStr[PERIODIC_TASK_APP_MISSION_STRING_VAL_LEN]; /**< An example string */
} PERIODIC_TASK_APP_DisplayParam_Payload_t;

typedef struct PERIODIC_TASK_APP_SimTraffic_Payload
{
    uint16 MsgId;     /**< Simulated CCSDS message identifier */
    uint16 CmdCode;   /**< Simulated command code */
    uint16 MsgLength; /**< Simulated command packet length */
    uint16 DelayMsec; /**< Delay since previous command in milliseconds */
} PERIODIC_TASK_APP_SimTraffic_Payload_t;

typedef struct PERIODIC_TASK_APP_RunBurst_Payload
{
    uint16 Count;       /**< Number of simulated fast commands to process */
    uint16 SpacingMsec; /**< Inter-command spacing in milliseconds */
} PERIODIC_TASK_APP_RunBurst_Payload_t;

/*************************************************************************/
/*
** Type definition (Counter App housekeeping)
*/

typedef struct PERIODIC_TASK_APP_HkTlm_Payload
{
    uint8 CommandCounter;
    uint8 CommandErrorCounter;
    uint8 UpdateStep;
    uint8 LastCommand;
    uint32 TickCount;
    uint32 ScheduledActionCount;
    uint32 SimTrafficCount;
    uint32 BurstCommandCount;
    uint16 LastSimMsgId;
    uint16 LastSimCmdCode;
} PERIODIC_TASK_APP_HkTlm_Payload_t;

#endif
