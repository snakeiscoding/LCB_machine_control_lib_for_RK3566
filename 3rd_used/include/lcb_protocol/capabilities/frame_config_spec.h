/**
 * @file frame_config_spec.h
 * @brief Wire-format JSON schema DTOs — Layer_1 "capabilities"
 *
 * Aligned to frame_configs_json_schema.md (the JHT frame_configs.json JSON
 * Schema spec), NOT the older ad-hoc schema this file previously mirrored
 * (which used min/max/defaultValue/stepSize and a 7-key features set taken
 * from Kotlin LCBCore's FrameConfigSpec.kt). This mirrors the documented
 * schema field-for-field (the "wire format"), separate from the runtime
 * frame_config_data_t. Nullable JSON objects (speedKph, inclinePercent,
 * inclineAdc, resistanceLevel, watts, rpm, climbmillParams) become
 * `bool has_xxx` + a value struct, matching the project's existing style of
 * representing optionality via a flag/count rather than a pointer or
 * dynamic container (see frame_config.h).
 *
 * Pragmatic deviation from the schema doc: the schema marks modelName,
 * category, framePrefixes, supportProductFlavors, speedKph, inclinePercent,
 * inclineAdc, resistanceLevel, watts, rpm, rateInclinePercentPerSec,
 * rateKphPerSec, supportedLcbTypes, and features as "required". This loader
 * only treats a MISSING modelName or category as a hard parse failure (see
 * frame_config_json_loader.c) — every other "required" field falls back to
 * a sensible default (empty array / false / 0) when absent, so real-world
 * frame_configs.json data that predates some of these fields still loads.
 */

#ifndef FRAME_CONFIG_SPEC_H
#define FRAME_CONFIG_SPEC_H

#include <stdbool.h>
#include "machine_category.h"
#include "value_range.h"
#include "frame_config_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Port of FrameFeatures — hardware safety/sensor/actuator capability
 * flags. Only hasEmergencyStop/usesReverseIncline/supportsCalibration are
 * "required" by the schema; the rest default to false (or the documented
 * sentinel, e.g. bump_sensor_trigger_threshold = -1 "if unsupported") when
 * absent from the JSON.
 */
typedef struct {
    bool  has_emergency_stop;
    bool  uses_reverse_incline;
    bool  supports_calibration;
    bool  requires_belt_lubrication;
    bool  requires_chain_lubrication;
    bool  has_cadence;
    bool  has_can;              /* schema: hasCan (not hasCanBus) */
    bool  use_can_heart_rate;
    bool  has_ir_sensor;
    bool  has_cm_control_zone;
    int   bump_sensor_trigger_threshold; /* default -1 = unsupported */
    bool  has_tm_lock_state;
    bool  has_tm_over_speed;
    bool  tm_supports_reverse_belt;
    bool  has_tm_reverse_belt_kph_max;   /* tmReverseBeltKphMax is nullable */
    float tm_reverse_belt_kph_max;
    bool  has_tm_auto_ramp;
    int   rpm_ramp_up_stage_value;
    int   rpm_ramp_down_stage_value;
    bool  lcb_can_control_watts;
    bool  watts_inaccurate;
    bool  has_current_oc;
    bool  is_hybrid_tm;
    bool  has_ecb_brake;
} frame_features_t;

typedef struct {
    int   steps_per_floor;   /* default 16 */
    float steps_per_meter;   /* default 4.37f */
} climbmill_params_t;

