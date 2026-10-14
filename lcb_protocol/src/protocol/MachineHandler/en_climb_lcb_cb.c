/**
 * @file en_climb_lcb_cb.c
 * @brief LCB Callback Bridge for EnClimb Handler
 *
 * Converted from: com/jht/lcbcommlib/machineHandler/EnClimbmill/EnClimbCallbackHandlerLCB.java
 *
 * Implements internal callbacks wired to lcb_comm_callbacks_t (and the nested
 * lcb_device_data_callbacks_t for the GetEStopVale sub-command). Each callback
 * receives en_climb_handler_t* via user_data and dispatches to the
 * user-facing callbacks. Java's EnClimbCallbackHandlerLCB overrides ~90
 * shared lcb_comm/device-data receive-interface methods but only these 7
 * (plus the 1 device-data one) do anything — those are the only ones wired
 * here; the rest are intentionally not bridged, matching the Java no-ops.
 */

#include "en_climb_handler_internal.h"
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_device_data.h"
#include "lcb_logging.h"
#include <pthread.h>
#include <string.h>

#define TAG "EnClimbHandlerLcbCb"

/* getLcbClimbmillStatusHandler */
static void ech_on_climbmill_status(void *ud, uint16_t status)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)ud;
    if (ech->user_cb.on_status)
        ech->user_cb.on_status(ech->user_cb.user_data, status);
}

/* getLcbSPMHandler */
static void ech_on_spm(void *ud, uint16_t spm)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)ud;
    if (ech->user_cb.on_spm)
        ech->user_cb.on_spm(ech->user_cb.user_data, spm);
}

/* getLcbBrakePressureValueHandler */
static void ech_on_brake_pressure(void *ud, uint16_t value)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)ud;
    if (ech->user_cb.on_brake_pressure)
        ech->user_cb.on_brake_pressure(ech->user_cb.user_data, value);
}

/* getLcbWattsHandler */
static void ech_on_watts(void *ud, uint16_t watts)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)ud;
    if (ech->user_cb.on_watts)
        ech->user_cb.on_watts(ech->user_cb.user_data, watts);
}

/* getLcbCalScalingHandler */
static void ech_on_cal_scaling(void *ud, uint16_t scaling)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)ud;
    if (ech->user_cb.on_cal_scaling)
        ech->user_cb.on_cal_scaling(ech->user_cb.user_data, scaling);
}

/* getLcbFrameCalScalingHandler */
static void ech_on_frame_cal_scaling(void *ud, uint16_t scaling)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)ud;
    if (ech->user_cb.on_frame_cal_scaling)
        ech->user_cb.on_frame_cal_scaling(ech->user_cb.user_data, scaling);
}

/* getOneShotLcbSpmHandler */
static void ech_on_one_shot_spm(void *ud, uint16_t spm)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)ud;
    if (ech->user_cb.on_one_shot_spm)
        ech->user_cb.on_one_shot_spm(ech->user_cb.user_data, spm);
}

/* deviceGetEStopValeHandler (device-data sub-command 0x04) */
static void ech_on_dd_get_estop_vale(void *ud, const uint8_t *data, uint8_t length)
{
    en_climb_handler_t *ech = (en_climb_handler_t *)ud;
    if (ech->user_cb.on_estop_vale)
        ech->user_cb.on_estop_vale(ech->user_cb.user_data, data, length);
}

/* ============================================================================
 * Registration
 * ============================================================================ */

void ech_lcb_cb_register(en_climb_handler_t *ech)
{
    lcb_comm_callbacks_init(&ech->lcb_cb);
    ech->lcb_cb.user_data = ech;

    ech->lcb_cb.on_lcb_climbmill_status_received = ech_on_climbmill_status;
    ech->lcb_cb.on_lcb_spm_received              = ech_on_spm;
    ech->lcb_cb.on_lcb_brake_pressure_value       = ech_on_brake_pressure;
    ech->lcb_cb.on_lcb_watts_received             = ech_on_watts;
    ech->lcb_cb.on_lcb_cal_scaling_received       = ech_on_cal_scaling;
    ech->lcb_cb.on_lcb_frame_cal_scaling_received = ech_on_frame_cal_scaling;
    ech->lcb_cb.on_one_shot_lcb_spm               = ech_on_one_shot_spm;

    /* Device-data sub-callback for GetEStopVale — same nesting pattern
     * general_handler uses for its own dd_cb (see
     * general_handler_lcb_cb.c). */
    lcb_device_data_callbacks_init(&ech->dd_cb);
    ech->dd_cb.user_data = ech;
    ech->dd_cb.on_dd_get_estop_vale = ech_on_dd_get_estop_vale;
    ech->lcb_cb.device_data_cb = &ech->dd_cb;
}

void ech_lcb_cb_unregister(en_climb_handler_t *ech)
{
    memset(&ech->lcb_cb, 0, sizeof(lcb_comm_callbacks_t));
    memset(&ech->dd_cb, 0, sizeof(lcb_device_data_callbacks_t));
}
