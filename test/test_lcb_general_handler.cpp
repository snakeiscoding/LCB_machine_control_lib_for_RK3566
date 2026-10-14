/**
 * @file test_lcb_general_handler.cpp
 * @brief Unit tests for LCB General Handler (MachineHandler layer)
 *
 * Tests lifecycle, init state machine (TM/BIKE), error state machine,
 * type discovery, serial number parsing, peripheral callbacks, and NULL safety.
 *
 * NOTE: These tests exercise the general_handler logic WITHOUT a real serial port
 * by directly manipulating internal state and invoking registered LCB callbacks.
 */

#include <gtest/gtest.h>
#include <cstring>

extern "C" {
#include "general_handler.h"
#include "general_handler_internal.h"
#include "lcb_comm_callback.h"
#include "lcb_type.h"
#include "lcb_proc_comm.h"
#include "lcb_constants.h"

/* Internal functions from general_handler.c */
extern void gh_proc_initial_lcb(general_handler_t *gh);
extern void gh_auto_initial_lcb_cmd(general_handler_t *gh);
extern bool gh_check_in_get_error_state(general_handler_t *gh);
extern void gh_run_get_error_processes(general_handler_t *gh);
extern void gh_handle_exception_error_code(general_handler_t *gh, uint16_t error_code);
extern void gh_set_machine_lcb_type(general_handler_t *gh, uint8_t type_byte);
extern void gh_set_reconnect_deadline(general_handler_t *gh);

/* Internal functions from general_handler_lcb_cb.c */
extern void gh_lcb_cb_register(general_handler_t *gh);
extern void gh_lcb_cb_unregister(general_handler_t *gh);
}

/* ============================================================================
 * Test Context — captures user callback invocations
 * ============================================================================ */

struct GHTestContext {
    /* Error & Version */
    int on_error_code_count;
    uint16_t last_error_code;
    int on_lcb_type_count;
    uint8_t last_lcb_type;
    int on_lcb_version_count;
    int last_version_major;
    int last_version_minor;

    /* Init */
    int on_lcb_initial_finish_count;

    /* Status bits */
    int on_incline_down_count;
    int on_main_motor_count;
    int on_cmd_error_count;
    bool last_incline_down;
    bool last_main_motor;
    bool last_cmd_error;

    /* Voltage & Status */
    int on_battery_voltage_count;
    uint16_t last_battery_voltage;
    int on_lcb_status_count;
    uint8_t last_lcb_status;

    /* Serial number */
    int on_clear_sn_count;
    bool last_clear_sn_success;
    int on_set_sn_count;
    bool last_set_sn_success;
    int on_get_sn_count;
    bool last_get_sn_success;
    char last_sn[256];
    int on_get_sn_max_len_count;
    bool last_sn_max_len_success;
    int last_sn_max_len;

    /* Peripheral */
    int on_magnet_angle_count;
    int last_magnet_angle;
    int on_moto_type_count;
    int last_moto_type;
    int on_mcb_voltage_count;
    int last_mcb_voltage;

    /* Update */
    int on_update_mode_count;
    bool last_update_mode_success;

    /* Reconnect */
    int on_reconnected_count;

    void reset() { memset(this, 0, sizeof(*this)); }
};

/* ============================================================================
 * Static Trampoline Functions
 * ============================================================================ */

