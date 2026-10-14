/**
 * @file treadmill_interface.c
 * @brief Treadmill safety/orchestration adapter — Layer_2 "adapters"
 *
 * Converted from (narrowed scope):
 *   com/jht/chimera/service/hal/lib/lcb/lcbcommlib/lcbInterface/TreadMillInterface.java
 */

#include "treadmill_interface_internal.h"
#include "lcb_constants.h"
#include "lcb_proc_comm.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>

#define TAG "TreadmillInterface"

/* ============================================================================
 * Ext-Status Parsing / Safety-Key / Auto-OC (Java: onExtStatusParser,
 * onExtStatusHnadler, onAutoOCExtStatusHnadler)
 * ============================================================================ */

static void ti_ext_status_parse(treadmill_interface_t *ti, uint16_t status)
{
    /* For treadmill byte define (status2)
     * Bit 0 : MCB safety trigger
     * Bit 1 : MCB lock state
     * Bit 2 : MCB zero RPM
     * Bit 3 : MCB auto OC protection */
    uint8_t low = (uint8_t)(status & 0xFF);

    ti->safetykey_down  = (low & 0x01) != 0;
    ti->safetykey_lock  = (low & 0x02) != 0;
    ti->zero_rpm        = (low & 0x04) == 0;
    ti->auto_oc_detect  = (low & 0x08) != 0;
}

static void ti_ext_status_handle(treadmill_interface_t *ti)
{
    if (ti->safetykey_down) {
        if (ti->user_cb.on_safetykey_lock_state)
            ti->user_cb.on_safetykey_lock_state(ti->user_cb.user_data, true);
    } else if (ti->safetykey_lock) {
        /* Lock-only (quick safetykey down-then-up): report locked and inject
         * the 0x02B1 unlock notification (same mechanism general_handler
         * uses for its own E-Stop-revert flow — see
         * treadmill_handler_send_estop_unlock()'s doc comment), matching
         * Java's onTreadmillStartUnlock() flow. */
        if (ti->user_cb.on_safetykey_lock_state)
            ti->user_cb.on_safetykey_lock_state(ti->user_cb.user_data, true);
        treadmill_handler_send_estop_unlock(ti->handler);
        if (ti->user_cb.on_start_unlock)
            ti->user_cb.on_start_unlock(ti->user_cb.user_data);
    } else {
        if (ti->user_cb.on_safetykey_lock_state)
            ti->user_cb.on_safetykey_lock_state(ti->user_cb.user_data, false);
    }
}

static void ti_auto_oc_ext_status_handle(treadmill_interface_t *ti)
{
    if (ti->old_oc_detect_status != ti->auto_oc_detect) {
        ti->old_oc_detect_status = ti->auto_oc_detect;
        if (ti->user_cb.on_auto_oc_state)
            ti->user_cb.on_auto_oc_state(ti->user_cb.user_data, ti->auto_oc_detect);
    }
}

void ti_proc_ext_status(treadmill_interface_t *ti, uint16_t status)
{
    if (ti == NULL) return;

    if (ti->old_status != (int)status) {
        ti_ext_status_parse(ti, status);
        ti_ext_status_handle(ti);
        ti_auto_oc_ext_status_handle(ti);
        ti->old_status = (int)status;
    }

    /* "For unlock MCB success but sometimes get 02B1 safety lock — update LCB
     * safetykey state every 400ms*15" (counts callback invocations). */
    if (++ti->update_safety_st_count == TI_SAFETY_REANNOUNCE_INTERVAL) {
        ti_ext_status_handle(ti);
        ti->update_safety_st_count = 0;
    }
}

static bool ti_check_safety_state(treadmill_interface_t *ti)
{
    return ti->safetykey_down;
}

/* ============================================================================
 * LCB Error-Status Loop (Java GeneralHandler: checkInGetErrorState,
 * runGetErrorProcesses, handleExceptionErrorCode — see the internal header)
 * ============================================================================ */

static bool ti_check_in_get_error_state(treadmill_interface_t *ti)
{
    switch (ti->error_state) {
        case TI_ERROR_GET:
            if (++ti->check_get_error_counter >= TI_CHECK_GET_ERROR_STATE_TIMES) {
                ti->check_get_error_counter = 0;
                ti->error_state = TI_ERROR_NONE;
            }
            return true;
        case TI_ERROR_SKIP:
            if (++ti->check_skip_error_counter >= TI_CHECK_SKIP_ERROR_STATE_TIMES) {
                ti->check_skip_error_counter = 0;
                ti->error_state = TI_ERROR_NONE;
            }
            return true;
        case TI_ERROR_NONE:
        default:
            ti->check_get_error_counter = 0;
            ti->check_skip_error_counter = 0;
            return false;
    }
}

