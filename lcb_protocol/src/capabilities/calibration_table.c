/**
 * @file calibration_table.c
 * @brief See calibration_table.h
 */

#include "calibration_table.h"

static int clamp_int(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static float clamp_float(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float calibration_table_adc_to_value(const calibration_table_t *table, int adc)
{
    int lo = table->adc_min < table->adc_max ? table->adc_min : table->adc_max;
    int hi = table->adc_min < table->adc_max ? table->adc_max : table->adc_min;
    int clamped_adc = clamp_int(adc, lo, hi);

    float ratio = (float)(clamped_adc - table->adc_min) / (float)(table->adc_max - table->adc_min);
    return table->value_min + ratio * (table->value_max - table->value_min);
}

int calibration_table_value_to_adc(const calibration_table_t *table, float value)
{
    float lo = table->value_min < table->value_max ? table->value_min : table->value_max;
    float hi = table->value_min < table->value_max ? table->value_max : table->value_min;
    float clamped_value = clamp_float(value, lo, hi);

    float ratio = (clamped_value - table->value_min) / (table->value_max - table->value_min);
    return (int)(table->adc_min + ratio * (table->adc_max - table->adc_min));
}