static void gh_test_on_error_code(void *ud, uint16_t code) {
    auto *c = (GHTestContext*)ud; c->on_error_code_count++; c->last_error_code = code;
}
static void gh_test_on_lcb_type(void *ud, uint8_t t) {
    auto *c = (GHTestContext*)ud; c->on_lcb_type_count++; c->last_lcb_type = t;
}
static void gh_test_on_lcb_version(void *ud, int maj, int min) {
    auto *c = (GHTestContext*)ud; c->on_lcb_version_count++; c->last_version_major = maj; c->last_version_minor = min;
}
static void gh_test_on_initial_finish(void *ud) {
    auto *c = (GHTestContext*)ud; c->on_lcb_initial_finish_count++;
}
static void gh_test_on_incline_down(void *ud, bool s) {
    auto *c = (GHTestContext*)ud; c->on_incline_down_count++; c->last_incline_down = s;
}
static void gh_test_on_main_motor(void *ud, bool s) {
    auto *c = (GHTestContext*)ud; c->on_main_motor_count++; c->last_main_motor = s;
}
static void gh_test_on_cmd_error(void *ud, bool s) {
    auto *c = (GHTestContext*)ud; c->on_cmd_error_count++; c->last_cmd_error = s;
}
static void gh_test_on_battery_voltage(void *ud, uint16_t v) {
    auto *c = (GHTestContext*)ud; c->on_battery_voltage_count++; c->last_battery_voltage = v;
}
static void gh_test_on_lcb_status(void *ud, uint8_t s) {
    auto *c = (GHTestContext*)ud; c->on_lcb_status_count++; c->last_lcb_status = s;
}
static void gh_test_on_clear_sn(void *ud, bool ok) {
    auto *c = (GHTestContext*)ud; c->on_clear_sn_count++; c->last_clear_sn_success = ok;
}
static void gh_test_on_set_sn(void *ud, bool ok) {
    auto *c = (GHTestContext*)ud; c->on_set_sn_count++; c->last_set_sn_success = ok;
}
static void gh_test_on_get_sn(void *ud, bool ok, const char *sn) {
    auto *c = (GHTestContext*)ud; c->on_get_sn_count++; c->last_get_sn_success = ok;
    if (sn) strncpy(c->last_sn, sn, sizeof(c->last_sn) - 1);
}
static void gh_test_on_sn_max_len(void *ud, bool ok, int len) {
    auto *c = (GHTestContext*)ud; c->on_get_sn_max_len_count++; c->last_sn_max_len_success = ok; c->last_sn_max_len = len;
}
static void gh_test_on_magnet_angle(void *ud, int a) {
    auto *c = (GHTestContext*)ud; c->on_magnet_angle_count++; c->last_magnet_angle = a;
}
static void gh_test_on_moto_type(void *ud, int t) {
    auto *c = (GHTestContext*)ud; c->on_moto_type_count++; c->last_moto_type = t;
}
static void gh_test_on_mcb_voltage(void *ud, int v) {
    auto *c = (GHTestContext*)ud; c->on_mcb_voltage_count++; c->last_mcb_voltage = v;
}
static void gh_test_on_update_mode(void *ud, bool ok) {
    auto *c = (GHTestContext*)ud; c->on_update_mode_count++; c->last_update_mode_success = ok;
}
static void gh_test_on_reconnected(void *ud) {
    auto *c = (GHTestContext*)ud; c->on_reconnected_count++;
}

/* ============================================================================
 * Setup Helper
 * ============================================================================ */

static void setup_gh_test_callbacks(general_handler_callbacks_t *cb, GHTestContext *ctx) {
    general_handler_callbacks_init(cb);
    cb->user_data = ctx;
    cb->on_error_code = gh_test_on_error_code;
    cb->on_lcb_type = gh_test_on_lcb_type;
    cb->on_lcb_version = gh_test_on_lcb_version;
    cb->on_lcb_initial_finish = gh_test_on_initial_finish;
    cb->on_incline_motor_downward_status = gh_test_on_incline_down;
    cb->on_main_motor_status = gh_test_on_main_motor;
    cb->on_command_error_status = gh_test_on_cmd_error;
    cb->on_battery_voltage = gh_test_on_battery_voltage;
    cb->on_lcb_status = gh_test_on_lcb_status;
    cb->on_clear_lcb_sn_status = gh_test_on_clear_sn;
    cb->on_set_lcb_sn_status = gh_test_on_set_sn;
    cb->on_get_lcb_serial_number = gh_test_on_get_sn;
    cb->on_get_lcb_sn_max_length = gh_test_on_sn_max_len;
    cb->on_lcb_finish_magnet_angle_value = gh_test_on_magnet_angle;
    cb->on_mcb_moto_type = gh_test_on_moto_type;
    cb->on_get_mcb_voltage_value = gh_test_on_mcb_voltage;
    cb->on_into_lcb_update_mode = gh_test_on_update_mode;
    cb->on_lcb_comm_reconnected = gh_test_on_reconnected;
}

/* ============================================================================
 * Test Fixture
 * ============================================================================ */

class LcbGeneralHandlerTest : public ::testing::Test {
protected:
    GHTestContext ctx;
    general_handler_t *gh;
    general_handler_callbacks_t user_cb;

    void SetUp() override {
        ctx.reset();
        gh = general_handler_create();
        ASSERT_NE(gh, nullptr);
        setup_gh_test_callbacks(&user_cb, &ctx);
        general_handler_set_callbacks(gh, &user_cb);
        /* Register internal LCB callbacks so we can invoke them */
        gh_lcb_cb_register(gh);
    }

