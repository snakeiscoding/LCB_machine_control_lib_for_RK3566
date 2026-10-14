/**
 * @file general_handler.c
 * @brief LCB General Handler — Core implementation
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/GeneralHandler.java
 */

#include "general_handler.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_constants.h"
#include "lcb_proc_comm.h"
#include "lcb_universal_data.h"
#include "lcb_device_data.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "general_handler_internal.h"

#define TAG "GeneralHandler"

/* ============================================================================
 * Constants (from GeneralHandler.java)
 * ============================================================================ */

#define GH_HEARTBEAT_PERIOD_MS          1000
#define GH_CHECK_LCB_INITIAL_TIMES      3
#define GH_LCB_INITIAL_RETRIES          1
#define GH_CHECK_NON_LCB_INFO_TIMES     3
#define GH_GET_LCB_INFO_RETRIES         8
#define GH_CHECK_GET_ERROR_STATE_TIMES  4
#define GH_CHECK_SKIP_ERROR_STATE_TIMES 3
#define GH_RECONNECT_TIMEOUT_MS         3000

/* ============================================================================
 * Forward declarations for callback bridge (general_handler_lcb_cb.c)
 * ============================================================================ */

extern void gh_lcb_cb_register(general_handler_t *gh);
extern void gh_lcb_cb_unregister(general_handler_t *gh);

/* ============================================================================
 * Machine Category Mapping
 * ============================================================================ */

static machine_category_type_t map_lcb_type_to_category(LcbType_t type)
{
    switch (type) {
        /* TM types */
        case LCB_TYPE_AC_LCB:
        case LCB_TYPE_DC_LCB:
        case LCB_TYPE_TM_LCB:
        case LCB_TYPE_DCI_DAUGHTER_BOARD:
        case LCB_TYPE_TOPTEK:
        case LCB_TYPE_DELTA:
        case LCB_TYPE_DELTA_AC_11KW:
        case LCB_TYPE_DELTA_HYBRID:
        case LCB_TYPE_DELTA_T1:
        case LCB_TYPE_DELTA_SPARE_1:
        case LCB_TYPE_DELTA_SPARE_2:
        case LCB_TYPE_DELTA_SPARE_3:
        case LCB_TYPE_E0:
        case LCB_TYPE_E1:
        case LCB_TYPE_GLOBAL_200_325HP_110V:
        case LCB_TYPE_GLOBAL_200_325HP_220V:
        case LCB_TYPE_DC_LCB_300_325:
        case LCB_TYPE_DC_LCB_200_275:
        case LCB_TYPE_AC_LCB_200_275:
        case LCB_TYPE_AC_LCB_300_325:
        case LCB_TYPE_JHT_VAVE_PS_ES_BOARD:
        case LCB_TYPE_JHT_VAVE_T600_BOARD:
        case LCB_TYPE_JHT_VAVE_LS_BOARD:
        case LCB_TYPE_JHT_VAVE_PP_BOARD:
            return MACHINE_CATEGORY_TM;

        /* BIKE types */
        case LCB_TYPE_LCB1_PWM:
        case LCB_TYPE_LCB3_PWM:
        case LCB_TYPE_LCB_CD:
        case LCB_TYPE_LCB1_CURRENT:
        case LCB_TYPE_LCB3_CURRENT:
        case LCB_TYPE_LCB1_PANATTA:
        case LCB_TYPE_LCB2:
        case LCB_TYPE_CLIMBMILL_OR_A30:
        case LCB_TYPE_LCBA:
        case LCB_TYPE_1X_BIKE_LCB:
        case LCB_TYPE_COMMERCIAL_CLIMBMILL:
        case LCB_TYPE_RETAIL_CLIMBMILL:
        case LCB_TYPE_ATHENA_LCB:
        case LCB_TYPE_JHT_AD_BOARD:
        case LCB_TYPE_JIS_ATHENA:
        case LCB_TYPE_BIKE:
        case LCB_TYPE_ELECTROMAGNETIC_INCLINE:
            return MACHINE_CATEGORY_BIKE;

        default:
            return MACHINE_CATEGORY_BIKE;
    }
}

