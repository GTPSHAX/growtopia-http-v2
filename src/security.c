#include "growtopia/security.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_TABLE_SIZE 1024
#define CLEANUP_INTERVAL_MS 60000  // Run cleanup every 60 seconds
#define ENTRY_TIMEOUT_SECONDS 300  // Remove inactive entries after 5 minutes

static security_context_t *g_security_ctx = NULL;
static pthread_mutex_t g_security_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static uint32_t hash_sockaddr(const struct sockaddr *addr);
static ip_tracker_entry_t *find_or_create_entry(const struct sockaddr *addr);
static void cleanup_expired_entries(h2o_timer_t *timer);
static int sockaddr_equals(const struct sockaddr_storage *a, const struct sockaddr *b);
static void remove_entry(const struct sockaddr *addr);

int security_init(h2o_context_t *ctx, const security_config_t *config)
{
    if (g_security_ctx != NULL) {
        fprintf(stderr, "Security module already initialized\n");
        return -1;
    }

    g_security_ctx = calloc(1, sizeof(security_context_t));
    if (g_security_ctx == NULL) {
        fprintf(stderr, "Failed to allocate security context\n");
        return -1;
    }

    g_security_ctx->config = config ? *config : security_get_default_config();
    g_security_ctx->table_size = DEFAULT_TABLE_SIZE;
    g_security_ctx->h2o_ctx = ctx;
    g_security_ctx->total_blocked_requests = 0;
    g_security_ctx->total_banned_ips = 0;

    g_security_ctx->ip_table = calloc(g_security_ctx->table_size, sizeof(ip_tracker_entry_t *));
    if (g_security_ctx->ip_table == NULL) {
        fprintf(stderr, "Failed to allocate IP tracking table\n");
        free(g_security_ctx);
        g_security_ctx = NULL;
        return -1;
    }

    // Setup periodic cleanup timer
    h2o_timer_init(&g_security_ctx->cleanup_timer, cleanup_expired_entries);
    h2o_timer_link(ctx->loop, CLEANUP_INTERVAL_MS, &g_security_ctx->cleanup_timer);

    printf("Security module initialized:\n");
    printf("  Max connections per IP: %u\n", g_security_ctx->config.max_connections_per_ip);
    printf("  Max requests per second: %u\n", g_security_ctx->config.max_requests_per_second);
    printf("  Ban duration: %u seconds\n", g_security_ctx->config.ban_duration_seconds);
    printf("  Rate limiting: %s\n", g_security_ctx->config.enable_rate_limiting ? "enabled" : "disabled");
    printf("  Auto-ban: %s\n", g_security_ctx->config.enable_auto_ban ? "enabled" : "disabled");

    return 0;
}

void security_cleanup(void)
{
    if (g_security_ctx == NULL)
        return;

    h2o_timer_unlink(&g_security_ctx->cleanup_timer);

    // Free all entries
    for (uint32_t i = 0; i < g_security_ctx->table_size; i++) {
        ip_tracker_entry_t *entry = g_security_ctx->ip_table[i];
        while (entry != NULL) {
            ip_tracker_entry_t *next = entry->next;
            free(entry);
            entry = next;
        }
    }

    free(g_security_ctx->ip_table);
    free(g_security_ctx);
    g_security_ctx = NULL;
}

static uint32_t hash_sockaddr(const struct sockaddr *addr)
{
    uint32_t hash = 0;

    if (addr->sa_family == AF_INET) {
        struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
        hash = addr_in->sin_addr.s_addr;
    } else if (addr->sa_family == AF_INET6) {
        struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)addr;
        uint32_t *words = (uint32_t *)&addr_in6->sin6_addr;
        hash = words[0] ^ words[1] ^ words[2] ^ words[3];
    }

    return hash % g_security_ctx->table_size;
}

static int sockaddr_equals(const struct sockaddr_storage *a, const struct sockaddr *b)
{
    if (a->ss_family != b->sa_family)
        return 0;

    if (a->ss_family == AF_INET) {
        struct sockaddr_in *a_in = (struct sockaddr_in *)a;
        struct sockaddr_in *b_in = (struct sockaddr_in *)b;
        return a_in->sin_addr.s_addr == b_in->sin_addr.s_addr;
    } else if (a->ss_family == AF_INET6) {
        struct sockaddr_in6 *a_in6 = (struct sockaddr_in6 *)a;
        struct sockaddr_in6 *b_in6 = (struct sockaddr_in6 *)b;
        return memcmp(&a_in6->sin6_addr, &b_in6->sin6_addr, sizeof(struct in6_addr)) == 0;
    }

    return 0;
}

