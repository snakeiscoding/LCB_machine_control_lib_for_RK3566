/**
 * @file frame_config_registry.h
 * @brief Model-name/serial-number/LCB-type indexed lookup of
 * (machine_config_t, frame_config_data_t) pairs, loaded from
 * frame_configs.json — Layer_1 "capabilities". This is the entry point
 * "control" (and, transitively, "protocol") should use to resolve a
 * machine's capability data.
 *
 * Converted from: com/jht/lcb/core/capabilities/FrameConfigRegistry.kt
 *
 * Kotlin's FrameConfigRegistry is a singleton `object` that loads
 * frame_configs.json from the JVM classpath at class-init time. C has no
 * equivalent resource-loading mechanism, so this module intentionally only
 * exposes explicit load functions (from a JSON string, or from a file path)
 * — wiring an actual runtime source (Android asset, a path handed over via
 * JNI, etc.) is left to a future integration task, matching the scope of
 * this port (see the plan this module was built from).
 *
 * A fixed-size array replaces Kotlin's `mutableMapOf` since C has no
 * built-in dynamic map; FC_REGISTRY_MAX_ENTRIES (128) comfortably exceeds
 * the 87 entries frame_configs.json has as of this port.
 */

#ifndef FRAME_CONFIG_REGISTRY_H
#define FRAME_CONFIG_REGISTRY_H

#include <stdbool.h>
#include <stdint.h>
#include "machine_config.h"
#include "machine_category.h"
#include "frame_config_data.h"
#include "frame_config_spec.h" /* category_preset_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Resets the registry to empty. Call once before the first load.
 */
void frame_config_registry_init(void);

/**
 * @brief Parses @p json and registers every frame it contains. Mirrors
 * Kotlin's fail-silent behavior: a parse failure (see
 * frame_config_json_loader_load_from_string()) leaves any already-registered
 * entries untouched and returns false, rather than aborting the process.
 */
bool frame_config_registry_load_from_json_string(const char *json);

/* Note: as of the frame_configs_json_schema.md alignment, a successful load
   also parses the root-level `categoryPresets` object (if present) and
   uses it (a) to fill in any range a frame omits, per that category, and
   (b) as the source for the "unknown model" fallback in
   frame_config_registry_get_pair_for_model() below, in place of the
   previously hardcoded literal (which remains only as a last-resort
   default if no matching preset was loaded). See
   frame_config_registry_get_category_preset(). */

/**
 * @brief Reads @p file_path fully into memory and calls
 * frame_config_registry_load_from_json_string() on its content. Returns
 * false if the file cannot be opened/read or if parsing fails.
 */
bool frame_config_registry_load_from_file(const char *file_path);

/**
 * @brief Registers (or overwrites, if @p model_name already exists) one
 * entry. The lookup key is @p model_name uppercased and trimmed, matching
 * Kotlin FrameConfigRegistry.register(). No-op if the registry is full or
 * any pointer is NULL.
 */
void frame_config_registry_register(const char *model_name,
                                     const machine_config_t *machine_config,
                                     const frame_config_data_t *frame_config);

/**
 * @brief Port of FrameConfigRegistry.getPairForModel(String): looks up
 * @p model_name (uppercased/trimmed). Never fails — if not found, fills
 * @p out_mc / @p out_fc with the same hardcoded default-treadmill fallback
 * Kotlin uses (getDefaultTreadmill). No-op if any pointer is NULL.
 */
void frame_config_registry_get_pair_for_model(const char *model_name,
                                               machine_config_t *out_mc,
                                               frame_config_data_t *out_fc);

/**
 * @brief Port of FrameConfigRegistry.getPairForSerialNumber(String): linear
 * scan for an entry whose frame_prefixes prefix-matches @p serial_number
 * (case-insensitive). Returns false (out params left untouched) if
 * @p serial_number is NULL/blank or no entry matches.
 */
bool frame_config_registry_get_pair_for_serial_number(const char *serial_number,
                                                       machine_config_t *out_mc,
                                                       frame_config_data_t *out_fc);

/**
 * @brief Port of FrameConfigRegistry.getPairForLcbType(String): linear scan
 * for an entry whose supported_lcb_types contains @p lcb_type_hex
 * (trimmed/uppercased exact match). Returns false (out params left
 * untouched) if @p lcb_type_hex is NULL/blank or no entry matches.
 */
bool frame_config_registry_get_pair_for_lcb_type(const char *lcb_type_hex,
                                                  machine_config_t *out_mc,
                                                  frame_config_data_t *out_fc);

/**
 * @brief Port of FrameConfigRegistry.getRegisteredCount().
 */
int frame_config_registry_get_registered_count(void);

/**
 * @brief Looks up the root-level `categoryPresets` entry for @p category
 * (parsed during the most recent successful load). Returns false (out
 * param untouched) if no preset for that category was loaded.
 */
bool frame_config_registry_get_category_preset(machine_category_t category, category_preset_t *out);

/**
 * @brief Bitmask of machine_category_t values (bit N = (1u << N)) supported
 * by the most recently loaded JSON (see frame_config_registry_load_from_file()
 * / frame_config_registry_load_from_json_string()). Prefers the document's
 * explicit root-level `supportedCategories` array; if that key is absent,
 * falls back to the union of every `frames[].category` the load actually
 * registered (so a standalone single-FrameConfig file, which has no
 * `supportedCategories` key, still reports its one category). Returns 0
 * (no bits set) before any JSON has been loaded this way — seeding the
 * registry via frame_config_registry_register() directly does not affect
 * this value.
 */
uint32_t frame_config_registry_get_supported_categories_bitmask(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAME_CONFIG_REGISTRY_H */
