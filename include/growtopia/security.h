#pragma once

#include "growtopia/config/server.h"
#include <h2o.h>
#include <netinet/in.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * IP tracking entry
 */
typedef struct ip_tracker_entry {
    struct sockaddr_storage addr;        // IP address
    uint32_t connection_count;           // Current active connections
    uint32_t request_count;              // Requests in current window
    time_t window_start;                 // Start of current rate limit window
    time_t ban_until;                    // Timestamp when ban expires (0 if not banned)
    uint32_t strike_count;               // Number of violations
    struct ip_tracker_entry *next;       // Next entry in linked list
} ip_tracker_entry_t;

/**
 * Security context
 */
typedef struct {
    security_config_t config;
    ip_tracker_entry_t **ip_table;       // Hash table of IP entries
    uint32_t table_size;                 // Size of hash table
    h2o_timer_t cleanup_timer;           // Timer for periodic cleanup
    h2o_context_t *h2o_ctx;              // h2o context for timer
    uint64_t total_blocked_requests;     // Statistics counter
    uint64_t total_banned_ips;           // Statistics counter
} security_context_t;

/**
 * Initialize the security module with configuration
 * @param ctx h2o context for timer integration
 * @param config Security configuration
 * @return 0 on success, negative on error
 */
int security_init(h2o_context_t *ctx, const security_config_t *config);

/**
 * Cleanup and free security module resources
 */
void security_cleanup(void);

/**
 * Check if a connection from an IP should be allowed
 * @param addr Socket address of the client
 * @return 1 if allowed, 0 if blocked
 */
int security_check_connection(const struct sockaddr *addr);

/**
 * Register a new connection from an IP
 * @param addr Socket address of the client
 * @return 0 on success, -1 if connection should be rejected
 */
int security_register_connection(const struct sockaddr *addr);

/**
 * Unregister a connection from an IP (called on disconnect)
 * @param addr Socket address of the client
 */
void security_unregister_connection(const struct sockaddr *addr);

/**
 * Check if a request from an IP should be allowed (rate limiting)
 * @param addr Socket address of the client
 * @return 1 if allowed, 0 if rate limit exceeded
 */
int security_check_request(const struct sockaddr *addr);

/**
 * Manually ban an IP address
 * @param addr Socket address to ban
 * @param duration_seconds Duration of ban in seconds (0 for permanent)
 * @return 0 on success, negative on error
 */
int security_ban_ip(const struct sockaddr *addr, uint32_t duration_seconds);

/**
 * Manually unban an IP address
 * @param addr Socket address to unban
 * @return 0 on success, negative on error
 */
int security_unban_ip(const struct sockaddr *addr);

/**
 * Check if an IP is currently banned
 * @param addr Socket address to check
 * @return 1 if banned, 0 if not banned
 */
int security_is_banned(const struct sockaddr *addr);

/**
 * Add an IP to whitelist (exempt from rate limiting)
 * @param addr Socket address to whitelist
 * @return 0 on success, negative on error
 */
int security_whitelist_ip(const struct sockaddr *addr);

/**
 * Get security statistics
 * @param blocked_requests Output: total blocked requests
 * @param banned_ips Output: total IPs currently banned
 */
void security_get_stats(uint64_t *blocked_requests, uint64_t *banned_ips);

#ifdef __cplusplus
}
#endif
