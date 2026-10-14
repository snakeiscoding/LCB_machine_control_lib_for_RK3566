/**
 * @file lcb_constants.c
 * @brief LCB Protocol Constants Implementation
 *
 * Converted from: com/jht/lcbcommlib/Constants.java
 *
 * This file provides utility functions for working with LCB protocol constants.
 */

#include "lcb_constants.h"
#include "lcb_commands.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Get status bit name string
 */
const char* lcb_status_bit_get_name(lcb_status_bit_t bit)
{
    switch (bit) {
        case LCB_STATUS_BIT_INCLINE_DOWN_ACTION:
            return "INCLINE_DOWN_ACTION";
        case LCB_STATUS_BIT_INCLINE_UP_ACTION:
            return "INCLINE_UP_ACTION";
        case LCB_STATUS_BIT_MAIN_MOTOR_ACTION:
            return "MAIN_MOTOR_ACTION";
        case LCB_STATUS_BIT_POWER_STATUS_AC:
            return "POWER_STATUS_AC";
        case LCB_STATUS_BIT_BATTERY_CAP_NOT_ENOUGH:
            return "BATTERY_CAP_NOT_ENOUGH";
        case LCB_STATUS_BIT_COMMAND_ERROR:
            return "COMMAND_ERROR";
        case LCB_STATUS_BIT_LCB_ERROR:
            return "LCB_ERROR";
        case LCB_STATUS_BIT_CALIBRATION_ACTION:
            return "CALIBRATION_ACTION";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Get expected receive data size for a command
 */
int lcb_get_expected_data_size(uint8_t command_id)
{
    switch (command_id) {
        /* 0x70-0x7F: System Commands */
        case 0x70: /* SET_INITIALIZE */
            return 0;
        case 0x71: /* GET_STATUS */
            return 0;
        case 0x72: /* GET_ERROR_CODE */
            return LCB_RECEIVE_DATA_SIZE_ERROR_CODE;
        case 0x73: /* GET_VERSION */
            return LCB_RECEIVE_DATA_SIZE_VERSION;
        case 0x74: /* SET_CALIBRATE */
            return 0;
        case 0x75: /* SET_UPDATE_PROGRAM */
            return LCB_RECEIVE_DATA_SIZE_UPDATE_PROGRAM;
        case 0x76: /* SET_SKIP_CURR_ERROR */
            return 0;
        case 0x77: /* SET_SPE_EXT_CMD */
            return LCB_RECEIVE_DATA_SIZE_SPE_EXT_CMD;
        case 0x78: /* GET_DCI_VERSION */
            return LCB_RECEIVE_DATA_SIZE_DCI_VERSION;
        case 0x79: /* GET_DCI_ENV */
            return LCB_RECEIVE_DATA_SIZE_DCI_ENV;
        case 0x7A: /* SET_CMD_EUPS_MODE */
            return LCB_RECEIVE_DATA_SIZE_CMD_EUPS_MODE;
        case 0x7B: /* SET_CMD_CONSOLE_POWER */
            return 0;
        case 0x7C: /* GET_PROTOCOL_VERSION */
            return LCB_RECEIVE_DATA_SIZE_PROTOCOL_VERSION;
        case 0x7D: /* SET_CMD_SERIAL_NUMBER */
            return -1; /* Variable length */
        case 0x7E: /* SET_TREADMILL_OPERATE */
            return -1; /* Variable length */

        /* 0xF0-0xFF: Treadmill Motor Commands */
        case 0xF0: /* SET_MOTOR_SPEED_RTN_RPM */
            return LCB_RECEIVE_DATA_SIZE_MOTOR_SPEED_RTN_RPM;
        case 0xF1: /* SET_MOTOR_RPM */
            return 0;
        case 0xF2: /* SET_INC_PWM_STEP */
            return 0;
        case 0xF3: /* SET_DEC_PWM_STEP */
            return 0;
        case 0xF4: /* SET_STOP_PWM_STEP */
            return 0;
        case 0xF5: /* SET_INCLINE_ACTION */
            return 0;
        case 0xF6: /* SET_INCLINE_LOC */
            return 0;
        case 0xF7: /* SET_OPERATING_STATUS */
            return 0;
        case 0xF8: /* GET_ROLLER_RPM */
            return LCB_RECEIVE_DATA_SIZE_ROLLER_RPM;
        case 0xF9: /* GET_MOTOR_RPM */
            return LCB_RECEIVE_DATA_SIZE_ROLLER_RPM;
        case 0xFA: /* GET_INCLINE_LOC */
            return LCB_RECEIVE_DATA_SIZE_INCLINE_LOC;
        case 0xFB: /* SET_TUNE_END_INCLINE */
            return 0;
        case 0xFC: /* SET_TUNE_END_INCLINE2 */
            return 0;
        case 0xFD: /* SET_INCLINE_STROKE */
            return 0;
        case 0xFE: /* SET_COMPENSATION_VOL */
            return LCB_RECEIVE_DATA_SIZE_COMPENSATION_VOL;

        /* 0x90-0x9F: Motor Commands */
        case 0x90: /* SET_MOTOR_SPECIFICATION */
            return 0;
        case 0x91: /* GET_MOTOR_TYPE */
            return LCB_RECEIVE_DATA_SIZE_MOTOR_TYPE;
        case 0x92: /* GET_DCI_DATA_MAX_LEN */
            return LCB_RECEIVE_DATA_SIZE_DCI_DATA_MAX_LEN;
        case 0x93: /* SET_MOTOR_POWER_OFF */
            return 0;
        case 0x94: /* SET_DCI_FORCE_INCLINE */
            return 0;
        case 0x95: /* GET_TREADMILL_IN_USED */
            return LCB_RECEIVE_DATA_SIZE_TREADMILL_IN_USED;
        case 0x96: /* GET_MAIN_MOTOR_INFO */
            return LCB_RECEIVE_DATA_SIZE_MAIN_MOTOR_INFO;
        case 0x97: /* GET_TREADMILL_EXT_STATUS */
            return LCB_RECEIVE_DATA_SIZE_TREADMILL_EXT_STATUS;
        case 0x98: /* SET_TREADMILL_ECB_ACTION */
            return LCB_RECEIVE_DATA_SIZE_TREADMILL_ECB_ACTION;

        /* 0xA0-0xAF: DCI Commands */
        case 0xA0: /* GET_DCI_INCLINE_RANGE */
            return LCB_RECEIVE_DATA_SIZE_DCI_INCLINE_RANGE;
        case 0xA1: /* SET_DCI_INCLINE_RANGE */
            return 0;
        case 0xA6: /* GET_DCI_RPM_SPEED_RANGE */
            return LCB_RECEIVE_DATA_SIZE_DCI_RPM_SPEED_RANGE;
        case 0xA7: /* SET_DCI_RPM_SPEED_RANGE */
            return 0;
        case 0xAA: /* SET_UNIVERSAL */
            return LCB_RECEIVE_DATA_SIZE_UNIVERSAL;
        case 0xAB: /* SET_DCI_MANUAL_CALIBRATION */
            return 0;

        /* 0x50-0x5F: Device Data Commands */
        case 0x50: /* SET_DEVICE_DATA */
            return -1; /* Variable length */
        case 0x51: /* GET_INDOOR_BIKE_SENSOR_DATA */
            return LCB_RECEIVE_DATA_SIZE_INDOORBIKE_SENSORDATA;
        case 0x52: /* SET_INDOOR_BIKE_STEPPER_MOTOR_POSITION */
            return 0;
        case 0x53: /* SET_INDOOR_BIKE_CALC_PARA_OF_DEVICE */
            return 0;
        case 0x54: /* GET_INDOOR_BIKE_CALC_PARA_OF_DEVICE */
            return LCB_RECEIVE_DATA_SIZE_INDOORBIKE_CALC_PARA_OF_DEVICE;
        case 0x55: /* SET_INDOOR_BIKE_ACTION_SPIN_DOWN */
            return LCB_RECEIVE_DATA_SIZE_INDOORBIKE_ACTION_SPIN_DOWN;
        case 0x56: /* GET_INDOOR_BIKE_INFO_SPIN_DOWN */
            return LCB_RECEIVE_DATA_SIZE_INDOORBIKE_INFO_SPIN_DOWN;
        case 0x57: /* SET_INDOOR_BIKE_ERG_MODE_SETTINGS */
            return -1; /* Variable length */
        case 0x58: /* SET_INDOOR_BIKE_RESISTANCE_LEVEL */
            return 0;
        case 0x59: /* GET_INDOOR_BIKE_ERG_MODE_SETTINGS */
            return LCB_RECEIVE_DATA_SIZE_INDOORBIKE_ERG_MODE_SETTINGS;

        /* 0x60-0x6F: ECB Commands */
        case 0x61: /* SET_RESET_ECB */
            return 0;
        case 0x62: /* SET_ECB_POSITION */
            return 0;
        case 0x63: /* GET_ECB_RPM */
            return LCB_RECEIVE_DATA_SIZE_ECB_RPM;
        case 0x64: /* GET_ECB_ADC */
            return LCB_RECEIVE_DATA_SIZE_ECB_ADC;
        case 0x65: /* SET_PWM_PERCENT */
            return 0;
        case 0x66: /* GET_BATTERY_VOLTAGE */
            return LCB_RECEIVE_DATA_SIZE_BATTERY_VOLTAGE;
        case 0x67: /* SET_WATTS */
            return 0;
        case 0x68: /* SET_RPM_GEAR_RATIO */
            return 0;
        case 0x69: /* SET_GENERATOR_POLE_PAIR */
            return 0;
        case 0x6A: /* SET_LIMIT_RPM_CHARGE */
            return 0;
        case 0x6B: /* SET_LIMIT_RPM_RESISTANCE */
            return 0;
        case 0x6C: /* SET_MACHINE_TYPE */
            return 0;
        case 0x6D: /* SET_POWER_OFF */
            return 0;
        case 0x6E: /* SET_BATTERY_CHARGE_LEVEL */
            return 0;
        case 0x6F: /* SET_MIN_RESISTANCE_INCLINE_WORKING */
            return 0;

        /* 0x80-0x8F: Bike Commands */
        case 0x80: /* SET_INCLINE_PERCENT */
            return 0;
        case 0x81: /* GET_INCLINE_PERCENT */
            return LCB_RECEIVE_DATA_SIZE_INCLINE_PERCENT;
        case 0x82: /* SET_ECB_ACTION */
            return 0;
        case 0x83: /* GET_ECB_STATUS */
            return LCB_RECEIVE_DATA_SIZE_ECB_STATUS;
        case 0x84: /* GET_ECB_COUNT */
            return LCB_RECEIVE_DATA_SIZE_ECB_COUNT;
        case 0x85: /* SET_ELECTROMAGNETIC_CURRENT */
            return 0;
        case 0x86: /* SET_RESISTANCE_TYPE */
            return 0;
        case 0x87: /* SET_BEGIN_BATTERY_CHARGE */
            return 0;
        case 0x88: /* GET_BATTERY_CAPACITY */
            return LCB_RECEIVE_DATA_SIZE_BATTERY_CAPACITY;
        case 0x8A: /* SET_CLIMBMILL_STATUS */
            return 0;
        case 0x8B: /* GET_CLIMBMILL_STATUS */
            return LCB_RECEIVE_DATA_SIZE_ENCLIMBMILL_STATUS;
        case 0x8C: /* SET_E_STOP_PARAMETER */
            return 0;
        case 0x8D: /* GET_BRAKE_PRESSURE_VALUE */
            return LCB_RECEIVE_DATA_SIZE_BRAKE_PRESSURE_VALUE;
        case 0x8E: /* GET_WATTS */
            return LCB_RECEIVE_DATA_SIZE_WATTS;
        case 0x8F: /* SET_CLIMBMILL_CAL */
            return 0;

        /* 0x40-0x4F: Calibration Commands */
        case 0x47: /* SET_RECORD_CURRENT */
            return 0;
        case 0x48: /* SET_PWM */
            return 0;
        case 0x49: /* SET_FINISH_CALIBRATING */
            return 0;
        case 0x4A: /* SET_SPM */
            return 0;
        case 0x4B: /* GET_SPM */
            return LCB_RECEIVE_DATA_SIZE_SPM;
        case 0x4C: /* SET_CAL_SCALING */
            return 0;
        case 0x4D: /* GET_CAL_SCALING */
            return LCB_RECEIVE_DATA_SIZE_CAL_SCALING;
        case 0x4E: /* SET_FRAME_CAL_SCALING */
            return 0;
        case 0x4F: /* GET_FRAME_CAL_SCALING */
            return LCB_RECEIVE_DATA_SIZE_FRAME_CAL_SCALING;

        default:
            return -1; /* Unknown command */
    }
}

/**
 * @brief Validate received data size
 */
int lcb_validate_data_size(uint8_t command_id, uint8_t received_size)
{
    int expected_size = lcb_get_expected_data_size(command_id);

    if (expected_size < 0) {
        /* Variable length or unknown command - cannot validate */
        return 1;
    }

    return (received_size == expected_size);
}

/**
 * @brief Format status byte as string
 */
int lcb_status_to_string(uint8_t status, char *buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size == 0) {
        return -1;
    }

    int pos = 0;
    int written;

    /* Add opening bracket */
    written = snprintf(buffer + pos, buffer_size - pos, "Status[0x%02X]: ", status);
    if (written < 0 || written >= (int)(buffer_size - pos)) {
        return -1;
    }
    pos += written;

    /* Check each status bit */
    int first = 1;

    if (status & LCB_STATUS_BIT_INCLINE_DOWN_ACTION) {
        written = snprintf(buffer + pos, buffer_size - pos, "%sINCLINE_DOWN", first ? "" : "|");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
        first = 0;
    }

    if (status & LCB_STATUS_BIT_INCLINE_UP_ACTION) {
        written = snprintf(buffer + pos, buffer_size - pos, "%sINCLINE_UP", first ? "" : "|");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
        first = 0;
    }

    if (status & LCB_STATUS_BIT_MAIN_MOTOR_ACTION) {
        written = snprintf(buffer + pos, buffer_size - pos, "%sMOTOR_ON", first ? "" : "|");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
        first = 0;
    }

    if (status & LCB_STATUS_BIT_POWER_STATUS_AC) {
        written = snprintf(buffer + pos, buffer_size - pos, "%sAC_POWER", first ? "" : "|");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
        first = 0;
    }

    if (status & LCB_STATUS_BIT_BATTERY_CAP_NOT_ENOUGH) {
        written = snprintf(buffer + pos, buffer_size - pos, "%sLOW_BATTERY", first ? "" : "|");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
        first = 0;
    }

    if (status & LCB_STATUS_BIT_COMMAND_ERROR) {
        written = snprintf(buffer + pos, buffer_size - pos, "%sCMD_ERROR", first ? "" : "|");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
        first = 0;
    }

    if (status & LCB_STATUS_BIT_LCB_ERROR) {
        written = snprintf(buffer + pos, buffer_size - pos, "%sLCB_ERROR", first ? "" : "|");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
        first = 0;
    }

    if (status & LCB_STATUS_BIT_CALIBRATION_ACTION) {
        written = snprintf(buffer + pos, buffer_size - pos, "%sCALIBRATING", first ? "" : "|");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
        first = 0;
    }

    if (first) {
        /* No bits set */
        written = snprintf(buffer + pos, buffer_size - pos, "NONE");
        if (written < 0 || written >= (int)(buffer_size - pos)) return -1;
        pos += written;
    }

    return pos;
}