/* ============================================================================
 * Internal: Init State Machine
 * ============================================================================ */

/* Reset init flags and trigger auto-initialization (Java: procInitialLcb) */
void gh_proc_initial_lcb(general_handler_t *gh)
{
    gh->initial_cmd_done = false;
    gh->default_initial_cmd_done = false;
    gh->operating_cmd_done = false;
    gh->initial_finish = false;
    gh->is_auto_initial_lcb = false;
    gh->check_lcb_initial_counter = 0;
    gh->retry_lcb_initial = 0;

    gh->init_state = GH_INIT_TM_3_CMDS_INITIAL;
LCB_LOGD(TAG, "####Frank_Debug general_handler gh_proc_initial_lcb frank_1 GH_INIT_TM_3_CMDS_INITIAL");

}

/* Send initialization commands based on machine category (Java: autoInitialLcbCmd) */
void gh_auto_initial_lcb_cmd(general_handler_t *gh)
{
    LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_1 gh->machine_category = %d",gh->machine_category);
    switch (gh->machine_category) {
        case MACHINE_CATEGORY_TM: {
            /* TM: 3-command init sequence */
            LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_1 gh->init_state = %d",gh->init_state);
            switch (gh->init_state) {
                case GH_INIT_TM_3_CMDS_INITIAL:
                case GH_INIT_CMD_INITIAL:
            LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_2 GH_INIT_TM_3_CMDS_INITIAL/GH_INIT_CMD_INITIAL");
                    if (!gh->initial_cmd_done && gh->comm != NULL) {
                        if (gh->skip_error_stage)
                            lcb_comm_set_lcb_initial_highest_priority(gh->comm);
                        else
                            lcb_comm_set_lcb_initial(gh->comm);
                    }
                    break;
                case GH_INIT_CMD_OPERATING_STOP:
                    LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_3 GH_INIT_CMD_OPERATING_STOP");
                    if (!gh->operating_cmd_done && gh->comm != NULL) {
                        if (gh->skip_error_stage)
                            lcb_comm_set_lcb_operating_status_highest_priority(gh->comm, LCB_OPERATING_STOP);
                        else
                            lcb_comm_set_lcb_operating_status(gh->comm, LCB_OPERATING_STOP);
                    }
                    break;
                case GH_INIT_CMD_OPERATING_ACTION:
                    LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_4 GH_INIT_CMD_OPERATING_ACTION");
                    if (!gh->operating_cmd_done && gh->comm != NULL) {
                        if (gh->skip_error_stage)
                            lcb_comm_set_lcb_operating_status_highest_priority(gh->comm, LCB_OPERATING_ACTION);
                        else
                            lcb_comm_set_lcb_operating_status(gh->comm, LCB_OPERATING_ACTION);
                    }
                    break;
                case GH_INIT_FINISH:
                    LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_5 GH_INIT_FINISH");
                    gh->initial_finish = true;
                    gh->is_auto_reply_cmd = true;
                    gh->is_auto_initial_lcb = true;
                    gh->skip_error_stage = false;
                    if (gh->estop_revert) {
                        gh->estop_revert = false;
                        if (gh->comm != NULL)
                            lcb_comm_proc_estop_unlock(gh->comm);
                    }
                    if (gh->user_cb.on_lcb_initial_finish != NULL)
                        gh->user_cb.on_lcb_initial_finish(gh->user_cb.user_data);
                    gh->init_state = GH_INIT_NONE;
                    break;
                default:
                    break;
            }
            break;
        }
        case MACHINE_CATEGORY_BIKE:
        default: {
            /* BIKE: default init (single command) */
            switch (gh->init_state) {
                case GH_INIT_TM_3_CMDS_INITIAL:
                    LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_6 GH_INIT_TM_3_CMDS_INITIAL");
                    gh->init_state = GH_INIT_DEFAULT_CMD_INITIAL;
                    /* fall through */
                case GH_INIT_DEFAULT_CMD_INITIAL:
                    LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_7 GH_INIT_DEFAULT_CMD_INITIAL");
                    if (!gh->default_initial_cmd_done && gh->comm != NULL)
                        lcb_comm_set_lcb_initial(gh->comm);
                    break;
                case GH_INIT_FINISH:
                    LCB_LOGD(TAG, "####Frank_Debug general_handler gh_auto_initial_lcb_cmd frank_8 GH_INIT_FINISH");
                    gh->initial_finish = true;
                    gh->is_auto_reply_cmd = true;
                    gh->is_auto_initial_lcb = true;
                    gh->skip_error_stage = false;
                    if (gh->estop_revert) {
                        gh->estop_revert = false;
                        if (gh->comm != NULL)
                            lcb_comm_proc_estop_unlock(gh->comm);
                    }
                    if (gh->user_cb.on_lcb_initial_finish != NULL)
                        gh->user_cb.on_lcb_initial_finish(gh->user_cb.user_data);
                    gh->init_state = GH_INIT_NONE;
                    break;
                default:
                    break;
            }
            break;
        }
    }
}

