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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "autonomy_guard_app.h"
#include "autonomy_guard_app_cmds.h"
#include "autonomy_guard_app_msgids.h"
#include "autonomy_guard_app_eventids.h"
#include "autonomy_guard_app_version.h"
#include "autonomy_guard_app_tbl.h"
#include "autonomy_guard_app_utils.h"
#include "autonomy_guard_app_msg.h"

#include "sample_lib.h"

#define AUTONOMY_GUARD_APP_WINDOW_PERIOD_SEC 40U
#define AUTONOMY_GUARD_APP_WINDOW_OPEN_SEC   20U
#define AUTONOMY_GUARD_APP_C1_WEIGHT_PCT     10U
#define AUTONOMY_GUARD_APP_C2_WEIGHT_PCT     90U
#define AUTONOMY_GUARD_APP_DEFAULT_SEED      0x4D4F4445U
#define AUTONOMY_GUARD_APP_UP_DEFAULT_PERIOD_SEC   16U
#define AUTONOMY_GUARD_APP_UP_DEFAULT_OPEN_LEN_SEC 6U
#define AUTONOMY_GUARD_APP_SIM_AP_COUNT             176U
#define AUTONOMY_GUARD_APP_SIM_AP_ACTIVE_STATE      1U
#define AUTONOMY_GUARD_APP_SIM_AP_PASSIVE_STATE     2U
#define AUTONOMY_GUARD_APP_SIM_AP_DISABLED_STATE    3U

static uint64 AUTONOMY_GUARD_APP_WindowEpochSec = 0;
static uint32 AUTONOMY_GUARD_APP_WindowSeed     = 0;

static uint64 AUTONOMY_GUARD_APP_GetMonotonicSec(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64)ts.tv_sec;
}

static uint32 AUTONOMY_GUARD_APP_GetSeedBase(void)
{
    const char *seed_text = getenv("AUTONOMY_GUARD_APP_POLICY_SEED");
    char *      end_ptr   = NULL;
    unsigned long parsed  = 0;

    if (seed_text != NULL && seed_text[0] != '\0')
    {
        parsed = strtoul(seed_text, &end_ptr, 0);
        if (end_ptr != seed_text && parsed != 0)
        {
            return (uint32)parsed;
        }
    }

    return AUTONOMY_GUARD_APP_DEFAULT_SEED;
}

static uint32 AUTONOMY_GUARD_APP_ParseEnvU32(const char *Name, uint32 DefaultValue)
{
    const char *text = getenv(Name);
    char *      end_ptr = NULL;
    unsigned long parsed = 0;

    if (text != NULL && text[0] != '\0')
    {
        parsed = strtoul(text, &end_ptr, 0);
        if (end_ptr != text)
        {
            return (uint32)parsed;
        }
    }

    return DefaultValue;
}

static bool AUTONOMY_GUARD_APP_IsFourWindowEnabled(void)
{
    const char *mode   = getenv("AUTONOMY_GUARD_APP_POLICY_PROFILE");
    const char *active = getenv("AUTONOMY_GUARD_APP_POLICY_ACTIVE");

    return (mode != NULL && strcmp(mode, "periodic") == 0 && active != NULL && strcmp(active, "1") == 0);
}

static bool AUTONOMY_GUARD_APP_IsLegacyWindowEnabled(void)
{
    const char *mode = getenv("AUTONOMY_GUARD_APP_POLICY_MODE");
    return (mode != NULL && mode[0] != '\0');
}

static bool AUTONOMY_GUARD_APP_IntervalsOpen(const char *Intervals, uint32 ElapsedSec)
{
    const char *cursor = Intervals;

    while (cursor != NULL && cursor[0] != '\0')
    {
        char *end_ptr = NULL;
        unsigned long start = strtoul(cursor, &end_ptr, 0);
        unsigned long end = start;

        if (end_ptr == cursor)
        {
            break;
        }

        if (*end_ptr == '-')
        {
            cursor = end_ptr + 1;
            end = strtoul(cursor, &end_ptr, 0);
            if (end_ptr == cursor)
            {
                end = start;
            }
        }

        if (ElapsedSec >= start && ElapsedSec <= end)
        {
            return true;
        }

        cursor = strchr(end_ptr, ',');
        if (cursor != NULL)
        {
            ++cursor;
        }
    }

    return false;
}

