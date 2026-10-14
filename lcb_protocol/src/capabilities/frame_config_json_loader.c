/**
 * @file frame_config_json_loader.c
 * @brief See frame_config_json_loader.h
 */

#include "frame_config_json_loader.h"
#include "cJSON.h"
#include <string.h>

/* Same result as strncpy(dst, src, dst_size - 1) + terminator (truncate,
 * zero-fill), written out to avoid gcc's -Wstringop-truncation. */
static void copy_string(char *dst, size_t dst_size, const char *src)
{
    if (src == NULL || dst_size == 0) return;
    size_t len = strnlen(src, dst_size - 1);
    memcpy(dst, src, len);
    memset(dst + len, 0, dst_size - len);
}

static void parse_unit(const cJSON *range_obj, char *out_unit, size_t unit_size, bool *out_has_unit)
{
    const cJSON *unit = cJSON_GetObjectItemCaseSensitive(range_obj, "unit");
    if (cJSON_IsString(unit) && unit->valuestring != NULL) {
        copy_string(out_unit, unit_size, unit->valuestring);
        *out_has_unit = true;
    } else {
        out_unit[0] = '\0';
        *out_has_unit = false;
    }
}

/* MetricRangeWithIncrement: minValue/maxValue/default/increment(+unit). */
static void parse_value_range_float(const cJSON *range_obj, value_range_float_t *out)
{
    const cJSON *min_value = cJSON_GetObjectItemCaseSensitive(range_obj, "minValue");
    const cJSON *max_value = cJSON_GetObjectItemCaseSensitive(range_obj, "maxValue");
    const cJSON *default_value = cJSON_GetObjectItemCaseSensitive(range_obj, "default");
    const cJSON *increment = cJSON_GetObjectItemCaseSensitive(range_obj, "increment");

    out->min_value = cJSON_IsNumber(min_value) ? (float)min_value->valuedouble : 0.0f;
    out->max_value = cJSON_IsNumber(max_value) ? (float)max_value->valuedouble : 0.0f;
    out->default_value = cJSON_IsNumber(default_value) ? (float)default_value->valuedouble : out->min_value;
    out->increment = cJSON_IsNumber(increment) ? (float)increment->valuedouble : 0.0f;
    parse_unit(range_obj, out->unit, sizeof(out->unit), &out->has_unit);
}

/* MetricRange: minValue/maxValue/default(+unit) - no increment field. */
static void parse_value_range_int(const cJSON *range_obj, value_range_int_t *out)
{
    const cJSON *min_value = cJSON_GetObjectItemCaseSensitive(range_obj, "minValue");
    const cJSON *max_value = cJSON_GetObjectItemCaseSensitive(range_obj, "maxValue");
    const cJSON *default_value = cJSON_GetObjectItemCaseSensitive(range_obj, "default");

    out->min_value = cJSON_IsNumber(min_value) ? min_value->valueint : 0;
    out->max_value = cJSON_IsNumber(max_value) ? max_value->valueint : 0;
    out->default_value = cJSON_IsNumber(default_value) ? default_value->valueint : out->min_value;
    parse_unit(range_obj, out->unit, sizeof(out->unit), &out->has_unit);
}

