/**
 * @file en_climb_handler.c
 * @brief LCB EnClimb Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/EnClimbmill/EnClimbHandler.java
 */

#include "en_climb_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_device_data.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "EnClimbHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

/* Unconditional Climbmill status keep-alive — Java's autoReplyHeartBeatEnClimbStatus()
 * calls mLcb.getLcbClimbmillStatus() every tick regardless of any auto-reply
 * toggle, same shape as Stepper's own always-on status poll. */
static void auto_reply_heart_beat_status(en_climb_handler_t *ech)
{
    if (ech->comm == NULL) return;
    lcb_comm_get_lcb_climbmill_status(ech->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_spm(en_climb_handler_t *ech)
{
    if (!ech->is_auto_reply_spm) return;
    if (ech->cnt_spm >= EN_CLIMB_INTERVAL_SPM) {
        ech->cnt_spm = 0;
        if (ech->comm != NULL)
            lcb_comm_get_lcb_spm(ech->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ech->cnt_spm++;
    }
}

static void auto_reply_brake_pressure(en_climb_handler_t *ech)
{
    if (!ech->is_auto_reply_brake_pressure) return;
    if (ech->cnt_brake_pressure >= EN_CLIMB_INTERVAL_BRAKE_PRESSURE) {
        ech->cnt_brake_pressure = 0;
        if (ech->comm != NULL)
            lcb_comm_get_lcb_brake_pressure_value(ech->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ech->cnt_brake_pressure++;
    }
}

static void auto_reply_watts(en_climb_handler_t *ech)
{
    if (!ech->is_auto_reply_watts) return;
    if (ech->cnt_watts >= EN_CLIMB_INTERVAL_WATTS) {
        ech->cnt_watts = 0;
        if (ech->comm != NULL)
            lcb_comm_get_lcb_watts(ech->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ech->cnt_watts++;
    }
}

/* Java's autoReplyCalScaling() has a copy-paste bug: it gates on
 * cntIntervalTimesWatts instead of cntIntervalTimesCalScaling. This C port
 * uses its own independent cnt_cal_scaling counter instead — replicating the
 * bug would make CalScaling auto-reply silently piggyback on the Watts
 * counter's cadence, which is not the intended behavior. */
static void auto_reply_cal_scaling(en_climb_handler_t *ech)
{
    if (!ech->is_auto_reply_cal_scaling) return;
    if (ech->cnt_cal_scaling >= EN_CLIMB_INTERVAL_CAL_SCALING) {
        ech->cnt_cal_scaling = 0;
        if (ech->comm != NULL)
            lcb_comm_get_lcb_cal_scaling(ech->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ech->cnt_cal_scaling++;
    }
}

/* Java: autoReplyCZcapaIRfreq() -> mDeviceLcb.funDeviceGetEStopVale() (device
 * data sub-command 0x04, not a plain lcb_comm getter). */
static void auto_reply_cz_capa_ir_freq(en_climb_handler_t *ech)
{
    if (!ech->is_auto_reply_cz_capa_ir_freq) return;
    if (ech->cnt_cz_capa_ir_freq >= EN_CLIMB_INTERVAL_CZ_CAPA_IR_FREQ) {
        ech->cnt_cz_capa_ir_freq = 0;
        if (ech->comm != NULL)
            lcb_dd_get_estop_vale(ech->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ech->cnt_cz_capa_ir_freq++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void ech_proc_auto_reply_tick(en_climb_handler_t *ech)
{
    if (ech == NULL) return;

    auto_reply_heart_beat_status(ech);
    auto_reply_spm(ech);
    auto_reply_brake_pressure(ech);
    auto_reply_watts(ech);
    auto_reply_cal_scaling(ech);
    auto_reply_cz_capa_ir_freq(ech);
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)arg;

    pthread_mutex_lock(&ech->mutex);
    while (ech->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += EN_CLIMB_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (EN_CLIMB_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&ech->heartbeat_cond, &ech->mutex, &ts);

        if (!ech->is_running) break;

        ech_proc_auto_reply_tick(ech);
    }
    pthread_mutex_unlock(&ech->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

en_climb_handler_t* en_climb_handler_create(void)
{
    en_climb_handler_t *ech = (en_climb_handler_t*)calloc(1, sizeof(en_climb_handler_t));
    if (ech == NULL) return NULL;

    /* Recursive mutex — same reason as treadmill_handler/bike_handler/
     * stepper_handler: lcb_comm callbacks may be invoked synchronously while
     * heartbeat thread holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&ech->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&ech->heartbeat_cond, NULL);

    return ech;
}

void en_climb_handler_destroy(en_climb_handler_t *ech)
{
    if (ech == NULL) return;
    en_climb_handler_stop(ech);
    pthread_cond_destroy(&ech->heartbeat_cond);
    pthread_mutex_destroy(&ech->mutex);
    free(ech);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void en_climb_handler_callbacks_init(en_climb_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(en_climb_handler_callbacks_t));
}

void en_climb_handler_set_callbacks(en_climb_handler_t *ech, const en_climb_handler_callbacks_t *cb)
{
    if (ech == NULL || cb == NULL) return;
    pthread_mutex_lock(&ech->mutex);
    ech->user_cb = *cb;
    pthread_mutex_unlock(&ech->mutex);
}

void en_climb_handler_unregister_callbacks(en_climb_handler_t *ech)
{
    if (ech == NULL) return;
    pthread_mutex_lock(&ech->mutex);
    memset(&ech->user_cb, 0, sizeof(en_climb_handler_callbacks_t));
    pthread_mutex_unlock(&ech->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool en_climb_handler_start(en_climb_handler_t *ech, lcb_comm_t *comm)
{
    if (ech == NULL || comm == NULL) return false;

    en_climb_handler_stop(ech);

    pthread_mutex_lock(&ech->mutex);
    ech->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    ech_lcb_cb_register(ech);
    lcb_comm_register_callbacks(comm, &ech->lcb_cb);

    ech->is_running = true;
    pthread_mutex_unlock(&ech->mutex);

    pthread_create(&ech->heartbeat_thread, NULL, heartbeat_thread_func, ech);
    LCB_LOGD(TAG, "EnClimb handler started");
    return true;
}

void en_climb_handler_stop(en_climb_handler_t *ech)
{
    if (ech == NULL) return;

    pthread_mutex_lock(&ech->mutex);
    if (ech->is_running) {
        ech->is_running = false;
        pthread_cond_signal(&ech->heartbeat_cond);
        pthread_mutex_unlock(&ech->mutex);
        pthread_join(ech->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&ech->mutex);
    }

    if (ech->comm != NULL) {
        lcb_comm_unregister_callbacks(ech->comm);
        ech->comm = NULL;
    }
    LCB_LOGD(TAG, "EnClimb handler stopped");
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

void en_climb_handler_set_spm(en_climb_handler_t *ech, uint16_t spm)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_spm(ech->comm, spm);
}

void en_climb_handler_set_resistance(en_climb_handler_t *ech, uint16_t value)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_resistance(ech->comm, value);
}

void en_climb_handler_unlock(en_climb_handler_t *ech)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_climbmill_status(ech->comm, 0x00);
}

void en_climb_handler_set_gear_ratio(en_climb_handler_t *ech, uint16_t ratio)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_rpm_gear_ratio(ech->comm, ratio);
}

void en_climb_handler_set_resistance_type(en_climb_handler_t *ech, uint16_t ohm)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_resistance_type(ech->comm, ohm);
}

void en_climb_handler_set_generator_pole(en_climb_handler_t *ech, uint16_t pair)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_generator_pole_pair(ech->comm, pair);
}

void en_climb_handler_set_estop_parameter(en_climb_handler_t *ech, uint8_t value)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_e_stop_parameter(ech->comm, value);
}

void en_climb_handler_set_climbmill_command(en_climb_handler_t *ech, uint8_t value)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_climbmill_status(ech->comm, value);
}

void en_climb_handler_set_climbmill_cal(en_climb_handler_t *ech, const uint8_t *data, uint8_t length)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_climbmill_cal(ech->comm, data, length);
}

void en_climb_handler_set_cal_scaling(en_climb_handler_t *ech, uint16_t cal_scaling)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_cal_scaling(ech->comm, cal_scaling);
}

void en_climb_handler_set_frame_cal_scaling(en_climb_handler_t *ech, uint16_t frame_cal_scaling)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_frame_cal_scaling(ech->comm, frame_cal_scaling);
}

/* ============================================================================
 * Public: One-shot Getters
 * ============================================================================ */

void en_climb_handler_get_cal_scaling(en_climb_handler_t *ech)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_get_lcb_cal_scaling(ech->comm, LCB_QUEUE_MODE_ALWAYS_ADD);
}

void en_climb_handler_get_frame_cal_scaling(en_climb_handler_t *ech)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_get_lcb_frame_cal_scaling(ech->comm);
}

void en_climb_handler_get_one_shot_spm(en_climb_handler_t *ech)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_spm_high_priority(ech->comm);
}

/* ============================================================================
 * Public: Auto-reply Toggles
 * ============================================================================ */

void en_climb_handler_set_auto_reply_spm(en_climb_handler_t *ech, bool enable)
{
    if (ech == NULL) return;
    pthread_mutex_lock(&ech->mutex);
    ech->is_auto_reply_spm = enable;
    pthread_mutex_unlock(&ech->mutex);
}

void en_climb_handler_set_auto_reply_brake_pressure(en_climb_handler_t *ech, bool enable)
{
    if (ech == NULL) return;
    pthread_mutex_lock(&ech->mutex);
    ech->is_auto_reply_brake_pressure = enable;
    pthread_mutex_unlock(&ech->mutex);
}

void en_climb_handler_set_auto_reply_watts(en_climb_handler_t *ech, bool enable)
{
    if (ech == NULL) return;
    pthread_mutex_lock(&ech->mutex);
    ech->is_auto_reply_watts = enable;
    pthread_mutex_unlock(&ech->mutex);
}

void en_climb_handler_set_auto_reply_cal_scaling(en_climb_handler_t *ech, bool enable)
{
    if (ech == NULL) return;
    pthread_mutex_lock(&ech->mutex);
    ech->is_auto_reply_cal_scaling = enable;
    pthread_mutex_unlock(&ech->mutex);
}

void en_climb_handler_set_auto_reply_cz_capa_ir_freq(en_climb_handler_t *ech, bool enable)
{
    if (ech == NULL) return;
    pthread_mutex_lock(&ech->mutex);
    ech->is_auto_reply_cz_capa_ir_freq = enable;
    pthread_mutex_unlock(&ech->mutex);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void en_climb_handler_estop_revert_lcb_initial(en_climb_handler_t *ech)
{
    if (ech == NULL || ech->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(ech->comm);
    lcb_comm_proc_estop_unlock(ech->comm);
}