static void ti_run_get_error_processes(treadmill_interface_t *ti)
{
    if (ti->comm == NULL) return;
    switch (ti->error_state) {
        case TI_ERROR_GET:
            /* A fresh request: anything 0x72 returns from here on is current. */
            ti->ignore_stale_estop = false;
            lcb_comm_get_lcb_error_code_high_priority(ti->comm);
            break;
        case TI_ERROR_SKIP:
            lcb_comm_set_lcb_skip_curr_error_high_priority(ti->comm);
            break;
        default:
            break;
    }
}

void ti_proc_lcb_error_status(treadmill_interface_t *ti, bool has_error)
{
    if (ti == NULL) return;
    pthread_mutex_lock(&ti->mutex);
    if (has_error && !ti_check_in_get_error_state(ti)) {
        LCB_LOGD(TAG, "B6 LCB error detected, requesting error code");
        ti->error_state = TI_ERROR_GET;
        ti_run_get_error_processes(ti);
    }
    pthread_mutex_unlock(&ti->mutex);
}

bool ti_proc_error_code(treadmill_interface_t *ti, uint16_t error_code)
{
    if (ti == NULL) return false;
    pthread_mutex_lock(&ti->mutex);

    if (error_code == LCB_ERROR_E_STOP && ti->ignore_stale_estop) {
        /* 0x72 answered before the E-Stop revert reached the LCB */
        LCB_LOGD(TAG, "Stale 0x02B2 after E-Stop revert ignored");
        pthread_mutex_unlock(&ti->mutex);
        return false;
    }

    switch (error_code) {
        case LCB_ERROR_COMM_LCB: /* 0x04B0: log only (Java) */
            LCB_LOGD(TAG, "Error 04B0 occurred (comm timeout)");
            pthread_mutex_unlock(&ti->mutex);
            return true;
        case LCB_ERROR_E_STOP: /* 0x02B2 */
            if (!ti->run_proc_estop) {
                ti->run_proc_estop = true;
                LCB_LOGD(TAG, "02B2 E-Stop highest priority stop cmd");
                treadmill_handler_set_rpm_high_priority(ti->handler, 0);
                treadmill_handler_set_incline_action_high_priority(ti->handler, 0x00);
            }
            ti->lcb_estop_error = true;
            break;
        case LCB_ERROR_E_STOP_UNLOCK: /* 0x02B1 */
            ti->run_proc_estop = false;
            ti->lcb_estop_error = false;
            break;
        case LCB_ERROR_E_STOP_MCB: /* 0x024F */
            ti->lcb_estop_error = false;
            break;
        default:
            break;
    }

    if (ti->error_state == TI_ERROR_GET) {
        ti->error_state = TI_ERROR_SKIP;
        ti_run_get_error_processes(ti);
    }
    pthread_mutex_unlock(&ti->mutex);
    return true;
}

/* ============================================================================
 * Internal Bridge Callbacks — wired onto the owned treadmill_handler_t
 * ============================================================================ */

static void ti_on_treadmill_rpm(void *ud, uint16_t rpm)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    if (ti->user_cb.on_rpm)
        ti->user_cb.on_rpm(ti->user_cb.user_data, rpm);
}

static void ti_on_treadmill_one_shot_rpm(void *ud, uint16_t rpm)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    if (ti->user_cb.on_one_shot_rpm)
        ti->user_cb.on_one_shot_rpm(ti->user_cb.user_data, rpm);
}

static void ti_on_treadmill_incline_loc(void *ud, uint16_t loc)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    ti->current_incline = loc;
    if (ti->user_cb.on_incline_adc)
        ti->user_cb.on_incline_adc(ti->user_cb.user_data, loc);
}

static void ti_on_treadmill_in_used(void *ud, uint8_t status)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    /* 0xFF: used, 0x55: non-used */
    if (ti->user_cb.on_person_in_place)
        ti->user_cb.on_person_in_place(ti->user_cb.user_data, status == (uint8_t)0xFF);
    treadmill_handler_set_auto_reply_speed_up_in_used(ti->handler, status == (uint8_t)0x55);
}