static void parse_features(const cJSON *features_obj, frame_features_t *out)
{
    const cJSON *v;

    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasEmergencyStop");
    if (cJSON_IsBool(v)) out->has_emergency_stop = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "usesReverseIncline");
    if (cJSON_IsBool(v)) out->uses_reverse_incline = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "supportsCalibration");
    if (cJSON_IsBool(v)) out->supports_calibration = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "requiresBeltLubrication");
    if (cJSON_IsBool(v)) out->requires_belt_lubrication = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "requiresChainLubrication");
    if (cJSON_IsBool(v)) out->requires_chain_lubrication = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasCadence");
    if (cJSON_IsBool(v)) out->has_cadence = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasCan");
    if (cJSON_IsBool(v)) out->has_can = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "useCanHeartRate");
    if (cJSON_IsBool(v)) out->use_can_heart_rate = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasIrSensor");
    if (cJSON_IsBool(v)) out->has_ir_sensor = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasCmControlZone");
    if (cJSON_IsBool(v)) out->has_cm_control_zone = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "bumpSensorTriggerThreshold");
    if (cJSON_IsNumber(v)) out->bump_sensor_trigger_threshold = v->valueint;
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasTmLockState");
    if (cJSON_IsBool(v)) out->has_tm_lock_state = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasTmOverSpeed");
    if (cJSON_IsBool(v)) out->has_tm_over_speed = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "tmSupportsReverseBelt");
    if (cJSON_IsBool(v)) out->tm_supports_reverse_belt = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "tmReverseBeltKphMax");
    if (cJSON_IsNumber(v)) {
        out->has_tm_reverse_belt_kph_max = true;
        out->tm_reverse_belt_kph_max = (float)v->valuedouble;
    }
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasTmAutoRamp");
    if (cJSON_IsBool(v)) out->has_tm_auto_ramp = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "rpmRampUpStageValue");
    if (cJSON_IsNumber(v)) out->rpm_ramp_up_stage_value = v->valueint;
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "rpmRampDownStageValue");
    if (cJSON_IsNumber(v)) out->rpm_ramp_down_stage_value = v->valueint;
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "lcbCanControlWatts");
    if (cJSON_IsBool(v)) out->lcb_can_control_watts = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "wattsInaccurate");
    if (cJSON_IsBool(v)) out->watts_inaccurate = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasCurrentOc");
    if (cJSON_IsBool(v)) out->has_current_oc = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "isHybridTm");
    if (cJSON_IsBool(v)) out->is_hybrid_tm = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(features_obj, "hasEcbBrake");
    if (cJSON_IsBool(v)) out->has_ecb_brake = cJSON_IsTrue(v);
}

