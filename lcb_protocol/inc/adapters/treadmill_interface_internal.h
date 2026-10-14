/**
 * @file treadmill_interface_internal.h
 * @brief Internal struct definition for treadmill_interface_t
 *
 * Shared with the test suite so it can drive the ext-status/safety-key bridge
 * callback directly and inspect derived state. NOT for external use.
 */

#ifndef TREADMILL_INTERFACE_INTERNAL_H
#define TREADMILL_INTERFACE_INTERNAL_H

#include "treadmill_interface.h"
#include "treadmill_handler.h"
#include <pthread.h>

/* Java: "update LCB safetykey state every 400ms*15" — counts ext-status
 * callback invocations, not raw heartbeat ticks. */
#define TI_SAFETY_REANNOUNCE_INTERVAL 15

/* LCB error-status loop (Java GeneralHandler: checkInGetErrorState /
 * runGetErrorProcesses / handleExceptionErrorCode). general_handler is retired
 * once this adapter owns lcb_comm, so its B6 -> 0x72 GetLCBErrorCode -> 0x75
 * SetSkipCurrError loop runs here; without it 0x02B2 (E-Stop) is never read on
 * models that have no 0x97 ext status (e.g. TM538). Same counts as
 * GH_CHECK_GET/SKIP_ERROR_STATE_TIMES (counted per B6 status seen). */
#define TI_CHECK_GET_ERROR_STATE_TIMES  4
#define TI_CHECK_SKIP_ERROR_STATE_TIMES 3

typedef enum {
    TI_ERROR_NONE = 0,
    TI_ERROR_GET,
    TI_ERROR_SKIP
} ti_error_state_t;

struct treadmill_interface {
    /* Owned — created in _start(), destroyed in _stop() */
    treadmill_handler_t *handler;

    /* NOT owned — set in _start(), cleared in _stop(); used by the LCB error loop */
    lcb_comm_t *comm;

    /* LCB error-status loop state */
    ti_error_state_t error_state;
    int  check_get_error_counter;
    int  check_skip_error_counter;
    bool run_proc_estop;       /* Java: runProcEStopLcb — highest-priority stop sent once per E-Stop */
    bool lcb_estop_error;      /* 0x02B2 latched until 0x02B1 (unlock) / 0x024F */
    bool ignore_stale_estop;   /* after E-Stop revert: drop 0x02B2 until a fresh 0x72 is requested */

    /* User-facing callbacks */
    treadmill_interface_callbacks_t user_cb;

    pthread_mutex_t mutex;

    LcbType_t machine_lcb_type;

    /* Ext-status derived safety state (Java: TreadMillInterface fields) */
    int  old_status;
    int  update_safety_st_count;
    bool safetykey_down;
    bool safetykey_lock;
    bool zero_rpm;
    bool auto_oc_detect;
    bool old_oc_detect_status;

    /* Incline location observed from on_treadmill_incline_loc (Java: currentIncline) */
    uint16_t current_incline;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

/* Java: onExtStatusParser + onExtStatusHnadler + onAutoOCExtStatusHnadler,
 * driven directly by tests without needing a real treadmill_handler/comm. */
void ti_proc_ext_status(treadmill_interface_t *ti, uint16_t status);

/* LCB status byte bit 6 (LCB error) seen: enter GET and request 0x72. */
void ti_proc_lcb_error_status(treadmill_interface_t *ti, bool has_error);

/* Error code from 0x72 (or an injected one). Returns false when the code is
 * swallowed (stale 0x02B2 right after an E-Stop revert) and must not be
 * forwarded to on_error_code. */
bool ti_proc_error_code(treadmill_interface_t *ti, uint16_t error_code);

/* Fills `cb` with the same bridge callbacks treadmill_interface_start() wires
 * onto its internal treadmill_handler — exposed so tests can wire
 * ti->handler's callbacks (via treadmill_handler_set_callbacks) and drive
 * them through th->user_cb.on_treadmill_*(...) without spawning the
 * heartbeat thread that a real _start() would require. */
void ti_build_handler_callbacks(treadmill_interface_t *ti, treadmill_handler_callbacks_t *cb);

#endif /* TREADMILL_INTERFACE_INTERNAL_H */
