/**
 * @file simple_usage.c
 * @brief Simple LCB Protocol Usage Example
 *
 * This example demonstrates basic usage of the LCB protocol library
 * without actual serial port hardware.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lcb_comm.h"
#include "lcb_comm_callback.h"
#include "lcb_constants.h"

/* Callback function prototypes */
static void on_lcb_status_received(void *user_data, uint8_t lcb_status);
static void on_lcb_version_received(void *user_data, uint16_t version);
static void on_lcb_error_code_received(void *user_data, uint16_t error_code);
static void on_motor_rpm_received(void *user_data, uint16_t rpm);

/**
 * @brief Example callback for LCB status
 */
static void on_lcb_status_received(void *user_data, uint8_t lcb_status)
{
    (void)user_data;
    printf("📊 LCB Status received: 0x%02X\n", lcb_status);

    /* Check individual status bits */
    if (lcb_status & LCB_STATUS_BIT_MAIN_MOTOR_ACTION) {
        printf("   ⚡ Main motor is running\n");
    }
    if (lcb_status & LCB_STATUS_BIT_LCB_ERROR) {
        printf("   ⚠️  LCB has error\n");
    }
    if (lcb_status & LCB_STATUS_BIT_CALIBRATION_ACTION) {
        printf("   🔧 Calibration in progress\n");
    }
}

/**
 * @brief Example callback for LCB version
 */
static void on_lcb_version_received(void *user_data, uint16_t version)
{
    (void)user_data;
    printf("📦 LCB Version: %d.%02d\n", version / 100, version % 100);
}

/**
 * @brief Example callback for error code
 */
static void on_lcb_error_code_received(void *user_data, uint16_t error_code)
{
    (void)user_data;
    printf("❌ LCB Error Code: 0x%04X\n", error_code);
}

/**
 * @brief Example callback for motor RPM
 */
static void on_motor_rpm_received(void *user_data, uint16_t rpm)
{
    (void)user_data;
    printf("🏃 Motor RPM: %d\n", rpm);
}

/**
 * @brief Main example program
 */
int main(void)
{
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║          LCB Protocol Simple Usage Example                ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    /* Create LCB communication instance */
    printf("1. Creating LCB communication instance...\n");
    lcb_comm_t *comm = lcb_comm_create();
    if (comm == NULL) {
        fprintf(stderr, "Failed to create LCB communication instance\n");
        return 1;
    }
    printf("   ✓ LCB communication instance created\n\n");

    /* Setup callbacks */
    printf("2. Setting up callbacks...\n");
    lcb_comm_callbacks_t callbacks;
    lcb_comm_callbacks_init(&callbacks);

    callbacks.user_data = NULL; /* Can pass custom context here */
    callbacks.on_lcb_status = on_lcb_status_received;
    callbacks.on_lcb_version = on_lcb_version_received;
    callbacks.on_lcb_error_code = on_lcb_error_code_received;
    callbacks.on_lcb_motor_rpm_received = on_motor_rpm_received;

    if (lcb_comm_register_callbacks(comm, &callbacks) < 0) {
        fprintf(stderr, "Failed to register callbacks\n");
        lcb_comm_destroy(comm);
        return 1;
    }
    printf("   ✓ Callbacks registered\n\n");

    /* Note: In real application, you would open serial port here */
    printf("3. Opening serial port...\n");
    printf("   ⚠️  Note: Serial port implementation is platform-specific\n");
    printf("   ⚠️  This example demonstrates the API without hardware\n\n");

    /*
     * Example serial port opening (pseudo-code):
     *
     * serial_port_t *port = serial_port_open("/dev/ttyS0", 115200);
     * if (port == NULL) {
     *     fprintf(stderr, "Failed to open serial port\n");
     *     lcb_comm_destroy(comm);
     *     return 1;
     * }
     *
     * if (!lcb_comm_start(comm, port)) {
     *     fprintf(stderr, "Failed to start communication\n");
     *     serial_port_close(port);
     *     lcb_comm_destroy(comm);
     *     return 1;
     * }
     * printf("   ✓ Communication started\n\n");
     */

    /* Send some commands (examples) */
    printf("4. Sending LCB commands...\n");

    /* Initialize LCB */
    printf("   → Sending LCB Initialize command\n");
    lcb_comm_set_lcb_initial(comm);

    /* Get LCB status */
    printf("   → Sending Get LCB Status command\n");
    lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ALWAYS_ADD);

    /* Get LCB version */
    printf("   → Sending Get LCB Version command\n");
    lcb_comm_get_lcb_version(comm);

    /* Set motor RPM */
    printf("   → Sending Set Motor RPM command (1000 RPM)\n");
    lcb_comm_set_lcb_motor_rpm(comm, 1000);

    /* Set operating status */
    printf("   → Sending Set Operating Status command (START)\n");
    lcb_comm_set_lcb_operating_status(comm, LCB_OPERATING_ACTION);

    printf("   ✓ Commands queued for transmission\n\n");

    /* Get queue size */
    uint32_t queue_size = lcb_comm_get_tx_queue_size(comm);
    printf("5. TX Queue Status: %u packets waiting\n\n", queue_size);

    /* In real application, wait for responses and process callbacks */
    printf("6. Processing...\n");
    printf("   ⏳ In real application, callbacks would be invoked here\n");
    printf("   ⏳ as responses are received from LCB hardware\n\n");

    /* Simulate some processing time */
    printf("   Waiting 3 seconds...\n");
    sleep(3);

    /* Stop communication */
    printf("\n7. Stopping communication...\n");
    lcb_comm_stop(comm);
    printf("   ✓ Communication stopped\n\n");

    /* Cleanup */
    printf("8. Cleaning up...\n");
    lcb_comm_destroy(comm);
    printf("   ✓ Resources released\n\n");

    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    Example Complete                        ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");

    printf("📝 Next Steps:\n");
    printf("   1. Implement platform-specific serial port driver\n");
    printf("   2. Connect to actual LCB hardware\n");
    printf("   3. Implement all command handlers in lcb_comm.c\n");
    printf("   4. Add error handling and retry logic\n");
    printf("   5. Test with real hardware\n\n");

    return 0;
}
