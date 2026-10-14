#include "gtest/gtest.h"

// Use extern "C" to link C-language functions into a C++ test file.
extern "C" {
    #include "lcb_crc8.h"
}

// Test Fixture for LcbCrc8
class LcbCrc8Test : public ::testing::Test {};

// Test: CRC8 calculation for empty buffer
TEST_F(LcbCrc8Test, HandlesEmptyBuffer) {
    uint8_t buffer[] = {};
    uint8_t result = lcb_crc8_calculate(buffer, 0);
    ASSERT_EQ(0x00, result);
}

// Test: CRC8 calculation for single byte
TEST_F(LcbCrc8Test, HandlesSingleByte) {
    uint8_t buffer[] = {0x12};
    uint8_t result = lcb_crc8_calculate(buffer, 1);
    ASSERT_NE(0x00, result);
}

// Test: CRC8 calculation for known LCB packet header
TEST_F(LcbCrc8Test, CalculatesPacketHeader) {
    uint8_t buffer[] = {0x00, 0xFF, 0x70, 0x00};
    uint8_t result = lcb_crc8_calculate(buffer, 4);
    ASSERT_NE(0x00, result);
}

// Test: CRC8 calculation consistency
TEST_F(LcbCrc8Test, IsConsistent) {
    uint8_t buffer[] = {0x00, 0xFF, 0x71, 0x02, 0xAA, 0xBB};
    uint8_t result1 = lcb_crc8_calculate(buffer, 6);
    uint8_t result2 = lcb_crc8_calculate(buffer, 6);
    ASSERT_EQ(result1, result2);
}

// Test: CRC8 with different data lengths
TEST_F(LcbCrc8Test, VariesWithLength) {
    uint8_t buffer[] = {0x00, 0xFF, 0x70, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t result_short = lcb_crc8_calculate(buffer, 4);
    uint8_t result_long = lcb_crc8_calculate(buffer, 9);
    ASSERT_NE(result_short, result_long);
}

// Test: CRC8 with all zeros
TEST_F(LcbCrc8Test, HandlesAllZeros) {
    uint8_t buffer[] = {0x00, 0x00, 0x00, 0x00};
    uint8_t result = lcb_crc8_calculate(buffer, 4);
    ASSERT_EQ(0x00, result);
}

// Test: CRC8 with all ones
TEST_F(LcbCrc8Test, HandlesAllOnes) {
    uint8_t buffer[] = {0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t result = lcb_crc8_calculate(buffer, 4);
    ASSERT_NE(0x00, result);
}

// Test: CRC8 sensitivity to bit changes
TEST_F(LcbCrc8Test, IsSensitiveToBitChanges) {
    uint8_t buffer1[] = {0x00, 0xFF, 0x70, 0x00};
    uint8_t buffer2[] = {0x00, 0xFF, 0x71, 0x00}; // Changed one byte
    uint8_t result1 = lcb_crc8_calculate(buffer1, 4);
    uint8_t result2 = lcb_crc8_calculate(buffer2, 4);
    ASSERT_NE(result1, result2);
}
