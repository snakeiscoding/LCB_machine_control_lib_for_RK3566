/**
 * @file indoor_bike_handler.h
 * @brief LCB IndoorBike Handler — IndoorBike machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/IndoorBike/IIndoorBikeHandler.java
 *   - com/jht/lcbcommlib/machineHandler/IndoorBike/IIndoorBikeHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/IndoorBike/IndoorBikeHandler.java
 *   - com/jht/lcbcommlib/machineHandler/IndoorBike/IndoorBikeCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent), same
 * as bike_handler. No Layer_2 adapter exists for this machine: there is no
 * IndoorBikeInterface.java in lcbInterface/ and LCBInterface.java has zero
 * IndoorBike wiring, so there is no business logic to port at that layer.
 *
 * Manages:
 *   - Heartbeat thread @ 400ms driving auto-reply polling
 *   - 4 auto-reply features: RPM, Watts (ICR70), Sensor Data, Info SpinDown
 *   - LCB-type-dependent gating (LCB_TYPE_JIS_ATHENA / "isIcr()") for the
 *     raw-ADC request/response path (reuses the 0xA0 DCI incline-range cmd)
 *
 * Out of scope (deliberately not ported — see plan/report for rationale):
 *   - IndoorBikeCalibrationData.java's calibration payload codec: it is not
 *     referenced by IndoorBikeHandler/IIndoorBikeHandlerCallback at all (only
 *     raw byte[] is passed for CalcParaOfDevice/ActionSpinDown/ErgModeSettings)
 *   - Decoded field-level structs for indoor_bike_sensor_data_t /
 *     indoor_bike_info_spin_down_t: Layer_4 (lcb_proc_comm.c) already casts
 *     the raw wire bytes to these types as an OPAQUE forward-declared marker
 *     (see lcb_comm_callback.h) without ever defining their bodies anywhere
 *     in this codebase, and existing tests (test_lcb_comm_cmd.cpp) only count
 *     callback invocations rather than dereferencing fields. Decoding the
 *     19-byte / 15-byte wire payloads into named fields was never done at
 *     Layer_4, and doing it here would require guessing byte widths/order not
 *     confirmed anywhere in the C or Java source. This handler forwards the
 *     same opaque pointer type unchanged, matching Layer_4's own convention.
 */

#ifndef INDOOR_BIKE_HANDLER_H
#define INDOOR_BIKE_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct indoor_bike_handler indoor_bike_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IIndoorBikeHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_rpm)(void *ud, int rpm);
    void (*on_watts)(void *ud, int watts);
    void (*on_one_shot_rpm)(void *ud, int rpm);
    void (*on_sensor_data)(void *ud, const indoor_bike_sensor_data_t *sensor_data);
    void (*on_one_shot_sensor_data)(void *ud, const indoor_bike_sensor_data_t *sensor_data);
    void (*on_get_calc_para_of_device)(void *ud, const uint8_t *data, uint8_t length);
    void (*on_set_action_spin_down)(void *ud, const uint8_t *data, uint8_t length);
    void (*on_set_erg_mode_settings)(void *ud, const uint8_t *data, uint8_t length);
    void (*on_info_spin_down)(void *ud, const indoor_bike_info_spin_down_t *info);
    void (*on_raw_adc)(void *ud, int adc);
    /* Note: no on_get_erg_mode_settings — Java's
     * getLcbIndoorBikeErgModeSettingsHandler() (0x59 GET reply) is a "do
     * nothing" stub with no corresponding IIndoorBikeHandlerCallback method;
     * intentionally left unbridged in indoor_bike_lcb_cb.c to match. */
} indoor_bike_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

indoor_bike_handler_t* indoor_bike_handler_create(void);
void                   indoor_bike_handler_destroy(indoor_bike_handler_t *ibh);

void indoor_bike_handler_callbacks_init(indoor_bike_handler_callbacks_t *cb);
void indoor_bike_handler_set_callbacks(indoor_bike_handler_t *ibh, const indoor_bike_handler_callbacks_t *cb);
void indoor_bike_handler_unregister_callbacks(indoor_bike_handler_t *ibh);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the indoor bike handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration)
 * and starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param ibh   IndoorBike handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool indoor_bike_handler_start(indoor_bike_handler_t *ibh, lcb_comm_t *comm);

