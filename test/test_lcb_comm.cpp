/**
 * @file test_lcb_comm.cpp
 * @brief Unit tests for lcb_comm.c - TX/RX packet rules, retry mechanism, RX error handling
 *
 * Testing strategy:
 * - #include "lcb_comm.c" directly to access static functions
 * - Never call lcb_comm_start() so no threads spawn and no serial port is needed
 * - Callback tracking via TestContext struct
 */

#include "gtest/gtest.h"

extern "C" {
    /* Include the .c file directly to access static functions:
     * lcb_check_status_bits, lcb_process_received_packet,
     * lcb_add_command_packet, lcb_add_command_packet_with_mode */
    #include "../lcb_protocol/src/Comm/lcb_comm.c"
}

/* ============================================================================
 * Test Infrastructure: Callback tracking context
 * ============================================================================ */

struct TestContext {
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

    /* Indoor bike sensor data (simplified - just track count) */
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

    void reset() {
        memset(this, 0, sizeof(*this));
    }
};

/* ============================================================================
 * Static callback trampoline functions
 * ============================================================================ */

/* Simple ACK trampolines */
static void cb_on_lcb_initial(void *ud) { ((TestContext*)ud)->on_lcb_initial_count++; }
static void cb_on_lcb_calibrate(void *ud) { ((TestContext*)ud)->on_lcb_calibrate_count++; }
static void cb_on_lcb_motor_rpm(void *ud) { ((TestContext*)ud)->on_lcb_motor_rpm_count++; }
static void cb_on_lcb_skip_curr_error(void *ud) { ((TestContext*)ud)->on_lcb_skip_curr_error_count++; }
static void cb_on_lcb_cmd_console_power(void *ud) { ((TestContext*)ud)->on_lcb_cmd_console_power_count++; }
static void cb_on_lcb_operating_status(void *ud) { ((TestContext*)ud)->on_lcb_operating_status_count++; }
static void cb_on_lcb_incline_action(void *ud) { ((TestContext*)ud)->on_lcb_incline_action_count++; }
static void cb_on_lcb_inc_pwm_step(void *ud) { ((TestContext*)ud)->on_lcb_inc_pwm_step_count++; }
static void cb_on_lcb_dec_pwm_step(void *ud) { ((TestContext*)ud)->on_lcb_dec_pwm_step_count++; }
static void cb_on_lcb_stop_pwm_step(void *ud) { ((TestContext*)ud)->on_lcb_stop_pwm_step_count++; }
static void cb_on_lcb_tune_end_incline(void *ud) { ((TestContext*)ud)->on_lcb_tune_end_incline_count++; }
static void cb_on_lcb_tune_end_incline2(void *ud) { ((TestContext*)ud)->on_lcb_tune_end_incline2_count++; }
static void cb_on_lcb_incline_stroke(void *ud) { ((TestContext*)ud)->on_lcb_incline_stroke_count++; }
static void cb_on_lcb_motor_specification(void *ud) { ((TestContext*)ud)->on_lcb_motor_specification_count++; }
static void cb_on_lcb_motor_power_off(void *ud) { ((TestContext*)ud)->on_lcb_motor_power_off_count++; }
static void cb_on_lcb_dci_force_incline(void *ud) { ((TestContext*)ud)->on_lcb_dci_force_incline_count++; }
static void cb_on_lcb_dci_incline_range_set(void *ud) { ((TestContext*)ud)->on_lcb_dci_incline_range_set_count++; }
static void cb_on_lcb_dci_rpm_speed_range_set(void *ud) { ((TestContext*)ud)->on_lcb_dci_rpm_speed_range_set_count++; }
static void cb_on_lcb_dci_manual_calibration(void *ud) { ((TestContext*)ud)->on_lcb_dci_manual_calibration_count++; }
static void cb_on_lcb_reset_ecb(void *ud) { ((TestContext*)ud)->on_lcb_reset_ecb_count++; }
static void cb_on_lcb_ecb_position(void *ud) { ((TestContext*)ud)->on_lcb_ecb_position_count++; }
static void cb_on_lcb_pwm_percent(void *ud) { ((TestContext*)ud)->on_lcb_pwm_percent_count++; }
static void cb_on_lcb_watts_cmd(void *ud) { ((TestContext*)ud)->on_lcb_watts_cmd_count++; }
static void cb_on_lcb_rpm_gear_ratio(void *ud) { ((TestContext*)ud)->on_lcb_rpm_gear_ratio_count++; }
static void cb_on_lcb_generator_pole_pair(void *ud) { ((TestContext*)ud)->on_lcb_generator_pole_pair_count++; }
static void cb_on_lcb_limit_rpm_charge(void *ud) { ((TestContext*)ud)->on_lcb_limit_rpm_charge_count++; }
static void cb_on_lcb_limit_rpm_resistance(void *ud) { ((TestContext*)ud)->on_lcb_limit_rpm_resistance_count++; }
static void cb_on_lcb_machine_type(void *ud) { ((TestContext*)ud)->on_lcb_machine_type_count++; }
static void cb_on_lcb_power_off(void *ud) { ((TestContext*)ud)->on_lcb_power_off_count++; }
static void cb_on_lcb_battery_charge_level(void *ud) { ((TestContext*)ud)->on_lcb_battery_charge_level_count++; }
static void cb_on_lcb_min_resistance_incline_working(void *ud) { ((TestContext*)ud)->on_lcb_min_resistance_incline_working_count++; }
static void cb_on_lcb_incline_percent(void *ud) { ((TestContext*)ud)->on_lcb_incline_percent_count++; }
static void cb_on_lcb_ecb_action(void *ud) { ((TestContext*)ud)->on_lcb_ecb_action_count++; }
static void cb_on_lcb_electromagnetic_current(void *ud) { ((TestContext*)ud)->on_lcb_electromagnetic_current_count++; }
static void cb_on_lcb_resistance_type(void *ud) { ((TestContext*)ud)->on_lcb_resistance_type_count++; }
static void cb_on_lcb_begin_battery_charge(void *ud) { ((TestContext*)ud)->on_lcb_begin_battery_charge_count++; }
static void cb_on_lcb_climbmill_status(void *ud) { ((TestContext*)ud)->on_lcb_climbmill_status_count++; }
static void cb_on_lcb_e_stop_parameter(void *ud) { ((TestContext*)ud)->on_lcb_e_stop_parameter_count++; }
static void cb_on_lcb_climbmill_cal(void *ud) { ((TestContext*)ud)->on_lcb_climbmill_cal_count++; }
static void cb_on_lcb_record_current(void *ud) { ((TestContext*)ud)->on_lcb_record_current_count++; }
static void cb_on_lcb_pwm(void *ud) { ((TestContext*)ud)->on_lcb_pwm_count++; }
static void cb_on_lcb_finish_calibrating(void *ud) { ((TestContext*)ud)->on_lcb_finish_calibrating_count++; }
static void cb_on_lcb_spm(void *ud) { ((TestContext*)ud)->on_lcb_spm_set_count++; }
static void cb_on_lcb_cal_scaling(void *ud) { ((TestContext*)ud)->on_lcb_cal_scaling_set_count++; }
static void cb_on_lcb_frame_cal_scaling(void *ud) { ((TestContext*)ud)->on_lcb_frame_cal_scaling_set_count++; }
static void cb_on_lcb_indoor_bike_stepper_motor_position(void *ud) { ((TestContext*)ud)->on_lcb_indoor_bike_stepper_motor_position_count++; }
static void cb_on_lcb_indoor_bike_calc_para(void *ud) { ((TestContext*)ud)->on_lcb_indoor_bike_calc_para_set_count++; }
static void cb_on_lcb_indoor_bike_resistance_level(void *ud) { ((TestContext*)ud)->on_lcb_indoor_bike_resistance_level_count++; }