/* ============================================================================
 * Internal: Error State Machine
 * ============================================================================ */

/* Check if in error processing state; timeout resets (Java: checkInGetErrorState) */
bool gh_check_in_get_error_state(general_handler_t *gh)
{
    switch (gh->error_state) {
        case GH_ERROR_NONE:
            gh->check_get_error_counter = 0;
            gh->check_skip_error_counter = 0;
            return false;
        case GH_ERROR_GET:
            gh->check_get_error_counter++;
            if (gh->check_get_error_counter >= GH_CHECK_GET_ERROR_STATE_TIMES) {
                gh->check_get_error_counter = 0;
                gh->error_state = GH_ERROR_NONE;
            }
            return true;
        case GH_ERROR_SKIP:
            gh->check_skip_error_counter++;
            if (gh->check_skip_error_counter >= GH_CHECK_SKIP_ERROR_STATE_TIMES) {
                gh->check_skip_error_counter = 0;
                gh->error_state = GH_ERROR_NONE;
            }
            return true;
    }
    return false;
}

/* Execute error state action (Java: runGetErrorProcesses) */
void gh_run_get_error_processes(general_handler_t *gh)
{
    if (gh->comm == NULL) return;

    switch (gh->error_state) {
        case GH_ERROR_GET:
            lcb_comm_get_lcb_error_code_high_priority(gh->comm);
            break;
        case GH_ERROR_SKIP:
            lcb_comm_set_lcb_skip_curr_error_high_priority(gh->comm);
            break;
        default:
            break;
    }
}

/* Handle specific error codes (Java: handleExceptionErrorCode) */
void gh_handle_exception_error_code(general_handler_t *gh, uint16_t error_code)
{
    switch (error_code) {
        case LCB_ERROR_E_STOP: /* 0x02B2 */
            if (gh->machine_category == MACHINE_CATEGORY_TM) {
                LCB_LOGD(TAG, "02B2 E-Stop highest priority stop cmd");
                if (!gh->run_proc_estop_lcb) {
                    gh->run_proc_estop_lcb = true;
                    if (gh->comm != NULL) {
                        lcb_comm_set_lcb_motor_rpm_highest_priority(gh->comm, 0);
                        lcb_comm_set_lcb_incline_action_highest_priority(gh->comm, 0x00);
                    }
                }
                if (gh->safe_key_stop_machine)
                    gh->safe_key_signal_estop_error = true;
                else
                    gh->mcb_estop_error = true;
            }
            break;
        case LCB_ERROR_E_STOP_MCB: /* 0x024F */
            LCB_LOGE(TAG, "MCB E-Stop error 0x024F");
            gh->safe_key_signal_estop_error = false;
            gh->mcb_estop_error = false;
            break;
        default:
            break;
    }
}

/* ============================================================================
 * Internal: Heartbeat Helpers
 * ============================================================================ */

