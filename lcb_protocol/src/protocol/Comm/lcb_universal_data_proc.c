/**
 * @file lcb_universal_data_proc.c
 * @brief LCB Universal Data RX Processing (Command 0xAA Sub-Commands)
 *
 * Converted from: com/jht/lcbcommlib/UniDataProcReceived.java
 *
 * Dispatches received universal data to per-sub-command callback handlers.
 */

#include "lcb_universal_data.h"
#include "lcb_logging.h"
#include <string.h>

#define TAG "LcbUniversalDataProc"

/* ============================================================================
 * Static RX Handlers
 * ============================================================================ */

/**
 * @brief 0x03 GetMotoType
 *
 * Java: procUniGetMotoTypeHandler
 * Validates length == 3, reads 2-byte BE short from data[1..2], converts to int.
 */
static void proc_ud_get_moto_type(const lcb_universal_data_callbacks_t *cb,
                                   const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_03) {
        LCB_LOGD(TAG, "UniversalData GetMotoType not match definition size");
        return;
    }

    /* 2-byte big-endian short → int (Java: ByteBuffer.getShort() BE) */
    int moto_type = ((int)data[1] << 8) | (int)data[2];

    if (cb->on_ud_get_moto_type != NULL)
        cb->on_ud_get_moto_type(cb->user_data, moto_type);
}

/**
 * @brief 0x11 LockMotoProtect
 * Validates length == 3, reads 2-byte BE short from data[1..2], converts to int.
 */
