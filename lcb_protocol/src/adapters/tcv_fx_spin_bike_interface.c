/**
 * @file tcv_fx_spin_bike_interface.c
 * @brief TcvFxSpinBike unit-conversion adapter — implementation
 *
 * Converted from: com/jht/lcbcommlib/lcbInterface/TcvFxSpinBikeInterface.java
 */

#include "tcv_fx_spin_bike_interface_internal.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>

#define TAG "TcvFxSpinBikeInterface"

/* ============================================================================
 * Internal: bridge from Layer_3 handler callbacks to this adapter's
 * user-facing callbacks. Java: TcvFxSpinBikeInterface's anonymous
 * ITcvFxSpinBikeHandlerCallback implementation.
 * ============================================================================ */

static void on_handler_sensor_data(void *ud, const tcv_fx_spin_bike_sensor_data_t *sensor_data)
{
    tcv_fx_spin_bike_interface_t *ti = (tcv_fx_spin_bike_interface_t *)ud;
    if (ti->user_cb.on_sensor_data)
        ti->user_cb.on_sensor_data(ti->user_cb.user_data, sensor_data);
}

static void on_handler_rpm(void *ud, int rpm)
{
    tcv_fx_spin_bike_interface_t *ti = (tcv_fx_spin_bike_interface_t *)ud;
    if (ti->user_cb.on_rpm)
        ti->user_cb.on_rpm(ti->user_cb.user_data, rpm);
}

static void on_handler_watts(void *ud, int watts)
{
    tcv_fx_spin_bike_interface_t *ti = (tcv_fx_spin_bike_interface_t *)ud;
    /* Java: "LCB delivers watts in 0.1W units; convert to W before surfacing." */
    if (ti->user_cb.on_watts)
        ti->user_cb.on_watts(ti->user_cb.user_data, watts / 10);
}

static void on_handler_resistance_level(void *ud, int resistance_level)
{
    tcv_fx_spin_bike_interface_t *ti = (tcv_fx_spin_bike_interface_t *)ud;
    if (ti->user_cb.on_resistance_level)
        ti->user_cb.on_resistance_level(ti->user_cb.user_data, resistance_level);
}

void tfsbi_build_handler_callbacks(tcv_fx_spin_bike_interface_t *ti, tcv_fx_spin_bike_handler_callbacks_t *cb)
{
    tcv_fx_spin_bike_handler_callbacks_init(cb);
    cb->user_data = ti;
    cb->on_sensor_data = on_handler_sensor_data;
    cb->on_rpm = on_handler_rpm;
    cb->on_watts = on_handler_watts;
    cb->on_resistance_level = on_handler_resistance_level;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

tcv_fx_spin_bike_interface_t* tcv_fx_spin_bike_interface_create(void)
{
    tcv_fx_spin_bike_interface_t *ti = (tcv_fx_spin_bike_interface_t*)calloc(1, sizeof(tcv_fx_spin_bike_interface_t));
    if (ti == NULL) return NULL;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&ti->mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    return ti;
}

void tcv_fx_spin_bike_interface_destroy(tcv_fx_spin_bike_interface_t *ti)
{
    if (ti == NULL) return;
    tcv_fx_spin_bike_interface_stop(ti);
    pthread_mutex_destroy(&ti->mutex);
    free(ti);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void tcv_fx_spin_bike_interface_callbacks_init(tcv_fx_spin_bike_interface_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(tcv_fx_spin_bike_interface_callbacks_t));
}

void tcv_fx_spin_bike_interface_set_callbacks(tcv_fx_spin_bike_interface_t *ti,
                                              const tcv_fx_spin_bike_interface_callbacks_t *cb)
{
    if (ti == NULL || cb == NULL) return;
    pthread_mutex_lock(&ti->mutex);
    ti->user_cb = *cb;
    pthread_mutex_unlock(&ti->mutex);
}

void tcv_fx_spin_bike_interface_unregister_callbacks(tcv_fx_spin_bike_interface_t *ti)
{
    if (ti == NULL) return;
    pthread_mutex_lock(&ti->mutex);
    memset(&ti->user_cb, 0, sizeof(tcv_fx_spin_bike_interface_callbacks_t));
    pthread_mutex_unlock(&ti->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool tcv_fx_spin_bike_interface_start(tcv_fx_spin_bike_interface_t *ti, lcb_comm_t *comm)
{
    if (ti == NULL || comm == NULL) return false;

    tcv_fx_spin_bike_interface_stop(ti);

    ti->handler = tcv_fx_spin_bike_handler_create();
    if (ti->handler == NULL) {
        LCB_LOGE(TAG, "Failed to create internal tcv_fx_spin_bike_handler");
        return false;
    }

    tcv_fx_spin_bike_handler_callbacks_t handler_cb;
    tfsbi_build_handler_callbacks(ti, &handler_cb);
    tcv_fx_spin_bike_handler_set_callbacks(ti->handler, &handler_cb);

    if (!tcv_fx_spin_bike_handler_start(ti->handler, comm)) {
        LCB_LOGE(TAG, "Failed to start internal tcv_fx_spin_bike_handler");
        tcv_fx_spin_bike_handler_destroy(ti->handler);
        ti->handler = NULL;
        return false;
    }

    LCB_LOGD(TAG, "TcvFxSpinBike interface started");
    return true;
}

void tcv_fx_spin_bike_interface_stop(tcv_fx_spin_bike_interface_t *ti)
{
    if (ti == NULL) return;
    if (ti->handler != NULL) {
        tcv_fx_spin_bike_handler_stop(ti->handler);
        tcv_fx_spin_bike_handler_destroy(ti->handler);
        ti->handler = NULL;
    }
    LCB_LOGD(TAG, "TcvFxSpinBike interface stopped");
}

/* ============================================================================
 * Public: TX Commands
 * ============================================================================ */

void tcv_fx_spin_bike_interface_get_one_shot_sensor_data(tcv_fx_spin_bike_interface_t *ti)
{
    if (ti == NULL || ti->handler == NULL) return;
    tcv_fx_spin_bike_handler_get_one_shot_sensor_data(ti->handler);
}

/* ============================================================================
 * Public: Auto-reply Toggle
 * ============================================================================ */

void tcv_fx_spin_bike_interface_start_reply_data(tcv_fx_spin_bike_interface_t *ti, bool start)
{
    if (ti == NULL || ti->handler == NULL) return;
    tcv_fx_spin_bike_handler_set_auto_reply_sensor_data(ti->handler, start);
}

/* ============================================================================
 * Public: E-Stop Revert
 * ============================================================================ */

void tcv_fx_spin_bike_interface_estop_revert_lcb_initial(tcv_fx_spin_bike_interface_t *ti)
{
    if (ti == NULL || ti->handler == NULL) return;
    tcv_fx_spin_bike_handler_estop_revert_lcb_initial(ti->handler);
}
