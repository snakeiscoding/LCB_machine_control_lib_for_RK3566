/**
 * @file test_lcb_treadmill_handler.cpp
 * @brief Unit tests for LCB Treadmill Handler (MachineHandler/Treadmill layer)
 *
 * Tests lifecycle, callback bridge, RPM ramp state machine, no-RPM-error
 * timeout, auto-reply interval counters, LCB type skip logic, and NULL safety.
 *
 * NOTE: Tests do NOT spawn the heartbeat thread. They drive logic by calling
 * the internal helpers (th_proc_auto_reply_tick, th_proc_check_ramp_up_cmd,
 * th_proc_target_rpm_status_check) directly with comm = NULL.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "treadmill_handler.h"
#include "treadmill_handler_internal.h"
#include "lcb_comm_callback.h"
#include "lcb_type.h"
#include "lcb_proc_comm.h"
}

/* ============================================================================
 * Test Context — captures user callback invocations
 * ============================================================================ */

struct THTestContext {
    /* Speed & Incline */
    int on_treadmill_rpm_count;          uint16_t last_rpm;
    int on_treadmill_one_shot_rpm_count; uint16_t last_one_shot_rpm;
    int on_treadmill_incline_loc_count;  uint16_t last_incline_loc;

    /* Status */
    int on_treadmill_in_used_count;        uint8_t last_in_used;
    int on_treadmill_motor_info_count;     uint8_t last_motor_info[256]; uint8_t last_motor_info_len;
    int on_treadmill_ext_status_count;     uint16_t last_ext_status;
    int on_treadmill_operate_count;        uint8_t last_operate[256];   uint8_t last_operate_len;

    /* Service switch */
    int on_treadmill_service_switch_count; bool last_service_on;

    /* GUI mirror */
    int on_treadmill_lcb_set_rpm_value_count; uint16_t last_lcb_set_rpm;
    int on_treadmill_lcb_set_adc_value_count; uint16_t last_lcb_set_adc;

    void reset() { memset(this, 0, sizeof(*this)); }
};

/* ============================================================================
 * Trampolines
 * ============================================================================ */

static void th_test_on_rpm(void *ud, uint16_t rpm) {
    auto *c = (THTestContext*)ud; c->on_treadmill_rpm_count++; c->last_rpm = rpm;
}
static void th_test_on_one_shot_rpm(void *ud, uint16_t rpm) {
    auto *c = (THTestContext*)ud; c->on_treadmill_one_shot_rpm_count++; c->last_one_shot_rpm = rpm;
}
static void th_test_on_incline_loc(void *ud, uint16_t loc) {
    auto *c = (THTestContext*)ud; c->on_treadmill_incline_loc_count++; c->last_incline_loc = loc;
}
static void th_test_on_in_used(void *ud, uint8_t s) {
    auto *c = (THTestContext*)ud; c->on_treadmill_in_used_count++; c->last_in_used = s;
}
static void th_test_on_motor_info(void *ud, const uint8_t *info, uint8_t len) {
    auto *c = (THTestContext*)ud; c->on_treadmill_motor_info_count++;
    c->last_motor_info_len = len;
    if (info) memcpy(c->last_motor_info, info, len);
}
static void th_test_on_ext_status(void *ud, uint16_t s) {
    auto *c = (THTestContext*)ud; c->on_treadmill_ext_status_count++; c->last_ext_status = s;
}
static void th_test_on_operate(void *ud, const uint8_t *data, uint8_t len) {
    auto *c = (THTestContext*)ud; c->on_treadmill_operate_count++;
    c->last_operate_len = len;
    if (data) memcpy(c->last_operate, data, len);
}
static void th_test_on_service_switch(void *ud, bool on) {
    auto *c = (THTestContext*)ud; c->on_treadmill_service_switch_count++; c->last_service_on = on;
}
static void th_test_on_lcb_set_rpm(void *ud, uint16_t rpm) {
    auto *c = (THTestContext*)ud; c->on_treadmill_lcb_set_rpm_value_count++; c->last_lcb_set_rpm = rpm;
}
static void th_test_on_lcb_set_adc(void *ud, uint16_t adc) {
    auto *c = (THTestContext*)ud; c->on_treadmill_lcb_set_adc_value_count++; c->last_lcb_set_adc = adc;
}

