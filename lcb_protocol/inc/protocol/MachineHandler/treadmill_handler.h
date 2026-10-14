/**
 * @file treadmill_handler.h
 * @brief LCB Treadmill Handler — TM machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/Treadmill/ITreadMillHandler.java
 *   - com/jht/lcbcommlib/machineHandler/Treadmill/ITreadMillHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/Treadmill/TreadMillHandler.java
 *   - com/jht/lcbcommlib/machineHandler/Treadmill/TreadMillCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent).
 * Manages:
 *   - Heartbeat thread @ 400ms driving auto-reply polling
 *   - 7 active auto-reply features + 1 stub feature (IR Addr IR status)
 *   - RPM ramp up/down state machine (default 250 RPM steps)
 *   - No-RPM-error timeout (40s = 100 ticks at 400ms)
 *   - LCB type skip logic (E0/Toptek skip InclineAction; Toptek skip status query)
 *   - GUI mirror callbacks (echo of set RPM/ADC values)
 */

#ifndef TREADMILL_HANDLER_H
#define TREADMILL_HANDLER_H

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

typedef struct treadmill_handler treadmill_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (ITreadMillHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    /* Speed & Incline */
    void (*on_treadmill_rpm)(void *ud, uint16_t rpm);
    void (*on_treadmill_one_shot_rpm)(void *ud, uint16_t rpm);
    void (*on_treadmill_incline_loc)(void *ud, uint16_t loc);

    /* Status */
    void (*on_treadmill_in_used)(void *ud, uint8_t status);
    void (*on_treadmill_motor_info)(void *ud, const uint8_t *info, uint8_t length);
    void (*on_treadmill_ext_status)(void *ud, uint16_t status);
    void (*on_treadmill_operate)(void *ud, const uint8_t *data, uint8_t length);

    /* Service-switch via battery-capacity-status bit (TM repurposes this) */
    void (*on_treadmill_service_switch)(void *ud, bool is_on);

    /* GUI mirroring of writes on serial port */
    void (*on_treadmill_lcb_set_rpm_value)(void *ud, uint16_t rpm);
    void (*on_treadmill_lcb_set_adc_value)(void *ud, uint16_t adc);

    /* Cadence (SPM), command 0x7F */
    void (*on_treadmill_cadence)(void *ud, const uint8_t *data, uint8_t length);

    /* LCB error codes observed after this handler is attached (e.g. 0x02B1/0x02B2
     * safety-key lock/unlock) — forwarded here in addition to the internal
     * no-RPM-timeout-clearing handling th_on_lcb_error_code() already does. */
    void (*on_treadmill_error_code)(void *ud, uint16_t error_code);

    /* Hybrid TM IR Address (0x26) — stubs for future lcb_comm support */
    void (*on_hybrid_tm_ir_addr_version)(void *ud, int version);
    void (*on_hybrid_tm_ir_addr_ir_status)(void *ud, int status);
    void (*on_hybrid_tm_ir_addr_enter_erp_mode)(void *ud, bool success);
    void (*on_hybrid_tm_ir_addr_leave_erp_mode)(void *ud, bool success);

    /* Raw LCB status byte carried by every response packet (incline up/down,
     * main motor, encoder/self-power bits). Added for the control layer's
     * safe-to-start / self-power logic. */
    void (*on_treadmill_lcb_status)(void *ud, uint8_t lcb_status);
} treadmill_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

treadmill_handler_t* treadmill_handler_create(void);
void                 treadmill_handler_destroy(treadmill_handler_t *th);

void treadmill_handler_callbacks_init(treadmill_handler_callbacks_t *cb);
void treadmill_handler_set_callbacks(treadmill_handler_t *th,
                                     const treadmill_handler_callbacks_t *cb);
void treadmill_handler_unregister_callbacks(treadmill_handler_t *th);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the treadmill handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration) and
 * starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param th    Treadmill handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool treadmill_handler_start(treadmill_handler_t *th, lcb_comm_t *comm);

void treadmill_handler_stop(treadmill_handler_t *th);

/* ============================================================================
 * LCB Type Injection (for E0/Toptek skip logic)
 * ============================================================================ */

void treadmill_handler_set_lcb_type(treadmill_handler_t *th, LcbType_t type);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

void treadmill_handler_set_rpm(treadmill_handler_t *th, uint16_t rpm);
void treadmill_handler_set_rpm_high_priority(treadmill_handler_t *th, uint16_t rpm);
void treadmill_handler_set_incline(treadmill_handler_t *th, uint16_t loc);
void treadmill_handler_set_incline_high_priority(treadmill_handler_t *th, uint16_t loc);

