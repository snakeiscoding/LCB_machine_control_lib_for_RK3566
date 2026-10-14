/**
 * @file lcb_device_data_proc.c
 * @brief LCB Device Data Sub-Command RX Processing
 *
 * Converted from: com/jht/lcbcommlib/DeviceDataProcReceived.java
 *
 * Dispatches received 0x50 device data to per-sub-command handlers.
 * The first byte of the data payload is the sub-command ID.
 */

#include "lcb_device_data.h"
#include "lcb_logging.h"
#include <string.h>

static const char *TAG = "LcbDeviceDataProc";

/* ============================================================================
 * Initialization
 * ============================================================================ */

void lcb_device_data_callbacks_init(lcb_device_data_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(lcb_device_data_callbacks_t));
}

/* ============================================================================
 * Internal Sub-Command Handlers
 *
 * Each handler mirrors the corresponding proc*Handler in
 * DeviceDataProcReceived.java. The receivedData parameter includes
 * the sub-command byte as data[0].
 * ============================================================================ */

/* 0x01 GetEEPromMemorySizes -- size-validated, int from 2 LE bytes */
static void proc_dd_get_eeprom_memory_sizes(const lcb_device_data_callbacks_t *cb,
                                             const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_01) {
        LCB_LOGD(TAG, "DeviceData GetEEPromMemorySizes not match definition size");
        return;
    }
    /* Skip cmd byte (data[0]), convert 2 bytes to int (LE: low byte first) */
    int memory_sizes = ((int)data[2] << 8) | (int)data[1];

    if (cb->on_dd_get_eeprom_memory_sizes != NULL)
        cb->on_dd_get_eeprom_memory_sizes(cb->user_data, memory_sizes);
}

/* 0x02 GetEEPromMemoryContent -- size-validated raw bytes */
static void proc_dd_get_eeprom_memory_content(const lcb_device_data_callbacks_t *cb,
                                               const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_02) {
        LCB_LOGD(TAG, "DeviceData GetEEPromMemoryContent not match definition size");
        return;
    }
    if (cb->on_dd_get_eeprom_memory_content != NULL)
        cb->on_dd_get_eeprom_memory_content(cb->user_data, data + 1, length - 1);
}

/* 0x03 GetECBCurrent -- size-validated raw bytes */
static void proc_dd_get_ecb_current(const lcb_device_data_callbacks_t *cb,
                                     const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_03) {
        LCB_LOGD(TAG, "DeviceData GetECBCurrent not match definition size");
        return;
    }
    if (cb->on_dd_get_ecb_current != NULL)
        cb->on_dd_get_ecb_current(cb->user_data, data + 1, length - 1);
}

/* 0x04 GetEStopVale -- size-validated raw bytes */
static void proc_dd_get_estop_vale(const lcb_device_data_callbacks_t *cb,
                                    const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_04) {
        LCB_LOGD(TAG, "DeviceData GetEStopVale not match definition size");
        return;
    }
    if (cb->on_dd_get_estop_vale != NULL)
        cb->on_dd_get_estop_vale(cb->user_data, data + 1, length - 1);
}

/* 0x05 GetDCBusStatus -- size-validated raw bytes */
static void proc_dd_get_dc_bus_status(const lcb_device_data_callbacks_t *cb,
                                       const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_05) {
        LCB_LOGD(TAG, "DeviceData GetDCBusStatus not match definition size");
        return;
    }
    if (cb->on_dd_get_dc_bus_status != NULL)
        cb->on_dd_get_dc_bus_status(cb->user_data, data + 1, length - 1);
}

/* 0x06 GetLCBversion -- variable-length raw bytes (no size check) */
static void proc_dd_get_lcb_version(const lcb_device_data_callbacks_t *cb,
                                     const uint8_t *data, uint8_t length)
{
    if (length < 2) return;  /* need at least cmd + 1 byte */
    if (cb->on_dd_get_lcb_version != NULL)
        cb->on_dd_get_lcb_version(cb->user_data, data + 1, length - 1);
}