static void AUTONOMY_GUARD_APP_ResetWindowState(void)
{
    AUTONOMY_GUARD_APP_WindowEpochSec = AUTONOMY_GUARD_APP_GetMonotonicSec();
    AUTONOMY_GUARD_APP_WindowSeed     = AUTONOMY_GUARD_APP_GetSeedBase() ^ 0x5A5A5A5AU;

    if (AUTONOMY_GUARD_APP_WindowSeed == 0)
    {
        AUTONOMY_GUARD_APP_WindowSeed = AUTONOMY_GUARD_APP_DEFAULT_SEED;
    }
}

static uint32 AUTONOMY_GUARD_APP_NextWindowDraw(void)
{
    AUTONOMY_GUARD_APP_WindowSeed = (AUTONOMY_GUARD_APP_WindowSeed * 1103515245U) + 12345U;
    return (AUTONOMY_GUARD_APP_WindowSeed >> 16) % 100U;
}

static bool AUTONOMY_GUARD_APP_CheckFourWindow(const char *CommandName)
{
    const char *interval_text = getenv("AUTONOMY_GUARD_APP_POLICY_INTERVALS");
    uint32      period_sec;
    uint32      phase_origin;
    uint32      open_len_sec;
    uint32      elapsed_sec;
    bool        intervals_open = false;
    bool        periodic_open;
    bool        allowed;

    if (AUTONOMY_GUARD_APP_WindowEpochSec == 0)
    {
        AUTONOMY_GUARD_APP_ResetWindowState();
    }

    period_sec = AUTONOMY_GUARD_APP_ParseEnvU32("AUTONOMY_GUARD_APP_POLICY_PERIOD_SEC", AUTONOMY_GUARD_APP_UP_DEFAULT_PERIOD_SEC);
    phase_origin = AUTONOMY_GUARD_APP_ParseEnvU32("AUTONOMY_GUARD_APP_POLICY_PHASE_SEC", 0U);
    open_len_sec = AUTONOMY_GUARD_APP_ParseEnvU32("AUTONOMY_GUARD_APP_POLICY_OPEN_LEN_SEC", AUTONOMY_GUARD_APP_UP_DEFAULT_OPEN_LEN_SEC);
    elapsed_sec = (uint32)(AUTONOMY_GUARD_APP_GetMonotonicSec() - AUTONOMY_GUARD_APP_WindowEpochSec);
    periodic_open = (period_sec > 0U) && (((elapsed_sec + period_sec - phase_origin) % period_sec) < open_len_sec);

    if (interval_text != NULL && interval_text[0] != '\0')
    {
        intervals_open = AUTONOMY_GUARD_APP_IntervalsOpen(interval_text, elapsed_sec);
    }

    allowed = intervals_open || periodic_open;
    if (!allowed)
    {
        ++AUTONOMY_GUARD_APP_Data.ErrCounter;
        CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_WINDOW_REJECT_EID, CFE_EVS_EventType_INFORMATION,
                          "AUTONOMY_GUARD_APP: %s deferred by command acceptance schedule (elapsed=%u, interval=%u, periodic=%u)",
                          CommandName,
                          (unsigned int)elapsed_sec, (unsigned int)intervals_open, (unsigned int)periodic_open);
        AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    }

    return allowed;
}

