/**
 * @file lcb_comm.h
 * @brief LCB Communication Interface
 *
 * Converted from: com/jht/lcbcommlib/ILcbComm.java and LcbComm.java
 *
 * This is the main interface for LCB communication.
 */

#ifndef LCB_COMM_H
#define LCB_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_commands.h"
#include "lcb_packet_queue.h"
#include "lcb_comm_callback.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
typedef struct lcb_comm lcb_comm_t;

/**
 * @brief Serial port configuration structure
 */
typedef struct serial_port_config {
    int baud_rate;
    int data_bits;
    int stop_bits;
    char parity;
} serial_port_config_t;

/**
 * @brief Serial port structure
 */
typedef struct serial_port {
    int fd;                     /* File descriptor */
    char device_path[256];      /* Device path */
    serial_port_config_t config;/* Port configuration */
} serial_port_t;

/**
 * @brief Queue mode enumeration
 * Converted from: LcbComm.QueueMode
 */
typedef enum {
    LCB_QUEUE_MODE_ALWAYS_ADD = 0,             /* Default: always add packet to queue */
    LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED = 1, /* Add only if identical packet not queued */
    LCB_QUEUE_MODE_REPLACE_SIMILAR = 2         /* Replace existing packet with same command */
} lcb_queue_mode_t;

/**
 * @brief Create LCB communication instance
 * @return Pointer to LCB communication instance or NULL on error
 */
lcb_comm_t* lcb_comm_create(void);

/**
 * @brief Destroy LCB communication instance
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_destroy(lcb_comm_t *comm);

/**
 * @brief Register receive callbacks
 * @param comm Pointer to LCB communication instance
 * @param callbacks Pointer to callback structure
 * @return 0 on success, negative on error
 */
int lcb_comm_register_callbacks(lcb_comm_t *comm, const lcb_comm_callbacks_t *callbacks);

/**
 * @brief Unregister receive callbacks
 * @param comm Pointer to LCB communication instance
 * @return 0 on success, negative on error
 */
int lcb_comm_unregister_callbacks(lcb_comm_t *comm);

/**
 * @brief Start communication with serial port
 * @param comm Pointer to LCB communication instance
 * @param device Serial port device
 * @return true on success, false on error
 */
bool lcb_comm_start(lcb_comm_t *comm, serial_port_t *device);

/**
 * @brief Restart communication
 * @param comm Pointer to LCB communication instance
 * @return true on success, false on error
 */
bool lcb_comm_restart(lcb_comm_t *comm);

/**
 * @brief Stop communication
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_stop(lcb_comm_t *comm);

/**
 * @brief Check if communication thread is running
 * @param comm Pointer to LCB communication instance
 * @return true if running, false otherwise
 */
bool lcb_comm_check_thread(lcb_comm_t *comm);

/**
 * @brief Check if communication thread has exception
 * @param comm Pointer to LCB communication instance
 * @return true if exception occurred, false otherwise
 */
bool lcb_comm_check_thread_except(lcb_comm_t *comm);

/**
 * @brief ERP stop UART port
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_erp_stop_uart_port(lcb_comm_t *comm);

/**
 * @brief Check boot up leave LCB ERP
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_check_boot_up_leave_lcb_erp(lcb_comm_t *comm);

/**
 * @brief Enter LCB ERP mode
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_enter_lcb_erp_mode(lcb_comm_t *comm);

/**
 * @brief Leave LCB ERP mode
 * @param comm Pointer to LCB communication instance
 * @param device_path LCB device path
 * @param baud_rate Baud rate
 */
void lcb_comm_leave_lcb_erp_mode(lcb_comm_t *comm, const char *device_path, uint32_t baud_rate);

/**
 * @brief Get transmit queue size
 * @param comm Pointer to LCB communication instance
 * @return Queue size
 */
uint32_t lcb_comm_get_tx_queue_size(const lcb_comm_t *comm);

/**
 * @brief Get Lcb Communication Disconnect state
 * @param comm Pointer to LCB communication instance
 * @return true if disconnected, false otherwise
 */
bool lcb_comm_get_lcb_comm_disconnect_state(const lcb_comm_t *comm);

/* ============================================================================
 * LCB Command Functions (0x70-0x7F)
 * ============================================================================ */

