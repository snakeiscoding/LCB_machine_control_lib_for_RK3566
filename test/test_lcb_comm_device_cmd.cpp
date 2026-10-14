/**
 * @file test_lcb_comm_device_cmd.cpp
 * @brief Unit tests for LCB Device Data sub-command handlers (0x50)
 *
 * Tests lcb_device_data_analyze() dispatcher + all 25 sub-command callbacks,
 * plus TX function NULL safety.
 *
 * NOTE: struct name DDTestContext is unique to avoid ODR violations with
 * CmdTestContext in test_lcb_comm_cmd.cpp.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "lcb_device_data.h"
}

/* ============================================================================
 * Test Context -- captures all 25 callback invocations
 * ============================================================================ */

struct DDTestContext {
    /* 0x01: GetEEPromMemorySizes -- (void*, int) */
    int on_dd_eeprom_sizes_count;
    int last_memory_sizes;

    /* 0x02: GetEEPromMemoryContent -- (void*, const uint8_t*, uint8_t) */
    int on_dd_eeprom_content_count;
    uint8_t last_eeprom_content_data[256];
    uint8_t last_eeprom_content_len;

    /* 0x03: GetECBCurrent */
    int on_dd_ecb_current_count;
    uint8_t last_ecb_current_data[256];
    uint8_t last_ecb_current_len;

    /* 0x04: GetEStopVale */
    int on_dd_estop_vale_count;
    uint8_t last_estop_vale_data[256];
    uint8_t last_estop_vale_len;

    /* 0x05: GetDCBusStatus */
    int on_dd_dc_bus_status_count;
    uint8_t last_dc_bus_data[256];
    uint8_t last_dc_bus_len;

    /* 0x06: GetLCBversion */
    int on_dd_lcb_version_count;
    uint8_t last_lcb_version_data[256];
    uint8_t last_lcb_version_len;

    /* 0x07: GetControlZoneInfo */
    int on_dd_zone_info_count;
    uint8_t last_zone_info_data[256];
    uint8_t last_zone_info_len;

    /* 0x08: GetUpdateLcbMcuType */
    int on_dd_mcu_type_count;
    uint8_t last_mcu_type_data[256];
    uint8_t last_mcu_type_len;

    /* 0x09: GetUpdateLcbModuleName */
    int on_dd_module_name_count;
    uint8_t last_module_name_data[256];
    uint8_t last_module_name_len;

    /* 0x0A: GetUpdateLcbBrand */
    int on_dd_brand_count;
    uint8_t last_brand_data[256];
    uint8_t last_brand_len;

    /* 0x0B: GetUpdateLcbBootVersion */
    int on_dd_boot_version_count;
    uint8_t last_boot_version_data[256];
    uint8_t last_boot_version_len;

    /* 0x0C: GetUpdateBinAddrModuleName */
    int on_dd_bin_module_name_count;
    uint8_t last_bin_module_name_data[256];
    uint8_t last_bin_module_name_len;

    /* 0x0D: GetUpdateBinAddrMcuType */
    int on_dd_bin_mcu_type_count;
    uint8_t last_bin_mcu_type_data[256];
    uint8_t last_bin_mcu_type_len;

    /* 0x0E: GetUpdateBinAddrBrand */
    int on_dd_bin_brand_count;
    uint8_t last_bin_brand_data[256];
    uint8_t last_bin_brand_len;

    /* 0x0F: GetUpdateBinAddrBootVersion */
    int on_dd_bin_boot_version_count;
    uint8_t last_bin_boot_version_data[256];
    uint8_t last_bin_boot_version_len;

    /* 0x11: GetUpdateMappingAddrBinToMcu */
    int on_dd_mapping_addr_count;
    uint8_t last_mapping_addr_data[256];
    uint8_t last_mapping_addr_len;

    /* 0x70: SoftwareReset -- (void*) */
    int on_dd_software_reset_count;

    /* 0x71: EraseLoaderMemory -- (void*, uint8_t) */
    int on_dd_erase_loader_count;
    uint8_t last_erase_status;

    /* 0x72: WriteWordLoaderData -- (void*, uint8_t) */
    int on_dd_write_word_count;
    uint8_t last_write_word_status;

    /* 0x73: ReadByteData */
    int on_dd_read_byte_count;
    uint8_t last_read_byte_data[256];
    uint8_t last_read_byte_len;

    /* 0x80: SavePWM -- (void*, uint8_t) */
    int on_dd_save_pwm_count;
    uint8_t last_save_pwm_status;

    /* 0x81: ReadPWM */
    int on_dd_read_pwm_count;
    uint8_t last_read_pwm_data[256];
    uint8_t last_read_pwm_len;

    /* 0x90: SetMcbLcbParameter -- (void*, uint8_t) */
    int on_dd_set_param_count;
    uint8_t last_set_param_status;

    /* 0x91: GetMcbLcbParameter */
    int on_dd_get_param_count;
    uint8_t last_get_param_data[256];
    uint8_t last_get_param_len;

    /* 0xF0: GetLCBinformation */
    int on_dd_lcb_info_count;
    uint8_t last_lcb_info_data[256];
    uint8_t last_lcb_info_len;

    void reset() { memset(this, 0, sizeof(*this)); }
};

/* ============================================================================
 * Trampoline callbacks
 * ============================================================================ */

