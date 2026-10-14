/**
 * @file frame_config_json_loader.h
 * @brief Parses frame_configs.json content (aligned to
 * frame_configs_json_schema.md) into frame_config_spec_t entries, converts
 * a spec into the runtime (machine_config_t, frame_config_data_t) pair, and
 * parses the root-level `metadata` / `categoryPresets` objects — Layer_1
 * "capabilities".
 *
 * Unknown/extra JSON keys are tolerated everywhere (forward-compatible
 * parsing), and every field with a documented schema default falls back to
 * that default when absent (see frame_config_spec_init() /
 * category_preset_init()). Only a genuinely malformed JSON document, a
 * missing/non-array "frames" key, or a frame entry missing modelName or
 * category (the only two fields with no sensible default) is treated as a
 * failure.
 */

#ifndef FRAME_CONFIG_JSON_LOADER_H
#define FRAME_CONFIG_JSON_LOADER_H

#include "frame_config_spec.h"
#include "machine_config.h"
#include "machine_category.h"
#include "frame_config_data.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parses @p json (the full frame_configs.json content, i.e. the
 * `{"version":..., "totalCount":..., "metadata":..., "supportedCategories":
 * ..., "categoryPresets":..., "frames":[...]}` envelope) into @p out_specs
 * (capacity @p max_specs).
 *
 * @return the number of specs written (>= 0) on success, or a negative
 * value on failure: -1 if @p json is not valid JSON or has no "frames"
 * array, -2 if a frame entry is missing modelName or category.
 * Truncates silently (does not fail) if the JSON has more than
 * @p max_specs frames.
 */
int frame_config_json_loader_load_from_string(const char *json, frame_config_spec_t *out_specs, int max_specs);

/**
 * @brief Derives the capability-flag summary (@p out_machine_config) and
 * the fully-resolved runtime config (@p out_frame_config) from @p spec.
 * No-op if any pointer is NULL.
 */
void frame_config_json_loader_to_pair(const frame_config_spec_t *spec,
                                       machine_config_t *out_machine_config,
                                       frame_config_data_t *out_frame_config);

/**
 * @brief Parses @p json's root-level `metadata` object into @p out.
 * @return true if a `metadata` object was present and parsed; false (with
 * @p out zeroed) if it was absent or @p json is not valid JSON.
 */
bool frame_config_json_loader_load_metadata(const char *json, frame_config_metadata_t *out);

/**
 * @brief Parses @p json's root-level `categoryPresets` object (a dict keyed
 * by category name) into @p out_presets (capacity @p max_presets).
 * @return the number of presets parsed (0 if the key was absent — not an
 * error), or -1 if @p json is not valid JSON.
 */
int frame_config_json_loader_load_category_presets(const char *json, category_preset_t *out_presets, int max_presets);

/**
 * @brief Parses @p json's root-level `supportedCategories` array (of
 * category-name strings, e.g. "TREADMILL") into @p out_categories (capacity
 * @p max_categories).
 * @return the number of categories parsed (0 if the key was absent — not an
 * error), or -1 if @p json is not valid JSON.
 */
int frame_config_json_loader_load_supported_categories(const char *json, machine_category_t *out_categories, int max_categories);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_CONFIG_JSON_LOADER_H */
