/**
 * @file bike_lcb_cb.c
 * @brief LCB Callback Bridge for Bike Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/Bike/BikeCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives bike_handler_t* via user_data and dispatches to the
 * user-facing callbacks. Java's BikeCallbackHandlerLCB overrides ~100
 * lcb_comm-receive-interface methods but only 2 do anything
 * (getLcbECBrpmHandler / getOneShotLcbECBrpmHandler) — those are the only two
 * wired here; the rest are intentionally not bridged, matching the Java no-ops.
 */

#include "bike_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "BikeHandlerLcbCb"

/* 0x63 GetECBrpm (auto-poll response) */
static void bh_on_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    bike_handler_t *bh = (bike_handler_t *)ud;
    if (bh->user_cb.on_bike_rpm)
        bh->user_cb.on_bike_rpm(bh->user_cb.user_data, rpm);
}

/* One-shot GetECBrpm */
static void bh_on_one_shot_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    bike_handler_t *bh = (bike_handler_t *)ud;
    if (bh->user_cb.on_bike_one_shot_rpm)
        bh->user_cb.on_bike_one_shot_rpm(bh->user_cb.user_data, rpm);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void bh_lcb_cb_register(bike_handler_t *bh)
{
    lcb_comm_callbacks_init(&bh->lcb_cb);
    bh->lcb_cb.user_data = bh;

    bh->lcb_cb.on_lcb_ecb_rpm         = bh_on_lcb_ecb_rpm;
    bh->lcb_cb.on_one_shot_lcb_ecb_rpm = bh_on_one_shot_lcb_ecb_rpm;
}

void bh_lcb_cb_unregister(bike_handler_t *bh)
{
    memset(&bh->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
