/**
 * @file ascent_handler.c
 * @brief LCB Ascent Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/AscentTrainer/AscentHandler.java
 */

#include "ascent_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TAG "AscentHandler"

/* ============================================================================
 * Internal Helpers — Auto-reply phase logic
 * ============================================================================ */

/* Unconditional LCB status keep-alive — in the Java original this traffic
 * comes from GeneralHandler's own heartbeat, which keeps running forever
 * alongside AscentHandler (Java never stops it). This C port hands the
 * shared comm exclusively to ascent_handler once general_handler is stopped
 * (see hal_ascent_gh_on_lcb_initial_finish in HalEntryPoint.c), so
 * ascent_handler must carry this heartbeat itself — same convention
 * treadmill_handler.c/bike_handler.c/elliptical_handler.c already follow via
 * their own auto_reply_heart_beat_status(). Without it, no RS485 traffic goes
 * out at all until the user toggles an Auto reply on. */
static void auto_reply_heart_beat_status(ascent_handler_t *ah)
{
    if (ah->comm == NULL) return;
    lcb_comm_get_lcb_status_low_priority(ah->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

static void auto_reply_rpm(ascent_handler_t *ah)
{
    if (!ah->is_auto_reply_rpm) return;
    if (ah->cnt_rpm >= ASCENT_INTERVAL_RPM) {
        ah->cnt_rpm = 0;
        if (ah->comm != NULL)
            lcb_comm_get_lcb_ecb_rpm(ah->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ah->cnt_rpm++;
    }
}

static void auto_reply_incline(ascent_handler_t *ah)
{
    if (!ah->is_auto_reply_incline) return;
    if (ah->cnt_incline >= ASCENT_INTERVAL_INCLINE) {
        ah->cnt_incline = 0;
        if (ah->comm != NULL)
            lcb_comm_get_lcb_incline_percent(ah->comm, LCB_QUEUE_MODE_REPLACE_SIMILAR);
    } else {
        ah->cnt_incline++;
    }
}

/* ============================================================================
 * Heartbeat Tick — orchestrates all auto-reply phases
 * (extracted for unit testing without spawning the heartbeat thread)
 * ============================================================================ */

void ah_proc_auto_reply_tick(ascent_handler_t *ah)
{
    if (ah == NULL) return;

    auto_reply_heart_beat_status(ah);
    auto_reply_rpm(ah);
    auto_reply_incline(ah);
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    ascent_handler_t *ah = (ascent_handler_t *)arg;

    pthread_mutex_lock(&ah->mutex);
    while (ah->is_running) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += ASCENT_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (ASCENT_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&ah->heartbeat_cond, &ah->mutex, &ts);

        if (!ah->is_running) break;

        ah_proc_auto_reply_tick(ah);
    }
    pthread_mutex_unlock(&ah->mutex);
    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

ascent_handler_t* ascent_handler_create(void)
{
    ascent_handler_t *ah = (ascent_handler_t*)calloc(1, sizeof(ascent_handler_t));
    if (ah == NULL) return NULL;

    /* Recursive mutex — same reason as treadmill_handler/bike_handler/
     * elliptical_handler: lcb_comm callbacks may be invoked synchronously
     * while heartbeat thread holds the lock. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&ah->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&ah->heartbeat_cond, NULL);

    ah->machine_lcb_type = LCB_TYPE_UNKNOWN;

    return ah;
}

void ascent_handler_destroy(ascent_handler_t *ah)
{
    if (ah == NULL) return;
    ascent_handler_stop(ah);
    pthread_cond_destroy(&ah->heartbeat_cond);
    pthread_mutex_destroy(&ah->mutex);
    free(ah);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void ascent_handler_callbacks_init(ascent_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(ascent_handler_callbacks_t));
}

void ascent_handler_set_callbacks(ascent_handler_t *ah, const ascent_handler_callbacks_t *cb)
{
    if (ah == NULL || cb == NULL) return;
    pthread_mutex_lock(&ah->mutex);
    ah->user_cb = *cb;
    pthread_mutex_unlock(&ah->mutex);
}

void ascent_handler_unregister_callbacks(ascent_handler_t *ah)
{
    if (ah == NULL) return;
    pthread_mutex_lock(&ah->mutex);
    memset(&ah->user_cb, 0, sizeof(ascent_handler_callbacks_t));
    pthread_mutex_unlock(&ah->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool ascent_handler_start(ascent_handler_t *ah, lcb_comm_t *comm)
{
    if (ah == NULL || comm == NULL) return false;

    ascent_handler_stop(ah);

    pthread_mutex_lock(&ah->mutex);
    ah->comm = comm;

    /* Wire internal LCB callbacks (overwrites any previous registration) */
    ah_lcb_cb_register(ah);
    lcb_comm_register_callbacks(comm, &ah->lcb_cb);

    ah->is_running = true;
    pthread_mutex_unlock(&ah->mutex);

    pthread_create(&ah->heartbeat_thread, NULL, heartbeat_thread_func, ah);
    LCB_LOGD(TAG, "Ascent handler started");
    return true;
}

void ascent_handler_stop(ascent_handler_t *ah)
{
    if (ah == NULL) return;

    pthread_mutex_lock(&ah->mutex);
    if (ah->is_running) {
        ah->is_running = false;
        pthread_cond_signal(&ah->heartbeat_cond);
        pthread_mutex_unlock(&ah->mutex);
        pthread_join(ah->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&ah->mutex);
    }

    if (ah->comm != NULL) {
        lcb_comm_unregister_callbacks(ah->comm);
        ah->comm = NULL;
    }
    LCB_LOGD(TAG, "Ascent handler stopped");
}

/* ============================================================================
 * Public: LCB Type Injection
 * ============================================================================ */

void ascent_handler_set_lcb_type(ascent_handler_t *ah, LcbType_t type)
{
    if (ah == NULL) return;
    pthread_mutex_lock(&ah->mutex);
    ah->machine_lcb_type = type;
    pthread_mutex_unlock(&ah->mutex);
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

/* Java: setAscentResistance() switches on the detected machine LCB type */
void ascent_handler_set_resistance(ascent_handler_t *ah, uint16_t value)
{
    if (ah == NULL || ah->comm == NULL) return;

    pthread_mutex_lock(&ah->mutex);
    switch (ah->machine_lcb_type) {
        case LCB_TYPE_CLIMBMILL_OR_A30:
            lcb_comm_set_lcb_ecb_position(ah->comm, value);
            break;
        default:
            lcb_comm_set_lcb_pwm_percent(ah->comm, value);
            break;
    }
    pthread_mutex_unlock(&ah->mutex);
}

/* Java: setAscentIncline() switches on the detected machine LCB type */
void ascent_handler_set_incline(ascent_handler_t *ah, uint16_t location)
{
    if (ah == NULL || ah->comm == NULL) return;

    pthread_mutex_lock(&ah->mutex);
    switch (ah->machine_lcb_type) {
        case LCB_TYPE_ELECTROMAGNETIC_INCLINE: /* A50 */
        case LCB_TYPE_CLIMBMILL_OR_A30:         /* A30 */
            lcb_comm_set_lcb_incline_loc(ah->comm, location);
            break;
        default:
            lcb_comm_set_lcb_incline_percent(ah->comm, location);
            break;
    }
    pthread_mutex_unlock(&ah->mutex);
}

void ascent_handler_set_gear_ratio(ascent_handler_t *ah, uint16_t ratio)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_rpm_gear_ratio(ah->comm, ratio);
}

void ascent_handler_set_resistance_type(ascent_handler_t *ah, uint16_t type)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_resistance_type(ah->comm, type);
}

void ascent_handler_set_generator_pole(ascent_handler_t *ah, uint16_t pair)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_generator_pole_pair(ah->comm, pair);
}

void ascent_handler_set_low_limit_resistance(ascent_handler_t *ah, uint16_t rpm)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_resistance(ah->comm, rpm);
}

