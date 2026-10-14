#include "gtest/gtest.h"

extern "C" {
    #include "lcb_packet.h"
    #include "lcb_crc8.h"
    #include "lcb_commands.h"
}

// Test fixture for LcbPacket tests
class LcbPacketTest : public ::testing::Test {
protected:
    lcb_packet_t packet;

    void SetUp() override {
        lcb_packet_init(&packet);
    }

    void TearDown() override {
        lcb_packet_free(&packet);
    }
};

// Test packet initialization
TEST_F(LcbPacketTest, Initialization) {
    ASSERT_EQ(lcb_packet_get_priority(&packet), LCB_PRIORITY_NORMAL);
    ASSERT_EQ(lcb_packet_get_command(&packet), LCB_CMD_GET_STATUS);

    uint8_t length;
    const uint8_t* data = lcb_packet_get_data(&packet, &length);
    ASSERT_EQ(length, 0);
}

// Test setting and getting packet command
TEST_F(LcbPacketTest, SetGetCommand) {
    lcb_packet_set_command(&packet, (lcb_command_t)0xAB);
    ASSERT_EQ(lcb_packet_get_command(&packet), (lcb_command_t)0xAB);
}

// Test setting and getting packet data
TEST_F(LcbPacketTest, SetGetData) {
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};

    ASSERT_EQ(lcb_packet_set_data(&packet, test_data, 4), 0);

    uint8_t length;
    const uint8_t* data = lcb_packet_get_data(&packet, &length);

    ASSERT_EQ(length, 4);
    ASSERT_EQ(data[0], 0x01);
    ASSERT_EQ(data[1], 0x02);
    ASSERT_EQ(data[2], 0x03);
    ASSERT_EQ(data[3], 0x04);
}

// Test setting and getting priority
TEST_F(LcbPacketTest, SetGetPriority) {
    lcb_packet_set_priority(&packet, LCB_PRIORITY_HIGH);
    ASSERT_EQ(lcb_packet_get_priority(&packet), LCB_PRIORITY_HIGH);

    lcb_packet_set_priority(&packet, LCB_PRIORITY_LOW);
    ASSERT_EQ(lcb_packet_get_priority(&packet), LCB_PRIORITY_LOW);
}

// Test building TX packet
TEST_F(LcbPacketTest, BuildTxPacket) {
    lcb_packet_set_command(&packet, (lcb_command_t)0xAB);

    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    lcb_packet_set_data(&packet, test_data, 4);

    uint8_t buffer[256];
    size_t packet_size;

    ASSERT_EQ(lcb_packet_build_tx(&packet, buffer, sizeof(buffer), &packet_size), 0);

    // Expected: Start(1) + Status(1) + Cmd(1) + Len(1) + Data(4) + CRC(1) = 9 bytes
    ASSERT_EQ(packet_size, 9);
    ASSERT_EQ(buffer[0], LCB_PACKET_START_BYTE_TX);
    ASSERT_EQ(buffer[1], LCB_PACKET_STATUS_BYTE);
    ASSERT_EQ(buffer[2], 0xAB);
    ASSERT_EQ(buffer[3], 4);
    ASSERT_EQ(buffer[4], 0x01);
    ASSERT_EQ(buffer[5], 0x02);
    ASSERT_EQ(buffer[6], 0x03);
    ASSERT_EQ(buffer[7], 0x04);

    // Verify CRC
    uint8_t expected_crc = lcb_crc8_calculate(buffer, packet_size - 1);
    ASSERT_EQ(buffer[packet_size - 1], expected_crc);
}

// Test building TX packet with no data
TEST_F(LcbPacketTest, BuildTxPacketNoData) {
    lcb_packet_set_command(&packet, (lcb_command_t)0x70);

    uint8_t buffer[256];
    size_t packet_size;

    ASSERT_EQ(lcb_packet_build_tx(&packet, buffer, sizeof(buffer), &packet_size), 0);

    // Expected: Start(1) + Status(1) + Cmd(1) + Len(1) + CRC(1) = 5 bytes
    ASSERT_EQ(packet_size, 5);
    ASSERT_EQ(buffer[0], LCB_PACKET_START_BYTE_TX);
    ASSERT_EQ(buffer[1], LCB_PACKET_STATUS_BYTE);
    ASSERT_EQ(buffer[2], 0x70);
    ASSERT_EQ(buffer[3], 0);
}

