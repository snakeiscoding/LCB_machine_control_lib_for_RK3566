/**
 * @file treadmill_machine_convert.c
 * @brief Treadmill unit conversion / range clamping — Layer_1 "capabilities"
 */

#include "treadmill_machine_convert.h"
#include <stddef.h>

#define TM_KPH_PER_MPH 1.609344f
#define TM_KG_PER_LBS  0.45359237f

float tm_clamp_float(float v, float lo, float hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

float tm_clamp_speed_kph(const frame_config_data_t *fc, float kph)
{
    if (fc == NULL || !fc->has_speed_kph_range) return 0.0f;
    return tm_clamp_float(kph, fc->speed_kph_range.min_value, fc->speed_kph_range.max_value);
}

float tm_clamp_incline_percent(const frame_config_data_t *fc, float percent)
{
    if (fc == NULL || !fc->has_incline_percent_range) return 0.0f;
    return tm_clamp_float(percent, fc->incline_percent_range.min_value,
                          fc->incline_percent_range.max_value);
}

float tm_clamp_weight_kg(const frame_config_data_t *fc, float kg)
{
    if (fc == NULL || !fc->has_weight_kg_range) return kg;
    return tm_clamp_float(kg, (float)fc->weight_kg_range.min_value,
                          (float)fc->weight_kg_range.max_value);
}

uint16_t tm_kph_to_rpm(const frame_config_data_t *fc, float kph)
{
    if (fc == NULL || !fc->has_speed_kph_range || !fc->has_rpm_range) return 0;
    if (kph <= 0.0f) return 0;

    float kmin = fc->speed_kph_range.min_value, kmax = fc->speed_kph_range.max_value;
    float rmin = (float)fc->rpm_range.min_value, rmax = (float)fc->rpm_range.max_value;
    if (kmax <= kmin) return (uint16_t)rmin;

    kph = tm_clamp_float(kph, kmin, kmax);
    float rpm = rmin + (kph - kmin) * (rmax - rmin) / (kmax - kmin);
    return (uint16_t)(rpm + 0.5f);
}

float tm_rpm_to_kph(const frame_config_data_t *fc, uint16_t rpm)
{
    if (fc == NULL || !fc->has_speed_kph_range || !fc->has_rpm_range) return 0.0f;
    rpm &= (uint16_t)~TM_REVERSE_BIT;
    if (rpm == 0) return 0.0f;

    float kmin = fc->speed_kph_range.min_value, kmax = fc->speed_kph_range.max_value;
    float rmin = (float)fc->rpm_range.min_value, rmax = (float)fc->rpm_range.max_value;
    if (rmax <= rmin) return kmin;

    float r = tm_clamp_float((float)rpm, rmin, rmax);
    return kmin + (r - rmin) * (kmax - kmin) / (rmax - rmin);
}

uint16_t tm_incline_to_adc(const frame_config_data_t *fc, float percent)
{
    if (fc == NULL || !fc->has_incline_percent_range || !fc->has_incline_adc_range) return 0;

    float pmin = fc->incline_percent_range.min_value, pmax = fc->incline_percent_range.max_value;
    float amin = (float)fc->incline_adc_range.min_value, amax = (float)fc->incline_adc_range.max_value;

    float adc = amin;
    if (pmax > pmin) {
        percent = tm_clamp_float(percent, pmin, pmax);
        adc = amin + (percent - pmin) * (amax - amin) / (pmax - pmin);
    }
    uint16_t out = (uint16_t)(adc + 0.5f);
    if (fc->uses_reverse_incline) out |= TM_REVERSE_BIT;
    return out;
}

float tm_adc_to_incline(const frame_config_data_t *fc, uint16_t adc)
{
    if (fc == NULL || !fc->has_incline_percent_range || !fc->has_incline_adc_range) return 0.0f;
    adc &= (uint16_t)~TM_REVERSE_BIT;

    float pmin = fc->incline_percent_range.min_value, pmax = fc->incline_percent_range.max_value;
    float amin = (float)fc->incline_adc_range.min_value, amax = (float)fc->incline_adc_range.max_value;
    if (amax <= amin) return pmin;

    float a = tm_clamp_float((float)adc, amin, amax);
    return pmin + (a - amin) * (pmax - pmin) / (amax - amin);
}

float tm_kph_to_mph(float kph) { return kph / TM_KPH_PER_MPH; }
float tm_mph_to_kph(float mph) { return mph * TM_KPH_PER_MPH; }
float tm_lbs_to_kg(float lbs)  { return lbs * TM_KG_PER_LBS; }
float tm_kg_to_lbs(float kg)   { return kg / TM_KG_PER_LBS; }
