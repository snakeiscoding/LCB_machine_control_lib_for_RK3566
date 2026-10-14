/**
 * @file general_handler_lcb_cb.c
 * @brief LCB Callback Bridge for General Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/GeneralCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives general_handler_t* via user_data, locks mutex, processes,
 * and dispatches to user-facing callbacks.
 */

#include "general_handler.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_constants.h"
#include "lcb_proc_comm.h"
#include "lcb_universal_data.h"
#include "lcb_device_data.h"
#include "lcb_logging.h"
#include <string.h>
#include <pthread.h>

#define TAG "GeneralHandlerLcbCb"

/* ============================================================================
 * Access to internal struct (defined in general_handler.c)
 * We use extern declarations for the internal helper functions.
 * ============================================================================ */

/* Internal functions from general_handler.c */
extern void gh_proc_initial_lcb(general_handler_t *gh);
extern void gh_auto_initial_lcb_cmd(general_handler_t *gh);
extern bool gh_check_in_get_error_state(general_handler_t *gh);
extern void gh_run_get_error_processes(general_handler_t *gh);
extern void gh_handle_exception_error_code(general_handler_t *gh, uint16_t error_code);
extern void gh_set_machine_lcb_type(general_handler_t *gh, uint8_t type_byte);
extern void gh_set_reconnect_deadline(general_handler_t *gh);
extern bool gh_validate_cal_points(general_handler_t *gh);

/* Access struct fields — include full struct definition */
#include "general_handler_internal.h"

/* ============================================================================
 * LCB Comm Callbacks
 * ============================================================================ */