// Test packet cloning
TEST_F(LcbPacketTest, ClonePacket) {
    lcb_packet_set_command(&packet, (lcb_command_t)0xAA);
    lcb_packet_set_priority(&packet, LCB_PRIORITY_HIGHEST);

    uint8_t test_data[] = {0x11, 0x22, 0x33};
    lcb_packet_set_data(&packet, test_data, 3);

    lcb_packet_t cloned_packet;
    lcb_packet_init(&cloned_packet);

    ASSERT_EQ(lcb_packet_clone(&cloned_packet, &packet), 0);

    ASSERT_EQ(lcb_packet_get_command(&cloned_packet), (lcb_command_t)0xAA);
    ASSERT_EQ(lcb_packet_get_priority(&cloned_packet), LCB_PRIORITY_HIGHEST);

    uint8_t length;
    const uint8_t* data = lcb_packet_get_data(&cloned_packet, &length);
    ASSERT_EQ(length, 3);
    ASSERT_EQ(data[0], 0x11);
    ASSERT_EQ(data[1], 0x22);
    ASSERT_EQ(data[2], 0x33);

    lcb_packet_free(&cloned_packet);
}

// Test packet equality
TEST_F(LcbPacketTest, PacketEquals) {
    lcb_packet_t packet1, packet2;

    lcb_packet_init(&packet1);
    lcb_packet_init(&packet2);

    lcb_packet_set_command(&packet1, (lcb_command_t)0x55);
    lcb_packet_set_command(&packet2, (lcb_command_t)0x55);

    uint8_t data[] = {0xAA, 0xBB};
    lcb_packet_set_data(&packet1, data, 2);
    lcb_packet_set_data(&packet2, data, 2);

    ASSERT_TRUE(lcb_packet_equals(&packet1, &packet2));

    // Change command
    lcb_packet_set_command(&packet2, (lcb_command_t)0x56);
    ASSERT_FALSE(lcb_packet_equals(&packet1, &packet2));

    lcb_packet_free(&packet1);
    lcb_packet_free(&packet2);
}

// Test packet comparison (for priority sorting)
TEST_F(LcbPacketTest, PacketCompare) {
    lcb_packet_t high_prio, low_prio;

    lcb_packet_init(&high_prio);
    lcb_packet_init(&low_prio);

    lcb_packet_set_command(&high_prio, (lcb_command_t)0x01);
    lcb_packet_set_command(&low_prio, (lcb_command_t)0x02);

    lcb_packet_set_priority(&high_prio, LCB_PRIORITY_HIGHEST);
    lcb_packet_set_priority(&low_prio, LCB_PRIORITY_LOWEST);

    // high_prio should have higher priority (negative result)
    ASSERT_LT(lcb_packet_compare(&high_prio, &low_prio), 0);

    // low_prio should have lower priority (positive result)
    ASSERT_GT(lcb_packet_compare(&low_prio, &high_prio), 0);

    lcb_packet_free(&high_prio);
    lcb_packet_free(&low_prio);
}

// Test building TX packet with maximum data length
TEST_F(LcbPacketTest, BuildTxPacketMaxData) {
    lcb_packet_set_command(&packet, (lcb_command_t)0xFF);

    uint8_t test_data[LCB_PACKET_MAX_DATA_SIZE];
    for (int i = 0; i < LCB_PACKET_MAX_DATA_SIZE; i++) {
        test_data[i] = (uint8_t)(i & 0xFF);
    }

    ASSERT_EQ(lcb_packet_set_data(&packet, test_data, LCB_PACKET_MAX_DATA_SIZE), 0);

    uint8_t buffer[512];
    size_t packet_size;

    ASSERT_EQ(lcb_packet_build_tx(&packet, buffer, sizeof(buffer), &packet_size), 0);

    // Verify header
    ASSERT_EQ(buffer[0], LCB_PACKET_START_BYTE_TX);
    ASSERT_EQ(buffer[1], LCB_PACKET_STATUS_BYTE);
    ASSERT_EQ(buffer[2], 0xFF);
    ASSERT_EQ(buffer[3], LCB_PACKET_MAX_DATA_SIZE);

    // Verify CRC is calculated
    uint8_t expected_crc = lcb_crc8_calculate(buffer, packet_size - 1);
    ASSERT_EQ(buffer[packet_size - 1], expected_crc);
}

