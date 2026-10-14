/**
 * @file test_lcb_comm_universal_cmd.cpp
 * @brief Unit tests for LCB Universal Data sub-commands (0xAA)
 *
 * Tests lcb_universal_data_analyze() dispatcher, 4 RX handlers,
 * and 6 TX functions from lcb_universal_data.h / lcb_universal_data_proc.c /
 * lcb_universal_data_tx.c.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "lcb_universal_data.h"
}

/* ============================================================================
 * Test Context — captures callback invocations
 * ============================================================================ */

struct UDTestContext {
    /* 0x03 GetMotoType */
    int on_ud_get_moto_type_count;
    int last_moto_type;

    /* 0x23 SetPFCmode */
    int on_ud_set_pfc_mode_count;
    uint8_t last_pfc_mode_data[256];
    uint8_t last_pfc_mode_len;

    /* 0x24 MagnetAngleCal */
    int on_ud_magnet_angle_state_count;
    uint8_t last_magnet_angle_data[256];
    uint8_t last_magnet_angle_len;

    /* 0x25 McbVoltage */
    int on_ud_mcb_voltage_count;
    uint8_t last_mcb_voltage_data[256];
    uint8_t last_mcb_voltage_len;

    void reset() { memset(this, 0, sizeof(*this)); }
};

/* ============================================================================
 * Static Trampoline Functions
 * ============================================================================ */

static void ud_cb_get_moto_type(void *ud, int moto_type) {
    auto *ctx = (UDTestContext*)ud;
    ctx->on_ud_get_moto_type_count++;
    ctx->last_moto_type = moto_type;
}

static void ud_cb_set_pfc_mode(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (UDTestContext*)ud;
    ctx->on_ud_set_pfc_mode_count++;
    ctx->last_pfc_mode_len = len;
    if (data)
        memcpy(ctx->last_pfc_mode_data, data, len);
}

static void ud_cb_magnet_angle_state(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (UDTestContext*)ud;
    ctx->on_ud_magnet_angle_state_count++;
    ctx->last_magnet_angle_len = len;
    if (data)
        memcpy(ctx->last_magnet_angle_data, data, len);
}

static void ud_cb_mcb_voltage(void *ud, const uint8_t *data, uint8_t len) {
    auto *ctx = (UDTestContext*)ud;
    ctx->on_ud_mcb_voltage_count++;
    ctx->last_mcb_voltage_len = len;
    if (data)
        memcpy(ctx->last_mcb_voltage_data, data, len);
}

/* ============================================================================
 * Setup Helper
 * ============================================================================ */