/* 0x70 SetLCBInitialize response — drive init state machine */
static void gh_on_lcb_initial(void *ud)
{
    general_handler_t *gh = (general_handler_t *)ud;
    pthread_mutex_lock(&gh->mutex);
    LCB_LOGD(TAG, "on_lcb_initial: init_state=%d", gh->init_state);

    switch (gh->init_state) {
        case GH_INIT_TM_3_CMDS_INITIAL:
            gh->initial_cmd_done = true;
            gh->init_state = GH_INIT_CMD_OPERATING_STOP;
            gh_auto_initial_lcb_cmd(gh);
            break;
        case GH_INIT_CMD_INITIAL:
            gh->initial_cmd_done = true;
            gh->init_state = GH_INIT_CMD_OPERATING_STOP;
            gh_auto_initial_lcb_cmd(gh);
            break;
        case GH_INIT_DEFAULT_CMD_INITIAL:
            gh->default_initial_cmd_done = true;
            gh->init_state = GH_INIT_FINISH;
            gh_auto_initial_lcb_cmd(gh);
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* 0xF7 SetLCBOperatingStatus response — drive init state machine */
static void gh_on_lcb_operating_status(void *ud)
{
    general_handler_t *gh = (general_handler_t *)ud;
    pthread_mutex_lock(&gh->mutex);
    LCB_LOGD(TAG, "on_lcb_operating_status: init_state=%d", gh->init_state);

    switch (gh->init_state) {
        case GH_INIT_CMD_OPERATING_STOP:
            gh->operating_cmd_done = false; /* Reset for next phase */
            gh->init_state = GH_INIT_CMD_OPERATING_ACTION;
            gh_auto_initial_lcb_cmd(gh);
            break;
        case GH_INIT_CMD_OPERATING_ACTION:
            gh->operating_cmd_done = true;
            gh->init_state = GH_INIT_FINISH;
            gh_auto_initial_lcb_cmd(gh);
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* B6 error status detected — enter error state machine */
static void gh_on_lcb_error_status_detected(void *ud, bool has_error)
{
    general_handler_t *gh = (general_handler_t *)ud;
    pthread_mutex_lock(&gh->mutex);

    if (has_error && !gh_check_in_get_error_state(gh)) {
        LCB_LOGD(TAG, "B6 error detected, entering GET state");
        gh->error_state = GH_ERROR_GET;
        gh_run_get_error_processes(gh);

        if (gh->skip_error_stage)
            gh->initial_have_error = true;
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* 0x72 GetLCBErrorCode response */
static void gh_on_lcb_error_code(void *ud, uint16_t error_code)
{
    general_handler_t *gh = (general_handler_t *)ud;
    pthread_mutex_lock(&gh->mutex);
    LCB_LOGD(TAG, "on_lcb_error_code: 0x%04X", error_code);

    if (error_code == LCB_ERROR_COMM_LCB) {
        /* 0x04B0: log only, do not process further */
        LCB_LOGD(TAG, "Error 04B0 occurred (comm timeout)");
    } else {
        gh_handle_exception_error_code(gh, error_code);

        if (!gh->safe_key_stop_machine &&
            gh->error_state == GH_ERROR_GET) {
            gh->error_state = GH_ERROR_SKIP;
            gh_run_get_error_processes(gh);
        }
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* Error code to UI callback */
static void gh_on_return_error_code_to_ui(void *ud, uint16_t error_code)
{
    general_handler_t *gh = (general_handler_t *)ud;
    pthread_mutex_lock(&gh->mutex);

    /* Filter during skip_error_stage (initial phase) */
    if (gh->initial_have_error && gh->skip_error_stage && error_code != LCB_ERROR_E_STOP) {
        LCB_LOGD(TAG, "Filtering error 0x%04X during skip_error_stage", error_code);
        pthread_mutex_unlock(&gh->mutex);
        return;
    }

    gh->last_error_code = error_code;
    if (gh->user_cb.on_error_code != NULL)
        gh->user_cb.on_error_code(gh->user_cb.user_data, error_code);

    pthread_mutex_unlock(&gh->mutex);
}

/* 0x73 GetLCBVersion response */
static void gh_on_lcb_version(void *ud, uint16_t version)
{
    general_handler_t *gh = (general_handler_t *)ud;
    pthread_mutex_lock(&gh->mutex);
    LCB_LOGD(TAG, "on_lcb_version: 0x%04X", version);

    if (version == 0xFF00) {
        /* Special: request device information instead */
        lcb_dd_get_lcb_information(gh->comm);
        pthread_mutex_unlock(&gh->mutex);
        return;
    }

    uint8_t type_byte = (uint8_t)(version >> 8);
    uint8_t ver_byte = (uint8_t)(version & 0xFF);

    /* Set machine type */
    gh_set_machine_lcb_type(gh, type_byte);

    if (gh->user_cb.on_lcb_type != NULL)
        gh->user_cb.on_lcb_type(gh->user_cb.user_data, type_byte);

    if (gh->user_cb.on_lcb_version != NULL)
        gh->user_cb.on_lcb_version(gh->user_cb.user_data, type_byte, ver_byte);

    /* Johnson MAB boards (type & 0xA0 == 0xA0, or type & 0xF0 == 0xF0) also
     * expose a device-data LCB version (0x50/0x06) — fetch it too. */
    if (((type_byte & 0xA0) == 0xA0) || ((type_byte & 0xF0) == 0xF0)) {
        if (gh->comm != NULL)
            lcb_dd_get_lcb_version_highest_priority(gh->comm);
    }

    /* Trigger initialization if not finished */
    if (!gh->initial_finish) {
        gh_proc_initial_lcb(gh);
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* 0x75 SetSkipCurrError response */
static void gh_on_lcb_skip_curr_error(void *ud)
{
    general_handler_t *gh = (general_handler_t *)ud;
    pthread_mutex_lock(&gh->mutex);

    if (gh->error_state == GH_ERROR_SKIP) {
        gh->error_state = GH_ERROR_NONE;
        LCB_LOGD(TAG, "Error skip completed, state → NONE");
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* 0x71 GetLCBStatus response */
static void gh_on_lcb_status(void *ud, uint8_t status)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_lcb_status != NULL)
        gh->user_cb.on_lcb_status(gh->user_cb.user_data, status);
}

/* Battery voltage response */
static void gh_on_battery_voltage(void *ud, uint16_t voltage)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_battery_voltage != NULL)
        gh->user_cb.on_battery_voltage(gh->user_cb.user_data, voltage);
}

/* ============================================================================
 * Status Byte Bit Change Callbacks (simple forwarding)
 * ============================================================================ */

static void gh_on_incline_down(void *ud, bool s) {
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_incline_motor_downward_status)
        gh->user_cb.on_incline_motor_downward_status(gh->user_cb.user_data, s);
}
static void gh_on_incline_up(void *ud, bool s) {
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_incline_motor_upward_status)
        gh->user_cb.on_incline_motor_upward_status(gh->user_cb.user_data, s);
}
static void gh_on_main_motor(void *ud, bool s) {
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_main_motor_status)
        gh->user_cb.on_main_motor_status(gh->user_cb.user_data, s);
}
static void gh_on_encoder_rpm(void *ud, bool s) {
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_encoder_rpm_status)
        gh->user_cb.on_encoder_rpm_status(gh->user_cb.user_data, s);
}
static void gh_on_battery_cap(void *ud, bool s) {
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_battery_capacity_status)
        gh->user_cb.on_battery_capacity_status(gh->user_cb.user_data, s);
}
static void gh_on_cmd_error(void *ud, bool s) {
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_command_error_status)
        gh->user_cb.on_command_error_status(gh->user_cb.user_data, s);
}
static void gh_on_initial_cal(void *ud, bool s) {
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_initial_calibration_status)
        gh->user_cb.on_initial_calibration_status(gh->user_cb.user_data, s);
}

/* ============================================================================
 * Serial Number Callback (0x7D)
 * ============================================================================ */

static void gh_on_lcb_cmd_serial_number(void *ud, const uint8_t *data, uint8_t length)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (length < 2) return;

    uint8_t handling_way = data[0];
    uint8_t status_byte = data[1];
    bool success = (status_byte == 0x00);

    pthread_mutex_lock(&gh->mutex);

    switch (handling_way) {
        case 0x00: /* Erase (clear) */
            if (gh->user_cb.on_clear_lcb_sn_status)
                gh->user_cb.on_clear_lcb_sn_status(gh->user_cb.user_data, success);
            break;
        case 0x01: /* Write (set) */
            if (gh->user_cb.on_set_lcb_sn_status)
                gh->user_cb.on_set_lcb_sn_status(gh->user_cb.user_data, success);
            break;
        case 0x02: /* Read */
        case 0x04: /* Read MCB self */
            if (success && length > 3) {
                uint8_t sn_len = data[2];
                if (sn_len > 0 && (3 + sn_len) <= length) {
                    char sn_buf[256] = {0}; /* uint8_t sn_len <= 255, always fits */
                    memcpy(sn_buf, &data[3], sn_len);
                    sn_buf[sn_len] = '\0';
                    if (gh->user_cb.on_get_lcb_serial_number)
                        gh->user_cb.on_get_lcb_serial_number(gh->user_cb.user_data, true, sn_buf);
                } else {
                    if (gh->user_cb.on_get_lcb_serial_number)
                        gh->user_cb.on_get_lcb_serial_number(gh->user_cb.user_data, false, "");
                }
            } else {
                if (gh->user_cb.on_get_lcb_serial_number)
                    gh->user_cb.on_get_lcb_serial_number(gh->user_cb.user_data, false, "");
            }
            break;
        case 0x03: /* Max length */
            if (success && length >= 4) {
                int max_len = ((int)data[2] << 8) | (int)data[3];
                if (gh->user_cb.on_get_lcb_sn_max_length)
                    gh->user_cb.on_get_lcb_sn_max_length(gh->user_cb.user_data, true, max_len);
            } else {
                if (gh->user_cb.on_get_lcb_sn_max_length)
                    gh->user_cb.on_get_lcb_sn_max_length(gh->user_cb.user_data, false, 0);
            }
            break;
        default:
            break;
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* ============================================================================
 * ERP (Standby) Mode Callbacks
 * ============================================================================ */

static void gh_on_enter_lcb_erp_mode(void *ud, bool success)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_enter_lcb_erp_mode)
        gh->user_cb.on_enter_lcb_erp_mode(gh->user_cb.user_data, success);
}

