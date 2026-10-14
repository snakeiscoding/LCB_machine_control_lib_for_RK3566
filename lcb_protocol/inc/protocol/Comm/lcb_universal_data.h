/**
 * @file lcb_universal_data.h
 * @brief LCB Universal Data Sub-Command Interface (Command 0xAA)
 *
 * Converted from:
 *   - com/jht/lcbcommlib/UniSubCommands.java
 *   - com/jht/lcbcommlib/UniDataConstants.java
 *   - com/jht/lcbcommlib/IUniDataReceived.java
 *   - com/jht/lcbcommlib/IUniDataTransmitted.java
 *
 * Command 0xAA carries a sub-command byte as the first data byte.
 * This module dispatches received data to per-sub-command callbacks (RX)
 * and provides helper functions to build sub-command packets (TX).
 */

#ifndef LCB_UNIVERSAL_DATA_H
#define LCB_UNIVERSAL_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Sub-Command Enum (from UniSubCommands.java)
 * ============================================================================ */

typedef enum {
    LCB_UD_CMD_GET_MOTO_TYPE                = 0x03,
    LCB_UD_CMD_LOCK_MOTO_PROTECT            = 0x11,
    LCB_UD_CMD_MOTO_RATE_SLIP               = 0x12,
    LCB_UD_CMD_MOTO_RATE_CURRENT            = 0x13,
    LCB_UD_CMD_RATE_VOL                     = 0x14,
    LCB_UD_CMD_RATE_FREQ                    = 0x15,
    LCB_UD_CMD_COMPENSATION_MOTO_RES        = 0x16,
    LCB_UD_CMD_OVER_CURRENT_DET_TIME        = 0x17,
    LCB_UD_CMD_OVER_LOAD_DET_TIME           = 0x18,
    LCB_UD_CMD_OVER_VOL_DET_TIME            = 0x19,
    LCB_UD_CMD_MOTO_WIRE_BREAK_DET_TIME     = 0x1A,
    LCB_UD_CMD_INCLINE_DIR_DEF              = 0x1B,
    LCB_UD_CMD_INCLINE_SEN_ERR_DET_TIME     = 0x1C,
    LCB_UD_CMD_INCLINE_ERR_DET_TIME         = 0x1D,
    LCB_UD_CMD_OVER_CURRENT_COUNT_H         = 0x1E,
    LCB_UD_CMD_OVER_CURRENT_COUNT_L         = 0x1F,
    LCB_UD_CMD_OVER_CURRENT_H               = 0x20,
    LCB_UD_CMD_OVER_CURRENT_L               = 0x21,
    LCB_UD_CMD_GET_CASTER_SWITCH_STATUS     = 0x22,
    LCB_UD_CMD_SET_PFC_MODE                 = 0x23,
    LCB_UD_CMD_MAGNET_ANGLE_CAL             = 0x24,
    LCB_UD_CMD_MCB_VOLTAGE                  = 0x25
} lcb_universal_data_cmd_t;

/* ============================================================================
 * Send / Receive Data Size Constants (from UniDataConstants.java)
 * ============================================================================ */

/* Send data sizes (total bytes including sub-command byte) */
#define LCB_UD_SEND_SIZE_CMD_03            1   /* GetMotoType */
#define LCB_UD_SEND_SIZE_CMD_11            2   /* LockMotoProtect */
#define LCB_UD_SEND_SIZE_CMD_12            2   /* MotoRateSlip */
#define LCB_UD_SEND_SIZE_CMD_13            2   /* MotoRateCurrent */
#define LCB_UD_SEND_SIZE_CMD_14            2   /* RateVol */
#define LCB_UD_SEND_SIZE_CMD_15            2   /* RateFreq */
#define LCB_UD_SEND_SIZE_CMD_16            2   /* CompensationMotoRes */
#define LCB_UD_SEND_SIZE_CMD_17            3   /* OverCurrentDetTime */
#define LCB_UD_SEND_SIZE_CMD_18            2   /* OverLoadDetTime */
#define LCB_UD_SEND_SIZE_CMD_19            2   /* OverVolDetTime */
#define LCB_UD_SEND_SIZE_CMD_1A            2   /* MotoWireBreakDetTime */
#define LCB_UD_SEND_SIZE_CMD_1B            2   /* InclineDirDef */
#define LCB_UD_SEND_SIZE_CMD_1C            2   /* InclineSenErrDetTime */
#define LCB_UD_SEND_SIZE_CMD_1D            2   /* InclineErrDetTime */
#define LCB_UD_SEND_SIZE_CMD_1E            2   /* OverCurrentCountH */
#define LCB_UD_SEND_SIZE_CMD_1F            2   /* OverCurrentCountL */
#define LCB_UD_SEND_SIZE_CMD_20            2   /* OverCurrentH */
#define LCB_UD_SEND_SIZE_CMD_21            2   /* OverCurrentL */
#define LCB_UD_SEND_SIZE_CMD_22            2   /* GetCasterSwitchStatus */
#define LCB_UD_SEND_SIZE_CMD_23            3   /* SetPFCmode */
#define LCB_UD_SEND_SET_SIZE_CMD_24        3   /* SetMagnetAngle */
#define LCB_UD_SEND_GET_SIZE_CMD_24        2   /* GetMagnetAngle */
#define LCB_UD_SEND_SET_SIZE_CMD_25        3   /* SetMcbVoltage */
#define LCB_UD_SEND_GET_SIZE_CMD_25        2   /* GetMcbVoltage */