static bool parse_one_spec(const cJSON *frame_obj, frame_config_spec_t *out)
{
    frame_config_spec_init(out);

    const cJSON *model_name = cJSON_GetObjectItemCaseSensitive(frame_obj, "modelName");
    const cJSON *category = cJSON_GetObjectItemCaseSensitive(frame_obj, "category");
    /* modelName / category have no schema default - required, and treated
       as a hard failure if missing (see frame_config_json_loader.h). */
    if (!cJSON_IsString(model_name) || model_name->valuestring == NULL) return false;
    if (!cJSON_IsString(category) || category->valuestring == NULL) return false;

    copy_string(out->model_name, sizeof(out->model_name), model_name->valuestring);
    out->category = machine_category_from_string(category->valuestring);

    const cJSON *is_lifestyle = cJSON_GetObjectItemCaseSensitive(frame_obj, "isLifestyle");
    if (cJSON_IsBool(is_lifestyle)) out->is_lifestyle = cJSON_IsTrue(is_lifestyle);

    const cJSON *frame_prefixes = cJSON_GetObjectItemCaseSensitive(frame_obj, "framePrefixes");
    if (cJSON_IsArray(frame_prefixes)) {
        const cJSON *prefix = NULL;
        cJSON_ArrayForEach(prefix, frame_prefixes) {
            if (out->frame_prefix_count >= FC_MAX_FRAME_PREFIXES) break;
            if (!cJSON_IsString(prefix) || prefix->valuestring == NULL) continue;
            copy_string(out->frame_prefixes[out->frame_prefix_count], FC_FRAME_PREFIX_LEN, prefix->valuestring);
            out->frame_prefix_count++;
        }
    }

    const cJSON *flavors = cJSON_GetObjectItemCaseSensitive(frame_obj, "supportProductFlavors");
    if (cJSON_IsArray(flavors)) {
        const cJSON *flavor = NULL;
        cJSON_ArrayForEach(flavor, flavors) {
            if (out->support_product_flavor_count >= FC_MAX_PRODUCT_FLAVORS) break;
            if (!cJSON_IsString(flavor) || flavor->valuestring == NULL) continue;
            copy_string(out->support_product_flavors[out->support_product_flavor_count], FC_PRODUCT_FLAVOR_LEN, flavor->valuestring);
            out->support_product_flavor_count++;
        }
    }

    const cJSON *speed_kph = cJSON_GetObjectItemCaseSensitive(frame_obj, "speedKph");
    if (cJSON_IsObject(speed_kph)) {
        out->has_speed_kph = true;
        parse_value_range_float(speed_kph, &out->speed_kph);
    }
    const cJSON *incline_percent = cJSON_GetObjectItemCaseSensitive(frame_obj, "inclinePercent");
    if (cJSON_IsObject(incline_percent)) {
        out->has_incline_percent = true;
        parse_value_range_float(incline_percent, &out->incline_percent);
    }
    const cJSON *incline_adc = cJSON_GetObjectItemCaseSensitive(frame_obj, "inclineAdc");
    if (cJSON_IsObject(incline_adc)) {
        out->has_incline_adc = true;
        parse_value_range_int(incline_adc, &out->incline_adc);
    }
    const cJSON *resistance_level = cJSON_GetObjectItemCaseSensitive(frame_obj, "resistanceLevel");
    if (cJSON_IsObject(resistance_level)) {
        out->has_resistance_level = true;
        parse_value_range_int(resistance_level, &out->resistance_level);
    }
    const cJSON *watts = cJSON_GetObjectItemCaseSensitive(frame_obj, "watts");
    if (cJSON_IsObject(watts)) {
        out->has_watts = true;
        parse_value_range_int(watts, &out->watts);
    }
    const cJSON *rpm = cJSON_GetObjectItemCaseSensitive(frame_obj, "rpm");
    if (cJSON_IsObject(rpm)) {
        out->has_rpm = true;
        parse_value_range_int(rpm, &out->rpm);
    } else {
        /* Treadmill frames carry their RPM range under "speedRpm" */
        const cJSON *speed_rpm = cJSON_GetObjectItemCaseSensitive(frame_obj, "speedRpm");
        if (cJSON_IsObject(speed_rpm)) {
            out->has_rpm = true;
            parse_value_range_int(speed_rpm, &out->rpm);
        }
    }
    const cJSON *speed_mph = cJSON_GetObjectItemCaseSensitive(frame_obj, "speedMph");
    if (cJSON_IsObject(speed_mph)) {
        out->has_speed_mph = true;
        parse_value_range_float(speed_mph, &out->speed_mph);
    }
    const cJSON *weight_lbs = cJSON_GetObjectItemCaseSensitive(frame_obj, "weightLbs");
    if (cJSON_IsObject(weight_lbs)) {
        out->has_weight_lbs = true;
        parse_value_range_int(weight_lbs, &out->weight_lbs);
    }
    const cJSON *weight_kg = cJSON_GetObjectItemCaseSensitive(frame_obj, "weightKg");
    if (cJSON_IsObject(weight_kg)) {
        out->has_weight_kg = true;
        parse_value_range_int(weight_kg, &out->weight_kg);
    }
    const cJSON *motor_type = cJSON_GetObjectItemCaseSensitive(frame_obj, "motorType");
    if (cJSON_IsNumber(motor_type)) out->motor_type = motor_type->valueint;

    const cJSON *rate_incline = cJSON_GetObjectItemCaseSensitive(frame_obj, "rateInclinePercentPerSec");
    if (cJSON_IsNumber(rate_incline)) out->rate_incline_percent_per_sec = (float)rate_incline->valuedouble;
    const cJSON *rate_kph = cJSON_GetObjectItemCaseSensitive(frame_obj, "rateKphPerSec");
    if (cJSON_IsNumber(rate_kph)) out->rate_kph_per_sec = (float)rate_kph->valuedouble;

    const cJSON *rate_resistance = cJSON_GetObjectItemCaseSensitive(frame_obj, "rateResistancePerSec");
    if (cJSON_IsNumber(rate_resistance)) {
        out->has_rate_resistance_per_sec = true;
        out->rate_resistance_per_sec = (float)rate_resistance->valuedouble;
    }

    const cJSON *key_map = cJSON_GetObjectItemCaseSensitive(frame_obj, "keyMap");
    if (cJSON_IsString(key_map) && key_map->valuestring != NULL) {
        out->has_key_map = true;
        copy_string(out->key_map, sizeof(out->key_map), key_map->valuestring);
    }
    const cJSON *power_rule = cJSON_GetObjectItemCaseSensitive(frame_obj, "powerRule");
    if (cJSON_IsString(power_rule) && power_rule->valuestring != NULL) {
        out->has_power_rule = true;
        copy_string(out->power_rule, sizeof(out->power_rule), power_rule->valuestring);
    }

    const cJSON *incline_adc_offset_min = cJSON_GetObjectItemCaseSensitive(frame_obj, "inclineAdcOffsetMin");
    if (cJSON_IsNumber(incline_adc_offset_min)) {
        out->has_incline_adc_offset_min = true;
        out->incline_adc_offset_min = incline_adc_offset_min->valueint;
    }
    const cJSON *incline_adc_offset_max = cJSON_GetObjectItemCaseSensitive(frame_obj, "inclineAdcOffsetMax");
    if (cJSON_IsNumber(incline_adc_offset_max)) {
        out->has_incline_adc_offset_max = true;
        out->incline_adc_offset_max = incline_adc_offset_max->valueint;
    }

    const cJSON *dc_bus_upper = cJSON_GetObjectItemCaseSensitive(frame_obj, "dcBusUpperLimit");
    if (cJSON_IsNumber(dc_bus_upper)) out->dc_bus_upper_limit = dc_bus_upper->valueint;
    const cJSON *dc_bus_lower = cJSON_GetObjectItemCaseSensitive(frame_obj, "dcBusLowerLimit");
    if (cJSON_IsNumber(dc_bus_lower)) out->dc_bus_lower_limit = dc_bus_lower->valueint;

    const cJSON *climbmill_params = cJSON_GetObjectItemCaseSensitive(frame_obj, "climbmillParams");
    if (cJSON_IsObject(climbmill_params)) {
        out->has_climbmill_params = true;
        const cJSON *steps_per_floor = cJSON_GetObjectItemCaseSensitive(climbmill_params, "stepsPerFloor");
        if (cJSON_IsNumber(steps_per_floor)) out->climbmill_params.steps_per_floor = steps_per_floor->valueint;
        const cJSON *steps_per_meter = cJSON_GetObjectItemCaseSensitive(climbmill_params, "stepsPerMeter");
        if (cJSON_IsNumber(steps_per_meter)) out->climbmill_params.steps_per_meter = (float)steps_per_meter->valuedouble;
    }

    const cJSON *supported_lcb_types = cJSON_GetObjectItemCaseSensitive(frame_obj, "supportedLcbTypes");
    if (cJSON_IsArray(supported_lcb_types)) {
        const cJSON *lcb_type = NULL;
        cJSON_ArrayForEach(lcb_type, supported_lcb_types) {
            if (out->supported_lcb_type_count >= FC_MAX_LCB_TYPES) break;
            if (!cJSON_IsString(lcb_type) || lcb_type->valuestring == NULL) continue;
            copy_string(out->supported_lcb_types[out->supported_lcb_type_count], FC_LCB_TYPE_STR_LEN, lcb_type->valuestring);
            out->supported_lcb_type_count++;
        }
    }

    const cJSON *features = cJSON_GetObjectItemCaseSensitive(frame_obj, "features");
    if (cJSON_IsObject(features)) {
        parse_features(features, &out->features);
    }

    return true;
}

