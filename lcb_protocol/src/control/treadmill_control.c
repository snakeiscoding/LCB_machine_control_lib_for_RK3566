/**
 * @file treadmill_control.c
 * @brief Treadmill machine control — Layer_3 "control"
 *
 * See treadmill_control.h for the chimera2 sources this was converted from.
 */

#include "treadmill_control_internal.h"
#include "treadmill_machine_convert.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>

#define TAG "TreadmillControl"

#define NOTIFY(tc, fn, ...) \
    do { if ((tc)->user_cb.fn) (tc)->user_cb.fn((tc)->user_cb.user_data, __VA_ARGS__); } while (0)

/* ============================================================================
 * Helpers
 * ============================================================================ */

static void tc_set_state(treadmill_control_t *tc, tm_run_state_t state)
{
    if (tc->state == state) return;
    tc->state = state;
    NOTIFY(tc, on_state_changed, state);
}

static bool tc_in_workout(const treadmill_control_t *tc)
{
    return tc->state == TM_RUN_STATE_RUNNING || tc->state == TM_RUN_STATE_PAUSED;
}

static bool tc_compute_safe(const treadmill_control_t *tc)
{
    if (tc->state == TM_RUN_STATE_ESTOP) return false;
    if (tc->lock_state != 0) return false;
    if (tc->main_motor_on) return false;
    if (tc->incline_moving) return false;
    if (treadmill_interface_is_safetykey_down(tc->ti)) return false;
    if (tc->encoder_check_enabled && tc->encoder_rpm_status) return false;
    if (tc->error_active) return false;
    return true;
}

/* Recompute safe-to-start; notify only on change. */
static void tc_update_safe(treadmill_control_t *tc)
{
    tc->safe_to_start = tc_compute_safe(tc);
    if (tc->safe_to_start != tc->safe_reported) {
        tc->safe_reported = tc->safe_to_start;
        NOTIFY(tc, on_safe_to_start, tc->safe_to_start);
    }
}

static void tc_apply_speed(treadmill_control_t *tc)
{
    tm_adapter_set_speed_kph(&tc->adapter, tc->target_kph);
}

static void tc_apply_incline(treadmill_control_t *tc)
{
    tm_adapter_set_incline_percent(&tc->adapter, tc->target_incline);
}

static float tc_min_speed(const treadmill_control_t *tc)
{
    return tc->fc.has_speed_kph_range ? tc->fc.speed_kph_range.min_value : 0.0f;
}

static void tc_emit_workout_data(treadmill_control_t *tc)
{
    if (tc->user_cb.on_workout_data == NULL) return;
    tm_workout_data_t d;
    treadmill_control_get_workout_data(tc, &d);
    tc->user_cb.on_workout_data(tc->user_cb.user_data, &d);
}

static void tc_enter_estop(treadmill_control_t *tc, tm_estop_source_t source)
{
    if (tc->state == TM_RUN_STATE_ESTOP) return;   /* keep the first trigger's source */
    LCB_LOGD(TAG, "E-Stop triggered (source %d)", (int)source);
    tc->target_kph = 0.0f;
    tc->cal_phase = TM_CAL_IDLE;
    tc->estop_source = source;
    tm_adapter_estop_stop(&tc->adapter);
    tc_set_state(tc, TM_RUN_STATE_ESTOP);
    NOTIFY(tc, on_estop, true, source);
    tc_update_safe(tc);
}

/* Java LCBService.onTmSafetykeyReset(): hide the E-Stop (release message) and
 * reset the treadmill (rpm 0, incline to ADC min). */
static void tc_estop_reset(treadmill_control_t *tc)
{
    tc->release_wait_ms = 0;
    tm_adapter_estop_reset(&tc->adapter);
    if (tc->state == TM_RUN_STATE_ESTOP) {
        tm_estop_source_t source = tc->estop_source;
        tc->estop_source = TM_ESTOP_SRC_NONE;
        tc->target_kph = 0.0f;
        tc->target_incline = 0.0f;
        tc_set_state(tc, TM_RUN_STATE_STOPPED);
        LCB_LOGD(TAG, "E-Stop released (source %d)", (int)source);
        NOTIFY(tc, on_estop, false, source);
    }
    tc_update_safe(tc);
}

