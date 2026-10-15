/**
 * @file lcb_logging.h
 * @brief Centralized logging interface for LCB protocol modules
 *
 * Provides runtime-configurable logging with three levels (INFO, DEBUG, ERROR).
 * Each level can be enabled/disabled independently at runtime with minimal overhead.
 *
 * Output goes to stderr as "[I|D|E] <tag>: <message>".
 *
 * The fmt argument of LCB_LOGx must be a string literal.
 */

#ifndef LCB_LOGGING_H
#define LCB_LOGGING_H

#include <stdbool.h>
#include <stdio.h>

#define LCB_LOG_PRINT_(level, tag, fmt, ...) \
    fprintf(stderr, "[" level "] %s: " fmt "\n", tag, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

/* Runtime enable/disable flags for each log level */
extern bool lcb_log_info_enabled;
extern bool lcb_log_debug_enabled;
extern bool lcb_log_error_enabled;

/**
 * @brief Initialize logging system with default levels
 */
void lcb_log_init(void);

/**
 * @brief Runtime control for INFO level logging
 * @param enabled true to enable INFO logs, false to disable
 */
void lcb_log_set_info(bool enabled);

/**
 * @brief Runtime control for DEBUG level logging
 * @param enabled true to enable DEBUG logs, false to disable
 */
void lcb_log_set_debug(bool enabled);

/**
 * @brief Runtime control for ERROR level logging
 * @param enabled true to enable ERROR logs, false to disable
 */
void lcb_log_set_error(bool enabled);

#ifdef __cplusplus
}
#endif

/**
 * @brief INFO level logging macro (conditional)
 * Only logs if lcb_log_info_enabled is true
 */
#define LCB_LOGI(tag, fmt, ...) \
    do { if (lcb_log_info_enabled) LCB_LOG_PRINT_("I", tag, fmt, ##__VA_ARGS__); } while(0)

/**
 * @brief DEBUG level logging macro (conditional)
 * Only logs if lcb_log_debug_enabled is true
 */
#define LCB_LOGD(tag, fmt, ...) \
    do { if (lcb_log_debug_enabled) LCB_LOG_PRINT_("D", tag, fmt, ##__VA_ARGS__); } while(0)

/**
 * @brief ERROR level logging macro (conditional)
 * Only logs if lcb_log_error_enabled is true
 */
#define LCB_LOGE(tag, fmt, ...) \
    do { if (lcb_log_error_enabled) LCB_LOG_PRINT_("E", tag, fmt, ##__VA_ARGS__); } while(0)

#endif /* LCB_LOGGING_H */
