/**
 * @file frame_config_data_treadmill.c
 * @brief See frame_config_data_treadmill.h
 */

#include "frame_config_data_treadmill.h"
#include <string.h>

/* mph -> kph is a standard, unambiguous conversion (1 mph = 1.609344 kph) —
 * matches Java's SpeedConvertUtil.convertMphToKph(0.5f) for
 * RATE_KPH_PER_SEC. */
#define MPH_TO_KPH(mph) ((float)(mph) * 1.609344f)

static void add_frame_prefix(frame_config_data_t *cfg, const char *prefix)
{
    if (cfg->frame_prefix_count >= FC_MAX_FRAME_PREFIXES) return;
    strncpy(cfg->frame_prefixes[cfg->frame_prefix_count], prefix, FC_FRAME_PREFIX_LEN - 1);
    cfg->frame_prefixes[cfg->frame_prefix_count][FC_FRAME_PREFIX_LEN - 1] = '\0';
    cfg->frame_prefix_count++;
}

static void add_lcb_type(frame_config_data_t *cfg, const char *type_hex)
{
    if (cfg->supported_lcb_type_count >= FC_MAX_LCB_TYPES) return;
    strncpy(cfg->supported_lcb_types[cfg->supported_lcb_type_count], type_hex, FC_LCB_TYPE_STR_LEN - 1);
    cfg->supported_lcb_types[cfg->supported_lcb_type_count][FC_LCB_TYPE_STR_LEN - 1] = '\0';
    cfg->supported_lcb_type_count++;
}

void frame_config_data_init_treadmill_performance(frame_config_data_t *cfg)
{
    if (cfg == NULL) return;

    frame_config_data_init(cfg);

    strncpy(cfg->model_name, "TM538_PERFORMANCE", sizeof(cfg->model_name) - 1);
    cfg->model_name[sizeof(cfg->model_name) - 1] = '\0';
    cfg->category = MC_TREADMILL;

    /* Java: FRAME_PREFIX = "TM538"; getFramePrefixes() also adds "TM549"
     * (used when the detected LCB type is 0xA1). */
    add_frame_prefix(cfg, "TM538");
    add_frame_prefix(cfg, "TM549");

    /* Java: supportLcbTypeList = new int[]{0xC3,0xC4,0xC5,0xC7,0xC8,0xA1}; */
    add_lcb_type(cfg, "0xC3");
    add_lcb_type(cfg, "0xC4");
    add_lcb_type(cfg, "0xC5");
    add_lcb_type(cfg, "0xC7");
    add_lcb_type(cfg, "0xC8");
    add_lcb_type(cfg, "0xA1");

    /* Java: speedKph.setMin(setMinModel(.8f)); setMax(20); precision=1; stepSize=.1f;
     * (setMaxModel(24) is the physical ceiling, not the UI-facing max — this
     * struct follows the registry's existing convention of storing the
     * UI-facing max, see fill_default_treadmill() in frame_config_registry.c) */
    cfg->has_speed_kph_range = true;
    cfg->speed_kph_range.min_value = 0.8f;
    cfg->speed_kph_range.max_value = 20.0f;
    cfg->speed_kph_range.default_value = 0.8f;
    cfg->speed_kph_range.increment = 0.1f;

    /* Java: inclineLimit 0-18 (stepSize .5f); incline = copy(inclineLimit) then
     * setMax(15) — "incline" is the UI/control-facing range actually used. */
    cfg->has_incline_percent_range = true;
    cfg->incline_percent_range.min_value = 0.0f;
    cfg->incline_percent_range.max_value = 15.0f;
    cfg->incline_percent_range.default_value = 0.0f;
    cfg->incline_percent_range.increment = 0.5f;

    /* Java: INCLINE_ADC_OFFSET_MIN=1408, INCLINE_ADC_OFFSET_MAX=128;
     * INCLINE_ADC_MIN = 2092+1408 = 3500; INCLINE_ADC_MAX = 23628-128 = 23500.
     * value_range_int_t has no increment field at all (frame_configs_json_schema.md's
     * MetricRange doesn't define one), so there's nothing to set here even
     * though Java's inclineAdc.stepSize=1. */
    cfg->has_incline_adc_range = true;
    cfg->incline_adc_range.min_value = 3500;
    cfg->incline_adc_range.max_value = 23500;
    cfg->incline_adc_range.default_value = 3500;

    /* Java: speedRPM.updateRange(178, 5290); */
    cfg->has_rpm_range = true;
    cfg->rpm_range.min_value = 178;
    cfg->rpm_range.max_value = 5290;
    cfg->rpm_range.default_value = 178;

    /* Fallback aligned with frame_configs.json Treadmill_Performance */
    cfg->has_speed_mph_range = true;
    cfg->speed_mph_range.min_value = 0.5f;
    cfg->speed_mph_range.max_value = 12.0f;
    cfg->speed_mph_range.default_value = 0.5f;
    cfg->speed_mph_range.increment = 0.1f;
    cfg->has_weight_lbs_range = true;
    cfg->weight_lbs_range.min_value = 50;
    cfg->weight_lbs_range.max_value = 400;
    cfg->weight_lbs_range.default_value = 150;
    cfg->has_weight_kg_range = true;
    cfg->weight_kg_range.min_value = 23;
    cfg->weight_kg_range.max_value = 181;
    cfg->weight_kg_range.default_value = 68;
    cfg->motor_type = 0;

    /* Java: RATE_INCLINE_PERCENT_PER_SEC = 0.47f; RATE_KPH_PER_SEC = convertMphToKph(0.5f); */
    cfg->rate_incline_percent_per_sec = 0.47f;
    cfg->rate_kph_per_sec = MPH_TO_KPH(0.5f);

    /* Java: USES_REVERSE_INCLINE = true; HAS_TM_ESTOP = false;
     * (deliberately the real value, not the registry's synthetic
     * has_emergency_stop=true fallback — see frame_config_data_treadmill.h) */
    cfg->uses_reverse_incline = true;
    cfg->has_emergency_stop = false;
}