int frame_config_json_loader_load_from_string(const char *json, frame_config_spec_t *out_specs, int max_specs)
{
    if (json == NULL || out_specs == NULL || max_specs <= 0) return -1;

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) return -1;

    const cJSON *frames = cJSON_GetObjectItemCaseSensitive(root, "frames");
    if (!cJSON_IsArray(frames)) {
        /* Support a standalone single-FrameConfig file: one bare object
           (not wrapped in a "frames" array), as produced by
           CREAT_CONSOLE_FRAME_CONFIG_JSON_SCHEMA for a single model. */
        const cJSON *model_name = cJSON_GetObjectItemCaseSensitive(root, "modelName");
        if (cJSON_IsObject(root) && cJSON_IsString(model_name) && model_name->valuestring != NULL) {
            int result = parse_one_spec(root, &out_specs[0]) ? 1 : -2;
            cJSON_Delete(root);
            return result;
        }
        cJSON_Delete(root);
        return -1;
    }

    int count = 0;
    const cJSON *frame_obj = NULL;
    cJSON_ArrayForEach(frame_obj, frames) {
        if (count >= max_specs) break;
        if (!cJSON_IsObject(frame_obj)) continue;
        if (!parse_one_spec(frame_obj, &out_specs[count])) {
            cJSON_Delete(root);
            return -2;
        }
        count++;
    }

    cJSON_Delete(root);
    return count;
}

