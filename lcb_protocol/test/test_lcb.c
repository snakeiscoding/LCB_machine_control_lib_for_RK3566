/**
 * @file test_lcb.c
 * @brief LCB Protocol Test Program
 *
 * This test program demonstrates basic usage of the LCB protocol library.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lcb_crc8.h"
#include "lcb_packet.h"
#include "lcb_parser.h"
#include "lcb_packet_queue.h"
#include "lcb_utils.h"

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
 * @brief Test CRC8 calculation
 */
void test_crc8(void)
{
    printf("\n=== Testing CRC8 ===\n");

    /* Expected values: CRC-8, poly 0x31, init 0x00, no reflection, no final XOR */
    uint8_t data1[] = {0x00, 0xFF, 0x71, 0x00};
    uint8_t crc1 = lcb_crc8_calculate(data1, 4);
    TEST_ASSERT(crc1 == 0x84, "CRC8 calculation test 1");

    uint8_t data2[] = {0x01, 0x00, 0x71, 0x00};
    uint8_t crc2 = lcb_crc8_calculate(data2, 4);
    TEST_ASSERT(crc2 == 0x54, "CRC8 calculation test 2");

    /* Test incremental calculation */
    lcb_crc8_context_t ctx;
    lcb_crc8_reset(&ctx);
    for (size_t i = 0; i < 4; i++) {
        lcb_crc8_feed_byte(&ctx, data1[i]);
    }
    uint8_t crc3 = lcb_crc8_get_value(&ctx);
    TEST_ASSERT(crc3 == crc1, "CRC8 incremental calculation");
}

/**
 * @brief Test packet building
 */
void test_packet_build(void)
{
    printf("\n=== Testing Packet Building ===\n");

    lcb_packet_t packet;
    lcb_packet_init(&packet);

    /* Set command */
    lcb_packet_set_command(&packet, LCB_CMD_GET_STATUS);
    TEST_ASSERT(lcb_packet_get_command(&packet) == LCB_CMD_GET_STATUS,
                "Packet command set/get");

    /* Set data */
    uint8_t data[] = {0x12, 0x34};
    lcb_packet_set_data(&packet, data, 2);
    uint8_t length;
    const uint8_t *pdata = lcb_packet_get_data(&packet, &length);
    TEST_ASSERT(length == 2 && memcmp(pdata, data, 2) == 0,
                "Packet data set/get");

    /* Build TX packet */
    uint8_t buffer[256];
    size_t packet_size;
    int result = lcb_packet_build_tx(&packet, buffer, sizeof(buffer), &packet_size);
    TEST_ASSERT(result == 0 && packet_size == 7,
                "Packet TX build");

    /* Verify packet structure */
    TEST_ASSERT(buffer[0] == 0x00, "Start byte");
    TEST_ASSERT(buffer[1] == 0xFF, "Status byte");
    TEST_ASSERT(buffer[2] == 0x71, "Command byte");
    TEST_ASSERT(buffer[3] == 0x02, "Length byte");
    TEST_ASSERT(buffer[4] == 0x12 && buffer[5] == 0x34, "Data bytes");

    /* Verify CRC */
    uint8_t expected_crc = lcb_crc8_calculate(buffer, 6);
    TEST_ASSERT(buffer[6] == expected_crc, "CRC byte");

    lcb_packet_free(&packet);
}

/**
 * @brief Test packet parsing
 */
void test_packet_parse(void)
{
    printf("\n=== Testing Packet Parsing ===\n");

    lcb_parser_t parser;
    lcb_parser_init(&parser);

    /* Build a test packet */
    uint8_t test_packet[] = {
        0x01,       /* Start byte (RX) */
        0x04,       /* Status */
        0x71,       /* Command (GET_STATUS) */
        0x02,       /* Length */
        0xAB, 0xCD, /* Data */
        0x00        /* CRC (placeholder) */
    };

    /* Calculate correct CRC */
    test_packet[6] = lcb_crc8_calculate(test_packet, 6);

    /* Parse packet byte by byte */
    lcb_parse_result_t result = LCB_PARSE_INCOMPLETE;
    for (size_t i = 0; i < sizeof(test_packet); i++) {
        result = lcb_parser_parse_byte(&parser, test_packet[i]);
        if (i < sizeof(test_packet) - 1) {
            TEST_ASSERT(result == LCB_PARSE_INCOMPLETE,
                       "Parser returns incomplete before last byte");
        }
    }

    TEST_ASSERT(result == LCB_PARSE_SUCCESS, "Parser returns success");

    /* Verify parsed packet */
    const lcb_packet_t *packet = lcb_parser_get_packet(&parser);
    TEST_ASSERT(packet != NULL, "Parser has packet");
    TEST_ASSERT(lcb_parser_get_command(&parser) == LCB_CMD_GET_STATUS,
                "Parsed command correct");
    TEST_ASSERT(lcb_parser_get_rx_status(&parser) == 0x04,
                "Parsed status correct");

    uint8_t data_len;
    const uint8_t *data = lcb_parser_get_data(&parser, &data_len);
    TEST_ASSERT(data_len == 2 && data[0] == 0xAB && data[1] == 0xCD,
                "Parsed data correct");

    lcb_parser_free(&parser);
}

