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
 *   Specification for the MODE_APP command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef MODE_APP_FCNCODES_H
#define MODE_APP_FCNCODES_H

#include "mode_app_fcncode_values.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** Mode App command codes
*/
#define MODE_APP_NOOP_CC          MODE_APP_CCVAL(NOOP)
#define MODE_APP_RESET_STATE_CC   MODE_APP_CCVAL(RESET_STATE)
#define MODE_APP_ADVANCE_MODE_CC  MODE_APP_CCVAL(ADVANCE_MODE)
#define MODE_APP_TOGGLE_POLICY_CC MODE_APP_CCVAL(TOGGLE_POLICY)
#define MODE_APP_SET_AP_STATE_CC  MODE_APP_CCVAL(SET_AP_STATE)
#define MODE_APP_DISABLE_CHECK_CC MODE_APP_CCVAL(DISABLE_CHECK)
#define MODE_APP_RESET_AP_STAT_CC MODE_APP_CCVAL(RESET_AP_STAT)

/* Compatibility aliases for existing local tests and tooling. */
#define MODE_APP_RESET_COUNTERS_CC MODE_APP_RESET_STATE_CC
#define MODE_APP_PROCESS_CC        MODE_APP_ADVANCE_MODE_CC
#define MODE_APP_DISPLAY_PARAM_CC  MODE_APP_TOGGLE_POLICY_CC

#endif