static void ti_on_treadmill_ext_status(void *ud, uint16_t status)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    ti_proc_ext_status(ti, status);
    if (ti->user_cb.on_ext_status)
        ti->user_cb.on_ext_status(ti->user_cb.user_data, status);
}

static void ti_on_treadmill_lcb_status(void *ud, uint8_t lcb_status)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    ti_proc_lcb_error_status(ti, (lcb_status & LCB_STATUS_BIT_LCB_ERROR) != 0);
    if (ti->user_cb.on_lcb_status)
        ti->user_cb.on_lcb_status(ti->user_cb.user_data, lcb_status);
}

static void ti_on_treadmill_motor_info(void *ud, const uint8_t *info, uint8_t length)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    if (ti->user_cb.on_motor_info)
        ti->user_cb.on_motor_info(ti->user_cb.user_data, info, length);
}

static void ti_on_treadmill_lcb_set_rpm_value(void *ud, uint16_t rpm)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    if (ti->user_cb.on_lcb_set_rpm_value)
        ti->user_cb.on_lcb_set_rpm_value(ti->user_cb.user_data, rpm);
}

static void ti_on_treadmill_lcb_set_adc_value(void *ud, uint16_t adc)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    if (ti->user_cb.on_lcb_set_adc_value)
        ti->user_cb.on_lcb_set_adc_value(ti->user_cb.user_data, adc);
}

static void ti_on_treadmill_error_code(void *ud, uint16_t error_code)
{
    treadmill_interface_t *ti = (treadmill_interface_t *)ud;
    if (!ti_proc_error_code(ti, error_code)) return;

    /* Java LCBInterface.mGeneralHandlerCallback.onErrorCode: the E-Stop codes
     * become lock / unlock messages and are not passed up as error codes. */
    if (error_code == LCB_ERROR_E_STOP || error_code == LCB_ERROR_E_STOP_UNLOCK) {
        if (ti->user_cb.on_safetykey_locked_code)
            ti->user_cb.on_safetykey_locked_code(ti->user_cb.user_data, error_code == LCB_ERROR_E_STOP);
        return;
    }
    if (ti->user_cb.on_error_code)
        ti->user_cb.on_error_code(ti->user_cb.user_data, error_code);
}

