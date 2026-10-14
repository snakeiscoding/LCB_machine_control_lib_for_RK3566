/**
 * @file indoor_bike_handler.c
 * @brief LCB IndoorBike Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/IndoorBike/IndoorBikeHandler.java
 */

#include "indoor_bike_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "IndoorBikeHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

/* Unconditional LCB status keep-alive — in the Java original this traffic
 * comes from GeneralHandler's own heartbeat, which keeps running forever
 * alongside IndoorBikeHandler (Java never stops it). This C port hands the
 * shared comm exclusively to indoor_bike_handler once general_handler is
 * stopped (see hal_indoor_bike_gh_on_lcb_initial_finish in HalEntryPoint.c),
 * so indoor_bike_handler must carry this heartbeat itself — same convention
 * bike_handler.c/treadmill_handler.c/elliptical_handler.c/ascent_handler.c/
 * stepper_handler.c already follow via their own
 * auto_reply_heart_beat_status(). Without it, no RS485 traffic goes out at
 * all until the user toggles an Auto reply on. */
static void auto_reply_heart_beat_status(indoor_bike_handler_t *ibh)
{
    if (ibh->comm == NULL) return;
    lcb_comm_get_lcb_status_low_priority(ibh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_rpm(indoor_bike_handler_t *ibh)
{
    if (!ibh->is_auto_reply_rpm) return;
    if (ibh->cnt_rpm >= INDOOR_BIKE_INTERVAL_RPM) {
        ibh->cnt_rpm = 0;
        if (ibh->comm != NULL)
            lcb_comm_get_lcb_ecb_rpm(ibh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ibh->cnt_rpm++;
    }
}

static void auto_reply_watts(indoor_bike_handler_t *ibh)
{
    if (!ibh->is_auto_reply_watts) return;
    if (ibh->cnt_watts >= INDOOR_BIKE_INTERVAL_WATTS) {
        ibh->cnt_watts = 0;
        if (ibh->comm != NULL)
            lcb_comm_get_lcb_watts(ibh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ibh->cnt_watts++;
    }
}

static void auto_reply_sensor_data(indoor_bike_handler_t *ibh)
{
    if (!ibh->is_auto_reply_sensor_data) return;
    if (ibh->cnt_sensor_data >= INDOOR_BIKE_INTERVAL_SENSOR_DATA) {
        ibh->cnt_sensor_data = 0;
        if (ibh->comm != NULL)
            lcb_comm_get_lcb_indoor_bike_sensor_data(ibh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ibh->cnt_sensor_data++;
    }
}

static void auto_reply_info_spin_down(indoor_bike_handler_t *ibh)
{
    if (!ibh->is_auto_reply_info_spin_down) return;
    if (ibh->cnt_info_spin_down >= INDOOR_BIKE_INTERVAL_INFO_SPINDOWN) {
        ibh->cnt_info_spin_down = 0;
        if (ibh->comm != NULL)
            lcb_comm_get_lcb_indoor_bike_info_spin_down(ibh->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ibh->cnt_info_spin_down++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void ibh_proc_auto_reply_tick(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL) return;

    auto_reply_heart_beat_status(ibh);
    auto_reply_rpm(ibh);
    auto_reply_watts(ibh);
    auto_reply_sensor_data(ibh);
    auto_reply_info_spin_down(ibh);
}

/* ============================================================================
 * Internal: isIcr()
 * ============================================================================ */

bool ibh_is_icr(const indoor_bike_handler_t *ibh)
{
    if (ibh == NULL) return false;
    return ibh->machine_lcb_type == LCB_TYPE_JIS_ATHENA;
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)arg;

    pthread_mutex_lock(&ibh->mutex);
    while (ibh->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += INDOOR_BIKE_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (INDOOR_BIKE_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&ibh->heartbeat_cond, &ibh->mutex, &ts);

        if (!ibh->is_running) break;

        ibh_proc_auto_reply_tick(ibh);
    }
    pthread_mutex_unlock(&ibh->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

indoor_bike_handler_t* indoor_bike_handler_create(void)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t*)calloc(1, sizeof(indoor_bike_handler_t));
    if (ibh == NULL) return NULL;

    /* Recursive mutex — same reason as bike_handler/treadmill_handler:
     * lcb_comm callbacks may be invoked synchronously while heartbeat thread
     * holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&ibh->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&ibh->heartbeat_cond, NULL);

    ibh->machine_lcb_type = LCB_TYPE_UNKNOWN;

    return ibh;
}

void indoor_bike_handler_destroy(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL) return;
    indoor_bike_handler_stop(ibh);
    pthread_cond_destroy(&ibh->heartbeat_cond);
    pthread_mutex_destroy(&ibh->mutex);
    free(ibh);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void indoor_bike_handler_callbacks_init(indoor_bike_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(indoor_bike_handler_callbacks_t));
}

void indoor_bike_handler_set_callbacks(indoor_bike_handler_t *ibh, const indoor_bike_handler_callbacks_t *cb)
{
    if (ibh == NULL || cb == NULL) return;
    pthread_mutex_lock(&ibh->mutex);
    ibh->user_cb = *cb;
    pthread_mutex_unlock(&ibh->mutex);
}

void indoor_bike_handler_unregister_callbacks(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL) return;
    pthread_mutex_lock(&ibh->mutex);
    memset(&ibh->user_cb, 0, sizeof(indoor_bike_handler_callbacks_t));
    pthread_mutex_unlock(&ibh->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool indoor_bike_handler_start(indoor_bike_handler_t *ibh, lcb_comm_t *comm)
{
    if (ibh == NULL || comm == NULL) return false;

    indoor_bike_handler_stop(ibh);

    pthread_mutex_lock(&ibh->mutex);
    ibh->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    ibh_lcb_cb_register(ibh);
    lcb_comm_register_callbacks(comm, &ibh->lcb_cb);

    /* Java: initialIndoorBike() resets all 4 auto-reply flags to false */
    ibh->is_auto_reply_rpm = false;
    ibh->is_auto_reply_watts = false;
    ibh->is_auto_reply_sensor_data = false;
    ibh->is_auto_reply_info_spin_down = false;

    ibh->is_running = true;
    pthread_mutex_unlock(&ibh->mutex);

    pthread_create(&ibh->heartbeat_thread, NULL, heartbeat_thread_func, ibh);
    LCB_LOGD(TAG, "IndoorBike handler started");
    return true;
}

void indoor_bike_handler_stop(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL) return;

    pthread_mutex_lock(&ibh->mutex);
    if (ibh->is_running) {
        ibh->is_running = false;
        pthread_cond_signal(&ibh->heartbeat_cond);
        pthread_mutex_unlock(&ibh->mutex);
        pthread_join(ibh->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&ibh->mutex);
    }

    if (ibh->comm != NULL) {
        lcb_comm_unregister_callbacks(ibh->comm);
        ibh->comm = NULL;
    }
    LCB_LOGD(TAG, "IndoorBike handler stopped");
}

/* ============================================================================
 * Public: LCB Type Injection
 * ============================================================================ */

void indoor_bike_handler_set_lcb_type(indoor_bike_handler_t *ibh, LcbType_t type)
{
    if (ibh == NULL) return;
    pthread_mutex_lock(&ibh->mutex);
    ibh->machine_lcb_type = type;
    pthread_mutex_unlock(&ibh->mutex);
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

void indoor_bike_handler_set_gear_ratio(indoor_bike_handler_t *ibh, uint16_t ratio)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_rpm_gear_ratio(ibh->comm, ratio);
}

void indoor_bike_handler_set_resistance_type(indoor_bike_handler_t *ibh, uint16_t type)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_resistance_type(ibh->comm, type);
}

void indoor_bike_handler_set_low_limit_resistance(indoor_bike_handler_t *ibh, uint16_t rpm)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_resistance(ibh->comm, rpm);
}

void indoor_bike_handler_set_low_limit_charge(indoor_bike_handler_t *ibh, uint16_t rpm)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_charge(ibh->comm, rpm);
}

void indoor_bike_handler_set_turn_off_lcb(indoor_bike_handler_t *ibh, uint16_t minutes)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_power_off(ibh->comm, minutes);
}

void indoor_bike_handler_get_one_shot_rpm(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(ibh->comm);
}

void indoor_bike_handler_get_one_shot_sensor_data(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_indoor_bike_sensor_data_high_priority(ibh->comm);
}

void indoor_bike_handler_set_stepper_motor_position(indoor_bike_handler_t *ibh, const uint8_t *data, uint8_t length)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_indoor_bike_stepper_motor_position(ibh->comm, data, length);
}

