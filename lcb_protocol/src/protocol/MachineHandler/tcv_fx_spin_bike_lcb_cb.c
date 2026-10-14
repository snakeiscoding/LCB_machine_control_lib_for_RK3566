/**
 * @file tcv_fx_spin_bike_lcb_cb.c
 * @brief LCB Callback Bridge for TcvFxSpinBike Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/TcvFxSpinBike/TcvFxSpinBikeCallbackHandlerLCB.java
 *
 * Registers on the SAME lcb_comm_callbacks_t slot as indoor_bike_handler
 * (on_lcb_indoor_bike_sensor_data / on_one_shot_lcb_indoor_bike_sensor_data)
 * because TcvFxSpinBike reuses IndoorBike's 0x51 command with a shorter,
 * distinct 11-byte payload. Since only one machine handler mode ever runs at
 * a time in this app (single-owner lcb_comm_register_callbacks), IndoorBike
 * and TcvFxSpinBike never fight over this slot at runtime — but this bridge
 * still checks the LCB-type discriminator byte defensively (raw[0]), matching
 * the Java source's own defensive check ("owned by IndoorBikeCallbackHandlerLCB"
 * comment), and drops any frame that isn't its own.
 */

#include "tcv_fx_spin_bike_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_logging.h"
#include <string.h>

#define TAG "TcvFxSpinBikeHandlerLcbCb"

static void dispatch_sensor_data(tcv_fx_spin_bike_handler_t *h, const uint8_t *raw, bool one_shot)
{
    tcv_fx_spin_bike_sensor_data_t parsed;
    if (!tfsb_parse_sensor_data(raw, &parsed)) return;

    if (h->user_cb.on_sensor_data)
        h->user_cb.on_sensor_data(h->user_cb.user_data, &parsed);
    /* Dispatch resistance before RPM/watts, matching the Java source's own
     * ordering ("Dispatch resistance before RPM so machineData.resistanceLevel
     * is set before onRpm triggers updateMachineData()"). */
    if (h->user_cb.on_resistance_level)
        h->user_cb.on_resistance_level(h->user_cb.user_data, parsed.resistance_level);
    if (h->user_cb.on_rpm)
        h->user_cb.on_rpm(h->user_cb.user_data, parsed.crank_rpm);
    if (h->user_cb.on_watts)
        h->user_cb.on_watts(h->user_cb.user_data, parsed.watts);

    (void)one_shot; /* no behavioral difference beyond which lcb_comm slot invoked us */
}

/* 0x51 GetLCBIndoorBikeSensorData (auto-poll response) — TCV-FX's 11-byte frame */
static void tfsb_on_lcb_sensor_data(void *ud, const indoor_bike_sensor_data_t *sensor_data)
{
    tcv_fx_spin_bike_handler_t *h = (tcv_fx_spin_bike_handler_t *)ud;
    dispatch_sensor_data(h, (const uint8_t *)sensor_data, false);
}

/* 0x51 One-shot GetLCBIndoorBikeSensorData */
static void tfsb_on_one_shot_lcb_sensor_data(void *ud, const indoor_bike_sensor_data_t *sensor_data)
{
    tcv_fx_spin_bike_handler_t *h = (tcv_fx_spin_bike_handler_t *)ud;
    dispatch_sensor_data(h, (const uint8_t *)sensor_data, true);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void tfsb_lcb_cb_register(tcv_fx_spin_bike_handler_t *h)
{
    lcb_comm_callbacks_init(&h->lcb_cb);
    h->lcb_cb.user_data = h;

    h->lcb_cb.on_lcb_indoor_bike_sensor_data          = tfsb_on_lcb_sensor_data;
    h->lcb_cb.on_one_shot_lcb_indoor_bike_sensor_data  = tfsb_on_one_shot_lcb_sensor_data;
}

void tfsb_lcb_cb_unregister(tcv_fx_spin_bike_handler_t *h)
{
    memset(&h->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
}
