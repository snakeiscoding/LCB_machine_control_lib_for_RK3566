/**
 * @file jht_ad_board_handler_internal.h
 * @brief Internal struct definition for jht_ad_board_handler_t
 *
 * Shared between jht_ad_board_handler.c and jht_ad_board_lcb_cb.c to allow
 * both files to access struct fields directly. NOT for external use.
 */

#ifndef JHT_AD_BOARD_HANDLER_INTERNAL_H
#define JHT_AD_BOARD_HANDLER_INTERNAL_H

#include "jht_ad_board_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define JHT_AD_BOARD_HEARTBEAT_PERIOD_MS      400

/* Java: TIMES_RPM_INTERVAL = 0 — fires on every heartbeat tick, kept as a
 * named constant for fidelity even though the counter comparison is
 * degenerate at 0. */
#define JHT_AD_BOARD_INTERVAL_RPM             0

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct jht_ad_board_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    jht_ad_board_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* LCB type — drives set_watt_or_resistance's command gate */
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

void jab_proc_auto_reply_tick(jht_ad_board_handler_t *h);

/* ============================================================================
 * Callback Bridge Registration (defined in jht_ad_board_lcb_cb.c)
 * ============================================================================ */

void jab_lcb_cb_register(jht_ad_board_handler_t *h);
void jab_lcb_cb_unregister(jht_ad_board_handler_t *h);

#endif /* JHT_AD_BOARD_HANDLER_INTERNAL_H */
