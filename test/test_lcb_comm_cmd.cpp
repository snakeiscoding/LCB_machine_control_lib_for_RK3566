/**
 * @file test_lcb_comm_cmd.cpp
 * @brief Systematic unit tests for all lcb_proc_* command handlers
 *
 * Tests every handler declared in lcb_proc_comm.h, covering:
 * - Pattern A: Simple ACK (callback invoked, no data)
 * - Pattern B: Typed data extraction (uint8/16/32)
 * - Pattern C: Raw bytes passthrough
 * - NULL safety and wrong-size rejection
 *
 * Uses ONLY the public API - no #include of .c files.
 * Infrastructure (CmdTestContext, trampolines) duplicated from test_lcb_comm.cpp
 * because those are static and cannot be shared across translation units.
 * CmdTestContext is intentionally named differently from TestContext in
 * test_lcb_comm.cpp to avoid C++ ODR (One Definition Rule) violations.
 */

#include "gtest/gtest.h"

extern "C" {
    #include "lcb_proc_comm.h"
    #include "lcb_comm_callback.h"
    #include "lcb_packet.h"
    #include "lcb_commands.h"
    #include "lcb_constants.h"
    #include "lcb_utils.h"
}

/* ============================================================================
 * Test Infrastructure: Callback tracking context
 * ============================================================================ */

struct CmdTestContext {
    /* Simple ACK counters */
    int on_lcb_initial_count;
    int on_lcb_calibrate_count;
    int on_lcb_motor_rpm_count;
    int on_lcb_skip_curr_error_count;
    int on_lcb_cmd_console_power_count;
    int on_lcb_operating_status_count;
    int on_lcb_incline_action_count;
    int on_lcb_inc_pwm_step_count;
    int on_lcb_dec_pwm_step_count;
    int on_lcb_stop_pwm_step_count;
    int on_lcb_tune_end_incline_count;
    int on_lcb_tune_end_incline2_count;
    int on_lcb_incline_stroke_count;
    int on_lcb_motor_specification_count;
    int on_lcb_motor_power_off_count;
    int on_lcb_dci_force_incline_count;
    int on_lcb_dci_incline_range_set_count;
    int on_lcb_dci_rpm_speed_range_set_count;
    int on_lcb_dci_manual_calibration_count;
    int on_lcb_reset_ecb_count;
    int on_lcb_ecb_position_count;
    int on_lcb_pwm_percent_count;
    int on_lcb_watts_cmd_count;
    int on_lcb_rpm_gear_ratio_count;
    int on_lcb_generator_pole_pair_count;
    int on_lcb_limit_rpm_charge_count;
    int on_lcb_limit_rpm_resistance_count;
    int on_lcb_machine_type_count;
    int on_lcb_power_off_count;
    int on_lcb_battery_charge_level_count;
    int on_lcb_min_resistance_incline_working_count;
    int on_lcb_incline_percent_count;
    int on_lcb_ecb_action_count;
    int on_lcb_electromagnetic_current_count;
    int on_lcb_resistance_type_count;
    int on_lcb_begin_battery_charge_count;
    int on_lcb_climbmill_status_count;
    int on_lcb_e_stop_parameter_count;
    int on_lcb_climbmill_cal_count;
    int on_lcb_record_current_count;
    int on_lcb_pwm_count;
    int on_lcb_finish_calibrating_count;
    int on_lcb_spm_set_count;
    int on_lcb_cal_scaling_set_count;
    int on_lcb_frame_cal_scaling_set_count;
    int on_lcb_indoor_bike_stepper_motor_position_count;
    int on_lcb_indoor_bike_calc_para_set_count;
    int on_lcb_indoor_bike_resistance_level_count;

    /* Typed data counters + captured values */
    int on_lcb_status_count;
    uint8_t last_lcb_status;

    int on_lcb_error_code_count;
    uint16_t last_error_code;

    int on_return_error_code_to_ui_count;
    uint16_t last_ui_error_code;

    int on_lcb_version_count;
    uint16_t last_version;

    int on_lcb_motor_speed_rtn_rpm_count;
    uint16_t last_motor_speed_rtn_rpm;

    int on_lcb_motor_rpm_received_count;
    uint16_t last_motor_rpm;

    int on_one_shot_lcb_motor_rpm_count;
    uint16_t last_one_shot_motor_rpm;

    int on_lcb_ecb_rpm_count;
    uint16_t last_ecb_rpm;

    int on_one_shot_lcb_ecb_rpm_count;
    uint16_t last_one_shot_ecb_rpm;

    int on_lcb_spm_received_count;
    uint16_t last_spm;

    int on_one_shot_lcb_spm_count;
    uint16_t last_one_shot_spm;

    int on_lcb_spe_ext_cmd_count;
    uint8_t last_spe_ext_cmd;

    int on_lcb_motor_type_count;
    uint8_t last_motor_type;

    int on_lcb_dci_data_max_len_count;
    uint8_t last_dci_data_max_len;

    int on_lcb_roller_rpm_count;
    uint16_t last_roller_rpm;

    int on_lcb_protocol_version_count;
    uint16_t last_protocol_version;

    int on_lcb_incline_loc_count;
    uint16_t last_incline_loc;

    int on_lcb_incline_loc_received_count;
    uint16_t last_incline_loc_received;

    int on_lcb_compensation_vol_count;
    uint16_t last_compensation_vol;

    int on_lcb_update_program_count;
    uint8_t last_update_program_response;

    int on_lcb_treadmill_in_used_count;
    uint8_t last_treadmill_in_used;

    int on_lcb_treadmill_ext_status_count;
    uint16_t last_treadmill_ext_status;

    int on_lcb_treadmill_ecb_action_count;
    uint8_t last_treadmill_ecb_action;

    int on_lcb_dci_incline_range_count;
    uint32_t last_dci_incline_range;

    int on_lcb_dci_rpm_speed_range_count;
    uint32_t last_dci_rpm_speed_range;

    int on_lcb_universal_count;
    uint16_t last_universal_value;

    int on_lcb_ecb_adc_count;
    uint16_t last_ecb_adc;

    int on_lcb_battery_voltage_count;
    uint16_t last_battery_voltage;

    int on_lcb_ecb_status_count;
    uint8_t last_ecb_status;

    int on_lcb_ecb_count_count;
    uint16_t last_ecb_count;

    int on_lcb_battery_capacity_count;
    uint16_t last_battery_capacity;

    int on_lcb_climbmill_status_received_count;
    uint16_t last_climbmill_status_received;

    int on_lcb_brake_pressure_value_count;
    uint16_t last_brake_pressure_value;

    int on_lcb_watts_received_count;
    uint16_t last_watts_received;

    int on_lcb_incline_percent_received_count;
    uint16_t last_incline_percent_received;

    int on_lcb_cal_scaling_received_count;
    uint16_t last_cal_scaling;

    int on_lcb_frame_cal_scaling_received_count;
    uint16_t last_frame_cal_scaling;

    /* Status bit change callbacks */
    int on_incline_motor_downward_count;
    bool last_incline_motor_downward;
    int on_incline_motor_upward_count;
    bool last_incline_motor_upward;
    int on_main_motor_status_count;
    bool last_main_motor_status;
    int on_encoder_rpm_status_count;
    bool last_encoder_rpm_status;
    int on_battery_capacity_status_count;
    bool last_battery_capacity_status;
    int on_command_error_status_count;
    bool last_command_error_status;
    int on_lcb_error_status_count;
    bool last_lcb_error_status;
    int on_initial_calibration_status_count;
    bool last_initial_calibration_status;

    /* Error injection callbacks */
    int on_estop_machine_stop_count;
    int on_estop_mcb_error_count;
    int on_estop_unlock_count;
    int on_tm_no_rpm_error_count;
    int on_set_adv_logging_status_count;

    /* Raw bytes callbacks */
    int on_lcb_dci_version_count;
    uint8_t last_dci_version_data[32];
    uint8_t last_dci_version_len;

    int on_lcb_dci_env_count;
    uint8_t last_dci_env_data[128];
    uint8_t last_dci_env_len;

    int on_lcb_cmd_serial_number_count;
    uint8_t last_serial_number_data[64];
    uint8_t last_serial_number_len;

    int on_lcb_main_motor_info_count;
    uint8_t last_main_motor_info_data[16];
    uint8_t last_main_motor_info_len;

    int on_lcb_treadmill_operate_count;
    uint8_t last_treadmill_operate_data[32];
    uint8_t last_treadmill_operate_len;

    /* Indoor bike sensor data */
    int on_lcb_indoor_bike_sensor_data_count;
    int on_one_shot_lcb_indoor_bike_sensor_data_count;

    /* Unknown command tracking */
    int on_unknown_command_count;
    uint8_t last_unknown_command;

    /* TX callbacks */
    int on_treadmill_rpm_value_set_count;
    uint16_t last_treadmill_rpm_set;
    int on_treadmill_adc_value_set_count;
    uint16_t last_treadmill_adc_set;

    /* === NEW fields for previously untracked callbacks === */

    /* Device data (0x50) */
    int on_lcb_device_data_count;
    uint8_t last_device_data[256];
    uint8_t last_device_data_len;

    /* Indoor bike calc para get (0x54) */
    int on_lcb_indoor_bike_calc_para_get_count;
    uint8_t last_indoor_bike_calc_para_get_data[64];
    uint8_t last_indoor_bike_calc_para_get_len;

    /* Indoor bike action spin down (0x55) */
    int on_lcb_indoor_bike_action_spin_down_count;
    uint8_t last_indoor_bike_action_spin_down_data[8];
    uint8_t last_indoor_bike_action_spin_down_len;

    /* Indoor bike info spin down (0x56) */
    int on_lcb_indoor_bike_info_spin_down_count;

    /* Indoor bike ERG mode settings (0x57) */
    int on_lcb_indoor_bike_erg_mode_settings_count;
    uint8_t last_indoor_bike_erg_settings_data[96];
    uint8_t last_indoor_bike_erg_settings_len;

    /* Indoor bike ERG mode settings get (0x59) */
    int on_lcb_indoor_bike_erg_mode_settings_get_count;
    uint8_t last_indoor_bike_erg_settings_get_data[96];
    uint8_t last_indoor_bike_erg_settings_get_len;

    void reset() {
        memset(this, 0, sizeof(*this));
    }
};

/* ============================================================================
 * Static callback trampoline functions
 * ============================================================================ */

