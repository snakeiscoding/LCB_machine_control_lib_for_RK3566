/**
 * @file frame_config_data_bike.h
 * @brief Real per-model Bike capability data — Layer_1 "capabilities"
 *
 * Converted from: com/jht/chimera/lib/common/config/frames/FrameConfigRecumbentBike_Performance.java
 * (chimera2 reference project). Outputs the *current* architecture's
 * frame_config_data_t (see frame_config_data.h) — NOT the older, separate
 * frame_config_t in frame_config.h (that module is a different, pre-existing
 * port of the same Java class, kept untouched — see its own file header).
 *
 * Deliberately NOT ported (no consumer needs them yet, and porting would
 * mean either guessing a value or a schema extension):
 *   - speed_kph_range — Java computes speedMph's max via
 *     SpeedConvertUtil.bikeWattsToMPH(500) and speedKph's max via
 *     UnitConverter.ConvertMphToKph(...) on that result; neither conversion
 *     utility is present in this codebase (same omission the pre-existing
 *     frame_config.h already documents for this exact Java class).
 *   - watts_range, rpm_range — Java's base FrameConfig class doesn't set
 *     model-specific min/max for these for this frame; nothing here to port.
 *   - rate_resistance_per_sec (Java: RATE_RESISTANCE_PER_SEC = 5f) —
 *     frame_config_data_t has no such field (only rate_incline_percent_per_sec
 *     and rate_kph_per_sec exist, ported from a different Kotlin project).
 *     Leave for a future schema-extension task, don't repurpose another field.
 *   - RESISTANCE_TABLE / WATTS_TABLE (CalibrationTable.ChopperBike) — no
 *     consumer, and the calibration table isn't ported.
 *   - weight ranges, KeyMap, workoutDistance — no consumer.
 */

#ifndef FRAME_CONFIG_DATA_BIKE_H
#define FRAME_CONFIG_DATA_BIKE_H

#include "frame_config_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Populate @p cfg with the RecumbentBike "Performance" ("RB317")
 * frame's capability data (frame prefix "RB317", LCB type 0x13, resistance
 * level range 1-30). No-op if @p cfg is NULL.
 */
void frame_config_data_init_bike_recumbent_performance(frame_config_data_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_CONFIG_DATA_BIKE_H */
