/**
 * @file tcv_fx_spin_bike_interface.h
 * @brief TcvFxSpinBike unit-conversion adapter — Layer_2 "adapters"
 *
 * Converted from:
 *   com/jht/lcbcommlib/lcbInterface/TcvFxSpinBikeInterface.java
 *
 * Wraps a tcv_fx_spin_bike_handler_t (Layer_3 Control) and adds the one piece
 * of business logic the Java Interface layer performs that the wire protocol
 * itself doesn't: converting the LCB's raw 0.1W watts units to whole watts.
 * RPM/resistance-level pass through unchanged. No FrameConfig integration —
 * the Java source has no handshake-completion FrameConfig dispatch for this
 * machine type (same as Elliptical/Ascent/Stepper/IndoorBike).
 */

#ifndef TCV_FX_SPIN_BIKE_INTERFACE_H
#define TCV_FX_SPIN_BIKE_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"
#include "lcb_type.h"
#include "tcv_fx_spin_bike_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct tcv_fx_spin_bike_interface tcv_fx_spin_bike_interface_t;

/* ============================================================================
 * User-Facing Callback Struct
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_rpm)(void *ud, int rpm);
    /* Already converted to whole watts (raw_watts / 10) */
    void (*on_watts)(void *ud, int watts);
    void (*on_resistance_level)(void *ud, int resistance_level);
    /* Raw, unconverted sensor data — for UI cards that want to display every
     * field (ecb_position_sensor, flywheel_rpm, ...) without re-deriving them. */
    void (*on_sensor_data)(void *ud, const tcv_fx_spin_bike_sensor_data_t *sensor_data);
} tcv_fx_spin_bike_interface_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

tcv_fx_spin_bike_interface_t* tcv_fx_spin_bike_interface_create(void);
void                          tcv_fx_spin_bike_interface_destroy(tcv_fx_spin_bike_interface_t *ti);

void tcv_fx_spin_bike_interface_callbacks_init(tcv_fx_spin_bike_interface_callbacks_t *cb);
void tcv_fx_spin_bike_interface_set_callbacks(tcv_fx_spin_bike_interface_t *ti,
                                              const tcv_fx_spin_bike_interface_callbacks_t *cb);
void tcv_fx_spin_bike_interface_unregister_callbacks(tcv_fx_spin_bike_interface_t *ti);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the adapter: creates its own internal tcv_fx_spin_bike_handler,
 * wires it up, and starts it. Unlike TreadMillInterface, the Java
 * TcvFxSpinBikeInterface constructor does NOT unconditionally enable any
 * auto-reply — it stays off until startReplyData(true) is called (Java:
 * IMachineInterfaceBase.startReplyData()).
 *
 * @param ti    TcvFxSpinBike interface instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool tcv_fx_spin_bike_interface_start(tcv_fx_spin_bike_interface_t *ti, lcb_comm_t *comm);

void tcv_fx_spin_bike_interface_stop(tcv_fx_spin_bike_interface_t *ti);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

void tcv_fx_spin_bike_interface_get_one_shot_sensor_data(tcv_fx_spin_bike_interface_t *ti);

/* ============================================================================
 * Auto-reply Toggle (Java: MachineInterfaceBase.startReplyData(boolean))
 * ============================================================================ */

void tcv_fx_spin_bike_interface_start_reply_data(tcv_fx_spin_bike_interface_t *ti, bool start);

/**
 * @brief Manual "EStop Revert" for when this adapter already owns lcb_comm
 * (post-handshake — see tcv_fx_spin_bike_handler_estop_revert_lcb_initial()'s
 * doc comment). No-op if @p ti or its internal handler is NULL.
 */
void tcv_fx_spin_bike_interface_estop_revert_lcb_initial(tcv_fx_spin_bike_interface_t *ti);

#ifdef __cplusplus
}
#endif

#endif /* TCV_FX_SPIN_BIKE_INTERFACE_H */
