/**
 * @file machine_category.h
 * @brief Machine category enum — Layer_1 "capabilities"
 *
 * Converted from: com/jht/lcb/core/capabilities/MachineConfig.kt (MachineCategory)
 *
 * frame_configs.json only ever uses 6 of these 8 values (TREADMILL, BIKE,
 * ASCENT_TRAINER, ELLIPTICAL, CLIMBMILL, STEPPER); INDOOR_BIKE and HYBRID_TM
 * are part of the Kotlin enum contract but never appear in the data — kept
 * here for schema completeness.
 */

#ifndef MACHINE_CATEGORY_H
#define MACHINE_CATEGORY_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MC_TREADMILL = 0,
    MC_BIKE,
    MC_ASCENT_TRAINER,
    MC_ELLIPTICAL,
    MC_CLIMBMILL,
    MC_STEPPER,
    MC_INDOOR_BIKE,
    MC_HYBRID_TM,
    MC_UNKNOWN
} machine_category_t;

/**
 * @brief Parse a JSON `category` string (e.g. "TREADMILL") into its enum value.
 * Case-sensitive, matches the exact Kotlin enum constant names. Returns
 * MC_UNKNOWN for NULL or an unrecognized string.
 */
machine_category_t machine_category_from_string(const char *str);

/**
 * @brief Inverse of machine_category_from_string(); returns "UNKNOWN" for
 * MC_UNKNOWN or an out-of-range value.
 */
const char *machine_category_to_string(machine_category_t category);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_CATEGORY_H */
