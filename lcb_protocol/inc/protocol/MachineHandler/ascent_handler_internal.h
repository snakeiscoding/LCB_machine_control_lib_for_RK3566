/**
 * @file ascent_handler_internal.h
 * @brief Internal struct definition for ascent_handler_t
 *
 * Shared between ascent_handler.c and ascent_lcb_cb.c to allow both files to
 * access struct fields directly. NOT for external use.
 */

#ifndef ASCENT_HANDLER_INTERNAL_H
#define ASCENT_HANDLER_INTERNAL_H

#include "ascent_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define ASCENT_HEARTBEAT_PERIOD_MS 400

/* Auto-reply interval counters (number of heartbeat ticks between TX).
 * Java: TIMES_RPM_INTERVAL = 0 (fires every tick), TIMES_INCLINE_INTERVAL = 1
 * (fires every other tick). */
#define ASCENT_INTERVAL_RPM     0
#define ASCENT_INTERVAL_INCLINE 1

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct ascent_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    ascent_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* LCB type — drives set_resistance's / set_incline's command selection */
    LcbType_t machine_lcb_type;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* 2 auto-reply features */
    bool is_auto_reply_rpm;     int cnt_rpm;
    bool is_auto_reply_incline; int cnt_incline;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void ah_proc_auto_reply_tick(ascent_handler_t *ah);

/* ============================================================================
 * Callback Bridge Registration (defined in ascent_lcb_cb.c)
 * ============================================================================ */

void ah_lcb_cb_register(ascent_handler_t *ah);
void ah_lcb_cb_unregister(ascent_handler_t *ah);

#endif /* ASCENT_HANDLER_INTERNAL_H */
