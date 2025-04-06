
#include "child.h"
#include "data_types.h"
#include "network.h"
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void check_library(struct context *ctx)
{
    struct stat file_stat;
    int         needs_reload = 0;
    printf("[check_library] start\n");

    if(!ctx || !ctx->lib_info.path)
    {
        fprintf(stderr, "Library path is NULL\n");
        return;
    }

    // printf("[DEBUG] Library path: %s\n", ctx->lib_info.path);
    // printf("[DEBUG] Current handle: %p\n", ctx->lib_info.handle);

    if(stat(ctx->lib_info.path, &file_stat) == -1)
    {
        perror("Error checking library");
        return;
    }

    if(ctx->lib_info.handle)
    {
        if(file_stat.st_mtime <= ctx->lib_info.last_mod)
        {
            printf("[check_library DEBUG] Library has not been modified since last load.\n");
            return;
        }
        needs_reload = 1;
    }
    else
    {
        printf("[check_library DEBUG] No handle yet. Loading library.\n");
        needs_reload = 1;
    }

    if(needs_reload)
    {
        printf("[check_library DEBUG] Reloading library: %s\n", ctx->lib_info.path);

        if(ctx->lib_info.handle)
        {
            // printf("[DEBUG] dlclose() on handle %p\n", ctx->lib_info.handle);
            if(dlclose(ctx->lib_info.handle) != 0)
            {
                fprintf(stderr, "dlclose failed: %s\n", dlerror());
            }
            ctx->lib_info.handle = NULL;
        }

        ctx->lib_info.handle = dlopen(ctx->lib_info.path, RTLD_LAZY);
        // printf("[DEBUG] dlopen() returned: %p\n", ctx->lib_info.handle);

        if(!ctx->lib_info.handle)
        {
            fprintf(stderr, "Failed to load library: %s\n", dlerror());
            return;
        }

        if(stat(ctx->lib_info.path, &file_stat) == 0)
        {
            ctx->lib_info.last_mod = file_stat.st_mtime;
        }

        printf("[check_library DEBUG] Library %s reloaded successfully.\n", ctx->lib_info.path);
    }
}

int handle_request(struct context *ctx, int client_fd)
{
    union
    {
        void *sym_ptr;
        void *(*func_ptr)(struct thread_state *);
    } http_respond_union;

    union
    {
        void *sym_ptr;
        struct thread_state *(*func_ptr)(struct thread_state *);
    } parse_request_union;

    union
    {
        void *sym_ptr;
        void (*func_ptr)(struct thread_state *);
    } cleanup_headers_union;

    struct thread_state ts;
    memset(&ts, 0, sizeof(struct thread_state));    // ✅ Clear all fields to 0
    printf("[handle request DEBUG] start\n");

    ts.client_fd           = client_fd;
    ts.request_line_string = NULL;
    ts.err                 = 0;

    // dlsym for parse_request
    parse_request_union.sym_ptr = dlsym(ctx->lib_info.handle, "parse_request");
    if(!parse_request_union.sym_ptr)
    {
        fprintf(stderr, "dlsym parse_request failed: %s\n", dlerror());
        return -1;
    }

    // dlsym for cleanup_headers
    cleanup_headers_union.sym_ptr = dlsym(ctx->lib_info.handle, "cleanup_headers");
    if(!cleanup_headers_union.sym_ptr)
    {
        fprintf(stderr, "dlsym cleanup_headers failed: %s\n", dlerror());
        return -1;
    }
    // dlsym for http_respond
    http_respond_union.sym_ptr = dlsym(ctx->lib_info.handle, "http_respond");
    if(!http_respond_union.sym_ptr)
    {
        fprintf(stderr, "dlsym http_respond failed: %s\n", dlerror());
        return -1;
    }

    // Parse request dynamically
    if(parse_request_union.func_ptr(&ts) == NULL)
    {
        fprintf(stderr, "[handle request DEBUG] parse_request failed\n");
        return 0;
    }

    if(!ts.resource_string)
    {
        const char *resp = "HTTP/1.0 400 Bad Request\r\n\r\nMissing URI.\n";
        write(client_fd, resp, strlen(resp));
        return -1;
    }

    // Call handler
    http_respond_union.func_ptr(&ts);

    // Cleanup dynamically
    cleanup_headers_union.func_ptr(&ts);

    return 0;
}
