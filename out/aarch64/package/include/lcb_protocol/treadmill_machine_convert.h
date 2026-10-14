/**
 * @file treadmill_machine_convert.h
 * @brief Treadmill unit conversion / range clamping — Layer_1 "capabilities"
 *
 * Converted from chimera2 Machine.rpmToKph()/kphToRpm()/adcToIncline()/
 * inclineToAdc(). Pure functions driven entirely by frame_config_data_t
 * (loaded from frame_configs.json) — no state, no I/O.
 */

#ifndef TREADMILL_MACHINE_CONVERT_H
#define TREADMILL_MACHINE_CONVERT_H

#include <stdint.h>
#include <stdbool.h>
#include "frame_config_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Reverse-incline direction bit folded into the incline ADC / RPM payload. */
#define TM_REVERSE_BIT 0x8000u

/** Clamp @p v into [lo, hi]. */
float tm_clamp_float(float v, float lo, float hi);

/** Clamp kph into fc->speed_kph_range (0 if no range). */
float tm_clamp_speed_kph(const frame_config_data_t *fc, float kph);
/** Clamp % into fc->incline_percent_range (0 if no range). */
float tm_clamp_incline_percent(const frame_config_data_t *fc, float percent);
/** Clamp kg into fc->weight_kg_range (unchanged if no range). */
float tm_clamp_weight_kg(const frame_config_data_t *fc, float kg);

/** Linear map between the kph range and the rpm range. 0 kph -> 0 rpm. */
uint16_t tm_kph_to_rpm(const frame_config_data_t *fc, float kph);
float    tm_rpm_to_kph(const frame_config_data_t *fc, uint16_t rpm);

/** Linear map between the incline % range and the incline ADC range.
 *  tm_incline_to_adc() folds in TM_REVERSE_BIT when fc->uses_reverse_incline. */
uint16_t tm_incline_to_adc(const frame_config_data_t *fc, float percent);
/** Strips TM_REVERSE_BIT before converting. */
float    tm_adc_to_incline(const frame_config_data_t *fc, uint16_t adc);

/** Chimera's kph<->mph and lbs<->kg helpers. */
float tm_kph_to_mph(float kph);
float tm_mph_to_kph(float mph);
float tm_lbs_to_kg(float lbs);
float tm_kg_to_lbs(float kg);

#ifdef __cplusplus
}
#endif

#endif /* TREADMILL_MACHINE_CONVERT_H */
