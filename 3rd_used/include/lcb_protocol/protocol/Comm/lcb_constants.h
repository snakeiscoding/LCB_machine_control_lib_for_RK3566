/**
 * @file lcb_constants.h
 * @brief LCB Protocol Constants Definition
 *
 * Converted from: com/jht/lcbcommlib/Constants.java
 */

#ifndef LCB_CONSTANTS_H
#define LCB_CONSTANTS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Receive data length limits */
#define LCB_RECEIVE_DATA_LENGTH_LIMITED_MAX         255

/* Receive data size definitions */
#define LCB_RECEIVE_DATA_SIZE_INFO                  2
#define LCB_RECEIVE_DATA_SIZE_ERROR_CODE            2
#define LCB_RECEIVE_DATA_SIZE_VERSION               2
#define LCB_RECEIVE_DATA_SIZE_ERP_MODE              1
#define LCB_RECEIVE_DATA_SIZE_UNKNOW_LCB_COMM       1
#define LCB_RECEIVE_DATA_SIZE_POS_SENSOR_CAL        4
#define LCB_RECEIVE_DATA_SIZE_ENTER_UPDATE_LCB_MODE 1
#define LCB_RECEIVE_DATA_SIZE_SENSORS_STATE         2
#define LCB_RECEIVE_DATA_SIZE_SET_CALIBRATE_SENSORS 1
#define LCB_RECEIVE_DATA_SIZE_UPDATE_PROGRAM        1
#define LCB_RECEIVE_DATA_SIZE_SPE_EXT_CMD           1
#define LCB_RECEIVE_DATA_SIZE_DCI_VERSION           27
#define LCB_RECEIVE_DATA_SIZE_DCI_ENV               84
#define LCB_RECEIVE_DATA_SIZE_CMD_EUPS_MODE         1
#define LCB_RECEIVE_DATA_SIZE_PROTOCOL_VERSION      2
#define LCB_RECEIVE_DATA_SIZE_MOTOR_SPEED_RTN_RPM   2
#define LCB_RECEIVE_DATA_SIZE_ROLLER_RPM            2
#define LCB_RECEIVE_DATA_SIZE_INCLINE_LOC           2
#define LCB_RECEIVE_DATA_SIZE_COMPENSATION_VOL      2
#define LCB_RECEIVE_DATA_SIZE_MOTOR_TYPE            1
#define LCB_RECEIVE_DATA_SIZE_DCI_DATA_MAX_LEN      1
#define LCB_RECEIVE_DATA_SIZE_TREADMILL_IN_USED     1
#define LCB_RECEIVE_DATA_SIZE_MAIN_MOTOR_INFO       12
#define LCB_RECEIVE_DATA_SIZE_TREADMILL_EXT_STATUS  2
#define LCB_RECEIVE_DATA_SIZE_DCI_INCLINE_RANGE     4
#define LCB_RECEIVE_DATA_SIZE_DCI_RPM_SPEED_RANGE   4
#define LCB_RECEIVE_DATA_SIZE_UNIVERSAL             3  /* cmd 1 byte + data 2 bytes */
#define LCB_RECEIVE_DATA_SIZE_ECB_RPM               2
#define LCB_RECEIVE_DATA_SIZE_ECB_ADC               2
#define LCB_RECEIVE_DATA_SIZE_BATTERY_VOLTAGE       2
#define LCB_RECEIVE_DATA_SIZE_INCLINE_PERCENT       2
#define LCB_RECEIVE_DATA_SIZE_ECB_STATUS            1
#define LCB_RECEIVE_DATA_SIZE_ECB_COUNT             2
#define LCB_RECEIVE_DATA_SIZE_BATTERY_CAPACITY      2
#define LCB_RECEIVE_DATA_SIZE_CLIMBMILL_STATUS      1
#define LCB_RECEIVE_DATA_SIZE_ENCLIMBMILL_STATUS    2
#define LCB_RECEIVE_DATA_SIZE_BRAKE_PRESSURE_VALUE  2
#define LCB_RECEIVE_DATA_SIZE_WATTS                 2
#define LCB_RECEIVE_DATA_SIZE_SPM                   2
#define LCB_RECEIVE_DATA_SIZE_CAL_SCALING           2
#define LCB_RECEIVE_DATA_SIZE_FRAME_CAL_SCALING     2
#define LCB_RECEIVE_DATA_SIZE_INDOORBIKE_SENSORDATA 19
/* TcvFxSpinBike reuses the same 0x51 command with a shorter, distinct payload layout */
#define LCB_RECEIVE_DATA_SIZE_TCV_FX_SPINBIKE_SENSORDATA 11
#define LCB_RECEIVE_DATA_SIZE_INDOORBIKE_CALC_PARA_OF_DEVICE   41
#define LCB_RECEIVE_DATA_SIZE_INDOORBIKE_ACTION_SPIN_DOWN      1
#define LCB_RECEIVE_DATA_SIZE_INDOORBIKE_INFO_SPIN_DOWN        15
#define LCB_RECEIVE_DATA_SIZE_INDOORBIKE_ERG_MODE_SETTINGS     88
#define LCB_RECEIVE_DATA_SIZE_TREADMILL_ECB_ACTION  1
/* 0x99 GetIrAddressIrStatus (IR Address board 0x26 only). The IR-board
 * variants of 0x73 GetLCBVersion and 0x7A SetLCBCmdEUPsMode reuse
 * LCB_RECEIVE_DATA_SIZE_VERSION and LCB_RECEIVE_DATA_SIZE_CMD_EUPS_MODE
 * above — same payload shape as the main LCB, just a different bus address. */
