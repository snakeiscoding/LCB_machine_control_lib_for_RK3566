/**
 * @file test_lcb_bike_handler.cpp
 * @brief Unit tests for LCB Bike Handler (MachineHandler/Bike layer)
 *
 * Tests lifecycle, callback bridge, auto-reply interval counters, LCB-type
 * dependent watt/resistance command dispatch, and NULL safety.
 *
 * NOTE: Tests do NOT spawn the heartbeat thread. They drive logic by calling
 * the internal helper (bh_proc_auto_reply_tick) directly with comm = NULL,
 * except the watt/resistance dispatch tests which attach a real (unstarted)
 * lcb_comm_t so the TX queue can be inspected to confirm the correct command
 * path was taken.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "bike_handler.h"
#include "bike_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_type.h"
}

/* ============================================================================
 * Test Context — captures user callback invocations
 * ============================================================================ */

struct BHTestContext {
    int on_bike_rpm_count;          uint16_t last_rpm;
    int on_bike_one_shot_rpm_count; uint16_t last_one_shot_rpm;

    void reset() { memset(this, 0, sizeof(*this)); }
};

/* ============================================================================
 * Trampolines
 * ============================================================================ */

static void bh_test_on_rpm(void *ud, uint16_t rpm) {
    auto *c = (BHTestContext*)ud; c->on_bike_rpm_count++; c->last_rpm = rpm;
}
static void bh_test_on_one_shot_rpm(void *ud, uint16_t rpm) {
    auto *c = (BHTestContext*)ud; c->on_bike_one_shot_rpm_count++; c->last_one_shot_rpm = rpm;
}

/* ============================================================================
 * Setup Helper
 * ============================================================================ */