typedef struct {
    char model_name[FC_MODEL_NAME_LEN];
    machine_category_t category;
    bool is_lifestyle;

    char frame_prefixes[FC_MAX_FRAME_PREFIXES][FC_FRAME_PREFIX_LEN];
    int  frame_prefix_count;

    /* supportProductFlavors: Android build-flavor compatibility list
     * (Parker/Chimera/LCGUI/Vision600E/T600XE/ProjectOne). Stored as plain
     * strings rather than an enum since it's purely informational here. */
    char support_product_flavors[FC_MAX_PRODUCT_FLAVORS][FC_PRODUCT_FLAVOR_LEN];
    int  support_product_flavor_count;

    bool                 has_speed_kph;
    value_range_float_t  speed_kph;
    bool                 has_incline_percent;
    value_range_float_t  incline_percent;
    bool                 has_incline_adc;
    value_range_int_t    incline_adc;
    bool                 has_resistance_level;
    value_range_int_t    resistance_level;
    bool                 has_watts;
    value_range_int_t    watts;
    bool                 has_rpm;
    value_range_int_t    rpm;          /* JSON "rpm", or "speedRpm" when "rpm" is absent (treadmill) */

    /* Treadmill additions (frame_configs.json: speedMph/weightLbs/weightKg/motorType) */
    bool                 has_speed_mph;
    value_range_float_t  speed_mph;
    bool                 has_weight_lbs;
    value_range_int_t    weight_lbs;
    bool                 has_weight_kg;
    value_range_int_t    weight_kg;
    int                  motor_type;   /* motorType, default 0 */

    float rate_incline_percent_per_sec; /* default 0.71f */
    float rate_kph_per_sec;             /* default 0.8f */

    bool  has_rate_resistance_per_sec;  /* rateResistancePerSec is nullable */
    float rate_resistance_per_sec;

    bool has_key_map;                   /* keyMap is nullable */
    char key_map[FC_KEY_MAP_LEN];
    bool has_power_rule;                /* powerRule is nullable */
    char power_rule[FC_POWER_RULE_LEN];

    bool has_incline_adc_offset_min;    /* inclineAdcOffsetMin is nullable */
    int  incline_adc_offset_min;
    bool has_incline_adc_offset_max;    /* inclineAdcOffsetMax is nullable */
    int  incline_adc_offset_max;

    int   dc_bus_upper_limit;           /* default 72 */
    int   dc_bus_lower_limit;           /* default 63 */

    bool                has_climbmill_params;
    climbmill_params_t  climbmill_params;

    char supported_lcb_types[FC_MAX_LCB_TYPES][FC_LCB_TYPE_STR_LEN];
    int  supported_lcb_type_count;

    frame_features_t features;
} frame_config_spec_t;

/**
 * @brief Port of CategoryPreset — category-level default metric ranges and
 * capability flags, used by the registry as a fallback for a frame that
 * omits a range, and as the basis for the "unknown model" fallback (see
 * frame_config_registry.h). One entry per key of the JSON's root-level
 * `categoryPresets` object.
 */
typedef struct {
    machine_category_t category;

    bool                 has_default_speed_kph;
    value_range_float_t  default_speed_kph;
    bool                 has_default_incline_percent;
    value_range_float_t  default_incline_percent;
    bool                 has_default_incline_adc;
    value_range_int_t    default_incline_adc;
    bool                 has_default_resistance_level;
    value_range_int_t    default_resistance_level;
    bool                 has_default_watts;
    value_range_int_t    default_watts;
    bool                 has_default_rpm;
    value_range_int_t    default_rpm;

    bool has_emergency_stop;
    bool uses_reverse_incline;
    bool supports_calibration;
} category_preset_t;

/**
 * @brief Port of FrameConfigMetadata — the root-level `metadata` object.
 * Purely descriptive/administrative; every field is optional and left as
 * an empty string when absent from the JSON.
 */
typedef struct {
    char title[FC_METADATA_STR_LEN];
    char organization[FC_METADATA_STR_LEN];
    char protocol_version[FC_METADATA_STR_LEN];
    char description[FC_METADATA_DESC_LEN];
    char last_updated[FC_METADATA_STR_LEN];
} frame_config_metadata_t;

/**
 * @brief Zero-initializes @p spec and applies the same field defaults the
 * schema documents (isLifestyle=false, rateInclinePercentPerSec=0.71f,
 * rateKphPerSec=0.8f, dcBusUpperLimit=72, dcBusLowerLimit=63,
 * bumpSensorTriggerThreshold=-1, climbmillParams stepsPerFloor=16/
 * stepsPerMeter=4.37f, all other features false). No-op if @p spec is NULL.
 */
void frame_config_spec_init(frame_config_spec_t *spec);

/**
 * @brief Zero-initializes @p preset (category = MC_UNKNOWN, all has_xxx
 * flags false, all bool feature flags false). No-op if @p preset is NULL.
 */
void category_preset_init(category_preset_t *preset);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_CONFIG_SPEC_H */