#define LCB_RECEIVE_DATA_SIZE_IR_ADDR_IR_STATUS     1

/* 0x73 GetLCBVersion return values */
#define LCB_RTN_HAVE_0X50_DATA                      0xFF00  /* LCB supports command 0x50/Data 0xF0 */

/* 0x7A CmdEUPsMode command values — also reused as-is (same byte values, same
 * meaning) for the IR Address board's enter/leave ERP mode request/response,
 * confirmed against chimera2 (IR_ADDR_ENTER_ERP=0xFF, IR_ADDR_LEAVE_ERP=0x00,
 * and the same 0x00/0x01/0xA1 response-byte meanings). */
#define LCB_CMD_EUPS_MODE_NOTIFY_ENTER_EUPS         0xFF    /* Notify LCB to enter EUPs mode */
#define LCB_CMD_EUPS_MODE_ASK_LEAVE_EUPS            0x00    /* Ask if LCB leaves EUPs mode */
#define LCB_CMD_EUPS_RTN_CAN_REQUISTION             0x01    /* LCB can implement UCB requisition */
#define LCB_CMD_EUPS_RTN_CAN_NOT_REQUISTION         0xA1    /* LCB cannot implement UCB requisition */
#define LCB_CMD_EUPS_RTN_NOT_SUPPORT                0x00    /* LCB does not support EUPs function */

/* 0xF7 SetLCBOperatingStatus values */
#define LCB_OPERATING_STOP                          0x00
#define LCB_OPERATING_ACTION                        0x01

/**
 * @brief LCB Status Bit Masks
 */
typedef enum {
    LCB_STATUS_BIT_INCLINE_DOWN_ACTION       = 0x01,  /* Bit 0 */
    LCB_STATUS_BIT_INCLINE_UP_ACTION         = 0x02,  /* Bit 1 */
    LCB_STATUS_BIT_MAIN_MOTOR_ACTION         = 0x04,  /* Bit 2 */
    LCB_STATUS_BIT_POWER_STATUS_AC           = 0x08,  /* Bit 3 */
    LCB_STATUS_BIT_BATTERY_CAP_NOT_ENOUGH    = 0x10,  /* Bit 4 */
    LCB_STATUS_BIT_COMMAND_ERROR             = 0x20,  /* Bit 5 */
    LCB_STATUS_BIT_LCB_ERROR                 = 0x40,  /* Bit 6 */
    LCB_STATUS_BIT_CALIBRATION_ACTION        = 0x80   /* Bit 7 */
} lcb_status_bit_t;

/**
 * @brief Check if a specific status bit is set
 * @param status LCB status byte
 * @param bit Bit mask to check
 * @return 1 if bit is set, 0 otherwise
 */
static inline int lcb_status_bit_is_set(uint8_t status, lcb_status_bit_t bit) {
    return (status & bit) != 0;
}

/**
 * @brief Get status bit name string (for debugging)
 * @param bit Status bit mask
 * @return Status bit name string
 */
const char* lcb_status_bit_get_name(lcb_status_bit_t bit);

/**
 * @brief Get receive data size for a command
 * @param command_id Command ID
 * @return Expected data size in bytes, or -1 if unknown
 */
int lcb_get_expected_data_size(uint8_t command_id);

/**
 * @brief Validate received data size
 * @param command_id Command ID
 * @param received_size Received data size
 * @return 1 if size is valid, 0 if invalid
 */
int lcb_validate_data_size(uint8_t command_id, uint8_t received_size);

/**
 * @brief Format status byte as string
 * @param status LCB status byte
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return Number of characters written
 */
int lcb_status_to_string(uint8_t status, char *buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* LCB_CONSTANTS_H */