static tm_estop_source_t tc_lock_source(uint8_t st)
{
    return st == TM_LOCK_MCB_CODE ? TM_ESTOP_SRC_LCB_ERROR : TM_ESTOP_SRC_SAFETY_KEY;
}

/* Java LCBService.onTmSafetykeyStateUpdate(trigger, st). */
static void tc_safetykey_state_update(treadmill_control_t *tc, bool trigger, uint8_t st)
{
    /* Normal state: nothing to release */
    if (!trigger && tc->lock_state == 0) return;

    if (trigger) {
        tc->lock_state |= st;
        tc_enter_estop(tc, tc_lock_source(st));      /* trigger message (once per E-Stop) */
        tc->release_wait_ms = 0;                     /* stop release timer */
        /* Safety switch is up but the MCB reports 0x02B2: unlock (re-init). If
         * the key is still out the LCB answers 0x02B2 again. */
        if (tc->lock_state == TM_LOCK_MCB_CODE) {
            LCB_LOGD(TAG, "MCB Safety: get 0x02B2, re-unlock");
            tm_adapter_estop_unlock(&tc->adapter);
        }
    } else {
        tc->lock_state &= (uint8_t)~st;
        tm_adapter_estop_unlock(&tc->adapter);
        /* State lock: quick stop, release after 2 s; otherwise wait 6 s to
         * make sure no new 0x02B2 arrives before releasing. */
        tc->release_wait_ms = (st == TM_LOCK_MCB_STATE) ? TM_SAFETYKEY_RELEASE_DELAY_MS
                                                         : TM_SAFETYKEY_UNLOCK_CHECK_MS;
    }
    LCB_LOGD(TAG, "MCB Safety: E-STOP %d, event 0x%02X, state 0x%02X",
             trigger ? 1 : 0, (unsigned)st, (unsigned)tc->lock_state);
    tc_update_safe(tc);
}

/* ============================================================================
 * Interface bridge callbacks (treadmill_interface -> control)
 * ============================================================================ */

void tc_on_rpm(void *ud, uint16_t rpm)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->actual_rpm = rpm;
    tc->actual_kph = tm_rpm_to_kph(&tc->fc, rpm);
    NOTIFY(tc, on_rpm, rpm);
    NOTIFY(tc, on_speed_kph, tc->actual_kph);
    pthread_mutex_unlock(&tc->mutex);
}

void tc_on_incline_adc(void *ud, uint16_t adc)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->actual_adc = adc;
    tc->actual_incline = tm_adc_to_incline(&tc->fc, adc);
    NOTIFY(tc, on_incline_percent, tc->actual_incline);
    pthread_mutex_unlock(&tc->mutex);
}

void tc_on_person_in_place(void *ud, bool in_place)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->person_in_place = in_place;
    NOTIFY(tc, on_person_in_place, in_place);
    pthread_mutex_unlock(&tc->mutex);
}

/* 0x97 ext status lock (Java onTreadmillSafetykeyLockState -> MCB_STATE_LOCK) */
void tc_on_safetykey_lock_state(void *ud, bool locked)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc_safetykey_state_update(tc, locked, TM_LOCK_MCB_STATE);
    pthread_mutex_unlock(&tc->mutex);
}

/* 0x02B2 / 0x02B1 (Java onLCBSafetykeyLockedCode -> MCB_CODE_LOCK) */
void tc_on_safetykey_locked_code(void *ud, bool locked)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc_safetykey_state_update(tc, locked, TM_LOCK_MCB_CODE);
    pthread_mutex_unlock(&tc->mutex);
}

/* Java onTreadmillStartUnlock -> release MCB_STATE_CIRCUIT_LOCK */
void tc_on_start_unlock(void *ud)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc_safetykey_state_update(tc, false, TM_LOCK_MCB_CIRCUIT);
    pthread_mutex_unlock(&tc->mutex);
}

void tc_on_auto_oc_state(void *ud, bool detected)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->oc_active = detected;
    NOTIFY(tc, on_over_current, detected);
    pthread_mutex_unlock(&tc->mutex);
}