// Test buffer too small for TX packet
TEST_F(LcbPacketTest, BuildTxPacketBufferTooSmall) {
    lcb_packet_set_command(&packet, (lcb_command_t)0xAB);

    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    lcb_packet_set_data(&packet, test_data, 4);

    uint8_t small_buffer[5]; // Too small
    size_t packet_size;

    // Should fail due to insufficient buffer size
    ASSERT_LT(lcb_packet_build_tx(&packet, small_buffer, sizeof(small_buffer), &packet_size), 0);
}

// ============================================================================
// Memory Management Tests
// ============================================================================

// Test double free safety (data set to NULL after first free)
TEST_F(LcbPacketTest, DoubleFree_NoCrash) {
    uint8_t data[] = {0x01, 0x02, 0x03};
    lcb_packet_set_data(&packet, data, 3);
    ASSERT_NE(packet.data, nullptr);

    lcb_packet_free(&packet);
    ASSERT_EQ(packet.data, nullptr);
    ASSERT_EQ(packet.data_length, 0);

    // Second free should be safe (data is already NULL)
    lcb_packet_free(&packet);
    ASSERT_EQ(packet.data, nullptr);
    ASSERT_EQ(packet.data_length, 0);
}

// Test set_data replaces existing data without leaking
TEST_F(LcbPacketTest, SetDataReplacesExisting_NoLeak) {
    uint8_t data_a[] = {0xAA, 0xBB};
    ASSERT_EQ(lcb_packet_set_data(&packet, data_a, 2), 0);
    const uint8_t *ptr_a = packet.data;
    ASSERT_NE(ptr_a, nullptr);
    ASSERT_EQ(packet.data_length, 2);

    // Replace with different data - old buffer freed internally
    uint8_t data_b[] = {0xCC, 0xDD, 0xEE};
    ASSERT_EQ(lcb_packet_set_data(&packet, data_b, 3), 0);
    ASSERT_EQ(packet.data_length, 3);
    ASSERT_EQ(packet.data[0], 0xCC);
    ASSERT_EQ(packet.data[1], 0xDD);
    ASSERT_EQ(packet.data[2], 0xEE);
}

// Test set_data(NULL, 0) clears existing data
TEST_F(LcbPacketTest, SetDataNullClearsExisting) {
    uint8_t data[] = {0x01, 0x02};
    lcb_packet_set_data(&packet, data, 2);
    ASSERT_NE(packet.data, nullptr);
    ASSERT_EQ(packet.data_length, 2);

    // Clear data with NULL
    ASSERT_EQ(lcb_packet_set_data(&packet, NULL, 0), 0);
    ASSERT_EQ(packet.data, nullptr);
    ASSERT_EQ(packet.data_length, 0);
}

// Test clone into a packet that already has data
TEST_F(LcbPacketTest, CloneIntoPacketWithExistingData) {
    // Set up source packet
    lcb_packet_t src;
    lcb_packet_init(&src);
    uint8_t src_data[] = {0x11, 0x22};
    lcb_packet_set_data(&src, src_data, 2);
    lcb_packet_set_command(&src, (lcb_command_t)0xAA);

    // Give destination packet some existing data
    uint8_t dest_data[] = {0xFF, 0xFE, 0xFD};
    lcb_packet_set_data(&packet, dest_data, 3);

    // Clone over it - lcb_packet_clone calls lcb_packet_init on dest
    ASSERT_EQ(lcb_packet_clone(&packet, &src), 0);
    ASSERT_EQ(packet.data_length, 2);
    ASSERT_EQ(packet.data[0], 0x11);
    ASSERT_EQ(packet.data[1], 0x22);
    ASSERT_EQ(lcb_packet_get_command(&packet), (lcb_command_t)0xAA);

    lcb_packet_free(&src);
}