static bool check_have_lcb_type(general_handler_t *gh)
{
    if (gh->machine_lcb_type != LCB_TYPE_UNKNOWN)
        return true;

    /* Retry getting LCB version */
    gh->check_lcb_info_counter++;
    if (gh->check_lcb_info_counter >= GH_CHECK_NON_LCB_INFO_TIMES) {
        gh->check_lcb_info_counter = 0;
        if (gh->retry_get_lcb_info < GH_GET_LCB_INFO_RETRIES) {
            gh->retry_get_lcb_info++;
            lcb_comm_get_lcb_version_highest_priority(gh->comm);
        }
    }
    return false;
}

static bool check_initial_finish(general_handler_t *gh)
{
    if (gh->machine_lcb_type == LCB_TYPE_UNKNOWN)
        return false;
    return gh->initial_finish;
}

static void auto_reply_cmd(general_handler_t *gh)
{
    if (gh->comm == NULL) return;

    /* Heartbeat: get LCB status */
    lcb_comm_get_lcb_status_low_priority(gh->comm, LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED);

    /* Optional: battery voltage */
    if (gh->is_auto_reply_battery) {
        lcb_comm_get_lcb_battery_voltage_low_priority(gh->comm, LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED);
    }

    /* Optional: magnet angle */
    if (gh->is_auto_reply_magnet_angle) {
        lcb_ud_get_magnet_angle(gh->comm, LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED);
    }
}

static void check_reconnect_timer(general_handler_t *gh)
{
    if (!gh->reconnect_timer_started) return;

    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);

    if (now.tv_sec > gh->reconnect_deadline.tv_sec ||
        (now.tv_sec == gh->reconnect_deadline.tv_sec &&
         now.tv_nsec >= gh->reconnect_deadline.tv_nsec)) {
        /* Timer expired — fire reconnect callback */
        gh->reconnect_timer_started = false;
        if (gh->user_cb.on_lcb_comm_reconnected != NULL)
            gh->user_cb.on_lcb_comm_reconnected(gh->user_cb.user_data);
    }
}

/* ============================================================================
 * Heartbeat Thread
 * ============================================================================ */

static void* heartbeat_thread_func(void *arg)
{
    general_handler_t *gh = (general_handler_t *)arg;

    pthread_mutex_lock(&gh->mutex);
    while (gh->is_running) {
        /* Wait 1000ms or until signaled */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += GH_HEARTBEAT_PERIOD_MS / 1000;
        ts.tv_nsec += (GH_HEARTBEAT_PERIOD_MS % 1000) * 1000000L;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000L;
        }
        pthread_cond_timedwait(&gh->heartbeat_cond, &gh->mutex, &ts);

        if (!gh->is_running) break;

        /* Phase 1: Check if we have LCB type info */
        if (!gh->is_auto_reply_cmd) {
            if (check_have_lcb_type(gh)) {
                gh->is_auto_reply_cmd = true;
            }
        } else {
            auto_reply_cmd(gh);
        }

        /* Phase 2: Auto-init (Java: autoInitialGeneralHandlerLCB)
         * Send init commands every tick based on current state.
         * Separately, timeout/retry logic re-triggers the whole init
         * if it hasn't completed within the counter threshold. */
        if (!gh->is_auto_initial_lcb) {
            if (check_initial_finish(gh)) {
                gh->is_auto_initial_lcb = true;
            } else if (gh->machine_lcb_type != LCB_TYPE_UNKNOWN) {
                /* Send init command based on current state (every tick) */
                gh_auto_initial_lcb_cmd(gh);

                /* Timeout: re-trigger init if stuck */
                gh->check_lcb_initial_counter++;
                if (gh->check_lcb_initial_counter >= GH_CHECK_LCB_INITIAL_TIMES) {
                    gh->check_lcb_initial_counter = 0;
                    if (gh->retry_lcb_initial < GH_LCB_INITIAL_RETRIES) {
                        gh->retry_lcb_initial++;
                        gh_proc_initial_lcb(gh);
                    }
                }
            }
        }

        /* Phase 3: Reconnect timer */
        check_reconnect_timer(gh);
    }
    pthread_mutex_unlock(&gh->mutex);

    return NULL;
}

