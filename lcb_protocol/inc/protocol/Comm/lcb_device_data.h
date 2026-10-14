/**
 * @file lcb_device_data.h
 * @brief LCB Device Data Sub-Command Interface (Command 0x50)
 *
 * Converted from:
 *   - com/jht/lcbcommlib/DeviceDataCommands.java
 *   - com/jht/lcbcommlib/DeviceDataConstants.java
 *   - com/jht/lcbcommlib/IDeviceDataReceived.java
 *   - com/jht/lcbcommlib/IDeviceDataTransmitted.java
 *
 * Command 0x50 carries a sub-command byte as the first data byte.
 * This module dispatches received data to per-sub-command callbacks (RX)
 * and provides helper functions to build sub-command packets (TX).
 */

#ifndef LCB_DEVICE_DATA_H
#define LCB_DEVICE_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Sub-Command Enum (from DeviceDataCommands.java)
 * ============================================================================ */

typedef enum {
    LCB_DD_CMD_GET_EEPROM_MEMORY_SIZES          = 0x01,
    LCB_DD_CMD_GET_EEPROM_MEMORY_CONTENT        = 0x02,
    LCB_DD_CMD_GET_ECB_CURRENT                  = 0x03,
    LCB_DD_CMD_GET_ESTOP_VALE                   = 0x04,
    LCB_DD_CMD_GET_DC_BUS_STATUS                = 0x05,
    LCB_DD_CMD_GET_LCB_VERSION                  = 0x06,
    LCB_DD_CMD_GET_CONTROL_ZONE_INFO            = 0x07,
    LCB_DD_CMD_GET_UPDATE_LCB_MCU_TYPE          = 0x08,
    LCB_DD_CMD_GET_UPDATE_LCB_MODULE_NAME       = 0x09,
    LCB_DD_CMD_GET_UPDATE_LCB_BRAND             = 0x0A,
    LCB_DD_CMD_GET_UPDATE_LCB_BOOT_VERSION      = 0x0B,
    LCB_DD_CMD_GET_UPDATE_BIN_ADDR_MODULE_NAME  = 0x0C,
    LCB_DD_CMD_GET_UPDATE_BIN_ADDR_MCU_TYPE     = 0x0D,
    LCB_DD_CMD_GET_UPDATE_BIN_ADDR_BRAND        = 0x0E,
    LCB_DD_CMD_GET_UPDATE_BIN_ADDR_BOOT_VERSION = 0x0F,
    LCB_DD_CMD_GET_UPDATE_MAPPING_ADDR_BIN_TO_MCU = 0x11,
    LCB_DD_CMD_SOFTWARE_RESET                   = 0x70,
    LCB_DD_CMD_ERASE_LOADER_MEMORY              = 0x71,
    LCB_DD_CMD_WRITE_WORD_LOADER_DATA           = 0x72,
    LCB_DD_CMD_READ_BYTE_DATA                   = 0x73,
    LCB_DD_CMD_SAVE_PWM                         = 0x80,
    LCB_DD_CMD_READ_PWM                         = 0x81,
    LCB_DD_CMD_SET_MCB_LCB_PARAMETER            = 0x90,
    LCB_DD_CMD_GET_MCB_LCB_PARAMETER            = 0x91,
    LCB_DD_CMD_GET_LCB_INFORMATION              = 0xF0
} lcb_device_data_cmd_t;

/* ============================================================================
 * Send / Receive Data Size Constants (from DeviceDataConstants.java)
 * ============================================================================ */