/* 0x01 */
static void dd_cb_eeprom_sizes(void *ud, int memory_sizes) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_eeprom_sizes_count++;
    ctx->last_memory_sizes = memory_sizes;
}

/* 0x02 */
static void dd_cb_eeprom_content(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_eeprom_content_count++;
    ctx->last_eeprom_content_len = len;
    if (data)
        memcpy(ctx->last_eeprom_content_data, data, len);
}

/* 0x03 */
static void dd_cb_ecb_current(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_ecb_current_count++;
    ctx->last_ecb_current_len = len;
    if (data)
        memcpy(ctx->last_ecb_current_data, data, len);
}

/* 0x04 */
static void dd_cb_estop_vale(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_estop_vale_count++;
    ctx->last_estop_vale_len = len;
    if (data)
        memcpy(ctx->last_estop_vale_data, data, len);
}

/* 0x05 */
static void dd_cb_dc_bus_status(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_dc_bus_status_count++;
    ctx->last_dc_bus_len = len;
    if (data)
        memcpy(ctx->last_dc_bus_data, data, len);
}

/* 0x06 */
static void dd_cb_lcb_version(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_lcb_version_count++;
    ctx->last_lcb_version_len = len;
    if (data)
        memcpy(ctx->last_lcb_version_data, data, len);
}

/* 0x07 */
static void dd_cb_zone_info(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_zone_info_count++;
    ctx->last_zone_info_len = len;
    if (data)
        memcpy(ctx->last_zone_info_data, data, len);
}

/* 0x08 */
static void dd_cb_mcu_type(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_mcu_type_count++;
    ctx->last_mcu_type_len = len;
    if (data)
        memcpy(ctx->last_mcu_type_data, data, len);
}

/* 0x09 */
static void dd_cb_module_name(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_module_name_count++;
    ctx->last_module_name_len = len;
    if (data)
        memcpy(ctx->last_module_name_data, data, len);
}

/* 0x0A */
static void dd_cb_brand(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_brand_count++;
    ctx->last_brand_len = len;
    if (data)
        memcpy(ctx->last_brand_data, data, len);
}

/* 0x0B */
static void dd_cb_boot_version(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_boot_version_count++;
    ctx->last_boot_version_len = len;
    if (data)
        memcpy(ctx->last_boot_version_data, data, len);
}

/* 0x0C */
static void dd_cb_bin_module_name(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_bin_module_name_count++;
    ctx->last_bin_module_name_len = len;
    if (data)
        memcpy(ctx->last_bin_module_name_data, data, len);
}

/* 0x0D */
static void dd_cb_bin_mcu_type(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_bin_mcu_type_count++;
    ctx->last_bin_mcu_type_len = len;
    if (data)
        memcpy(ctx->last_bin_mcu_type_data, data, len);
}

/* 0x0E */
static void dd_cb_bin_brand(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_bin_brand_count++;
    ctx->last_bin_brand_len = len;
    if (data)
        memcpy(ctx->last_bin_brand_data, data, len);
}

/* 0x0F */
static void dd_cb_bin_boot_version(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_bin_boot_version_count++;
    ctx->last_bin_boot_version_len = len;
    if (data)
        memcpy(ctx->last_bin_boot_version_data, data, len);
}

/* 0x11 */
static void dd_cb_mapping_addr(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_mapping_addr_count++;
    ctx->last_mapping_addr_len = len;
    if (data)
        memcpy(ctx->last_mapping_addr_data, data, len);
}

/* 0x70 */
static void dd_cb_software_reset(void *ud) {
    ((DDTestContext*)ud)->on_dd_software_reset_count++;
}

/* 0x71 */
static void dd_cb_erase_loader(void *ud, uint8_t status) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_erase_loader_count++;
    ctx->last_erase_status = status;
}

/* 0x72 */
static void dd_cb_write_word(void *ud, uint8_t status) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_write_word_count++;
    ctx->last_write_word_status = status;
}

/* 0x73 */
static void dd_cb_read_byte(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_read_byte_count++;
    ctx->last_read_byte_len = len;
    if (data)
        memcpy(ctx->last_read_byte_data, data, len);
}

/* 0x80 */
static void dd_cb_save_pwm(void *ud, uint8_t status) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_save_pwm_count++;
    ctx->last_save_pwm_status = status;
}

/* 0x81 */
static void dd_cb_read_pwm(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_read_pwm_count++;
    ctx->last_read_pwm_len = len;
    if (data)
        memcpy(ctx->last_read_pwm_data, data, len);
}

/* 0x90 */
static void dd_cb_set_param(void *ud, uint8_t status) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_set_param_count++;
    ctx->last_set_param_status = status;
}

/* 0x91 */
static void dd_cb_get_param(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_get_param_count++;
    ctx->last_get_param_len = len;
    if (data)
        memcpy(ctx->last_get_param_data, data, len);
}

/* 0xF0 */
static void dd_cb_lcb_info(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (DDTestContext*)ud;
    ctx->on_dd_lcb_info_count++;
    ctx->last_lcb_info_len = len;
    if (data)
        memcpy(ctx->last_lcb_info_data, data, len);
}

/* ============================================================================
 * Setup helper
 * ============================================================================ */