/* ============================================================================
 * Public: Lifecycle
 * ============================================================================ */

general_handler_t* general_handler_create(void)
{
    general_handler_t *gh = (general_handler_t*)calloc(1, sizeof(general_handler_t));
    if (gh == NULL) return NULL;

    /* Use recursive mutex — callbacks from lcb_comm (e.g. lcb_proc_e_stop_unlock)
     * can be invoked synchronously while the heartbeat thread holds the mutex,
     * causing the callback bridge to re-lock from the same thread. */
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&gh->mutex, &attr);
    pthread_mutexattr_destroy(&attr);
    pthread_cond_init(&gh->heartbeat_cond, NULL);

    gh->machine_lcb_type = LCB_TYPE_UNKNOWN;
    gh->machine_category = MACHINE_CATEGORY_UNKNOWN;
    gh->init_state = GH_INIT_NONE;
    gh->error_state = GH_ERROR_NONE;

    return gh;
}

void general_handler_destroy(general_handler_t *gh)
{
    if (gh == NULL) return;

    general_handler_auto_stop(gh);
    pthread_cond_destroy(&gh->heartbeat_cond);
    pthread_mutex_destroy(&gh->mutex);
    free(gh);
}

/* ============================================================================
 * Public: Callback Registration
 * ============================================================================ */

void general_handler_callbacks_init(general_handler_callbacks_t *cb)
{
    if (cb == NULL) return;
    memset(cb, 0, sizeof(general_handler_callbacks_t));
}

void general_handler_set_callbacks(general_handler_t *gh,
                                   const general_handler_callbacks_t *cb)
{
    if (gh == NULL || cb == NULL) return;
    pthread_mutex_lock(&gh->mutex);
    gh->user_cb = *cb;
    pthread_mutex_unlock(&gh->mutex);
}

void general_handler_unregister_callbacks(general_handler_t *gh)
{
    if (gh == NULL) return;
    pthread_mutex_lock(&gh->mutex);
    memset(&gh->user_cb, 0, sizeof(general_handler_callbacks_t));
    pthread_mutex_unlock(&gh->mutex);
}

/* ============================================================================
 * Public: Start / Stop
 * ============================================================================ */

bool general_handler_auto_start(general_handler_t *gh, lcb_comm_t *comm,
                                serial_port_t *port)
{
    if (gh == NULL || comm == NULL || port == NULL) return false;

    general_handler_auto_stop(gh);

    pthread_mutex_lock(&gh->mutex);
    gh->comm = comm;
    gh->skip_error_stage = true;
    gh->initial_have_error = false;
    gh->boot_up_initial = true;
    gh->is_auto_reply_cmd = false;
    gh->is_auto_initial_lcb = false;
    gh->machine_lcb_type = LCB_TYPE_UNKNOWN;
    gh->machine_category = MACHINE_CATEGORY_UNKNOWN;

    /* Wire internal LCB callbacks */
    gh_lcb_cb_register(gh);
    lcb_comm_register_callbacks(comm, &gh->lcb_cb);

    /* Start serial comm */
    if (!lcb_comm_start(comm, port)) {
        LCB_LOGE(TAG, "Failed to start LCB communication");
        gh->comm = NULL;
        pthread_mutex_unlock(&gh->mutex);
        return false;
    }

    /* Request LCB version to discover type */
    lcb_comm_get_lcb_version_highest_priority(comm);

    /* Start heartbeat thread */
    gh->is_running = true;
    pthread_mutex_unlock(&gh->mutex);

    pthread_create(&gh->heartbeat_thread, NULL, heartbeat_thread_func, gh);

    LCB_LOGD(TAG, "General handler started");
    return true;
}

void general_handler_auto_stop(general_handler_t *gh)
{
    if (gh == NULL) return;

    /* Stop heartbeat thread */
    pthread_mutex_lock(&gh->mutex);
    if (gh->is_running) {
        gh->is_running = false;
        pthread_cond_signal(&gh->heartbeat_cond);
        pthread_mutex_unlock(&gh->mutex);
        pthread_join(gh->heartbeat_thread, NULL);
    } else {
        pthread_mutex_unlock(&gh->mutex);
    }

    /* Unregister and stop LCB comm */
    if (gh->comm != NULL) {
        lcb_comm_unregister_callbacks(gh->comm);
        lcb_comm_stop(gh->comm);
        gh->comm = NULL;
    }

    LCB_LOGD(TAG, "General handler stopped");
}