/* Simple ACK trampolines */
static void cb_on_lcb_initial(void *ud) { ((CmdTestContext*)ud)->on_lcb_initial_count++; }
static void cb_on_lcb_calibrate(void *ud) { ((CmdTestContext*)ud)->on_lcb_calibrate_count++; }
static void cb_on_lcb_motor_rpm(void *ud) { ((CmdTestContext*)ud)->on_lcb_motor_rpm_count++; }
static void cb_on_lcb_skip_curr_error(void *ud) { ((CmdTestContext*)ud)->on_lcb_skip_curr_error_count++; }
static void cb_on_lcb_cmd_console_power(void *ud) { ((CmdTestContext*)ud)->on_lcb_cmd_console_power_count++; }
static void cb_on_lcb_operating_status(void *ud) { ((CmdTestContext*)ud)->on_lcb_operating_status_count++; }
static void cb_on_lcb_incline_action(void *ud) { ((CmdTestContext*)ud)->on_lcb_incline_action_count++; }
static void cb_on_lcb_inc_pwm_step(void *ud) { ((CmdTestContext*)ud)->on_lcb_inc_pwm_step_count++; }
static void cb_on_lcb_dec_pwm_step(void *ud) { ((CmdTestContext*)ud)->on_lcb_dec_pwm_step_count++; }
static void cb_on_lcb_stop_pwm_step(void *ud) { ((CmdTestContext*)ud)->on_lcb_stop_pwm_step_count++; }
static void cb_on_lcb_tune_end_incline(void *ud) { ((CmdTestContext*)ud)->on_lcb_tune_end_incline_count++; }
static void cb_on_lcb_tune_end_incline2(void *ud) { ((CmdTestContext*)ud)->on_lcb_tune_end_incline2_count++; }
static void cb_on_lcb_incline_stroke(void *ud) { ((CmdTestContext*)ud)->on_lcb_incline_stroke_count++; }
static void cb_on_lcb_motor_specification(void *ud) { ((CmdTestContext*)ud)->on_lcb_motor_specification_count++; }
static void cb_on_lcb_motor_power_off(void *ud) { ((CmdTestContext*)ud)->on_lcb_motor_power_off_count++; }
static void cb_on_lcb_dci_force_incline(void *ud) { ((CmdTestContext*)ud)->on_lcb_dci_force_incline_count++; }
static void cb_on_lcb_dci_incline_range_set(void *ud) { ((CmdTestContext*)ud)->on_lcb_dci_incline_range_set_count++; }
static void cb_on_lcb_dci_rpm_speed_range_set(void *ud) { ((CmdTestContext*)ud)->on_lcb_dci_rpm_speed_range_set_count++; }
static void cb_on_lcb_dci_manual_calibration(void *ud) { ((CmdTestContext*)ud)->on_lcb_dci_manual_calibration_count++; }
static void cb_on_lcb_reset_ecb(void *ud) { ((CmdTestContext*)ud)->on_lcb_reset_ecb_count++; }
static void cb_on_lcb_ecb_position(void *ud) { ((CmdTestContext*)ud)->on_lcb_ecb_position_count++; }
static void cb_on_lcb_pwm_percent(void *ud) { ((CmdTestContext*)ud)->on_lcb_pwm_percent_count++; }
static void cb_on_lcb_watts_cmd(void *ud) { ((CmdTestContext*)ud)->on_lcb_watts_cmd_count++; }
static void cb_on_lcb_rpm_gear_ratio(void *ud) { ((CmdTestContext*)ud)->on_lcb_rpm_gear_ratio_count++; }
static void cb_on_lcb_generator_pole_pair(void *ud) { ((CmdTestContext*)ud)->on_lcb_generator_pole_pair_count++; }
static void cb_on_lcb_limit_rpm_charge(void *ud) { ((CmdTestContext*)ud)->on_lcb_limit_rpm_charge_count++; }
static void cb_on_lcb_limit_rpm_resistance(void *ud) { ((CmdTestContext*)ud)->on_lcb_limit_rpm_resistance_count++; }
static void cb_on_lcb_machine_type(void *ud) { ((CmdTestContext*)ud)->on_lcb_machine_type_count++; }
static void cb_on_lcb_power_off(void *ud) { ((CmdTestContext*)ud)->on_lcb_power_off_count++; }
static void cb_on_lcb_battery_charge_level(void *ud) { ((CmdTestContext*)ud)->on_lcb_battery_charge_level_count++; }
static void cb_on_lcb_min_resistance_incline_working(void *ud) { ((CmdTestContext*)ud)->on_lcb_min_resistance_incline_working_count++; }
static void cb_on_lcb_incline_percent(void *ud) { ((CmdTestContext*)ud)->on_lcb_incline_percent_count++; }
static void cb_on_lcb_ecb_action(void *ud) { ((CmdTestContext*)ud)->on_lcb_ecb_action_count++; }
static void cb_on_lcb_electromagnetic_current(void *ud) { ((CmdTestContext*)ud)->on_lcb_electromagnetic_current_count++; }
static void cb_on_lcb_resistance_type(void *ud) { ((CmdTestContext*)ud)->on_lcb_resistance_type_count++; }
static void cb_on_lcb_begin_battery_charge(void *ud) { ((CmdTestContext*)ud)->on_lcb_begin_battery_charge_count++; }
static void cb_on_lcb_climbmill_status(void *ud) { ((CmdTestContext*)ud)->on_lcb_climbmill_status_count++; }
static void cb_on_lcb_e_stop_parameter(void *ud) { ((CmdTestContext*)ud)->on_lcb_e_stop_parameter_count++; }
static void cb_on_lcb_climbmill_cal(void *ud) { ((CmdTestContext*)ud)->on_lcb_climbmill_cal_count++; }
static void cb_on_lcb_record_current(void *ud) { ((CmdTestContext*)ud)->on_lcb_record_current_count++; }
static void cb_on_lcb_pwm(void *ud) { ((CmdTestContext*)ud)->on_lcb_pwm_count++; }
static void cb_on_lcb_finish_calibrating(void *ud) { ((CmdTestContext*)ud)->on_lcb_finish_calibrating_count++; }
static void cb_on_lcb_spm(void *ud) { ((CmdTestContext*)ud)->on_lcb_spm_set_count++; }
static void cb_on_lcb_cal_scaling(void *ud) { ((CmdTestContext*)ud)->on_lcb_cal_scaling_set_count++; }
static void cb_on_lcb_frame_cal_scaling(void *ud) { ((CmdTestContext*)ud)->on_lcb_frame_cal_scaling_set_count++; }
static void cb_on_lcb_indoor_bike_stepper_motor_position(void *ud) { ((CmdTestContext*)ud)->on_lcb_indoor_bike_stepper_motor_position_count++; }
static void cb_on_lcb_indoor_bike_calc_para(void *ud) { ((CmdTestContext*)ud)->on_lcb_indoor_bike_calc_para_set_count++; }
static void cb_on_lcb_indoor_bike_resistance_level(void *ud) { ((CmdTestContext*)ud)->on_lcb_indoor_bike_resistance_level_count++; }

/* Typed data trampolines */
static void cb_on_lcb_status(void *ud, uint8_t status) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_status_count++; ctx->last_lcb_status = status;
}
static void cb_on_lcb_error_code(void *ud, uint16_t error_code) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_error_code_count++; ctx->last_error_code = error_code;
}
static void cb_on_return_error_code_to_ui(void *ud, uint16_t error_code) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_return_error_code_to_ui_count++; ctx->last_ui_error_code = error_code;
}
static void cb_on_lcb_version(void *ud, uint16_t version) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_version_count++; ctx->last_version = version;
}
static void cb_on_lcb_motor_speed_rtn_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_motor_speed_rtn_rpm_count++; ctx->last_motor_speed_rtn_rpm = rpm;
}
static void cb_on_lcb_motor_rpm_received(void *ud, uint16_t rpm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_motor_rpm_received_count++; ctx->last_motor_rpm = rpm;
}
static void cb_on_one_shot_lcb_motor_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_one_shot_lcb_motor_rpm_count++; ctx->last_one_shot_motor_rpm = rpm;
}
static void cb_on_lcb_ecb_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_ecb_rpm_count++; ctx->last_ecb_rpm = rpm;
}
static void cb_on_one_shot_lcb_ecb_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_one_shot_lcb_ecb_rpm_count++; ctx->last_one_shot_ecb_rpm = rpm;
}
static void cb_on_lcb_spm_received(void *ud, uint16_t spm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_spm_received_count++; ctx->last_spm = spm;
}
static void cb_on_one_shot_lcb_spm(void *ud, uint16_t spm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_one_shot_lcb_spm_count++; ctx->last_one_shot_spm = spm;
}
static void cb_on_lcb_spe_ext_cmd(void *ud, uint8_t cmd) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_spe_ext_cmd_count++; ctx->last_spe_ext_cmd = cmd;
}
static void cb_on_lcb_motor_type(void *ud, uint8_t type) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_motor_type_count++; ctx->last_motor_type = type;
}
static void cb_on_lcb_dci_data_max_len(void *ud, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_dci_data_max_len_count++; ctx->last_dci_data_max_len = len;
}
static void cb_on_lcb_roller_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_roller_rpm_count++; ctx->last_roller_rpm = rpm;
}
static void cb_on_lcb_protocol_version(void *ud, uint16_t version) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_protocol_version_count++; ctx->last_protocol_version = version;
}
static void cb_on_lcb_incline_loc(void *ud, uint16_t loc) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_incline_loc_count++; ctx->last_incline_loc = loc;
}
static void cb_on_lcb_incline_loc_received(void *ud, uint16_t loc) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_incline_loc_received_count++; ctx->last_incline_loc_received = loc;
}
static void cb_on_lcb_compensation_vol(void *ud, uint16_t vol) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_compensation_vol_count++; ctx->last_compensation_vol = vol;
}
static void cb_on_lcb_update_program(void *ud, uint8_t resp) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_update_program_count++; ctx->last_update_program_response = resp;
}
static void cb_on_lcb_treadmill_in_used(void *ud, uint8_t status) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_treadmill_in_used_count++; ctx->last_treadmill_in_used = status;
}
static void cb_on_lcb_treadmill_ext_status(void *ud, uint16_t status) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_treadmill_ext_status_count++; ctx->last_treadmill_ext_status = status;
}
static void cb_on_lcb_treadmill_ecb_action(void *ud, uint8_t data) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_treadmill_ecb_action_count++; ctx->last_treadmill_ecb_action = data;
}
static void cb_on_lcb_dci_incline_range(void *ud, uint32_t range) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_dci_incline_range_count++; ctx->last_dci_incline_range = range;
}
static void cb_on_lcb_dci_rpm_speed_range(void *ud, uint32_t range) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_dci_rpm_speed_range_count++; ctx->last_dci_rpm_speed_range = range;
}
static void cb_on_lcb_universal(void *ud, uint16_t val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_universal_count++; ctx->last_universal_value = val;
}
static void cb_on_lcb_ecb_adc(void *ud, uint16_t adc) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_ecb_adc_count++; ctx->last_ecb_adc = adc;
}
static void cb_on_lcb_battery_voltage(void *ud, uint16_t voltage) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_battery_voltage_count++; ctx->last_battery_voltage = voltage;
}
static void cb_on_lcb_ecb_status(void *ud, uint8_t status) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_ecb_status_count++; ctx->last_ecb_status = status;
}
static void cb_on_lcb_ecb_count(void *ud, uint16_t count) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_ecb_count_count++; ctx->last_ecb_count = count;
}
static void cb_on_lcb_battery_capacity(void *ud, uint16_t cap) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_battery_capacity_count++; ctx->last_battery_capacity = cap;
}
static void cb_on_lcb_climbmill_status_received(void *ud, uint16_t status) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_climbmill_status_received_count++; ctx->last_climbmill_status_received = status;
}
static void cb_on_lcb_brake_pressure_value(void *ud, uint16_t val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_brake_pressure_value_count++; ctx->last_brake_pressure_value = val;
}
static void cb_on_lcb_watts_received(void *ud, uint16_t watts) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_watts_received_count++; ctx->last_watts_received = watts;
}
static void cb_on_lcb_incline_percent_received(void *ud, uint16_t pct) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_incline_percent_received_count++; ctx->last_incline_percent_received = pct;
}
static void cb_on_lcb_cal_scaling_received(void *ud, uint16_t val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_cal_scaling_received_count++; ctx->last_cal_scaling = val;
}
static void cb_on_lcb_frame_cal_scaling_received(void *ud, uint16_t val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_frame_cal_scaling_received_count++; ctx->last_frame_cal_scaling = val;
}

