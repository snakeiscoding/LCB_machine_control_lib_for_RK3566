/**
 * @file lcb_commands.h
 * @brief LCB Protocol Commands Definition
 *
 * Converted from: com/jht/lcbcommlib/LcbCommands.java
 */

#ifndef LCB_COMMANDS_H
#define LCB_COMMANDS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief LCB Protocol Command IDs
 */
typedef enum {
    LCB_CMD_SET_INITIALIZE                  = 0x70,
    LCB_CMD_GET_STATUS                      = 0x71,
    LCB_CMD_GET_ERROR_CODE                  = 0x72,
    LCB_CMD_GET_VERSION                     = 0x73,
    LCB_CMD_SET_CALIBRATE                   = 0x74,
    LCB_CMD_SET_UPDATE_PROGRAM              = 0x75,
    LCB_CMD_SET_SKIP_CURR_ERROR             = 0x76,
    LCB_CMD_SET_SPE_EXT_CMD                 = 0x77,
    LCB_CMD_GET_DCI_VERSION                 = 0x78,
    LCB_CMD_GET_DCI_ENV                     = 0x79,
    LCB_CMD_SET_CMD_EUPS_MODE               = 0x7A,
    LCB_CMD_SET_CMD_CONSOLE_POWER           = 0x7B,
    LCB_CMD_GET_PROTOCOL_VERSION            = 0x7C,
    LCB_CMD_SET_CMD_SERIAL_NUMBER           = 0x7D,
    LCB_CMD_SET_TREADMILL_OPERATE           = 0x7E,
    LCB_CMD_GET_TREADMILL_CADENCE           = 0x7F,

    LCB_CMD_SET_MOTOR_SPEED_RTN_RPM         = 0xF0,
    LCB_CMD_SET_MOTOR_RPM                   = 0xF1,
    LCB_CMD_SET_INC_PWM_STEP                = 0xF2,
    LCB_CMD_SET_DEC_PWM_STEP                = 0xF3,
    LCB_CMD_SET_STOP_PWM_STEP               = 0xF4,
    LCB_CMD_SET_INCLINE_ACTION              = 0xF5,
    LCB_CMD_SET_INCLINE_LOC                 = 0xF6,
    LCB_CMD_SET_OPERATING_STATUS            = 0xF7,
    LCB_CMD_GET_ROLLER_RPM                  = 0xF8,
    LCB_CMD_GET_MOTOR_RPM                   = 0xF9,
    LCB_CMD_GET_INCLINE_LOC                 = 0xFA,
    LCB_CMD_SET_TUNE_END_INCLINE            = 0xFB,
    LCB_CMD_SET_TUNE_END_INCLINE2           = 0xFC,
    LCB_CMD_SET_INCLINE_STROKE              = 0xFD,
    LCB_CMD_SET_COMPENSATION_VOL            = 0xFE,

    LCB_CMD_SET_MOTOR_SPECIFICATION         = 0x90,
    LCB_CMD_GET_MOTOR_TYPE                  = 0x91,
    LCB_CMD_GET_DCI_DATA_MAX_LEN            = 0x92,
    LCB_CMD_SET_MOTOR_POWER_OFF             = 0x93,
    LCB_CMD_SET_DCI_FORCE_INCLINE           = 0x94,
    LCB_CMD_GET_TREADMILL_IN_USED           = 0x95,
    LCB_CMD_GET_MAIN_MOTOR_INFO             = 0x96,
    LCB_CMD_GET_TREADMILL_EXT_STATUS        = 0x97,
    LCB_CMD_SET_TREADMILL_ECB_ACTION        = 0x98,
    /* IR Address board (0x26) only — "Hybrid TM" IR beam E-Stop sensor.
     * GetLCBVersion (0x73) and SetLCBCmdEUPsMode (0x7A) above are reused for
     * this board too, at the different bus address; only this opcode is
     * IR-board-specific. */
    LCB_CMD_GET_IR_ADDRESS_IR_STATUS        = 0x99,

    LCB_CMD_GET_DCI_INCLINE_RANGE           = 0xA0,
    LCB_CMD_SET_DCI_INCLINE_RANGE           = 0xA1,
    LCB_CMD_GET_DCI_RPM_SPEED_RANGE         = 0xA6,
    LCB_CMD_SET_DCI_RPM_SPEED_RANGE         = 0xA7,
    LCB_CMD_SET_UNIVERSAL                   = 0xAA,
    LCB_CMD_SET_DCI_MANUAL_CALIBRATION      = 0xAB,

    LCB_CMD_SET_DEVICE_DATA                 = 0x50,
    LCB_CMD_GET_INDOOR_BIKE_SENSOR_DATA     = 0x51,
    LCB_CMD_SET_INDOOR_BIKE_STEPPER_MOTOR_POSITION = 0x52,
    LCB_CMD_SET_INDOOR_BIKE_CALC_PARA_OF_DEVICE = 0x53,
    LCB_CMD_GET_INDOOR_BIKE_CALC_PARA_OF_DEVICE = 0x54,
    LCB_CMD_SET_INDOOR_BIKE_ACTION_SPIN_DOWN = 0x55,
    LCB_CMD_GET_INDOOR_BIKE_INFO_SPIN_DOWN  = 0x56,
    LCB_CMD_SET_INDOOR_BIKE_ERG_MODE_SETTINGS = 0x57,
    LCB_CMD_SET_INDOOR_BIKE_RESISTANCE_LEVEL = 0x58,
    LCB_CMD_GET_INDOOR_BIKE_ERG_MODE_SETTINGS = 0x59,

    LCB_CMD_SET_RESET_ECB                   = 0x61,
    LCB_CMD_SET_ECB_POSITION                = 0x62,
    LCB_CMD_GET_ECB_RPM                     = 0x63,
    LCB_CMD_GET_ECB_ADC                     = 0x64,
    LCB_CMD_SET_PWM_PERCENT                 = 0x65,
    LCB_CMD_GET_BATTERY_VOLTAGE             = 0x66,
    LCB_CMD_SET_WATTS                       = 0x67,
    LCB_CMD_SET_RPM_GEAR_RATIO              = 0x68,
    LCB_CMD_SET_GENERATOR_POLE_PAIR         = 0x69,
    LCB_CMD_SET_LIMIT_RPM_CHARGE            = 0x6A,
    LCB_CMD_SET_LIMIT_RPM_RESISTANCE        = 0x6B,
    LCB_CMD_SET_MACHINE_TYPE                = 0x6C,
    LCB_CMD_SET_POWER_OFF                   = 0x6D,
    LCB_CMD_SET_BATTERY_CHARGE_LEVEL        = 0x6E,
    LCB_CMD_SET_MIN_RESISTANCE_INCLINE_WORKING = 0x6F,

    LCB_CMD_SET_INCLINE_PERCENT             = 0x80,
    LCB_CMD_GET_INCLINE_PERCENT             = 0x81,
    LCB_CMD_SET_ECB_ACTION                  = 0x82,
    LCB_CMD_GET_ECB_STATUS                  = 0x83,
    LCB_CMD_GET_ECB_COUNT                   = 0x84,
    LCB_CMD_SET_ELECTROMAGNETIC_CURRENT     = 0x85,
    LCB_CMD_SET_RESISTANCE_TYPE             = 0x86,
    LCB_CMD_SET_BEGIN_BATTERY_CHARGE        = 0x87,
    LCB_CMD_GET_BATTERY_CAPACITY            = 0x88,
    LCB_CMD_SET_CLIMBMILL_STATUS            = 0x8A,
    LCB_CMD_GET_CLIMBMILL_STATUS            = 0x8B,
    LCB_CMD_SET_E_STOP_PARAMETER            = 0x8C,
    LCB_CMD_GET_BRAKE_PRESSURE_VALUE        = 0x8D,
    LCB_CMD_GET_WATTS                       = 0x8E,
    LCB_CMD_SET_CLIMBMILL_CAL               = 0x8F,

    LCB_CMD_SET_RECORD_CURRENT              = 0x47,
    LCB_CMD_SET_PWM                         = 0x48,
    LCB_CMD_SET_FINISH_CALIBRATING          = 0x49,
    LCB_CMD_SET_SPM                         = 0x4A,
    LCB_CMD_GET_SPM                         = 0x4B,
    LCB_CMD_SET_CAL_SCALING                 = 0x4C,
    LCB_CMD_GET_CAL_SCALING                 = 0x4D,
    LCB_CMD_SET_FRAME_CAL_SCALING           = 0x4E,
    LCB_CMD_GET_FRAME_CAL_SCALING           = 0x4F
} lcb_command_t;

/**
 * @brief Check if a command ID is valid
 * @param cmd Command ID to validate
 * @return 1 if valid, 0 otherwise
 */
int lcb_command_is_valid(uint8_t cmd);

/**
 * @brief Get command name string (for debugging)
 * @param cmd Command ID
 * @return Command name string or "UNKNOWN"
 */
const char* lcb_command_get_name(lcb_command_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* LCB_COMMANDS_H */
