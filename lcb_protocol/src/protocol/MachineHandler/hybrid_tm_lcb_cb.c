/**
 * @file hybrid_tm_lcb_cb.c
 * @brief LCB Callback Bridge for Hybrid TM Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/HybridTM/HybridTmCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives hybrid_tm_handler_t* via user_data and dispatches to
 * the user-facing callbacks. Java's HybridTmCallbackHandlerLCB overrides the
 * full ~130-method lcb_comm-receive interface but only 5 do anything
 * (getLcbErrorCodeHandler / getLcbMotorRPMHandler / getLcbInclineLocHandler /
 * getLcbTreadmillInUsedHandler / getLcbMainMotorInfoHandler) — those are
 * wired here; the rest are intentionally not bridged, matching the Java
 * no-ops.
 *
 * Additionally wires the 4 IR Address board (0x26) callbacks
 * (on_ir_addr_version/_ir_status/_enter_erp_mode/_leave_erp_mode) — these
 * have no Java counterpart in chimera2's own HybridTmHandler/
 * HybridTmCallbackHandlerLCB; they're ported here from chimera2's
 * TreadMillInterface/TreadMillHandler per explicit user direction (see
 * hybrid_tm_handler.h's file-header NOTE for the full explanation).
 */

#include "hybrid_tm_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_proc_comm.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "HybridTmHandlerLcbCb"

/* 0xF1 GetMotorRPM (auto-poll response) */
static void hth_on_lcb_motor_rpm_received(void *ud, uint16_t rpm)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    pthread_mutex_lock(&hth->mutex);
    hth->machine_rpm = rpm;
    if (hth->user_cb.on_hybrid_tm_rpm)
        hth->user_cb.on_hybrid_tm_rpm(hth->user_cb.user_data, rpm);
    pthread_mutex_unlock(&hth->mutex);
}

/* 0xF6 GetInclineLoc (auto-poll response) */
static void hth_on_lcb_incline_loc_received(void *ud, uint16_t loc)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    if (hth->user_cb.on_hybrid_tm_incline_loc)
        hth->user_cb.on_hybrid_tm_incline_loc(hth->user_cb.user_data, loc);
}

/* Treadmill InUsed */
static void hth_on_lcb_treadmill_in_used(void *ud, uint8_t status)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    if (hth->user_cb.on_hybrid_tm_in_used)
        hth->user_cb.on_hybrid_tm_in_used(hth->user_cb.user_data, status);
}

/* Main Motor Info */
static void hth_on_lcb_main_motor_info(void *ud, const uint8_t *info, uint8_t length)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    if (hth->user_cb.on_hybrid_tm_motor_info)
        hth->user_cb.on_hybrid_tm_motor_info(hth->user_cb.user_data, info, length);
}

/* ============================================================================
 * Error Code — clears target_rpm_check_flag on 0x02B2 (TM no-RPM error), and
 * (per hybrid_tm_handler.h's file-header NOTE) triggers the IR-EStop
 * disambiguation check ported from chimera2's LCBInterface.onErrorCode():
 * `if (lcbErrorCode == 0x02B2) { if (isHybridTmIrEnabled() &&
 * !mTreadmill.isHybridTmIrEStopTriggered()) mTreadmill.triggerHybridTmIrEStopCheck(); }`
 * — isHybridTmIrEnabled() is unconditionally true here (see file-header NOTE).
 * ============================================================================ */

static void hth_on_lcb_error_code(void *ud, uint16_t error_code)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    pthread_mutex_lock(&hth->mutex);
    if (error_code == LCB_ERROR_E_STOP) {
        hth->target_rpm_check_flag = false;
        LCB_LOGD(TAG, "0x02B2 received, clearing target_rpm_check_flag");

        if (!hth->ir_estop_triggered) {
            hth->pending_ir_estop_check = true;
            if (hth->comm != NULL) {
                lcb_comm_get_ir_addr_ir_status_highest_priority(hth->comm);
                LCB_LOGD(TAG, "Querying IR Addr board status to disambiguate E-Stop cause");
            }
        }
    }
    pthread_mutex_unlock(&hth->mutex);
}

/* ============================================================================
 * IR Address Board (0x26) — see hybrid_tm_handler.h file-header NOTE
 * ============================================================================ */

/* 0x73 GetLCBVersion (IR Address board variant) */
static void hth_on_ir_addr_version(void *ud, int version)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    if (hth->user_cb.on_hybrid_tm_ir_addr_version)
        hth->user_cb.on_hybrid_tm_ir_addr_version(hth->user_cb.user_data, version);
}

/* 0x99 GetIrAddressIrStatus — also drives the IR-EStop state machine, ported
 * from chimera2's TreadMillInterface.onHybridTmIrAddrIrStatus(int):
 *   if (pendingIrEStopCheck) {
 *       pendingIrEStopCheck = false;
 *       if (status == 0x01) { wasTriggered = true; onHybridTmIrAddrIrEStop(true); }
 *       else if (status == 0x00) { onLCBSafetykeyLockedCode(true); }
 *   } else if (wasTriggered && status == 0x00) {
 *       wasTriggered = false; onHybridTmIrAddrIrEStop(false); onTreadmillStartUnlock();
 *   }
 *   onHybridTmIrAddrIrStatus(status); // always, regardless of the above
 */
