#include "gtest/gtest.h"

extern "C" {
    #include "lcb_parser.h"
    #include "lcb_packet.h"
    #include "lcb_crc8.h"
    #include "lcb_commands.h"
}

// Helper: build a valid RX packet buffer and return its total length.
// Format: [START_BYTE_RX, status, command, data_len, data..., CRC]
static size_t build_rx_packet(uint8_t *buf, size_t buf_size,
                              uint8_t status, uint8_t command,
                              const uint8_t *data, uint8_t data_len) {
    size_t total = 4 + data_len + 1; // header(4) + data + crc(1)
    if (total > buf_size) return 0;

    buf[0] = LCB_PACKET_START_BYTE_RX;
    buf[1] = status;
    buf[2] = command;
    buf[3] = data_len;
    for (uint8_t i = 0; i < data_len; i++) {
        buf[4 + i] = data[i];
    }
    // CRC over everything except the CRC byte itself
    buf[4 + data_len] = lcb_crc8_calculate(buf, 4 + data_len);
    return total;
}

// Helper: feed an entire buffer into the parser byte-by-byte,
// return the result of the last byte.
static lcb_parse_result_t feed_buffer(lcb_parser_t *parser,
                                      const uint8_t *buf, size_t len) {
    lcb_parse_result_t result = LCB_PARSE_INCOMPLETE;
    for (size_t i = 0; i < len; i++) {
        result = lcb_parser_parse_byte(parser, buf[i]);
    }
    return result;
}

// Test fixture
class LcbParserTest : public ::testing::Test {
protected:
    lcb_parser_t parser;

    void SetUp() override {
        lcb_parser_init(&parser);
    }

    void TearDown() override {
        lcb_parser_free(&parser);
    }
};

// ============================================================================
// Initialization tests
// ============================================================================

TEST_F(LcbParserTest, Initialization) {
    ASSERT_EQ(parser.parse_state, LCB_PARSE_STATE_START_BYTE);
    ASSERT_EQ(parser.parse_result, LCB_PARSE_INCOMPLETE);
    ASSERT_EQ(parser.parse_field_index, 0);
}

// ============================================================================
// Valid packet parsing
// ============================================================================

TEST_F(LcbParserTest, ParseValidPacketNoData) {
    // Packet: command=0x70 (SET_INITIALIZE), status=0x00, no data
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x70, NULL, 0);
    ASSERT_GT(len, 0);

    lcb_parse_result_t result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);

    // Verify parsed fields
    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0x70);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0x00);

    uint8_t data_len;
    lcb_parser_get_data(&parser, &data_len);
    ASSERT_EQ(data_len, 0);
}

TEST_F(LcbParserTest, ParseValidPacketWithData) {
    // Packet: command=0x72 (GET_ERROR_CODE), status=0x04, data=[0xAA, 0xBB]
    uint8_t data[] = {0xAA, 0xBB};
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x04, 0x72, data, 2);
    ASSERT_GT(len, 0);

    lcb_parse_result_t result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);

    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0x72);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0x04);

    uint8_t data_len;
    const uint8_t *parsed_data = lcb_parser_get_data(&parser, &data_len);
    ASSERT_EQ(data_len, 2);
    ASSERT_NE(parsed_data, nullptr);
    ASSERT_EQ(parsed_data[0], 0xAA);
    ASSERT_EQ(parsed_data[1], 0xBB);
}

TEST_F(LcbParserTest, ParseValidPacketLargeData) {
    // Packet with 20 bytes of data
    uint8_t data[20];
    for (int i = 0; i < 20; i++) {
        data[i] = (uint8_t)(i + 1);
    }

    uint8_t buf[64];
    size_t len = build_rx_packet(buf, sizeof(buf), 0xFF, 0x50, data, 20);
    ASSERT_GT(len, 0);

    lcb_parse_result_t result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);

    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0x50);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0xFF);

    uint8_t data_len;
    const uint8_t *parsed_data = lcb_parser_get_data(&parser, &data_len);
    ASSERT_EQ(data_len, 20);
    for (int i = 0; i < 20; i++) {
        ASSERT_EQ(parsed_data[i], (uint8_t)(i + 1));
    }
}

// ============================================================================
// Intermediate state tests
// ============================================================================