// Test free > re-init > set_data > free cycle
TEST_F(LcbPacketTest, FreeAndReinitCycle) {
    // First cycle
    uint8_t data1[] = {0x01, 0x02, 0x03};
    lcb_packet_set_data(&packet, data1, 3);
    lcb_packet_free(&packet);
    ASSERT_EQ(packet.data, nullptr);

    // Re-init and use again
    lcb_packet_init(&packet);
    uint8_t data2[] = {0x0A, 0x0B};
    ASSERT_EQ(lcb_packet_set_data(&packet, data2, 2), 0);
    ASSERT_EQ(packet.data_length, 2);
    ASSERT_EQ(packet.data[0], 0x0A);
    lcb_packet_free(&packet);

    // Third cycle
    lcb_packet_init(&packet);
    uint8_t data3[] = {0xFF};
    ASSERT_EQ(lcb_packet_set_data(&packet, data3, 1), 0);
    ASSERT_EQ(packet.data_length, 1);
    // TearDown will call lcb_packet_free
}

// Stress test: repeated set_data calls
TEST_F(LcbPacketTest, RepeatedSetDataCycles) {
    for (int i = 0; i < 100; i++) {
        uint8_t len = (uint8_t)((i % 10) + 1);
        uint8_t data[10];
        for (uint8_t j = 0; j < len; j++) {
            data[j] = (uint8_t)(i + j);
        }
        ASSERT_EQ(lcb_packet_set_data(&packet, data, len), 0);
        ASSERT_EQ(packet.data_length, len);
        ASSERT_EQ(packet.data[0], (uint8_t)i);
    }
}

// Test NULL safety for all packet API functions
TEST_F(LcbPacketTest, NullSafety_AllFunctions) {
    // init / free
    ASSERT_EQ(lcb_packet_init(NULL), -1);
    lcb_packet_free(NULL);  // should not crash

    // set/get command
    lcb_packet_set_command(NULL, (lcb_command_t)0x01);  // no crash
    ASSERT_EQ(lcb_packet_get_command(NULL), LCB_CMD_GET_STATUS);

    // set/get data
    uint8_t data[] = {0x01};
    ASSERT_EQ(lcb_packet_set_data(NULL, data, 1), -1);
    uint8_t len;
    ASSERT_EQ(lcb_packet_get_data(NULL, &len), nullptr);
    ASSERT_EQ(len, 0);

    // set/get priority
    lcb_packet_set_priority(NULL, 1);  // no crash
    ASSERT_EQ(lcb_packet_get_priority(NULL), LCB_PRIORITY_NORMAL);

    // clone
    lcb_packet_t dummy;
    lcb_packet_init(&dummy);
    ASSERT_EQ(lcb_packet_clone(NULL, &dummy), -1);
    ASSERT_EQ(lcb_packet_clone(&dummy, NULL), -1);
    lcb_packet_free(&dummy);

    // equals / compare
    ASSERT_FALSE(lcb_packet_equals(NULL, &packet));
    ASSERT_FALSE(lcb_packet_equals(&packet, NULL));
    ASSERT_EQ(lcb_packet_compare(NULL, &packet), 0);
    ASSERT_EQ(lcb_packet_compare(&packet, NULL), 0);

    // build_tx
    uint8_t buffer[16];
    size_t pkt_size;
    ASSERT_LT(lcb_packet_build_tx(NULL, buffer, sizeof(buffer), &pkt_size), 0);
    ASSERT_LT(lcb_packet_build_tx(&packet, NULL, 16, &pkt_size), 0);
    ASSERT_LT(lcb_packet_build_tx(&packet, buffer, sizeof(buffer), NULL), 0);
}

// Test set_data with zero length but non-NULL pointer
TEST_F(LcbPacketTest, SetDataZeroLengthWithNonNullPtr) {
    // First set some data
    uint8_t data[] = {0x01, 0x02};
    lcb_packet_set_data(&packet, data, 2);
    ASSERT_EQ(packet.data_length, 2);

    // Set with valid pointer but zero length - should clear data
    uint8_t dummy[] = {0xFF};
    ASSERT_EQ(lcb_packet_set_data(&packet, dummy, 0), 0);
    ASSERT_EQ(packet.data, nullptr);
    ASSERT_EQ(packet.data_length, 0);
}
