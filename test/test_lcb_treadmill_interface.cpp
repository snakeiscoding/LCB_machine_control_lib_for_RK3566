/**
 * @file test_lcb_treadmill_interface.cpp
 * @brief Unit tests for the Treadmill safety/orchestration adapter (adapters layer)
 *
 * Tests ext-status bit parsing, safety-key dispatch (incl. the periodic
 * 15-invocation re-announce and the real unlock TX it triggers), auto-OC
 * edge-triggering, person-in-place mapping, safety-gated TX commands, and
 * NULL safety.
 *
 * NOTE: Tests drive ti_proc_ext_status() directly (no heartbeat thread). For
 * the unlock-TX assertions, a real (never-started) lcb_comm_t is attached
 * directly to the internal treadmill_handler so lcb_comm_get_tx_queue_size()
 * can confirm the unlock command was actually enqueued.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "treadmill_interface.h"
#include "treadmill_interface_internal.h"
#include "treadmill_handler.h"
#include "treadmill_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_type.h"
}

/* ============================================================================
 * Test Context — captures user callback invocations
 * ============================================================================ */

struct TITestContext {
    int on_rpm_count;                uint16_t last_rpm;
    int on_one_shot_rpm_count;       uint16_t last_one_shot_rpm;
    int on_incline_adc_count;        uint16_t last_incline_adc;
    int on_person_in_place_count;    bool last_in_place;
    int on_safetykey_lock_state_count; bool last_locked;
    int on_start_unlock_count;
    int on_auto_oc_state_count;      bool last_auto_oc;
    int on_lcb_set_rpm_value_count;  uint16_t last_set_rpm;
    int on_lcb_set_adc_value_count;  uint16_t last_set_adc;
    int on_error_code_count;        uint16_t last_error_code;

    void reset() { memset(this, 0, sizeof(*this)); }
};

static void ti_test_on_rpm(void *ud, uint16_t rpm) {
    auto *c = (TITestContext*)ud; c->on_rpm_count++; c->last_rpm = rpm;
}
static void ti_test_on_one_shot_rpm(void *ud, uint16_t rpm) {
    auto *c = (TITestContext*)ud; c->on_one_shot_rpm_count++; c->last_one_shot_rpm = rpm;
}
static void ti_test_on_incline_adc(void *ud, uint16_t adc) {
    auto *c = (TITestContext*)ud; c->on_incline_adc_count++; c->last_incline_adc = adc;
}
static void ti_test_on_person_in_place(void *ud, bool in_place) {
    auto *c = (TITestContext*)ud; c->on_person_in_place_count++; c->last_in_place = in_place;
}
static void ti_test_on_safetykey_lock_state(void *ud, bool locked) {
    auto *c = (TITestContext*)ud; c->on_safetykey_lock_state_count++; c->last_locked = locked;
}
static void ti_test_on_start_unlock(void *ud) {
    auto *c = (TITestContext*)ud; c->on_start_unlock_count++;
}
static void ti_test_on_auto_oc_state(void *ud, bool detected) {
    auto *c = (TITestContext*)ud; c->on_auto_oc_state_count++; c->last_auto_oc = detected;
}
static void ti_test_on_lcb_set_rpm_value(void *ud, uint16_t rpm) {
    auto *c = (TITestContext*)ud; c->on_lcb_set_rpm_value_count++; c->last_set_rpm = rpm;
}
static void ti_test_on_lcb_set_adc_value(void *ud, uint16_t adc) {
    auto *c = (TITestContext*)ud; c->on_lcb_set_adc_value_count++; c->last_set_adc = adc;
}
static void ti_test_on_error_code(void *ud, uint16_t error_code) {
    auto *c = (TITestContext*)ud; c->on_error_code_count++; c->last_error_code = error_code;
}