/* Typed data trampolines */
static void cb_on_lcb_status(void *ud, uint8_t status) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_status_count++; ctx->last_lcb_status = status;
}
static void cb_on_lcb_error_code(void *ud, uint16_t error_code) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_error_code_count++; ctx->last_error_code = error_code;
}
static void cb_on_return_error_code_to_ui(void *ud, uint16_t error_code) {
    auto *ctx = (TestContext*)ud; ctx->on_return_error_code_to_ui_count++; ctx->last_ui_error_code = error_code;
}
static void cb_on_lcb_version(void *ud, uint16_t version) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_version_count++; ctx->last_version = version;
}
static void cb_on_lcb_motor_speed_rtn_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_motor_speed_rtn_rpm_count++; ctx->last_motor_speed_rtn_rpm = rpm;
}
static void cb_on_lcb_motor_rpm_received(void *ud, uint16_t rpm) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_motor_rpm_received_count++; ctx->last_motor_rpm = rpm;
}
static void cb_on_one_shot_lcb_motor_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (TestContext*)ud; ctx->on_one_shot_lcb_motor_rpm_count++; ctx->last_one_shot_motor_rpm = rpm;
}
static void cb_on_lcb_ecb_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_ecb_rpm_count++; ctx->last_ecb_rpm = rpm;
}
static void cb_on_one_shot_lcb_ecb_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (TestContext*)ud; ctx->on_one_shot_lcb_ecb_rpm_count++; ctx->last_one_shot_ecb_rpm = rpm;
}
static void cb_on_lcb_spm_received(void *ud, uint16_t spm) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_spm_received_count++; ctx->last_spm = spm;
}
static void cb_on_one_shot_lcb_spm(void *ud, uint16_t spm) {
    auto *ctx = (TestContext*)ud; ctx->on_one_shot_lcb_spm_count++; ctx->last_one_shot_spm = spm;
}
static void cb_on_lcb_spe_ext_cmd(void *ud, uint8_t cmd) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_spe_ext_cmd_count++; ctx->last_spe_ext_cmd = cmd;
}
static void cb_on_lcb_motor_type(void *ud, uint8_t type) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_motor_type_count++; ctx->last_motor_type = type;
}
static void cb_on_lcb_dci_data_max_len(void *ud, uint8_t len) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_dci_data_max_len_count++; ctx->last_dci_data_max_len = len;
}
static void cb_on_lcb_roller_rpm(void *ud, uint16_t rpm) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_roller_rpm_count++; ctx->last_roller_rpm = rpm;
}
static void cb_on_lcb_protocol_version(void *ud, uint16_t version) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_protocol_version_count++; ctx->last_protocol_version = version;
}
static void cb_on_lcb_incline_loc(void *ud, uint16_t loc) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_incline_loc_count++; ctx->last_incline_loc = loc;
}
static void cb_on_lcb_incline_loc_received(void *ud, uint16_t loc) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_incline_loc_received_count++; ctx->last_incline_loc_received = loc;
}
static void cb_on_lcb_compensation_vol(void *ud, uint16_t vol) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_compensation_vol_count++; ctx->last_compensation_vol = vol;
}
static void cb_on_lcb_update_program(void *ud, uint8_t resp) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_update_program_count++; ctx->last_update_program_response = resp;
}
static void cb_on_lcb_treadmill_in_used(void *ud, uint8_t status) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_treadmill_in_used_count++; ctx->last_treadmill_in_used = status;
}
static void cb_on_lcb_treadmill_ext_status(void *ud, uint16_t status) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_treadmill_ext_status_count++; ctx->last_treadmill_ext_status = status;
}
static void cb_on_lcb_treadmill_ecb_action(void *ud, uint8_t data) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_treadmill_ecb_action_count++; ctx->last_treadmill_ecb_action = data;
}
static void cb_on_lcb_dci_incline_range(void *ud, uint32_t range) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_dci_incline_range_count++; ctx->last_dci_incline_range = range;
}
static void cb_on_lcb_dci_rpm_speed_range(void *ud, uint32_t range) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_dci_rpm_speed_range_count++; ctx->last_dci_rpm_speed_range = range;
}
static void cb_on_lcb_universal(void *ud, uint16_t val) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_universal_count++; ctx->last_universal_value = val;
}
static void cb_on_lcb_ecb_adc(void *ud, uint16_t adc) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_ecb_adc_count++; ctx->last_ecb_adc = adc;
}
static void cb_on_lcb_battery_voltage(void *ud, uint16_t voltage) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_battery_voltage_count++; ctx->last_battery_voltage = voltage;
}
static void cb_on_lcb_ecb_status(void *ud, uint8_t status) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_ecb_status_count++; ctx->last_ecb_status = status;
}
static void cb_on_lcb_ecb_count(void *ud, uint16_t count) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_ecb_count_count++; ctx->last_ecb_count = count;
}
static void cb_on_lcb_battery_capacity(void *ud, uint16_t cap) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_battery_capacity_count++; ctx->last_battery_capacity = cap;
}
static void cb_on_lcb_climbmill_status_received(void *ud, uint16_t status) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_climbmill_status_received_count++; ctx->last_climbmill_status_received = status;
}
static void cb_on_lcb_brake_pressure_value(void *ud, uint16_t val) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_brake_pressure_value_count++; ctx->last_brake_pressure_value = val;
}
static void cb_on_lcb_watts_received(void *ud, uint16_t watts) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_watts_received_count++; ctx->last_watts_received = watts;
}
static void cb_on_lcb_incline_percent_received(void *ud, uint16_t pct) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_incline_percent_received_count++; ctx->last_incline_percent_received = pct;
}
static void cb_on_lcb_cal_scaling_received(void *ud, uint16_t val) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_cal_scaling_received_count++; ctx->last_cal_scaling = val;
}
static void cb_on_lcb_frame_cal_scaling_received(void *ud, uint16_t val) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_frame_cal_scaling_received_count++; ctx->last_frame_cal_scaling = val;
}

/* Status bit callbacks */
static void cb_on_incline_motor_downward(void *ud, bool val) {
    auto *ctx = (TestContext*)ud; ctx->on_incline_motor_downward_count++; ctx->last_incline_motor_downward = val;
}
static void cb_on_incline_motor_upward(void *ud, bool val) {
    auto *ctx = (TestContext*)ud; ctx->on_incline_motor_upward_count++; ctx->last_incline_motor_upward = val;
}
static void cb_on_main_motor_status(void *ud, bool val) {
    auto *ctx = (TestContext*)ud; ctx->on_main_motor_status_count++; ctx->last_main_motor_status = val;
}
static void cb_on_encoder_rpm_status(void *ud, bool val) {
    auto *ctx = (TestContext*)ud; ctx->on_encoder_rpm_status_count++; ctx->last_encoder_rpm_status = val;
}
static void cb_on_battery_capacity_status(void *ud, bool val) {
    auto *ctx = (TestContext*)ud; ctx->on_battery_capacity_status_count++; ctx->last_battery_capacity_status = val;
}
static void cb_on_command_error_status(void *ud, bool val) {
    auto *ctx = (TestContext*)ud; ctx->on_command_error_status_count++; ctx->last_command_error_status = val;
}
static void cb_on_lcb_error_status_detected(void *ud, bool val) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_error_status_count++; ctx->last_lcb_error_status = val;
}
static void cb_on_initial_calibration_status(void *ud, bool val) {
    auto *ctx = (TestContext*)ud; ctx->on_initial_calibration_status_count++; ctx->last_initial_calibration_status = val;
}

/* Error injection callbacks */
static void cb_on_estop_machine_stop(void *ud) { ((TestContext*)ud)->on_estop_machine_stop_count++; }
static void cb_on_estop_mcb_error(void *ud) { ((TestContext*)ud)->on_estop_mcb_error_count++; }
static void cb_on_estop_unlock(void *ud) { ((TestContext*)ud)->on_estop_unlock_count++; }
static void cb_on_tm_no_rpm_error(void *ud) { ((TestContext*)ud)->on_tm_no_rpm_error_count++; }
static void cb_on_set_adv_logging_status(void *ud) { ((TestContext*)ud)->on_set_adv_logging_status_count++; }

/* Raw bytes callbacks */
static void cb_on_lcb_dci_version(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_dci_version_count++;
    ctx->last_dci_version_len = len;
    if (data && len <= sizeof(ctx->last_dci_version_data))
        memcpy(ctx->last_dci_version_data, data, len);
}
static void cb_on_lcb_dci_env(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_dci_env_count++;
    ctx->last_dci_env_len = len;
    if (data && len <= sizeof(ctx->last_dci_env_data))
        memcpy(ctx->last_dci_env_data, data, len);
}
static void cb_on_lcb_cmd_serial_number(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_cmd_serial_number_count++;
    ctx->last_serial_number_len = len;
    if (data && len <= sizeof(ctx->last_serial_number_data))
        memcpy(ctx->last_serial_number_data, data, len);
}
static void cb_on_lcb_main_motor_info(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_main_motor_info_count++;
    ctx->last_main_motor_info_len = len;
    if (data && len <= sizeof(ctx->last_main_motor_info_data))
        memcpy(ctx->last_main_motor_info_data, data, len);
}
static void cb_on_lcb_treadmill_operate(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (TestContext*)ud; ctx->on_lcb_treadmill_operate_count++;
    ctx->last_treadmill_operate_len = len;
    if (data && len <= sizeof(ctx->last_treadmill_operate_data))
        memcpy(ctx->last_treadmill_operate_data, data, len);
}

/* Indoor bike sensor data (simplified) */
static void cb_on_lcb_indoor_bike_sensor_data(void *ud, const indoor_bike_sensor_data_t *d) {
    ((TestContext*)ud)->on_lcb_indoor_bike_sensor_data_count++;
}
static void cb_on_one_shot_lcb_indoor_bike_sensor_data(void *ud, const indoor_bike_sensor_data_t *d) {
    ((TestContext*)ud)->on_one_shot_lcb_indoor_bike_sensor_data_count++;
}

/* Unknown command */
static void cb_on_unknown_command(void *ud, uint8_t cmd, const uint8_t *data, uint8_t len) {
    auto *ctx = (TestContext*)ud; ctx->on_unknown_command_count++; ctx->last_unknown_command = cmd;
}

/* TX callbacks */
static void cb_on_treadmill_rpm_value_set(void *ud, uint16_t rpm) {
    auto *ctx = (TestContext*)ud; ctx->on_treadmill_rpm_value_set_count++; ctx->last_treadmill_rpm_set = rpm;
}
static void cb_on_treadmill_adc_value_set(void *ud, uint16_t adc) {
    auto *ctx = (TestContext*)ud; ctx->on_treadmill_adc_value_set_count++; ctx->last_treadmill_adc_set = adc;
}

/* ============================================================================
 * Helper: wire all callbacks
 * ============================================================================ */
