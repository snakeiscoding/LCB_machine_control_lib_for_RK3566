/**
 * @file stepper_lcb_cb.c
 * @brief LCB Callback Bridge for Stepper Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/Stepper/StepperCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives stepper_handler_t* via user_data and dispatches to
 * the user-facing callbacks. Java's StepperCallbackHandlerLCB overrides
 * ~130 lcb_comm-receive-interface methods but only 2 do anything
 * (getLcbECBrpmHandler / getOneShotLcbECBrpmHandler) — those are the only
 * two wired here; the rest are intentionally not bridged, matching the Java
 * no-ops.
 */

#include "stepper_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "StepperHandlerLcbCb"

/* 0x63 GetECBrpm (auto-poll response) */
static void sh_on_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    stepper_handler_t *sh = (stepper_handler_t *)ud;
    if (sh->user_cb.on_stepper_rpm)
        sh->user_cb.on_stepper_rpm(sh->user_cb.user_data, rpm);
}

/* One-shot GetECBrpm */
static void sh_on_one_shot_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    stepper_handler_t *sh = (stepper_handler_t *)ud;
    if (sh->user_cb.on_stepper_one_shot_rpm)
        sh->user_cb.on_stepper_one_shot_rpm(sh->user_cb.user_data, rpm);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void sh_lcb_cb_register(stepper_handler_t *sh)
{
    lcb_comm_callbacks_init(&sh->lcb_cb);
    sh->lcb_cb.user_data = sh;

    sh->lcb_cb.on_lcb_ecb_rpm          = sh_on_lcb_ecb_rpm;
    sh->lcb_cb.on_one_shot_lcb_ecb_rpm = sh_on_one_shot_lcb_ecb_rpm;
}

void sh_lcb_cb_unregister(stepper_handler_t *sh)
{
    memset(&sh->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
