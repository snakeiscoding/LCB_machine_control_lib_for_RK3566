/**
 * @file tcv_fx_spin_bike_handler_internal.h
 * @brief Internal struct definition for tcv_fx_spin_bike_handler_t
 *
 * Shared between tcv_fx_spin_bike_handler.c and tcv_fx_spin_bike_lcb_cb.c to
 * allow both files to access struct fields directly. NOT for external use.
 */

#ifndef TCV_FX_SPIN_BIKE_HANDLER_INTERNAL_H
#define TCV_FX_SPIN_BIKE_HANDLER_INTERNAL_H

#include "tcv_fx_spin_bike_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define TCV_FX_SPIN_BIKE_HEARTBEAT_PERIOD_MS 400

/* Java: TIMES_SENSORDATA_INTERVAL = 0 -> fires every heartbeat tick */
#define TCV_FX_SPIN_BIKE_INTERVAL_SENSOR_DATA 0

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct tcv_fx_spin_bike_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    tcv_fx_spin_bike_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* Single auto-reply feature */
    bool is_auto_reply_sensor_data;  int cnt_sensor_data;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void tfsb_proc_auto_reply_tick(tcv_fx_spin_bike_handler_t *h);

/* Decodes an 11-byte TCV-FX SensorData payload into *out. Returns false
 * (and leaves *out untouched) if raw is NULL or raw[0] != LCB_TYPE_TCV_FX —
 * i.e. the frame belongs to IndoorBike, not this handler. Exposed for
 * testing. */
bool tfsb_parse_sensor_data(const uint8_t *raw, tcv_fx_spin_bike_sensor_data_t *out);

/* ============================================================================
 * Callback Bridge Registration (defined in tcv_fx_spin_bike_lcb_cb.c)
 * ============================================================================ */

void tfsb_lcb_cb_register(tcv_fx_spin_bike_handler_t *h);
void tfsb_lcb_cb_unregister(tcv_fx_spin_bike_handler_t *h);

#endif /* TCV_FX_SPIN_BIKE_HANDLER_INTERNAL_H */