void ascent_handler_set_low_limit_charge(ascent_handler_t *ah, uint16_t rpm)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_limit_rpm_charge(ah->comm, rpm);
}

void ascent_handler_set_tune_end_incline(ascent_handler_t *ah, uint16_t percentage)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_tune_end_incline(ah->comm, percentage);
}

void ascent_handler_set_console_power(ascent_handler_t *ah, uint16_t watts)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_cmd_console_power(ah->comm, watts);
}

void ascent_handler_set_incline_action(ascent_handler_t *ah, uint8_t action)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_incline_action(ah->comm, action);
}

void ascent_handler_auto_calibrate_incline(ascent_handler_t *ah)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_calibrate(ah->comm);
}

void ascent_handler_set_turn_off_lcb(ascent_handler_t *ah, uint16_t minutes)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_power_off(ah->comm, minutes);
}

void ascent_handler_stop_calibrate(ascent_handler_t *ah)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_incline_action_highest_priority(ah->comm, 0x00);
}

void ascent_handler_stop_incline_action(ascent_handler_t *ah)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_incline_action(ah->comm, 0x00);
}

/* ============================================================================
 * Public: One-shot Getter
 * ============================================================================ */

void ascent_handler_get_one_shot_rpm(ascent_handler_t *ah)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(ah->comm);
}

/* ============================================================================
 * Public: Auto-reply Toggles
 * ============================================================================ */

void ascent_handler_set_auto_reply_rpm(ascent_handler_t *ah, bool enable)
{
    if (ah == NULL) return;
    pthread_mutex_lock(&ah->mutex);
    ah->is_auto_reply_rpm = enable;
    pthread_mutex_unlock(&ah->mutex);
}

void ascent_handler_set_auto_reply_incline(ascent_handler_t *ah, bool enable)
{
    if (ah == NULL) return;
    pthread_mutex_lock(&ah->mutex);
    ah->is_auto_reply_incline = enable;
    pthread_mutex_unlock(&ah->mutex);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void ascent_handler_estop_revert_lcb_initial(ascent_handler_t *ah)
{
    if (ah == NULL || ah->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(ah->comm);
    lcb_comm_proc_estop_unlock(ah->comm);
}