static void setup_ti_test_callbacks(treadmill_interface_callbacks_t *cb, TITestContext *ctx) {
    treadmill_interface_callbacks_init(cb);
    cb->user_data = ctx;
    cb->on_rpm = ti_test_on_rpm;
    cb->on_one_shot_rpm = ti_test_on_one_shot_rpm;
    cb->on_incline_adc = ti_test_on_incline_adc;
    cb->on_person_in_place = ti_test_on_person_in_place;
    cb->on_safetykey_lock_state = ti_test_on_safetykey_lock_state;
    cb->on_start_unlock = ti_test_on_start_unlock;
    cb->on_auto_oc_state = ti_test_on_auto_oc_state;
    cb->on_lcb_set_rpm_value = ti_test_on_lcb_set_rpm_value;
    cb->on_lcb_set_adc_value = ti_test_on_lcb_set_adc_value;
    cb->on_error_code = ti_test_on_error_code;
}

/* ============================================================================
 * Test Fixture
 *
 * Attaches a real (never-started) treadmill_handler_t directly to the
 * adapter's internal `handler` field so ti_proc_ext_status()'s unlock-TX call
 * (treadmill_handler_send_estop_unlock) and the safety-gated TX commands have
 * something real to operate on, without spawning any threads.
 * ============================================================================ */

class LcbTreadmillInterfaceTest : public ::testing::Test {
protected:
    TITestContext ctx;
    treadmill_interface_t *ti;
    treadmill_interface_callbacks_t user_cb;
    treadmill_handler_t *th;

    void SetUp() override {
        ctx.reset();
        ti = treadmill_interface_create();
        ASSERT_NE(ti, nullptr);
        setup_ti_test_callbacks(&user_cb, &ctx);
        treadmill_interface_set_callbacks(ti, &user_cb);

        /* Attach a bare internal handler (no comm, no thread) so TX
         * passthroughs and ti_proc_ext_status() don't crash on a NULL
         * ti->handler. Tests that need to observe TX enqueue attach a real
         * lcb_comm_t via AttachRealComm(). */
        th = treadmill_handler_create();
        ti->handler = th;
    }

    void AttachRealComm(lcb_comm_t *comm) {
        th->comm = comm;
    }

    void TearDown() override {
        /* ti->handler == th; treadmill_interface_destroy will stop+destroy it. */
        treadmill_interface_destroy(ti);
    }
};

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

TEST_F(LcbTreadmillInterfaceTest, Create_DefaultsAreSet) {
    EXPECT_EQ(ti->machine_lcb_type, LCB_TYPE_UNKNOWN);
    EXPECT_FALSE(ti->safetykey_down);
    EXPECT_FALSE(ti->safetykey_lock);
    EXPECT_FALSE(ti->auto_oc_detect);
    EXPECT_EQ(ti->old_status, 0);
    EXPECT_EQ(ti->update_safety_st_count, 0);
}

TEST_F(LcbTreadmillInterfaceTest, Destroy_NullSafe) {
    treadmill_interface_destroy(NULL);
}

TEST_F(LcbTreadmillInterfaceTest, CallbacksInit_ZerosAllFields) {
    treadmill_interface_callbacks_t cb;
    memset(&cb, 0xFF, sizeof(cb));
    treadmill_interface_callbacks_init(&cb);
    EXPECT_EQ(cb.user_data, nullptr);
    EXPECT_EQ(cb.on_rpm, nullptr);
    EXPECT_EQ(cb.on_error_code, nullptr);
}

TEST_F(LcbTreadmillInterfaceTest, CallbacksInit_NullSafe) {
    treadmill_interface_callbacks_init(NULL);
}

/* ============================================================================
 * Ext-Status Bit Parsing
 * ============================================================================ */

TEST_F(LcbTreadmillInterfaceTest, ExtStatus_Bit0_SafetyDown) {
    ti_proc_ext_status(ti, 0x0001);
    EXPECT_TRUE(ti->safetykey_down);
    EXPECT_EQ(ctx.on_safetykey_lock_state_count, 1);
    EXPECT_TRUE(ctx.last_locked);
    EXPECT_EQ(ctx.on_start_unlock_count, 0); /* down (not lock-only) does not send unlock */
}

