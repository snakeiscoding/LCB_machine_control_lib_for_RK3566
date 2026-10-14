/**
 * @file frame_config_data.c
 * @brief See frame_config_data.h
 */

#include "frame_config_data.h"
#include <string.h>
#include <ctype.h>

void frame_config_data_init(frame_config_data_t *cfg)
{
    if (cfg == NULL) return;

    memset(cfg, 0, sizeof(*cfg));

    cfg->frame_id = 0;
    cfg->category = MC_TREADMILL;
    cfg->is_lifestyle = false;

    cfg->rate_incline_percent_per_sec = 0.71f;
    cfg->rate_kph_per_sec = 0.8f;
    cfg->dc_bus_upper_limit = 72;
    cfg->dc_bus_lower_limit = 63;
    cfg->steps_per_floor = 16;
    cfg->steps_per_meter = 4.37f;
    cfg->bump_sensor_trigger_threshold = -1;
}

const char *frame_config_data_primary_frame_prefix(const frame_config_data_t *cfg)
{
    if (cfg == NULL || cfg->frame_prefix_count <= 0) return "";
    return cfg->frame_prefixes[0];
}

static int starts_with_ignore_case(const char *str, const char *prefix)
{
    size_t prefix_len = strlen(prefix);
    if (strlen(str) < prefix_len) return 0;
    for (size_t i = 0; i < prefix_len; i++) {
        if (tolower((unsigned char)str[i]) != tolower((unsigned char)prefix[i])) return 0;
    }
    return 1;
}

bool frame_config_data_matches_serial_number(const frame_config_data_t *cfg, const char *serial_number)
{
    if (cfg == NULL || serial_number == NULL) return false;
    if (serial_number[0] == '\0') return false;

    for (int i = 0; i < cfg->frame_prefix_count; i++) {
        if (starts_with_ignore_case(serial_number, cfg->frame_prefixes[i])) return true;
    }
    return false;
}
