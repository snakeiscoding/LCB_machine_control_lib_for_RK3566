/**
 * @file hw_logging.h
 * @brief Logging macros for Layer_5 hardware modules (private to src/hardware)
 *
 * The hardware layer sits below protocol/Comm, so it must not include
 * lcb_logging.h (that would be an upward dependency). These macros write to
 * stderr in the same "[I|D|E] <tag>: <message>" format, and are not affected
 * by lcb_log_set_info/debug/error().
 *
 * The fmt argument must be a string literal.
 */

#ifndef HW_LOGGING_H
#define HW_LOGGING_H

#include <stdio.h>

#define HW_LOG_PRINT_(level, tag, fmt, ...) \
    fprintf(stderr, "[" level "] %s: " fmt "\n", tag, ##__VA_ARGS__)

#define HW_LOGI(tag, fmt, ...) HW_LOG_PRINT_("I", tag, fmt, ##__VA_ARGS__)
#define HW_LOGD(tag, fmt, ...) HW_LOG_PRINT_("D", tag, fmt, ##__VA_ARGS__)
#define HW_LOGE(tag, fmt, ...) HW_LOG_PRINT_("E", tag, fmt, ##__VA_ARGS__)

#endif /* HW_LOGGING_H */
