/**
 * @file en_climb_handler_internal.h
 * @brief Internal struct definition for en_climb_handler_t
 *
 * Shared between en_climb_handler.c and en_climb_lcb_cb.c to allow both files
 * to access struct fields directly. NOT for external use.
 */

#ifndef EN_CLIMB_HANDLER_INTERNAL_H
#define EN_CLIMB_HANDLER_INTERNAL_H

#include "en_climb_handler.h"
#include "lcb_comm_callback.h"
#include "lcb_device_data.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define EN_CLIMB_HEARTBEAT_PERIOD_MS   400

/* Auto-reply interval counters (number of heartbeat ticks between TX). Named
 * per Java's TIMES_*_INTERVAL constants for fidelity, even where the value is
 * 0 (degenerate — fires on every tick). */
#define EN_CLIMB_INTERVAL_SPM              0
#define EN_CLIMB_INTERVAL_BRAKE_PRESSURE   0
#define EN_CLIMB_INTERVAL_WATTS            0
#define EN_CLIMB_INTERVAL_CAL_SCALING      0
#define EN_CLIMB_INTERVAL_CZ_CAPA_IR_FREQ  1

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct en_climb_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    en_climb_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* Device-data (0x50-family) sub-callback for GetEStopVale (sub-cmd 0x04),
     * wired via lcb_cb.device_data_cb — same pattern general_handler uses for
     * its own dd_cb (see general_handler_internal.h / general_handler_lcb_cb.c). */
    lcb_device_data_callbacks_t dd_cb;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* 5 auto-reply features */
    bool is_auto_reply_spm;             int cnt_spm;
    bool is_auto_reply_brake_pressure;  int cnt_brake_pressure;
    bool is_auto_reply_watts;           int cnt_watts;
    bool is_auto_reply_cal_scaling;     int cnt_cal_scaling;
    bool is_auto_reply_cz_capa_ir_freq; int cnt_cz_capa_ir_freq;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void ech_proc_auto_reply_tick(en_climb_handler_t *ech);

/* ============================================================================
 * Callback Bridge Registration (defined in en_climb_lcb_cb.c)
 * ============================================================================ */

void ech_lcb_cb_register(en_climb_handler_t *ech);
void ech_lcb_cb_unregister(en_climb_handler_t *ech);

#endif /* EN_CLIMB_HANDLER_INTERNAL_H */
