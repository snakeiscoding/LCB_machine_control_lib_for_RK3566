/**
 * @file tcv_fx_spin_bike_interface_internal.h
 * @brief Internal struct definition for tcv_fx_spin_bike_interface_t
 *
 * Shared with the test suite so it can wire the internal handler's callbacks
 * directly without spawning the heartbeat thread a real _start() requires.
 * NOT for external use.
 */

#ifndef TCV_FX_SPIN_BIKE_INTERFACE_INTERNAL_H
#define TCV_FX_SPIN_BIKE_INTERFACE_INTERNAL_H

#include "tcv_fx_spin_bike_interface.h"
#include "tcv_fx_spin_bike_handler.h"
#include <pthread.h>

struct tcv_fx_spin_bike_interface {
    /* Owned — created in _start(), destroyed in _stop() */
    tcv_fx_spin_bike_handler_t *handler;

    /* User-facing callbacks */
    tcv_fx_spin_bike_interface_callbacks_t user_cb;

    pthread_mutex_t mutex;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

/* Fills `cb` with the same bridge callbacks tcv_fx_spin_bike_interface_start()
 * wires onto its internal handler — exposed so tests can wire ti->handler's
 * callbacks and drive them through h->user_cb.on_*(...) without spawning the
 * heartbeat thread that a real _start() would require. */
void tfsbi_build_handler_callbacks(tcv_fx_spin_bike_interface_t *ti, tcv_fx_spin_bike_handler_callbacks_t *cb);

#endif /* TCV_FX_SPIN_BIKE_INTERFACE_INTERNAL_H */