    void TearDown() override {
        general_handler_destroy(gh);
    }
};

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, Create_ReturnsNonNull) {
    /* Already verified in SetUp — just check defaults */
    EXPECT_EQ(gh->machine_lcb_type, LCB_TYPE_UNKNOWN);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_UNKNOWN);
    EXPECT_EQ(gh->init_state, GH_INIT_NONE);
    EXPECT_EQ(gh->error_state, GH_ERROR_NONE);
    EXPECT_FALSE(gh->is_running);
}

TEST_F(LcbGeneralHandlerTest, Destroy_Null_NoCrash) {
    general_handler_destroy(NULL);
}

TEST_F(LcbGeneralHandlerTest, CallbacksInit_ZerosAllFields) {
    general_handler_callbacks_t cb;
    memset(&cb, 0xFF, sizeof(cb));
    general_handler_callbacks_init(&cb);
    EXPECT_EQ(cb.user_data, nullptr);
    EXPECT_EQ(cb.on_error_code, nullptr);
    EXPECT_EQ(cb.on_lcb_initial_finish, nullptr);
}

TEST_F(LcbGeneralHandlerTest, CallbacksInit_NullSafe) {
    general_handler_callbacks_init(NULL);
}

/* ============================================================================
 * Machine Type Category Mapping
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, TypeMapping_TM_AcLcb) {
    gh_set_machine_lcb_type(gh, LCB_TYPE_AC_LCB);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_TM);
}

TEST_F(LcbGeneralHandlerTest, TypeMapping_TM_DcLcb) {
    gh_set_machine_lcb_type(gh, LCB_TYPE_DC_LCB);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_TM);
}

TEST_F(LcbGeneralHandlerTest, TypeMapping_TM_Toptek) {
    gh_set_machine_lcb_type(gh, LCB_TYPE_TOPTEK);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_TM);
}

TEST_F(LcbGeneralHandlerTest, TypeMapping_TM_Delta) {
    gh_set_machine_lcb_type(gh, LCB_TYPE_DELTA);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_TM);
}

TEST_F(LcbGeneralHandlerTest, TypeMapping_BIKE_Lcb1Pwm) {
    gh_set_machine_lcb_type(gh, LCB_TYPE_LCB1_PWM);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_BIKE);
}

TEST_F(LcbGeneralHandlerTest, TypeMapping_BIKE_Lcb2) {
    gh_set_machine_lcb_type(gh, LCB_TYPE_LCB2);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_BIKE);
}

TEST_F(LcbGeneralHandlerTest, TypeMapping_BIKE_Climbmill) {
    gh_set_machine_lcb_type(gh, LCB_TYPE_CLIMBMILL_OR_A30);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_BIKE);
}

TEST_F(LcbGeneralHandlerTest, TypeMapping_Unknown_DefaultsBIKE) {
    gh_set_machine_lcb_type(gh, 0xFE);  /* Unknown type */
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_BIKE);
}

/* ============================================================================
 * Init State Machine — TM Path
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, InitTM_ProcInitialLcb_SetsState) {
    gh->machine_category = MACHINE_CATEGORY_TM;
    gh_proc_initial_lcb(gh);
    EXPECT_EQ(gh->init_state, GH_INIT_TM_3_CMDS_INITIAL);
    EXPECT_FALSE(gh->initial_cmd_done);
    EXPECT_FALSE(gh->initial_finish);
}

TEST_F(LcbGeneralHandlerTest, InitTM_OnLcbInitial_TransitionsToOperatingStop) {
    gh->machine_category = MACHINE_CATEGORY_TM;
    gh->init_state = GH_INIT_TM_3_CMDS_INITIAL;

    /* Simulate 0x70 response */
    gh->lcb_cb.on_lcb_initial(gh);

    EXPECT_TRUE(gh->initial_cmd_done);
    EXPECT_EQ(gh->init_state, GH_INIT_CMD_OPERATING_STOP);
}

TEST_F(LcbGeneralHandlerTest, InitTM_OnOperatingStatus_StopToAction) {
    gh->machine_category = MACHINE_CATEGORY_TM;
    gh->init_state = GH_INIT_CMD_OPERATING_STOP;

    /* Simulate 0xF7 response */
    gh->lcb_cb.on_lcb_operating_status(gh);

    EXPECT_EQ(gh->init_state, GH_INIT_CMD_OPERATING_ACTION);
}