static void gh_on_leave_lcb_erp_mode(void *ud, bool is_lcb_comm_ok, bool success)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_leave_lcb_erp_mode)
        gh->user_cb.on_leave_lcb_erp_mode(gh->user_cb.user_data, is_lcb_comm_ok, success);
}

static void gh_on_boot_up_leave_lcb_erp(void *ud, bool success)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_boot_up_leave_lcb_erp)
        gh->user_cb.on_boot_up_leave_lcb_erp(gh->user_cb.user_data, success);
}

/* ============================================================================
 * Update Program Handler (0x74)
 * ============================================================================ */

static void gh_on_lcb_update_program(void *ud, uint8_t response)
{
    general_handler_t *gh = (general_handler_t *)ud;
    bool success = (response == 0x59);
    if (gh->user_cb.on_into_lcb_update_mode)
        gh->user_cb.on_into_lcb_update_mode(gh->user_cb.user_data, success);
}

/* ============================================================================
 * Device Data Callbacks
 * ============================================================================ */

/* Device 0x50 sub-cmd 0x06: GetLCBversion → extract major/minor */
static void gh_on_dd_get_lcb_version(void *ud, const uint8_t *data, uint8_t length)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (length < 4) return;

    /* Major = data[0..1] BE, Minor = data[2..3] BE */
    int major = ((int)data[0] << 8) | (int)data[1];
    int minor = ((int)data[2] << 8) | (int)data[3];

    if (major != 0 || minor != 0) {
        if (gh->user_cb.on_lcb_version)
            gh->user_cb.on_lcb_version(gh->user_cb.user_data, major, minor);
    }
}

