/**
 * @file general_handler.h
 * @brief LCB General Handler — High-level machine orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/IGeneralHandler.java
 *   - com/jht/lcbcommlib/machineHandler/IGeneralHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/GeneralHandler.java
 *   - com/jht/lcbcommlib/machineHandler/GeneralCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer. Manages:
 *   - LCB initialization state machine
 *   - Error handling state machine (B6 error → get error code → skip)
 *   - Heartbeat thread (1s periodic status query, auto-init retry)
 *   - E-Stop handling (force stop motor/incline, revert → re-init)
 *   - Serial number operations
 *   - Peripheral queries (PFC, motor type, MCB voltage, magnet angle)
 */

#ifndef GENERAL_HANDLER_H
#define GENERAL_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"
#include "lcb_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Enums
 * ============================================================================ */

/** LCB initialization state machine */
typedef enum {
    GH_INIT_NONE = 0,
    GH_INIT_CMD_INITIAL,
    GH_INIT_DEFAULT_CMD_INITIAL,
    GH_INIT_CMD_OPERATING_STOP,
    GH_INIT_CMD_OPERATING_ACTION,
    GH_INIT_TM_3_CMDS_INITIAL,
    GH_INIT_FINISH
} gh_init_state_t;

/** Error retrieval state machine */
typedef enum {
    GH_ERROR_NONE = 0,
    GH_ERROR_GET,
    GH_ERROR_SKIP
} gh_error_state_t;

/** Machine category (determined from LCB type) */
typedef enum {
    MACHINE_CATEGORY_UNKNOWN = 0,
    MACHINE_CATEGORY_TM,
    MACHINE_CATEGORY_BIKE
} machine_category_type_t;

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct general_handler general_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IGeneralHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    /* Error & Version */
    void (*on_error_code)(void *user_data, uint16_t error_code);
    void (*on_lcb_type)(void *user_data, uint8_t lcb_type);
    void (*on_lcb_version)(void *user_data, int major, int minor);

    /* Initialization */
    void (*on_lcb_initial_finish)(void *user_data);

    /* Status byte bit callbacks */
    void (*on_incline_motor_downward_status)(void *user_data, bool status);
    void (*on_incline_motor_upward_status)(void *user_data, bool status);
    void (*on_main_motor_status)(void *user_data, bool status);
    void (*on_encoder_rpm_status)(void *user_data, bool status);
    void (*on_battery_capacity_status)(void *user_data, bool status);
    void (*on_command_error_status)(void *user_data, bool status);
    void (*on_initial_calibration_status)(void *user_data, bool status);

    /* Voltage & Status */
    void (*on_battery_voltage)(void *user_data, uint16_t voltage);
    void (*on_lcb_status)(void *user_data, uint8_t status);

    /* Serial number */
    void (*on_clear_lcb_sn_status)(void *user_data, bool success);
    void (*on_set_lcb_sn_status)(void *user_data, bool success);
    void (*on_get_lcb_serial_number)(void *user_data, bool success, const char *sn);
    void (*on_get_lcb_sn_max_length)(void *user_data, bool success, int max_length);

    /* Update & Calibration */
    void (*on_into_lcb_update_mode)(void *user_data, bool success);
    void (*on_lcb_finish_magnet_angle_value)(void *user_data, int angle);
    void (*on_mcb_moto_type)(void *user_data, int moto_type);
    void (*on_get_mcb_voltage_value)(void *user_data, int voltage);

    /* Communication */
    void (*on_lcb_comm_reconnected)(void *user_data);

    /* ERP (console power-save standby) mode — Java: enterLcbErpMode()/
     * leaveLcbErpMode()/erpStopUartPort(), gated there behind
     * ProductConfig.hasIO(); this console app has no such gate, so calling
     * these is entirely up to the caller. */
    void (*on_enter_lcb_erp_mode)(void *user_data, bool success);
    void (*on_leave_lcb_erp_mode)(void *user_data, bool is_lcb_comm_ok, bool success);
    void (*on_boot_up_leave_lcb_erp)(void *user_data, bool success);

} general_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

general_handler_t* general_handler_create(void);
void               general_handler_destroy(general_handler_t *gh);

/* ============================================================================
 * Callback Registration
 * ============================================================================ */

void general_handler_set_callbacks(general_handler_t *gh,
                                   const general_handler_callbacks_t *cb);
