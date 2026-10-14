/**
 * @file test_lcb_error_code.cpp
 * @brief Unit tests for LCB error code handling and status byte B6 error bit
 *
 * Tests:
 * - Error code constants (0x04B0, 0x024F, 0x00A1)
 * - Error injection functions (lcb_proc_comm_lcb_error, lcb_proc_e_stop_mcb_error, etc.)
 * - Error code extraction from 0x72 GetLCBErrorCode packet
 * - Status byte B6 (LCB_STATUS_BIT_LCB_ERROR = 0x40) detection
 * - NULL safety
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "lcb_proc_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_packet.h"
#include "lcb_constants.h"
}

/* ============================================================================
 * Test Context — captures error callback invocations
 * ============================================================================ */

struct ErrorTestContext {
    /* on_lcb_error_status_detected (B6 bit) */
    int on_lcb_error_status_count;
    bool last_error_status;

    /* on_lcb_error_code */
    int on_lcb_error_code_count;
    uint16_t last_error_code;

    /* on_return_error_code_to_ui */
    int on_return_error_code_to_ui_count;
    uint16_t last_ui_error_code;

    void reset() { memset(this, 0, sizeof(*this)); }
};

/* ============================================================================
 * Static Trampoline Functions
 * ============================================================================ */

static void err_cb_error_status_detected(void *ud, bool has_error) {
    auto *ctx = (ErrorTestContext*)ud;
    ctx->on_lcb_error_status_count++;
    ctx->last_error_status = has_error;
}

static void err_cb_error_code(void *ud, uint16_t error_code) {
    auto *ctx = (ErrorTestContext*)ud;
    ctx->on_lcb_error_code_count++;
    ctx->last_error_code = error_code;
}

static void err_cb_return_error_code_to_ui(void *ud, uint16_t error_code) {
    auto *ctx = (ErrorTestContext*)ud;
    ctx->on_return_error_code_to_ui_count++;
    ctx->last_ui_error_code = error_code;
}

/* ============================================================================
 * Setup Helper
 * ============================================================================ */

