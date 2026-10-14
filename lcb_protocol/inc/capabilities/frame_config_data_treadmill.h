/**
 * @file frame_config_data_treadmill.h
 * @brief Real per-model Treadmill capability data — Layer_1 "capabilities"
 *
 * Converted from: com/jht/chimera/lib/common/config/frames/FrameConfigTreadmill_Performance.java
 * (chimera2 reference project). Outputs the *current* architecture's
 * frame_config_data_t (see frame_config_data.h) — NOT the older, separate
 * frame_config_t in frame_config.h (that module is a different, pre-existing
 * port and is Bike-only).
 *
 * Deliberately NOT ported (no consumer needs them yet, and frame_config_data_t
 * has no field for them — porting would mean guessing a schema extension):
 *   - MOTOR_TYPE, HAS_TM_LOCK_STATE, HAS_CURRENT_OC,
 *     HAS_TM_AUTO_RAMP_UP_AND_LAST_RPM_VALUE / RPM_RAMP_UP_STAGE_VALUE /
 *     RPM_RAMP_DOWN_STAGE_VALUE, HAS_TM_OVER_SPEED, HAS_IR_SENSOR — these are
 *     chimera2 FrameConfig fields with no equivalent in frame_config_data_t
 *     (that struct was ported from a different Kotlin project, LCBCore, not
 *     chimera2). Leave for a future schema-extension task.
 *   - resistance/watts ranges — the Java base class sets a generic
 *     resistance range, but nothing in the TM control path (Java or this
 *     port) reads it for a treadmill.
 *   - speedMph / weight / KeyMap / RESISTANCE_TABLE — no consumer.
 */

#ifndef FRAME_CONFIG_DATA_TREADMILL_H
#define FRAME_CONFIG_DATA_TREADMILL_H

#include "frame_config_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Populate @p cfg with the TM538 "Performance" frame's capability
 * data (frame prefixes "TM538"/"TM549", incline/speed/RPM ranges, reverse
 * incline direction). No-op if @p cfg is NULL.
 */
void frame_config_data_init_treadmill_performance(frame_config_data_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_CONFIG_DATA_TREADMILL_H */
