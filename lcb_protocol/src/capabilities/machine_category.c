/**
 * @file machine_category.c
 * @brief See machine_category.h
 */

#include "machine_category.h"
#include <string.h>

machine_category_t machine_category_from_string(const char *str)
{
    if (str == NULL) return MC_UNKNOWN;

    if (strcmp(str, "TREADMILL") == 0)      return MC_TREADMILL;
    if (strcmp(str, "BIKE") == 0)           return MC_BIKE;
    if (strcmp(str, "ASCENT_TRAINER") == 0) return MC_ASCENT_TRAINER;
    if (strcmp(str, "ELLIPTICAL") == 0)     return MC_ELLIPTICAL;
    if (strcmp(str, "CLIMBMILL") == 0)      return MC_CLIMBMILL;
    if (strcmp(str, "STEPPER") == 0)        return MC_STEPPER;
    if (strcmp(str, "INDOOR_BIKE") == 0)    return MC_INDOOR_BIKE;
    if (strcmp(str, "HYBRID_TM") == 0)      return MC_HYBRID_TM;

    return MC_UNKNOWN;
}

const char *machine_category_to_string(machine_category_t category)
{
    switch (category) {
        case MC_TREADMILL:      return "TREADMILL";
        case MC_BIKE:            return "BIKE";
        case MC_ASCENT_TRAINER:  return "ASCENT_TRAINER";
        case MC_ELLIPTICAL:      return "ELLIPTICAL";
        case MC_CLIMBMILL:       return "CLIMBMILL";
        case MC_STEPPER:         return "STEPPER";
        case MC_INDOOR_BIKE:     return "INDOOR_BIKE";
        case MC_HYBRID_TM:       return "HYBRID_TM";
        default:                 return "UNKNOWN";
    }
}
