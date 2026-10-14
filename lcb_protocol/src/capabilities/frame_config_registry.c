/**
 * @file frame_config_registry.c
 * @brief See frame_config_registry.h
 */

#include "frame_config_registry.h"
#include "frame_config_json_loader.h"
#include "frame_config_limits.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    char                 model_name_key[FC_MODEL_NAME_LEN]; /* uppercased, trimmed */
    machine_config_t     machine_config;
    frame_config_data_t  frame_config;
} registry_entry_t;

static registry_entry_t g_entries[FC_REGISTRY_MAX_ENTRIES];
static int g_entry_count = 0;

static category_preset_t g_category_presets[FC_MAX_CATEGORY_PRESETS];
static int g_category_preset_count = 0;

static uint32_t g_supported_categories_bitmask = 0;

/* Scratch buffer for parsing - kept off the stack since
   FC_REGISTRY_MAX_ENTRIES frame_config_spec_t entries would be tens of KB. */
static frame_config_spec_t g_parse_scratch[FC_REGISTRY_MAX_ENTRIES];

static void normalize_key(char *dst, size_t dst_size, const char *src)
{
    if (dst_size == 0) return;
    if (src == NULL) { dst[0] = '\0'; return; }

    /* trim leading whitespace */
    while (*src == ' ' || *src == '\t') src++;

    size_t len = strlen(src);
    while (len > 0 && (src[len - 1] == ' ' || src[len - 1] == '\t')) len--;

    if (len >= dst_size) len = dst_size - 1;
    for (size_t i = 0; i < len; i++) {
        dst[i] = (char)toupper((unsigned char)src[i]);
    }
    dst[len] = '\0';
}

static registry_entry_t *find_entry(const char *key)
{
    for (int i = 0; i < g_entry_count; i++) {
        if (strcmp(g_entries[i].model_name_key, key) == 0) return &g_entries[i];
    }
    return NULL;
}

static const category_preset_t *find_category_preset(machine_category_t category)
{
    for (int i = 0; i < g_category_preset_count; i++) {
        if (g_category_presets[i].category == category) return &g_category_presets[i];
    }
    return NULL;
}

/* Fills in any range a frame omitted, using its category's preset (if one
   was loaded). Per-frame data always wins; this only ever touches a
   has_xxx_range that is still false. */
static void apply_category_preset_fallback(frame_config_data_t *fc)
{
    const category_preset_t *preset = find_category_preset(fc->category);
    if (preset == NULL) return;

    if (!fc->has_speed_kph_range && preset->has_default_speed_kph) {
        fc->has_speed_kph_range = true;
        fc->speed_kph_range = preset->default_speed_kph;
    }
    if (!fc->has_incline_percent_range && preset->has_default_incline_percent) {
        fc->has_incline_percent_range = true;
        fc->incline_percent_range = preset->default_incline_percent;
    }
    if (!fc->has_incline_adc_range && preset->has_default_incline_adc) {
        fc->has_incline_adc_range = true;
        fc->incline_adc_range = preset->default_incline_adc;
    }
    if (!fc->has_resistance_level_range && preset->has_default_resistance_level) {
        fc->has_resistance_level_range = true;
        fc->resistance_level_range = preset->default_resistance_level;
    }
    if (!fc->has_watts_range && preset->has_default_watts) {
        fc->has_watts_range = true;
        fc->watts_range = preset->default_watts;
    }
    if (!fc->has_rpm_range && preset->has_default_rpm) {
        fc->has_rpm_range = true;
        fc->rpm_range = preset->default_rpm;
    }
}

/* Last-resort fallback for an unknown model when no TREADMILL
   categoryPreset was loaded (e.g. the caller only fed frames, no
   categoryPresets block). Mirrors the schema doc's own TREADMILL
   categoryPreset example values. */