void general_handler_unregister_callbacks(general_handler_t *gh);
void general_handler_callbacks_init(general_handler_callbacks_t *cb);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the general handler with LCB communication
 *
 * Wires internal callbacks, starts lcb_comm, requests LCB version,
 * and starts the heartbeat thread.
 *
 * @param gh    General handler instance
 * @param comm  LCB communication instance (NOT owned — caller must keep alive)
 * @param port  Serial port configuration
 * @return true on success
 */
bool general_handler_auto_start(general_handler_t *gh, lcb_comm_t *comm,
                                serial_port_t *port);

void general_handler_auto_stop(general_handler_t *gh);

/**
 * @brief Request the heartbeat thread to stop and detach from lcb_comm,
 * WITHOUT joining the heartbeat thread or freeing @p gh.
 *
 * Safe to call from within the `on_lcb_initial_finish` user callback — that
 * callback is invoked synchronously from inside the heartbeat thread itself
 * (see gh_auto_initial_lcb_cmd()), so general_handler_auto_stop() (which
 * joins) or general_handler_destroy() (which joins and frees) would either
 * deadlock (self-join) or free @p gh while its own thread is still unwinding
 * through it (use-after-free). Use this instead in that specific situation;
 * the thread notices `is_running == false` on its next loop check and exits
 * on its own shortly after the callback returns. The caller intentionally
 * does NOT reclaim @p gh's small heap allocation in that case — see
 * HalEntryPoint.c's TM/Bike bring-up callbacks for the accepted trade-off.
 *
 * @param gh General handler instance
 */
void general_handler_request_stop(general_handler_t *gh);

/* ============================================================================
 * E-Stop & Force Stop
 * ============================================================================ */

void general_handler_estop_machine_stop(general_handler_t *gh);
void general_handler_estop_revert_lcb_initial(general_handler_t *gh);
void general_handler_force_stop_machine(general_handler_t *gh);

/* ============================================================================
 * Control & Auto-Reply
 * ============================================================================ */

/* LCB "console/control type" byte (Command ID 0x6C) — ported from
 * app/src/main/java/com/jht/lcbcommlib/ControlType.java. Tells the LCB
 * hardware which machine category is now controlling it; required once per
 * bring-up for machine types Java actually sends it for (see
 * LCBInterface.setLCBControlType() — Treadmill sends nothing, Bike/Elliptical
 * variants send GH_CONTROL_TYPE_BIKE, etc.). */
#define GH_CONTROL_TYPE_BIKE             0x00
#define GH_CONTROL_TYPE_STEPPER          0x01
#define GH_CONTROL_TYPE_RAMP_EP          0x02
#define GH_CONTROL_TYPE_SWING_EP         0x03
#define GH_CONTROL_TYPE_TM               0x04
#define GH_CONTROL_TYPE_CLIMBMILL        0x05
#define GH_CONTROL_TYPE_RETAIL_CLIMBMILL 0x06
#define GH_CONTROL_TYPE_INDOORBIKE       0x07
#define GH_CONTROL_TYPE_UNKNOWN          0xFF

/**
 * @brief Set the LCB control type — stores it locally AND transmits LCB
 * command 0x6C (lcb_comm_set_lcb_machine_type) if @p gh is currently started
 * (comm != NULL). Mirrors GeneralHandler.procSetControlType() exactly (Java:
 * setMachineControlType() + mLcb.setLcbMachineType()) — this is a real
 * hardware command, not just a local flag.
 */
void general_handler_set_control_type(general_handler_t *gh, uint8_t type);
/** @brief Get the locally-stored control type (Java: GeneralHandler.getControlType()). */
uint8_t general_handler_get_control_type(const general_handler_t *gh);
void general_handler_set_auto_reply_battery(general_handler_t *gh, bool enable);
void general_handler_set_auto_reply_magnet_angle(general_handler_t *gh, bool enable);

/**
 * @brief Set advanced-logging verbosity bits (Java: GeneralHandler.
 * setAdvancedLogging()). This does NOT transmit anything to the LCB — it
 * only stores the two bits and triggers lcb_comm's own internal log-level
 * bookkeeping via lcb_comm_proc_set_adv_logging_status(), exactly mirroring
 * the Java source (mLcb.procSetAdvLoggingStatus() never sends a packet).
 */
void general_handler_set_advanced_logging(general_handler_t *gh, bool log_messages, bool log_message_details);

/* ============================================================================
 * ERP (Standby) Mode
 * ============================================================================ */

