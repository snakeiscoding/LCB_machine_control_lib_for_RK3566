/**
 * @file ascent_handler.h
 * @brief LCB Ascent Handler — Ascent Trainer machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/AscentTrainer/IAscentHandler.java
 *   - com/jht/lcbcommlib/machineHandler/AscentTrainer/IAscentHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/AscentTrainer/AscentHandler.java
 *   - com/jht/lcbcommlib/machineHandler/AscentTrainer/AscentCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent),
 * same as bike_handler/elliptical_handler. Manages:
 *   - Heartbeat thread @ 400ms driving auto-reply polling
 *   - 2 auto-reply features: RPM read-back, Incline read-back
 *   - LCB-type-dependent resistance and incline command selection
 *
 * com/jht/lcbcommlib/lcbInterface/AscentInterface.java is a confirmed pure
 * stub (no business logic) — there is no Layer_2 adapter for Ascent.
 */

#ifndef ASCENT_HANDLER_H
#define ASCENT_HANDLER_H

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

typedef struct ascent_handler ascent_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IAscentHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_ascent_rpm)(void *ud, uint16_t rpm);
    void (*on_ascent_one_shot_rpm)(void *ud, uint16_t rpm);
    void (*on_ascent_incline)(void *ud, uint16_t incline);
} ascent_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

ascent_handler_t* ascent_handler_create(void);
void              ascent_handler_destroy(ascent_handler_t *ah);

void ascent_handler_callbacks_init(ascent_handler_callbacks_t *cb);
void ascent_handler_set_callbacks(ascent_handler_t *ah, const ascent_handler_callbacks_t *cb);
void ascent_handler_unregister_callbacks(ascent_handler_t *ah);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the ascent handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration) and
 * starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param ah    Ascent handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool ascent_handler_start(ascent_handler_t *ah, lcb_comm_t *comm);

void ascent_handler_stop(ascent_handler_t *ah);

/* ============================================================================
 * LCB Type Injection (for setAscentResistance's / setAscentIncline's
 * machine-type branches)
 * ============================================================================ */

void ascent_handler_set_lcb_type(ascent_handler_t *ah, LcbType_t type);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

void ascent_handler_set_resistance(ascent_handler_t *ah, uint16_t value);
void ascent_handler_set_incline(ascent_handler_t *ah, uint16_t location);
void ascent_handler_set_gear_ratio(ascent_handler_t *ah, uint16_t ratio);
void ascent_handler_set_resistance_type(ascent_handler_t *ah, uint16_t type);
void ascent_handler_set_generator_pole(ascent_handler_t *ah, uint16_t pair);
void ascent_handler_set_low_limit_resistance(ascent_handler_t *ah, uint16_t rpm);
void ascent_handler_set_low_limit_charge(ascent_handler_t *ah, uint16_t rpm);
void ascent_handler_set_tune_end_incline(ascent_handler_t *ah, uint16_t percentage);
void ascent_handler_set_console_power(ascent_handler_t *ah, uint16_t watts);
void ascent_handler_set_incline_action(ascent_handler_t *ah, uint8_t action);
void ascent_handler_auto_calibrate_incline(ascent_handler_t *ah);
void ascent_handler_set_turn_off_lcb(ascent_handler_t *ah, uint16_t minutes);
void ascent_handler_stop_calibrate(ascent_handler_t *ah);
void ascent_handler_stop_incline_action(ascent_handler_t *ah);

/* ============================================================================
 * One-shot Getter
 * ============================================================================ */

void ascent_handler_get_one_shot_rpm(ascent_handler_t *ah);

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

void ascent_handler_set_auto_reply_rpm(ascent_handler_t *ah, bool enable);
void ascent_handler_set_auto_reply_incline(ascent_handler_t *ah, bool enable);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to ascent_handler; see the matching doc
 * comment in bike_handler.h for the full explanation). Re-sends the same
 * single SetLCBInitial command general_handler's MACHINE_CATEGORY_BIKE
 * (default) branch uses — neither LCB_TYPE_ELECTROMAGNETIC_INCLINE nor
 * LCB_TYPE_CLIMBMILL_OR_A30 is in map_lcb_type_to_category()'s TM list, so
 * Ascent falls to the default/BIKE single-command category — highest-priority
 * since the LCB is already known-alive, then calls
 * lcb_comm_proc_estop_unlock() for the local UI notification. No-op if not
 * started (comm == NULL).
 */
void ascent_handler_estop_revert_lcb_initial(ascent_handler_t *ah);

#ifdef __cplusplus
}
#endif

#endif /* ASCENT_HANDLER_H */