TEST_F(LcbGeneralHandlerTest, InitTM_OnOperatingStatus_ActionToFinishToNone) {
    gh->machine_category = MACHINE_CATEGORY_TM;
    gh->init_state = GH_INIT_CMD_OPERATING_ACTION;

    gh->lcb_cb.on_lcb_operating_status(gh);

    /* FINISH is transient — auto-advances to NONE */
    EXPECT_EQ(gh->init_state, GH_INIT_NONE);
    EXPECT_TRUE(gh->initial_finish);
    EXPECT_EQ(ctx.on_lcb_initial_finish_count, 1);
}

TEST_F(LcbGeneralHandlerTest, InitTM_FullSequence_FiresCallback) {
    gh->machine_category = MACHINE_CATEGORY_TM;
    gh_proc_initial_lcb(gh);

    /* Step 1: 0x70 response → TM_3_CMDS → OPERATING_STOP */
    gh->lcb_cb.on_lcb_initial(gh);
    EXPECT_EQ(gh->init_state, GH_INIT_CMD_OPERATING_STOP);

    /* Step 2: 0xF7 response → OPERATING_STOP → OPERATING_ACTION */
    gh->lcb_cb.on_lcb_operating_status(gh);
    EXPECT_EQ(gh->init_state, GH_INIT_CMD_OPERATING_ACTION);

    /* Step 3: 0xF7 response → OPERATING_ACTION → FINISH → NONE */
    gh->lcb_cb.on_lcb_operating_status(gh);
    /* FINISH triggers auto_initial which fires callback and transitions to NONE */
    EXPECT_EQ(gh->init_state, GH_INIT_NONE);
    EXPECT_TRUE(gh->initial_finish);
    EXPECT_EQ(ctx.on_lcb_initial_finish_count, 1);
}

/* ============================================================================
 * Init State Machine — BIKE Path
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, InitBIKE_FullSequence_FiresCallback) {
    gh->machine_category = MACHINE_CATEGORY_BIKE;
    gh_proc_initial_lcb(gh);

    /* TM_3_CMDS → DEFAULT_CMD_INITIAL (auto-converted for BIKE) */
    gh_auto_initial_lcb_cmd(gh);
    EXPECT_EQ(gh->init_state, GH_INIT_DEFAULT_CMD_INITIAL);

    /* 0x70 response → DEFAULT → FINISH → NONE */
    gh->lcb_cb.on_lcb_initial(gh);
    EXPECT_EQ(gh->init_state, GH_INIT_NONE);
    EXPECT_TRUE(gh->initial_finish);
    EXPECT_EQ(ctx.on_lcb_initial_finish_count, 1);
}

/* ============================================================================
 * Init State Machine — E-Stop Revert
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, InitFinish_EStopRevert_ClearsFlag) {
    gh->machine_category = MACHINE_CATEGORY_BIKE;
    gh->estop_revert = true;
    gh_proc_initial_lcb(gh);
    gh_auto_initial_lcb_cmd(gh);  /* TM_3_CMDS → DEFAULT */
    gh->lcb_cb.on_lcb_initial(gh);  /* DEFAULT → FINISH → NONE */

    EXPECT_FALSE(gh->estop_revert);
    EXPECT_EQ(ctx.on_lcb_initial_finish_count, 1);
}

/* ============================================================================
 * Error State Machine
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, ErrorState_None_ReturnsFalse) {
    gh->error_state = GH_ERROR_NONE;
    EXPECT_FALSE(gh_check_in_get_error_state(gh));
}

TEST_F(LcbGeneralHandlerTest, ErrorState_Get_ReturnsTrue) {
    gh->error_state = GH_ERROR_GET;
    EXPECT_TRUE(gh_check_in_get_error_state(gh));
}

TEST_F(LcbGeneralHandlerTest, ErrorState_Get_TimeoutAfter4) {
    gh->error_state = GH_ERROR_GET;
    for (int i = 0; i < 3; i++)
        gh_check_in_get_error_state(gh);
    EXPECT_EQ(gh->error_state, GH_ERROR_GET);

    /* 4th call triggers timeout reset */
    gh_check_in_get_error_state(gh);
    EXPECT_EQ(gh->error_state, GH_ERROR_NONE);
}

TEST_F(LcbGeneralHandlerTest, ErrorState_Skip_TimeoutAfter3) {
    gh->error_state = GH_ERROR_SKIP;
    for (int i = 0; i < 2; i++)
        gh_check_in_get_error_state(gh);
    EXPECT_EQ(gh->error_state, GH_ERROR_SKIP);

    /* 3rd call triggers timeout reset */
    gh_check_in_get_error_state(gh);
    EXPECT_EQ(gh->error_state, GH_ERROR_NONE);
}

