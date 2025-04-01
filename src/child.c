
#include "child.h"
#include "data_types.h"
#include "http.h"
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
            // printf("[DEBUG] Library has not been modified since last load.\n");
            return;
        }
        needs_reload = 1;
    }
    else
    {
        // printf("[DEBUG] No handle yet. Loading library.\n");
        needs_reload = 1;
    }

    if(needs_reload)
    {
        // printf("[DEBUG] Reloading library: %s\n", ctx->lib_info.path);

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

        // printf("[DEBUG] Library %s reloaded successfully.\n", ctx->lib_info.path);
    }
}

int handle_request(struct context *ctx, int client_fd)
{
    union
    {
        void *sym_ptr;
        void *(*func_ptr)(struct thread_state *);
    } cast_union;

    struct thread_state ts;

    if(client_fd <= 0)
    {
        fprintf(stderr, "Invalid client_fd: %d\n", client_fd);
        return -1;
    }

    // Assign initial values after declaration
    ts.client_fd           = client_fd;
    ts.request_line_string = NULL;
    ts.err                 = 0;

    cast_union.sym_ptr = dlsym(ctx->lib_info.handle, "http_respond");
    if(!cast_union.sym_ptr)
    {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        close(client_fd);
        return -1;
    }

    parse_request(&ts);
    cast_union.func_ptr(&ts);
    cleanup_headers(&ts);

    return 0;
}