/**
 * @brief Command ID 0x70: Initialize the LCB.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_lcb_initial(lcb_comm_t *comm);

/**
 * @brief Command ID 0x70: Initialize the LCB with highest priority.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_lcb_initial_highest_priority(lcb_comm_t *comm);

/**
 * @brief Command ID 0x71: Get The LCB specific status.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_status(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x71: Get The LCB specific status with low priority.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_status_low_priority(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x72: Get The LCB most recent error code.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_error_code(lcb_comm_t *comm);

/**
 * @brief Command ID 0x72: Get The LCB most recent error code with high priority.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_error_code_high_priority(lcb_comm_t *comm);

/**
 * @brief Command ID 0x73: Get the LCB firmware revision.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_version(lcb_comm_t *comm);

/**
 * @brief Command ID 0x73: Get the LCB firmware revision with highest priority.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_version_highest_priority(lcb_comm_t *comm);

/**
 * @brief Command ID 0x74: Force the LCB to perform a calibration.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_lcb_calibrate(lcb_comm_t *comm);

/**
 * @brief Command ID 0x75: Update program 2 bytes. For LCBA : "JS" => LCB MCU-B ,"JT" => LCB MCU-B Loader ,"JA" => LCB MCU-A ,"JB" => LCB MCU-A Loader.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_update_program(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/**
 * @brief Command ID 0x76: UCB received the error code already the LCB to allow to skip current error.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_lcb_skip_curr_error(lcb_comm_t *comm);

/**
 * @brief Command ID 0x76: UCB received the error code already the LCB to allow to skip current error with high priority.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_lcb_skip_curr_error_high_priority(lcb_comm_t *comm);

/**
 * @brief Command ID 0x77: Special extend command.
 * @param comm Pointer to LCB communication instance
 * @param cmd Command value
 */
void lcb_comm_set_lcb_spe_ext_cmd(lcb_comm_t *comm, uint8_t cmd);

/**
 * @brief Command ID 0x78: Get DCI Version (DCI LCB only).
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_dci_version(lcb_comm_t *comm);

/**
 * @brief Command ID 0x79: Get DCI Environment.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_dci_env(lcb_comm_t *comm);

/**
 * @brief Command ID 0x7A: Set CmdEUPsMode command.
 * @param comm Pointer to LCB communication instance
 * @param cmd Command value
 * @param do_pulse Pulse flag
 */
void lcb_comm_set_lcb_cmd_eups_mode(lcb_comm_t *comm, uint8_t cmd, bool do_pulse);

/**
 * @brief Command ID 0x7B: Set the console power to LCB.
 * @param comm Pointer to LCB communication instance
 * @param watts Watts value
 */
void lcb_comm_set_lcb_cmd_console_power(lcb_comm_t *comm, uint16_t watts);

/**
 * @brief Command ID 0x7C: Get digital communication protocol version 0x0064 => v1.00.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_protocol_version(lcb_comm_t *comm);

/**
 * @brief Command ID 0x7D: Set serial number save to MCB/LCB.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_cmd_serial_number(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/**
 * @brief Command ID 0x7E: Treadmill Operate.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_treadmill_operate(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/* ============================================================================
 * Treadmill Motor Commands (0xF0-0xFF)
 * ============================================================================ */

/**
 * @brief Command ID 0xF0: Motor speed setting and response RPM. LCB Motor RPM setting and require LCB to sent the motor RPM data to UCB.
 * @param comm Pointer to LCB communication instance
 * @param speed Speed value
 */
void lcb_comm_set_lcb_motor_speed_rtn_rpm(lcb_comm_t *comm, uint16_t speed);

/**
 * @brief Command ID 0xF1: Set motor RPM. Bit 15 : 0 (Motor control signal in normal) Bit 15 : 1 (Motor control signal in reverse).
 * @param comm Pointer to LCB communication instance
 * @param rpm RPM value
 */
void lcb_comm_set_lcb_motor_rpm(lcb_comm_t *comm, uint16_t rpm);

/**
 * @brief Command ID 0xF1: Set motor RPM with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param rpm RPM value
 */
void lcb_comm_set_lcb_motor_rpm_highest_priority(lcb_comm_t *comm, uint16_t rpm);

/**
 * @brief Command ID 0xF2: SPWM add step. Increment the PWM step by the specified value 50~500 (0.01M).
 * @param comm Pointer to LCB communication instance
 * @param pwm PWM value
 */
void lcb_comm_set_lcb_inc_pwm_step(lcb_comm_t *comm, uint16_t pwm);

