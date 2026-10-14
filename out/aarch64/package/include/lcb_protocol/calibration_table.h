/**
 * @file calibration_table.h
 * @brief ADC <-> physical value linear interpolation — Layer_1 "capabilities"
 *
 * Converted from: com/jht/lcb/core/capabilities/CalibrationTable.kt
 *
 * Plain data, no lifecycle API — matches the CREAT_MACHINE_HANDLER skill's
 * guidance for this layer.
 */

#ifndef CALIBRATION_TABLE_H
#define CALIBRATION_TABLE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int   adc_min;
    int   adc_max;
    float value_min;
    float value_max;
} calibration_table_t;

/**
 * @brief Port of CalibrationTable.adcToValue(Int): clamps @p adc into
 * [min(adc_min,adc_max), max(adc_min,adc_max)], then linearly maps it into
 * [value_min, value_max].
 */
float calibration_table_adc_to_value(const calibration_table_t *table, int adc);

/**
 * @brief Port of CalibrationTable.valueToAdc(Float): clamps @p value into
 * [min(value_min,value_max), max(value_min,value_max)], then linearly maps
 * it into [adc_min, adc_max]. Truncates toward zero (matches Kotlin's
 * `.toInt()`), does not round.
 */
int calibration_table_value_to_adc(const calibration_table_t *table, float value);

#ifdef __cplusplus
}
#endif

#endif /* CALIBRATION_TABLE_H */