void frame_config_json_loader_to_pair(const frame_config_spec_t *spec,
                                       machine_config_t *out_machine_config,
                                       frame_config_data_t *out_frame_config)
{
    if (spec == NULL || out_machine_config == NULL || out_frame_config == NULL) return;

    machine_config_init(out_machine_config);
    out_machine_config->category = spec->category;
    out_machine_config->has_speed_control = spec->has_speed_kph;
    out_machine_config->has_incline_control = spec->has_incline_percent;
    out_machine_config->has_resistance_control = spec->has_resistance_level;
    out_machine_config->has_watts_control = spec->has_watts;
    out_machine_config->has_emergency_stop = spec->features.has_emergency_stop;
    out_machine_config->has_cadence = spec->features.has_cadence;
    out_machine_config->has_can_bus = spec->features.has_can;

    frame_config_data_init(out_frame_config);
    copy_string(out_frame_config->model_name, sizeof(out_frame_config->model_name), spec->model_name);
    out_frame_config->frame_id = 0;
    out_frame_config->category = spec->category;
    out_frame_config->is_lifestyle = spec->is_lifestyle;

    out_frame_config->frame_prefix_count = spec->frame_prefix_count;
    for (int i = 0; i < spec->frame_prefix_count; i++) {
        copy_string(out_frame_config->frame_prefixes[i], FC_FRAME_PREFIX_LEN, spec->frame_prefixes[i]);
    }

    out_frame_config->support_product_flavor_count = spec->support_product_flavor_count;
    for (int i = 0; i < spec->support_product_flavor_count; i++) {
        copy_string(out_frame_config->support_product_flavors[i], FC_PRODUCT_FLAVOR_LEN, spec->support_product_flavors[i]);
    }

    /* All six ranges carry straight through - unlike the old min/max/
       defaultValue/stepSize schema, MetricRange (the int ranges) simply has
       no increment field to drop, so there is no special-casing needed
       here anymore. */
    out_frame_config->has_speed_kph_range = spec->has_speed_kph;
    out_frame_config->speed_kph_range = spec->speed_kph;
    out_frame_config->has_incline_percent_range = spec->has_incline_percent;
    out_frame_config->incline_percent_range = spec->incline_percent;
    out_frame_config->has_incline_adc_range = spec->has_incline_adc;
    out_frame_config->incline_adc_range = spec->incline_adc;
    out_frame_config->has_resistance_level_range = spec->has_resistance_level;
    out_frame_config->resistance_level_range = spec->resistance_level;
    out_frame_config->has_watts_range = spec->has_watts;
    out_frame_config->watts_range = spec->watts;
    out_frame_config->has_rpm_range = spec->has_rpm;
    out_frame_config->rpm_range = spec->rpm;
    out_frame_config->has_speed_mph_range = spec->has_speed_mph;
    out_frame_config->speed_mph_range = spec->speed_mph;
    out_frame_config->has_weight_lbs_range = spec->has_weight_lbs;
    out_frame_config->weight_lbs_range = spec->weight_lbs;
    out_frame_config->has_weight_kg_range = spec->has_weight_kg;
    out_frame_config->weight_kg_range = spec->weight_kg;
    out_frame_config->motor_type = spec->motor_type;

    out_frame_config->rate_incline_percent_per_sec = spec->rate_incline_percent_per_sec;
    out_frame_config->rate_kph_per_sec = spec->rate_kph_per_sec;

    out_frame_config->has_rate_resistance_per_sec = spec->has_rate_resistance_per_sec;
    out_frame_config->rate_resistance_per_sec = spec->rate_resistance_per_sec;

    out_frame_config->has_key_map = spec->has_key_map;
    copy_string(out_frame_config->key_map, sizeof(out_frame_config->key_map), spec->key_map);
    out_frame_config->has_power_rule = spec->has_power_rule;
    copy_string(out_frame_config->power_rule, sizeof(out_frame_config->power_rule), spec->power_rule);

    out_frame_config->has_incline_adc_offset_min = spec->has_incline_adc_offset_min;
    out_frame_config->incline_adc_offset_min = spec->incline_adc_offset_min;
    out_frame_config->has_incline_adc_offset_max = spec->has_incline_adc_offset_max;
    out_frame_config->incline_adc_offset_max = spec->incline_adc_offset_max;

    out_frame_config->dc_bus_upper_limit = spec->dc_bus_upper_limit;
    out_frame_config->dc_bus_lower_limit = spec->dc_bus_lower_limit;

    if (spec->has_climbmill_params) {
        out_frame_config->steps_per_floor = spec->climbmill_params.steps_per_floor;
        out_frame_config->steps_per_meter = spec->climbmill_params.steps_per_meter;
    } else {
        out_frame_config->steps_per_floor = 16;
        out_frame_config->steps_per_meter = 4.37f;
    }

    out_frame_config->supported_lcb_type_count = spec->supported_lcb_type_count;
    for (int i = 0; i < spec->supported_lcb_type_count; i++) {
        copy_string(out_frame_config->supported_lcb_types[i], FC_LCB_TYPE_STR_LEN, spec->supported_lcb_types[i]);
    }

    out_frame_config->has_emergency_stop = spec->features.has_emergency_stop;
    out_frame_config->uses_reverse_incline = spec->features.uses_reverse_incline;
    out_frame_config->supports_calibration = spec->features.supports_calibration;
    out_frame_config->requires_belt_lubrication = spec->features.requires_belt_lubrication;
    out_frame_config->requires_chain_lubrication = spec->features.requires_chain_lubrication;
    out_frame_config->has_cadence = spec->features.has_cadence;
    out_frame_config->has_can = spec->features.has_can;
    out_frame_config->use_can_heart_rate = spec->features.use_can_heart_rate;
    out_frame_config->has_ir_sensor = spec->features.has_ir_sensor;
    out_frame_config->has_cm_control_zone = spec->features.has_cm_control_zone;
    out_frame_config->bump_sensor_trigger_threshold = spec->features.bump_sensor_trigger_threshold;
    out_frame_config->has_tm_lock_state = spec->features.has_tm_lock_state;
    out_frame_config->has_tm_over_speed = spec->features.has_tm_over_speed;
    out_frame_config->tm_supports_reverse_belt = spec->features.tm_supports_reverse_belt;
    out_frame_config->has_tm_reverse_belt_kph_max = spec->features.has_tm_reverse_belt_kph_max;
    out_frame_config->tm_reverse_belt_kph_max = spec->features.tm_reverse_belt_kph_max;
    out_frame_config->has_tm_auto_ramp = spec->features.has_tm_auto_ramp;
    out_frame_config->rpm_ramp_up_stage_value = spec->features.rpm_ramp_up_stage_value;
    out_frame_config->rpm_ramp_down_stage_value = spec->features.rpm_ramp_down_stage_value;
    out_frame_config->lcb_can_control_watts = spec->features.lcb_can_control_watts;
    out_frame_config->watts_inaccurate = spec->features.watts_inaccurate;
    out_frame_config->has_current_oc = spec->features.has_current_oc;
    out_frame_config->is_hybrid_tm = spec->features.is_hybrid_tm;
    out_frame_config->has_ecb_brake = spec->features.has_ecb_brake;
}

