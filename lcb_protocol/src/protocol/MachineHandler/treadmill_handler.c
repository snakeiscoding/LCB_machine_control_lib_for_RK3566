/**
 * @file treadmill_handler.c
 * @brief LCB Treadmill Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/Treadmill/TreadMillHandler.java
 */

#include "treadmill_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_constants.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "TreadmillHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

static void auto_reply_heart_beat_status(treadmill_handler_t *th)
{
    if (th->comm == NULL) return;
    /* Toptek (0xC1) doesn't support heartbeat status query */
    if (th->machine_lcb_type == LCB_TYPE_TOPTEK) return;
    lcb_comm_get_lcb_status_low_priority(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_ext_status(treadmill_handler_t *th)
{
    if (!th->is_auto_reply_ext_status) return;
    if (th->cnt_ext_status >= TM_INTERVAL_EXT_STATUS) {
        th->cnt_ext_status = 0;
        if (th->comm != NULL)
            lcb_comm_get_lcb_treadmill_ext_status(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        th->cnt_ext_status++;
    }
}

static void auto_reply_rpm(treadmill_handler_t *th)
{
    if (!th->is_auto_reply_rpm) return;
    if (th->cnt_rpm >= TM_INTERVAL_RPM) {
        th->cnt_rpm = 0;
        if (th->comm != NULL)
            lcb_comm_get_lcb_motor_rpm(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        th->cnt_rpm++;
    }
}

static void auto_reply_cadence(treadmill_handler_t *th)
{
    if (!th->is_auto_reply_cadence) return;
    if (th->cnt_cadence >= TM_INTERVAL_CADENCE) {
        th->cnt_cadence = 0;
        if (th->comm != NULL)
            lcb_comm_get_lcb_treadmill_cadence(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        th->cnt_cadence++;
    }
}

static void auto_reply_incline(treadmill_handler_t *th)
{
    if (!th->is_auto_reply_incline) return;
    if (th->cnt_incline >= TM_INTERVAL_INCLINE) {
        th->cnt_incline = 0;
        if (th->comm != NULL)
            lcb_comm_get_lcb_incline_loc(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        th->cnt_incline++;
    }
}

static void auto_reply_in_used(treadmill_handler_t *th)
{
    if (!th->is_auto_reply_in_used) return;
    if (th->cnt_in_used >= TM_INTERVAL_IN_USED) {
        th->cnt_in_used = 0;
        if (th->comm != NULL)
            lcb_comm_get_lcb_treadmill_in_used(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        th->cnt_in_used++;
    }
}

static void auto_reply_speed_up_in_used(treadmill_handler_t *th)
{
    if (!th->is_auto_reply_speed_up_in_used) return;
    if (th->cnt_speed_up_in_used >= TM_INTERVAL_SPEEDUP_IN_USED) {
        th->cnt_speed_up_in_used = 0;
        if (th->comm != NULL)
            lcb_comm_get_lcb_treadmill_in_used(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        th->cnt_speed_up_in_used++;
    }
}

static void auto_reply_motor_info(treadmill_handler_t *th)
{
    if (!th->is_auto_reply_motor_info) return;
    if (th->cnt_motor_info >= TM_INTERVAL_MOTOR_INFO) {
        th->cnt_motor_info = 0;
        if (th->comm != NULL)
            lcb_comm_get_lcb_main_motor_info(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        th->cnt_motor_info++;
    }
}

/* ============================================================================
 * RPM Ramp State Machine (Java: checkSetAutoReplyTmRpmValueCmd)
 * ============================================================================ */

void th_proc_check_ramp_up_cmd(treadmill_handler_t *th)
{
    if (th == NULL) return;

    uint16_t target = th->gui_set_rpm;
    uint16_t current = th->set_current_rpm;
    int up_step = th->ramp_up_step;
    int down_step = th->ramp_down_step;

    if (target == 0) {
        /* Immediate stop */
        th->set_current_rpm = 0;
        if (th->comm != NULL)
            lcb_comm_set_lcb_motor_rpm_highest_priority(th->comm, 0);
        return;
    }

    if (current == target) {
        /* Already at target — re-send to keep alive */
        if (th->comm != NULL)
            lcb_comm_set_lcb_motor_rpm_highest_priority(th->comm, current);
        return;
    }

    if (target > current) {
        /* Ramp up — clamp to step */
        int diff = (int)target - (int)current;
        int delta = (diff > up_step) ? up_step : diff;
        current = (uint16_t)((int)current + delta);
    } else {
        /* Ramp down — clamp to step */
        int diff = (int)current - (int)target;
        int delta = (diff > down_step) ? down_step : diff;
        current = (uint16_t)((int)current - delta);
    }

    th->set_current_rpm = current;
    if (th->comm != NULL)
        lcb_comm_set_lcb_motor_rpm_highest_priority(th->comm, current);
}

static void auto_reply_set_ramp_up_and_last_rpm_cmd(treadmill_handler_t *th)
{
    if (!th->is_auto_ramp_rpm) return;
    if (th->cnt_ramp_rpm >= TM_INTERVAL_RAMP_RPM) {
        th->cnt_ramp_rpm = 0;
        th_proc_check_ramp_up_cmd(th);
    } else {
        th->cnt_ramp_rpm++;
    }
}

/* ============================================================================
 * No-RPM Error Timeout (Java: targetRpmStatusCheck)
 * ============================================================================ */

void th_proc_target_rpm_status_check(treadmill_handler_t *th)
{
    if (th == NULL) return;
    if (!th->target_rpm_check_flag) {
        th->cnt_target_rpm = 0;
        return;
    }

    if (th->machine_rpm > 0) {
        /* RPM responded — clear flag and counter */
        th->target_rpm_check_flag = false;
        th->cnt_target_rpm = 0;
        return;
    }

    th->cnt_target_rpm++;
    if (th->cnt_target_rpm >= TM_INTERVAL_TARGET_RPM_TIMEOUT) {
        /* 40-second timeout — fire no-RPM error */
        th->target_rpm_check_flag = false;
        th->cnt_target_rpm = 0;
        if (th->comm != NULL)
            lcb_comm_proc_tm_no_rpm_error(th->comm);
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void th_proc_auto_reply_tick(treadmill_handler_t *th)
{
    if (th == NULL) return;

    auto_reply_heart_beat_status(th);
    auto_reply_ext_status(th);
    auto_reply_rpm(th);
    auto_reply_cadence(th);
    auto_reply_incline(th);
    auto_reply_in_used(th);
    auto_reply_speed_up_in_used(th);
    auto_reply_motor_info(th);
    auto_reply_set_ramp_up_and_last_rpm_cmd(th);
    th_proc_target_rpm_status_check(th);
    /* IR Addr IR Status — stub, no-op */
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    treadmill_handler_t *th = (treadmill_handler_t *)arg;

    pthread_mutex_lock(&th->mutex);
    while (th->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += TM_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (TM_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&th->heartbeat_cond, &th->mutex, &ts);

        if (!th->is_running) break;

        th_proc_auto_reply_tick(th);
    }
    pthread_mutex_unlock(&th->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

treadmill_handler_t* treadmill_handler_create(void)
{
    treadmill_handler_t *th = (treadmill_handler_t*)calloc(1, sizeof(treadmill_handler_t));
    if (th == NULL) return NULL;

    /* Recursive mutex — same reason as general_handler: lcb_comm callbacks
     * may be invoked synchronously while heartbeat thread holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&th->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&th->heartbeat_cond, NULL);

    th->machine_lcb_type = LCB_TYPE_UNKNOWN;
    th->ramp_up_step = TM_DEFAULT_RAMP_STEP;
    th->ramp_down_step = TM_DEFAULT_RAMP_STEP;

    return th;
}

void treadmill_handler_destroy(treadmill_handler_t *th)
{
    if (th == NULL) return;
    treadmill_handler_stop(th);
    pthread_cond_destroy(&th->heartbeat_cond);
    pthread_mutex_destroy(&th->mutex);
    free(th);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void treadmill_handler_callbacks_init(treadmill_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(treadmill_handler_callbacks_t));
}

void treadmill_handler_set_callbacks(treadmill_handler_t *th,
                                     const treadmill_handler_callbacks_t *cb)
{
    if (th == NULL || cb == NULL) return;
    pthread_mutex_lock(&th->mutex);
    th->user_cb = *cb;
    pthread_mutex_unlock(&th->mutex);
}

void treadmill_handler_unregister_callbacks(treadmill_handler_t *th)
{
    if (th == NULL) return;
    pthread_mutex_lock(&th->mutex);
    memset(&th->user_cb, 0, sizeof(treadmill_handler_callbacks_t));
    pthread_mutex_unlock(&th->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool treadmill_handler_start(treadmill_handler_t *th, lcb_comm_t *comm)
{
    if (th == NULL || comm == NULL) return false;

    treadmill_handler_stop(th);

    pthread_mutex_lock(&th->mutex);
    th->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    th_lcb_cb_register(th);
    lcb_comm_register_callbacks(comm, &th->lcb_cb);

    th->is_running = true;
    pthread_mutex_unlock(&th->mutex);

    pthread_create(&th->heartbeat_thread, NULL, heartbeat_thread_func, th);
    LCB_LOGD(TAG, "Treadmill handler started");
    return true;
}

void treadmill_handler_stop(treadmill_handler_t *th)
{
    if (th == NULL) return;

    pthread_mutex_lock(&th->mutex);
    if (th->is_running) {
        th->is_running = false;
        pthread_cond_signal(&th->heartbeat_cond);
        pthread_mutex_unlock(&th->mutex);
        pthread_join(th->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&th->mutex);
    }

    if (th->comm != NULL) {
        lcb_comm_unregister_callbacks(th->comm);
        th->comm = NULL;
    }
    LCB_LOGD(TAG, "Treadmill handler stopped");
}

/* ============================================================================
 * Public: LCB Type Injection
 * ============================================================================ */

void treadmill_handler_set_lcb_type(treadmill_handler_t *th, LcbType_t type)
{
    if (th == NULL) return;
    pthread_mutex_lock(&th->mutex);
    th->machine_lcb_type = type;
    pthread_mutex_unlock(&th->mutex);
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

void treadmill_handler_set_rpm(treadmill_handler_t *th, uint16_t rpm)
{
    if (th == NULL) return;
    pthread_mutex_lock(&th->mutex);

    if (rpm == 0) {
        th->target_rpm_check_flag = false;
        th->cnt_target_rpm = 0;
    } else {
        th->target_rpm_check_flag = true;
        th->cnt_target_rpm = 0;
    }

    if (th->is_auto_ramp_rpm) {
        /* Ramp mode — store target; ramp tick will TX gradually */
        th->gui_set_rpm = rpm;
        if (rpm == 0) {
            /* Immediate stop even in ramp mode */
            th->set_current_rpm = 0;
            if (th->comm != NULL)
                lcb_comm_set_lcb_motor_rpm(th->comm, 0);
        }
    } else {
        if (th->comm != NULL)
            lcb_comm_set_lcb_motor_rpm(th->comm, rpm);
    }

    pthread_mutex_unlock(&th->mutex);
}

void treadmill_handler_set_rpm_high_priority(treadmill_handler_t *th, uint16_t rpm)
{
    if (th == NULL) return;
    pthread_mutex_lock(&th->mutex);

    if (rpm == 0) {
        th->target_rpm_check_flag = false;
        th->cnt_target_rpm = 0;
    } else {
        th->target_rpm_check_flag = true;
        th->cnt_target_rpm = 0;
    }

    if (th->is_auto_ramp_rpm) {
        th->gui_set_rpm = rpm;
        if (rpm == 0) {
            th->set_current_rpm = 0;
            if (th->comm != NULL)
                lcb_comm_set_lcb_motor_rpm_highest_priority(th->comm, 0);
        }
    } else {
        if (th->comm != NULL)
            lcb_comm_set_lcb_motor_rpm_highest_priority(th->comm, rpm);
    }

    pthread_mutex_unlock(&th->mutex);
}

void treadmill_handler_set_incline(treadmill_handler_t *th, uint16_t loc)
{
    if (th == NULL || th->comm == NULL) return;
    lcb_comm_set_lcb_incline_loc(th->comm, loc);
}

void treadmill_handler_set_incline_high_priority(treadmill_handler_t *th, uint16_t loc)
{
    if (th == NULL || th->comm == NULL) return;
    lcb_comm_set_lcb_incline_loc_highest_priority(th->comm, loc);
}

void treadmill_handler_set_incline_action(treadmill_handler_t *th, uint8_t action)
{
    if (th == NULL || th->comm == NULL) return;
    /* Skip for LCB_TYPE_E0 and LCB_TYPE_TOPTEK (Java behavior) */
    if (th->machine_lcb_type == LCB_TYPE_E0 || th->machine_lcb_type == LCB_TYPE_TOPTEK) {
        LCB_LOGD(TAG, "set_incline_action skipped for LCB type 0x%02X", th->machine_lcb_type);
        return;
    }
    lcb_comm_set_lcb_incline_action(th->comm, action);
}

void treadmill_handler_set_incline_action_high_priority(treadmill_handler_t *th, uint8_t action)
{
    if (th == NULL || th->comm == NULL) return;
    /* Skip for LCB_TYPE_E0 and LCB_TYPE_TOPTEK (Java behavior) */
    if (th->machine_lcb_type == LCB_TYPE_E0 || th->machine_lcb_type == LCB_TYPE_TOPTEK) {
        LCB_LOGD(TAG, "set_incline_action_high_priority skipped for LCB type 0x%02X", th->machine_lcb_type);
        return;
    }
    lcb_comm_set_lcb_incline_action_highest_priority(th->comm, action);
}

void treadmill_handler_set_motor_type(treadmill_handler_t *th, uint16_t watts)
{
    if (th == NULL || th->comm == NULL) return;
    lcb_comm_set_lcb_motor_specification(th->comm, watts);
}

void treadmill_handler_set_ecb_action_high_priority(treadmill_handler_t *th, uint8_t value)
{
    if (th == NULL || th->comm == NULL) return;
    lcb_comm_set_lcb_treadmill_ecb_action_highest_priority(th->comm, value);
}

void treadmill_handler_send_treadmill_operate(treadmill_handler_t *th,
                                              const uint8_t *data, uint8_t length)
{
    if (th == NULL || th->comm == NULL || data == NULL) return;
    lcb_comm_set_lcb_treadmill_operate(th->comm, data, length);
}

/* ============================================================================
 * Public: One-shot Getters
 * ============================================================================ */

void treadmill_handler_get_one_shot_rpm(treadmill_handler_t *th)
{
    if (th == NULL || th->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_motor_rpm_high_priority(th->comm);
}

void treadmill_handler_get_one_shot_incline_adc(treadmill_handler_t *th)
{
    if (th == NULL || th->comm == NULL) return;
    lcb_comm_get_lcb_incline_loc(th->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

/* ============================================================================
 * Public: Auto-reply Toggles
 * ============================================================================ */

#define TM_DEFINE_AUTO_REPLY_TOGGLE(name) \
    void treadmill_handler_set_auto_reply_##name(treadmill_handler_t *th, bool enable) { \
        if (th == NULL) return; \
        pthread_mutex_lock(&th->mutex); \
        th->is_auto_reply_##name = enable; \
        pthread_mutex_unlock(&th->mutex); \
    }

TM_DEFINE_AUTO_REPLY_TOGGLE(rpm)
TM_DEFINE_AUTO_REPLY_TOGGLE(incline)
TM_DEFINE_AUTO_REPLY_TOGGLE(in_used)
TM_DEFINE_AUTO_REPLY_TOGGLE(speed_up_in_used)
TM_DEFINE_AUTO_REPLY_TOGGLE(motor_info)
TM_DEFINE_AUTO_REPLY_TOGGLE(ext_status)
TM_DEFINE_AUTO_REPLY_TOGGLE(cadence)
TM_DEFINE_AUTO_REPLY_TOGGLE(ir_addr_ir_status)   /* stub */

/* ============================================================================
 * Public: RPM Ramp Control
 * ============================================================================ */

void treadmill_handler_set_auto_ramp_rpm(treadmill_handler_t *th, bool enable,
                                         int ramp_up_step, int ramp_down_step)
{
    if (th == NULL) return;
    pthread_mutex_lock(&th->mutex);
    th->is_auto_ramp_rpm = enable;
    if (ramp_up_step > 0)   th->ramp_up_step   = ramp_up_step;
    if (ramp_down_step > 0) th->ramp_down_step = ramp_down_step;
    pthread_mutex_unlock(&th->mutex);
}

void treadmill_handler_estop_reset_gui_current_rpm(treadmill_handler_t *th)
{
    if (th == NULL) return;
    pthread_mutex_lock(&th->mutex);
    th->gui_set_rpm = 0;
    th->set_current_rpm = 0;
    pthread_mutex_unlock(&th->mutex);
}

void treadmill_handler_send_estop_unlock(treadmill_handler_t *th)
{
    if (th == NULL || th->comm == NULL) return;
    lcb_comm_proc_estop_unlock(th->comm);
}

void treadmill_handler_estop_revert_lcb_initial(treadmill_handler_t *th)
{
    if (th == NULL || th->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(th->comm);
    lcb_comm_set_lcb_operating_status_highest_priority(th->comm, LCB_OPERATING_STOP);
    lcb_comm_set_lcb_operating_status_highest_priority(th->comm, LCB_OPERATING_ACTION);
    lcb_comm_proc_estop_unlock(th->comm);
}

/* ============================================================================
 * Public: Hybrid TM IR Address — stubs (no underlying lcb_comm support yet)
 * ============================================================================ */

void treadmill_handler_get_hybrid_tm_ir_addr_version(treadmill_handler_t *th)        { (void)th; /* TODO */ }
void treadmill_handler_get_hybrid_tm_ir_addr_ir_status(treadmill_handler_t *th)      { (void)th; /* TODO */ }
void treadmill_handler_set_hybrid_tm_ir_addr_enter_erp_mode(treadmill_handler_t *th) { (void)th; /* TODO */ }
void treadmill_handler_set_hybrid_tm_ir_addr_leave_erp_mode(treadmill_handler_t *th) { (void)th; /* TODO */ }
