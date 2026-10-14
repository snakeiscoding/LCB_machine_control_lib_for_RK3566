/**
 * @file frame_config_data_bike.c
 * @brief See frame_config_data_bike.h
 */

#include "frame_config_data_bike.h"
#include <string.h>

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

void frame_config_data_init_bike_recumbent_performance(frame_config_data_t *cfg)
{
    if (cfg == NULL) return;

    frame_config_data_init(cfg);

    strncpy(cfg->model_name, "RB317_PERFORMANCE", sizeof(cfg->model_name) - 1);
    cfg->model_name[sizeof(cfg->model_name) - 1] = '\0';
    cfg->category = MC_BIKE;

    /* Java: FRAME_PREFIX = "RB317"; */
    add_frame_prefix(cfg, "RB317");

    /* Java: supportLcbTypeList = new int[]{0x13}; */
    add_lcb_type(cfg, "0x13");

    /* Java: resistance.setMin(setMinModel(1)); resistance.setMax(setMaxModel(30));
     * defaultValue = 1; value_range_int_t has no increment field at all
     * (frame_configs_json_schema.md's MetricRange doesn't define one), so
     * there's nothing to set here even though Java's stepSize=1. */
    cfg->has_resistance_level_range = true;
    cfg->resistance_level_range.min_value = 1;
    cfg->resistance_level_range.max_value = 30;
    cfg->resistance_level_range.default_value = 1;
}