static void fill_hardcoded_default_treadmill(const char *model_name, machine_config_t *out_mc, frame_config_data_t *out_fc)
{
    machine_config_init(out_mc);
    out_mc->category = MC_TREADMILL;
    out_mc->has_speed_control = true;
    out_mc->has_incline_control = true;
    out_mc->has_emergency_stop = true;

    frame_config_data_init(out_fc);
    if (model_name != NULL) {
        strncpy(out_fc->model_name, model_name, sizeof(out_fc->model_name) - 1);
        out_fc->model_name[sizeof(out_fc->model_name) - 1] = '\0';
    }
    out_fc->category = MC_TREADMILL;

    out_fc->has_speed_kph_range = true;
    out_fc->speed_kph_range.min_value = 0.8f;
    out_fc->speed_kph_range.max_value = 20.0f;
    out_fc->speed_kph_range.default_value = 0.8f;
    out_fc->speed_kph_range.increment = 0.1f;

    out_fc->has_incline_percent_range = true;
    out_fc->incline_percent_range.min_value = 0.0f;
    out_fc->incline_percent_range.max_value = 15.0f;
    out_fc->incline_percent_range.default_value = 0.0f;
    out_fc->incline_percent_range.increment = 0.5f;

    out_fc->has_incline_adc_range = true;
    out_fc->incline_adc_range.min_value = 4224;
    out_fc->incline_adc_range.max_value = 25600;
    out_fc->incline_adc_range.default_value = 4224;

    out_fc->has_emergency_stop = true;
}

/* Builds the "unknown model" fallback from the loaded TREADMILL
   categoryPreset when available, else falls back to the hardcoded literal
   above. */
static void fill_default_treadmill(const char *model_name, machine_config_t *out_mc, frame_config_data_t *out_fc)
{
    const category_preset_t *preset = find_category_preset(MC_TREADMILL);
    if (preset == NULL) {
        fill_hardcoded_default_treadmill(model_name, out_mc, out_fc);
        return;
    }

    machine_config_init(out_mc);
    out_mc->category = MC_TREADMILL;
    out_mc->has_speed_control = preset->has_default_speed_kph;
    out_mc->has_incline_control = preset->has_default_incline_percent;
    out_mc->has_emergency_stop = preset->has_emergency_stop;

    frame_config_data_init(out_fc);
    if (model_name != NULL) {
        strncpy(out_fc->model_name, model_name, sizeof(out_fc->model_name) - 1);
        out_fc->model_name[sizeof(out_fc->model_name) - 1] = '\0';
    }
    out_fc->category = MC_TREADMILL;
    out_fc->uses_reverse_incline = preset->uses_reverse_incline;
    out_fc->supports_calibration = preset->supports_calibration;
    out_fc->has_emergency_stop = preset->has_emergency_stop;

    if (preset->has_default_speed_kph) {
        out_fc->has_speed_kph_range = true;
        out_fc->speed_kph_range = preset->default_speed_kph;
    }
    if (preset->has_default_incline_percent) {
        out_fc->has_incline_percent_range = true;
        out_fc->incline_percent_range = preset->default_incline_percent;
    }
    if (preset->has_default_incline_adc) {
        out_fc->has_incline_adc_range = true;
        out_fc->incline_adc_range = preset->default_incline_adc;
    }
    if (preset->has_default_resistance_level) {
        out_fc->has_resistance_level_range = true;
        out_fc->resistance_level_range = preset->default_resistance_level;
    }
    if (preset->has_default_watts) {
        out_fc->has_watts_range = true;
        out_fc->watts_range = preset->default_watts;
    }
    if (preset->has_default_rpm) {
        out_fc->has_rpm_range = true;
        out_fc->rpm_range = preset->default_rpm;
    }
}

void frame_config_registry_init(void)
{
    g_entry_count = 0;
    memset(g_entries, 0, sizeof(g_entries));
    g_category_preset_count = 0;
    memset(g_category_presets, 0, sizeof(g_category_presets));
    g_supported_categories_bitmask = 0;
}

bool frame_config_registry_load_from_json_string(const char *json)
{
    if (json == NULL) return false;

    int parsed = frame_config_json_loader_load_from_string(json, g_parse_scratch, FC_REGISTRY_MAX_ENTRIES);
    if (parsed < 0) return false;

    g_category_preset_count = frame_config_json_loader_load_category_presets(json, g_category_presets, FC_MAX_CATEGORY_PRESETS);
    if (g_category_preset_count < 0) g_category_preset_count = 0;

    for (int i = 0; i < parsed; i++) {
        machine_config_t mc;
        frame_config_data_t fc;
        frame_config_json_loader_to_pair(&g_parse_scratch[i], &mc, &fc);
        apply_category_preset_fallback(&fc);
        frame_config_registry_register(g_parse_scratch[i].model_name, &mc, &fc);
    }

    /* Prefer the document's explicit root-level `supportedCategories`; fall
       back to the union of categories actually parsed above, so a
       standalone single-FrameConfig file (no `supportedCategories` key)
       still reports its one category. */
    machine_category_t categories[FC_REGISTRY_MAX_ENTRIES];
    int category_count = frame_config_json_loader_load_supported_categories(json, categories, FC_REGISTRY_MAX_ENTRIES);

    g_supported_categories_bitmask = 0;
    if (category_count > 0) {
        for (int i = 0; i < category_count; i++) {
            g_supported_categories_bitmask |= (1u << (uint32_t)categories[i]);
        }
    } else {
        for (int i = 0; i < parsed; i++) {
            g_supported_categories_bitmask |= (1u << (uint32_t)g_parse_scratch[i].category);
        }
    }

    return true;
}

