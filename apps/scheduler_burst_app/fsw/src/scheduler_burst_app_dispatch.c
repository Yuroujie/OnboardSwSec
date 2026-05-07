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

#include "scheduler_burst_app.h"
#include "scheduler_burst_app_dispatch.h"
#include "scheduler_burst_app_cmds.h"
#include "scheduler_burst_app_eventids.h"
#include "scheduler_burst_app_msgids.h"
#include "scheduler_burst_app_msg.h"

bool SCHEDULER_BURST_APP_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
{
    bool              result       = true;
    size_t            ActualLength = 0;
    CFE_SB_MsgId_t    MsgId        = CFE_SB_INVALID_MSG_ID;
    CFE_MSG_FcnCode_t FcnCode      = 0;

    CFE_MSG_GetSize(MsgPtr, &ActualLength);

    if (ExpectedLength != ActualLength)
    {
        CFE_MSG_GetMsgId(MsgPtr, &MsgId);
        CFE_MSG_GetFcnCode(MsgPtr, &FcnCode);

        CFE_EVS_SendEvent(SCHEDULER_BURST_APP_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        SCHEDULER_BURST_APP_Data.ErrCounter++;
    }

    return result;
}

void SCHEDULER_BURST_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    switch (CommandCode)
    {
        case SCHEDULER_BURST_APP_NOOP_CC:
            if (SCHEDULER_BURST_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(SCHEDULER_BURST_APP_NoopCmd_t)))
            {
                SCHEDULER_BURST_APP_NoopCmd((const SCHEDULER_BURST_APP_NoopCmd_t *)SBBufPtr);
            }
            break;

        case SCHEDULER_BURST_APP_RESET_STATE_CC:
            if (SCHEDULER_BURST_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(SCHEDULER_BURST_APP_ResetCountersCmd_t)))
            {
                SCHEDULER_BURST_APP_ResetCountersCmd((const SCHEDULER_BURST_APP_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case SCHEDULER_BURST_APP_INCREASE_STEP_CC:
            if (SCHEDULER_BURST_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(SCHEDULER_BURST_APP_ProcessCmd_t)))
            {
                SCHEDULER_BURST_APP_ProcessCmd((const SCHEDULER_BURST_APP_ProcessCmd_t *)SBBufPtr);
            }
            break;

        case SCHEDULER_BURST_APP_DECREASE_STEP_CC:
            if (SCHEDULER_BURST_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(SCHEDULER_BURST_APP_DisplayParamCmd_t)))
            {
                SCHEDULER_BURST_APP_DisplayParamCmd((const SCHEDULER_BURST_APP_DisplayParamCmd_t *)SBBufPtr);
            }
            break;

        case SCHEDULER_BURST_APP_SIM_TRAFFIC_CC:
            if (SCHEDULER_BURST_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(SCHEDULER_BURST_APP_SimTrafficCmd_t)))
            {
                SCHEDULER_BURST_APP_SimTrafficCmd((const SCHEDULER_BURST_APP_SimTrafficCmd_t *)SBBufPtr);
            }
            break;

        case SCHEDULER_BURST_APP_RUN_BURST_CC:
            if (SCHEDULER_BURST_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(SCHEDULER_BURST_APP_RunBurstCmd_t)))
            {
                SCHEDULER_BURST_APP_RunBurstCmd((const SCHEDULER_BURST_APP_RunBurstCmd_t *)SBBufPtr);
            }
            break;

        default:
            CFE_EVS_SendEvent(SCHEDULER_BURST_APP_CC_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid ground command code: CC = %d",
                              CommandCode);
            break;
    }
}

void SCHEDULER_BURST_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    if (!CFE_SB_IsValidMsgId(CMD_MID))
    {
        CMD_MID     = CFE_SB_ValueToMsgId(SCHEDULER_BURST_APP_CMD_MID);
        SEND_HK_MID = CFE_SB_ValueToMsgId(SCHEDULER_BURST_APP_SEND_HK_MID);
    }

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
    {
        SCHEDULER_BURST_APP_SendHkCmd((const SCHEDULER_BURST_APP_SendHkCmd_t *)SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    {
        SCHEDULER_BURST_APP_ProcessGroundCommand(SBBufPtr);
    }
    else
    {
        CFE_EVS_SendEvent(SCHEDULER_BURST_APP_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                          "SCHEDULER_BURST_APP: invalid command packet, MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }
}