TEST_F(LcbParserTest, ParseByteByByte) {
    uint8_t data[] = {0x11, 0x22};
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0xAC, data, 2);

    // All bytes except the last should return INCOMPLETE
    for (size_t i = 0; i < len - 1; i++) {
        lcb_parse_result_t result = lcb_parser_parse_byte(&parser, buf[i]);
        if (i == 0) {
            // Start byte => INCOMPLETE
            ASSERT_EQ(result, LCB_PARSE_INCOMPLETE) << "Failed at byte " << i;
        } else {
            ASSERT_EQ(result, LCB_PARSE_INCOMPLETE) << "Failed at byte " << i;
        }
    }

    // Last byte (CRC) should return SUCCESS
    lcb_parse_result_t result = lcb_parser_parse_byte(&parser, buf[len - 1]);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
}

// ============================================================================
// Error cases
// ============================================================================

TEST_F(LcbParserTest, ParseInvalidStartByte) {
    // Feed a non-start byte first
    lcb_parse_result_t result = lcb_parser_parse_byte(&parser, 0x00);
    ASSERT_EQ(result, LCB_PARSE_INVALID_START_BYTE);

    result = lcb_parser_parse_byte(&parser, 0xFF);
    ASSERT_EQ(result, LCB_PARSE_INVALID_START_BYTE);

    result = lcb_parser_parse_byte(&parser, 0x42);
    ASSERT_EQ(result, LCB_PARSE_INVALID_START_BYTE);
}

TEST_F(LcbParserTest, ParseInvalidCrc) {
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x70, NULL, 0);

    // Corrupt the CRC byte
    buf[len - 1] ^= 0xFF;

    lcb_parse_result_t result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_INVALID_CRC);
}

TEST_F(LcbParserTest, ParseInvalidCrcWithData) {
    uint8_t data[] = {0xAA, 0xBB};
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x72, data, 2);

    // Corrupt the CRC byte
    buf[len - 1] = 0x00;

    lcb_parse_result_t result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_INVALID_CRC);
}

// ============================================================================
// Reset and re-parse
// ============================================================================

TEST_F(LcbParserTest, ResetAndReparse) {
    // Parse a valid packet
    uint8_t data1[] = {0x11};
    uint8_t buf1[16];
    size_t len1 = build_rx_packet(buf1, sizeof(buf1), 0x00, 0x71, data1, 1);

    lcb_parse_result_t result = feed_buffer(&parser, buf1, len1);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0x71);

    // Reset parser
    lcb_parser_reset(&parser);
    ASSERT_EQ(parser.parse_state, LCB_PARSE_STATE_START_BYTE);

    // Parse a different packet
    uint8_t data2[] = {0x22, 0x33};
    uint8_t buf2[16];
    size_t len2 = build_rx_packet(buf2, sizeof(buf2), 0x04, 0x73, data2, 2);

    result = feed_buffer(&parser, buf2, len2);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0x73);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0x04);

    uint8_t data_len;
    const uint8_t *parsed_data = lcb_parser_get_data(&parser, &data_len);
    ASSERT_EQ(data_len, 2);
    ASSERT_EQ(parsed_data[0], 0x22);
    ASSERT_EQ(parsed_data[1], 0x33);
}

// ============================================================================
// Sequential packet parsing
// ============================================================================

TEST_F(LcbParserTest, ParseMultiplePacketsSequentially) {
    // Build and parse packet 1
    uint8_t buf1[16];
    size_t len1 = build_rx_packet(buf1, sizeof(buf1), 0x00, 0x70, NULL, 0);

    lcb_parse_result_t result = feed_buffer(&parser, buf1, len1);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0x70);

    // Reset for next packet
    lcb_parser_reset(&parser);

    // Build and parse packet 2
    uint8_t data2[] = {0x01, 0x02};
    uint8_t buf2[16];
    size_t len2 = build_rx_packet(buf2, sizeof(buf2), 0x40, 0xF1, data2, 2);

    result = feed_buffer(&parser, buf2, len2);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0xF1);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0x40);

    // Reset for next packet
    lcb_parser_reset(&parser);

    // Build and parse packet 3
    uint8_t buf3[16];
    size_t len3 = build_rx_packet(buf3, sizeof(buf3), 0x80, 0x73, NULL, 0);

    result = feed_buffer(&parser, buf3, len3);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0x73);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0x80);
}

