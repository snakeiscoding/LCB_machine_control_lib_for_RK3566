/**
 * @file lcb_proc_comm.c
 * @brief LCB Communication Response Processing Implementation
 *
 * Converted from: com/jht/lcbcommlib/LcbProcComm.java
 *
 * Each handler follows one of these patterns:
 * - Pattern A (simple ack): Invoke callback with no data extraction
 * - Pattern B (typed data): Validate data size, extract value, invoke callback
 * - Pattern C (raw bytes): Validate data size, pass byte array to callback
 * - Pattern D (error injection): Inject synthetic error codes
 */

#include "lcb_proc_comm.h"
#include "lcb_device_data.h"
#include "lcb_universal_data.h"
#include "lcb_utils.h"
#include "lcb_constants.h"
#include "lcb_logging.h"
#include <string.h>

#define TAG "lcb_proc_comm"

/* ============================================================================
 * Helper: invoke error code callback with return-to-UI
 * ============================================================================ */
static void lcb_proc_error_code_with_ui(const lcb_comm_callbacks_t *cb, uint16_t error_code)
{
    if (cb == NULL) return;

    if (cb->on_lcb_error_code != NULL)
        cb->on_lcb_error_code(cb->user_data, error_code);

    if (error_code != 0x0000) {
        if (cb->on_return_error_code_to_ui != NULL)
            cb->on_return_error_code_to_ui(cb->user_data, error_code);
    }
}

/* ============================================================================
 * Command 0x70-0x7E Response Handlers
 * ============================================================================ */

/* 0x70 SetLCBInitialize */
void lcb_proc_set_initial(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x70 (SetLCBInitialize)");
    if (cb->on_lcb_initial != NULL)
        cb->on_lcb_initial(cb->user_data);
}

/* 0x71 GetLCBStatus */
void lcb_proc_get_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    LCB_LOGD(TAG, "Processing 0x71 status=0x%02X", pkt->rx_lcb_status);
    if (cb->on_lcb_status != NULL)
        cb->on_lcb_status(cb->user_data, pkt->rx_lcb_status);
}

/* 0x72 GetLCBErrorCode */
void lcb_proc_get_error_code(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_ERROR_CODE) return;

    uint16_t error_code = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x72 value=0x%04X", error_code);
    lcb_proc_error_code_with_ui(cb, error_code);
}

/* 0x73 GetLCBVersion */
void lcb_proc_get_version(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_VERSION) return;

    uint16_t version = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x73 value=0x%04X", version);
    if (cb->on_lcb_version != NULL)
        cb->on_lcb_version(cb->user_data, version);
}

/* ============================================================================
 * IR Address Board (0x26) Response Handlers
 * ============================================================================ */

/* 0x73 GetLCBVersion (IR Address board variant — same opcode as main LCB,
 * dispatched here instead because the request targeted address 0x26) */
void lcb_proc_get_ir_addr_version(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_VERSION) return;

    uint16_t version = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing IR Addr 0x73 value=0x%04X", version);
    if (cb->on_ir_addr_version != NULL)
        cb->on_ir_addr_version(cb->user_data, (int)version);
}

/* 0x99 GetIrAddressIrStatus (IR Address board only) */
void lcb_proc_get_ir_addr_ir_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_IR_ADDR_IR_STATUS) return;

    LCB_LOGD(TAG, "Processing IR Addr 0x99 value=0x%02X", data[0]);
    if (cb->on_ir_addr_ir_status != NULL)
        cb->on_ir_addr_ir_status(cb->user_data, data[0]);
}

/* 0x74 SetLCBCalibrate */
void lcb_proc_set_calibrate(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x74 (SetLCBCalibrate)");
    if (cb->on_lcb_calibrate != NULL)
        cb->on_lcb_calibrate(cb->user_data);
}

/* 0x75 SetLCBUpdateProgram */
void lcb_proc_set_update_program(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_UPDATE_PROGRAM) return;

    LCB_LOGD(TAG, "Processing 0x75 (SetLCBUpdateProgram)");
    if (cb->on_lcb_update_program != NULL)
        cb->on_lcb_update_program(cb->user_data, data[0]);
}

/* 0x76 SetLCBSkipCurrError */
void lcb_proc_set_skip_curr_error(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x76 (SetLCBSkipCurrError)");
    if (cb->on_lcb_skip_curr_error != NULL)
        cb->on_lcb_skip_curr_error(cb->user_data);
}

/* 0x77 SetLCBSpeExtCmd */
void lcb_proc_set_spe_ext_cmd(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_SPE_EXT_CMD) return;

    LCB_LOGD(TAG, "Processing 0x77 (SetLCBSpeExtCmd)");
    if (cb->on_lcb_spe_ext_cmd != NULL)
        cb->on_lcb_spe_ext_cmd(cb->user_data, data[0]);
}

/* 0x78 GetLCBDciVersion */
void lcb_proc_get_dci_version(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_DCI_VERSION) return;

    LCB_LOGD(TAG, "Processing 0x78 (GetLCBDciVersion)");
    if (cb->on_lcb_dci_version != NULL)
        cb->on_lcb_dci_version(cb->user_data, data, data_len);
}