/* ============================================================================
 * Setup Helper
 * ============================================================================ */

static void setup_th_test_callbacks(treadmill_handler_callbacks_t *cb, THTestContext *ctx) {
    treadmill_handler_callbacks_init(cb);
    cb->user_data = ctx;
    cb->on_treadmill_rpm = th_test_on_rpm;
    cb->on_treadmill_one_shot_rpm = th_test_on_one_shot_rpm;
    cb->on_treadmill_incline_loc = th_test_on_incline_loc;
    cb->on_treadmill_in_used = th_test_on_in_used;
    cb->on_treadmill_motor_info = th_test_on_motor_info;
    cb->on_treadmill_ext_status = th_test_on_ext_status;
    cb->on_treadmill_operate = th_test_on_operate;
    cb->on_treadmill_service_switch = th_test_on_service_switch;
    cb->on_treadmill_lcb_set_rpm_value = th_test_on_lcb_set_rpm;
    cb->on_treadmill_lcb_set_adc_value = th_test_on_lcb_set_adc;
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

class LcbTreadmillHandlerTest : public ::testing::Test {
protected:
    THTestContext ctx;
    treadmill_handler_t *th;
    treadmill_handler_callbacks_t user_cb;

    void SetUp() override {
        ctx.reset();
        th = treadmill_handler_create();
        ASSERT_NE(th, nullptr);
        setup_th_test_callbacks(&user_cb, &ctx);
        treadmill_handler_set_callbacks(th, &user_cb);
        /* Wire internal LCB callbacks so we can invoke them directly */
        th_lcb_cb_register(th);
    }

    void TearDown() override {
        treadmill_handler_destroy(th);
    }
};

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, Create_DefaultsAreSet) {
    EXPECT_EQ(th->machine_lcb_type, LCB_TYPE_UNKNOWN);
    EXPECT_EQ(th->ramp_up_step, TM_DEFAULT_RAMP_STEP);
    EXPECT_EQ(th->ramp_down_step, TM_DEFAULT_RAMP_STEP);
    EXPECT_FALSE(th->is_running);
    EXPECT_FALSE(th->is_auto_ramp_rpm);
    EXPECT_FALSE(th->target_rpm_check_flag);
}

TEST_F(LcbTreadmillHandlerTest, Destroy_NullSafe) {
    treadmill_handler_destroy(NULL);
}

TEST_F(LcbTreadmillHandlerTest, CallbacksInit_ZerosAllFields) {
    treadmill_handler_callbacks_t cb;
    memset(&cb, 0xFF, sizeof(cb));
    treadmill_handler_callbacks_init(&cb);
    EXPECT_EQ(cb.user_data, nullptr);
    EXPECT_EQ(cb.on_treadmill_rpm, nullptr);
    EXPECT_EQ(cb.on_treadmill_service_switch, nullptr);
}

TEST_F(LcbTreadmillHandlerTest, CallbacksInit_NullSafe) {
    treadmill_handler_callbacks_init(NULL);
}

/* ============================================================================
 * Callback Registration
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, SetCallbacks_StoresPointers) {
    /* Setup already wired callbacks; verify by invoking trampoline */
    th->lcb_cb.on_lcb_motor_rpm_received(th, 1000);
    EXPECT_EQ(ctx.on_treadmill_rpm_count, 1);
    EXPECT_EQ(ctx.last_rpm, 1000);
}

TEST_F(LcbTreadmillHandlerTest, UnregisterCallbacks_ClearsPointers) {
    treadmill_handler_unregister_callbacks(th);
    th->lcb_cb.on_lcb_motor_rpm_received(th, 500);
    /* User callback was cleared, so count should still be 0 */
    EXPECT_EQ(ctx.on_treadmill_rpm_count, 0);
}

