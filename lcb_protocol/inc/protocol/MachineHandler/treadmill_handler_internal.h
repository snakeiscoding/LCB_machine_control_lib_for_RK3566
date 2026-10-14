/**
 * @file treadmill_handler_internal.h
 * @brief Internal struct definition for treadmill_handler_t
 *
 * Shared between treadmill_handler.c and treadmill_handler_lcb_cb.c
 * to allow both files to access struct fields directly. NOT for external use.
 */

#ifndef TREADMILL_HANDLER_INTERNAL_H
#define TREADMILL_HANDLER_INTERNAL_H

#include "treadmill_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define TM_HEARTBEAT_PERIOD_MS         400

/* Auto-reply interval counters (number of heartbeat ticks between TX) */
#define TM_INTERVAL_EXT_STATUS         0
#define TM_INTERVAL_RPM                0
#define TM_INTERVAL_CADENCE            3
#define TM_INTERVAL_INCLINE            1
#define TM_INTERVAL_IN_USED            7
#define TM_INTERVAL_SPEEDUP_IN_USED    3
#define TM_INTERVAL_MOTOR_INFO         3
#define TM_INTERVAL_RAMP_RPM           1
#define TM_INTERVAL_TARGET_RPM_TIMEOUT 100  /* 100 * 400ms = 40s */
#define TM_INTERVAL_IR_ADDR_IR_STATUS  1

#define TM_DEFAULT_RAMP_STEP           250

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct treadmill_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    treadmill_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* LCB type for skip logic */
    LcbType_t machine_lcb_type;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* 8 auto-reply features */
    bool is_auto_reply_ext_status;        int cnt_ext_status;
    bool is_auto_reply_rpm;               int cnt_rpm;
    bool is_auto_reply_cadence;           int cnt_cadence;
    bool is_auto_reply_incline;           int cnt_incline;
    bool is_auto_reply_in_used;           int cnt_in_used;
    bool is_auto_reply_speed_up_in_used;  int cnt_speed_up_in_used;
    bool is_auto_reply_motor_info;        int cnt_motor_info;
    bool is_auto_reply_ir_addr_ir_status; int cnt_ir_addr_ir_status;  /* stub */

    /* RPM observation (from get_lcb_motor_rpm callbacks) */
    uint16_t machine_rpm;

    /* No-RPM-error timeout */
    bool target_rpm_check_flag;
    int  cnt_target_rpm;

    /* RPM ramp state */
    bool     is_auto_ramp_rpm;
    int      cnt_ramp_rpm;
    uint16_t gui_set_rpm;
    uint16_t set_current_rpm;
    int      ramp_up_step;
    int      ramp_down_step;
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void th_proc_target_rpm_status_check(treadmill_handler_t *th);
void th_proc_check_ramp_up_cmd(treadmill_handler_t *th);
void th_proc_auto_reply_tick(treadmill_handler_t *th);

/* ============================================================================
 * Callback Bridge Registration (defined in treadmill_handler_lcb_cb.c)
 * ============================================================================ */

void th_lcb_cb_register(treadmill_handler_t *th);
void th_lcb_cb_unregister(treadmill_handler_t *th);

#endif /* TREADMILL_HANDLER_INTERNAL_H */
