/**
 * @file treadmill_interface.h
 * @brief Treadmill safety/orchestration adapter — Layer_2 "adapters"
 *
 * Converted from (narrowed scope — see plan/commit message for what was
 * deliberately deferred):
 *   com/jht/chimera/service/hal/lib/lcb/lcbcommlib/lcbInterface/TreadMillInterface.java
 *
 * Wraps a treadmill_handler_t (Layer_3 Control) and adds exactly the
 * ext-status/safety-key/error-code logic that TreadMillInterface.java's
 * constructor and onTreadmillExtStatus() handler perform unconditionally at
 * bring-up. Over-speed rules, roller-guard, and Hybrid-TM-IR-EStop are NOT
 * ported here — out of scope for this pass (see FrameConfig/Capabilities
 * deferral noted in the plan).
 */

#ifndef TREADMILL_INTERFACE_H
#define TREADMILL_INTERFACE_H

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

typedef struct treadmill_interface treadmill_interface_t;

/* ============================================================================
 * User-Facing Callback Struct
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_rpm)(void *ud, uint16_t rpm);
    void (*on_one_shot_rpm)(void *ud, uint16_t rpm);
    void (*on_incline_adc)(void *ud, uint16_t adc);
    void (*on_person_in_place)(void *ud, bool in_place);

    /* Safety (ext-status derived) */
    void (*on_safetykey_lock_state)(void *ud, bool locked);
    void (*on_start_unlock)(void *ud);
    void (*on_auto_oc_state)(void *ud, bool detected);

    /* GUI mirror */
    void (*on_lcb_set_rpm_value)(void *ud, uint16_t rpm);
    void (*on_lcb_set_adc_value)(void *ud, uint16_t adc);

    /* LCB error codes (handshake-phase and post-attach). The E-Stop codes
     * 0x02B2 / 0x02B1 never come through here — see on_safetykey_locked_code. */
    void (*on_error_code)(void *ud, uint16_t error_code);

    /* E-Stop lock by LCB error code (Java ILCBCallback.onLCBSafetykeyLockedCode):
     * true on 0x02B2 (E-Stop), false on 0x02B1 (E-Stop unlock). */
    void (*on_safetykey_locked_code)(void *ud, bool locked);

    /* Pass-throughs added for the control layer (treadmill_control) */
    void (*on_motor_info)(void *ud, const uint8_t *info, uint8_t length);   /* 0x96 */
    void (*on_ext_status)(void *ud, uint16_t status);                       /* 0x97 raw */
    void (*on_lcb_status)(void *ud, uint8_t lcb_status);                    /* status byte, every packet */
} treadmill_interface_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

treadmill_interface_t* treadmill_interface_create(void);
void                   treadmill_interface_destroy(treadmill_interface_t *ti);

void treadmill_interface_callbacks_init(treadmill_interface_callbacks_t *cb);
void treadmill_interface_set_callbacks(treadmill_interface_t *ti,
                                       const treadmill_interface_callbacks_t *cb);