static void setup_all_callbacks(lcb_comm_callbacks_t *cb, TestContext *ctx) {
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
 * Test Fixtures
 * ============================================================================ */

/* Fixture for TX queue tests - uses public API, no threads */
class LcbCommTxTest : public ::testing::Test {
protected:
    lcb_comm_t *comm;
    TestContext ctx;
    lcb_comm_callbacks_t cb;

    void SetUp() override {
        ctx.reset();
        comm = lcb_comm_create();
        ASSERT_NE(comm, nullptr);
        setup_all_callbacks(&cb, &ctx);
        lcb_comm_register_callbacks(comm, &cb);
    }

    void TearDown() override {
        lcb_comm_destroy(comm);
    }
};

/* Fixture for RX processing tests - uses static functions directly */
class LcbCommRxTest : public ::testing::Test {
protected:
    lcb_comm_t *comm;
    TestContext ctx;
    lcb_comm_callbacks_t cb;

    void SetUp() override {
        ctx.reset();
        comm = lcb_comm_create();
        ASSERT_NE(comm, nullptr);
        setup_all_callbacks(&cb, &ctx);
        lcb_comm_register_callbacks(comm, &cb);
    }

    void TearDown() override {
        lcb_comm_destroy(comm);
    }

    void processPacket(lcb_command_t cmd, uint8_t status,
                       const uint8_t *data, uint8_t data_len) {
        lcb_packet_t pkt = make_rx_packet(cmd, status, data, data_len);
        lcb_process_received_packet(comm, &pkt);
        lcb_packet_free(&pkt);
    }
};

/* Fixture for lcb_proc_* function tests - no comm instance needed */
class LcbProcCommTest : public ::testing::Test {
protected:
    TestContext ctx;
    lcb_comm_callbacks_t cb;

    void SetUp() override {
        ctx.reset();
        setup_all_callbacks(&cb, &ctx);
    }
};

/* ============================================================================
 * Category A: TX Queue Tests
 * ============================================================================ */

TEST_F(LcbCommTxTest, CreateReturnsNonNull) {
    ASSERT_NE(comm, nullptr);
}

TEST_F(LcbCommTxTest, DestroyNull_NoCrash) {
    lcb_comm_destroy(NULL);
}

TEST_F(LcbCommTxTest, InitialQueueSizeIsZero) {
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 0u);
}

TEST_F(LcbCommTxTest, RegisterCallbacksSuccess) {
    ASSERT_EQ(lcb_comm_register_callbacks(comm, &cb), 0);
}

TEST_F(LcbCommTxTest, RegisterCallbacksNullComm) {
    ASSERT_EQ(lcb_comm_register_callbacks(NULL, &cb), -1);
}

TEST_F(LcbCommTxTest, RegisterCallbacksNullCallbacks) {
    ASSERT_EQ(lcb_comm_register_callbacks(comm, NULL), -1);
}

TEST_F(LcbCommTxTest, UnregisterCallbacksSuccess) {
    ASSERT_EQ(lcb_comm_unregister_callbacks(comm), 0);
    ASSERT_FALSE(comm->has_callbacks);
}

TEST_F(LcbCommTxTest, SetInitial_QueuesOnePacket) {
    lcb_comm_set_lcb_initial(comm);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 1u);
}

TEST_F(LcbCommTxTest, SetMotorRpm_DataEncodedBigEndian) {
    lcb_comm_set_lcb_motor_rpm(comm, 0x1234);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 1u);

    lcb_packet_t pkt;
    ASSERT_EQ(lcb_packet_queue_remove(&comm->tx_queue, &pkt), 0);
    ASSERT_EQ(pkt.command, LCB_CMD_SET_MOTOR_RPM);

    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(&pkt, &data_len);
    ASSERT_EQ(data_len, 2);
    ASSERT_EQ(data[0], 0x12);
    ASSERT_EQ(data[1], 0x34);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommTxTest, SetConsolePower_DataEncodedBigEndian) {
    lcb_comm_set_lcb_cmd_console_power(comm, 0xABCD);

    lcb_packet_t pkt;
    ASSERT_EQ(lcb_packet_queue_remove(&comm->tx_queue, &pkt), 0);
    ASSERT_EQ(pkt.command, LCB_CMD_SET_CMD_CONSOLE_POWER);

    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(&pkt, &data_len);
    ASSERT_EQ(data_len, 2);
    ASSERT_EQ(data[0], 0xAB);
    ASSERT_EQ(data[1], 0xCD);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommTxTest, SetOperatingStatus_SingleByteData) {
    lcb_comm_set_lcb_operating_status(comm, 0x01);

    lcb_packet_t pkt;
    ASSERT_EQ(lcb_packet_queue_remove(&comm->tx_queue, &pkt), 0);
    ASSERT_EQ(pkt.command, LCB_CMD_SET_OPERATING_STATUS);

    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(&pkt, &data_len);
    ASSERT_EQ(data_len, 1);
    ASSERT_EQ(data[0], 0x01);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommTxTest, SetUpdateProgram_RawBytesPassthrough) {
    uint8_t raw[] = {0xDE, 0xAD, 0xBE, 0xEF};
    lcb_comm_set_lcb_update_program(comm, raw, 4);

    lcb_packet_t pkt;
    ASSERT_EQ(lcb_packet_queue_remove(&comm->tx_queue, &pkt), 0);

    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(&pkt, &data_len);
    ASSERT_EQ(data_len, 4);
    ASSERT_EQ(memcmp(data, raw, 4), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommTxTest, AlwaysAdd_DuplicateCommandsAllQueued) {
    lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ALWAYS_ADD);
    lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ALWAYS_ADD);
    lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ALWAYS_ADD);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 3u);
}

TEST_F(LcbCommTxTest, AddIfNotQueued_SecondDuplicateRejected) {
    lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED);
    lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 1u);
}

TEST_F(LcbCommTxTest, AddIfNotQueued_DifferentCommandsAllAdded) {
    lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED);
    lcb_comm_get_lcb_motor_rpm(comm, LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 2u);
}

TEST_F(LcbCommTxTest, ReplaceSimilar_ReplacesExistingCommand) {
    lcb_comm_set_lcb_motor_rpm_highest_priority(comm, 100);
    lcb_comm_set_lcb_motor_rpm_highest_priority(comm, 200);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 1u);
}

TEST_F(LcbCommTxTest, ReplaceSimilar_UpdatesDataPayload) {
    lcb_comm_set_lcb_motor_rpm_highest_priority(comm, 100);
    lcb_comm_set_lcb_motor_rpm_highest_priority(comm, 200);

    lcb_packet_t pkt;
    ASSERT_EQ(lcb_packet_queue_remove(&comm->tx_queue, &pkt), 0);

    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(&pkt, &data_len);
    ASSERT_EQ(data_len, 2);
    /* 200 = 0x00C8 big-endian */
    ASSERT_EQ(data[0], 0x00);
    ASSERT_EQ(data[1], 0xC8);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommTxTest, HighestPriority_DequeuedFirst) {
    /* Enqueue low, normal, highest priority */
    lcb_comm_get_lcb_status_low_priority(comm, LCB_QUEUE_MODE_ALWAYS_ADD);     /* LOW=4 */
    lcb_comm_set_lcb_initial(comm);                                              /* NORMAL=3 */
    lcb_comm_get_lcb_version_highest_priority(comm);                             /* HIGHEST=1 */

    lcb_packet_t pkt;

    /* First dequeued should be highest priority */
    ASSERT_EQ(lcb_packet_queue_remove(&comm->tx_queue, &pkt), 0);
    ASSERT_EQ(pkt.command, LCB_CMD_GET_VERSION);
    lcb_packet_free(&pkt);

    /* Second: normal priority */
    ASSERT_EQ(lcb_packet_queue_remove(&comm->tx_queue, &pkt), 0);
    ASSERT_EQ(pkt.command, LCB_CMD_SET_INITIALIZE);
    lcb_packet_free(&pkt);

    /* Third: low priority */
    ASSERT_EQ(lcb_packet_queue_remove(&comm->tx_queue, &pkt), 0);
    ASSERT_EQ(pkt.command, LCB_CMD_GET_STATUS);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommTxTest, OneShotMotorRpm_SetsFlag) {
    lcb_comm_get_one_shot_lcb_motor_rpm_high_priority(comm);
    ASSERT_TRUE(comm->is_one_shot_rpm);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 1u);
}

TEST_F(LcbCommTxTest, OneShotEcbRpm_SetsFlag) {
    lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(comm);
    ASSERT_TRUE(comm->is_one_shot_rpm);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 1u);
}

TEST_F(LcbCommTxTest, OneShotSpm_SetsFlag) {
    lcb_comm_get_one_shot_lcb_spm_high_priority(comm);
    ASSERT_TRUE(comm->is_one_shot_spm);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 1u);
}

TEST_F(LcbCommTxTest, OneShotSensorData_SetsFlag) {
    lcb_comm_get_one_shot_lcb_indoor_bike_sensor_data_high_priority(comm);
    ASSERT_TRUE(comm->is_one_shot_sensor_data);
    ASSERT_EQ(lcb_comm_get_tx_queue_size(comm), 1u);
}

TEST_F(LcbCommTxTest, QueueSizeNullComm_ReturnsZero) {
    ASSERT_EQ(lcb_comm_get_tx_queue_size(NULL), 0u);
}

/* ============================================================================
 * Category B: RX Response Processing Tests (lcb_proc_* direct)
 * ============================================================================ */

