/**
 * @file hybrid_tm_handler.h
 * @brief LCB Hybrid TM Handler — HybridTM machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/HybridTM/IHybridTmHandler.java
 *   - com/jht/lcbcommlib/machineHandler/HybridTM/IHybridTmHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/HybridTM/HybridTmHandler.java
 *   - com/jht/lcbcommlib/machineHandler/HybridTM/HybridTmCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent).
 * Manages:
 *   - Heartbeat thread @ 400ms driving auto-reply polling
 *   - 4 auto-reply features: RPM, Incline, InUsed (gated on machine_rpm > 100),
 *     MotorInfo
 *   - No-RPM-error timeout (40s = 100 ticks at 400ms)
 *   - No LCB-type-dependent branching — machineLcbType is dead code in the
 *     Java source (declared, never read or written), confirmed against both
 *     this project's and chimera2's HybridTmHandler.java.
 *
 * NOTE: This project's local Java source has
 * timerStartAutoHeartBeatThread()'s body fully commented out, so the
 * heartbeat thread never actually starts there. Cross-checked against the
 * chimera2 reference project's HybridTmHandler.java (ground truth for this
 * file) where it IS fully implemented — confirming this is an incomplete
 * local port, not intentional design. This C port implements a working
 * heartbeat thread, matching chimera2's true behavior and every other
 * converted machine.
 *
 * NOTE (acknowledged, deliberate gap): chimera2's HeartBeatThread.run() also
 * gates every autoReplyCmd() invocation behind
 * `if (!FrictionTestManagerKt.isSendingData())` — auto-reply is paused while
 * a friction test is transmitting. FrictionTestManagerKt (and the friction-
 * test subsystem it belongs to) does not exist anywhere in this project
 * (confirmed by a project-wide search), so there is nothing to port this
 * guard onto. hth_proc_auto_reply_tick() therefore fires unconditionally
 * every heartbeat period, with no equivalent suppression. This is a known,
 * accepted divergence from chimera2 ground truth, not an oversight.
 *
 * NOTE (architectural divergence, by explicit user direction): the "IR
 * Address board" (0x26) IR beam E-Stop feature — GetLCBVersion(0x73)/
 * GetIrAddressIrStatus(0x99)/SetLCBCmdEUPsMode(0x7A) at bus address 0x26,
 * plus the pendingIrEStopCheck/wasHybridTmIrEStopTriggered state machine —
 * is, in chimera2, entirely owned by the plain Treadmill machine
 * (TreadMillInterface.java / TreadMillHandler.java), gated by
 * FrameConfig.HAS_IR_SENSOR, and has NOTHING to do with HybridTmHandler
 * there (confirmed by reading chimera2's HybridTmHandler.java: zero
 * IR-related code). The user explicitly asked for this feature to be
 * implemented on hybrid_tm_handler instead, so it lives here rather than on
 * treadmill_handler (which still carries its own long-standing, unrelated
 * on_hybrid_tm_ir_addr_* stubs — see treadmill_handler.h — left untouched).
 * Since this handler only ever runs for Hybrid-TM-class machines, and no
 * FrameConfig/capabilities plumbing exists for HybridTm in this port at all,
 * the feature is unconditionally enabled here rather than gated behind a
 * fabricated capability flag.
 */

#ifndef HYBRID_TM_HANDLER_H
#define HYBRID_TM_HANDLER_H

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

typedef struct hybrid_tm_handler hybrid_tm_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IHybridTmHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_hybrid_tm_rpm)(void *ud, uint16_t rpm);
    void (*on_hybrid_tm_incline_loc)(void *ud, uint16_t loc);
    void (*on_hybrid_tm_in_used)(void *ud, uint8_t status);
    void (*on_hybrid_tm_motor_info)(void *ud, const uint8_t *info, uint8_t length);

    /* IR Address board (0x26) raw responses — see the file-header NOTE on
     * why this lives here rather than on treadmill_handler. */
    void (*on_hybrid_tm_ir_addr_version)(void *ud, int version);
    void (*on_hybrid_tm_ir_addr_ir_status)(void *ud, uint8_t status);
    void (*on_hybrid_tm_ir_addr_enter_erp_mode)(void *ud, bool success);
    void (*on_hybrid_tm_ir_addr_leave_erp_mode)(void *ud, bool success);
    /* Raw SetLCBCmdEUPsMode response byte (0x7A @ 0x26) — Java:
     * setIrAddrCmdEUPsModeHandler(byte) -> onHybridTmIrAddrErpRtnByte,
     * fires unconditionally alongside the enter/leave callbacks above. */
    void (*on_hybrid_tm_ir_addr_erp_rtn_byte)(void *ud, uint8_t byte_rtn);

    /* IR-EStop state-machine events (mirrors chimera2 TreadMillInterface's
     * serviceCallback.onHybridTmIrAddrIrEStop(bool) /
     * onLCBSafetykeyLockedCode(bool) / onTreadmillStartUnlock()) */
    void (*on_hybrid_tm_ir_estop)(void *ud, bool triggered);
    void (*on_hybrid_tm_safetykey_locked)(void *ud, bool locked);
    void (*on_hybrid_tm_start_unlock)(void *ud);
} hybrid_tm_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

