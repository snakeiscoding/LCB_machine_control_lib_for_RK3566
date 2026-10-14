/**
 * @file treadmill_machine_adapter.h
 * @brief Treadmill machine-control command adapter — Layer_2 "adapters"
 *
 * Translates control-layer intents (speed in kph, incline in %, EStop,
 * detect-machine, tread sense, over-current, motor type, ERP ...) into calls
 * on the existing treadmill_interface_t / lcb_comm API, using the model's
 * frame_config_data_t (loaded from frame_configs.json) for unit conversion.
 *
 * Converted from chimera2:
 *   LCBInterface.startMachine()/stopMachine()/setTreadmillDetectMachine()
 *   LCBDataHandler.setTreadmillMachineControl()
 *   GeneralHandler.procEStopLcb()
 * Nothing here touches the protocol layer's sources.
 */

#ifndef TREADMILL_MACHINE_ADAPTER_H
#define TREADMILL_MACHINE_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>
#include "treadmill_interface.h"
#include "frame_config_data.h"
#include "lcb_comm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Incline-action byte values (SetLCBInclineAction 0xF5) */
#define TM_INCLINE_ACTION_STOP 0x00
#define TM_INCLINE_ACTION_UP   0x01
#define TM_INCLINE_ACTION_DOWN 0xFF

typedef struct {
    treadmill_interface_t *ti;        /* NOT owned */
    lcb_comm_t            *comm;      /* NOT owned */
    const frame_config_data_t *fc;    /* NOT owned */
    bool tread_sense;                 /* Java: treadSense   (gates 0x95 polling) */
    bool current_oc;                  /* Java: currentOC    (gates 0x96 polling) */
} tm_adapter_t;

void tm_adapter_init(tm_adapter_t *a, treadmill_interface_t *ti, lcb_comm_t *comm,
                     const frame_config_data_t *fc);

/* Speed / incline (values are clamped to the frame config's range) */
void tm_adapter_set_speed_kph(tm_adapter_t *a, float kph);
void tm_adapter_set_incline_percent(tm_adapter_t *a, float percent);
void tm_adapter_set_incline_adc(tm_adapter_t *a, uint16_t adc); /* raw, reverse bit added if needed */
void tm_adapter_incline_action(tm_adapter_t *a, uint8_t action);

/* Start / stop */
void tm_adapter_start_machine(tm_adapter_t *a);   /* detect-machine on */
void tm_adapter_stop_machine(tm_adapter_t *a);    /* rpm 0 + detect-machine off */
void tm_adapter_set_detect_machine(tm_adapter_t *a, bool on);

/* Function enables */
void tm_adapter_set_tread_sense(tm_adapter_t *a, bool enable);
void tm_adapter_set_current_oc_detect(tm_adapter_t *a, bool enable);
void tm_adapter_set_motor_type(tm_adapter_t *a, uint16_t watts);   /* ignored when 0 */
void tm_adapter_apply_motor_type_from_config(tm_adapter_t *a);

/* EStop */
void tm_adapter_estop_stop(tm_adapter_t *a);      /* highest-priority rpm 0 + incline stop */
void tm_adapter_estop_reset(tm_adapter_t *a);     /* rpm 0 + incline -> ADC min */
void tm_adapter_estop_unlock(tm_adapter_t *a);    /* LCB re-initialize (0x70 ...) */

/* ERP (0x7A) */
void tm_adapter_enter_erp(tm_adapter_t *a);

#ifdef __cplusplus
}
#endif

#endif /* TREADMILL_MACHINE_ADAPTER_H */