void tc_on_error_code(void *ud, uint16_t code)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL) return;
    if (code == TM_ERR_ESTOP || code == TM_ERR_ESTOP_UNLOCK) {
        /* E-Stop arrives as on_safetykey_locked_code; its codes are never
         * reported upward as errors. */
        return;
    }
    /* Reported only; the UI decides (by severity) whether it blocks Go and calls
     * treadmill_control_set_error_block(). */
    pthread_mutex_lock(&tc->mutex);
    NOTIFY(tc, on_error, code);
    pthread_mutex_unlock(&tc->mutex);
}

/* Chimera stores the DC-bus current only while in a workout. */
void tc_on_motor_info(void *ud, const uint8_t *info, uint8_t length)
{
    treadmill_control_t *tc = (treadmill_control_t *)ud;
    if (tc == NULL || info == NULL || length <= TM_MOTOR_INFO_DC_BUS_HIGH_IDX) return;
    pthread_mutex_lock(&tc->mutex);
    if (tc->fc.has_current_oc && tc->oc_detect_enabled && tc_in_workout(tc)) {
        tc->dc_bus_current = (uint16_t)(info[TM_MOTOR_INFO_DC_BUS_LOW_IDX] |
                                        (info[TM_MOTOR_INFO_DC_BUS_HIGH_IDX] << 8));
        NOTIFY(tc, on_dc_bus_current, tc->dc_bus_current);
    }
    pthread_mutex_unlock(&tc->mutex);
}

void tc_on_lcb_status(void *ud, uint8_t status)
{
    treadmill_control_on_lcb_status((treadmill_control_t *)ud, status);
}

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

treadmill_control_t *treadmill_control_create_with_interface(const frame_config_data_t *fc,
                                                             treadmill_interface_t *ti)
{
    if (fc == NULL) return NULL;
    treadmill_control_t *tc = (treadmill_control_t *)calloc(1, sizeof(*tc));
    if (tc == NULL) return NULL;

    tc->fc = *fc;
    tc->owns_ti = (ti == NULL);
    tc->ti = tc->owns_ti ? treadmill_interface_create() : ti;
    if (tc->ti == NULL) {
        free(tc);
        return NULL;
    }

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&tc->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    treadmill_interface_callbacks_t icb;
    treadmill_interface_callbacks_init(&icb);
    icb.user_data = tc;
    icb.on_rpm = tc_on_rpm;
    icb.on_incline_adc = tc_on_incline_adc;
    icb.on_person_in_place = tc_on_person_in_place;
    icb.on_safetykey_lock_state = tc_on_safetykey_lock_state;
    icb.on_safetykey_locked_code = tc_on_safetykey_locked_code;
    icb.on_start_unlock = tc_on_start_unlock;
    icb.on_auto_oc_state = tc_on_auto_oc_state;
    icb.on_error_code = tc_on_error_code;
    icb.on_motor_info = tc_on_motor_info;
    icb.on_lcb_status = tc_on_lcb_status;
    treadmill_interface_set_callbacks(tc->ti, &icb);

    tm_adapter_init(&tc->adapter, tc->ti, NULL, &tc->fc);

    tc->state = TM_RUN_STATE_IDLE;
    tc->motor_type = (uint16_t)(tc->fc.motor_type > 0 ? tc->fc.motor_type : 0);
    tc->weight_kg = tc->fc.has_weight_kg_range ? (float)tc->fc.weight_kg_range.default_value : 68.0f;
    tc->cal_last_adc = -1;
    tc->safe_to_start = true;
    tc->safe_reported = true;
    return tc;
}

treadmill_control_t *treadmill_control_create(const frame_config_data_t *fc)
{
    return treadmill_control_create_with_interface(fc, NULL);
}

void treadmill_control_destroy(treadmill_control_t *tc)
{
    if (tc == NULL) return;
    treadmill_control_stop(tc);
    if (tc->owns_ti) treadmill_interface_destroy(tc->ti);
    pthread_mutex_destroy(&tc->mutex);
    free(tc);
}

void treadmill_control_callbacks_init(treadmill_control_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(*cb));
}