static void proc_ud_lock_moto_protect(const lcb_universal_data_callbacks_t *cb,
                                       const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_11) {
        LCB_LOGD(TAG, "UniversalData LockMotoProtect not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_lock_moto_protect != NULL)
        cb->on_ud_lock_moto_protect(cb->user_data, value);
}

/**
 * @brief 0x12 MotoRateSlip
 */
static void proc_ud_moto_rate_slip(const lcb_universal_data_callbacks_t *cb,
                                    const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_12) {
        LCB_LOGD(TAG, "UniversalData MotoRateSlip not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_moto_rate_slip != NULL)
        cb->on_ud_moto_rate_slip(cb->user_data, value);
}

/**
 * @brief 0x13 MotoRateCurrent
 */
static void proc_ud_moto_rate_current(const lcb_universal_data_callbacks_t *cb,
                                       const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_13) {
        LCB_LOGD(TAG, "UniversalData MotoRateCurrent not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_moto_rate_current != NULL)
        cb->on_ud_moto_rate_current(cb->user_data, value);
}

/**
 * @brief 0x14 RateVol
 */
static void proc_ud_rate_vol(const lcb_universal_data_callbacks_t *cb,
                              const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_14) {
        LCB_LOGD(TAG, "UniversalData RateVol not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_rate_vol != NULL)
        cb->on_ud_rate_vol(cb->user_data, value);
}

/**
 * @brief 0x15 RateFreq
 */
static void proc_ud_rate_freq(const lcb_universal_data_callbacks_t *cb,
                               const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_15) {
        LCB_LOGD(TAG, "UniversalData RateFreq not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_rate_freq != NULL)
        cb->on_ud_rate_freq(cb->user_data, value);
}

/**
 * @brief 0x16 CompensationMotoRes
 */
static void proc_ud_compensation_moto_res(const lcb_universal_data_callbacks_t *cb,
                                            const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_16) {
        LCB_LOGD(TAG, "UniversalData CompensationMotoRes not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_compensation_moto_res != NULL)
        cb->on_ud_compensation_moto_res(cb->user_data, value);
}

/**
 * @brief 0x17 OverCurrentDetTime
 *
 * Validates length == 3, reads 2-byte BE short from data[1..2], converts to int.
 */
static void proc_ud_over_current_det_time(const lcb_universal_data_callbacks_t *cb,
                                           const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_17) {
        LCB_LOGD(TAG, "UniversalData OverCurrentDetTime not match definition size");
        return;
    }

    int value = ((int)data[1] << 8) | (int)data[2];

    if (cb->on_ud_over_current_det_time != NULL)
        cb->on_ud_over_current_det_time(cb->user_data, value);
}

/**
 * @brief 0x18 OverLoadDetTime
 */
static void proc_ud_over_load_det_time(const lcb_universal_data_callbacks_t *cb,
                                        const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_18) {
        LCB_LOGD(TAG, "UniversalData OverLoadDetTime not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_over_load_det_time != NULL)
        cb->on_ud_over_load_det_time(cb->user_data, value);
}

/**
 * @brief 0x19 OverVolDetTime
 */
static void proc_ud_over_vol_det_time(const lcb_universal_data_callbacks_t *cb,
                                       const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_19) {
        LCB_LOGD(TAG, "UniversalData OverVolDetTime not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_over_vol_det_time != NULL)
        cb->on_ud_over_vol_det_time(cb->user_data, value);
}

/**
 * @brief 0x1A MotoWireBreakDetTime
 */
static void proc_ud_moto_wire_break_det_time(const lcb_universal_data_callbacks_t *cb,
                                               const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_1A) {
        LCB_LOGD(TAG, "UniversalData MotoWireBreakDetTime not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_moto_wire_break_det_time != NULL)
        cb->on_ud_moto_wire_break_det_time(cb->user_data, value);
}

/**
 * @brief 0x1B InclineDirDef
 *
 * Validates length == 3, reads 2-byte BE short from data[1..2], converts to int.
 */
static void proc_ud_incline_dir_def(const lcb_universal_data_callbacks_t *cb,
                                     const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_1B) {
        LCB_LOGD(TAG, "UniversalData InclineDirDef not match definition size");
        return;
    }

    int value = ((int)data[1] << 8) | (int)data[2];

    if (cb->on_ud_incline_dir_def != NULL)
        cb->on_ud_incline_dir_def(cb->user_data, value);
}

/**
 * @brief 0x1C InclineSenErrDetTime
 */
static void proc_ud_incline_sen_err_det_time(const lcb_universal_data_callbacks_t *cb,
                                               const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_1C) {
        LCB_LOGD(TAG, "UniversalData InclineSenErrDetTime not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_incline_sen_err_det_time != NULL)
        cb->on_ud_incline_sen_err_det_time(cb->user_data, value);
}

/**
 * @brief 0x1D InclineErrDetTime
 */
static void proc_ud_incline_err_det_time(const lcb_universal_data_callbacks_t *cb,
                                           const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_1D) {
        LCB_LOGD(TAG, "UniversalData InclineErrDetTime not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_incline_err_det_time != NULL)
        cb->on_ud_incline_err_det_time(cb->user_data, value);
}

/**
 * @brief 0x1E OverCurrentCountH
 */
static void proc_ud_over_current_count_h(const lcb_universal_data_callbacks_t *cb,
                                           const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_1E) {
        LCB_LOGD(TAG, "UniversalData OverCurrentCountH not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_over_current_count_h != NULL)
        cb->on_ud_over_current_count_h(cb->user_data, value);
}

/**
 * @brief 0x1F OverCurrentCountL
 */
static void proc_ud_over_current_count_l(const lcb_universal_data_callbacks_t *cb,
                                           const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_1F) {
        LCB_LOGD(TAG, "UniversalData OverCurrentCountL not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_over_current_count_l != NULL)
        cb->on_ud_over_current_count_l(cb->user_data, value);
}

/**
 * @brief 0x20 OverCurrentH
 *
 * Validates length == 3, reads 2-byte BE short from data[1..2], converts to int.
 */
static void proc_ud_over_current_h(const lcb_universal_data_callbacks_t *cb,
                                    const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_20) {
        LCB_LOGD(TAG, "UniversalData OverCurrentH not match definition size");
        return;
    }

    int value = ((int)data[1] << 8) | (int)data[2];

    if (cb->on_ud_over_current_h != NULL)
        cb->on_ud_over_current_h(cb->user_data, value);
}

/**
 * @brief 0x21 OverCurrentL
 */
static void proc_ud_over_current_l(const lcb_universal_data_callbacks_t *cb,
                                    const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_21) {
        LCB_LOGD(TAG, "UniversalData OverCurrentL not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_over_current_l != NULL)
        cb->on_ud_over_current_l(cb->user_data, value);
}

/**
 * @brief 0x22 GetCasterSwitchStatus
 */
static void proc_ud_get_caster_switch_status(const lcb_universal_data_callbacks_t *cb,
                                               const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_22) {
        LCB_LOGD(TAG, "UniversalData GetCasterSwitchStatus not match definition size");
        return;
    }
    int value = ((int)data[1] << 8) | (int)data[2];
    if (cb->on_ud_get_caster_switch_status != NULL)
        cb->on_ud_get_caster_switch_status(cb->user_data, value);
}

/**
 * @brief 0x23 SetPFCmode
 *
 * Java: procUniSetPFCmodeHandler
 * Validates length == 4, extracts bytes after cmd+len (data[2..3]).
 */
static void proc_ud_set_pfc_mode(const lcb_universal_data_callbacks_t *cb,
                                  const uint8_t *data, uint8_t length)
{
    if (length != LCB_UD_RECV_SIZE_CMD_23) {
        LCB_LOGD(TAG, "UniversalData SetPFCmode not match definition size");
        return;
    }

    /* Skip cmd byte (data[0]) and length byte (data[1]), pass remaining */
    if (cb->on_ud_set_pfc_mode != NULL)
        cb->on_ud_set_pfc_mode(cb->user_data, data + 2, length - 2);
}

/**
 * @brief 0x24 MagnetAngleCal
 *
 * Java: procUniMagnetAngleStateHandler
 * Returns early if length == 1 (only cmd byte), otherwise passes data[1..n].
 */
static void proc_ud_magnet_angle_state(const lcb_universal_data_callbacks_t *cb,
                                        const uint8_t *data, uint8_t length)
{
    if (length <= 1) {
        LCB_LOGD(TAG, "UniversalData MagnetAngleCal: only cmd byte, skipping");
        return;
    }

    /* Skip cmd byte (data[0]), pass remaining */
    if (cb->on_ud_magnet_angle_state != NULL)
        cb->on_ud_magnet_angle_state(cb->user_data, data + 1, length - 1);
}

/**
 * @brief 0x25 McbVoltage
 *
 * Java: procUniMcbVoltageHandler
 * Returns early if length == 1 (only cmd byte), otherwise passes data[1..n].
 */
static void proc_ud_mcb_voltage(const lcb_universal_data_callbacks_t *cb,
                                 const uint8_t *data, uint8_t length)
{
    if (length <= 1) {
        LCB_LOGD(TAG, "UniversalData McbVoltage: only cmd byte, skipping");
        return;
    }

    /* Skip cmd byte (data[0]), pass remaining */
    if (cb->on_ud_mcb_voltage != NULL)
        cb->on_ud_mcb_voltage(cb->user_data, data + 1, length - 1);
}

/* ============================================================================
 * Public Functions
 * ============================================================================ */

void lcb_universal_data_callbacks_init(lcb_universal_data_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(lcb_universal_data_callbacks_t));
}

void lcb_universal_data_analyze(const lcb_universal_data_callbacks_t *cb,
                                const uint8_t *data, uint8_t length)
{
    if (cb == NULL || data == NULL || length == 0) return;

    uint8_t cmd_byte = data[0];
    LCB_LOGD(TAG, "Analyzing UniversalData sub-command 0x%02X (len=%d)", cmd_byte, length);

    switch (cmd_byte) {
        case LCB_UD_CMD_GET_MOTO_TYPE:
            proc_ud_get_moto_type(cb, data, length);
            break;
        case LCB_UD_CMD_LOCK_MOTO_PROTECT:
            proc_ud_lock_moto_protect(cb, data, length);
            break;
        case LCB_UD_CMD_MOTO_RATE_SLIP:
            proc_ud_moto_rate_slip(cb, data, length);
            break;
        case LCB_UD_CMD_MOTO_RATE_CURRENT:
            proc_ud_moto_rate_current(cb, data, length);
            break;
        case LCB_UD_CMD_RATE_VOL:
            proc_ud_rate_vol(cb, data, length);
            break;
        case LCB_UD_CMD_RATE_FREQ:
            proc_ud_rate_freq(cb, data, length);
            break;
        case LCB_UD_CMD_COMPENSATION_MOTO_RES:
            proc_ud_compensation_moto_res(cb, data, length);
            break;
        case LCB_UD_CMD_OVER_CURRENT_DET_TIME:
            proc_ud_over_current_det_time(cb, data, length);
            break;
        case LCB_UD_CMD_OVER_LOAD_DET_TIME:
            proc_ud_over_load_det_time(cb, data, length);
            break;
        case LCB_UD_CMD_OVER_VOL_DET_TIME:
            proc_ud_over_vol_det_time(cb, data, length);
            break;
        case LCB_UD_CMD_MOTO_WIRE_BREAK_DET_TIME:
            proc_ud_moto_wire_break_det_time(cb, data, length);
            break;
        case LCB_UD_CMD_INCLINE_DIR_DEF:
            proc_ud_incline_dir_def(cb, data, length);
            break;
        case LCB_UD_CMD_INCLINE_SEN_ERR_DET_TIME:
            proc_ud_incline_sen_err_det_time(cb, data, length);
            break;
        case LCB_UD_CMD_INCLINE_ERR_DET_TIME:
            proc_ud_incline_err_det_time(cb, data, length);
            break;
        case LCB_UD_CMD_OVER_CURRENT_COUNT_H:
            proc_ud_over_current_count_h(cb, data, length);
            break;
        case LCB_UD_CMD_OVER_CURRENT_COUNT_L:
            proc_ud_over_current_count_l(cb, data, length);
            break;
        case LCB_UD_CMD_OVER_CURRENT_H:
            proc_ud_over_current_h(cb, data, length);
            break;
        case LCB_UD_CMD_OVER_CURRENT_L:
            proc_ud_over_current_l(cb, data, length);
            break;
        case LCB_UD_CMD_GET_CASTER_SWITCH_STATUS:
            proc_ud_get_caster_switch_status(cb, data, length);
            break;
        case LCB_UD_CMD_SET_PFC_MODE:
            proc_ud_set_pfc_mode(cb, data, length);
            break;
        case LCB_UD_CMD_MAGNET_ANGLE_CAL:
            proc_ud_magnet_angle_state(cb, data, length);
            break;
        case LCB_UD_CMD_MCB_VOLTAGE:
            proc_ud_mcb_voltage(cb, data, length);
            break;
        default:
            LCB_LOGD(TAG, "Unknown UniversalData sub-command 0x%02X", cmd_byte);
            break;
    }
}