TEST_F(LcbProcCommTest, ProcSetInitial_FiresCallback) {
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_INITIALIZE, 0x00, NULL, 0);
    lcb_proc_set_initial(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_initial_count, 1);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcSetCalibrate_FiresCallback) {
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_CALIBRATE, 0x00, NULL, 0);
    lcb_proc_set_calibrate(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_calibrate_count, 1);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcSetMotorRpm_FiresCallback) {
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_MOTOR_RPM, 0x00, NULL, 0);
    lcb_proc_set_motor_rpm(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_motor_rpm_count, 1);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetErrorCode_ExtractsUint16) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(0xABCD, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_ERROR_CODE, 0x00, data, 2);
    lcb_proc_get_error_code(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.last_error_code, 0xABCD);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetVersion_ExtractsUint16) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(0x0102, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_VERSION, 0x00, data, 2);
    lcb_proc_get_version(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_version_count, 1);
    ASSERT_EQ(ctx.last_version, 0x0102);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetProtocolVersion_ExtractsUint16) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(0x0305, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_PROTOCOL_VERSION, 0x00, data, 2);
    lcb_proc_get_protocol_version(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_protocol_version_count, 1);
    ASSERT_EQ(ctx.last_protocol_version, 0x0305);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetMotorSpeedRtnRpm_ExtractsUint16) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(1500, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_MOTOR_SPEED_RTN_RPM, 0x00, data, 2);
    lcb_proc_set_motor_speed_rtn_rpm(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_motor_speed_rtn_rpm_count, 1);
    ASSERT_EQ(ctx.last_motor_speed_rtn_rpm, 1500);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetRollerRpm_ExtractsUint16) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(3200, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_ROLLER_RPM, 0x00, data, 2);
    lcb_proc_get_roller_rpm(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_roller_rpm_count, 1);
    ASSERT_EQ(ctx.last_roller_rpm, 3200);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetMotorRpm_ExtractsUint16) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(2400, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_MOTOR_RPM, 0x00, data, 2);
    lcb_proc_get_motor_rpm(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_motor_rpm_received_count, 1);
    ASSERT_EQ(ctx.last_motor_rpm, 2400);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetEcbRpm_ExtractsUint16) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(800, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_ECB_RPM, 0x00, data, 2);
    lcb_proc_get_ecb_rpm(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_ecb_rpm_count, 1);
    ASSERT_EQ(ctx.last_ecb_rpm, 800);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetBatteryVoltage_ExtractsUint16) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(1200, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_BATTERY_VOLTAGE, 0x00, data, 2);
    lcb_proc_get_battery_voltage(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_battery_voltage_count, 1);
    ASSERT_EQ(ctx.last_battery_voltage, 1200);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetMotorType_ExtractsUint8) {
    uint8_t data[] = {0x42};
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_MOTOR_TYPE, 0x00, data, 1);
    lcb_proc_get_motor_type(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_motor_type_count, 1);
    ASSERT_EQ(ctx.last_motor_type, 0x42);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcSetSpeExtCmd_ExtractsUint8) {
    uint8_t data[] = {0x07};
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_SPE_EXT_CMD, 0x00, data, 1);
    lcb_proc_set_spe_ext_cmd(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_spe_ext_cmd_count, 1);
    ASSERT_EQ(ctx.last_spe_ext_cmd, 0x07);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetDciVersion_PassesAllBytes) {
    uint8_t data[27];
    for (int i = 0; i < 27; i++) data[i] = (uint8_t)(i + 0x10);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_DCI_VERSION, 0x00, data, 27);
    lcb_proc_get_dci_version(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_dci_version_count, 1);
    ASSERT_EQ(ctx.last_dci_version_len, 27);
    ASSERT_EQ(memcmp(ctx.last_dci_version_data, data, 27), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetDciEnv_PassesAllBytes) {
    uint8_t data[84];
    for (int i = 0; i < 84; i++) data[i] = (uint8_t)(i);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_DCI_ENV, 0x00, data, 84);
    lcb_proc_get_dci_env(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_dci_env_count, 1);
    ASSERT_EQ(ctx.last_dci_env_len, 84);
    ASSERT_EQ(memcmp(ctx.last_dci_env_data, data, 84), 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetErrorCode_FiresBothCallbacks) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(0x1234, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_ERROR_CODE, 0x00, data, 2);
    lcb_proc_get_error_code(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    ASSERT_EQ(ctx.last_ui_error_code, 0x1234);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetErrorCode_ZeroDoesNotFireUI) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(0x0000, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_ERROR_CODE, 0x00, data, 2);
    lcb_proc_get_error_code(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.on_return_error_code_to_ui_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetErrorCode_WrongSize_NoCallback) {
    uint8_t data[] = {0xAB}; /* Only 1 byte, expects 2 */
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_ERROR_CODE, 0x00, data, 1);
    lcb_proc_get_error_code(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetVersion_WrongSize_NoCallback) {
    uint8_t data[] = {0xAB}; /* Only 1 byte, expects 2 */
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_VERSION, 0x00, data, 1);
    lcb_proc_get_version(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_version_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetDciVersion_WrongSize_NoCallback) {
    uint8_t data[10]; /* Only 10 bytes, expects 27 */
    memset(data, 0, 10);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_DCI_VERSION, 0x00, data, 10);
    lcb_proc_get_dci_version(&cb, &pkt);
    ASSERT_EQ(ctx.on_lcb_dci_version_count, 0);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcSetInitial_NullCallbacks_NoCrash) {
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_INITIALIZE, 0x00, NULL, 0);
    lcb_proc_set_initial(NULL, &pkt);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetErrorCode_NullCallbackField_NoCrash) {
    lcb_comm_callbacks_t cb_partial;
    lcb_comm_callbacks_init(&cb_partial);
    cb_partial.user_data = &ctx;
    /* on_lcb_error_code is NULL, on_return_error_code_to_ui is NULL */

    uint8_t data[2]; lcb_uint16_to_bytes_be(0x1234, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_ERROR_CODE, 0x00, data, 2);
    lcb_proc_get_error_code(&cb_partial, &pkt);
    /* Should not crash */
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetOneShotMotorRpm_FiresOneShotCallback) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(1800, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_MOTOR_RPM, 0x00, data, 2);
    lcb_proc_get_one_shot_motor_rpm(&cb, &pkt);
    ASSERT_EQ(ctx.on_one_shot_lcb_motor_rpm_count, 1);
    ASSERT_EQ(ctx.last_one_shot_motor_rpm, 1800);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcGetOneShotEcbRpm_FiresOneShotCallback) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(600, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_ECB_RPM, 0x00, data, 2);
    lcb_proc_get_one_shot_ecb_rpm(&cb, &pkt);
    ASSERT_EQ(ctx.on_one_shot_lcb_ecb_rpm_count, 1);
    ASSERT_EQ(ctx.last_one_shot_ecb_rpm, 600);
    lcb_packet_free(&pkt);
}

/* ============================================================================
 * Category C: Status Bit Tests
 * ============================================================================ */

TEST_F(LcbCommRxTest, StatusBit0_InclineDown_FiresOnChange) {
    /* Initial state: all off */
    lcb_check_status_bits(comm, 0x00);
    ASSERT_EQ(ctx.on_incline_motor_downward_count, 0); /* No change from initial false */

    /* Set bit 0 */
    lcb_check_status_bits(comm, 0x01);
    ASSERT_EQ(ctx.on_incline_motor_downward_count, 1);
    ASSERT_TRUE(ctx.last_incline_motor_downward);

    /* Same state - no fire */
    lcb_check_status_bits(comm, 0x01);
    ASSERT_EQ(ctx.on_incline_motor_downward_count, 1);

    /* Clear bit 0 */
    lcb_check_status_bits(comm, 0x00);
    ASSERT_EQ(ctx.on_incline_motor_downward_count, 2);
    ASSERT_FALSE(ctx.last_incline_motor_downward);
}

TEST_F(LcbCommRxTest, StatusBit1_InclineUp_FiresOnChange) {
    lcb_check_status_bits(comm, 0x02);
    ASSERT_EQ(ctx.on_incline_motor_upward_count, 1);
    ASSERT_TRUE(ctx.last_incline_motor_upward);

    lcb_check_status_bits(comm, 0x02);
    ASSERT_EQ(ctx.on_incline_motor_upward_count, 1); /* No change */
}

TEST_F(LcbCommRxTest, StatusBit2_MainMotor_FiresOnChange) {
    lcb_check_status_bits(comm, 0x04);
    ASSERT_EQ(ctx.on_main_motor_status_count, 1);
    ASSERT_TRUE(ctx.last_main_motor_status);
}

TEST_F(LcbCommRxTest, StatusBit3_EncoderRpm_FiresOnChange) {
    lcb_check_status_bits(comm, 0x08);
    ASSERT_EQ(ctx.on_encoder_rpm_status_count, 1);
    ASSERT_TRUE(ctx.last_encoder_rpm_status);
}

TEST_F(LcbCommRxTest, StatusBit4_BatteryCap_FiresOnChange) {
    lcb_check_status_bits(comm, 0x10);
    ASSERT_EQ(ctx.on_battery_capacity_status_count, 1);
    ASSERT_TRUE(ctx.last_battery_capacity_status);
}

TEST_F(LcbCommRxTest, StatusBit7_Calibration_FiresOnChange) {
    lcb_check_status_bits(comm, 0x80);
    ASSERT_EQ(ctx.on_initial_calibration_status_count, 1);
    ASSERT_TRUE(ctx.last_initial_calibration_status);
}

TEST_F(LcbCommRxTest, StatusBit6_LcbError_AlwaysFires) {
    /* Bit 6 (0x40) ALWAYS fires callback, not just on change */
    lcb_check_status_bits(comm, 0x40);
    ASSERT_EQ(ctx.on_lcb_error_status_count, 1);
    ASSERT_TRUE(ctx.last_lcb_error_status);

    lcb_check_status_bits(comm, 0x40);
    ASSERT_EQ(ctx.on_lcb_error_status_count, 2); /* Fires again! */

    lcb_check_status_bits(comm, 0x40);
    ASSERT_EQ(ctx.on_lcb_error_status_count, 3); /* And again! */
}

TEST_F(LcbCommRxTest, StatusBit5_CommandError_SetsNotSupportCmd) {
    lcb_check_status_bits(comm, 0x20);
    ASSERT_TRUE(comm->not_support_cmd);
    ASSERT_EQ(ctx.on_command_error_status_count, 1);
    ASSERT_TRUE(ctx.last_command_error_status);
}