/* 0x79 GetLCBDciEnv */
void lcb_proc_get_dci_env(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_DCI_ENV) return;

    LCB_LOGD(TAG, "Processing 0x79 (GetLCBDciEnv)");
    if (cb->on_lcb_dci_env != NULL)
        cb->on_lcb_dci_env(cb->user_data, data, data_len);
}

/* 0x7B SetLCBCmdConsolePower */
void lcb_proc_set_cmd_console_power(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x7B (SetLCBCmdConsolePower)");
    if (cb->on_lcb_cmd_console_power != NULL)
        cb->on_lcb_cmd_console_power(cb->user_data);
}

/* 0x7C GetLCBProtocolVersion */
void lcb_proc_get_protocol_version(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_PROTOCOL_VERSION) return;

    uint16_t version = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x7C value=0x%04X", version);
    if (cb->on_lcb_protocol_version != NULL)
        cb->on_lcb_protocol_version(cb->user_data, version);
}

/* 0x7D SetLCBCmdSerialNumber */
void lcb_proc_set_cmd_serial_number(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0x7D (SetLCBCmdSerialNumber)");
    if (cb->on_lcb_cmd_serial_number != NULL)
        cb->on_lcb_cmd_serial_number(cb->user_data, data, data_len);
}

/* 0x7E SetLCBTreadmillOperate */
void lcb_proc_set_treadmill_operate(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0x7E (SetLCBTreadmillOperate)");
    if (cb->on_lcb_treadmill_operate != NULL)
        cb->on_lcb_treadmill_operate(cb->user_data, data, data_len);
}

/* ============================================================================
 * Command 0xF0-0xFE Response Handlers
 * ============================================================================ */

/* 0xF0 SetLCBMotorSpeedRtnRPM */
void lcb_proc_set_motor_speed_rtn_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_MOTOR_SPEED_RTN_RPM) return;

    uint16_t rpm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0xF0 value=0x%04X", rpm);
    if (cb->on_lcb_motor_speed_rtn_rpm != NULL)
        cb->on_lcb_motor_speed_rtn_rpm(cb->user_data, rpm);
}

/* 0xF1 SetLCBMotorRPM */
void lcb_proc_set_motor_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xF1 (SetLCBMotorRPM)");
    if (cb->on_lcb_motor_rpm != NULL)
        cb->on_lcb_motor_rpm(cb->user_data);
}

/* 0xF2 SetLCBIncPWMStep */
void lcb_proc_set_inc_pwm_step(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xF2 (SetLCBIncPWMStep)");
    if (cb->on_lcb_inc_pwm_step != NULL)
        cb->on_lcb_inc_pwm_step(cb->user_data);
}

/* 0xF3 SetLCBDecPWMStep */
void lcb_proc_set_dec_pwm_step(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xF3 (SetLCBDecPWMStep)");
    if (cb->on_lcb_dec_pwm_step != NULL)
        cb->on_lcb_dec_pwm_step(cb->user_data);
}

/* 0xF4 SetLCBStopPWMStep */
void lcb_proc_set_stop_pwm_step(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xF4 (SetLCBStopPWMStep)");
    if (cb->on_lcb_stop_pwm_step != NULL)
        cb->on_lcb_stop_pwm_step(cb->user_data);
}

/* 0xF5 SetLCBInclineAction */
void lcb_proc_set_incline_action(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xF5 (SetLCBInclineAction)");
    if (cb->on_lcb_incline_action != NULL)
        cb->on_lcb_incline_action(cb->user_data);
}

/* 0xF6 SetLCBInclineLoc */
void lcb_proc_set_incline_loc(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len >= 2) {
        uint16_t loc = lcb_bytes_to_uint16_be(data);
        LCB_LOGD(TAG, "Processing 0xF6 value=0x%04X", loc);
        if (cb->on_lcb_incline_loc != NULL)
            cb->on_lcb_incline_loc(cb->user_data, loc);
    }
}

/* 0xF7 SetLCBOperatingStatus */
void lcb_proc_set_operating_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xF7 (SetLCBOperatingStatus)");
    if (cb->on_lcb_operating_status != NULL)
        cb->on_lcb_operating_status(cb->user_data);
}

/* 0xF8 GetLCBRollerRPM */
void lcb_proc_get_roller_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_ROLLER_RPM) return;

    uint16_t rpm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0xF8 value=0x%04X", rpm);
    if (cb->on_lcb_roller_rpm != NULL)
        cb->on_lcb_roller_rpm(cb->user_data, rpm);
}

/* 0xF9 GetLCBMotorRPM */
void lcb_proc_get_motor_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len < 2) return;

    uint16_t rpm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0xF9 value=0x%04X", rpm);
    if (cb->on_lcb_motor_rpm_received != NULL)
        cb->on_lcb_motor_rpm_received(cb->user_data, rpm);
}

/* 0xF9 GetOneShotLCBMotorRPM */
void lcb_proc_get_one_shot_motor_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len < 2) return;

    uint16_t rpm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0xF9 value=0x%04X (OneShot)", rpm);
    if (cb->on_one_shot_lcb_motor_rpm != NULL)
        cb->on_one_shot_lcb_motor_rpm(cb->user_data, rpm);
}