/* Send data sizes (total bytes including sub-command byte) */
#define LCB_DD_SEND_SIZE_CMD_01    3   /* GetEEPromMemorySizes */
#define LCB_DD_SEND_SIZE_CMD_02    3   /* GetEEPromMemoryContent */
#define LCB_DD_SEND_SIZE_CMD_03    3   /* GetECBCurrent */
#define LCB_DD_SEND_SIZE_CMD_04    3   /* GetEStopVale */
#define LCB_DD_SEND_SIZE_CMD_05    3   /* GetDCBusStatus */
#define LCB_DD_SEND_SIZE_CMD_06    3   /* GetLCBversion */
#define LCB_DD_SEND_SIZE_CMD_07    3   /* GetControlZoneInfo */
#define LCB_DD_SEND_SIZE_CMD_08    1   /* GetUpdateLcbMcuType */
#define LCB_DD_SEND_SIZE_CMD_09    1   /* GetUpdateLcbModuleName */
#define LCB_DD_SEND_SIZE_CMD_0A    1   /* GetUpdateLcbBrand */
#define LCB_DD_SEND_SIZE_CMD_0B    1   /* GetUpdateLcbBootVersion */
#define LCB_DD_SEND_SIZE_CMD_0C    1   /* GetUpdateBinAddrModuleName */
#define LCB_DD_SEND_SIZE_CMD_0D    1   /* GetUpdateBinAddrMcuType */
#define LCB_DD_SEND_SIZE_CMD_0E    1   /* GetUpdateBinAddrBrand */
#define LCB_DD_SEND_SIZE_CMD_0F    1   /* GetUpdateBinAddrBootVersion */
#define LCB_DD_SEND_SIZE_CMD_11    1   /* GetUpdateMappingAddrBinToMcu */
#define LCB_DD_SEND_SIZE_CMD_70    1   /* SoftwareReset */
#define LCB_DD_SEND_SIZE_CMD_71    9   /* EraseLoaderMemory */
#define LCB_DD_SEND_SIZE_CMD_73    6   /* ReadByteData */
#define LCB_DD_SEND_SIZE_CMD_80    4   /* SavePWM */
#define LCB_DD_SEND_SIZE_CMD_81    2   /* ReadPWM */
#define LCB_DD_SEND_SIZE_CMD_91    2   /* GetMcbLcbParameter */
#define LCB_DD_SEND_SIZE_CMD_F0    1   /* GetLCBinformation */

/* Receive data sizes (total bytes including sub-command byte) */
#define LCB_DD_RECV_SIZE_CMD_01    3   /* GetEEPromMemorySizes */
#define LCB_DD_RECV_SIZE_CMD_02   17   /* GetEEPromMemoryContent */
#define LCB_DD_RECV_SIZE_CMD_03    5   /* GetECBCurrent */
#define LCB_DD_RECV_SIZE_CMD_04    5   /* GetEStopVale */
#define LCB_DD_RECV_SIZE_CMD_05    5   /* GetDCBusStatus */
#define LCB_DD_RECV_SIZE_CMD_06    9   /* GetLCBversion */
#define LCB_DD_RECV_SIZE_CMD_0C    9   /* GetUpdateBinAddrModuleName */
#define LCB_DD_RECV_SIZE_CMD_0D    9   /* GetUpdateBinAddrMcuType */
#define LCB_DD_RECV_SIZE_CMD_0E    9   /* GetUpdateBinAddrBrand */
#define LCB_DD_RECV_SIZE_CMD_0F    9   /* GetUpdateBinAddrBootVersion */
#define LCB_DD_RECV_SIZE_CMD_70    1   /* SoftwareReset */
#define LCB_DD_RECV_SIZE_CMD_71    2   /* EraseLoaderMemory */
#define LCB_DD_RECV_SIZE_CMD_72    2   /* WriteWordLoaderData */
#define LCB_DD_RECV_SIZE_CMD_80    2   /* SavePWM */
#define LCB_DD_RECV_SIZE_CMD_81    3   /* ReadPWM */
#define LCB_DD_RECV_SIZE_CMD_90    2   /* SetMcbLcbParameter */

/* ============================================================================
 * Callback Structure (from IDeviceDataReceived.java)
 * ============================================================================ */

/**
 * @brief Device Data sub-command receive callback structure
 *
 * All callback function pointers are optional (NULL = not registered).
 * The user_data pointer is passed as the first argument to every callback.
 */