static void setup_dd_callbacks(lcb_device_data_callbacks_t *cb, DDTestContext *ctx) {
    lcb_device_data_callbacks_init(cb);
    cb->user_data = ctx;
    cb->on_dd_get_eeprom_memory_sizes       = dd_cb_eeprom_sizes;
    cb->on_dd_get_eeprom_memory_content     = dd_cb_eeprom_content;
    cb->on_dd_get_ecb_current               = dd_cb_ecb_current;
    cb->on_dd_get_estop_vale                = dd_cb_estop_vale;
    cb->on_dd_get_dc_bus_status             = dd_cb_dc_bus_status;
    cb->on_dd_get_lcb_version               = dd_cb_lcb_version;
    cb->on_dd_get_control_zone_info         = dd_cb_zone_info;
    cb->on_dd_get_update_lcb_mcu_type       = dd_cb_mcu_type;
    cb->on_dd_get_update_lcb_module_name    = dd_cb_module_name;
    cb->on_dd_get_update_lcb_brand          = dd_cb_brand;
    cb->on_dd_get_update_lcb_boot_version   = dd_cb_boot_version;
    cb->on_dd_get_update_bin_addr_module_name = dd_cb_bin_module_name;
    cb->on_dd_get_update_bin_addr_mcu_type  = dd_cb_bin_mcu_type;
    cb->on_dd_get_update_bin_addr_brand     = dd_cb_bin_brand;
    cb->on_dd_get_update_bin_addr_boot_version = dd_cb_bin_boot_version;
    cb->on_dd_get_update_mapping_addr_bin_to_mcu = dd_cb_mapping_addr;
    cb->on_dd_software_reset                = dd_cb_software_reset;
    cb->on_dd_erase_loader_memory           = dd_cb_erase_loader;
    cb->on_dd_write_word_loader_data        = dd_cb_write_word;
    cb->on_dd_read_byte_data                = dd_cb_read_byte;
    cb->on_dd_save_pwm                      = dd_cb_save_pwm;
    cb->on_dd_read_pwm                      = dd_cb_read_pwm;
    cb->on_dd_set_mcb_lcb_parameter         = dd_cb_set_param;
    cb->on_dd_get_mcb_lcb_parameter         = dd_cb_get_param;
    cb->on_dd_get_lcb_information           = dd_cb_lcb_info;
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

class LcbDeviceDataTest : public ::testing::Test {
protected:
    DDTestContext ctx;
    lcb_device_data_callbacks_t cb;

    void SetUp() override {
        ctx.reset();
        setup_dd_callbacks(&cb, &ctx);
    }
};

/* ============================================================================
 * Initialization & Dispatcher Edge Cases
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, CallbacksInit_ZerosAllFields) {
    lcb_device_data_callbacks_t test_cb;
    memset(&test_cb, 0xFF, sizeof(test_cb));
    lcb_device_data_callbacks_init(&test_cb);
    EXPECT_EQ(test_cb.user_data, nullptr);
    EXPECT_EQ(test_cb.on_dd_get_eeprom_memory_sizes, nullptr);
    EXPECT_EQ(test_cb.on_dd_software_reset, nullptr);
    EXPECT_EQ(test_cb.on_dd_get_lcb_information, nullptr);
}

TEST_F(LcbDeviceDataTest, CallbacksInit_NullSafe) {
    lcb_device_data_callbacks_init(NULL);
}

TEST_F(LcbDeviceDataTest, Analyze_NullCb_NoCrash) {
    uint8_t data[] = {0x01, 0x00, 0x00};
    lcb_device_data_analyze(NULL, data, 3);
}

TEST_F(LcbDeviceDataTest, Analyze_NullData_NoCrash) {
    lcb_device_data_analyze(&cb, NULL, 5);
}

TEST_F(LcbDeviceDataTest, Analyze_ZeroLength_NoCrash) {
    uint8_t data[] = {0x01};
    lcb_device_data_analyze(&cb, data, 0);
}

TEST_F(LcbDeviceDataTest, Analyze_UnknownSubCmd_NoCrash) {
    uint8_t data[] = {0xFF};
    lcb_device_data_analyze(&cb, data, 1);
    /* No callback should have been invoked */
    EXPECT_EQ(ctx.on_dd_eeprom_sizes_count, 0);
    EXPECT_EQ(ctx.on_dd_software_reset_count, 0);
}

/* ============================================================================
 * 0x01 GetEEPromMemorySizes (Special: LE int from 2 bytes)
 * recv size = 3: [0x01, low_byte, high_byte]
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x01_EepromMemorySizes_Normal) {
    uint8_t data[] = {0x01, 0x34, 0x12};  /* LE: 0x1234 */
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_eeprom_sizes_count, 1);
    EXPECT_EQ(ctx.last_memory_sizes, 0x1234);
}

TEST_F(LcbDeviceDataTest, DD_0x01_EepromMemorySizes_ZeroValue) {
    uint8_t data[] = {0x01, 0x00, 0x00};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_eeprom_sizes_count, 1);
    EXPECT_EQ(ctx.last_memory_sizes, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x01_EepromMemorySizes_WrongSize_Ignored) {
    uint8_t data[] = {0x01, 0x34};  /* Only 2 bytes, need 3 */
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_eeprom_sizes_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x01_EepromMemorySizes_NullCallback_NoCrash) {
    cb.on_dd_get_eeprom_memory_sizes = NULL;
    uint8_t data[] = {0x01, 0x34, 0x12};
    lcb_device_data_analyze(&cb, data, 3);
}