void indoor_bike_handler_stop(indoor_bike_handler_t *ibh);

/* ============================================================================
 * LCB Type Injection (Java: isIcr(), gates the raw-ADC request/response path)
 * ============================================================================ */

void indoor_bike_handler_set_lcb_type(indoor_bike_handler_t *ibh, LcbType_t type);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

void indoor_bike_handler_set_gear_ratio(indoor_bike_handler_t *ibh, uint16_t ratio);
void indoor_bike_handler_set_resistance_type(indoor_bike_handler_t *ibh, uint16_t type);
void indoor_bike_handler_set_low_limit_resistance(indoor_bike_handler_t *ibh, uint16_t rpm);
void indoor_bike_handler_set_low_limit_charge(indoor_bike_handler_t *ibh, uint16_t rpm);
void indoor_bike_handler_set_turn_off_lcb(indoor_bike_handler_t *ibh, uint16_t minutes);

void indoor_bike_handler_get_one_shot_rpm(indoor_bike_handler_t *ibh);
void indoor_bike_handler_get_one_shot_sensor_data(indoor_bike_handler_t *ibh);

void indoor_bike_handler_set_stepper_motor_position(indoor_bike_handler_t *ibh, const uint8_t *data, uint8_t length);
void indoor_bike_handler_set_calc_para_of_device(indoor_bike_handler_t *ibh, const uint8_t *data, uint8_t length);
void indoor_bike_handler_get_calc_para_of_device(indoor_bike_handler_t *ibh);
void indoor_bike_handler_set_action_spin_down(indoor_bike_handler_t *ibh, const uint8_t *data, uint8_t length);
void indoor_bike_handler_get_info_spin_down(indoor_bike_handler_t *ibh);
void indoor_bike_handler_set_erg_mode_settings(indoor_bike_handler_t *ibh, const uint8_t *data, uint8_t length);
void indoor_bike_handler_get_erg_mode_settings(indoor_bike_handler_t *ibh);
void indoor_bike_handler_set_resistance_level(indoor_bike_handler_t *ibh, uint16_t level);

/* Java: setLcbIndoorBikeErgWatts() reuses mLcb.setLcbWatts(targetWatts,
 * QueueMode.AlwaysAdd) — the same 0x6x watts command bike_handler uses for
 * setBikeWattOrResistance's LCBA/1X_BIKE_LCB branch, not a distinct ERG-only
 * command. */
void indoor_bike_handler_set_erg_watts(indoor_bike_handler_t *ibh, uint16_t target_watts);

/* Java: requestRawAdc(int index) — index parameter is unused in the Java
 * body; ICR70 (LCB_TYPE_JIS_ATHENA) re-uses the 0xA0 DCI-incline-range
 * command to read raw ADC for calibration. No-op unless the injected LCB
 * type is LCB_TYPE_JIS_ATHENA (Java: isIcr()). */
void indoor_bike_handler_request_raw_adc(indoor_bike_handler_t *ibh);

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

void indoor_bike_handler_set_auto_reply_rpm(indoor_bike_handler_t *ibh, bool enable);
void indoor_bike_handler_set_auto_reply_watts(indoor_bike_handler_t *ibh, bool enable);
void indoor_bike_handler_set_auto_reply_sensor_data(indoor_bike_handler_t *ibh, bool enable);
void indoor_bike_handler_set_auto_reply_info_spin_down(indoor_bike_handler_t *ibh, bool enable);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to indoor_bike_handler; see the matching
 * doc comment in bike_handler.h / treadmill_handler.h for the full
 * explanation). Re-sends the same single SetLCBInitial command
 * general_handler's MACHINE_CATEGORY_BIKE branch uses (LCB_TYPE_JIS_ATHENA
 * maps to MACHINE_CATEGORY_BIKE in general_handler.c's
 * map_lcb_type_to_category(), same as every other IndoorBike-capable LCB
 * type), highest-priority since the LCB is already known-alive, then calls
 * lcb_comm_proc_estop_unlock() for the local UI notification. No-op if not
 * started (comm == NULL).
 */
void indoor_bike_handler_estop_revert_lcb_initial(indoor_bike_handler_t *ibh);

#ifdef __cplusplus
}
#endif

#endif /* INDOOR_BIKE_HANDLER_H */