TEST_F(LcbCommRxTest, StatusBits_MultipleBitsSet) {
    lcb_check_status_bits(comm, 0xFF);
    /* All 8 bits should fire at least once (first time = change for all) */
    ASSERT_EQ(ctx.on_incline_motor_downward_count, 1);
    ASSERT_EQ(ctx.on_incline_motor_upward_count, 1);
    ASSERT_EQ(ctx.on_main_motor_status_count, 1);
    ASSERT_EQ(ctx.on_encoder_rpm_status_count, 1);
    ASSERT_EQ(ctx.on_battery_capacity_status_count, 1);
    ASSERT_EQ(ctx.on_command_error_status_count, 1);
    ASSERT_EQ(ctx.on_lcb_error_status_count, 1);
    ASSERT_EQ(ctx.on_initial_calibration_status_count, 1);

    /* Call again with same value: only bit 6 should fire again */
    lcb_check_status_bits(comm, 0xFF);
    ASSERT_EQ(ctx.on_incline_motor_downward_count, 1); /* No change */
    ASSERT_EQ(ctx.on_lcb_error_status_count, 2);       /* Always fires */
}

TEST_F(LcbCommRxTest, StatusBits_AllClear) {
    /* First set all bits */
    lcb_check_status_bits(comm, 0xFF);
    ctx.reset();

    /* Clear all bits */
    lcb_check_status_bits(comm, 0x00);
    /* All change-detection bits should fire with false */
    ASSERT_EQ(ctx.on_incline_motor_downward_count, 1);
    ASSERT_FALSE(ctx.last_incline_motor_downward);
    ASSERT_EQ(ctx.on_incline_motor_upward_count, 1);
    ASSERT_FALSE(ctx.last_incline_motor_upward);
    ASSERT_EQ(ctx.on_main_motor_status_count, 1);
    ASSERT_FALSE(ctx.last_main_motor_status);
    /* Bit 6 always fires */
    ASSERT_EQ(ctx.on_lcb_error_status_count, 1);
    ASSERT_FALSE(ctx.last_lcb_error_status);
}

/* ============================================================================
 * Category D: Error Injection Tests
 * ============================================================================ */

TEST_F(LcbCommRxTest, ProcEstopMachineStop_FiresErrorAndCallback) {
    lcb_comm_proc_estop_machine_stop(comm);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.last_error_code, 0x02B2);
    ASSERT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    ASSERT_EQ(ctx.on_estop_machine_stop_count, 1);
}

TEST_F(LcbCommRxTest, ProcEstopMcbError_FiresErrorAndCallback) {
    lcb_comm_proc_estop_mcb_error(comm);
    ASSERT_EQ(ctx.last_error_code, 0x024F);
    ASSERT_EQ(ctx.on_estop_mcb_error_count, 1);
}

TEST_F(LcbCommRxTest, ProcEstopUnlock_FiresErrorAndCallback) {
    lcb_comm_proc_estop_unlock(comm);
    ASSERT_EQ(ctx.last_error_code, 0x02B1);
    ASSERT_EQ(ctx.on_estop_unlock_count, 1);
}

TEST_F(LcbCommRxTest, ProcTmNoRpmError_FiresErrorAndCallback) {
    lcb_comm_proc_tm_no_rpm_error(comm);
    ASSERT_EQ(ctx.last_error_code, 0x00A1);
    ASSERT_EQ(ctx.on_tm_no_rpm_error_count, 1);
}

TEST_F(LcbCommRxTest, ProcSetAdvLoggingStatus_FiresCallback) {
    lcb_comm_proc_set_adv_logging_status(comm);
    ASSERT_EQ(ctx.on_set_adv_logging_status_count, 1);
}

TEST_F(LcbCommRxTest, ProcEstopMachineStop_NoCallbacks_NoCrash) {
    lcb_comm_unregister_callbacks(comm);
    lcb_comm_proc_estop_machine_stop(comm);
    /* Should not crash */
}

/* ============================================================================
 * Category E: OneShot Routing Tests
 * ============================================================================ */

TEST_F(LcbCommRxTest, MotorRpm_NormalRoute) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(2000, data);
    comm->is_one_shot_rpm = false;
    processPacket(LCB_CMD_GET_MOTOR_RPM, 0x00, data, 2);
    ASSERT_EQ(ctx.on_lcb_motor_rpm_received_count, 1);
    ASSERT_EQ(ctx.last_motor_rpm, 2000);
    ASSERT_EQ(ctx.on_one_shot_lcb_motor_rpm_count, 0);
}

TEST_F(LcbCommRxTest, MotorRpm_OneShotRoute) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(2500, data);
    comm->is_one_shot_rpm = true;
    processPacket(LCB_CMD_GET_MOTOR_RPM, 0x00, data, 2);
    ASSERT_EQ(ctx.on_one_shot_lcb_motor_rpm_count, 1);
    ASSERT_EQ(ctx.last_one_shot_motor_rpm, 2500);
    ASSERT_EQ(ctx.on_lcb_motor_rpm_received_count, 0);
}

TEST_F(LcbCommRxTest, MotorRpm_OneShotClearsFlag) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(100, data);
    comm->is_one_shot_rpm = true;
    processPacket(LCB_CMD_GET_MOTOR_RPM, 0x00, data, 2);
    ASSERT_FALSE(comm->is_one_shot_rpm); /* Flag cleared after use */
}

TEST_F(LcbCommRxTest, EcbRpm_NormalRoute) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(500, data);
    comm->is_one_shot_rpm = false;
    processPacket(LCB_CMD_GET_ECB_RPM, 0x00, data, 2);
    ASSERT_EQ(ctx.on_lcb_ecb_rpm_count, 1);
    ASSERT_EQ(ctx.on_one_shot_lcb_ecb_rpm_count, 0);
}

TEST_F(LcbCommRxTest, EcbRpm_OneShotRoute) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(700, data);
    comm->is_one_shot_rpm = true;
    processPacket(LCB_CMD_GET_ECB_RPM, 0x00, data, 2);
    ASSERT_EQ(ctx.on_one_shot_lcb_ecb_rpm_count, 1);
    ASSERT_EQ(ctx.last_one_shot_ecb_rpm, 700);
    ASSERT_EQ(ctx.on_lcb_ecb_rpm_count, 0);
}

TEST_F(LcbCommRxTest, Spm_NormalRoute) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(120, data);
    comm->is_one_shot_spm = false;
    processPacket(LCB_CMD_GET_SPM, 0x00, data, 2);
    ASSERT_EQ(ctx.on_lcb_spm_received_count, 1);
    ASSERT_EQ(ctx.on_one_shot_lcb_spm_count, 0);
}

TEST_F(LcbCommRxTest, Spm_OneShotRoute) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(150, data);
    comm->is_one_shot_spm = true;
    processPacket(LCB_CMD_GET_SPM, 0x00, data, 2);
    ASSERT_EQ(ctx.on_one_shot_lcb_spm_count, 1);
    ASSERT_EQ(ctx.last_one_shot_spm, 150);
    ASSERT_EQ(ctx.on_lcb_spm_received_count, 0);
}

TEST_F(LcbCommRxTest, SensorData_NormalRoute) {
    uint8_t data[19];
    memset(data, 0x11, 19);
    comm->is_one_shot_sensor_data = false;
    processPacket(LCB_CMD_GET_INDOOR_BIKE_SENSOR_DATA, 0x00, data, 19);
    ASSERT_EQ(ctx.on_lcb_indoor_bike_sensor_data_count, 1);
    ASSERT_EQ(ctx.on_one_shot_lcb_indoor_bike_sensor_data_count, 0);
}

TEST_F(LcbCommRxTest, SensorData_OneShotRoute) {
    uint8_t data[19];
    memset(data, 0x22, 19);
    comm->is_one_shot_sensor_data = true;
    processPacket(LCB_CMD_GET_INDOOR_BIKE_SENSOR_DATA, 0x00, data, 19);
    ASSERT_EQ(ctx.on_one_shot_lcb_indoor_bike_sensor_data_count, 1);
    ASSERT_EQ(ctx.on_lcb_indoor_bike_sensor_data_count, 0);
}

/* ============================================================================
 * Category F: RX Abnormal Condition Tests
 * ============================================================================ */

TEST_F(LcbCommRxTest, ProcessPacket_CommandErrorBit_SkipsProcessing) {
    /* Status byte 0x20 = bit 5 (command error) set.
     * First time: changes from false to true -> sets not_support_cmd -> skips processing */
    uint8_t data[2]; lcb_uint16_to_bytes_be(0x0102, data);
    processPacket(LCB_CMD_GET_VERSION, 0x20, data, 2);
    ASSERT_EQ(ctx.on_lcb_version_count, 0); /* Skipped due to not_support_cmd */
    ASSERT_EQ(ctx.on_command_error_status_count, 1);
}

TEST_F(LcbCommRxTest, ProcessPacket_CommandErrorBitAlreadySet_NotSkipped) {
    /* First packet: sets command_error_status = true, not_support_cmd = true */
    uint8_t data[2]; lcb_uint16_to_bytes_be(0x0102, data);
    processPacket(LCB_CMD_GET_VERSION, 0x20, data, 2);
    ASSERT_EQ(ctx.on_lcb_version_count, 0);

    /* Second packet: command_error_status already true, no change,
     * not_support_cmd resets to false at start -> packet IS processed */
    processPacket(LCB_CMD_GET_VERSION, 0x20, data, 2);
    ASSERT_EQ(ctx.on_lcb_version_count, 1);
}

TEST_F(LcbCommRxTest, ProcessPacket_UnknownCommand) {
    processPacket((lcb_command_t)0x33, 0x00, NULL, 0);
    ASSERT_EQ(ctx.on_unknown_command_count, 1);
    ASSERT_EQ(ctx.last_unknown_command, 0x33);
}

