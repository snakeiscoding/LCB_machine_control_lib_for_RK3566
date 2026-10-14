/**
 * @file frame_config.h
 * @brief Machine capability / calibration data — Layer_1 "capabilities"
 *
 * Converted from (narrowed scope — see the three omissions noted below):
 *   com/jht/chimera/lib/common/config/frames/FrameConfigRecumbentBike_Performance.java
 *   (chimera2 reference project; NOT the local project's FrameConfig, which is
 *   an explicitly-labeled placeholder stub with no real calibration data)
 *
 * This is plain data — no opaque pointer, no lifecycle API, no state machine —
 * matching the CREAT_MACHINE_HANDLER skill's guidance for this layer.
 *
 * Deliberately NOT ported (do not fabricate these — no consumer needs them
 * yet, and guessing would be worse than omitting):
 *   - speed_mph max model: Java computes it via
 *     `SpeedConvertUtil.bikeWattsToMPH(500)`, a conversion utility not present
 *     in this codebase.
 *   - speed_kph min/max: Java computes them via
 *     `UnitConverter.ConvertMphToKph(...)`, also not present here.
 *   - RESISTANCE_TABLE / WATTS_TABLE: Java references
 *     `CalibrationTable.ChopperBike.{RESISTANCE_TABLE,WATTS_TABLE}`, a
 *     calibration table not available in this codebase.
 * When any of these are actually needed (e.g. a future watt<->resistance
 * conversion feature), port the real utility/table first — don't invent
 * numbers to fill this struct.
 *
 * NOTE: as of this port, nothing in bike_handler.c/bike_interface consumes
 * this data yet (confirmed: neither the local BikeHandler.java/BikeInterface.java
 * nor the real chimera2 BikeInterface.java read FrameConfig at all). This
 * module is groundwork only.
 */

#ifndef FRAME_CONFIG_H
#define FRAME_CONFIG_H

#include <stdbool.h>
#include "lcb_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FRAME_CONFIG_MAX_SUPPORTED_LCB_TYPES 8

typedef struct {
    const char *frame_prefix;   /* Java: FRAME_PREFIX */
    int         frame_id;       /* Java: FRAME_ID */
    const char *model;          /* Java: MODEL (an Android string-resource id there; stored here as a name) */

    LcbType_t support_lcb_types[FRAME_CONFIG_MAX_SUPPORTED_LCB_TYPES]; /* Java: supportLcbTypeList */
    int       support_lcb_type_count;

    /* Ranges — only the literal *model* values the Java constructor actually
     * sets directly (see file header for what's deliberately omitted) */
    float speed_mph_min_model;      /* Java: speedMph.setMinModel(0) */
    float resistance_min_model;     /* Java: resistance.setMinModel(1) */
    float resistance_max_model;     /* Java: resistance.setMaxModel(30) */
    float weight_kg_min_model;      /* Java: weightKg.setMinModel(WeightConvertUtil.convertLbsToKg(50)) */
    float weight_kg_max_model;      /* Java: weightKg.setMaxModel(WeightConvertUtil.convertLbsToKg(400)) */
    float weight_lbs_min_model;     /* Java: weightLbs.setMinModel(50) */
    float weight_lbs_max_model;     /* Java: weightLbs.setMaxModel(400) */

    float rate_resistance_per_sec;  /* Java: RATE_RESISTANCE_PER_SEC = 5f */
    bool  lcb_can_control_watts;    /* Java: LCB_CAN_CONTROL_WATTS = true */
} frame_config_t;

/**
 * @brief Populate @p cfg with the RecumbentBike_Performance ("RB317") frame's
 * capability data. No-op if @p cfg is NULL.
 */
void frame_config_init_recumbent_bike_performance(frame_config_t *cfg);

/**
 * @brief Port of FrameConfig.supportsLcbType(int) — true if @p type is in
 * cfg->support_lcb_types. False (not true) if @p cfg is NULL.
 */
bool frame_config_supports_lcb_type(const frame_config_t *cfg, LcbType_t type);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_CONFIG_H */