/* 0xFA GetLCBInclineLoc */
void lcb_proc_get_incline_loc(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_INCLINE_LOC) return;

    uint16_t loc = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0xFA value=0x%04X", loc);
    if (cb->on_lcb_incline_loc_received != NULL)
        cb->on_lcb_incline_loc_received(cb->user_data, loc);
}

/* 0xFB SetLCBTuneEndIncline */
void lcb_proc_set_tune_end_incline(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xFB (SetLCBTuneEndIncline)");
    if (cb->on_lcb_tune_end_incline != NULL)
        cb->on_lcb_tune_end_incline(cb->user_data);
}

/* 0xFC SetLCBTuneEndIncline2 */
void lcb_proc_set_tune_end_incline2(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xFC (SetLCBTuneEndIncline2)");
    if (cb->on_lcb_tune_end_incline2 != NULL)
        cb->on_lcb_tune_end_incline2(cb->user_data);
}

/* 0xFD SetLCBInclineStroke */
void lcb_proc_set_incline_stroke(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xFD (SetLCBInclineStroke)");
    if (cb->on_lcb_incline_stroke != NULL)
        cb->on_lcb_incline_stroke(cb->user_data);
}

/* 0xFE SetLCBCompensationVol */
void lcb_proc_set_compensation_vol(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_COMPENSATION_VOL) return;

    uint16_t vol = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0xFE value=0x%04X", vol);
    if (cb->on_lcb_compensation_vol != NULL)
        cb->on_lcb_compensation_vol(cb->user_data, vol);
}

/* ============================================================================
 * Command 0x90-0xAF Response Handlers
 * ============================================================================ */

/* 0x90 SetLCBMotorSpecification */
void lcb_proc_set_motor_specification(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x90 (SetLCBMotorSpecification)");
    if (cb->on_lcb_motor_specification != NULL)
        cb->on_lcb_motor_specification(cb->user_data);
}

/* 0x91 GetLCBMotorType */
void lcb_proc_get_motor_type(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_MOTOR_TYPE) return;

    LCB_LOGD(TAG, "Processing 0x91 (GetLCBMotorType)");
    if (cb->on_lcb_motor_type != NULL)
        cb->on_lcb_motor_type(cb->user_data, data[0]);
}

/* 0x92 GetLCBDCIdataMaxLen */
void lcb_proc_get_dci_data_max_len(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_DCI_DATA_MAX_LEN) return;

    LCB_LOGD(TAG, "Processing 0x92 (GetLCBDCIdataMaxLen)");
    if (cb->on_lcb_dci_data_max_len != NULL)
        cb->on_lcb_dci_data_max_len(cb->user_data, data[0]);
}

/* 0x93 SetLCBMotorPowerOff */
void lcb_proc_set_motor_power_off(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x93 (SetLCBMotorPowerOff)");
    if (cb->on_lcb_motor_power_off != NULL)
        cb->on_lcb_motor_power_off(cb->user_data);
}

/* 0x94 SetLCBDCIforceIncline */
void lcb_proc_set_dci_force_incline(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x94 (SetLCBDCIforceIncline)");
    if (cb->on_lcb_dci_force_incline != NULL)
        cb->on_lcb_dci_force_incline(cb->user_data);
}

/* 0x95 GetLCBTreadmillInUsed */
void lcb_proc_get_treadmill_in_used(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_TREADMILL_IN_USED) return;

    LCB_LOGD(TAG, "Processing 0x95 (GetLCBTreadmillInUsed)");
    if (cb->on_lcb_treadmill_in_used != NULL)
        cb->on_lcb_treadmill_in_used(cb->user_data, data[0]);
}

/* 0x96 GetLCBMainMotorInfo */
void lcb_proc_get_main_motor_info(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_MAIN_MOTOR_INFO) return;

    LCB_LOGD(TAG, "Processing 0x96 (GetLCBMainMotorInfo)");
    if (cb->on_lcb_main_motor_info != NULL)
        cb->on_lcb_main_motor_info(cb->user_data, data, data_len);
}

/* 0x97 GetLCBTreadmillExtStatus */
void lcb_proc_get_treadmill_ext_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_TREADMILL_EXT_STATUS) return;

    uint16_t status = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x97 value=0x%04X", status);
    if (cb->on_lcb_treadmill_ext_status != NULL)
        cb->on_lcb_treadmill_ext_status(cb->user_data, status);
}

/* 0x7F GetLCBTreadmillCadence — forwarded as raw bytes, no fixed-length
 * check (Java's procGetLcbTreadmillCadenceHandler() forwards packet.getData()
 * unconditionally, whatever its length). */
void lcb_proc_get_treadmill_cadence(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0x7F (GetLCBTreadmillCadence) length=%d", data_len);
    if (cb->on_lcb_treadmill_cadence != NULL)
        cb->on_lcb_treadmill_cadence(cb->user_data, data, data_len);
}

