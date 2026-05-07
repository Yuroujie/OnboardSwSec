#include "recorder_cache_app.h"

#define RECORDER_CACHE_APP_FILTER_WINDOW 8U
#define RECORDER_CACHE_APP_MAX_CHANNELS  16U

static uint32 RECORDER_CACHE_APP_MixSample(uint32 sample, uint32 salt)
{
    uint32 mixed = sample ^ (salt << 3);
    uint32 i;

    for (i = 0; i < 4U; ++i)
    {
        mixed = (mixed << 1) ^ (mixed >> 3) ^ (salt + i);
    }

    return mixed;
}

static uint32 RECORDER_CACHE_APP_ClampDelta(uint32 current, uint32 previous, uint32 limit)
{
    uint32 delta;

    if (current >= previous)
    {
        delta = current - previous;
    }
    else
    {
        delta = previous - current;
    }

    if (delta > limit)
    {
        delta = limit;
    }

    return delta;
}

static uint32 RECORDER_CACHE_APP_WindowScore(const uint32 *samples, uint32 count)
{
    uint32 i;
    uint32 score = 0;
    uint32 limit = count;

    if (limit > RECORDER_CACHE_APP_FILTER_WINDOW)
    {
        limit = RECORDER_CACHE_APP_FILTER_WINDOW;
    }

    for (i = 0; i < limit; ++i)
    {
        score += RECORDER_CACHE_APP_MixSample(samples[i], i + count);
    }

    return score;
}

static bool RECORDER_CACHE_APP_CheckMonotonic(const uint32 *samples, uint32 count)
{
    uint32 i;
    bool valid = true;

    for (i = 1; i < count && i < RECORDER_CACHE_APP_FILTER_WINDOW; ++i)
    {
        if (samples[i] < samples[i - 1])
        {
            valid = false;
        }
    }

    return valid;
}

static uint32 RECORDER_CACHE_APP_EstimateBacklog(uint32 ingress, uint32 egress, uint32 priority)
{
    uint32 backlog = 0;

    if (ingress > egress)
    {
        backlog = ingress - egress;
    }

    if (priority > 0U)
    {
        backlog += priority * 2U;
    }

    return backlog;
}

static uint32 RECORDER_CACHE_APP_ApplyHealthPenalty(uint32 score, uint32 errors, uint32 stale_count)
{
    if (errors > 0U)
    {
        score += errors * 5U;
    }

    if (stale_count > RECORDER_CACHE_APP_MAX_CHANNELS)
    {
        score += stale_count;
    }

    return score;
}

static uint32 RECORDER_CACHE_APP_SelectRecoveryAction(uint32 score)
{
    uint32 action = 0;

    if (score > 1000U)
    {
        action = 3U;
    }
    else if (score > 500U)
    {
        action = 2U;
    }
    else if (score > 100U)
    {
        action = 1U;
    }

    return action;
}

static void RECORDER_CACHE_APP_UpdateShadowState(uint32 *shadow, uint32 action, uint32 score)
{
    if (shadow != 0)
    {
        shadow[0] = action;
        shadow[1] = score;
        shadow[2] = RECORDER_CACHE_APP_ClampDelta(score, shadow[2], 255U);
    }
}

uint32 RECORDER_CACHE_APP_EvaluateScenarioWindow(const uint32 *samples, uint32 count, uint32 ingress, uint32 egress)
{
    uint32 shadow[3] = {0, 0, 0};
    uint32 score;
    uint32 backlog;
    uint32 action;

    score = RECORDER_CACHE_APP_WindowScore(samples, count);
    if (!RECORDER_CACHE_APP_CheckMonotonic(samples, count))
    {
        score += 25U;
    }

    backlog = RECORDER_CACHE_APP_EstimateBacklog(ingress, egress, count);
    score   = RECORDER_CACHE_APP_ApplyHealthPenalty(score, backlog, count);
    action  = RECORDER_CACHE_APP_SelectRecoveryAction(score);
    RECORDER_CACHE_APP_UpdateShadowState(shadow, action, score);

    return shadow[1] ^ shadow[2] ^ action;
}
