/**
 * @file stepper_handler.h
 * @brief LCB Stepper Handler — Stepper machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/Stepper/IStepperHandler.java
 *   - com/jht/lcbcommlib/machineHandler/Stepper/IStepperHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/Stepper/StepperHandler.java
 *   - com/jht/lcbcommlib/machineHandler/Stepper/StepperCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent).
 * Manages:
 *   - Heartbeat thread @ 400ms driving auto-reply polling
 *   - 1 auto-reply feature: RPM read-back
 *
 * Unlike Bike/Elliptical, StepperHandler.java has no LCB-type-dependent
 * command branching anywhere (its `machineLcbType` field is dead code, never
 * read or written) — every setter maps 1:1 to a single lcb_comm call, so
 * there is no stepper_handler_set_lcb_type() here.
 */

#ifndef STEPPER_HANDLER_H
#define STEPPER_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct stepper_handler stepper_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IStepperHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_stepper_rpm)(void *ud, uint16_t rpm);
    void (*on_stepper_one_shot_rpm)(void *ud, uint16_t rpm);
} stepper_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

stepper_handler_t* stepper_handler_create(void);
void                stepper_handler_destroy(stepper_handler_t *sh);

void stepper_handler_callbacks_init(stepper_handler_callbacks_t *cb);
void stepper_handler_set_callbacks(stepper_handler_t *sh, const stepper_handler_callbacks_t *cb);
void stepper_handler_unregister_callbacks(stepper_handler_t *sh);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the stepper handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration) and
 * starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param sh    Stepper handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool stepper_handler_start(stepper_handler_t *sh, lcb_comm_t *comm);

void stepper_handler_stop(stepper_handler_t *sh);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

/* Java: setStepperResistance() -> mLcb.setLcbLimitRPMresistance() (no branching) */
void stepper_handler_set_resistance(stepper_handler_t *sh, uint16_t value);
void stepper_handler_set_gear_ratio(stepper_handler_t *sh, uint16_t ratio);
void stepper_handler_set_resistance_type(stepper_handler_t *sh, uint16_t type);
void stepper_handler_set_turn_off_lcb(stepper_handler_t *sh, uint16_t minutes);

/* ============================================================================
 * One-shot Getter
 * ============================================================================ */

void stepper_handler_get_one_shot_rpm(stepper_handler_t *sh);

/* ============================================================================
 * Auto-reply Toggle
 * ============================================================================ */

void stepper_handler_set_auto_reply_rpm(stepper_handler_t *sh, bool enable);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to stepper_handler; see the matching TM
 * doc comment in treadmill_handler.h for the full explanation). Re-sends the
 * same single SetLCBInitial command general_handler's default/Bike-category
 * branch uses, highest-priority since the LCB is already known-alive, then
 * calls lcb_comm_proc_estop_unlock() for the local UI notification. No-op if
 * not started (comm == NULL).
 */
void stepper_handler_estop_revert_lcb_initial(stepper_handler_t *sh);

#ifdef __cplusplus
}
#endif

#endif /* STEPPER_HANDLER_H */