/* ============================================================================
 * 0x02 GetEEPromMemoryContent (Size-validated, recv=17)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x02_EepromContent_Normal) {
    uint8_t data[17];
    data[0] = 0x02;
    for (int i = 1; i < 17; i++) data[i] = (uint8_t)(i * 0x11);
    lcb_device_data_analyze(&cb, data, 17);
    EXPECT_EQ(ctx.on_dd_eeprom_content_count, 1);
    EXPECT_EQ(ctx.last_eeprom_content_len, 16);
    EXPECT_EQ(memcmp(ctx.last_eeprom_content_data, data + 1, 16), 0);
}

TEST_F(LcbDeviceDataTest, DD_0x02_EepromContent_WrongSize_Ignored) {
    uint8_t data[10];
    data[0] = 0x02;
    lcb_device_data_analyze(&cb, data, 10);
    EXPECT_EQ(ctx.on_dd_eeprom_content_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x02_EepromContent_NullCallback_NoCrash) {
    cb.on_dd_get_eeprom_memory_content = NULL;
    uint8_t data[17] = {0x02};
    lcb_device_data_analyze(&cb, data, 17);
}

/* ============================================================================
 * 0x03 GetECBCurrent (Size-validated, recv=5)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x03_EcbCurrent_Normal) {
    uint8_t data[] = {0x03, 0xAA, 0xBB, 0xCC, 0xDD};
    lcb_device_data_analyze(&cb, data, 5);
    EXPECT_EQ(ctx.on_dd_ecb_current_count, 1);
    EXPECT_EQ(ctx.last_ecb_current_len, 4);
    EXPECT_EQ(ctx.last_ecb_current_data[0], 0xAA);
    EXPECT_EQ(ctx.last_ecb_current_data[3], 0xDD);
}

TEST_F(LcbDeviceDataTest, DD_0x03_EcbCurrent_WrongSize_Ignored) {
    uint8_t data[] = {0x03, 0xAA, 0xBB};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_ecb_current_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x03_EcbCurrent_NullCallback_NoCrash) {
    cb.on_dd_get_ecb_current = NULL;
    uint8_t data[] = {0x03, 0xAA, 0xBB, 0xCC, 0xDD};
    lcb_device_data_analyze(&cb, data, 5);
}

/* ============================================================================
 * 0x04 GetEStopVale (Size-validated, recv=5)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x04_EstopVale_Normal) {
    uint8_t data[] = {0x04, 0x01, 0x02, 0x03, 0x04};
    lcb_device_data_analyze(&cb, data, 5);
    EXPECT_EQ(ctx.on_dd_estop_vale_count, 1);
    EXPECT_EQ(ctx.last_estop_vale_len, 4);
    EXPECT_EQ(ctx.last_estop_vale_data[0], 0x01);
}

TEST_F(LcbDeviceDataTest, DD_0x04_EstopVale_WrongSize_Ignored) {
    uint8_t data[] = {0x04, 0x01, 0x02, 0x03};
    lcb_device_data_analyze(&cb, data, 4);
    EXPECT_EQ(ctx.on_dd_estop_vale_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x04_EstopVale_NullCallback_NoCrash) {
    cb.on_dd_get_estop_vale = NULL;
    uint8_t data[] = {0x04, 0x01, 0x02, 0x03, 0x04};
    lcb_device_data_analyze(&cb, data, 5);
}

/* ============================================================================
 * 0x05 GetDCBusStatus (Size-validated, recv=5)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x05_DcBusStatus_Normal) {
    uint8_t data[] = {0x05, 0x10, 0x20, 0x30, 0x40};
    lcb_device_data_analyze(&cb, data, 5);
    EXPECT_EQ(ctx.on_dd_dc_bus_status_count, 1);
    EXPECT_EQ(ctx.last_dc_bus_len, 4);
    EXPECT_EQ(ctx.last_dc_bus_data[0], 0x10);
}

TEST_F(LcbDeviceDataTest, DD_0x05_DcBusStatus_WrongSize_Ignored) {
    uint8_t data[] = {0x05, 0x10, 0x20, 0x30, 0x40, 0x50};
    lcb_device_data_analyze(&cb, data, 6);
    EXPECT_EQ(ctx.on_dd_dc_bus_status_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x05_DcBusStatus_NullCallback_NoCrash) {
    cb.on_dd_get_dc_bus_status = NULL;
    uint8_t data[] = {0x05, 0x10, 0x20, 0x30, 0x40};
    lcb_device_data_analyze(&cb, data, 5);
}

/* ============================================================================
 * 0x06 GetLCBversion (Variable-length, min 2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x06_LcbVersion_Normal) {
    uint8_t data[] = {0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    lcb_device_data_analyze(&cb, data, 9);
    EXPECT_EQ(ctx.on_dd_lcb_version_count, 1);
    EXPECT_EQ(ctx.last_lcb_version_len, 8);
    EXPECT_EQ(ctx.last_lcb_version_data[0], 0x01);
    EXPECT_EQ(ctx.last_lcb_version_data[7], 0x08);
}

TEST_F(LcbDeviceDataTest, DD_0x06_LcbVersion_MinSize) {
    uint8_t data[] = {0x06, 0xAA};
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_lcb_version_count, 1);
    EXPECT_EQ(ctx.last_lcb_version_len, 1);
    EXPECT_EQ(ctx.last_lcb_version_data[0], 0xAA);
}

TEST_F(LcbDeviceDataTest, DD_0x06_LcbVersion_TooShort_Ignored) {
    uint8_t data[] = {0x06};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_lcb_version_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x06_LcbVersion_NullCallback_NoCrash) {
    cb.on_dd_get_lcb_version = NULL;
    uint8_t data[] = {0x06, 0x01, 0x02};
    lcb_device_data_analyze(&cb, data, 3);
}

/* ============================================================================
 * 0x07 GetControlZoneInfo (Special: zone count + zone data)
 * Output: [zone_count, zone_data_bytes...]
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x07_ZoneInfo_Normal) {
    /* 2 zones, each 2 bytes */
    uint8_t data[] = {0x07, 0x02, 0xA1, 0xA2, 0xB1, 0xB2};
    lcb_device_data_analyze(&cb, data, 6);
    EXPECT_EQ(ctx.on_dd_zone_info_count, 1);
    EXPECT_EQ(ctx.last_zone_info_len, 5);  /* 1 (zone_count) + 4 (2*2) */
    EXPECT_EQ(ctx.last_zone_info_data[0], 0x02);  /* zone count */
    EXPECT_EQ(ctx.last_zone_info_data[1], 0xA1);
    EXPECT_EQ(ctx.last_zone_info_data[4], 0xB2);
}

