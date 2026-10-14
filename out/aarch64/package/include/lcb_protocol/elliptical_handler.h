/**
 * @file elliptical_handler.h
 * @brief LCB Elliptical Handler — Elliptical machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/Elliptical/IEllipticalHandler.java
 *   - com/jht/lcbcommlib/machineHandler/Elliptical/IEllipticalHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/Elliptical/EllipticalHandler.java
 *   - com/jht/lcbcommlib/machineHandler/Elliptical/EllipticalCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent),
 * same as bike_handler. Manages:
 *   - Heartbeat thread @ 400ms driving auto-reply polling
 *   - 1 auto-reply feature: RPM read-back
 *   - LCB-type-dependent resistance command selection
 *
 * com/jht/lcbcommlib/lcbInterface/EllipticalInterface.java is a confirmed
 * pure stub (no business logic) — there is no Layer_2 adapter for Elliptical.
 */

#ifndef ELLIPTICAL_HANDLER_H
#define ELLIPTICAL_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"
#include "lcb_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct elliptical_handler elliptical_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IEllipticalHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_elliptical_rpm)(void *ud, uint16_t rpm);
    void (*on_elliptical_one_shot_rpm)(void *ud, uint16_t rpm);
} elliptical_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

elliptical_handler_t* elliptical_handler_create(void);
void                  elliptical_handler_destroy(elliptical_handler_t *eh);

void elliptical_handler_callbacks_init(elliptical_handler_callbacks_t *cb);
void elliptical_handler_set_callbacks(elliptical_handler_t *eh, const elliptical_handler_callbacks_t *cb);
void elliptical_handler_unregister_callbacks(elliptical_handler_t *eh);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the elliptical handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration) and
 * starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param eh    Elliptical handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool elliptical_handler_start(elliptical_handler_t *eh, lcb_comm_t *comm);

void elliptical_handler_stop(elliptical_handler_t *eh);

/* ============================================================================
 * LCB Type Injection (for setEllipticalResistance's machine-type branch)
 * ============================================================================ */

void elliptical_handler_set_lcb_type(elliptical_handler_t *eh, LcbType_t type);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

void elliptical_handler_set_resistance(elliptical_handler_t *eh, uint16_t value);
void elliptical_handler_set_gear_ratio(elliptical_handler_t *eh, uint16_t ratio);
void elliptical_handler_set_resistance_type(elliptical_handler_t *eh, uint16_t type);
void elliptical_handler_set_low_limit_resistance(elliptical_handler_t *eh, uint16_t rpm);
void elliptical_handler_set_low_limit_charge(elliptical_handler_t *eh, uint16_t rpm);
void elliptical_handler_set_turn_off_lcb(elliptical_handler_t *eh, uint16_t minutes);

/* ============================================================================
 * One-shot Getter
 * ============================================================================ */

void elliptical_handler_get_one_shot_rpm(elliptical_handler_t *eh);

/* ============================================================================
 * Auto-reply Toggle
 * ============================================================================ */

void elliptical_handler_set_auto_reply_rpm(elliptical_handler_t *eh, bool enable);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to elliptical_handler; see the matching
 * doc comment in bike_handler.h for the full explanation). Re-sends the same
 * single SetLCBInitial command general_handler's MACHINE_CATEGORY_BIKE
 * (default) branch uses — LCB_TYPE_ECB_BOARD is not in
 * map_lcb_type_to_category()'s TM list, so it falls to the default/BIKE
 * single-command category — highest-priority since the LCB is already
 * known-alive, then calls lcb_comm_proc_estop_unlock() for the local UI
 * notification. No-op if not started (comm == NULL).
 */
void elliptical_handler_estop_revert_lcb_initial(elliptical_handler_t *eh);

#ifdef __cplusplus
}
#endif

#endif /* ELLIPTICAL_HANDLER_H */