static ip_tracker_entry_t *find_or_create_entry(const struct sockaddr *addr)
{
    uint32_t hash = hash_sockaddr(addr);
    ip_tracker_entry_t *entry = g_security_ctx->ip_table[hash];

    // Search for existing entry
    while (entry != NULL) {
        if (sockaddr_equals(&entry->addr, addr))
            return entry;
        entry = entry->next;
    }

    // Create new entry
    entry = calloc(1, sizeof(ip_tracker_entry_t));
    if (entry == NULL)
        return NULL;

    memcpy(&entry->addr, addr, sizeof(struct sockaddr_storage));
    entry->connection_count = 0;
    entry->request_count = 0;
    entry->window_start = time(NULL);
    entry->ban_until = 0;
    entry->strike_count = 0;

    // Insert at head of list
    entry->next = g_security_ctx->ip_table[hash];
    g_security_ctx->ip_table[hash] = entry;

    return entry;
}

static void remove_entry(const struct sockaddr *addr)
{
    uint32_t hash = hash_sockaddr(addr);
    ip_tracker_entry_t **entry_ptr = &g_security_ctx->ip_table[hash];

    while (*entry_ptr != NULL) {
        if (sockaddr_equals(&(*entry_ptr)->addr, addr)) {
            ip_tracker_entry_t *to_remove = *entry_ptr;
            *entry_ptr = to_remove->next;
            free(to_remove);
            return;
        }
        entry_ptr = &(*entry_ptr)->next;
    }
}

int security_check_connection(const struct sockaddr *addr)
{
    if (g_security_ctx == NULL)
        return 1;  // Allow if security not initialized

    pthread_mutex_lock(&g_security_mutex);

    ip_tracker_entry_t *entry = find_or_create_entry(addr);
    if (entry == NULL) {
        pthread_mutex_unlock(&g_security_mutex);
        return 1;  // Allow on allocation failure
    }

    time_t now = time(NULL);

    // Check if IP is banned
    if (entry->ban_until > 0 && now < entry->ban_until) {
        g_security_ctx->total_blocked_requests++;
        pthread_mutex_unlock(&g_security_mutex);
        return 0;  // Blocked
    }

    // Clear ban if expired
    if (entry->ban_until > 0 && now >= entry->ban_until) {
        entry->ban_until = 0;
        entry->strike_count = 0;
    }

    // Check connection limit
    if (g_security_ctx->config.enable_connection_limit &&
        entry->connection_count >= g_security_ctx->config.max_connections_per_ip) {
        entry->strike_count++;
        
        // Auto-ban if threshold exceeded
        if (g_security_ctx->config.enable_auto_ban &&
            entry->strike_count >= g_security_ctx->config.strike_threshold) {
            entry->ban_until = now + g_security_ctx->config.ban_duration_seconds;
            g_security_ctx->total_banned_ips++;
            
            char ip_str[INET6_ADDRSTRLEN];
            if (addr->sa_family == AF_INET) {
                inet_ntop(AF_INET, &((struct sockaddr_in *)addr)->sin_addr, ip_str, sizeof(ip_str));
            } else {
                inet_ntop(AF_INET6, &((struct sockaddr_in6 *)addr)->sin6_addr, ip_str, sizeof(ip_str));
            }
            printf("Auto-banned IP %s for %u seconds (connection limit exceeded)\n",
                   ip_str, g_security_ctx->config.ban_duration_seconds);
        }
        
        g_security_ctx->total_blocked_requests++;
        pthread_mutex_unlock(&g_security_mutex);
        return 0;  // Blocked
    }

    pthread_mutex_unlock(&g_security_mutex);
    return 1;  // Allowed
}

int security_register_connection(const struct sockaddr *addr)
{
    if (g_security_ctx == NULL)
        return 0;

    pthread_mutex_lock(&g_security_mutex);

    ip_tracker_entry_t *entry = find_or_create_entry(addr);
    if (entry == NULL) {
        pthread_mutex_unlock(&g_security_mutex);
        return -1;
    }

    entry->connection_count++;
    pthread_mutex_unlock(&g_security_mutex);
    return 0;
}

void security_unregister_connection(const struct sockaddr *addr)
{
    if (g_security_ctx == NULL)
        return;

    pthread_mutex_lock(&g_security_mutex);

    ip_tracker_entry_t *entry = find_or_create_entry(addr);
    if (entry == NULL) {
        pthread_mutex_unlock(&g_security_mutex);
        return;
    }

    if (entry->connection_count > 0)
        entry->connection_count--;
    
    pthread_mutex_unlock(&g_security_mutex);
}