/* Skipped for LCB_TYPE_E0 and LCB_TYPE_TOPTEK */
void treadmill_handler_set_incline_action(treadmill_handler_t *th, uint8_t action);
void treadmill_handler_set_incline_action_high_priority(treadmill_handler_t *th, uint8_t action);

void treadmill_handler_set_motor_type(treadmill_handler_t *th, uint16_t watts);
void treadmill_handler_set_ecb_action_high_priority(treadmill_handler_t *th, uint8_t value);

/* Raw treadmill operate (cmd 0x7E) — caller composes the data payload */
void treadmill_handler_send_treadmill_operate(treadmill_handler_t *th,
                                              const uint8_t *data, uint8_t length);

/* ============================================================================
 * One-shot Getters
 * ============================================================================ */

void treadmill_handler_get_one_shot_rpm(treadmill_handler_t *th);
void treadmill_handler_get_one_shot_incline_adc(treadmill_handler_t *th);

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

void treadmill_handler_set_auto_reply_rpm(treadmill_handler_t *th, bool enable);
void treadmill_handler_set_auto_reply_incline(treadmill_handler_t *th, bool enable);
void treadmill_handler_set_auto_reply_in_used(treadmill_handler_t *th, bool enable);
void treadmill_handler_set_auto_reply_speed_up_in_used(treadmill_handler_t *th, bool enable);
void treadmill_handler_set_auto_reply_motor_info(treadmill_handler_t *th, bool enable);
void treadmill_handler_set_auto_reply_ext_status(treadmill_handler_t *th, bool enable);

/* Stubs — toggles flag but no underlying lcb_comm support yet */
void treadmill_handler_set_auto_reply_cadence(treadmill_handler_t *th, bool enable);
void treadmill_handler_set_auto_reply_ir_addr_ir_status(treadmill_handler_t *th, bool enable);

/* ============================================================================
 * RPM Ramp Control
 * ============================================================================ */

void treadmill_handler_set_auto_ramp_rpm(treadmill_handler_t *th, bool enable,
                                         int ramp_up_step, int ramp_down_step);
void treadmill_handler_estop_reset_gui_current_rpm(treadmill_handler_t *th);

/**
 * @brief Inject the 0x02B1 (E-Stop unlock) error code into this handler's
 * lcb_comm, notifying whichever callbacks are currently registered on it —
 * same mechanism general_handler's own E-Stop-revert flow uses
 * (lcb_comm_proc_estop_unlock()). This is a local software notification, not
 * a command transmitted over the wire — the physical MCB unlocks on its own
 * when the safety-key circuit opens; this just tells the app layer (and, via
 * treadmill_handler's own th_on_lcb_error_code, this handler itself) that an
 * unlock condition should be treated as having occurred. No-op if not
 * started (comm == NULL) or if nothing is registered on comm.
 */
void treadmill_handler_send_estop_unlock(treadmill_handler_t *th);

/**
 * @brief Manual "EStop Revert" action for when this handler already owns
 * lcb_comm (i.e. after general_handler has handed off post-handshake — see
 * HalEntryPoint.c's hal_tm_gh_on_lcb_initial_finish()). general_handler's own
 * general_handler_estop_revert_lcb_initial() only works pre-handoff, since
 * general_handler is torn down (g_general_handler set to NULL) once TM is
 * running — this is the equivalent for that later window.
 *
 * Re-sends the same 3-command TM init sequence general_handler's
 * MACHINE_CATEGORY_TM branch uses (SetLCBInitial, SetLCBOperatingStatus
 * STOP, SetLCBOperatingStatus ACTION), all highest-priority since the LCB is
 * already known-alive (unlike first bring-up, there is no need to wait for
 * each ACK before queuing the next command), then calls
 * treadmill_handler_send_estop_unlock() for the local UI notification.
 * No-op if not started (comm == NULL).
 */
void treadmill_handler_estop_revert_lcb_initial(treadmill_handler_t *th);

/* ============================================================================
 * Hybrid TM IR Address — stubs for future lcb_comm support
 * ============================================================================ */

void treadmill_handler_get_hybrid_tm_ir_addr_version(treadmill_handler_t *th);
void treadmill_handler_get_hybrid_tm_ir_addr_ir_status(treadmill_handler_t *th);
void treadmill_handler_set_hybrid_tm_ir_addr_enter_erp_mode(treadmill_handler_t *th);
void treadmill_handler_set_hybrid_tm_ir_addr_leave_erp_mode(treadmill_handler_t *th);

#ifdef __cplusplus
}
#endif

#endif /* TREADMILL_HANDLER_H */