void general_handler_request_stop(general_handler_t *gh)
{
    if (gh == NULL) return;

    pthread_mutex_lock(&gh->mutex);
    gh->is_running = false;
    pthread_cond_signal(&gh->heartbeat_cond);
    pthread_mutex_unlock(&gh->mutex);

    /* No pthread_join here (would self-deadlock if called from the heartbeat
     * thread) and gh->comm is only detached, not stopped/destroyed — the
     * caller still owns comm and may hand it to another handler immediately. */
    if (gh->comm != NULL) {
        lcb_comm_unregister_callbacks(gh->comm);
        gh->comm = NULL;
    }

    LCB_LOGD(TAG, "General handler stop requested (async, self-callback-safe)");
}

/* ============================================================================
 * Public: E-Stop & Force Stop
 * ============================================================================ */

void general_handler_estop_machine_stop(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    pthread_mutex_lock(&gh->mutex);
    gh->safe_key_stop_machine = true;
    pthread_mutex_unlock(&gh->mutex);
    lcb_comm_proc_estop_machine_stop(gh->comm);
}

void general_handler_estop_revert_lcb_initial(general_handler_t *gh)
{
    if (gh == NULL) return;
    pthread_mutex_lock(&gh->mutex);
LCB_LOGD(TAG, "####Frank_Debug general_handler general_handler_estop_revert_lcb_initial frank_1");
    gh->safe_key_stop_machine = false;
    gh->safe_key_signal_estop_error = false;
    gh->mcb_estop_error = false;
    gh->run_proc_estop_lcb = false;
    gh->estop_revert = true;
    gh_proc_initial_lcb(gh);
LCB_LOGD(TAG, "####Frank_Debug general_handler general_handler_estop_revert_lcb_initial frank_2");
    pthread_mutex_unlock(&gh->mutex);
}

void general_handler_force_stop_machine(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    pthread_mutex_lock(&gh->mutex);

    if (gh->machine_category == MACHINE_CATEGORY_TM) {
        lcb_comm_set_lcb_motor_rpm_highest_priority(gh->comm, 0);
        lcb_comm_set_lcb_incline_action_highest_priority(gh->comm, 0x00);
    }
    /* BIKE force stop is type-dependent — simplified to basic stop */

    pthread_mutex_unlock(&gh->mutex);
}

/* ============================================================================
 * Public: Control & Auto-Reply
 * ============================================================================ */

void general_handler_set_control_type(general_handler_t *gh, uint8_t type)
{
    if (gh == NULL) return;
    pthread_mutex_lock(&gh->mutex);
    gh->machine_control_type = type;
    if (gh->comm != NULL)
        lcb_comm_set_lcb_machine_type(gh->comm, type);
    pthread_mutex_unlock(&gh->mutex);
}

uint8_t general_handler_get_control_type(const general_handler_t *gh)
{
    if (gh == NULL) return GH_CONTROL_TYPE_UNKNOWN;
    return gh->machine_control_type;
}

void general_handler_set_auto_reply_battery(general_handler_t *gh, bool enable)
{
    if (gh == NULL) return;
    pthread_mutex_lock(&gh->mutex);
    gh->is_auto_reply_battery = enable;
    pthread_mutex_unlock(&gh->mutex);
}

void general_handler_set_auto_reply_magnet_angle(general_handler_t *gh, bool enable)
{
    if (gh == NULL) return;
    pthread_mutex_lock(&gh->mutex);
    gh->is_auto_reply_magnet_angle = enable;
    pthread_mutex_unlock(&gh->mutex);
}