int security_check_request(const struct sockaddr *addr)
{
    if (g_security_ctx == NULL || !g_security_ctx->config.enable_rate_limiting)
        return 1;  // Allow if rate limiting disabled

    ip_tracker_entry_t *entry = find_or_create_entry(addr);
    if (entry == NULL)
        return 1;  // Allow on allocation failure

    time_t now = time(NULL);

    // Check if IP is banned
    if (entry->ban_until > 0 && now < entry->ban_until) {
        g_security_ctx->total_blocked_requests++;
        return 0;  // Blocked
    }

    // Reset window if expired
    if (now - entry->window_start >= g_security_ctx->config.request_window_seconds) {
        entry->window_start = now;
        entry->request_count = 0;
    }

    entry->request_count++;

    // Check rate limit
    if (entry->request_count > g_security_ctx->config.max_requests_per_second) {
        entry->strike_count++;
        
        // Auto-ban if threshold exceeded
        if (g_security_ctx->config.enable_auto_ban &&
            entry->strike_count >= g_security_ctx->config.strike_threshold) {
            entry->ban_until = now + g_security_ctx->config.ban_duration_seconds;
            g_security_ctx->total_banned_ips++;
            
            char ip_str[INET6_ADDRSTRLEN];
            if (addr->sa_family == AF_INET) {
                inet_ntop(AF_INET, &((struct sockaddr_in *)addr)->sin_addr, ip_str, sizeof(ip_str));
            } else {
                inet_ntop(AF_INET6, &((struct sockaddr_in6 *)addr)->sin6_addr, ip_str, sizeof(ip_str));
            }
            printf("Auto-banned IP %s for %u seconds (rate limit exceeded)\n",
                   ip_str, g_security_ctx->config.ban_duration_seconds);
        }
        
        g_security_ctx->total_blocked_requests++;
        return 0;  // Blocked
    }

    return 1;  // Allowed
}

int security_ban_ip(const struct sockaddr *addr, uint32_t duration_seconds)
{
    if (g_security_ctx == NULL)
        return -1;

    ip_tracker_entry_t *entry = find_or_create_entry(addr);
    if (entry == NULL)
        return -1;

    time_t now = time(NULL);
    entry->ban_until = duration_seconds > 0 ? now + duration_seconds : UINT32_MAX;
    g_security_ctx->total_banned_ips++;

    char ip_str[INET6_ADDRSTRLEN];
    if (addr->sa_family == AF_INET) {
        inet_ntop(AF_INET, &((struct sockaddr_in *)addr)->sin_addr, ip_str, sizeof(ip_str));
    } else {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)addr)->sin6_addr, ip_str, sizeof(ip_str));
    }
    printf("Manually banned IP %s for %u seconds\n", ip_str, duration_seconds);

    return 0;
}

int security_unban_ip(const struct sockaddr *addr)
{
    if (g_security_ctx == NULL)
        return -1;

    ip_tracker_entry_t *entry = find_or_create_entry(addr);
    if (entry == NULL)
        return -1;

    entry->ban_until = 0;
    entry->strike_count = 0;

    return 0;
}

int security_is_banned(const struct sockaddr *addr)
{
    if (g_security_ctx == NULL)
        return 0;

    ip_tracker_entry_t *entry = find_or_create_entry(addr);
    if (entry == NULL)
        return 0;

    time_t now = time(NULL);
    return (entry->ban_until > 0 && now < entry->ban_until);
}

int security_whitelist_ip(const struct sockaddr *addr)
{
    if (g_security_ctx == NULL)
        return -1;

    ip_tracker_entry_t *entry = find_or_create_entry(addr);
    if (entry == NULL)
        return -1;

    // Set very high limits for whitelisted IPs
    entry->ban_until = 0;
    entry->strike_count = 0;
    // Could add a flag to mark as whitelisted for complete exemption

    return 0;
}

void security_get_stats(uint64_t *blocked_requests, uint64_t *banned_ips)
{
    if (g_security_ctx == NULL) {
        if (blocked_requests) *blocked_requests = 0;
        if (banned_ips) *banned_ips = 0;
        return;
    }

    if (blocked_requests)
        *blocked_requests = g_security_ctx->total_blocked_requests;
    if (banned_ips)
        *banned_ips = g_security_ctx->total_banned_ips;
}

static void cleanup_expired_entries(h2o_timer_t *timer)
{
    if (g_security_ctx == NULL)
        return;

    pthread_mutex_lock(&g_security_mutex);

    time_t now = time(NULL);
    uint32_t removed_count = 0;

    for (uint32_t i = 0; i < g_security_ctx->table_size; i++) {
        ip_tracker_entry_t **entry_ptr = &g_security_ctx->ip_table[i];

        while (*entry_ptr != NULL) {
            ip_tracker_entry_t *entry = *entry_ptr;

            // Remove entries that are inactive and not banned
            int should_remove = 0;
            if (entry->ban_until == 0 && entry->connection_count == 0) {
                if (now - entry->window_start > ENTRY_TIMEOUT_SECONDS) {
                    should_remove = 1;
                }
            }

            if (should_remove) {
                *entry_ptr = entry->next;
                free(entry);
                removed_count++;
            } else {
                entry_ptr = &entry->next;
            }
        }
    }

    if (removed_count > 0) {
        printf("Security cleanup: removed %u inactive IP entries\n", removed_count);
    }

    pthread_mutex_unlock(&g_security_mutex);

    // Re-arm timer
    h2o_timer_link(g_security_ctx->h2o_ctx->loop, CLEANUP_INTERVAL_MS, timer);
}
