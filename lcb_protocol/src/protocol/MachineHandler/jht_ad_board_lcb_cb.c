/**
 * @file jht_ad_board_lcb_cb.c
 * @brief LCB Callback Bridge for JhtADboard Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/JhtADboard/JhtADboardCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives jht_ad_board_handler_t* via user_data and dispatches
 * to the user-facing callbacks. Java's JhtADboardCallbackHandlerLCB overrides
 * ~100 lcb_comm-receive-interface methods but only 2 do anything
 * (getLcbECBrpmHandler / getOneShotLcbECBrpmHandler) — those are the only two
 * wired here; the rest are intentionally not bridged, matching the Java no-ops.
 */

#include "jht_ad_board_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "JhtADboardHandlerLcbCb"

/* 0x63 GetECBrpm (auto-poll response) */
static void jab_on_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    jht_ad_board_handler_t *h = (jht_ad_board_handler_t *)ud;
    if (h->user_cb.on_jht_ad_board_rpm)
        h->user_cb.on_jht_ad_board_rpm(h->user_cb.user_data, rpm);
}

/* One-shot GetECBrpm */
static void jab_on_one_shot_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    jht_ad_board_handler_t *h = (jht_ad_board_handler_t *)ud;
    if (h->user_cb.on_jht_ad_board_one_shot_rpm)
        h->user_cb.on_jht_ad_board_one_shot_rpm(h->user_cb.user_data, rpm);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void jab_lcb_cb_register(jht_ad_board_handler_t *h)
{
    lcb_comm_callbacks_init(&h->lcb_cb);
    h->lcb_cb.user_data = h;

    h->lcb_cb.on_lcb_ecb_rpm          = jab_on_lcb_ecb_rpm;
    h->lcb_cb.on_one_shot_lcb_ecb_rpm = jab_on_one_shot_lcb_ecb_rpm;
}

void jab_lcb_cb_unregister(jht_ad_board_handler_t *h)
{
    memset(&h->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
