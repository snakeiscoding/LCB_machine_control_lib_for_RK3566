/**
 * @file frame_config_data.h
 * @brief Runtime, fully-resolved per-model capability data — Layer_1
 * "capabilities". Consumed by "control" (and, transitively, "protocol") to
 * clamp/convert command values per machine model.
 *
 * Field set mirrors frame_config_spec_t (see frame_config_spec.h), which is
 * aligned to frame_configs_json_schema.md. This is the runtime/resolved
 * counterpart of that wire-format DTO (kept as a separate struct rather
 * than reusing frame_config_spec_t directly, so future registry-side
 * resolution — e.g. category-preset fallback filling — has somewhere to
 * write results without mutating the parsed spec).
 *
 * Not to be confused with the separate, pre-existing, hardcoded-data
 * `frame_config_t` in frame_config.h (ported from a different Java source,
 * chimera2) — this type is named frame_config_data_t specifically to avoid
 * colliding with that one.
 */

#ifndef FRAME_CONFIG_DATA_H
#define FRAME_CONFIG_DATA_H

#include <stdbool.h>
#include "machine_category.h"
#include "value_range.h"
#include "frame_config_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char model_name[FC_MODEL_NAME_LEN];
    int  frame_id; /* always 0 - no frameId key in the schema */
    machine_category_t category;
    bool is_lifestyle;

    char frame_prefixes[FC_MAX_FRAME_PREFIXES][FC_FRAME_PREFIX_LEN];
    int  frame_prefix_count;

    char support_product_flavors[FC_MAX_PRODUCT_FLAVORS][FC_PRODUCT_FLAVOR_LEN];
    int  support_product_flavor_count;

    bool                 has_speed_kph_range;
    value_range_float_t  speed_kph_range;
    bool                 has_incline_percent_range;
    value_range_float_t  incline_percent_range;
    bool                 has_incline_adc_range;
    value_range_int_t    incline_adc_range;
    bool                 has_resistance_level_range;
    value_range_int_t    resistance_level_range;
    bool                 has_watts_range;
    value_range_int_t    watts_range;
    bool                 has_rpm_range;
    value_range_int_t    rpm_range;

    /* Treadmill additions (see frame_config_spec_t) */
    bool                 has_speed_mph_range;
    value_range_float_t  speed_mph_range;
    bool                 has_weight_lbs_range;
    value_range_int_t    weight_lbs_range;
    bool                 has_weight_kg_range;
    value_range_int_t    weight_kg_range;
    int                  motor_type;

    float rate_incline_percent_per_sec; /* default 0.71f */
    float rate_kph_per_sec;             /* default 0.8f */

    bool  has_rate_resistance_per_sec;
    float rate_resistance_per_sec;

    bool has_key_map;
    char key_map[FC_KEY_MAP_LEN];
    bool has_power_rule;
    char power_rule[FC_POWER_RULE_LEN];

    bool has_incline_adc_offset_min;
    int  incline_adc_offset_min;
    bool has_incline_adc_offset_max;
    int  incline_adc_offset_max;

    int   dc_bus_upper_limit;           /* default 72 */
    int   dc_bus_lower_limit;           /* default 63 */
    int   steps_per_floor;              /* default 16 */
    float steps_per_meter;              /* default 4.37f */

    char supported_lcb_types[FC_MAX_LCB_TYPES][FC_LCB_TYPE_STR_LEN];
    int  supported_lcb_type_count;

    bool  has_emergency_stop;
    bool  uses_reverse_incline;
    bool  supports_calibration;
    bool  requires_belt_lubrication;
    bool  requires_chain_lubrication;
    bool  has_cadence;
    bool  has_can;
    bool  use_can_heart_rate;
    bool  has_ir_sensor;
    bool  has_cm_control_zone;
    int   bump_sensor_trigger_threshold; /* default -1 */
    bool  has_tm_lock_state;
    bool  has_tm_over_speed;
    bool  tm_supports_reverse_belt;
    bool  has_tm_reverse_belt_kph_max;
    float tm_reverse_belt_kph_max;
    bool  has_tm_auto_ramp;
    int   rpm_ramp_up_stage_value;
    int   rpm_ramp_down_stage_value;
    bool  lcb_can_control_watts;
    bool  watts_inaccurate;
    bool  has_current_oc;
    bool  is_hybrid_tm;
    bool  has_ecb_brake;
} frame_config_data_t;

/**
 * @brief Zero-initializes @p cfg and applies the same field defaults
 * frame_config_spec_init() does. No-op if @p cfg is NULL.
 */
void frame_config_data_init(frame_config_data_t *cfg);

/**
 * @brief First entry of frame_prefixes, or "" if empty/NULL.
 */
const char *frame_config_data_primary_frame_prefix(const frame_config_data_t *cfg);

/**
 * @brief True if @p serial_number case-insensitively starts with any of
 * cfg->frame_prefixes. False if @p cfg is NULL, @p serial_number is
 * NULL/blank, or there is no match.
 */
bool frame_config_data_matches_serial_number(const frame_config_data_t *cfg, const char *serial_number);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_CONFIG_DATA_H */