void treadmill_control_set_callbacks(treadmill_control_t *tc, const treadmill_control_callbacks_t *cb)
{
    if (tc == NULL || cb == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->user_cb = *cb;
    pthread_mutex_unlock(&tc->mutex);
}

void treadmill_control_unregister_callbacks(treadmill_control_t *tc)
{
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    memset(&tc->user_cb, 0, sizeof(tc->user_cb));
    pthread_mutex_unlock(&tc->mutex);
}

bool treadmill_control_start(treadmill_control_t *tc, lcb_comm_t *comm)
{
    if (tc == NULL || comm == NULL) return false;
    if (!treadmill_interface_start(tc->ti, comm)) return false;

    pthread_mutex_lock(&tc->mutex);
    tc->comm = comm;
    tc->adapter.comm = comm;
    treadmill_interface_apply_frame_config(tc->ti, &tc->fc);   /* rpm 0, incline min */
    tm_adapter_apply_motor_type_from_config(&tc->adapter);
    treadmill_interface_set_auto_reply_rpm(tc->ti, true);
    treadmill_interface_set_auto_reply_incline(tc->ti, true);
    tc_set_state(tc, TM_RUN_STATE_IDLE);
    pthread_mutex_unlock(&tc->mutex);
    return true;
}

void treadmill_control_stop(treadmill_control_t *tc)
{
    if (tc == NULL) return;
    treadmill_interface_stop(tc->ti);
    tc->comm = NULL;
    tc->adapter.comm = NULL;
}

void treadmill_control_set_lcb_type(treadmill_control_t *tc, LcbType_t type)
{
    if (tc == NULL) return;
    treadmill_interface_set_lcb_type(tc->ti, type);
}

/* ============================================================================
 * Calibration (tick driven)
 * ============================================================================ */

static void tc_cal_finish(treadmill_control_t *tc, bool success)
{
    tm_adapter_incline_action(&tc->adapter, TM_INCLINE_ACTION_STOP);
    tc->cal_phase = TM_CAL_IDLE;
    tc->cal_last_adc = -1;

    if (success) {
        int off_min = tc->fc.has_incline_adc_offset_min ? tc->fc.incline_adc_offset_min : 0;
        int off_max = tc->fc.has_incline_adc_offset_max ? tc->fc.incline_adc_offset_max : 0;
        tc->cal_result_min = (uint16_t)(tc->cal_low_adc + off_min);
        tc->cal_result_max = (uint16_t)(tc->cal_high_adc - off_max);
        tc->fc.has_incline_adc_range = true;
        tc->fc.incline_adc_range.min_value = tc->cal_result_min;
        tc->fc.incline_adc_range.max_value = tc->cal_result_max;
        tc->fc.incline_adc_range.default_value = tc->cal_result_min;
        tm_adapter_set_incline_adc(&tc->adapter, tc->cal_result_min);   /* back to ADC_MIN */
    }
    NOTIFY(tc, on_calibration_result, success, tc->cal_result_min, tc->cal_result_max);
}

static void tc_cal_tick(treadmill_control_t *tc, uint32_t now_ms)
{
    if (tc->cal_phase == TM_CAL_IDLE) return;

    if (now_ms - tc->cal_start_ms >= TM_CAL_TIMEOUT_MS) {
        LCB_LOGE(TAG, "calibration timeout");
        tc_cal_finish(tc, false);
        return;
    }
    if ((int32_t)(now_ms - tc->cal_next_poll_ms) < 0) return;
    tc->cal_next_poll_ms = now_ms + TM_CAL_POLL_INTERVAL_MS;

    int adc = (int)(tc->actual_adc & ~TM_REVERSE_BIT);
    bool stable = tc->cal_last_adc >= 0 && abs(adc - tc->cal_last_adc) < TM_CAL_STABLE_ADC_DELTA;
    tc->cal_last_adc = adc;
    if (!stable) return;

    if (tc->cal_phase == TM_CAL_INCLINE_UP) {
        tc->cal_high_adc = adc;
        tm_adapter_incline_action(&tc->adapter, TM_INCLINE_ACTION_STOP);
        tm_adapter_incline_action(&tc->adapter, TM_INCLINE_ACTION_DOWN);
        tc->cal_phase = TM_CAL_INCLINE_DOWN;
        tc->cal_last_adc = -1;
    } else {
        tc->cal_low_adc = adc;
        tc_cal_finish(tc, (tc->cal_high_adc - tc->cal_low_adc) > TM_CAL_MIN_SPAN_ADC);
    }
}

bool treadmill_control_calibration_start(treadmill_control_t *tc, uint32_t now_ms)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    bool ok = tc->fc.supports_calibration && !tc_in_workout(tc) &&
              tc->state != TM_RUN_STATE_ESTOP && tc->cal_phase == TM_CAL_IDLE;
    if (ok) {
        treadmill_interface_set_auto_reply_incline(tc->ti, true);
        tm_adapter_incline_action(&tc->adapter, TM_INCLINE_ACTION_UP);
        tc->cal_phase = TM_CAL_INCLINE_UP;
        tc->cal_start_ms = now_ms;
        tc->cal_next_poll_ms = now_ms + TM_CAL_POLL_INTERVAL_MS;
        tc->cal_last_adc = -1;
    }
    pthread_mutex_unlock(&tc->mutex);
    return ok;
}