TEST_F(LcbCommRxTest, ProcessPacket_NoCallbacks_NoCrash) {
    lcb_comm_unregister_callbacks(comm);
    uint8_t data[2]; lcb_uint16_to_bytes_be(0x0102, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_GET_VERSION, 0x00, data, 2);
    lcb_process_received_packet(comm, &pkt);
    lcb_packet_free(&pkt);
    /* Should not crash */
}

TEST_F(LcbCommRxTest, ProcessPacket_NullPacket_NoCrash) {
    lcb_process_received_packet(comm, NULL);
    /* Should not crash */
}

TEST_F(LcbCommRxTest, ProcessPacket_EmptyDataForVersionCmd) {
    processPacket(LCB_CMD_GET_VERSION, 0x00, NULL, 0);
    ASSERT_EQ(ctx.on_lcb_version_count, 0); /* Empty data, proc function rejects */
}

TEST_F(LcbCommRxTest, ProcessPacket_MaxDataLen255_Accepted) {
    /* 255-byte data should pass the LCB_RECEIVE_DATA_LENGTH_LIMITED_MAX check */
    uint8_t data[255];
    memset(data, 0xAA, 255);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_INITIALIZE, 0x00, data, 255);
    lcb_process_received_packet(comm, &pkt);
    /* SET_INITIALIZE is a simple ACK, so callback fires regardless of data */
    ASSERT_EQ(ctx.on_lcb_initial_count, 1);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommRxTest, ProcessPacket_EveryCommandGroup_SmokeTest) {
    /* Test one command from each group to verify dispatch works */

    /* 0x70 group: SET_INITIALIZE */
    processPacket(LCB_CMD_SET_INITIALIZE, 0x00, NULL, 0);
    ASSERT_EQ(ctx.on_lcb_initial_count, 1);

    /* 0xF0 group: SET_MOTOR_RPM */
    processPacket(LCB_CMD_SET_MOTOR_RPM, 0x00, NULL, 0);
    ASSERT_EQ(ctx.on_lcb_motor_rpm_count, 1);

    /* 0x90 group: SET_MOTOR_SPECIFICATION */
    processPacket(LCB_CMD_SET_MOTOR_SPECIFICATION, 0x00, NULL, 0);
    ASSERT_EQ(ctx.on_lcb_motor_specification_count, 1);

    /* 0x50 group: SET_DEVICE_DATA */
    processPacket(LCB_CMD_SET_DEVICE_DATA, 0x00, NULL, 0);
    /* device_data requires data, but with no data, proc function may not fire */

    /* 0x60 group: SET_RESET_ECB */
    processPacket(LCB_CMD_SET_RESET_ECB, 0x00, NULL, 0);
    ASSERT_EQ(ctx.on_lcb_reset_ecb_count, 1);

    /* 0x80 group: SET_INCLINE_PERCENT */
    processPacket(LCB_CMD_SET_INCLINE_PERCENT, 0x00, NULL, 0);
    ASSERT_EQ(ctx.on_lcb_incline_percent_count, 1);

    /* 0x40 group: SET_RECORD_CURRENT */
    processPacket(LCB_CMD_SET_RECORD_CURRENT, 0x00, NULL, 0);
    ASSERT_EQ(ctx.on_lcb_record_current_count, 1);
}

/* ============================================================================
 * Category G: Error/TX Proc Functions (via lcb_proc_comm.h)
 * ============================================================================ */

TEST_F(LcbProcCommTest, ProcCommLcbError_FiresErrorCode0x04B0) {
    lcb_proc_comm_lcb_error(&cb);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.last_error_code, LCB_ERROR_COMM_LCB);
    ASSERT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    ASSERT_EQ(ctx.last_ui_error_code, LCB_ERROR_COMM_LCB);
}

TEST_F(LcbProcCommTest, ProcEStopError_FiresErrorCode0x02B2) {
    lcb_proc_e_stop_error(&cb);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.last_error_code, LCB_ERROR_E_STOP);
    ASSERT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    ASSERT_EQ(ctx.last_ui_error_code, LCB_ERROR_E_STOP);
}

TEST_F(LcbProcCommTest, ProcEStopMcbError_FiresErrorCode0x024F) {
    lcb_proc_e_stop_mcb_error(&cb);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.last_error_code, LCB_ERROR_E_STOP_MCB);
    ASSERT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    ASSERT_EQ(ctx.last_ui_error_code, LCB_ERROR_E_STOP_MCB);
}

TEST_F(LcbProcCommTest, ProcEStopUnlock_FiresErrorCode0x02B1) {
    lcb_proc_e_stop_unlock(&cb);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.last_error_code, LCB_ERROR_E_STOP_UNLOCK);
    ASSERT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    ASSERT_EQ(ctx.last_ui_error_code, LCB_ERROR_E_STOP_UNLOCK);
}

TEST_F(LcbProcCommTest, ProcTmNoRpmError_FiresErrorCode0x00A1) {
    lcb_proc_tm_no_rpm_error(&cb);
    ASSERT_EQ(ctx.on_lcb_error_code_count, 1);
    ASSERT_EQ(ctx.last_error_code, LCB_ERROR_TM_NO_RPM);
    ASSERT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    ASSERT_EQ(ctx.last_ui_error_code, LCB_ERROR_TM_NO_RPM);
}

TEST_F(LcbProcCommTest, ProcSetAdvLoggingStatus_FiresCallback) {
    lcb_proc_set_adv_logging_status(&cb);
    ASSERT_EQ(ctx.on_set_adv_logging_status_count, 1);
}

TEST_F(LcbProcCommTest, ProcSetTreadmillRpmValue_ExtractsRpm) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(3000, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_MOTOR_RPM, 0x00, data, 2);
    lcb_proc_set_treadmill_rpm_value(&cb, &pkt);
    ASSERT_EQ(ctx.on_treadmill_rpm_value_set_count, 1);
    ASSERT_EQ(ctx.last_treadmill_rpm_set, 3000);
    lcb_packet_free(&pkt);
}

TEST_F(LcbProcCommTest, ProcSetTreadmillAdcValue_ExtractsAdc) {
    uint8_t data[2]; lcb_uint16_to_bytes_be(4096, data);
    lcb_packet_t pkt = make_rx_packet(LCB_CMD_SET_INCLINE_LOC, 0x00, data, 2);
    lcb_proc_set_treadmill_adc_value(&cb, &pkt);
    ASSERT_EQ(ctx.on_treadmill_adc_value_set_count, 1);
    ASSERT_EQ(ctx.last_treadmill_adc_set, 4096);
    lcb_packet_free(&pkt);
}

/* ============================================================================
 * Category H: ReceiveDataBuffer Tests
 * ============================================================================ */

TEST(ReceiveDataBufferTest, InitSetsFieldsToZero) {
    lcb_receive_data_buffer_t buf;
    lcb_receive_data_buffer_init(&buf);
    ASSERT_EQ(buf.data, nullptr);
    ASSERT_EQ(buf.length, 0u);
}

TEST(ReceiveDataBufferTest, InitNull_NoCrash) {
    lcb_receive_data_buffer_init(NULL);
}

TEST(ReceiveDataBufferTest, FreeNull_NoCrash) {
    lcb_receive_data_buffer_free(NULL);
}

TEST(ReceiveDataBufferTest, CloneCreatesDeepCopy) {
    lcb_receive_data_buffer_t buf;
    lcb_receive_data_buffer_init(&buf);

    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    ASSERT_EQ(lcb_receive_data_buffer_clone(&buf, data, 5), 0);
    ASSERT_NE(buf.data, nullptr);
    ASSERT_EQ(buf.length, 5u);
    ASSERT_EQ(buf.data[0], 0x01);
    ASSERT_EQ(buf.data[4], 0x05);

    /* Verify it's a deep copy (different pointer) */
    ASSERT_NE(buf.data, data);

    lcb_receive_data_buffer_free(&buf);
}

TEST(ReceiveDataBufferTest, CloneNullDest_ReturnsError) {
    uint8_t data[] = {0x01};
    ASSERT_EQ(lcb_receive_data_buffer_clone(NULL, data, 1), -1);
}

TEST(ReceiveDataBufferTest, CloneNullData_ReturnsError) {
    lcb_receive_data_buffer_t buf;
    lcb_receive_data_buffer_init(&buf);
    ASSERT_EQ(lcb_receive_data_buffer_clone(&buf, NULL, 5), -1);
}

TEST(ReceiveDataBufferTest, CloneZeroLength_ReturnsError) {
    lcb_receive_data_buffer_t buf;
    lcb_receive_data_buffer_init(&buf);
    uint8_t data[] = {0x01};
    ASSERT_EQ(lcb_receive_data_buffer_clone(&buf, data, 0), -1);
}

TEST(ReceiveDataBufferTest, GetDataReturnsPointer) {
    lcb_receive_data_buffer_t buf;
    lcb_receive_data_buffer_init(&buf);
    uint8_t data[] = {0xAB, 0xCD};
    lcb_receive_data_buffer_clone(&buf, data, 2);

    const uint8_t *ptr = lcb_receive_data_buffer_get_data(&buf);
    ASSERT_NE(ptr, nullptr);
    ASSERT_EQ(ptr[0], 0xAB);
    ASSERT_EQ(ptr[1], 0xCD);

    lcb_receive_data_buffer_free(&buf);
}

TEST(ReceiveDataBufferTest, GetDataNull_ReturnsNull) {
    ASSERT_EQ(lcb_receive_data_buffer_get_data(NULL), nullptr);
}

TEST(ReceiveDataBufferTest, GetLengthNull_ReturnsZero) {
    ASSERT_EQ(lcb_receive_data_buffer_get_length(NULL), 0u);
}

