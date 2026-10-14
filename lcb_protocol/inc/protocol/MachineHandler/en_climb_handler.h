/**
 * @file en_climb_handler.h
 * @brief LCB EnClimb Handler — Climbmill machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/EnClimbmill/IEnClimbHandler.java
 *   - com/jht/lcbcommlib/machineHandler/EnClimbmill/IEnClimbHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/EnClimbmill/EnClimbHandler.java
 *   - com/jht/lcbcommlib/machineHandler/EnClimbmill/EnClimbCallbackHandlerLCB.java
 *
 * NOTE: the Java package/directory is "EnClimbmill" but the class prefix is
 * "EnClimb" (IEnClimbHandler, EnClimbHandler, ...) — this C module follows the
 * class-prefix naming convention (same rule AscentTrainer -> ascent_handler
 * used), hence "en_climb_handler", not "climbmill_handler".
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent).
 * Manages:
 *   - Heartbeat thread @ 400ms driving auto-reply polling
 *   - Unconditional per-tick Climbmill status keep-alive poll
 *   - 5 auto-reply features: SPM, brake pressure, watts, cal scaling,
 *     CZ-capacitance/IR-frequency (via the device-data 0x04 GetEStopVale
 *     sub-command)
 *
 * Like Stepper/TcvFxSpinBike, EnClimbHandler.java has no LCB-type-dependent
 * command branching (its `machineLcbType` field is dead code, never read or
 * written) — every setter maps 1:1 to a single lcb_comm/lcb_dd call, so there
 * is no en_climb_handler_set_lcb_type() here. There is also no ramp/state
 * machine and no timeout/error-code interaction logic in the Java source.
 *
 * Java's `EnClimbmillInterface.java` (lcbInterface/) is an explicit STUB, so
 * no Layer_2 adapter was created for this machine (same as Bike/Elliptical/
 * Stepper/IndoorBike).
 */

#ifndef EN_CLIMB_HANDLER_H
#define EN_CLIMB_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct en_climb_handler en_climb_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IEnClimbHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_status)(void *ud, uint16_t status);
    void (*on_spm)(void *ud, uint16_t spm);
    void (*on_brake_pressure)(void *ud, uint16_t value);
    void (*on_watts)(void *ud, uint16_t watts);
    void (*on_estop_vale)(void *ud, const uint8_t *data, uint8_t length);
    void (*on_cal_scaling)(void *ud, uint16_t scaling);
    void (*on_frame_cal_scaling)(void *ud, uint16_t scaling);
    void (*on_one_shot_spm)(void *ud, uint16_t spm);
} en_climb_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

en_climb_handler_t* en_climb_handler_create(void);
void                en_climb_handler_destroy(en_climb_handler_t *ech);

void en_climb_handler_callbacks_init(en_climb_handler_callbacks_t *cb);
void en_climb_handler_set_callbacks(en_climb_handler_t *ech, const en_climb_handler_callbacks_t *cb);
void en_climb_handler_unregister_callbacks(en_climb_handler_t *ech);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the EnClimb handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration) and
 * starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param ech   EnClimb handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool en_climb_handler_start(en_climb_handler_t *ech, lcb_comm_t *comm);

void en_climb_handler_stop(en_climb_handler_t *ech);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

/* Java: setEnClimbSPM() -> mLcb.setLcbSPM() */
void en_climb_handler_set_spm(en_climb_handler_t *ech, uint16_t spm);

/* Java: setEnClimbResistance() -> mLcb.setLcbLimitRPMresistance() (no branching) */
void en_climb_handler_set_resistance(en_climb_handler_t *ech, uint16_t value);

/* Java: setEnClimbUnlock() -> mLcb.setLcbClimbmillStatus((byte)0x00) (normal priority) */
void en_climb_handler_unlock(en_climb_handler_t *ech);

void en_climb_handler_set_gear_ratio(en_climb_handler_t *ech, uint16_t ratio);
void en_climb_handler_set_resistance_type(en_climb_handler_t *ech, uint16_t ohm);
void en_climb_handler_set_generator_pole(en_climb_handler_t *ech, uint16_t pair);
void en_climb_handler_set_estop_parameter(en_climb_handler_t *ech, uint8_t value);

/* Java: setEnClimbClimbmillCommand() -> mLcb.setLcbClimbmillStatus() (normal priority) */
void en_climb_handler_set_climbmill_command(en_climb_handler_t *ech, uint8_t value);

void en_climb_handler_set_climbmill_cal(en_climb_handler_t *ech, const uint8_t *data, uint8_t length);
void en_climb_handler_set_cal_scaling(en_climb_handler_t *ech, uint16_t cal_scaling);
void en_climb_handler_set_frame_cal_scaling(en_climb_handler_t *ech, uint16_t frame_cal_scaling);

/* ============================================================================
 * One-shot Getters
 * ============================================================================ */

/* Java: getEnClimbClimbmillCalScaling() */
void en_climb_handler_get_cal_scaling(en_climb_handler_t *ech);

/* Java: getEnClimbClimbmillFrameCalScaling() */
void en_climb_handler_get_frame_cal_scaling(en_climb_handler_t *ech);

/* Java: getEnClimbOneShotReplySpm() */
void en_climb_handler_get_one_shot_spm(en_climb_handler_t *ech);

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

void en_climb_handler_set_auto_reply_spm(en_climb_handler_t *ech, bool enable);
void en_climb_handler_set_auto_reply_brake_pressure(en_climb_handler_t *ech, bool enable);
void en_climb_handler_set_auto_reply_watts(en_climb_handler_t *ech, bool enable);
void en_climb_handler_set_auto_reply_cal_scaling(en_climb_handler_t *ech, bool enable);
void en_climb_handler_set_auto_reply_cz_capa_ir_freq(en_climb_handler_t *ech, bool enable);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to en_climb_handler; see the matching TM
 * doc comment in treadmill_handler.h for the full explanation). All climbmill
 * LCB types (LCB_TYPE_CLIMBMILL_OR_A30 / LCB_TYPE_COMMERCIAL_CLIMBMILL /
 * LCB_TYPE_RETAIL_CLIMBMILL) fall into MACHINE_CATEGORY_BIKE in
 * general_handler.c's map_lcb_type_to_category(), so this re-sends the same
 * single SetLCBInitial command that category uses, highest-priority since the
 * LCB is already known-alive, then calls lcb_comm_proc_estop_unlock() for the
 * local UI notification. No-op if not started (comm == NULL).
 */
void en_climb_handler_estop_revert_lcb_initial(en_climb_handler_t *ech);

#ifdef __cplusplus
}
#endif

#endif /* EN_CLIMB_HANDLER_H */