bool treadmill_control_calibration_in_progress(const treadmill_control_t *tc)
{
    return tc != NULL && tc->cal_phase != TM_CAL_IDLE;
}

void treadmill_control_calibration_get_result(const treadmill_control_t *tc,
                                              uint16_t *adc_min, uint16_t *adc_max)
{
    if (tc == NULL) return;
    if (adc_min) *adc_min = tc->cal_result_min;
    if (adc_max) *adc_max = tc->cal_result_max;
}

/* ============================================================================
 * Tick: workout data + calibration
 * ============================================================================ */

void treadmill_control_tick(treadmill_control_t *tc, uint32_t now_ms)
{
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);

    uint32_t dt = tc->tick_primed ? (now_ms - tc->last_tick_ms) : 0;
    tc->last_tick_ms = now_ms;
    tc->tick_primed = true;

    if (tc->state == TM_RUN_STATE_RUNNING && dt > 0) {
        tc->elapsed_ms += dt;
        tc->distance_km += tc->actual_kph * (float)dt / 3600000.0f;

        /* ACSM walking/running: VO2 = 0.1*S + 1.8*S*grade + 3.5 (ml/kg/min), S in m/min */
        float s = tc->actual_kph * 1000.0f / 60.0f;
        float vo2 = 0.1f * s + 1.8f * s * (tc->actual_incline / 100.0f) + 3.5f;
        tc->calories_kcal += vo2 * tc->weight_kg / 1000.0f * 5.0f * ((float)dt / 60000.0f);
        tc_emit_workout_data(tc);
    }

    /* Safety-key release check (Java LCBDataTimerHandler.checkTmSafetykeyRelease) */
    if (tc->release_wait_ms > 0 && dt > 0) {
        if (dt >= tc->release_wait_ms) {
            tc->release_wait_ms = 0;
            if (tc->lock_state == 0) tc_estop_reset(tc);   /* release message */
        } else {
            tc->release_wait_ms -= dt;
        }
    }

    tc_cal_tick(tc, now_ms);
    pthread_mutex_unlock(&tc->mutex);
}

/* ============================================================================
 * Machine control
 * ============================================================================ */

bool treadmill_control_play(treadmill_control_t *tc)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    bool ok = (tc->state == TM_RUN_STATE_IDLE || tc->state == TM_RUN_STATE_STOPPED) &&
              tc->cal_phase == TM_CAL_IDLE && tc_compute_safe(tc);
    if (ok) {
        tc->elapsed_ms = 0;
        tc->distance_km = 0.0f;
        tc->calories_kcal = 0.0f;
        tc->tick_primed = false;
        if (tc->target_kph <= 0.0f) tc->target_kph = tc_min_speed(tc);
        tm_adapter_start_machine(&tc->adapter);
        tc_apply_speed(tc);
        tc_apply_incline(tc);
        tc_set_state(tc, TM_RUN_STATE_RUNNING);
    }
    pthread_mutex_unlock(&tc->mutex);
    return ok;
}

