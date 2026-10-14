/**
 * @file machine_config.h
 * @brief Lightweight machine capability flags — Layer_1 "capabilities"
 *
 * Converted from: com/jht/lcb/core/capabilities/MachineConfig.kt
 *
 * Deliberately has no has_rpm_control flag even though `rpm` is a JSON/spec
 * field — the Kotlin MachineConfig doesn't carry one either (rpm range only
 * lives in the FrameConfig side). This is an existing asymmetry in the
 * Kotlin source, not a porting omission.
 */

#ifndef MACHINE_CONFIG_H
#define MACHINE_CONFIG_H

#include <stdbool.h>
#include "machine_category.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    machine_category_t category;
    bool has_speed_control;
    bool has_incline_control;
    bool has_resistance_control;
    bool has_watts_control;
    bool has_emergency_stop;
    bool has_cadence;
    bool has_can_bus;
} machine_config_t;

/**
 * @brief Zero-initializes @p cfg (category = MC_UNKNOWN, all flags false).
 * No-op if @p cfg is NULL.
 */
void machine_config_init(machine_config_t *cfg);

#ifdef __cplusplus
}
#endif

#endif /* MACHINE_CONFIG_H */