/* ============================================================================
 * Error Callback — B6 Error Status Detected
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, ErrorDetected_TransitionsToGet) {
    gh->error_state = GH_ERROR_NONE;
    gh->lcb_cb.on_lcb_error_status_detected(gh, true);
    EXPECT_EQ(gh->error_state, GH_ERROR_GET);
}

TEST_F(LcbGeneralHandlerTest, ErrorDetected_False_NoTransition) {
    gh->error_state = GH_ERROR_NONE;
    gh->lcb_cb.on_lcb_error_status_detected(gh, false);
    EXPECT_EQ(gh->error_state, GH_ERROR_NONE);
}

TEST_F(LcbGeneralHandlerTest, ErrorDetected_AlreadyInGet_NoChange) {
    gh->error_state = GH_ERROR_GET;
    gh->lcb_cb.on_lcb_error_status_detected(gh, true);
    /* Should remain in GET (check_in_get_error_state returns true) */
    EXPECT_EQ(gh->error_state, GH_ERROR_GET);
}

TEST_F(LcbGeneralHandlerTest, ErrorDetected_SkipErrorStage_SetsInitialHaveError) {
    gh->error_state = GH_ERROR_NONE;
    gh->skip_error_stage = true;
    gh->lcb_cb.on_lcb_error_status_detected(gh, true);
    EXPECT_TRUE(gh->initial_have_error);
}

/* ============================================================================
 * Error Callback — Error Code Received (0x72)
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, ErrorCode_0x04B0_LogOnly_NoStateTransition) {
    gh->error_state = GH_ERROR_GET;
    gh->lcb_cb.on_lcb_error_code(gh, LCB_ERROR_COMM_LCB);
    /* 0x04B0 is log-only: state should remain GET */
    EXPECT_EQ(gh->error_state, GH_ERROR_GET);
}

TEST_F(LcbGeneralHandlerTest, ErrorCode_Normal_TransitionsGetToSkip) {
    gh->error_state = GH_ERROR_GET;
    gh->safe_key_stop_machine = false;
    gh->lcb_cb.on_lcb_error_code(gh, 0x0123);
    EXPECT_EQ(gh->error_state, GH_ERROR_SKIP);
}

TEST_F(LcbGeneralHandlerTest, ErrorCode_SafeKeyStop_BlocksTransition) {
    gh->error_state = GH_ERROR_GET;
    gh->safe_key_stop_machine = true;
    gh->lcb_cb.on_lcb_error_code(gh, 0x0123);
    /* safe_key blocks GET → SKIP transition */
    EXPECT_EQ(gh->error_state, GH_ERROR_GET);
}

/* ============================================================================
 * Error Callback — Return Error Code to UI
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, ReturnErrorToUI_Normal_FiresCallback) {
    gh->lcb_cb.on_return_error_code_to_ui(gh, 0x00A1);
    EXPECT_EQ(ctx.on_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, 0x00A1);
}

TEST_F(LcbGeneralHandlerTest, ReturnErrorToUI_Filtered_DuringSkipErrorStage) {
    gh->initial_have_error = true;
    gh->skip_error_stage = true;
    gh->lcb_cb.on_return_error_code_to_ui(gh, 0x00A1);
    /* Non-0x02B2 errors filtered during skip_error_stage */
    EXPECT_EQ(ctx.on_error_code_count, 0);
}

TEST_F(LcbGeneralHandlerTest, ReturnErrorToUI_EStop_NeverFiltered) {
    gh->initial_have_error = true;
    gh->skip_error_stage = true;
    gh->lcb_cb.on_return_error_code_to_ui(gh, LCB_ERROR_E_STOP);
    /* 0x02B2 is NEVER filtered */
    EXPECT_EQ(ctx.on_error_code_count, 1);
    EXPECT_EQ(ctx.last_error_code, LCB_ERROR_E_STOP);
}