/* 0x98 SetLCBTreadmillEcbAction */
void lcb_proc_set_treadmill_ecb_action(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_TREADMILL_ECB_ACTION) return;

    LCB_LOGD(TAG, "Processing 0x98 (SetLCBTreadmillEcbAction)");
    if (cb->on_lcb_treadmill_ecb_action != NULL)
        cb->on_lcb_treadmill_ecb_action(cb->user_data, data[0]);
}

/* 0xA0 GetLCBDCIinclineRange */
void lcb_proc_get_dci_incline_range(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_DCI_INCLINE_RANGE) return;

    uint32_t range = lcb_bytes_to_uint32_be(data);
    LCB_LOGD(TAG, "Processing 0xA0 value=0x%04X", range);
    if (cb->on_lcb_dci_incline_range != NULL)
        cb->on_lcb_dci_incline_range(cb->user_data, range);
}

/* 0xA1 SetLCBDCIinclineRange */
void lcb_proc_set_dci_incline_range(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xA1 (SetLCBDCIinclineRange)");
    if (cb->on_lcb_dci_incline_range_set != NULL)
        cb->on_lcb_dci_incline_range_set(cb->user_data);
}

/* 0xA6 GetLCBDCIrpmSpeedRange */
void lcb_proc_get_dci_rpm_speed_range(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_DCI_RPM_SPEED_RANGE) return;

    uint32_t range = lcb_bytes_to_uint32_be(data);
    LCB_LOGD(TAG, "Processing 0xA6 value=0x%04X", range);
    if (cb->on_lcb_dci_rpm_speed_range != NULL)
        cb->on_lcb_dci_rpm_speed_range(cb->user_data, range);
}

/* 0xA7 SetLCBDCIrpmSpeedRange */
void lcb_proc_set_dci_rpm_speed_range(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xA7 (SetLCBDCIrpmSpeedRange)");
    if (cb->on_lcb_dci_rpm_speed_range_set != NULL)
        cb->on_lcb_dci_rpm_speed_range_set(cb->user_data);
}

/* 0xAA SetLCBUniversal */
void lcb_proc_set_universal(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0xAA (SetLCBUniversal)");

    /* Keep raw callback for backward compatibility */
    if (data_len >= 2) {
        uint16_t value = lcb_bytes_to_uint16_be(data);
        LCB_LOGD(TAG, "Processing 0xAA value=0x%04X", value);
        if (cb->on_lcb_universal != NULL)
            cb->on_lcb_universal(cb->user_data, value);
    }

    /* Dispatch to universal data sub-command handlers */
    if (cb->universal_data_cb != NULL && data != NULL && data_len > 0)
        lcb_universal_data_analyze(cb->universal_data_cb, data, data_len);
}

/* 0xAB SetLCBDCIManualCalibration */
void lcb_proc_set_dci_manual_calibration(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0xAB (SetLCBDCIManualCalibration)");
    if (cb->on_lcb_dci_manual_calibration != NULL)
        cb->on_lcb_dci_manual_calibration(cb->user_data);
}

/* ============================================================================
 * Command 0x50-0x5F Response Handlers
 * ============================================================================ */

/* 0x50 SetLCBDeviceData */
void lcb_proc_set_device_data(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0x50 (SetLCBDeviceData)");

    /* Keep raw callback for backward compatibility */
    if (cb->on_lcb_device_data != NULL)
        cb->on_lcb_device_data(cb->user_data, data, data_len);

    /* Dispatch to device data sub-command handlers */
    if (cb->device_data_cb != NULL && data != NULL && data_len > 0)
        lcb_device_data_analyze(cb->device_data_cb, data, data_len);
}

/* 0x51 GetLCBIndoorBikeSensorData */
void lcb_proc_get_indoor_bike_sensor_data(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    /* TcvFxSpinBike reuses this same 0x51 command with a distinct, shorter payload
     * (11 bytes) than native IndoorBike (19 bytes); accept both. */
    if (data_len != LCB_RECEIVE_DATA_SIZE_INDOORBIKE_SENSORDATA &&
        data_len != LCB_RECEIVE_DATA_SIZE_TCV_FX_SPINBIKE_SENSORDATA) return;

    LCB_LOGD(TAG, "Processing 0x51 (GetLCBIndoorBikeSensorData)");
    /* Pass raw data as indoor_bike_sensor_data_t - caller interprets */
    if (cb->on_lcb_indoor_bike_sensor_data != NULL)
        cb->on_lcb_indoor_bike_sensor_data(cb->user_data, (const indoor_bike_sensor_data_t *)data);
}

/* 0x51 GetOneShotLCBIndoorBikeSensorData */
void lcb_proc_get_one_shot_indoor_bike_sensor_data(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    /* See lcb_proc_get_indoor_bike_sensor_data() for why both lengths are accepted. */
    if (data_len != LCB_RECEIVE_DATA_SIZE_INDOORBIKE_SENSORDATA &&
        data_len != LCB_RECEIVE_DATA_SIZE_TCV_FX_SPINBIKE_SENSORDATA) return;

    LCB_LOGD(TAG, "Processing 0x51 (GetOneShotLCBIndoorBikeSensorData)");
    if (cb->on_one_shot_lcb_indoor_bike_sensor_data != NULL)
        cb->on_one_shot_lcb_indoor_bike_sensor_data(cb->user_data, (const indoor_bike_sensor_data_t *)data);
}

