#include "lcb_type.h"

const char* LcbType_toString(LcbType_t type) {
    switch (type) {
        case LCB_TYPE_AC_LCB:                   return "AC_LCB (JIS)";
        case LCB_TYPE_LCB1_PWM:                 return "LCB1_PWM (JHT)";
        case LCB_TYPE_LCB3_PWM:                 return "LCB3_PWM (JHT)";
        case LCB_TYPE_DC_LCB:                   return "DC_LCB (JIS)";
        case LCB_TYPE_LCB_CD:                   return "LCB_CD";
        case LCB_TYPE_TM_LCB:                   return "TM_LCB (Panatta)";
        case LCB_TYPE_LCB1_CURRENT:             return "LCB1_Current";
        case LCB_TYPE_LCB3_CURRENT:             return "LCB3_Current";
        case LCB_TYPE_LCB1_PANATTA:             return "LCB1 (Panatta)";
        case LCB_TYPE_LCB2:                     return "LCB2 (A5x Redesign)";
        case LCB_TYPE_CLIMBMILL_OR_A30:         return "ClimbMill/A30";
        case LCB_TYPE_BIKE:                     return "BIKE";
        
        case LCB_TYPE_DC_LCB_200_275:           return "DC_LCB 2.0-2.75HP";
        case LCB_TYPE_DC_LCB_300_325:           return "DC_LCB 3.0-3.25HP";
        case LCB_TYPE_AC_LCB_200_275:           return "AC_LCB 2.0-2.75HP";
        case LCB_TYPE_AC_LCB_300_325:           return "AC_LCB 3.0-3.25HP";
        
        case LCB_TYPE_AD_BOARD:                 return "AD_Board";
        case LCB_TYPE_ECB_BOARD:                return "ECB_Board";
        case LCB_TYPE_LCBA:                     return "LCBA";
        case LCB_TYPE_1X_BIKE_LCB:              return "1x_Bike_LCB";
        case LCB_TYPE_ELECTROMAGNETIC_INCLINE:  return "Electromagnetic_Incline";
        case LCB_TYPE_SMART_STRENGTH_LOAD_CELL: return "Smart_Strength_Load_Cell";
        case LCB_TYPE_COMMERCIAL_CLIMBMILL:     return "Commercial_Climbmill";
        case LCB_TYPE_R3XM_LCB:                 return "R3xm_LCB";
        case LCB_TYPE_RETAIL_CLIMBMILL:         return "Retail_Climbmill";
        case LCB_TYPE_ATHENA_LCB:               return "ATHENA_LCB";
        case LCB_TYPE_JHT_AD_BOARD:             return "JHT_AD_Board";
        case LCB_TYPE_JIS_ATHENA:               return "JIS_ATHENA";

        case LCB_TYPE_JHT_VAVE_PS_ES_BOARD:    return "JHT_VAVE_PS_ES_Board";
        case LCB_TYPE_JHT_VAVE_T600_BOARD:     return "JHT_VAVE_T600_Board";
        case LCB_TYPE_JHT_VAVE_LS_BOARD:       return "JHT_VAVE_LS_Board";

        case LCB_TYPE_GLOBAL_15_20HP_110V:      return "Global 1.5-2.0HP 110V";
        case LCB_TYPE_GLOBAL_15_20HP_220V:      return "Global 1.5-2.0HP 220V";
        case LCB_TYPE_GLOBAL_20_25HP_110V:      return "Global 2.0-2.5HP 110V";
        case LCB_TYPE_GLOBAL_20_25HP_220V:      return "Global 2.0-2.5HP 220V";
        case LCB_TYPE_ECB_JIS:                  return "ECB_JIS";
        case LCB_TYPE_ECB_INDUCTOR_BRAKE:       return "ECB_INDUCTOR_BRAKE";
        case LCB_TYPE_GLOBAL_ECB_INCLINE:       return "Global ECB Incline";
        case LCB_TYPE_GLOBAL_INDUCTION:         return "Global Induction";
        case LCB_TYPE_GLOBAL_INDUCTION_INCLINE: return "Global Induction Incline";
        case LCB_TYPE_GLOBAL_FULL_FUNCTION:     return "Global Full Function";
        
        case LCB_TYPE_DCI_DAUGHTER_BOARD:       return "DCI_Daughter_Board";
        case LCB_TYPE_TOPTEK:                   return "Toptek";
        case LCB_TYPE_DELTA:                    return "DELTA";
        case LCB_TYPE_DELTA_AC_11KW:            return "DELTA AC 1.1KW";
        case LCB_TYPE_DELTA_HYBRID:             return "DELTA Hybrid";
        case LCB_TYPE_DELTA_T1:                 return "DELTA T1";
        case LCB_TYPE_DELTA_SPARE_1:            return "DELTA Spare 1";
        case LCB_TYPE_DELTA_SPARE_2:            return "DELTA Spare 2";
        case LCB_TYPE_DELTA_SPARE_3:            return "DELTA Spare 3";
        
        case LCB_TYPE_GLOBAL_200_325HP_110V:    return "Global 2.0-3.25HP 110V";
        case LCB_TYPE_GLOBAL_200_325HP_220V:    return "Global 2.0-3.25HP 220V";
        
        case LCB_TYPE_E0:                       return "E0";
        case LCB_TYPE_E1:                       return "E1";

        case LCB_TYPE_JHT_VAVE_PP_BOARD:       return "JHT_VAVE_PP_Board";

        default:                                return "UNKNOWN";
    }
}