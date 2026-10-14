#include "gtest/gtest.h"

extern "C" {
    #include "lcb_packet_queue.h"
    #include "lcb_packet.h"
}

// Test fixture for PacketQueue tests
class PacketQueueTest : public ::testing::Test {
protected:
    lcb_packet_queue_t queue;

    void SetUp() override {
        // Initialize the queue before each test
        lcb_packet_queue_init(&queue);
    }

    void TearDown() override {
        // Clean up after each test
        lcb_packet_queue_free(&queue);
    }
};

// Test initialization
TEST_F(PacketQueueTest, Initialization) {
    ASSERT_EQ(queue.size, 0);
    ASSERT_TRUE(lcb_packet_queue_is_empty(&queue));
}

// Test adding a single packet and removing it
TEST_F(PacketQueueTest, AddAndRemoveSingle) {
    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, (lcb_command_t)0x01);

    uint8_t data[] = {0xAA, 0xBB};
    lcb_packet_set_data(&packet, data, 2);
    lcb_packet_set_priority(&packet, 1);

    ASSERT_EQ(lcb_packet_queue_add(&queue, &packet), 0);
    ASSERT_EQ(queue.size, 1);

    lcb_packet_t removed_packet;
    lcb_packet_init(&removed_packet);
    ASSERT_EQ(lcb_packet_queue_remove(&queue, &removed_packet), 0);
    ASSERT_EQ(queue.size, 0);
    ASSERT_EQ(lcb_packet_get_command(&removed_packet), (lcb_command_t)0x01);

    lcb_packet_free(&packet);
    lcb_packet_free(&removed_packet);
}

// Test priority order (lower number means higher priority)
TEST_F(PacketQueueTest, PriorityOrder) {
    lcb_packet_t low_prio, high_prio, mid_prio;

    lcb_packet_init(&low_prio);
    lcb_packet_set_command(&low_prio, (lcb_command_t)0x01);
    lcb_packet_set_priority(&low_prio, 10);

    lcb_packet_init(&high_prio);
    lcb_packet_set_command(&high_prio, (lcb_command_t)0x02);
    lcb_packet_set_priority(&high_prio, 1);

    lcb_packet_init(&mid_prio);
    lcb_packet_set_command(&mid_prio, (lcb_command_t)0x03);
    lcb_packet_set_priority(&mid_prio, 5);

    lcb_packet_queue_add(&queue, &low_prio);
    lcb_packet_queue_add(&queue, &high_prio);
    lcb_packet_queue_add(&queue, &mid_prio);

    ASSERT_EQ(queue.size, 3);

    lcb_packet_t polled_packet;
    lcb_packet_init(&polled_packet);

    // 1. Remove high priority packet (priority 1)
    lcb_packet_queue_remove(&queue, &polled_packet);
    ASSERT_EQ(lcb_packet_get_command(&polled_packet), lcb_packet_get_command(&high_prio));

    // 2. Remove mid priority packet (priority 5)
    lcb_packet_queue_remove(&queue, &polled_packet);
    ASSERT_EQ(lcb_packet_get_command(&polled_packet), lcb_packet_get_command(&mid_prio));

    // 3. Remove low priority packet (priority 10)
    lcb_packet_queue_remove(&queue, &polled_packet);
    ASSERT_EQ(lcb_packet_get_command(&polled_packet), lcb_packet_get_command(&low_prio));

    lcb_packet_free(&low_prio);
    lcb_packet_free(&high_prio);
    lcb_packet_free(&mid_prio);
    lcb_packet_free(&polled_packet);
}

// Test clearing the queue
TEST_F(PacketQueueTest, Clear) {
    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, (lcb_command_t)0x01);
    lcb_packet_set_priority(&packet, 1);

    lcb_packet_queue_add(&queue, &packet);
    lcb_packet_queue_add(&queue, &packet);
    ASSERT_EQ(queue.size, 2);

    lcb_packet_queue_clear(&queue);
    ASSERT_EQ(queue.size, 0);

    lcb_packet_t removed_packet;
    lcb_packet_init(&removed_packet);
    ASSERT_EQ(lcb_packet_queue_remove(&queue, &removed_packet), -1); // Queue should be empty

    lcb_packet_free(&packet);
    lcb_packet_free(&removed_packet);
}