/* 0x52 SetLCBIndoorBikeStepperMotorPosition */
void lcb_proc_set_indoor_bike_stepper_motor_pos(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x52 (SetLCBIndoorBikeStepperMotorPosition)");
    if (cb->on_lcb_indoor_bike_stepper_motor_position != NULL)
        cb->on_lcb_indoor_bike_stepper_motor_position(cb->user_data);
}

/* 0x53 SetLCBIndoorBikeCalcParaOfDevice */
void lcb_proc_set_indoor_bike_calc_para(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x53 (SetLCBIndoorBikeCalcParaOfDevice)");
    if (cb->on_lcb_indoor_bike_calc_para_of_device != NULL)
        cb->on_lcb_indoor_bike_calc_para_of_device(cb->user_data);
}

/* 0x54 GetLCBIndoorBikeCalcParaOfDevice */
void lcb_proc_get_indoor_bike_calc_para(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0x54 (GetLCBIndoorBikeCalcParaOfDevice)");
    if (cb->on_lcb_indoor_bike_calc_para_of_device_get != NULL)
        cb->on_lcb_indoor_bike_calc_para_of_device_get(cb->user_data, data, data_len);
}

/* 0x55 SetLCBIndoorBikeActionSpinDown */
void lcb_proc_set_indoor_bike_action_spin_down(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0x55 (SetLCBIndoorBikeActionSpinDown)");
    if (cb->on_lcb_indoor_bike_action_spin_down != NULL)
        cb->on_lcb_indoor_bike_action_spin_down(cb->user_data, data, data_len);
}

/* 0x56 GetLCBIndoorBikeInfoSpinDown */
void lcb_proc_get_indoor_bike_info_spin_down(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_INDOORBIKE_INFO_SPIN_DOWN) return;

    LCB_LOGD(TAG, "Processing 0x56 (GetLCBIndoorBikeInfoSpinDown)");
    if (cb->on_lcb_indoor_bike_info_spin_down != NULL)
        cb->on_lcb_indoor_bike_info_spin_down(cb->user_data, (const indoor_bike_info_spin_down_t *)data);
}

/* 0x57 SetLCBIndoorBikeErgModeSettings */
void lcb_proc_set_indoor_bike_erg_mode_settings(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0x57 (SetLCBIndoorBikeErgModeSettings)");
    if (cb->on_lcb_indoor_bike_erg_mode_settings != NULL)
        cb->on_lcb_indoor_bike_erg_mode_settings(cb->user_data, data, data_len);
}

/* 0x59 GetLCBIndoorBikeErgModeSettings */
void lcb_proc_get_indoor_bike_erg_mode_settings(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    LCB_LOGD(TAG, "Processing 0x59 (GetLCBIndoorBikeErgModeSettings)");
    if (cb->on_lcb_indoor_bike_erg_mode_settings_get != NULL)
        cb->on_lcb_indoor_bike_erg_mode_settings_get(cb->user_data, data, data_len);
}

/* 0x58 SetLCBIndoorBikeResistanceLevel */
void lcb_proc_set_indoor_bike_resistance_level(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x58 (SetLCBIndoorBikeResistanceLevel)");
    if (cb->on_lcb_indoor_bike_resistance_level != NULL)
        cb->on_lcb_indoor_bike_resistance_level(cb->user_data);
}

/* ============================================================================
 * Command 0x60-0x6F Response Handlers
 * ============================================================================ */

/* 0x61 SetLCBResetECB */
void lcb_proc_set_reset_ecb(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x61 (SetLCBResetECB)");
    if (cb->on_lcb_reset_ecb != NULL)
        cb->on_lcb_reset_ecb(cb->user_data);
}

/* 0x62 SetLCBECBposition */
void lcb_proc_set_ecb_position(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x62 (SetLCBECBposition)");
    if (cb->on_lcb_ecb_position != NULL)
        cb->on_lcb_ecb_position(cb->user_data);
}

/* 0x63 GetLCBECBrpm */
void lcb_proc_get_ecb_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_ECB_RPM) return;

    uint16_t rpm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x63 value=0x%04X", rpm);
    if (cb->on_lcb_ecb_rpm != NULL)
        cb->on_lcb_ecb_rpm(cb->user_data, rpm);
}

/* 0x63 GetOneShotLCBECBrpm */
void lcb_proc_get_one_shot_ecb_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_ECB_RPM) return;

    uint16_t rpm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x63 value=0x%04X (OneShot)", rpm);
    if (cb->on_one_shot_lcb_ecb_rpm != NULL)
        cb->on_one_shot_lcb_ecb_rpm(cb->user_data, rpm);
}

/* 0x64 GetLCBECBadc */
void lcb_proc_get_ecb_adc(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_ECB_ADC) return;

    uint16_t adc = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x64 value=0x%04X", adc);
    if (cb->on_lcb_ecb_adc != NULL)
        cb->on_lcb_ecb_adc(cb->user_data, adc);
}

