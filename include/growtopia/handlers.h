#pragma once

#include <h2o.h>

#ifdef __cplusplus
extern "C" {
#endif

h2o_pathconf_t *register_handler(h2o_hostconf_t *hostconf, const char *path, int (*on_req)(h2o_handler_t *, h2o_req_t *));
int chunked_test(h2o_handler_t *self, h2o_req_t *req);
int reproxy_test(h2o_handler_t *self, h2o_req_t *req);
int post_test(h2o_handler_t *self, h2o_req_t *req);

#ifdef __cplusplus
}
#endif