static void setup_error_callbacks(lcb_comm_callbacks_t *cb, ErrorTestContext *ctx) {
    lcb_comm_callbacks_init(cb);
    cb->user_data = ctx;
    cb->on_lcb_error_status_detected = err_cb_error_status_detected;
    cb->on_lcb_error_code = err_cb_error_code;
    cb->on_return_error_code_to_ui = err_cb_return_error_code_to_ui;
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

class LcbErrorCodeTest : public ::testing::Test {
protected:
    ErrorTestContext ctx;
    lcb_comm_callbacks_t cb;

    void SetUp() override {
        ctx.reset();
        setup_error_callbacks(&cb, &ctx);
    }
};

/* ============================================================================
 * Error Code Constants Validation
 * ============================================================================ */

TEST_F(LcbErrorCodeTest, ErrorCode_CommLcb_Is0x04B0) {
    EXPECT_EQ(LCB_ERROR_COMM_LCB, 0x04B0);
}

TEST_F(LcbErrorCodeTest, ErrorCode_EStopMcb_Is0x024F) {
    EXPECT_EQ(LCB_ERROR_E_STOP_MCB, 0x024F);
}

TEST_F(LcbErrorCodeTest, ErrorCode_TmNoRpm_Is0x00A1) {
    EXPECT_EQ(LCB_ERROR_TM_NO_RPM, 0x00A1);
}

/* ============================================================================
 * Status Byte B6 Bit Definition
 * ============================================================================ */

TEST_F(LcbErrorCodeTest, StatusBit_B6_LcbError_Is0x40) {
    EXPECT_EQ(LCB_STATUS_BIT_LCB_ERROR, 0x40);
}

TEST_F(LcbErrorCodeTest, StatusBit_B6_DetectedInByte) {
    uint8_t status_with_error = 0x40;
    EXPECT_TRUE(lcb_status_bit_is_set(status_with_error, LCB_STATUS_BIT_LCB_ERROR));
}

TEST_F(LcbErrorCodeTest, StatusBit_B6_NotDetectedWhenClear) {
    uint8_t status_no_error = 0x00;
    EXPECT_FALSE(lcb_status_bit_is_set(status_no_error, LCB_STATUS_BIT_LCB_ERROR));
}

TEST_F(LcbErrorCodeTest, StatusBit_B6_DetectedWithOtherBitsSet) {
    uint8_t status_mixed = 0xFF;  /* All bits set */
    EXPECT_TRUE(lcb_status_bit_is_set(status_mixed, LCB_STATUS_BIT_LCB_ERROR));
}

/* ============================================================================
 * 0x04B0 Communication Disconnect Error Injection
 * ============================================================================ */

TEST_F(LcbErrorCodeTest, CommLcbError_FiresErrorCodeCallback) {
    lcb_proc_comm_lcb_error(&cb);
    EXPECT_EQ(ctx.on_lcb_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x04B0);
}

TEST_F(LcbErrorCodeTest, CommLcbError_FiresReturnToUiCallback) {
    lcb_proc_comm_lcb_error(&cb);
    EXPECT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    EXPECT_EQ(ctx.last_ui_error_code, 0x04B0);
}

TEST_F(LcbErrorCodeTest, CommLcbError_NullCb_NoCrash) {
    lcb_proc_comm_lcb_error(NULL);
    /* No crash = pass */
}

/* ============================================================================
 * 0x024F MCB Error Injection
 * ============================================================================ */

TEST_F(LcbErrorCodeTest, EStopMcbError_FiresErrorCodeCallback) {
    lcb_proc_e_stop_mcb_error(&cb);
    EXPECT_EQ(ctx.on_lcb_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x024F);
}

TEST_F(LcbErrorCodeTest, EStopMcbError_FiresReturnToUiCallback) {
    lcb_proc_e_stop_mcb_error(&cb);
    EXPECT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    EXPECT_EQ(ctx.last_ui_error_code, 0x024F);
}

TEST_F(LcbErrorCodeTest, EStopMcbError_NullCb_NoCrash) {
    lcb_proc_e_stop_mcb_error(NULL);
}

/* ============================================================================
 * 0x00A1 Treadmill No RPM Response Error Injection
 * ============================================================================ */

TEST_F(LcbErrorCodeTest, TmNoRpmError_FiresErrorCodeCallback) {
    lcb_proc_tm_no_rpm_error(&cb);
    EXPECT_EQ(ctx.on_lcb_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x00A1);
}

TEST_F(LcbErrorCodeTest, TmNoRpmError_FiresReturnToUiCallback) {
    lcb_proc_tm_no_rpm_error(&cb);
    EXPECT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    EXPECT_EQ(ctx.last_ui_error_code, 0x00A1);
}

TEST_F(LcbErrorCodeTest, TmNoRpmError_NullCb_NoCrash) {
    lcb_proc_tm_no_rpm_error(NULL);
}

/* ============================================================================
 * 0x72 GetLCBErrorCode — Packet Parsing (2-byte BE error code)
 * ============================================================================ */

TEST_F(LcbErrorCodeTest, GetErrorCode_Packet_0x04B0) {
    lcb_packet_t pkt;
    lcb_packet_init(&pkt);
    pkt.command = LCB_CMD_GET_ERROR_CODE;
    uint8_t data[] = {0x04, 0xB0};  /* BE for 0x04B0 */
    lcb_packet_set_data(&pkt, data, sizeof(data));

    lcb_proc_get_error_code(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x04B0);
    EXPECT_EQ(ctx.on_return_error_code_to_ui_count, 1);
    EXPECT_EQ(ctx.last_ui_error_code, 0x04B0);

    lcb_packet_free(&pkt);
}

TEST_F(LcbErrorCodeTest, GetErrorCode_Packet_0x024F_McbError) {
    lcb_packet_t pkt;
    lcb_packet_init(&pkt);
    pkt.command = LCB_CMD_GET_ERROR_CODE;
    uint8_t data[] = {0x02, 0x4F};  /* BE for 0x024F */
    lcb_packet_set_data(&pkt, data, sizeof(data));

    lcb_proc_get_error_code(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x024F);

    lcb_packet_free(&pkt);
}

TEST_F(LcbErrorCodeTest, GetErrorCode_Packet_0x00A1_TmNoRpm) {
    lcb_packet_t pkt;
    lcb_packet_init(&pkt);
    pkt.command = LCB_CMD_GET_ERROR_CODE;
    uint8_t data[] = {0x00, 0xA1};  /* BE for 0x00A1 */
    lcb_packet_set_data(&pkt, data, sizeof(data));

    lcb_proc_get_error_code(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x00A1);

    lcb_packet_free(&pkt);
}

TEST_F(LcbErrorCodeTest, GetErrorCode_WrongDataSize_Ignored) {
    lcb_packet_t pkt;
    lcb_packet_init(&pkt);
    pkt.command = LCB_CMD_GET_ERROR_CODE;
    uint8_t data[] = {0x04};  /* Only 1 byte, need 2 */
    lcb_packet_set_data(&pkt, data, sizeof(data));

    lcb_proc_get_error_code(&cb, &pkt);
    EXPECT_EQ(ctx.on_lcb_error_code_count, 0);  /* Not invoked */

    lcb_packet_free(&pkt);
}

TEST_F(LcbErrorCodeTest, GetErrorCode_NullCb_NoCrash) {
    lcb_packet_t pkt;
    lcb_packet_init(&pkt);
    uint8_t data[] = {0x04, 0xB0};
    lcb_packet_set_data(&pkt, data, sizeof(data));

    lcb_proc_get_error_code(NULL, &pkt);
    /* No crash = pass */

    lcb_packet_free(&pkt);
}

TEST_F(LcbErrorCodeTest, GetErrorCode_NullPkt_NoCrash) {
    lcb_proc_get_error_code(&cb, NULL);
    /* No crash = pass */
}

/* ============================================================================
 * All Error Codes — E-Stop and E-Stop Unlock
 * ============================================================================ */

TEST_F(LcbErrorCodeTest, EStopError_FiresCorrectCode) {
    lcb_proc_e_stop_error(&cb);
    EXPECT_EQ(ctx.last_error_code, 0x02B2);
    EXPECT_EQ(ctx.on_return_error_code_to_ui_count, 1);
}

TEST_F(LcbErrorCodeTest, EStopUnlock_FiresCorrectCode) {
    lcb_proc_e_stop_unlock(&cb);
    EXPECT_EQ(ctx.last_error_code, 0x02B1);
    EXPECT_EQ(ctx.on_return_error_code_to_ui_count, 1);
}
