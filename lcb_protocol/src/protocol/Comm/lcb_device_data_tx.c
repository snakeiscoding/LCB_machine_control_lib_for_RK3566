/**
 * @file lcb_device_data_tx.c
 * @brief LCB Device Data Sub-Command TX (Transmit) Functions
 *
 * Converted from: com/jht/lcbcommlib/DeviceDataTransmitted.java
 *
 * Each function builds a sub-command data array [sub_cmd_id, params...]
 * and sends it via lcb_comm_set_lcb_device_data() (command 0x50).
 */

#include "lcb_device_data.h"
#include "lcb_logging.h"
#include <string.h>

static const char *TAG __attribute__((unused)) = "LcbDeviceDataTx";

/* ============================================================================
 * Sub-Command 0x01: GetEEPromMemorySizes
 * Send: [0x01, 0x00, 0x00]
 * ============================================================================ */
void lcb_dd_get_eeprom_memory_sizes(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_01];
    data[0] = LCB_DD_CMD_GET_EEPROM_MEMORY_SIZES;
    data[1] = 0x00;
    data[2] = 0x00;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x02: GetEEPromMemoryContent
 * Send: [0x02, addr[0], addr[1]]
 * ============================================================================ */
void lcb_dd_get_eeprom_memory_content(lcb_comm_t *comm, const uint8_t *address)
{
    if (address == NULL) return;
    uint8_t data[LCB_DD_SEND_SIZE_CMD_02];
    data[0] = LCB_DD_CMD_GET_EEPROM_MEMORY_CONTENT;
    data[1] = address[0];
    data[2] = address[1];
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x03: GetECBCurrent
 * Send: [0x03, 0x00, 0x00]
 * ============================================================================ */
void lcb_dd_get_ecb_current(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_03];
    data[0] = LCB_DD_CMD_GET_ECB_CURRENT;
    data[1] = 0x00;
    data[2] = 0x00;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x04: GetEStopVale
 * Send: [0x04, 0x00, 0x00] with queue mode
 * ============================================================================ */
void lcb_dd_get_estop_vale(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_04];
    data[0] = LCB_DD_CMD_GET_ESTOP_VALE;
    data[1] = 0x00;
    data[2] = 0x00;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), mode);
}

/* ============================================================================
 * Sub-Command 0x05: GetDCBusStatus
 * Send: [0x05, 0x00, 0x00]
 * ============================================================================ */
void lcb_dd_get_dc_bus_status(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_05];
    data[0] = LCB_DD_CMD_GET_DC_BUS_STATUS;
    data[1] = 0x00;
    data[2] = 0x00;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x06: GetLCBversion
 * Send: [0x06, 0x00, 0x00]
 * ============================================================================ */
void lcb_dd_get_lcb_version(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_06];
    data[0] = LCB_DD_CMD_GET_LCB_VERSION;
    data[1] = 0x00;
    data[2] = 0x00;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0x06: GetLCBversion (highest priority) */
void lcb_dd_get_lcb_version_highest_priority(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_06];
    data[0] = LCB_DD_CMD_GET_LCB_VERSION;
    data[1] = 0x00;
    data[2] = 0x00;
    lcb_comm_set_lcb_device_data_highest_priority(comm, data, sizeof(data));
}

/* ============================================================================
 * Sub-Command 0x07: GetControlZoneInfo
 * Send: [0x07, 0x00, 0x00]
 * ============================================================================ */
void lcb_dd_get_control_zone_info(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_07];
    data[0] = LCB_DD_CMD_GET_CONTROL_ZONE_INFO;
    data[1] = 0x00;
    data[2] = 0x00;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x08: GetUpdateLcbMcuType
 * Send: [0x08]
 * ============================================================================ */
void lcb_dd_get_update_lcb_mcu_type(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_08];
    data[0] = LCB_DD_CMD_GET_UPDATE_LCB_MCU_TYPE;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x09: GetUpdateLcbModuleName
 * Send: [0x09]
 * ============================================================================ */
void lcb_dd_get_update_lcb_module_name(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_09];
    data[0] = LCB_DD_CMD_GET_UPDATE_LCB_MODULE_NAME;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x0A: GetUpdateLcbBrand
 * Send: [0x0A]
 * ============================================================================ */
void lcb_dd_get_update_lcb_brand(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_0A];
    data[0] = LCB_DD_CMD_GET_UPDATE_LCB_BRAND;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x0B: GetUpdateLcbBootVersion
 * Send: [0x0B]
 * ============================================================================ */
void lcb_dd_get_update_lcb_boot_version(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_0B];
    data[0] = LCB_DD_CMD_GET_UPDATE_LCB_BOOT_VERSION;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x0C: GetUpdateBinAddrModuleName
 * Send: [0x0C]
 * ============================================================================ */
void lcb_dd_get_update_bin_addr_module_name(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_0C];
    data[0] = LCB_DD_CMD_GET_UPDATE_BIN_ADDR_MODULE_NAME;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x0D: GetUpdateBinAddrMcuType
 * Send: [0x0D]
 * ============================================================================ */
void lcb_dd_get_update_bin_addr_mcu_type(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_0D];
    data[0] = LCB_DD_CMD_GET_UPDATE_BIN_ADDR_MCU_TYPE;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x0E: GetUpdateBinAddrBrand
 * Send: [0x0E]
 * ============================================================================ */