/* 0x07 GetControlZoneInfo -- special: dynamic struct with zone count */
static void proc_dd_get_control_zone_info(const lcb_device_data_callbacks_t *cb,
                                           const uint8_t *data, uint8_t length)
{
    if (length < 2) return;  /* need at least cmd + zone_count byte */

    /* data[0] = cmd, data[1] = zone count */
    uint8_t zone_count = data[1];

    /* Build output: [zone_count, zone_data...] */
    /* Each zone uses 2 bytes, so expected data from data[2] onward = zone_count * 2 */
    uint8_t expected_zone_data = zone_count * 2;
    uint8_t available = (length > 2) ? (length - 2) : 0;
    uint8_t zone_data_len = (available < expected_zone_data) ? available : expected_zone_data;

    /* Output buffer: zone_count byte + zone data */
    uint8_t out_buf[256];
    uint8_t out_len = 1 + zone_data_len;
    out_buf[0] = zone_count;
    if (zone_data_len > 0)
        memcpy(&out_buf[1], data + 2, zone_data_len);

    if (cb->on_dd_get_control_zone_info != NULL)
        cb->on_dd_get_control_zone_info(cb->user_data, out_buf, out_len);
}

/* 0x08 GetUpdateLcbMcuType -- variable-length raw bytes */
static void proc_dd_get_update_lcb_mcu_type(const lcb_device_data_callbacks_t *cb,
                                             const uint8_t *data, uint8_t length)
{
    if (length < 2) return;
    if (cb->on_dd_get_update_lcb_mcu_type != NULL)
        cb->on_dd_get_update_lcb_mcu_type(cb->user_data, data + 1, length - 1);
}

/* 0x09 GetUpdateLcbModuleName -- variable-length raw bytes */
static void proc_dd_get_update_lcb_module_name(const lcb_device_data_callbacks_t *cb,
                                                const uint8_t *data, uint8_t length)
{
    if (length < 2) return;
    if (cb->on_dd_get_update_lcb_module_name != NULL)
        cb->on_dd_get_update_lcb_module_name(cb->user_data, data + 1, length - 1);
}

/* 0x0A GetUpdateLcbBrand -- variable-length raw bytes */
static void proc_dd_get_update_lcb_brand(const lcb_device_data_callbacks_t *cb,
                                          const uint8_t *data, uint8_t length)
{
    if (length < 2) return;
    if (cb->on_dd_get_update_lcb_brand != NULL)
        cb->on_dd_get_update_lcb_brand(cb->user_data, data + 1, length - 1);
}

/* 0x0B GetUpdateLcbBootVersion -- variable-length raw bytes */
static void proc_dd_get_update_lcb_boot_version(const lcb_device_data_callbacks_t *cb,
                                                  const uint8_t *data, uint8_t length)
{
    if (length < 2) return;
    if (cb->on_dd_get_update_lcb_boot_version != NULL)
        cb->on_dd_get_update_lcb_boot_version(cb->user_data, data + 1, length - 1);
}

/* 0x0C GetUpdateBinAddrModuleName -- size-validated raw bytes */
static void proc_dd_get_update_bin_addr_module_name(const lcb_device_data_callbacks_t *cb,
                                                     const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_0C) {
        LCB_LOGD(TAG, "DeviceData GetUpdateBinAddrModuleName not match definition size");
        return;
    }
    if (cb->on_dd_get_update_bin_addr_module_name != NULL)
        cb->on_dd_get_update_bin_addr_module_name(cb->user_data, data + 1, length - 1);
}

/* 0x0D GetUpdateBinAddrMcuType -- size-validated raw bytes */
static void proc_dd_get_update_bin_addr_mcu_type(const lcb_device_data_callbacks_t *cb,
                                                  const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_0D) {
        LCB_LOGD(TAG, "DeviceData GetUpdateBinAddrMcuType not match definition size");
        return;
    }
    if (cb->on_dd_get_update_bin_addr_mcu_type != NULL)
        cb->on_dd_get_update_bin_addr_mcu_type(cb->user_data, data + 1, length - 1);
}

/* 0x0E GetUpdateBinAddrBrand -- size-validated raw bytes */
static void proc_dd_get_update_bin_addr_brand(const lcb_device_data_callbacks_t *cb,
                                               const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_0E) {
        LCB_LOGD(TAG, "DeviceData GetUpdateBinAddrBrand not match definition size");
        return;
    }
    if (cb->on_dd_get_update_bin_addr_brand != NULL)
        cb->on_dd_get_update_bin_addr_brand(cb->user_data, data + 1, length - 1);
}

/* 0x0F GetUpdateBinAddrBootVersion -- size-validated raw bytes */
static void proc_dd_get_update_bin_addr_boot_version(const lcb_device_data_callbacks_t *cb,
                                                      const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_0F) {
        LCB_LOGD(TAG, "DeviceData GetUpdateBinAddrBootVersion not match definition size");
        return;
    }
    if (cb->on_dd_get_update_bin_addr_boot_version != NULL)
        cb->on_dd_get_update_bin_addr_boot_version(cb->user_data, data + 1, length - 1);
}

