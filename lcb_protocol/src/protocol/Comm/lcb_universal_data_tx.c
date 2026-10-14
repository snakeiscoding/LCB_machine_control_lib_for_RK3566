/**
 * @file lcb_universal_data_tx.c
 * @brief LCB Universal Data TX Functions (Command 0xAA Sub-Commands)
 *
 * Converted from: com/jht/lcbcommlib/UniDataTransmitted.java
 *
 * Each function builds a sub-command data array and enqueues it via
 * lcb_comm_set_lcb_universal() (command 0xAA).
 */

#include "lcb_universal_data.h"
#include "lcb_logging.h"

#define TAG "LcbUniversalDataTx"

/* Sub-Command 0x03: GetMotoType -- send [0x03] */
void lcb_ud_get_moto_type(lcb_comm_t *comm)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_03];
    data[0] = LCB_UD_CMD_GET_MOTO_TYPE;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x11: LockMotoProtect -- send [0x11, param] */
void lcb_ud_set_lock_moto_protect(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_11];
    data[0] = LCB_UD_CMD_LOCK_MOTO_PROTECT;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x12: MotoRateSlip -- send [0x12, param] */
void lcb_ud_set_moto_rate_slip(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_12];
    data[0] = LCB_UD_CMD_MOTO_RATE_SLIP;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x13: MotoRateCurrent -- send [0x13, param] */
void lcb_ud_set_moto_rate_current(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_13];
    data[0] = LCB_UD_CMD_MOTO_RATE_CURRENT;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x14: RateVol -- send [0x14, param] */
void lcb_ud_set_rate_vol(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_14];
    data[0] = LCB_UD_CMD_RATE_VOL;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x15: RateFreq -- send [0x15, param] */
void lcb_ud_set_rate_freq(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_15];
    data[0] = LCB_UD_CMD_RATE_FREQ;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x16: CompensationMotoRes -- send [0x16, param] */
void lcb_ud_set_compensation_moto_res(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_16];
    data[0] = LCB_UD_CMD_COMPENSATION_MOTO_RES;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x17: SetOverCurrentDetTime -- send [0x17, param_h, param_l] */
void lcb_ud_set_over_current_det_time(lcb_comm_t *comm, uint8_t param_h, uint8_t param_l)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_17];
    data[0] = LCB_UD_CMD_OVER_CURRENT_DET_TIME;
    data[1] = param_h;
    data[2] = param_l;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x18: OverLoadDetTime -- send [0x18, param] */
void lcb_ud_set_over_load_det_time(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_18];
    data[0] = LCB_UD_CMD_OVER_LOAD_DET_TIME;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x19: OverVolDetTime -- send [0x19, param] */
void lcb_ud_set_over_vol_det_time(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_19];
    data[0] = LCB_UD_CMD_OVER_VOL_DET_TIME;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x1A: MotoWireBreakDetTime -- send [0x1A, param] */
void lcb_ud_set_moto_wire_break_det_time(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_1A];
    data[0] = LCB_UD_CMD_MOTO_WIRE_BREAK_DET_TIME;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x1B: SetInclineDirDef -- send [0x1B, param] */
void lcb_ud_set_incline_dir_def(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_1B];
    data[0] = LCB_UD_CMD_INCLINE_DIR_DEF;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x1C: InclineSenErrDetTime -- send [0x1C, param] */
void lcb_ud_set_incline_sen_err_det_time(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_1C];
    data[0] = LCB_UD_CMD_INCLINE_SEN_ERR_DET_TIME;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x1D: InclineErrDetTime -- send [0x1D, param] */
void lcb_ud_set_incline_err_det_time(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_1D];
    data[0] = LCB_UD_CMD_INCLINE_ERR_DET_TIME;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x1E: OverCurrentCountH -- send [0x1E, param] */
void lcb_ud_set_over_current_count_h(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_1E];
    data[0] = LCB_UD_CMD_OVER_CURRENT_COUNT_H;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x1F: OverCurrentCountL -- send [0x1F, param] */
void lcb_ud_set_over_current_count_l(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_1F];
    data[0] = LCB_UD_CMD_OVER_CURRENT_COUNT_L;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x20: SetOverCurrentH -- send [0x20, param] */
void lcb_ud_set_over_current_h(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_20];
    data[0] = LCB_UD_CMD_OVER_CURRENT_H;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x21: OverCurrentL -- send [0x21, param] */
void lcb_ud_set_over_current_l(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_21];
    data[0] = LCB_UD_CMD_OVER_CURRENT_L;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x22: GetCasterSwitchStatus -- send [0x22, param] */
void lcb_ud_get_caster_switch_status(lcb_comm_t *comm, uint8_t param)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_22];
    data[0] = LCB_UD_CMD_GET_CASTER_SWITCH_STATUS;
    data[1] = param;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x23: SetPFCmode -- send [0x23, 0x01, state] */
void lcb_ud_set_pfc_mode(lcb_comm_t *comm, uint8_t state)
{
    uint8_t data[LCB_UD_SEND_SIZE_CMD_23];
    data[0] = LCB_UD_CMD_SET_PFC_MODE;
    data[1] = 0x01;  /* Write flag */
    data[2] = state;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x24: SetMagnetAngleState -- send [0x24, 0x01, state] */
void lcb_ud_set_magnet_angle_state(lcb_comm_t *comm, uint8_t state)
{
    uint8_t data[LCB_UD_SEND_SET_SIZE_CMD_24];
    data[0] = LCB_UD_CMD_MAGNET_ANGLE_CAL;
    data[1] = 0x01;  /* Write flag */
    data[2] = state;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x24: GetMagnetAngle -- send [0x24, 0x00] with queue mode */
void lcb_ud_get_magnet_angle(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    uint8_t data[LCB_UD_SEND_GET_SIZE_CMD_24];
    data[0] = LCB_UD_CMD_MAGNET_ANGLE_CAL;
    data[1] = 0x00;  /* Read flag */
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), mode);
}

/* Sub-Command 0x25: SetMcbVoltage -- send [0x25, 0x01, data] */
void lcb_ud_set_mcb_voltage(lcb_comm_t *comm, uint8_t voltage_data)
{
    uint8_t data[LCB_UD_SEND_SET_SIZE_CMD_25];
    data[0] = LCB_UD_CMD_MCB_VOLTAGE;
    data[1] = 0x01;  /* Write flag */
    data[2] = voltage_data;
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x25: GetMcbVoltage -- send [0x25, 0x00] with queue mode */
void lcb_ud_get_mcb_voltage(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    uint8_t data[LCB_UD_SEND_GET_SIZE_CMD_25];
    data[0] = LCB_UD_CMD_MCB_VOLTAGE;
    data[1] = 0x00;  /* Read flag */
    lcb_comm_set_lcb_universal(comm, data, sizeof(data), mode);
}