void ti_build_handler_callbacks(treadmill_interface_t *ti, treadmill_handler_callbacks_t *cb)
{
    treadmill_handler_callbacks_init(cb);
    cb->user_data = ti;
    cb->on_treadmill_rpm = ti_on_treadmill_rpm;
    cb->on_treadmill_one_shot_rpm = ti_on_treadmill_one_shot_rpm;
    cb->on_treadmill_incline_loc = ti_on_treadmill_incline_loc;
    cb->on_treadmill_in_used = ti_on_treadmill_in_used;
    cb->on_treadmill_ext_status = ti_on_treadmill_ext_status;
    cb->on_treadmill_motor_info = ti_on_treadmill_motor_info;
    cb->on_treadmill_lcb_status = ti_on_treadmill_lcb_status;
    cb->on_treadmill_lcb_set_rpm_value = ti_on_treadmill_lcb_set_rpm_value;
    cb->on_treadmill_lcb_set_adc_value = ti_on_treadmill_lcb_set_adc_value;
    cb->on_treadmill_error_code = ti_on_treadmill_error_code;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

treadmill_interface_t* treadmill_interface_create(void)
{
    treadmill_interface_t *ti = (treadmill_interface_t*)calloc(1, sizeof(treadmill_interface_t));
    if (ti == NULL) return NULL;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&ti->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    ti->machine_lcb_type = LCB_TYPE_UNKNOWN;

    return ti;
}

void treadmill_interface_destroy(treadmill_interface_t *ti)
{
    if (ti == NULL) return;
    treadmill_interface_stop(ti);
    pthread_mutex_destroy(&ti->mutex);
    free(ti);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void treadmill_interface_callbacks_init(treadmill_interface_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(treadmill_interface_callbacks_t));
}

void treadmill_interface_set_callbacks(treadmill_interface_t *ti,
                                       const treadmill_interface_callbacks_t *cb)
{
    if (ti == NULL || cb == NULL) return;
    pthread_mutex_lock(&ti->mutex);
    ti->user_cb = *cb;
    pthread_mutex_unlock(&ti->mutex);
}

void treadmill_interface_unregister_callbacks(treadmill_interface_t *ti)
{
    if (ti == NULL) return;
    pthread_mutex_lock(&ti->mutex);
    memset(&ti->user_cb, 0, sizeof(treadmill_interface_callbacks_t));
    pthread_mutex_unlock(&ti->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool treadmill_interface_start(treadmill_interface_t *ti, lcb_comm_t *comm)
{
    if (ti == NULL || comm == NULL) return false;

    treadmill_interface_stop(ti);

    /* Set before the handler starts: status bytes may arrive right away. */
    pthread_mutex_lock(&ti->mutex);
    ti->comm = comm;
    ti->error_state = TI_ERROR_NONE;
    ti->check_get_error_counter = 0;
    ti->check_skip_error_counter = 0;
    ti->run_proc_estop = false;
    ti->lcb_estop_error = false;
    ti->ignore_stale_estop = false;
    pthread_mutex_unlock(&ti->mutex);

    ti->handler = treadmill_handler_create();
    if (ti->handler == NULL) {
        LCB_LOGE(TAG, "Failed to create internal treadmill_handler");
        ti->comm = NULL;
        return false;
    }

    treadmill_handler_set_lcb_type(ti->handler, ti->machine_lcb_type);

    treadmill_handler_callbacks_t handler_cb;
    ti_build_handler_callbacks(ti, &handler_cb);
    treadmill_handler_set_callbacks(ti->handler, &handler_cb);

    if (!treadmill_handler_start(ti->handler, comm)) {
        LCB_LOGE(TAG, "Failed to start internal treadmill_handler");
        treadmill_handler_destroy(ti->handler);
        ti->handler = NULL;
        ti->comm = NULL;
        return false;
    }

    /* Java: TreadMillInterface constructor unconditionally turns on ext-status
     * auto-reply — this is what drives safety-key/auto-OC state discovery and
     * the automatic unlock recovery. */
    treadmill_handler_set_auto_reply_ext_status(ti->handler, true);

    LCB_LOGD(TAG, "Treadmill interface started");
    return true;
}

void treadmill_interface_stop(treadmill_interface_t *ti)
{
    if (ti == NULL) return;
    if (ti->handler != NULL) {
        treadmill_handler_stop(ti->handler);
        treadmill_handler_destroy(ti->handler);
        ti->handler = NULL;
    }
    ti->comm = NULL;
    LCB_LOGD(TAG, "Treadmill interface stopped");
}

/* ============================================================================
 * Public: LCB Type Injection
 * ============================================================================ */

void treadmill_interface_set_lcb_type(treadmill_interface_t *ti, LcbType_t type)
{
    if (ti == NULL) return;
    ti->machine_lcb_type = type;
    if (ti->handler != NULL)
        treadmill_handler_set_lcb_type(ti->handler, type);
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

void treadmill_interface_set_rpm(treadmill_interface_t *ti, uint16_t rpm)
{
    if (ti == NULL || ti->handler == NULL) return;
    if (ti_check_safety_state(ti)) return;
    treadmill_handler_set_rpm(ti->handler, rpm);
}

void treadmill_interface_set_rpm_high_priority(treadmill_interface_t *ti, uint16_t rpm)
{
    if (ti == NULL || ti->handler == NULL) return;
    if (ti_check_safety_state(ti)) return;
    treadmill_handler_set_rpm_high_priority(ti->handler, rpm);
}

void treadmill_interface_set_incline(treadmill_interface_t *ti, uint16_t loc)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_incline(ti->handler, loc);
}

void treadmill_interface_set_incline_high_priority(treadmill_interface_t *ti, uint16_t loc)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_incline_high_priority(ti->handler, loc);
}

void treadmill_interface_set_incline_action(treadmill_interface_t *ti, uint8_t action)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_incline_action(ti->handler, action);
}

void treadmill_interface_set_incline_action_high_priority(treadmill_interface_t *ti, uint8_t action)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_incline_action_high_priority(ti->handler, action);
}

void treadmill_interface_set_motor_type(treadmill_interface_t *ti, uint16_t watts)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_motor_type(ti->handler, watts);
}