/* 0x11 GetUpdateMappingAddrBinToMcu -- variable-length raw bytes */
static void proc_dd_get_update_mapping_addr_bin_to_mcu(const lcb_device_data_callbacks_t *cb,
                                                        const uint8_t *data, uint8_t length)
{
    if (length < 2) return;
    if (cb->on_dd_get_update_mapping_addr_bin_to_mcu != NULL)
        cb->on_dd_get_update_mapping_addr_bin_to_mcu(cb->user_data, data + 1, length - 1);
}

/* 0x70 SoftwareReset -- simple ACK (size=1, cmd byte only) */
static void proc_dd_software_reset(const lcb_device_data_callbacks_t *cb,
                                    const uint8_t *data, uint8_t length)
{
    (void)data;
    if (length != LCB_DD_RECV_SIZE_CMD_70) {
        LCB_LOGD(TAG, "DeviceData SoftwareReset not match definition size");
        return;
    }
    if (cb->on_dd_software_reset != NULL)
        cb->on_dd_software_reset(cb->user_data);
}

/* 0x71 EraseLoaderMemory -- status byte */
static void proc_dd_erase_loader_memory(const lcb_device_data_callbacks_t *cb,
                                         const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_71) {
        LCB_LOGD(TAG, "DeviceData EraseLoaderMemory not match definition size");
        return;
    }
    uint8_t status = data[1];
    if (cb->on_dd_erase_loader_memory != NULL)
        cb->on_dd_erase_loader_memory(cb->user_data, status);
}

/* 0x72 WriteWordLoaderData -- status byte */
static void proc_dd_write_word_loader_data(const lcb_device_data_callbacks_t *cb,
                                            const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_72) {
        LCB_LOGD(TAG, "DeviceData WriteWordLoaderData not match definition size");
        return;
    }
    uint8_t status = data[1];
    if (cb->on_dd_write_word_loader_data != NULL)
        cb->on_dd_write_word_loader_data(cb->user_data, status);
}

/* 0x73 ReadByteData -- special: status + len + data */
static void proc_dd_read_byte_data(const lcb_device_data_callbacks_t *cb,
                                    const uint8_t *data, uint8_t length)
{
    if (length < 3) return;  /* need at least cmd + status + len_of_data */

    uint8_t status = data[1];
    uint8_t len_of_data = data[2];

    /* Build output: [status, len_of_data, actual_data...] */
    uint8_t out_buf[256];
    uint8_t available_data = (length > 3) ? (length - 3) : 0;
    uint8_t copy_len = (available_data < len_of_data) ? available_data : len_of_data;
    uint8_t out_len = 2 + copy_len;

    out_buf[0] = status;
    out_buf[1] = len_of_data;
    if (copy_len > 0)
        memcpy(&out_buf[2], data + 3, copy_len);

    if (cb->on_dd_read_byte_data != NULL)
        cb->on_dd_read_byte_data(cb->user_data, out_buf, out_len);
}

/* 0x80 SavePWM -- status byte */
static void proc_dd_save_pwm(const lcb_device_data_callbacks_t *cb,
                               const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_80) {
        LCB_LOGD(TAG, "DeviceData SavePWM not match definition size");
        return;
    }
    uint8_t status = data[1];
    if (cb->on_dd_save_pwm != NULL)
        cb->on_dd_save_pwm(cb->user_data, status);
}

/* 0x81 ReadPWM -- size-validated raw bytes */
static void proc_dd_read_pwm(const lcb_device_data_callbacks_t *cb,
                               const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_81) {
        LCB_LOGD(TAG, "DeviceData ReadPWM not match definition size");
        return;
    }
    if (cb->on_dd_read_pwm != NULL)
        cb->on_dd_read_pwm(cb->user_data, data + 1, length - 1);
}

/* 0x90 SetMcbLcbParameter -- status byte */
static void proc_dd_set_mcb_lcb_parameter(const lcb_device_data_callbacks_t *cb,
                                            const uint8_t *data, uint8_t length)
{
    if (length != LCB_DD_RECV_SIZE_CMD_90) {
        LCB_LOGD(TAG, "DeviceData SetMcbLcbParameter not match definition size");
        return;
    }
    uint8_t status = data[1];
    if (cb->on_dd_set_mcb_lcb_parameter != NULL)
        cb->on_dd_set_mcb_lcb_parameter(cb->user_data, status);
}

/* 0x91 GetMcbLcbParameter -- variable-length raw bytes */
static void proc_dd_get_mcb_lcb_parameter(const lcb_device_data_callbacks_t *cb,
                                            const uint8_t *data, uint8_t length)
{
    if (length < 2) return;
    if (cb->on_dd_get_mcb_lcb_parameter != NULL)
        cb->on_dd_get_mcb_lcb_parameter(cb->user_data, data + 1, length - 1);
}

