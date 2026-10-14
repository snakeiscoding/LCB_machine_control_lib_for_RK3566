#ifndef LCB_TYPE_H
#define LCB_TYPE_H

#include <stdint.h>

// Defines the LcbType type alias for use in function parameters
typedef uint8_t LcbType_t;

// --- LCB Type Definitions ---

// Standard / JIS Types
#define LCB_TYPE_AC_LCB                     ((uint8_t)0x01) // (JIS)
#define LCB_TYPE_LCB1_PWM                   ((uint8_t)0x02) // (JHT)
#define LCB_TYPE_LCB3_PWM                   ((uint8_t)0x03) // (JHT)
#define LCB_TYPE_DC_LCB                     ((uint8_t)0x04) // (JIS)
#define LCB_TYPE_LCB_CD                     ((uint8_t)0x05) // (JHT LCB1 cost down version)
#define LCB_TYPE_TM_LCB                     ((uint8_t)0x06) // (Panatta)
#define LCB_TYPE_LCB1_CURRENT               ((uint8_t)0x07) // (JHT)
#define LCB_TYPE_LCB3_CURRENT               ((uint8_t)0x08) // (JHT)
#define LCB_TYPE_LCB1_PANATTA               ((uint8_t)0x09) // (Panatta)
#define LCB_TYPE_LCB2                       ((uint8_t)0x0A) // (JHT), A5x Redesign
#define LCB_TYPE_CLIMBMILL_OR_A30           ((uint8_t)0x0B) // (JHT)/ EP (JIS)
#define LCB_TYPE_BIKE                       ((uint8_t)0x0C) // (JIS Electromagnet)

// DC LCB Variants
#define LCB_TYPE_DC_LCB_200_275             ((uint8_t)0x0D) // (JIS 2.0~2.75HP DC LCB)
#define LCB_TYPE_DC_LCB_300_325             ((uint8_t)0x0E) // (JIS 3.0~3.25HP DC LCB)

// AC LCB Variants
#define LCB_TYPE_AC_LCB_200_275             ((uint8_t)0x0F) // (JIS 2.0~2.75HP AC LCB)
#define LCB_TYPE_AC_LCB_300_325             ((uint8_t)0x10) // (JIS 3.0~3.25HP AC LCB)

// Boards & Specific Models
#define LCB_TYPE_AD_BOARD                   ((uint8_t)0x11) // (JIS Analog conversion digital board)
#define LCB_TYPE_ECB_BOARD                  ((uint8_t)0x12) // (JIS)
#define LCB_TYPE_LCBA                       ((uint8_t)0x13)
#define LCB_TYPE_1X_BIKE_LCB                ((uint8_t)0x14) // (Watt control)
#define LCB_TYPE_ELECTROMAGNETIC_INCLINE    ((uint8_t)0x15) // (JIS) Electromagnetic brake + Incline LCB
#define LCB_TYPE_SMART_STRENGTH_LOAD_CELL   ((uint8_t)0x16)
#define LCB_TYPE_COMMERCIAL_CLIMBMILL       ((uint8_t)0x17)
#define LCB_TYPE_R3XM_LCB                   ((uint8_t)0x18)
#define LCB_TYPE_RETAIL_CLIMBMILL           ((uint8_t)0x19)
#define LCB_TYPE_ATHENA_LCB                 ((uint8_t)0x1B)
#define LCB_TYPE_JHT_AD_BOARD               ((uint8_t)0x1C) // (JHT Analog conversion digital board)
#define LCB_TYPE_JIS_ATHENA                 ((uint8_t)0x1D)
#define LCB_TYPE_TCV_FX                     ((uint8_t)0x22)

// JHT VAVE Boards (0xA1 - 0xA3)
#define LCB_TYPE_JHT_VAVE_PS_ES_BOARD      ((uint8_t)0xA1)
#define LCB_TYPE_JHT_VAVE_T600_BOARD       ((uint8_t)0xA2)
#define LCB_TYPE_JHT_VAVE_LS_BOARD         ((uint8_t)0xA3)

// Global Models (0xB0 - 0xB9)
#define LCB_TYPE_GLOBAL_15_20HP_110V        ((uint8_t)0xB0)
#define LCB_TYPE_GLOBAL_15_20HP_220V        ((uint8_t)0xB1)
#define LCB_TYPE_GLOBAL_20_25HP_110V        ((uint8_t)0xB2)
#define LCB_TYPE_GLOBAL_20_25HP_220V        ((uint8_t)0xB3)
#define LCB_TYPE_ECB_JIS                    ((uint8_t)0xB4)
#define LCB_TYPE_ECB_INDUCTOR_BRAKE         ((uint8_t)0xB5)
#define LCB_TYPE_GLOBAL_ECB_INCLINE         ((uint8_t)0xB6)
#define LCB_TYPE_GLOBAL_INDUCTION           ((uint8_t)0xB7)
#define LCB_TYPE_GLOBAL_INDUCTION_INCLINE   ((uint8_t)0xB8)
#define LCB_TYPE_GLOBAL_FULL_FUNCTION       ((uint8_t)0xB9)

// 3rd Party / Vendor Specific (0xC0 - 0xC8)
#define LCB_TYPE_DCI_DAUGHTER_BOARD         ((uint8_t)0xC0)
#define LCB_TYPE_TOPTEK                     ((uint8_t)0xC1)
#define LCB_TYPE_DELTA                      ((uint8_t)0xC2)
#define LCB_TYPE_DELTA_AC_11KW              ((uint8_t)0xC3)
#define LCB_TYPE_DELTA_HYBRID               ((uint8_t)0xC4)
#define LCB_TYPE_DELTA_T1                   ((uint8_t)0xC5)
#define LCB_TYPE_DELTA_SPARE_1              ((uint8_t)0xC6)
#define LCB_TYPE_DELTA_SPARE_2              ((uint8_t)0xC7)
#define LCB_TYPE_DELTA_SPARE_3              ((uint8_t)0xC8)

// Global High Power (0xD0 - 0xD1)
#define LCB_TYPE_GLOBAL_200_325HP_110V      ((uint8_t)0xD0) // (Global 2.0~3.25HP/110V)
#define LCB_TYPE_GLOBAL_200_325HP_220V      ((uint8_t)0xD1) // (Global 2.0~3.25HP/220V)

// Unknown / Reserved
#define LCB_TYPE_E0                         ((uint8_t)0xE0)
#define LCB_TYPE_E1                         ((uint8_t)0xE1)

// JHT VAVE PP Board (0xF1)
#define LCB_TYPE_JHT_VAVE_PP_BOARD         ((uint8_t)0xF1)

#define LCB_TYPE_UNKNOWN                    ((uint8_t)0x00)

// Function declaration: for debugging, converts a type value into its string name
const char* LcbType_toString(LcbType_t type);

#endif // LCB_TYPE_H