/* 0x65 SetLCBPWMpercent */
void lcb_proc_set_pwm_percent(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x65 (SetLCBPWMpercent)");
    if (cb->on_lcb_pwm_percent != NULL)
        cb->on_lcb_pwm_percent(cb->user_data);
}

/* 0x66 GetLCBBatteryVoltage */
void lcb_proc_get_battery_voltage(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_BATTERY_VOLTAGE) return;

    uint16_t voltage = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x66 value=0x%04X", voltage);
    if (cb->on_lcb_battery_voltage != NULL)
        cb->on_lcb_battery_voltage(cb->user_data, voltage);
}

/* 0x66 GetLCBBatteryVoltageLowPriority */
void lcb_proc_get_battery_voltage_low_priority(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_BATTERY_VOLTAGE) return;

    uint16_t voltage = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x66 value=0x%04X (Low Priority)", voltage);
    if (cb->on_lcb_battery_voltage_low_priority != NULL)
        cb->on_lcb_battery_voltage_low_priority(cb->user_data, voltage);
}

/* 0x67 SetLCBWatts */
void lcb_proc_set_watts_cmd(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x67 (SetLCBWatts)");
    if (cb->on_lcb_watts != NULL)
        cb->on_lcb_watts(cb->user_data);
}

/* 0x68 SetLCBRPMgearRatio */
void lcb_proc_set_rpm_gear_ratio(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x68 (SetLCBRPMgearRatio)");
    if (cb->on_lcb_rpm_gear_ratio != NULL)
        cb->on_lcb_rpm_gear_ratio(cb->user_data);
}

/* 0x69 SetLCBGeneratorPolePair */
void lcb_proc_set_generator_pole_pair(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x69 (SetLCBGeneratorPolePair)");
    if (cb->on_lcb_generator_pole_pair != NULL)
        cb->on_lcb_generator_pole_pair(cb->user_data);
}

/* 0x6A SetLCBLimitRPMcharge */
void lcb_proc_set_limit_rpm_charge(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x6A (SetLCBLimitRPMcharge)");
    if (cb->on_lcb_limit_rpm_charge != NULL)
        cb->on_lcb_limit_rpm_charge(cb->user_data);
}

/* 0x6B SetLCBLimitRPMresistance */
void lcb_proc_set_limit_rpm_resistance(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x6B (SetLCBLimitRPMresistance)");
    if (cb->on_lcb_limit_rpm_resistance != NULL)
        cb->on_lcb_limit_rpm_resistance(cb->user_data);
}

/* 0x6C SetLCBMachineType */
void lcb_proc_set_machine_type(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x6C (SetLCBMachineType)");
    if (cb->on_lcb_machine_type != NULL)
        cb->on_lcb_machine_type(cb->user_data);
}

/* 0x6D SetLCBPowerOff */
void lcb_proc_set_power_off(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x6D (SetLCBPowerOff)");
    if (cb->on_lcb_power_off != NULL)
        cb->on_lcb_power_off(cb->user_data);
}

/* 0x6E SetLCBBatteryChargeLevel */
void lcb_proc_set_battery_charge_level(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x6E (SetLCBBatteryChargeLevel)");
    if (cb->on_lcb_battery_charge_level != NULL)
        cb->on_lcb_battery_charge_level(cb->user_data);
}

/* 0x6F SetLCBMinResistanceInclineWorking */
void lcb_proc_set_min_resistance_incline_working(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x6F (SetLCBMinResistanceInclineWorking)");
    if (cb->on_lcb_min_resistance_incline_working != NULL)
        cb->on_lcb_min_resistance_incline_working(cb->user_data);
}

/* ============================================================================
 * Command 0x80-0x8F Response Handlers
 * ============================================================================ */

/* 0x80 SetLCBInclinePercent */
void lcb_proc_set_incline_percent(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x80 (SetLCBInclinePercent)");
    if (cb->on_lcb_incline_percent != NULL)
        cb->on_lcb_incline_percent(cb->user_data);
}

/* 0x81 GetLCBInclinePercent */
void lcb_proc_get_incline_percent(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_INCLINE_PERCENT) return;

    uint16_t percent = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x81 value=0x%04X", percent);
    if (cb->on_lcb_incline_percent_received != NULL)
        cb->on_lcb_incline_percent_received(cb->user_data, percent);
}

/* 0x82 SetLCBECBaction */
void lcb_proc_set_ecb_action(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x82 (SetLCBECBaction)");
    if (cb->on_lcb_ecb_action != NULL)
        cb->on_lcb_ecb_action(cb->user_data);
}

/* 0x83 GetLCBECBstatus */
void lcb_proc_get_ecb_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_ECB_STATUS) return;

    LCB_LOGD(TAG, "Processing 0x83 (GetLCBECBstatus)");
    if (cb->on_lcb_ecb_status != NULL)
        cb->on_lcb_ecb_status(cb->user_data, data[0]);
}

