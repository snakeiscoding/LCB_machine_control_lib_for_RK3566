/**
 * @file jht_ad_board_handler.h
 * @brief LCB JhtADboard Handler — JHT Analog-conversion-digital board orchestration layer
 *
 * Converted from:
 *   - com/jht/lcbcommlib/machineHandler/JhtADboard/IJhtADboardHandler.java
 *   - com/jht/lcbcommlib/machineHandler/JhtADboard/IJhtADboardHandlerCallback.java
 *   - com/jht/lcbcommlib/machineHandler/JhtADboard/JhtADboardHandler.java
 *   - com/jht/lcbcommlib/machineHandler/JhtADboard/JhtADboardCallbackHandlerLCB.java
 *
 * Sits on top of the lcb_comm layer (NOT general_handler — independent).
 * This is the thinnest machine handler in the codebase: a single auto-reply
 * feature (RPM read-back) and two simple TX setters, no ramp/state machine,
 * no frame-config integration (Java has none at this layer — default gear
 * ratios are set externally by LCBDataHandler.java per product model, out of
 * scope here).
 */

#ifndef JHT_AD_BOARD_HANDLER_H
#define JHT_AD_BOARD_HANDLER_H

#include <stdint.h>
#include <stdbool.h>
#include "lcb_comm.h"
#include "lcb_type.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Opaque Type
 * ============================================================================ */

typedef struct jht_ad_board_handler jht_ad_board_handler_t;

/* ============================================================================
 * User-Facing Callback Struct (IJhtADboardHandlerCallback equivalent)
 * ============================================================================ */

typedef struct {
    void *user_data;

    void (*on_jht_ad_board_rpm)(void *ud, uint16_t rpm);
    void (*on_jht_ad_board_one_shot_rpm)(void *ud, uint16_t rpm);
} jht_ad_board_handler_callbacks_t;

/* ============================================================================
 * Lifecycle
 * ============================================================================ */

jht_ad_board_handler_t* jht_ad_board_handler_create(void);
void                    jht_ad_board_handler_destroy(jht_ad_board_handler_t *h);

void jht_ad_board_handler_callbacks_init(jht_ad_board_handler_callbacks_t *cb);
void jht_ad_board_handler_set_callbacks(jht_ad_board_handler_t *h, const jht_ad_board_handler_callbacks_t *cb);
void jht_ad_board_handler_unregister_callbacks(jht_ad_board_handler_t *h);

/* ============================================================================
 * Start / Stop
 * ============================================================================ */

/**
 * @brief Start the JhtADboard handler with LCB communication
 *
 * Wires internal lcb_comm callbacks (overwrites any previous registration) and
 * starts the 400ms heartbeat thread.
 *
 * NOTE: Caller must already have started lcb_comm (lcb_comm_start). This
 * function only registers callbacks — it does NOT start serial communication.
 *
 * @param h     JhtADboard handler instance
 * @param comm  LCB communication instance (NOT owned)
 * @return true on success
 */
bool jht_ad_board_handler_start(jht_ad_board_handler_t *h, lcb_comm_t *comm);

void jht_ad_board_handler_stop(jht_ad_board_handler_t *h);

/* ============================================================================
 * LCB Type Injection (for setJhtADboardWattOrResistance's machine-type branch)
 * ============================================================================ */

void jht_ad_board_handler_set_lcb_type(jht_ad_board_handler_t *h, LcbType_t type);

/* ============================================================================
 * TX Commands
 * ============================================================================ */

/* Java: setJhtADboardWattOrResistance() — only sends (PWM percent) when the
 * injected LCB type is LCB_TYPE_JHT_AD_BOARD; any other type is a no-op,
 * matching Java's switch-default `return`. */
void jht_ad_board_handler_set_watt_or_resistance(jht_ad_board_handler_t *h, uint16_t value);

/* Java: setJhtADboardGearRatio() — unconditional, no LCB-type gate. */
void jht_ad_board_handler_set_gear_ratio(jht_ad_board_handler_t *h, uint16_t ratio);

/* ============================================================================
 * One-shot Getter
 * ============================================================================ */

void jht_ad_board_handler_get_one_shot_rpm(jht_ad_board_handler_t *h);

/* ============================================================================
 * Auto-reply Toggle
 * ============================================================================ */

void jht_ad_board_handler_set_auto_reply_rpm(jht_ad_board_handler_t *h, bool enable);

/* ============================================================================
 * E-Stop Revert
 * ============================================================================ */

/**
 * @brief Manual "EStop Revert" for when this handler already owns lcb_comm
 * (post-handshake — general_handler_estop_revert_lcb_initial() only works
 * before general_handler hands off to jht_ad_board_handler; see the matching
 * doc comment in bike_handler.h for the full explanation). LCB_TYPE_JHT_AD_BOARD
 * falls into general_handler.c's map_lcb_type_to_category() default branch
 * (MACHINE_CATEGORY_BIKE), so this re-sends the same single SetLCBInitial
 * command that category's handshake uses, highest-priority since the LCB is
 * already known-alive, then calls lcb_comm_proc_estop_unlock() for the local
 * UI notification. No-op if not started (comm == NULL).
 */
void jht_ad_board_handler_estop_revert_lcb_initial(jht_ad_board_handler_t *h);

#ifdef __cplusplus
}
#endif

#endif /* JHT_AD_BOARD_HANDLER_H */