bool treadmill_control_pause(treadmill_control_t *tc)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    bool ok = tc->state == TM_RUN_STATE_RUNNING;
    if (ok) {
        tm_adapter_stop_machine(&tc->adapter);                            /* rpm 0 */
        tm_adapter_incline_action(&tc->adapter, TM_INCLINE_ACTION_STOP);
        tc_set_state(tc, TM_RUN_STATE_PAUSED);
    }
    pthread_mutex_unlock(&tc->mutex);
    return ok;
}

bool treadmill_control_resume(treadmill_control_t *tc)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    bool ok = tc->state == TM_RUN_STATE_PAUSED && tc_compute_safe(tc);   /* chimera: blocked if not safe */
    if (ok) {
        tm_adapter_start_machine(&tc->adapter);
        tc_apply_speed(tc);
        tc_apply_incline(tc);
        tc->tick_primed = false;
        tc_set_state(tc, TM_RUN_STATE_RUNNING);
    }
    pthread_mutex_unlock(&tc->mutex);
    return ok;
}

bool treadmill_control_stop_workout(treadmill_control_t *tc)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    bool ok = tc_in_workout(tc);
    if (ok) {
        tm_adapter_stop_machine(&tc->adapter);
        tm_adapter_incline_action(&tc->adapter, TM_INCLINE_ACTION_STOP);
        tc->target_kph = 0.0f;
        tc->target_incline = 0.0f;
        tc_set_state(tc, TM_RUN_STATE_STOPPED);
    }
    pthread_mutex_unlock(&tc->mutex);
    return ok;
}

bool treadmill_control_estop(treadmill_control_t *tc)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    bool ok = tc->state != TM_RUN_STATE_ESTOP;
    if (ok) tc_enter_estop(tc, TM_ESTOP_SRC_COMMAND);
    pthread_mutex_unlock(&tc->mutex);
    return ok;
}

bool treadmill_control_estop_release(treadmill_control_t *tc)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    /* Manual release only when no MCB lock is left — an MCB lock releases on
     * its own once the LCB stops reporting it (see tc_safetykey_state_update). */
    bool ok = tc->state == TM_RUN_STATE_ESTOP && tc->lock_state == 0 &&
              !treadmill_interface_is_safetykey_down(tc->ti);
    if (ok) {
        tm_adapter_estop_unlock(&tc->adapter);
        tc_estop_reset(tc);
    }
    pthread_mutex_unlock(&tc->mutex);
    return ok;
}

tm_estop_source_t treadmill_control_get_estop_source(const treadmill_control_t *tc)
{
    return tc ? tc->estop_source : TM_ESTOP_SRC_NONE;
}

float treadmill_control_set_speed_kph(treadmill_control_t *tc, float kph)
{
    if (tc == NULL) return 0.0f;
    pthread_mutex_lock(&tc->mutex);
    tc->target_kph = (kph <= 0.0f) ? 0.0f : tm_clamp_speed_kph(&tc->fc, kph);
    if (tc->state == TM_RUN_STATE_RUNNING) tc_apply_speed(tc);
    float applied = tc->target_kph;
    pthread_mutex_unlock(&tc->mutex);
    return applied;
}

float treadmill_control_set_incline_percent(treadmill_control_t *tc, float percent)
{
    if (tc == NULL) return 0.0f;
    pthread_mutex_lock(&tc->mutex);
    tc->target_incline = tm_clamp_incline_percent(&tc->fc, percent);
    if (tc->state == TM_RUN_STATE_RUNNING) tc_apply_incline(tc);
    float applied = tc->target_incline;
    pthread_mutex_unlock(&tc->mutex);
    return applied;
}

float treadmill_control_set_weight_kg(treadmill_control_t *tc, float kg)
{
    if (tc == NULL) return 0.0f;
    pthread_mutex_lock(&tc->mutex);
    tc->weight_kg = tm_clamp_weight_kg(&tc->fc, kg);
    float applied = tc->weight_kg;
    pthread_mutex_unlock(&tc->mutex);
    return applied;
}

float treadmill_control_get_weight_kg(const treadmill_control_t *tc)
{
    return tc ? tc->weight_kg : 0.0f;
}

tm_run_state_t treadmill_control_get_state(const treadmill_control_t *tc)
{
    return tc ? tc->state : TM_RUN_STATE_IDLE;
}