TEST_F(LcbTreadmillInterfaceTest, ExtStatus_Bit1_LockOnly_SendsUnlock) {
    ti_proc_ext_status(ti, 0x0002);
    EXPECT_TRUE(ti->safetykey_lock);
    EXPECT_FALSE(ti->safetykey_down);
    EXPECT_EQ(ctx.on_safetykey_lock_state_count, 1);
    EXPECT_TRUE(ctx.last_locked);
    EXPECT_EQ(ctx.on_start_unlock_count, 1);
}

TEST_F(LcbTreadmillInterfaceTest, ExtStatus_Bit2Clear_ZeroRpmTrue) {
    /* zero_rpm = bit2 == 0. old_status defaults to 0, same as the first
     * status value below, so force a reparse rather than relying on the
     * default. */
    ti->old_status = -1;
    ti_proc_ext_status(ti, 0x0000);
    EXPECT_TRUE(ti->zero_rpm);
    ti->old_status = -1; /* force reparse */
    ti_proc_ext_status(ti, 0x0004);
    EXPECT_FALSE(ti->zero_rpm);
}

TEST_F(LcbTreadmillInterfaceTest, ExtStatus_Bit3_AutoOc) {
    ti_proc_ext_status(ti, 0x0008);
    EXPECT_TRUE(ti->auto_oc_detect);
    EXPECT_EQ(ctx.on_auto_oc_state_count, 1);
    EXPECT_TRUE(ctx.last_auto_oc);
}

TEST_F(LcbTreadmillInterfaceTest, ExtStatus_NoBitsSet_ReportsUnlocked) {
    ti->old_status = -1; /* force reparse (0 default == 0x0000 first value) */
    ti_proc_ext_status(ti, 0x0000);
    EXPECT_FALSE(ti->safetykey_down);
    EXPECT_FALSE(ti->safetykey_lock);
    EXPECT_EQ(ctx.on_safetykey_lock_state_count, 1);
    EXPECT_FALSE(ctx.last_locked);
}

TEST_F(LcbTreadmillInterfaceTest, ExtStatus_SameValue_NoReparse) {
    ti_proc_ext_status(ti, 0x0001);
    int count_after_first = ctx.on_safetykey_lock_state_count;
    ti_proc_ext_status(ti, 0x0001); /* same value again */
    /* old_status unchanged -> parse/handle skipped (only periodic re-announce
     * counter increments, which won't fire until the 15th call) */
    EXPECT_EQ(ctx.on_safetykey_lock_state_count, count_after_first);
}

/* ============================================================================
 * Periodic Re-announce (every 15th invocation)
 * ============================================================================ */

TEST_F(LcbTreadmillInterfaceTest, PeriodicReannounce_Fires15thInvocation) {
    ti_proc_ext_status(ti, 0x0000); /* invocation 1: parses (status changed from 0 default... */
    /* old_status starts at 0 and 0x0000 == 0, so this first call does NOT
     * reparse (no change) but DOES count toward the periodic counter. */
    int initial_count = ctx.on_safetykey_lock_state_count;
    for (int i = 0; i < 13; i++) ti_proc_ext_status(ti, 0x0000); /* invocations 2-14 */
    EXPECT_EQ(ctx.on_safetykey_lock_state_count, initial_count);
    ti_proc_ext_status(ti, 0x0000); /* invocation 15 -> periodic re-announce fires */
    EXPECT_EQ(ctx.on_safetykey_lock_state_count, initial_count + 1);
    EXPECT_EQ(ti->update_safety_st_count, 0); /* resets after firing */
}

/* ============================================================================
 * Auto-OC Edge Trigger
 * ============================================================================ */

TEST_F(LcbTreadmillInterfaceTest, AutoOc_OnlyFiresOnChange) {
    ti_proc_ext_status(ti, 0x0008);
    EXPECT_EQ(ctx.on_auto_oc_state_count, 1);
    ti->old_status = -1; /* force reparse with same auto_oc bit */
    ti_proc_ext_status(ti, 0x0008);
    EXPECT_EQ(ctx.on_auto_oc_state_count, 1); /* unchanged -> no second fire */
    ti->old_status = -1;
    ti_proc_ext_status(ti, 0x0000); /* bit cleared -> change -> fires again */
    EXPECT_EQ(ctx.on_auto_oc_state_count, 2);
    EXPECT_FALSE(ctx.last_auto_oc);
}

