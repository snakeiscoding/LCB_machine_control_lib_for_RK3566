/**
 * @file general_handler_internal.h
 * @brief Internal struct definition for general_handler_t
 *
 * This header is shared between general_handler.c and general_handler_lcb_cb.c
 * to allow both files to access struct fields directly. NOT for external use.
 */

#ifndef GENERAL_HANDLER_INTERNAL_H
#define GENERAL_HANDLER_INTERNAL_H

#include "general_handler.h"
#include "lcb_comm_callback.h"
#include "lcb_universal_data.h"
#include "lcb_device_data.h"
#include <pthread.h>
#include <time.h>

/* LCB comm status values */
#define GH_COMM_STATUS_CONNECTED  1
#define GH_COMM_STATUS_ERROR      3

struct general_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    general_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t           lcb_cb;
    lcb_universal_data_callbacks_t uni_cb;
    lcb_device_data_callbacks_t    dd_cb;

    /* Machine identity */
    LcbType_t               machine_lcb_type;
    machine_category_type_t machine_category;
    uint8_t                 machine_control_type;

    /* Advanced logging bits (Java: AdvancedLogging singleton's
     * mLoggingMsg/mLoggingDetail + byteLoggingStatus) — local bookkeeping
     * only; lcb_comm_proc_set_adv_logging_status() does not transmit
     * anything to the LCB, it just gates LcbComm's own log verbosity. */
    bool logging_msg;
    bool logging_detail;

    /* Induction PWM calibration points (Java: GeneralHandler.calPoint +
     * static FrameConfig.inductionCalPoints[]/hasInductionCal/validInductionCal).
     * Populated by 5x funDeviceReadPWM(1..5) triggered from
     * gh_set_machine_lcb_type() for induction-brake LCB types. */
    int  cal_point;
    int  induction_cal_points[5];
    bool has_induction_cal;
    bool valid_induction_cal;

    /* Init state machine */
    gh_init_state_t  init_state;
    bool             boot_up_initial;
    bool             initial_cmd_done;
    bool             default_initial_cmd_done;
    bool             operating_cmd_done;
    bool             initial_finish;
    bool             skip_error_stage;
    bool             initial_have_error;
    int              check_lcb_initial_counter;
    int              retry_lcb_initial;
    int              check_lcb_info_counter;
    int              retry_get_lcb_info;

    /* Error state machine */
    gh_error_state_t error_state;
    int              check_get_error_counter;
    int              check_skip_error_counter;
    bool             is_lcb_error_detected;
    int              last_error_code;
    bool             safe_key_stop_machine;
    bool             run_proc_estop_lcb;
    bool             safe_key_signal_estop_error;
    bool             mcb_estop_error;
    bool             estop_revert;

    /* Heartbeat thread */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;
    bool            is_auto_reply_cmd;
    bool            is_auto_initial_lcb;
    bool            is_auto_reply_battery;
    bool            is_auto_reply_magnet_angle;
    int             cnt_interval_battery;
    int             cnt_interval_magnet_angle;

    /* Reconnect timer state */
    bool            reconnect_timer_started;
    int             last_lcb_comm_status;
    struct timespec reconnect_deadline;
};

#endif /* GENERAL_HANDLER_INTERNAL_H */