void treadmill_control_get_workout_data(const treadmill_control_t *tc, tm_workout_data_t *out)
{
    if (out == NULL) return;
    memset(out, 0, sizeof(*out));
    if (tc == NULL) return;
    out->elapsed_ms = tc->elapsed_ms;
    out->distance_km = tc->distance_km;
    out->calories_kcal = tc->calories_kcal;
    out->actual_speed_kph = tc->actual_kph;
    out->actual_incline_percent = tc->actual_incline;
    out->target_speed_kph = tc->target_kph;
    out->target_incline_percent = tc->target_incline;
    out->actual_rpm = tc->actual_rpm;
}

/* ============================================================================
 * Function control
 * ============================================================================ */

bool treadmill_control_set_over_current_detect(treadmill_control_t *tc, bool enable)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    bool applied = tc->fc.has_current_oc;
    tc->oc_detect_enabled = applied && enable;
    tm_adapter_set_current_oc_detect(&tc->adapter, tc->oc_detect_enabled);
    if (tc_in_workout(tc)) tm_adapter_set_detect_machine(&tc->adapter, true);
    pthread_mutex_unlock(&tc->mutex);
    return applied;
}

bool treadmill_control_get_over_current_active(const treadmill_control_t *tc)
{
    return tc != NULL && tc->oc_active;
}

uint16_t treadmill_control_get_dc_bus_current(const treadmill_control_t *tc)
{
    return tc ? tc->dc_bus_current : 0;
}

void treadmill_control_set_motor_type(treadmill_control_t *tc, uint16_t watts)
{
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    if (watts > 0) {
        tc->motor_type = watts;
        tm_adapter_set_motor_type(&tc->adapter, watts);
    }
    pthread_mutex_unlock(&tc->mutex);
}

uint16_t treadmill_control_get_motor_type(const treadmill_control_t *tc)
{
    return tc ? tc->motor_type : 0;
}

void treadmill_control_set_tread_sense(treadmill_control_t *tc, bool enable)
{
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->tread_sense = enable;
    tm_adapter_set_tread_sense(&tc->adapter, enable);
    if (tc_in_workout(tc)) tm_adapter_set_detect_machine(&tc->adapter, true);
    pthread_mutex_unlock(&tc->mutex);
}

bool treadmill_control_get_person_in_place(const treadmill_control_t *tc)
{
    return tc != NULL && tc->person_in_place;
}

bool treadmill_control_is_safe_to_start(const treadmill_control_t *tc)
{
    return tc != NULL && tc_compute_safe(tc);
}

void treadmill_control_set_encoder_check(treadmill_control_t *tc, bool enable)
{
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->encoder_check_enabled = enable;
    tc_update_safe(tc);
    pthread_mutex_unlock(&tc->mutex);
}

void treadmill_control_set_error_block(treadmill_control_t *tc, bool blocked)
{
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->error_active = blocked;
    tc_update_safe(tc);
    pthread_mutex_unlock(&tc->mutex);
}

void treadmill_control_clear_error(treadmill_control_t *tc)
{
    treadmill_control_set_error_block(tc, false);
}

void treadmill_control_on_lcb_status(treadmill_control_t *tc, uint8_t status)
{
    if (tc == NULL) return;
    pthread_mutex_lock(&tc->mutex);
    tc->incline_moving = (status & (TM_STATUS_BIT_INCLINE_DOWN | TM_STATUS_BIT_INCLINE_UP)) != 0;
    tc->main_motor_on = (status & TM_STATUS_BIT_MAIN_MOTOR) != 0;
    tc->encoder_rpm_status = (status & TM_STATUS_BIT_ENCODER_SELF_POWER) != 0;
    tc->self_power = tc->encoder_rpm_status;
    tc_update_safe(tc);
    pthread_mutex_unlock(&tc->mutex);
}

bool treadmill_control_get_self_power(const treadmill_control_t *tc)
{
    return tc != NULL && tc->self_power;
}

bool treadmill_control_enter_erp(treadmill_control_t *tc)
{
    if (tc == NULL) return false;
    pthread_mutex_lock(&tc->mutex);
    bool ok = !tc_in_workout(tc);
    if (ok) tm_adapter_enter_erp(&tc->adapter);
    pthread_mutex_unlock(&tc->mutex);
    return ok;
}
