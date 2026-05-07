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

#include "payload_task_app.h"
#include "payload_task_app_dispatch.h"
#include "payload_task_app_cmds.h"
#include "payload_task_app_eventids.h"
#include "payload_task_app_msgids.h"
#include "payload_task_app_msg.h"

bool PAYLOAD_TASK_APP_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
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

        CFE_EVS_SendEvent(PAYLOAD_TASK_APP_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        PAYLOAD_TASK_APP_Data.ErrCounter++;
    }

    return result;
}

void PAYLOAD_TASK_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    switch (CommandCode)
    {
        case PAYLOAD_TASK_APP_NOOP_CC:
            if (PAYLOAD_TASK_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(PAYLOAD_TASK_APP_NoopCmd_t)))
            {
                PAYLOAD_TASK_APP_NoopCmd((const PAYLOAD_TASK_APP_NoopCmd_t *)SBBufPtr);
            }
            break;

        case PAYLOAD_TASK_APP_RESET_STATE_CC:
            if (PAYLOAD_TASK_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(PAYLOAD_TASK_APP_ResetCountersCmd_t)))
            {
                PAYLOAD_TASK_APP_ResetCountersCmd((const PAYLOAD_TASK_APP_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case PAYLOAD_TASK_APP_INCREASE_STEP_CC:
            if (PAYLOAD_TASK_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(PAYLOAD_TASK_APP_ProcessCmd_t)))
            {
                PAYLOAD_TASK_APP_ProcessCmd((const PAYLOAD_TASK_APP_ProcessCmd_t *)SBBufPtr);
            }
            break;

        case PAYLOAD_TASK_APP_DECREASE_STEP_CC:
            if (PAYLOAD_TASK_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(PAYLOAD_TASK_APP_DisplayParamCmd_t)))
            {
                PAYLOAD_TASK_APP_DisplayParamCmd((const PAYLOAD_TASK_APP_DisplayParamCmd_t *)SBBufPtr);
            }
            break;

        case PAYLOAD_TASK_APP_SIM_TRAFFIC_CC:
            if (PAYLOAD_TASK_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(PAYLOAD_TASK_APP_SimTrafficCmd_t)))
            {
                PAYLOAD_TASK_APP_SimTrafficCmd((const PAYLOAD_TASK_APP_SimTrafficCmd_t *)SBBufPtr);
            }
            break;

        case PAYLOAD_TASK_APP_RUN_BURST_CC:
            if (PAYLOAD_TASK_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(PAYLOAD_TASK_APP_RunBurstCmd_t)))
            {
                PAYLOAD_TASK_APP_RunBurstCmd((const PAYLOAD_TASK_APP_RunBurstCmd_t *)SBBufPtr);
            }
            break;

        default:
            CFE_EVS_SendEvent(PAYLOAD_TASK_APP_CC_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid ground command code: CC = %d",
                              CommandCode);
            break;
    }
}

void PAYLOAD_TASK_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    if (!CFE_SB_IsValidMsgId(CMD_MID))
    {
        CMD_MID     = CFE_SB_ValueToMsgId(PAYLOAD_TASK_APP_CMD_MID);
        SEND_HK_MID = CFE_SB_ValueToMsgId(PAYLOAD_TASK_APP_SEND_HK_MID);
    }

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
    {
        PAYLOAD_TASK_APP_SendHkCmd((const PAYLOAD_TASK_APP_SendHkCmd_t *)SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    {
        PAYLOAD_TASK_APP_ProcessGroundCommand(SBBufPtr);
    }
    else
    {
        CFE_EVS_SendEvent(PAYLOAD_TASK_APP_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                          "PAYLOAD_TASK_APP: invalid command packet, MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }
}