void lcb_dd_get_update_bin_addr_brand(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_0E];
    data[0] = LCB_DD_CMD_GET_UPDATE_BIN_ADDR_BRAND;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x0F: GetUpdateBinAddrBootVersion
 * Send: [0x0F]
 * ============================================================================ */
void lcb_dd_get_update_bin_addr_boot_version(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_0F];
    data[0] = LCB_DD_CMD_GET_UPDATE_BIN_ADDR_BOOT_VERSION;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x11: GetUpdateMappingAddrBinToMcu
 * Send: [0x11]
 * ============================================================================ */
void lcb_dd_get_update_mapping_addr_bin_to_mcu(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_11];
    data[0] = LCB_DD_CMD_GET_UPDATE_MAPPING_ADDR_BIN_TO_MCU;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x70: SoftwareReset
 * Send: [0x70]
 * ============================================================================ */
void lcb_dd_software_reset(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_70];
    data[0] = LCB_DD_CMD_SOFTWARE_RESET;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x71: EraseLoaderMemory
 * Send: [0x71, address[0..7]]
 * ============================================================================ */
void lcb_dd_erase_loader_memory(lcb_comm_t *comm, const uint8_t *address)
{
    if (address == NULL) return;
    uint8_t data[LCB_DD_SEND_SIZE_CMD_71];
    data[0] = LCB_DD_CMD_ERASE_LOADER_MEMORY;
    memcpy(&data[1], address, LCB_DD_SEND_SIZE_CMD_71 - 1);
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x72: WriteWordLoaderData
 * Send: [0x72, data[0..N-1]]  (variable length)
 * ============================================================================ */
void lcb_dd_write_word_loader_data(lcb_comm_t *comm, const uint8_t *payload, uint8_t length)
{
    if (payload == NULL || length == 0) return;
    uint8_t data[255];
    uint8_t send_len = (length < 254) ? (uint8_t)(length + 1) : 255;

    data[0] = LCB_DD_CMD_WRITE_WORD_LOADER_DATA;
    memcpy(&data[1], payload, send_len - 1);
    lcb_comm_set_lcb_device_data(comm, data, send_len, LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x73: ReadByteData
 * Send: [0x73, addr_len[0..4]]
 * ============================================================================ */
void lcb_dd_read_byte_data(lcb_comm_t *comm, const uint8_t *address_len)
{
    if (address_len == NULL) return;
    uint8_t data[LCB_DD_SEND_SIZE_CMD_73];
    data[0] = LCB_DD_CMD_READ_BYTE_DATA;
    memcpy(&data[1], address_len, LCB_DD_SEND_SIZE_CMD_73 - 1);
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x80: SavePWM
 * Send: [0x80, group_data[0..2]]
 * ============================================================================ */
void lcb_dd_save_pwm(lcb_comm_t *comm, const uint8_t *group_data)
{
    if (group_data == NULL) return;
    uint8_t data[LCB_DD_SEND_SIZE_CMD_80];
    data[0] = LCB_DD_CMD_SAVE_PWM;
    memcpy(&data[1], group_data, LCB_DD_SEND_SIZE_CMD_80 - 1);
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x81: ReadPWM
 * Send: [0x81, group]
 * ============================================================================ */
void lcb_dd_read_pwm(lcb_comm_t *comm, uint8_t group)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_81];
    data[0] = LCB_DD_CMD_READ_PWM;
    data[1] = group;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x90: SetMcbLcbParameter
 * Send: [0x90, type_data[0..N-1]]  (variable length)
 * ============================================================================ */
void lcb_dd_set_mcb_lcb_parameter(lcb_comm_t *comm, const uint8_t *type_data, uint8_t length)
{
    if (type_data == NULL || length == 0) return;
    uint8_t data[255];
    uint8_t send_len = (length < 254) ? (uint8_t)(length + 1) : 255;

    data[0] = LCB_DD_CMD_SET_MCB_LCB_PARAMETER;
    memcpy(&data[1], type_data, send_len - 1);
    lcb_comm_set_lcb_device_data(comm, data, send_len, LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0x91: GetMcbLcbParameter
 * Send: [0x91, type]
 * ============================================================================ */
void lcb_dd_get_mcb_lcb_parameter(lcb_comm_t *comm, uint8_t type)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_91];
    data[0] = LCB_DD_CMD_GET_MCB_LCB_PARAMETER;
    data[1] = type;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* ============================================================================
 * Sub-Command 0xF0: GetLCBinformation
 * Send: [0xF0]
 * ============================================================================ */
void lcb_dd_get_lcb_information(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_F0];
    data[0] = LCB_DD_CMD_GET_LCB_INFORMATION;
    lcb_comm_set_lcb_device_data(comm, data, sizeof(data), LCB_QUEUE_MODE_ALWAYS_ADD);
}

/* Sub-Command 0xF0: GetLCBinformation (highest priority) */
void lcb_dd_get_lcb_information_highest_priority(lcb_comm_t *comm)
{
    uint8_t data[LCB_DD_SEND_SIZE_CMD_F0];
    data[0] = LCB_DD_CMD_GET_LCB_INFORMATION;
    lcb_comm_set_lcb_device_data_highest_priority(comm, data, sizeof(data));
}