/* Device 0x50 sub-cmd 0xF0: GetLCBinformation → extract type, trigger init
 *
 * Mirrors Java's two-branch deviceGetLCBinformationHandler() exactly:
 *   - length == 4 (JIS 1x Bike-style): type = data[0], version = (data[2], -1)
 *   - otherwise (general case):        type = data[1], version = (data[3],
 *     and for lcbType 0x1B/0x1D only, an extra minor byte at data[4])
 */
static void gh_on_dd_get_lcb_information(void *ud, const uint8_t *data, uint8_t length)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (length < 1) return;

    pthread_mutex_lock(&gh->mutex);

    uint8_t lcb_type;
    if (length == 4) {
        lcb_type = data[0];
        gh_set_machine_lcb_type(gh, lcb_type);

        if (gh->user_cb.on_lcb_type)
            gh->user_cb.on_lcb_type(gh->user_cb.user_data, lcb_type);

        if (gh->user_cb.on_lcb_version)
            gh->user_cb.on_lcb_version(gh->user_cb.user_data, (int)data[2], -1);
    } else {
        if (length < 2) {
            pthread_mutex_unlock(&gh->mutex);
            return;
        }
        lcb_type = data[1];
        gh_set_machine_lcb_type(gh, lcb_type);

        if (gh->user_cb.on_lcb_type)
            gh->user_cb.on_lcb_type(gh->user_cb.user_data, lcb_type);

        if (length >= 4 && gh->user_cb.on_lcb_version) {
            if ((lcb_type == 0x1B || lcb_type == 0x1D) && length >= 5) {
                gh->user_cb.on_lcb_version(gh->user_cb.user_data, (int)data[3], (int)data[4]);
            } else {
                gh->user_cb.on_lcb_version(gh->user_cb.user_data, (int)data[3], -1);
            }
        }
    }

    /* Trigger initialization if not finished */
    if (!gh->initial_finish) {
        gh_proc_initial_lcb(gh);
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* Device 0x50 sub-cmd 0x81: ReadPWM → accumulate 5 induction cal points
 * (Java: GeneralCallbackHandlerLCB.deviceReadPWMHandler()) */
static void gh_on_dd_read_pwm(void *ud, const uint8_t *data, uint8_t length)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (length < 2) return;

    pthread_mutex_lock(&gh->mutex);

    int pwm = ((int)data[0] << 8) | (int)data[1];
    if (gh->cal_point >= 0 && gh->cal_point < 5) {
        gh->induction_cal_points[gh->cal_point] = pwm;
        LCB_LOGD(TAG, "read lcb pwm (%d) = %d", gh->cal_point, pwm);
        gh->cal_point++;
        if (gh->cal_point >= 5) {
            gh->cal_point = 0;
            gh->has_induction_cal = true;
            if (gh_validate_cal_points(gh))
                gh->valid_induction_cal = true;
        }
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* ============================================================================
 * Universal Data Callbacks
 * ============================================================================ */

static void gh_on_ud_moto_type(void *ud, int moto_type)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (gh->user_cb.on_mcb_moto_type)
        gh->user_cb.on_mcb_moto_type(gh->user_cb.user_data, moto_type);
}

static void gh_on_ud_magnet_angle(void *ud, const uint8_t *data, uint8_t length)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (length < 2) return;

    int angle = ((int)data[0] << 8) | (int)data[1];

    /* If not 0xFFFF, calibration done — stop auto-reply and report */
    if (angle != 0xFFFF) {
        pthread_mutex_lock(&gh->mutex);
        gh->is_auto_reply_magnet_angle = false;
        pthread_mutex_unlock(&gh->mutex);

        if (gh->user_cb.on_lcb_finish_magnet_angle_value)
            gh->user_cb.on_lcb_finish_magnet_angle_value(gh->user_cb.user_data, angle);
    }
}

static void gh_on_ud_mcb_voltage(void *ud, const uint8_t *data, uint8_t length)
{
    general_handler_t *gh = (general_handler_t *)ud;
    if (length < 1) return;

    int voltage = (int)data[0];
    if (gh->user_cb.on_get_mcb_voltage_value)
        gh->user_cb.on_get_mcb_voltage_value(gh->user_cb.user_data, voltage);
}

/* ============================================================================
 * LCB Communication Status (reconnect timer)
 * ============================================================================ */