bool frame_config_json_loader_load_metadata(const char *json, frame_config_metadata_t *out)
{
    if (out != NULL) memset(out, 0, sizeof(*out));
    if (json == NULL || out == NULL) return false;

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) return false;

    const cJSON *metadata = cJSON_GetObjectItemCaseSensitive(root, "metadata");
    if (!cJSON_IsObject(metadata)) {
        cJSON_Delete(root);
        return false;
    }

    const cJSON *v;
    v = cJSON_GetObjectItemCaseSensitive(metadata, "title");
    if (cJSON_IsString(v) && v->valuestring != NULL) copy_string(out->title, sizeof(out->title), v->valuestring);
    v = cJSON_GetObjectItemCaseSensitive(metadata, "organization");
    if (cJSON_IsString(v) && v->valuestring != NULL) copy_string(out->organization, sizeof(out->organization), v->valuestring);
    v = cJSON_GetObjectItemCaseSensitive(metadata, "protocolVersion");
    if (cJSON_IsString(v) && v->valuestring != NULL) copy_string(out->protocol_version, sizeof(out->protocol_version), v->valuestring);
    v = cJSON_GetObjectItemCaseSensitive(metadata, "description");
    if (cJSON_IsString(v) && v->valuestring != NULL) copy_string(out->description, sizeof(out->description), v->valuestring);
    v = cJSON_GetObjectItemCaseSensitive(metadata, "lastUpdated");
    if (cJSON_IsString(v) && v->valuestring != NULL) copy_string(out->last_updated, sizeof(out->last_updated), v->valuestring);

    cJSON_Delete(root);
    return true;
}