// Test replacing a command
TEST_F(PacketQueueTest, ReplaceCommand) {
    lcb_packet_t packet1, packet2, packet_replace;

    lcb_packet_init(&packet1);
    lcb_packet_set_command(&packet1, (lcb_command_t)0xAA);
    uint8_t data1[] = {1, 2};
    lcb_packet_set_data(&packet1, data1, 2);
    lcb_packet_set_priority(&packet1, 5);

    lcb_packet_init(&packet2);
    lcb_packet_set_command(&packet2, (lcb_command_t)0xBB);
    uint8_t data2[] = {3};
    lcb_packet_set_data(&packet2, data2, 1);
    lcb_packet_set_priority(&packet2, 5);

    lcb_packet_init(&packet_replace);
    lcb_packet_set_command(&packet_replace, (lcb_command_t)0xAA);
    uint8_t data_replace[] = {99};
    lcb_packet_set_data(&packet_replace, data_replace, 1);
    lcb_packet_set_priority(&packet_replace, 2);

    lcb_packet_queue_add(&queue, &packet1);
    lcb_packet_queue_add(&queue, &packet2);
    ASSERT_EQ(queue.size, 2);

    lcb_packet_queue_replace_command(&queue, &packet_replace);
    ASSERT_EQ(queue.size, 2);

    lcb_packet_t polled_packet;
    lcb_packet_init(&polled_packet);

    // First removed should be the new, high-priority packet
    lcb_packet_queue_remove(&queue, &polled_packet);
    ASSERT_EQ(lcb_packet_get_command(&polled_packet), lcb_packet_get_command(&packet_replace));
    uint8_t length;
    const uint8_t* polled_data = lcb_packet_get_data(&polled_packet, &length);
    ASSERT_EQ(length, 1);
    ASSERT_EQ(polled_data[0], 99);
    ASSERT_EQ(lcb_packet_get_priority(&polled_packet), 2);

    // Second removed should be the other packet
    lcb_packet_queue_remove(&queue, &polled_packet);
    ASSERT_EQ(lcb_packet_get_command(&polled_packet), lcb_packet_get_command(&packet2));

    lcb_packet_free(&packet1);
    lcb_packet_free(&packet2);
    lcb_packet_free(&packet_replace);
    lcb_packet_free(&polled_packet);
}

// Test polling (peek without removing)
TEST_F(PacketQueueTest, PollWithoutRemoving) {
    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, (lcb_command_t)0x55);
    lcb_packet_set_priority(&packet, 1);

    lcb_packet_queue_add(&queue, &packet);
    ASSERT_EQ(queue.size, 1);

    lcb_packet_t polled_packet;
    lcb_packet_init(&polled_packet);

    // Poll should return the packet but not remove it
    ASSERT_EQ(lcb_packet_queue_poll(&queue, &polled_packet), 0);
    ASSERT_EQ(lcb_packet_get_command(&polled_packet), (lcb_command_t)0x55);
    ASSERT_EQ(queue.size, 1); // Size should remain 1

    // Remove it now
    lcb_packet_queue_remove(&queue, &polled_packet);
    ASSERT_EQ(queue.size, 0);

    lcb_packet_free(&packet);
    lcb_packet_free(&polled_packet);
}

// Test add if not queued
TEST_F(PacketQueueTest, AddIfNotQueued) {
    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, (lcb_command_t)0x77);
    lcb_packet_set_priority(&packet, 1);

    // First add should succeed
    ASSERT_EQ(lcb_packet_queue_add_if_not_queued(&queue, &packet), 1);
    ASSERT_EQ(queue.size, 1);

    // Second add should not add (duplicate found)
    ASSERT_EQ(lcb_packet_queue_add_if_not_queued(&queue, &packet), 0);
    ASSERT_EQ(queue.size, 1);

    lcb_packet_free(&packet);
}

