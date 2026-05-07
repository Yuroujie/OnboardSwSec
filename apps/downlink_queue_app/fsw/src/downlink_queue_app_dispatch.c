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
#include "downlink_queue_app_dispatch.h"
#include "downlink_queue_app_cmds.h"
#include "downlink_queue_app_eventids.h"
#include "downlink_queue_app_msgids.h"
#include "downlink_queue_app_msg.h"

bool DOWNLINK_QUEUE_APP_VerifyCmdLength(const CFE_MSG_Message_t *MsgPtr, size_t ExpectedLength)
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

        CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_CMD_LEN_ERR_EID, CFE_EVS_EventType_ERROR,
                          "Invalid Msg length: ID = 0x%X,  CC = %u, Len = %u, Expected = %u",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId), (unsigned int)FcnCode, (unsigned int)ActualLength,
                          (unsigned int)ExpectedLength);

        result = false;

        DOWNLINK_QUEUE_APP_Data.ErrCounter++;
    }

    return result;
}

void DOWNLINK_QUEUE_APP_ProcessGroundCommand(const CFE_SB_Buffer_t *SBBufPtr)
{
    CFE_MSG_FcnCode_t CommandCode = 0;

    CFE_MSG_GetFcnCode(&SBBufPtr->Msg, &CommandCode);

    switch (CommandCode)
    {
        case DOWNLINK_QUEUE_APP_NOOP_CC:
            if (DOWNLINK_QUEUE_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DOWNLINK_QUEUE_APP_NoopCmd_t)))
            {
                DOWNLINK_QUEUE_APP_NoopCmd((const DOWNLINK_QUEUE_APP_NoopCmd_t *)SBBufPtr);
            }
            break;

        case DOWNLINK_QUEUE_APP_RESET_BUFFER_CC:
            if (DOWNLINK_QUEUE_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DOWNLINK_QUEUE_APP_ResetCountersCmd_t)))
            {
                DOWNLINK_QUEUE_APP_ResetCountersCmd((const DOWNLINK_QUEUE_APP_ResetCountersCmd_t *)SBBufPtr);
            }
            break;

        case DOWNLINK_QUEUE_APP_STAGE_DATA_CC:
            if (DOWNLINK_QUEUE_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DOWNLINK_QUEUE_APP_PushDataCmd_t)))
            {
                DOWNLINK_QUEUE_APP_PushDataCmd((const DOWNLINK_QUEUE_APP_PushDataCmd_t *)SBBufPtr);
            }
            break;

        case DOWNLINK_QUEUE_APP_FLUSH_BUFFER_CC:
            if (DOWNLINK_QUEUE_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DOWNLINK_QUEUE_APP_ClearCmd_t)))
            {
                DOWNLINK_QUEUE_APP_ClearCmd((const DOWNLINK_QUEUE_APP_ClearCmd_t *)SBBufPtr);
            }
            break;

        case DOWNLINK_QUEUE_APP_CREATE_STORE_CC:
            if (DOWNLINK_QUEUE_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DOWNLINK_QUEUE_APP_CreateStoreCmd_t)))
            {
                DOWNLINK_QUEUE_APP_CreateStoreCmd((const DOWNLINK_QUEUE_APP_CreateStoreCmd_t *)SBBufPtr);
            }
            break;

        case DOWNLINK_QUEUE_APP_COPY_STORE_CC:
            if (DOWNLINK_QUEUE_APP_VerifyCmdLength(&SBBufPtr->Msg, sizeof(DOWNLINK_QUEUE_APP_CopyStoreCmd_t)))
            {
                DOWNLINK_QUEUE_APP_CopyStoreCmd((const DOWNLINK_QUEUE_APP_CopyStoreCmd_t *)SBBufPtr);
            }
            break;

        default:
            CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_CC_ERR_EID, CFE_EVS_EventType_ERROR, "Invalid ground command code: CC = %d",
                              CommandCode);
            break;
    }
}

void DOWNLINK_QUEUE_APP_TaskPipe(const CFE_SB_Buffer_t *SBBufPtr)
{
    static CFE_SB_MsgId_t CMD_MID     = CFE_SB_MSGID_RESERVED;
    static CFE_SB_MsgId_t SEND_HK_MID = CFE_SB_MSGID_RESERVED;

    CFE_SB_MsgId_t MsgId = CFE_SB_INVALID_MSG_ID;

    if (!CFE_SB_IsValidMsgId(CMD_MID))
    {
        CMD_MID     = CFE_SB_ValueToMsgId(DOWNLINK_QUEUE_APP_CMD_MID);
        SEND_HK_MID = CFE_SB_ValueToMsgId(DOWNLINK_QUEUE_APP_SEND_HK_MID);
    }

    CFE_MSG_GetMsgId(&SBBufPtr->Msg, &MsgId);

    if (CFE_SB_MsgId_Equal(MsgId, SEND_HK_MID))
    {
        DOWNLINK_QUEUE_APP_SendHkCmd((const DOWNLINK_QUEUE_APP_SendHkCmd_t *)SBBufPtr);
    }
    else if (CFE_SB_MsgId_Equal(MsgId, CMD_MID))
    {
        DOWNLINK_QUEUE_APP_ProcessGroundCommand(SBBufPtr);
    }
    else
    {
        CFE_EVS_SendEvent(DOWNLINK_QUEUE_APP_MID_ERR_EID, CFE_EVS_EventType_ERROR,
                          "DOWNLINK_QUEUE_APP: invalid command packet, MID = 0x%x",
                          (unsigned int)CFE_SB_MsgIdToValue(MsgId));
    }
}