TEST_F(LcbDeviceDataTest, DD_0x07_ZoneInfo_ZeroZones) {
    uint8_t data[] = {0x07, 0x00};
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_zone_info_count, 1);
    EXPECT_EQ(ctx.last_zone_info_len, 1);  /* just zone_count byte */
    EXPECT_EQ(ctx.last_zone_info_data[0], 0x00);
}

TEST_F(LcbDeviceDataTest, DD_0x07_ZoneInfo_PartialData) {
    /* zone_count=3 expects 6 bytes but only 2 available */
    uint8_t data[] = {0x07, 0x03, 0xAA, 0xBB};
    lcb_device_data_analyze(&cb, data, 4);
    EXPECT_EQ(ctx.on_dd_zone_info_count, 1);
    EXPECT_EQ(ctx.last_zone_info_len, 3);  /* 1 + min(2, 6) = 3 */
    EXPECT_EQ(ctx.last_zone_info_data[0], 0x03);
    EXPECT_EQ(ctx.last_zone_info_data[1], 0xAA);
    EXPECT_EQ(ctx.last_zone_info_data[2], 0xBB);
}

TEST_F(LcbDeviceDataTest, DD_0x07_ZoneInfo_TooShort_Ignored) {
    uint8_t data[] = {0x07};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_zone_info_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x07_ZoneInfo_NullCallback_NoCrash) {
    cb.on_dd_get_control_zone_info = NULL;
    uint8_t data[] = {0x07, 0x01, 0xAA, 0xBB};
    lcb_device_data_analyze(&cb, data, 4);
}

