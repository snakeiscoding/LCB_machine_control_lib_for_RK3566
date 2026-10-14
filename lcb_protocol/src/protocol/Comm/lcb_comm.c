/**
 * @file lcb_comm.c
 * @brief LCB Communication Implementation
 *
 * Converted from: com/jht/lcbcommlib/LcbComm.java
 *
 * This file implements the main LCB communication logic including:
 * - Thread management (TX/RX threads)
 * - Command queueing and transmission
 * - Response parsing and callbacks
 * - Error handling and retry logic
 */

#include "lcb_comm.h"
#include "lcb_proc_comm.h"
#include "lcb_parser.h"
#include "lcb_receive_data_queue.h"
#include "lcb_utils.h"
#include "lcb_constants.h"
#include "lcb_logging.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#define TAG "lcb_comm"

/* Configuration constants */
#define LCB_RECEIVE_TIMEOUT_MS              150
#define LCB_RECEIVE_TIMEOUT_EUPS_MS         300
#define LCB_RETRY_DELAY_TIME_MS             50
#define LCB_CMD_DELAY_TIME_MS               30
#define LCB_LIMITED_TOTAL_RETRY_COUNTER     25
#define LCB_LIMITED_ONE_CMD_COMM_RETRIES    3
#define LCB_RX_BUFFER_SIZE                  512
#define LCB_BOOT_UP_LEAVE_ERP_RETRY_PERIOD  1000
#define LCB_BOOT_UP_LEAVE_ERP_RETRIES       3
#define LCB_WAKE_UP_LCB_RETRIES             3
#define LCB_LEAVE_ERP_GET_LCB_STATUS_RETRIES 3
#define LCB_WAKE_PULSE_MS                   405
#define LCB_WAKE_PULSE_SETTLE_MS            10

/* ERP mode constants */
#define LCB_ENTER_ERP                       0xFF
#define LCB_LEAVE_ERP                       0x00

/**
 * @brief LCB Communication Context Structure
 */
struct lcb_comm {
    /* Serial port */
    serial_port_t *port;
    int port_fd;
    bool is_port_opened;
    bool is_running;

    /* Packet queues */
    lcb_packet_queue_t tx_queue;
    lcb_packet_queue_t rx_queue;

    /* Raw receive data buffer queue
     * (Java: ReceiveDataQueue _receiveDataBufferQueue) */
    lcb_receive_data_queue_t rx_data_queue;

    /* Parser */
    lcb_parser_t parser;

    /* Threads */
    pthread_t comm_thread;
    pthread_t rx_thread;
    pthread_t erp_stop_thread;

    /* Synchronization */
    pthread_mutex_t tx_sync_mutex;
    pthread_cond_t tx_sync_cond;
    pthread_mutex_t rx_sync_mutex;
    pthread_cond_t rx_sync_cond;
    pthread_mutex_t erp_stop_mutex;
    pthread_cond_t erp_stop_cond;

    /* Callbacks */
    lcb_comm_callbacks_t callbacks;
    bool has_callbacks;

    /* Status flags */
    bool incline_motor_downward_status;
    bool incline_motor_upward_status;
    bool main_motor_status;
    bool encoder_rpm_status;
    bool battery_capacity_status;
    bool command_error_status;
    bool lcb_error_status;
    bool initial_calibration_status;
    bool not_support_cmd;
    bool is_boot_up_leave_erp;
    bool blocked_flag;
    bool comm_thread_except_flag;

    /* OneShot tracking flags */
    bool is_one_shot_rpm;           /* For commands 0xF9 and 0x63 */
    bool is_one_shot_spm;           /* For command 0x4B */
    bool is_one_shot_sensor_data;   /* For command 0x51 */

    /* Retry counters */
    int retry_total_counter;
    int boot_up_leave_erp_time_counter;
    int wake_up_lcb_counter;
    int leave_erp_get_lcb_status_counter;

    /* ERP state */
    uint8_t lcb_erp_state;

    /* Transmit command tracking */
    lcb_command_t tx_command;

    /* IR Address board (0x26) — "Hybrid TM" IR beam E-Stop sensor.
     * last_tx_address mirrors Java's _txLcbAddr: remembered at TX time and
     * used (not anything parsed from the RX bytes) to decide whether the
     * matching response belongs to the main LCB or the IR board, since both
     * devices reuse opcodes 0x73/0x7A. ir_addr_erp_state mirrors Java's
     * _irAddrErpState: which of enter/leave ERP is outstanding, needed to
     * disambiguate the single response byte into the right success callback. */
    uint8_t last_tx_address;
    uint8_t ir_addr_erp_state;

    /* Receive buffer */
    uint8_t rx_buffer[LCB_RX_BUFFER_SIZE];
};

/* Forward declarations of thread functions */
static void* lcb_comm_thread_func(void *arg);
static void* lcb_rx_thread_func(void *arg);
static void* lcb_erp_stop_thread_func(void *arg);

/* Forward declarations of internal functions */
static void lcb_on_rx_packet_parsed(const lcb_packet_t *packet, void *user_data);
static void lcb_receive_data_handler(lcb_comm_t *comm, const uint8_t *data, uint32_t length);
static void lcb_process_receive_data_buffer(lcb_comm_t *comm, const lcb_receive_data_buffer_t *buf);
static void lcb_process_received_packet(lcb_comm_t *comm, const lcb_packet_t *packet);
static void lcb_process_ir_addr_received_packet(lcb_comm_t *comm, const lcb_packet_t *packet);
static void lcb_check_status_bits(lcb_comm_t *comm, uint8_t status);
static int lcb_add_command_packet(lcb_comm_t *comm, lcb_command_t cmd,
                                  const uint8_t *data, uint8_t length,
                                  uint8_t priority);
static int lcb_add_command_packet_with_mode(lcb_comm_t *comm, lcb_command_t cmd,
                                            const uint8_t *data, uint8_t length,
                                            uint8_t priority, lcb_queue_mode_t mode);
static int lcb_add_ir_addr_command_packet_with_mode(lcb_comm_t *comm, lcb_command_t cmd,
                                                    const uint8_t *data, uint8_t length,
                                                    uint8_t priority, lcb_queue_mode_t mode);
static void lcb_proc_set_cmd_eups_mode_handler(lcb_comm_t *comm, const lcb_packet_t *packet);
static void lcb_proc_set_ir_addr_cmd_eups_mode_handler(lcb_comm_t *comm, const lcb_packet_t *packet);

/**
 * @brief Create LCB communication instance
 */
lcb_comm_t* lcb_comm_create(void)
{
    LCB_LOGI(TAG, "Creating LCB communication instance");

    lcb_comm_t *comm = (lcb_comm_t*)calloc(1, sizeof(lcb_comm_t));
    if (comm == NULL) {
        LCB_LOGE(TAG, "Failed to allocate lcb_comm instance");
        return NULL;
    }

    /* Initialize queues */
    if (lcb_packet_queue_init(&comm->tx_queue) < 0) {
        LCB_LOGE(TAG, "Failed to initialize TX queue");
        free(comm);
        return NULL;
    }

    if (lcb_packet_queue_init(&comm->rx_queue) < 0) {
        LCB_LOGE(TAG, "Failed to initialize RX queue");
        lcb_packet_queue_free(&comm->tx_queue);
        free(comm);
        return NULL;
    }

    /* Initialize receive data buffer queue
     * (Java: _receiveDataBufferQueue = new ReceiveDataQueue()) */
    if (lcb_receive_data_queue_init(&comm->rx_data_queue) < 0) {
        LCB_LOGE(TAG, "Failed to initialize RX data queue");
        lcb_packet_queue_free(&comm->rx_queue);
        lcb_packet_queue_free(&comm->tx_queue);
        free(comm);
        return NULL;
    }

    /* Initialize parser */
    if (lcb_parser_init(&comm->parser) < 0) {
        LCB_LOGE(TAG, "Failed to initialize parser");
        lcb_receive_data_queue_free(&comm->rx_data_queue);
        lcb_packet_queue_free(&comm->rx_queue);
        lcb_packet_queue_free(&comm->tx_queue);
        free(comm);
        return NULL;
    }

    /* Initialize mutexes and condition variables */
    pthread_mutex_init(&comm->tx_sync_mutex, NULL);
    pthread_cond_init(&comm->tx_sync_cond, NULL);
    pthread_mutex_init(&comm->rx_sync_mutex, NULL);
    pthread_cond_init(&comm->rx_sync_cond, NULL);
    pthread_mutex_init(&comm->erp_stop_mutex, NULL);
    pthread_cond_init(&comm->erp_stop_cond, NULL);

    /* Initialize state */
    comm->is_port_opened = false;
    comm->is_running = false;
    comm->lcb_erp_state = LCB_ENTER_ERP;
    comm->tx_command = LCB_CMD_GET_STATUS;
    comm->last_tx_address = LCB_PACKET_ADDRESS_MAIN;
    comm->ir_addr_erp_state = LCB_CMD_EUPS_MODE_NOTIFY_ENTER_EUPS;
    comm->has_callbacks = false;

    LCB_LOGI(TAG, "LCB comm instance created successfully");
    return comm;
}

/**
 * @brief Destroy LCB communication instance
 */
void lcb_comm_destroy(lcb_comm_t *comm)
{
    if (comm == NULL) {
        return;
    }

    LCB_LOGI(TAG, "Destroying LCB comm instance");

    /* Stop communication if running */
    if (comm->is_running) {
        lcb_comm_stop(comm);
    }

    /* Free resources */
    lcb_parser_free(&comm->parser);
    lcb_receive_data_queue_free(&comm->rx_data_queue);
    lcb_packet_queue_free(&comm->rx_queue);
    lcb_packet_queue_free(&comm->tx_queue);

    /* Destroy mutexes and condition variables */
    pthread_mutex_destroy(&comm->tx_sync_mutex);
    pthread_cond_destroy(&comm->tx_sync_cond);
    pthread_mutex_destroy(&comm->rx_sync_mutex);
    pthread_cond_destroy(&comm->rx_sync_cond);
    pthread_mutex_destroy(&comm->erp_stop_mutex);
    pthread_cond_destroy(&comm->erp_stop_cond);

    free(comm);
    LCB_LOGI(TAG, "LCB comm instance destroyed");
}

/**
 * @brief Register receive callbacks
 */
/**
 * @brief Initialize callback structure with NULL values
 */
void lcb_comm_callbacks_init(lcb_comm_callbacks_t *callbacks)
{
    if (callbacks == NULL) {
        return;
    }
    memset(callbacks, 0, sizeof(lcb_comm_callbacks_t));
}