// ============================================================================
// Garbage data before valid packet
// ============================================================================

TEST_F(LcbParserTest, SkipGarbageBeforeValidPacket) {
    // Feed some garbage bytes (non-start bytes)
    lcb_parse_result_t result;
    result = lcb_parser_parse_byte(&parser, 0x55);
    ASSERT_EQ(result, LCB_PARSE_INVALID_START_BYTE);
    result = lcb_parser_parse_byte(&parser, 0xAA);
    ASSERT_EQ(result, LCB_PARSE_INVALID_START_BYTE);
    result = lcb_parser_parse_byte(&parser, 0x00);
    ASSERT_EQ(result, LCB_PARSE_INVALID_START_BYTE);

    // Now feed a valid packet - parser should still be in START_BYTE state
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x70, NULL, 0);

    result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_command(&parser), (lcb_command_t)0x70);
}

// ============================================================================
// Accessor function tests
// ============================================================================

TEST_F(LcbParserTest, GetPacketReturnsValidPointer) {
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x71, NULL, 0);

    feed_buffer(&parser, buf, len);

    const lcb_packet_t *pkt = lcb_parser_get_packet(&parser);
    ASSERT_NE(pkt, nullptr);
    ASSERT_EQ(pkt->command, (lcb_command_t)0x71);
}

TEST_F(LcbParserTest, GetDataWithNullLength) {
    uint8_t data[] = {0x11};
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x71, data, 1);

    feed_buffer(&parser, buf, len);

    // Passing NULL for length should not crash
    const uint8_t *parsed_data = lcb_parser_get_data(&parser, NULL);
    ASSERT_NE(parsed_data, nullptr);
    ASSERT_EQ(parsed_data[0], 0x11);
}

// ============================================================================
// Clone packet test
// ============================================================================

TEST_F(LcbParserTest, ClonePacket) {
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x04, 0x72, data, 3);

    lcb_parse_result_t result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);

    lcb_packet_t cloned;
    lcb_packet_init(&cloned);
    ASSERT_EQ(lcb_parser_clone_packet(&cloned, &parser), 0);

    ASSERT_EQ(lcb_packet_get_command(&cloned), (lcb_command_t)0x72);

    uint8_t clone_len;
    const uint8_t *clone_data = lcb_packet_get_data(&cloned, &clone_len);
    ASSERT_EQ(clone_len, 3);
    ASSERT_EQ(clone_data[0], 0xAA);
    ASSERT_EQ(clone_data[1], 0xBB);
    ASSERT_EQ(clone_data[2], 0xCC);

    lcb_packet_free(&cloned);
}

// ============================================================================
// Null pointer safety
// ============================================================================

TEST_F(LcbParserTest, NullPointerSafety) {
    ASSERT_EQ(lcb_parser_init(NULL), -1);
    ASSERT_EQ(lcb_parser_parse_byte(NULL, 0x01), LCB_PARSE_UNKNOWN_ERROR);
    ASSERT_EQ(lcb_parser_get_packet(NULL), nullptr);
    ASSERT_EQ(lcb_parser_get_rx_status(NULL), 0);
    ASSERT_EQ(lcb_parser_get_command(NULL), LCB_CMD_GET_STATUS);

    uint8_t len;
    ASSERT_EQ(lcb_parser_get_data(NULL, &len), nullptr);
    ASSERT_EQ(len, 0);

    lcb_packet_t dest;
    lcb_packet_init(&dest);
    ASSERT_EQ(lcb_parser_clone_packet(&dest, NULL), -1);
    ASSERT_EQ(lcb_parser_clone_packet(NULL, &parser), -1);
    lcb_packet_free(&dest);
}

// ============================================================================
// Status byte variations
// ============================================================================

TEST_F(LcbParserTest, ParseWithVariousStatusBytes) {
    // Status with motor running (bit 2)
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x04, 0x71, NULL, 0);
    lcb_parse_result_t result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0x04);

    lcb_parser_reset(&parser);

    // Status with error (bit 6)
    len = build_rx_packet(buf, sizeof(buf), 0x40, 0x71, NULL, 0);
    result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0x40);

    lcb_parser_reset(&parser);

    // Status with all bits set
    len = build_rx_packet(buf, sizeof(buf), 0xFF, 0x71, NULL, 0);
    result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);
    ASSERT_EQ(lcb_parser_get_rx_status(&parser), 0xFF);
}

