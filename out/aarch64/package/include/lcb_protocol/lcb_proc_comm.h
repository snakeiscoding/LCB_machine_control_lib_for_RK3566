/**
 * @file lcb_proc_comm.h
 * @brief LCB Communication Response Processing Module
 *
 * Converted from: com/jht/lcbcommlib/LcbProcComm.java
 *
 * This module handles processing of received LCB packets,
 * extracting data and invoking the appropriate callbacks.
 */

#ifndef LCB_PROC_COMM_H
#define LCB_PROC_COMM_H

#include "lcb_packet.h"
#include "lcb_comm_callback.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Error code constants */
#define LCB_ERROR_COMM_LCB          0x04B0  /* Communication timeout error */
#define LCB_ERROR_E_STOP            0x02B2  /* E-Stop error */
#define LCB_ERROR_E_STOP_MCB        0x024F  /* E-Stop MCB error */
#define LCB_ERROR_E_STOP_UNLOCK     0x02B1  /* E-Stop unlock */
#define LCB_ERROR_TM_NO_RPM         0x00A1  /* Treadmill no RPM response */

/* ============================================================================
 * Command 0x70-0x7E Response Handlers
 * ============================================================================ */

void lcb_proc_set_initial(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_error_code(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_version(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_calibrate(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_update_program(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_skip_curr_error(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_spe_ext_cmd(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_dci_version(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_dci_env(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_cmd_console_power(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_protocol_version(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_cmd_serial_number(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);

/* ============================================================================
 * IR Address Board (0x26) Response Handlers
 * ============================================================================ */

void lcb_proc_get_ir_addr_version(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_ir_addr_ir_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_treadmill_operate(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);

/* ============================================================================
 * Command 0xF0-0xFE Response Handlers
 * ============================================================================ */

void lcb_proc_set_motor_speed_rtn_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_motor_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_inc_pwm_step(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_dec_pwm_step(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_stop_pwm_step(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_incline_action(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_incline_loc(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_operating_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_roller_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_motor_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_one_shot_motor_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_incline_loc(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_tune_end_incline(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_tune_end_incline2(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_incline_stroke(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_compensation_vol(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);

/* ============================================================================
 * Command 0x90-0xAF Response Handlers
 * ============================================================================ */

void lcb_proc_set_motor_specification(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_motor_type(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_dci_data_max_len(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_motor_power_off(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_dci_force_incline(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_treadmill_in_used(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_main_motor_info(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_treadmill_ext_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_treadmill_cadence(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_treadmill_ecb_action(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_dci_incline_range(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_dci_incline_range(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_dci_rpm_speed_range(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_dci_rpm_speed_range(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_universal(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_dci_manual_calibration(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);

/* ============================================================================
 * Command 0x50-0x5F Response Handlers
 * ============================================================================ */

void lcb_proc_set_device_data(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_indoor_bike_sensor_data(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_one_shot_indoor_bike_sensor_data(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_indoor_bike_stepper_motor_pos(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_indoor_bike_calc_para(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_indoor_bike_calc_para(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_indoor_bike_action_spin_down(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_indoor_bike_info_spin_down(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_indoor_bike_erg_mode_settings(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_indoor_bike_erg_mode_settings(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_indoor_bike_resistance_level(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);

/* ============================================================================
 * Command 0x60-0x6F Response Handlers
 * ============================================================================ */

void lcb_proc_set_reset_ecb(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_ecb_position(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_ecb_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_one_shot_ecb_rpm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_ecb_adc(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_pwm_percent(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_battery_voltage(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_watts_cmd(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_rpm_gear_ratio(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_generator_pole_pair(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_limit_rpm_charge(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_limit_rpm_resistance(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_machine_type(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_power_off(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_battery_charge_level(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_min_resistance_incline_working(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);

/* ============================================================================
 * Command 0x80-0x8F Response Handlers
 * ============================================================================ */

void lcb_proc_set_incline_percent(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_incline_percent(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_ecb_action(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_ecb_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_ecb_count(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_electromagnetic_current(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_resistance_type(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_begin_battery_charge(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_battery_capacity(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_climbmill_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_climbmill_status(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_e_stop_parameter(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_brake_pressure_value(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_watts(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_climbmill_cal(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);

/* ============================================================================
 * Command 0x40-0x4F Response Handlers
 * ============================================================================ */

void lcb_proc_set_record_current(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_pwm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_finish_calibrating(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_spm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_spm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_one_shot_spm(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_cal_scaling(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_cal_scaling(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_frame_cal_scaling(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_get_frame_cal_scaling(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);

/* ============================================================================
 * Error Process Handlers
 * ============================================================================ */

void lcb_proc_comm_lcb_error(const lcb_comm_callbacks_t *cb);
void lcb_proc_e_stop_error(const lcb_comm_callbacks_t *cb);
void lcb_proc_e_stop_mcb_error(const lcb_comm_callbacks_t *cb);
void lcb_proc_e_stop_unlock(const lcb_comm_callbacks_t *cb);
void lcb_proc_tm_no_rpm_error(const lcb_comm_callbacks_t *cb);

/* ============================================================================
 * TX Callback Handlers
 * ============================================================================ */

void lcb_proc_set_treadmill_rpm_value(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_treadmill_adc_value(const lcb_comm_callbacks_t *cb, const lcb_packet_t *pkt);
void lcb_proc_set_adv_logging_status(const lcb_comm_callbacks_t *cb);

#ifdef __cplusplus
}
#endif

#endif /* LCB_PROC_COMM_H */