/* ============================================================================
 * Person In Place / Speed-Up-In-Used
 *
 * Wire the adapter's real bridge callbacks (ti_build_handler_callbacks — the
 * same helper treadmill_interface_start() uses) onto th WITHOUT calling
 * _start(), so no heartbeat thread runs, then drive them via
 * th->user_cb.on_treadmill_*(...) directly.
 * ============================================================================ */

class LcbTreadmillInterfaceBridgeTest : public LcbTreadmillInterfaceTest {
protected:
    void SetUp() override {
        LcbTreadmillInterfaceTest::SetUp();
        treadmill_handler_callbacks_t handler_cb;
        ti_build_handler_callbacks(ti, &handler_cb);
        treadmill_handler_set_callbacks(th, &handler_cb);
    }
};

TEST_F(LcbTreadmillInterfaceBridgeTest, InUsed_0xFF_ReportsInPlaceTrueAndTogglesSpeedUp) {
    th->user_cb.on_treadmill_in_used(th->user_cb.user_data, 0xFF);
    EXPECT_EQ(ctx.on_person_in_place_count, 1);
    EXPECT_TRUE(ctx.last_in_place);
    EXPECT_FALSE(th->is_auto_reply_speed_up_in_used);
}

TEST_F(LcbTreadmillInterfaceBridgeTest, InUsed_0x55_ReportsInPlaceFalseAndEnablesSpeedUp) {
    th->user_cb.on_treadmill_in_used(th->user_cb.user_data, 0x55);
    EXPECT_EQ(ctx.on_person_in_place_count, 1);
    EXPECT_FALSE(ctx.last_in_place);
    EXPECT_TRUE(th->is_auto_reply_speed_up_in_used);
}

TEST_F(LcbTreadmillInterfaceBridgeTest, ExtStatus_ForwardedThroughHandlerBridge) {
    th->user_cb.on_treadmill_ext_status(th->user_cb.user_data, 0x0001);
    EXPECT_TRUE(ti->safetykey_down);
    EXPECT_EQ(ctx.on_safetykey_lock_state_count, 1);
}

TEST_F(LcbTreadmillInterfaceBridgeTest, RpmAndInclineAndGuiMirror_ForwardThroughHandlerBridge) {
    th->user_cb.on_treadmill_rpm(th->user_cb.user_data, 1500);
    EXPECT_EQ(ctx.on_rpm_count, 1);
    EXPECT_EQ(ctx.last_rpm, 1500);

    th->user_cb.on_treadmill_one_shot_rpm(th->user_cb.user_data, 800);
    EXPECT_EQ(ctx.on_one_shot_rpm_count, 1);

    th->user_cb.on_treadmill_incline_loc(th->user_cb.user_data, 1234);
    EXPECT_EQ(ctx.on_incline_adc_count, 1);
    EXPECT_EQ(ti->current_incline, 1234);

    th->user_cb.on_treadmill_lcb_set_rpm_value(th->user_cb.user_data, 900);
    EXPECT_EQ(ctx.on_lcb_set_rpm_value_count, 1);

    th->user_cb.on_treadmill_lcb_set_adc_value(th->user_cb.user_data, 2048);
    EXPECT_EQ(ctx.on_lcb_set_adc_value_count, 1);

    th->user_cb.on_treadmill_error_code(th->user_cb.user_data, 0x02B2);
    EXPECT_EQ(ctx.on_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x02B2);
}

/* ============================================================================
 * Safety Gating
 * ============================================================================ */