/* Receive data sizes (total bytes including sub-command byte) */
#define LCB_UD_RECV_SIZE_CMD_03            3   /* GetMotoType */
#define LCB_UD_RECV_SIZE_CMD_11            3   /* LockMotoProtect */
#define LCB_UD_RECV_SIZE_CMD_12            3   /* MotoRateSlip */
#define LCB_UD_RECV_SIZE_CMD_13            3   /* MotoRateCurrent */
#define LCB_UD_RECV_SIZE_CMD_14            3   /* RateVol */
#define LCB_UD_RECV_SIZE_CMD_15            3   /* RateFreq */
#define LCB_UD_RECV_SIZE_CMD_16            3   /* CompensationMotoRes */
#define LCB_UD_RECV_SIZE_CMD_17            3   /* OverCurrentDetTime */
#define LCB_UD_RECV_SIZE_CMD_18            3   /* OverLoadDetTime */
#define LCB_UD_RECV_SIZE_CMD_19            3   /* OverVolDetTime */
#define LCB_UD_RECV_SIZE_CMD_1A            3   /* MotoWireBreakDetTime */
#define LCB_UD_RECV_SIZE_CMD_1B            3   /* InclineDirDef */
#define LCB_UD_RECV_SIZE_CMD_1C            3   /* InclineSenErrDetTime */
#define LCB_UD_RECV_SIZE_CMD_1D            3   /* InclineErrDetTime */
#define LCB_UD_RECV_SIZE_CMD_1E            3   /* OverCurrentCountH */
#define LCB_UD_RECV_SIZE_CMD_1F            3   /* OverCurrentCountL */
#define LCB_UD_RECV_SIZE_CMD_20            3   /* OverCurrentH */
#define LCB_UD_RECV_SIZE_CMD_21            3   /* OverCurrentL */
#define LCB_UD_RECV_SIZE_CMD_22            3   /* GetCasterSwitchStatus */
#define LCB_UD_RECV_SIZE_CMD_23            4   /* SetPFCmode */
/* 0x24 MagnetAngleCal: variable length (no fixed recv size) */
/* 0x25 McbVoltage: variable length (no fixed recv size) */

/* ============================================================================
 * Callback Structure (from IUniDataReceived.java)
 * ============================================================================ */

/**
 * @brief Universal Data sub-command receive callback structure
 *
 * All callback function pointers are optional (NULL = not registered).
 * The user_data pointer is passed as the first argument to every callback.
 */
typedef struct lcb_universal_data_callbacks {
    void *user_data;

    /* Sub-Command 0x03: GetMotoType */
    void (*on_ud_get_moto_type)(void *user_data, int moto_type);

    /* Sub-Command 0x11: LockMotoProtect */
    void (*on_ud_lock_moto_protect)(void *user_data, int value);

    /* Sub-Command 0x12: MotoRateSlip */
    void (*on_ud_moto_rate_slip)(void *user_data, int value);

    /* Sub-Command 0x13: MotoRateCurrent */
    void (*on_ud_moto_rate_current)(void *user_data, int value);

    /* Sub-Command 0x14: RateVol */
    void (*on_ud_rate_vol)(void *user_data, int value);

    /* Sub-Command 0x15: RateFreq */
    void (*on_ud_rate_freq)(void *user_data, int value);

    /* Sub-Command 0x16: CompensationMotoRes */
    void (*on_ud_compensation_moto_res)(void *user_data, int value);

    /* Sub-Command 0x17: OverCurrentDetTime */
    void (*on_ud_over_current_det_time)(void *user_data, int value);

    /* Sub-Command 0x18: OverLoadDetTime */
    void (*on_ud_over_load_det_time)(void *user_data, int value);

    /* Sub-Command 0x19: OverVolDetTime */
    void (*on_ud_over_vol_det_time)(void *user_data, int value);

    /* Sub-Command 0x1A: MotoWireBreakDetTime */
    void (*on_ud_moto_wire_break_det_time)(void *user_data, int value);

    /* Sub-Command 0x1B: InclineDirDef */
    void (*on_ud_incline_dir_def)(void *user_data, int value);

    /* Sub-Command 0x1C: InclineSenErrDetTime */
    void (*on_ud_incline_sen_err_det_time)(void *user_data, int value);

    /* Sub-Command 0x1D: InclineErrDetTime */
    void (*on_ud_incline_err_det_time)(void *user_data, int value);

    /* Sub-Command 0x1E: OverCurrentCountH */
    void (*on_ud_over_current_count_h)(void *user_data, int value);

    /* Sub-Command 0x1F: OverCurrentCountL */
    void (*on_ud_over_current_count_l)(void *user_data, int value);

    /* Sub-Command 0x20: OverCurrentH */
    void (*on_ud_over_current_h)(void *user_data, int value);

    /* Sub-Command 0x21: OverCurrentL */
    void (*on_ud_over_current_l)(void *user_data, int value);

    /* Sub-Command 0x22: GetCasterSwitchStatus */
    void (*on_ud_get_caster_switch_status)(void *user_data, int value);

    /* Sub-Command 0x23: SetPFCmode */
    void (*on_ud_set_pfc_mode)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x24: MagnetAngleCal */
    void (*on_ud_magnet_angle_state)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x25: McbVoltage */
    void (*on_ud_mcb_voltage)(void *user_data, const uint8_t *data, uint8_t length);

} lcb_universal_data_callbacks_t;