/* Status bit callbacks */
static void cb_on_incline_motor_downward(void *ud, bool val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_incline_motor_downward_count++; ctx->last_incline_motor_downward = val;
}
static void cb_on_incline_motor_upward(void *ud, bool val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_incline_motor_upward_count++; ctx->last_incline_motor_upward = val;
}
static void cb_on_main_motor_status(void *ud, bool val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_main_motor_status_count++; ctx->last_main_motor_status = val;
}
static void cb_on_encoder_rpm_status(void *ud, bool val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_encoder_rpm_status_count++; ctx->last_encoder_rpm_status = val;
}
static void cb_on_battery_capacity_status(void *ud, bool val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_battery_capacity_status_count++; ctx->last_battery_capacity_status = val;
}
static void cb_on_command_error_status(void *ud, bool val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_command_error_status_count++; ctx->last_command_error_status = val;
}
static void cb_on_lcb_error_status_detected(void *ud, bool val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_error_status_count++; ctx->last_lcb_error_status = val;
}
static void cb_on_initial_calibration_status(void *ud, bool val) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_initial_calibration_status_count++; ctx->last_initial_calibration_status = val;
}

/* Error injection callbacks */
static void cb_on_estop_machine_stop(void *ud) { ((CmdTestContext*)ud)->on_estop_machine_stop_count++; }
static void cb_on_estop_mcb_error(void *ud) { ((CmdTestContext*)ud)->on_estop_mcb_error_count++; }
static void cb_on_estop_unlock(void *ud) { ((CmdTestContext*)ud)->on_estop_unlock_count++; }
static void cb_on_tm_no_rpm_error(void *ud) { ((CmdTestContext*)ud)->on_tm_no_rpm_error_count++; }
static void cb_on_set_adv_logging_status(void *ud) { ((CmdTestContext*)ud)->on_set_adv_logging_status_count++; }

/* Raw bytes callbacks */
static void cb_on_lcb_dci_version(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_dci_version_count++;
    ctx->last_dci_version_len = len;
    if (data && len <= sizeof(ctx->last_dci_version_data))
        memcpy(ctx->last_dci_version_data, data, len);
}
static void cb_on_lcb_dci_env(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_dci_env_count++;
    ctx->last_dci_env_len = len;
    if (data && len <= sizeof(ctx->last_dci_env_data))
        memcpy(ctx->last_dci_env_data, data, len);
}
static void cb_on_lcb_cmd_serial_number(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_cmd_serial_number_count++;
    ctx->last_serial_number_len = len;
    if (data && len <= sizeof(ctx->last_serial_number_data))
        memcpy(ctx->last_serial_number_data, data, len);
}
static void cb_on_lcb_main_motor_info(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_main_motor_info_count++;
    ctx->last_main_motor_info_len = len;
    if (data && len <= sizeof(ctx->last_main_motor_info_data))
        memcpy(ctx->last_main_motor_info_data, data, len);
}
static void cb_on_lcb_treadmill_operate(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_treadmill_operate_count++;
    ctx->last_treadmill_operate_len = len;
    if (data && len <= sizeof(ctx->last_treadmill_operate_data))
        memcpy(ctx->last_treadmill_operate_data, data, len);
}

/* Indoor bike sensor data (simplified - just track count) */
static void cb_on_lcb_indoor_bike_sensor_data(void *ud, const indoor_bike_sensor_data_t *d) {
    ((CmdTestContext*)ud)->on_lcb_indoor_bike_sensor_data_count++;
}
static void cb_on_one_shot_lcb_indoor_bike_sensor_data(void *ud, const indoor_bike_sensor_data_t *d) {
    ((CmdTestContext*)ud)->on_one_shot_lcb_indoor_bike_sensor_data_count++;
}

/* Unknown command */
static void cb_on_unknown_command(void *ud, uint8_t cmd, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_unknown_command_count++; ctx->last_unknown_command = cmd;
}

/* TX callbacks */
static void cb_on_treadmill_rpm_value_set(void *ud, uint16_t rpm) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_treadmill_rpm_value_set_count++; ctx->last_treadmill_rpm_set = rpm;
}
static void cb_on_treadmill_adc_value_set(void *ud, uint16_t adc) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_treadmill_adc_value_set_count++; ctx->last_treadmill_adc_set = adc;
}

/* === NEW callback trampolines for previously untracked callbacks === */

static void cb_on_lcb_device_data(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_device_data_count++;
    ctx->last_device_data_len = len;
    if (data)
        memcpy(ctx->last_device_data, data, len);
}
static void cb_on_lcb_indoor_bike_calc_para_get(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_indoor_bike_calc_para_get_count++;
    ctx->last_indoor_bike_calc_para_get_len = len;
    if (data && len <= sizeof(ctx->last_indoor_bike_calc_para_get_data))
        memcpy(ctx->last_indoor_bike_calc_para_get_data, data, len);
}
static void cb_on_lcb_indoor_bike_action_spin_down(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_indoor_bike_action_spin_down_count++;
    ctx->last_indoor_bike_action_spin_down_len = len;
    if (data && len <= sizeof(ctx->last_indoor_bike_action_spin_down_data))
        memcpy(ctx->last_indoor_bike_action_spin_down_data, data, len);
}
static void cb_on_lcb_indoor_bike_info_spin_down(void *ud, const indoor_bike_info_spin_down_t *info) {
    ((CmdTestContext*)ud)->on_lcb_indoor_bike_info_spin_down_count++;
}
static void cb_on_lcb_indoor_bike_erg_mode_settings(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_indoor_bike_erg_mode_settings_count++;
    ctx->last_indoor_bike_erg_settings_len = len;
    if (data && len <= sizeof(ctx->last_indoor_bike_erg_settings_data))
        memcpy(ctx->last_indoor_bike_erg_settings_data, data, len);
}
static void cb_on_lcb_indoor_bike_erg_mode_settings_get(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (CmdTestContext*)ud; ctx->on_lcb_indoor_bike_erg_mode_settings_get_count++;
    ctx->last_indoor_bike_erg_settings_get_len = len;
    if (data && len <= sizeof(ctx->last_indoor_bike_erg_settings_get_data))
        memcpy(ctx->last_indoor_bike_erg_settings_get_data, data, len);
}

/* ============================================================================
 * Helper: wire all callbacks
 * ============================================================================ */
static void setup_all_callbacks(lcb_comm_callbacks_t *cb, CmdTestContext *ctx) {
    lcb_comm_callbacks_init(cb);
    cb->user_data = ctx;

    /* Simple ACK */
    cb->on_lcb_initial = cb_on_lcb_initial;
    cb->on_lcb_calibrate = cb_on_lcb_calibrate;
    cb->on_lcb_motor_rpm = cb_on_lcb_motor_rpm;
    cb->on_lcb_skip_curr_error = cb_on_lcb_skip_curr_error;
    cb->on_lcb_cmd_console_power = cb_on_lcb_cmd_console_power;
    cb->on_lcb_operating_status = cb_on_lcb_operating_status;
    cb->on_lcb_incline_action = cb_on_lcb_incline_action;
    cb->on_lcb_inc_pwm_step = cb_on_lcb_inc_pwm_step;
    cb->on_lcb_dec_pwm_step = cb_on_lcb_dec_pwm_step;
    cb->on_lcb_stop_pwm_step = cb_on_lcb_stop_pwm_step;
    cb->on_lcb_tune_end_incline = cb_on_lcb_tune_end_incline;
    cb->on_lcb_tune_end_incline2 = cb_on_lcb_tune_end_incline2;
    cb->on_lcb_incline_stroke = cb_on_lcb_incline_stroke;
    cb->on_lcb_motor_specification = cb_on_lcb_motor_specification;
    cb->on_lcb_motor_power_off = cb_on_lcb_motor_power_off;
    cb->on_lcb_dci_force_incline = cb_on_lcb_dci_force_incline;
    cb->on_lcb_dci_incline_range_set = cb_on_lcb_dci_incline_range_set;
    cb->on_lcb_dci_rpm_speed_range_set = cb_on_lcb_dci_rpm_speed_range_set;
    cb->on_lcb_dci_manual_calibration = cb_on_lcb_dci_manual_calibration;
    cb->on_lcb_reset_ecb = cb_on_lcb_reset_ecb;
    cb->on_lcb_ecb_position = cb_on_lcb_ecb_position;
    cb->on_lcb_pwm_percent = cb_on_lcb_pwm_percent;
    cb->on_lcb_watts = cb_on_lcb_watts_cmd;
    cb->on_lcb_rpm_gear_ratio = cb_on_lcb_rpm_gear_ratio;
    cb->on_lcb_generator_pole_pair = cb_on_lcb_generator_pole_pair;
    cb->on_lcb_limit_rpm_charge = cb_on_lcb_limit_rpm_charge;
    cb->on_lcb_limit_rpm_resistance = cb_on_lcb_limit_rpm_resistance;
    cb->on_lcb_machine_type = cb_on_lcb_machine_type;
    cb->on_lcb_power_off = cb_on_lcb_power_off;
    cb->on_lcb_battery_charge_level = cb_on_lcb_battery_charge_level;
    cb->on_lcb_min_resistance_incline_working = cb_on_lcb_min_resistance_incline_working;
    cb->on_lcb_incline_percent = cb_on_lcb_incline_percent;
    cb->on_lcb_ecb_action = cb_on_lcb_ecb_action;
    cb->on_lcb_electromagnetic_current = cb_on_lcb_electromagnetic_current;
    cb->on_lcb_resistance_type = cb_on_lcb_resistance_type;
    cb->on_lcb_begin_battery_charge = cb_on_lcb_begin_battery_charge;
    cb->on_lcb_climbmill_status = cb_on_lcb_climbmill_status;
    cb->on_lcb_e_stop_parameter = cb_on_lcb_e_stop_parameter;
    cb->on_lcb_climbmill_cal = cb_on_lcb_climbmill_cal;
    cb->on_lcb_record_current = cb_on_lcb_record_current;
    cb->on_lcb_pwm = cb_on_lcb_pwm;
    cb->on_lcb_finish_calibrating = cb_on_lcb_finish_calibrating;
    cb->on_lcb_spm = cb_on_lcb_spm;
    cb->on_lcb_cal_scaling = cb_on_lcb_cal_scaling;
    cb->on_lcb_frame_cal_scaling = cb_on_lcb_frame_cal_scaling;
    cb->on_lcb_indoor_bike_stepper_motor_position = cb_on_lcb_indoor_bike_stepper_motor_position;
    cb->on_lcb_indoor_bike_calc_para_of_device = cb_on_lcb_indoor_bike_calc_para;
    cb->on_lcb_indoor_bike_resistance_level = cb_on_lcb_indoor_bike_resistance_level;

    /* Typed data */
    cb->on_lcb_status = cb_on_lcb_status;
    cb->on_lcb_error_code = cb_on_lcb_error_code;
    cb->on_return_error_code_to_ui = cb_on_return_error_code_to_ui;
    cb->on_lcb_version = cb_on_lcb_version;
    cb->on_lcb_motor_speed_rtn_rpm = cb_on_lcb_motor_speed_rtn_rpm;
    cb->on_lcb_motor_rpm_received = cb_on_lcb_motor_rpm_received;
    cb->on_one_shot_lcb_motor_rpm = cb_on_one_shot_lcb_motor_rpm;
    cb->on_lcb_ecb_rpm = cb_on_lcb_ecb_rpm;
    cb->on_one_shot_lcb_ecb_rpm = cb_on_one_shot_lcb_ecb_rpm;
    cb->on_lcb_spm_received = cb_on_lcb_spm_received;
    cb->on_one_shot_lcb_spm = cb_on_one_shot_lcb_spm;
    cb->on_lcb_spe_ext_cmd = cb_on_lcb_spe_ext_cmd;
    cb->on_lcb_motor_type = cb_on_lcb_motor_type;
    cb->on_lcb_dci_data_max_len = cb_on_lcb_dci_data_max_len;
    cb->on_lcb_roller_rpm = cb_on_lcb_roller_rpm;
    cb->on_lcb_protocol_version = cb_on_lcb_protocol_version;
    cb->on_lcb_incline_loc = cb_on_lcb_incline_loc;
    cb->on_lcb_incline_loc_received = cb_on_lcb_incline_loc_received;
    cb->on_lcb_compensation_vol = cb_on_lcb_compensation_vol;
    cb->on_lcb_update_program = cb_on_lcb_update_program;
    cb->on_lcb_treadmill_in_used = cb_on_lcb_treadmill_in_used;
    cb->on_lcb_treadmill_ext_status = cb_on_lcb_treadmill_ext_status;
    cb->on_lcb_treadmill_ecb_action = cb_on_lcb_treadmill_ecb_action;
    cb->on_lcb_dci_incline_range = cb_on_lcb_dci_incline_range;
    cb->on_lcb_dci_rpm_speed_range = cb_on_lcb_dci_rpm_speed_range;
    cb->on_lcb_universal = cb_on_lcb_universal;
    cb->on_lcb_ecb_adc = cb_on_lcb_ecb_adc;
    cb->on_lcb_battery_voltage = cb_on_lcb_battery_voltage;
    cb->on_lcb_ecb_status = cb_on_lcb_ecb_status;
    cb->on_lcb_ecb_count = cb_on_lcb_ecb_count;
    cb->on_lcb_battery_capacity = cb_on_lcb_battery_capacity;
    cb->on_lcb_climbmill_status_received = cb_on_lcb_climbmill_status_received;
    cb->on_lcb_brake_pressure_value = cb_on_lcb_brake_pressure_value;
    cb->on_lcb_watts_received = cb_on_lcb_watts_received;
    cb->on_lcb_incline_percent_received = cb_on_lcb_incline_percent_received;
    cb->on_lcb_cal_scaling_received = cb_on_lcb_cal_scaling_received;
    cb->on_lcb_frame_cal_scaling_received = cb_on_lcb_frame_cal_scaling_received;

    /* Status bit */
    cb->on_incline_motor_downward_status_changed = cb_on_incline_motor_downward;
    cb->on_incline_motor_upward_status_changed = cb_on_incline_motor_upward;
    cb->on_main_motor_status_changed = cb_on_main_motor_status;
    cb->on_encoder_rpm_status_changed = cb_on_encoder_rpm_status;
    cb->on_battery_capacity_status_changed = cb_on_battery_capacity_status;
    cb->on_command_error_status_changed = cb_on_command_error_status;
    cb->on_lcb_error_status_detected = cb_on_lcb_error_status_detected;
    cb->on_initial_calibration_status_changed = cb_on_initial_calibration_status;

    /* Error injection */
    cb->on_estop_machine_stop = cb_on_estop_machine_stop;
    cb->on_estop_mcb_error = cb_on_estop_mcb_error;
    cb->on_estop_unlock = cb_on_estop_unlock;
    cb->on_tm_no_rpm_error = cb_on_tm_no_rpm_error;
    cb->on_set_adv_logging_status = cb_on_set_adv_logging_status;

    /* Raw bytes */
    cb->on_lcb_dci_version = cb_on_lcb_dci_version;
    cb->on_lcb_dci_env = cb_on_lcb_dci_env;
    cb->on_lcb_cmd_serial_number = cb_on_lcb_cmd_serial_number;
    cb->on_lcb_main_motor_info = cb_on_lcb_main_motor_info;
    cb->on_lcb_treadmill_operate = cb_on_lcb_treadmill_operate;

    /* Indoor bike sensor data */
    cb->on_lcb_indoor_bike_sensor_data = cb_on_lcb_indoor_bike_sensor_data;
    cb->on_one_shot_lcb_indoor_bike_sensor_data = cb_on_one_shot_lcb_indoor_bike_sensor_data;

    /* Unknown command */
    cb->on_unknown_command = cb_on_unknown_command;

    /* TX callbacks */
    cb->on_treadmill_rpm_value_set = cb_on_treadmill_rpm_value_set;
    cb->on_treadmill_adc_value_set = cb_on_treadmill_adc_value_set;

    /* NEW callbacks */
    cb->on_lcb_device_data = cb_on_lcb_device_data;
    cb->on_lcb_indoor_bike_calc_para_of_device_get = cb_on_lcb_indoor_bike_calc_para_get;
    cb->on_lcb_indoor_bike_action_spin_down = cb_on_lcb_indoor_bike_action_spin_down;
    cb->on_lcb_indoor_bike_info_spin_down = cb_on_lcb_indoor_bike_info_spin_down;
    cb->on_lcb_indoor_bike_erg_mode_settings = cb_on_lcb_indoor_bike_erg_mode_settings;
    cb->on_lcb_indoor_bike_erg_mode_settings_get = cb_on_lcb_indoor_bike_erg_mode_settings_get;
}