/**
 * @brief Command ID 0xF3: PWM decrement step. Decrement the PWM step by the specified value 50~500 (0.01M).
 * @param comm Pointer to LCB communication instance
 * @param pwm PWM value
 */
void lcb_comm_set_lcb_dec_pwm_step(lcb_comm_t *comm, uint16_t pwm);

/**
 * @brief Command ID 0xF4: PWM stop step. Set PWM step decrement value in Ramp down condition 50~500 (0.01M).
 * @param comm Pointer to LCB communication instance
 * @param pwm PWM value
 */
void lcb_comm_set_lcb_stop_pwm_step(lcb_comm_t *comm, uint16_t pwm);

/**
 * @brief Command ID 0xF5: Set incline action. 0x00: to stop ; 0x01: to rise ; 0xFF: to drop.
 * @param comm Pointer to LCB communication instance
 * @param action Action value
 */
void lcb_comm_set_lcb_incline_action(lcb_comm_t *comm, uint8_t action);

/**
 * @brief Command ID 0xF5: Set incline action with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param action Action value
 */
void lcb_comm_set_lcb_incline_action_highest_priority(lcb_comm_t *comm, uint8_t action);

/**
 * @brief Command ID 0xF6: Set incline location. Bit 15 : 0 (Normal) Bit 15 : 1 (Reverse) 0~32767.
 * @param comm Pointer to LCB communication instance
 * @param location Location value
 */
void lcb_comm_set_lcb_incline_loc(lcb_comm_t *comm, uint16_t location);

/**
 * @brief Command ID 0xF6: Set incline location with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param location Location value
 */
void lcb_comm_set_lcb_incline_loc_highest_priority(lcb_comm_t *comm, uint16_t location);

/**
 * @brief Command ID 0xF7: Set operating status. 0x00: to stop ; 0x01: to operating.
 * @param comm Pointer to LCB communication instance
 * @param status Status value
 */
void lcb_comm_set_lcb_operating_status(lcb_comm_t *comm, uint8_t status);

/**
 * @brief Command ID 0xF7: Set operating status with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param status Status value
 */
void lcb_comm_set_lcb_operating_status_highest_priority(lcb_comm_t *comm, uint8_t status);

/**
 * @brief Command ID 0xF8: Get the roller RPM.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_roller_rpm(lcb_comm_t *comm);

/**
 * @brief Command ID 0xF9: Get motor RPM. Bit 15 : 0 (Normal) Bit 15 : 1 (Reverse).
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_motor_rpm(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0xF9: Get one shot LCB motor RPM with high priority.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_one_shot_lcb_motor_rpm_high_priority(lcb_comm_t *comm);

/**
 * @brief Command ID 0xFA: Get incline location ADC value 0~32767.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_incline_loc(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0xFB: Tune endpoint incline. 1st: Up limit 0~25.5%, 2nd: Down limit 0~25.5% of full stroke 0.1%.
 * @param comm Pointer to LCB communication instance
 * @param percentage Percentage value
 */
void lcb_comm_set_lcb_tune_end_incline(lcb_comm_t *comm, uint16_t percentage);

/**
 * @brief Command ID 0xFC: Tune endpoint incline 2. 1st: Up limit 0~25.5%, 2nd: Down limit 0~25.5% of full stroke 0.1%.
 * @param comm Pointer to LCB communication instance
 * @param percentage Percentage value
 */
void lcb_comm_set_lcb_tune_end_incline2(lcb_comm_t *comm, uint16_t percentage);

/**
 * @brief Command ID 0xFD: Set incline stroke. Incline stroke setting 0~32768 %.
 * @param comm Pointer to LCB communication instance
 * @param percentage Percentage value
 */
void lcb_comm_set_lcb_incline_stroke(lcb_comm_t *comm, uint16_t percentage);

/**
 * @brief Command ID 0xFE: Set compensation voltage for Electromagnetic temperature compensation 0~65535 0.01V.
 * @param comm Pointer to LCB communication instance
 * @param voltage Voltage value
 */
void lcb_comm_set_lcb_compensation_vol(lcb_comm_t *comm, uint16_t voltage);

/* ============================================================================
 * Motor and DCI Commands (0x90-0xAF)
 * ============================================================================ */

/**
 * @brief Command ID 0x90: Motor specification set. 1100:1.1k W / 1500:1.5k W / 1800:1.8k W watts.
 * @param comm Pointer to LCB communication instance
 * @param watts Watts value
 */
