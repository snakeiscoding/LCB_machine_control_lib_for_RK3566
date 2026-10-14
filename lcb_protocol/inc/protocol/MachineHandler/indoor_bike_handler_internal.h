/**
 * @file indoor_bike_handler_internal.h
 * @brief Internal struct definition for indoor_bike_handler_t
 *
 * Shared between indoor_bike_handler.c and indoor_bike_lcb_cb.c to allow both
 * files to access struct fields directly. NOT for external use.
 */

#ifndef INDOOR_BIKE_HANDLER_INTERNAL_H
#define INDOOR_BIKE_HANDLER_INTERNAL_H

#include "indoor_bike_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define INDOOR_BIKE_HEARTBEAT_PERIOD_MS         400

/* Auto-reply interval counters (number of heartbeat ticks between TX).
 * RPM/Watts/SensorData are all 0 in the Java source (TIMES_RPM_INTERVAL /
 * TIMES_WATTS_INTERVAL / TIMES_SENSORDATA_INTERVAL), meaning they fire on
 * every heartbeat tick. InfoSpinDown is 1 (TIMES_INFO_SPINDOWN_INTERVAL),
 * meaning it fires every OTHER tick. Kept as named constants for fidelity
 * even where the counter comparison is degenerate at 0. */
#define INDOOR_BIKE_INTERVAL_RPM                0
#define INDOOR_BIKE_INTERVAL_WATTS              0
#define INDOOR_BIKE_INTERVAL_SENSOR_DATA        0
#define INDOOR_BIKE_INTERVAL_INFO_SPINDOWN      1

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct indoor_bike_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    indoor_bike_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* LCB type — drives request_raw_adc's isIcr() gate (LCB_TYPE_JIS_ATHENA) */
    LcbType_t machine_lcb_type;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* 4 auto-reply features */
    bool is_auto_reply_rpm;          int cnt_rpm;
    bool is_auto_reply_watts;        int cnt_watts;
    bool is_auto_reply_sensor_data;  int cnt_sensor_data;
    bool is_auto_reply_info_spin_down; int cnt_info_spin_down;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void ibh_proc_auto_reply_tick(indoor_bike_handler_t *ibh);

/* Java: isIcr() — refreshes/reads machine_lcb_type and compares to
 * LCB_TYPE_JIS_ATHENA. Exposed for testing. */
bool ibh_is_icr(const indoor_bike_handler_t *ibh);

/* ============================================================================
 * Callback Bridge Registration (defined in indoor_bike_lcb_cb.c)
 * ============================================================================ */

void ibh_lcb_cb_register(indoor_bike_handler_t *ibh);
void ibh_lcb_cb_unregister(indoor_bike_handler_t *ibh);

#endif /* INDOOR_BIKE_HANDLER_INTERNAL_H */
