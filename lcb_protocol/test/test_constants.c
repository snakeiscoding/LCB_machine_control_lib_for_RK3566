/**
 * @file test_constants.c
 * @brief Test program for LCB Constants
 *
 * This program tests the functionality of lcb_constants module.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lcb_constants.h"
#include "lcb_commands.h"

/* Test result counters */
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            printf("  ✓ %s\n", msg); \
            tests_passed++; \
        } else { \
            printf("  ✗ %s\n", msg); \
            tests_failed++; \
        } \
    } while(0)

/**
 * @brief Test status bit functions
 */
void test_status_bits(void)
{
    printf("\n=== Testing Status Bits ===\n");

    /* Test status bit checking */
    uint8_t status = LCB_STATUS_BIT_MAIN_MOTOR_ACTION | LCB_STATUS_BIT_LCB_ERROR;

    TEST_ASSERT(lcb_status_bit_is_set(status, LCB_STATUS_BIT_MAIN_MOTOR_ACTION),
                "Main motor bit is set");
    TEST_ASSERT(lcb_status_bit_is_set(status, LCB_STATUS_BIT_LCB_ERROR),
                "LCB error bit is set");
    TEST_ASSERT(!lcb_status_bit_is_set(status, LCB_STATUS_BIT_CALIBRATION_ACTION),
                "Calibration bit is not set");

    /* Test status bit names */
    const char *name = lcb_status_bit_get_name(LCB_STATUS_BIT_MAIN_MOTOR_ACTION);
    TEST_ASSERT(strcmp(name, "MAIN_MOTOR_ACTION") == 0,
                "Status bit name retrieval");

    /* Test status to string */
    char buffer[256];
    int result = lcb_status_to_string(status, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "Status to string conversion");
    TEST_ASSERT(strstr(buffer, "MOTOR_ON") != NULL, "Status string contains MOTOR_ON");
    TEST_ASSERT(strstr(buffer, "LCB_ERROR") != NULL, "Status string contains LCB_ERROR");

    printf("  Status string: %s\n", buffer);
}

/**
 * @brief Test data size functions
 */
void test_data_sizes(void)
{
    printf("\n=== Testing Data Sizes ===\n");

    /* Test expected data sizes */
    TEST_ASSERT(lcb_get_expected_data_size(0x72) == 2,
                "GET_ERROR_CODE expects 2 bytes");
    TEST_ASSERT(lcb_get_expected_data_size(0x73) == 2,
                "GET_VERSION expects 2 bytes");
    TEST_ASSERT(lcb_get_expected_data_size(0x78) == 27,
                "GET_DCI_VERSION expects 27 bytes");
    TEST_ASSERT(lcb_get_expected_data_size(0x96) == 12,
                "GET_MAIN_MOTOR_INFO expects 12 bytes");
    TEST_ASSERT(lcb_get_expected_data_size(0x70) == 0,
                "SET_INITIALIZE expects 0 bytes");

    /* Test data size validation */
    TEST_ASSERT(lcb_validate_data_size(0x72, 2) == 1,
                "Valid data size for GET_ERROR_CODE");
    TEST_ASSERT(lcb_validate_data_size(0x72, 3) == 0,
                "Invalid data size for GET_ERROR_CODE");
    TEST_ASSERT(lcb_validate_data_size(0x73, 2) == 1,
                "Valid data size for GET_VERSION");
}

/**
 * @brief Test all defined constants
 */
void test_constants_defined(void)
{
    printf("\n=== Testing Constants Defined ===\n");

    /* Test receive data size constants */
    TEST_ASSERT(LCB_RECEIVE_DATA_LENGTH_LIMITED_MAX == 255,
                "Max data length is 255");
    TEST_ASSERT(LCB_RECEIVE_DATA_SIZE_ERROR_CODE == 2,
                "Error code size is 2");
    TEST_ASSERT(LCB_RECEIVE_DATA_SIZE_DCI_VERSION == 27,
                "DCI version size is 27");
    TEST_ASSERT(LCB_RECEIVE_DATA_SIZE_DCI_ENV == 84,
                "DCI env size is 84");

    /* Test command constants */
    TEST_ASSERT(LCB_RTN_HAVE_0X50_DATA == 0xFF00,
                "0x50 support flag is 0xFF00");
    TEST_ASSERT(LCB_CMD_EUPS_MODE_NOTIFY_ENTER_EUPS == 0xFF,
                "Enter EUPs is 0xFF");
    TEST_ASSERT(LCB_OPERATING_STOP == 0x00,
                "Operating stop is 0x00");
    TEST_ASSERT(LCB_OPERATING_ACTION == 0x01,
                "Operating action is 0x01");

    /* Test status bit values */
    TEST_ASSERT(LCB_STATUS_BIT_INCLINE_DOWN_ACTION == 0x01,
                "Incline down bit is 0x01");
    TEST_ASSERT(LCB_STATUS_BIT_INCLINE_UP_ACTION == 0x02,
                "Incline up bit is 0x02");
    TEST_ASSERT(LCB_STATUS_BIT_MAIN_MOTOR_ACTION == 0x04,
                "Main motor bit is 0x04");
    TEST_ASSERT(LCB_STATUS_BIT_CALIBRATION_ACTION == 0x80,
                "Calibration bit is 0x80");
}