static bool AUTONOMY_GUARD_APP_CheckLegacyWindow(const char *CommandName)
{
    const char *mode_text = getenv("AUTONOMY_GUARD_APP_POLICY_MODE");
    bool        w1_open;
    bool        w2_open;
    bool        allowed;
    uint32      phase_sec;
    uint32      activation_pct;

    if (AUTONOMY_GUARD_APP_WindowEpochSec == 0 || AUTONOMY_GUARD_APP_WindowSeed == 0)
    {
        AUTONOMY_GUARD_APP_ResetWindowState();
    }

    phase_sec = (uint32)((AUTONOMY_GUARD_APP_GetMonotonicSec() - AUTONOMY_GUARD_APP_WindowEpochSec) % AUTONOMY_GUARD_APP_WINDOW_PERIOD_SEC);
    w1_open   = (phase_sec < AUTONOMY_GUARD_APP_WINDOW_OPEN_SEC);
    if (mode_text != NULL && strcmp(mode_text, "staggered") == 0)
    {
        w2_open = !w1_open;
    }
    else
    {
        w2_open = w1_open;
    }

    activation_pct = (w1_open ? AUTONOMY_GUARD_APP_C1_WEIGHT_PCT : 0U) + (w2_open ? AUTONOMY_GUARD_APP_C2_WEIGHT_PCT : 0U);
    allowed        = (AUTONOMY_GUARD_APP_NextWindowDraw() < activation_pct);

    if (!allowed)
    {
        ++AUTONOMY_GUARD_APP_Data.ErrCounter;
        CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_WINDOW_REJECT_EID, CFE_EVS_EventType_INFORMATION,
                          "AUTONOMY_GUARD_APP: %s deferred by command acceptance policy (mode=%s, phase=%u, pct=%u)",
                          CommandName, (mode_text != NULL && mode_text[0] != '\0') ? mode_text : "aligned",
                          (unsigned int)phase_sec, (unsigned int)activation_pct);
        AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    }

    return allowed;
}

static bool AUTONOMY_GUARD_APP_CheckTemporalWindow(const char *CommandName)
{
    if (AUTONOMY_GUARD_APP_IsFourWindowEnabled())
    {
        return AUTONOMY_GUARD_APP_CheckFourWindow(CommandName);
    }

    if (AUTONOMY_GUARD_APP_IsLegacyWindowEnabled())
    {
        return AUTONOMY_GUARD_APP_CheckLegacyWindow(CommandName);
    }

    return true;
}

CFE_Status_t AUTONOMY_GUARD_APP_SendHkCmd(const AUTONOMY_GUARD_APP_SendHkCmd_t *Msg)
{
    int i;

    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.CommandErrorCounter = AUTONOMY_GUARD_APP_Data.ErrCounter;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.CommandCounter      = AUTONOMY_GUARD_APP_Data.CmdCounter;

    CFE_SB_TimeStampMsg(CFE_MSG_PTR(AUTONOMY_GUARD_APP_Data.HkTlm.TelemetryHeader));
    CFE_SB_TransmitMsg(CFE_MSG_PTR(AUTONOMY_GUARD_APP_Data.HkTlm.TelemetryHeader), true);

    for (i = 0; i < AUTONOMY_GUARD_APP_PLATFORM_NUMBER_OF_TABLES; i++)
    {
        CFE_TBL_Manage(AUTONOMY_GUARD_APP_Data.TblHandles[i]);
    }

    return CFE_SUCCESS;
}