bool frame_config_registry_load_from_file(const char *file_path)
{
    if (file_path == NULL) return false;

    FILE *f = fopen(file_path, "rb");
    if (f == NULL) return false;

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return false; }
    long size = ftell(f);
    if (size < 0) { fclose(f); return false; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return false; }

    char *buffer = (char *)malloc((size_t)size + 1);
    if (buffer == NULL) { fclose(f); return false; }

    size_t read = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    buffer[read] = '\0';

    bool ok = frame_config_registry_load_from_json_string(buffer);
    free(buffer);
    return ok;
}

void frame_config_registry_register(const char *model_name,
                                     const machine_config_t *machine_config,
                                     const frame_config_data_t *frame_config)
{
    if (model_name == NULL || machine_config == NULL || frame_config == NULL) return;

    char key[FC_MODEL_NAME_LEN];
    normalize_key(key, sizeof(key), model_name);

    registry_entry_t *entry = find_entry(key);
    if (entry == NULL) {
        if (g_entry_count >= FC_REGISTRY_MAX_ENTRIES) return;
        entry = &g_entries[g_entry_count];
        g_entry_count++;
    }

    strncpy(entry->model_name_key, key, sizeof(entry->model_name_key) - 1);
    entry->model_name_key[sizeof(entry->model_name_key) - 1] = '\0';
    entry->machine_config = *machine_config;
    entry->frame_config = *frame_config;
}

void frame_config_registry_get_pair_for_model(const char *model_name,
                                               machine_config_t *out_mc,
                                               frame_config_data_t *out_fc)
{
    if (out_mc == NULL || out_fc == NULL) return;

    char key[FC_MODEL_NAME_LEN];
    normalize_key(key, sizeof(key), model_name);

    registry_entry_t *entry = find_entry(key);
    if (entry != NULL) {
        *out_mc = entry->machine_config;
        *out_fc = entry->frame_config;
        return;
    }

    fill_default_treadmill(model_name, out_mc, out_fc);
}

bool frame_config_registry_get_pair_for_serial_number(const char *serial_number,
                                                       machine_config_t *out_mc,
                                                       frame_config_data_t *out_fc)
{
    if (serial_number == NULL || serial_number[0] == '\0') return false;

    for (int i = 0; i < g_entry_count; i++) {
        if (frame_config_data_matches_serial_number(&g_entries[i].frame_config, serial_number)) {
            if (out_mc != NULL) *out_mc = g_entries[i].machine_config;
            if (out_fc != NULL) *out_fc = g_entries[i].frame_config;
            return true;
        }
    }
    return false;
}

bool frame_config_registry_get_pair_for_lcb_type(const char *lcb_type_hex,
                                                  machine_config_t *out_mc,
                                                  frame_config_data_t *out_fc)
{
    if (lcb_type_hex == NULL || lcb_type_hex[0] == '\0') return false;

    char key[FC_LCB_TYPE_STR_LEN];
    normalize_key(key, sizeof(key), lcb_type_hex);

    for (int i = 0; i < g_entry_count; i++) {
        const frame_config_data_t *fc = &g_entries[i].frame_config;
        for (int j = 0; j < fc->supported_lcb_type_count; j++) {
            char candidate[FC_LCB_TYPE_STR_LEN];
            normalize_key(candidate, sizeof(candidate), fc->supported_lcb_types[j]);
            if (strcmp(candidate, key) == 0) {
                if (out_mc != NULL) *out_mc = g_entries[i].machine_config;
                if (out_fc != NULL) *out_fc = g_entries[i].frame_config;
                return true;
            }
        }
    }
    return false;
}

int frame_config_registry_get_registered_count(void)
{
    return g_entry_count;
}

bool frame_config_registry_get_category_preset(machine_category_t category, category_preset_t *out)
{
    if (out == NULL) return false;

    const category_preset_t *preset = find_category_preset(category);
    if (preset == NULL) return false;

    *out = *preset;
    return true;
}

uint32_t frame_config_registry_get_supported_categories_bitmask(void)
{
    return g_supported_categories_bitmask;
}
