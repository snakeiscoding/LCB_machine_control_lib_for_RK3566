/**
 * @file ascent_lcb_cb.c
 * @brief LCB Callback Bridge for Ascent Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/AscentTrainer/AscentCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives ascent_handler_t* via user_data and dispatches to
 * the user-facing callbacks. Java's AscentCallbackHandlerLCB overrides ~100
 * lcb_comm-receive-interface methods but only 3 do anything
 * (getLcbECBrpmHandler / getOneShotLcbECBrpmHandler / getLcbInclinePercentHandler)
 * — those are the only three wired here; the rest are intentionally not
 * bridged, matching the Java no-ops.
 */

#include "ascent_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "AscentHandlerLcbCb"

/* 0x63 GetECBrpm (auto-poll response) */
static void ah_on_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    ascent_handler_t *ah = (ascent_handler_t *)ud;
    if (ah->user_cb.on_ascent_rpm)
        ah->user_cb.on_ascent_rpm(ah->user_cb.user_data, rpm);
}

/* One-shot GetECBrpm */
static void ah_on_one_shot_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    ascent_handler_t *ah = (ascent_handler_t *)ud;
    if (ah->user_cb.on_ascent_one_shot_rpm)
        ah->user_cb.on_ascent_one_shot_rpm(ah->user_cb.user_data, rpm);
}

/* GetInclinePercent (carries the value — Java: getLcbInclinePercentHandler(int)) */
static void ah_on_lcb_incline_percent_received(void *ud, uint16_t percent)
{
    ascent_handler_t *ah = (ascent_handler_t *)ud;
    if (ah->user_cb.on_ascent_incline)
        ah->user_cb.on_ascent_incline(ah->user_cb.user_data, percent);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void ah_lcb_cb_register(ascent_handler_t *ah)
{
    lcb_comm_callbacks_init(&ah->lcb_cb);
    ah->lcb_cb.user_data = ah;

    ah->lcb_cb.on_lcb_ecb_rpm                = ah_on_lcb_ecb_rpm;
    ah->lcb_cb.on_one_shot_lcb_ecb_rpm       = ah_on_one_shot_lcb_ecb_rpm;
    ah->lcb_cb.on_lcb_incline_percent_received = ah_on_lcb_incline_percent_received;
}

void ah_lcb_cb_unregister(ascent_handler_t *ah)
{
    memset(&ah->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