TEST(ReceiveDataBufferTest, FreeReleasesMemory) {
    lcb_receive_data_buffer_t buf;
    lcb_receive_data_buffer_init(&buf);
    uint8_t data[] = {0x01, 0x02};
    lcb_receive_data_buffer_clone(&buf, data, 2);
    lcb_receive_data_buffer_free(&buf);

    ASSERT_EQ(buf.data, nullptr);
    ASSERT_EQ(buf.length, 0u);
}

TEST(ReceiveDataBufferTest, CloneOverwritesPreviousData) {
    lcb_receive_data_buffer_t buf;
    lcb_receive_data_buffer_init(&buf);

    uint8_t data1[] = {0x01, 0x02};
    lcb_receive_data_buffer_clone(&buf, data1, 2);
    ASSERT_EQ(buf.length, 2u);

    uint8_t data2[] = {0x0A, 0x0B, 0x0C};
    lcb_receive_data_buffer_clone(&buf, data2, 3);
    ASSERT_EQ(buf.length, 3u);
    ASSERT_EQ(buf.data[0], 0x0A);
    ASSERT_EQ(buf.data[2], 0x0C);

    lcb_receive_data_buffer_free(&buf);
}

/* ============================================================================
 * Category I: ReceiveDataQueue Tests
 * ============================================================================ */

TEST(ReceiveDataQueueTest, InitiallyEmpty) {
    lcb_receive_data_queue_t queue;
    ASSERT_EQ(lcb_receive_data_queue_init(&queue), 0);
    ASSERT_TRUE(lcb_receive_data_queue_is_empty(&queue));
    ASSERT_EQ(lcb_receive_data_queue_size(&queue), 0u);
    lcb_receive_data_queue_free(&queue);
}

TEST(ReceiveDataQueueTest, InitNull_ReturnsError) {
    ASSERT_LT(lcb_receive_data_queue_init(NULL), 0);
}

TEST(ReceiveDataQueueTest, AddAndRemove) {
    lcb_receive_data_queue_t queue;
    lcb_receive_data_queue_init(&queue);

    uint8_t data[] = {0x10, 0x20, 0x30};
    ASSERT_EQ(lcb_receive_data_queue_add(&queue, data, 3), 0);
    ASSERT_EQ(lcb_receive_data_queue_size(&queue), 1u);

    lcb_receive_data_buffer_t buf;
    ASSERT_EQ(lcb_receive_data_queue_remove(&queue, &buf), 0);
    ASSERT_EQ(buf.length, 3u);
    ASSERT_EQ(buf.data[0], 0x10);
    ASSERT_EQ(buf.data[1], 0x20);
    ASSERT_EQ(buf.data[2], 0x30);
    ASSERT_TRUE(lcb_receive_data_queue_is_empty(&queue));

    lcb_receive_data_buffer_free(&buf);
    lcb_receive_data_queue_free(&queue);
}

TEST(ReceiveDataQueueTest, FIFOOrder) {
    lcb_receive_data_queue_t queue;
    lcb_receive_data_queue_init(&queue);

    uint8_t d1[] = {0x01};
    uint8_t d2[] = {0x02};
    uint8_t d3[] = {0x03};
    lcb_receive_data_queue_add(&queue, d1, 1);
    lcb_receive_data_queue_add(&queue, d2, 1);
    lcb_receive_data_queue_add(&queue, d3, 1);
    ASSERT_EQ(lcb_receive_data_queue_size(&queue), 3u);

    lcb_receive_data_buffer_t buf;

    lcb_receive_data_queue_remove(&queue, &buf);
    ASSERT_EQ(buf.data[0], 0x01);
    lcb_receive_data_buffer_free(&buf);

    lcb_receive_data_queue_remove(&queue, &buf);
    ASSERT_EQ(buf.data[0], 0x02);
    lcb_receive_data_buffer_free(&buf);

    lcb_receive_data_queue_remove(&queue, &buf);
    ASSERT_EQ(buf.data[0], 0x03);
    lcb_receive_data_buffer_free(&buf);

    ASSERT_TRUE(lcb_receive_data_queue_is_empty(&queue));
    lcb_receive_data_queue_free(&queue);
}

TEST(ReceiveDataQueueTest, RemoveFromEmpty_ReturnsError) {
    lcb_receive_data_queue_t queue;
    lcb_receive_data_queue_init(&queue);

    lcb_receive_data_buffer_t buf;
    ASSERT_EQ(lcb_receive_data_queue_remove(&queue, &buf), -1);

    lcb_receive_data_queue_free(&queue);
}

TEST(ReceiveDataQueueTest, ClearEmptiesQueue) {
    lcb_receive_data_queue_t queue;
    lcb_receive_data_queue_init(&queue);

    uint8_t d[] = {0xAA};
    lcb_receive_data_queue_add(&queue, d, 1);
    lcb_receive_data_queue_add(&queue, d, 1);
    ASSERT_EQ(lcb_receive_data_queue_size(&queue), 2u);

    lcb_receive_data_queue_clear(&queue);
    ASSERT_EQ(lcb_receive_data_queue_size(&queue), 0u);
    ASSERT_TRUE(lcb_receive_data_queue_is_empty(&queue));

    lcb_receive_data_queue_free(&queue);
}

TEST(ReceiveDataQueueTest, AddNullData_ReturnsError) {
    lcb_receive_data_queue_t queue;
    lcb_receive_data_queue_init(&queue);
    ASSERT_LT(lcb_receive_data_queue_add(&queue, NULL, 5), 0);
    lcb_receive_data_queue_free(&queue);
}

TEST(ReceiveDataQueueTest, SizeNull_ReturnsZero) {
    ASSERT_EQ(lcb_receive_data_queue_size(NULL), 0u);
}

TEST(ReceiveDataQueueTest, IsEmptyNull_ReturnsTrue) {
    ASSERT_TRUE(lcb_receive_data_queue_is_empty(NULL));
}

/* ============================================================================
 * Category J: ReceiveDataHandler / Parser Integration Tests
 * ============================================================================ */

/* Helper: build a raw RX wire-format packet (same as test_lcb_parser.cpp) */
static size_t build_raw_rx_packet(uint8_t *buf, size_t buf_size,
                                   uint8_t status, uint8_t command,
                                   const uint8_t *data, uint8_t data_len) {
    size_t total = 4 + data_len + 1;  /* start + status + cmd + len + data + crc */
    if (total > buf_size) return 0;

    buf[0] = LCB_PACKET_START_BYTE_RX;
    buf[1] = status;
    buf[2] = command;
    buf[3] = data_len;
    for (uint8_t i = 0; i < data_len; i++) {
        buf[4 + i] = data[i];
    }
    buf[4 + data_len] = lcb_crc8_calculate(buf, 4 + data_len);
    return total;
}

TEST_F(LcbCommRxTest, RxDataQueue_InitialEmpty) {
    /* After lcb_comm_create(), rx_data_queue should be empty */
    ASSERT_TRUE(lcb_receive_data_queue_is_empty(&comm->rx_data_queue));
    ASSERT_EQ(lcb_receive_data_queue_size(&comm->rx_data_queue), 0u);
}

TEST_F(LcbCommRxTest, RxDataQueue_AddAndRemoveRawData) {
    uint8_t data[] = {0x01, 0x00, 0x71, 0x00, 0xAA};
    ASSERT_EQ(lcb_receive_data_queue_add(&comm->rx_data_queue, data, 5), 0);
    ASSERT_EQ(lcb_receive_data_queue_size(&comm->rx_data_queue), 1u);

    lcb_receive_data_buffer_t buf;
    ASSERT_EQ(lcb_receive_data_queue_remove(&comm->rx_data_queue, &buf), 0);
    ASSERT_EQ(buf.length, 5u);
    ASSERT_EQ(buf.data[0], 0x01);
    lcb_receive_data_buffer_free(&buf);
}

