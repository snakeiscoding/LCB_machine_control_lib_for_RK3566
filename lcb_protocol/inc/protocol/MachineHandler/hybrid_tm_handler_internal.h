/**
 * @file hybrid_tm_handler_internal.h
 * @brief Internal struct definition for hybrid_tm_handler_t
 *
 * Shared between hybrid_tm_handler.c and hybrid_tm_lcb_cb.c to allow both
 * files to access struct fields directly. NOT for external use.
 */

#ifndef HYBRID_TM_HANDLER_INTERNAL_H
#define HYBRID_TM_HANDLER_INTERNAL_H

#include "hybrid_tm_handler.h"
#include "lcb_comm_callback.h"
#include <pthread.h>

/* ============================================================================
 * Constants (from HybridTmHandler.java, cross-checked against chimera2)
 * ============================================================================ */

#define HYBRID_TM_HEARTBEAT_PERIOD_MS              400

/* Auto-reply interval counters (number of heartbeat ticks between TX) */
#define HYBRID_TM_INTERVAL_RPM                     0
#define HYBRID_TM_INTERVAL_INCLINE                 1
#define HYBRID_TM_INTERVAL_IN_USED                 7
#define HYBRID_TM_INTERVAL_MOTOR_INFO              2
#define HYBRID_TM_INTERVAL_TARGET_RPM_TIMEOUT      100  /* 100 * 400ms = 40s */

/* Java: autoReplyInUsed() only polls while mMachineRpm > 100 */
#define HYBRID_TM_IN_USED_RPM_GATE                 100

/* IR Address board (0x26) auto-poll interval — chimera2 TreadMillHandler's
 * TIMES_IR_ADDR_IR_STATUS_INTERVAL = 1 (every heartbeat tick; the fastest
 * polled auto-reply in that class, reflecting its safety-critical role).
 * Ported onto this handler per the file-header NOTE in hybrid_tm_handler.h. */
#define HYBRID_TM_INTERVAL_IR_ADDR_IR_STATUS        1

/* 0x01 = IR beam blocked, 0x00 = clear (chimera2 TreadMillInterface) */
#define HYBRID_TM_IR_STATUS_BLOCKED                 0x01
#define HYBRID_TM_IR_STATUS_CLEAR                   0x00

/* ============================================================================
 * Internal Struct Definition
 * ============================================================================ */

struct hybrid_tm_handler {
    /* Dependency — NOT owned */
    lcb_comm_t *comm;

    /* User-facing callbacks */
    hybrid_tm_handler_callbacks_t user_cb;

    /* Internal callbacks wired to lcb_comm */
    lcb_comm_callbacks_t lcb_cb;

    /* Heartbeat thread (uses RECURSIVE mutex) */
    pthread_t       heartbeat_thread;
    pthread_mutex_t mutex;
    pthread_cond_t  heartbeat_cond;
    bool            is_running;

    /* 4 auto-reply features */
    bool is_auto_reply_rpm;         int cnt_rpm;
    bool is_auto_reply_incline;     int cnt_incline;
    bool is_auto_reply_in_used;     int cnt_in_used;
    bool is_auto_reply_motor_info;  int cnt_motor_info;

    /* RPM observation (from motor-rpm-received callbacks) */
    uint16_t machine_rpm;

    /* No-RPM-error timeout (Java: mTargetRpmCheckFlag / cntIntervalTimesTargetRpm) */
    bool target_rpm_check_flag;
    int  cnt_target_rpm;

    /* IR Address board (0x26) auto-poll */
    bool is_auto_reply_ir_addr_ir_status;  int cnt_ir_addr_ir_status;

    /* IR-EStop state machine (ported from chimera2 TreadMillInterface onto
     * this handler — see hybrid_tm_handler.h's file-header NOTE) */
    bool pending_ir_estop_check;   /* Java: pendingIrEStopCheck */
    bool ir_estop_triggered;       /* Java: wasHybridTmIrEStopTriggered */
};

/* ============================================================================
 * Internal Helpers (extern, exposed for testing)
 * ============================================================================ */

void hth_proc_target_rpm_status_check(hybrid_tm_handler_t *hth);
void hth_proc_auto_reply_tick(hybrid_tm_handler_t *hth);

/* ============================================================================
 * Callback Bridge Registration (defined in hybrid_tm_lcb_cb.c)
 * ============================================================================ */

void hth_lcb_cb_register(hybrid_tm_handler_t *hth);
void hth_lcb_cb_unregister(hybrid_tm_handler_t *hth);

#endif /* HYBRID_TM_HANDLER_INTERNAL_H */
