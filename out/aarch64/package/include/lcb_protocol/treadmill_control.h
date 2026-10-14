/**
 * @file treadmill_control.h
 * @brief Treadmill machine control — Layer_3 "control"
 *
 * Converted from chimera2:
 *   WorkoutControllerService (Play/Pause/Resume/Stop/EStop, safety status)
 *   LCBService / LCBCmdHandler / LCBDataHandler (function control, data return)
 *   LCBMachineInfoHandler.treadmillCalibration()
 *
 * Config-driven: every range / feature flag comes from the frame_config_data_t
 * the caller resolved from frame_configs.json (frame_config_registry_*), so
 * TM538 / TM548 / TM560 / TM561 differ only by data. Sits above
 * treadmill_machine_adapter / treadmill_interface (adapters) and
 * treadmill_machine_convert (capabilities); the protocol layer is untouched.
 *
 * Threading: no threads are created here. Call treadmill_control_tick()
 * periodically (e.g. every 200 ms) to advance workout data and calibration.
 */

#ifndef TREADMILL_CONTROL_H
#define TREADMILL_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"
#include "lcb_type.h"
#include "frame_config_data.h"
#include "treadmill_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct treadmill_control treadmill_control_t;

typedef enum {
    TM_RUN_STATE_IDLE = 0,
    TM_RUN_STATE_RUNNING,
    TM_RUN_STATE_PAUSED,
    TM_RUN_STATE_STOPPED,
    TM_RUN_STATE_ESTOP
} tm_run_state_t;

typedef struct {
    uint32_t elapsed_ms;
    float    distance_km;
    float    calories_kcal;
    float    actual_speed_kph;
    float    actual_incline_percent;
    float    target_speed_kph;
    float    target_incline_percent;
    uint16_t actual_rpm;
} tm_workout_data_t;

typedef enum {
    TM_CAL_IDLE = 0,
    TM_CAL_INCLINE_UP,
    TM_CAL_INCLINE_DOWN
} tm_cal_phase_t;

/* What put the control into ESTOP (reported with on_estop, kept until release). */
typedef enum {
    TM_ESTOP_SRC_NONE = 0,
    TM_ESTOP_SRC_COMMAND,     /* treadmill_control_estop() (UI EStop button) */
    TM_ESTOP_SRC_SAFETY_KEY,  /* 0x97 ext status: safety key down / MCB lock */
    TM_ESTOP_SRC_LCB_ERROR    /* LCB error code 0x02B2 (E-Stop / safety key on models without 0x97) */
} tm_estop_source_t;

typedef struct {
    void *user_data;

    void (*on_state_changed)(void *ud, tm_run_state_t state);
    void (*on_speed_kph)(void *ud, float actual_kph);
    void (*on_incline_percent)(void *ud, float actual_percent);
    void (*on_rpm)(void *ud, uint16_t rpm);
    void (*on_safe_to_start)(void *ud, bool safe);
    void (*on_person_in_place)(void *ud, bool in_place);
    /* E-Stop trigger (active=true) / release (active=false) message; source is
     * what triggered it. The E-Stop error codes 0x02B2 / 0x02B1 are never
     * reported through on_error. */
    void (*on_estop)(void *ud, bool active, tm_estop_source_t source);
    void (*on_over_current)(void *ud, bool active);
    void (*on_dc_bus_current)(void *ud, uint16_t current);
    void (*on_calibration_result)(void *ud, bool success, uint16_t adc_min, uint16_t adc_max);
    void (*on_workout_data)(void *ud, const tm_workout_data_t *data);
    void (*on_error)(void *ud, uint16_t error_code);
} treadmill_control_callbacks_t;

/* ---------------------------------------------------------------- lifecycle */

/** @p fc is copied. Returns NULL if fc is NULL. */
treadmill_control_t *treadmill_control_create(const frame_config_data_t *fc);
/** Like _create(), but wraps an interface the caller owns (e.g. HalEntryPoint's
 *  g_treadmill_interface). The control never destroys it. */
treadmill_control_t *treadmill_control_create_with_interface(const frame_config_data_t *fc,
                                                             treadmill_interface_t *ti);
void treadmill_control_destroy(treadmill_control_t *tc);

void treadmill_control_callbacks_init(treadmill_control_callbacks_t *cb);
void treadmill_control_set_callbacks(treadmill_control_t *tc, const treadmill_control_callbacks_t *cb);
void treadmill_control_unregister_callbacks(treadmill_control_t *tc);

