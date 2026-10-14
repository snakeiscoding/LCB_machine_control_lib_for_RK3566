/**
 * @file stepper_handler_internal.h
 * @brief Internal struct definition for stepper_handler_t
 *
 * Shared between stepper_handler.c and stepper_lcb_cb.c to allow both files
 * to access struct fields directly. NOT for external use.
 */

#ifndef STEPPER_HANDLER_INTERNAL_H
#define STEPPER_HANDLER_INTERNAL_H

#include "stepper_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define STEPPER_HEARTBEAT_PERIOD_MS   400

/* Auto-reply interval counter (number of heartbeat ticks between TX). 0 in
 * the Java source (TIMES_RPM_INTERVAL), meaning it fires on every heartbeat
 * tick — kept as a named constant for fidelity even though the counter
 * comparison is degenerate at 0. */
#define STEPPER_INTERVAL_RPM          0

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct stepper_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    stepper_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* 1 auto-reply feature */
    bool is_auto_reply_rpm;   int cnt_rpm;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void sh_proc_auto_reply_tick(stepper_handler_t *sh);

/* ============================================================================
 * Callback Bridge Registration (defined in stepper_lcb_cb.c)
 * ============================================================================ */

void sh_lcb_cb_register(stepper_handler_t *sh);
void sh_lcb_cb_unregister(stepper_handler_t *sh);

#endif /* STEPPER_HANDLER_INTERNAL_H */
