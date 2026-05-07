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
 *   Specification for the STORAGE_AUDIT_APP command function codes
 *
 * @note
 *   This file should be strictly limited to the command/function code (CC)
 *   macro definitions.  Other definitions such as enums, typedefs, or other
 *   macros should be placed in the msgdefs.h or msg.h files.
 */
#ifndef STORAGE_AUDIT_APP_FCNCODES_H
#define STORAGE_AUDIT_APP_FCNCODES_H

#include "storage_audit_app_fcncode_values.h"

/************************************************************************
 * Macro Definitions
 ************************************************************************/

/*
** Storage Demo App command codes
*/
#define STORAGE_AUDIT_APP_NOOP_CC         STORAGE_AUDIT_APP_CCVAL(NOOP)
#define STORAGE_AUDIT_APP_RESET_BUFFER_CC STORAGE_AUDIT_APP_CCVAL(RESET_BUFFER)
#define STORAGE_AUDIT_APP_STAGE_DATA_CC   STORAGE_AUDIT_APP_CCVAL(STAGE_DATA)
#define STORAGE_AUDIT_APP_FLUSH_BUFFER_CC STORAGE_AUDIT_APP_CCVAL(FLUSH_BUFFER)
#define STORAGE_AUDIT_APP_CREATE_STORE_CC STORAGE_AUDIT_APP_CCVAL(CREATE_STORE)
#define STORAGE_AUDIT_APP_COPY_STORE_CC   STORAGE_AUDIT_APP_CCVAL(COPY_STORE)

/* Compatibility aliases for existing local tests and tooling. */
#define STORAGE_AUDIT_APP_RESET_COUNTERS_CC STORAGE_AUDIT_APP_RESET_BUFFER_CC
#define STORAGE_AUDIT_APP_PUSH_DATA_CC      STORAGE_AUDIT_APP_STAGE_DATA_CC
#define STORAGE_AUDIT_APP_CLEAR_CC          STORAGE_AUDIT_APP_FLUSH_BUFFER_CC

#endif