/** Starts the owned treadmill_interface on @p comm (NOT owned), applies the
 *  frame config (rpm 0, incline min, motor type). */
bool treadmill_control_start(treadmill_control_t *tc, lcb_comm_t *comm);
void treadmill_control_stop(treadmill_control_t *tc);
void treadmill_control_set_lcb_type(treadmill_control_t *tc, LcbType_t type);

/** Advance workout data (while RUNNING) and the calibration state machine. */
void treadmill_control_tick(treadmill_control_t *tc, uint32_t now_ms);

/* ------------------------------------------------------------ machine control */

bool treadmill_control_play(treadmill_control_t *tc);    /* IDLE/STOPPED -> RUNNING */
bool treadmill_control_pause(treadmill_control_t *tc);   /* RUNNING -> PAUSED */
bool treadmill_control_resume(treadmill_control_t *tc);  /* PAUSED -> RUNNING */
bool treadmill_control_stop_workout(treadmill_control_t *tc); /* RUNNING/PAUSED -> STOPPED */
bool treadmill_control_estop(treadmill_control_t *tc);   /* any -> ESTOP */
/* ESTOP -> STOPPED. Refused while an MCB lock (0x02B2 / 0x97) is active: those
 * release on their own (Java LCBService.onTmSafetykeyStateUpdate) once the LCB
 * stops reporting them — 2 s after a 0x97 unlock, 6 s without a new 0x02B2. */
bool treadmill_control_estop_release(treadmill_control_t *tc);

/** Values are clamped to the frame config's ranges; returns the applied value. */
float treadmill_control_set_speed_kph(treadmill_control_t *tc, float kph);
float treadmill_control_set_incline_percent(treadmill_control_t *tc, float percent);
float treadmill_control_set_weight_kg(treadmill_control_t *tc, float kg);

tm_run_state_t treadmill_control_get_state(const treadmill_control_t *tc);
/** TM_ESTOP_SRC_NONE unless in ESTOP. */
tm_estop_source_t treadmill_control_get_estop_source(const treadmill_control_t *tc);
void treadmill_control_get_workout_data(const treadmill_control_t *tc, tm_workout_data_t *out);
float treadmill_control_get_weight_kg(const treadmill_control_t *tc);

/* ---------------------------------------------------------- function control */

/** Effective only if frame config has_current_oc; returns whether applied. */
bool treadmill_control_set_over_current_detect(treadmill_control_t *tc, bool enable);
bool treadmill_control_get_over_current_active(const treadmill_control_t *tc);
uint16_t treadmill_control_get_dc_bus_current(const treadmill_control_t *tc);

void     treadmill_control_set_motor_type(treadmill_control_t *tc, uint16_t watts);
uint16_t treadmill_control_get_motor_type(const treadmill_control_t *tc);

void treadmill_control_set_tread_sense(treadmill_control_t *tc, bool enable);
bool treadmill_control_get_person_in_place(const treadmill_control_t *tc);

bool treadmill_control_is_safe_to_start(const treadmill_control_t *tc);
void treadmill_control_set_encoder_check(treadmill_control_t *tc, bool enable); /* Performance Plus only */
/* Error codes are only reported (on_error); whether one blocks Go is decided by
 * the UI's error-code database (A-level blocks, B-level does not) and pushed
 * here — Java: safe-to-start is false while the error panel is shown. */
void treadmill_control_set_error_block(treadmill_control_t *tc, bool blocked);
void treadmill_control_clear_error(treadmill_control_t *tc);   /* = set_error_block(false) */

/** Feed the 0x71 status byte (incline down/up, main motor, encoder/self power). */
void treadmill_control_on_lcb_status(treadmill_control_t *tc, uint8_t status);
bool treadmill_control_get_self_power(const treadmill_control_t *tc);

/** Ignored (returns false) while RUNNING/PAUSED, as in chimera. */
bool treadmill_control_enter_erp(treadmill_control_t *tc);

/** Needs frame config supports_calibration; not allowed while RUNNING/PAUSED. */
bool treadmill_control_calibration_start(treadmill_control_t *tc, uint32_t now_ms);
bool treadmill_control_calibration_in_progress(const treadmill_control_t *tc);
void treadmill_control_calibration_get_result(const treadmill_control_t *tc,
                                              uint16_t *adc_min, uint16_t *adc_max);

#ifdef __cplusplus
}
#endif

#endif /* TREADMILL_CONTROL_H */
