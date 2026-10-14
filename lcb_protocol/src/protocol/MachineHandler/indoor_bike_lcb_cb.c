/**
 * @file indoor_bike_lcb_cb.c
 * @brief LCB Callback Bridge for IndoorBike Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/IndoorBike/IndoorBikeCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t.
 * Each callback receives indoor_bike_handler_t* via user_data and dispatches
 * to the user-facing callbacks. Java's IndoorBikeCallbackHandlerLCB overrides
 * ~100 lcb_comm-receive-interface methods but only 10 do anything — those are
 * the only ones wired here; the rest are intentionally not bridged, matching
 * the Java no-ops. Notably getLcbIndoorBikeErgModeSettingsHandler (0x59 GET
 * reply) is ALSO a Java no-op (asymmetric with its "set" sibling) and is
 * likewise left unbridged here — do not invent an on_get_erg_mode_settings
 * callback; IIndoorBikeHandlerCallback has no such method.
 */

#include "indoor_bike_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "IndoorBikeHandlerLcbCb"

/* 0xA0 GetDCIinclineRange — ICR70 reuses this for raw ADC. Java gates BOTH
 * the TX request (indoor_bike_handler_request_raw_adc) and this RX forward
 * on isIcr() (LCB_TYPE_JIS_ATHENA). */
static void ibh_on_lcb_dci_incline_range(void *ud, uint32_t range)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (!ibh_is_icr(ibh)) return;
    if (ibh->user_cb.on_raw_adc)
        ibh->user_cb.on_raw_adc(ibh->user_cb.user_data, (int)range);
}

/* 0x63 GetECBrpm (auto-poll response) */
static void ibh_on_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_rpm)
        ibh->user_cb.on_rpm(ibh->user_cb.user_data, (int)rpm);
}

/* One-shot GetECBrpm */
static void ibh_on_one_shot_lcb_ecb_rpm(void *ud, uint16_t rpm)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_one_shot_rpm)
        ibh->user_cb.on_one_shot_rpm(ibh->user_cb.user_data, (int)rpm);
}

/* GetLCBWatts response (ICR70) */
static void ibh_on_lcb_watts_received(void *ud, uint16_t watts)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_watts)
        ibh->user_cb.on_watts(ibh->user_cb.user_data, (int)watts);
}

/* 0x51 GetLCBIndoorBikeSensorData (auto-poll response) */
static void ibh_on_lcb_indoor_bike_sensor_data(void *ud, const indoor_bike_sensor_data_t *sensor_data)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_sensor_data)
        ibh->user_cb.on_sensor_data(ibh->user_cb.user_data, sensor_data);
}

/* 0x51 One-shot GetLCBIndoorBikeSensorData */
static void ibh_on_one_shot_lcb_indoor_bike_sensor_data(void *ud, const indoor_bike_sensor_data_t *sensor_data)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_one_shot_sensor_data)
        ibh->user_cb.on_one_shot_sensor_data(ibh->user_cb.user_data, sensor_data);
}

/* 0x54 GetLCBIndoorBikeCalcParaOfDevice (GET reply, carries data) */
static void ibh_on_lcb_indoor_bike_calc_para_of_device_get(void *ud, const uint8_t *data, uint8_t length)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_get_calc_para_of_device)
        ibh->user_cb.on_get_calc_para_of_device(ibh->user_cb.user_data, data, length);
}

/* 0x55 SetLCBIndoorBikeActionSpinDown (ACK, carries echoed data) */
static void ibh_on_lcb_indoor_bike_action_spin_down(void *ud, const uint8_t *data, uint8_t length)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_set_action_spin_down)
        ibh->user_cb.on_set_action_spin_down(ibh->user_cb.user_data, data, length);
}

/* 0x56 GetLCBIndoorBikeInfoSpinDown */
static void ibh_on_lcb_indoor_bike_info_spin_down(void *ud, const indoor_bike_info_spin_down_t *info)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_info_spin_down)
        ibh->user_cb.on_info_spin_down(ibh->user_cb.user_data, info);
}

/* 0x57 SetLCBIndoorBikeErgModeSettings (ACK, carries echoed data) */
static void ibh_on_lcb_indoor_bike_erg_mode_settings(void *ud, const uint8_t *data, uint8_t length)
{
    indoor_bike_handler_t *ibh = (indoor_bike_handler_t *)ud;
    if (ibh->user_cb.on_set_erg_mode_settings)
        ibh->user_cb.on_set_erg_mode_settings(ibh->user_cb.user_data, data, length);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void ibh_lcb_cb_register(indoor_bike_handler_t *ibh)
{
    lcb_comm_callbacks_init(&ibh->lcb_cb);
    ibh->lcb_cb.user_data = ibh;

    ibh->lcb_cb.on_lcb_dci_incline_range                    = ibh_on_lcb_dci_incline_range;
    ibh->lcb_cb.on_lcb_ecb_rpm                              = ibh_on_lcb_ecb_rpm;
    ibh->lcb_cb.on_one_shot_lcb_ecb_rpm                     = ibh_on_one_shot_lcb_ecb_rpm;
    ibh->lcb_cb.on_lcb_watts_received                       = ibh_on_lcb_watts_received;
    ibh->lcb_cb.on_lcb_indoor_bike_sensor_data              = ibh_on_lcb_indoor_bike_sensor_data;
    ibh->lcb_cb.on_one_shot_lcb_indoor_bike_sensor_data     = ibh_on_one_shot_lcb_indoor_bike_sensor_data;
    ibh->lcb_cb.on_lcb_indoor_bike_calc_para_of_device_get  = ibh_on_lcb_indoor_bike_calc_para_of_device_get;
    ibh->lcb_cb.on_lcb_indoor_bike_action_spin_down         = ibh_on_lcb_indoor_bike_action_spin_down;
    ibh->lcb_cb.on_lcb_indoor_bike_info_spin_down           = ibh_on_lcb_indoor_bike_info_spin_down;
    ibh->lcb_cb.on_lcb_indoor_bike_erg_mode_settings        = ibh_on_lcb_indoor_bike_erg_mode_settings;
}

void ibh_lcb_cb_unregister(indoor_bike_handler_t *ibh)
{
    memset(&ibh->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