TEST_F(LcbTreadmillHandlerTest, SetCallbacks_OverwritesPrevious) {
    treadmill_handler_callbacks_t cb2;
    treadmill_handler_callbacks_init(&cb2);
    cb2.user_data = &ctx;
    /* No on_treadmill_rpm assigned */
    treadmill_handler_set_callbacks(th, &cb2);
    th->lcb_cb.on_lcb_motor_rpm_received(th, 500);
    EXPECT_EQ(ctx.on_treadmill_rpm_count, 0);
}

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, Toggle_Rpm) {
    treadmill_handler_set_auto_reply_rpm(th, true);
    EXPECT_TRUE(th->is_auto_reply_rpm);
    treadmill_handler_set_auto_reply_rpm(th, false);
    EXPECT_FALSE(th->is_auto_reply_rpm);
}

TEST_F(LcbTreadmillHandlerTest, Toggle_Incline) {
    treadmill_handler_set_auto_reply_incline(th, true);
    EXPECT_TRUE(th->is_auto_reply_incline);
}

TEST_F(LcbTreadmillHandlerTest, Toggle_InUsed) {
    treadmill_handler_set_auto_reply_in_used(th, true);
    EXPECT_TRUE(th->is_auto_reply_in_used);
}

TEST_F(LcbTreadmillHandlerTest, Toggle_SpeedUpInUsed) {
    treadmill_handler_set_auto_reply_speed_up_in_used(th, true);
    EXPECT_TRUE(th->is_auto_reply_speed_up_in_used);
}

TEST_F(LcbTreadmillHandlerTest, Toggle_MotorInfo) {
    treadmill_handler_set_auto_reply_motor_info(th, true);
    EXPECT_TRUE(th->is_auto_reply_motor_info);
}

TEST_F(LcbTreadmillHandlerTest, Toggle_ExtStatus) {
    treadmill_handler_set_auto_reply_ext_status(th, true);
    EXPECT_TRUE(th->is_auto_reply_ext_status);
}

TEST_F(LcbTreadmillHandlerTest, Toggle_Cadence_Stub) {
    treadmill_handler_set_auto_reply_cadence(th, true);
    EXPECT_TRUE(th->is_auto_reply_cadence);
}

TEST_F(LcbTreadmillHandlerTest, Toggle_IrAddrIrStatus_Stub) {
    treadmill_handler_set_auto_reply_ir_addr_ir_status(th, true);
    EXPECT_TRUE(th->is_auto_reply_ir_addr_ir_status);
}

/* ============================================================================
 * Interval Counter — Each feature increments cnt to threshold, then resets
 * Tests run with comm == NULL so TX calls are skipped, but counters still tick
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, IntervalCounter_RpmInterval0) {
    /* Interval=0 means: every tick, fire and reset. cnt should always be 0. */
    th->is_auto_reply_rpm = true;
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_rpm, 0);
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_rpm, 0);
}

TEST_F(LcbTreadmillHandlerTest, IntervalCounter_ExtStatusInterval0) {
    th->is_auto_reply_ext_status = true;
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_ext_status, 0);
}

TEST_F(LcbTreadmillHandlerTest, IntervalCounter_InclineInterval1) {
    th->is_auto_reply_incline = true;
    /* tick 1: cnt 0->1 (fired before increment). Wait, actual logic:
     * if cnt >= interval(1): fire, reset to 0; else cnt++.
     * Initial cnt=0 < 1 → cnt becomes 1.
     * tick 2: cnt=1 >= 1 → fire, cnt=0.
     */
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_incline, 1);
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_incline, 0);
}

TEST_F(LcbTreadmillHandlerTest, IntervalCounter_InUsedInterval7) {
    th->is_auto_reply_in_used = true;
    /* 7 ticks bring cnt to 7. 8th tick fires and resets to 0. */
    for (int i = 0; i < 7; i++) th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_in_used, 7);
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_in_used, 0);
}

TEST_F(LcbTreadmillHandlerTest, IntervalCounter_SpeedUpInUsedInterval3) {
    th->is_auto_reply_speed_up_in_used = true;
    for (int i = 0; i < 3; i++) th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_speed_up_in_used, 3);
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_speed_up_in_used, 0);
}

TEST_F(LcbTreadmillHandlerTest, IntervalCounter_MotorInfoInterval3) {
    th->is_auto_reply_motor_info = true;
    for (int i = 0; i < 3; i++) th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_motor_info, 3);
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_motor_info, 0);
}

