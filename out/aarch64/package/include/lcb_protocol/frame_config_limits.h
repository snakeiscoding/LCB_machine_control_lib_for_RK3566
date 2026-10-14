/**
 * @file frame_config_limits.h
 * @brief Fixed-size buffer limits shared by the JSON-driven capabilities
 * structs (frame_config_spec_t, frame_config_data_t). C has no dynamic
 * lists, so framePrefixes/supportedLcbTypes become fixed arrays + a count,
 * matching the style of the existing frame_config.h
 * (FRAME_CONFIG_MAX_SUPPORTED_LCB_TYPES).
 *
 * Sizes were derived from the actual data in frame_configs.json (87 entries):
 * max framePrefixes per entry = 3 (longest prefix string = 6 chars), max
 * supportedLcbTypes per entry = 6 (longest = 4 chars, e.g. "0xB6"), longest
 * modelName = 32 chars. Limits below give headroom above the observed
 * maximums.
 */

#ifndef FRAME_CONFIG_LIMITS_H
#define FRAME_CONFIG_LIMITS_H

#define FC_MODEL_NAME_LEN        64
#define FC_MAX_FRAME_PREFIXES    8
#define FC_FRAME_PREFIX_LEN      16
#define FC_MAX_LCB_TYPES         12
#define FC_LCB_TYPE_STR_LEN      8

/* Registry capacity: frame_configs.json has 87 entries as of this port. */
#define FC_REGISTRY_MAX_ENTRIES  128

/* Added for the frame_configs_json_schema.md-aligned schema (metadata /
 * supportProductFlavors / keyMap / powerRule / categoryPresets). */
#define FC_UNIT_LEN              16
#define FC_KEY_MAP_LEN           32
#define FC_POWER_RULE_LEN        64
#define FC_MAX_PRODUCT_FLAVORS   8
#define FC_PRODUCT_FLAVOR_LEN    16
/* One preset per machine_category_t value (8 enumerators, only 6 used by
 * frame_configs.json's categoryPresets today). */
#define FC_MAX_CATEGORY_PRESETS  8
#define FC_METADATA_STR_LEN      128
#define FC_METADATA_DESC_LEN     256

#endif /* FRAME_CONFIG_LIMITS_H */
