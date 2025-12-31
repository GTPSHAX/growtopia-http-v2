#pragma once

#include <h2o.h>
#ifdef H2O_USE_LIBUV
#include <uv.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the listener module with the h2o context and accept context.
 * Must be called before listener_start().
 */
int listener_init(h2o_context_t *ctx, h2o_accept_ctx_t *accept_ctx);

/**
 * Create and start the listener (bind/listen).
 * Returns 0 on success, negative or errno-style value on failure.
 */
int listener_start(void);

/**
 * Stop/cleanup the listener (best-effort).
 */
void listener_stop(void);

#ifdef __cplusplus
}
#endif