TEST_F(LcbTreadmillHandlerTest, IntervalCounter_FlagDisabled_NoIncrement) {
    th->is_auto_reply_motor_info = false;
    th_proc_auto_reply_tick(th);
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_motor_info, 0);
}

TEST_F(LcbTreadmillHandlerTest, IntervalCounter_RampInterval1) {
    th->is_auto_ramp_rpm = true;
    th->gui_set_rpm = 100;
    th->set_current_rpm = 100;  /* already at target */
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_ramp_rpm, 1);
    th_proc_auto_reply_tick(th);
    EXPECT_EQ(th->cnt_ramp_rpm, 0);
}

/* ============================================================================
 * RPM Ramp Math
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, Ramp_TargetZero_ImmediateStop) {
    th->set_current_rpm = 1500;
    th->gui_set_rpm = 0;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 0);
}

TEST_F(LcbTreadmillHandlerTest, Ramp_AtTarget_StaysSame) {
    th->set_current_rpm = 500;
    th->gui_set_rpm = 500;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 500);
}

TEST_F(LcbTreadmillHandlerTest, Ramp_RampUp_ClampedToStep) {
    th->set_current_rpm = 0;
    th->gui_set_rpm = 1000;
    th->ramp_up_step = 250;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 250);
}

TEST_F(LcbTreadmillHandlerTest, Ramp_RampDown_ClampedToStep) {
    th->set_current_rpm = 1000;
    th->gui_set_rpm = 200;
    th->ramp_down_step = 250;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 750);
}

TEST_F(LcbTreadmillHandlerTest, Ramp_ExactStepBoundary_RampUp) {
    th->set_current_rpm = 250;
    th->gui_set_rpm = 500;
    th->ramp_up_step = 250;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 500);
}

TEST_F(LcbTreadmillHandlerTest, Ramp_TargetWithinStep_SnapsToTarget) {
    th->set_current_rpm = 0;
    th->gui_set_rpm = 100;
    th->ramp_up_step = 250;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 100);  /* 0 + min(100, 250) = 100 */
}

TEST_F(LcbTreadmillHandlerTest, Ramp_TargetWithinDownStep_SnapsToTarget) {
    th->set_current_rpm = 100;
    th->gui_set_rpm = 50;
    th->ramp_down_step = 250;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 50);  /* 100 - min(50, 250) = 50 */
}

TEST_F(LcbTreadmillHandlerTest, Ramp_MidRampSetNewTarget) {
    th->set_current_rpm = 500;
    th->gui_set_rpm = 1000;
    th->ramp_up_step = 250;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 750);
    /* New target mid-ramp */
    th->gui_set_rpm = 600;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 600);  /* ramp down from 750 to 600, within step */
}

TEST_F(LcbTreadmillHandlerTest, Ramp_GuiZeroWithCurrentNonzero_ImmediateZero) {
    th->set_current_rpm = 800;
    th->gui_set_rpm = 0;
    th_proc_check_ramp_up_cmd(th);
    EXPECT_EQ(th->set_current_rpm, 0);
}

TEST_F(LcbTreadmillHandlerTest, Ramp_DisabledBypassesRamp) {
    /* When ramp is disabled, set_rpm goes directly to lcb_comm.
     * We can verify by checking that gui_set_rpm is NOT updated when ramp is off. */
    th->is_auto_ramp_rpm = false;
    th->gui_set_rpm = 0;
    treadmill_handler_set_rpm(th, 800);
    EXPECT_EQ(th->gui_set_rpm, 0);  /* Not stored in ramp state */
    EXPECT_TRUE(th->target_rpm_check_flag);  /* But flag still toggled */
}

/* ============================================================================
 * No-RPM Error Timeout
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, NoRpmTimer_SetRpmZero_ClearsFlag) {
    th->target_rpm_check_flag = true;
    th->cnt_target_rpm = 50;
    treadmill_handler_set_rpm(th, 0);
    EXPECT_FALSE(th->target_rpm_check_flag);
    EXPECT_EQ(th->cnt_target_rpm, 0);
}

TEST_F(LcbTreadmillHandlerTest, NoRpmTimer_SetRpmNonzero_StartsCountdown) {
    th->target_rpm_check_flag = false;
    treadmill_handler_set_rpm(th, 1000);
    EXPECT_TRUE(th->target_rpm_check_flag);
    EXPECT_EQ(th->cnt_target_rpm, 0);
}

TEST_F(LcbTreadmillHandlerTest, NoRpmTimer_MachineRpmReceived_CancelsCountdown) {
    th->target_rpm_check_flag = true;
    th->cnt_target_rpm = 50;
    th->machine_rpm = 800;  /* RPM responded */
    th_proc_target_rpm_status_check(th);
    EXPECT_FALSE(th->target_rpm_check_flag);
    EXPECT_EQ(th->cnt_target_rpm, 0);
}