void lcb_comm_set_lcb_motor_specification(lcb_comm_t *comm, uint16_t watts);

/**
 * @brief Command ID 0x91: Get motor type 0x00 : with encoder 0xFF : without encoder.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_motor_type(lcb_comm_t *comm);

/**
 * @brief Command ID 0x92: Get DCI max. data length. Get the equipment max data length.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_dci_data_max_len(lcb_comm_t *comm);

/**
 * @brief Command ID 0x93: Set motor power off (used for incline calibration only).
 * @param comm Pointer to LCB communication instance
 * @param time Time value
 */
void lcb_comm_set_lcb_motor_power_off(lcb_comm_t *comm, uint8_t time);

/**
 * @brief Command ID 0x94: Set DCI FORCE INCLINE OPERATION (used for incline calibration only).
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_dci_force_incline(lcb_comm_t *comm, uint8_t value);

/**
 * @brief Command ID 0x95: Get Treadmill in used. 0xFF: used , 0x55: non used.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_treadmill_in_used(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x96: CmdGetMainMotor Information 12 bytes (Output Freq, Current, Volt, DC Bus, Temp, etc.).
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_main_motor_info(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x97: Get the status of the LCB for treadmill extend (bit 0: TM-Estop).
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_treadmill_ext_status(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x98: Set Treadmill ECB Action (bit 0: speed, bit 1: position, bit 2: board on/off).
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_treadmill_ecb_action(lcb_comm_t *comm, uint8_t value);

/**
 * @brief Command ID 0x7F: Get Cadence (SPM).
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_treadmill_cadence(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x98: Set Treadmill ECB Action with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_treadmill_ecb_action_highest_priority(lcb_comm_t *comm, uint8_t value);

/**
 * @brief Command ID 0xA0: DCI Get Incline Range.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_dci_incline_range(lcb_comm_t *comm);

/**
 * @brief Command ID 0xA1: DCI Set Incline Range (works under MANUAL_CALIBRATION).
 * @param comm Pointer to LCB communication instance
 * @param range Range value
 */
void lcb_comm_set_lcb_dci_incline_range(lcb_comm_t *comm, uint32_t range);

/**
 * @brief Command ID 0xA6: DCI Get Rpm Speed Range.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_dci_rpm_speed_range(lcb_comm_t *comm);

/**
 * @brief Command ID 0xA7: DCI Set Rpm Speed Range (speed unit is 0.001MPH).
 * @param comm Pointer to LCB communication instance
 * @param range Range value
 */
void lcb_comm_set_lcb_dci_rpm_speed_range(lcb_comm_t *comm, uint32_t range);

/**
 * @brief Command ID 0xAA: Set special command for LCB.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 * @param mode Queue mode
 */