/* 0x84 GetLCBECBcount */
void lcb_proc_get_ecb_count(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_ECB_COUNT) return;

    uint16_t count = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x84 value=0x%04X", count);
    if (cb->on_lcb_ecb_count != NULL)
        cb->on_lcb_ecb_count(cb->user_data, count);
}

/* 0x85 SetLCBElectromagneticCurrent */
void lcb_proc_set_electromagnetic_current(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x85 (SetLCBElectromagneticCurrent)");
    if (cb->on_lcb_electromagnetic_current != NULL)
        cb->on_lcb_electromagnetic_current(cb->user_data);
}

/* 0x86 SetLCBResistanceType */
void lcb_proc_set_resistance_type(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x86 (SetLCBResistanceType)");
    if (cb->on_lcb_resistance_type != NULL)
        cb->on_lcb_resistance_type(cb->user_data);
}

/* 0x87 SetLCBBeginBatteryCharge */
void lcb_proc_set_begin_battery_charge(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x87 (SetLCBBeginBatteryCharge)");
    if (cb->on_lcb_begin_battery_charge != NULL)
        cb->on_lcb_begin_battery_charge(cb->user_data);
}

/* 0x88 GetLCBBatteryCapacity */
void lcb_proc_get_battery_capacity(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_BATTERY_CAPACITY) return;

    uint16_t capacity = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x88 value=0x%04X", capacity);
    if (cb->on_lcb_battery_capacity != NULL)
        cb->on_lcb_battery_capacity(cb->user_data, capacity);
}

/* 0x8A SetLCBClimbmillStatus */
void lcb_proc_set_climbmill_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x8A (SetLCBClimbmillStatus)");
    if (cb->on_lcb_climbmill_status != NULL)
        cb->on_lcb_climbmill_status(cb->user_data);
}

/* 0x8B GetLCBClimbmillStatus */
void lcb_proc_get_climbmill_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);

    /* Support both 1-byte (old) and 2-byte (new) format */
    uint16_t status = 0;
    if (data_len >= LCB_RECEIVE_DATA_SIZE_ENCLIMBMILL_STATUS) {
        status = lcb_bytes_to_uint16_be(data);
        LCB_LOGD(TAG, "Processing 0x8B value=0x%04X", status);
        if (cb->on_lcb_climbmill_status_received != NULL)
            cb->on_lcb_climbmill_status_received(cb->user_data, status);
    } else if (data_len >= LCB_RECEIVE_DATA_SIZE_CLIMBMILL_STATUS) {
        status = (uint16_t)data[0];
        LCB_LOGD(TAG, "Processing 0x8B value=0x%04X", status);
        if (cb->on_lcb_climbmill_status_received != NULL)
            cb->on_lcb_climbmill_status_received(cb->user_data, status);
    }
}

/* 0x8C SetLCBEStopParameter */
void lcb_proc_set_e_stop_parameter(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x8C (SetLCBEStopParameter)");
    if (cb->on_lcb_e_stop_parameter != NULL)
        cb->on_lcb_e_stop_parameter(cb->user_data);
}

/* 0x8D GetLCBBrakePressureValue */
void lcb_proc_get_brake_pressure_value(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_BRAKE_PRESSURE_VALUE) return;

    uint16_t value = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x8D value=0x%04X", value);
    if (cb->on_lcb_brake_pressure_value != NULL)
        cb->on_lcb_brake_pressure_value(cb->user_data, value);
}

/* 0x8E GetLCBWatts */
void lcb_proc_get_watts(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_WATTS) return;

    uint16_t watts = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x8E value=0x%04X", watts);
    if (cb->on_lcb_watts_received != NULL)
        cb->on_lcb_watts_received(cb->user_data, watts);
}

/* 0x8F SetLCBClimbmillCal */
void lcb_proc_set_climbmill_cal(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x8F (SetLCBClimbmillCal)");
    if (cb->on_lcb_climbmill_cal != NULL)
        cb->on_lcb_climbmill_cal(cb->user_data);
}

/* ============================================================================
 * Command 0x40-0x4F Response Handlers
 * ============================================================================ */

/* 0x47 SetLCBRecordCurrent */
void lcb_proc_set_record_current(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x47 (SetLCBRecordCurrent)");
    if (cb->on_lcb_record_current != NULL)
        cb->on_lcb_record_current(cb->user_data);
}

/* 0x48 SetLCBPWM */
void lcb_proc_set_pwm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x48 (SetLCBPWM)");
    if (cb->on_lcb_pwm != NULL)
        cb->on_lcb_pwm(cb->user_data);
}

/* 0x49 SetLCBFinishCalibrating */
void lcb_proc_set_finish_calibrating(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x49 (SetLCBFinishCalibrating)");
    if (cb->on_lcb_finish_calibrating != NULL)
        cb->on_lcb_finish_calibrating(cb->user_data);
}

/* 0x4A SetLCBSPM */
void lcb_proc_set_spm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x4A (SetLCBSPM)");
    if (cb->on_lcb_spm != NULL)
        cb->on_lcb_spm(cb->user_data);
}

/* 0x4B GetLCBSPM */
void lcb_proc_get_spm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_SPM) return;

    uint16_t spm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x4B value=0x%04X", spm);
    if (cb->on_lcb_spm_received != NULL)
        cb->on_lcb_spm_received(cb->user_data, spm);
}