void general_handler_set_advanced_logging(general_handler_t *gh, bool log_messages, bool log_message_details)
{
    if (gh == NULL) return;
    pthread_mutex_lock(&gh->mutex);
    gh->logging_msg = log_messages;
    gh->logging_detail = log_message_details;
    if (gh->comm != NULL)
        lcb_comm_proc_set_adv_logging_status(gh->comm);
    pthread_mutex_unlock(&gh->mutex);
}

/* ============================================================================
 * Public: ERP (Standby) Mode
 * ============================================================================ */

void general_handler_enter_erp_mode(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    lcb_comm_enter_lcb_erp_mode(gh->comm);
}

void general_handler_leave_erp_mode(general_handler_t *gh, const char *device_path, uint32_t baud_rate)
{
    if (gh == NULL || gh->comm == NULL) return;
    /* device_path/baud_rate are not yet used to reopen the port — see the
     * NOTE on this function's declaration in general_handler.h. */
    (void)device_path;
    (void)baud_rate;
    lcb_comm_leave_lcb_erp_mode(gh->comm, device_path, baud_rate);
}

void general_handler_erp_stop_uart_port(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    lcb_comm_erp_stop_uart_port(gh->comm);
}

/* ============================================================================
 * Public: Serial Number Operations
 * ============================================================================ */

void general_handler_clear_serial_number(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    uint8_t data[] = {0x00, 0x00, 0x00};
    lcb_comm_set_lcb_cmd_serial_number(gh->comm, data, sizeof(data));
}

void general_handler_set_serial_number(general_handler_t *gh, const char *serial)
{
    if (gh == NULL || gh->comm == NULL || serial == NULL) return;
    size_t len = strlen(serial);
    if (len == 0 || len > 250) return;

    uint8_t data[256];
    data[0] = 0x01;        /* Write handling way */
    data[1] = 0x00;        /* Reserved */
    data[2] = (uint8_t)len;
    memcpy(&data[3], serial, len);
    lcb_comm_set_lcb_cmd_serial_number(gh->comm, data, (uint8_t)(3 + len));
}

void general_handler_get_serial_number(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    uint8_t data[] = {0x02, 0x00, 0x00};
    lcb_comm_set_lcb_cmd_serial_number(gh->comm, data, sizeof(data));
}

void general_handler_get_mcb_self_serial_number(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    uint8_t data[] = {0x04, 0x00, 0x00};
    lcb_comm_set_lcb_cmd_serial_number(gh->comm, data, sizeof(data));
}

void general_handler_get_sn_max_length(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    uint8_t data[] = {0x03, 0x00, 0x00};
    lcb_comm_set_lcb_cmd_serial_number(gh->comm, data, sizeof(data));
}

/* ============================================================================
 * Public: Peripheral Queries
 * ============================================================================ */

void general_handler_start_magnet_angle_calibration(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    lcb_ud_set_magnet_angle_state(gh->comm, 0x00);
}

void general_handler_get_magnet_angle(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    lcb_ud_get_magnet_angle(gh->comm, LCB_QUEUE_MODE_ALWAYS_ADD);
}

void general_handler_set_pfc_mode(general_handler_t *gh, bool enable)
{
    if (gh == NULL || gh->comm == NULL) return;
    /* Java: enable → 0x00, disable → 0x01 (inverted) */
    lcb_ud_set_pfc_mode(gh->comm, enable ? 0x00 : 0x01);
}

void general_handler_get_moto_type(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    lcb_ud_get_moto_type(gh->comm);
}

void general_handler_get_mcb_voltage(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    lcb_ud_get_mcb_voltage(gh->comm, LCB_QUEUE_MODE_ALWAYS_ADD);
}

void general_handler_init_lcb_mcb_before_workout(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    lcb_comm_set_lcb_initial_highest_priority(gh->comm);
}

void general_handler_set_lcb_into_update_program_mode(general_handler_t *gh)
{
    if (gh == NULL || gh->comm == NULL) return;
    static const uint8_t data[] = { 'J', 'S' };
    lcb_comm_set_lcb_update_program(gh->comm, data, sizeof(data));
}

/* ============================================================================
 * Public: Getters
 * ============================================================================ */