/**
 * @brief Test packet queue
 */
void test_packet_queue(void)
{
    printf("\n=== Testing Packet Queue ===\n");

    lcb_packet_queue_t queue;
    lcb_packet_queue_init(&queue);

    TEST_ASSERT(lcb_packet_queue_is_empty(&queue), "Queue initially empty");

    /* Add packets with different priorities */
    lcb_packet_t packet1, packet2, packet3;
    lcb_packet_init(&packet1);
    lcb_packet_init(&packet2);
    lcb_packet_init(&packet3);

    lcb_packet_set_command(&packet1, LCB_CMD_GET_STATUS);
    lcb_packet_set_priority(&packet1, LCB_PRIORITY_LOW);

    lcb_packet_set_command(&packet2, LCB_CMD_GET_VERSION);
    lcb_packet_set_priority(&packet2, LCB_PRIORITY_HIGHEST);

    lcb_packet_set_command(&packet3, LCB_CMD_GET_ERROR_CODE);
    lcb_packet_set_priority(&packet3, LCB_PRIORITY_NORMAL);

    lcb_packet_queue_add(&queue, &packet1);
    lcb_packet_queue_add(&queue, &packet2);
    lcb_packet_queue_add(&queue, &packet3);

    TEST_ASSERT(lcb_packet_queue_size(&queue) == 3, "Queue has 3 packets");

    /* Remove packets - should come out in priority order */
    lcb_packet_t removed;
    lcb_packet_queue_remove(&queue, &removed);
    TEST_ASSERT(lcb_packet_get_command(&removed) == LCB_CMD_GET_VERSION,
                "Highest priority packet removed first");
    lcb_packet_free(&removed);

    lcb_packet_queue_remove(&queue, &removed);
    TEST_ASSERT(lcb_packet_get_command(&removed) == LCB_CMD_GET_ERROR_CODE,
                "Normal priority packet removed second");
    lcb_packet_free(&removed);

    lcb_packet_queue_remove(&queue, &removed);
    TEST_ASSERT(lcb_packet_get_command(&removed) == LCB_CMD_GET_STATUS,
                "Low priority packet removed last");
    lcb_packet_free(&removed);

    TEST_ASSERT(lcb_packet_queue_is_empty(&queue), "Queue empty after removing all");

    lcb_packet_free(&packet1);
    lcb_packet_free(&packet2);
    lcb_packet_free(&packet3);
    lcb_packet_queue_free(&queue);
}

/**
 * @brief Test utility functions
 */
void test_utils(void)
{
    printf("\n=== Testing Utilities ===\n");

    /* Test byte to uint16 conversion */
    uint8_t bytes16[] = {0x12, 0x34};
    uint16_t value16 = lcb_bytes_to_uint16_be(bytes16);
    TEST_ASSERT(value16 == 0x1234, "Bytes to uint16 conversion");

    /* Test uint16 to bytes conversion */
    uint8_t result16[2];
    lcb_uint16_to_bytes_be(0x5678, result16);
    TEST_ASSERT(result16[0] == 0x56 && result16[1] == 0x78,
                "Uint16 to bytes conversion");

    /* Test byte to uint32 conversion */
    uint8_t bytes32[] = {0x12, 0x34, 0x56, 0x78};
    uint32_t value32 = lcb_bytes_to_uint32_be(bytes32);
    TEST_ASSERT(value32 == 0x12345678, "Bytes to uint32 conversion");

    /* Test hex string conversion */
    uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    char hex_string[32];
    lcb_bytes_to_hex_string(data, 4, hex_string, sizeof(hex_string));
    TEST_ASSERT(strcmp(hex_string, "DE AD BE EF") == 0,
                "Bytes to hex string");
}

/**
 * @brief Main test program
 */
int main(void)
{
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║        LCB Protocol Library Test Suite                    ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    /* Run all tests */
    test_crc8();
    test_packet_build();
    test_packet_parse();
    test_packet_queue();
    test_utils();

    /* Print summary */
    printf("\n╔════════════════════════════════════════════════════════════╗\n");
    printf("║                    Test Summary                            ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Tests Passed: %-4d                                        ║\n", tests_passed);
    printf("║  Tests Failed: %-4d                                        ║\n", tests_failed);
    printf("╚════════════════════════════════════════════════════════════╝\n");

    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed.\n\n");
        return 1;
    }
}