void indoor_bike_handler_set_calc_para_of_device(indoor_bike_handler_t *ibh, const uint8_t *data, uint8_t length)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_indoor_bike_calc_para_of_device(ibh->comm, data, length);
}

void indoor_bike_handler_get_calc_para_of_device(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_get_lcb_indoor_bike_calc_para_of_device(ibh->comm);
}

void indoor_bike_handler_set_action_spin_down(indoor_bike_handler_t *ibh, const uint8_t *data, uint8_t length)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_indoor_bike_action_spin_down(ibh->comm, data, length);
}

/* Java: getLcbIndoorBikeInfoSpinDown() sends with QueueMode.AlwaysAdd (not
 * ReplaceSimilar — distinct from the auto-reply tick's use of the same
 * underlying command). */
void indoor_bike_handler_get_info_spin_down(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_get_lcb_indoor_bike_info_spin_down(ibh->comm, LCB_QUEUE_MODE_ALWAYS_ADD);
}

void indoor_bike_handler_set_erg_mode_settings(indoor_bike_handler_t *ibh, const uint8_t *data, uint8_t length)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_indoor_bike_erg_mode_settings(ibh->comm, data, length);
}

void indoor_bike_handler_get_erg_mode_settings(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_get_lcb_indoor_bike_erg_mode_settings(ibh->comm);
}