/* ============================================================================
 * Error Callback — Skip Current Error (0x75)
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, SkipCurrError_TransitionsSkipToNone) {
    gh->error_state = GH_ERROR_SKIP;
    gh->lcb_cb.on_lcb_skip_curr_error(gh);
    EXPECT_EQ(gh->error_state, GH_ERROR_NONE);
}

TEST_F(LcbGeneralHandlerTest, SkipCurrError_NotInSkip_NoChange) {
    gh->error_state = GH_ERROR_GET;
    gh->lcb_cb.on_lcb_skip_curr_error(gh);
    EXPECT_EQ(gh->error_state, GH_ERROR_GET);
}

/* ============================================================================
 * Exception Error Code Handling
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, HandleException_0x02B2_TM_SetsEStopFlags) {
    gh->machine_category = MACHINE_CATEGORY_TM;
    gh->safe_key_stop_machine = true;
    gh_handle_exception_error_code(gh, LCB_ERROR_E_STOP);
    EXPECT_TRUE(gh->run_proc_estop_lcb);
    EXPECT_TRUE(gh->safe_key_signal_estop_error);
}

TEST_F(LcbGeneralHandlerTest, HandleException_0x02B2_BIKE_NoAction) {
    gh->machine_category = MACHINE_CATEGORY_BIKE;
    gh_handle_exception_error_code(gh, LCB_ERROR_E_STOP);
    EXPECT_FALSE(gh->run_proc_estop_lcb);
}

TEST_F(LcbGeneralHandlerTest, HandleException_0x024F_ClearsFlags) {
    gh->safe_key_signal_estop_error = true;
    gh->mcb_estop_error = true;
    gh_handle_exception_error_code(gh, LCB_ERROR_E_STOP_MCB);
    EXPECT_FALSE(gh->safe_key_signal_estop_error);
    EXPECT_FALSE(gh->mcb_estop_error);
}

/* ============================================================================
 * Version Callback (0x73)
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, Version_NormalValue_SetsTypeAndFiresCallbacks) {
    /* Version = 0x0601 → type=0x06 (TM_LCB), ver=0x01 */
    gh->lcb_cb.on_lcb_version(gh, 0x0601);
    EXPECT_EQ(gh->machine_lcb_type, LCB_TYPE_TM_LCB);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_TM);
    EXPECT_EQ(ctx.on_lcb_type_count, 1);
    EXPECT_EQ(ctx.last_lcb_type, 0x06);
    EXPECT_EQ(ctx.on_lcb_version_count, 1);
    EXPECT_EQ(ctx.last_version_major, 0x06);
    EXPECT_EQ(ctx.last_version_minor, 0x01);
}

TEST_F(LcbGeneralHandlerTest, Version_TriggersInitIfNotFinished) {
    gh->initial_finish = false;
    gh->lcb_cb.on_lcb_version(gh, 0x0601);
    /* Should trigger proc_initial_lcb → init_state = TM_3_CMDS_INITIAL */
    EXPECT_EQ(gh->init_state, GH_INIT_TM_3_CMDS_INITIAL);
}

/* ============================================================================
 * Device Data — LCB Information (0x50 sub-cmd 0xF0)
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, DdLcbInfo_SetsTypeAndVersion) {
    uint8_t data[] = {0x02, 0x00, 0x05, 0x03};  /* type=0x02, major=0x0005, minor=0x03 */
    gh->dd_cb.on_dd_get_lcb_information(gh, data, sizeof(data));
    EXPECT_EQ(gh->machine_lcb_type, LCB_TYPE_LCB1_PWM);
    EXPECT_EQ(gh->machine_category, MACHINE_CATEGORY_BIKE);
    EXPECT_EQ(ctx.on_lcb_type_count, 1);
    EXPECT_EQ(ctx.on_lcb_version_count, 1);
    EXPECT_EQ(ctx.last_version_major, 5);
    EXPECT_EQ(ctx.last_version_minor, 3);
}

/* ============================================================================
 * Serial Number Callback (0x7D) — Parsing
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, SerialNumber_Clear_Success) {
    uint8_t data[] = {0x00, 0x00};  /* handling=erase, status=success */
    gh->lcb_cb.on_lcb_cmd_serial_number(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_clear_sn_count, 1);
    EXPECT_TRUE(ctx.last_clear_sn_success);
}

TEST_F(LcbGeneralHandlerTest, SerialNumber_Clear_Failure) {
    uint8_t data[] = {0x00, 0x01};  /* handling=erase, status=fail */
    gh->lcb_cb.on_lcb_cmd_serial_number(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_clear_sn_count, 1);
    EXPECT_FALSE(ctx.last_clear_sn_success);
}

TEST_F(LcbGeneralHandlerTest, SerialNumber_Set_Success) {
    uint8_t data[] = {0x01, 0x00};
    gh->lcb_cb.on_lcb_cmd_serial_number(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_set_sn_count, 1);
    EXPECT_TRUE(ctx.last_set_sn_success);
}

