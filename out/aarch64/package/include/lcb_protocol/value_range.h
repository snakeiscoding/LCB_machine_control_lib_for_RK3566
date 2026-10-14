/**
 * @file value_range.h
 * @brief JSON-driven numeric range types — Layer_1 "capabilities"
 *
 * Aligned to frame_configs_json_schema.md's `MetricRangeWithIncrement`
 * (float ranges: speedKph, inclinePercent) and `MetricRange` (int ranges:
 * inclineAdc, resistanceLevel, watts, rpm).
 *
 * Note the asymmetry is in the schema itself, not a C limitation:
 * `MetricRangeWithIncrement` has `minValue`/`maxValue`/`default`/`increment`
 * (+ optional `unit`), while `MetricRange` has only `minValue`/`maxValue`/
 * `default` (+ optional `unit`) — no increment/step field at all. So
 * value_range_int_t simply has no increment member; there is nothing to
 * drop or special-case during conversion (frame_config_json_loader.c no
 * longer needs the old "int ranges drop stepSize" hack this replaced).
 */

#ifndef VALUE_RANGE_H
#define VALUE_RANGE_H

#include <stdbool.h>
#include "frame_config_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float min_value;
    float max_value;
    float default_value;
    float increment;
    char  unit[FC_UNIT_LEN];
    bool  has_unit;
} value_range_float_t;

typedef struct {
    int  min_value;
    int  max_value;
    int  default_value;
    char unit[FC_UNIT_LEN];
    bool has_unit;
} value_range_int_t;

#ifdef __cplusplus
}
#endif

#endif /* VALUE_RANGE_H */