void indoor_bike_handler_set_resistance_level(indoor_bike_handler_t *ibh, uint16_t level)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_indoor_bike_resistance_level(ibh->comm, level);
}

/* Java: setLcbIndoorBikeErgWatts() sends with QueueMode.AlwaysAdd, reusing
 * the same 0x6x watts command as bike_handler's LCBA/1X_BIKE_LCB branch. */
void indoor_bike_handler_set_erg_watts(indoor_bike_handler_t *ibh, uint16_t target_watts)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_watts(ibh->comm, target_watts, LCB_QUEUE_MODE_ALWAYS_ADD);
}

void indoor_bike_handler_request_raw_adc(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    if (!ibh_is_icr(ibh)) return;
    lcb_comm_get_lcb_dci_incline_range(ibh->comm);
}

/* ============================================================================
 * Public: Auto-reply Toggles
 * ============================================================================ */

void indoor_bike_handler_set_auto_reply_rpm(indoor_bike_handler_t *ibh, bool enable)
{
    if (ibh == NULL) return;
    pthread_mutex_lock(&ibh->mutex);
    ibh->is_auto_reply_rpm = enable;
    pthread_mutex_unlock(&ibh->mutex);
}

void indoor_bike_handler_set_auto_reply_watts(indoor_bike_handler_t *ibh, bool enable)
{
    if (ibh == NULL) return;
    pthread_mutex_lock(&ibh->mutex);
    ibh->is_auto_reply_watts = enable;
    pthread_mutex_unlock(&ibh->mutex);
}

void indoor_bike_handler_set_auto_reply_sensor_data(indoor_bike_handler_t *ibh, bool enable)
{
    if (ibh == NULL) return;
    pthread_mutex_lock(&ibh->mutex);
    ibh->is_auto_reply_sensor_data = enable;
    pthread_mutex_unlock(&ibh->mutex);
}

void indoor_bike_handler_set_auto_reply_info_spin_down(indoor_bike_handler_t *ibh, bool enable)
{
    if (ibh == NULL) return;
    pthread_mutex_lock(&ibh->mutex);
    ibh->is_auto_reply_info_spin_down = enable;
    pthread_mutex_unlock(&ibh->mutex);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void indoor_bike_handler_estop_revert_lcb_initial(indoor_bike_handler_t *ibh)
{
    if (ibh == NULL || ibh->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(ibh->comm);
    lcb_comm_proc_estop_unlock(ibh->comm);
}
