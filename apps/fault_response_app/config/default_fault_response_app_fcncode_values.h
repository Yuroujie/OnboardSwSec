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
 *   Specification for the CFE Executive Services (CFE_ES) command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef DEFAULT_FAULT_RESPONSE_APP_FCNCODE_VALUES_H
#define DEFAULT_FAULT_RESPONSE_APP_FCNCODE_VALUES_H

/************************************************************************
 * Macro Definitions
 ************************************************************************/

#define FAULT_RESPONSE_APP_CCVAL(x) FAULT_RESPONSE_APP_FunctionCode_##x

enum FAULT_RESPONSE_APP_FunctionCode_
{
    FAULT_RESPONSE_APP_FunctionCode_NOOP          = 0,
    FAULT_RESPONSE_APP_FunctionCode_RESET_STATE   = 1,
    FAULT_RESPONSE_APP_FunctionCode_ADVANCE_MODE  = 2,
    FAULT_RESPONSE_APP_FunctionCode_TOGGLE_POLICY = 3,
    FAULT_RESPONSE_APP_FunctionCode_SET_AP_STATE  = 4,
    FAULT_RESPONSE_APP_FunctionCode_DISABLE_CHECK = 5,
    FAULT_RESPONSE_APP_FunctionCode_RESET_AP_STAT = 6,
};

#endif
