/**
 * @file hybrid_tm_handler.c
 * @brief LCB Hybrid TM Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/HybridTM/HybridTmHandler.java
 */

#include "hybrid_tm_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_constants.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "HybridTmHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic (Java: autoReplyCmd() and its
 * per-feature sub-methods)
 * ============================================================================ */

/* Java: autoReplyHeartBeatStatus() — unconditional every tick */
static void auto_reply_heart_beat_status(hybrid_tm_handler_t *hth)
{
    if (hth->comm == NULL) return;
    lcb_comm_get_lcb_status(hth->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_rpm(hybrid_tm_handler_t *hth)
{
    if (!hth->is_auto_reply_rpm) return;
    if (hth->cnt_rpm >= HYBRID_TM_INTERVAL_RPM) {
        hth->cnt_rpm = 0;
        if (hth->comm != NULL)
            lcb_comm_get_lcb_motor_rpm(hth->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        hth->cnt_rpm++;
    }
}

static void auto_reply_incline(hybrid_tm_handler_t *hth)
{
    if (!hth->is_auto_reply_incline) return;
    if (hth->cnt_incline >= HYBRID_TM_INTERVAL_INCLINE) {
        hth->cnt_incline = 0;
        if (hth->comm != NULL)
            lcb_comm_get_lcb_incline_loc(hth->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        hth->cnt_incline++;
    }
}

/* Java: autoReplyInUsed() only polls while mMachineRpm > 100 */
static void auto_reply_in_used(hybrid_tm_handler_t *hth)
{
    if (!hth->is_auto_reply_in_used) return;
    if (hth->machine_rpm <= HYBRID_TM_IN_USED_RPM_GATE) return;

    if (hth->cnt_in_used >= HYBRID_TM_INTERVAL_IN_USED) {
        hth->cnt_in_used = 0;
        if (hth->comm != NULL)
            lcb_comm_get_lcb_treadmill_in_used(hth->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        hth->cnt_in_used++;
    }
}

static void auto_reply_motor_info(hybrid_tm_handler_t *hth)
{
    if (!hth->is_auto_reply_motor_info) return;
    if (hth->cnt_motor_info >= HYBRID_TM_INTERVAL_MOTOR_INFO) {
        hth->cnt_motor_info = 0;
        if (hth->comm != NULL)
            lcb_comm_get_lcb_main_motor_info(hth->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        hth->cnt_motor_info++;
    }
}

/* IR Address board (0x26) periodic beam-status poll — ported onto this
 * handler per hybrid_tm_handler.h's file-header NOTE. */
static void auto_reply_ir_addr_ir_status(hybrid_tm_handler_t *hth)
{
    if (!hth->is_auto_reply_ir_addr_ir_status) return;
    if (hth->cnt_ir_addr_ir_status >= HYBRID_TM_INTERVAL_IR_ADDR_IR_STATUS) {
        hth->cnt_ir_addr_ir_status = 0;
        if (hth->comm != NULL)
            lcb_comm_get_ir_addr_ir_status(hth->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        hth->cnt_ir_addr_ir_status++;
    }
}

/* ============================================================================
 * No-RPM Error Timeout (Java: targetRpmStatusCheck)
 *
 * Literal translation of the Java state machine — note this does NOT clear
 * the flag early when machine_rpm becomes nonzero while the flag is set
 * (unlike treadmill_handler's th_proc_target_rpm_status_check, which does);
 * HybridTmHandler.java only resets the counter when the flag itself is
 * false, and only inspects machine_rpm to gate whether the counter advances.
 * Confirmed identical in both this project's and chimera2's Java sources.
 * ============================================================================ */

void hth_proc_target_rpm_status_check(hybrid_tm_handler_t *hth)
{
    if (hth == NULL) return;

    if (!hth->target_rpm_check_flag) {
        hth->cnt_target_rpm = 0;
        return;
    }

    if (hth->machine_rpm != 0) return;

    if (hth->cnt_target_rpm >= HYBRID_TM_INTERVAL_TARGET_RPM_TIMEOUT) {
        hth->target_rpm_check_flag = false;
        hth->cnt_target_rpm = 0;
        if (hth->comm != NULL)
            lcb_comm_proc_tm_no_rpm_error(hth->comm);
    } else {
        hth->cnt_target_rpm++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases (Java: autoReplyCmd())
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void hth_proc_auto_reply_tick(hybrid_tm_handler_t *hth)
{
    if (hth == NULL) return;

    auto_reply_heart_beat_status(hth);
    auto_reply_rpm(hth);
    auto_reply_incline(hth);
    auto_reply_in_used(hth);
    auto_reply_motor_info(hth);

    if (hth->target_rpm_check_flag)
        hth_proc_target_rpm_status_check(hth);

    auto_reply_ir_addr_ir_status(hth);
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)arg;

    pthread_mutex_lock(&hth->mutex);
    while (hth->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += HYBRID_TM_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (HYBRID_TM_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&hth->heartbeat_cond, &hth->mutex, &ts);

        if (!hth->is_running) break;

        hth_proc_auto_reply_tick(hth);
    }
    pthread_mutex_unlock(&hth->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

hybrid_tm_handler_t* hybrid_tm_handler_create(void)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t*)calloc(1, sizeof(hybrid_tm_handler_t));
    if (hth == NULL) return NULL;

    /* Recursive mutex — same reason as treadmill_handler/general_handler:
     * lcb_comm callbacks may be invoked synchronously while heartbeat thread
     * holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&hth->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&hth->heartbeat_cond, NULL);

    return hth;
}

void hybrid_tm_handler_destroy(hybrid_tm_handler_t *hth)
{
    if (hth == NULL) return;
    hybrid_tm_handler_stop(hth);
    pthread_cond_destroy(&hth->heartbeat_cond);
    pthread_mutex_destroy(&hth->mutex);
    free(hth);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void hybrid_tm_handler_callbacks_init(hybrid_tm_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(hybrid_tm_handler_callbacks_t));
}

void hybrid_tm_handler_set_callbacks(hybrid_tm_handler_t *hth,
                                     const hybrid_tm_handler_callbacks_t *cb)
{
    if (hth == NULL || cb == NULL) return;
    pthread_mutex_lock(&hth->mutex);
    hth->user_cb = *cb;
    pthread_mutex_unlock(&hth->mutex);
}

void hybrid_tm_handler_unregister_callbacks(hybrid_tm_handler_t *hth)
{
    if (hth == NULL) return;
    pthread_mutex_lock(&hth->mutex);
    memset(&hth->user_cb, 0, sizeof(hybrid_tm_handler_callbacks_t));
    pthread_mutex_unlock(&hth->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool hybrid_tm_handler_start(hybrid_tm_handler_t *hth, lcb_comm_t *comm)
{
    if (hth == NULL || comm == NULL) return false;

    hybrid_tm_handler_stop(hth);

    pthread_mutex_lock(&hth->mutex);
    hth->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    hth_lcb_cb_register(hth);
    lcb_comm_register_callbacks(comm, &hth->lcb_cb);

    /* Java: initialHybridTm() explicitly resets all 4 auto-reply flags */
    hth->is_auto_reply_rpm = false;
    hth->is_auto_reply_incline = false;
    hth->is_auto_reply_in_used = false;
    hth->is_auto_reply_motor_info = false;
    hth->is_auto_reply_ir_addr_ir_status = false;
    hth->pending_ir_estop_check = false;
    hth->ir_estop_triggered = false;

    hth->is_running = true;
    pthread_mutex_unlock(&hth->mutex);

    pthread_create(&hth->heartbeat_thread, NULL, heartbeat_thread_func, hth);
    LCB_LOGD(TAG, "HybridTm handler started");
    return true;
}

void hybrid_tm_handler_stop(hybrid_tm_handler_t *hth)
{
    if (hth == NULL) return;

    pthread_mutex_lock(&hth->mutex);
    if (hth->is_running) {
        hth->is_running = false;
        pthread_cond_signal(&hth->heartbeat_cond);
        pthread_mutex_unlock(&hth->mutex);
        pthread_join(hth->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&hth->mutex);
    }

    if (hth->comm != NULL) {
        lcb_comm_unregister_callbacks(hth->comm);
        hth->comm = NULL;
    }
    LCB_LOGD(TAG, "HybridTm handler stopped");
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

void hybrid_tm_handler_set_rpm(hybrid_tm_handler_t *hth, uint16_t rpm)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_set_lcb_motor_rpm(hth->comm, rpm);
}

void hybrid_tm_handler_set_incline(hybrid_tm_handler_t *hth, uint16_t loc)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_set_lcb_incline_loc(hth->comm, loc);
}

void hybrid_tm_handler_set_incline_action(hybrid_tm_handler_t *hth, uint8_t action)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_set_lcb_incline_action(hth->comm, action);
}

void hybrid_tm_handler_set_motor_type(hybrid_tm_handler_t *hth, uint16_t watts)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_set_lcb_motor_specification(hth->comm, watts);
}

/* ============================================================================
 * Public: Auto-reply Toggles
 * ============================================================================ */

void hybrid_tm_handler_set_auto_reply_rpm(hybrid_tm_handler_t *hth, bool enable)
{
    if (hth == NULL) return;
    pthread_mutex_lock(&hth->mutex);
    hth->is_auto_reply_rpm = enable;
    pthread_mutex_unlock(&hth->mutex);
}

void hybrid_tm_handler_set_auto_reply_incline(hybrid_tm_handler_t *hth, bool enable)
{
    if (hth == NULL) return;
    pthread_mutex_lock(&hth->mutex);
    hth->is_auto_reply_incline = enable;
    pthread_mutex_unlock(&hth->mutex);
}

void hybrid_tm_handler_set_auto_reply_in_used(hybrid_tm_handler_t *hth, bool enable)
{
    if (hth == NULL) return;
    pthread_mutex_lock(&hth->mutex);
    hth->is_auto_reply_in_used = enable;
    pthread_mutex_unlock(&hth->mutex);
}

void hybrid_tm_handler_set_auto_reply_motor_info(hybrid_tm_handler_t *hth, bool enable)
{
    if (hth == NULL) return;
    pthread_mutex_lock(&hth->mutex);
    hth->is_auto_reply_motor_info = enable;
    pthread_mutex_unlock(&hth->mutex);
}

/* ============================================================================
 * Public: IR Address Board (0x26) — see hybrid_tm_handler.h file-header NOTE
 * ============================================================================ */

void hybrid_tm_handler_get_ir_addr_version(hybrid_tm_handler_t *hth)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_get_ir_addr_version(hth->comm);
}

void hybrid_tm_handler_get_ir_addr_ir_status(hybrid_tm_handler_t *hth)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_get_ir_addr_ir_status_highest_priority(hth->comm);
}

void hybrid_tm_handler_set_ir_addr_enter_erp_mode(hybrid_tm_handler_t *hth)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_set_ir_addr_enter_erp_mode(hth->comm);
}

void hybrid_tm_handler_set_ir_addr_leave_erp_mode(hybrid_tm_handler_t *hth)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_set_ir_addr_leave_erp_mode(hth->comm);
}

void hybrid_tm_handler_set_auto_reply_ir_addr_ir_status(hybrid_tm_handler_t *hth, bool enable)
{
    if (hth == NULL) return;
    pthread_mutex_lock(&hth->mutex);
    hth->is_auto_reply_ir_addr_ir_status = enable;
    pthread_mutex_unlock(&hth->mutex);
}

bool hybrid_tm_handler_is_ir_estop_triggered(hybrid_tm_handler_t *hth)
{
    if (hth == NULL) return false;
    pthread_mutex_lock(&hth->mutex);
    bool triggered = hth->ir_estop_triggered;
    pthread_mutex_unlock(&hth->mutex);
    return triggered;
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void hybrid_tm_handler_estop_revert_lcb_initial(hybrid_tm_handler_t *hth)
{
    if (hth == NULL || hth->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(hth->comm);
    lcb_comm_set_lcb_operating_status_highest_priority(hth->comm, LCB_OPERATING_STOP);
    lcb_comm_set_lcb_operating_status_highest_priority(hth->comm, LCB_OPERATING_ACTION);
    lcb_comm_proc_estop_unlock(hth->comm);
}
