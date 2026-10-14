/**
 * @file bike_handler_internal.h
 * @brief Internal struct definition for bike_handler_t
 *
 * Shared between bike_handler.c and bike_lcb_cb.c to allow both files to
 * access struct fields directly. NOT for external use.
 */

#ifndef BIKE_HANDLER_INTERNAL_H
#define BIKE_HANDLER_INTERNAL_H

#include "bike_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define BIKE_HEARTBEAT_PERIOD_MS      400

/* Auto-reply interval counters (number of heartbeat ticks between TX).
 * Both are 0 in the Java source (TIMES_RPM_INTERVAL / TIMES_SEND_WATT_INTERVAL),
 * meaning they fire on every heartbeat tick — kept as named constants for
 * fidelity even though the counter comparison is degenerate at 0. */
#define BIKE_INTERVAL_RPM             0
#define BIKE_INTERVAL_SEND_WATT       0

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct bike_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    bike_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* LCB type — drives set_watt_or_resistance's command selection */
    LcbType_t machine_lcb_type;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* 2 auto-reply features */
    bool is_auto_reply_rpm;      int cnt_rpm;
    bool is_auto_resend_watt;    int cnt_send_watt;

    /* Last watt/resistance value sent, cached for resend (Java: mResendWatt) */
    uint16_t resend_watt;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void bh_proc_auto_reply_tick(bike_handler_t *bh);

/* ============================================================================
 * Callback Bridge Registration (defined in bike_lcb_cb.c)
 * ============================================================================ */

void bh_lcb_cb_register(bike_handler_t *bh);
void bh_lcb_cb_unregister(bike_handler_t *bh);

#endif /* BIKE_HANDLER_INTERNAL_H */