typedef struct lcb_device_data_callbacks {
    void *user_data;

    /* Sub-Command 0x01: GetEEPromMemorySizes */
    void (*on_dd_get_eeprom_memory_sizes)(void *user_data, int memory_sizes);

    /* Sub-Command 0x02: GetEEPromMemoryContent */
    void (*on_dd_get_eeprom_memory_content)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x03: GetECBCurrent */
    void (*on_dd_get_ecb_current)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x04: GetEStopVale */
    void (*on_dd_get_estop_vale)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x05: GetDCBusStatus */
    void (*on_dd_get_dc_bus_status)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x06: GetLCBversion */
    void (*on_dd_get_lcb_version)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x07: GetControlZoneInfo */
    void (*on_dd_get_control_zone_info)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x08: GetUpdateLcbMcuType */
    void (*on_dd_get_update_lcb_mcu_type)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x09: GetUpdateLcbModuleName */
    void (*on_dd_get_update_lcb_module_name)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x0A: GetUpdateLcbBrand */
    void (*on_dd_get_update_lcb_brand)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x0B: GetUpdateLcbBootVersion */
    void (*on_dd_get_update_lcb_boot_version)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x0C: GetUpdateBinAddrModuleName */
    void (*on_dd_get_update_bin_addr_module_name)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x0D: GetUpdateBinAddrMcuType */
    void (*on_dd_get_update_bin_addr_mcu_type)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x0E: GetUpdateBinAddrBrand */
    void (*on_dd_get_update_bin_addr_brand)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x0F: GetUpdateBinAddrBootVersion */
    void (*on_dd_get_update_bin_addr_boot_version)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x11: GetUpdateMappingAddrBinToMcu */
    void (*on_dd_get_update_mapping_addr_bin_to_mcu)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x70: SoftwareReset */
    void (*on_dd_software_reset)(void *user_data);

    /* Sub-Command 0x71: EraseLoaderMemory */
    void (*on_dd_erase_loader_memory)(void *user_data, uint8_t status);

    /* Sub-Command 0x72: WriteWordLoaderData */
    void (*on_dd_write_word_loader_data)(void *user_data, uint8_t status);

    /* Sub-Command 0x73: ReadByteData */
    void (*on_dd_read_byte_data)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x80: SavePWM */
    void (*on_dd_save_pwm)(void *user_data, uint8_t status);

    /* Sub-Command 0x81: ReadPWM */
    void (*on_dd_read_pwm)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0x90: SetMcbLcbParameter */
    void (*on_dd_set_mcb_lcb_parameter)(void *user_data, uint8_t status);

    /* Sub-Command 0x91: GetMcbLcbParameter */
    void (*on_dd_get_mcb_lcb_parameter)(void *user_data, const uint8_t *data, uint8_t length);

    /* Sub-Command 0xF0: GetLCBinformation */
    void (*on_dd_get_lcb_information)(void *user_data, const uint8_t *data, uint8_t length);

} lcb_device_data_callbacks_t;

/* ============================================================================
 * RX Functions (implemented in lcb_device_data_proc.c)
 * ============================================================================ */

/**
 * @brief Initialize device data callbacks structure with NULL values
 * @param cb Pointer to callback structure
 */
void lcb_device_data_callbacks_init(lcb_device_data_callbacks_t *cb);

/**
 * @brief Analyze received device data and dispatch to sub-command handlers
 *
 * This is the C equivalent of DeviceDataProcReceived.analyzeDeviceData().
 * The first byte of data is the sub-command ID.
 *
 * @param cb Pointer to device data callback structure
 * @param data Raw device data (first byte = sub-command)
 * @param length Total data length
 */
void lcb_device_data_analyze(const lcb_device_data_callbacks_t *cb,
                             const uint8_t *data, uint8_t length);

/* ============================================================================
 * TX Functions (implemented in lcb_device_data_tx.c)
 *
 * Each function builds a sub-command packet and sends it via
 * lcb_comm_set_lcb_device_data() (command 0x50).
 * ============================================================================ */

/* Sub-Command 0x01: GetEEPromMemorySizes -- send [0x01, 0x00, 0x00] */
void lcb_dd_get_eeprom_memory_sizes(lcb_comm_t *comm);

/* Sub-Command 0x02: GetEEPromMemoryContent -- send [0x02, addr[0], addr[1]] */
void lcb_dd_get_eeprom_memory_content(lcb_comm_t *comm, const uint8_t *address);

/* Sub-Command 0x03: GetECBCurrent -- send [0x03, 0x00, 0x00] */
void lcb_dd_get_ecb_current(lcb_comm_t *comm);

/* Sub-Command 0x04: GetEStopVale -- send [0x04, 0x00, 0x00] with queue mode */
void lcb_dd_get_estop_vale(lcb_comm_t *comm, lcb_queue_mode_t mode);

