/**
 * @file elliptical_handler_internal.h
 * @brief Internal struct definition for elliptical_handler_t
 *
 * Shared between elliptical_handler.c and elliptical_lcb_cb.c to allow both
 * files to access struct fields directly. NOT for external use.
 */

#ifndef ELLIPTICAL_HANDLER_INTERNAL_H
#define ELLIPTICAL_HANDLER_INTERNAL_H

#include "elliptical_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define ELLIPTICAL_HEARTBEAT_PERIOD_MS 400

/* Auto-reply interval counter (number of heartbeat ticks between TX).
 * 0 in the Java source (TIMES_RPM_INTERVAL), meaning it fires on every
 * heartbeat tick — kept as a named constant for fidelity even though the
 * counter comparison is degenerate at 0. */
#define ELLIPTICAL_INTERVAL_RPM 0

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct elliptical_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    elliptical_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* LCB type — drives set_resistance's command selection */
    LcbType_t machine_lcb_type;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* Single auto-reply feature */
    bool is_auto_reply_rpm; int cnt_rpm;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void ep_proc_auto_reply_tick(elliptical_handler_t *eh);

/* ============================================================================
 * Callback Bridge Registration (defined in elliptical_lcb_cb.c)
 * ============================================================================ */

void ep_lcb_cb_register(elliptical_handler_t *eh);
void ep_lcb_cb_unregister(elliptical_handler_t *eh);

#endif /* ELLIPTICAL_HANDLER_INTERNAL_H */