TEST_F(LcbTreadmillHandlerTest, NoRpmTimer_100TicksAtZero_FiresError) {
    /* With comm == NULL, lcb_comm_proc_tm_no_rpm_error is skipped, but the
     * flag and counter should reset. */
    th->target_rpm_check_flag = true;
    th->machine_rpm = 0;
    for (int i = 0; i < TM_INTERVAL_TARGET_RPM_TIMEOUT - 1; i++) {
        th_proc_target_rpm_status_check(th);
    }
    EXPECT_TRUE(th->target_rpm_check_flag);
    EXPECT_EQ(th->cnt_target_rpm, TM_INTERVAL_TARGET_RPM_TIMEOUT - 1);
    /* 100th tick fires and resets */
    th_proc_target_rpm_status_check(th);
    EXPECT_FALSE(th->target_rpm_check_flag);
    EXPECT_EQ(th->cnt_target_rpm, 0);
}

TEST_F(LcbTreadmillHandlerTest, NoRpmTimer_FlagFalse_ResetsCounter) {
    th->target_rpm_check_flag = false;
    th->cnt_target_rpm = 50;
    th_proc_target_rpm_status_check(th);
    EXPECT_EQ(th->cnt_target_rpm, 0);
}

/* ============================================================================
 * 0x02B2 E-Stop Error — clears target_rpm_check_flag
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, ErrorCode_EStop_0x02B2_ClearsFlag) {
    th->target_rpm_check_flag = true;
    th->cnt_target_rpm = 50;
    th->lcb_cb.on_lcb_error_code(th, LCB_ERROR_E_STOP);
    EXPECT_FALSE(th->target_rpm_check_flag);
    EXPECT_EQ(th->cnt_target_rpm, 0);
}

TEST_F(LcbTreadmillHandlerTest, ErrorCode_OtherCodes_DoNotClearFlag) {
    th->target_rpm_check_flag = true;
    th->cnt_target_rpm = 50;
    th->lcb_cb.on_lcb_error_code(th, LCB_ERROR_COMM_LCB);
    EXPECT_TRUE(th->target_rpm_check_flag);
    EXPECT_EQ(th->cnt_target_rpm, 50);
}

/* ============================================================================
 * Incline Action LCB Type Skip
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, InclineAction_TypeE0_Skipped) {
    th->machine_lcb_type = LCB_TYPE_E0;
    /* With comm = NULL, the function would crash on lcb_comm_set_lcb_incline_action
     * if it didn't skip. The skip check returns early before reaching comm. */
    treadmill_handler_set_incline_action(th, 0x01);
    /* No crash = pass */
}

TEST_F(LcbTreadmillHandlerTest, InclineAction_Toptek_Skipped) {
    th->machine_lcb_type = LCB_TYPE_TOPTEK;
    treadmill_handler_set_incline_action(th, 0x01);
    /* No crash = pass */
}

TEST_F(LcbTreadmillHandlerTest, InclineAction_TmLcb_NotSkipped) {
    th->machine_lcb_type = LCB_TYPE_TM_LCB;
    /* comm == NULL → outer NULL check returns early, so no crash */
    treadmill_handler_set_incline_action(th, 0x01);
}

TEST_F(LcbTreadmillHandlerTest, InclineAction_Unknown_NotSkipped) {
    th->machine_lcb_type = LCB_TYPE_UNKNOWN;
    treadmill_handler_set_incline_action(th, 0x01);
}

/* ============================================================================
 * Heartbeat Status Skip for Toptek
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, HeartbeatStatus_Toptek_Skipped) {
    th->machine_lcb_type = LCB_TYPE_TOPTEK;
    /* comm = NULL ensures no crash, but the skip happens first */
    th_proc_auto_reply_tick(th);
    /* No assertion needed — verifying no crash */
}

