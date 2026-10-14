/**
 * @file elliptical_handler.c
 * @brief LCB Elliptical Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/Elliptical/EllipticalHandler.java
 */

#include "elliptical_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "EllipticalHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

/* Unconditional LCB status keep-alive — in the Java original this traffic
 * comes from GeneralHandler's own heartbeat, which keeps running forever
 * alongside EllipticalHandler (Java never stops it). This C port hands the
 * shared comm exclusively to elliptical_handler once general_handler is
 * stopped (see hal_ep_gh_on_lcb_initial_finish in HalEntryPoint.c), so
 * elliptical_handler must carry this heartbeat itself — same convention
 * treadmill_handler.c and bike_handler.c already follow via their own
 * auto_reply_heart_beat_status(). Without it, no RS485 traffic goes out at
 * all until the user toggles the Auto reply on. */
static void auto_reply_heart_beat_status(elliptical_handler_t *eh)
{
    if (eh->comm == NULL) return;
    lcb_comm_get_lcb_status_low_priority(eh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_rpm(elliptical_handler_t *eh)
{
    if (!eh->is_auto_reply_rpm) return;
    if (eh->cnt_rpm >= ELLIPTICAL_INTERVAL_RPM) {
        eh->cnt_rpm = 0;
        if (eh->comm != NULL)
            lcb_comm_get_lcb_ecb_rpm(eh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        eh->cnt_rpm++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void ep_proc_auto_reply_tick(elliptical_handler_t *eh)
{
    if (eh == NULL) return;

    auto_reply_heart_beat_status(eh);
    auto_reply_rpm(eh);
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    elliptical_handler_t *eh = (elliptical_handler_t *)arg;

    pthread_mutex_lock(&eh->mutex);
    while (eh->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += ELLIPTICAL_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (ELLIPTICAL_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&eh->heartbeat_cond, &eh->mutex, &ts);

        if (!eh->is_running) break;

        ep_proc_auto_reply_tick(eh);
    }
    pthread_mutex_unlock(&eh->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

elliptical_handler_t* elliptical_handler_create(void)
{
    elliptical_handler_t *eh = (elliptical_handler_t*)calloc(1, sizeof(elliptical_handler_t));
    if (eh == NULL) return NULL;

    /* Recursive mutex — same reason as treadmill_handler/bike_handler:
     * lcb_comm callbacks may be invoked synchronously while heartbeat thread
     * holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&eh->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&eh->heartbeat_cond, NULL);

    eh->machine_lcb_type = LCB_TYPE_UNKNOWN;

    return eh;
}

void elliptical_handler_destroy(elliptical_handler_t *eh)
{
    if (eh == NULL) return;
    elliptical_handler_stop(eh);
    pthread_cond_destroy(&eh->heartbeat_cond);
    pthread_mutex_destroy(&eh->mutex);
    free(eh);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void elliptical_handler_callbacks_init(elliptical_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(elliptical_handler_callbacks_t));
}

void elliptical_handler_set_callbacks(elliptical_handler_t *eh, const elliptical_handler_callbacks_t *cb)
{
    if (eh == NULL || cb == NULL) return;
    pthread_mutex_lock(&eh->mutex);
    eh->user_cb = *cb;
    pthread_mutex_unlock(&eh->mutex);
}

void elliptical_handler_unregister_callbacks(elliptical_handler_t *eh)
{
    if (eh == NULL) return;
    pthread_mutex_lock(&eh->mutex);
    memset(&eh->user_cb, 0, sizeof(elliptical_handler_callbacks_t));
    pthread_mutex_unlock(&eh->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool elliptical_handler_start(elliptical_handler_t *eh, lcb_comm_t *comm)
{
    if (eh == NULL || comm == NULL) return false;

    elliptical_handler_stop(eh);

    pthread_mutex_lock(&eh->mutex);
    eh->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    ep_lcb_cb_register(eh);
    lcb_comm_register_callbacks(comm, &eh->lcb_cb);

    eh->is_running = true;
    pthread_mutex_unlock(&eh->mutex);

    pthread_create(&eh->heartbeat_thread, NULL, heartbeat_thread_func, eh);
    LCB_LOGD(TAG, "Elliptical handler started");
    return true;
}

void elliptical_handler_stop(elliptical_handler_t *eh)
{
    if (eh == NULL) return;

    pthread_mutex_lock(&eh->mutex);
    if (eh->is_running) {
        eh->is_running = false;
        pthread_cond_signal(&eh->heartbeat_cond);
        pthread_mutex_unlock(&eh->mutex);
        pthread_join(eh->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&eh->mutex);
    }

    if (eh->comm != NULL) {
        lcb_comm_unregister_callbacks(eh->comm);
        eh->comm = NULL;
    }
    LCB_LOGD(TAG, "Elliptical handler stopped");
}

/* ============================================================================
 * Public: LCB Type Injection
 * ============================================================================ */

void elliptical_handler_set_lcb_type(elliptical_handler_t *eh, LcbType_t type)
{
    if (eh == NULL) return;
    pthread_mutex_lock(&eh->mutex);
    eh->machine_lcb_type = type;
    pthread_mutex_unlock(&eh->mutex);
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

/* Java: setEllipticalResistance() switches on the detected machine LCB type */
void elliptical_handler_set_resistance(elliptical_handler_t *eh, uint16_t value)
{
    if (eh == NULL || eh->comm == NULL) return;

    pthread_mutex_lock(&eh->mutex);
    switch (eh->machine_lcb_type) {
        case LCB_TYPE_ECB_BOARD:
            lcb_comm_set_lcb_ecb_position(eh->comm, value);
            break;
        default:
            lcb_comm_set_lcb_pwm_percent(eh->comm, value);
            break;
    }
    pthread_mutex_unlock(&eh->mutex);
}

void elliptical_handler_set_gear_ratio(elliptical_handler_t *eh, uint16_t ratio)
{
    if (eh == NULL || eh->comm == NULL) return;
    lcb_comm_set_lcb_rpm_gear_ratio(eh->comm, ratio);
}

void elliptical_handler_set_resistance_type(elliptical_handler_t *eh, uint16_t type)
{
    if (eh == NULL || eh->comm == NULL) return;
    lcb_comm_set_lcb_resistance_type(eh->comm, type);
}

void elliptical_handler_set_low_limit_resistance(elliptical_handler_t *eh, uint16_t rpm)
{
    if (eh == NULL || eh->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_resistance(eh->comm, rpm);
}

void elliptical_handler_set_low_limit_charge(elliptical_handler_t *eh, uint16_t rpm)
{
    if (eh == NULL || eh->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_charge(eh->comm, rpm);
}

void elliptical_handler_set_turn_off_lcb(elliptical_handler_t *eh, uint16_t minutes)
{
    if (eh == NULL || eh->comm == NULL) return;
    lcb_comm_set_lcb_power_off(eh->comm, minutes);
}

/* ============================================================================
 * Public: One-shot Getter
 * ============================================================================ */

void elliptical_handler_get_one_shot_rpm(elliptical_handler_t *eh)
{
    if (eh == NULL || eh->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(eh->comm);
}

/* ============================================================================
 * Public: Auto-reply Toggle
 * ============================================================================ */

void elliptical_handler_set_auto_reply_rpm(elliptical_handler_t *eh, bool enable)
{
    if (eh == NULL) return;
    pthread_mutex_lock(&eh->mutex);
    eh->is_auto_reply_rpm = enable;
    pthread_mutex_unlock(&eh->mutex);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void elliptical_handler_estop_revert_lcb_initial(elliptical_handler_t *eh)
{
    if (eh == NULL || eh->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(eh->comm);
    lcb_comm_proc_estop_unlock(eh->comm);
}