TEST_F(LcbCommRxTest, ReceiveDataHandler_ParsesCompletePacket) {
    /* Build a valid wire-format packet: SET_INITIALIZE (0x70), no data */
    uint8_t raw[16];
    size_t len = build_raw_rx_packet(raw, sizeof(raw), 0x00, 0x70, NULL, 0);
    ASSERT_GT(len, 0u);

    /* Feed through receive data handler (simulates the RX thread path) */
    lcb_receive_data_handler(comm, raw, (uint32_t)len);

    /* A complete packet should now be in the rx_queue */
    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 1u);

    lcb_packet_t pkt;
    ASSERT_EQ(lcb_packet_queue_remove(&comm->rx_queue, &pkt), 0);
    ASSERT_EQ(pkt.command, (lcb_command_t)0x70);
    ASSERT_EQ(pkt.rx_lcb_status, 0x00);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommRxTest, ReceiveDataHandler_ParsesPacketWithData) {
    /* Build a packet: GET_ERROR_CODE (0x72) with 2 bytes of data {0xAB, 0xCD} */
    uint8_t payload[] = {0xAB, 0xCD};
    uint8_t raw[16];
    size_t len = build_raw_rx_packet(raw, sizeof(raw), 0x00, 0x72, payload, 2);
    ASSERT_GT(len, 0u);

    lcb_receive_data_handler(comm, raw, (uint32_t)len);

    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 1u);
    lcb_packet_t pkt;
    lcb_packet_queue_remove(&comm->rx_queue, &pkt);
    ASSERT_EQ(pkt.command, (lcb_command_t)0x72);
    ASSERT_EQ(pkt.data_length, 2);
    ASSERT_EQ(pkt.data[0], 0xAB);
    ASSERT_EQ(pkt.data[1], 0xCD);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommRxTest, ReceiveDataHandler_PartialThenComplete) {
    /* Build a valid packet and split it into two partial reads */
    uint8_t raw[16];
    size_t len = build_raw_rx_packet(raw, sizeof(raw), 0x00, 0x70, NULL, 0);
    ASSERT_GT(len, 2u);  /* Must be > 2 to split */

    /* First partial: just the first 2 bytes */
    lcb_receive_data_handler(comm, raw, 2);
    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 0u);  /* Not yet complete */

    /* Second partial: remaining bytes */
    lcb_receive_data_handler(comm, raw + 2, (uint32_t)(len - 2));
    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 1u);  /* Now complete */

    lcb_packet_t pkt;
    lcb_packet_queue_remove(&comm->rx_queue, &pkt);
    ASSERT_EQ(pkt.command, (lcb_command_t)0x70);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommRxTest, ReceiveDataHandler_TwoPacketsInOneBuffer) {
    /* Build two complete packets back-to-back in one buffer */
    uint8_t raw[32];
    size_t len1 = build_raw_rx_packet(raw, sizeof(raw), 0x00, 0x70, NULL, 0);
    size_t len2 = build_raw_rx_packet(raw + len1, sizeof(raw) - len1, 0x00, 0x71, NULL, 0);

    /* Feed both at once */
    lcb_receive_data_handler(comm, raw, (uint32_t)(len1 + len2));

    /* Both packets should be in rx_queue */
    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 2u);

    lcb_packet_t pkt1, pkt2;
    lcb_packet_queue_remove(&comm->rx_queue, &pkt1);
    lcb_packet_queue_remove(&comm->rx_queue, &pkt2);
    /* Both have same priority so order is preserved */
    ASSERT_EQ(pkt1.command, (lcb_command_t)0x70);
    ASSERT_EQ(pkt2.command, (lcb_command_t)0x71);
    lcb_packet_free(&pkt1);
    lcb_packet_free(&pkt2);
}

TEST_F(LcbCommRxTest, ProcessReceiveDataBuffer_EndToEnd) {
    /* Build a valid packet, wrap in buffer, process through full path */
    uint8_t raw[16];
    size_t len = build_raw_rx_packet(raw, sizeof(raw), 0x00, 0x73, NULL, 0);

    lcb_receive_data_buffer_t buf;
    lcb_receive_data_buffer_init(&buf);
    lcb_receive_data_buffer_clone(&buf, raw, (uint32_t)len);

    lcb_process_receive_data_buffer(comm, &buf);
    lcb_receive_data_buffer_free(&buf);

    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 1u);
    lcb_packet_t pkt;
    lcb_packet_queue_remove(&comm->rx_queue, &pkt);
    ASSERT_EQ(pkt.command, (lcb_command_t)0x73);
    lcb_packet_free(&pkt);
}

TEST_F(LcbCommRxTest, ReceiveDataHandler_InvalidCRC_NoPacketQueued) {
    /* Build a packet but corrupt the CRC */
    uint8_t raw[16];
    size_t len = build_raw_rx_packet(raw, sizeof(raw), 0x00, 0x70, NULL, 0);
    raw[len - 1] ^= 0xFF;  /* Flip CRC bits */

    lcb_receive_data_handler(comm, raw, (uint32_t)len);

    /* No packet should be queued due to CRC error */
    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 0u);
}

TEST_F(LcbCommRxTest, ReceiveDataHandler_InvalidStartByte_Recovers) {
    /* Garbage byte followed by a valid packet */
    uint8_t raw[32];
    raw[0] = 0xFF;  /* Invalid start byte */

    size_t len = build_raw_rx_packet(raw + 1, sizeof(raw) - 1, 0x00, 0x70, NULL, 0);

    lcb_receive_data_handler(comm, raw, (uint32_t)(1 + len));

    /* Parser should recover and parse the valid packet */
    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 1u);

    lcb_packet_t pkt;
    lcb_packet_queue_remove(&comm->rx_queue, &pkt);
    ASSERT_EQ(pkt.command, (lcb_command_t)0x70);
    lcb_packet_free(&pkt);
}

/* ============================================================================
 * Category K: Memory Management Tests
 * ============================================================================ */

TEST_F(LcbCommRxTest, CreateDestroyCycle) {
    /* Repeated create/destroy cycles — all resources properly released */
    for (int i = 0; i < 10; i++) {
        lcb_comm_t *c = lcb_comm_create();
        ASSERT_NE(c, nullptr);
        lcb_comm_destroy(c);
    }
}

TEST_F(LcbCommRxTest, DestroyFreesAllQueues) {
    /* Create a separate comm instance, populate its queues, then destroy */
    lcb_comm_t *c = lcb_comm_create();
    ASSERT_NE(c, nullptr);

    /* Add packets to tx_queue */
    lcb_packet_t tx_pkt;
    lcb_packet_init(&tx_pkt);
    lcb_packet_set_command(&tx_pkt, (lcb_command_t)0x70);
    uint8_t tx_data[] = {0x01, 0x02};
    lcb_packet_set_data(&tx_pkt, tx_data, 2);
    lcb_packet_set_priority(&tx_pkt, 5);
    lcb_packet_queue_add(&c->tx_queue, &tx_pkt);
    lcb_packet_queue_add(&c->tx_queue, &tx_pkt);
    lcb_packet_free(&tx_pkt);

    /* Add a packet to rx_queue */
    lcb_packet_t rx_pkt;
    lcb_packet_init(&rx_pkt);
    lcb_packet_set_command(&rx_pkt, (lcb_command_t)0x71);
    lcb_packet_set_priority(&rx_pkt, 5);
    lcb_packet_queue_add(&c->rx_queue, &rx_pkt);
    lcb_packet_free(&rx_pkt);

    /* Add raw data to rx_data_queue */
    uint8_t raw[] = {0xAA, 0xBB, 0xCC};
    lcb_receive_data_queue_add(&c->rx_data_queue, raw, 3);

    ASSERT_EQ(lcb_packet_queue_size(&c->tx_queue), 2u);
    ASSERT_EQ(lcb_packet_queue_size(&c->rx_queue), 1u);
    ASSERT_EQ(lcb_receive_data_queue_size(&c->rx_data_queue), 1u);

    /* Destroy — all queues freed internally, no leaks */
    lcb_comm_destroy(c);
}

TEST_F(LcbCommRxTest, RxDataQueue_RepeatedAddRemoveCycles) {
    /* Stress test: repeated add/remove on comm's embedded rx_data_queue */
    for (int i = 0; i < 50; i++) {
        uint8_t data[5] = {(uint8_t)i, (uint8_t)(i+1), (uint8_t)(i+2),
                           (uint8_t)(i+3), (uint8_t)(i+4)};
        ASSERT_EQ(lcb_receive_data_queue_add(&comm->rx_data_queue, data, 5), 0);
        ASSERT_EQ(lcb_receive_data_queue_size(&comm->rx_data_queue), 1u);

        lcb_receive_data_buffer_t buf;
        ASSERT_EQ(lcb_receive_data_queue_remove(&comm->rx_data_queue, &buf), 0);
        ASSERT_EQ(buf.length, 5u);
        ASSERT_EQ(buf.data[0], (uint8_t)i);
        lcb_receive_data_buffer_free(&buf);
    }
    ASSERT_TRUE(lcb_receive_data_queue_is_empty(&comm->rx_data_queue));
}

TEST_F(LcbCommRxTest, ReceiveDataHandler_RepeatedParsing) {
    /* Feed 20 valid packets through receive_data_handler, remove all */
    for (int i = 0; i < 20; i++) {
        uint8_t payload[] = {(uint8_t)i};
        uint8_t raw[16];
        size_t len = build_raw_rx_packet(raw, sizeof(raw),
                                          0x00, (uint8_t)(0x70 + (i % 4)),
                                          payload, 1);
        ASSERT_GT(len, 0u);
        lcb_receive_data_handler(comm, raw, (uint32_t)len);
    }

    /* All 20 packets should be in rx_queue */
    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 20u);

    /* Remove and free all */
    for (int i = 0; i < 20; i++) {
        lcb_packet_t pkt;
        ASSERT_EQ(lcb_packet_queue_remove(&comm->rx_queue, &pkt), 0);
        ASSERT_EQ(pkt.data_length, 1);
        lcb_packet_free(&pkt);
    }
    ASSERT_TRUE(lcb_packet_queue_is_empty(&comm->rx_queue));
}

TEST_F(LcbCommRxTest, OnRxPacketParsed_ClonesAndFrees) {
    /* Feed a packet with data through receive_data_handler */
    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    uint8_t raw[16];
    size_t len = build_raw_rx_packet(raw, sizeof(raw), 0x00, 0x72, payload, 4);
    ASSERT_GT(len, 0u);

    lcb_receive_data_handler(comm, raw, (uint32_t)len);

    /* rx_queue should have the cloned packet */
    ASSERT_EQ(lcb_packet_queue_size(&comm->rx_queue), 1u);

    lcb_packet_t pkt;
    ASSERT_EQ(lcb_packet_queue_remove(&comm->rx_queue, &pkt), 0);
    ASSERT_EQ(pkt.command, (lcb_command_t)0x72);
    ASSERT_EQ(pkt.data_length, 4);
    ASSERT_EQ(pkt.data[0], 0xDE);
    ASSERT_EQ(pkt.data[1], 0xAD);
    ASSERT_EQ(pkt.data[2], 0xBE);
    ASSERT_EQ(pkt.data[3], 0xEF);

    /* Free the removed packet — the clone in lcb_on_rx_packet_parsed was already freed */
    lcb_packet_free(&pkt);
    ASSERT_TRUE(lcb_packet_queue_is_empty(&comm->rx_queue));
}
