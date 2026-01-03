#include "growtopia/handlers.h"
#include "growtopia/security.h"
#include <h2o.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    h2o_handler_t super;
    h2o_handler_t *next;
} security_filter_t;

static int security_filter_on_req(h2o_handler_t *_self, h2o_req_t *req)
{
    security_filter_t *self = (security_filter_t *)_self;
    
    /* Get client address - h2o stores this in req->conn */
    struct sockaddr_storage addr;
    
    /* Use h2o's get_peername callback to get client address */
    if (req->conn->callbacks->get_peername != NULL) {
        socklen_t addr_len = req->conn->callbacks->get_peername(req->conn, (struct sockaddr *)&addr);
        
        if (addr_len > 0) {
            /* Check request rate limit */
            if (!security_check_request((struct sockaddr *)&addr)) {
                /* Rate limit exceeded, return 429 Too Many Requests */
                static h2o_generator_t generator = {NULL, NULL};
                req->res.status = 429;
                req->res.reason = "Too Many Requests";
                h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE, NULL, H2O_STRLIT("text/plain"));
                h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_RETRY_AFTER, NULL, H2O_STRLIT("60"));
                h2o_start_response(req, &generator);
                h2o_send(req, &(h2o_iovec_t){H2O_STRLIT("Rate limit exceeded. Please try again later.\n")}, 1, H2O_SEND_STATE_FINAL);
                return 0;
            }
        }
    }

    /* Pass to next handler */
    if (self->next)
        return self->next->on_req(self->next, req);
    return -1;
}

h2o_handler_t *register_security_filter(h2o_pathconf_t *pathconf)
{
    security_filter_t *filter = (security_filter_t *)h2o_create_handler(pathconf, sizeof(*filter));
    filter->super.on_req = security_filter_on_req;
    filter->next = NULL;
    return &filter->super;
}

int security_stats_handler(h2o_handler_t *self, h2o_req_t *req)
{
    static h2o_generator_t generator = {NULL, NULL};
    
    if (!h2o_memis(req->method.base, req->method.len, H2O_STRLIT("GET")))
        return -1;

    uint64_t blocked_requests = 0;
    uint64_t banned_ips = 0;
    security_get_stats(&blocked_requests, &banned_ips);

    char response[512];
    int len = snprintf(response, sizeof(response),
        "Security Statistics\n"
        "===================\n"
        "Blocked requests: %llu\n"
        "Banned IPs: %llu\n",
        (unsigned long long)blocked_requests,
        (unsigned long long)banned_ips);

    req->res.status = 200;
    req->res.reason = "OK";
    h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE, NULL, H2O_STRLIT("text/plain"));
    h2o_start_response(req, &generator);
    h2o_iovec_t body = h2o_strdup(&req->pool, response, len);
    h2o_send(req, &body, 1, H2O_SEND_STATE_FINAL);
    return 0;
}

h2o_pathconf_t *register_handler(h2o_hostconf_t *hostconf, const char *path, int (*on_req)(h2o_handler_t *, h2o_req_t *))
{
    h2o_pathconf_t *pathconf = h2o_config_register_path(hostconf, path, 0);
    h2o_handler_t *handler = h2o_create_handler(pathconf, sizeof(*handler));
    handler->on_req = on_req;
    return pathconf;
}

int chunked_test(h2o_handler_t *self, h2o_req_t *req)
{
    static h2o_generator_t generator = {NULL, NULL};

    if (!h2o_memis(req->method.base, req->method.len, H2O_STRLIT("GET")))
        return -1;

    h2o_iovec_t body = h2o_strdup(&req->pool, "hello world\n", SIZE_MAX);
    req->res.status = 200;
    req->res.reason = "OK";
    h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE, NULL, H2O_STRLIT("text/plain"));
    h2o_start_response(req, &generator);
    h2o_send(req, &body, 1, 1);

    return 0;
}

int reproxy_test(h2o_handler_t *self, h2o_req_t *req)
{
    if (!h2o_memis(req->method.base, req->method.len, H2O_STRLIT("GET")))
        return -1;

    req->res.status = 200;
    req->res.reason = "OK";
    h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_X_REPROXY_URL, NULL, H2O_STRLIT("http://www.ietf.org/"));
    h2o_send_inline(req, H2O_STRLIT("you should never see this!\n"));

    return 0;
}

int post_test(h2o_handler_t *self, h2o_req_t *req)
{
    if (h2o_memis(req->method.base, req->method.len, H2O_STRLIT("POST")) &&
        h2o_memis(req->path_normalized.base, req->path_normalized.len, H2O_STRLIT("/post-test/"))) {
        static h2o_generator_t generator = {NULL, NULL};
        req->res.status = 200;
        req->res.reason = "OK";
        h2o_add_header(&req->pool, &req->res.headers, H2O_TOKEN_CONTENT_TYPE, NULL, H2O_STRLIT("text/plain; charset=utf-8"));
        h2o_start_response(req, &generator);
        h2o_send(req, &req->entity, 1, 1);
        return 0;
    }

    return -1;
}