// ============================================================================
// Memory Management Tests
// ============================================================================

// Regression test: replace_command properly frees local highest_packet copy
TEST_F(PacketQueueTest, ReplaceCommand_FreesLocalCopy) {
    lcb_packet_t p1;
    lcb_packet_init(&p1);
    lcb_packet_set_command(&p1, (lcb_command_t)0xAA);
    uint8_t d1[] = {0x01, 0x02};
    lcb_packet_set_data(&p1, d1, 2);
    lcb_packet_set_priority(&p1, 5);

    lcb_packet_queue_add(&queue, &p1);

    // Replace with new data - exercises the fixed lcb_packet_free(&highest_packet) path
    lcb_packet_t p_replace;
    lcb_packet_init(&p_replace);
    lcb_packet_set_command(&p_replace, (lcb_command_t)0xAA);
    uint8_t d_replace[] = {0xCC, 0xDD, 0xEE};
    lcb_packet_set_data(&p_replace, d_replace, 3);
    lcb_packet_set_priority(&p_replace, 3);

    ASSERT_EQ(lcb_packet_queue_replace_command(&queue, &p_replace), 0);
    ASSERT_EQ(queue.size, 1);

    // Verify the replaced packet
    lcb_packet_t result;
    lcb_packet_init(&result);
    lcb_packet_queue_remove(&queue, &result);
    ASSERT_EQ(result.data_length, 3);
    ASSERT_EQ(result.data[0], 0xCC);
    ASSERT_EQ(result.data[1], 0xDD);
    ASSERT_EQ(result.data[2], 0xEE);

    lcb_packet_free(&p1);
    lcb_packet_free(&p_replace);
    lcb_packet_free(&result);
}

// Test that remove transfers ownership: caller must free
TEST_F(PacketQueueTest, RemoveTransfersOwnership) {
    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, (lcb_command_t)0x55);
    uint8_t data[] = {0xAA, 0xBB, 0xCC};
    lcb_packet_set_data(&packet, data, 3);
    lcb_packet_set_priority(&packet, 1);

    lcb_packet_queue_add(&queue, &packet);
    ASSERT_EQ(queue.size, 1);

    lcb_packet_t removed;
    lcb_packet_init(&removed);
    ASSERT_EQ(lcb_packet_queue_remove(&queue, &removed), 0);
    ASSERT_EQ(queue.size, 0);

    // removed packet data should be valid and accessible
    ASSERT_EQ(removed.data_length, 3);
    ASSERT_EQ(removed.data[0], 0xAA);
    ASSERT_EQ(removed.data[1], 0xBB);
    ASSERT_EQ(removed.data[2], 0xCC);

    // Queue is empty - no double-free possible
    ASSERT_TRUE(lcb_packet_queue_is_empty(&queue));

    lcb_packet_free(&packet);
    lcb_packet_free(&removed);
}

// Test that add clones packet - modifying original doesn't affect queued copy
TEST_F(PacketQueueTest, AddClonesPacket_ModifyOriginal) {
    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, (lcb_command_t)0x33);
    uint8_t data[] = {0x10, 0x20};
    lcb_packet_set_data(&packet, data, 2);
    lcb_packet_set_priority(&packet, 1);

    lcb_packet_queue_add(&queue, &packet);

    // Modify the original packet's data
    uint8_t new_data[] = {0xFF, 0xFE, 0xFD};
    lcb_packet_set_data(&packet, new_data, 3);

    // Queued packet should still have old data
    lcb_packet_t removed;
    lcb_packet_init(&removed);
    lcb_packet_queue_remove(&queue, &removed);
    ASSERT_EQ(removed.data_length, 2);
    ASSERT_EQ(removed.data[0], 0x10);
    ASSERT_EQ(removed.data[1], 0x20);

    lcb_packet_free(&packet);
    lcb_packet_free(&removed);
}

