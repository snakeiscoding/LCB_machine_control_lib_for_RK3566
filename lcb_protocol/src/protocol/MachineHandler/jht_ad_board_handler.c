/**
 * @file jht_ad_board_handler.c
 * @brief LCB JhtADboard Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/JhtADboard/JhtADboardHandler.java
 */

#include "jht_ad_board_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "JhtADboardHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

/* Unconditional LCB status keep-alive — same convention as bike_handler.c/
 * elliptical_handler.c/etc: once general_handler hands the shared comm over
 * exclusively to this handler (general_handler_request_stop()), nothing else
 * keeps the RS485 bus alive until the user toggles the RPM auto-reply on. */
static void auto_reply_heart_beat_status(jht_ad_board_handler_t *h)
{
    if (h->comm == NULL) return;
    lcb_comm_get_lcb_status_low_priority(h->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_rpm(jht_ad_board_handler_t *h)
{
    if (!h->is_auto_reply_rpm) return;
    if (h->cnt_rpm >= JHT_AD_BOARD_INTERVAL_RPM) {
        h->cnt_rpm = 0;
        if (h->comm != NULL)
            lcb_comm_get_lcb_ecb_rpm(h->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        h->cnt_rpm++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void jab_proc_auto_reply_tick(jht_ad_board_handler_t *h)
{
    if (h == NULL) return;

    auto_reply_heart_beat_status(h);
    auto_reply_rpm(h);
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    jht_ad_board_handler_t *h = (jht_ad_board_handler_t *)arg;

    pthread_mutex_lock(&h->mutex);
    while (h->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += JHT_AD_BOARD_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (JHT_AD_BOARD_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&h->heartbeat_cond, &h->mutex, &ts);

        if (!h->is_running) break;

        jab_proc_auto_reply_tick(h);
    }
    pthread_mutex_unlock(&h->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

jht_ad_board_handler_t* jht_ad_board_handler_create(void)
{
    jht_ad_board_handler_t *h = (jht_ad_board_handler_t*)calloc(1, sizeof(jht_ad_board_handler_t));
    if (h == NULL) return NULL;

    /* Recursive mutex — same reason as bike_handler/treadmill_handler:
     * lcb_comm callbacks may be invoked synchronously while heartbeat thread
     * holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&h->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&h->heartbeat_cond, NULL);

    h->machine_lcb_type = LCB_TYPE_UNKNOWN;

    return h;
}

void jht_ad_board_handler_destroy(jht_ad_board_handler_t *h)
{
    if (h == NULL) return;
    jht_ad_board_handler_stop(h);
    pthread_cond_destroy(&h->heartbeat_cond);
    pthread_mutex_destroy(&h->mutex);
    free(h);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void jht_ad_board_handler_callbacks_init(jht_ad_board_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(jht_ad_board_handler_callbacks_t));
}

void jht_ad_board_handler_set_callbacks(jht_ad_board_handler_t *h, const jht_ad_board_handler_callbacks_t *cb)
{
    if (h == NULL || cb == NULL) return;
    pthread_mutex_lock(&h->mutex);
    h->user_cb = *cb;
    pthread_mutex_unlock(&h->mutex);
}

void jht_ad_board_handler_unregister_callbacks(jht_ad_board_handler_t *h)
{
    if (h == NULL) return;
    pthread_mutex_lock(&h->mutex);
    memset(&h->user_cb, 0, sizeof(jht_ad_board_handler_callbacks_t));
    pthread_mutex_unlock(&h->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool jht_ad_board_handler_start(jht_ad_board_handler_t *h, lcb_comm_t *comm)
{
    if (h == NULL || comm == NULL) return false;

    jht_ad_board_handler_stop(h);

    pthread_mutex_lock(&h->mutex);
    h->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    jab_lcb_cb_register(h);
    lcb_comm_register_callbacks(comm, &h->lcb_cb);

    h->is_running = true;
    pthread_mutex_unlock(&h->mutex);

    pthread_create(&h->heartbeat_thread, NULL, heartbeat_thread_func, h);
    LCB_LOGD(TAG, "JhtADboard handler started");
    return true;
}

void jht_ad_board_handler_stop(jht_ad_board_handler_t *h)
{
    if (h == NULL) return;

    pthread_mutex_lock(&h->mutex);
    if (h->is_running) {
        h->is_running = false;
        pthread_cond_signal(&h->heartbeat_cond);
        pthread_mutex_unlock(&h->mutex);
        pthread_join(h->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&h->mutex);
    }

    if (h->comm != NULL) {
        lcb_comm_unregister_callbacks(h->comm);
        h->comm = NULL;
    }
    LCB_LOGD(TAG, "JhtADboard handler stopped");
}

/* ============================================================================
 * Public: LCB Type Injection
 * ============================================================================ */

void jht_ad_board_handler_set_lcb_type(jht_ad_board_handler_t *h, LcbType_t type)
{
    if (h == NULL) return;
    pthread_mutex_lock(&h->mutex);
    h->machine_lcb_type = type;
    pthread_mutex_unlock(&h->mutex);
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

/* Java: setJhtADboardWattOrResistance() only acts when machineLcbType ==
 * LcbType.LCB_JHT_AD_Board; every other type falls through the switch
 * statement's default and returns with no command sent. */
void jht_ad_board_handler_set_watt_or_resistance(jht_ad_board_handler_t *h, uint16_t value)
{
    if (h == NULL || h->comm == NULL) return;

    pthread_mutex_lock(&h->mutex);
    if (h->machine_lcb_type == LCB_TYPE_JHT_AD_BOARD) {
        lcb_comm_set_lcb_pwm_percent(h->comm, value);
    }
    pthread_mutex_unlock(&h->mutex);
}

void jht_ad_board_handler_set_gear_ratio(jht_ad_board_handler_t *h, uint16_t ratio)
{
    if (h == NULL || h->comm == NULL) return;
    lcb_comm_set_lcb_rpm_gear_ratio(h->comm, ratio);
}

/* ============================================================================
 * Public: One-shot Getter
 * ============================================================================ */

void jht_ad_board_handler_get_one_shot_rpm(jht_ad_board_handler_t *h)
{
    if (h == NULL || h->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(h->comm);
}

/* ============================================================================
 * Public: Auto-reply Toggle
 * ============================================================================ */

void jht_ad_board_handler_set_auto_reply_rpm(jht_ad_board_handler_t *h, bool enable)
{
    if (h == NULL) return;
    pthread_mutex_lock(&h->mutex);
    h->is_auto_reply_rpm = enable;
    pthread_mutex_unlock(&h->mutex);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void jht_ad_board_handler_estop_revert_lcb_initial(jht_ad_board_handler_t *h)
{
    if (h == NULL || h->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(h->comm);
    lcb_comm_proc_estop_unlock(h->comm);
}
