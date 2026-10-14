/**
 * @file lcb_logging.c
 * @brief Implementation of LCB logging control
 */

#include "lcb_logging.h"

/* Default: all logging enabled */
bool lcb_log_info_enabled = true;
bool lcb_log_debug_enabled = true;
bool lcb_log_error_enabled = true;

void lcb_log_init(void)
{
    /* Set default log levels - can be overridden later */
    lcb_log_info_enabled = true;
    lcb_log_debug_enabled = true;  /* Change to false for production */
    lcb_log_error_enabled = true;
}

void lcb_log_set_info(bool enabled)
{
    lcb_log_info_enabled = enabled;
}

void lcb_log_set_debug(bool enabled)
{
    lcb_log_debug_enabled = enabled;
}

void lcb_log_set_error(bool enabled)
{
    lcb_log_error_enabled = enabled;
}