// Test that poll returns a clone that must be freed independently
TEST_F(PacketQueueTest, PollClonesPacket_MustFree) {
    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, (lcb_command_t)0x44);
    uint8_t data[] = {0xAA, 0xBB};
    lcb_packet_set_data(&packet, data, 2);
    lcb_packet_set_priority(&packet, 1);

    lcb_packet_queue_add(&queue, &packet);

    // Poll - gets a clone
    lcb_packet_t polled;
    lcb_packet_init(&polled);
    ASSERT_EQ(lcb_packet_queue_poll(&queue, &polled), 0);
    ASSERT_EQ(polled.data_length, 2);
    ASSERT_EQ(polled.data[0], 0xAA);
    ASSERT_EQ(queue.size, 1);  // Not removed from queue

    // Free polled copy
    lcb_packet_free(&polled);

    // Queue packet should still be intact
    lcb_packet_t removed;
    lcb_packet_init(&removed);
    lcb_packet_queue_remove(&queue, &removed);
    ASSERT_EQ(removed.data_length, 2);
    ASSERT_EQ(removed.data[0], 0xAA);
    ASSERT_EQ(removed.data[1], 0xBB);

    lcb_packet_free(&packet);
    lcb_packet_free(&removed);
}

// Test that clear frees all packet data buffers
TEST_F(PacketQueueTest, ClearFreesAllPacketData) {
    for (int i = 0; i < 5; i++) {
        lcb_packet_t p;
        lcb_packet_init(&p);
        lcb_packet_set_command(&p, (lcb_command_t)(0x10 + i));
        uint8_t data[4] = {(uint8_t)i, (uint8_t)(i+1), (uint8_t)(i+2), (uint8_t)(i+3)};
        lcb_packet_set_data(&p, data, 4);
        lcb_packet_set_priority(&p, (uint8_t)(i + 1));
        lcb_packet_queue_add(&queue, &p);
        lcb_packet_free(&p);
    }
    ASSERT_EQ(queue.size, 5);

    lcb_packet_queue_clear(&queue);
    ASSERT_EQ(queue.size, 0);
    ASSERT_TRUE(lcb_packet_queue_is_empty(&queue));
}

// Test that queue free releases all resources (nodes + packets + mutex)
TEST_F(PacketQueueTest, FreeWithMultiplePackets) {
    // Create a separate queue (not the fixture one) to control its lifecycle
    lcb_packet_queue_t local_queue;
    lcb_packet_queue_init(&local_queue);

    for (int i = 0; i < 3; i++) {
        lcb_packet_t p;
        lcb_packet_init(&p);
        lcb_packet_set_command(&p, (lcb_command_t)(0x20 + i));
        uint8_t data[] = {(uint8_t)(i * 10), (uint8_t)(i * 10 + 1)};
        lcb_packet_set_data(&p, data, 2);
        lcb_packet_set_priority(&p, 5);
        lcb_packet_queue_add(&local_queue, &p);
        lcb_packet_free(&p);
    }
    ASSERT_EQ(local_queue.size, 3);

    // Free the entire queue - all nodes, packets, and mutex are released
    lcb_packet_queue_free(&local_queue);
    ASSERT_EQ(local_queue.size, 0);
    ASSERT_EQ(local_queue.mutex, nullptr);
}

// Stress test: repeated add/remove cycles
TEST_F(PacketQueueTest, RepeatedAddRemoveCycles) {
    for (int i = 0; i < 50; i++) {
        lcb_packet_t p;
        lcb_packet_init(&p);
        lcb_packet_set_command(&p, (lcb_command_t)(i & 0xFF));
        uint8_t data[3] = {(uint8_t)i, (uint8_t)(i+1), (uint8_t)(i+2)};
        lcb_packet_set_data(&p, data, 3);
        lcb_packet_set_priority(&p, 5);

        ASSERT_EQ(lcb_packet_queue_add(&queue, &p), 0);
        ASSERT_EQ(queue.size, 1);

        lcb_packet_t removed;
        lcb_packet_init(&removed);
        ASSERT_EQ(lcb_packet_queue_remove(&queue, &removed), 0);
        ASSERT_EQ(queue.size, 0);
        ASSERT_EQ(removed.data[0], (uint8_t)i);

        lcb_packet_free(&p);
        lcb_packet_free(&removed);
    }
}
