/**
 * @file frame_config.c
 * @brief Machine capability / calibration data — Layer_1 "capabilities"
 *
 * Converted from: com/jht/chimera/lib/common/config/frames/FrameConfigRecumbentBike_Performance.java
 * (chimera2 reference project). See frame_config.h for what was deliberately
 * NOT ported and why.
 */

#include "frame_config.h"
#include <string.h>

/* lbs -> kg is a standard, unambiguous conversion (1 lb = 0.45359237 kg) —
 * unlike SpeedConvertUtil/UnitConverter/CalibrationTable, this isn't a
 * project-specific utility with unknown behavior, so it's safe to compute
 * directly rather than omit. */
#define LBS_TO_KG(lbs) ((float)(lbs) * 0.45359237f)

void frame_config_init_recumbent_bike_performance(frame_config_t *cfg)
{
    if (cfg == NULL) return;

    memset(cfg, 0, sizeof(*cfg));

    cfg->frame_prefix = "RB317";
    cfg->frame_id = 103;
    cfg->model = "PERFORMANCE";

    /* Java: supportLcbTypeList = new int[]{0x13}; (LCB_TYPE_LCBA) */
    cfg->support_lcb_types[0] = LCB_TYPE_LCBA;
    cfg->support_lcb_type_count = 1;

    cfg->speed_mph_min_model = 0.0f;

    cfg->resistance_min_model = 1.0f;
    cfg->resistance_max_model = 30.0f;

    cfg->weight_kg_min_model = LBS_TO_KG(50);
    cfg->weight_kg_max_model = LBS_TO_KG(400);
    cfg->weight_lbs_min_model = 50.0f;
    cfg->weight_lbs_max_model = 400.0f;

    cfg->rate_resistance_per_sec = 5.0f;
    cfg->lcb_can_control_watts = true;
}

bool frame_config_supports_lcb_type(const frame_config_t *cfg, LcbType_t type)
{
    if (cfg == NULL) return false;
    for (int i = 0; i < cfg->support_lcb_type_count; i++) {
        if (cfg->support_lcb_types[i] == type) return true;
    }
    return false;
}