static void setup_bh_test_callbacks(bike_handler_callbacks_t *cb, BHTestContext *ctx) {
    bike_handler_callbacks_init(cb);
    cb->user_data = ctx;
    cb->on_bike_rpm = bh_test_on_rpm;
    cb->on_bike_one_shot_rpm = bh_test_on_one_shot_rpm;
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

class LcbBikeHandlerTest : public ::testing::Test {
protected:
    BHTestContext ctx;
    bike_handler_t *bh;
    bike_handler_callbacks_t user_cb;

    void SetUp() override {
        ctx.reset();
        bh = bike_handler_create();
        ASSERT_NE(bh, nullptr);
        setup_bh_test_callbacks(&user_cb, &ctx);
        bike_handler_set_callbacks(bh, &user_cb);
        /* Wire internal LCB callbacks so we can invoke them directly */
        bh_lcb_cb_register(bh);
    }

    void TearDown() override {
        bike_handler_destroy(bh);
    }
};

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

TEST_F(LcbBikeHandlerTest, Create_DefaultsAreSet) {
    EXPECT_EQ(bh->machine_lcb_type, LCB_TYPE_UNKNOWN);
    EXPECT_FALSE(bh->is_running);
    EXPECT_FALSE(bh->is_auto_reply_rpm);
    EXPECT_FALSE(bh->is_auto_resend_watt);
    EXPECT_EQ(bh->resend_watt, 0);
}

TEST_F(LcbBikeHandlerTest, Destroy_NullSafe) {
    bike_handler_destroy(NULL);
}

TEST_F(LcbBikeHandlerTest, CallbacksInit_ZerosAllFields) {
    bike_handler_callbacks_t cb;
    memset(&cb, 0xFF, sizeof(cb));
    bike_handler_callbacks_init(&cb);
    EXPECT_EQ(cb.user_data, nullptr);
    EXPECT_EQ(cb.on_bike_rpm, nullptr);
    EXPECT_EQ(cb.on_bike_one_shot_rpm, nullptr);
}

TEST_F(LcbBikeHandlerTest, CallbacksInit_NullSafe) {
    bike_handler_callbacks_init(NULL);
}

/* ============================================================================
 * Callback Registration
 * ============================================================================ */

TEST_F(LcbBikeHandlerTest, SetCallbacks_StoresPointers) {
    bh->lcb_cb.on_lcb_ecb_rpm(bh, 120);
    EXPECT_EQ(ctx.on_bike_rpm_count, 1);
    EXPECT_EQ(ctx.last_rpm, 120);
}

TEST_F(LcbBikeHandlerTest, UnregisterCallbacks_ClearsPointers) {
    bike_handler_unregister_callbacks(bh);
    bh->lcb_cb.on_lcb_ecb_rpm(bh, 90);
    EXPECT_EQ(ctx.on_bike_rpm_count, 0);
}

TEST_F(LcbBikeHandlerTest, SetCallbacks_OverwritesPrevious) {
    bike_handler_callbacks_t cb2;
    bike_handler_callbacks_init(&cb2);
    cb2.user_data = &ctx;
    /* No on_bike_rpm assigned */
    bike_handler_set_callbacks(bh, &cb2);
    bh->lcb_cb.on_lcb_ecb_rpm(bh, 90);
    EXPECT_EQ(ctx.on_bike_rpm_count, 0);
}

/* ============================================================================
 * Auto-reply Toggles
 * ============================================================================ */

TEST_F(LcbBikeHandlerTest, Toggle_Rpm) {
    bike_handler_set_auto_reply_rpm(bh, true);
    EXPECT_TRUE(bh->is_auto_reply_rpm);
    bike_handler_set_auto_reply_rpm(bh, false);
    EXPECT_FALSE(bh->is_auto_reply_rpm);
}

TEST_F(LcbBikeHandlerTest, Toggle_ResendWatt) {
    bike_handler_set_auto_resend_watt(bh, true);
    EXPECT_TRUE(bh->is_auto_resend_watt);
    bike_handler_set_auto_resend_watt(bh, false);
    EXPECT_FALSE(bh->is_auto_resend_watt);
}

/* ============================================================================
 * Interval Counter — Both features have interval == 0 (Java: TIMES_RPM_INTERVAL /
 * TIMES_SEND_WATT_INTERVAL are both 0), so every tick fires and cnt stays 0.
 * ============================================================================ */

TEST_F(LcbBikeHandlerTest, IntervalCounter_RpmInterval0_AlwaysFires) {
    bh->is_auto_reply_rpm = true;
    bh_proc_auto_reply_tick(bh);
    EXPECT_EQ(bh->cnt_rpm, 0);
    bh_proc_auto_reply_tick(bh);
    EXPECT_EQ(bh->cnt_rpm, 0);
}

TEST_F(LcbBikeHandlerTest, IntervalCounter_SendWattInterval0_AlwaysFires) {
    bh->is_auto_resend_watt = true;
    bh_proc_auto_reply_tick(bh);
    EXPECT_EQ(bh->cnt_send_watt, 0);
    bh_proc_auto_reply_tick(bh);
    EXPECT_EQ(bh->cnt_send_watt, 0);
}

TEST_F(LcbBikeHandlerTest, IntervalCounter_FlagsDisabled_NoIncrementNoCrash) {
    bh->is_auto_reply_rpm = false;
    bh->is_auto_resend_watt = false;
    bh_proc_auto_reply_tick(bh);
    bh_proc_auto_reply_tick(bh);
    EXPECT_EQ(bh->cnt_rpm, 0);
    EXPECT_EQ(bh->cnt_send_watt, 0);
}

/* ============================================================================
 * Watt/Resistance Command Dispatch (LCB-type dependent)
 *
 * Attaches a real (never-started) lcb_comm_t so lcb_comm_get_tx_queue_size()
 * can confirm a packet was actually enqueued for the selected branch — an
 * honest test of dispatch, since with comm == NULL the function short-circuits
 * before reaching the switch at all (matching Java's `if (mLcb != null)` guard).
 * ============================================================================ */

class LcbBikeHandlerWattDispatchTest : public LcbBikeHandlerTest {
protected:
    lcb_comm_t *comm = nullptr;

    void SetUp() override {
        LcbBikeHandlerTest::SetUp();
        comm = lcb_comm_create();
        ASSERT_NE(comm, nullptr);
        bh->comm = comm;
    }

    void TearDown() override {
        bh->comm = nullptr; /* detach before destroying comm to avoid UAF in bike_handler_stop */
        lcb_comm_destroy(comm);
        LcbBikeHandlerTest::TearDown();
    }
};

TEST_F(LcbBikeHandlerWattDispatchTest, Lcb1Current_EnqueuesPwmPercent) {
    bh->machine_lcb_type = LCB_TYPE_LCB1_CURRENT;
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    bike_handler_set_watt_or_resistance(bh, 50);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);
    EXPECT_EQ(bh->resend_watt, 0); /* only cached for LCBA/1x-Bike branches */
}

TEST_F(LcbBikeHandlerWattDispatchTest, Lcb3Current_EnqueuesPwmPercent) {
    bh->machine_lcb_type = LCB_TYPE_LCB3_CURRENT;
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    bike_handler_set_watt_or_resistance(bh, 50);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);
}

TEST_F(LcbBikeHandlerWattDispatchTest, Lcba_EnqueuesWattsAndCachesResend) {
    bh->machine_lcb_type = LCB_TYPE_LCBA;
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    bike_handler_set_watt_or_resistance(bh, 150);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);
    EXPECT_EQ(bh->resend_watt, 150);
}

TEST_F(LcbBikeHandlerWattDispatchTest, OneXBikeLcb_EnqueuesWattsAndCachesResend) {
    bh->machine_lcb_type = LCB_TYPE_1X_BIKE_LCB;
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    bike_handler_set_watt_or_resistance(bh, 220);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);
    EXPECT_EQ(bh->resend_watt, 220);
}

