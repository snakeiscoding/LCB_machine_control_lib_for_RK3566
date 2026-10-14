/**
 * @file bike_handler.c
 * @brief LCB Bike Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/Bike/BikeHandler.java
 */

#include "bike_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "BikeHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

/* Unconditional LCB status keep-alive — in the Java original this traffic
 * comes from GeneralHandler's own heartbeat, which keeps running forever
 * alongside BikeHandler (Java never stops it). This C port hands the shared
 * comm exclusively to bike_handler once general_handler is stopped (see
 * hal_bike_gh_on_lcb_initial_finish in HalEntryPoint.c), so bike_handler must
 * carry this heartbeat itself — same convention treadmill_handler.c already
 * follows via its auto_reply_heart_beat_status(). Without it, no RS485
 * traffic goes out at all until the user toggles an Auto reply on. */
static void auto_reply_heart_beat_status(bike_handler_t *bh)
{
    if (bh->comm == NULL) return;
    lcb_comm_get_lcb_status_low_priority(bh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_rpm(bike_handler_t *bh)
{
    if (!bh->is_auto_reply_rpm) return;
    if (bh->cnt_rpm >= BIKE_INTERVAL_RPM) {
        bh->cnt_rpm = 0;
        if (bh->comm != NULL)
            lcb_comm_get_lcb_ecb_rpm(bh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        bh->cnt_rpm++;
    }
}

static void auto_resend_watt(bike_handler_t *bh)
{
    if (!bh->is_auto_resend_watt) return;
    if (bh->cnt_send_watt >= BIKE_INTERVAL_SEND_WATT) {
        bh->cnt_send_watt = 0;
        if (bh->comm != NULL)
            lcb_comm_set_lcb_watts(bh->comm, bh->resend_watt, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        bh->cnt_send_watt++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void bh_proc_auto_reply_tick(bike_handler_t *bh)
{
    if (bh == NULL) return;

    auto_reply_heart_beat_status(bh);
    auto_reply_rpm(bh);
    auto_resend_watt(bh);
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    bike_handler_t *bh = (bike_handler_t *)arg;

    pthread_mutex_lock(&bh->mutex);
    while (bh->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += BIKE_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (BIKE_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&bh->heartbeat_cond, &bh->mutex, &ts);

        if (!bh->is_running) break;

        bh_proc_auto_reply_tick(bh);
    }
    pthread_mutex_unlock(&bh->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

bike_handler_t* bike_handler_create(void)
{
    bike_handler_t *bh = (bike_handler_t*)calloc(1, sizeof(bike_handler_t));
    if (bh == NULL) return NULL;

    /* Recursive mutex — same reason as treadmill_handler/general_handler:
     * lcb_comm callbacks may be invoked synchronously while heartbeat thread
     * holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&bh->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&bh->heartbeat_cond, NULL);

    bh->machine_lcb_type = LCB_TYPE_UNKNOWN;

    return bh;
}

void bike_handler_destroy(bike_handler_t *bh)
{
    if (bh == NULL) return;
    bike_handler_stop(bh);
    pthread_cond_destroy(&bh->heartbeat_cond);
    pthread_mutex_destroy(&bh->mutex);
    free(bh);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void bike_handler_callbacks_init(bike_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(bike_handler_callbacks_t));
}

void bike_handler_set_callbacks(bike_handler_t *bh, const bike_handler_callbacks_t *cb)
{
    if (bh == NULL || cb == NULL) return;
    pthread_mutex_lock(&bh->mutex);
    bh->user_cb = *cb;
    pthread_mutex_unlock(&bh->mutex);
}

void bike_handler_unregister_callbacks(bike_handler_t *bh)
{
    if (bh == NULL) return;
    pthread_mutex_lock(&bh->mutex);
    memset(&bh->user_cb, 0, sizeof(bike_handler_callbacks_t));
    pthread_mutex_unlock(&bh->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool bike_handler_start(bike_handler_t *bh, lcb_comm_t *comm)
{
    if (bh == NULL || comm == NULL) return false;

    bike_handler_stop(bh);

    pthread_mutex_lock(&bh->mutex);
    bh->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    bh_lcb_cb_register(bh);
    lcb_comm_register_callbacks(comm, &bh->lcb_cb);

    bh->is_running = true;
    pthread_mutex_unlock(&bh->mutex);

    pthread_create(&bh->heartbeat_thread, NULL, heartbeat_thread_func, bh);
    LCB_LOGD(TAG, "Bike handler started");
    return true;
}

void bike_handler_stop(bike_handler_t *bh)
{
    if (bh == NULL) return;

    pthread_mutex_lock(&bh->mutex);
    if (bh->is_running) {
        bh->is_running = false;
        pthread_cond_signal(&bh->heartbeat_cond);
        pthread_mutex_unlock(&bh->mutex);
        pthread_join(bh->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&bh->mutex);
    }

    if (bh->comm != NULL) {
        lcb_comm_unregister_callbacks(bh->comm);
        bh->comm = NULL;
    }
    LCB_LOGD(TAG, "Bike handler stopped");
}

/* ============================================================================
 * Public: LCB Type Injection
 * ============================================================================ */

void bike_handler_set_lcb_type(bike_handler_t *bh, LcbType_t type)
{
    if (bh == NULL) return;
    pthread_mutex_lock(&bh->mutex);
    bh->machine_lcb_type = type;
    pthread_mutex_unlock(&bh->mutex);
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

/* Java: setBikeWattOrResistance() switches on the detected machine LCB type */
void bike_handler_set_watt_or_resistance(bike_handler_t *bh, uint16_t value)
{
    if (bh == NULL || bh->comm == NULL) return;

    pthread_mutex_lock(&bh->mutex);
    switch (bh->machine_lcb_type) {
        case LCB_TYPE_LCB1_CURRENT:
        case LCB_TYPE_LCB3_CURRENT:
            lcb_comm_set_lcb_pwm_percent(bh->comm, value);
            break;
        case LCB_TYPE_LCBA:
        case LCB_TYPE_1X_BIKE_LCB:
            lcb_comm_set_lcb_watts(bh->comm, value, LCB_QUEUE_MODE_REPLACE_SIMILAR);
            bh->resend_watt = value;
            break;
        case LCB_TYPE_ECB_BOARD:
            lcb_comm_set_lcb_ecb_position(bh->comm, value);
            break;
        default:
            lcb_comm_set_lcb_pwm_percent(bh->comm, value);
            break;
    }
    pthread_mutex_unlock(&bh->mutex);
}

void bike_handler_set_gear_ratio(bike_handler_t *bh, uint16_t ratio)
{
    if (bh == NULL || bh->comm == NULL) return;
    lcb_comm_set_lcb_rpm_gear_ratio(bh->comm, ratio);
}

void bike_handler_set_resistance_type(bike_handler_t *bh, uint16_t type)
{
    if (bh == NULL || bh->comm == NULL) return;
    lcb_comm_set_lcb_resistance_type(bh->comm, type);
}

void bike_handler_set_low_limit_resistance(bike_handler_t *bh, uint16_t rpm)
{
    if (bh == NULL || bh->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_resistance(bh->comm, rpm);
}

void bike_handler_set_low_limit_charge(bike_handler_t *bh, uint16_t rpm)
{
    if (bh == NULL || bh->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_charge(bh->comm, rpm);
}

void bike_handler_set_turn_off_lcb(bike_handler_t *bh, uint16_t minutes)
{
    if (bh == NULL || bh->comm == NULL) return;
    lcb_comm_set_lcb_power_off(bh->comm, minutes);
}

void bike_handler_set_generator_pole(bike_handler_t *bh, uint16_t pair)
{
    if (bh == NULL || bh->comm == NULL) return;
    lcb_comm_set_lcb_generator_pole_pair(bh->comm, pair);
}

void bike_handler_apply_frame_config(bike_handler_t *bh, const frame_config_data_t *fc)
{
    if (bh == NULL || bh->comm == NULL || fc == NULL) return;

    /* Java: LCBInfo.isChopper() == (lcbType==0x13 || lcbType==0x14); RB317
     * only supports 0x13 so this is always true for this frame, but check
     * the injected LCB type directly rather than hardcoding the branch. */
    pthread_mutex_lock(&bh->mutex);
    LcbType_t type = bh->machine_lcb_type;
    pthread_mutex_unlock(&bh->mutex);

    if (type == LCB_TYPE_LCBA || type == LCB_TYPE_1X_BIKE_LCB) {
        bike_handler_set_gear_ratio(bh, 840);
        bike_handler_set_resistance_type(bh, 0);
    } else {
        bike_handler_set_gear_ratio(bh, 1500);
        bike_handler_set_resistance_type(bh, 0x2000 | 100);
    }
    bike_handler_set_generator_pole(bh, 4);
}

/* ============================================================================
 * Public: One-shot Getter
 * ============================================================================ */

void bike_handler_get_one_shot_rpm(bike_handler_t *bh)
{
    if (bh == NULL || bh->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(bh->comm);
}

/* ============================================================================
 * Public: Auto-reply Toggles
 * ============================================================================ */

void bike_handler_set_auto_reply_rpm(bike_handler_t *bh, bool enable)
{
    if (bh == NULL) return;
    pthread_mutex_lock(&bh->mutex);
    bh->is_auto_reply_rpm = enable;
    pthread_mutex_unlock(&bh->mutex);
}

void bike_handler_set_auto_resend_watt(bike_handler_t *bh, bool enable)
{
    if (bh == NULL) return;
    pthread_mutex_lock(&bh->mutex);
    bh->is_auto_resend_watt = enable;
    pthread_mutex_unlock(&bh->mutex);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void bike_handler_estop_revert_lcb_initial(bike_handler_t *bh)
{
    if (bh == NULL || bh->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(bh->comm);
    lcb_comm_proc_estop_unlock(bh->comm);
}
