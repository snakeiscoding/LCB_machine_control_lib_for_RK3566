/**
 * @file machine_config.c
 * @brief See machine_config.h
 */

#include "machine_config.h"
#include <string.h>

void machine_config_init(machine_config_t *cfg)
{
    if (cfg == NULL) return;
    memset(cfg, 0, sizeof(*cfg));
    cfg->category = MC_UNKNOWN;
}
