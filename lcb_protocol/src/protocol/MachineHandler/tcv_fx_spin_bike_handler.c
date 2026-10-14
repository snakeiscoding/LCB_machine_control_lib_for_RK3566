/**
 * @file tcv_fx_spin_bike_handler.c
 * @brief LCB TcvFxSpinBike Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/TcvFxSpinBike/TcvFxSpinBikeHandler.java
 */

#include "tcv_fx_spin_bike_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "TcvFxSpinBikeHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

/* Unconditional LCB status keep-alive — same rationale as
 * indoor_bike_handler.c/bike_handler.c/treadmill_handler.c: once
 * general_handler hands the shared comm off exclusively to this handler
 * (see hal_tcv_fx_spin_bike_gh_on_lcb_initial_finish in HalEntryPoint.c),
 * this handler must carry its own keep-alive traffic — general_handler's own
 * heartbeat, which the Java original relied on running forever in parallel,
 * has already been stopped. Without it, no RS485 traffic goes out at all
 * until the user toggles the SensorData auto-reply on. */
static void auto_reply_heart_beat_status(tcv_fx_spin_bike_handler_t *h)
{
    if (h->comm == NULL) return;
    lcb_comm_get_lcb_status_low_priority(h->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_sensor_data(tcv_fx_spin_bike_handler_t *h)
{
    if (!h->is_auto_reply_sensor_data) return;
    if (h->cnt_sensor_data >= TCV_FX_SPIN_BIKE_INTERVAL_SENSOR_DATA) {
        h->cnt_sensor_data = 0;
        if (h->comm != NULL)
            lcb_comm_get_lcb_indoor_bike_sensor_data(h->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        h->cnt_sensor_data++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void tfsb_proc_auto_reply_tick(tcv_fx_spin_bike_handler_t *h)
{
    if (h == NULL) return;

    auto_reply_heart_beat_status(h);
    auto_reply_sensor_data(h);
}

/* ============================================================================
 * Internal: SensorData parse (11-byte TCV-FX layout, big-endian)
 *
 * Layout (confirmed field-by-field from Java TcvFxSpinBikeCallbackHandlerLCB):
 *   [0]    lcb_type            (u8, must be LCB_TYPE_TCV_FX / 0x22)
 *   [1]    machine_type        (u8)
 *   [2-3]  crank_rpm           (s16)
 *   [4-5]  flywheel_rpm        (s16)
 *   [6-7]  ecb_position_sensor (s16)
 *   [8]    resistance_level    (u8)
 *   [9-10] watts               (s16, raw 0.1W units)
 * ============================================================================ */

bool tfsb_parse_sensor_data(const uint8_t *raw, tcv_fx_spin_bike_sensor_data_t *out)
{
    if (raw == NULL || out == NULL) return false;
    if (raw[0] != LCB_TYPE_TCV_FX) return false;

    out->lcb_type            = raw[0];
    out->machine_type         = raw[1];
    out->crank_rpm            = (int16_t)((raw[2] << 8) | raw[3]);
    out->flywheel_rpm         = (int16_t)((raw[4] << 8) | raw[5]);
    out->ecb_position_sensor  = (int16_t)((raw[6] << 8) | raw[7]);
    out->resistance_level     = raw[8];
    out->watts                = (int16_t)((raw[9] << 8) | raw[10]);
    return true;
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    tcv_fx_spin_bike_handler_t *h = (tcv_fx_spin_bike_handler_t *)arg;

    pthread_mutex_lock(&h->mutex);
    while (h->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += TCV_FX_SPIN_BIKE_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (TCV_FX_SPIN_BIKE_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&h->heartbeat_cond, &h->mutex, &ts);

        if (!h->is_running) break;

        tfsb_proc_auto_reply_tick(h);
    }
    pthread_mutex_unlock(&h->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

tcv_fx_spin_bike_handler_t* tcv_fx_spin_bike_handler_create(void)
{
    tcv_fx_spin_bike_handler_t *h = (tcv_fx_spin_bike_handler_t*)calloc(1, sizeof(tcv_fx_spin_bike_handler_t));
    if (h == NULL) return NULL;

    /* Recursive mutex — same reason as indoor_bike_handler/bike_handler:
     * lcb_comm callbacks may be invoked synchronously while heartbeat thread
     * holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&h->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&h->heartbeat_cond, NULL);

    return h;
}

void tcv_fx_spin_bike_handler_destroy(tcv_fx_spin_bike_handler_t *h)
{
    if (h == NULL) return;
    tcv_fx_spin_bike_handler_stop(h);
    pthread_cond_destroy(&h->heartbeat_cond);
    pthread_mutex_destroy(&h->mutex);
    free(h);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void tcv_fx_spin_bike_handler_callbacks_init(tcv_fx_spin_bike_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(tcv_fx_spin_bike_handler_callbacks_t));
}

void tcv_fx_spin_bike_handler_set_callbacks(tcv_fx_spin_bike_handler_t *h, const tcv_fx_spin_bike_handler_callbacks_t *cb)
{
    if (h == NULL || cb == NULL) return;
    pthread_mutex_lock(&h->mutex);
    h->user_cb = *cb;
    pthread_mutex_unlock(&h->mutex);
}

void tcv_fx_spin_bike_handler_unregister_callbacks(tcv_fx_spin_bike_handler_t *h)
{
    if (h == NULL) return;
    pthread_mutex_lock(&h->mutex);
    memset(&h->user_cb, 0, sizeof(tcv_fx_spin_bike_handler_callbacks_t));
    pthread_mutex_unlock(&h->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool tcv_fx_spin_bike_handler_start(tcv_fx_spin_bike_handler_t *h, lcb_comm_t *comm)
{
    if (h == NULL || comm == NULL) return false;

    tcv_fx_spin_bike_handler_stop(h);

    pthread_mutex_lock(&h->mutex);
    h->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    tfsb_lcb_cb_register(h);
    lcb_comm_register_callbacks(comm, &h->lcb_cb);

    /* Java: initialTcvFxSpinBike() resets the auto-reply flag to false */
    h->is_auto_reply_sensor_data = false;

    h->is_running = true;
    pthread_mutex_unlock(&h->mutex);

    pthread_create(&h->heartbeat_thread, NULL, heartbeat_thread_func, h);
    LCB_LOGD(TAG, "TcvFxSpinBike handler started");
    return true;
}

void tcv_fx_spin_bike_handler_stop(tcv_fx_spin_bike_handler_t *h)
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
    LCB_LOGD(TAG, "TcvFxSpinBike handler stopped");
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

void tcv_fx_spin_bike_handler_get_one_shot_sensor_data(tcv_fx_spin_bike_handler_t *h)
{
    if (h == NULL || h->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_indoor_bike_sensor_data_high_priority(h->comm);
}

/* ============================================================================
 * Public: Auto-reply Toggles
 * ============================================================================ */

void tcv_fx_spin_bike_handler_set_auto_reply_sensor_data(tcv_fx_spin_bike_handler_t *h, bool enable)
{
    if (h == NULL) return;
    pthread_mutex_lock(&h->mutex);
    h->is_auto_reply_sensor_data = enable;
    pthread_mutex_unlock(&h->mutex);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void tcv_fx_spin_bike_handler_estop_revert_lcb_initial(tcv_fx_spin_bike_handler_t *h)
{
    if (h == NULL || h->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(h->comm);
    lcb_comm_proc_estop_unlock(h->comm);
}
