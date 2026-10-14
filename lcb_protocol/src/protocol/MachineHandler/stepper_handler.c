/**
 * @file stepper_handler.c
 * @brief LCB Stepper Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/Stepper/StepperHandler.java
 */

#include "stepper_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "StepperHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

/* Unconditional LCB status keep-alive — in the Java original this traffic
 * comes from GeneralHandler's own heartbeat, which keeps running forever
 * alongside StepperHandler (Java never stops it). This C port hands the
 * shared comm exclusively to stepper_handler once general_handler is
 * stopped (see hal_stepper_gh_on_lcb_initial_finish in HalEntryPoint.c), so
 * stepper_handler must carry this heartbeat itself — same convention
 * treadmill_handler.c/bike_handler.c/elliptical_handler.c already follow via
 * their own auto_reply_heart_beat_status(). Without it, no RS485 traffic
 * goes out at all until the user toggles the Auto reply on. */
static void auto_reply_heart_beat_status(stepper_handler_t *sh)
{
    if (sh->comm == NULL) return;
    lcb_comm_get_lcb_status_low_priority(sh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_rpm(stepper_handler_t *sh)
{
    if (!sh->is_auto_reply_rpm) return;
    if (sh->cnt_rpm >= STEPPER_INTERVAL_RPM) {
        sh->cnt_rpm = 0;
        if (sh->comm != NULL)
            lcb_comm_get_lcb_ecb_rpm(sh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        sh->cnt_rpm++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void sh_proc_auto_reply_tick(stepper_handler_t *sh)
{
    if (sh == NULL) return;

    auto_reply_heart_beat_status(sh);
    auto_reply_rpm(sh);
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    stepper_handler_t *sh = (stepper_handler_t *)arg;

    pthread_mutex_lock(&sh->mutex);
    while (sh->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += STEPPER_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (STEPPER_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&sh->heartbeat_cond, &sh->mutex, &ts);

        if (!sh->is_running) break;

        sh_proc_auto_reply_tick(sh);
    }
    pthread_mutex_unlock(&sh->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

stepper_handler_t* stepper_handler_create(void)
{
    stepper_handler_t *sh = (stepper_handler_t*)calloc(1, sizeof(stepper_handler_t));
    if (sh == NULL) return NULL;

    /* Recursive mutex — same reason as treadmill_handler/bike_handler/
     * elliptical_handler: lcb_comm callbacks may be invoked synchronously
     * while heartbeat thread holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&sh->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&sh->heartbeat_cond, NULL);

    return sh;
}

void stepper_handler_destroy(stepper_handler_t *sh)
{
    if (sh == NULL) return;
    stepper_handler_stop(sh);
    pthread_cond_destroy(&sh->heartbeat_cond);
    pthread_mutex_destroy(&sh->mutex);
    free(sh);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void stepper_handler_callbacks_init(stepper_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(stepper_handler_callbacks_t));
}

void stepper_handler_set_callbacks(stepper_handler_t *sh, const stepper_handler_callbacks_t *cb)
{
    if (sh == NULL || cb == NULL) return;
    pthread_mutex_lock(&sh->mutex);
    sh->user_cb = *cb;
    pthread_mutex_unlock(&sh->mutex);
}

void stepper_handler_unregister_callbacks(stepper_handler_t *sh)
{
    if (sh == NULL) return;
    pthread_mutex_lock(&sh->mutex);
    memset(&sh->user_cb, 0, sizeof(stepper_handler_callbacks_t));
    pthread_mutex_unlock(&sh->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool stepper_handler_start(stepper_handler_t *sh, lcb_comm_t *comm)
{
    if (sh == NULL || comm == NULL) return false;

    stepper_handler_stop(sh);

    pthread_mutex_lock(&sh->mutex);
    sh->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    sh_lcb_cb_register(sh);
    lcb_comm_register_callbacks(comm, &sh->lcb_cb);

    sh->is_running = true;
    pthread_mutex_unlock(&sh->mutex);

    pthread_create(&sh->heartbeat_thread, NULL, heartbeat_thread_func, sh);
    LCB_LOGD(TAG, "Stepper handler started");
    return true;
}

void stepper_handler_stop(stepper_handler_t *sh)
{
    if (sh == NULL) return;

    pthread_mutex_lock(&sh->mutex);
    if (sh->is_running) {
        sh->is_running = false;
        pthread_cond_signal(&sh->heartbeat_cond);
        pthread_mutex_unlock(&sh->mutex);
        pthread_join(sh->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&sh->mutex);
    }

    if (sh->comm != NULL) {
        lcb_comm_unregister_callbacks(sh->comm);
        sh->comm = NULL;
    }
    LCB_LOGD(TAG, "Stepper handler stopped");
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

void stepper_handler_set_resistance(stepper_handler_t *sh, uint16_t value)
{
    if (sh == NULL || sh->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_resistance(sh->comm, value);
}

void stepper_handler_set_gear_ratio(stepper_handler_t *sh, uint16_t ratio)
{
    if (sh == NULL || sh->comm == NULL) return;
    lcb_comm_set_lcb_rpm_gear_ratio(sh->comm, ratio);
}

void stepper_handler_set_resistance_type(stepper_handler_t *sh, uint16_t type)
{
    if (sh == NULL || sh->comm == NULL) return;
    lcb_comm_set_lcb_resistance_type(sh->comm, type);
}

void stepper_handler_set_turn_off_lcb(stepper_handler_t *sh, uint16_t minutes)
{
    if (sh == NULL || sh->comm == NULL) return;
    lcb_comm_set_lcb_power_off(sh->comm, minutes);
}

/* ============================================================================
 * Public: One-shot Getter
 * ============================================================================ */

void stepper_handler_get_one_shot_rpm(stepper_handler_t *sh)
{
    if (sh == NULL || sh->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(sh->comm);
}

/* ============================================================================
 * Public: Auto-reply Toggle
 * ============================================================================ */

void stepper_handler_set_auto_reply_rpm(stepper_handler_t *sh, bool enable)
{
    if (sh == NULL) return;
    pthread_mutex_lock(&sh->mutex);
    sh->is_auto_reply_rpm = enable;
    pthread_mutex_unlock(&sh->mutex);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void stepper_handler_estop_revert_lcb_initial(stepper_handler_t *sh)
{
    if (sh == NULL || sh->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(sh->comm);
    lcb_comm_proc_estop_unlock(sh->comm);
}