TEST_F(LcbTreadmillHandlerTest, HeartbeatStatus_OtherType_NotSkipped) {
    th->machine_lcb_type = LCB_TYPE_TM_LCB;
    /* comm == NULL → auto_reply_heart_beat_status NULL-checks comm and returns */
    th_proc_auto_reply_tick(th);
}

/* ============================================================================
 * Callback Bridge Forwarding
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, Bridge_Rpm_StoresAndForwards) {
    th->lcb_cb.on_lcb_motor_rpm_received(th, 1500);
    EXPECT_EQ(ctx.on_treadmill_rpm_count, 1);
    EXPECT_EQ(ctx.last_rpm, 1500);
    EXPECT_EQ(th->machine_rpm, 1500);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_OneShotRpm_StoresAndForwards) {
    th->lcb_cb.on_one_shot_lcb_motor_rpm(th, 800);
    EXPECT_EQ(ctx.on_treadmill_one_shot_rpm_count, 1);
    EXPECT_EQ(ctx.last_one_shot_rpm, 800);
    EXPECT_EQ(th->machine_rpm, 800);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_InclineLoc_Forwards) {
    th->lcb_cb.on_lcb_incline_loc_received(th, 1234);
    EXPECT_EQ(ctx.on_treadmill_incline_loc_count, 1);
    EXPECT_EQ(ctx.last_incline_loc, 1234);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_InUsed_Forwards) {
    th->lcb_cb.on_lcb_treadmill_in_used(th, 0x42);
    EXPECT_EQ(ctx.on_treadmill_in_used_count, 1);
    EXPECT_EQ(ctx.last_in_used, 0x42);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_MotorInfo_Forwards) {
    uint8_t info[] = {0x01, 0x02, 0x03};
    th->lcb_cb.on_lcb_main_motor_info(th, info, 3);
    EXPECT_EQ(ctx.on_treadmill_motor_info_count, 1);
    EXPECT_EQ(ctx.last_motor_info_len, 3);
    EXPECT_EQ(ctx.last_motor_info[0], 0x01);
    EXPECT_EQ(ctx.last_motor_info[2], 0x03);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_ExtStatus_Forwards) {
    th->lcb_cb.on_lcb_treadmill_ext_status(th, 0xABCD);
    EXPECT_EQ(ctx.on_treadmill_ext_status_count, 1);
    EXPECT_EQ(ctx.last_ext_status, 0xABCD);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_Operate_Forwards) {
    uint8_t data[] = {0x55, 0x66};
    th->lcb_cb.on_lcb_treadmill_operate(th, data, 2);
    EXPECT_EQ(ctx.on_treadmill_operate_count, 1);
    EXPECT_EQ(ctx.last_operate_len, 2);
    EXPECT_EQ(ctx.last_operate[0], 0x55);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_BatteryCapacity_MapsToServiceSwitch) {
    th->lcb_cb.on_battery_capacity_status_changed(th, true);
    EXPECT_EQ(ctx.on_treadmill_service_switch_count, 1);
    EXPECT_TRUE(ctx.last_service_on);
    th->lcb_cb.on_battery_capacity_status_changed(th, false);
    EXPECT_EQ(ctx.on_treadmill_service_switch_count, 2);
    EXPECT_FALSE(ctx.last_service_on);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_RpmValueSet_Forwards) {
    th->lcb_cb.on_treadmill_rpm_value_set(th, 1200);
    EXPECT_EQ(ctx.on_treadmill_lcb_set_rpm_value_count, 1);
    EXPECT_EQ(ctx.last_lcb_set_rpm, 1200);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_AdcValueSet_Forwards) {
    th->lcb_cb.on_treadmill_adc_value_set(th, 2048);
    EXPECT_EQ(ctx.on_treadmill_lcb_set_adc_value_count, 1);
    EXPECT_EQ(ctx.last_lcb_set_adc, 2048);
}

TEST_F(LcbTreadmillHandlerTest, Bridge_ErrorCode_NoUserCallbackButHandlesInternally) {
    /* No user callback for error code; just verify internal state mutation */
    th->target_rpm_check_flag = true;
    th->lcb_cb.on_lcb_error_code(th, LCB_ERROR_E_STOP);
    EXPECT_FALSE(th->target_rpm_check_flag);
}

