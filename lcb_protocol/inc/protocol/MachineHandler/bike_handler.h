/**
 * @file bike_handler.h
 * @brief LCB Bike Handler — Bike machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/Bike/IBikeHandler.java
 *   - com/jht/lcbcommlib/machineHandler/Bike/IBikeHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/Bike/BikeHandler.java
 *   - com/jht/lcbcommlib/machineHandler/Bike/BikeCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent).
 * Manages:
 *   - Heartbeat thread @ 400ms driving auto-reply polling
 *   - 2 auto-reply features: RPM read-back, Watts resend
 *   - LCB-type-dependent watt/resistance command selection
 */

#ifndef BIKE_HANDLER_H
#define BIKE_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"
#include "lcb_type.h"
#include "frame_config_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct bike_handler bike_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IBikeHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_bike_rpm)(void *ud, uint16_t rpm);
    void (*on_bike_one_shot_rpm)(void *ud, uint16_t rpm);
} bike_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

bike_handler_t* bike_handler_create(void);
void            bike_handler_destroy(bike_handler_t *bh);

void bike_handler_callbacks_init(bike_handler_callbacks_t *cb);
void bike_handler_set_callbacks(bike_handler_t *bh, const bike_handler_callbacks_t *cb);
void bike_handler_unregister_callbacks(bike_handler_t *bh);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the bike handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration) and
 * starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param bh    Bike handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool bike_handler_start(bike_handler_t *bh, lcb_comm_t *comm);

void bike_handler_stop(bike_handler_t *bh);

/* ============================================================================
 * LCB Type Injection (for setBikeWattOrResistance's machine-type branch)
 * ============================================================================ */

void bike_handler_set_lcb_type(bike_handler_t *bh, LcbType_t type);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

void bike_handler_set_watt_or_resistance(bike_handler_t *bh, uint16_t value);
void bike_handler_set_gear_ratio(bike_handler_t *bh, uint16_t ratio);
void bike_handler_set_resistance_type(bike_handler_t *bh, uint16_t type);
void bike_handler_set_low_limit_resistance(bike_handler_t *bh, uint16_t rpm);
void bike_handler_set_low_limit_charge(bike_handler_t *bh, uint16_t rpm);
void bike_handler_set_turn_off_lcb(bike_handler_t *bh, uint16_t minutes);

/* Java: LCBDataHandler.setBikeMachineControl()'s unconditional
 * lcbInterface.setGeneratorPole((byte) 0x04) call — nothing in this codebase
 * called lcb_comm_set_lcb_generator_pole_pair() (cmd 0x69) before this. */
void bike_handler_set_generator_pole(bike_handler_t *bh, uint16_t pair);

/**
 * @brief Applies Java's LCBDataHandler.setBikeMachineControl() — sent once
 * general_handler's LCB init handshake finishes and bike_handler has
 * attached. Java: LCBInfo.isChopper() (lcbType==0x13 || lcbType==0x14)
 * selects gearRatio=840/resistanceType=0, otherwise gearRatio=1500/
 * resistanceType=(0x2000|100); either way followed by the unconditional
 * generator-pole-pair=4 command. Uses the LCB type injected via
 * bike_handler_set_lcb_type() rather than @p fc (frame_config_data_t has no
 * per-model gear-ratio/resistance-type field to read this from — those are
 * derived from the LCB type, matching Java's isChopper() check). @p fc is
 * accepted for API symmetry with treadmill_interface_apply_frame_config()
 * and future extension, but unused today. No-op if @p bh or @p fc is NULL,
 * or the handler has not been started (comm == NULL).
 */
void bike_handler_apply_frame_config(bike_handler_t *bh, const frame_config_data_t *fc);

/* ============================================================================
 * One-shot Getter
 * ============================================================================ */

void bike_handler_get_one_shot_rpm(bike_handler_t *bh);

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

void bike_handler_set_auto_reply_rpm(bike_handler_t *bh, bool enable);
void bike_handler_set_auto_resend_watt(bike_handler_t *bh, bool enable);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to bike_handler; see the matching TM
 * doc comment in treadmill_handler.h for the full explanation). Re-sends the
 * same single SetLCBInitial command general_handler's MACHINE_CATEGORY_BIKE
 * (default) branch uses, highest-priority since the LCB is already
 * known-alive, then calls lcb_comm_proc_estop_unlock() for the local UI
 * notification. No-op if not started (comm == NULL).
 */
void bike_handler_estop_revert_lcb_initial(bike_handler_t *bh);

#ifdef __cplusplus
}
#endif

#endif /* BIKE_HANDLER_H */
