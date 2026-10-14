/**
 * @file elliptical_lcb_cb.c
 * @brief LCB Callback Bridge for Elliptical Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/Elliptical/EllipticalCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives elliptical_handler_t* via user_data and dispatches
 * to the user-facing callbacks. Java's EllipticalCallbackHandlerLCB overrides
 * ~140 lcb_comm-receive-interface methods but only 2 do anything
 * (getLcbECBrpmHandler / getOneShotLcbECBrpmHandler) — those are the only two
 * wired here; the rest are intentionally not bridged, matching the Java no-ops.
 */

#include "elliptical_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "EllipticalHandlerLcbCb"

/* 0x63 GetECBrpm (auto-poll response) */
static void ep_on_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    elliptical_handler_t *eh = (elliptical_handler_t *)ud;
    if (eh->user_cb.on_elliptical_rpm)
        eh->user_cb.on_elliptical_rpm(eh->user_cb.user_data, rpm);
}

/* One-shot GetECBrpm */
static void ep_on_one_shot_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    elliptical_handler_t *eh = (elliptical_handler_t *)ud;
    if (eh->user_cb.on_elliptical_one_shot_rpm)
        eh->user_cb.on_elliptical_one_shot_rpm(eh->user_cb.user_data, rpm);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void ep_lcb_cb_register(elliptical_handler_t *eh)
{
    lcb_comm_callbacks_init(&eh->lcb_cb);
    eh->lcb_cb.user_data = eh;

    eh->lcb_cb.on_lcb_ecb_rpm          = ep_on_lcb_ecb_rpm;
    eh->lcb_cb.on_one_shot_lcb_ecb_rpm = ep_on_one_shot_lcb_ecb_rpm;
}

void ep_lcb_cb_unregister(elliptical_handler_t *eh)
{
    memset(&eh->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