/* ============================================================================
 * RX Functions (implemented in lcb_universal_data_proc.c)
 * ============================================================================ */

/**
 * @brief Initialize universal data callbacks structure with NULL values
 * @param cb Pointer to callback structure
 */
void lcb_universal_data_callbacks_init(lcb_universal_data_callbacks_t *cb);

/**
 * @brief Analyze received universal data and dispatch to sub-command handlers
 *
 * This is the C equivalent of UniDataProcReceived.analyzeUniData().
 * The first byte of data is the sub-command ID.
 *
 * @param cb Pointer to universal data callback structure
 * @param data Raw universal data (first byte = sub-command)
 * @param length Total data length
 */
void lcb_universal_data_analyze(const lcb_universal_data_callbacks_t *cb,
                                const uint8_t *data, uint8_t length);

/* ============================================================================
 * TX Functions (implemented in lcb_universal_data_tx.c)
 *
 * Each function builds a sub-command packet and sends it via
 * lcb_comm_set_lcb_universal() (command 0xAA).
 * ============================================================================ */

/* Sub-Command 0x03: GetMotoType -- send [0x03] */
void lcb_ud_get_moto_type(lcb_comm_t *comm);

/* Sub-Command 0x11: LockMotoProtect -- send [0x11, param] */
void lcb_ud_set_lock_moto_protect(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x12: MotoRateSlip -- send [0x12, param] */
void lcb_ud_set_moto_rate_slip(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x13: MotoRateCurrent -- send [0x13, param] */
void lcb_ud_set_moto_rate_current(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x14: RateVol -- send [0x14, param] */
void lcb_ud_set_rate_vol(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x15: RateFreq -- send [0x15, param] */
void lcb_ud_set_rate_freq(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x16: CompensationMotoRes -- send [0x16, param] */
void lcb_ud_set_compensation_moto_res(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x17: SetOverCurrentDetTime -- send [0x17, param_h, param_l] */
void lcb_ud_set_over_current_det_time(lcb_comm_t *comm, uint8_t param_h, uint8_t param_l);

/* Sub-Command 0x18: OverLoadDetTime -- send [0x18, param] */
void lcb_ud_set_over_load_det_time(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x19: OverVolDetTime -- send [0x19, param] */
void lcb_ud_set_over_vol_det_time(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x1A: MotoWireBreakDetTime -- send [0x1A, param] */
void lcb_ud_set_moto_wire_break_det_time(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x1B: SetInclineDirDef -- send [0x1B, param] */
void lcb_ud_set_incline_dir_def(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x1C: InclineSenErrDetTime -- send [0x1C, param] */
void lcb_ud_set_incline_sen_err_det_time(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x1D: InclineErrDetTime -- send [0x1D, param] */
void lcb_ud_set_incline_err_det_time(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x1E: OverCurrentCountH -- send [0x1E, param] */
void lcb_ud_set_over_current_count_h(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x1F: OverCurrentCountL -- send [0x1F, param] */
void lcb_ud_set_over_current_count_l(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x20: SetOverCurrentH -- send [0x20, param] */
void lcb_ud_set_over_current_h(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x21: OverCurrentL -- send [0x21, param] */
void lcb_ud_set_over_current_l(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x22: GetCasterSwitchStatus -- send [0x22, param] */
void lcb_ud_get_caster_switch_status(lcb_comm_t *comm, uint8_t param);

/* Sub-Command 0x23: SetPFCmode -- send [0x23, 0x01, state] */
void lcb_ud_set_pfc_mode(lcb_comm_t *comm, uint8_t state);

/* Sub-Command 0x24: SetMagnetAngleState -- send [0x24, 0x01, state] */
void lcb_ud_set_magnet_angle_state(lcb_comm_t *comm, uint8_t state);

/* Sub-Command 0x24: GetMagnetAngle -- send [0x24, 0x00] with queue mode */
void lcb_ud_get_magnet_angle(lcb_comm_t *comm, lcb_queue_mode_t mode);

/* Sub-Command 0x25: SetMcbVoltage -- send [0x25, 0x01, data] */
void lcb_ud_set_mcb_voltage(lcb_comm_t *comm, uint8_t voltage_data);

/* Sub-Command 0x25: GetMcbVoltage -- send [0x25, 0x00] with queue mode */
void lcb_ud_get_mcb_voltage(lcb_comm_t *comm, lcb_queue_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* LCB_UNIVERSAL_DATA_H */