CFE_Status_t AUTONOMY_GUARD_APP_NoopCmd(const AUTONOMY_GUARD_APP_NoopCmd_t *Msg)
{
    (void)Msg;

    AUTONOMY_GUARD_APP_Data.CmdCounter++;

    CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_NOOP_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "AUTONOMY_GUARD_APP: NOOP command accepted (%s)", AUTONOMY_GUARD_APP_VERSION);

    AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t AUTONOMY_GUARD_APP_ResetCountersCmd(const AUTONOMY_GUARD_APP_ResetCountersCmd_t *Msg)
{
    (void)Msg;

    AUTONOMY_GUARD_APP_ResetWindowState();
    AUTONOMY_GUARD_APP_Data.CmdCounter = 0;
    AUTONOMY_GUARD_APP_Data.ErrCounter = 0;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.PolicyEnabled  = 1;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ActiveMode     = 0;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ModeChangeCount = 0;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.MonitorActiveCount = AUTONOMY_GUARD_APP_SIM_AP_COUNT;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.MonitorPassiveCount = 0;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.MonitorDisabledCount = 0;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ChecksumDisabledCount = 0;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ApStatsResetCount = 0;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.LastActionPoint = 0;

    CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_RESET_INF_EID, CFE_EVS_EventType_INFORMATION, "AUTONOMY_GUARD_APP: state reset command");

    AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t AUTONOMY_GUARD_APP_SetApStateCmd(const AUTONOMY_GUARD_APP_SetApStateCmd_t *Msg)
{
    uint8 new_state = Msg->Payload.NewApState;

    if (!AUTONOMY_GUARD_APP_CheckTemporalWindow("SET_AP_STATE"))
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    AUTONOMY_GUARD_APP_Data.CmdCounter++;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.LastActionPoint = Msg->Payload.ApNumber;

    if (new_state == AUTONOMY_GUARD_APP_SIM_AP_ACTIVE_STATE)
    {
        AUTONOMY_GUARD_APP_Data.HkTlm.Payload.MonitorActiveCount++;
    }
    else if (new_state == AUTONOMY_GUARD_APP_SIM_AP_PASSIVE_STATE)
    {
        AUTONOMY_GUARD_APP_Data.HkTlm.Payload.MonitorPassiveCount++;
    }
    else if (new_state == AUTONOMY_GUARD_APP_SIM_AP_DISABLED_STATE)
    {
        AUTONOMY_GUARD_APP_Data.HkTlm.Payload.MonitorDisabledCount++;
    }
    else
    {
        AUTONOMY_GUARD_APP_Data.ErrCounter++;
        CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_VALUE_INF_EID, CFE_EVS_EventType_ERROR,
                          "AUTONOMY_GUARD_APP: invalid simulated AP state %u", (unsigned int)new_state);
        AUTONOMY_GUARD_APP_SendHkCmd(NULL);
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ModeChangeCount++;

    CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "AUTONOMY_GUARD_APP: simulated AP %u set to state %u",
                      (unsigned int)Msg->Payload.ApNumber, (unsigned int)new_state);

    AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t AUTONOMY_GUARD_APP_DisableCheckCmd(const AUTONOMY_GUARD_APP_DisableCheckCmd_t *Msg)
{
    if (!AUTONOMY_GUARD_APP_CheckTemporalWindow("DISABLE_CHECK"))
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    AUTONOMY_GUARD_APP_Data.CmdCounter++;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ChecksumDisabledCount++;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ModeChangeCount++;

    CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "AUTONOMY_GUARD_APP: simulated checksum entry %lu disabled",
                      (unsigned long)Msg->Payload.EntryId);

    AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t AUTONOMY_GUARD_APP_ResetApStatsCmd(const AUTONOMY_GUARD_APP_ResetApStatsCmd_t *Msg)
{
    if (!AUTONOMY_GUARD_APP_CheckTemporalWindow("RESET_AP_STATS"))
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    AUTONOMY_GUARD_APP_Data.CmdCounter++;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ApStatsResetCount++;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.LastActionPoint = Msg->Payload.ApNumber;

    CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "AUTONOMY_GUARD_APP: simulated AP %u statistics reset",
                      (unsigned int)Msg->Payload.ApNumber);

    AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t AUTONOMY_GUARD_APP_ProcessCmd(const AUTONOMY_GUARD_APP_ProcessCmd_t *Msg)
{
    (void)Msg;

    if (!AUTONOMY_GUARD_APP_CheckTemporalWindow("ADVANCE_MODE"))
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    AUTONOMY_GUARD_APP_Data.CmdCounter++;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ActiveMode = (AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ActiveMode + 1) % 3;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ModeChangeCount++;

    CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "AUTONOMY_GUARD_APP: active mode advanced to %u", AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ActiveMode);

    AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}

CFE_Status_t AUTONOMY_GUARD_APP_DisplayParamCmd(const AUTONOMY_GUARD_APP_DisplayParamCmd_t *Msg)
{
    (void)Msg;

    if (!AUTONOMY_GUARD_APP_CheckTemporalWindow("TOGGLE_POLICY"))
    {
        return CFE_STATUS_EXTERNAL_RESOURCE_FAIL;
    }

    AUTONOMY_GUARD_APP_Data.CmdCounter++;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.PolicyEnabled = !AUTONOMY_GUARD_APP_Data.HkTlm.Payload.PolicyEnabled;
    AUTONOMY_GUARD_APP_Data.HkTlm.Payload.ModeChangeCount++;

    CFE_EVS_SendEvent(AUTONOMY_GUARD_APP_VALUE_INF_EID, CFE_EVS_EventType_INFORMATION,
                      "AUTONOMY_GUARD_APP: admission policy enabled set to %u", AUTONOMY_GUARD_APP_Data.HkTlm.Payload.PolicyEnabled);

    AUTONOMY_GUARD_APP_SendHkCmd(NULL);
    return CFE_SUCCESS;
}