/* ============================================================================
 * Helper: create a simulated RX packet
 * ============================================================================ */
static lcb_packet_t make_rx_packet(lcb_command_t cmd, uint8_t rx_status,
                                    const uint8_t *data, uint8_t data_len) {
    lcb_packet_t pkt;
    lcb_packet_init(&pkt);
    pkt.command = cmd;
    pkt.rx_lcb_status = rx_status;
    if (data != NULL && data_len > 0) {
        lcb_packet_set_data(&pkt, data, data_len);
    }
    return pkt;
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

class LcbProcCmdTest : public ::testing::Test {
protected:
    CmdTestContext ctx;
    lcb_comm_callbacks_t cb;
    lcb_packet_t pkt0;  /* empty packet for simple ACK tests */

    void SetUp() override {
        ctx.reset();
        setup_all_callbacks(&cb, &ctx);
        pkt0 = make_rx_packet((lcb_command_t)0, 0x00, NULL, 0);
    }
};

/* ============================================================================
 * Suite A: Command 0x70-0x7E (5 untested commands)
 * ============================================================================ */

/* --- 0x75 set_update_program: uint8 extraction --- */

TEST_F(LcbProcCmdTest, Proc_0x75_UpdateProgram_ExtractsUint8) {
    uint8_t data[] = {0xAB};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x75, 0x00, data, 1);
    lcb_proc_set_update_program(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_update_program_count, 1);
    EXPECT_EQ(ctx.last_update_program_response, 0xAB);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x75_UpdateProgram_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x75, 0x00, data, 2);
    lcb_proc_set_update_program(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_update_program_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x75_UpdateProgram_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x75, 0x00, data, 1);
    lcb_proc_set_update_program(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x75_UpdateProgram_NullPkt_NoCrash) {
    lcb_proc_set_update_program(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_update_program_count, 0);
}

/* --- 0x76 set_skip_curr_error: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x76_SkipCurrError_Invoked) {
    lcb_proc_set_skip_curr_error(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_skip_curr_error_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x76_SkipCurrError_NullCb_NoCrash) {
    lcb_proc_set_skip_curr_error(NULL, &pkt0);
}

/* --- 0x7B set_cmd_console_power: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x7B_CmdConsolePower_Invoked) {
    lcb_proc_set_cmd_console_power(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_cmd_console_power_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x7B_CmdConsolePower_NullCb_NoCrash) {
    lcb_proc_set_cmd_console_power(NULL, &pkt0);
}

/* --- 0x7D set_cmd_serial_number: raw bytes, no size validation --- */

TEST_F(LcbProcCmdTest, Proc_0x7D_CmdSerialNumber_RawBytes) {
    uint8_t data[] = {0x41, 0x42, 0x43, 0x44, 0x45};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x7D, 0x00, data, 5);
    lcb_proc_set_cmd_serial_number(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_cmd_serial_number_count, 1);
    EXPECT_EQ(ctx.last_serial_number_len, 5);
    EXPECT_EQ(memcmp(ctx.last_serial_number_data, data, 5), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x7D_CmdSerialNumber_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x7D, 0x00, data, 1);
    lcb_proc_set_cmd_serial_number(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x7D_CmdSerialNumber_NullPkt_NoCrash) {
    lcb_proc_set_cmd_serial_number(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_cmd_serial_number_count, 0);
}

/* --- 0x7E set_treadmill_operate: raw bytes, no size validation --- */

TEST_F(LcbProcCmdTest, Proc_0x7E_TreadmillOperate_RawBytes) {
    uint8_t data[] = {0x10, 0x20, 0x30};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x7E, 0x00, data, 3);
    lcb_proc_set_treadmill_operate(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_treadmill_operate_count, 1);
    EXPECT_EQ(ctx.last_treadmill_operate_len, 3);
    EXPECT_EQ(memcmp(ctx.last_treadmill_operate_data, data, 3), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x7E_TreadmillOperate_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x7E, 0x00, data, 1);
    lcb_proc_set_treadmill_operate(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x7E_TreadmillOperate_NullPkt_NoCrash) {
    lcb_proc_set_treadmill_operate(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_treadmill_operate_count, 0);
}

/* ============================================================================
 * Suite B: Command 0xF0-0xFE (11 untested commands)
 * ============================================================================ */

/* --- 0xF2 set_inc_pwm_step: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xF2_IncPwmStep_Invoked) {
    lcb_proc_set_inc_pwm_step(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_inc_pwm_step_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xF2_IncPwmStep_NullCb_NoCrash) {
    lcb_proc_set_inc_pwm_step(NULL, &pkt0);
}

/* --- 0xF3 set_dec_pwm_step: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xF3_DecPwmStep_Invoked) {
    lcb_proc_set_dec_pwm_step(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_dec_pwm_step_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xF3_DecPwmStep_NullCb_NoCrash) {
    lcb_proc_set_dec_pwm_step(NULL, &pkt0);
}

/* --- 0xF4 set_stop_pwm_step: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xF4_StopPwmStep_Invoked) {
    lcb_proc_set_stop_pwm_step(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_stop_pwm_step_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xF4_StopPwmStep_NullCb_NoCrash) {
    lcb_proc_set_stop_pwm_step(NULL, &pkt0);
}

/* --- 0xF5 set_incline_action: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xF5_InclineAction_Invoked) {
    lcb_proc_set_incline_action(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_incline_action_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xF5_InclineAction_NullCb_NoCrash) {
    lcb_proc_set_incline_action(NULL, &pkt0);
}

/* --- 0xF6 set_incline_loc: uint16 BE, loose check (data_len >= 2) --- */

TEST_F(LcbProcCmdTest, Proc_0xF6_SetInclineLoc_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x1234, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xF6, 0x00, data, 2);
    lcb_proc_set_incline_loc(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_incline_loc_count, 1);
    EXPECT_EQ(ctx.last_incline_loc, 0x1234);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xF6_SetInclineLoc_ExtraBytes_StillWorks) {
    uint8_t data[] = {0x00, 0xFF, 0xAA};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xF6, 0x00, data, 3);
    lcb_proc_set_incline_loc(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_incline_loc_count, 1);
    EXPECT_EQ(ctx.last_incline_loc, 0x00FF);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xF6_SetInclineLoc_TooShort_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xF6, 0x00, data, 1);
    lcb_proc_set_incline_loc(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_incline_loc_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xF6_SetInclineLoc_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x1234, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xF6, 0x00, data, 2);
    lcb_proc_set_incline_loc(NULL, &pkt);
    lcb_packet_free(&pkt);
}

/* --- 0xF7 set_operating_status: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xF7_OperatingStatus_Invoked) {
    lcb_proc_set_operating_status(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_operating_status_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xF7_OperatingStatus_NullCb_NoCrash) {
    lcb_proc_set_operating_status(NULL, &pkt0);
}

/* --- 0xFA get_incline_loc: uint16 BE, strict check (data_len != 2) --- */