static void hth_on_ir_addr_ir_status(void *ud, uint8_t status)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    pthread_mutex_lock(&hth->mutex);

    if (hth->pending_ir_estop_check) {
        hth->pending_ir_estop_check = false;
        if (status == HYBRID_TM_IR_STATUS_BLOCKED) {
            hth->ir_estop_triggered = true;
            LCB_LOGD(TAG, "IR beam blocked -> Hybrid TM IR-EStop triggered");
            if (hth->user_cb.on_hybrid_tm_ir_estop)
                hth->user_cb.on_hybrid_tm_ir_estop(hth->user_cb.user_data, true);
        } else if (status == HYBRID_TM_IR_STATUS_CLEAR) {
            LCB_LOGD(TAG, "IR beam clear -> normal E-Stop (safety key)");
            if (hth->user_cb.on_hybrid_tm_safetykey_locked)
                hth->user_cb.on_hybrid_tm_safetykey_locked(hth->user_cb.user_data, true);
        }
    } else if (hth->ir_estop_triggered && status == HYBRID_TM_IR_STATUS_CLEAR) {
        hth->ir_estop_triggered = false;
        LCB_LOGD(TAG, "IR beam cleared -> Hybrid TM IR-EStop recovered");
        if (hth->user_cb.on_hybrid_tm_ir_estop)
            hth->user_cb.on_hybrid_tm_ir_estop(hth->user_cb.user_data, false);
        if (hth->user_cb.on_hybrid_tm_start_unlock)
            hth->user_cb.on_hybrid_tm_start_unlock(hth->user_cb.user_data);
    }

    if (hth->user_cb.on_hybrid_tm_ir_addr_ir_status)
        hth->user_cb.on_hybrid_tm_ir_addr_ir_status(hth->user_cb.user_data, status);

    pthread_mutex_unlock(&hth->mutex);
}

/* 0x7A SetLCBCmdEUPsMode (IR Address board variant) — enter ERP success/fail */
static void hth_on_ir_addr_enter_erp_mode(void *ud, bool success)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    if (hth->user_cb.on_hybrid_tm_ir_addr_enter_erp_mode)
        hth->user_cb.on_hybrid_tm_ir_addr_enter_erp_mode(hth->user_cb.user_data, success);
}

/* 0x7A SetLCBCmdEUPsMode (IR Address board variant) — leave ERP success/fail */
static void hth_on_ir_addr_leave_erp_mode(void *ud, bool success)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    if (hth->user_cb.on_hybrid_tm_ir_addr_leave_erp_mode)
        hth->user_cb.on_hybrid_tm_ir_addr_leave_erp_mode(hth->user_cb.user_data, success);
}

/* 0x7A SetLCBCmdEUPsMode (IR Address board variant) — raw response byte,
 * forwarded unconditionally regardless of enter/leave context (Java:
 * setIrAddrCmdEUPsModeHandler -> onHybridTmIrAddrErpRtnByte). */
static void hth_on_ir_addr_erp_rtn_byte(void *ud, uint8_t byte_rtn)
{
    hybrid_tm_handler_t *hth = (hybrid_tm_handler_t *)ud;
    if (hth->user_cb.on_hybrid_tm_ir_addr_erp_rtn_byte)
        hth->user_cb.on_hybrid_tm_ir_addr_erp_rtn_byte(hth->user_cb.user_data, byte_rtn);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void hth_lcb_cb_register(hybrid_tm_handler_t *hth)
{
    lcb_comm_callbacks_init(&hth->lcb_cb);
    hth->lcb_cb.user_data = hth;

    hth->lcb_cb.on_lcb_motor_rpm_received   = hth_on_lcb_motor_rpm_received;
    hth->lcb_cb.on_lcb_incline_loc_received = hth_on_lcb_incline_loc_received;
    hth->lcb_cb.on_lcb_treadmill_in_used    = hth_on_lcb_treadmill_in_used;
    hth->lcb_cb.on_lcb_main_motor_info      = hth_on_lcb_main_motor_info;
    hth->lcb_cb.on_lcb_error_code           = hth_on_lcb_error_code;

    hth->lcb_cb.on_ir_addr_version          = hth_on_ir_addr_version;
    hth->lcb_cb.on_ir_addr_ir_status        = hth_on_ir_addr_ir_status;
    hth->lcb_cb.on_ir_addr_enter_erp_mode   = hth_on_ir_addr_enter_erp_mode;
    hth->lcb_cb.on_ir_addr_leave_erp_mode   = hth_on_ir_addr_leave_erp_mode;
    hth->lcb_cb.on_ir_addr_erp_rtn_byte     = hth_on_ir_addr_erp_rtn_byte;
}

void hth_lcb_cb_unregister(hybrid_tm_handler_t *hth)
{
    memset(&hth->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