/* Sub-Command 0x05: GetDCBusStatus -- send [0x05, 0x00, 0x00] */
void lcb_dd_get_dc_bus_status(lcb_comm_t *comm);

/* Sub-Command 0x06: GetLCBversion -- send [0x06, 0x00, 0x00] */
void lcb_dd_get_lcb_version(lcb_comm_t *comm);

/* Sub-Command 0x06: GetLCBversion (highest priority) */
void lcb_dd_get_lcb_version_highest_priority(lcb_comm_t *comm);

/* Sub-Command 0x07: GetControlZoneInfo -- send [0x07, 0x00, 0x00] */
void lcb_dd_get_control_zone_info(lcb_comm_t *comm);

/* Sub-Command 0x08: GetUpdateLcbMcuType -- send [0x08] */
void lcb_dd_get_update_lcb_mcu_type(lcb_comm_t *comm);

/* Sub-Command 0x09: GetUpdateLcbModuleName -- send [0x09] */
void lcb_dd_get_update_lcb_module_name(lcb_comm_t *comm);

/* Sub-Command 0x0A: GetUpdateLcbBrand -- send [0x0A] */
void lcb_dd_get_update_lcb_brand(lcb_comm_t *comm);

/* Sub-Command 0x0B: GetUpdateLcbBootVersion -- send [0x0B] */
void lcb_dd_get_update_lcb_boot_version(lcb_comm_t *comm);

/* Sub-Command 0x0C: GetUpdateBinAddrModuleName -- send [0x0C] */
void lcb_dd_get_update_bin_addr_module_name(lcb_comm_t *comm);

/* Sub-Command 0x0D: GetUpdateBinAddrMcuType -- send [0x0D] */
void lcb_dd_get_update_bin_addr_mcu_type(lcb_comm_t *comm);

/* Sub-Command 0x0E: GetUpdateBinAddrBrand -- send [0x0E] */
void lcb_dd_get_update_bin_addr_brand(lcb_comm_t *comm);

/* Sub-Command 0x0F: GetUpdateBinAddrBootVersion -- send [0x0F] */
void lcb_dd_get_update_bin_addr_boot_version(lcb_comm_t *comm);

/* Sub-Command 0x11: GetUpdateMappingAddrBinToMcu -- send [0x11] */
void lcb_dd_get_update_mapping_addr_bin_to_mcu(lcb_comm_t *comm);

/* Sub-Command 0x70: SoftwareReset -- send [0x70] */
void lcb_dd_software_reset(lcb_comm_t *comm);

/* Sub-Command 0x71: EraseLoaderMemory -- send [0x71, address[0..7]] */
void lcb_dd_erase_loader_memory(lcb_comm_t *comm, const uint8_t *address);

/* Sub-Command 0x72: WriteWordLoaderData -- send [0x72, data[0..N-1]] */
void lcb_dd_write_word_loader_data(lcb_comm_t *comm, const uint8_t *data, uint8_t length);

/* Sub-Command 0x73: ReadByteData -- send [0x73, addr_len[0..4]] */
void lcb_dd_read_byte_data(lcb_comm_t *comm, const uint8_t *address_len);

/* Sub-Command 0x80: SavePWM -- send [0x80, group_data[0..2]] */
void lcb_dd_save_pwm(lcb_comm_t *comm, const uint8_t *group_data);

/* Sub-Command 0x81: ReadPWM -- send [0x81, group] */
void lcb_dd_read_pwm(lcb_comm_t *comm, uint8_t group);

/* Sub-Command 0x90: SetMcbLcbParameter -- send [0x90, type_data[0..N-1]] */
void lcb_dd_set_mcb_lcb_parameter(lcb_comm_t *comm, const uint8_t *type_data, uint8_t length);

/* Sub-Command 0x91: GetMcbLcbParameter -- send [0x91, type] */
void lcb_dd_get_mcb_lcb_parameter(lcb_comm_t *comm, uint8_t type);

/* Sub-Command 0xF0: GetLCBinformation -- send [0xF0] */
void lcb_dd_get_lcb_information(lcb_comm_t *comm);

/* Sub-Command 0xF0: GetLCBinformation (highest priority) */
void lcb_dd_get_lcb_information_highest_priority(lcb_comm_t *comm);

#ifdef __cplusplus
}
#endif

#endif /* LCB_DEVICE_DATA_H */