void treadmill_interface_unregister_callbacks(treadmill_interface_t *ti);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the adapter: creates its own internal treadmill_handler,
 * wires it up, starts it, and unconditionally enables ext-status auto-reply
 * (mirrors TreadMillInterface.java's constructor).
 *
 * @param ti    Treadmill interface instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool treadmill_interface_start(treadmill_interface_t *ti, lcb_comm_t *comm);

void treadmill_interface_stop(treadmill_interface_t *ti);

/* ============================================================================
 * LCB Type Injection (for E0/Toptek skip logic in the underlying handler)
 * ============================================================================ */

void treadmill_interface_set_lcb_type(treadmill_interface_t *ti, LcbType_t type);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

/* Safety-gated: no-op while the MCB safety key is down (matches Java's
 * checkSafetyState() guard on setTreadMillRPM/setTreadmillStopMachine). */
void treadmill_interface_set_rpm(treadmill_interface_t *ti, uint16_t rpm);
void treadmill_interface_set_rpm_high_priority(treadmill_interface_t *ti, uint16_t rpm);

void treadmill_interface_set_incline(treadmill_interface_t *ti, uint16_t loc);
void treadmill_interface_set_incline_high_priority(treadmill_interface_t *ti, uint16_t loc);
void treadmill_interface_set_incline_action(treadmill_interface_t *ti, uint8_t action);
void treadmill_interface_set_incline_action_high_priority(treadmill_interface_t *ti, uint8_t action);
void treadmill_interface_set_motor_type(treadmill_interface_t *ti, uint16_t watts);
void treadmill_interface_set_ecb_action_high_priority(treadmill_interface_t *ti, uint8_t value);

/**
 * @brief Applies the "core subset" of Java's
 * LCBDataHandler.setTreadmillMachineControl() — the config-driven commands
 * sent once general_handler's LCB init handshake finishes and this adapter
 * has attached: RPM reset to 0, incline-stop (highest priority), and the
 * model's initial incline ADC (with the reverse-incline direction bit folded
 * in per @p fc's uses_reverse_incline). No-op if @p ti or @p fc is NULL.
 *
 * Everything else Java's setTreadmillMachineControl() sends (motor type,
 * ramp-up/down, overspeed enable, IR sensor, current-OC-detect,
 * no-person-detect, ext-status gating) has no field in frame_config_data_t
 * yet (that schema was ported from a different project than chimera2) and
 * stays deferred — see the stubs in HalEntryPoint.c.
 */
void treadmill_interface_apply_frame_config(treadmill_interface_t *ti,
                                            const frame_config_data_t *fc);

/* ============================================================================
 * Safety State Queries (ext-status derived)
 * ============================================================================ */

bool treadmill_interface_is_safetykey_down(const treadmill_interface_t *ti);
bool treadmill_interface_is_safetykey_locked(const treadmill_interface_t *ti);
/* 0x02B2 E-Stop error read through the LCB error-status loop, latched until unlock. */
bool treadmill_interface_is_lcb_estop_error(const treadmill_interface_t *ti);

/* ============================================================================
 * One-shot Getters
 * ============================================================================ */

void treadmill_interface_get_one_shot_rpm(treadmill_interface_t *ti);
void treadmill_interface_get_one_shot_incline_adc(treadmill_interface_t *ti);

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

void treadmill_interface_set_auto_reply_rpm(treadmill_interface_t *ti, bool enable);
void treadmill_interface_set_auto_reply_incline(treadmill_interface_t *ti, bool enable);
void treadmill_interface_set_auto_reply_in_used(treadmill_interface_t *ti, bool enable);
/* Manual override — note the adapter's own on_treadmill_in_used bridge also
 * toggles this automatically based on the in-used byte (Java parity), so a
 * manual call here gets overwritten on the next in-used tick. */
void treadmill_interface_set_auto_reply_speed_up_in_used(treadmill_interface_t *ti, bool enable);
void treadmill_interface_set_auto_reply_motor_info(treadmill_interface_t *ti, bool enable);
void treadmill_interface_set_auto_reply_ext_status(treadmill_interface_t *ti, bool enable);

/* ============================================================================
 * RPM Ramp Control
 * ============================================================================ */

void treadmill_interface_set_auto_ramp_rpm(treadmill_interface_t *ti, bool enable,
                                           int ramp_up_step, int ramp_down_step);
void treadmill_interface_estop_reset_gui_current_rpm(treadmill_interface_t *ti);

/**
 * @brief Manual "EStop Revert" for when this adapter already owns lcb_comm
 * (post-handshake — see treadmill_handler_estop_revert_lcb_initial()'s doc
 * comment for why general_handler's own revert can't reach this window).
 * No-op if @p ti or its internal handler is NULL.
 */
void treadmill_interface_estop_revert_lcb_initial(treadmill_interface_t *ti);

#ifdef __cplusplus
}
#endif

#endif /* TREADMILL_INTERFACE_H */