/* ============================================================================
 * 0x08 GetUpdateLcbMcuType (Variable-length, min 2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x08_McuType_Normal) {
    uint8_t data[] = {0x08, 0xAA, 0xBB};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_mcu_type_count, 1);
    EXPECT_EQ(ctx.last_mcu_type_len, 2);
    EXPECT_EQ(ctx.last_mcu_type_data[0], 0xAA);
}

TEST_F(LcbDeviceDataTest, DD_0x08_McuType_TooShort_Ignored) {
    uint8_t data[] = {0x08};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_mcu_type_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x08_McuType_NullCallback_NoCrash) {
    cb.on_dd_get_update_lcb_mcu_type = NULL;
    uint8_t data[] = {0x08, 0xAA};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x09 GetUpdateLcbModuleName (Variable-length, min 2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x09_ModuleName_Normal) {
    uint8_t data[] = {0x09, 'A', 'B', 'C'};
    lcb_device_data_analyze(&cb, data, 4);
    EXPECT_EQ(ctx.on_dd_module_name_count, 1);
    EXPECT_EQ(ctx.last_module_name_len, 3);
    EXPECT_EQ(ctx.last_module_name_data[0], 'A');
}

TEST_F(LcbDeviceDataTest, DD_0x09_ModuleName_TooShort_Ignored) {
    uint8_t data[] = {0x09};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_module_name_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x09_ModuleName_NullCallback_NoCrash) {
    cb.on_dd_get_update_lcb_module_name = NULL;
    uint8_t data[] = {0x09, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x0A GetUpdateLcbBrand (Variable-length, min 2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x0A_Brand_Normal) {
    uint8_t data[] = {0x0A, 0x01, 0x02};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_brand_count, 1);
    EXPECT_EQ(ctx.last_brand_len, 2);
}

TEST_F(LcbDeviceDataTest, DD_0x0A_Brand_TooShort_Ignored) {
    uint8_t data[] = {0x0A};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_brand_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0A_Brand_NullCallback_NoCrash) {
    cb.on_dd_get_update_lcb_brand = NULL;
    uint8_t data[] = {0x0A, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x0B GetUpdateLcbBootVersion (Variable-length, min 2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x0B_BootVersion_Normal) {
    uint8_t data[] = {0x0B, 'v', '1'};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_boot_version_count, 1);
    EXPECT_EQ(ctx.last_boot_version_len, 2);
    EXPECT_EQ(ctx.last_boot_version_data[0], 'v');
}

TEST_F(LcbDeviceDataTest, DD_0x0B_BootVersion_TooShort_Ignored) {
    uint8_t data[] = {0x0B};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_boot_version_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0B_BootVersion_NullCallback_NoCrash) {
    cb.on_dd_get_update_lcb_boot_version = NULL;
    uint8_t data[] = {0x0B, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x0C GetUpdateBinAddrModuleName (Size-validated, recv=9)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x0C_BinModuleName_Normal) {
    uint8_t data[9];
    data[0] = 0x0C;
    for (int i = 1; i < 9; i++) data[i] = (uint8_t)i;
    lcb_device_data_analyze(&cb, data, 9);
    EXPECT_EQ(ctx.on_dd_bin_module_name_count, 1);
    EXPECT_EQ(ctx.last_bin_module_name_len, 8);
    EXPECT_EQ(memcmp(ctx.last_bin_module_name_data, data + 1, 8), 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0C_BinModuleName_WrongSize_Ignored) {
    uint8_t data[7] = {0x0C};
    lcb_device_data_analyze(&cb, data, 7);
    EXPECT_EQ(ctx.on_dd_bin_module_name_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0C_BinModuleName_NullCallback_NoCrash) {
    cb.on_dd_get_update_bin_addr_module_name = NULL;
    uint8_t data[9] = {0x0C};
    lcb_device_data_analyze(&cb, data, 9);
}

/* ============================================================================
 * 0x0D GetUpdateBinAddrMcuType (Size-validated, recv=9)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x0D_BinMcuType_Normal) {
    uint8_t data[9];
    data[0] = 0x0D;
    for (int i = 1; i < 9; i++) data[i] = (uint8_t)(0x10 + i);
    lcb_device_data_analyze(&cb, data, 9);
    EXPECT_EQ(ctx.on_dd_bin_mcu_type_count, 1);
    EXPECT_EQ(ctx.last_bin_mcu_type_len, 8);
    EXPECT_EQ(memcmp(ctx.last_bin_mcu_type_data, data + 1, 8), 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0D_BinMcuType_WrongSize_Ignored) {
    uint8_t data[10] = {0x0D};
    lcb_device_data_analyze(&cb, data, 10);
    EXPECT_EQ(ctx.on_dd_bin_mcu_type_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0D_BinMcuType_NullCallback_NoCrash) {
    cb.on_dd_get_update_bin_addr_mcu_type = NULL;
    uint8_t data[9] = {0x0D};
    lcb_device_data_analyze(&cb, data, 9);
}

/* ============================================================================
 * 0x0E GetUpdateBinAddrBrand (Size-validated, recv=9)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x0E_BinBrand_Normal) {
    uint8_t data[9];
    data[0] = 0x0E;
    for (int i = 1; i < 9; i++) data[i] = (uint8_t)(0x20 + i);
    lcb_device_data_analyze(&cb, data, 9);
    EXPECT_EQ(ctx.on_dd_bin_brand_count, 1);
    EXPECT_EQ(ctx.last_bin_brand_len, 8);
    EXPECT_EQ(memcmp(ctx.last_bin_brand_data, data + 1, 8), 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0E_BinBrand_WrongSize_Ignored) {
    uint8_t data[5] = {0x0E};
    lcb_device_data_analyze(&cb, data, 5);
    EXPECT_EQ(ctx.on_dd_bin_brand_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0E_BinBrand_NullCallback_NoCrash) {
    cb.on_dd_get_update_bin_addr_brand = NULL;
    uint8_t data[9] = {0x0E};
    lcb_device_data_analyze(&cb, data, 9);
}

/* ============================================================================
 * 0x0F GetUpdateBinAddrBootVersion (Size-validated, recv=9)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x0F_BinBootVersion_Normal) {
    uint8_t data[9];
    data[0] = 0x0F;
    for (int i = 1; i < 9; i++) data[i] = (uint8_t)(0x30 + i);
    lcb_device_data_analyze(&cb, data, 9);
    EXPECT_EQ(ctx.on_dd_bin_boot_version_count, 1);
    EXPECT_EQ(ctx.last_bin_boot_version_len, 8);
    EXPECT_EQ(memcmp(ctx.last_bin_boot_version_data, data + 1, 8), 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0F_BinBootVersion_WrongSize_Ignored) {
    uint8_t data[8] = {0x0F};
    lcb_device_data_analyze(&cb, data, 8);
    EXPECT_EQ(ctx.on_dd_bin_boot_version_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x0F_BinBootVersion_NullCallback_NoCrash) {
    cb.on_dd_get_update_bin_addr_boot_version = NULL;
    uint8_t data[9] = {0x0F};
    lcb_device_data_analyze(&cb, data, 9);
}

/* ============================================================================
 * 0x11 GetUpdateMappingAddrBinToMcu (Variable-length, min 2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x11_MappingAddr_Normal) {
    uint8_t data[] = {0x11, 0x01, 0x02, 0x03};
    lcb_device_data_analyze(&cb, data, 4);
    EXPECT_EQ(ctx.on_dd_mapping_addr_count, 1);
    EXPECT_EQ(ctx.last_mapping_addr_len, 3);
    EXPECT_EQ(ctx.last_mapping_addr_data[0], 0x01);
}

TEST_F(LcbDeviceDataTest, DD_0x11_MappingAddr_TooShort_Ignored) {
    uint8_t data[] = {0x11};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_mapping_addr_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x11_MappingAddr_NullCallback_NoCrash) {
    cb.on_dd_get_update_mapping_addr_bin_to_mcu = NULL;
    uint8_t data[] = {0x11, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x70 SoftwareReset (Simple ACK, recv=1)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x70_SoftwareReset_Normal) {
    uint8_t data[] = {0x70};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_software_reset_count, 1);
}

TEST_F(LcbDeviceDataTest, DD_0x70_SoftwareReset_WrongSize_Ignored) {
    uint8_t data[] = {0x70, 0x00};
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_software_reset_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x70_SoftwareReset_NullCallback_NoCrash) {
    cb.on_dd_software_reset = NULL;
    uint8_t data[] = {0x70};
    lcb_device_data_analyze(&cb, data, 1);
}

/* ============================================================================
 * 0x71 EraseLoaderMemory (Status byte, recv=2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x71_EraseLoader_Normal) {
    uint8_t data[] = {0x71, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_erase_loader_count, 1);
    EXPECT_EQ(ctx.last_erase_status, 0x01);
}

TEST_F(LcbDeviceDataTest, DD_0x71_EraseLoader_WrongSize_Ignored) {
    uint8_t data[] = {0x71};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_erase_loader_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x71_EraseLoader_NullCallback_NoCrash) {
    cb.on_dd_erase_loader_memory = NULL;
    uint8_t data[] = {0x71, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x72 WriteWordLoaderData (Status byte, recv=2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x72_WriteWord_Normal) {
    uint8_t data[] = {0x72, 0xAB};
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_write_word_count, 1);
    EXPECT_EQ(ctx.last_write_word_status, 0xAB);
}

TEST_F(LcbDeviceDataTest, DD_0x72_WriteWord_WrongSize_Ignored) {
    uint8_t data[] = {0x72, 0x01, 0x02};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_write_word_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x72_WriteWord_NullCallback_NoCrash) {
    cb.on_dd_write_word_loader_data = NULL;
    uint8_t data[] = {0x72, 0xAB};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x73 ReadByteData (Special: status + len + data)
 * Output: [status, len_of_data, actual_data...]
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x73_ReadByte_Normal) {
    uint8_t data[] = {0x73, 0x00, 0x03, 0xAA, 0xBB, 0xCC};
    lcb_device_data_analyze(&cb, data, 6);
    EXPECT_EQ(ctx.on_dd_read_byte_count, 1);
    EXPECT_EQ(ctx.last_read_byte_len, 5);  /* 2 (status+len) + 3 (data) */
    EXPECT_EQ(ctx.last_read_byte_data[0], 0x00);  /* status */
    EXPECT_EQ(ctx.last_read_byte_data[1], 0x03);  /* len_of_data */
    EXPECT_EQ(ctx.last_read_byte_data[2], 0xAA);
    EXPECT_EQ(ctx.last_read_byte_data[4], 0xCC);
}