hybrid_tm_handler_t* hybrid_tm_handler_create(void);
void                 hybrid_tm_handler_destroy(hybrid_tm_handler_t *hth);

void hybrid_tm_handler_callbacks_init(hybrid_tm_handler_callbacks_t *cb);
void hybrid_tm_handler_set_callbacks(hybrid_tm_handler_t *hth,
                                     const hybrid_tm_handler_callbacks_t *cb);
void hybrid_tm_handler_unregister_callbacks(hybrid_tm_handler_t *hth);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the hybrid TM handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration) and
 * starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param hth   Hybrid TM handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool hybrid_tm_handler_start(hybrid_tm_handler_t *hth, lcb_comm_t *comm);

void hybrid_tm_handler_stop(hybrid_tm_handler_t *hth);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

void hybrid_tm_handler_set_rpm(hybrid_tm_handler_t *hth, uint16_t rpm);
void hybrid_tm_handler_set_incline(hybrid_tm_handler_t *hth, uint16_t loc);
void hybrid_tm_handler_set_incline_action(hybrid_tm_handler_t *hth, uint8_t action);
void hybrid_tm_handler_set_motor_type(hybrid_tm_handler_t *hth, uint16_t watts);

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

void hybrid_tm_handler_set_auto_reply_rpm(hybrid_tm_handler_t *hth, bool enable);
void hybrid_tm_handler_set_auto_reply_incline(hybrid_tm_handler_t *hth, bool enable);
void hybrid_tm_handler_set_auto_reply_in_used(hybrid_tm_handler_t *hth, bool enable);
void hybrid_tm_handler_set_auto_reply_motor_info(hybrid_tm_handler_t *hth, bool enable);

/* ============================================================================
 * IR Address Board (0x26) — "Hybrid TM" IR beam E-Stop sensor
 *
 * See the file-header NOTE for why this feature lives here instead of on
 * treadmill_handler (chimera2's actual architecture) — implemented here per
 * explicit user direction.
 * ============================================================================ */

/** One-shot version query (0x73 @ 0x26). Java: getHybridTMIrAddrVersion(). */
void hybrid_tm_handler_get_ir_addr_version(hybrid_tm_handler_t *hth);

/** One-shot, highest-priority IR status query (0x99 @ 0x26), used for manual
 *  testing. Java: getHybridTMIrAddrIrStatus(). */
void hybrid_tm_handler_get_ir_addr_ir_status(hybrid_tm_handler_t *hth);

/** Enter/leave ERP (standby) mode on the IR board (0x7A @ 0x26). */
void hybrid_tm_handler_set_ir_addr_enter_erp_mode(hybrid_tm_handler_t *hth);
void hybrid_tm_handler_set_ir_addr_leave_erp_mode(hybrid_tm_handler_t *hth);

/** Toggle periodic auto-poll of the IR beam status (every heartbeat tick,
 *  matching chimera2's TIMES_IR_ADDR_IR_STATUS_INTERVAL = 1). Java:
 *  setHybridTMIrAddrIrDetection(bool). */
void hybrid_tm_handler_set_auto_reply_ir_addr_ir_status(hybrid_tm_handler_t *hth, bool enable);

/** True while the IR beam is blocked and the resulting E-Stop is active
 *  (mirrors chimera2's TreadMillInterface.isHybridTmIrEStopTriggered()). */
bool hybrid_tm_handler_is_ir_estop_triggered(hybrid_tm_handler_t *hth);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to hybrid_tm_handler; see the matching TM
 * doc comment in treadmill_handler.h for the full explanation). LCB_TYPE_
 * DELTA_HYBRID maps to MACHINE_CATEGORY_TM in general_handler.c's
 * map_lcb_type_to_category(), so this re-sends the same 3-command TM init
 * sequence general_handler's MACHINE_CATEGORY_TM branch uses (SetLCBInitial,
 * SetLCBOperatingStatus STOP, SetLCBOperatingStatus ACTION), all
 * highest-priority since the LCB is already known-alive, then calls
 * lcb_comm_proc_estop_unlock() for the local UI notification. No-op if not
 * started (comm == NULL).
 */
void hybrid_tm_handler_estop_revert_lcb_initial(hybrid_tm_handler_t *hth);

#ifdef __cplusplus
}
#endif

#endif /* HYBRID_TM_HANDLER_H */
