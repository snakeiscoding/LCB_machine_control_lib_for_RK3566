/**
 * @file treadmill_handler_lcb_cb.c
 * @brief LCB Callback Bridge for Treadmill Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/Treadmill/TreadMillCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives treadmill_handler_t* via user_data, locks the
 * recursive mutex, processes, and dispatches to user-facing callbacks.
 */

#include "treadmill_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_proc_comm.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "TreadmillHandlerLcbCb"

/* ============================================================================
 * Speed & Incline
 * ============================================================================ */

/* 0xF1 GetMotorRPM (auto-poll response) */
static void th_on_lcb_motor_rpm_received(void *ud, uint16_t rpm)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    pthread_mutex_lock(&th->mutex);
    th->machine_rpm = rpm;
    if (th->user_cb.on_treadmill_rpm)
        th->user_cb.on_treadmill_rpm(th->user_cb.user_data, rpm);
    pthread_mutex_unlock(&th->mutex);
}

/* One-shot GetMotorRPM */
static void th_on_one_shot_lcb_motor_rpm(void *ud, uint16_t rpm)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    pthread_mutex_lock(&th->mutex);
    th->machine_rpm = rpm;
    if (th->user_cb.on_treadmill_one_shot_rpm)
        th->user_cb.on_treadmill_one_shot_rpm(th->user_cb.user_data, rpm);
    pthread_mutex_unlock(&th->mutex);
}

/* 0xF6 GetInclineLoc (auto-poll response) */
static void th_on_lcb_incline_loc_received(void *ud, uint16_t loc)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_incline_loc)
        th->user_cb.on_treadmill_incline_loc(th->user_cb.user_data, loc);
}

/* ============================================================================
 * Status
 * ============================================================================ */

/* Raw LCB status byte (every response packet) */
static void th_on_lcb_status(void *ud, uint8_t lcb_status)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_lcb_status)
        th->user_cb.on_treadmill_lcb_status(th->user_cb.user_data, lcb_status);
}

/* Treadmill InUsed */
static void th_on_lcb_treadmill_in_used(void *ud, uint8_t status)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_in_used)
        th->user_cb.on_treadmill_in_used(th->user_cb.user_data, status);
}

/* Main Motor Info */
static void th_on_lcb_main_motor_info(void *ud, const uint8_t *info, uint8_t length)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_motor_info)
        th->user_cb.on_treadmill_motor_info(th->user_cb.user_data, info, length);
}

/* Treadmill Ext Status */
static void th_on_lcb_treadmill_ext_status(void *ud, uint16_t status)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_ext_status)
        th->user_cb.on_treadmill_ext_status(th->user_cb.user_data, status);
}

/* 0x7F Cadence (SPM) */
static void th_on_lcb_treadmill_cadence(void *ud, const uint8_t *data, uint8_t length)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_cadence)
        th->user_cb.on_treadmill_cadence(th->user_cb.user_data, data, length);
}

/* 0x7E Treadmill Operate response */
static void th_on_lcb_treadmill_operate(void *ud, const uint8_t *data, uint8_t length)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_operate)
        th->user_cb.on_treadmill_operate(th->user_cb.user_data, data, length);
}

/* ============================================================================
 * Service Switch (TM repurposes battery-capacity-status bit)
 * ============================================================================ */

static void th_on_battery_capacity_changed(void *ud, bool is_not_enough)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    /* Java: maps battery-capacity-status to service switch state */
    if (th->user_cb.on_treadmill_service_switch)
        th->user_cb.on_treadmill_service_switch(th->user_cb.user_data, is_not_enough);
}

/* ============================================================================
 * GUI Mirror Callbacks
 * ============================================================================ */

static void th_on_treadmill_rpm_value_set(void *ud, uint16_t rpm)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_lcb_set_rpm_value)
        th->user_cb.on_treadmill_lcb_set_rpm_value(th->user_cb.user_data, rpm);
}

static void th_on_treadmill_adc_value_set(void *ud, uint16_t adc)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    if (th->user_cb.on_treadmill_lcb_set_adc_value)
        th->user_cb.on_treadmill_lcb_set_adc_value(th->user_cb.user_data, adc);
}

/* ============================================================================
 * Error Code — clears target_rpm_check_flag on 0x02B2 (E-Stop)
 * ============================================================================ */

static void th_on_lcb_error_code(void *ud, uint16_t error_code)
{
    treadmill_handler_t *th = (treadmill_handler_t *)ud;
    pthread_mutex_lock(&th->mutex);
    /* Java: 0x02B2 indicates RPM-related E-Stop — stop the timeout monitoring */
    if (error_code == LCB_ERROR_E_STOP) {
        th->target_rpm_check_flag = false;
        th->cnt_target_rpm = 0;
        LCB_LOGD(TAG, "0x02B2 received, clearing target_rpm_check_flag");
    }
    pthread_mutex_unlock(&th->mutex);

    if (th->user_cb.on_treadmill_error_code)
        th->user_cb.on_treadmill_error_code(th->user_cb.user_data, error_code);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void th_lcb_cb_register(treadmill_handler_t *th)
{
    lcb_comm_callbacks_init(&th->lcb_cb);
    th->lcb_cb.user_data = th;

    /* Speed & Incline */
    th->lcb_cb.on_lcb_motor_rpm_received  = th_on_lcb_motor_rpm_received;
    th->lcb_cb.on_one_shot_lcb_motor_rpm  = th_on_one_shot_lcb_motor_rpm;
    th->lcb_cb.on_lcb_incline_loc_received = th_on_lcb_incline_loc_received;

    /* Status */
    th->lcb_cb.on_lcb_status              = th_on_lcb_status;
    th->lcb_cb.on_lcb_treadmill_in_used   = th_on_lcb_treadmill_in_used;
    th->lcb_cb.on_lcb_main_motor_info     = th_on_lcb_main_motor_info;
    th->lcb_cb.on_lcb_treadmill_ext_status = th_on_lcb_treadmill_ext_status;
    th->lcb_cb.on_lcb_treadmill_operate   = th_on_lcb_treadmill_operate;
    th->lcb_cb.on_lcb_treadmill_cadence   = th_on_lcb_treadmill_cadence;

    /* Service switch */
    th->lcb_cb.on_battery_capacity_status_changed = th_on_battery_capacity_changed;

    /* GUI mirroring */
    th->lcb_cb.on_treadmill_rpm_value_set = th_on_treadmill_rpm_value_set;
    th->lcb_cb.on_treadmill_adc_value_set = th_on_treadmill_adc_value_set;

    /* Error code */
    th->lcb_cb.on_lcb_error_code = th_on_lcb_error_code;
}

void th_lcb_cb_unregister(treadmill_handler_t *th)
{
    memset(&th->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