int lcb_comm_register_callbacks(lcb_comm_t *comm, const lcb_comm_callbacks_t *callbacks)
{
    if (comm == NULL || callbacks == NULL) {
        return -1;
    }

    memcpy(&comm->callbacks, callbacks, sizeof(lcb_comm_callbacks_t));
    comm->has_callbacks = true;

    return 0;
}

/**
 * @brief Unregister receive callbacks
 */
int lcb_comm_unregister_callbacks(lcb_comm_t *comm)
{
    if (comm == NULL) {
        return -1;
    }

    memset(&comm->callbacks, 0, sizeof(lcb_comm_callbacks_t));
    comm->has_callbacks = false;

    return 0;
}

/**
 * @brief Start communication with serial port
 */
bool lcb_comm_start(lcb_comm_t *comm, serial_port_t *device)
{
    if (comm == NULL || device == NULL) {
        return false;
    }

    if (comm->is_port_opened) {
        /* Already opened */
        return true;
    }

    if (comm->is_running) {
        /* Already running */
        return true;
    }

    /* Store port reference and file descriptor */
    comm->port = device;
    comm->port_fd = device->fd;
    if (comm->port_fd < 0) {
        LCB_LOGE(TAG, "Invalid port file descriptor");
        return false;
    }

    LCB_LOGI(TAG, "Starting LCB communication (port_fd=%d)", comm->port_fd);

    /* Clear queues (Java: _receiveDataBufferQueue.clear()) */
    lcb_packet_queue_clear(&comm->tx_queue);
    lcb_packet_queue_clear(&comm->rx_queue);
    lcb_receive_data_queue_clear(&comm->rx_data_queue);

    /* Reset parser */
    lcb_parser_reset(&comm->parser);

    comm->is_running = true;

    /* Create threads */
    if (pthread_create(&comm->comm_thread, NULL, lcb_comm_thread_func, comm) != 0) {
        LCB_LOGE(TAG, "Failed to create comm_thread");
        comm->is_running = false;
        return false;
    }

    if (pthread_create(&comm->rx_thread, NULL, lcb_rx_thread_func, comm) != 0) {
        LCB_LOGE(TAG, "Failed to create rx_thread");
        comm->is_running = false;
        pthread_cond_broadcast(&comm->tx_sync_cond);
        pthread_join(comm->comm_thread, NULL);
        return false;
    }

    if (pthread_create(&comm->erp_stop_thread, NULL, lcb_erp_stop_thread_func, comm) != 0) {
        LCB_LOGE(TAG, "Failed to create erp_stop_thread");
        comm->is_running = false;
        pthread_cond_broadcast(&comm->tx_sync_cond);
        pthread_cond_broadcast(&comm->rx_sync_cond);
        pthread_join(comm->comm_thread, NULL);
        pthread_join(comm->rx_thread, NULL);
        return false;
    }

    comm->is_port_opened = true;

    LCB_LOGI(TAG, "LCB threads started successfully");
    return true;
}

/**
 * @brief Restart communication
 */
bool lcb_comm_restart(lcb_comm_t *comm)
{
    if (comm == NULL || comm->port == NULL) {
        return false;
    }

    /* TODO: Close and reopen serial port */
    // serial_port_close(comm->port);

    return lcb_comm_start(comm, comm->port);
}

/**
 * @brief Stop communication
 */
void lcb_comm_stop(lcb_comm_t *comm)
{
    if (comm == NULL) {
        return;
    }

    if (!comm->is_port_opened) {
        return;
    }

    LCB_LOGI(TAG, "Stopping LCB communication");

    if (comm->is_running) {
        /* Signal threads to stop */
        comm->is_running = false;

        /* Wake up threads */
        pthread_cond_broadcast(&comm->tx_sync_cond);
        pthread_cond_broadcast(&comm->rx_sync_cond);
        pthread_cond_broadcast(&comm->erp_stop_cond);

        /* Wait for threads to finish */
        pthread_join(comm->comm_thread, NULL);
        pthread_join(comm->rx_thread, NULL);
        pthread_join(comm->erp_stop_thread, NULL);

        /* Clear queues (Java: _receiveDataBufferQueue.clear()) */
        lcb_packet_queue_clear(&comm->tx_queue);
        lcb_packet_queue_clear(&comm->rx_queue);
        lcb_receive_data_queue_clear(&comm->rx_data_queue);
    }

    /* Reset serial port reference (caller is responsible for closing fd) */
    comm->port = NULL;
    comm->port_fd = -1;

    comm->is_port_opened = false;

    LCB_LOGI(TAG, "LCB communication stopped");
}

/**
 * @brief Check if communication thread is running
 */
bool lcb_comm_check_thread(lcb_comm_t *comm)
{
    if (comm == NULL) {
        return false;
    }
    return comm->is_running;
}

/**
 * @brief Check if communication thread has exception
 */
bool lcb_comm_check_thread_except(lcb_comm_t *comm)
{
    if (comm == NULL) {
        return false;
    }
    return comm->comm_thread_except_flag;
}

/**
 * @brief Get transmit queue size
 */
uint32_t lcb_comm_get_tx_queue_size(const lcb_comm_t *comm)
{
    if (comm == NULL) {
        return 0;
    }
    return lcb_packet_queue_size(&comm->tx_queue);
}

/* ============================================================================
 * Thread Functions
 * ============================================================================ */

/**
 * @brief Communication thread - handles TX queue and packet transmission
 */
static void* lcb_comm_thread_func(void *arg)
{
    lcb_comm_t *comm = (lcb_comm_t*)arg;
    lcb_packet_t packet;
    uint8_t tx_buffer[512];
    size_t packet_size;

    comm->comm_thread_except_flag = true;

    while (comm->is_running) {
        /* Wait for packet in TX queue (Java: _notifyTxPacketSync.wait()) */
        pthread_mutex_lock(&comm->tx_sync_mutex);
        while (comm->is_running && lcb_packet_queue_is_empty(&comm->tx_queue)) {
            pthread_cond_wait(&comm->tx_sync_cond, &comm->tx_sync_mutex);
        }
        pthread_mutex_unlock(&comm->tx_sync_mutex);

        if (!comm->is_running) {
            break;
        }

        /* Get packet from queue */
        if (lcb_packet_queue_remove(&comm->tx_queue, &packet) == 0) {
            LCB_LOGD(TAG, "TX dequeued: cmd=0x%02X pri=%u id=%llu", packet.command, packet.priority,
                     (unsigned long long)packet.id);
            int retries = 0;

            /* Retry loop (Java: while retries < LIMITED_ONE_CMD_COMM_RETRIES) */
            while ((retries < LCB_LIMITED_ONE_CMD_COMM_RETRIES) && comm->is_running) {
                /* Build TX packet */
                if (lcb_packet_build_tx(&packet, tx_buffer, sizeof(tx_buffer), &packet_size) != 0) {
                    LCB_LOGE(TAG, "Failed to build TX packet for cmd=0x%02X", packet.command);
                    break;
                }

                /* Reset parser before sending (Java: resetRxPacket()) */
                lcb_parser_reset(&comm->parser);

                /* Process any pulses required by the packet (Java: processAnyPulses) */
                if (packet.special_operation == LCB_SPECIAL_400MS_PULSE) {
                    usleep(LCB_WAKE_PULSE_MS * 1000);
                    usleep(LCB_WAKE_PULSE_SETTLE_MS * 1000);
                }

                /* Send packet (Java: _outStream.write(txPacket)) */
                ssize_t written = write(comm->port_fd, tx_buffer, packet_size);
                if (written != (ssize_t)packet_size) {
                    LCB_LOGE(TAG, "TX write failed: %zd of %zu bytes (errno=%d)",
                             written, packet_size, errno);
                }
                LCB_LOGD(TAG, "TX sending %zu bytes", packet_size);

                /* TX callback process (Java: processTxHandlerCallback) */
                if (comm->has_callbacks) {
                    if (packet.command == LCB_CMD_SET_MOTOR_RPM) {
                        lcb_proc_set_treadmill_rpm_value(&comm->callbacks, &packet);
                    } else if (packet.command == LCB_CMD_SET_INCLINE_LOC) {
                        lcb_proc_set_treadmill_adc_value(&comm->callbacks, &packet);
                    }
                }

                /* Store current command + address (Java: _txCommand = packet.getCommand();
                 * _txLcbAddr = packet.getTxAddress()) — the address is what
                 * decides, on response, whether this is a main-LCB or IR
                 * Address board (0x26) reply, since both reuse opcodes. */
                comm->tx_command = packet.command;
                comm->last_tx_address = packet.address;

                /* Wait for RX response with timeout
                 * 300ms for EUPS/SerialNumber, 150ms for others (Java: _notifyRxPacketSync.wait) */
                {
                    struct timespec ts;
                    clock_gettime(CLOCK_REALTIME, &ts);
                    long timeout_ms = LCB_RECEIVE_TIMEOUT_MS;
                    if (comm->tx_command == LCB_CMD_SET_CMD_EUPS_MODE ||
                        comm->tx_command == LCB_CMD_SET_CMD_SERIAL_NUMBER) {
                        timeout_ms = LCB_RECEIVE_TIMEOUT_EUPS_MS;
                    }
                    ts.tv_sec += timeout_ms / 1000;
                    ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
                    if (ts.tv_nsec >= 1000000000L) {
                        ts.tv_sec += 1;
                        ts.tv_nsec -= 1000000000L;
                    }

                    pthread_mutex_lock(&comm->rx_sync_mutex);
                    if (lcb_packet_queue_is_empty(&comm->rx_queue)) {
                        pthread_cond_timedwait(&comm->rx_sync_cond,
                                               &comm->rx_sync_mutex, &ts);
                    }
                    bool has_rx_packet = !lcb_packet_queue_is_empty(&comm->rx_queue);
                    pthread_mutex_unlock(&comm->rx_sync_mutex);

                    if (!comm->is_running) {
                        break;
                    }

                    if (has_rx_packet) {
                        LCB_LOGD(TAG, "RX response received for cmd=0x%02X", comm->tx_command);
                        /* Response received - delay between commands
                         * (Java: Thread.sleep(CMD_DELAY_TIME_MS)) */
                        usleep(LCB_CMD_DELAY_TIME_MS * 1000);

                        /* Dequeue RX packet */
                        lcb_packet_t rx_packet;
                        if (lcb_packet_queue_remove(&comm->rx_queue, &rx_packet) == 0) {
                            /* Reset retry total counter (Java: _retryTotalCounter = 0) */
                            comm->retry_total_counter = 0;

                            /* Process received packet — route to the IR
                             * Address board dispatch if the just-sent request
                             * targeted 0x26 (Java: if (_txLcbAddr ==
                             * IR_BOARD_ADDRESS_BYTE) processIrAddrPacket(...)
                             * else processPacket(...)) */
                            if (comm->last_tx_address == LCB_PACKET_ADDRESS_IR_BOARD) {
                                lcb_process_ir_addr_received_packet(comm, &rx_packet);
                            } else {
                                lcb_process_received_packet(comm, &rx_packet);
                            }
                            lcb_packet_free(&rx_packet);
                        }

                        break;  /* Success - exit retry loop */
                    } else {
                        /* Timeout - no response, retry after delay
                         * (Java: Thread.sleep(RETRY_DELAY_TIME_MS)) */
                        LCB_LOGE(TAG, "RX timeout for cmd=0x%02X (retry %d/%d)", comm->tx_command, retries, LCB_LIMITED_ONE_CMD_COMM_RETRIES);
                        usleep(LCB_RETRY_DELAY_TIME_MS * 1000);
                    }
                }

                /* Increment counters (Java: _retryTotalCounter++; retries++) */
                comm->retry_total_counter++;
                retries++;
            }

            /* Check if command failed after max retries
             * (Java: if (retries >= LIMITED_ONE_CMD_COMM_RETRIES)) */
            if (retries >= LCB_LIMITED_ONE_CMD_COMM_RETRIES) {
                LCB_LOGE(TAG, "Command 0x%02X failed after %d retries", comm->tx_command, retries);
                /* Handle retry failure for specific commands */
                if (comm->tx_command == LCB_CMD_SET_CMD_EUPS_MODE) {
                    /* ERP leave blocked - procLeaveErpBlockedLcbError() */
                    if (comm->blocked_flag && comm->lcb_erp_state == LCB_LEAVE_ERP) {
                        comm->wake_up_lcb_counter++;
                        if (comm->wake_up_lcb_counter < LCB_WAKE_UP_LCB_RETRIES) {
                            lcb_comm_set_lcb_cmd_eups_mode(comm, LCB_LEAVE_ERP, true);
                        } else {
                            comm->blocked_flag = false;
                            comm->retry_total_counter = 0;
                            if (comm->has_callbacks) {
                                lcb_proc_comm_lcb_error(&comm->callbacks);
                            }
                        }
                    }
                } else if (comm->tx_command == LCB_CMD_GET_STATUS) {
                    /* ERP check LCB status - procLeaveErpCheckLcbStatus() */
                    if (comm->blocked_flag && comm->lcb_erp_state == LCB_LEAVE_ERP) {
                        comm->leave_erp_get_lcb_status_counter++;
                        if (comm->leave_erp_get_lcb_status_counter < LCB_LEAVE_ERP_GET_LCB_STATUS_RETRIES) {
                            lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ALWAYS_ADD);
                        } else {
                            comm->blocked_flag = false;
                            comm->retry_total_counter = 0;
                            if (comm->has_callbacks) {
                                lcb_proc_comm_lcb_error(&comm->callbacks);
                            }
                        }
                    }
                }
            }

            /* Check total retry counter limit
             * (Java: if (_retryTotalCounter >= LIMITED_TOTAL_RETRY_COUNTER)) */
            if (comm->retry_total_counter >= LCB_LIMITED_TOTAL_RETRY_COUNTER) {
                LCB_LOGE(TAG, "Total retry limit exceeded: %d retries", comm->retry_total_counter);
                /* Communication error - too many total retries */
                if (comm->has_callbacks) {
                    lcb_proc_comm_lcb_error(&comm->callbacks);
                }
            }

            /* Free packet resources */
            lcb_packet_free(&packet);
        }
    }

    comm->comm_thread_except_flag = false;
    return NULL;
}