void lcb_comm_set_lcb_universal(lcb_comm_t *comm, const uint8_t *data, uint8_t length, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0xAB: DCI Set ManualCalibration.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_lcb_dci_manual_calibration(lcb_comm_t *comm);

/* ============================================================================
 * Device Data and Indoor Bike Commands (0x50-0x5F)
 * ============================================================================ */

/**
 * @brief Command ID 0x50: Transmitted LCBDeviceData.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 * @param mode Queue mode
 */
void lcb_comm_set_lcb_device_data(lcb_comm_t *comm, const uint8_t *data, uint8_t length, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x50: Transmitted LCBDeviceData with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_device_data_highest_priority(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/**
 * @brief Command ID 0x51: Get Indoor Bike Sensor Data.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_indoor_bike_sensor_data(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x51: Get one shot LCB indoor bike sensor data with high priority.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_one_shot_lcb_indoor_bike_sensor_data_high_priority(lcb_comm_t *comm);

/**
 * @brief Command ID 0x52: Set stepper motor position.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_indoor_bike_stepper_motor_position(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/**
 * @brief Command ID 0x53: Set calculation parameters of device or algorithm.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_indoor_bike_calc_para_of_device(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/**
 * @brief Command ID 0x54: Get calculation parameters of device or algorithm.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_indoor_bike_calc_para_of_device(lcb_comm_t *comm);

/**
 * @brief Command ID 0x55: Set the action of spin down.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_indoor_bike_action_spin_down(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/**
 * @brief Command ID 0x56: Get the info of spin down.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_indoor_bike_info_spin_down(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x57: Set Erg Mode Settings.
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_indoor_bike_erg_mode_settings(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/**
 * @brief Command ID 0x58: Set IndoorBike resistance level based on frame design. Level 0 is no resistance.
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_indoor_bike_resistance_level(lcb_comm_t *comm, uint16_t value);

/**
 * @brief Command ID 0x59: Get Erg Mode Settings.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_indoor_bike_erg_mode_settings(lcb_comm_t *comm);

/* ============================================================================
 * ECB and Bike Commands (0x60-0x6F)
 * ============================================================================ */

/**
 * @brief Command ID 0x61: Reset ECB value.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_lcb_reset_ecb(lcb_comm_t *comm);

/**
 * @brief Command ID 0x62: Set the ECB position.
 * @param comm Pointer to LCB communication instance
 * @param position Position value
 */
void lcb_comm_set_lcb_ecb_position(lcb_comm_t *comm, uint16_t position);

/**
 * @brief Command ID 0x63: Get ECB RPM 0~200.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_ecb_rpm(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x63: Get one shot LCB ECB RPM with high priority.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(lcb_comm_t *comm);

/**
 * @brief Command ID 0x64: Get ECB ADC.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_ecb_adc(lcb_comm_t *comm);

/**
 * @brief Command ID 0x65: Set PWM percent or resistance current (mA).
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_pwm_percent(lcb_comm_t *comm, uint16_t value);

/**
 * @brief Command ID 0x65: Set PWM percent or resistance current (mA) with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_pwm_percent_highest_priority(lcb_comm_t *comm, uint16_t value);

/**
 * @brief Command ID 0x66: Get battery capacity data 0.1V.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_battery_voltage(lcb_comm_t *comm);

/**
 * @brief Command ID 0x66: Get battery capacity data 0.1V with low priority.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_battery_voltage_low_priority(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x67: Set the Watts.
 * @param comm Pointer to LCB communication instance
 * @param watts Watts value
 * @param mode Queue mode
 */
void lcb_comm_set_lcb_watts(lcb_comm_t *comm, uint16_t watts, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x67: Set the Watts with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param watts Watts value
 */
void lcb_comm_set_lcb_watts_highest_priority(lcb_comm_t *comm, uint16_t watts);

/**
 * @brief Command ID 0x68: Set Set RPM gear ratio.
 * @param comm Pointer to LCB communication instance
 * @param ratio Ratio value
 */
void lcb_comm_set_lcb_rpm_gear_ratio(lcb_comm_t *comm, uint16_t ratio);

/**
 * @brief Command ID 0x69: Set the generator pole pair.
 * @param comm Pointer to LCB communication instance
 * @param pair Pair value
 */
void lcb_comm_set_lcb_generator_pole_pair(lcb_comm_t *comm, uint16_t pair);

/**
 * @brief Command ID 0x6A: Set limit RPM for charge (disables charge if below).
 * @param comm Pointer to LCB communication instance
 * @param rpm RPM value
 */
void lcb_comm_set_lcb_limit_rpm_charge(lcb_comm_t *comm, uint16_t rpm);

/**
 * @brief Command ID 0x6B: Set limit RPM for resistance (zero resistance if below).
 * @param comm Pointer to LCB communication instance
 * @param rpm RPM value
 */
void lcb_comm_set_lcb_limit_rpm_resistance(lcb_comm_t *comm, uint16_t rpm);

/**
 * @brief Command ID 0x6B: Set limit RPM for resistance with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param rpm RPM value
 */
void lcb_comm_set_lcb_limit_rpm_resistance_highest_priority(lcb_comm_t *comm, uint16_t rpm);

/**
 * @brief Command ID 0x6C: Set machine type (bike, stepper, EP, TM, ClimbMill).
 * @param comm Pointer to LCB communication instance
 * @param type Type value
 */
void lcb_comm_set_lcb_machine_type(lcb_comm_t *comm, uint8_t type);

/**
 * @brief Command ID 0x6D: Set power off (minute) for self-powered system.
 * @param comm Pointer to LCB communication instance
 * @param minutes Minutes value
 */
void lcb_comm_set_lcb_power_off(lcb_comm_t *comm, uint16_t minutes);

/**
 * @brief Command ID 0x6E: Set the battery charge level 0~100 %.
 * @param comm Pointer to LCB communication instance
 * @param level Level value
 */
void lcb_comm_set_lcb_battery_charge_level(lcb_comm_t *comm, uint16_t level);

/**
 * @brief Command ID 0x6F: Set min resistance when incline working (disable, enable, auto).
 * @param comm Pointer to LCB communication instance
 * @param type Type value
 */
void lcb_comm_set_lcb_min_resistance_incline_working(lcb_comm_t *comm, uint8_t type);

/* ============================================================================
 * Incline and ECB Commands (0x80-0x8F)
 * ============================================================================ */

/**
 * @brief Command ID 0x80: Set incline percent (0: 0% 32768: 100%).
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_incline_percent(lcb_comm_t *comm, uint16_t value);

/**
 * @brief Command ID 0x81: Get incline percent data travel speed (0: 0% 32768: 100%).
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_incline_percent(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x82: Set ECB action (upward, downward, stop).
 * @param comm Pointer to LCB communication instance
 * @param type Type value
 */
void lcb_comm_set_lcb_ecb_action(lcb_comm_t *comm, uint8_t type);

/**
 * @brief Command ID 0x83: Get ECB status (up, down, stop, etc.).
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_ecb_status(lcb_comm_t *comm);

/**
 * @brief Command ID 0x84: Get ECB count.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_ecb_count(lcb_comm_t *comm);

/**
 * @brief Command ID 0x85: Electromagnetic current. (mA).
 * @param comm Pointer to LCB communication instance
 * @param ma Milliampere value
 */
void lcb_comm_set_lcb_electromagnetic_current(lcb_comm_t *comm, uint16_t ma);

/**
 * @brief Command ID 0x86: Set Resistance type (electromagnet, resistor).
 * @param comm Pointer to LCB communication instance
 * @param ohm Ohm value
 */
void lcb_comm_set_lcb_resistance_type(lcb_comm_t *comm, uint16_t ohm);

/**
 * @brief Command ID 0x87: Set the battery charge current of no resistance 0 ~ 1000 mA.
 * @param comm Pointer to LCB communication instance
 * @param ma Milliampere value
 */
void lcb_comm_set_lcb_begin_battery_charge(lcb_comm_t *comm, uint16_t ma);

/**
 * @brief Command ID 0x88: Get battery the percentage of capacity 0 ~ 100 %.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_battery_capacity(lcb_comm_t *comm);

/**
 * @brief Command ID 0x8A: Set the action (include E-STOP) of the LCB for climbmill.
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_climbmill_status(lcb_comm_t *comm, uint8_t value);

/**
 * @brief Command ID 0x8A: Set the action (include E-STOP) of the LCB for climbmill with highest priority.
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_climbmill_status_highest_priority(lcb_comm_t *comm, uint8_t value);

/**
 * @brief Command ID 0x8B: Get the status of the LCB for climbmill (IR sensor, speed control, etc.).
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_climbmill_status(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x8C: Set the E-STOP parameter (4 ~ 100).
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_e_stop_parameter(lcb_comm_t *comm, uint8_t value);

/**
 * @brief Command ID 0x8D: Get brake pressure value 0~65535 0.01 Volt.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_brake_pressure_value(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x8E: Get Watts 0~65535 Watts.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_watts(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x8F: Set Calibrate climbmill (Weight, SPM, Time).
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_climbmill_cal(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/* ============================================================================
 * Calibration Commands (0x40-0x4F)
 * ============================================================================ */

/**
 * @brief Command ID 0x47: Set record current (point, current).
 * @param comm Pointer to LCB communication instance
 * @param data Data buffer
 * @param length Data length
 */
void lcb_comm_set_lcb_record_current(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/**
 * @brief Command ID 0x48: Set PWM 0 - 1000 (0.0% - 100.0%).
 * @param comm Pointer to LCB communication instance
 * @param percentage Percentage value
 */
void lcb_comm_set_lcb_pwm(lcb_comm_t *comm, uint16_t percentage);

/**
 * @brief Command ID 0x49: Finished calibrating one point (0: first, 1: second).
 * @param comm Pointer to LCB communication instance
 * @param value Value
 */
void lcb_comm_set_lcb_finish_calibrating(lcb_comm_t *comm, uint8_t value);

/**
 * @brief Command ID 0x4A: Set the step speed (SPM) of a climbmill (X10 resolution).
 * @param comm Pointer to LCB communication instance
 * @param spm SPM value
 */
void lcb_comm_set_lcb_spm(lcb_comm_t *comm, uint16_t spm);

/**
 * @brief Command ID 0x4B: Get SPM 0.1 SPM.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_spm(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x4B: Get one shot LCB SPM with high priority.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_one_shot_lcb_spm_high_priority(lcb_comm_t *comm);

/**
 * @brief Command ID 0x4C: Set calibration scaling 10-3000 (0.1%).
 * @param comm Pointer to LCB communication instance
 * @param cal_scaling Cal scaling value
 */
void lcb_comm_set_lcb_cal_scaling(lcb_comm_t *comm, uint16_t cal_scaling);

/**
 * @brief Command ID 0x4D: Get calibration scaling 0.1% 10-3000.
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode
 */
void lcb_comm_get_lcb_cal_scaling(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief Command ID 0x4E: Set frame calibration scaling 10-3000 (0.1%).
 * @param comm Pointer to LCB communication instance
 * @param frame_cal_scaling Frame cal scaling value
 */
void lcb_comm_set_lcb_frame_cal_scaling(lcb_comm_t *comm, uint16_t frame_cal_scaling);

/**
 * @brief Command ID 0x4F: Get frame calibration scaling 0.1% 10-3000.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_lcb_frame_cal_scaling(lcb_comm_t *comm);

/* ============================================================================
 * Process Functions (Error Injection and Utilities)
 * ============================================================================ */

/**
 * @brief Process ESTOP machine stop.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_proc_estop_machine_stop(lcb_comm_t *comm);

/**
 * @brief Process If UCB get estop signal, But MCB not have return error within 5s.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_proc_estop_mcb_error(lcb_comm_t *comm);

/**
 * @brief Process E-Stopp unlock trigger to UI.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_proc_estop_unlock(lcb_comm_t *comm);

/**
 * @brief Process If UCB not get RPM from LCB when already set RPM within 40s.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_proc_tm_no_rpm_error(lcb_comm_t *comm);

/**
 * @brief Process setAdvLoggingStatus.
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_proc_set_adv_logging_status(lcb_comm_t *comm);

/* ============================================================================
 * IR Address Board (0x26) API — "Hybrid TM" IR beam E-Stop sensor
 *
 * A second device on the same physical serial bus as the main LCB, reached
 * by addressing TX packets to 0x26 instead of 0xFF (same framing/CRC — see
 * lcb_packet.h's LCB_PACKET_ADDRESS_IR_BOARD). Responses are routed here
 * automatically by the comm thread based on which address the matching
 * request targeted (lcb_comm.c tracks this internally), since the board
 * reuses the main LCB's 0x73 GetLCBVersion / 0x7A SetLCBCmdEUPsMode opcodes.
 * ============================================================================ */

/**
 * @brief Request the IR Address board's firmware version (0x73 @ 0x26)
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_ir_addr_version(lcb_comm_t *comm);

/**
 * @brief Request the IR Address board's beam status (0x99 @ 0x26)
 * @param comm Pointer to LCB communication instance
 * @param mode Queue mode (e.g. ReplaceSimilar for periodic auto-poll)
 */
void lcb_comm_get_ir_addr_ir_status(lcb_comm_t *comm, lcb_queue_mode_t mode);

/**
 * @brief One-shot, highest-priority request for the IR beam status —
 * used to disambiguate a 0x02B2 E-Stop error code (IR-blocked vs real E-Stop).
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_get_ir_addr_ir_status_highest_priority(lcb_comm_t *comm);

/**
 * @brief Ask the IR Address board to enter ERP/standby mode (0x7A @ 0x26)
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_ir_addr_enter_erp_mode(lcb_comm_t *comm);

/**
 * @brief Ask the IR Address board to leave ERP/standby mode (0x7A @ 0x26,
 * highest priority — the board cannot process any other command until woken)
 * @param comm Pointer to LCB communication instance
 */
void lcb_comm_set_ir_addr_leave_erp_mode(lcb_comm_t *comm);

/* ============================================================================
 * Logging Control API
 * ============================================================================ */

/**
 * @brief Initialize LCB logging
 */
void lcb_log_init(void);

/**
 * @brief Set info logging enabled
 * @param enabled True to enable, false to disable
 */
void lcb_log_set_info(bool enabled);

/**
 * @brief Set debug logging enabled
 * @param enabled True to enable, false to disable
 */
void lcb_log_set_debug(bool enabled);

/**
 * @brief Set error logging enabled
 * @param enabled True to enable, false to disable
 */
void lcb_log_set_error(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* LCB_COMM_H */
