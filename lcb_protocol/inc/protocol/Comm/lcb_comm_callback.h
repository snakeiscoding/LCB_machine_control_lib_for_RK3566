/**
 * @file lcb_comm_callback.h
 * @brief LCB Communication Callback Interface
 *
 * Converted from: com/jht/lcbcommlib/ILcbCommReceive.java
 *
 * This file defines callback functions for LCB communication events.
 * The application should implement these callbacks to handle received data.
 */

#ifndef LCB_COMM_CALLBACK_H
#define LCB_COMM_CALLBACK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for machine-specific data structures */
typedef struct indoor_bike_sensor_data indoor_bike_sensor_data_t;
typedef struct indoor_bike_info_spin_down indoor_bike_info_spin_down_t;

/* Forward declaration for device data sub-command callbacks */
struct lcb_device_data_callbacks;

/* Forward declaration for universal data sub-command callbacks */
struct lcb_universal_data_callbacks;

/**
 * @brief LCB Communication Callback Structure
 *
 * All callback functions are optional. Set to NULL if not needed.
 * The user_data pointer is passed to all callbacks.
 */
typedef struct {
    void *user_data;  /* User-defined context pointer */

    /* Boot up and ERP mode callbacks */
    void (*on_boot_up_leave_lcb_erp)(void *user_data, bool is_success);
    void (*on_enter_lcb_erp_mode)(void *user_data, bool is_success);
    void (*on_leave_lcb_erp_mode)(void *user_data, bool is_lcb_comm_ok, bool is_success);

    /* Status change callbacks */
    void (*on_incline_motor_downward_status_changed)(void *user_data, bool is_downward);
    void (*on_incline_motor_upward_status_changed)(void *user_data, bool is_upward);
    void (*on_main_motor_status_changed)(void *user_data, bool is_running);
    void (*on_encoder_rpm_status_changed)(void *user_data, bool is_running);
    void (*on_battery_capacity_status_changed)(void *user_data, bool is_not_enough);
    void (*on_command_error_status_changed)(void *user_data, bool has_error);
    void (*on_lcb_error_status_detected)(void *user_data, bool has_error);
    void (*on_initial_calibration_status_changed)(void *user_data, bool is_calibrating);

    /* Command 0x70-0x7F callbacks */
    /**
     * @brief Callback for LCB initial command
     */
    void (*on_lcb_initial)(void *user_data);
    /**
     * @brief Callback for LCB status command
     * @param user_data User-defined context pointer
     * @param lcb_status LCB status value
     */
    void (*on_lcb_status)(void *user_data, uint8_t lcb_status);
    /**
     * @brief Callback for LCB error code command
     * @param user_data User-defined context pointer
     * @param error_code Error code value
     */
    void (*on_lcb_error_code)(void *user_data, uint16_t error_code);
    /**
     * @brief Callback to return error code to UI
     * @param user_data User-defined context pointer
     * @param error_code Error code value
     */
    void (*on_return_error_code_to_ui)(void *user_data, uint16_t error_code);
    /**
     * @brief Callback for LCB version command
     * @param user_data User-defined context pointer
     * @param version Version value
     */
    void (*on_lcb_version)(void *user_data, uint16_t version);
    /**
     * @brief Callback for LCB calibrate command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_calibrate)(void *user_data);
    /**
     * @brief Callback for LCB update program command
     * @param user_data User-defined context pointer
     * @param response Response value
     */
    void (*on_lcb_update_program)(void *user_data, uint8_t response);
    /**
     * @brief Callback for LCB skip current error command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_skip_curr_error)(void *user_data);
    /**
     * @brief Callback for LCB special external command
     * @param user_data User-defined context pointer
     * @param cmd Command value
     */
    void (*on_lcb_spe_ext_cmd)(void *user_data, uint8_t cmd);
    /**
     * @brief Callback for LCB DCI version command
     * @param user_data User-defined context pointer
     * @param dci_info DCI info data
     * @param length Data length
     */
    void (*on_lcb_dci_version)(void *user_data, const uint8_t *dci_info, uint8_t length);
    /**
     * @brief Callback for LCB DCI environment command
     * @param user_data User-defined context pointer
     * @param dci_env DCI environment data
     * @param length Data length
     */
    void (*on_lcb_dci_env)(void *user_data, const uint8_t *dci_env, uint8_t length);
    /**
     * @brief Callback for LCB command EUPS mode
     * @param user_data User-defined context pointer
     * @param response Response value
     */
    void (*on_lcb_cmd_eups_mode)(void *user_data, uint8_t response);
    /**
     * @brief Callback for LCB command console power
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_cmd_console_power)(void *user_data);
    /**
     * @brief Callback for LCB protocol version command
     * @param user_data User-defined context pointer
     * @param version Protocol version value
     */
    void (*on_lcb_protocol_version)(void *user_data, uint16_t version);
    /**
     * @brief Callback for LCB command serial number
     * @param user_data User-defined context pointer
     * @param data Serial number data
     * @param length Data length
     */
    void (*on_lcb_cmd_serial_number)(void *user_data, const uint8_t *data, uint8_t length);
    /**
     * @brief Callback for LCB treadmill operate command
     * @param user_data User-defined context pointer
     * @param data Operation data
     * @param length Data length
     */
    void (*on_lcb_treadmill_operate)(void *user_data, const uint8_t *data, uint8_t length);

    /* Treadmill commands 0xF0-0xFF */
    /**
     * @brief Callback for LCB motor speed return RPM command
     * @param user_data User-defined context pointer
     * @param rpm Motor speed in RPM
     */
    void (*on_lcb_motor_speed_rtn_rpm)(void *user_data, uint16_t rpm);
    /**
     * @brief Callback for LCB motor RPM command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_motor_rpm)(void *user_data);
    /**
     * @brief Callback for LCB increase PWM step command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_inc_pwm_step)(void *user_data);
    /**
     * @brief Callback for LCB decrease PWM step command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_dec_pwm_step)(void *user_data);
    /**
     * @brief Callback for LCB stop PWM step command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_stop_pwm_step)(void *user_data);
    /**
     * @brief Callback for LCB incline action command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_incline_action)(void *user_data);
//    void (*on_lcb_incline_loc)(void *user_data);
    /**
     * @brief Callback for LCB incline location command
     * @param user_data User-defined context pointer
     * @param location Incline location value
     */
    void (*on_lcb_incline_loc)(void *user_data, uint16_t location);
    /**
     * @brief Callback for LCB operating status command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_operating_status)(void *user_data);
    /**
     * @brief Callback for LCB roller RPM command
     * @param user_data User-defined context pointer
     * @param rpm Roller speed in RPM
     */
    void (*on_lcb_roller_rpm)(void *user_data, uint16_t rpm);
    /**
     * @brief Callback for LCB motor RPM received command
     * @param user_data User-defined context pointer
     * @param rpm Motor speed in RPM
     */
    void (*on_lcb_motor_rpm_received)(void *user_data, uint16_t rpm);
    /**
     * @brief Callback for one-shot LCB motor RPM command
     * @param user_data User-defined context pointer
     * @param rpm Motor speed in RPM
     */
    void (*on_one_shot_lcb_motor_rpm)(void *user_data, uint16_t rpm);
    /**
     * @brief Callback for LCB incline location received command
     * @param user_data User-defined context pointer
     * @param location Incline location value
     */
    void (*on_lcb_incline_loc_received)(void *user_data, uint16_t location);
    /**
     * @brief Callback for LCB tune end incline command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_tune_end_incline)(void *user_data);
    /**
     * @brief Callback for LCB tune end incline 2 command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_tune_end_incline2)(void *user_data);
    /**
     * @brief Callback for LCB incline stroke command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_incline_stroke)(void *user_data);
    /**
     * @brief Callback for LCB compensation voltage command
     * @param user_data User-defined context pointer
     * @param voltage Compensation voltage value
     */
    void (*on_lcb_compensation_vol)(void *user_data, uint16_t voltage);

    /* Motor and DCI commands 0x90-0xAF */
    /**
     * @brief Callback for LCB motor specification command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_motor_specification)(void *user_data);
    /**
     * @brief Callback for LCB motor type command
     * @param user_data User-defined context pointer
     * @param motor_type Motor type value
     */
    void (*on_lcb_motor_type)(void *user_data, uint8_t motor_type);
    /**
     * @brief Callback for LCB DCI data max length command
     * @param user_data User-defined context pointer
     * @param max_len Maximum data length
     */
    void (*on_lcb_dci_data_max_len)(void *user_data, uint8_t max_len);
    /**
     * @brief Callback for LCB motor power off command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_motor_power_off)(void *user_data);
    /**
     * @brief Callback for LCB DCI force incline command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_dci_force_incline)(void *user_data);
    /**
     * @brief Callback for LCB treadmill in use command
     * @param user_data User-defined context pointer
     * @param status In use status
     */
    void (*on_lcb_treadmill_in_used)(void *user_data, uint8_t status);
    /**
     * @brief Callback for LCB main motor info command
     * @param user_data User-defined context pointer
     * @param info Motor info data
     * @param length Data length
     */
    void (*on_lcb_main_motor_info)(void *user_data, const uint8_t *info, uint8_t length);
    /**
     * @brief Callback for LCB treadmill extended status command
     * @param user_data User-defined context pointer
     * @param status Extended status value
     */
    void (*on_lcb_treadmill_ext_status)(void *user_data, uint16_t status);
    /**
     * @brief Callback for LCB treadmill cadence (SPM) command (0x7F)
     * @param data Raw cadence payload (forwarded as-is, matching Java's byte[])
     * @param length Data length
     */
    void (*on_lcb_treadmill_cadence)(void *user_data, const uint8_t *data, uint8_t length);
    /**
     * @brief Callback for LCB treadmill ECB action command
     * @param user_data User-defined context pointer
     * @param data Action data
     */
    void (*on_lcb_treadmill_ecb_action)(void *user_data, uint8_t data);
    /**
     * @brief Callback for LCB DCI incline range command
     * @param user_data User-defined context pointer
     * @param range Incline range value
     */
    void (*on_lcb_dci_incline_range)(void *user_data, uint32_t range);
    /**
     * @brief Callback for LCB DCI incline range set command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_dci_incline_range_set)(void *user_data);
    /**
     * @brief Callback for LCB DCI RPM speed range command
     * @param user_data User-defined context pointer
     * @param range RPM speed range value
     */
    void (*on_lcb_dci_rpm_speed_range)(void *user_data, uint32_t range);
    /**
     * @brief Callback for LCB DCI RPM speed range set command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_dci_rpm_speed_range_set)(void *user_data);
    /**
     * @brief Callback for LCB universal command
     * @param user_data User-defined context pointer
     * @param value Universal value
     */
    void (*on_lcb_universal)(void *user_data, uint16_t value);
    /**
     * @brief Universal data sub-command callbacks (0xAA sub-commands)
     * Set to NULL if not using universal data sub-command dispatching.
     * When set, lcb_proc_set_universal() will dispatch sub-commands
     * to the appropriate handler in addition to firing on_lcb_universal.
     */
    const struct lcb_universal_data_callbacks *universal_data_cb;
    /**
     * @brief Callback for LCB DCI manual calibration command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_dci_manual_calibration)(void *user_data);

    /* Device data and indoor bike commands 0x50-0x5F */
    /**
     * @brief Callback for LCB device data command
     * @param user_data User-defined context pointer
     * @param data Device data
     * @param length Data length
     */
    void (*on_lcb_device_data)(void *user_data, const uint8_t *data, uint8_t length);
    /**
     * @brief Device data sub-command callbacks (0x50 sub-commands)
     * Set to NULL if not using device data sub-command dispatching.
     * When set, lcb_proc_set_device_data() will dispatch sub-commands
     * to the appropriate handler in addition to firing on_lcb_device_data.
     */
    const struct lcb_device_data_callbacks *device_data_cb;
    /**
     * @brief Callback for LCB indoor bike sensor data command
     * @param user_data User-defined context pointer
     * @param sensor_data Indoor bike sensor data structure
     */
    void (*on_lcb_indoor_bike_sensor_data)(void *user_data, const indoor_bike_sensor_data_t *sensor_data);
    /**
     * @brief Callback for one-shot LCB indoor bike sensor data command
     * @param user_data User-defined context pointer
     * @param sensor_data Indoor bike sensor data structure
     */
    void (*on_one_shot_lcb_indoor_bike_sensor_data)(void *user_data, const indoor_bike_sensor_data_t *sensor_data);
    /**
     * @brief Callback for LCB indoor bike stepper motor position command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_indoor_bike_stepper_motor_position)(void *user_data);
    /**
     * @brief Callback for LCB indoor bike calculate parameters of device command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_indoor_bike_calc_para_of_device)(void *user_data);
    /**
     * @brief Callback for LCB indoor bike calculate parameters of device get command
     * @param user_data User-defined context pointer
     * @param data Parameter data
     * @param length Data length
     */
    void (*on_lcb_indoor_bike_calc_para_of_device_get)(void *user_data, const uint8_t *data, uint8_t length);
    /**
     * @brief Callback for LCB indoor bike action spin down command
     * @param user_data User-defined context pointer
     * @param data Spin down data
     * @param length Data length
     */
    void (*on_lcb_indoor_bike_action_spin_down)(void *user_data, const uint8_t *data, uint8_t length);
    /**
     * @brief Callback for LCB indoor bike info spin down command
     * @param user_data User-defined context pointer
     * @param info Spin down info structure
     */
    void (*on_lcb_indoor_bike_info_spin_down)(void *user_data, const indoor_bike_info_spin_down_t *info);
    /**
     * @brief Callback for LCB indoor bike ERG mode settings command
     * @param user_data User-defined context pointer
     * @param data Settings data
     * @param length Data length
     */
    void (*on_lcb_indoor_bike_erg_mode_settings)(void *user_data, const uint8_t *data, uint8_t length);
    /**
     * @brief Callback for LCB indoor bike ERG mode settings get command
     * @param user_data User-defined context pointer
     * @param data Settings data
     * @param length Data length
     */
    void (*on_lcb_indoor_bike_erg_mode_settings_get)(void *user_data, const uint8_t *data, uint8_t length);
    /**
     * @brief Callback for LCB indoor bike resistance level command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_indoor_bike_resistance_level)(void *user_data);

    /* ECB and bike commands 0x60-0x6F */
    /**
     * @brief Callback for LCB reset ECB command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_reset_ecb)(void *user_data);
    /**
     * @brief Callback for LCB ECB position command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_ecb_position)(void *user_data);
    /**
     * @brief Callback for LCB ECB RPM command
     * @param user_data User-defined context pointer
     * @param rpm ECB speed in RPM
     */
    void (*on_lcb_ecb_rpm)(void *user_data, uint16_t rpm);
    /**
     * @brief Callback for one-shot LCB ECB RPM command
     * @param user_data User-defined context pointer
     * @param rpm ECB speed in RPM
     */
    void (*on_one_shot_lcb_ecb_rpm)(void *user_data, uint16_t rpm);
    /**
     * @brief Callback for LCB ECB ADC command
     * @param user_data User-defined context pointer
     * @param adc ADC value
     */
    void (*on_lcb_ecb_adc)(void *user_data, uint16_t adc);
    /**
     * @brief Callback for LCB PWM percent command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_pwm_percent)(void *user_data);
    /**
     * @brief Callback for LCB battery voltage command
     * @param user_data User-defined context pointer
     * @param voltage Battery voltage value
     */
    void (*on_lcb_battery_voltage)(void *user_data, uint16_t voltage);
    /**
     * @brief Callback for LCB battery voltage low priority command
     * @param user_data User-defined context pointer
     * @param voltage Battery voltage value
     */
    void (*on_lcb_battery_voltage_low_priority)(void *user_data, uint16_t voltage);
    /**
     * @brief Callback for LCB watts command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_watts)(void *user_data);
    /**
     * @brief Callback for LCB RPM gear ratio command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_rpm_gear_ratio)(void *user_data);
    /**
     * @brief Callback for LCB generator pole pair command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_generator_pole_pair)(void *user_data);
    /**
     * @brief Callback for LCB limit RPM charge command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_limit_rpm_charge)(void *user_data);
    /**
     * @brief Callback for LCB limit RPM resistance command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_limit_rpm_resistance)(void *user_data);
    /**
     * @brief Callback for LCB machine type command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_machine_type)(void *user_data);
    /**
     * @brief Callback for LCB power off command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_power_off)(void *user_data);
    /**
     * @brief Callback for LCB battery charge level command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_battery_charge_level)(void *user_data);
    /**
     * @brief Callback for LCB minimum resistance incline working command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_min_resistance_incline_working)(void *user_data);

    /* Incline and ECB commands 0x80-0x8F */
    /**
     * @brief Callback for LCB incline percent command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_incline_percent)(void *user_data);
    /**
     * @brief Callback for LCB incline percent received command
     * @param user_data User-defined context pointer
     * @param percent Incline percentage value
     */
    void (*on_lcb_incline_percent_received)(void *user_data, uint16_t percent);
    /**
     * @brief Callback for LCB ECB action command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_ecb_action)(void *user_data);
    /**
     * @brief Callback for LCB ECB status command
     * @param user_data User-defined context pointer
     * @param status ECB status value
     */
    void (*on_lcb_ecb_status)(void *user_data, uint8_t status);
    /**
     * @brief Callback for LCB ECB count command
     * @param user_data User-defined context pointer
     * @param count ECB count value
     */
    void (*on_lcb_ecb_count)(void *user_data, uint16_t count);
    /**
     * @brief Callback for LCB electromagnetic current command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_electromagnetic_current)(void *user_data);
    /**
     * @brief Callback for LCB resistance type command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_resistance_type)(void *user_data);
    /**
     * @brief Callback for LCB begin battery charge command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_begin_battery_charge)(void *user_data);
    /**
     * @brief Callback for LCB battery capacity command
     * @param user_data User-defined context pointer
     * @param capacity Battery capacity value
     */
    void (*on_lcb_battery_capacity)(void *user_data, uint16_t capacity);
    /**
     * @brief Callback for LCB climbmill status command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_climbmill_status)(void *user_data);
    /**
     * @brief Callback for LCB climbmill status received command
     * @param user_data User-defined context pointer
     * @param status Climbmill status value
     */
    void (*on_lcb_climbmill_status_received)(void *user_data, uint16_t status);
    /**
     * @brief Callback for LCB E-stop parameter command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_e_stop_parameter)(void *user_data);
    /**
     * @brief Callback for LCB brake pressure value command
     * @param user_data User-defined context pointer
     * @param value Brake pressure value
     */
    void (*on_lcb_brake_pressure_value)(void *user_data, uint16_t value);
    /**
     * @brief Callback for LCB watts received command
     * @param user_data User-defined context pointer
     * @param watts Power in watts
     */
    void (*on_lcb_watts_received)(void *user_data, uint16_t watts);
    /**
     * @brief Callback for LCB climbmill calibration command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_climbmill_cal)(void *user_data);

    /* Calibration commands 0x40-0x4F */
    /**
     * @brief Callback for LCB record current command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_record_current)(void *user_data);
    /**
     * @brief Callback for LCB PWM command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_pwm)(void *user_data);
    /**
     * @brief Callback for LCB finish calibrating command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_finish_calibrating)(void *user_data);
    /**
     * @brief Callback for LCB SPM command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_spm)(void *user_data);
    /**
     * @brief Callback for LCB SPM received command
     * @param user_data User-defined context pointer
     * @param spm SPM value
     */
    void (*on_lcb_spm_received)(void *user_data, uint16_t spm);
    /**
     * @brief Callback for one-shot LCB SPM command
     * @param user_data User-defined context pointer
     * @param spm SPM value
     */
    void (*on_one_shot_lcb_spm)(void *user_data, uint16_t spm);
    /**
     * @brief Callback for LCB calibration scaling command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_cal_scaling)(void *user_data);
    /**
     * @brief Callback for LCB calibration scaling received command
     * @param user_data User-defined context pointer
     * @param scaling Scaling value
     */
    void (*on_lcb_cal_scaling_received)(void *user_data, uint16_t scaling);
    /**
     * @brief Callback for LCB frame calibration scaling command
     * @param user_data User-defined context pointer
     */
    void (*on_lcb_frame_cal_scaling)(void *user_data);
    /**
     * @brief Callback for LCB frame calibration scaling received command
     * @param user_data User-defined context pointer
     * @param scaling Frame scaling value
     */
    void (*on_lcb_frame_cal_scaling_received)(void *user_data, uint16_t scaling);

    /* Special callbacks */
    /**
     * @brief Callback for treadmill RPM value set command
     * @param user_data User-defined context pointer
     * @param rpm RPM value to set
     */
    void (*on_treadmill_rpm_value_set)(void *user_data, uint16_t rpm);
    /**
     * @brief Callback for treadmill ADC value set command
     * @param user_data User-defined context pointer
     * @param adc ADC value to set
     */
    void (*on_treadmill_adc_value_set)(void *user_data, uint16_t adc);
    /**
     * @brief Callback for E-stop machine stop command
     * @param user_data User-defined context pointer
     */
    void (*on_estop_machine_stop)(void *user_data);
    /**
     * @brief Callback for E-stop MCB error command
     * @param user_data User-defined context pointer
     */
    void (*on_estop_mcb_error)(void *user_data);
    /**
     * @brief Callback for E-stop unlock command
     * @param user_data User-defined context pointer
     */
    void (*on_estop_unlock)(void *user_data);
    /**
     * @brief Callback for treadmill no RPM error command
     * @param user_data User-defined context pointer
     */
    void (*on_tm_no_rpm_error)(void *user_data);
    /**
     * @brief Callback for set advanced logging status command
     * @param user_data User-defined context pointer
     */
    void (*on_set_adv_logging_status)(void *user_data);

    /* Generic callback for unhandled commands */
    /**
     * @brief Callback for unknown/unhandled commands
     * @param user_data User-defined context pointer
     * @param command Command byte
     * @param data Command data
     * @param data_len Data length
     */
    void (*on_unknown_command)(void *user_data, uint8_t command, const uint8_t *data, uint8_t data_len);

    /* ========================================================================
     * IR Address Board (0x26) — "Hybrid TM" IR beam E-Stop sensor.
     * Dispatched separately from the main-LCB switch above: the comm layer
     * remembers which address the just-sent request targeted and routes the
     * matching response here instead, since 0x73/0x7A are the same opcodes
     * the main LCB also uses (see lcb_packet.h's LCB_PACKET_ADDRESS_IR_BOARD).
     * ======================================================================== */
    /**
     * @brief IR Address board firmware version (0x73 GetLCBVersion @ 0x26)
     */
    void (*on_ir_addr_version)(void *user_data, int version);
    /**
     * @brief IR beam status (0x99 GetIrAddressIrStatus @ 0x26): 0x00=clear, 0x01=blocked
     */
    void (*on_ir_addr_ir_status)(void *user_data, uint8_t status);
    /**
     * @brief IR Address board entered ERP/standby mode successfully (0x7A @ 0x26)
     */
    void (*on_ir_addr_enter_erp_mode)(void *user_data, bool success);
    /**
     * @brief IR Address board left ERP/standby mode successfully (0x7A @ 0x26)
     */
    void (*on_ir_addr_leave_erp_mode)(void *user_data, bool success);
    /**
     * @brief Raw SetLCBCmdEUPsMode response byte from the IR Address board
     * (0x7A @ 0x26), forwarded unconditionally regardless of enter/leave
     * context — mirrors Java's setIrAddrCmdEUPsModeHandler(byte byteRtn),
     * which fires independently of on_ir_addr_enter_erp_mode/leave_erp_mode.
     */
    void (*on_ir_addr_erp_rtn_byte)(void *user_data, uint8_t byte_rtn);

} lcb_comm_callbacks_t;

/**
 * @brief Initialize callback structure with NULL values
 * @param callbacks Pointer to callback structure
 */
void lcb_comm_callbacks_init(lcb_comm_callbacks_t *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* LCB_COMM_CALLBACK_H */