/**
 * @brief Callback invoked by lcb_parser_parse_data() for each complete packet
 *
 * Clones the parsed packet and adds it to the RX packet queue,
 * then signals the comm thread.
 * (Java: _rxPacketQueue.add() + _notifyRxPacketSync.notify())
 */
static void lcb_on_rx_packet_parsed(const lcb_packet_t *packet, void *user_data)
{
    lcb_comm_t *comm = (lcb_comm_t*)user_data;
    lcb_packet_t rx_packet;

    LCB_LOGD(TAG, "Packet parsed: cmd=0x%02X status=0x%02X len=%u", packet->command, packet->rx_lcb_status, packet->data_length);

    if (lcb_parser_clone_packet(&rx_packet, &comm->parser) == 0) {
        pthread_mutex_lock(&comm->rx_sync_mutex);
        lcb_packet_queue_add(&comm->rx_queue, &rx_packet);
        pthread_cond_signal(&comm->rx_sync_cond);
        pthread_mutex_unlock(&comm->rx_sync_mutex);

        lcb_packet_free(&rx_packet);
    }
}

/**
 * @brief Process raw byte data through the parser
 *
 * Feeds each byte to the parser. When a complete packet is parsed,
 * lcb_on_rx_packet_parsed() is called to queue it.
 *
 * Java equivalent: receiveDataHandler(byte[] data, int length) in LcbComm.java
 */
static void lcb_receive_data_handler(lcb_comm_t *comm,
                                      const uint8_t *data, uint32_t length)
{
    LCB_LOGD(TAG, "Processing %u raw bytes through parser", length);
    lcb_parser_parse_data(&comm->parser, data, length,
                           lcb_on_rx_packet_parsed, comm);
}

/**
 * @brief Process a receive data buffer
 *
 * Extracts the data and length from the buffer and passes to the handler.
 *
 * Java equivalent: processRceiveDataBuffer(ReceiveDataBuffer dataBuffer) in LcbComm.java
 */
static void lcb_process_receive_data_buffer(lcb_comm_t *comm,
                                             const lcb_receive_data_buffer_t *buf)
{
    lcb_receive_data_handler(comm,
        lcb_receive_data_buffer_get_data(buf),
        lcb_receive_data_buffer_get_length(buf));
}

/**
 * @brief RX thread - receives data from serial port
 *
 * Uses ReceiveDataBuffer/Queue pattern matching Java's LcbRxThread:
 * 1. Read raw bytes from serial port
 * 2. Clone into ReceiveDataBuffer and add to queue
 * 3. Dequeue and process through parser
 */
static void* lcb_rx_thread_func(void *arg)
{
    lcb_comm_t *comm = (lcb_comm_t*)arg;
    ssize_t bytes_read;

    while (comm->is_running) {
        /* Read from serial port (Java: bytes = _inStream.read(rxBuffer)) */
        bytes_read = read(comm->port_fd, comm->rx_buffer, LCB_RX_BUFFER_SIZE);

        if (bytes_read > 0) {
            /* Clone raw data into buffer queue
             * (Java: _receiveDataBufferQueue.add(
             *          _receiveDataBuffer.clone(rxBuffer, bytes))) */
            lcb_receive_data_queue_add(&comm->rx_data_queue,
                                        comm->rx_buffer, (uint32_t)bytes_read);
        } else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            /* Error reading from port */
            LCB_LOGE(TAG, "Serial port read error: errno=%d", errno);
            comm->comm_thread_except_flag = true;
            break;
        }

        /* Process queued buffers
         * (Java: if (_receiveDataBufferQueue.size() > 0)
         *          processRceiveDataBuffer(_receiveDataBufferQueue.remove())) */
        while (!lcb_receive_data_queue_is_empty(&comm->rx_data_queue)) {
            lcb_receive_data_buffer_t buf;
            if (lcb_receive_data_queue_remove(&comm->rx_data_queue, &buf) == 0) {
                lcb_process_receive_data_buffer(comm, &buf);
                lcb_receive_data_buffer_free(&buf);
            }
        }

        /* Small delay to prevent busy loop */
        usleep(1000);
    }

    return NULL;
}

/**
 * @brief ERP stop thread - handles ERP mode control
 */
static void* lcb_erp_stop_thread_func(void *arg)
{
    lcb_comm_t *comm = (lcb_comm_t*)arg;

    while (comm->is_running) {
        pthread_mutex_lock(&comm->erp_stop_mutex);
        pthread_cond_wait(&comm->erp_stop_cond, &comm->erp_stop_mutex);
        pthread_mutex_unlock(&comm->erp_stop_mutex);

        if (!comm->is_running) {
            break;
        }

        /* Java's UartPortErpStop() physically closes and 100ms-sleeps the
         * serial port here. Not implemented: this codebase has no
         * serial_port_close() anywhere (see the matching NOTE in
         * lcb_comm_leave_lcb_erp_mode()/lcb_comm_restart()) — closing a
         * port lcb_comm doesn't own would also race with comm_thread/
         * rx_thread still reading comm->port_fd. Out of scope until that
         * port-ownership/lifecycle infrastructure exists. */
    }

    return NULL;
}

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

/**
 * @brief Add command packet to TX queue with queue mode
 */
static int lcb_add_command_packet_with_mode(lcb_comm_t *comm, lcb_command_t cmd,
                                            const uint8_t *data, uint8_t length,
                                            uint8_t priority, lcb_queue_mode_t mode)
{
    if (comm == NULL) {
        return -1;
    }

    LCB_LOGD(TAG, "Adding TX packet: cmd=0x%02X mode=%d pri=%u", cmd, mode, priority);

    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, cmd);

    if (data != NULL && length > 0) {
        lcb_packet_set_data(&packet, data, length);
    }

    lcb_packet_set_priority(&packet, priority);

    int result;
    switch (mode) {
        case LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED:
            result = lcb_packet_queue_add_if_not_queued(&comm->tx_queue, &packet);
            break;
        case LCB_QUEUE_MODE_REPLACE_SIMILAR:
            result = lcb_packet_queue_replace_command(&comm->tx_queue, &packet);
            break;
        case LCB_QUEUE_MODE_ALWAYS_ADD:
        default:
            result = lcb_packet_queue_add(&comm->tx_queue, &packet);
            break;
    }

    /* Signal that packet is available */
    pthread_cond_signal(&comm->tx_sync_cond);

    lcb_packet_free(&packet);

    return result;
}

/**
 * @brief Add command packet to TX queue (always add mode)
 */
static int lcb_add_command_packet(lcb_comm_t *comm, lcb_command_t cmd,
                                  const uint8_t *data, uint8_t length,
                                  uint8_t priority)
{
    return lcb_add_command_packet_with_mode(comm, cmd, data, length,
                                            priority, LCB_QUEUE_MODE_ALWAYS_ADD);
}

/**
 * @brief Add command packet to TX queue, addressed to the IR Address board
 * (0x26) instead of the main LCB. Otherwise identical to
 * lcb_add_command_packet_with_mode() — same queue, same priority/mode
 * handling, just a different destination address on the packet.
 */