static void setup_ud_callbacks(lcb_universal_data_callbacks_t *cb, UDTestContext *ctx) {
    lcb_universal_data_callbacks_init(cb);
    cb->user_data = ctx;
    cb->on_ud_get_moto_type = ud_cb_get_moto_type;
    cb->on_ud_set_pfc_mode = ud_cb_set_pfc_mode;
    cb->on_ud_magnet_angle_state = ud_cb_magnet_angle_state;
    cb->on_ud_mcb_voltage = ud_cb_mcb_voltage;
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

class LcbUniversalDataTest : public ::testing::Test {
protected:
    UDTestContext ctx;
    lcb_universal_data_callbacks_t cb;

    void SetUp() override {
        ctx.reset();
        setup_ud_callbacks(&cb, &ctx);
    }
};

/* ============================================================================
 * Init & Dispatcher Edge Cases
 * ============================================================================ */

TEST_F(LcbUniversalDataTest, CallbacksInit_ZerosAllFields) {
    lcb_universal_data_callbacks_t test_cb;
    memset(&test_cb, 0xFF, sizeof(test_cb));
    lcb_universal_data_callbacks_init(&test_cb);
    EXPECT_EQ(test_cb.user_data, nullptr);
    EXPECT_EQ(test_cb.on_ud_get_moto_type, nullptr);
    EXPECT_EQ(test_cb.on_ud_set_pfc_mode, nullptr);
    EXPECT_EQ(test_cb.on_ud_magnet_angle_state, nullptr);
    EXPECT_EQ(test_cb.on_ud_mcb_voltage, nullptr);
}

TEST_F(LcbUniversalDataTest, CallbacksInit_NullSafe) {
    lcb_universal_data_callbacks_init(NULL);
    /* No crash = pass */
}

TEST_F(LcbUniversalDataTest, Analyze_NullCb_NoCrash) {
    uint8_t data[] = {0x03, 0x00, 0x01};
    lcb_universal_data_analyze(NULL, data, sizeof(data));
}

TEST_F(LcbUniversalDataTest, Analyze_NullData_NoCrash) {
    lcb_universal_data_analyze(&cb, NULL, 3);
}

TEST_F(LcbUniversalDataTest, Analyze_ZeroLength_NoCrash) {
    uint8_t data[] = {0x03};
    lcb_universal_data_analyze(&cb, data, 0);
}

TEST_F(LcbUniversalDataTest, Analyze_UnknownSubCmd_NoCrash) {
    uint8_t data[] = {0xFF, 0x00, 0x00};
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    /* No callback should be invoked */
    EXPECT_EQ(ctx.on_ud_get_moto_type_count, 0);
    EXPECT_EQ(ctx.on_ud_set_pfc_mode_count, 0);
    EXPECT_EQ(ctx.on_ud_magnet_angle_state_count, 0);
    EXPECT_EQ(ctx.on_ud_mcb_voltage_count, 0);
}

/* ============================================================================
 * 0x03 GetMotoType — 2-byte BE short → int
 * ============================================================================ */

TEST_F(LcbUniversalDataTest, UD_0x03_GetMotoType_Normal) {
    uint8_t data[] = {0x03, 0x01, 0x02};  /* BE: 0x0102 = 258 */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_get_moto_type_count, 1);
    EXPECT_EQ(ctx.last_moto_type, 258);
}

TEST_F(LcbUniversalDataTest, UD_0x03_GetMotoType_ZeroValue) {
    uint8_t data[] = {0x03, 0x00, 0x00};
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_get_moto_type_count, 1);
    EXPECT_EQ(ctx.last_moto_type, 0);
}

TEST_F(LcbUniversalDataTest, UD_0x03_GetMotoType_WrongSize_Ignored) {
    uint8_t data[] = {0x03, 0x01};  /* Only 2 bytes, need 3 */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_get_moto_type_count, 0);
}

TEST_F(LcbUniversalDataTest, UD_0x03_GetMotoType_NullCallback_NoCrash) {
    cb.on_ud_get_moto_type = NULL;
    uint8_t data[] = {0x03, 0x01, 0x02};
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    /* No crash = pass */
}

/* ============================================================================
 * 0x23 SetPFCmode — fixed length 4, extract data[2..3]
 * ============================================================================ */

TEST_F(LcbUniversalDataTest, UD_0x23_SetPFCmode_Normal) {
    uint8_t data[] = {0x23, 0x01, 0xAA, 0xBB};  /* 4 bytes */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_set_pfc_mode_count, 1);
    EXPECT_EQ(ctx.last_pfc_mode_len, 2);
    EXPECT_EQ(ctx.last_pfc_mode_data[0], 0xAA);
    EXPECT_EQ(ctx.last_pfc_mode_data[1], 0xBB);
}

TEST_F(LcbUniversalDataTest, UD_0x23_SetPFCmode_WrongSize_Ignored) {
    uint8_t data[] = {0x23, 0x01, 0xAA};  /* 3 bytes, need 4 */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_set_pfc_mode_count, 0);
}

TEST_F(LcbUniversalDataTest, UD_0x23_SetPFCmode_NullCallback_NoCrash) {
    cb.on_ud_set_pfc_mode = NULL;
    uint8_t data[] = {0x23, 0x01, 0xAA, 0xBB};
    lcb_universal_data_analyze(&cb, data, sizeof(data));
}

/* ============================================================================
 * 0x24 MagnetAngleCal — variable length, skip cmd byte
 * ============================================================================ */

TEST_F(LcbUniversalDataTest, UD_0x24_MagnetAngle_Normal) {
    uint8_t data[] = {0x24, 0x55, 0x66};  /* 3 bytes */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_magnet_angle_state_count, 1);
    EXPECT_EQ(ctx.last_magnet_angle_len, 2);
    EXPECT_EQ(ctx.last_magnet_angle_data[0], 0x55);
    EXPECT_EQ(ctx.last_magnet_angle_data[1], 0x66);
}