TEST_F(LcbBikeHandlerWattDispatchTest, EcbBoard_EnqueuesEcbPosition) {
    bh->machine_lcb_type = LCB_TYPE_ECB_BOARD;
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    bike_handler_set_watt_or_resistance(bh, 300);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);
    EXPECT_EQ(bh->resend_watt, 0); /* ECB branch does not cache resend_watt */
}

TEST_F(LcbBikeHandlerWattDispatchTest, UnknownType_DefaultsToPwmPercent) {
    bh->machine_lcb_type = LCB_TYPE_UNKNOWN;
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    bike_handler_set_watt_or_resistance(bh, 75);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);
    EXPECT_EQ(bh->resend_watt, 0);
}

TEST_F(LcbBikeHandlerWattDispatchTest, AutoResendWatt_UsesCachedValue) {
    /* Set the cached resend value directly rather than via
     * set_watt_or_resistance() first — that call would already enqueue a
     * LCB_QUEUE_MODE_REPLACE_SIMILAR "set watts" packet, and the tick's
     * resend uses the same queue mode/command, so it would *replace* (not
     * add to) that still-queued packet, leaving the queue size unchanged
     * even though the resend genuinely fired. */
    bh->resend_watt = 400;
    bh->is_auto_resend_watt = true;
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    bh_proc_auto_reply_tick(bh);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);
}

TEST_F(LcbBikeHandlerWattDispatchTest, Tick_SendsStatusHeartbeat_EvenWithAutoFlagsOff) {
    /* Regression test: after Initial Bike hands the shared comm over from
     * general_handler to bike_handler, nothing else keeps the RS485 bus
     * alive unless bike_handler's own heartbeat does it. is_auto_reply_rpm
     * and is_auto_resend_watt both default false, so this must fire on its
     * own — matching Java's GeneralHandler, which never stops polling
     * status regardless of BikeHandler's auto-reply toggles. */
    ASSERT_FALSE(bh->is_auto_reply_rpm);
    ASSERT_FALSE(bh->is_auto_resend_watt);
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    bh_proc_auto_reply_tick(bh);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);
}

/* ============================================================================
 * Callback Bridge Forwarding
 * ============================================================================ */

TEST_F(LcbBikeHandlerTest, Bridge_Rpm_Forwards) {
    bh->lcb_cb.on_lcb_ecb_rpm(bh, 175);
    EXPECT_EQ(ctx.on_bike_rpm_count, 1);
    EXPECT_EQ(ctx.last_rpm, 175);
}

TEST_F(LcbBikeHandlerTest, Bridge_OneShotRpm_Forwards) {
    bh->lcb_cb.on_one_shot_lcb_ecb_rpm(bh, 88);
    EXPECT_EQ(ctx.on_bike_one_shot_rpm_count, 1);
    EXPECT_EQ(ctx.last_one_shot_rpm, 88);
}

/* ============================================================================
 * Set LCB Type
 * ============================================================================ */

TEST_F(LcbBikeHandlerTest, SetLcbType_Stored) {
    bike_handler_set_lcb_type(bh, LCB_TYPE_LCBA);
    EXPECT_EQ(bh->machine_lcb_type, LCB_TYPE_LCBA);
}

/* ============================================================================
 * NULL Safety
 * ============================================================================ */

TEST_F(LcbBikeHandlerTest, NullSafety_SetCallbacks) {
    bike_handler_set_callbacks(NULL, &user_cb);
    bike_handler_set_callbacks(bh, NULL);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetWattOrResistance) {
    bike_handler_set_watt_or_resistance(NULL, 100);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetGearRatio) {
    bike_handler_set_gear_ratio(NULL, 100);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetResistanceType) {
    bike_handler_set_resistance_type(NULL, 1);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetLowLimitResistance) {
    bike_handler_set_low_limit_resistance(NULL, 50);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetLowLimitCharge) {
    bike_handler_set_low_limit_charge(NULL, 50);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetTurnOffLcb) {
    bike_handler_set_turn_off_lcb(NULL, 10);
}

TEST_F(LcbBikeHandlerTest, NullSafety_GetOneShotRpm) {
    bike_handler_get_one_shot_rpm(NULL);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetAutoReplyRpm) {
    bike_handler_set_auto_reply_rpm(NULL, true);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetAutoResendWatt) {
    bike_handler_set_auto_resend_watt(NULL, true);
}

TEST_F(LcbBikeHandlerTest, NullSafety_Stop) {
    bike_handler_stop(NULL);
}

TEST_F(LcbBikeHandlerTest, NullSafety_SetLcbType) {
    bike_handler_set_lcb_type(NULL, LCB_TYPE_LCBA);
}

TEST_F(LcbBikeHandlerTest, NullSafety_CommNull_WattOrResistance_NoCrash) {
    /* bh->comm is NULL in the base fixture — function must short-circuit */
    bh->machine_lcb_type = LCB_TYPE_LCBA;
    bike_handler_set_watt_or_resistance(bh, 100);
    EXPECT_EQ(bh->resend_watt, 0);
}
