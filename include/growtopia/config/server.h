#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Security Configuration
 */
typedef struct {
    uint32_t max_connections_per_ip;     // Maximum concurrent connections per IP
    uint32_t max_requests_per_second;    // Maximum requests per second per IP
    uint32_t ban_duration_seconds;       // Duration to ban an IP after threshold exceeded
    uint32_t request_window_seconds;     // Time window for request rate calculation
    uint32_t strike_threshold;           // Number of violations before auto-ban
    uint8_t enable_rate_limiting;        // Enable/disable rate limiting
    uint8_t enable_auto_ban;             // Enable/disable automatic IP banning
    uint8_t enable_connection_limit;     // Enable/disable connection limiting
} security_config_t;

/**
 * Server Configuration
 */
typedef struct {
    const char *bind_address;            // Server bind address (e.g., "0.0.0.0")
    uint16_t port;                       // Server port (e.g., 8000)
    uint32_t max_connections;            // Maximum total connections
    uint32_t timeout_seconds;            // Connection timeout
    security_config_t security;          // Security configuration
} server_config_t;

/**
 * Get default server configuration
 */
static inline server_config_t server_get_default_config(void) {
    server_config_t config = {
        .bind_address = "0.0.0.0",
        .port = 8000,
        .max_connections = 10000,
        .timeout_seconds = 30,
        .security = {
            .max_connections_per_ip = 100,
            .max_requests_per_second = 100,
            .ban_duration_seconds = 300,
            .request_window_seconds = 1,
            .strike_threshold = 3,
            .enable_rate_limiting = 0,
            .enable_auto_ban = 0,
            .enable_connection_limit = 0
        }
    };
    return config;
}

/**
 * Get default security configuration
 */
static inline security_config_t security_get_default_config(void) {
    security_config_t config = {
        .max_connections_per_ip = 100,
        .max_requests_per_second = 100,
        .ban_duration_seconds = 300,
        .request_window_seconds = 1,
        .strike_threshold = 3,
        .enable_rate_limiting = 0,
        .enable_auto_ban = 0,
        .enable_connection_limit = 0
    };
    return config;
}

#ifdef __cplusplus
}
#endif
