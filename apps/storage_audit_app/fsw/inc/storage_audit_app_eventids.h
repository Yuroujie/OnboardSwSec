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
 * Define Storage Demo App Events IDs
 */

#ifndef STORAGE_AUDIT_APP_EVENTS_H
#define STORAGE_AUDIT_APP_EVENTS_H

#define STORAGE_AUDIT_APP_RESERVED_EID      0
#define STORAGE_AUDIT_APP_INIT_INF_EID      1
#define STORAGE_AUDIT_APP_CC_ERR_EID        2
#define STORAGE_AUDIT_APP_NOOP_INF_EID      3
#define STORAGE_AUDIT_APP_RESET_INF_EID     4
#define STORAGE_AUDIT_APP_MID_ERR_EID       5
#define STORAGE_AUDIT_APP_CMD_LEN_ERR_EID   6
#define STORAGE_AUDIT_APP_PIPE_ERR_EID      7
#define STORAGE_AUDIT_APP_VALUE_INF_EID     8
#define STORAGE_AUDIT_APP_CR_PIPE_ERR_EID   9
#define STORAGE_AUDIT_APP_SUB_HK_ERR_EID    10
#define STORAGE_AUDIT_APP_SUB_CMD_ERR_EID   11
#define STORAGE_AUDIT_APP_TABLE_REG_ERR_EID 12
#define STORAGE_AUDIT_APP_WINDOW_OPEN_EID   13
#define STORAGE_AUDIT_APP_RELEASE_INF_EID   14
#define STORAGE_AUDIT_APP_VULN_HIT_EID      15

#endif /* STORAGE_AUDIT_APP_EVENTS_H */