TEST_F(LcbProcCmdTest, Proc_0xFA_GetInclineLoc_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0xBEEF, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xFA, 0x00, data, 2);
    lcb_proc_get_incline_loc(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_incline_loc_received_count, 1);
    EXPECT_EQ(ctx.last_incline_loc_received, 0xBEEF);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xFA_GetInclineLoc_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xFA, 0x00, data, 3);
    lcb_proc_get_incline_loc(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_incline_loc_received_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xFA_GetInclineLoc_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xFA, 0x00, data, 2);
    lcb_proc_get_incline_loc(NULL, &pkt);
    lcb_packet_free(&pkt);
}

/* --- 0xFB set_tune_end_incline: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xFB_TuneEndIncline_Invoked) {
    lcb_proc_set_tune_end_incline(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_tune_end_incline_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xFB_TuneEndIncline_NullCb_NoCrash) {
    lcb_proc_set_tune_end_incline(NULL, &pkt0);
}

/* --- 0xFC set_tune_end_incline2: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xFC_TuneEndIncline2_Invoked) {
    lcb_proc_set_tune_end_incline2(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_tune_end_incline2_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xFC_TuneEndIncline2_NullCb_NoCrash) {
    lcb_proc_set_tune_end_incline2(NULL, &pkt0);
}

/* --- 0xFD set_incline_stroke: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xFD_InclineStroke_Invoked) {
    lcb_proc_set_incline_stroke(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_incline_stroke_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xFD_InclineStroke_NullCb_NoCrash) {
    lcb_proc_set_incline_stroke(NULL, &pkt0);
}

/* --- 0xFE set_compensation_vol: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0xFE_CompensationVol_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0xCAFE, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xFE, 0x00, data, 2);
    lcb_proc_set_compensation_vol(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_compensation_vol_count, 1);
    EXPECT_EQ(ctx.last_compensation_vol, 0xCAFE);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xFE_CompensationVol_WrongSize_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xFE, 0x00, data, 1);
    lcb_proc_set_compensation_vol(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_compensation_vol_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xFE_CompensationVol_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xFE, 0x00, data, 2);
    lcb_proc_set_compensation_vol(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xFE_CompensationVol_NullPkt_NoCrash) {
    lcb_proc_set_compensation_vol(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_compensation_vol_count, 0);
}

/* ============================================================================
 * Suite C: Command 0x90-0xAF (14 untested commands)
 * ============================================================================ */

/* --- 0x90 set_motor_specification: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x90_MotorSpecification_Invoked) {
    lcb_proc_set_motor_specification(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_motor_specification_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x90_MotorSpecification_NullCb_NoCrash) {
    lcb_proc_set_motor_specification(NULL, &pkt0);
}

/* --- 0x92 get_dci_data_max_len: uint8 extraction, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x92_DciDataMaxLen_ExtractsUint8) {
    uint8_t data[] = {0x40};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x92, 0x00, data, 1);
    lcb_proc_get_dci_data_max_len(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_dci_data_max_len_count, 1);
    EXPECT_EQ(ctx.last_dci_data_max_len, 0x40);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x92_DciDataMaxLen_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x92, 0x00, data, 2);
    lcb_proc_get_dci_data_max_len(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_dci_data_max_len_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x92_DciDataMaxLen_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x92, 0x00, data, 1);
    lcb_proc_get_dci_data_max_len(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x92_DciDataMaxLen_NullPkt_NoCrash) {
    lcb_proc_get_dci_data_max_len(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_dci_data_max_len_count, 0);
}

/* --- 0x93 set_motor_power_off: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x93_MotorPowerOff_Invoked) {
    lcb_proc_set_motor_power_off(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_motor_power_off_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x93_MotorPowerOff_NullCb_NoCrash) {
    lcb_proc_set_motor_power_off(NULL, &pkt0);
}

/* --- 0x94 set_dci_force_incline: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x94_DciForceIncline_Invoked) {
    lcb_proc_set_dci_force_incline(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_dci_force_incline_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x94_DciForceIncline_NullCb_NoCrash) {
    lcb_proc_set_dci_force_incline(NULL, &pkt0);
}

/* --- 0x95 get_treadmill_in_used: uint8 extraction, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x95_TreadmillInUsed_ExtractsUint8) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x95, 0x00, data, 1);
    lcb_proc_get_treadmill_in_used(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_treadmill_in_used_count, 1);
    EXPECT_EQ(ctx.last_treadmill_in_used, 0x01);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x95_TreadmillInUsed_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x95, 0x00, data, 2);
    lcb_proc_get_treadmill_in_used(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_treadmill_in_used_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x95_TreadmillInUsed_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x95, 0x00, data, 1);
    lcb_proc_get_treadmill_in_used(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x95_TreadmillInUsed_NullPkt_NoCrash) {
    lcb_proc_get_treadmill_in_used(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_treadmill_in_used_count, 0);
}

/* --- 0x96 get_main_motor_info: raw bytes, strict check (12 bytes) --- */

TEST_F(LcbProcCmdTest, Proc_0x96_MainMotorInfo_RawBytes) {
    uint8_t data[12];
    for (int i = 0; i < 12; i++) data[i] = (uint8_t)(i + 1);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x96, 0x00, data, 12);
    lcb_proc_get_main_motor_info(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_main_motor_info_count, 1);
    EXPECT_EQ(ctx.last_main_motor_info_len, 12);
    EXPECT_EQ(memcmp(ctx.last_main_motor_info_data, data, 12), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x96_MainMotorInfo_WrongSize_Ignored) {
    uint8_t data[8] = {0};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x96, 0x00, data, 8);
    lcb_proc_get_main_motor_info(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_main_motor_info_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x96_MainMotorInfo_NullCb_NoCrash) {
    uint8_t data[12] = {0};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x96, 0x00, data, 12);
    lcb_proc_get_main_motor_info(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x96_MainMotorInfo_NullPkt_NoCrash) {
    lcb_proc_get_main_motor_info(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_main_motor_info_count, 0);
}

/* --- 0x97 get_treadmill_ext_status: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x97_TreadmillExtStatus_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0xDEAD, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x97, 0x00, data, 2);
    lcb_proc_get_treadmill_ext_status(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_treadmill_ext_status_count, 1);
    EXPECT_EQ(ctx.last_treadmill_ext_status, 0xDEAD);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x97_TreadmillExtStatus_WrongSize_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x97, 0x00, data, 1);
    lcb_proc_get_treadmill_ext_status(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_treadmill_ext_status_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x97_TreadmillExtStatus_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x97, 0x00, data, 2);
    lcb_proc_get_treadmill_ext_status(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x97_TreadmillExtStatus_NullPkt_NoCrash) {
    lcb_proc_get_treadmill_ext_status(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_treadmill_ext_status_count, 0);
}

/* --- 0x98 set_treadmill_ecb_action: uint8 extraction, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x98_TreadmillEcbAction_ExtractsUint8) {
    uint8_t data[] = {0x07};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x98, 0x00, data, 1);
    lcb_proc_set_treadmill_ecb_action(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_treadmill_ecb_action_count, 1);
    EXPECT_EQ(ctx.last_treadmill_ecb_action, 0x07);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x98_TreadmillEcbAction_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x98, 0x00, data, 2);
    lcb_proc_set_treadmill_ecb_action(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_treadmill_ecb_action_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x98_TreadmillEcbAction_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x98, 0x00, data, 1);
    lcb_proc_set_treadmill_ecb_action(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x98_TreadmillEcbAction_NullPkt_NoCrash) {
    lcb_proc_set_treadmill_ecb_action(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_treadmill_ecb_action_count, 0);
}

/* --- 0xA0 get_dci_incline_range: uint32 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0xA0_DciInclineRange_ExtractsUint32) {
    uint8_t data[4];
    lcb_uint32_to_bytes_be(0x12345678, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xA0, 0x00, data, 4);
    lcb_proc_get_dci_incline_range(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_dci_incline_range_count, 1);
    EXPECT_EQ(ctx.last_dci_incline_range, 0x12345678u);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xA0_DciInclineRange_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xA0, 0x00, data, 2);
    lcb_proc_get_dci_incline_range(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_dci_incline_range_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xA0_DciInclineRange_NullCb_NoCrash) {
    uint8_t data[4];
    lcb_uint32_to_bytes_be(0x00000001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xA0, 0x00, data, 4);
    lcb_proc_get_dci_incline_range(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xA0_DciInclineRange_NullPkt_NoCrash) {
    lcb_proc_get_dci_incline_range(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_dci_incline_range_count, 0);
}

/* --- 0xA1 set_dci_incline_range: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xA1_DciInclineRangeSet_Invoked) {
    lcb_proc_set_dci_incline_range(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_dci_incline_range_set_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xA1_DciInclineRangeSet_NullCb_NoCrash) {
    lcb_proc_set_dci_incline_range(NULL, &pkt0);
}

/* --- 0xA6 get_dci_rpm_speed_range: uint32 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0xA6_DciRpmSpeedRange_ExtractsUint32) {
    uint8_t data[4];
    lcb_uint32_to_bytes_be(0xAABBCCDD, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xA6, 0x00, data, 4);
    lcb_proc_get_dci_rpm_speed_range(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_dci_rpm_speed_range_count, 1);
    EXPECT_EQ(ctx.last_dci_rpm_speed_range, 0xAABBCCDDu);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xA6_DciRpmSpeedRange_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xA6, 0x00, data, 3);
    lcb_proc_get_dci_rpm_speed_range(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_dci_rpm_speed_range_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xA6_DciRpmSpeedRange_NullCb_NoCrash) {
    uint8_t data[4];
    lcb_uint32_to_bytes_be(0x00000001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xA6, 0x00, data, 4);
    lcb_proc_get_dci_rpm_speed_range(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xA6_DciRpmSpeedRange_NullPkt_NoCrash) {
    lcb_proc_get_dci_rpm_speed_range(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_dci_rpm_speed_range_count, 0);
}

/* --- 0xA7 set_dci_rpm_speed_range: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xA7_DciRpmSpeedRangeSet_Invoked) {
    lcb_proc_set_dci_rpm_speed_range(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_dci_rpm_speed_range_set_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xA7_DciRpmSpeedRangeSet_NullCb_NoCrash) {
    lcb_proc_set_dci_rpm_speed_range(NULL, &pkt0);
}

/* --- 0xAA set_universal: uint16 BE, loose check (data_len < 2 returns) --- */

TEST_F(LcbProcCmdTest, Proc_0xAA_Universal_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x5678, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xAA, 0x00, data, 2);
    lcb_proc_set_universal(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_universal_count, 1);
    EXPECT_EQ(ctx.last_universal_value, 0x5678);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xAA_Universal_ExtraBytes_StillWorks) {
    uint8_t data[] = {0xAB, 0xCD, 0xEF};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xAA, 0x00, data, 3);
    lcb_proc_set_universal(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_universal_count, 1);
    EXPECT_EQ(ctx.last_universal_value, 0xABCD);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xAA_Universal_TooShort_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xAA, 0x00, data, 1);
    lcb_proc_set_universal(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_universal_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xAA_Universal_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0xAA, 0x00, data, 2);
    lcb_proc_set_universal(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0xAA_Universal_NullPkt_NoCrash) {
    lcb_proc_set_universal(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_universal_count, 0);
}

/* --- 0xAB set_dci_manual_calibration: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0xAB_DciManualCalibration_Invoked) {
    lcb_proc_set_dci_manual_calibration(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_dci_manual_calibration_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0xAB_DciManualCalibration_NullCb_NoCrash) {
    lcb_proc_set_dci_manual_calibration(NULL, &pkt0);
}

/* ============================================================================
 * Suite D: Command 0x50-0x5F (11 untested commands)
 * ============================================================================ */

