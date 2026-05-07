#include "payload_task_app.h"

#define PAYLOAD_TASK_APP_FILTER_WINDOW 8U
#define PAYLOAD_TASK_APP_MAX_CHANNELS  16U

static uint32 PAYLOAD_TASK_APP_MixSample(uint32 sample, uint32 salt)
{
    uint32 mixed = sample ^ (salt << 3);
    uint32 i;

    for (i = 0; i < 4U; ++i)
    {
        mixed = (mixed << 1) ^ (mixed >> 3) ^ (salt + i);
    }

    return mixed;
}

static uint32 PAYLOAD_TASK_APP_ClampDelta(uint32 current, uint32 previous, uint32 limit)
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

static uint32 PAYLOAD_TASK_APP_WindowScore(const uint32 *samples, uint32 count)
{
    uint32 i;
    uint32 score = 0;
    uint32 limit = count;

    if (limit > PAYLOAD_TASK_APP_FILTER_WINDOW)
    {
        limit = PAYLOAD_TASK_APP_FILTER_WINDOW;
    }

    for (i = 0; i < limit; ++i)
    {
        score += PAYLOAD_TASK_APP_MixSample(samples[i], i + count);
    }

    return score;
}

static bool PAYLOAD_TASK_APP_CheckMonotonic(const uint32 *samples, uint32 count)
{
    uint32 i;
    bool valid = true;

    for (i = 1; i < count && i < PAYLOAD_TASK_APP_FILTER_WINDOW; ++i)
    {
        if (samples[i] < samples[i - 1])
        {
            valid = false;
        }
    }

    return valid;
}

static uint32 PAYLOAD_TASK_APP_EstimateBacklog(uint32 ingress, uint32 egress, uint32 priority)
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

static uint32 PAYLOAD_TASK_APP_ApplyHealthPenalty(uint32 score, uint32 errors, uint32 stale_count)
{
    if (errors > 0U)
    {
        score += errors * 5U;
    }

    if (stale_count > PAYLOAD_TASK_APP_MAX_CHANNELS)
    {
        score += stale_count;
    }

    return score;
}

static uint32 PAYLOAD_TASK_APP_SelectRecoveryAction(uint32 score)
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

static void PAYLOAD_TASK_APP_UpdateShadowState(uint32 *shadow, uint32 action, uint32 score)
{
    if (shadow != 0)
    {
        shadow[0] = action;
        shadow[1] = score;
        shadow[2] = PAYLOAD_TASK_APP_ClampDelta(score, shadow[2], 255U);
    }
}

uint32 PAYLOAD_TASK_APP_EvaluateScenarioWindow(const uint32 *samples, uint32 count, uint32 ingress, uint32 egress)
{
    uint32 shadow[3] = {0, 0, 0};
    uint32 score;
    uint32 backlog;
    uint32 action;

    score = PAYLOAD_TASK_APP_WindowScore(samples, count);
    if (!PAYLOAD_TASK_APP_CheckMonotonic(samples, count))
    {
        score += 25U;
    }

    backlog = PAYLOAD_TASK_APP_EstimateBacklog(ingress, egress, count);
    score   = PAYLOAD_TASK_APP_ApplyHealthPenalty(score, backlog, count);
    action  = PAYLOAD_TASK_APP_SelectRecoveryAction(score);
    PAYLOAD_TASK_APP_UpdateShadowState(shadow, action, score);

    return shadow[1] ^ shadow[2] ^ action;
}