/* 0xF0 GetLCBinformation -- variable-length raw bytes */
static void proc_dd_get_lcb_information(const lcb_device_data_callbacks_t *cb,
                                         const uint8_t *data, uint8_t length)
{
    if (length < 2) return;
    if (cb->on_dd_get_lcb_information != NULL)
        cb->on_dd_get_lcb_information(cb->user_data, data + 1, length - 1);
}

/* ============================================================================
 * Main Dispatcher (equivalent to DeviceDataProcReceived.analyzeDeviceData)
 * ============================================================================ */

void lcb_device_data_analyze(const lcb_device_data_callbacks_t *cb,
                             const uint8_t *data, uint8_t length)
{
    if (cb == NULL || data == NULL || length == 0) return;

    uint8_t cmd_byte = data[0];

    LCB_LOGD(TAG, "Analyzing DeviceData sub-command 0x%02X (len=%d)", cmd_byte, length);

    switch (cmd_byte) {
        case LCB_DD_CMD_GET_EEPROM_MEMORY_SIZES:
            proc_dd_get_eeprom_memory_sizes(cb, data, length);
            break;
        case LCB_DD_CMD_GET_EEPROM_MEMORY_CONTENT:
            proc_dd_get_eeprom_memory_content(cb, data, length);
            break;
        case LCB_DD_CMD_GET_ECB_CURRENT:
            proc_dd_get_ecb_current(cb, data, length);
            break;
        case LCB_DD_CMD_GET_ESTOP_VALE:
            proc_dd_get_estop_vale(cb, data, length);
            break;
        case LCB_DD_CMD_GET_DC_BUS_STATUS:
            proc_dd_get_dc_bus_status(cb, data, length);
            break;
        case LCB_DD_CMD_GET_LCB_VERSION:
            proc_dd_get_lcb_version(cb, data, length);
            break;
        case LCB_DD_CMD_GET_CONTROL_ZONE_INFO:
            proc_dd_get_control_zone_info(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_LCB_MCU_TYPE:
            proc_dd_get_update_lcb_mcu_type(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_LCB_MODULE_NAME:
            proc_dd_get_update_lcb_module_name(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_LCB_BRAND:
            proc_dd_get_update_lcb_brand(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_LCB_BOOT_VERSION:
            proc_dd_get_update_lcb_boot_version(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_BIN_ADDR_MODULE_NAME:
            proc_dd_get_update_bin_addr_module_name(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_BIN_ADDR_MCU_TYPE:
            proc_dd_get_update_bin_addr_mcu_type(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_BIN_ADDR_BRAND:
            proc_dd_get_update_bin_addr_brand(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_BIN_ADDR_BOOT_VERSION:
            proc_dd_get_update_bin_addr_boot_version(cb, data, length);
            break;
        case LCB_DD_CMD_GET_UPDATE_MAPPING_ADDR_BIN_TO_MCU:
            proc_dd_get_update_mapping_addr_bin_to_mcu(cb, data, length);
            break;
        case LCB_DD_CMD_SOFTWARE_RESET:
            proc_dd_software_reset(cb, data, length);
            break;
        case LCB_DD_CMD_ERASE_LOADER_MEMORY:
            proc_dd_erase_loader_memory(cb, data, length);
            break;
        case LCB_DD_CMD_WRITE_WORD_LOADER_DATA:
            proc_dd_write_word_loader_data(cb, data, length);
            break;
        case LCB_DD_CMD_READ_BYTE_DATA:
            proc_dd_read_byte_data(cb, data, length);
            break;
        case LCB_DD_CMD_SAVE_PWM:
            proc_dd_save_pwm(cb, data, length);
            break;
        case LCB_DD_CMD_READ_PWM:
            proc_dd_read_pwm(cb, data, length);
            break;
        case LCB_DD_CMD_SET_MCB_LCB_PARAMETER:
            proc_dd_set_mcb_lcb_parameter(cb, data, length);
            break;
        case LCB_DD_CMD_GET_MCB_LCB_PARAMETER:
            proc_dd_get_mcb_lcb_parameter(cb, data, length);
            break;
        case LCB_DD_CMD_GET_LCB_INFORMATION:
            proc_dd_get_lcb_information(cb, data, length);
            break;
        default:
            LCB_LOGD(TAG, "Unknown DeviceData sub-command 0x%02X", cmd_byte);
            break;
    }
}