/* --- 0x50 set_device_data: raw bytes, no size validation --- */

TEST_F(LcbProcCmdTest, Proc_0x50_DeviceData_RawBytes) {
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x50, 0x00, data, 4);
    lcb_proc_set_device_data(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_device_data_count, 1);
    EXPECT_EQ(ctx.last_device_data_len, 4);
    EXPECT_EQ(memcmp(ctx.last_device_data, data, 4), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x50_DeviceData_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x50, 0x00, data, 1);
    lcb_proc_set_device_data(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x50_DeviceData_NullPkt_NoCrash) {
    lcb_proc_set_device_data(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_device_data_count, 0);
}

/* --- 0x51 get_indoor_bike_sensor_data: struct cast, strict 19 bytes --- */

TEST_F(LcbProcCmdTest, Proc_0x51_IndoorBikeSensorData_CorrectSize) {
    uint8_t data[19];
    memset(data, 0xAA, 19);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x51, 0x00, data, 19);
    lcb_proc_get_indoor_bike_sensor_data(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_sensor_data_count, 1);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x51_IndoorBikeSensorData_WrongSize_Ignored) {
    uint8_t data[10];
    memset(data, 0xBB, 10);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x51, 0x00, data, 10);
    lcb_proc_get_indoor_bike_sensor_data(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_sensor_data_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x51_IndoorBikeSensorData_NullCb_NoCrash) {
    uint8_t data[19] = {0};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x51, 0x00, data, 19);
    lcb_proc_get_indoor_bike_sensor_data(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x51_IndoorBikeSensorData_NullPkt_NoCrash) {
    lcb_proc_get_indoor_bike_sensor_data(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_sensor_data_count, 0);
}

/* --- 0x51 get_one_shot_indoor_bike_sensor_data: struct cast, strict 19 bytes --- */

TEST_F(LcbProcCmdTest, Proc_0x51_OneShotIndoorBikeSensorData_CorrectSize) {
    uint8_t data[19];
    memset(data, 0xCC, 19);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x51, 0x00, data, 19);
    lcb_proc_get_one_shot_indoor_bike_sensor_data(&cb, &pkt);
    EXPECT_EQ(ctx.on_one_shot_lcb_indoor_bike_sensor_data_count, 1);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x51_OneShotIndoorBikeSensorData_WrongSize_Ignored) {
    uint8_t data[5] = {0};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x51, 0x00, data, 5);
    lcb_proc_get_one_shot_indoor_bike_sensor_data(&cb, &pkt);
    EXPECT_EQ(ctx.on_one_shot_lcb_indoor_bike_sensor_data_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x51_OneShotIndoorBikeSensorData_NullCb_NoCrash) {
    uint8_t data[19] = {0};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x51, 0x00, data, 19);
    lcb_proc_get_one_shot_indoor_bike_sensor_data(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x51_OneShotIndoorBikeSensorData_NullPkt_NoCrash) {
    lcb_proc_get_one_shot_indoor_bike_sensor_data(&cb, NULL);
    EXPECT_EQ(ctx.on_one_shot_lcb_indoor_bike_sensor_data_count, 0);
}

/* --- 0x52 set_indoor_bike_stepper_motor_pos: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x52_IndoorBikeStepperMotorPos_Invoked) {
    lcb_proc_set_indoor_bike_stepper_motor_pos(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_stepper_motor_position_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x52_IndoorBikeStepperMotorPos_NullCb_NoCrash) {
    lcb_proc_set_indoor_bike_stepper_motor_pos(NULL, &pkt0);
}

/* --- 0x53 set_indoor_bike_calc_para: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x53_IndoorBikeCalcPara_Invoked) {
    lcb_proc_set_indoor_bike_calc_para(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_calc_para_set_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x53_IndoorBikeCalcPara_NullCb_NoCrash) {
    lcb_proc_set_indoor_bike_calc_para(NULL, &pkt0);
}

/* --- 0x54 get_indoor_bike_calc_para: raw bytes, no size validation --- */

TEST_F(LcbProcCmdTest, Proc_0x54_GetIndoorBikeCalcPara_RawBytes) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x54, 0x00, data, 5);
    lcb_proc_get_indoor_bike_calc_para(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_calc_para_get_count, 1);
    EXPECT_EQ(ctx.last_indoor_bike_calc_para_get_len, 5);
    EXPECT_EQ(memcmp(ctx.last_indoor_bike_calc_para_get_data, data, 5), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x54_GetIndoorBikeCalcPara_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x54, 0x00, data, 1);
    lcb_proc_get_indoor_bike_calc_para(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x54_GetIndoorBikeCalcPara_NullPkt_NoCrash) {
    lcb_proc_get_indoor_bike_calc_para(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_calc_para_get_count, 0);
}

/* --- 0x55 set_indoor_bike_action_spin_down: raw bytes, no size validation --- */

TEST_F(LcbProcCmdTest, Proc_0x55_IndoorBikeActionSpinDown_RawBytes) {
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x55, 0x00, data, 3);
    lcb_proc_set_indoor_bike_action_spin_down(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_action_spin_down_count, 1);
    EXPECT_EQ(ctx.last_indoor_bike_action_spin_down_len, 3);
    EXPECT_EQ(memcmp(ctx.last_indoor_bike_action_spin_down_data, data, 3), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x55_IndoorBikeActionSpinDown_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x55, 0x00, data, 1);
    lcb_proc_set_indoor_bike_action_spin_down(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x55_IndoorBikeActionSpinDown_NullPkt_NoCrash) {
    lcb_proc_set_indoor_bike_action_spin_down(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_action_spin_down_count, 0);
}

/* --- 0x56 get_indoor_bike_info_spin_down: struct cast, strict 15 bytes --- */

TEST_F(LcbProcCmdTest, Proc_0x56_IndoorBikeInfoSpinDown_CorrectSize) {
    uint8_t data[15];
    memset(data, 0xDD, 15);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x56, 0x00, data, 15);
    lcb_proc_get_indoor_bike_info_spin_down(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_info_spin_down_count, 1);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x56_IndoorBikeInfoSpinDown_WrongSize_Ignored) {
    uint8_t data[10] = {0};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x56, 0x00, data, 10);
    lcb_proc_get_indoor_bike_info_spin_down(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_info_spin_down_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x56_IndoorBikeInfoSpinDown_NullCb_NoCrash) {
    uint8_t data[15] = {0};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x56, 0x00, data, 15);
    lcb_proc_get_indoor_bike_info_spin_down(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x56_IndoorBikeInfoSpinDown_NullPkt_NoCrash) {
    lcb_proc_get_indoor_bike_info_spin_down(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_info_spin_down_count, 0);
}

/* --- 0x57 set_indoor_bike_erg_mode_settings: raw bytes, no size validation --- */