static int lcb_add_ir_addr_command_packet_with_mode(lcb_comm_t *comm, lcb_command_t cmd,
                                                    const uint8_t *data, uint8_t length,
                                                    uint8_t priority, lcb_queue_mode_t mode)
{
    if (comm == NULL) {
        return -1;
    }

    LCB_LOGD(TAG, "Adding IR Addr TX packet: cmd=0x%02X mode=%d pri=%u", cmd, mode, priority);

    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, cmd);
    lcb_packet_set_address(&packet, LCB_PACKET_ADDRESS_IR_BOARD);

    if (data != NULL && length > 0) {
        lcb_packet_set_data(&packet, data, length);
    }

    lcb_packet_set_priority(&packet, priority);

    int result;
    switch (mode) {
        case LCB_QUEUE_MODE_ADD_IF_SAME_NOT_QUEUED:
            result = lcb_packet_queue_add_if_not_queued(&comm->tx_queue, &packet);
            break;
        case LCB_QUEUE_MODE_REPLACE_SIMILAR:
            result = lcb_packet_queue_replace_command(&comm->tx_queue, &packet);
            break;
        case LCB_QUEUE_MODE_ALWAYS_ADD:
        default:
            result = lcb_packet_queue_add(&comm->tx_queue, &packet);
            break;
    }

    pthread_cond_signal(&comm->tx_sync_cond);
    lcb_packet_free(&packet);

    return result;
}

/* ============================================================================
 * IR Address Board (0x26) Public API — "Hybrid TM" IR beam E-Stop sensor
 * ============================================================================ */

void lcb_comm_get_ir_addr_version(lcb_comm_t *comm)
{
    lcb_add_ir_addr_command_packet_with_mode(comm, LCB_CMD_GET_VERSION, NULL, 0,
                                             LCB_PRIORITY_NORMAL, LCB_QUEUE_MODE_ALWAYS_ADD);
}

