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
 *   Specification for the PERIODIC_TASK_APP command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef PERIODIC_TASK_APP_FCNCODES_H
#define PERIODIC_TASK_APP_FCNCODES_H

#include "periodic_task_app_fcncode_values.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** Counter App command codes
*/
#define PERIODIC_TASK_APP_NOOP_CC          PERIODIC_TASK_APP_CCVAL(NOOP)
#define PERIODIC_TASK_APP_RESET_STATE_CC   PERIODIC_TASK_APP_CCVAL(RESET_STATE)
#define PERIODIC_TASK_APP_INCREASE_STEP_CC PERIODIC_TASK_APP_CCVAL(INCREASE_STEP)
#define PERIODIC_TASK_APP_DECREASE_STEP_CC PERIODIC_TASK_APP_CCVAL(DECREASE_STEP)

/* Compatibility aliases for existing local tests and tooling. */
#define PERIODIC_TASK_APP_RESET_COUNTERS_CC PERIODIC_TASK_APP_RESET_STATE_CC
#define PERIODIC_TASK_APP_PROCESS_CC        PERIODIC_TASK_APP_INCREASE_STEP_CC
#define PERIODIC_TASK_APP_DISPLAY_PARAM_CC  PERIODIC_TASK_APP_DECREASE_STEP_CC

#endif