bool general_handler_get_lcb_comm_disconnect_state(const general_handler_t *gh)
{
    (void)gh;
    return false; /* Placeholder — matches Java implementation */
}

LcbType_t general_handler_get_lcb_type(const general_handler_t *gh)
{
    if (gh == NULL) return LCB_TYPE_UNKNOWN;
    return gh->machine_lcb_type;
}

machine_category_type_t general_handler_get_category(const general_handler_t *gh)
{
    if (gh == NULL) return MACHINE_CATEGORY_UNKNOWN;
    return gh->machine_category;
}

bool general_handler_get_induction_cal_points(const general_handler_t *gh, int out_points[5])
{
    if (gh == NULL || out_points == NULL || !gh->has_induction_cal) return false;
    memcpy(out_points, gh->induction_cal_points, sizeof(gh->induction_cal_points));
    return true;
}

bool general_handler_has_induction_cal(const general_handler_t *gh)
{
    if (gh == NULL) return false;
    return gh->has_induction_cal;
}

bool general_handler_is_induction_cal_valid(const general_handler_t *gh)
{
    if (gh == NULL) return false;
    return gh->valid_induction_cal;
}

/* Induction cal-point monotonicity check (Java: Machine.validateCalPoints()) */
bool gh_validate_cal_points(general_handler_t *gh)
{
    const int *p = gh->induction_cal_points;
    if (p[1] == 0 && p[3] == 0) {
        if (p[0] > 0 && p[4] > p[2] && p[2] > p[0])
            return true;
    } else if (p[0] > 0 && p[4] > p[3] && p[3] > p[2] && p[2] > p[1] && p[1] > p[0]) {
        return true;
    }
    return false;
}

/* ============================================================================
 * Internal: Set Machine LCB Type (called from callback bridge)
 * ============================================================================ */

void gh_set_machine_lcb_type(general_handler_t *gh, uint8_t type_byte)
{
    gh->machine_lcb_type = type_byte;
    gh->machine_category = map_lcb_type_to_category(type_byte);
    LCB_LOGD(TAG, "Machine LCB type=0x%02X category=%d", type_byte, gh->machine_category);

    LCB_LOGD(TAG, "####Frank_Debug general_handler gh_proc_initial_lcb frank_1 GH_INIT_TM_3_CMDS_INITIAL");

    /* Induction-brake LCB types: read back 5 PWM calibration points
     * (Java: GeneralHandler.setMachineLcbType()'s byteGroup loop). */
    switch (type_byte) {
        case LCB_TYPE_ELECTROMAGNETIC_INCLINE:
        case LCB_TYPE_BIKE:
        case LCB_TYPE_GLOBAL_INDUCTION:
        case LCB_TYPE_GLOBAL_FULL_FUNCTION:
        case LCB_TYPE_GLOBAL_INDUCTION_INCLINE:
            gh->cal_point = 0;
            if (gh->comm != NULL) {
                lcb_dd_read_pwm(gh->comm, 1);
                lcb_dd_read_pwm(gh->comm, 2);
                lcb_dd_read_pwm(gh->comm, 3);
                lcb_dd_read_pwm(gh->comm, 4);
                lcb_dd_read_pwm(gh->comm, 5);
            }
            LCB_LOGD(TAG, "start read lcb pwm");
            break;
        default:
            break;
    }
}

/* ============================================================================
 * Internal: Reconnect timer (called from callback bridge)
 * ============================================================================ */

void gh_set_reconnect_deadline(general_handler_t *gh)
{
    clock_gettime(CLOCK_REALTIME, &gh->reconnect_deadline);
    gh->reconnect_deadline.tv_sec += GH_RECONNECT_TIMEOUT_MS / 1000;
    gh->reconnect_deadline.tv_nsec += (GH_RECONNECT_TIMEOUT_MS % 1000) * 1000000L;
    if (gh->reconnect_deadline.tv_nsec >= 1000000000L) {
        gh->reconnect_deadline.tv_sec += 1;
        gh->reconnect_deadline.tv_nsec -= 1000000000L;
    }
    gh->reconnect_timer_started = true;
}