/**
 * @brief Ask the LCB to enter ERP/standby mode (Java: GeneralHandler.
 * enterLcbErpMode() -> mLcb.enterLcbErpMode()). Response arrives via
 * general_handler_callbacks_t::on_enter_lcb_erp_mode.
 */
void general_handler_enter_erp_mode(general_handler_t *gh);

/**
 * @brief Wake the LCB and ask it to leave ERP/standby mode (Java:
 * GeneralHandler.leaveLcbErpMode() -> mLcb.leaveLcbErpMode(path, baud)).
 * Sends the wake pulse via the existing lcb_comm_leave_lcb_erp_mode().
 *
 * NOTE: unlike the Java original, this does NOT reopen/reconfigure the
 * serial port — this codebase has no serial_port_open()/close()
 * implementation anywhere (lcb_comm_restart() has the same pre-existing
 * TODO), so physically power-cycling the UART during ERP is out of scope
 * here. The LCB wake pulse itself (do_pulse=true) is unaffected and still
 * sent. Response arrives via general_handler_callbacks_t::on_leave_lcb_erp_mode.
 *
 * @param gh General handler instance
 * @param device_path LCB serial device path (informational only — see NOTE)
 * @param baud_rate Baud rate (informational only — see NOTE)
 */
void general_handler_leave_erp_mode(general_handler_t *gh, const char *device_path, uint32_t baud_rate);

/**
 * @brief Signal the ERP-stop-UART-port thread (Java: GeneralHandler delegates
 * to mLcb.erpStopUartPort()). See the NOTE on general_handler_leave_erp_mode()
 * — the underlying lcb_comm_erp_stop_uart_port() only signals a condition
 * variable today; it does not yet physically close the port (same missing
 * serial_port_close() dependency).
 */
void general_handler_erp_stop_uart_port(general_handler_t *gh);

/* ============================================================================
 * Serial Number Operations
 * ============================================================================ */

void general_handler_clear_serial_number(general_handler_t *gh);
void general_handler_set_serial_number(general_handler_t *gh, const char *serial);
void general_handler_get_serial_number(general_handler_t *gh);
void general_handler_get_mcb_self_serial_number(general_handler_t *gh);
void general_handler_get_sn_max_length(general_handler_t *gh);

/* ============================================================================
 * Peripheral Queries
 * ============================================================================ */

void general_handler_start_magnet_angle_calibration(general_handler_t *gh);
void general_handler_get_magnet_angle(general_handler_t *gh);
void general_handler_set_pfc_mode(general_handler_t *gh, bool enable);
void general_handler_get_moto_type(general_handler_t *gh);
void general_handler_get_mcb_voltage(general_handler_t *gh);
void general_handler_init_lcb_mcb_before_workout(general_handler_t *gh);

/**
 * @brief Tell the LCB to enter firmware-update program mode (Java:
 * GeneralHandler.setLcbIntoUpdateProgramMode() -> mLcb.setLcbUpdateProgram("JS")).
 * Response arrives via general_handler_callbacks_t::on_into_lcb_update_mode.
 */
void general_handler_set_lcb_into_update_program_mode(general_handler_t *gh);

/* ============================================================================
 * Getters
 * ============================================================================ */

bool                    general_handler_get_lcb_comm_disconnect_state(const general_handler_t *gh);
LcbType_t               general_handler_get_lcb_type(const general_handler_t *gh);
machine_category_type_t general_handler_get_category(const general_handler_t *gh);

/**
 * @brief Read back the 5 induction-brake PWM calibration points gathered
 * during LCB-type detection (Java: static FrameConfig.inductionCalPoints[]).
 * @param gh General handler instance
 * @param out_points Caller-provided array of 5 ints to receive the values
 * @return true if induction calibration data has been fully read
 * (has_induction_cal) — @p out_points is only populated when true
 */
bool general_handler_get_induction_cal_points(const general_handler_t *gh, int out_points[5]);

/** @brief Whether the 5 induction cal points have been fully read (Java: FrameConfig.hasInductionCal). */
bool general_handler_has_induction_cal(const general_handler_t *gh);

/** @brief Whether the read-back induction cal points passed monotonicity validation (Java: FrameConfig.validInductionCal / validateCalPoints()). */
bool general_handler_is_induction_cal_valid(const general_handler_t *gh);

#ifdef __cplusplus
}
#endif

#endif /* GENERAL_HANDLER_H */
