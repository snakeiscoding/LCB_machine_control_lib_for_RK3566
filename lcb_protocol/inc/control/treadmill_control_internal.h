/**
 * @file treadmill_control_internal.h
 * @brief Internal struct for treadmill_control_t — shared with the test suite
 * so it can drive the interface-bridge callbacks directly. NOT for external use.
 */

#ifndef TREADMILL_CONTROL_INTERNAL_H
#define TREADMILL_CONTROL_INTERNAL_H

#include "treadmill_control.h"
#include "treadmill_machine_adapter.h"
#include <pthread.h>

/* 0x71 status byte bits */
#define TM_STATUS_BIT_INCLINE_DOWN 0x01
#define TM_STATUS_BIT_INCLINE_UP   0x02
#define TM_STATUS_BIT_MAIN_MOTOR   0x04
#define TM_STATUS_BIT_ENCODER_SELF_POWER 0x08

/* In-used (0x95) values */
#define TM_IN_USED_YES 0xFF
#define TM_IN_USED_NO  0x55

/* 0x96 main motor info: DC-bus current, little-endian within the 12-byte
 * payload (chimera reads packet bytes [14],[15] = payload [10],[11]). */
#define TM_MOTOR_INFO_DC_BUS_LOW_IDX  10
#define TM_MOTOR_INFO_DC_BUS_HIGH_IDX 11

/* E-Stop error codes. treadmill_interface turns them into
 * on_safetykey_locked_code (Java LCBInterface: 0x02B2 -> onLCBSafetykeyLockedCode(true),
 * 0x02B1 -> (false)); if one still reaches tc_on_error_code it is dropped, never
 * reported through on_error. */
#define TM_ERR_ESTOP          0x02B2
#define TM_ERR_ESTOP_UNLOCK   0x02B1

/* MCB lock sources (Java LCBInfo.MCBState), OR-ed into lock_state */
#define TM_LOCK_SWITCH        0x01   /* IO safety switch (not wired here) */
#define TM_LOCK_MCB_CODE      0x02   /* 0x02B2 / 0x02B1 */
#define TM_LOCK_MCB_STATE     0x04   /* 0x97 ext status lock */
#define TM_LOCK_MCB_CIRCUIT   0x08   /* start unlock */

/* Java LCBDataTimerHandler: SAFETYKEY_RELEASE_DELAY = 2 s after a state-lock
 * release, SAFETYKEY_UNLOCK_CHECK = 6 s ("make sure no 0x02B2") otherwise. */
#define TM_SAFETYKEY_RELEASE_DELAY_MS  2000
#define TM_SAFETYKEY_UNLOCK_CHECK_MS   6000

/* Calibration (LCBMachineInfoHandler.treadmillCalibration) */
#define TM_CAL_POLL_INTERVAL_MS   3000
#define TM_CAL_STABLE_ADC_DELTA   200
#define TM_CAL_MIN_SPAN_ADC       400
#define TM_CAL_TIMEOUT_MS         120000

struct treadmill_control {
    frame_config_data_t fc;              /* owned copy (calibration may update ADC range) */
    treadmill_interface_t *ti;           /* owned unless owns_ti == false */
    bool owns_ti;
    tm_adapter_t adapter;
    lcb_comm_t *comm;                    /* NOT owned */

    treadmill_control_callbacks_t user_cb;
    pthread_mutex_t mutex;

    tm_run_state_t state;
    tm_estop_source_t estop_source;      /* valid while state == ESTOP */

    /* Safety key / MCB lock (Java LCBService.onTmSafetykeyStateUpdate) */
    uint8_t  lock_state;                 /* TM_LOCK_* bits (Java LCBInfo._mcbLockstate) */
    uint32_t release_wait_ms;            /* >0: release check running (Java tmSafetykeyCodeReleaseCheck) */

    /* targets / actuals */
    float target_kph;
    float target_incline;
    uint16_t actual_rpm;
    uint16_t actual_adc;                 /* raw, reverse bit possibly set */
    float actual_kph;
    float actual_incline;

    /* function control */
    uint16_t motor_type;
    bool tread_sense;
    bool oc_detect_enabled;
    bool oc_active;
    uint16_t dc_bus_current;
    bool person_in_place;

    /* safe-to-start inputs */
    bool main_motor_on;
    bool incline_moving;
    bool encoder_rpm_status;
    bool encoder_check_enabled;
    bool self_power;
    bool error_active;
    bool safe_to_start;
    bool safe_reported;                  /* last value sent to on_safe_to_start */

    /* workout data */
    float weight_kg;
    uint32_t elapsed_ms;
    float distance_km;
    float calories_kcal;
    uint32_t last_tick_ms;
    bool tick_primed;

    /* calibration */
    tm_cal_phase_t cal_phase;
    uint32_t cal_start_ms;
    uint32_t cal_next_poll_ms;
    int      cal_last_adc;               /* -1 = no sample yet */
    int      cal_high_adc;
    int      cal_low_adc;
    uint16_t cal_result_min;
    uint16_t cal_result_max;
};

/* Interface-bridge callbacks — exposed so tests can drive them without a
 * heartbeat thread. `ud` is the treadmill_control_t*. */
void tc_on_rpm(void *ud, uint16_t rpm);
void tc_on_incline_adc(void *ud, uint16_t adc);
void tc_on_person_in_place(void *ud, bool in_place);
void tc_on_safetykey_lock_state(void *ud, bool locked);
void tc_on_safetykey_locked_code(void *ud, bool locked);
void tc_on_start_unlock(void *ud);
void tc_on_auto_oc_state(void *ud, bool detected);
void tc_on_error_code(void *ud, uint16_t code);
void tc_on_motor_info(void *ud, const uint8_t *info, uint8_t length);
void tc_on_lcb_status(void *ud, uint8_t status);

#endif /* TREADMILL_CONTROL_INTERNAL_H */