TEST_F(LcbProcCmdTest, Proc_0x57_IndoorBikeErgModeSettings_RawBytes) {
    uint8_t data[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x57, 0x00, data, 6);
    lcb_proc_set_indoor_bike_erg_mode_settings(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_erg_mode_settings_count, 1);
    EXPECT_EQ(ctx.last_indoor_bike_erg_settings_len, 6);
    EXPECT_EQ(memcmp(ctx.last_indoor_bike_erg_settings_data, data, 6), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x57_IndoorBikeErgModeSettings_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x57, 0x00, data, 1);
    lcb_proc_set_indoor_bike_erg_mode_settings(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x57_IndoorBikeErgModeSettings_NullPkt_NoCrash) {
    lcb_proc_set_indoor_bike_erg_mode_settings(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_erg_mode_settings_count, 0);
}

/* --- 0x58 set_indoor_bike_resistance_level: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x58_IndoorBikeResistanceLevel_Invoked) {
    lcb_proc_set_indoor_bike_resistance_level(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_resistance_level_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x58_IndoorBikeResistanceLevel_NullCb_NoCrash) {
    lcb_proc_set_indoor_bike_resistance_level(NULL, &pkt0);
}

/* --- 0x59 get_indoor_bike_erg_mode_settings: raw bytes, no size validation --- */

TEST_F(LcbProcCmdTest, Proc_0x59_GetIndoorBikeErgModeSettings_RawBytes) {
    uint8_t data[] = {0xF0, 0xE0, 0xD0, 0xC0};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x59, 0x00, data, 4);
    lcb_proc_get_indoor_bike_erg_mode_settings(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_erg_mode_settings_get_count, 1);
    EXPECT_EQ(ctx.last_indoor_bike_erg_settings_get_len, 4);
    EXPECT_EQ(memcmp(ctx.last_indoor_bike_erg_settings_get_data, data, 4), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x59_GetIndoorBikeErgModeSettings_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x59, 0x00, data, 1);
    lcb_proc_get_indoor_bike_erg_mode_settings(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x59_GetIndoorBikeErgModeSettings_NullPkt_NoCrash) {
    lcb_proc_get_indoor_bike_erg_mode_settings(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_indoor_bike_erg_mode_settings_get_count, 0);
}

/* ============================================================================
 * Suite E: Command 0x60-0x6F (13 untested commands)
 * ============================================================================ */

/* --- 0x61 set_reset_ecb: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x61_ResetEcb_Invoked) {
    lcb_proc_set_reset_ecb(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_reset_ecb_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x61_ResetEcb_NullCb_NoCrash) {
    lcb_proc_set_reset_ecb(NULL, &pkt0);
}

/* --- 0x62 set_ecb_position: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x62_EcbPosition_Invoked) {
    lcb_proc_set_ecb_position(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_ecb_position_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x62_EcbPosition_NullCb_NoCrash) {
    lcb_proc_set_ecb_position(NULL, &pkt0);
}

/* --- 0x64 get_ecb_adc: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x64_EcbAdc_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0ABC, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x64, 0x00, data, 2);
    lcb_proc_get_ecb_adc(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_ecb_adc_count, 1);
    EXPECT_EQ(ctx.last_ecb_adc, 0x0ABC);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x64_EcbAdc_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x64, 0x00, data, 3);
    lcb_proc_get_ecb_adc(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_ecb_adc_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x64_EcbAdc_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x64, 0x00, data, 2);
    lcb_proc_get_ecb_adc(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x64_EcbAdc_NullPkt_NoCrash) {
    lcb_proc_get_ecb_adc(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_ecb_adc_count, 0);
}

/* --- 0x65 set_pwm_percent: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x65_PwmPercent_Invoked) {
    lcb_proc_set_pwm_percent(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_pwm_percent_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x65_PwmPercent_NullCb_NoCrash) {
    lcb_proc_set_pwm_percent(NULL, &pkt0);
}

/* --- 0x67 set_watts_cmd: simple ACK (callback field: on_lcb_watts) --- */

TEST_F(LcbProcCmdTest, Proc_0x67_WattsCmd_Invoked) {
    lcb_proc_set_watts_cmd(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_watts_cmd_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x67_WattsCmd_NullCb_NoCrash) {
    lcb_proc_set_watts_cmd(NULL, &pkt0);
}

/* --- 0x68 set_rpm_gear_ratio: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x68_RpmGearRatio_Invoked) {
    lcb_proc_set_rpm_gear_ratio(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_rpm_gear_ratio_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x68_RpmGearRatio_NullCb_NoCrash) {
    lcb_proc_set_rpm_gear_ratio(NULL, &pkt0);
}

/* --- 0x69 set_generator_pole_pair: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x69_GeneratorPolePair_Invoked) {
    lcb_proc_set_generator_pole_pair(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_generator_pole_pair_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x69_GeneratorPolePair_NullCb_NoCrash) {
    lcb_proc_set_generator_pole_pair(NULL, &pkt0);
}

/* --- 0x6A set_limit_rpm_charge: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x6A_LimitRpmCharge_Invoked) {
    lcb_proc_set_limit_rpm_charge(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_limit_rpm_charge_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x6A_LimitRpmCharge_NullCb_NoCrash) {
    lcb_proc_set_limit_rpm_charge(NULL, &pkt0);
}

/* --- 0x6B set_limit_rpm_resistance: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x6B_LimitRpmResistance_Invoked) {
    lcb_proc_set_limit_rpm_resistance(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_limit_rpm_resistance_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x6B_LimitRpmResistance_NullCb_NoCrash) {
    lcb_proc_set_limit_rpm_resistance(NULL, &pkt0);
}

/* --- 0x6C set_machine_type: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x6C_MachineType_Invoked) {
    lcb_proc_set_machine_type(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_machine_type_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x6C_MachineType_NullCb_NoCrash) {
    lcb_proc_set_machine_type(NULL, &pkt0);
}

/* --- 0x6D set_power_off: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x6D_PowerOff_Invoked) {
    lcb_proc_set_power_off(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_power_off_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x6D_PowerOff_NullCb_NoCrash) {
    lcb_proc_set_power_off(NULL, &pkt0);
}

/* --- 0x6E set_battery_charge_level: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x6E_BatteryChargeLevel_Invoked) {
    lcb_proc_set_battery_charge_level(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_battery_charge_level_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x6E_BatteryChargeLevel_NullCb_NoCrash) {
    lcb_proc_set_battery_charge_level(NULL, &pkt0);
}

/* --- 0x6F set_min_resistance_incline_working: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x6F_MinResistanceInclineWorking_Invoked) {
    lcb_proc_set_min_resistance_incline_working(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_min_resistance_incline_working_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x6F_MinResistanceInclineWorking_NullCb_NoCrash) {
    lcb_proc_set_min_resistance_incline_working(NULL, &pkt0);
}

/* ============================================================================
 * Suite F: Command 0x80-0x8F (15 untested commands)
 * ============================================================================ */

/* --- 0x80 set_incline_percent: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x80_InclinePercent_Invoked) {
    lcb_proc_set_incline_percent(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_incline_percent_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x80_InclinePercent_NullCb_NoCrash) {
    lcb_proc_set_incline_percent(NULL, &pkt0);
}

/* --- 0x81 get_incline_percent: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x81_GetInclinePercent_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x03E8, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x81, 0x00, data, 2);
    lcb_proc_get_incline_percent(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_incline_percent_received_count, 1);
    EXPECT_EQ(ctx.last_incline_percent_received, 0x03E8);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x81_GetInclinePercent_WrongSize_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x81, 0x00, data, 1);
    lcb_proc_get_incline_percent(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_incline_percent_received_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x81_GetInclinePercent_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x81, 0x00, data, 2);
    lcb_proc_get_incline_percent(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x81_GetInclinePercent_NullPkt_NoCrash) {
    lcb_proc_get_incline_percent(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_incline_percent_received_count, 0);
}

/* --- 0x82 set_ecb_action: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x82_EcbAction_Invoked) {
    lcb_proc_set_ecb_action(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_ecb_action_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x82_EcbAction_NullCb_NoCrash) {
    lcb_proc_set_ecb_action(NULL, &pkt0);
}

/* --- 0x83 get_ecb_status: uint8 extraction, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x83_EcbStatus_ExtractsUint8) {
    uint8_t data[] = {0x5A};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x83, 0x00, data, 1);
    lcb_proc_get_ecb_status(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_ecb_status_count, 1);
    EXPECT_EQ(ctx.last_ecb_status, 0x5A);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x83_EcbStatus_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x83, 0x00, data, 2);
    lcb_proc_get_ecb_status(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_ecb_status_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x83_EcbStatus_NullCb_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x83, 0x00, data, 1);
    lcb_proc_get_ecb_status(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x83_EcbStatus_NullPkt_NoCrash) {
    lcb_proc_get_ecb_status(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_ecb_status_count, 0);
}

/* --- 0x84 get_ecb_count: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x84_EcbCount_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x1234, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x84, 0x00, data, 2);
    lcb_proc_get_ecb_count(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_ecb_count_count, 1);
    EXPECT_EQ(ctx.last_ecb_count, 0x1234);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x84_EcbCount_WrongSize_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x84, 0x00, data, 1);
    lcb_proc_get_ecb_count(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_ecb_count_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x84_EcbCount_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x84, 0x00, data, 2);
    lcb_proc_get_ecb_count(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x84_EcbCount_NullPkt_NoCrash) {
    lcb_proc_get_ecb_count(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_ecb_count_count, 0);
}

/* --- 0x85 set_electromagnetic_current: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x85_ElectromagneticCurrent_Invoked) {
    lcb_proc_set_electromagnetic_current(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_electromagnetic_current_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x85_ElectromagneticCurrent_NullCb_NoCrash) {
    lcb_proc_set_electromagnetic_current(NULL, &pkt0);
}

/* --- 0x86 set_resistance_type: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x86_ResistanceType_Invoked) {
    lcb_proc_set_resistance_type(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_resistance_type_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x86_ResistanceType_NullCb_NoCrash) {
    lcb_proc_set_resistance_type(NULL, &pkt0);
}

/* --- 0x87 set_begin_battery_charge: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x87_BeginBatteryCharge_Invoked) {
    lcb_proc_set_begin_battery_charge(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_begin_battery_charge_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x87_BeginBatteryCharge_NullCb_NoCrash) {
    lcb_proc_set_begin_battery_charge(NULL, &pkt0);
}

/* --- 0x88 get_battery_capacity: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x88_BatteryCapacity_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0064, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x88, 0x00, data, 2);
    lcb_proc_get_battery_capacity(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_battery_capacity_count, 1);
    EXPECT_EQ(ctx.last_battery_capacity, 0x0064);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x88_BatteryCapacity_WrongSize_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x88, 0x00, data, 1);
    lcb_proc_get_battery_capacity(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_battery_capacity_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x88_BatteryCapacity_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x88, 0x00, data, 2);
    lcb_proc_get_battery_capacity(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x88_BatteryCapacity_NullPkt_NoCrash) {
    lcb_proc_get_battery_capacity(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_battery_capacity_count, 0);
}

/* --- 0x8A set_climbmill_status: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x8A_SetClimbmillStatus_Invoked) {
    lcb_proc_set_climbmill_status(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_climbmill_status_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x8A_SetClimbmillStatus_NullCb_NoCrash) {
    lcb_proc_set_climbmill_status(NULL, &pkt0);
}

/* --- 0x8B get_climbmill_status: dual format (1-byte and 2-byte) --- */

TEST_F(LcbProcCmdTest, Proc_0x8B_GetClimbmillStatus_2Byte_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0xABCD, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8B, 0x00, data, 2);
    lcb_proc_get_climbmill_status(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_climbmill_status_received_count, 1);
    EXPECT_EQ(ctx.last_climbmill_status_received, 0xABCD);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8B_GetClimbmillStatus_1Byte_PromotsToUint16) {
    uint8_t data[] = {0x7F};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8B, 0x00, data, 1);
    lcb_proc_get_climbmill_status(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_climbmill_status_received_count, 1);
    EXPECT_EQ(ctx.last_climbmill_status_received, (uint16_t)0x7F);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8B_GetClimbmillStatus_Empty_Ignored) {
    lcb_proc_get_climbmill_status(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_climbmill_status_received_count, 0);
}

TEST_F(LcbProcCmdTest, Proc_0x8B_GetClimbmillStatus_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8B, 0x00, data, 2);
    lcb_proc_get_climbmill_status(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8B_GetClimbmillStatus_NullPkt_NoCrash) {
    lcb_proc_get_climbmill_status(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_climbmill_status_received_count, 0);
}

/* --- 0x8C set_e_stop_parameter: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x8C_EStopParameter_Invoked) {
    lcb_proc_set_e_stop_parameter(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_e_stop_parameter_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x8C_EStopParameter_NullCb_NoCrash) {
    lcb_proc_set_e_stop_parameter(NULL, &pkt0);
}

/* --- 0x8D get_brake_pressure_value: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x8D_BrakePressureValue_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0200, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8D, 0x00, data, 2);
    lcb_proc_get_brake_pressure_value(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_brake_pressure_value_count, 1);
    EXPECT_EQ(ctx.last_brake_pressure_value, 0x0200);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8D_BrakePressureValue_WrongSize_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8D, 0x00, data, 1);
    lcb_proc_get_brake_pressure_value(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_brake_pressure_value_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8D_BrakePressureValue_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8D, 0x00, data, 2);
    lcb_proc_get_brake_pressure_value(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8D_BrakePressureValue_NullPkt_NoCrash) {
    lcb_proc_get_brake_pressure_value(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_brake_pressure_value_count, 0);
}

/* --- 0x8E get_watts: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x8E_GetWatts_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x012C, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8E, 0x00, data, 2);
    lcb_proc_get_watts(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_watts_received_count, 1);
    EXPECT_EQ(ctx.last_watts_received, 0x012C);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8E_GetWatts_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8E, 0x00, data, 3);
    lcb_proc_get_watts(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_watts_received_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8E_GetWatts_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8E, 0x00, data, 2);
    lcb_proc_get_watts(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x8E_GetWatts_NullPkt_NoCrash) {
    lcb_proc_get_watts(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_watts_received_count, 0);
}

/* --- 0x8F set_climbmill_cal: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x8F_ClimbmillCal_Invoked) {
    lcb_proc_set_climbmill_cal(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_climbmill_cal_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x8F_ClimbmillCal_NullCb_NoCrash) {
    lcb_proc_set_climbmill_cal(NULL, &pkt0);
}

/* ============================================================================
 * Suite G: Command 0x40-0x4F (9 untested commands + 1 one-shot)
 * ============================================================================ */

/* --- 0x47 set_record_current: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x47_RecordCurrent_Invoked) {
    lcb_proc_set_record_current(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_record_current_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x47_RecordCurrent_NullCb_NoCrash) {
    lcb_proc_set_record_current(NULL, &pkt0);
}

/* --- 0x48 set_pwm: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x48_Pwm_Invoked) {
    lcb_proc_set_pwm(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_pwm_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x48_Pwm_NullCb_NoCrash) {
    lcb_proc_set_pwm(NULL, &pkt0);
}

/* --- 0x49 set_finish_calibrating: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x49_FinishCalibrating_Invoked) {
    lcb_proc_set_finish_calibrating(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_finish_calibrating_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x49_FinishCalibrating_NullCb_NoCrash) {
    lcb_proc_set_finish_calibrating(NULL, &pkt0);
}

/* --- 0x4A set_spm: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x4A_SetSpm_Invoked) {
    lcb_proc_set_spm(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_spm_set_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x4A_SetSpm_NullCb_NoCrash) {
    lcb_proc_set_spm(NULL, &pkt0);
}

/* --- 0x4B get_spm: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x4B_GetSpm_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x00B4, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4B, 0x00, data, 2);
    lcb_proc_get_spm(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_spm_received_count, 1);
    EXPECT_EQ(ctx.last_spm, 0x00B4);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4B_GetSpm_WrongSize_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4B, 0x00, data, 1);
    lcb_proc_get_spm(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_spm_received_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4B_GetSpm_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4B, 0x00, data, 2);
    lcb_proc_get_spm(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4B_GetSpm_NullPkt_NoCrash) {
    lcb_proc_get_spm(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_spm_received_count, 0);
}

/* --- 0x4B get_one_shot_spm: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x4B_GetOneShotSpm_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x00C8, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4B, 0x00, data, 2);
    lcb_proc_get_one_shot_spm(&cb, &pkt);
    EXPECT_EQ(ctx.on_one_shot_lcb_spm_count, 1);
    EXPECT_EQ(ctx.last_one_shot_spm, 0x00C8);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4B_GetOneShotSpm_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4B, 0x00, data, 3);
    lcb_proc_get_one_shot_spm(&cb, &pkt);
    EXPECT_EQ(ctx.on_one_shot_lcb_spm_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4B_GetOneShotSpm_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4B, 0x00, data, 2);
    lcb_proc_get_one_shot_spm(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4B_GetOneShotSpm_NullPkt_NoCrash) {
    lcb_proc_get_one_shot_spm(&cb, NULL);
    EXPECT_EQ(ctx.on_one_shot_lcb_spm_count, 0);
}

/* --- 0x4C set_cal_scaling: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x4C_SetCalScaling_Invoked) {
    lcb_proc_set_cal_scaling(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_cal_scaling_set_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x4C_SetCalScaling_NullCb_NoCrash) {
    lcb_proc_set_cal_scaling(NULL, &pkt0);
}

/* --- 0x4D get_cal_scaling: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x4D_GetCalScaling_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0190, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4D, 0x00, data, 2);
    lcb_proc_get_cal_scaling(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_cal_scaling_received_count, 1);
    EXPECT_EQ(ctx.last_cal_scaling, 0x0190);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4D_GetCalScaling_WrongSize_Ignored) {
    uint8_t data[] = {0x01};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4D, 0x00, data, 1);
    lcb_proc_get_cal_scaling(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_cal_scaling_received_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4D_GetCalScaling_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4D, 0x00, data, 2);
    lcb_proc_get_cal_scaling(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4D_GetCalScaling_NullPkt_NoCrash) {
    lcb_proc_get_cal_scaling(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_cal_scaling_received_count, 0);
}

/* --- 0x4E set_frame_cal_scaling: simple ACK --- */