/* 0x4B GetOneShotLCBSPM */
void lcb_proc_get_one_shot_spm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_SPM) return;

    uint16_t spm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x4B value=0x%04X (OneShot)", spm);
    if (cb->on_one_shot_lcb_spm != NULL)
        cb->on_one_shot_lcb_spm(cb->user_data, spm);
}

/* 0x4C SetLCBCalScaling */
void lcb_proc_set_cal_scaling(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x4C (SetLCBCalScaling)");
    if (cb->on_lcb_cal_scaling != NULL)
        cb->on_lcb_cal_scaling(cb->user_data);
}

/* 0x4D GetLCBCalScaling */
void lcb_proc_get_cal_scaling(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_CAL_SCALING) return;

    uint16_t scaling = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x4D value=0x%04X", scaling);
    if (cb->on_lcb_cal_scaling_received != NULL)
        cb->on_lcb_cal_scaling_received(cb->user_data, scaling);
}

/* 0x4E SetLCBFrameCalScaling */
void lcb_proc_set_frame_cal_scaling(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    (void)pkt;
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing 0x4E (SetLCBFrameCalScaling)");
    if (cb->on_lcb_frame_cal_scaling != NULL)
        cb->on_lcb_frame_cal_scaling(cb->user_data);
}

/* 0x4F GetLCBFrameCalScaling */
void lcb_proc_get_frame_cal_scaling(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_FRAME_CAL_SCALING) return;

    uint16_t scaling = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing 0x4F value=0x%04X", scaling);
    if (cb->on_lcb_frame_cal_scaling_received != NULL)
        cb->on_lcb_frame_cal_scaling_received(cb->user_data, scaling);
}

/* ============================================================================
 * Error Process Handlers
 * ============================================================================ */

/* Communication timeout error (0x04B0) */
void lcb_proc_comm_lcb_error(const lcb_comm_callbacks_t *cb)
{
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Injecting error code 0x%04X", LCB_ERROR_COMM_LCB);
    lcb_proc_error_code_with_ui(cb, LCB_ERROR_COMM_LCB);
}

/* E-Stop error (0x02B2) */
void lcb_proc_e_stop_error(const lcb_comm_callbacks_t *cb)
{
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Injecting error code 0x%04X", LCB_ERROR_E_STOP);
    lcb_proc_error_code_with_ui(cb, LCB_ERROR_E_STOP);
}

/* E-Stop MCB error (0x024F) */
void lcb_proc_e_stop_mcb_error(const lcb_comm_callbacks_t *cb)
{
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Injecting error code 0x%04X", LCB_ERROR_E_STOP_MCB);
    lcb_proc_error_code_with_ui(cb, LCB_ERROR_E_STOP_MCB);
}

/* E-Stop unlock (0x02B1) */
void lcb_proc_e_stop_unlock(const lcb_comm_callbacks_t *cb)
{
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Injecting error code 0x%04X", LCB_ERROR_E_STOP_UNLOCK);
    lcb_proc_error_code_with_ui(cb, LCB_ERROR_E_STOP_UNLOCK);
    LCB_LOGD(TAG, "####Frank_Debug lcb_proc_comm lcb_proc_e_stop_unlock frank_1 LCB_ERROR_E_STOP_UNLOCK");
}

/* Treadmill no RPM response (0x00A1) */
void lcb_proc_tm_no_rpm_error(const lcb_comm_callbacks_t *cb)
{
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Injecting error code 0x%04X", LCB_ERROR_TM_NO_RPM);
    lcb_proc_error_code_with_ui(cb, LCB_ERROR_TM_NO_RPM);
}

/* ============================================================================
 * TX Callback Handlers
 * ============================================================================ */

/* Treadmill RPM set-point feedback on TX write */
void lcb_proc_set_treadmill_rpm_value(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len < 2) return;

    uint16_t rpm = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing TX callback: Treadmill RPM value=0x%04X", rpm);
    if (cb->on_treadmill_rpm_value_set != NULL)
        cb->on_treadmill_rpm_value_set(cb->user_data, rpm);
}

/* Treadmill ADC set-point feedback on TX write */
void lcb_proc_set_treadmill_adc_value(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt)
{
    if (cb == NULL || pkt == NULL) return;
    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(pkt, &data_len);
    if (data_len < 2) return;

    uint16_t adc = lcb_bytes_to_uint16_be(data);
    LCB_LOGD(TAG, "Processing TX callback: Treadmill ADC value=0x%04X", adc);
    if (cb->on_treadmill_adc_value_set != NULL)
        cb->on_treadmill_adc_value_set(cb->user_data, adc);
}

/* Advanced logging status */
void lcb_proc_set_adv_logging_status(const lcb_comm_callbacks_t *cb)
{
    if (cb == NULL) return;
    LCB_LOGD(TAG, "Processing TX callback: Advanced logging status");
    if (cb->on_set_adv_logging_status != NULL)
        cb->on_set_adv_logging_status(cb->user_data);
}
