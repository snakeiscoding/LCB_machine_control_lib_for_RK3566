/**
 * @file frame_config_spec.c
 * @brief See frame_config_spec.h
 */

#include "frame_config_spec.h"
#include <string.h>

void frame_config_spec_init(frame_config_spec_t *spec)
{
    if (spec == NULL) return;

    memset(spec, 0, sizeof(*spec));

    spec->category = MC_UNKNOWN;
    spec->is_lifestyle = false;

    spec->rate_incline_percent_per_sec = 0.71f;
    spec->rate_kph_per_sec = 0.8f;
    spec->dc_bus_upper_limit = 72;
    spec->dc_bus_lower_limit = 63;

    spec->climbmill_params.steps_per_floor = 16;
    spec->climbmill_params.steps_per_meter = 4.37f;

    spec->features.bump_sensor_trigger_threshold = -1;
}

void category_preset_init(category_preset_t *preset)
{
    if (preset == NULL) return;

    memset(preset, 0, sizeof(*preset));
    preset->category = MC_UNKNOWN;
}
