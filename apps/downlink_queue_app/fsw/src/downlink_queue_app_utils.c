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
#include "downlink_queue_app_eventids.h"
#include "downlink_queue_app_tbl.h"
#include "downlink_queue_app_utils.h"

CFE_Status_t DOWNLINK_QUEUE_APP_TblValidationFunc(void *TblData)
{
    CFE_Status_t               ReturnCode = CFE_SUCCESS;
    DOWNLINK_QUEUE_APP_ExampleTable_t *TblDataPtr = (DOWNLINK_QUEUE_APP_ExampleTable_t *)TblData;

    if (TblDataPtr->Int1 > DOWNLINK_QUEUE_APP_PLATFORM_TBL_ELEMENT_1_MAX)
    {
        ReturnCode = DOWNLINK_QUEUE_APP_PLATFORM_TABLE_OUT_OF_RANGE_ERR_CODE;
    }

    return ReturnCode;
}

void DOWNLINK_QUEUE_APP_GetCrc(const char *TableName)
{
    CFE_Status_t   status;
    uint32         Crc;
    CFE_TBL_Info_t TblInfoPtr;

    status = CFE_TBL_GetInfo(&TblInfoPtr, TableName);
    if (status != CFE_SUCCESS)
    {
        CFE_ES_WriteToSysLog("Storage Buffer App: Error getting buffer policy table info");
    }
    else
    {
        Crc = TblInfoPtr.Crc;
        CFE_ES_WriteToSysLog("Storage Buffer App: CRC: 0x%08lX\n\n", (unsigned long)Crc);
    }
}
