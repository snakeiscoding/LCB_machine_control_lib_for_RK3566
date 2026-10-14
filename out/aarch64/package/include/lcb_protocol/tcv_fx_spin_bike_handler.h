/**
 * @file tcv_fx_spin_bike_handler.h
 * @brief LCB TcvFxSpinBike Handler — TcvFxSpinBike (manual-resistance spin bike) orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/TcvFxSpinBike/ITcvFxSpinBikeHandler.java
 *   - com/jht/lcbcommlib/machineHandler/TcvFxSpinBike/ITcvFxSpinBikeHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/TcvFxSpinBike/TcvFxSpinBikeHandler.java
 *   - com/jht/lcbcommlib/machineHandler/TcvFxSpinBike/TcvFxSpinBikeCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent), same
 * as bike_handler/indoor_bike_handler.
 *
 * Manages:
 *   - Heartbeat thread @ 400ms driving a single auto-reply feature: SensorData
 *
 * IMPORTANT — reuses IndoorBike's 0x51 GetLCBIndoorBikeSensorData command/
 * callback slot, not a distinct command. TCV-FX's payload is a distinct,
 * shorter 11-byte layout (vs IndoorBike's 19 bytes) discriminated by the
 * LCB type byte at data[0] (LCB_TYPE_TCV_FX = 0x22). Because the
 * MachineHandlerUI architecture only ever runs one machine mode at a time
 * (single g_lcb_comm, single-owner lcb_comm_register_callbacks), this handler
 * and indoor_bike_handler never register simultaneously, so no cross-module
 * gating is needed on IndoorBike's side. This handler still checks the
 * discriminator byte defensively in its own bridge (tcv_fx_spin_bike_lcb_cb.c),
 * matching the Java source's own defensive check.
 */

#ifndef TCV_FX_SPIN_BIKE_HANDLER_H
#define TCV_FX_SPIN_BIKE_HANDLER_H

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

typedef struct tcv_fx_spin_bike_handler tcv_fx_spin_bike_handler_t;

/* ============================================================================
 * Sensor Data — real, named-field struct (unlike indoor_bike_sensor_data_t,
 * which stays opaque because its byte layout was never confirmed). TCV-FX's
 * 11-byte layout IS confirmed field-by-field from the Java source, so it is
 * decoded here rather than passed through as raw bytes.
 * ============================================================================ */

typedef struct {
    uint8_t  lcb_type;
    uint8_t  machine_type;
    int16_t  crank_rpm;
    int16_t  flywheel_rpm;
    int16_t  ecb_position_sensor;
    uint8_t  resistance_level;
    /* Raw 0.1W units — unit conversion to W is done by the Layer_2 adapter
     * (tcv_fx_spin_bike_interface), not here, matching wire-protocol fidelity
     * conventions used by other Layer_3 handlers. */
    int16_t  watts;
} tcv_fx_spin_bike_sensor_data_t;

/* ============================================================================
 * User-Facing Callback Struct (ITcvFxSpinBikeHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_sensor_data)(void *ud, const tcv_fx_spin_bike_sensor_data_t *sensor_data);
    void (*on_rpm)(void *ud, int rpm);
    void (*on_watts)(void *ud, int watts);
    void (*on_resistance_level)(void *ud, int resistance_level);
} tcv_fx_spin_bike_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

tcv_fx_spin_bike_handler_t* tcv_fx_spin_bike_handler_create(void);
void                        tcv_fx_spin_bike_handler_destroy(tcv_fx_spin_bike_handler_t *h);

void tcv_fx_spin_bike_handler_callbacks_init(tcv_fx_spin_bike_handler_callbacks_t *cb);
void tcv_fx_spin_bike_handler_set_callbacks(tcv_fx_spin_bike_handler_t *h, const tcv_fx_spin_bike_handler_callbacks_t *cb);
void tcv_fx_spin_bike_handler_unregister_callbacks(tcv_fx_spin_bike_handler_t *h);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the TcvFxSpinBike handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration)
 * and starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param h     TcvFxSpinBike handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool tcv_fx_spin_bike_handler_start(tcv_fx_spin_bike_handler_t *h, lcb_comm_t *comm);

void tcv_fx_spin_bike_handler_stop(tcv_fx_spin_bike_handler_t *h);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

void tcv_fx_spin_bike_handler_get_one_shot_sensor_data(tcv_fx_spin_bike_handler_t *h);

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

void tcv_fx_spin_bike_handler_set_auto_reply_sensor_data(tcv_fx_spin_bike_handler_t *h, bool enable);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to this handler; see the matching doc
 * comment in bike_handler.h/indoor_bike_handler.h for the full explanation).
 * TCV-FX falls into general_handler.c's map_lcb_type_to_category() default
 * branch (MACHINE_CATEGORY_BIKE), so this re-sends the same single
 * SetLCBInitial command that category uses, highest-priority since the LCB
 * is already known-alive, then calls lcb_comm_proc_estop_unlock() for the
 * local UI notification. No-op if not started (comm == NULL).
 */
void tcv_fx_spin_bike_handler_estop_revert_lcb_initial(tcv_fx_spin_bike_handler_t *h);

#ifdef __cplusplus
}
#endif

#endif /* TCV_FX_SPIN_BIKE_HANDLER_H */