void lcb_comm_get_ir_addr_ir_status(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_ir_addr_command_packet_with_mode(comm, LCB_CMD_GET_IR_ADDRESS_IR_STATUS, NULL, 0,
                                             LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_ir_addr_ir_status_highest_priority(lcb_comm_t *comm)
{
    lcb_add_ir_addr_command_packet_with_mode(comm, LCB_CMD_GET_IR_ADDRESS_IR_STATUS, NULL, 0,
                                             LCB_PRIORITY_HIGHEST, LCB_QUEUE_MODE_ALWAYS_ADD);
}

void lcb_comm_set_ir_addr_enter_erp_mode(lcb_comm_t *comm)
{
    if (comm == NULL) return;
    uint8_t cmd = LCB_CMD_EUPS_MODE_NOTIFY_ENTER_EUPS;
    comm->ir_addr_erp_state = cmd;
    lcb_add_ir_addr_command_packet_with_mode(comm, LCB_CMD_SET_CMD_EUPS_MODE, &cmd, 1,
                                             LCB_PRIORITY_NORMAL, LCB_QUEUE_MODE_ALWAYS_ADD);
}

void lcb_comm_set_ir_addr_leave_erp_mode(lcb_comm_t *comm)
{
    if (comm == NULL) return;
    uint8_t cmd = LCB_CMD_EUPS_MODE_ASK_LEAVE_EUPS;
    comm->ir_addr_erp_state = cmd;
    /* Java: "cannot process any commands until woken!" -> highest priority */
    lcb_add_ir_addr_command_packet_with_mode(comm, LCB_CMD_SET_CMD_EUPS_MODE, &cmd, 1,
                                             LCB_PRIORITY_HIGHEST, LCB_QUEUE_MODE_ALWAYS_ADD);
}

/**
 * @brief Process received packet and invoke callbacks
 */
static void lcb_process_received_packet(lcb_comm_t *comm, const lcb_packet_t *packet)
{
    if (comm == NULL || packet == NULL || !comm->has_callbacks) {
        return;
    }

    /* checkPacketInitial: reset not_support_cmd before status check */
    comm->not_support_cmd = false;

    /* Check status bits (may set not_support_cmd via bit 5) */
    lcb_check_status_bits(comm, packet->rx_lcb_status);

    /* If command not supported, skip processing */
    if (comm->not_support_cmd) {
        return;
    }

    /* Data length limit (Java: length > RECEIVE_DATA_LENGTH_LIMITED_MAX) needs no
     * check here: the length field is uint8_t and the limit is 255. */

    const lcb_comm_callbacks_t *cb = &comm->callbacks;

    /* Process based on command */
    switch (packet->command) {
        /* System commands 0x70-0x7E */
        case LCB_CMD_SET_INITIALIZE:
            lcb_proc_set_initial(cb, packet);
            break;
        case LCB_CMD_GET_STATUS:
            lcb_proc_get_status(cb, packet);
            break;
        case LCB_CMD_GET_ERROR_CODE:
            lcb_proc_get_error_code(cb, packet);
            break;
        case LCB_CMD_GET_VERSION:
            lcb_proc_get_version(cb, packet);
            break;
        case LCB_CMD_SET_CALIBRATE:
            lcb_proc_set_calibrate(cb, packet);
            break;
        case LCB_CMD_SET_UPDATE_PROGRAM:
            lcb_proc_set_update_program(cb, packet);
            break;
        case LCB_CMD_SET_SKIP_CURR_ERROR:
            lcb_proc_set_skip_curr_error(cb, packet);
            break;
        case LCB_CMD_SET_SPE_EXT_CMD:
            lcb_proc_set_spe_ext_cmd(cb, packet);
            break;
        case LCB_CMD_GET_DCI_VERSION:
            lcb_proc_get_dci_version(cb, packet);
            break;
        case LCB_CMD_GET_DCI_ENV:
            lcb_proc_get_dci_env(cb, packet);
            break;
        case LCB_CMD_SET_CMD_EUPS_MODE:
            lcb_proc_set_cmd_eups_mode_handler(comm, packet);
            break;
        case LCB_CMD_SET_CMD_CONSOLE_POWER:
            lcb_proc_set_cmd_console_power(cb, packet);
            break;
        case LCB_CMD_GET_PROTOCOL_VERSION:
            lcb_proc_get_protocol_version(cb, packet);
            break;
        case LCB_CMD_SET_CMD_SERIAL_NUMBER:
            lcb_proc_set_cmd_serial_number(cb, packet);
            break;
        case LCB_CMD_SET_TREADMILL_OPERATE:
            lcb_proc_set_treadmill_operate(cb, packet);
            break;

        /* Treadmill motor commands 0xF0-0xFE */
        case LCB_CMD_SET_MOTOR_SPEED_RTN_RPM:
            lcb_proc_set_motor_speed_rtn_rpm(cb, packet);
            break;
        case LCB_CMD_SET_MOTOR_RPM:
            lcb_proc_set_motor_rpm(cb, packet);
            break;
        case LCB_CMD_SET_INC_PWM_STEP:
            lcb_proc_set_inc_pwm_step(cb, packet);
            break;
        case LCB_CMD_SET_DEC_PWM_STEP:
            lcb_proc_set_dec_pwm_step(cb, packet);
            break;
        case LCB_CMD_SET_STOP_PWM_STEP:
            lcb_proc_set_stop_pwm_step(cb, packet);
            break;
        case LCB_CMD_SET_INCLINE_ACTION:
            lcb_proc_set_incline_action(cb, packet);
            break;
        case LCB_CMD_SET_INCLINE_LOC:
            lcb_proc_set_incline_loc(cb, packet);
            break;
        case LCB_CMD_SET_OPERATING_STATUS:
            lcb_proc_set_operating_status(cb, packet);
            break;
        case LCB_CMD_GET_ROLLER_RPM:
            lcb_proc_get_roller_rpm(cb, packet);
            break;
        case LCB_CMD_GET_MOTOR_RPM:
            /* OneShot routing for motor RPM */
            if (comm->is_one_shot_rpm) {
                comm->is_one_shot_rpm = false;
                lcb_proc_get_one_shot_motor_rpm(cb, packet);
            } else {
                lcb_proc_get_motor_rpm(cb, packet);
            }
            break;
        case LCB_CMD_GET_INCLINE_LOC:
            lcb_proc_get_incline_loc(cb, packet);
            break;
        case LCB_CMD_SET_TUNE_END_INCLINE:
            lcb_proc_set_tune_end_incline(cb, packet);
            break;
        case LCB_CMD_SET_TUNE_END_INCLINE2:
            lcb_proc_set_tune_end_incline2(cb, packet);
            break;
        case LCB_CMD_SET_INCLINE_STROKE:
            lcb_proc_set_incline_stroke(cb, packet);
            break;
        case LCB_CMD_SET_COMPENSATION_VOL:
            lcb_proc_set_compensation_vol(cb, packet);
            break;

        /* Motor/DCI commands 0x90-0xAF */
        case LCB_CMD_SET_MOTOR_SPECIFICATION:
            lcb_proc_set_motor_specification(cb, packet);
            break;
        case LCB_CMD_GET_MOTOR_TYPE:
            lcb_proc_get_motor_type(cb, packet);
            break;
        case LCB_CMD_GET_DCI_DATA_MAX_LEN:
            lcb_proc_get_dci_data_max_len(cb, packet);
            break;
        case LCB_CMD_SET_MOTOR_POWER_OFF:
            lcb_proc_set_motor_power_off(cb, packet);
            break;
        case LCB_CMD_SET_DCI_FORCE_INCLINE:
            lcb_proc_set_dci_force_incline(cb, packet);
            break;
        case LCB_CMD_GET_TREADMILL_IN_USED:
            lcb_proc_get_treadmill_in_used(cb, packet);
            break;
        case LCB_CMD_GET_MAIN_MOTOR_INFO:
            lcb_proc_get_main_motor_info(cb, packet);
            break;
        case LCB_CMD_GET_TREADMILL_EXT_STATUS:
            lcb_proc_get_treadmill_ext_status(cb, packet);
            break;
        case LCB_CMD_GET_TREADMILL_CADENCE:
            lcb_proc_get_treadmill_cadence(cb, packet);
            break;
        case LCB_CMD_SET_TREADMILL_ECB_ACTION:
            lcb_proc_set_treadmill_ecb_action(cb, packet);
            break;
        case LCB_CMD_GET_DCI_INCLINE_RANGE:
            lcb_proc_get_dci_incline_range(cb, packet);
            break;
        case LCB_CMD_SET_DCI_INCLINE_RANGE:
            lcb_proc_set_dci_incline_range(cb, packet);
            break;
        case LCB_CMD_GET_DCI_RPM_SPEED_RANGE:
            lcb_proc_get_dci_rpm_speed_range(cb, packet);
            break;
        case LCB_CMD_SET_DCI_RPM_SPEED_RANGE:
            lcb_proc_set_dci_rpm_speed_range(cb, packet);
            break;
        case LCB_CMD_SET_UNIVERSAL:
            lcb_proc_set_universal(cb, packet);
            break;
        case LCB_CMD_SET_DCI_MANUAL_CALIBRATION:
            lcb_proc_set_dci_manual_calibration(cb, packet);
            break;

        /* Device data and indoor bike commands 0x50-0x5F */
        case LCB_CMD_SET_DEVICE_DATA:
            lcb_proc_set_device_data(cb, packet);
            break;
        case LCB_CMD_GET_INDOOR_BIKE_SENSOR_DATA:
            /* OneShot routing for sensor data */
            if (comm->is_one_shot_sensor_data) {
                comm->is_one_shot_sensor_data = false;
                lcb_proc_get_one_shot_indoor_bike_sensor_data(cb, packet);
            } else {
                lcb_proc_get_indoor_bike_sensor_data(cb, packet);
            }
            break;
        case LCB_CMD_SET_INDOOR_BIKE_STEPPER_MOTOR_POSITION:
            lcb_proc_set_indoor_bike_stepper_motor_pos(cb, packet);
            break;
        case LCB_CMD_SET_INDOOR_BIKE_CALC_PARA_OF_DEVICE:
            lcb_proc_set_indoor_bike_calc_para(cb, packet);
            break;
        case LCB_CMD_GET_INDOOR_BIKE_CALC_PARA_OF_DEVICE:
            lcb_proc_get_indoor_bike_calc_para(cb, packet);
            break;
        case LCB_CMD_SET_INDOOR_BIKE_ACTION_SPIN_DOWN:
            lcb_proc_set_indoor_bike_action_spin_down(cb, packet);
            break;
        case LCB_CMD_GET_INDOOR_BIKE_INFO_SPIN_DOWN:
            lcb_proc_get_indoor_bike_info_spin_down(cb, packet);
            break;
        case LCB_CMD_SET_INDOOR_BIKE_ERG_MODE_SETTINGS:
            lcb_proc_set_indoor_bike_erg_mode_settings(cb, packet);
            break;
        case LCB_CMD_GET_INDOOR_BIKE_ERG_MODE_SETTINGS:
            lcb_proc_get_indoor_bike_erg_mode_settings(cb, packet);
            break;
        case LCB_CMD_SET_INDOOR_BIKE_RESISTANCE_LEVEL:
            lcb_proc_set_indoor_bike_resistance_level(cb, packet);
            break;

        /* ECB and bike commands 0x60-0x6F */
        case LCB_CMD_SET_RESET_ECB:
            lcb_proc_set_reset_ecb(cb, packet);
            break;
        case LCB_CMD_SET_ECB_POSITION:
            lcb_proc_set_ecb_position(cb, packet);
            break;
        case LCB_CMD_GET_ECB_RPM:
            /* OneShot routing for ECB RPM */
            if (comm->is_one_shot_rpm) {
                comm->is_one_shot_rpm = false;
                lcb_proc_get_one_shot_ecb_rpm(cb, packet);
            } else {
                lcb_proc_get_ecb_rpm(cb, packet);
            }
            break;
        case LCB_CMD_GET_ECB_ADC:
            lcb_proc_get_ecb_adc(cb, packet);
            break;
        case LCB_CMD_SET_PWM_PERCENT:
            lcb_proc_set_pwm_percent(cb, packet);
            break;
        case LCB_CMD_GET_BATTERY_VOLTAGE:
            lcb_proc_get_battery_voltage(cb, packet);
            break;
        case LCB_CMD_SET_WATTS:
            lcb_proc_set_watts_cmd(cb, packet);
            break;
        case LCB_CMD_SET_RPM_GEAR_RATIO:
            lcb_proc_set_rpm_gear_ratio(cb, packet);
            break;
        case LCB_CMD_SET_GENERATOR_POLE_PAIR:
            lcb_proc_set_generator_pole_pair(cb, packet);
            break;
        case LCB_CMD_SET_LIMIT_RPM_CHARGE:
            lcb_proc_set_limit_rpm_charge(cb, packet);
            break;
        case LCB_CMD_SET_LIMIT_RPM_RESISTANCE:
            lcb_proc_set_limit_rpm_resistance(cb, packet);
            break;
        case LCB_CMD_SET_MACHINE_TYPE:
            lcb_proc_set_machine_type(cb, packet);
            break;
        case LCB_CMD_SET_POWER_OFF:
            lcb_proc_set_power_off(cb, packet);
            break;
        case LCB_CMD_SET_BATTERY_CHARGE_LEVEL:
            lcb_proc_set_battery_charge_level(cb, packet);
            break;
        case LCB_CMD_SET_MIN_RESISTANCE_INCLINE_WORKING:
            lcb_proc_set_min_resistance_incline_working(cb, packet);
            break;

        /* Incline and ECB commands 0x80-0x8F */
        case LCB_CMD_SET_INCLINE_PERCENT:
            lcb_proc_set_incline_percent(cb, packet);
            break;
        case LCB_CMD_GET_INCLINE_PERCENT:
            lcb_proc_get_incline_percent(cb, packet);
            break;
        case LCB_CMD_SET_ECB_ACTION:
            lcb_proc_set_ecb_action(cb, packet);
            break;
        case LCB_CMD_GET_ECB_STATUS:
            lcb_proc_get_ecb_status(cb, packet);
            break;
        case LCB_CMD_GET_ECB_COUNT:
            lcb_proc_get_ecb_count(cb, packet);
            break;
        case LCB_CMD_SET_ELECTROMAGNETIC_CURRENT:
            lcb_proc_set_electromagnetic_current(cb, packet);
            break;
        case LCB_CMD_SET_RESISTANCE_TYPE:
            lcb_proc_set_resistance_type(cb, packet);
            break;
        case LCB_CMD_SET_BEGIN_BATTERY_CHARGE:
            lcb_proc_set_begin_battery_charge(cb, packet);
            break;
        case LCB_CMD_GET_BATTERY_CAPACITY:
            lcb_proc_get_battery_capacity(cb, packet);
            break;
        case LCB_CMD_SET_CLIMBMILL_STATUS:
            lcb_proc_set_climbmill_status(cb, packet);
            break;
        case LCB_CMD_GET_CLIMBMILL_STATUS:
            lcb_proc_get_climbmill_status(cb, packet);
            break;
        case LCB_CMD_SET_E_STOP_PARAMETER:
            lcb_proc_set_e_stop_parameter(cb, packet);
            break;
        case LCB_CMD_GET_BRAKE_PRESSURE_VALUE:
            lcb_proc_get_brake_pressure_value(cb, packet);
            break;
        case LCB_CMD_GET_WATTS:
            lcb_proc_get_watts(cb, packet);
            break;
        case LCB_CMD_SET_CLIMBMILL_CAL:
            lcb_proc_set_climbmill_cal(cb, packet);
            break;

        /* Calibration commands 0x40-0x4F */
        case LCB_CMD_SET_RECORD_CURRENT:
            lcb_proc_set_record_current(cb, packet);
            break;
        case LCB_CMD_SET_PWM:
            lcb_proc_set_pwm(cb, packet);
            break;
        case LCB_CMD_SET_FINISH_CALIBRATING:
            lcb_proc_set_finish_calibrating(cb, packet);
            break;
        case LCB_CMD_SET_SPM:
            lcb_proc_set_spm(cb, packet);
            break;
        case LCB_CMD_GET_SPM:
            /* OneShot routing for SPM */
            if (comm->is_one_shot_spm) {
                comm->is_one_shot_spm = false;
                lcb_proc_get_one_shot_spm(cb, packet);
            } else {
                lcb_proc_get_spm(cb, packet);
            }
            break;
        case LCB_CMD_SET_CAL_SCALING:
            lcb_proc_set_cal_scaling(cb, packet);
            break;
        case LCB_CMD_GET_CAL_SCALING:
            lcb_proc_get_cal_scaling(cb, packet);
            break;
        case LCB_CMD_SET_FRAME_CAL_SCALING:
            lcb_proc_set_frame_cal_scaling(cb, packet);
            break;
        case LCB_CMD_GET_FRAME_CAL_SCALING:
            lcb_proc_get_frame_cal_scaling(cb, packet);
            break;

        default:
            if (comm->callbacks.on_unknown_command != NULL) {
                uint8_t unk_len;
                const uint8_t *unk_data = lcb_packet_get_data(packet, &unk_len);
                comm->callbacks.on_unknown_command(comm->callbacks.user_data,
                                                   packet->command, unk_data, unk_len);
            }
            break;
    }
}

/**
 * @brief Process a received packet whose request targeted the IR Address
 * board (0x26) instead of the main LCB (Java: processIrAddrPacket()).
 * Mirrors lcb_process_received_packet()'s status-bit/length guards exactly
 * (chimera2 reuses the same checkStatusByteDefinition() for both), but
 * dispatches through a separate 3-case switch since 0x73/0x7A here mean "IR
 * board" versions of those commands, not the main-LCB ones.
 */
static void lcb_process_ir_addr_received_packet(lcb_comm_t *comm, const lcb_packet_t *packet)
{
    if (comm == NULL || packet == NULL || !comm->has_callbacks) {
        return;
    }

    comm->not_support_cmd = false;
    lcb_check_status_bits(comm, packet->rx_lcb_status);
    if (comm->not_support_cmd) {
        return;
    }

    /* Data length limit (Java: length > RECEIVE_DATA_LENGTH_LIMITED_MAX) needs no
     * check here: the length field is uint8_t and the limit is 255. */

    const lcb_comm_callbacks_t *cb = &comm->callbacks;

    switch (packet->command) {
        case LCB_CMD_GET_VERSION:
            lcb_proc_get_ir_addr_version(cb, packet);
            break;
        case LCB_CMD_GET_IR_ADDRESS_IR_STATUS:
            lcb_proc_get_ir_addr_ir_status(cb, packet);
            break;
        case LCB_CMD_SET_CMD_EUPS_MODE:
            lcb_proc_set_ir_addr_cmd_eups_mode_handler(comm, packet);
            break;
        default:
            break;
    }
}

/**
 * @brief Check and notify status bit changes
 */
static void lcb_check_status_bits(lcb_comm_t *comm, uint8_t status)
{
    if (comm == NULL || !comm->has_callbacks) {
        return;
    }

    bool bit_value;

    /* Bit 6: LCB error - ALWAYS fire callback (not just on change), matching Java behavior */
    bit_value = (status & LCB_STATUS_BIT_LCB_ERROR) != 0;
    comm->lcb_error_status = bit_value;
    if (comm->callbacks.on_lcb_error_status_detected != NULL) {
        comm->callbacks.on_lcb_error_status_detected(comm->callbacks.user_data, bit_value);
    }

    /* Bit 0: Incline motor downward - notify on change */
    bit_value = (status & LCB_STATUS_BIT_INCLINE_DOWN_ACTION) != 0;
    if (bit_value != comm->incline_motor_downward_status) {
        comm->incline_motor_downward_status = bit_value;
        if (comm->callbacks.on_incline_motor_downward_status_changed != NULL) {
            comm->callbacks.on_incline_motor_downward_status_changed(
                comm->callbacks.user_data, bit_value);
        }
    }

    /* Bit 1: Incline motor upward - notify on change */
    bit_value = (status & LCB_STATUS_BIT_INCLINE_UP_ACTION) != 0;
    if (bit_value != comm->incline_motor_upward_status) {
        comm->incline_motor_upward_status = bit_value;
        if (comm->callbacks.on_incline_motor_upward_status_changed != NULL) {
            comm->callbacks.on_incline_motor_upward_status_changed(
                comm->callbacks.user_data, bit_value);
        }
    }

    /* Bit 2: Main motor - notify on change */
    bit_value = (status & LCB_STATUS_BIT_MAIN_MOTOR_ACTION) != 0;
    if (bit_value != comm->main_motor_status) {
        comm->main_motor_status = bit_value;
        if (comm->callbacks.on_main_motor_status_changed != NULL) {
            comm->callbacks.on_main_motor_status_changed(
                comm->callbacks.user_data, bit_value);
        }
    }

    /* Bit 3: Encoder RPM / Power status AC - notify on change */
    bit_value = (status & LCB_STATUS_BIT_POWER_STATUS_AC) != 0;
    if (bit_value != comm->encoder_rpm_status) {
        comm->encoder_rpm_status = bit_value;
        if (comm->callbacks.on_encoder_rpm_status_changed != NULL) {
            comm->callbacks.on_encoder_rpm_status_changed(
                comm->callbacks.user_data, bit_value);
        }
    }

    /* Bit 4: Battery capacity not enough - notify on change */
    bit_value = (status & LCB_STATUS_BIT_BATTERY_CAP_NOT_ENOUGH) != 0;
    if (bit_value != comm->battery_capacity_status) {
        comm->battery_capacity_status = bit_value;
        if (comm->callbacks.on_battery_capacity_status_changed != NULL) {
            comm->callbacks.on_battery_capacity_status_changed(
                comm->callbacks.user_data, bit_value);
        }
    }

    /* Bit 5: Command error - notify on change, set not_support_cmd flag */
    bit_value = (status & LCB_STATUS_BIT_COMMAND_ERROR) != 0;
    if (bit_value != comm->command_error_status) {
        comm->command_error_status = bit_value;
        if (comm->callbacks.on_command_error_status_changed != NULL) {
            comm->callbacks.on_command_error_status_changed(
                comm->callbacks.user_data, bit_value);
        }
        if (bit_value) {
            comm->not_support_cmd = true;
        }
    }

    /* Bit 7: Initial calibration status - notify on change */
    bit_value = (status & LCB_STATUS_BIT_CALIBRATION_ACTION) != 0;
    if (bit_value != comm->initial_calibration_status) {
        comm->initial_calibration_status = bit_value;
        if (comm->callbacks.on_initial_calibration_status_changed != NULL) {
            comm->callbacks.on_initial_calibration_status_changed(
                comm->callbacks.user_data, bit_value);
        }
    }
}

/* ============================================================================
 * Command Functions - System Commands (0x70-0x7F)
 * ============================================================================ */

void lcb_comm_set_lcb_initial(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_INITIALIZE, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_initial_highest_priority(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_INITIALIZE, NULL, 0, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_get_lcb_status(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_STATUS, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_lcb_status_low_priority(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_STATUS, NULL, 0, LCB_PRIORITY_LOW, mode);
}

void lcb_comm_get_lcb_error_code(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_ERROR_CODE, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_error_code_high_priority(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_ERROR_CODE, NULL, 0, LCB_PRIORITY_HIGH);
}

void lcb_comm_get_lcb_version(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_VERSION, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_version_highest_priority(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_VERSION, NULL, 0, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_set_lcb_calibrate(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_CALIBRATE, NULL, 0, LCB_PRIORITY_NORMAL);
}

/* ============================================================================
 * Motor Control Commands (0xF0-0xFF)
 * ============================================================================ */

void lcb_comm_set_lcb_motor_rpm(lcb_comm_t *comm, uint16_t rpm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(rpm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_MOTOR_RPM, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_motor_rpm_highest_priority(lcb_comm_t *comm, uint16_t rpm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(rpm, data);
    lcb_add_command_packet_with_mode(comm, LCB_CMD_SET_MOTOR_RPM, data, 2,
                                      LCB_PRIORITY_HIGHEST, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

void lcb_comm_set_lcb_operating_status(lcb_comm_t *comm, uint8_t status)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_OPERATING_STATUS, &status, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_operating_status_highest_priority(lcb_comm_t *comm, uint8_t status)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_OPERATING_STATUS, &status, 1, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_set_lcb_update_program(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_UPDATE_PROGRAM, data, length, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_skip_curr_error(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_SKIP_CURR_ERROR, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_skip_curr_error_high_priority(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_SKIP_CURR_ERROR, NULL, 0, LCB_PRIORITY_HIGH);
}

void lcb_comm_set_lcb_spe_ext_cmd(lcb_comm_t *comm, uint8_t cmd)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_SPE_EXT_CMD, &cmd, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_dci_version(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_DCI_VERSION, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_dci_env(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_DCI_ENV, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_cmd_eups_mode(lcb_comm_t *comm, uint8_t cmd, bool do_pulse)
{
    if (comm == NULL) return;

    lcb_packet_t packet;
    lcb_packet_init(&packet);
    lcb_packet_set_command(&packet, LCB_CMD_SET_CMD_EUPS_MODE);
    lcb_packet_set_data(&packet, &cmd, 1);

    uint8_t priority = (cmd == LCB_LEAVE_ERP) ? LCB_PRIORITY_HIGHEST : LCB_PRIORITY_NORMAL;
    lcb_packet_set_priority(&packet, priority);

    if (do_pulse) {
        packet.special_operation = LCB_SPECIAL_400MS_PULSE;
    }

    lcb_packet_queue_add(&comm->tx_queue, &packet);
    pthread_cond_signal(&comm->tx_sync_cond);
    lcb_packet_free(&packet);
}

void lcb_comm_set_lcb_cmd_console_power(lcb_comm_t *comm, uint16_t watts)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(watts, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_CMD_CONSOLE_POWER, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_protocol_version(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_PROTOCOL_VERSION, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_cmd_serial_number(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_CMD_SERIAL_NUMBER, data, length, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_treadmill_operate(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_TREADMILL_OPERATE, data, length, LCB_PRIORITY_NORMAL);
}

void lcb_comm_enter_lcb_erp_mode(lcb_comm_t *comm)
{
    if (comm == NULL) {
        return;
    }
    lcb_comm_set_lcb_cmd_eups_mode(comm, LCB_ENTER_ERP, false);
    comm->lcb_erp_state = LCB_ENTER_ERP;
}

void lcb_comm_leave_lcb_erp_mode(lcb_comm_t *comm, const char *device_path, uint32_t baud_rate)
{
    (void)device_path;
    (void)baud_rate;
    if (comm == NULL) {
        return;
    }
    /* Wake pulse: do_pulse=true below sets packet.special_operation =
     * LCB_SPECIAL_400MS_PULSE, matching Java's setLcbCmdEUPsMode(LEAVE_ERP, true).
     * NOT implemented: reconfiguring/reopening the serial port with
     * device_path/baud_rate (Java: start(createLcbPort(path, baud))) — this
     * codebase has no serial_port_open()/close() anywhere (see the identical
     * TODO in lcb_comm_restart() above); physically power-cycling the UART
     * during ERP is out of scope until that infrastructure exists. */
    lcb_comm_set_lcb_cmd_eups_mode(comm, LCB_LEAVE_ERP, true);
    comm->lcb_erp_state = LCB_LEAVE_ERP;
    comm->blocked_flag = true;
}

void lcb_comm_check_boot_up_leave_lcb_erp(lcb_comm_t *comm)
{
    if (comm == NULL) {
        return;
    }
    comm->is_boot_up_leave_erp = true;
    comm->lcb_erp_state = LCB_LEAVE_ERP;
    comm->blocked_flag = true;
    /* TODO: Java's BootUpLeaveErpTimerTask retries setLcbCmdEUPsMode(LEAVE_ERP, ...)
     * every 1000ms up to 3 times (pulsing on the 2nd attempt) until
     * is_boot_up_leave_erp clears, then calls on_boot_up_leave_lcb_erp.
     * Not implemented: no periodic timer infrastructure for this exists yet
     * at the comm layer (general_handler's heartbeat thread is a Layer_3
     * concept and doesn't drive Layer_4 state). This function currently
     * only arms the state; nothing retries the request yet. */
}

void lcb_comm_erp_stop_uart_port(lcb_comm_t *comm)
{
    if (comm == NULL) {
        return;
    }
    /* Signal ERP stop thread */
    pthread_cond_signal(&comm->erp_stop_cond);
}

/* ============================================================================
 * Motor Control Commands (0xF0-0xFF) - Additional Functions
 * ============================================================================ */

void lcb_comm_set_lcb_motor_speed_rtn_rpm(lcb_comm_t *comm, uint16_t speed)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(speed, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_MOTOR_SPEED_RTN_RPM, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_inc_pwm_step(lcb_comm_t *comm, uint16_t pwm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(pwm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_INC_PWM_STEP, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_dec_pwm_step(lcb_comm_t *comm, uint16_t pwm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(pwm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_DEC_PWM_STEP, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_stop_pwm_step(lcb_comm_t *comm, uint16_t pwm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(pwm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_STOP_PWM_STEP, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_incline_action(lcb_comm_t *comm, uint8_t action)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_INCLINE_ACTION, &action, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_incline_action_highest_priority(lcb_comm_t *comm, uint8_t action)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_INCLINE_ACTION, &action, 1, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_set_lcb_incline_loc(lcb_comm_t *comm, uint16_t location)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(location, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_INCLINE_LOC, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_incline_loc_highest_priority(lcb_comm_t *comm, uint16_t location)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(location, data);
    lcb_add_command_packet_with_mode(comm, LCB_CMD_SET_INCLINE_LOC, data, 2,
                                      LCB_PRIORITY_HIGHEST, LCB_QUEUE_MODE_REPLACE_SIMILAR);
}

void lcb_comm_get_lcb_roller_rpm(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_ROLLER_RPM, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_motor_rpm(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_MOTOR_RPM, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_one_shot_lcb_motor_rpm_high_priority(lcb_comm_t *comm)
{
    if (comm != NULL) comm->is_one_shot_rpm = true;
    lcb_add_command_packet(comm, LCB_CMD_GET_MOTOR_RPM, NULL, 0, LCB_PRIORITY_HIGH);
}

void lcb_comm_get_lcb_incline_loc(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_INCLINE_LOC, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_tune_end_incline(lcb_comm_t *comm, uint16_t percentage)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(percentage, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_TUNE_END_INCLINE, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_tune_end_incline2(lcb_comm_t *comm, uint16_t percentage)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(percentage, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_TUNE_END_INCLINE2, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_incline_stroke(lcb_comm_t *comm, uint16_t percentage)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(percentage, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_INCLINE_STROKE, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_compensation_vol(lcb_comm_t *comm, uint16_t voltage)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(voltage, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_COMPENSATION_VOL, data, 2, LCB_PRIORITY_NORMAL);
}

/* ============================================================================
 * Motor and DCI Commands (0x90-0xAF)
 * ============================================================================ */

void lcb_comm_set_lcb_motor_specification(lcb_comm_t *comm, uint16_t watts)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(watts, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_MOTOR_SPECIFICATION, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_motor_type(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_MOTOR_TYPE, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_dci_data_max_len(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_DCI_DATA_MAX_LEN, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_motor_power_off(lcb_comm_t *comm, uint8_t time)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_MOTOR_POWER_OFF, &time, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_dci_force_incline(lcb_comm_t *comm, uint8_t value)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_DCI_FORCE_INCLINE, &value, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_treadmill_in_used(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_TREADMILL_IN_USED, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_lcb_main_motor_info(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_MAIN_MOTOR_INFO, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_lcb_treadmill_ext_status(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_TREADMILL_EXT_STATUS, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_lcb_treadmill_cadence(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_TREADMILL_CADENCE, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_treadmill_ecb_action(lcb_comm_t *comm, uint8_t value)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_TREADMILL_ECB_ACTION, &value, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_treadmill_ecb_action_highest_priority(lcb_comm_t *comm, uint8_t value)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_TREADMILL_ECB_ACTION, &value, 1, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_get_lcb_dci_incline_range(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_DCI_INCLINE_RANGE, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_dci_incline_range(lcb_comm_t *comm, uint32_t range)
{
    uint8_t data[4];
    lcb_uint32_to_bytes_be(range, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_DCI_INCLINE_RANGE, data, 4, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_dci_rpm_speed_range(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_DCI_RPM_SPEED_RANGE, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_dci_rpm_speed_range(lcb_comm_t *comm, uint32_t range)
{
    uint8_t data[4];
    lcb_uint32_to_bytes_be(range, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_DCI_RPM_SPEED_RANGE, data, 4, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_universal(lcb_comm_t *comm, const uint8_t *data, uint8_t length, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_SET_UNIVERSAL, data, length, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_dci_manual_calibration(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_DCI_MANUAL_CALIBRATION, NULL, 0, LCB_PRIORITY_NORMAL);
}

/* ============================================================================
 * Device Data and Indoor Bike Commands (0x50-0x5F)
 * ============================================================================ */

void lcb_comm_set_lcb_device_data(lcb_comm_t *comm, const uint8_t *data, uint8_t length, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_SET_DEVICE_DATA, data, length, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_device_data_highest_priority(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_DEVICE_DATA, data, length, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_get_lcb_indoor_bike_sensor_data(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_INDOOR_BIKE_SENSOR_DATA, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_one_shot_lcb_indoor_bike_sensor_data_high_priority(lcb_comm_t *comm)
{
    if (comm != NULL) comm->is_one_shot_sensor_data = true;
    lcb_add_command_packet(comm, LCB_CMD_GET_INDOOR_BIKE_SENSOR_DATA, NULL, 0, LCB_PRIORITY_HIGH);
}

void lcb_comm_set_lcb_indoor_bike_stepper_motor_position(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_INDOOR_BIKE_STEPPER_MOTOR_POSITION, data, length, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_indoor_bike_calc_para_of_device(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_INDOOR_BIKE_CALC_PARA_OF_DEVICE, data, length, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_indoor_bike_calc_para_of_device(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_INDOOR_BIKE_CALC_PARA_OF_DEVICE, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_indoor_bike_action_spin_down(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_INDOOR_BIKE_ACTION_SPIN_DOWN, data, length, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_indoor_bike_info_spin_down(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_INDOOR_BIKE_INFO_SPIN_DOWN, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_indoor_bike_erg_mode_settings(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_INDOOR_BIKE_ERG_MODE_SETTINGS, data, length, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_indoor_bike_resistance_level(lcb_comm_t *comm, uint16_t value)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(value, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_INDOOR_BIKE_RESISTANCE_LEVEL, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_indoor_bike_erg_mode_settings(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_INDOOR_BIKE_ERG_MODE_SETTINGS, NULL, 0, LCB_PRIORITY_NORMAL);
}

/* ============================================================================
 * ECB and Bike Commands (0x60-0x6F)
 * ============================================================================ */

void lcb_comm_set_lcb_reset_ecb(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_RESET_ECB, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_ecb_position(lcb_comm_t *comm, uint16_t position)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(position, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_ECB_POSITION, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_ecb_rpm(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_ECB_RPM, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_one_shot_lcb_ecb_rpm_high_priority(lcb_comm_t *comm)
{
    if (comm != NULL) comm->is_one_shot_rpm = true;
    lcb_add_command_packet(comm, LCB_CMD_GET_ECB_RPM, NULL, 0, LCB_PRIORITY_HIGH);
}

void lcb_comm_get_lcb_ecb_adc(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_ECB_ADC, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_pwm_percent(lcb_comm_t *comm, uint16_t value)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(value, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_PWM_PERCENT, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_pwm_percent_highest_priority(lcb_comm_t *comm, uint16_t value)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(value, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_PWM_PERCENT, data, 2, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_get_lcb_battery_voltage(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_BATTERY_VOLTAGE, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_battery_voltage_low_priority(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_BATTERY_VOLTAGE, NULL, 0, LCB_PRIORITY_LOW, mode);
}

void lcb_comm_set_lcb_watts(lcb_comm_t *comm, uint16_t watts, lcb_queue_mode_t mode)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(watts, data);
    lcb_add_command_packet_with_mode(comm, LCB_CMD_SET_WATTS, data, 2, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_watts_highest_priority(lcb_comm_t *comm, uint16_t watts)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(watts, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_WATTS, data, 2, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_set_lcb_rpm_gear_ratio(lcb_comm_t *comm, uint16_t ratio)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(ratio, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_RPM_GEAR_RATIO, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_generator_pole_pair(lcb_comm_t *comm, uint16_t pair)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(pair, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_GENERATOR_POLE_PAIR, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_limit_rpm_charge(lcb_comm_t *comm, uint16_t rpm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(rpm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_LIMIT_RPM_CHARGE, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_limit_rpm_resistance(lcb_comm_t *comm, uint16_t rpm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(rpm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_LIMIT_RPM_RESISTANCE, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_limit_rpm_resistance_highest_priority(lcb_comm_t *comm, uint16_t rpm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(rpm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_LIMIT_RPM_RESISTANCE, data, 2, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_set_lcb_machine_type(lcb_comm_t *comm, uint8_t type)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_MACHINE_TYPE, &type, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_power_off(lcb_comm_t *comm, uint16_t minutes)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(minutes, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_POWER_OFF, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_battery_charge_level(lcb_comm_t *comm, uint16_t level)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(level, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_BATTERY_CHARGE_LEVEL, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_min_resistance_incline_working(lcb_comm_t *comm, uint8_t type)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_MIN_RESISTANCE_INCLINE_WORKING, &type, 1, LCB_PRIORITY_NORMAL);
}

/* ============================================================================
 * Incline and ECB Commands (0x80-0x8F)
 * ============================================================================ */

void lcb_comm_set_lcb_incline_percent(lcb_comm_t *comm, uint16_t value)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(value, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_INCLINE_PERCENT, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_incline_percent(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_INCLINE_PERCENT, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_ecb_action(lcb_comm_t *comm, uint8_t type)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_ECB_ACTION, &type, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_ecb_status(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_ECB_STATUS, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_ecb_count(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_ECB_COUNT, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_electromagnetic_current(lcb_comm_t *comm, uint16_t ma)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(ma, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_ELECTROMAGNETIC_CURRENT, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_resistance_type(lcb_comm_t *comm, uint16_t ohm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(ohm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_RESISTANCE_TYPE, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_begin_battery_charge(lcb_comm_t *comm, uint16_t ma)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(ma, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_BEGIN_BATTERY_CHARGE, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_battery_capacity(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_BATTERY_CAPACITY, NULL, 0, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_climbmill_status(lcb_comm_t *comm, uint8_t value)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_CLIMBMILL_STATUS, &value, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_climbmill_status_highest_priority(lcb_comm_t *comm, uint8_t value)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_CLIMBMILL_STATUS, &value, 1, LCB_PRIORITY_HIGHEST);
}

void lcb_comm_get_lcb_climbmill_status(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_CLIMBMILL_STATUS, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_e_stop_parameter(lcb_comm_t *comm, uint8_t value)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_E_STOP_PARAMETER, &value, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_brake_pressure_value(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_BRAKE_PRESSURE_VALUE, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_lcb_watts(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_WATTS, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_climbmill_cal(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_CLIMBMILL_CAL, data, length, LCB_PRIORITY_NORMAL);
}

/* ============================================================================
 * Calibration Commands (0x40-0x4F)
 * ============================================================================ */

void lcb_comm_set_lcb_record_current(lcb_comm_t *comm, const uint8_t *data, uint8_t length)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_RECORD_CURRENT, data, length, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_pwm(lcb_comm_t *comm, uint16_t percentage)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(percentage, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_PWM, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_finish_calibrating(lcb_comm_t *comm, uint8_t value)
{
    lcb_add_command_packet(comm, LCB_CMD_SET_FINISH_CALIBRATING, &value, 1, LCB_PRIORITY_NORMAL);
}

void lcb_comm_set_lcb_spm(lcb_comm_t *comm, uint16_t spm)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(spm, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_SPM, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_spm(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_SPM, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_get_one_shot_lcb_spm_high_priority(lcb_comm_t *comm)
{
    if (comm != NULL) comm->is_one_shot_spm = true;
    lcb_add_command_packet(comm, LCB_CMD_GET_SPM, NULL, 0, LCB_PRIORITY_HIGH);
}

void lcb_comm_set_lcb_cal_scaling(lcb_comm_t *comm, uint16_t cal_scaling)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(cal_scaling, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_CAL_SCALING, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_cal_scaling(lcb_comm_t *comm, lcb_queue_mode_t mode)
{
    lcb_add_command_packet_with_mode(comm, LCB_CMD_GET_CAL_SCALING, NULL, 0, LCB_PRIORITY_NORMAL, mode);
}

void lcb_comm_set_lcb_frame_cal_scaling(lcb_comm_t *comm, uint16_t frame_cal_scaling)
{
    uint8_t data[2];
    lcb_uint16_to_bytes_be(frame_cal_scaling, data);
    lcb_add_command_packet(comm, LCB_CMD_SET_FRAME_CAL_SCALING, data, 2, LCB_PRIORITY_NORMAL);
}

void lcb_comm_get_lcb_frame_cal_scaling(lcb_comm_t *comm)
{
    lcb_add_command_packet(comm, LCB_CMD_GET_FRAME_CAL_SCALING, NULL, 0, LCB_PRIORITY_NORMAL);
}

/* ============================================================================
 * EUPS Mode Handler (internal)
 * ============================================================================ */

static void lcb_unblocked_lcb_error(lcb_comm_t *comm)
{
    comm->blocked_flag = false;
    comm->retry_total_counter = 0;
}

static void lcb_proc_set_cmd_eups_mode_handler(lcb_comm_t *comm, const lcb_packet_t *packet)
{
    if (comm == NULL || packet == NULL) return;

    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(packet, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_ERP_MODE) return;

    uint8_t erp_mode = data[0];

    /* First, invoke the callback to notify application */
    if (comm->has_callbacks && comm->callbacks.on_lcb_cmd_eups_mode != NULL) {
        comm->callbacks.on_lcb_cmd_eups_mode(comm->callbacks.user_data, erp_mode);
    }

    if (comm->is_boot_up_leave_erp) {
        /* Boot-up leave ERP path */
        if (erp_mode == LCB_CMD_EUPS_RTN_NOT_SUPPORT) {
            /* LCB not support ERP mode (0x00) */
            if (comm->lcb_erp_state == LCB_LEAVE_ERP) {
                lcb_unblocked_lcb_error(comm);
                if (comm->has_callbacks && comm->callbacks.on_boot_up_leave_lcb_erp != NULL) {
                    comm->callbacks.on_boot_up_leave_lcb_erp(comm->callbacks.user_data, false);
                }
                if (comm->has_callbacks) {
                    lcb_proc_comm_lcb_error(&comm->callbacks);
                }
            }
        } else if (erp_mode == LCB_CMD_EUPS_RTN_CAN_REQUISTION) {
            /* LCB enter/leave ERP mode success (0x01) */
            if (comm->lcb_erp_state == LCB_LEAVE_ERP) {
                lcb_unblocked_lcb_error(comm);
                if (comm->has_callbacks && comm->callbacks.on_boot_up_leave_lcb_erp != NULL) {
                    comm->callbacks.on_boot_up_leave_lcb_erp(comm->callbacks.user_data, true);
                }
            }
        } else if (erp_mode == LCB_CMD_EUPS_RTN_CAN_NOT_REQUISTION) {
            /* LCB leave ERP mode fail (0xA1) */
            if (comm->lcb_erp_state == LCB_LEAVE_ERP) {
                lcb_unblocked_lcb_error(comm);
                if (comm->has_callbacks && comm->callbacks.on_boot_up_leave_lcb_erp != NULL) {
                    comm->callbacks.on_boot_up_leave_lcb_erp(comm->callbacks.user_data, false);
                }
                if (comm->has_callbacks) {
                    lcb_proc_comm_lcb_error(&comm->callbacks);
                }
            }
        }
        comm->is_boot_up_leave_erp = false;
    } else {
        /* Console suspend/resume leave ERP path */
        if (erp_mode == LCB_CMD_EUPS_RTN_NOT_SUPPORT) {
            /* LCB not support ERP mode (0x00) */
            if (comm->lcb_erp_state == LCB_ENTER_ERP) {
                if (comm->has_callbacks && comm->callbacks.on_enter_lcb_erp_mode != NULL) {
                    comm->callbacks.on_enter_lcb_erp_mode(comm->callbacks.user_data, false);
                }
            } else if (comm->lcb_erp_state == LCB_LEAVE_ERP) {
                lcb_unblocked_lcb_error(comm);
                if (comm->has_callbacks && comm->callbacks.on_leave_lcb_erp_mode != NULL) {
                    comm->callbacks.on_leave_lcb_erp_mode(comm->callbacks.user_data, true, false);
                }
            }
        } else if (erp_mode == LCB_CMD_EUPS_RTN_CAN_REQUISTION) {
            /* LCB enter/leave ERP mode success (0x01) */
            if (comm->lcb_erp_state == LCB_ENTER_ERP) {
                if (comm->has_callbacks && comm->callbacks.on_enter_lcb_erp_mode != NULL) {
                    comm->callbacks.on_enter_lcb_erp_mode(comm->callbacks.user_data, true);
                }
            } else if (comm->lcb_erp_state == LCB_LEAVE_ERP) {
                /* Leave ERP success - must retry getLcbStatus to confirm */
                lcb_comm_get_lcb_status(comm, LCB_QUEUE_MODE_ALWAYS_ADD);
            }
        } else if (erp_mode == LCB_CMD_EUPS_RTN_CAN_NOT_REQUISTION) {
            /* LCB enter/leave ERP mode fail (0xA1) */
            if (comm->lcb_erp_state == LCB_ENTER_ERP) {
                if (comm->has_callbacks && comm->callbacks.on_enter_lcb_erp_mode != NULL) {
                    comm->callbacks.on_enter_lcb_erp_mode(comm->callbacks.user_data, false);
                }
            } else if (comm->lcb_erp_state == LCB_LEAVE_ERP) {
                lcb_unblocked_lcb_error(comm);
                if (comm->has_callbacks && comm->callbacks.on_leave_lcb_erp_mode != NULL) {
                    comm->callbacks.on_leave_lcb_erp_mode(comm->callbacks.user_data, true, false);
                }
            }
        }
    }
}

/**
 * @brief Process 0x7A SetLCBCmdEUPsMode response from the IR Address board
 * (Java: procSetIrAddrCmdEUPsModeHandler()). Much simpler than the main-LCB
 * handler above — the IR board has no boot-up/blocked-flag machinery, just a
 * single outstanding enter-or-leave request disambiguated via
 * ir_addr_erp_state (set at TX time by lcb_comm_set_ir_addr_enter_erp_mode()/
 * _leave_erp_mode()).
 */
static void lcb_proc_set_ir_addr_cmd_eups_mode_handler(lcb_comm_t *comm, const lcb_packet_t *packet)
{
    if (comm == NULL || packet == NULL) return;

    uint8_t data_len;
    const uint8_t *data = lcb_packet_get_data(packet, &data_len);
    if (data_len != LCB_RECEIVE_DATA_SIZE_CMD_EUPS_MODE) return;

    uint8_t byte_rtn = data[0];
    bool is_enter = (comm->ir_addr_erp_state == LCB_CMD_EUPS_MODE_NOTIFY_ENTER_EUPS);
    bool success;

    /* Java's setIrAddrCmdEUPsModeHandler(byteRtn) forwards the raw byte
     * unconditionally, regardless of enter/leave context or whether it's a
     * recognized value — dispatch it first, before any early-return below. */
    if (comm->has_callbacks && comm->callbacks.on_ir_addr_erp_rtn_byte != NULL)
        comm->callbacks.on_ir_addr_erp_rtn_byte(comm->callbacks.user_data, byte_rtn);

    if (byte_rtn == LCB_CMD_EUPS_RTN_NOT_SUPPORT) {
        success = false;        /* IR board does not support ERP mode */
    } else if (byte_rtn == LCB_CMD_EUPS_RTN_CAN_REQUISTION) {
        success = true;         /* enter/leave ERP succeeded */
    } else if (byte_rtn == LCB_CMD_EUPS_RTN_CAN_NOT_REQUISTION) {
        success = false;        /* enter/leave ERP wait-fail */
    } else {
        return;                 /* unrecognized value — no callback, matches Java */
    }

    if (!comm->has_callbacks) return;
    if (is_enter) {
        if (comm->callbacks.on_ir_addr_enter_erp_mode != NULL)
            comm->callbacks.on_ir_addr_enter_erp_mode(comm->callbacks.user_data, success);
    } else {
        if (comm->callbacks.on_ir_addr_leave_erp_mode != NULL)
            comm->callbacks.on_ir_addr_leave_erp_mode(comm->callbacks.user_data, success);
    }
}

/* ============================================================================
 * Process Functions (Error Injection)
 * ============================================================================ */

void lcb_comm_proc_estop_machine_stop(lcb_comm_t *comm)
{
    if (comm == NULL || !comm->has_callbacks) return;
    lcb_proc_e_stop_error(&comm->callbacks);
    if (comm->callbacks.on_estop_machine_stop != NULL)
        comm->callbacks.on_estop_machine_stop(comm->callbacks.user_data);
}

void lcb_comm_proc_estop_mcb_error(lcb_comm_t *comm)
{
    if (comm == NULL || !comm->has_callbacks) return;
    lcb_proc_e_stop_mcb_error(&comm->callbacks);
    if (comm->callbacks.on_estop_mcb_error != NULL)
        comm->callbacks.on_estop_mcb_error(comm->callbacks.user_data);
}

void lcb_comm_proc_estop_unlock(lcb_comm_t *comm)
{
    if (comm == NULL || !comm->has_callbacks) return;
    lcb_proc_e_stop_unlock(&comm->callbacks);
    if (comm->callbacks.on_estop_unlock != NULL)
        comm->callbacks.on_estop_unlock(comm->callbacks.user_data);
}

void lcb_comm_proc_tm_no_rpm_error(lcb_comm_t *comm)
{
    if (comm == NULL || !comm->has_callbacks) return;
    lcb_proc_tm_no_rpm_error(&comm->callbacks);
    if (comm->callbacks.on_tm_no_rpm_error != NULL)
        comm->callbacks.on_tm_no_rpm_error(comm->callbacks.user_data);
}

void lcb_comm_proc_set_adv_logging_status(lcb_comm_t *comm)
{
    if (comm == NULL || !comm->has_callbacks) return;
    lcb_proc_set_adv_logging_status(&comm->callbacks);
}