static void parse_one_category_preset(const char *category_name, const cJSON *preset_obj, category_preset_t *out)
{
    category_preset_init(out);
    out->category = machine_category_from_string(category_name);

    const cJSON *default_speed_kph = cJSON_GetObjectItemCaseSensitive(preset_obj, "defaultSpeedKph");
    if (cJSON_IsObject(default_speed_kph)) {
        out->has_default_speed_kph = true;
        parse_value_range_float(default_speed_kph, &out->default_speed_kph);
    }
    const cJSON *default_incline_percent = cJSON_GetObjectItemCaseSensitive(preset_obj, "defaultInclinePercent");
    if (cJSON_IsObject(default_incline_percent)) {
        out->has_default_incline_percent = true;
        parse_value_range_float(default_incline_percent, &out->default_incline_percent);
    }
    const cJSON *default_incline_adc = cJSON_GetObjectItemCaseSensitive(preset_obj, "defaultInclineAdc");
    if (cJSON_IsObject(default_incline_adc)) {
        out->has_default_incline_adc = true;
        parse_value_range_int(default_incline_adc, &out->default_incline_adc);
    }
    const cJSON *default_resistance_level = cJSON_GetObjectItemCaseSensitive(preset_obj, "defaultResistanceLevel");
    if (cJSON_IsObject(default_resistance_level)) {
        out->has_default_resistance_level = true;
        parse_value_range_int(default_resistance_level, &out->default_resistance_level);
    }
    const cJSON *default_watts = cJSON_GetObjectItemCaseSensitive(preset_obj, "defaultWatts");
    if (cJSON_IsObject(default_watts)) {
        out->has_default_watts = true;
        parse_value_range_int(default_watts, &out->default_watts);
    }
    const cJSON *default_rpm = cJSON_GetObjectItemCaseSensitive(preset_obj, "defaultRpm");
    if (cJSON_IsObject(default_rpm)) {
        out->has_default_rpm = true;
        parse_value_range_int(default_rpm, &out->default_rpm);
    }

    const cJSON *v;
    v = cJSON_GetObjectItemCaseSensitive(preset_obj, "hasEmergencyStop");
    if (cJSON_IsBool(v)) out->has_emergency_stop = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(preset_obj, "usesReverseIncline");
    if (cJSON_IsBool(v)) out->uses_reverse_incline = cJSON_IsTrue(v);
    v = cJSON_GetObjectItemCaseSensitive(preset_obj, "supportsCalibration");
    if (cJSON_IsBool(v)) out->supports_calibration = cJSON_IsTrue(v);
}

int frame_config_json_loader_load_supported_categories(const char *json, machine_category_t *out_categories, int max_categories)
{
    if (json == NULL || out_categories == NULL || max_categories <= 0) return -1;

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) return -1;

    const cJSON *supported_categories = cJSON_GetObjectItemCaseSensitive(root, "supportedCategories");
    if (!cJSON_IsArray(supported_categories)) {
        cJSON_Delete(root);
        return 0;
    }

    int count = 0;
    const cJSON *category = NULL;
    cJSON_ArrayForEach(category, supported_categories) {
        if (count >= max_categories) break;
        if (!cJSON_IsString(category) || category->valuestring == NULL) continue;
        out_categories[count] = machine_category_from_string(category->valuestring);
        count++;
    }

    cJSON_Delete(root);
    return count;
}

int frame_config_json_loader_load_category_presets(const char *json, category_preset_t *out_presets, int max_presets)
{
    if (json == NULL || out_presets == NULL || max_presets <= 0) return -1;

    cJSON *root = cJSON_Parse(json);
    if (root == NULL) return -1;

    const cJSON *category_presets = cJSON_GetObjectItemCaseSensitive(root, "categoryPresets");
    if (!cJSON_IsObject(category_presets)) {
        cJSON_Delete(root);
        return 0;
    }

    int count = 0;
    const cJSON *preset_obj = NULL;
    cJSON_ArrayForEach(preset_obj, category_presets) {
        if (count >= max_presets) break;
        if (preset_obj->string == NULL || !cJSON_IsObject(preset_obj)) continue;
        parse_one_category_preset(preset_obj->string, preset_obj, &out_presets[count]);
        count++;
    }

    cJSON_Delete(root);
    return count;
}