TEST_F(LcbDeviceDataTest, DD_0x73_ReadByte_ZeroDataLen) {
    uint8_t data[] = {0x73, 0x01, 0x00};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_read_byte_count, 1);
    EXPECT_EQ(ctx.last_read_byte_len, 2);  /* status + len, no data */
    EXPECT_EQ(ctx.last_read_byte_data[0], 0x01);
    EXPECT_EQ(ctx.last_read_byte_data[1], 0x00);
}

TEST_F(LcbDeviceDataTest, DD_0x73_ReadByte_TruncatedData) {
    /* len_of_data=5 but only 2 bytes available after header */
    uint8_t data[] = {0x73, 0x00, 0x05, 0xAA, 0xBB};
    lcb_device_data_analyze(&cb, data, 5);
    EXPECT_EQ(ctx.on_dd_read_byte_count, 1);
    EXPECT_EQ(ctx.last_read_byte_len, 4);  /* 2 + min(2, 5) = 4 */
    EXPECT_EQ(ctx.last_read_byte_data[0], 0x00);
    EXPECT_EQ(ctx.last_read_byte_data[1], 0x05);
    EXPECT_EQ(ctx.last_read_byte_data[2], 0xAA);
    EXPECT_EQ(ctx.last_read_byte_data[3], 0xBB);
}

TEST_F(LcbDeviceDataTest, DD_0x73_ReadByte_TooShort_Ignored) {
    uint8_t data[] = {0x73, 0x00};  /* Only 2 bytes, need >= 3 */
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_read_byte_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x73_ReadByte_NullCallback_NoCrash) {
    cb.on_dd_read_byte_data = NULL;
    uint8_t data[] = {0x73, 0x00, 0x03, 0xAA, 0xBB, 0xCC};
    lcb_device_data_analyze(&cb, data, 6);
}