// ============================================================================
// Memory Management Tests
// ============================================================================

// Test that reset frees packet data
TEST_F(LcbParserTest, ResetFreesPacketData) {
    // Parse a packet with data
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x72, data, 3);

    lcb_parse_result_t result = feed_buffer(&parser, buf, len);
    ASSERT_EQ(result, LCB_PARSE_SUCCESS);

    // Parser should have allocated data
    ASSERT_NE(parser.packet.data, nullptr);
    ASSERT_EQ(parser.packet.data_length, 3);

    // Reset should free packet data
    lcb_parser_reset(&parser);
    ASSERT_EQ(parser.packet.data, nullptr);
    ASSERT_EQ(parser.packet.data_length, 0);
    ASSERT_EQ(parser.parse_state, LCB_PARSE_STATE_START_BYTE);
}

// Test sequential parses with reset between each — no memory leak
TEST_F(LcbParserTest, SequentialParsesWithReset_NoLeak) {
    for (int i = 0; i < 3; i++) {
        uint8_t data[4] = {(uint8_t)(i+1), (uint8_t)(i+2), (uint8_t)(i+3), (uint8_t)(i+4)};
        uint8_t buf[16];
        size_t len = build_rx_packet(buf, sizeof(buf), (uint8_t)i, (uint8_t)(0x70 + i), data, 4);
        ASSERT_GT(len, 0u);

        lcb_parse_result_t result = feed_buffer(&parser, buf, len);
        ASSERT_EQ(result, LCB_PARSE_SUCCESS);
        ASSERT_EQ(parser.packet.data_length, 4);
        ASSERT_EQ(parser.packet.data[0], (uint8_t)(i+1));

        // Reset frees previous packet data before re-init
        lcb_parser_reset(&parser);
        ASSERT_EQ(parser.packet.data, nullptr);
        ASSERT_EQ(parser.packet.data_length, 0);
    }
}

// Helper callback for ParseDataCallback_PacketClone test
static lcb_packet_t g_cloned_packet;
static int g_clone_callback_count;

static void clone_callback(const lcb_packet_t *packet, void *user_data) {
    (void)user_data;
    lcb_packet_init(&g_cloned_packet);
    lcb_packet_clone(&g_cloned_packet, packet);
    g_clone_callback_count++;
}

// Test lcb_parser_parse_data with callback that clones packet
TEST_F(LcbParserTest, ParseDataCallback_PacketClone) {
    g_clone_callback_count = 0;

    uint8_t data[] = {0x11, 0x22};
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x72, data, 2);
    ASSERT_GT(len, 0u);

    int count = lcb_parser_parse_data(&parser, buf, (uint32_t)len, clone_callback, NULL);
    ASSERT_EQ(count, 1);
    ASSERT_EQ(g_clone_callback_count, 1);

    // Cloned packet should be independent
    ASSERT_EQ(g_cloned_packet.command, (lcb_command_t)0x72);
    ASSERT_EQ(g_cloned_packet.data_length, 2);
    ASSERT_EQ(g_cloned_packet.data[0], 0x11);
    ASSERT_EQ(g_cloned_packet.data[1], 0x22);

    // Free cloned packet — parser's packet was already reset by parse_data
    lcb_packet_free(&g_cloned_packet);
}

// Test lcb_parser_parse_data with NULL callback — counts but doesn't crash
TEST_F(LcbParserTest, ParseDataNullCallback_NoCrash) {
    uint8_t buf[16];
    size_t len = build_rx_packet(buf, sizeof(buf), 0x00, 0x70, NULL, 0);

    int count = lcb_parser_parse_data(&parser, buf, (uint32_t)len, NULL, NULL);
    ASSERT_EQ(count, 1);  // Packet counted even with NULL callback
}

// Test lcb_parser_parse_data with NULL data — returns error
TEST_F(LcbParserTest, ParseDataNullData_ReturnsError) {
    int result = lcb_parser_parse_data(&parser, NULL, 5, clone_callback, NULL);
    ASSERT_EQ(result, -1);
}