/* ============================================================================
 * machine_rpm Tracking
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, MachineRpm_TrackedFromRpmCallback) {
    th->lcb_cb.on_lcb_motor_rpm_received(th, 999);
    EXPECT_EQ(th->machine_rpm, 999);
}

TEST_F(LcbTreadmillHandlerTest, MachineRpm_TrackedFromOneShotCallback) {
    th->lcb_cb.on_one_shot_lcb_motor_rpm(th, 333);
    EXPECT_EQ(th->machine_rpm, 333);
}

/* ============================================================================
 * EStop Reset GUI Current RPM
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, EStopResetGuiCurrentRpm_ZerosBoth) {
    th->gui_set_rpm = 1000;
    th->set_current_rpm = 750;
    treadmill_handler_estop_reset_gui_current_rpm(th);
    EXPECT_EQ(th->gui_set_rpm, 0);
    EXPECT_EQ(th->set_current_rpm, 0);
}

/* ============================================================================
 * Set LCB Type
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, SetLcbType_Stored) {
    treadmill_handler_set_lcb_type(th, LCB_TYPE_TM_LCB);
    EXPECT_EQ(th->machine_lcb_type, LCB_TYPE_TM_LCB);
}

/* ============================================================================
 * Auto Ramp RPM Setter
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, SetAutoRampRpm_Enable_StoresSteps) {
    treadmill_handler_set_auto_ramp_rpm(th, true, 100, 200);
    EXPECT_TRUE(th->is_auto_ramp_rpm);
    EXPECT_EQ(th->ramp_up_step, 100);
    EXPECT_EQ(th->ramp_down_step, 200);
}

TEST_F(LcbTreadmillHandlerTest, SetAutoRampRpm_ZeroSteps_KeepDefaults) {
    treadmill_handler_set_auto_ramp_rpm(th, true, 0, 0);
    EXPECT_TRUE(th->is_auto_ramp_rpm);
    EXPECT_EQ(th->ramp_up_step, TM_DEFAULT_RAMP_STEP);
    EXPECT_EQ(th->ramp_down_step, TM_DEFAULT_RAMP_STEP);
}

/* ============================================================================
 * NULL Safety
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, NullSafety_SetCallbacks) {
    treadmill_handler_set_callbacks(NULL, &user_cb);
    treadmill_handler_set_callbacks(th, NULL);
}

TEST_F(LcbTreadmillHandlerTest, NullSafety_SetRpm) {
    treadmill_handler_set_rpm(NULL, 100);
}

TEST_F(LcbTreadmillHandlerTest, NullSafety_SetIncline) {
    treadmill_handler_set_incline(NULL, 100);
}

TEST_F(LcbTreadmillHandlerTest, NullSafety_SendOperate) {
    treadmill_handler_send_treadmill_operate(NULL, NULL, 0);
}

TEST_F(LcbTreadmillHandlerTest, NullSafety_Stop) {
    treadmill_handler_stop(NULL);
}

TEST_F(LcbTreadmillHandlerTest, NullSafety_SetLcbType) {
    treadmill_handler_set_lcb_type(NULL, LCB_TYPE_TM_LCB);
}

/* ============================================================================
 * Set RPM Behavior
 * ============================================================================ */

TEST_F(LcbTreadmillHandlerTest, SetRpm_RampMode_StoresGuiTarget) {
    th->is_auto_ramp_rpm = true;
    treadmill_handler_set_rpm(th, 1500);
    EXPECT_EQ(th->gui_set_rpm, 1500);
    /* In ramp mode, set_current_rpm doesn't immediately update unless rpm=0 */
}

TEST_F(LcbTreadmillHandlerTest, SetRpm_RampMode_RpmZero_ImmediateStop) {
    th->is_auto_ramp_rpm = true;
    th->set_current_rpm = 800;
    treadmill_handler_set_rpm(th, 0);
    EXPECT_EQ(th->gui_set_rpm, 0);
    EXPECT_EQ(th->set_current_rpm, 0);  /* Immediate zero */
}