/* ============================================================================
 * 0x80 SavePWM (Status byte, recv=2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x80_SavePwm_Normal) {
    uint8_t data[] = {0x80, 0x00};
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_save_pwm_count, 1);
    EXPECT_EQ(ctx.last_save_pwm_status, 0x00);
}

TEST_F(LcbDeviceDataTest, DD_0x80_SavePwm_WrongSize_Ignored) {
    uint8_t data[] = {0x80};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_save_pwm_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x80_SavePwm_NullCallback_NoCrash) {
    cb.on_dd_save_pwm = NULL;
    uint8_t data[] = {0x80, 0x00};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x81 ReadPWM (Size-validated, recv=3)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x81_ReadPwm_Normal) {
    uint8_t data[] = {0x81, 0x01, 0x02};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_read_pwm_count, 1);
    EXPECT_EQ(ctx.last_read_pwm_len, 2);
    EXPECT_EQ(ctx.last_read_pwm_data[0], 0x01);
    EXPECT_EQ(ctx.last_read_pwm_data[1], 0x02);
}

TEST_F(LcbDeviceDataTest, DD_0x81_ReadPwm_WrongSize_Ignored) {
    uint8_t data[] = {0x81, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_read_pwm_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x81_ReadPwm_NullCallback_NoCrash) {
    cb.on_dd_read_pwm = NULL;
    uint8_t data[] = {0x81, 0x01, 0x02};
    lcb_device_data_analyze(&cb, data, 3);
}

/* ============================================================================
 * 0x90 SetMcbLcbParameter (Status byte, recv=2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x90_SetParam_Normal) {
    uint8_t data[] = {0x90, 0xFF};
    lcb_device_data_analyze(&cb, data, 2);
    EXPECT_EQ(ctx.on_dd_set_param_count, 1);
    EXPECT_EQ(ctx.last_set_param_status, 0xFF);
}

TEST_F(LcbDeviceDataTest, DD_0x90_SetParam_WrongSize_Ignored) {
    uint8_t data[] = {0x90, 0x01, 0x02};
    lcb_device_data_analyze(&cb, data, 3);
    EXPECT_EQ(ctx.on_dd_set_param_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x90_SetParam_NullCallback_NoCrash) {
    cb.on_dd_set_mcb_lcb_parameter = NULL;
    uint8_t data[] = {0x90, 0xFF};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0x91 GetMcbLcbParameter (Variable-length, min 2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0x91_GetParam_Normal) {
    uint8_t data[] = {0x91, 0x01, 0x02, 0x03};
    lcb_device_data_analyze(&cb, data, 4);
    EXPECT_EQ(ctx.on_dd_get_param_count, 1);
    EXPECT_EQ(ctx.last_get_param_len, 3);
    EXPECT_EQ(ctx.last_get_param_data[0], 0x01);
}

TEST_F(LcbDeviceDataTest, DD_0x91_GetParam_TooShort_Ignored) {
    uint8_t data[] = {0x91};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_get_param_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0x91_GetParam_NullCallback_NoCrash) {
    cb.on_dd_get_mcb_lcb_parameter = NULL;
    uint8_t data[] = {0x91, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * 0xF0 GetLCBinformation (Variable-length, min 2)
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, DD_0xF0_LcbInfo_Normal) {
    uint8_t data[] = {0xF0, 0x01, 0x02, 0x03, 0x04};
    lcb_device_data_analyze(&cb, data, 5);
    EXPECT_EQ(ctx.on_dd_lcb_info_count, 1);
    EXPECT_EQ(ctx.last_lcb_info_len, 4);
    EXPECT_EQ(ctx.last_lcb_info_data[0], 0x01);
    EXPECT_EQ(ctx.last_lcb_info_data[3], 0x04);
}

TEST_F(LcbDeviceDataTest, DD_0xF0_LcbInfo_TooShort_Ignored) {
    uint8_t data[] = {0xF0};
    lcb_device_data_analyze(&cb, data, 1);
    EXPECT_EQ(ctx.on_dd_lcb_info_count, 0);
}

TEST_F(LcbDeviceDataTest, DD_0xF0_LcbInfo_NullCallback_NoCrash) {
    cb.on_dd_get_lcb_information = NULL;
    uint8_t data[] = {0xF0, 0x01};
    lcb_device_data_analyze(&cb, data, 2);
}

/* ============================================================================
 * TX NULL Safety Tests
 * ============================================================================ */

TEST_F(LcbDeviceDataTest, TX_GetEepromMemorySizes_NullComm_NoCrash) {
    lcb_dd_get_eeprom_memory_sizes(NULL);
}

TEST_F(LcbDeviceDataTest, TX_EraseLoaderMemory_NullComm_NoCrash) {
    uint8_t addr[8] = {0};
    lcb_dd_erase_loader_memory(NULL, addr);
}

TEST_F(LcbDeviceDataTest, TX_WriteWordLoaderData_NullComm_NoCrash) {
    uint8_t payload[] = {0x01, 0x02, 0x03};
    lcb_dd_write_word_loader_data(NULL, payload, 3);
}

TEST_F(LcbDeviceDataTest, TX_ReadPwm_NullComm_NoCrash) {
    lcb_dd_read_pwm(NULL, 0x01);
}

TEST_F(LcbDeviceDataTest, TX_GetLcbInfoHighestPriority_NullComm_NoCrash) {
    lcb_dd_get_lcb_information_highest_priority(NULL);
}