TEST_F(LcbGeneralHandlerTest, SerialNumber_Read_Success) {
    uint8_t data[] = {0x02, 0x00, 0x03, 'A', 'B', 'C'};
    gh->lcb_cb.on_lcb_cmd_serial_number(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_get_sn_count, 1);
    EXPECT_TRUE(ctx.last_get_sn_success);
    EXPECT_STREQ(ctx.last_sn, "ABC");
}

TEST_F(LcbGeneralHandlerTest, SerialNumber_Read_Failure) {
    uint8_t data[] = {0x02, 0x01};  /* status=fail */
    gh->lcb_cb.on_lcb_cmd_serial_number(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_get_sn_count, 1);
    EXPECT_FALSE(ctx.last_get_sn_success);
}

TEST_F(LcbGeneralHandlerTest, SerialNumber_MaxLength_Success) {
    uint8_t data[] = {0x03, 0x00, 0x00, 0x20};  /* max_len = 0x0020 = 32 */
    gh->lcb_cb.on_lcb_cmd_serial_number(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_get_sn_max_len_count, 1);
    EXPECT_TRUE(ctx.last_sn_max_len_success);
    EXPECT_EQ(ctx.last_sn_max_len, 32);
}

TEST_F(LcbGeneralHandlerTest, SerialNumber_TooShort_Ignored) {
    uint8_t data[] = {0x00};  /* length < 2 */
    gh->lcb_cb.on_lcb_cmd_serial_number(gh, data, 1);
    EXPECT_EQ(ctx.on_clear_sn_count, 0);
}

/* ============================================================================
 * Universal Data Callbacks — Magnet Angle
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, MagnetAngle_NormalValue_FiresCallback) {
    uint8_t data[] = {0x01, 0x2C};  /* angle = 0x012C = 300 */
    gh->is_auto_reply_magnet_angle = true;
    gh->uni_cb.on_ud_magnet_angle_state(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_magnet_angle_count, 1);
    EXPECT_EQ(ctx.last_magnet_angle, 300);
    EXPECT_FALSE(gh->is_auto_reply_magnet_angle);  /* Auto-reply disabled */
}

TEST_F(LcbGeneralHandlerTest, MagnetAngle_0xFFFF_Ignored) {
    uint8_t data[] = {0xFF, 0xFF};  /* Calibration in progress */
    gh->is_auto_reply_magnet_angle = true;
    gh->uni_cb.on_ud_magnet_angle_state(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_magnet_angle_count, 0);  /* NOT fired */
    EXPECT_TRUE(gh->is_auto_reply_magnet_angle);  /* Still enabled */
}

