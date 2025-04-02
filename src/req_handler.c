//
// Created by blaise-klein on 3/6/25.
//

#include "req_handler.h"
#include "network.h"
#include <child.h>
#include <server.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int setup_children(struct context *ctx)
{
    for(int i = 0; i < CHILD_COUNT; ++i)
    {
        fork_child(ctx, i);
    }
    return 0;
}

_Noreturn void startup_child(struct context *ctx)
{
    ctx->lib_info.path = strdup("./libhandler.so");

    while(1)
    {
        // 1. Receive file descriptor for new client
        int client_fd = recv_fd(ctx->network.domain_fd[1]);
        if(client_fd < 0)
        {
            perror("recv_fd failed");
            sleep(1);
            continue;    // Skip this iteration
        }

        // 2. Check if the shared library was updated
        check_library(ctx);
        if(ctx->lib_info.handle == NULL)
        {
            fprintf(stderr, "No valid library loaded. Skipping request.\n");
            close(client_fd);
            continue;
        }

        // 3. Handle the HTTP request using the shared library
        if(handle_request(ctx, client_fd) < 0)
        {
            fprintf(stderr, "Failed to handle request.\n");
            // Still close client_fd in handle_request()
        }
        // 4. Notify parent that this fd can be used again
        send_fd(ctx->network.domain_fd[1], client_fd);

        // Client FD is closed inside handle_request()
    }
}
