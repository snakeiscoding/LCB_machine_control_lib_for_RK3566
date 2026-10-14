/**
 * @file treadmill_machine_adapter.c
 * @brief Treadmill machine-control command adapter — Layer_2 "adapters"
 */

#include "treadmill_machine_adapter.h"
#include "treadmill_machine_convert.h"
#include "lcb_logging.h"

#define TAG "TmMachineAdapter"

void tm_adapter_init(tm_adapter_t *a, treadmill_interface_t *ti, lcb_comm_t *comm,
                     const frame_config_data_t *fc)
{
    if (a == NULL) return;
    a->ti = ti;
    a->comm = comm;
    a->fc = fc;
    a->tread_sense = false;
    a->current_oc = false;
}

void tm_adapter_set_speed_kph(tm_adapter_t *a, float kph)
{
    if (a == NULL || a->ti == NULL || a->fc == NULL) return;
    float clamped = (kph <= 0.0f) ? 0.0f : tm_clamp_speed_kph(a->fc, kph);
    treadmill_interface_set_rpm(a->ti, tm_kph_to_rpm(a->fc, clamped));
}

void tm_adapter_set_incline_percent(tm_adapter_t *a, float percent)
{
    if (a == NULL || a->ti == NULL || a->fc == NULL) return;
    treadmill_interface_set_incline(a->ti, tm_incline_to_adc(a->fc, percent));
}

void tm_adapter_set_incline_adc(tm_adapter_t *a, uint16_t adc)
{
    if (a == NULL || a->ti == NULL || a->fc == NULL) return;
    adc &= (uint16_t)~TM_REVERSE_BIT;
    if (a->fc->uses_reverse_incline) adc |= TM_REVERSE_BIT;
    treadmill_interface_set_incline(a->ti, adc);
}

void tm_adapter_incline_action(tm_adapter_t *a, uint8_t action)
{
    if (a == NULL || a->ti == NULL) return;
    treadmill_interface_set_incline_action(a->ti, action);
}

/* Java: LCBInterface.setTreadmillDetectMachine(on):
 *   setAutoReplyTmInUsed(on && treadSense); setAutoReplyTmMotorInfo(on && currentOC) */
void tm_adapter_set_detect_machine(tm_adapter_t *a, bool on)
{
    if (a == NULL || a->ti == NULL) return;
    treadmill_interface_set_auto_reply_in_used(a->ti, on && a->tread_sense);
    treadmill_interface_set_auto_reply_motor_info(a->ti, on && a->current_oc);
}

void tm_adapter_start_machine(tm_adapter_t *a)
{
    tm_adapter_set_detect_machine(a, true);
}

void tm_adapter_stop_machine(tm_adapter_t *a)
{
    if (a == NULL || a->ti == NULL) return;
    treadmill_interface_set_rpm(a->ti, 0);          /* gated while safety key is down */
    tm_adapter_set_detect_machine(a, false);
}

void tm_adapter_set_tread_sense(tm_adapter_t *a, bool enable)
{
    if (a == NULL) return;
    a->tread_sense = enable;
}

void tm_adapter_set_current_oc_detect(tm_adapter_t *a, bool enable)
{
    if (a == NULL) return;
    a->current_oc = enable;
}

void tm_adapter_set_motor_type(tm_adapter_t *a, uint16_t watts)
{
    if (a == NULL || a->ti == NULL) return;
    if (watts == 0) return;                          /* Java: only if > 0 */
    treadmill_interface_set_motor_type(a->ti, watts);
}

void tm_adapter_apply_motor_type_from_config(tm_adapter_t *a)
{
    if (a == NULL || a->fc == NULL || a->fc->motor_type <= 0) return;
    tm_adapter_set_motor_type(a, (uint16_t)a->fc->motor_type);
}

void tm_adapter_estop_stop(tm_adapter_t *a)
{
    if (a == NULL || a->ti == NULL) return;
    treadmill_interface_set_rpm_high_priority(a->ti, 0);
    treadmill_interface_set_incline_action_high_priority(a->ti, TM_INCLINE_ACTION_STOP);
}

void tm_adapter_estop_reset(tm_adapter_t *a)
{
    if (a == NULL || a->ti == NULL || a->fc == NULL) return;
    treadmill_interface_estop_reset_gui_current_rpm(a->ti);
    treadmill_interface_set_rpm_high_priority(a->ti, 0);
    if (a->fc->has_incline_adc_range)
        tm_adapter_set_incline_adc(a, (uint16_t)a->fc->incline_adc_range.min_value);
}

void tm_adapter_estop_unlock(tm_adapter_t *a)
{
    if (a == NULL || a->ti == NULL) return;
    treadmill_interface_estop_revert_lcb_initial(a->ti);
}

void tm_adapter_enter_erp(tm_adapter_t *a)
{
    if (a == NULL || a->comm == NULL) return;
    LCB_LOGD(TAG, "enter ERP");
    lcb_comm_enter_lcb_erp_mode(a->comm);
}