/* ============================================================================
 * Universal Data Callbacks — MCB Voltage & Moto Type
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, McbVoltage_FiresCallback) {
    uint8_t data[] = {0xAB};
    gh->uni_cb.on_ud_mcb_voltage(gh, data, sizeof(data));
    EXPECT_EQ(ctx.on_mcb_voltage_count, 1);
    EXPECT_EQ(ctx.last_mcb_voltage, 0xAB);
}

TEST_F(LcbGeneralHandlerTest, MotoType_FiresCallback) {
    gh->uni_cb.on_ud_get_moto_type(gh, 42);
    EXPECT_EQ(ctx.on_moto_type_count, 1);
    EXPECT_EQ(ctx.last_moto_type, 42);
}

/* ============================================================================
 * Status Byte Bit Forwarding
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, StatusBit_InclineDown_Forwarded) {
    gh->lcb_cb.on_incline_motor_downward_status_changed(gh, true);
    EXPECT_EQ(ctx.on_incline_down_count, 1);
    EXPECT_TRUE(ctx.last_incline_down);
}

TEST_F(LcbGeneralHandlerTest, StatusBit_MainMotor_Forwarded) {
    gh->lcb_cb.on_main_motor_status_changed(gh, true);
    EXPECT_EQ(ctx.on_main_motor_count, 1);
    EXPECT_TRUE(ctx.last_main_motor);
}

TEST_F(LcbGeneralHandlerTest, StatusBit_CommandError_Forwarded) {
    gh->lcb_cb.on_command_error_status_changed(gh, true);
    EXPECT_EQ(ctx.on_cmd_error_count, 1);
    EXPECT_TRUE(ctx.last_cmd_error);
}

/* ============================================================================
 * LCB Status & Battery Voltage Forwarding
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, LcbStatus_Forwarded) {
    gh->lcb_cb.on_lcb_status(gh, 0x42);
    EXPECT_EQ(ctx.on_lcb_status_count, 1);
    EXPECT_EQ(ctx.last_lcb_status, 0x42);
}

TEST_F(LcbGeneralHandlerTest, BatteryVoltage_Forwarded) {
    gh->lcb_cb.on_lcb_battery_voltage(gh, 1234);
    EXPECT_EQ(ctx.on_battery_voltage_count, 1);
    EXPECT_EQ(ctx.last_battery_voltage, 1234);
}

/* ============================================================================
 * Update Program Handler
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, UpdateProgram_0x59_Success) {
    gh->lcb_cb.on_lcb_update_program(gh, 0x59);
    EXPECT_EQ(ctx.on_update_mode_count, 1);
    EXPECT_TRUE(ctx.last_update_mode_success);
}

TEST_F(LcbGeneralHandlerTest, UpdateProgram_Other_Failure) {
    gh->lcb_cb.on_lcb_update_program(gh, 0x00);
    EXPECT_EQ(ctx.on_update_mode_count, 1);
    EXPECT_FALSE(ctx.last_update_mode_success);
}

/* ============================================================================
 * Getters
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, GetLcbType_ReturnsType) {
    gh->machine_lcb_type = LCB_TYPE_TM_LCB;
    EXPECT_EQ(general_handler_get_lcb_type(gh), LCB_TYPE_TM_LCB);
}

TEST_F(LcbGeneralHandlerTest, GetCategory_ReturnsCategory) {
    gh->machine_category = MACHINE_CATEGORY_TM;
    EXPECT_EQ(general_handler_get_category(gh), MACHINE_CATEGORY_TM);
}

TEST_F(LcbGeneralHandlerTest, GetDisconnectState_ReturnsFalse) {
    EXPECT_FALSE(general_handler_get_lcb_comm_disconnect_state(gh));
}

/* ============================================================================
 * Control Type — general_handler_set_control_type() must both store the value
 * locally AND transmit LCB command 0x6C (lcb_comm_set_lcb_machine_type), not
 * just set a local field (Java: GeneralHandler.procSetControlType()).
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, SetControlType_StoresLocally) {
    general_handler_set_control_type(gh, GH_CONTROL_TYPE_BIKE);
    EXPECT_EQ(gh->machine_control_type, GH_CONTROL_TYPE_BIKE);
}

TEST_F(LcbGeneralHandlerTest, SetControlType_NoComm_NoCrash) {
    /* gh->comm is NULL in this fixture (never _start()'d) — must not crash. */
    general_handler_set_control_type(gh, GH_CONTROL_TYPE_BIKE);
}

TEST_F(LcbGeneralHandlerTest, SetControlType_WithComm_EnqueuesTx) {
    lcb_comm_t *comm = lcb_comm_create();
    ASSERT_NE(comm, nullptr);
    gh->comm = comm;

    uint32_t before = lcb_comm_get_tx_queue_size(comm);
    general_handler_set_control_type(gh, GH_CONTROL_TYPE_BIKE);
    EXPECT_GT(lcb_comm_get_tx_queue_size(comm), before);

    gh->comm = nullptr; /* detach before destroying comm (avoid UAF in destroy/stop) */
    lcb_comm_destroy(comm);
}

/* ============================================================================
 * NULL Safety — Public API
 * ============================================================================ */

TEST_F(LcbGeneralHandlerTest, NullSafety_SetCallbacks) {
    general_handler_set_callbacks(NULL, &user_cb);
    general_handler_set_callbacks(gh, NULL);
}

TEST_F(LcbGeneralHandlerTest, NullSafety_SetControlType) {
    general_handler_set_control_type(NULL, GH_CONTROL_TYPE_BIKE);
}

TEST_F(LcbGeneralHandlerTest, NullSafety_UnregisterCallbacks) {
    general_handler_unregister_callbacks(NULL);
}

TEST_F(LcbGeneralHandlerTest, NullSafety_EStopMachineStop) {
    general_handler_estop_machine_stop(NULL);
}

TEST_F(LcbGeneralHandlerTest, NullSafety_ForceStop) {
    general_handler_force_stop_machine(NULL);
}

TEST_F(LcbGeneralHandlerTest, NullSafety_GetLcbType_Null) {
    EXPECT_EQ(general_handler_get_lcb_type(NULL), LCB_TYPE_UNKNOWN);
}

TEST_F(LcbGeneralHandlerTest, NullSafety_GetCategory_Null) {
    EXPECT_EQ(general_handler_get_category(NULL), MACHINE_CATEGORY_UNKNOWN);
}