TEST_F(LcbProcCmdTest, Proc_0x4E_SetFrameCalScaling_Invoked) {
    lcb_proc_set_frame_cal_scaling(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_frame_cal_scaling_set_count, 1);
}

TEST_F(LcbProcCmdTest, Proc_0x4E_SetFrameCalScaling_NullCb_NoCrash) {
    lcb_proc_set_frame_cal_scaling(NULL, &pkt0);
}

/* --- 0x4F get_frame_cal_scaling: uint16 BE, strict check --- */

TEST_F(LcbProcCmdTest, Proc_0x4F_GetFrameCalScaling_ExtractsUint16) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0258, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4F, 0x00, data, 2);
    lcb_proc_get_frame_cal_scaling(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_frame_cal_scaling_received_count, 1);
    EXPECT_EQ(ctx.last_frame_cal_scaling, 0x0258);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4F_GetFrameCalScaling_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4F, 0x00, data, 3);
    lcb_proc_get_frame_cal_scaling(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_frame_cal_scaling_received_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4F_GetFrameCalScaling_NullCb_NoCrash) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0001, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4F, 0x00, data, 2);
    lcb_proc_get_frame_cal_scaling(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Proc_0x4F_GetFrameCalScaling_NullPkt_NoCrash) {
    lcb_proc_get_frame_cal_scaling(&cb, NULL);
    EXPECT_EQ(ctx.on_lcb_frame_cal_scaling_received_count, 0);
}

/* ============================================================================
 * Suite H: Cross-cutting robustness tests
 * ============================================================================ */

/* --- H1: All simple ACK handlers with NULL cb produce no crash --- */

TEST_F(LcbProcCmdTest, Robustness_AllSimpleAck_NullCb_NoCrash) {
    /* Suite E simple ACKs */
    lcb_proc_set_reset_ecb(NULL, &pkt0);
    lcb_proc_set_ecb_position(NULL, &pkt0);
    lcb_proc_set_pwm_percent(NULL, &pkt0);
    lcb_proc_set_watts_cmd(NULL, &pkt0);
    lcb_proc_set_rpm_gear_ratio(NULL, &pkt0);
    lcb_proc_set_generator_pole_pair(NULL, &pkt0);
    lcb_proc_set_limit_rpm_charge(NULL, &pkt0);
    lcb_proc_set_limit_rpm_resistance(NULL, &pkt0);
    lcb_proc_set_machine_type(NULL, &pkt0);
    lcb_proc_set_power_off(NULL, &pkt0);
    lcb_proc_set_battery_charge_level(NULL, &pkt0);
    lcb_proc_set_min_resistance_incline_working(NULL, &pkt0);

    /* Suite F simple ACKs */
    lcb_proc_set_incline_percent(NULL, &pkt0);
    lcb_proc_set_ecb_action(NULL, &pkt0);
    lcb_proc_set_electromagnetic_current(NULL, &pkt0);
    lcb_proc_set_resistance_type(NULL, &pkt0);
    lcb_proc_set_begin_battery_charge(NULL, &pkt0);
    lcb_proc_set_climbmill_status(NULL, &pkt0);
    lcb_proc_set_e_stop_parameter(NULL, &pkt0);
    lcb_proc_set_climbmill_cal(NULL, &pkt0);

    /* Suite G simple ACKs */
    lcb_proc_set_record_current(NULL, &pkt0);
    lcb_proc_set_pwm(NULL, &pkt0);
    lcb_proc_set_finish_calibrating(NULL, &pkt0);
    lcb_proc_set_spm(NULL, &pkt0);
    lcb_proc_set_cal_scaling(NULL, &pkt0);
    lcb_proc_set_frame_cal_scaling(NULL, &pkt0);
}

/* --- H2: All typed data handlers with NULL cb produce no crash --- */

TEST_F(LcbProcCmdTest, Robustness_AllTypedData_NullCb_NoCrash) {
    uint8_t data2[2]; lcb_uint16_to_bytes_be(0x1234, data2);
    uint8_t data1[] = {0x42};

    lcb_packet_t pkt2 = make_rx_packet((lcb_command_t)0x64, 0x00, data2, 2);
    lcb_proc_get_ecb_adc(NULL, &pkt2);

    pkt2 = make_rx_packet((lcb_command_t)0x81, 0x00, data2, 2);
    lcb_proc_get_incline_percent(NULL, &pkt2);

    lcb_packet_t pkt1 = make_rx_packet((lcb_command_t)0x83, 0x00, data1, 1);
    lcb_proc_get_ecb_status(NULL, &pkt1);

    pkt2 = make_rx_packet((lcb_command_t)0x84, 0x00, data2, 2);
    lcb_proc_get_ecb_count(NULL, &pkt2);

    pkt2 = make_rx_packet((lcb_command_t)0x88, 0x00, data2, 2);
    lcb_proc_get_battery_capacity(NULL, &pkt2);

    pkt2 = make_rx_packet((lcb_command_t)0x8B, 0x00, data2, 2);
    lcb_proc_get_climbmill_status(NULL, &pkt2);

    pkt2 = make_rx_packet((lcb_command_t)0x8D, 0x00, data2, 2);
    lcb_proc_get_brake_pressure_value(NULL, &pkt2);

    pkt2 = make_rx_packet((lcb_command_t)0x8E, 0x00, data2, 2);
    lcb_proc_get_watts(NULL, &pkt2);

    pkt2 = make_rx_packet((lcb_command_t)0x4B, 0x00, data2, 2);
    lcb_proc_get_spm(NULL, &pkt2);
    lcb_proc_get_one_shot_spm(NULL, &pkt2);

    pkt2 = make_rx_packet((lcb_command_t)0x4D, 0x00, data2, 2);
    lcb_proc_get_cal_scaling(NULL, &pkt2);

    pkt2 = make_rx_packet((lcb_command_t)0x4F, 0x00, data2, 2);
    lcb_proc_get_frame_cal_scaling(NULL, &pkt2);
    lcb_packet_free(&pkt1);
    lcb_packet_free(&pkt2);
}

/* --- H3: All typed data handlers with NULL pkt produce no crash --- */

TEST_F(LcbProcCmdTest, Robustness_AllTypedData_NullPkt_NoCrash) {
    lcb_proc_get_ecb_adc(&cb, NULL);
    lcb_proc_get_incline_percent(&cb, NULL);
    lcb_proc_get_ecb_status(&cb, NULL);
    lcb_proc_get_ecb_count(&cb, NULL);
    lcb_proc_get_battery_capacity(&cb, NULL);
    lcb_proc_get_climbmill_status(&cb, NULL);
    lcb_proc_get_brake_pressure_value(&cb, NULL);
    lcb_proc_get_watts(&cb, NULL);
    lcb_proc_get_spm(&cb, NULL);
    lcb_proc_get_one_shot_spm(&cb, NULL);
    lcb_proc_get_cal_scaling(&cb, NULL);
    lcb_proc_get_frame_cal_scaling(&cb, NULL);

    /* Verify no callbacks were invoked */
    EXPECT_EQ(ctx.on_lcb_ecb_adc_count, 0);
    EXPECT_EQ(ctx.on_lcb_incline_percent_received_count, 0);
    EXPECT_EQ(ctx.on_lcb_ecb_status_count, 0);
    EXPECT_EQ(ctx.on_lcb_ecb_count_count, 0);
    EXPECT_EQ(ctx.on_lcb_battery_capacity_count, 0);
    EXPECT_EQ(ctx.on_lcb_climbmill_status_received_count, 0);
    EXPECT_EQ(ctx.on_lcb_brake_pressure_value_count, 0);
    EXPECT_EQ(ctx.on_lcb_watts_received_count, 0);
    EXPECT_EQ(ctx.on_lcb_spm_received_count, 0);
    EXPECT_EQ(ctx.on_one_shot_lcb_spm_count, 0);
    EXPECT_EQ(ctx.on_lcb_cal_scaling_received_count, 0);
    EXPECT_EQ(ctx.on_lcb_frame_cal_scaling_received_count, 0);
}

/* --- H4: Multiple calls accumulate count correctly --- */

TEST_F(LcbProcCmdTest, Robustness_MultipleCalls_ResetEcb_Accumulates) {
    lcb_proc_set_reset_ecb(&cb, &pkt0);
    lcb_proc_set_reset_ecb(&cb, &pkt0);
    lcb_proc_set_reset_ecb(&cb, &pkt0);
    EXPECT_EQ(ctx.on_lcb_reset_ecb_count, 3);
}

TEST_F(LcbProcCmdTest, Robustness_MultipleCalls_GetWatts_Accumulates) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0100, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x8E, 0x00, data, 2);
    lcb_proc_get_watts(&cb, &pkt);
    lcb_uint16_to_bytes_be(0x0200, data);
    pkt = make_rx_packet((lcb_command_t)0x8E, 0x00, data, 2);
    lcb_proc_get_watts(&cb, &pkt);
    lcb_uint16_to_bytes_be(0x0300, data);
    pkt = make_rx_packet((lcb_command_t)0x8E, 0x00, data, 2);
    lcb_proc_get_watts(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_watts_received_count, 3);
    EXPECT_EQ(ctx.last_watts_received, 0x0300);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCmdTest, Robustness_MultipleCalls_GetSpm_Accumulates) {
    uint8_t data[2];
    lcb_uint16_to_bytes_be(0x0050, data);
    lcb_packet_t pkt = make_rx_packet((lcb_command_t)0x4B, 0x00, data, 2);
    lcb_proc_get_spm(&cb, &pkt);
    lcb_uint16_to_bytes_be(0x0060, data);
    pkt = make_rx_packet((lcb_command_t)0x4B, 0x00, data, 2);
    lcb_proc_get_spm(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_spm_received_count, 2);
    EXPECT_EQ(ctx.last_spm, 0x0060);
    lcb_packet_free(&pkt);
}