TEST_F(LcbTreadmillInterfaceTest, SetRpm_BlockedWhenSafetyDown) {
    lcb_comm_t *comm = lcb_comm_create();
    ASSERT_NE(comm, nullptr);
    AttachRealComm(comm);

    ti_proc_ext_status(ti, 0x0001); /* safetykey_down = true */
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    treadmill_interface_set_rpm(ti, 1000);
    treadmill_interface_set_rpm_high_priority(ti, 1000);
    EXPECT_EQ(lcb_comm_get_tx_queue_size(comm), before); /* nothing enqueued */

    th->comm = nullptr; /* detach before destroying comm (avoid UAF in stop) */
    lcb_comm_destroy(comm);
}

TEST_F(LcbTreadmillInterfaceTest, SetRpm_AllowedWhenSafetyClear) {
    lcb_comm_t *comm = lcb_comm_create();
    ASSERT_NE(comm, nullptr);
    AttachRealComm(comm);

    ti_proc_ext_status(ti, 0x0000); /* clear */
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    treadmill_interface_set_rpm(ti, 1000);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);

    th->comm = nullptr;
    lcb_comm_destroy(comm);
}

TEST_F(LcbTreadmillInterfaceTest, SetIncline_NotGatedBySafety) {
    lcb_comm_t *comm = lcb_comm_create();
    ASSERT_NE(comm, nullptr);
    AttachRealComm(comm);

    ti_proc_ext_status(ti, 0x0001); /* safetykey_down = true */
    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    treadmill_interface_set_incline(ti, 500);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before); /* incline is NOT gated */

    th->comm = nullptr;
    lcb_comm_destroy(comm);
}

TEST_F(LcbTreadmillInterfaceTest, LockOnly_InjectsUnlockNotification) {
    /* lcb_comm_proc_estop_unlock() (called by
     * treadmill_handler_send_estop_unlock()) is a local software
     * notification — it injects the 0x02B1 error code through whichever
     * callbacks are registered on comm, it does NOT enqueue a TX packet (see
     * that function's doc comment). Register the handler's own lcb_cb so the
     * injected code round-trips through th_on_lcb_error_code ->
     * on_treadmill_error_code -> this adapter's on_error_code, proving the
     * lock-only path really does notify the app of the unlock condition. */
    lcb_comm_t *comm = lcb_comm_create();
    ASSERT_NE(comm, nullptr);
    AttachRealComm(comm);

    treadmill_handler_callbacks_t handler_cb;
    ti_build_handler_callbacks(ti, &handler_cb);
    treadmill_handler_set_callbacks(th, &handler_cb);
    th_lcb_cb_register(th);
    lcb_comm_register_callbacks(comm, &th->lcb_cb);

    ti_proc_ext_status(ti, 0x0002); /* lock-only */
    EXPECT_EQ(ctx.on_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x02B1);

    lcb_comm_unregister_callbacks(comm);
    th->comm = nullptr;
    lcb_comm_destroy(comm);
}

/* Error-code forwarding through the full handler bridge is covered by
 * RpmAndInclineAndGuiMirror_ForwardThroughHandlerBridge above. */

/* ============================================================================
 * NULL Safety
 * ============================================================================ */

TEST_F(LcbTreadmillInterfaceTest, NullSafety_SetCallbacks) {
    treadmill_interface_set_callbacks(NULL, &user_cb);
    treadmill_interface_set_callbacks(ti, NULL);
}

TEST_F(LcbTreadmillInterfaceTest, NullSafety_SetRpm) {
    treadmill_interface_set_rpm(NULL, 100);
}

TEST_F(LcbTreadmillInterfaceTest, NullSafety_SetIncline) {
    treadmill_interface_set_incline(NULL, 100);
}

TEST_F(LcbTreadmillInterfaceTest, NullSafety_Stop) {
    treadmill_interface_stop(NULL);
}

TEST_F(LcbTreadmillInterfaceTest, NullSafety_SetLcbType) {
    treadmill_interface_set_lcb_type(NULL, LCB_TYPE_TM_LCB);
}

TEST_F(LcbTreadmillInterfaceTest, NullSafety_ProcExtStatus) {
    ti_proc_ext_status(NULL, 0x0001);
}

TEST_F(LcbTreadmillInterfaceTest, NullSafety_Start) {
    EXPECT_FALSE(treadmill_interface_start(NULL, NULL));
}