/**
 * @brief Test multiple status bits
 */
void test_multiple_status_bits(void)
{
    printf("\n=== Testing Multiple Status Bits ===\n");

    /* Test all bits set */
    uint8_t all_bits = 0xFF;
    char buffer[512];
    lcb_status_to_string(all_bits, buffer, sizeof(buffer));
    printf("  All bits set: %s\n", buffer);
    TEST_ASSERT(strlen(buffer) > 50, "All bits string is comprehensive");

    /* Test no bits set */
    uint8_t no_bits = 0x00;
    lcb_status_to_string(no_bits, buffer, sizeof(buffer));
    printf("  No bits set: %s\n", buffer);
    TEST_ASSERT(strstr(buffer, "NONE") != NULL, "No bits shows NONE");

    /* Test selective bits */
    uint8_t selective = LCB_STATUS_BIT_INCLINE_UP_ACTION |
                       LCB_STATUS_BIT_MAIN_MOTOR_ACTION |
                       LCB_STATUS_BIT_CALIBRATION_ACTION;
    lcb_status_to_string(selective, buffer, sizeof(buffer));
    printf("  Selective bits: %s\n", buffer);
    TEST_ASSERT(strstr(buffer, "INCLINE_UP") != NULL, "Contains INCLINE_UP");
    TEST_ASSERT(strstr(buffer, "MOTOR_ON") != NULL, "Contains MOTOR_ON");
    TEST_ASSERT(strstr(buffer, "CALIBRATING") != NULL, "Contains CALIBRATING");
}

/**
 * @brief Test data size for all command categories
 */
void test_command_categories(void)
{
    printf("\n=== Testing Command Categories ===\n");

    int system_cmds = 0, motor_cmds = 0, dci_cmds = 0;
    int device_cmds = 0, ecb_cmds = 0, bike_cmds = 0, cal_cmds = 0;

    /* Count commands by category */
    for (int cmd = 0; cmd <= 0xFF; cmd++) {
        int size = lcb_get_expected_data_size(cmd);
        if (size >= 0 || size == -1) {
            if (cmd >= 0x70 && cmd <= 0x7F) system_cmds++;
            else if (cmd >= 0xF0 && cmd <= 0xFF) motor_cmds++;
            else if (cmd >= 0x90 && cmd <= 0x9F) motor_cmds++;
            else if (cmd >= 0xA0 && cmd <= 0xAF) dci_cmds++;
            else if (cmd >= 0x50 && cmd <= 0x5F) device_cmds++;
            else if (cmd >= 0x60 && cmd <= 0x6F) ecb_cmds++;
            else if (cmd >= 0x80 && cmd <= 0x8F) bike_cmds++;
            else if (cmd >= 0x40 && cmd <= 0x4F) cal_cmds++;
        }
    }

    printf("  System commands: %d\n", system_cmds);
    printf("  Motor commands: %d\n", motor_cmds);
    printf("  DCI commands: %d\n", dci_cmds);
    printf("  Device commands: %d\n", device_cmds);
    printf("  ECB commands: %d\n", ecb_cmds);
    printf("  Bike commands: %d\n", bike_cmds);
    printf("  Calibration commands: %d\n", cal_cmds);

    TEST_ASSERT(system_cmds >= 10, "At least 10 system commands defined");
    TEST_ASSERT(motor_cmds >= 15, "At least 15 motor commands defined");
    TEST_ASSERT(dci_cmds >= 5, "At least 5 DCI commands defined");
}

/**
 * @brief Main test program
 */
int main(void)
{
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        LCB Constants Test Suite                           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    /* Run all tests */
    test_status_bits();
    test_data_sizes();
    test_constants_defined();
    test_multiple_status_bits();
    test_command_categories();

    /* Print summary */
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                            ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Tests Passed: %-4d                                        ║\n", tests_passed);
    printf("║  Tests Failed: %-4d                                        ║\n", tests_failed);
    printf("╚════════════════════════════════════════════════════════════╝\n");

    if (tests_failed == 0) {
        printf("\n✓ All constants tests passed!\n\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed.\n\n");
        return 1;
    }
}