/* Not registered yet: lcb_comm_callbacks_t has no comm-status slot. */
static __attribute__((unused)) void gh_on_lcb_comm_status(void *ud, int status)
{
    general_handler_t *gh = (general_handler_t *)ud;
    pthread_mutex_lock(&gh->mutex);

    /* Only handle CONNECTED (1) and ERROR (3) */
    if (status == GH_COMM_STATUS_CONNECTED || status == GH_COMM_STATUS_ERROR) {
        if (status != gh->last_lcb_comm_status) {
            gh_set_reconnect_deadline(gh);
        }
        gh->last_lcb_comm_status = status;
    }

    pthread_mutex_unlock(&gh->mutex);
}

/* ============================================================================
 * Registration: Wire all internal callbacks
 * ============================================================================ */

void gh_lcb_cb_register(general_handler_t *gh)
{
    /* Main LCB callbacks */
    lcb_comm_callbacks_init(&gh->lcb_cb);
    gh->lcb_cb.user_data = gh;

    /* Init & Status */
    gh->lcb_cb.on_lcb_initial = gh_on_lcb_initial;
    gh->lcb_cb.on_lcb_operating_status = gh_on_lcb_operating_status;
    gh->lcb_cb.on_lcb_status = gh_on_lcb_status;
    gh->lcb_cb.on_lcb_version = gh_on_lcb_version;
    gh->lcb_cb.on_lcb_update_program = gh_on_lcb_update_program;

    /* ERP (standby) mode */
    gh->lcb_cb.on_enter_lcb_erp_mode = gh_on_enter_lcb_erp_mode;
    gh->lcb_cb.on_leave_lcb_erp_mode = gh_on_leave_lcb_erp_mode;
    gh->lcb_cb.on_boot_up_leave_lcb_erp = gh_on_boot_up_leave_lcb_erp;

    /* Error handling */
    gh->lcb_cb.on_lcb_error_status_detected = gh_on_lcb_error_status_detected;
    gh->lcb_cb.on_lcb_error_code = gh_on_lcb_error_code;
    gh->lcb_cb.on_return_error_code_to_ui = gh_on_return_error_code_to_ui;
    gh->lcb_cb.on_lcb_skip_curr_error = gh_on_lcb_skip_curr_error;

    /* Status byte bit changes */
    gh->lcb_cb.on_incline_motor_downward_status_changed = gh_on_incline_down;
    gh->lcb_cb.on_incline_motor_upward_status_changed = gh_on_incline_up;
    gh->lcb_cb.on_main_motor_status_changed = gh_on_main_motor;
    gh->lcb_cb.on_encoder_rpm_status_changed = gh_on_encoder_rpm;
    gh->lcb_cb.on_battery_capacity_status_changed = gh_on_battery_cap;
    gh->lcb_cb.on_command_error_status_changed = gh_on_cmd_error;
    gh->lcb_cb.on_initial_calibration_status_changed = gh_on_initial_cal;

    /* Battery voltage */
    gh->lcb_cb.on_lcb_battery_voltage = gh_on_battery_voltage;

    /* Serial number */
    gh->lcb_cb.on_lcb_cmd_serial_number = gh_on_lcb_cmd_serial_number;

    /* Universal data callbacks */
    lcb_universal_data_callbacks_init(&gh->uni_cb);
    gh->uni_cb.user_data = gh;
    gh->uni_cb.on_ud_get_moto_type = gh_on_ud_moto_type;
    gh->uni_cb.on_ud_magnet_angle_state = gh_on_ud_magnet_angle;
    gh->uni_cb.on_ud_mcb_voltage = gh_on_ud_mcb_voltage;
    gh->lcb_cb.universal_data_cb = &gh->uni_cb;

    /* Device data callbacks */
    lcb_device_data_callbacks_init(&gh->dd_cb);
    gh->dd_cb.user_data = gh;
    gh->dd_cb.on_dd_get_lcb_version = gh_on_dd_get_lcb_version;
    gh->dd_cb.on_dd_get_lcb_information = gh_on_dd_get_lcb_information;
    gh->dd_cb.on_dd_read_pwm = gh_on_dd_read_pwm;
    gh->lcb_cb.device_data_cb = &gh->dd_cb;
}

void gh_lcb_cb_unregister(general_handler_t *gh)
{
    memset(&gh->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
    memset(&gh->uni_cb, 0, sizeof(lcb_universal_data_callbacks_t));
    memset(&gh->dd_cb, 0, sizeof(lcb_device_data_callbacks_t));
}
