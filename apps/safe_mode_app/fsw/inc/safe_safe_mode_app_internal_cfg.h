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
 *
 * SAFE_SAFE_MODE_APP Application Platform Configuration Header File
 *
 * This is a compatibility header for the "platform_cfg.h" file that has
 * traditionally provided both public and private config definitions
 * for each CFS app.
 *
 * These definitions are now provided in two separate files, one for
 * the public/mission scope and one for internal scope.
 *
 * @note This file may be overridden/superceded by mission-provided definitions
 * either by overriding this header or by generating definitions from a command/data
 * dictionary tool.
 */
#ifndef SAFE_SAFE_MODE_APP_INTERNAL_CFG_H
#define SAFE_SAFE_MODE_APP_INTERNAL_CFG_H

#include "safe_mode_app_internal_cfg_values.h"

/***********************************************************************/
#define SAFE_SAFE_MODE_APP_PLATFORM_PIPE_DEPTH         SAFE_SAFE_MODE_APP_PLATFORM_CFGVAL(PIPE_DEPTH)
#define DEFAULT_SAFE_SAFE_MODE_APP_PLATFORM_PIPE_DEPTH 32 /* Depth of the Command Pipe for Application */

#define SAFE_SAFE_MODE_APP_PLATFORM_PIPE_NAME         SAFE_SAFE_MODE_APP_PLATFORM_CFGVAL(PIPE_NAME)
#define DEFAULT_SAFE_SAFE_MODE_APP_PLATFORM_PIPE_NAME "SAFE_SAFE_MODE_APP_CMD_PIPE"

#define SAFE_SAFE_MODE_APP_PLATFORM_NUMBER_OF_TABLES         SAFE_SAFE_MODE_APP_PLATFORM_CFGVAL(NUMBER_OF_TABLES)
#define DEFAULT_SAFE_SAFE_MODE_APP_PLATFORM_NUMBER_OF_TABLES 1 /* Number of Example Table(s) */

#define SAFE_SAFE_MODE_APP_PLATFORM_TABLE_OUT_OF_RANGE_ERR_CODE         SAFE_SAFE_MODE_APP_PLATFORM_CFGVAL(TABLE_OUT_OF_RANGE_ERR_CODE)
#define DEFAULT_SAFE_SAFE_MODE_APP_PLATFORM_TABLE_OUT_OF_RANGE_ERR_CODE -1

#define SAFE_SAFE_MODE_APP_PLATFORM_TBL_ELEMENT_1_MAX         SAFE_SAFE_MODE_APP_PLATFORM_CFGVAL(TBL_ELEMENT_1_MAX)
#define DEFAULT_SAFE_SAFE_MODE_APP_PLATFORM_TBL_ELEMENT_1_MAX 10

#define SAFE_SAFE_MODE_APP_PLATFORM_TABLE_FILE         SAFE_SAFE_MODE_APP_PLATFORM_CFGVAL(TABLE_FILE)
#define DEFAULT_SAFE_SAFE_MODE_APP_PLATFORM_TABLE_FILE "/cf/safe_mode_app_tbl.tbl"

#endif