void treadmill_interface_apply_frame_config(treadmill_interface_t *ti, const frame_config_data_t *fc)
{
    if (ti == NULL || fc == NULL) return;

    /* Java: lcbInterface.setTreadmillRPM(0); */
    treadmill_interface_set_rpm(ti, 0);

    /* Java: setTmInclineActionStop() -> mRealLcb.setLcbInclineActionHighestPriority((byte) 0);
     * "stop incline may be highest priority" */
    treadmill_interface_set_incline_action_high_priority(ti, 0);

    /* Java: setTreadMillIncline(frameConfig.INCLINE_ADC_MIN), with the
     * reverse-incline direction folded into bit 15 of the raw ADC value
     * (see lcb_comm_set_lcb_incline_loc's doc comment — there is no separate
     * "set direction" command). */
    if (fc->has_incline_adc_range) {
        uint16_t adc = (uint16_t)fc->incline_adc_range.min_value;
        if (fc->uses_reverse_incline) adc |= 0x8000;
        treadmill_interface_set_incline(ti, adc);
    }
}

bool treadmill_interface_is_safetykey_down(const treadmill_interface_t *ti)
{
    return ti != NULL && ti->safetykey_down;
}

bool treadmill_interface_is_safetykey_locked(const treadmill_interface_t *ti)
{
    return ti != NULL && ti->safetykey_lock;
}

bool treadmill_interface_is_lcb_estop_error(const treadmill_interface_t *ti)
{
    return ti != NULL && ti->lcb_estop_error;
}

void treadmill_interface_set_ecb_action_high_priority(treadmill_interface_t *ti, uint8_t value)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_ecb_action_high_priority(ti->handler, value);
}

/* ============================================================================
 * Public: One-shot Getters
 * ============================================================================ */

void treadmill_interface_get_one_shot_rpm(treadmill_interface_t *ti)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_get_one_shot_rpm(ti->handler);
}

void treadmill_interface_get_one_shot_incline_adc(treadmill_interface_t *ti)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_get_one_shot_incline_adc(ti->handler);
}

/* ============================================================================
 * Public: Auto-reply Toggles
 * ============================================================================ */

void treadmill_interface_set_auto_reply_rpm(treadmill_interface_t *ti, bool enable)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_auto_reply_rpm(ti->handler, enable);
}

void treadmill_interface_set_auto_reply_incline(treadmill_interface_t *ti, bool enable)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_auto_reply_incline(ti->handler, enable);
}

void treadmill_interface_set_auto_reply_in_used(treadmill_interface_t *ti, bool enable)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_auto_reply_in_used(ti->handler, enable);
}

void treadmill_interface_set_auto_reply_speed_up_in_used(treadmill_interface_t *ti, bool enable)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_auto_reply_speed_up_in_used(ti->handler, enable);
}

void treadmill_interface_set_auto_reply_motor_info(treadmill_interface_t *ti, bool enable)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_auto_reply_motor_info(ti->handler, enable);
}

void treadmill_interface_set_auto_reply_ext_status(treadmill_interface_t *ti, bool enable)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_auto_reply_ext_status(ti->handler, enable);
}

/* ============================================================================
 * Public: RPM Ramp Control
 * ============================================================================ */

void treadmill_interface_set_auto_ramp_rpm(treadmill_interface_t *ti, bool enable,
                                           int ramp_up_step, int ramp_down_step)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_set_auto_ramp_rpm(ti->handler, enable, ramp_up_step, ramp_down_step);
}

void treadmill_interface_estop_reset_gui_current_rpm(treadmill_interface_t *ti)
{
    if (ti == NULL || ti->handler == NULL) return;
    treadmill_handler_estop_reset_gui_current_rpm(ti->handler);
}

void treadmill_interface_estop_revert_lcb_initial(treadmill_interface_t *ti)
{
    if (ti == NULL || ti->handler == NULL) return;
    pthread_mutex_lock(&ti->mutex);
    /* Java eStopRevertLcbInitial(): runProcEStopLcb = false, restart the error loop.
     * A 0x72 already in flight may still answer 0x02B2 from before the revert —
     * drop that until a fresh request goes out. */
    ti->run_proc_estop = false;
    ti->error_state = TI_ERROR_NONE;
    ti->check_get_error_counter = 0;
    ti->check_skip_error_counter = 0;
    ti->ignore_stale_estop = true;
    pthread_mutex_unlock(&ti->mutex);
    /* Re-init sequence + injected 0x02B1, which clears lcb_estop_error */
    treadmill_handler_estop_revert_lcb_initial(ti->handler);
}