TEST_F(LcbUniversalDataTest, UD_0x24_MagnetAngle_VariableLen) {
    uint8_t data[] = {0x24, 0x11};  /* 2 bytes — minimum valid */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_magnet_angle_state_count, 1);
    EXPECT_EQ(ctx.last_magnet_angle_len, 1);
    EXPECT_EQ(ctx.last_magnet_angle_data[0], 0x11);
}

TEST_F(LcbUniversalDataTest, UD_0x24_MagnetAngle_OnlyCmdByte_Ignored) {
    uint8_t data[] = {0x24};  /* 1 byte — only cmd byte */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_magnet_angle_state_count, 0);
}

TEST_F(LcbUniversalDataTest, UD_0x24_MagnetAngle_NullCallback_NoCrash) {
    cb.on_ud_magnet_angle_state = NULL;
    uint8_t data[] = {0x24, 0x55, 0x66};
    lcb_universal_data_analyze(&cb, data, sizeof(data));
}

/* ============================================================================
 * 0x25 McbVoltage — variable length, skip cmd byte
 * ============================================================================ */

TEST_F(LcbUniversalDataTest, UD_0x25_McbVoltage_Normal) {
    uint8_t data[] = {0x25, 0xAA, 0xBB};  /* 3 bytes */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_mcb_voltage_count, 1);
    EXPECT_EQ(ctx.last_mcb_voltage_len, 2);
    EXPECT_EQ(ctx.last_mcb_voltage_data[0], 0xAA);
    EXPECT_EQ(ctx.last_mcb_voltage_data[1], 0xBB);
}

TEST_F(LcbUniversalDataTest, UD_0x25_McbVoltage_VariableLen) {
    uint8_t data[] = {0x25, 0x77};  /* 2 bytes — minimum valid */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_mcb_voltage_count, 1);
    EXPECT_EQ(ctx.last_mcb_voltage_len, 1);
    EXPECT_EQ(ctx.last_mcb_voltage_data[0], 0x77);
}

TEST_F(LcbUniversalDataTest, UD_0x25_McbVoltage_OnlyCmdByte_Ignored) {
    uint8_t data[] = {0x25};  /* 1 byte — only cmd byte */
    lcb_universal_data_analyze(&cb, data, sizeof(data));
    EXPECT_EQ(ctx.on_ud_mcb_voltage_count, 0);
}

TEST_F(LcbUniversalDataTest, UD_0x25_McbVoltage_NullCallback_NoCrash) {
    cb.on_ud_mcb_voltage = NULL;
    uint8_t data[] = {0x25, 0xAA, 0xBB};
    lcb_universal_data_analyze(&cb, data, sizeof(data));
}

/* ============================================================================
 * TX NULL Safety — each TX function with NULL comm must not crash
 * ============================================================================ */

TEST_F(LcbUniversalDataTest, TX_GetMotoType_NullComm_NoCrash) {
    lcb_ud_get_moto_type(NULL);
}

TEST_F(LcbUniversalDataTest, TX_SetPFCmode_NullComm_NoCrash) {
    lcb_ud_set_pfc_mode(NULL, 0x01);
}

TEST_F(LcbUniversalDataTest, TX_SetMagnetAngleState_NullComm_NoCrash) {
    lcb_ud_set_magnet_angle_state(NULL, 0x00);
}

TEST_F(LcbUniversalDataTest, TX_GetMagnetAngle_NullComm_NoCrash) {
    lcb_ud_get_magnet_angle(NULL, LCB_QUEUE_MODE_ALWAYS_ADD);
}

TEST_F(LcbUniversalDataTest, TX_SetMcbVoltage_NullComm_NoCrash) {
    lcb_ud_set_mcb_voltage(NULL, 0x55);
}

TEST_F(LcbUniversalDataTest, TX_GetMcbVoltage_NullComm_NoCrash) {
    lcb_ud_get_mcb_voltage(NULL, LCB_QUEUE_MODE_ALWAYS_ADD);
}
